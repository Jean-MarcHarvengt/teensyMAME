/***************************************************************************

	Xenophobe, (c) 1987 Bally Midway, Game No. 0E85
	Spy Hunter 2 (Not working yet)
	Blasted (Not working yet)
	Zwackery (Not working yet)

    The timer chip (see machine/mcr68.c) needs to be fully emulated before
    the other games will work.

	Emulation by Bryan McPhail, mish@tendril.force9.net

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/6821pia.h"

void mcr68_videoram_w(int offset,int data);
void mcr68_paletteram_w(int offset,int data);

extern unsigned char *mcr68_spriteram;
int mcr68_videoram_r(int offset);
void mcr68_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void xenophobe_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void mcr68_init_machine(void);
int mcr68_interrupt(void);

void mcr_soundstatus_w (int offset,int data);
int mcr_soundlatch_r (int offset);
void mcr_pia_1_w (int offset, int data);
int mcr_pia_1_r (int offset);

int mcr68_control_0(int offset);
int mcr68_control_1(int offset);
int mcr68_control_2(int offset);
int mcr68_6840_r(int offset);
void mcr68_6840_w(int offset,int data);
void mcr68_sg_w(int offset,int data);

/***************************************************************************

  Memory maps

***************************************************************************/

static struct MemoryReadAddress mcr68_readmem[] =
{
	{ 0x00000, 0x3ffff, MRA_ROM },
	{ 0x60000, 0x63fff, MRA_BANK1 }, /* Main RAM */
	{ 0x70000, 0x70fff, mcr68_videoram_r },
	{ 0x71000, 0x71fff, MRA_BANK2 },
	{ 0x80000, 0x80fff, MRA_BANK3 },
	{ 0x90000, 0x900ff, paletteram_word_r },
	{ 0xa0000, 0xa000f, mcr68_6840_r },
	{ 0xd0000, 0xd0001, mcr68_control_0 },
	{ 0xe0000, 0xe0001, mcr68_control_1 },
	{ 0xf0000, 0xf0001, mcr68_control_2 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress mcr68_writemem[] =
{
	{ 0x00000, 0x3ffff, MWA_ROM },
	{ 0x60000, 0x63fff, MWA_BANK1 }, /* Main RAM */
	{ 0x70000, 0x70fff, mcr68_videoram_w, &videoram, &videoram_size },
	{ 0x71000, 0x71fff, MWA_BANK2 },
	{ 0x80000, 0x80fff, MWA_BANK3, &mcr68_spriteram },
	{ 0x90000, 0x900ff, mcr68_paletteram_w, &paletteram },
	{ 0xa0000, 0xa000f, mcr68_6840_w },
	{ 0xb0000, 0xb0001, MWA_NOP }, /* Watchdog is 0xb0000 */
	{ 0xc0000, 0xc0001, mcr68_sg_w }, /* sound cpu */
	{ -1 }  /* end of table */
};

/* Not finished yet */
static struct MemoryReadAddress zwackery_readmem[] =
{
	{ 0x00000, 0x3ffff, MRA_ROM },
	{ 0x80000, 0x80fff, MRA_BANK1 }, /* Main RAM */

	{ 0x70000, 0x70fff, mcr68_videoram_r },
	{ 0x71000, 0x71fff, MRA_BANK2 },
//	{ 0x80000, 0x80fff, MRA_BANK3 },
	{ 0x90000, 0x900ff, paletteram_word_r },
	{ 0xa0000, 0xa000f, mcr68_6840_r },
	{ 0xd0000, 0xd0001, mcr68_control_0 },
	{ 0xe0000, 0xe0001, mcr68_control_1 },
	{ 0xf0000, 0xf0001, mcr68_control_2 },

	{ 0x104000, 0x14000f, MRA_NOP },

	{ 0xc00000, 0xc00fff, MRA_BANK2 },

	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress zwackery_writemem[] =
{
	{ 0x00000, 0x3ffff, MWA_ROM },
	{ 0x80000, 0x80fff, MWA_BANK1 }, /* Main RAM */

	{ 0x70000, 0x70fff, mcr68_videoram_w, &videoram, &videoram_size },
	{ 0x71000, 0x71fff, MWA_BANK2 },
//	{ 0x80000, 0x80fff, MWA_BANK3, &mcr68_spriteram },

	{ 0x90000, 0x900ff, mcr68_paletteram_w, &paletteram },
	{ 0xa0000, 0xa000f, mcr68_6840_w },
	{ 0xb0000, 0xb0001, MWA_NOP }, /* Watchdog is 0xb0000 */
	{ 0xc0000, 0xc0001, mcr68_sg_w }, /* sound cpu */

	{ 0x104000, 0x14000f, MWA_NOP },

	{ 0xc00000, 0xc00fff, MWA_BANK2 },


	{ -1 }  /* end of table */
};

/* Xenophobe Sounds Good board */
static struct MemoryReadAddress sg_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x060000, 0x060007, mcr_pia_1_r },
	{ 0x070000, 0x070fff, MRA_BANK8 },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sg_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x060000, 0x060007, mcr_pia_1_w },
	{ 0x070000, 0x070fff, MWA_BANK8 },
	{ -1 }	/* end of table */
};

