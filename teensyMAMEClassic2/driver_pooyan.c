/***************************************************************************

Pooyan memory map (preliminary)

Thanks must go to Mike Cuddy for providing information on this one.

Sound processor memory map.
0x3000-0x33ff RAM.
AY-8910 #1 : reg 0x5000
	     wr  0x4000
             rd  0x4000

AY-8910 #2 : reg 0x7000
	     wr  0x6000
             rd  0x6000

Main processor memory map.
0000-7fff ROM
8000-83ff color RAM
8400-87ff video RAM
8800-8fff RAM
9000-97ff sprite RAM (only areas 0x9010 and 0x9410 are used).

memory mapped ports:

read:
0xA000	Dipswitch 2 adddbtll
        a = attract mode
        ddd = difficulty 0=easy, 7=hardest.
        b = bonus setting (easy/hard)
        t = table / upright
        ll = lives: 11=3, 10=4, 01=5, 00=255.

0xA0E0  llllrrrr
        l == left coin mech, r = right coinmech.

0xA080	IN0 Port
0xA0A0	IN1 Port
0xA0C0	IN2 Port

write:
0xA100	command for the audio CPU.
0xA180	NMI enable. (0xA180 == 1 = deliver NMI to CPU).

0xA181	interrupt trigger on audio CPU.

0xA183	maybe reset sound cpu?

0xA184	????

0xA187	Flip screen

interrupts:
standard NMI at 0x66

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


void pooyan_flipscreen_w(int offset,int data);
void pooyan_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void pooyan_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/* I am not 100% sure that this timer is correct, but */
/* I'm using the Gyruss wired to the higher 4 bits    */
/* instead of the lower ones, so there is a good      */
/* chance it's the right one. */

/* The timer clock which feeds the lower 4 bits of    */
/* AY-3-8910 port A is based on the same clock        */
/* feeding the sound CPU Z80.  It is a divide by      */
/* 10240, formed by a standard divide by 1024,        */
/* followed by a divide by 10 using a 4 bit           */
/* bi-quinary count sequence. (See LS90 data sheet    */
/* for an example).                                   */
/* Bits 1-3 come directly from the upper three bits   */
/* of the bi-quinary counter. Bit 0 comes from the    */
/* output of the divide by 1024.                      */

static int pooyan_timer[20] = {
0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03, 0x04, 0x05,
0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b, 0x0c, 0x0d
};

static int pooyan_portB_r(int offset)
{
	/* need to protect from totalcycles overflow */
	static int last_totalcycles = 0;

	/* number of Z80 clock cycles to count */
	static int clock;

	int current_totalcycles;

	current_totalcycles = cpu_gettotalcycles();
	clock = (clock + (current_totalcycles-last_totalcycles)) % 10240;

	last_totalcycles = current_totalcycles;

	return pooyan_timer[clock/512] << 4;
}

