/***************************************************************************

The New Zealand Story driver, used for tnzs & tnzs2.

TO-DO: - Find out how the hardware credit-counter works
       - Fix problem with sprites and tiles being a few pixels out when the
         screen is flipped.

13/6/1998: - Hi-score saving/loading for tnzs & tnzs2, by Santeri Saarimaa

****************************************************************************

The New Zealand Story memory map (preliminary)

CPU #1
0000-7fff ROM
8000-bfff banked - banks 0-1 RAM; banks 2-7 ROM
c000-dfff object RAM, including:
  c000-c1ff sprites (code, low byte)
  c200-c3ff sprites (x-coord, low byte)
  c400-c5ff tiles (code, low byte)

  d000-d1ff sprites (code, high byte)
  d200-d3ff sprites (x-coord and colour, high byte)
  d400-d5ff tiles (code, high byte)
  d600-d7ff tiles (colour)
e000-efff RAM shared with CPU #2
f000-ffff VDC RAM, including:
  f000-f1ff sprites (y-coord)
  f200-f2ff scrolling info
  f300-f301 vdc controller
  f302-f303 scroll x-coords (high bits)
  f600      bankswitch
  f800-fbff palette

CPU #2
0000-7fff ROM
8000-9fff banked ROM
a000      bankswitch
b000-b001 YM2203 interface (with DIPs on YM2203 ports)
c000-c001 input ports (and coin counter)
e000-efff RAM shared with CPU #1
f000-f003 ???

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80/Z80.h"



/* prototypes for functions in ../machine/tnzs.c */
unsigned char *tnzs_objram, *tnzs_workram;
unsigned char *tnzs_vdcram, *tnzs_scrollram;
void tnzs_init_machine(void);

int tnzs_inputport_r(int offset);
int tnzs_workram_r(int offset);

void tnzs_inputport_w(int offset, int data);
void tnzs_workram_w(int offset, int data);

void tnzs_bankswitch_w(int offset, int data);
void tnzs_bankswitch1_w(int offset, int data);

void tnzs_sound_command_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog, "play sound %02x\n", data);
}

int tnzs_interrupt(void) { return 0; }

