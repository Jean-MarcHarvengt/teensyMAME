/***************************************************************************

Side Pocket - (c) 1986 Data East

Ernesto Corvi
ernesto@imagina.com

Thanks must go to Mirko Buffoni for testing the music.

Memory Map:
Main CPU:
0000-0fff RAM
1000-13ff Character's RAM
1800-1bff Character's Color RAM and attributes
2000-20ff Sprite RAM
3000-3004 I/O Ports
4000-ffff ROM

Sound CPU:
0000-0fff RAM
1000-1001 YM2203
2000-2001 YM3526
3000-3000 Input from Main CPU
8000-ffff ROM

TODO:
- Add proper colors
- Fix sprites enable/disable? (entering a High Score sprites remain onscreen, maybe hidden by color?)
- Clip out sprites?
- Find input ports settings
- Fix High Score rectangles

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809/M6809.h"
#include "M6502/m6502.h"

/* from vidhrdw */
extern void sidepocket_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* stub for reading input ports as active low (makes building ports much easier) */
static int low_input_r( int offset ) {
    return ~readinputport( offset );
}

static void sound_cpu_command_w( int offset, int v ) {
    soundlatch_w( offset, v );
    cpu_cause_interrupt( 1, M6502_INT_NMI );
}

static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0x0fff, MRA_RAM },
    { 0x1000, 0x13ff, videoram_r },
    { 0x1800, 0x1bff, colorram_r },
    { 0x2000, 0x20ff, MRA_RAM },
    { 0x3000, 0x3003, low_input_r },
    { 0x4000, 0xffff, MRA_ROM },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
    { 0x0000, 0x0fff, MWA_RAM },
    { 0x1000, 0x13ff, videoram_w, &videoram, &videoram_size },
    { 0x1800, 0x1bff, colorram_w, &colorram },
    { 0x2000, 0x20ff, MWA_RAM, &spriteram, &spriteram_size },
    { 0x3004, 0x3004, sound_cpu_command_w },
    { 0x4000, 0xffff, MWA_ROM },
    { -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
    { 0x0000, 0x0fff, MRA_RAM },
    { 0x3000, 0x3000, soundlatch_r },
    { 0x8000, 0xffff, MRA_ROM },
    { -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
    { 0x0000, 0x0fff, MWA_RAM },
    { 0x1000, 0x1000, YM2203_control_port_0_w },
    { 0x1001, 0x1001, YM2203_write_port_0_w },
    { 0x2000, 0x2000, YM3526_control_port_0_w },
    { 0x2001, 0x2001, YM3526_write_port_0_w },
    { 0x8000, 0xffff, MWA_ROM },
    { -1 }  /* end of table */
};

INPUT_PORTS_START( input_ports )
    PORT_START /* 0x3000 */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

    PORT_START /* 0x3001 */
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN2 ) /* adds 2 credits? */
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN3 )
    PORT_START /* 0x3002 */

    PORT_START /* 0x3003 */
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
    8,8,    /* 8*8 characters */
    2048,   /* 2048 characters */
    3,      /* 2 bits per pixel */
    { 0, 0x8000*8, 0x10000*8 },     /* the bitplanes are separated */
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
    16,16,  /* 16*16 sprites */
    1024,   /* 1024 sprites */
    3,      /* 3 bits per pixel */
    { 0, 0x8000*8, 0x10000*8 },     /* the bitplanes are separated */
    { 128+0, 128+1, 128+2, 128+3, 128+4, 128+5, 128+6, 128+7, 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
    32*8    /* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
    { 1, 0x00000, &charlayout,   0, 16 }, /* characters */
    { 1, 0x18000, &spritelayout, 16*8, 16 }, /* sprites */
    { -1 } /* end of array */
};

static unsigned char palette[] =
{
    0x00,0x00,0x00, /* 0 - BLACK */
    0xff,0x00,0x00, /* 1 - RED */
    0x00,0xff,0x00, /* 2 - GREEN */
    0x00,0x00,0xff, /* 3 - BLUE */
    0xff,0xff,0x00, /* 4 - YELLOW */
    0xff,0x00,0xff, /* 5 - MAGENTA */
    0x00,0xff,0xff, /* 6 - CYAN */
    0xff,0xff,0xff, /* 7 - WHITE */
    0xE0,0xE0,0xE0, /* 8 - LTGRAY */
    0xC0,0xC0,0xC0, /* 9 - DKGRAY */
    0xe0,0xb0,0x70, /* 10 - BROWN */
    0xd0,0xa0,0x60, /* 11 - BROWN0 */
    0xc0,0x90,0x50, /* 12 - BROWN1 */
    0xa3,0x78,0x3a, /* 13 - BROWN2 */
    0x80,0x60,0x20, /* 14 - BROWN3 */
    0x54,0x40,0x14, /* 15 - BROWN4 */
    0x00,0x00,0x9f, /* 16 - DKBLUE */
    0x00,0x70,0x00, /* 17 - DKGREEN */
    0x00,0xe0,0x00, /* 18 - GRASSGREEN */
    0x50,0xff,0x6a, /* 19 - LTGREEN */
    0x49,0xb6,0xdb, /* 20 - DKCYAN */
    0xff,0xbf,0x59, /* 21 - LTORANGE */
    0xff,128,0x00,  /* 22 - ORANGE */
    0xdb,0xdb,0xdb, /* 23 - GREY */
    0x10,0x10,0xff, /* 24 - MDBLUE */
    0x54,0x90,0xff, /* 25 - LTBLUE */
    0xff,0x9a,0x36  /* 26 - MDORANGE */
};