void pooyan_sh_irqtrigger_w(int offset,int data)
{
	static int last;


	if (last == 1 && data == 0)
	{
		/* setting bit 0 high then low triggers IRQ on the sound CPU */
		cpu_cause_interrupt(1,0xff);
	}

	last = data;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },	/* color and video RAM */
	{ 0xa000, 0xa000, input_port_4_r },	/* DSW2 */
	{ 0xa080, 0xa080, input_port_0_r },	/* IN0 */
	{ 0xa0a0, 0xa0a0, input_port_1_r },	/* IN1 */
	{ 0xa0c0, 0xa0c0, input_port_2_r },	/* IN2 */
	{ 0xa0e0, 0xa0e0, input_port_3_r },	/* DSW1 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, colorram_w, &colorram },
	{ 0x8400, 0x87ff, videoram_w, &videoram, &videoram_size },
	{ 0x8800, 0x8fff, MWA_RAM },
	{ 0x9010, 0x903f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9410, 0x943f, MWA_RAM, &spriteram_2 },
	{ 0xa000, 0xa000, MWA_NOP },	/* watchdog reset? */
	{ 0xa100, 0xa100, soundlatch_w },
	{ 0xa180, 0xa180, interrupt_enable_w },
	{ 0xa181, 0xa181, pooyan_sh_irqtrigger_w },
	{ 0xa187, 0xa187, pooyan_flipscreen_w },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x3000, 0x33ff, MRA_RAM },
	{ 0x4000, 0x4000, AY8910_read_port_0_r },
	{ 0x6000, 0x6000, AY8910_read_port_1_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x3000, 0x33ff, MWA_RAM },
	{ 0x4000, 0x4000, AY8910_write_port_0_w },
	{ 0x5000, 0x5000, AY8910_control_port_0_w },
	{ 0x6000, 0x6000, AY8910_write_port_1_w },
	{ 0x7000, 0x7000, AY8910_control_port_1_w },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
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
	PORT_DIPSETTING(    0x00, "Attract Mode - No Play" )
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

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x08, 0x08, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "50000 80000" )
	PORT_DIPSETTING(    0x00, "30000 70000" )
	PORT_DIPNAME( 0x70, 0x70, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "Easiest" )
	PORT_DIPSETTING(    0x60, "Easier" )
	PORT_DIPSETTING(    0x50, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x30, "Medium" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	4,	/* 4 bits per pixel */
	{ 0x1000*8+4, 0x1000*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8*8+0,8*8+1,8*8+2,8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	4,	/* 4 bits per pixel */
	{ 0x1000*8+4, 0x1000*8+0, 4, 0 },
	{ 0, 1, 2, 3,  8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3,  24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 16 },
	{ 1, 0x2000, &spritelayout, 16*16, 16 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1789750,	/* 1.78975 MHz ? (same as other Konami games) */
	{ 0x30ff, 0x30ff },
	{ soundlatch_r },
	{ pooyan_portB_r },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz (?) */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318180/4,	/* ???? same as other Konami games */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32,16*16+16*16,
	pooyan_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	pooyan_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( pooyan_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1.4a",         0x0000, 0x2000, 0xbb319c63 )
	ROM_LOAD( "2.5a",         0x2000, 0x2000, 0xa1463d98 )
	ROM_LOAD( "3.6a",         0x4000, 0x2000, 0xfe1a9e08 )
	ROM_LOAD( "4.7a",         0x6000, 0x2000, 0x9e0f9bcc )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "8.10g",        0x0000, 0x1000, 0x931b29eb )
	ROM_LOAD( "7.9g",         0x1000, 0x1000, 0xbbe6d6e4 )
	ROM_LOAD( "6.9a",         0x2000, 0x1000, 0xb2d8c121 )
	ROM_LOAD( "5.8a",         0x3000, 0x1000, 0x1097c2b6 )

	ROM_REGION(0x0220)	/* color proms */
	ROM_LOAD( "pooyan.pr1",   0x0000, 0x0020, 0xa06a6d0e ) /* palette */
	ROM_LOAD( "pooyan.pr2",   0x0020, 0x0100, 0x82748c0b ) /* sprites */
	ROM_LOAD( "pooyan.pr3",   0x0120, 0x0100, 0x8cd4cd60 ) /* characters */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "xx.7a",        0x0000, 0x1000, 0xfbe2b368 )
	ROM_LOAD( "xx.8a",        0x1000, 0x1000, 0xe1795b3d )
ROM_END

ROM_START( pooyans_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic22_a4.cpu",  0x0000, 0x2000, 0x916ae7d7 )
	ROM_LOAD( "ic23_a5.cpu",  0x2000, 0x2000, 0x8fe38c61 )
	ROM_LOAD( "ic24_a6.cpu",  0x4000, 0x2000, 0x2660218a )
	ROM_LOAD( "ic25_a7.cpu",  0x6000, 0x2000, 0x3d2a10ad )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic13_g10.cpu", 0x0000, 0x1000, 0x7433aea9 )
	ROM_LOAD( "ic14_g9.cpu",  0x1000, 0x1000, 0x87c1789e )
	ROM_LOAD( "6.9a",         0x2000, 0x1000, 0xb2d8c121 )
	ROM_LOAD( "5.8a",         0x3000, 0x1000, 0x1097c2b6 )

	ROM_REGION(0x0220)	/* color proms */
	ROM_LOAD( "pooyan.pr1",   0x0000, 0x0020, 0xa06a6d0e ) /* palette */
	ROM_LOAD( "pooyan.pr2",   0x0020, 0x0100, 0x82748c0b ) /* sprites */
	ROM_LOAD( "pooyan.pr3",   0x0120, 0x0100, 0x8cd4cd60 ) /* characters */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "xx.7a",        0x0000, 0x1000, 0xfbe2b368 )
	ROM_LOAD( "xx.8a",        0x1000, 0x1000, 0xe1795b3d )
ROM_END

ROM_START( pootan_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "poo_ic22.bin", 0x0000, 0x2000, 0x41b23a24 )
	ROM_LOAD( "poo_ic23.bin", 0x2000, 0x2000, 0xc9d94661 )
	ROM_LOAD( "3.6a",         0x4000, 0x2000, 0xfe1a9e08 )
	ROM_LOAD( "poo_ic25.bin", 0x6000, 0x2000, 0x8ae459ef )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "poo_ic13.bin", 0x0000, 0x1000, 0x0be802e4 )
	ROM_LOAD( "poo_ic14.bin", 0x1000, 0x1000, 0xcba29096 )
	ROM_LOAD( "6.9a",         0x2000, 0x1000, 0xb2d8c121 )
	ROM_LOAD( "5.8a",         0x3000, 0x1000, 0x1097c2b6 )

	ROM_REGION(0x0220)	/* color proms */
	ROM_LOAD( "pooyan.pr1",   0x0000, 0x0020, 0xa06a6d0e ) /* palette */
	ROM_LOAD( "pooyan.pr2",   0x0020, 0x0100, 0x82748c0b ) /* sprites */
	ROM_LOAD( "pooyan.pr3",   0x0120, 0x0100, 0x8cd4cd60 ) /* characters */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "xx.7a",        0x0000, 0x1000, 0xfbe2b368 )
	ROM_LOAD( "xx.8a",        0x1000, 0x1000, 0xe1795b3d )
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8a00],"\x00\x00\x01",3) == 0 &&
			memcmp(&RAM[0x8a1b],"\x00\x00\x01",3) == 0)
	{
		void *f;
		static int first_loop;


		if (first_loop++ <= 1) return 0;	/* wait a little more, otherwise the times */
											/* will be overwritten */

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x89c0],3*10);
			osd_fread(f,&RAM[0x8a00],3*10);
			osd_fread(f,&RAM[0x8e00],3*10);
			RAM[0x88a8] = RAM[0x8a00];
			RAM[0x88a9] = RAM[0x8a01];
			RAM[0x88aa] = RAM[0x8a02];
			osd_fclose(f);
		}

		first_loop = 0;

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x89c0],3*10);
		osd_fwrite(f,&RAM[0x8a00],3*10);
		osd_fwrite(f,&RAM[0x8e00],3*10);
		osd_fclose(f);
	}
}



struct GameDriver pooyan_driver =
{
	__FILE__,
	0,
	"pooyan",
	"Pooyan (Konami)",
	"1982",
	"Konami",
	"Mike Cuddy (hardware info)\nAllard Van Der Bas (Pooyan emulator)\nNicola Salmoria (MAME driver)\nMartin Binder (color info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	pooyan_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver pooyans_driver =
{
	__FILE__,
	&pooyan_driver,
	"pooyans",
	"Pooyan (Stern)",
	"1982",
	"Stern",
	"Mike Cuddy (hardware info)\nAllard Van Der Bas (Pooyan emulator)\nNicola Salmoria (MAME driver)\nMartin Binder (color info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	pooyans_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver pootan_driver =
{
	__FILE__,
	&pooyan_driver,
	"pootan",
	"Pootan",
	"1982",
	"bootleg",
	"Mike Cuddy (hardware info)\nAllard Van Der Bas (Pooyan emulator)\nNicola Salmoria (MAME driver)\nMartin Binder (color info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	pootan_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};
