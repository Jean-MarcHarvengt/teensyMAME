/***************************************************************************

Circus memory map

0000-00FF Base Page RAM
0100-01FF Stack RAM
1000-1FFF ROM
2000      Clown Verticle Position
3000      Clown Horizontal Position
4000-43FF Video RAM
8000      Clown Rotation and Audio Controls
F000-FFF7 ROM
FFF8-FFFF Interrupt and Reset Vectors

A000      Control Switches
C000      Option Switches
D000      Paddle Position and Interrupt Reset

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

void circus_clown_x_w(int offset, int data);
void circus_clown_y_w(int offset, int data);
void circus_clown_z_w(int offset, int data);


void crash_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void circus_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void robotbowl_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

int crash_interrupt(void);

static int circus_interrupt;

static int ripcord_IN2_r (int offset)
{
	circus_interrupt ++;
	if (errorlog) fprintf (errorlog, "circus_int: %02x\n", circus_interrupt);
	return readinputport (2);
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x01FF, MRA_RAM },
	{ 0x4000, 0x43FF, MRA_RAM },
	{ 0x1000, 0x1fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },
//	{ 0xd000, 0xd000, input_port_2_r },
	{ 0xd000, 0xd000, ripcord_IN2_r },
	{ 0xA000, 0xA000, input_port_0_r },
	{ 0xC000, 0xC000, input_port_1_r }, /* DSW */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x2000, 0x2000, circus_clown_x_w },
	{ 0x3000, 0x3000, circus_clown_y_w },
	{ 0x8000, 0x8000, circus_clown_z_w },
	{ 0x1000, 0x1fff, MWA_ROM },
	{ 0xF000, 0xFFFF, MWA_ROM },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* Dip Switch */
	PORT_DIPNAME( 0x03, 0x00, "Jumps", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x02, "7" )
	PORT_DIPSETTING(    0x03, "9" )
	PORT_DIPNAME( 0x0C, 0x04, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Player - 1 Coin" )
	PORT_DIPSETTING(    0x04, "1 Player - 1 Coin" )
	PORT_DIPSETTING(    0x08, "1 Player - 2 Coin" )
	PORT_DIPNAME( 0x10, 0x00, "Top Score", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Credit Awarded" )
	PORT_DIPSETTING(    0x00, "No Award" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Single Line" )
	PORT_DIPSETTING(    0x20, "Super Bonus" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START      /* IN2 - paddle */
	PORT_ANALOG ( 0xff, 115, IPT_PADDLE, 50, 0, 64, 167 )
INPUT_PORTS_END

static unsigned char palette[] = /* Smoothed pure colors, overlays are not so contrasted */
{
	0x00,0x00,0x00, /* BLACK */
	0xff,0xff,0x20, /* YELLOW */
	0x20,0xff,0x20, /* GREEN */
	0x20,0x20,0xFF, /* BLUE */
	0xff,0xff,0xff, /* WHITE */
	0xff,0x20,0x20, /* RED */
};

static unsigned short colortable[] =
{
	0,0,
	0,1,
	0,2,
	0,3,
	0,4,
	0,5
};


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	1,              /* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout clownlayout =
{
	16,16,  /* 16*16 characters */
	16,             /* 16 characters */
	1,              /* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  16*8, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*16   /* every char takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout, 0, 5 },
	{ 1, 0x0800, &clownlayout, 0, 5 },
	{ -1 } /* end of array */
};



static struct DACinterface dac_interface =
{
	1,
	{ 255, 255 }
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			11289000/16,	/* 705.562kHz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60, 3500,	/* frames per second, vblank duration (complete guess) */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	circus_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/***************************************************************************

 Circus

***************************************************************************/

ROM_START( circus_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "circus.1a",    0x1000, 0x0200, 0x7654ea75 ) /* Code */
	ROM_LOAD( "circus.2a",    0x1200, 0x0200, 0xb8acdbc5 )
	ROM_LOAD( "circus.3a",    0x1400, 0x0200, 0x901dfff6 )
	ROM_LOAD( "circus.5a",    0x1600, 0x0200, 0x9dfdae38 )
	ROM_LOAD( "circus.6a",    0x1800, 0x0200, 0xc8681cf6 )
	ROM_LOAD( "circus.7a",    0x1A00, 0x0200, 0x585f633e )
	ROM_LOAD( "circus.8a",    0x1C00, 0x0200, 0x69cc409f )
	ROM_LOAD( "circus.9a",    0x1E00, 0x0200, 0xaff835eb )
	ROM_RELOAD(            0xFE00, 0x0200 ) /* for the reset and interrupt vectors */

	ROM_REGION_DISPOSE(0x0A00)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "circus.1c",    0x0600, 0x0200, 0x1f954bb3 )     /* Character Set */
	ROM_LOAD( "circus.2c",    0x0400, 0x0200, 0x361da7ee )
	ROM_LOAD( "circus.3c",    0x0200, 0x0200, 0x30d72ef5 )
	ROM_LOAD( "circus.4c",    0x0000, 0x0200, 0x6efc315a )
	ROM_LOAD( "circus.14d",   0x0800, 0x0200, 0x2fde3930 ) /* Clown */

ROM_END


static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x0036],"\x00\x00",2) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0036],2);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0036],2);
		osd_fclose(f);
	}
}


