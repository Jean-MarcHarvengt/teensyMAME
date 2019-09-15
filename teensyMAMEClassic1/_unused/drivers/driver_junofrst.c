/***************************************************************************

Juno First :  memory map same as tutankham with some address changes
Chris Hardy (chrish@kcbbs.gen.nz)

Thanks to Rob Jarret for the original Tutankham memory map on which both the
Juno First emu and the mame driver is based on.

		Juno First memory map by Chris Hardy

Read/Write memory

$0000-$7FFF = Screen RAM (only written to)
$8000-$800f = Palette RAM. BBGGGRRR (D7->D0)
$8100-$8FFF = Work RAM

Write memory

$8030	- interrupt control register D0 = interupts on or off
$8031	- unknown
$8032	- unknown
$8033	- unknown
$8034	- flip screen x
$8035	- flip screen y

$8040	- Sound CPU req/ack data
$8050	- Sound CPU command data
$8060	- Banked memory page select.
$8070/1 - Blitter source data word
$8072/3 - Blitter destination word. Write to $8073 triggers a blit

Read memory

$8010	- Dipswitch 2
$801c	- Watchdog
$8020	- Start/Credit IO
				D2 = Credit 1
				D3 = Start 1
				D4 = Start 2
$8024	- P1 IO
				D0 = left
				D1 = right
				D2 = up
				D3 = down
				D4 = fire 2
				D5 = fire 1

$8028	- P2 IO - same as P1 IO
$802c	- Dipswitch 1



$9000->$9FFF Banked Memory - see below
$A000->$BFFF "juno\\JFA_B9.BIN",
$C000->$DFFF "juno\\JFB_B10.BIN",
$E000->$FFFF "juno\\JFC_A10.BIN",

Banked memory - Paged into $9000->$9FFF..

NOTE - In Tutankhm this only contains graphics, in Juno First it also contains code. (which
		generally sets up the blitter)

	"juno\\JFC1_A4.BIN",	$0000->$1FFF
	"juno\\JFC2_A5.BIN",	$2000->$3FFF
	"juno\\JFC3_A6.BIN",	$4000->$5FFF
	"juno\\JFC4_A7.BIN",	$6000->$7FFF
	"juno\\JFC5_A8.BIN",	$8000->$9FFF
	"juno\\JFC6_A9.BIN",	$A000->$bFFF

Blitter source graphics

	"juno\\JFS3_C7.BIN",	$C000->$DFFF
	"juno\\JFS4_D7.BIN",	$E000->$FFFF
	"juno\\JFS5_E7.BIN",	$10000->$11FFF


***************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809/m6809.h"
#include "I8039/I8039.h"


extern unsigned char *tutankhm_scrollx;

void tutankhm_videoram_w( int offset, int data );
void tutankhm_flipscreen_w( int offset, int data );

void tutankhm_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


void tutankhm_init_machine(void);

void junofrst_blitter_w( int offset, int data );

void tutankhm_sh_irqtrigger_w(int offset,int data);

unsigned char KonamiDecode( unsigned char opcode, unsigned short address );

void junofrst_bankselect_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + (data & 0x0f) * 0x1000;

/*
	This bank code SHOULD be this but it doesn't work because the opcodes
   are encrypted

	cpu_setbank(1,&RAM[bankaddress]);
*/
	memcpy(&RAM[0x9000],&RAM[bankaddress],0x1000);
	memcpy(&ROM[0x9000],&ROM[bankaddress],0x1000);

}

static unsigned char JunoNibbleSwapTable[256];

void junofrst_init_machine(void)
{
	int i;

	for (i=0;i<256;i++) {
		JunoNibbleSwapTable[i] = (i>>4) | ((i & 0xF) << 4);
	}

	tutankhm_init_machine();
}


static int junofrst_portA_r(int offset)
{
	#define TIMER_RATE 40

	return cpu_gettotalcycles() / TIMER_RATE;
}

void junofrst_sh_irqtrigger_w(int offset,int data)
{
	static int last;


	if (last == 0 && data == 1)
	{
		/* setting bit 0 low then high triggers IRQ on the sound CPU */
		cpu_cause_interrupt(1,0xff);
	}

	last = data;
}

void junofrst_i8039_irq_w(int offset,int data)
{
	cpu_cause_interrupt(2,I8039_EXT_INT);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_RAM },
	{ 0x8010, 0x8010, input_port_0_r },	/* DSW2 (inverted bits) */
	{ 0x801c, 0x801c, watchdog_reset_r },
	{ 0x8020, 0x8020, input_port_1_r },	/* IN0 I/O: Coin slots, service, 1P/2P buttons */
	{ 0x8024, 0x8024, input_port_2_r },	/* IN1: Player 1 I/O */
	{ 0x8028, 0x8028, input_port_3_r },	/* IN2: Player 2 I/O */
	{ 0x802c, 0x802c, input_port_4_r },	/* DSW1 (inverted bits) */
	{ 0x8100, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9fff, MRA_BANK1 },