/* prototypes for functions in ../vidhrdw/tnzs.c */
int tnzs_vh_start(void);
void tnzs_vh_stop(void);
void tnzs_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0x7fff, MRA_ROM },
    { 0x8000, 0xbfff, MRA_BANK1 }, /* BANK RAM */
    { 0xc000, 0xdfff, MRA_RAM },
    { 0xe000, 0xefff, MRA_RAM },   /* WORK RAM - shared with audio CPU */
    { 0xf000, 0xf1ff, MRA_RAM },   /* VDC RAM  */
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },

	/* { 0xef10, 0xef10, tnzs_sound_command_w }, */

    { 0x8000, 0xbfff, MWA_BANK1 },
    { 0xc000, 0xdfff, MWA_RAM, &tnzs_objram },
    { 0xe000, 0xefff, MWA_RAM, &tnzs_workram }, /* WORK RAM - shared with audio CPU */
    { 0xf000, 0xf1ff, MWA_RAM, &tnzs_vdcram },
    { 0xf200, 0xf3ff, MWA_RAM, &tnzs_scrollram }, /* scrolling info */
	{ 0xf600, 0xf600, tnzs_bankswitch_w },
    { 0xf800, 0xfbff, paletteram_xRRRRRGGGGGBBBBB_w, &paletteram },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress readmem1[] =
{
    { 0x0000, 0x7fff, MRA_ROM },
    { 0x8000, 0x9fff, MRA_BANK2 },
    { 0xb000, 0xb000, YM2203_status_port_0_r  },
    { 0xb001, 0xb001, YM2203_read_port_0_r  },
    { 0xc000, 0xc001, tnzs_inputport_r},
    { 0xd000, 0xdfff, MRA_RAM },
    { 0xe000, 0xefff, tnzs_workram_r },
	{ 0xf000, 0xf003, MRA_RAM },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem1[] =
{
    { 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xa000, tnzs_bankswitch1_w },
    { 0xb000, 0xb000, YM2203_control_port_0_w  },
    { 0xb001, 0xb001, YM2203_write_port_0_w  },
    { 0xc000, 0xc001, tnzs_inputport_w },
    { 0xd000, 0xdfff, MWA_RAM },
    { 0xe000, 0xefff, tnzs_workram_w },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( tnzs_input_ports )
	PORT_START      /* IN0 - number of credits??? */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

 /* DIP switch settings supplied by Greg Best <gregbest98@hotmail.com> */
	PORT_START      /* DSW A - ef0e */
    PORT_DIPNAME( 0x01, 0x01, "Cabinet", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Upright" )
    PORT_DIPSETTING(    0x01, "Cocktail" )
    PORT_DIPNAME( 0x02, 0x02, "Flip Screen", IP_KEY_NONE )
    PORT_DIPSETTING(    0x02, "Off" )
    PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* Always off */
    PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
    PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
    PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
    PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
    PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
    PORT_DIPSETTING(    0xc0, "1 Coin/2 Credits" )
    PORT_DIPSETTING(    0x80, "1 Coin/3 Credits" )
    PORT_DIPSETTING(    0x40, "1 Coin/4 Credits" )
    PORT_DIPSETTING(    0x00, "1 Coin/6 Credits" )

	PORT_START      /* DSW B - ef0f */
    PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE)
    PORT_DIPSETTING(    0x02, "Easy" )
    PORT_DIPSETTING(    0x03, "Medium" )
    PORT_DIPSETTING(    0x01, "Hard" )
    PORT_DIPSETTING(    0x00, "Hardest" )
    PORT_DIPNAME( 0x0c, 0x0c, "Bonus Life", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "50000 150000" )
    PORT_DIPSETTING(    0x0c, "70000 200000" )
    PORT_DIPSETTING(    0x04, "100000 250000" )
    PORT_DIPSETTING(    0x08, "200000 300000" )
    PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
    PORT_DIPSETTING(    0x20, "2" )
    PORT_DIPSETTING(    0x30, "3" )
    PORT_DIPSETTING(    0x00, "4" )
    PORT_DIPSETTING(    0x10, "5" )
    PORT_DIPNAME( 0x40, 0x40, "Allow Continue", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "No" )
    PORT_DIPSETTING(    0x40, "Yes" )
    PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
    PORT_DIPSETTING(    0x80, "Off" )
    PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	16,16,	/* the characters are 16x16 pixels */
    0x100 * 32,
	4,	/* 4 bits per pixel */
    { 0xc0000*8, 0x80000*8, 0x40000*8, 0x00000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 64, 65, 66, 67, 68, 69, 70, 71 },
	{ 0, 8, 16, 24, 32, 40, 48, 56, 128, 136, 144, 152, 160, 168, 176, 184},
	32*8	/* every char takes 32 bytes in four ROMs */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
    { 1, 0x00000, &charlayout, 0, 32 },
	{ -1 }	/* end of array */
};



static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	1500000,	/* 1.5 MHz ??? */
	{ YM2203_VOL(255,255) },
	{ input_port_6_r },
	{ input_port_7_r },
	{ 0 },
	{ 0 }
};



static struct MachineDriver tnzs_machine_driver =
{
    /* basic machine hardware */
    {				/* MachineCPU */
		{
			CPU_Z80,
			4000000,		/* 4 Mhz??? */
			0,			/* memory_region */
			readmem,writemem,0,0,
			tnzs_interrupt,1
		},
		{
			CPU_Z80,
            6000000,        /* 6 Mhz??? */
			2,			/* memory_region */
			readmem1,writemem1,0,0,
			interrupt,1
		}
    },
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	200,	/* 100 CPU slices per frame - an high value to ensure proper */
			/* synchronization of the CPUs */
    tnzs_init_machine,		/* init_machine() */

    /* video hardware */
    16*16, 14*16,         /* screen_width, height */
    { 0, 16*16-1, 0, 14*16-1 }, /* visible_area */
    gfxdecodeinfo,
    512, 512,
    0,

    VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
    0,
	tnzs_vh_start,
	tnzs_vh_stop,
	tnzs_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};




/***************************************************************************

  Game driver(s)

***************************************************************************/

/* 00000 ROM (static)
   10000 RAM page 0
   14000 RAM page 1
   18000 ROM page 2
   1c000 ROM page 3
   20000 ROM page 4
   24000 ROM page 5
   28000 ROM page 6
   2c000 ROM page 7
  */
