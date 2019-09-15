/***************************************************************************

Rally X memory map (preliminary)

0000-3fff ROM
8000-83ff Radar video RAM + other
8400-87ff video RAM
8800-8bff Radar color RAM + other
8c00-8fff color RAM
9800-9fff RAM

memory mapped ports:

read:
a000      IN0
a080      IN1
a100      DSW1

*
 * IN0 (all bits are inverted)
 * bit 7 : ?
 * bit 6 : START 1
 * bit 5 : UP player 1
 * bit 4 : DOWN player 1
 * bit 3 : RIGHT player 1
 * bit 2 : LEFT player 1
 * bit 1 : SMOKE player 1
 * bit 0 : CREDIT
 *
*
 * IN1 (all bits are inverted)
 * bit 7 : ?
 * bit 6 : START 2
 * bit 5 : UP player 2 (TABLE only)
 * bit 4 : DOWN player 2 (TABLE only)
 * bit 3 : RIGHT player 2 (TABLE only)
 * bit 2 : LEFT player 2 (TABLE only)
 * bit 1 : SMOKE player 2 (TABLE only)
 * bit 0 : COCKTAIL or UPRIGHT cabinet 1 = UPRIGHT
 *
*
 * DSW1 (all bits are inverted)
 * bit 7 :\ 00 = free play      01 = 2 coins 1 play
 * bit 6 :/ 10 = 1 coin 2 play  11 = 1 coin 1 play
 * bit 5 :\ xxx = cars,rank:
 * bit 4 :| 000 = 2,A  001 = 3,A  010 = 1,B  011 = 2,B
 * bit 3 :/ 100 = 3,B  101 = 1,C  110 = 2,C  111 = 3,C
 * bit 2 :  0 = bonus at 10(1 car)/15(2 cars)/20(3 cars)  1 = bonus at 30/40/60
 * bit 1 :  1 = bonus at xxx points  0 = no bonus
 * bit 0 :  TEST
 *

write:
8014-801f sprites - 6 pairs: code (including flipping) and X position
8814-881f sprites - 6 pairs: Y position and color
8034-880c radar car indicators x position
8834-883c radar car indicators y position
a004-a00c radar car indicators color and x position MSB
a080      watchdog reset?
a105      sound voice 1 waveform (nibble)
a111-a113 sound voice 1 frequency (nibble)
a115      sound voice 1 volume (nibble)
a10a      sound voice 2 waveform (nibble)
a116-a118 sound voice 2 frequency (nibble)
a11a      sound voice 2 volume (nibble)
a10f      sound voice 3 waveform (nibble)
a11b-a11d sound voice 3 frequency (nibble)
a11f      sound voice 3 volume (nibble)
a130      virtual screen X scroll position
a140      virtual screen Y scroll position
a170      ? this is written to A LOT of times every frame
a180      explosion sound trigger
a181      interrupt enable
a182      ?
a183      flip screen
a184      1 player start lamp
a185      2 players start lamp
a186      ?

I/O ports:
OUT on port $0 sets the interrupt vector/instruction (the game uses both
IM 2 and IM 0)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



void pengo_sound_w(int offset,int data);
extern unsigned char *pengo_soundregs;

extern unsigned char *rallyx_videoram2,*rallyx_colorram2;
extern unsigned char *rallyx_radarcarx,*rallyx_radarcary,*rallyx_radarcarcolor;
extern int rallyx_radarram_size;
extern unsigned char *rallyx_scrollx,*rallyx_scrolly;
void rallyx_videoram2_w(int offset,int data);
void rallyx_colorram2_w(int offset,int data);
void rallyx_flipscreen_w(int offset,int data);
void rallyx_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int rallyx_vh_start(void);
void rallyx_vh_stop(void);
void rallyx_init(void);
void rallyx_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static void rallyx_play_sound_w(int offset, int data)
{
	static int last;


	if (data == 0 && last != 0)
		sample_start(0,0,0);

	last = data;
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9800, 0x9fff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa080, 0xa080, input_port_1_r },	/* IN1 */
	{ 0xa100, 0xa100, input_port_2_r },	/* DSW1 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, rallyx_videoram2_w, &rallyx_videoram2 },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, rallyx_colorram2_w, &rallyx_colorram2 },
	{ 0x9800, 0x9fff, MWA_RAM },
	{ 0xa004, 0xa00f, MWA_RAM, &rallyx_radarcarcolor },
	{ 0xa080, 0xa080, watchdog_reset_w },
	{ 0xa100, 0xa11f, pengo_sound_w, &pengo_soundregs },
	{ 0xa130, 0xa130, MWA_RAM, &rallyx_scrollx },
	{ 0xa140, 0xa140, MWA_RAM, &rallyx_scrolly },
	{ 0xa170, 0xa170, MWA_NOP },	/* ????? */
	{ 0xa180, 0xa180, rallyx_play_sound_w },
	{ 0xa181, 0xa181, interrupt_enable_w },
	{ 0xa183, 0xa183, rallyx_flipscreen_w },
	{ 0xa184, 0xa185, osd_led_w },
	{ 0xa186, 0xa186, MWA_NOP },
	{ 0x8014, 0x801f, MWA_RAM, &spriteram, &spriteram_size },	/* these are here just to initialize */
	{ 0x8814, 0x881f, MWA_RAM, &spriteram_2 },	/* the pointers. */
	{ 0x8034, 0x803f, MWA_RAM, &rallyx_radarcarx, &rallyx_radarram_size },	/* ditto */
	{ 0x8834, 0x883f, MWA_RAM, &rallyx_radarcary },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0, 0, interrupt_vector_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( rallyx_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT |IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* DSW0 */
	PORT_BITX(    0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* the bonus score depends on the number of lives */
	PORT_DIPNAME( 0x02, 0x02, "Bonus Life Score", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Low" )
	PORT_DIPSETTING(    0x02, "High" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Not Awarded" )
	PORT_DIPSETTING(    0x04, "Awarded" )
	PORT_DIPNAME( 0x38, 0x08, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "1 Car, Medium" )
	PORT_DIPSETTING(    0x28, "1 Car, Hard" )
	PORT_DIPSETTING(    0x00, "2 Cars, Easy" )
	PORT_DIPSETTING(    0x18, "2 Cars, Medium" )
	PORT_DIPSETTING(    0x30, "2 Cars, Hard" )
	PORT_DIPSETTING(    0x08, "3 Cars, Easy" )
	PORT_DIPSETTING(    0x20, "3 Cars, Medium" )
	PORT_DIPSETTING(    0x38, "3 Cars, Hard" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
INPUT_PORTS_END

INPUT_PORTS_START( nrallyx_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT |IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* DSW0 */
	PORT_BITX(    0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* the bonus score depends on the number of lives */
	PORT_DIPNAME( 0x02, 0x02, "Bonus Life Score", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Low" )
	PORT_DIPSETTING(    0x02, "High" )
	PORT_DIPNAME( 0x04, 0x04, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Not Awarded" )
	PORT_DIPSETTING(    0x04, "Awarded" )
	PORT_DIPNAME( 0x38, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "1 Car, Medium" )
	PORT_DIPSETTING(    0x28, "1 Car, Hard" )
	PORT_DIPSETTING(    0x18, "2 Cars, Medium" )
	PORT_DIPSETTING(    0x30, "2 Cars, Hard" )
	PORT_DIPSETTING(    0x00, "3 Cars, Easy" )
	PORT_DIPSETTING(    0x20, "3 Cars, Medium" )
	PORT_DIPSETTING(    0x38, "3 Cars, Hard" )
	PORT_DIPSETTING(    0x08, "4 Cars, Easy" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },	/* bits are packed in groups of four */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,	/* bits are packed in groups of four */
			 24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 bytes */
};
static struct GfxLayout radardotlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	2,2,	/* 2*2 square */
	1,	/* just one */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 0 },	/* I "know" that this bit of the */
	{ 0, 0 },	/* graphics ROMs is 1 */
	0	/* no use */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,        0, 64 },
	{ 1, 0x0000, &spritelayout,      0, 64 },
	{ 1, 0x0000, &radardotlayout, 64*4, 4 },
	{ -1 } /* end of array */
};



static struct namco_interface namco_interface =
{
	3072000/32,	/* sample rate */
	3,			/* number of voices */
	32,			/* gain adjustment */
	255,		/* playback volume */
	3			/* memory region */
};

static struct Samplesinterface samples_interface =
{
	1	/* 1 channel */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz ? */
			0,
			readmem,writemem,0,writeport,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	36*8, 28*8, { 0*8, 36*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	32,64*4+4*2,
	rallyx_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	rallyx_vh_start,
	rallyx_vh_stop,
	rallyx_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_NAMCO,
			&namco_interface
		},
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( rallyx_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1b",           0x0000, 0x1000, 0x5882700d )
	ROM_LOAD( "1e",           0x1000, 0x1000, 0x786585ec )
	ROM_LOAD( "1h",           0x2000, 0x1000, 0x110d7dcd )
	ROM_LOAD( "1k",           0x3000, 0x1000, 0x473ab447 )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "8e",           0x0000, 0x1000, 0x277c1de5 )

	ROM_REGION(0x0120)	/* color proms */
	ROM_LOAD( "m3-7603.11n",  0x0000, 0x0020, 0xc7865434 )
	ROM_LOAD( "im5623.8p",    0x0020, 0x0100, 0x834d4fda )

	ROM_REGION(0x0100)	/* sound prom */
	ROM_LOAD( "im5623.3p",    0x0000, 0x0100, 0x4bad7017 )
ROM_END

ROM_START( nrallyx_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "nrallyx.1b",   0x0000, 0x1000, 0x9404c8d6 )
	ROM_LOAD( "nrallyx.1e",   0x1000, 0x1000, 0xac01bf3f )
	ROM_LOAD( "nrallyx.1h",   0x2000, 0x1000, 0xaeba29b5 )
	ROM_LOAD( "nrallyx.1k",   0x3000, 0x1000, 0x78f17da7 )

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "nrallyx.8e",   0x0000, 0x1000, 0xca7a174a )

	ROM_REGION(0x0120)	/* color proms */
	ROM_LOAD( "nrallyx.pr1",  0x0000, 0x0020, 0xa0a49017 )
	ROM_LOAD( "nrallyx.pr2",  0x0020, 0x0100, 0xb2b7ca15 )

	ROM_REGION(0x0100)	/* sound prom */
	ROM_LOAD( "nrallyx.spr",  0x0000, 0x0100, 0xb75c4e87 )
ROM_END



static const char *rallyx_sample_names[] =
{
	"*rallyx",
	"BANG.SAM",
	0	/* end of array */
};



static int hiload(void)     /* V.V */
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8060],"\x00\x00\x00\x30\x40\x40\x40\x02",8) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x08060],8);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */

}


static void hisave(void)    /* V.V */
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x08060],8);
		osd_fclose(f);
	}
}



struct GameDriver rallyx_driver =
{
	__FILE__,
	0,
	"rallyx",
	"Rally X",
	"1980",
	"[Namco] (Midway license)",
	"Nicola Salmoria (MAME driver)\nMirko Buffoni (bang sound)\nMarco Cassili\nGary Walton (color info)\nSimon Walls (color info)",
	0,
	&machine_driver,
	rallyx_init,

	rallyx_rom,
	0, 0,
	rallyx_sample_names,
	0,	/* sound_prom */

	rallyx_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver nrallyx_driver =
{
	__FILE__,
	0,
	"nrallyx",
	"New Rally X",
	"1981",
	"Namco",
	"Nicola Salmoria (MAME driver)\nMirko Buffoni (bang sound)\nMarco Cassili",
	0,
	&machine_driver,
	rallyx_init,

	nrallyx_rom,
	0, 0,
	rallyx_sample_names,
	0,	/* sound_prom */

	nrallyx_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
