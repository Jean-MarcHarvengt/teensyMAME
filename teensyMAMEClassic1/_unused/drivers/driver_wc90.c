/*
World Cup 90 ( Tecmo ) driver
-----------------------------

Ernesto Corvi
(ernesto@imagina.com)

CPU #1 : Handles background & foreground tiles, controllers, dipswitches.
CPU #2 : Handles sprites and palette
CPU #3 : Audio.

Memory Layout:

CPU #1
0000-8000 ROM
8000-9000 RAM
a000-a800 Color Ram for background #1 tiles
a800-b000 Video Ram for background #1 tiles
c000-c800 Color Ram for background #2 tiles
c800-c000 Video Ram for background #2 tiles
e000-e800 Color Ram for foreground tiles
e800-f000 Video Ram for foreground tiles
f800-fc00 Common Ram with CPU #2
fc00-fc00 Stick 1 input port
fc02-fc02 Stick 2 input port
fc05-fc05 Start buttons and Coins input port
fc06-fc06 Dip Switch A
fc07-fc07 Dip Switch B

CPU #2
0000-c000 ROM
c000-d000 RAM
d000-d800 RAM Sprite Ram
e000-e800 RAM Palette Ram
f800-fc00 Common Ram with CPU #1

CPU #3
0000-0xc000 ROM
???????????

Notes:
-----
Dip switches mapping is not complete. ( Anyone has the manual handy? )
Other than that, everything else seems to be complete.
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80/Z80.h"

extern unsigned char *wc90_shared;

extern unsigned char *wc90_tile_colorram, *wc90_tile_videoram;
extern unsigned char *wc90_tile_colorram2, *wc90_tile_videoram2;


extern unsigned char *wc90_scroll0xlo, *wc90_scroll0xhi;
extern unsigned char *wc90_scroll1xlo, *wc90_scroll1xhi;
extern unsigned char *wc90_scroll2xlo, *wc90_scroll2xhi;

extern unsigned char *wc90_scroll0ylo, *wc90_scroll0yhi;
extern unsigned char *wc90_scroll1ylo, *wc90_scroll1yhi;
extern unsigned char *wc90_scroll2ylo, *wc90_scroll2yhi;

extern int wc90_tile_videoram_size;
extern int wc90_tile_videoram_size2;

int wc90_vh_start( void );
void wc90_vh_stop( void );
int wc90_tile_videoram_r( int offset );
void wc90_tile_videoram_w( int offset, int v );
int wc90_tile_colorram_r( int offset );
void wc90_tile_colorram_w( int offset, int v );
int wc90_tile_videoram2_r( int offset );
void wc90_tile_videoram2_w( int offset, int v );
int wc90_tile_colorram2_r( int offset );
void wc90_tile_colorram2_w( int offset, int v );
int wc90_shared_r ( int offset );
void wc90_shared_w( int offset, int v );
void wc90_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static void wc90_bankswitch_w( int offset,int data )
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + ( ( data & 0xf8 ) << 8 );
	cpu_setbank( 1,&RAM[bankaddress] );
}

static void wc90_bankswitch1_w( int offset,int data )
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];


	bankaddress = 0x10000 + ( ( data & 0xf8 ) << 8 );
	cpu_setbank( 2,&RAM[bankaddress] );
}

static void wc90_sound_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(2,Z80_NMI_INT);
}

static struct MemoryReadAddress wc90_readmem1[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_RAM }, /* Main RAM */
	{ 0xa000, 0xa7ff, wc90_tile_colorram_r }, /* bg 1 color ram */
	{ 0xa800, 0xafff, wc90_tile_videoram_r }, /* bg 1 tile ram */
	{ 0xb000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xc7ff, wc90_tile_colorram2_r }, /* bg 2 color ram */
	{ 0xc800, 0xcfff, wc90_tile_videoram2_r }, /* bg 2 tile ram */
	{ 0xd000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe7ff, colorram_r }, /* fg color ram */
	{ 0xe800, 0xefff, videoram_r }, /* fg tile ram */
	{ 0xf000, 0xf7ff, MRA_BANK1 },
	{ 0xf800, 0xfbff, wc90_shared_r },
	{ 0xfc00, 0xfc00, input_port_0_r }, /* Stick 1 */
	{ 0xfc02, 0xfc02, input_port_1_r }, /* Stick 2 */
	{ 0xfc05, 0xfc05, input_port_4_r }, /* Start & Coin */
	{ 0xfc06, 0xfc06, input_port_2_r }, /* DIP Switch A */
	{ 0xfc07, 0xfc07, input_port_3_r }, /* DIP Switch B */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress wc90_readmem2[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ 0xd800, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0xf000, 0xf7ff, MRA_BANK2 },
	{ 0xf800, 0xfbff, wc90_shared_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress wc90_writemem1[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x9fff, MWA_RAM },
	{ 0xa000, 0xa7ff, wc90_tile_colorram_w, &wc90_tile_colorram },
	{ 0xa800, 0xafff, wc90_tile_videoram_w, &wc90_tile_videoram, &wc90_tile_videoram_size },
	{ 0xb000, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc7ff, wc90_tile_colorram2_w, &wc90_tile_colorram2 },
	{ 0xc800, 0xcfff, wc90_tile_videoram2_w, &wc90_tile_videoram2, &wc90_tile_videoram_size2 },
	{ 0xd000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, colorram_w, &colorram },
	{ 0xe800, 0xefff, videoram_w, &videoram, &videoram_size },
	{ 0xf000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xfbff, wc90_shared_w, &wc90_shared },
	{ 0xfc02, 0xfc02, MWA_RAM, &wc90_scroll0ylo },
	{ 0xfc03, 0xfc03, MWA_RAM, &wc90_scroll0yhi },
	{ 0xfc06, 0xfc06, MWA_RAM, &wc90_scroll0xlo },
	{ 0xfc07, 0xfc07, MWA_RAM, &wc90_scroll0xhi },
	{ 0xfc22, 0xfc22, MWA_RAM, &wc90_scroll1ylo },
	{ 0xfc23, 0xfc23, MWA_RAM, &wc90_scroll1yhi },
	{ 0xfc26, 0xfc26, MWA_RAM, &wc90_scroll1xlo },
	{ 0xfc27, 0xfc27, MWA_RAM, &wc90_scroll1xhi },
	{ 0xfc42, 0xfc42, MWA_RAM, &wc90_scroll2ylo },
	{ 0xfc43, 0xfc43, MWA_RAM, &wc90_scroll2yhi },
	{ 0xfc46, 0xfc46, MWA_RAM, &wc90_scroll2xlo },
	{ 0xfc47, 0xfc47, MWA_RAM, &wc90_scroll2xhi },
	{ 0xfcc0, 0xfcc0, wc90_sound_command_w },
	{ 0xfcd0, 0xfcd0, MWA_NOP },	/* ??? */
	{ 0xfce0, 0xfce0, wc90_bankswitch_w },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress wc90_writemem2[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd800, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe7ff, paletteram_xxxxBBBBRRRRGGGG_swap_w, &paletteram },
	{ 0xf000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xfbff, wc90_shared_w },
	{ 0xfc00, 0xfc00, wc90_bankswitch1_w },
	{ 0xfc01, 0xfc01, MWA_NOP },	/* ??? */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, YM2203_status_port_0_r },
	{ 0xf801, 0xf801, YM2203_read_port_0_r },
	{ 0xf802, 0xf802, YM2203_status_port_1_r },
	{ 0xf803, 0xf803, YM2203_read_port_1_r },
	{ 0xfc00, 0xfc00, MRA_NOP }, /* ??? adpcm ??? */
	{ 0xfc10, 0xfc10, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2203_control_port_0_w },
	{ 0xf801, 0xf801, YM2203_write_port_0_w },
	{ 0xf802, 0xf802, YM2203_control_port_1_w },
	{ 0xf803, 0xf803, YM2203_write_port_1_w },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( wc90_input_ports )
	PORT_START	/* IN0 bit 0-5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 bit 0-5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSWA */
	PORT_DIPNAME( 0x0f, 0x0f, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "9 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "8 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "7 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "6 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x06, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0e, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x09, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "Easy" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, "Countdown Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START	/* DSWB */
	PORT_DIPNAME( 0x03, 0x03, "1 Player Game Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "1:00" )
	PORT_DIPSETTING(    0x02, "1:30" )
	PORT_DIPSETTING(    0x03, "2:00" )
	PORT_DIPSETTING(    0x00, "2:30" )
	PORT_DIPNAME( 0x1c, 0x1c, "2 Players Game Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "1:00" )
	PORT_DIPSETTING(    0x14, "1:30" )
	PORT_DIPSETTING(    0x04, "2:00" )
	PORT_DIPSETTING(    0x18, "2:30" )
	PORT_DIPSETTING(    0x1c, "3:00" )
	PORT_DIPSETTING(    0x08, "3:30" )
	PORT_DIPSETTING(    0x10, "4:00" )
	PORT_DIPSETTING(    0x00, "5:00" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x80, "Japanese" )

	PORT_START	/* IN2 bit 0-3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 1024 characters */
	4,	/* 8 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 1024 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	4096,	/* 1024 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 1024*256*8+0*4, 1024*256*8+1*4, 2*4, 3*4, 1024*256*8+2*4, 1024*256*8+3*4,
			16*8+0*4, 16*8+1*4, 1024*256*8+16*8+0*4, 1024*256*8+16*8+1*4, 16*8+2*4, 16*8+3*4, 1024*256*8+16*8+2*4, 1024*256*8+16*8+3*4 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	64*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,      	1*16*16, 16*16 },

	{ 1, 0x10000, &tilelayout,			2*16*16, 16*16 },
	{ 1, 0x18000, &tilelayout,			2*16*16, 16*16 },

	{ 1, 0x20000, &tilelayout,			2*16*16, 16*16 },
	{ 1, 0x28000, &tilelayout,			2*16*16, 16*16 },
	{ 1, 0x30000, &tilelayout,			2*16*16, 16*16 },
	{ 1, 0x38000, &tilelayout,			2*16*16, 16*16 },
	{ 1, 0x40000, &tilelayout,			2*16*16, 16*16 },
	{ 1, 0x48000, &tilelayout,			2*16*16, 16*16 },

	{ 1, 0x50000, &tilelayout,			3*16*16, 16*16 },
	{ 1, 0x58000, &tilelayout,			3*16*16, 16*16 },
	{ 1, 0x60000, &tilelayout,			3*16*16, 16*16 },
	{ 1, 0x68000, &tilelayout,			3*16*16, 16*16 },
	{ 1, 0x70000, &tilelayout,			3*16*16, 16*16 },
	{ 1, 0x78000, &tilelayout,			3*16*16, 16*16 },
	{ 1, 0x80000, &tilelayout,			3*16*16, 16*16 },
	{ 1, 0x88000, &tilelayout,			3*16*16, 16*16 },

	{ 1, 0x90000, &spritelayout,		0*16*16, 16*16 }, // sprites
	{ -1 } /* end of array */
};



/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(void)
{
	cpu_cause_interrupt(2,0xff);
}

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	6000000,	/* 6 MHz ????? */
	{ YM2203_VOL(255,255), YM2203_VOL(255,255) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

static struct MachineDriver wc90_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			6000000,	/* 6.0 Mhz ??? */
			0,
			wc90_readmem1, wc90_writemem1,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			6000000,	/* 6.0 Mhz ??? */
			2,
			wc90_readmem2, wc90_writemem2,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 MHz ???? */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2203 */
								/* NMIs are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	4*16*16, 4*16*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	wc90_vh_start,
	wc90_vh_stop,
	wc90_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

static int wc90_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* the color RAM is initialized when the startup reset is finished */
        if (RAM[0xc200] == 0x69)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0x800F],6*5);
			osd_fclose(f);
		}

		return 1;
	}

        else
                return 0;  /* we can't load the hi scores yet */
}