ROM_START( tnzs_rom )
    ROM_REGION(0x30000)	/* 64k + bankswitch areas for the first CPU */
    ROM_LOAD( "nzsb5310.bin", 0x00000, 0x08000, 0xa73745c6 )
    /* 32k padding for 2 RAM banks, followed by 5 ROM banks */
    ROM_CONTINUE(             0x18000, 0x18000 )

    ROM_REGION_DISPOSE(0x100000)	/* temporary space for graphics (disposed after conversion) */
	/* ROMs taken from another set (the ones from this set were read incorrectly) */
	ROM_LOAD( "nzsb5316.bin", 0x00000, 0x20000, 0xc3519c2a )
	ROM_LOAD( "nzsb5317.bin", 0x20000, 0x20000, 0x2bf199e8 )
	ROM_LOAD( "nzsb5318.bin", 0x40000, 0x20000, 0x92f35ed9 )
	ROM_LOAD( "nzsb5319.bin", 0x60000, 0x20000, 0xedbb9581 )
	ROM_LOAD( "nzsb5322.bin", 0x80000, 0x20000, 0x59d2aef6 )
	ROM_LOAD( "nzsb5323.bin", 0xa0000, 0x20000, 0x74acfb9b )
	ROM_LOAD( "nzsb5320.bin", 0xc0000, 0x20000, 0x095d0dc0 )
	ROM_LOAD( "nzsb5321.bin", 0xe0000, 0x20000, 0x9800c54d )

    ROM_REGION(0x18000)	/* 64k for the second CPU */
    ROM_LOAD( "nzsb5311.bin", 0x00000, 0x08000, 0x9784d443 )
    ROM_CONTINUE(             0x10000, 0x08000 )
ROM_END




ROM_START( tnzs2_rom )
    ROM_REGION(0x30000)	/* 64k + bankswitch areas for the first CPU */
    ROM_LOAD( "ns_c-11.rom",  0x00000, 0x08000, 0x3c1dae7b )
    /* 32k padding for 2 RAM banks, followed by 5 ROM banks */
    ROM_CONTINUE(             0x18000, 0x18000 )

    ROM_REGION_DISPOSE(0x100000)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "ns_a13.rom",   0x00000, 0x20000, 0x7e0bd5bb )
    ROM_LOAD( "ns_a12.rom",   0x20000, 0x20000, 0x95880726 )
    ROM_LOAD( "ns_a10.rom",   0x40000, 0x20000, 0x2bc4c053 )
    ROM_LOAD( "ns_a08.rom",   0x60000, 0x20000, 0x8ff8d88c )
    ROM_LOAD( "ns_a07.rom",   0x80000, 0x20000, 0x291bcaca )
    ROM_LOAD( "ns_a05.rom",   0xa0000, 0x20000, 0x6e762e20 )
    ROM_LOAD( "ns_a04.rom",   0xc0000, 0x20000, 0xe1fd1b9d )
    ROM_LOAD( "ns_a02.rom",   0xe0000, 0x20000, 0x2ab06bda )

    ROM_REGION(0x18000)	/* 64k for the second CPU */
    ROM_LOAD( "ns_e-3.rom",   0x00000, 0x08000, 0xc7662e96 )
    ROM_CONTINUE(             0x10000, 0x08000 )
ROM_END

static int tnzs_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0xe6ad], "\x47\x55\x55", 3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
            osd_fread(f, &RAM[0xe68d], 35);
			osd_fclose(f);
		}

		return 1;
	}
    else return 0; /* we can't load the hi scores yet */
}

static void tnzs_hisave(void)
{
    unsigned char *RAM = Machine->memory_region[0];
    void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
        osd_fwrite(f, &RAM[0xe68d], 35);
		osd_fclose(f);
	}
}

static int tnzs2_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0xec2a], "\x47\x55\x55", 3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
            osd_fread(f, &RAM[0xec0a], 35);
			osd_fclose(f);
		}

		return 1;
	}
    else return 0; /* we can't load the hi scores yet */
}

static void tnzs2_hisave(void)
{
    unsigned char *RAM = Machine->memory_region[0];
    void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
        osd_fwrite(f, &RAM[0xec0a], 35);
		osd_fclose(f);
	}
}


struct GameDriver tnzs_driver =
{
	__FILE__,
	0,
	"tnzs",
	"The New Zealand Story",
	"1988",
	"Taito",
    "Chris Moore\nMartin Scragg\nRichard Mitton\nSanteri Saarimaa (hi-scores)",
	0,
	&tnzs_machine_driver,
	0,

	tnzs_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tnzs_input_ports,

    0, 0, 0,
    ORIENTATION_DEFAULT,

    tnzs_hiload, tnzs_hisave
};

struct GameDriver tnzs2_driver =
{
	__FILE__,
	0,
	"tnzs2",
	"The New Zealand Story 2",
	"1988",
	"Taito",
    "Chris Moore\nMartin Scragg\nRichard Mitton\nSanteri Saarimaa (hi-scores)",
	0,
	&tnzs_machine_driver,
	0,

	tnzs2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tnzs_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

    tnzs2_hiload, tnzs2_hisave
};
