#include "driver.h"
#include "M68000/M68000.h"
#include "vidhrdw/generic.h"



int ssi_videoram_r(int offset);
void ssi_videoram_w(int offset,int data);
int ssi_vh_start(void);
void ssi_vh_stop (void);
void ssi_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void rastan_sound_port_w(int offset,int data);
void rastan_sound_comm_w(int offset,int data);
int rastan_sound_comm_r(int offset);
void rastan_irq_handler (void);

void r_wr_a000(int offset,int data);
void r_wr_a001(int offset,int data);
int  r_rd_a001(int offset);

static void bankswitch_w ( int offset, int data ) {

	unsigned char *RAM = Machine->memory_region[2];

	int banknum = ( data - 1 ) & 3;

	cpu_setbank( 2, &RAM[ 0x10000 + ( banknum * 0x4000 ) ] );
}

static int ssi_input_r (int offset)
{
    switch (offset)
    {
         case 0x00:
              return readinputport(3); /* DSW A */

         case 0x02:
              return readinputport(4); /* DSW B */

         case 0x04:
              return readinputport(0); /* IN0 */

         case 0x06:
              return readinputport(1); /* IN1 */

         case 0x0e:
              return readinputport(2); /* IN2 */
    }

if (errorlog) fprintf(errorlog,"CPU #0 PC %06x: warning - read unmapped memory address %06x\n",cpu_getpc(),0x100000+offset);

	return 0xff;
}

void ssi_sound_w(int offset,int data)
{
	if (offset == 0)
		rastan_sound_port_w(0,(data >> 8) & 0xff);
	else if (offset == 2)
		rastan_sound_comm_w(0,(data >> 8) & 0xff);
}

int ssi_sound_r(int offset)
{
	if (offset == 2)
		return ( ( rastan_sound_comm_r(0) & 0xff ) << 8 );
	else return 0;
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x10000f, ssi_input_r },
	{ 0x200000, 0x20ffff, MRA_BANK1 },
	{ 0x300000, 0x301fff, paletteram_word_r },
	{ 0x400000, 0x400003, ssi_sound_r },
	{ 0x800000, 0x80ffff, ssi_videoram_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x10000f, watchdog_reset_w },	/* ? */
	{ 0x200000, 0x20ffff, MWA_BANK1 },
	{ 0x300000, 0x301fff, paletteram_RRRRGGGGBBBBxxxx_word_w, &paletteram },
	{ 0x400000, 0x400003, ssi_sound_w },