static void wc90_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* for avoiding problems when we reset the game by pressing the F3 key */
        RAM[0xc200] = 0x00;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0x800F],6*5);
		osd_fclose(f);
	}

}


ROM_START( wc90_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "ic87_01.bin",  0x00000, 0x08000, 0x4a1affbc )	/* c000-ffff is not used */
	ROM_LOAD( "ic95_02.bin",  0x10000, 0x10000, 0x847d439c )	/* banked at f000-f7ff */

	ROM_REGION_DISPOSE(0x110000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic85_07v.bin", 0x00000, 0x10000, 0xc5219426 )	/* characters */
	ROM_LOAD( "ic86_08v.bin", 0x10000, 0x20000, 0x8fa1a1ff )	/* tiles #1 */
	ROM_LOAD( "ic90_09v.bin", 0x30000, 0x20000, 0x99f8841c )	/* tiles #2 */
	ROM_LOAD( "ic87_10v.bin", 0x50000, 0x20000, 0x8232093d )	/* tiles #3 */
	ROM_LOAD( "ic91_11v.bin", 0x70000, 0x20000, 0x188d3789 )	/* tiles #4 */
	ROM_LOAD( "ic50_12v.bin", 0x90000, 0x20000, 0xda1fe922 )	/* sprites  */
	ROM_LOAD( "ic54_13v.bin", 0xb0000, 0x20000, 0x9ad03c2c )	/* sprites  */
	ROM_LOAD( "ic60_14v.bin", 0xd0000, 0x20000, 0x499dfb1b )	/* sprites  */
	ROM_LOAD( "ic65_15v.bin", 0xf0000, 0x20000, 0xd8ea5c81 )	/* sprites  */

	ROM_REGION(0x20000)	/* 96k for code */  /* Second CPU */
	ROM_LOAD( "ic67_04.bin",  0x00000, 0x10000, 0xdc6eaf00 )	/* c000-ffff is not used */
	ROM_LOAD( "ic56_03.bin",  0x10000, 0x10000, 0x1ac02b3b )	/* banked at f000-f7ff */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ic54_05.bin",  0x00000, 0x10000, 0x27c348b3 )

	ROM_REGION(0x20000)	/* 64k for ADPCM samples */
	ROM_LOAD( "ic82_06.bin",  0x00000, 0x20000, 0x2fd692ed )
ROM_END

struct GameDriver wc90_driver =
{
	__FILE__,
	0,
	"wc90",
	"World Cup 90",
	"1989",
	"Tecmo",
	"Ernesto Corvi",
	0,
	&wc90_machine_driver,
	0,

	wc90_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	wc90_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	wc90_hiload, wc90_hisave
};