struct GameDriver circus_driver =
{
	__FILE__,
	0,
	"circus",
	"Circus",
	"1977",
	"Exidy",
	"Mike Coates",
	0,
	&machine_driver,
	0,

	circus_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	hiload,hisave
};

/***************************************************************************

 Robot Bowl

***************************************************************************/

ROM_START( robotbowl_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "robotbwl.1a",  0xF000, 0x0200, 0xdf387a0b ) /* Code */
	ROM_LOAD( "robotbwl.2a",  0xF200, 0x0200, 0xc948274d )
	ROM_LOAD( "robotbwl.3a",  0xF400, 0x0200, 0x8fdb3ec5 )
	ROM_LOAD( "robotbwl.5a",  0xF600, 0x0200, 0xba9a6929 )
	ROM_LOAD( "robotbwl.6a",  0xF800, 0x0200, 0x16fd8480 )
	ROM_LOAD( "robotbwl.7a",  0xFA00, 0x0200, 0x4cadbf06 )
	ROM_LOAD( "robotbwl.8a",  0xFC00, 0x0200, 0xbc809ed3 )
	ROM_LOAD( "robotbwl.9a",  0xFE00, 0x0200, 0x07487e27 )

	ROM_REGION_DISPOSE(0x0A00)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "robotbwl.1c",  0x0600, 0x0200, 0xb2991e7e )     /* Character Set */
	ROM_LOAD( "robotbwl.2c",  0x0400, 0x0200, 0x47b3e39c )
	ROM_LOAD( "robotbwl.3c",  0x0200, 0x0200, 0xd5380c9b )
	ROM_LOAD( "robotbwl.4c",  0x0000, 0x0200, 0xa5f7acb9 )
	ROM_LOAD( "robotbwl.14d", 0x0800, 0x0020, 0xa402ac06 ) /* Ball */

ROM_END

INPUT_PORTS_START( robotbowl_input_ports )

	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, 0,"Hook Right", OSD_KEY_X, 0, 0 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0,"Hook Left", OSD_KEY_Z, 0, 0 )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* Dip Switch */
	PORT_DIPNAME( 0x60, 0x00, "Bowl Timer", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x40, "7" )
	PORT_DIPSETTING(    0x60, "9" )
	PORT_DIPNAME( 0x18, 0x08, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Player - 1 Coin" )
	PORT_DIPSETTING(    0x08, "1 Player - 1 Coin" )
	PORT_DIPSETTING(    0x10, "1 Player - 2 Coin" )
	PORT_DIPNAME( 0x04, 0x04, "Beer Frame", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

INPUT_PORTS_END

static struct GfxLayout robotlayout =
{
	8,8,  /* 16*16 characters */
	1,      /* 1 character */
	1,      /* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8
};

static struct GfxDecodeInfo robotbowl_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout, 0, 5 },
	{ 1, 0x0800, &robotlayout, 0, 5 },
	{ -1 } /* end of array */
};

static void robotbowl_decode(void)
{
	/* PROM is reversed, fix it! */

	int Count;

    for(Count=31;Count>=0;Count--) Machine->memory_region[1][0x800+Count]^=0xFF;
}

static struct MachineDriver robotbowl_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			11289000/16,	/* 705.562kHz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60, 3500,	/* frames per second, vblank duration (complete guess) */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	robotbowl_gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	robotbowl_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

struct GameDriver robotbwl_driver =
{
	__FILE__,
	0,
	"robotbwl",
	"Robot Bowl",
	"1977",
	"Exidy",
	"Mike Coates",
	0,
	&robotbowl_machine_driver,
	0,

	robotbowl_rom,
	robotbowl_decode, 0,
	0,
	0,      /* sound_prom */

	robotbowl_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0,0
};

/***************************************************************************

 Crash

***************************************************************************/

ROM_START( crash_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "crash.a1",     0x1000, 0x0200, 0xb9571203 ) /* Code */
	ROM_LOAD( "crash.a2",     0x1200, 0x0200, 0xb4581a95 )
	ROM_LOAD( "crash.a3",     0x1400, 0x0200, 0x597555ae )
	ROM_LOAD( "crash.a4",     0x1600, 0x0200, 0x0a15d69f )
	ROM_LOAD( "crash.a5",     0x1800, 0x0200, 0xa9c7a328 )
	ROM_LOAD( "crash.a6",     0x1A00, 0x0200, 0xc7d62d27 )
	ROM_LOAD( "crash.a7",     0x1C00, 0x0200, 0x5e5af244 )
	ROM_LOAD( "crash.a8",     0x1E00, 0x0200, 0x3dc50839 )
	ROM_RELOAD(            0xFE00, 0x0200 ) /* for the reset and interrupt vectors */

	ROM_REGION_DISPOSE(0x0A00)      /* temporary space for graphics */
	ROM_LOAD( "crash.c1",     0x0600, 0x0200, 0xe9adf1e1 )  /* Character Set */
	ROM_LOAD( "crash.c2",     0x0400, 0x0200, 0x38f3e4ed )
	ROM_LOAD( "crash.c3",     0x0200, 0x0200, 0x3c8f7560 )
	ROM_LOAD( "crash.c4",     0x0000, 0x0200, 0xba16f9e8 )
	ROM_LOAD( "crash.d14",    0x0800, 0x0200, 0x833f81e4 ) /* Cars */
ROM_END

INPUT_PORTS_START( crash_input_ports )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* Dip Switch */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0C, 0x04, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x10, 0x00, "Top Score", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No Award" )
	PORT_DIPSETTING(    0x10, "Credit Awarded" )
	PORT_BIT( 0x60, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

static int crash_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (RAM[0x004B] != 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x000F],2);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void crash_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x000F],2);
		osd_fclose(f);
		RAM[0x004B] =0;
	}
}