/***************************************************************************

  Input port definitions

***************************************************************************/

INPUT_PORTS_START( xenophobe_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* actually tilt? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )  /* actually service..? */
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x04, 0x04, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPNAME( 0x08, 0x08, "Coins per Life Unit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPNAME( 0x10, 0x10, "Life Unit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2000" )
	PORT_DIPSETTING(    0x10, "1000" )
	PORT_DIPNAME( 0x20, 0x20, "Attract Music", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0xC0, 0xC0, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Medium" )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x80, "Hard" )
	PORT_DIPSETTING(    0xC0, "Medium" )
INPUT_PORTS_END

INPUT_PORTS_START( spyhunt2_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )  //test
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START
 	PORT_ANALOGX( 0xff, 0x74, IPT_AD_STICK_X | IPF_CENTER, 100, 0, 0x20, 0xff, OSD_KEY_LEFT, OSD_KEY_RIGHT, IPT_JOYSTICK_LEFT, IPT_JOYSTICK_RIGHT, 2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) /* Left Trigger */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) /* Left Button */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 ) /* Right Trigger */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 ) /* Right Button */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )

	PORT_START //to do
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )


	PORT_START	/* IN3 -- dipswitches */
//	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )/
//	PORT_DIPSETTING(    0x02, "Easy" )
//	PORT_DIPSETTING(    0x03, "Normal" )

		PORT_DIPNAME( 0x01, 0x01, "Unknown 1", IP_KEY_NONE )
		PORT_DIPSETTING(    0x00, "a1" )
		PORT_DIPSETTING(    0x01, "A2" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2", IP_KEY_NONE )
		PORT_DIPSETTING(    0x00, "a1" )
		PORT_DIPSETTING(    0x02, "A2" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 3 (FREE PLAY)", IP_KEY_NONE )
		PORT_DIPSETTING(    0x00, "a1" )
		PORT_DIPSETTING(    0x04, "A2" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 4", IP_KEY_NONE )
		PORT_DIPSETTING(    0x00, "a1" )
		PORT_DIPSETTING(    0x08, "A2" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 5", IP_KEY_NONE )
		PORT_DIPSETTING(    0x00, "a1" )
		PORT_DIPSETTING(    0x10, "A2" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 6", IP_KEY_NONE )
		PORT_DIPSETTING(    0x00, "a1" )
		PORT_DIPSETTING(    0x20, "A2" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 7", IP_KEY_NONE )
		PORT_DIPSETTING(    0x00, "a1" )
		PORT_DIPSETTING(    0x40, "A2" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 8", IP_KEY_NONE )
		PORT_DIPSETTING(    0x00, "a1" )
		PORT_DIPSETTING(    0x80, "A2" )

//	PORT_DIPSETTING(    0x01, "Hard" )
//	PORT_DIPSETTING(    0x00, "Free Play" )


	/* Free play in Spy 2 *
	PORT_DIPNAME( 0x04, 0x04, "Score Option", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Keep score when continuing" )
	PORT_DIPSETTING(    0x00, "Lose score when continuing" )



	PORT_DIPNAME( 0x08, 0x08, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x70, 0x70, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x70, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x60, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x50, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/6 Credits" )
	PORT_BITX( 0x80,    0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Advance", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

*/

INPUT_PORTS_END

/***************************************************************************/

/* note that characters are half the resolution of sprites in each direction, so we generate
   them at double size */

static struct GfxLayout mcr68_charlayout_2048 =
{
	16,16,
	2048,
	4,
	{ 0,1,2048*16*8,2048*16*8+1 },
	{ 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14 },
	{ 0, 0, 2*8, 2*8, 4*8, 4*8, 6*8, 6*8, 8*8, 8*8, 10*8, 10*8, 12*8, 12*8, 14*8, 14*8 },
	16*8
};


/* 512 sprites; used by Rampage, Xenophobe */
#define X (512*128*8)
#define Y (2*X)
#define Z (3*X)
static struct GfxLayout mcr3_spritelayout_512 =
{
   32,32,
   512,
   4,
   { 0, 1, 2, 3 },
   {  Z+0, Z+4, Y+0, Y+4, X+0, X+4, 0, 4, Z+8, Z+12, Y+8, Y+12, X+8, X+12, 8, 12,
      Z+16, Z+20, Y+16, Y+20, X+16, X+20, 16, 20, Z+24, Z+28, Y+24, Y+28,
      X+24, X+28, 24, 28 },
	 {  0, 32, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7, 32*8, 32*9, 32*10, 32*11,
      32*12, 32*13, 32*14, 32*15, 32*16, 32*17, 32*18, 32*19, 32*20, 32*21,
      32*22, 32*23, 32*24, 32*25, 32*26, 32*27, 32*28, 32*29, 32*30, 32*31 },
   128*8
};
#undef X
#undef Y
#undef Z


static struct GfxDecodeInfo xenophobe_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mcr68_charlayout_2048,  0, 4 },
	{ 1, 0x10000, &mcr3_spritelayout_512, 0, 4 },
	{ -1 } /* end of array */
};

/***************************************************************************

  Sound interfaces

***************************************************************************/

static struct DACinterface dac_interface =
{
	1,
	{ 255 }
};

/***************************************************************************

	Machine drivers

***************************************************************************/

static struct MachineDriver mcr68_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 Mhz */
			0,
			mcr68_readmem,mcr68_writemem,0,0,
			mcr68_interrupt,4
		},
		{
			CPU_M68000 | CPU_AUDIO_CPU,
			8000000,
			2,
			sg_readmem,sg_writemem,0,0,
			ignore_interrupt,0
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - sound CPU synchronization is done via timers */
	mcr68_init_machine,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	xenophobe_gfxdecodeinfo,
	8*16, 8*16,
	mcr68_vh_convert_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	xenophobe_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver zwackery_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 5 Mhz */
			0,
			zwackery_readmem,zwackery_writemem,0,0,
			mcr68_interrupt,1
		},
		{
			CPU_M68000 | CPU_AUDIO_CPU,
			7500000,	/* 7.5 Mhz */
			2,
			sg_readmem,sg_writemem,0,0,
			ignore_interrupt,1
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - sound CPU synchronization is done via timers */
	0, /* No INIT machine */

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	xenophobe_gfxdecodeinfo,
	8*16, 8*16,
	mcr68_vh_convert_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	xenophobe_vh_screenrefresh,

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

  Game driver(s)

***************************************************************************/

ROM_START( xenophob_rom )
	ROM_REGION(0x40000)
	ROM_LOAD_EVEN( "xeno_pro.3c",  0x00000, 0x10000, 0xf44c2e60 )
	ROM_LOAD_ODD ( "xeno_pro.3b",  0x00000, 0x10000, 0x01609a3b )
	ROM_LOAD_EVEN( "xeno_pro.2c",  0x20000, 0x10000, 0xe45bf669 )
	ROM_LOAD_ODD ( "xeno_pro.2b",  0x20000, 0x10000, 0xda5d39d5 )

	ROM_REGION_DISPOSE(0x50000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "xeno_bg.12d",  0x00000, 0x08000, 0xc32288b1 )
	ROM_LOAD( "xeno_bg.11d",  0x08000, 0x08000, 0x3d2cf284 )
	ROM_LOAD( "xeno_fg.10j",  0x10000, 0x10000, 0x6a7a3516 )
	ROM_LOAD( "xeno_fg.9j",   0x20000, 0x10000, 0x82fb3e09 )
	ROM_LOAD( "xeno_fg.8j",   0x30000, 0x10000, 0x20e682f5 )
	ROM_LOAD( "xeno_fg.7j",   0x40000, 0x10000, 0xb12eddb2 )

	ROM_REGION(0x40000)  /* Sounds Good board */
	ROM_LOAD_EVEN( "xeno_snd.u7",  0x00000, 0x10000, 0x77561d15 )
	ROM_LOAD_ODD ( "xeno_snd.u17", 0x00000, 0x10000, 0x837a1a71 )
	ROM_LOAD_EVEN( "xeno_snd.u8",  0x20000, 0x10000, 0x6e2915c7 )
	ROM_LOAD_ODD ( "xeno_snd.u18", 0x20000, 0x10000, 0x12492145 )
ROM_END

ROM_START( spyhunt2_rom )
	ROM_REGION(0x40000)
	ROM_LOAD_EVEN( "3c",           0x0000, 0x10000, 0x0 )
	ROM_LOAD_ODD ( "3b",           0x0000, 0x10000, 0x0 )
	ROM_LOAD_EVEN( "2c",           0x20000, 0x10000, 0x0 )
	ROM_LOAD_ODD ( "2b",           0x20000, 0x10000, 0x0 )

	ROM_REGION_DISPOSE(0x90000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bg1.12d",      0x00000, 0x8000, 0x0 )
	ROM_LOAD( "bg0.11d",      0x08000, 0x8000, 0x0 )
	ROM_LOAD( "fg3.10j",      0x10000, 0x20000, 0x0 )
	ROM_LOAD( "fg2.9j",       0x30000, 0x20000, 0x0 )
	ROM_LOAD( "fg1.8j",       0x50000, 0x20000, 0x0 )
	ROM_LOAD( "fg0.7j",       0x70000, 0x20000, 0x0 )

	ROM_REGION(0x20000)  /* Sound */

ROM_END

ROM_START( blasted_rom )
	ROM_REGION(0x40000)
	ROM_LOAD_EVEN( "3c",           0x0000, 0x10000, 0x0 )
	ROM_LOAD_ODD ( "3b",           0x0000, 0x10000, 0x0 )
	ROM_LOAD_EVEN( "2c",           0x20000, 0x10000, 0x0 )
	ROM_LOAD_ODD ( "2b",           0x20000, 0x10000, 0x0 )

	ROM_REGION_DISPOSE(0x50000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "12d",          0x00000, 0x8000, 0x0 )
	ROM_LOAD( "11d",          0x08000, 0x8000, 0x0 )
	ROM_LOAD( "fg0",          0x10000, 0x10000, 0x0 )
	ROM_LOAD( "fg1",          0x20000, 0x10000, 0x0 )
	ROM_LOAD( "fg2",          0x30000, 0x10000, 0x0 )
	ROM_LOAD( "fg3",          0x40000, 0x10000, 0x0 )

	ROM_REGION(0x20000)  /* Unknown */

ROM_END

ROM_START( zwackery_rom )
	ROM_REGION(0x40000)
	ROM_LOAD_EVEN( "pro0.bin",     0x0000, 0x4000, 0x0 )
	ROM_LOAD_ODD ( "pro1.bin",     0x0000, 0x4000, 0x0 )
	ROM_LOAD_EVEN( "pro2.bin",     0x8000, 0x4000, 0x0 )
	ROM_LOAD_ODD ( "pro3.bin",     0x8000, 0x4000, 0x0 )
	ROM_LOAD_EVEN( "pro4.bin",     0x10000, 0x4000, 0x0 )
	ROM_LOAD_ODD ( "pro5.bin",     0x10000, 0x4000, 0x0 )
	ROM_LOAD_EVEN( "pro6.bin",     0x18000, 0x4000, 0x0 )
	ROM_LOAD_ODD ( "pro7.bin",     0x18000, 0x4000, 0x0 )
	ROM_LOAD_EVEN( "pro8.bin",     0x20000, 0x4000, 0x0 )
	ROM_LOAD_ODD ( "pro9.bin",     0x20000, 0x4000, 0x0 )
	ROM_LOAD_EVEN( "pro10.bin",    0x28000, 0x4000, 0x0 )
	ROM_LOAD_ODD ( "pro11.bin",    0x28000, 0x4000, 0x0 )
	ROM_LOAD_EVEN( "pro12.bin",    0x30000, 0x4000, 0x0 )
	ROM_LOAD_ODD ( "pro13.bin",    0x30000, 0x4000, 0x0 )

	ROM_REGION_DISPOSE(0x50000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tilee.bin",    0x00000, 0x4000, 0x0 )
	ROM_LOAD( "tilef.bin",    0x04000, 0x4000, 0x0 )
	ROM_LOAD( "tileg.bin",    0x08000, 0x4000, 0x0 )
	ROM_LOAD( "tileh.bin",    0x0C000, 0x4000, 0x0 )

	ROM_LOAD( "spr6h.bin",    0x10000, 0x4000, 0x0 )
	ROM_LOAD( "spr6j.bin",    0x14000, 0x4000, 0x0 )
	ROM_LOAD( "spr7h.bin",    0x18000, 0x4000, 0x0 )
	ROM_LOAD( "spr7j.bin",    0x1C000, 0x4000, 0x0 )
	ROM_LOAD( "spr10h.bin",   0x20000, 0x4000, 0x0 )
	ROM_LOAD( "spr10j.bin",   0x24000, 0x4000, 0x0 )
	ROM_LOAD( "spr11h.bin",   0x28000, 0x4000, 0x0 )
	ROM_LOAD( "spr11j.bin",   0x2C000, 0x4000, 0x0 )

	ROM_REGION(0x20000)

	/* CSD roms */

ROM_END

struct GameDriver xenophob_driver =
{
	__FILE__,
	0,
	"xenophob",
	"Xenophobe",
	"1987",
	"Bally Midway",
	"Bryan McPhail\n",
	0,
	&mcr68_machine_driver,
	0,

	xenophob_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	xenophobe_input_ports,

	0, 0,0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver spyhunt2_driver =
{
	__FILE__,
	0,
	"spyhunt2",
	"Spy Hunter 2",
	"????",
	"?????",
	"Bryan McPhail\n",
	0,
	&mcr68_machine_driver,
	0,

	spyhunt2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	spyhunt2_input_ports,

	0, 0,0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver blasted_driver =
{
	__FILE__,
	0,
	"blasted",
	"Blasted",
	"????",
	"?????",
	"Bryan McPhail\n",
	0,
	&mcr68_machine_driver,
	0,

	blasted_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	xenophobe_input_ports,

	0, 0,0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver zwackery_driver =
{
	__FILE__,
	0,
	"zwackery",
	"Zwackery",
	"????",
	"?????",
	"Bryan McPhail\n",
	0,
	&zwackery_machine_driver,
	0,

	zwackery_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	xenophobe_input_ports,

	0, 0,0,
	ORIENTATION_DEFAULT,

	0,0
};