//	{ 0x9000, 0x9fff, MRA_ROM },
	{ 0xa000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, tutankhm_videoram_w, &videoram, &videoram_size },
	{ 0x8000, 0x800f, paletteram_BBGGGRRR_w, &paletteram },
	{ 0x8030, 0x8030, interrupt_enable_w },
	{ 0x8031, 0x8031, MWA_RAM },	/* ??? */
	{ 0x8032, 0x8032, MWA_RAM },	/* coin counters */
	{ 0x8033, 0x8033, MWA_RAM, &tutankhm_scrollx },              /* video x pan hardware reg - Not USED in Juno*/
	{ 0x8034, 0x8035, tutankhm_flipscreen_w },
	{ 0x8040, 0x8040, junofrst_sh_irqtrigger_w },
	{ 0x8050, 0x8050, soundlatch_w },
	{ 0x8060, 0x8060, junofrst_bankselect_w },
	{ 0x8070, 0x8073, junofrst_blitter_w },
	{ 0x8100, 0x8fff, MWA_RAM },
	{ 0xa000, 0xffff, MWA_ROM },
	{ -1 } /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x3000, 0x3000, soundlatch_r },
	{ 0x4001, 0x4001, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x4000, 0x4000, AY8910_control_port_0_w },
	{ 0x4002, 0x4002, AY8910_write_port_0_w },
	{ 0x5000, 0x5000, soundlatch2_w },
	{ 0x6000, 0x6000, junofrst_i8039_irq_w },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress i8039_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress i8039_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct IOReadPort i8039_readport[] =
{
	{ 0x00, 0xff, soundlatch2_r },
	{ 0x111,0x111, IORP_NOP },
	{ -1 }
};

static struct IOWritePort i8039_writeport[] =
{
	{ I8039_p1, I8039_p1, DAC_data_w },
	{ I8039_p2, I8039_p2, IOWP_NOP },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x70, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "Real Easy" )
	PORT_DIPSETTING(    0x60, "Not so Easy" )
	PORT_DIPSETTING(    0x50, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x30, "Harder than Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x10, "More Harder" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x01, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x50, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x10, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x70, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Disabled" )
/* 0x00 not remmed out since the game makes the usual sound if you insert the coin */
INPUT_PORTS_END



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1789750,	/* 1.78975 MHz ? (same as other Konami games) */
	{ 255 },
	{ junofrst_portA_r },
	{ 0 },
	{ 0 },
	{ 0 }	/* port B - shoot noise? */
};

static struct DACinterface dac_interface =
{
	1,
	{ 0x10ff }
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1500000,			/* 1.5 Mhz ??? */
			0,				/* memory region # 0 */
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2100000,	/* ??????? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		},
		{
			CPU_I8039 | CPU_AUDIO_CPU,
			8000000/15,	/* 8Mhz crystal???? */
			3,	/* memory region #3 */
			i8039_readmem,i8039_writemem,i8039_readport,i8039_writeport,
			ignore_interrupt,1
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	junofrst_init_machine,				/* init machine routine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },	/* not sure about the visible area */
	0,					/* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	0,			/* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,						/* vh_init routine */
	generic_vh_start,					/* vh_start routine */
	generic_vh_stop,					/* vh_stop routine */
	tutankhm_vh_screenrefresh,				/* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


ROM_START( junofrst_rom )
	ROM_REGION( 0x22000 )      /* 64k for M6809 CPU code + 64k for ROM banks */
	ROM_LOAD( "jfa_b9.bin",   0x0a000, 0x2000, 0xf5a7ab9d ) /* program ROMs */
	ROM_LOAD( "jfb_b10.bin",  0x0c000, 0x2000, 0xf20626e0 )
	ROM_LOAD( "jfc_a10.bin",  0x0e000, 0x2000, 0x1e7744a7 )

	ROM_LOAD( "jfc1_a4.bin",  0x10000, 0x2000, 0x03ccbf1d ) /* graphic and code ROMs (banked) */
	ROM_LOAD( "jfc2_a5.bin",  0x12000, 0x2000, 0xcb372372 )
	ROM_LOAD( "jfc3_a6.bin",  0x14000, 0x2000, 0x879d194b )
	ROM_LOAD( "jfc4_a7.bin",  0x16000, 0x2000, 0xf28af80b )
	ROM_LOAD( "jfc5_a8.bin",  0x18000, 0x2000, 0x0539f328 )
	ROM_LOAD( "jfc6_a9.bin",  0x1a000, 0x2000, 0x1da2ad6e )

	ROM_LOAD( "jfs3_c7.bin",  0x1c000, 0x2000, 0xaeacf6db )
	ROM_LOAD( "jfs4_d7.bin",  0x1e000, 0x2000, 0x206d954c )
	ROM_LOAD( "jfs5_e7.bin",  0x20000, 0x2000, 0x1eb87a6e )