static struct MachineDriver crash_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			11289000/16,	/* 705.562kHz */
			0,
			readmem,writemem,0,0,
			interrupt,2
		}
	},
	60, 3500,	/* frames per second, vblank duration (complete guess) */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	crash_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

struct GameDriver crash_driver =
{
	__FILE__,
	0,
	"crash",
	"Crash",
	"1979",
	"Exidy",
	"Mike Coates",
	0,
	&crash_machine_driver,
	0,

	crash_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	crash_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	crash_hiload,crash_hisave
};

/***************************************************************************

 Rip Cord

***************************************************************************/

ROM_START( ripcord_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "9027.1a",      0x1000, 0x0200, 0x56b8dc06 ) /* Code */
	ROM_LOAD( "9028.2a",      0x1200, 0x0200, 0xa8a78a30 )
	ROM_LOAD( "9029.4a",      0x1400, 0x0200, 0xfc5c8e07 )
	ROM_LOAD( "9030.5a",      0x1600, 0x0200, 0xb496263c )
	ROM_LOAD( "9031.6a",      0x1800, 0x0200, 0xcdc7d46e )
	ROM_LOAD( "9032.7a",      0x1A00, 0x0200, 0xa6588bec )
	ROM_LOAD( "9033.8a",      0x1C00, 0x0200, 0xfd49b806 )
	ROM_LOAD( "9034.9a",      0x1E00, 0x0200, 0x7caf926d )
	ROM_RELOAD(          0xFE00, 0x0200 ) /* for the reset and interrupt vectors */

	ROM_REGION_DISPOSE(0x0A00)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "9026.5c",      0x0000, 0x0200, 0x06e7adbb )
	ROM_LOAD( "9025.4c",      0x0200, 0x0200, 0x3129527e )
	ROM_LOAD( "9024.2c",      0x0400, 0x0200, 0xbcb88396 )
	ROM_LOAD( "9023.1c",      0x0600, 0x0200, 0x9f86ed5b )     /* Character Set */
	ROM_LOAD( "9035.14d",     0x0800, 0x0200, 0xc9979802 )
ROM_END

INPUT_PORTS_START( ripcord_input_ports )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* Dip Switch */
	PORT_DIPNAME( 0x03, 0x03, "Jumps", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x02, "7" )
	PORT_DIPSETTING(    0x03, "9" )
	PORT_DIPNAME( 0x0C, 0x04, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Player - 1 Coin" )
	PORT_DIPSETTING(    0x04, "1 Player - 1 Coin" )
	PORT_DIPSETTING(    0x08, "1 Player - 2 Coin" )
	PORT_DIPNAME( 0x10, 0x10, "Top Score", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Credit Awarded" )
	PORT_DIPSETTING(    0x00, "No Award" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START      /* IN2 - paddle */
	PORT_ANALOG ( 0xff, 115, IPT_PADDLE, 50, 0, 64, 167 )
INPUT_PORTS_END

static int ripcord_interrupt (void)
{
	circus_interrupt = 0;
	if (1)
		return ignore_interrupt();
	else
		return interrupt();
}

static struct MachineDriver ripcord_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			705562,        /* 11.289MHz / 16 */
			0,
			readmem,writemem,0,0,
			ripcord_interrupt,1
		}
	},
	60, 3500,	/* frames per second, vblank duration (complete guess) */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	crash_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

struct GameDriver ripcord_driver =
{
	__FILE__,
	0,
	"ripcord",
	"Rip Cord",
	"1977",
	"Exidy",
	"Mike Coates",
	GAME_NOT_WORKING,
	&ripcord_machine_driver,
	0,

	ripcord_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	ripcord_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0
};