static unsigned short colortable[] =
{
/* Characters colortable */
    0,7,7,7,7,7,7,7,            /* Hi-score alphabet */
    0,7,14,9,17,8,12,7,         /* Table-edge */
    0,18,2,3,7,16,18,7,         /* Table-top */
    0,0,16,7,16,4,24,7,         /* Title */

    0,1,1,1,1,1,1,1,            /* ??? */
    0,2,2,2,2,2,2,2,            /* ??? */
    0,3,3,3,3,3,3,3,            /* ??? */
    0,4,4,4,4,4,4,4,            /* ??? */

    0,9,23,7,1,19,8,15,         /* Shot info */
    0,5,5,5,5,5,5,5,            /* ??? */
    0,6,6,6,6,6,6,6,            /* ??? */
    0,4,4,4,4,4,4,4,            /* Selected character */

    0,7,7,7,7,7,7,7,            /* ??? */
    0,10,10,10,10,10,10,10,     /* ??? */
    0,16,16,16,16,16,16,16,     /* ??? */
    0,1,2,3,4,5,6,7,            /* ??? */

/* Sprites colortable */

    0,7,8,23,7,23,17,7,           /* White ball */
    0,7,21,4,4,23,17,4,           /* Yellow ball */
    0,7,25,24,3,23,17,3,          /* Blue ball */
    0,7,21,26,22,23,17,22,        /* Orange ball */

    0,7,5,5,5,23,17,5,            /* Purple ball */
    0,7,21,21,21,23,17,21,        /* Lt Orange ball */
    0,7,18,19,2,23,17,2,          /* Green ball */
    0,7,1,1,1,23,17,1,            /* Red ball */

    0,7,0,0,0,23,17,0,            /* Black ball */
    0,7,6,6,6,23,17,6,            /* Cyan ball */
    0,7,19,19,19,23,17,19,        /* Pale Green ball */
    0,23,13,23,15,21,24,25,       /* Player */

    0,23,13,23,15,21,24,25,       /* Player */
    0,23,13,23,15,21,24,25,       /* Player */
    0,23,13,23,15,21,24,25,       /* Player */
    0,1,2,3,4,5,6,7,              /* ??? */
};

static struct YM2203interface ym2203_interface =
{
    1,      /* 1 chip */
    1500000,        /* 1.5 MHz ??? */
    { YM2203_VOL(255,255) },
    { 0 },
    { 0 },
    { 0 },
    { 0 }
};

static struct YM3526interface ym3526_interface =
{
    1,                      /* 1 chip (no more supported) */
	3600000,	/* 3.600000 MHz ? (partially supported) */
    { 255 }         /* (not supported) */
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
		    CPU_M6809,
		    3000000,        /* 3 Mhz ??? */
		    0,
		    readmem,writemem,0,0,
		    nmi_interrupt,1
		},
		{
		    CPU_M6502 | CPU_AUDIO_CPU,
		    3000000,        /* 3 Mhz ??? */
		    2,
		    sound_readmem,sound_writemem,0,0,
		    interrupt,16 /* guessed value, altho sounds ok (check on power bar) */
		    /* nmi's are triggered from the main cpu */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	1, /* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	sidepocket_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
	    {
	        SOUND_YM2203,
	        &ym2203_interface
	    },
	    {
	        SOUND_YM3526,
	        &ym3526_interface
	    }
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( sidepckt_rom )
    ROM_REGION(0x10000)     /* 64k for code */
    ROM_LOAD( "sp_09.bin",    0x04000, 0x4000, 0x3c6fe54b )
    ROM_LOAD( "sp_08.bin",    0x08000, 0x8000, 0x347f81cd )

    ROM_REGION_DISPOSE(0x30000)     /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "sp_05.bin",    0x00000, 0x8000, 0x05ab71d2 ) /* characters */
    ROM_LOAD( "sp_06.bin",    0x08000, 0x8000, 0x580e4e43 ) /* characters */
    ROM_LOAD( "sp_07.bin",    0x10000, 0x8000, 0x9d6f7969 ) /* characters */
    ROM_LOAD( "sp_01.bin",    0x18000, 0x8000, 0xa2cdfbea ) /* sprites */
    ROM_LOAD( "sp_02.bin",    0x20000, 0x8000, 0xeeb5c3e7 ) /* sprites */
    ROM_LOAD( "sp_03.bin",    0x28000, 0x8000, 0x8e18d21d ) /* sprites */

    ROM_REGION(0x10000)     /* 64k for the audio cpu */
    ROM_LOAD( "sp_04.bin",    0x08000, 0x8000, 0xd076e62e )
ROM_END

struct GameDriver sidepckt_driver =
{
	__FILE__,
	0,
    "sidepckt",
    "Side Pocket",
	"1986",
	"Data East Corporation",
    "Ernesto Corvi\nMarc Vergoossen (hardware info)",
	0,
    &machine_driver,
	0,

    sidepckt_rom,
    0, 0,
    0,
    0,      /* sound_prom */

    input_ports,

    0, palette, colortable,
    ORIENTATION_DEFAULT,

    0, 0
};