	ROM_REGION_DISPOSE( 0x1000 ) /* ROM Region 1 -- discarded */
	/* empty memory region - not used by the game, but needed bacause the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION( 0x10000 ) /* 64k for Z80 sound CPU code */
	ROM_LOAD( "jfs1_j3.bin",  0x0000, 0x1000, 0x235a2893 )

	ROM_REGION(0x1000)	/* 8039 */
	ROM_LOAD( "jfs2_p4.bin",  0x0000, 0x1000, 0xd0fa5d5f )
ROM_END



static void junofrst_decode(void)
{
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	for (A = 0x6000;A < 0x1c000;A++)
	{
		ROM[A] = KonamiDecode(RAM[A],A);
	}
}

/* Juno First Blitter Hardware emulation

	Juno First can blit a 16x16 graphics which comes from un-memory mapped graphics roms

	$8070->$8071 specifies the destination NIBBLE address
	$8072->$8073 specifies the source NIBBLE address

	Depending on bit 0 of the source address either the source pixels will be copied to
	the destination address, or a zero will be written.
	This allows the game to quickly clear the sprites from the screen

	A lookup table is used to swap the source nibbles as they are the wrong way round in the
	source data.

	Bugs -

		Currently only the even pixels will be written to. This is to speed up the blit routine
		as it does not have to worry about shifting the source data.
		This means that all destination X values will be rounded to even values.
		In practice no one actaully notices this.

		The clear works properly.
*/

void junofrst_blitter_w( int offset, int data )
{
	static byte blitterdata[4];
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	blitterdata[offset] = data;

	/* Blitter is triggered by $8073 */
	if (offset==3)
	{
		int i;
		unsigned long srcaddress;
		unsigned long destaddress;
		byte srcflag;
		byte destflag;
		byte *JunoBLTRom = &RAM[0x1C000];

		srcaddress = (blitterdata[0x2]<<8) | (blitterdata[0x3]);
		srcflag = srcaddress & 1;
		srcaddress >>= 1;
		srcaddress &= 0x7FFE;
		destaddress = (blitterdata[0x0]<<8)  | (blitterdata[0x1]);

		destflag = destaddress & 1;

		destaddress >>= 1;
		destaddress &= 0x7fff;

		if (srcflag) {
			for (i=0;i<16;i++) {

#define JUNOBLITPIXEL(x)															\
					if (JunoBLTRom[srcaddress+x])									\
						tutankhm_videoram_w( destaddress+x,						\
									JunoNibbleSwapTable[								\
										JunoBLTRom[srcaddress+x]					\
									]);

				JUNOBLITPIXEL(0);
				JUNOBLITPIXEL(1);
				JUNOBLITPIXEL(2);
				JUNOBLITPIXEL(3);
				JUNOBLITPIXEL(4);
				JUNOBLITPIXEL(5);
				JUNOBLITPIXEL(6);
				JUNOBLITPIXEL(7);

				destaddress += 128;
				srcaddress += 8;
			}
		} else {
			for (i=0;i<16;i++) {

#define JUNOCLEARPIXEL(x) \
					if ((JunoBLTRom[srcaddress+x] & 0xF0)) \
						tutankhm_videoram_w( destaddress+x,						\
							RAM[destaddress+x] & 0xF0);					\
					if ((JunoBLTRom[srcaddress+x] & 0x0F)) \
						tutankhm_videoram_w( destaddress+x,						\
							RAM[destaddress+x] & 0x0F);

				JUNOCLEARPIXEL(0);
				JUNOCLEARPIXEL(1);
				JUNOCLEARPIXEL(2);
				JUNOCLEARPIXEL(3);
				JUNOCLEARPIXEL(4);
				JUNOCLEARPIXEL(5);
				JUNOCLEARPIXEL(6);
				JUNOCLEARPIXEL(7);
				destaddress += 128;
				srcaddress+= 8;
			}
		}
	}

}

static int hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */

	if (  (RAM[0x8100]==0x01)
		&& (RAM[0x8101]==0x00)
		&& (RAM[0x8102]==0x00)
		&& (RAM[0x8103]==0x02)
		&& (RAM[0x8104]==0x41)
		&& (RAM[0x8105]==0x41)
		&& (RAM[0x8106]==0x41)
		)
	{
			if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
			{

				osd_fread(f,&RAM[0x8100],0x81a8-0x8100);
				osd_fclose(f);
			}
		return 1;
	}
	else return 0; /* we can't load the hi scores yet */
}

static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8100],0x81a8-0x8100);
		osd_fclose(f);
	}
}


struct GameDriver junofrst_driver =
{
	__FILE__,
	0,
	"junofrst",
	"Juno First",
	"1983",
	"Konami",
	"Chris Hardy (MAME driver)\nMirko Buffoni (Tutankham driver)\nDavid Dahl (hardware info)\nAaron Giles\nMarco Cassili",
	0,
	&machine_driver,
	0,

	junofrst_rom,
	0, junofrst_decode,   /* ROM decode and opcode decode functions */
	0,      /* Sample names */
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_ROTATE_90,

	hiload, hisave		        /* High score load and save */
};
