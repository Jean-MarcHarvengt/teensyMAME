/***************************************************************************

I'm in no mood to write documentation now, so if you have any questions
about this driver, please address them to Pat Lawrence <pjl@ns.net>.  I'll
be happy to help you out any way I can.

Crystal Castles memory map.

 Address  A A A A A A A A A A A A A A A A  R  D D D D D D D D  Function
          1 1 1 1 1 1 9 8 7 6 5 4 3 2 1 0  /  7 6 5 4 3 2 1 0
          5 4 3 2 1 0                      W
-------------------------------------------------------------------------------
0000      X X X X X X X X X X X X X X X X  W  X X X X X X X X  X Coordinate
0001      0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1  W  D D D D D D D D  Y Coordinate
0002      0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 R/W D D D D          Bit Mode
0003-0BFF 0 0 0 0 A A A A A A A A A A A A R/W D D D D D D D D  RAM (DRAM)
0C00-7FFF 0 A A A A A A A A A A A A A A A R/W D D D D D D D D  Screen RAM
8000-8DFF 1 0 0 0 A A A A A A A A A A A A R/W D D D D D D D D  RAM (STATIC)
8E00-8EFF 1 0 0 0 1 1 1 0 A A A A A A A A R/W D D D D D D D D  MOB BUF 2
-------------------------------------------------------------------------------
8F00-8FFF 1 0 0 0 1 1 1 1 A A A A A A A A R/W D D D D D D D D  MOB BUF 1
                                      0 0 R/W D D D D D D D D  MOB Picture
                                      0 1 R/W D D D D D D D D  MOB Vertical
                                      1 0 R/W D D D D D D D D  MOB Priority
                                      1 1 R/W D D D D D D D D  MOB Horizontal
-------------------------------------------------------------------------------
9000-90FF 1 0 0 1 0 0 X X A A A A A A A A R/W D D D D D D D D  NOVRAM
9400-9401 1 0 0 1 0 1 0 X X X X X X X 0 A  R                   TRAK-BALL 1
9402-9403 1 0 0 1 0 1 0 X X X X X X X 1 A  R                   TRAK-BALL 2
9500-9501 1 0 0 1 0 1 0 X X X X X X X X A  R                   TRAK-BALL 1 mirror
9600      1 0 0 1 0 1 1 X X X X X X X X X  R                   IN0
                                           R                D  COIN R
                                           R              D    COIN L
                                           R            D      COIN AUX
                                           R          D        SLAM
                                           R        D          SELF TEST
                                           R      D            VBLANK
                                           R    D              JMP1
                                           R  D                JMP2
-------------------------------------------------------------------------------
9800-980F 1 0 0 1 1 0 0 X X X X X A A A A R/W D D D D D D D D  CI/O 0
9A00-9A0F 1 0 0 1 1 0 1 X X X X X A A A A R/W D D D D D D D D  CI/O 1
9A08                                                    D D D  Option SW
                                                      D        SPARE
                                                    D          SPARE
                                                  D            SPARE
9C00      1 0 0 1 1 1 0 0 0 X X X X X X X  W                   RECALL
-------------------------------------------------------------------------------
9C80      1 0 0 1 1 1 0 0 1 X X X X X X X  W  D D D D D D D D  H Scr Ctr Load
9D00      1 0 0 1 1 1 0 1 0 X X X X X X X  W  D D D D D D D D  V Scr Ctr Load
9D80      1 0 0 1 1 1 0 1 1 X X X X X X X  W                   Int. Acknowledge
9E00      1 0 0 1 1 1 1 0 0 X X X X X X X  W                   WDOG
          1 0 0 1 1 1 1 0 1 X X X X A A A  W                D  OUT0
9E80                                0 0 0  W                D  Trak Ball Light P1
9E81                                0 0 1  W                D  Trak Ball Light P2
9E82                                0 1 0  W                D  Store Low
9E83                                0 1 1  W                D  Store High
9E84                                1 0 0  W                D  Spare
9E85                                1 0 1  W                D  Coin Counter R
9E86                                1 1 0  W                D  Coin Counter L
9E87                                1 1 1  W                D  BANK0-BANK1
          1 0 0 1 1 1 1 1 0 X X X X A A A  W          D        OUT1
9F00                                0 0 0  W          D        ^AX
9F01                                0 0 1  W          D        ^AY
9F02                                0 1 0  W          D        ^XINC
9F03                                0 1 1  W          D        ^YINC
9F04                                1 0 0  W          D        PLAYER2 (flip screen)
9F05                                1 0 1  W          D        ^SIRE
9F06                                1 1 0  W          D        BOTHRAM
9F07                                1 1 1  W          D        BUF1/^BUF2 (sprite bank)
9F80-9FBF 1 0 0 1 1 1 1 1 1 X A A A A A A  W  D D D D D D D D  COLORAM
A000-FFFF 1 A A A A A A A A A A A A A A A  R  D D D D D D D D  Program ROM

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *screen_addr;
extern unsigned char *screen_inc;
extern unsigned char *screen_inc_enable;
extern unsigned char *sprite_bank;
extern unsigned char *ccastles_scrollx;
extern unsigned char *ccastles_scrolly;

void ccastles_paletteram_w(int offset,int data);
int ccastles_vh_start(void);
void ccastles_vh_stop(void);
void ccastles_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

int ccastles_bitmode_r(int offset);
void ccastles_bitmode_w(int offset, int data);

void ccastles_flipscreen_w(int offset,int data);



static void ccastles_led_w(int offset,int data)
{
	osd_led_w(offset,~data);
}

static void ccastles_bankswitch_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (data) { cpu_setbank(1,&RAM[0x10000]); }
	else { cpu_setbank(1,&RAM[0xa000]); }
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x0001, MRA_RAM },
	{ 0x0002, 0x0002, ccastles_bitmode_r },
	{ 0x0003, 0x90ff, MRA_RAM },	/* All RAM */
	{ 0x9400, 0x9400, input_port_2_r },	/* trackball y - player 1 */
	{ 0x9402, 0x9402, input_port_2_r },	/* trackball y - player 2 */
	{ 0x9500, 0x9500, input_port_2_r },	/* trackball y - player 1 mirror */
	{ 0x9401, 0x9401, input_port_3_r },	/* trackball x - player 1 */
	{ 0x9403, 0x9403, input_port_3_r },	/* trackball x - player 2 */
	{ 0x9501, 0x9501, input_port_3_r },	/* trackball x - player 1 mirror */
	{ 0x9600, 0x9600, input_port_0_r },	/* IN0 */
	{ 0x9800, 0x980f, pokey1_r }, /* Random # generator on a Pokey */
	{ 0x9a00, 0x9a0f, pokey2_r }, /* Random #, IN1 */
	{ 0xa000, 0xdfff, MRA_BANK1 },
	{ 0xe000, 0xffff, MRA_ROM },	/* ROMs/interrupt vectors */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0001, MWA_RAM, &screen_addr },
	{ 0x0002, 0x0002, ccastles_bitmode_w },
	{ 0x0003, 0x0bff, MWA_RAM },
	{ 0x0c00, 0x7fff, MWA_RAM, &videoram },
	{ 0x8000, 0x8dff, MWA_RAM },
	{ 0x8e00, 0x8eff, MWA_RAM, &spriteram_2, &spriteram_size },
	{ 0x8f00, 0x8fff, MWA_RAM, &spriteram },
	{ 0x9000, 0x90ff, MWA_RAM },  /* NVRAM */
	{ 0x9800, 0x980f, pokey1_w },
	{ 0x9a00, 0x9a0f, pokey2_w },
	{ 0x9c80, 0x9c80, MWA_RAM, &ccastles_scrollx },
	{ 0x9d00, 0x9d00, MWA_RAM, &ccastles_scrolly },
	{ 0x9d80, 0x9d80, MWA_NOP },
	{ 0x9e00, 0x9e00, MWA_NOP },
	{ 0x9e80, 0x9e81, ccastles_led_w },
	{ 0x9e85, 0x9e86, MWA_NOP },
	{ 0x9e87, 0x9e87, ccastles_bankswitch_w },
	{ 0x9f00, 0x9f01, MWA_RAM, &screen_inc_enable },
	{ 0x9f02, 0x9f03, MWA_RAM, &screen_inc },
	{ 0x9f04, 0x9f04, ccastles_flipscreen_w },
	{ 0x9f05, 0x9f06, MWA_RAM },
	{ 0x9f07, 0x9f07, MWA_RAM, &sprite_bank },
	{ 0x9f80, 0x9fbf, ccastles_paletteram_w },
	{ 0xa000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )				/* 1p Jump, non-cocktail start1 */
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) /* 2p Jump, non-cocktail start2 */

	PORT_START	/* IN1 */
	PORT_BIT ( 0x07, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_START1 )				/* cocktail only */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START2 )				/* cocktail only */
	PORT_DIPNAME (0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "Upright" )
	PORT_DIPSETTING (   0x20, "Cocktail" )
	PORT_BIT ( 0xc0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_ANALOGX( 0xff, 0x7f, IPT_TRACKBALL_Y | IPF_REVERSE, 10, 0, 0, 0, OSD_KEY_UP, OSD_KEY_DOWN, OSD_JOY_UP, OSD_JOY_DOWN, 30 )

	PORT_START	/* IN3 */
	PORT_ANALOGX( 0xff, 0x7f, IPT_TRACKBALL_X, 10, 0, 0, 0, OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_JOY_LEFT, OSD_JOY_RIGHT, 30 )
INPUT_PORTS_END

static struct GfxLayout ccastles_spritelayout =
{
	8,16,	/* 8*16 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel (the most significant bit is always 0) */
	{ 0x2000*8+0, 0x2000*8+4, 0, 4 },	/* the three bitplanes are separated */
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

/* there's nothing here, this is just a placeholder to let the video hardware */
/* pick the background color table. */
static struct GfxLayout fakelayout =
{
	1,1,
	0,
	4,	/* 4 bits per pixel */
	{ 0 },
	{ 0 },
	{ 0 },
	0
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &ccastles_spritelayout,  0, 1 },
	{ 0, 0,      &fakelayout,            16, 1 },
	{ -1 } /* end of array */
};




static struct POKEYinterface pokey_interface =
{
	2,	/* 2 chips */
	1250000,	/* 1.25 MHz??? */
	50,
	POKEY_DEFAULT_GAIN,
	NO_CLIP,
	/* The 8 pot handlers */
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	/* The allpot handler */
	{ 0, input_port_1_r },
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,4
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */

	0,
	256, 232, { 0, 255, 0, 231 },
	gfxdecodeinfo,
	32, 32,
	0,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	ccastles_vh_start,
	ccastles_vh_stop,
	ccastles_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_POKEY,
			&pokey_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(ccastles_rom)
     ROM_REGION(0x14000)	/* 64k for code */
     ROM_LOAD( "ccastles.303", 0xA000, 0x2000, 0x10e39fce )
     ROM_LOAD( "ccastles.304", 0xC000, 0x2000, 0x74510f72 )
     ROM_LOAD( "ccastles.305", 0xE000, 0x2000, 0x9418cf8a )
     ROM_LOAD( "ccastles.102", 0x10000, 0x2000, 0xf6ccfbd4 )	/* Bank switched ROMs */
     ROM_LOAD( "ccastles.101", 0x12000, 0x2000, 0xe2e17236 )	/* containing level data. */

     ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics */
     ROM_LOAD( "ccastles.107", 0x0000, 0x2000, 0x39960b7d )
     ROM_LOAD( "ccastles.106", 0x2000, 0x2000, 0x9d1d89fc )
ROM_END

ROM_START(ccastle2_rom)
     ROM_REGION(0x14000)	/* 64k for code */
     ROM_LOAD( "ccastles.203", 0xA000, 0x2000, 0x348a96f0 )
     ROM_LOAD( "ccastles.204", 0xC000, 0x2000, 0xd48d8c1f )
     ROM_LOAD( "ccastles.205", 0xE000, 0x2000, 0x0e4883cc )
     ROM_LOAD( "ccastles.102", 0x10000, 0x2000, 0xf6ccfbd4 )	/* Bank switched ROMs */
     ROM_LOAD( "ccastles.101", 0x12000, 0x2000, 0xe2e17236 )	/* containing level data. */

     ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics */
     ROM_LOAD( "ccastles.107", 0x0000, 0x2000, 0x39960b7d )
     ROM_LOAD( "ccastles.106", 0x2000, 0x2000, 0x9d1d89fc )
ROM_END



static int hiload(void)
{
	/* Read the NVRAM contents from disk */
	/* No check necessary */
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0x9000],0x100);
		osd_fclose(f);
	}
	return 1;
}



static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x9000],0x100);
		osd_fclose(f);
	}
}



struct GameDriver ccastles_driver =
{
	__FILE__,
	0,
	"ccastles",
	"Crystal Castles (set 1)",
	"1983",
	"Atari",
	"Pat Lawrence\nChris Hardy\nSteve Clynes\nNicola Salmoria\nBrad Oliver",
	0,
	&machine_driver,
	0,

	ccastles_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver ccastle2_driver =
{
	__FILE__,
	&ccastles_driver,
	"ccastle2",
	"Crystal Castles (set 2)",
	"1983",
	"Atari",
	"Pat Lawrence\nChris Hardy\nSteve Clynes\nNicola Salmoria\nBrad Oliver",
	0,
	&machine_driver,
	0,

	ccastle2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