//	{ 0x500000, 0x500001, MWA_NOP },	/* ?? */
	{ 0x800000, 0x80ffff, ssi_videoram_w, &videoram, &videoram_size }, /* sprite ram */
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, YM2610_status_port_0_A_r },
	{ 0xe001, 0xe001, YM2610_read_port_0_r },
	{ 0xe002, 0xe002, YM2610_status_port_0_B_r },
	{ 0xe200, 0xe200, MRA_NOP },
	{ 0xe201, 0xe201, r_rd_a001 },
	{ 0xea00, 0xea00, MRA_NOP },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2610_control_port_0_A_w },
	{ 0xe001, 0xe001, YM2610_data_port_0_A_w },
	{ 0xe002, 0xe002, YM2610_control_port_0_B_w },
	{ 0xe003, 0xe003, YM2610_data_port_0_B_w },
	{ 0xe200, 0xe200, r_wr_a000 },
	{ 0xe201, 0xe201, r_wr_a001 },
	{ 0xe400, 0xe403, MWA_NOP }, /* pan */
	{ 0xee00, 0xee00, MWA_NOP }, /* ? */
	{ 0xf000, 0xf000, MWA_NOP }, /* ? */
	{ 0xf200, 0xf200, bankswitch_w },	/* ?? */
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( ssi_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1)

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2)

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE)
	PORT_DIPSETTING(    0x01, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE)
	PORT_DIPSETTING(    0x02, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE)
	PORT_DIPSETTING(    0x08, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit")
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE)
	PORT_DIPSETTING(    0xc0, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0x80, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0x40, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0x00, "1 Coin/6 Credits")

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, "Unknown",IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0x02, 0x02, "Unknown",IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0x0c, 0x0c, "Shields",IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "None")
	PORT_DIPSETTING(    0x0c, "1")
	PORT_DIPSETTING(    0x04, "2")
	PORT_DIPSETTING(    0x08, "3")
	PORT_DIPNAME( 0x10, 0x10, "Lives",IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2")
	PORT_DIPSETTING(    0x10, "3")
	PORT_DIPNAME( 0x20, 0x20, "2 Players Mode", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "Alternate")
	PORT_DIPSETTING(    0x20, "Simultaneous")
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue", IP_KEY_NONE)
	PORT_DIPSETTING(    0x40, "Yes")
	PORT_DIPSETTING(    0x00, "No")
	PORT_DIPNAME( 0x80, 0x80, "Allow Simultaneous Game", IP_KEY_NONE)
	PORT_DIPSETTING(    0x80, "Yes")
	PORT_DIPSETTING(    0x00, "No")
 /* I think the cabinet for this game should have two joysticks even
	in upright mode. Maybe this dip actually set cocktail mode where
	simultaneous game is now allowed of course.
	Or maybe it's just what I described... Sand666 21/5 */
INPUT_PORTS_END



static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 sprites */
	8192,	/* 8192 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 1*4, 0*4, 3*4, 2*4, 5*4, 4*4, 7*4, 6*4, 9*4, 8*4, 11*4, 10*4, 13*4, 12*4, 15*4, 14*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64, 8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo ssi_gfxdecodeinfo[] =
{
	{ 1, 0x000000, &tilelayout, 0, 256 },         /* sprites & playfield */
	{ -1 } /* end of array */
};

static struct YM2610interface ym2610_interface =
{
	1,	/* 1 chip */
	8000000,	/* 8 MHz ?????? */
	{ YM2203_VOL(60,30) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ rastan_irq_handler },
	{ 4 }, /* Does not have Delta T adpcm, so this can point to a non-existing region */
	{ 3 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* 6 MHz ??? */
			0,
			readmem,writemem,0,0,
			m68_level5_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ??? */
			2,
			sound_readmem, sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2610 */
		}
	},
	60, 1800,	/* frames per second, vblank duration hand tuned to avoid flicker */
	10,
	0,

	/* video hardware */
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 },

	ssi_gfxdecodeinfo,
	4096, 4096,
	0,

	VIDEO_TYPE_RASTER| VIDEO_MODIFIES_PALETTE,
	0,
	ssi_vh_start,
	ssi_vh_stop,
	ssi_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&ym2610_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ssi_rom )
	ROM_REGION(0x80000)     /* 512k for 68000 code */
	ROM_LOAD_EVEN( "ssi_15-1.rom", 0x0000, 0x40000, 0xce9308a6 )
	ROM_LOAD_ODD ( "ssi_16-1.rom", 0x0000, 0x40000, 0x470a483a )

	ROM_REGION_DISPOSE(0x100000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ssi_m01.rom",  0x0000, 0x100000, 0xa1b4f486 )

	ROM_REGION(0x1c000)      /* sound cpu */
	ROM_LOAD( "ssi_09.rom",   0x00000, 0x04000, 0x88d7f65c )
	ROM_CONTINUE(			  0x10000, 0x0c000 ) /* banked stuff */

	ROM_REGION(0x20000)      /* ADPCM samples */
	ROM_LOAD( "ssi_m02.rom",  0x0000, 0x20000, 0x3cb0b907 )
ROM_END



struct GameDriver ssi_driver =
{
	__FILE__,
	0,
	"ssi",
	"Super Space Invaders '91",
	"1990",
	"Taito",
	"Howie Cohen \nAlex Pasadyn \nBill Boyle (graphics info) \nRichard Bush (technical information)",
	0,
	&machine_driver,
	0,

	ssi_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	ssi_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_ROTATE_270,
	0, 0
};
