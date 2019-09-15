/***************************************************************************

  GUNSMOKE
  ========

  Driver provided by Paul Leaman

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



int gunsmoke_bankedrom_r(int offset);
extern void gunsmoke_init_machine(void);

extern unsigned char *gunsmoke_bg_scrollx;
extern unsigned char *gunsmoke_bg_scrolly;

void gunsmoke_c804_w(int offset,int data);	/* in vidhrdw/c1943.c */
void gunsmoke_d806_w(int offset,int data);	/* in vidhrdw/c1943.c */
void gunsmoke_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void gunsmoke_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int gunsmoke_vh_start(void);
void gunsmoke_vh_stop(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc002, 0xc002, input_port_2_r },
	{ 0xc003, 0xc003, input_port_3_r },
	{ 0xc004, 0xc004, input_port_4_r },
	{ 0xe000, 0xffff, MRA_RAM }, /* Work + sprite RAM */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xc804, 0xc804, gunsmoke_c804_w },	/* ROM bank switch, screen flip */
	{ 0xc806, 0xc806, MWA_NOP }, /* Watchdog ?? */
	{ 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
	{ 0xd400, 0xd7ff, colorram_w, &colorram },
	{ 0xd800, 0xd801, MWA_RAM, &gunsmoke_bg_scrolly },
	{ 0xd802, 0xd802, MWA_RAM, &gunsmoke_bg_scrollx },
	{ 0xd806, 0xd806, gunsmoke_d806_w },	/* sprites and bg enable */
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xffff, MWA_RAM, &spriteram, &spriteram_size },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xc800, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe000, YM2203_control_port_0_w },
	{ 0xe001, 0xe001, YM2203_write_port_0_w },
	{ 0xe002, 0xe002, YM2203_control_port_1_w },
	{ 0xe003, 0xe003, YM2203_write_port_1_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "30k, 100k & every 100k")
	PORT_DIPSETTING(    0x02, "30k, 80k & every 80k" )
	PORT_DIPSETTING(    0x01, "30k & 100K only")
	PORT_DIPSETTING(    0x00, "30k, 100k & every 150k" )
	PORT_DIPNAME( 0x04, 0x04, "Demonstration", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off")
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright")
	PORT_DIPSETTING(    0x08, "Cocktail")
	PORT_DIPNAME( 0x30, 0x30, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x40, 0x40, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off")
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x38, 0x38, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x18, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No")
	PORT_DIPSETTING(    0x40, "Yes")
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On")
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3},
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	2048,	/* 2048 sprites */
	4,      /* 4 bits per pixel */
	{ 2048*64*8+4, 2048*64*8+0, 4, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	{ 33*8+3, 33*8+2, 33*8+1, 33*8+0, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
			8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	32,32,  /* 32*32 tiles */
	512,    /* 512 tiles */
	4,      /* 4 bits per pixel */
	{ 512*256*8+4, 512*256*8+0, 4, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
			24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16 },
	{ 192*8+8+3, 192*8+8+2, 192*8+8+1, 192*8+8+0, 192*8+3, 192*8+2, 192*8+1, 192*8+0,
			128*8+8+3, 128*8+8+2, 128*8+8+1, 128*8+8+0, 128*8+3, 128*8+2, 128*8+1, 128*8+0,
			64*8+8+3, 64*8+8+2, 64*8+8+1, 64*8+8+0, 64*8+3, 64*8+2, 64*8+1, 64*8+0,
			8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	256*8	/* every tile takes 256 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,            0, 32 },
	{ 1, 0x04000, &tilelayout,         32*4, 16 }, /* Tiles */
	{ 1, 0x44000, &spritelayout, 32*4+16*16, 16 }, /* Sprites */
	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz (?) */
	{ YM2203_VOL(15,25), YM2203_VOL(15,25) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,        /* 4 Mhz (?) */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 Mhz (?) */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			interrupt,4
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,32*4+16*16+16*16,
	gunsmoke_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	gunsmoke_vh_start,
	gunsmoke_vh_stop,
	gunsmoke_vh_screenrefresh,

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

ROM_START( gunsmoke_rom )
	ROM_REGION(0x20000)     /* 2*64k for code */
	ROM_LOAD( "09n_gs03.bin", 0x00000, 0x8000, 0x40a06cef ) /* Code 0000-7fff */
	ROM_LOAD( "10n_gs04.bin", 0x10000, 0x8000, 0x8d4b423f ) /* Paged code */
	ROM_LOAD( "12n_gs05.bin", 0x18000, 0x8000, 0x2b5667fb ) /* Paged code */

	ROM_REGION_DISPOSE(0x84000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "11f_gs01.bin", 0x00000, 0x4000, 0xb61ece9b ) /* Characters */
	ROM_LOAD( "06c_gs13.bin", 0x04000, 0x8000, 0xf6769fc5 ) /* 32x32 tiles planes 2-3 */
	ROM_LOAD( "05c_gs12.bin", 0x0c000, 0x8000, 0xd997b78c )
	ROM_LOAD( "04c_gs11.bin", 0x14000, 0x8000, 0x125ba58e )
	ROM_LOAD( "02c_gs10.bin", 0x1c000, 0x8000, 0xf469c13c )
	ROM_LOAD( "06a_gs09.bin", 0x24000, 0x8000, 0x539f182d ) /* 32x32 tiles planes 0-1 */
	ROM_LOAD( "05a_gs08.bin", 0x2c000, 0x8000, 0xe87e526d )
	ROM_LOAD( "04a_gs07.bin", 0x34000, 0x8000, 0x4382c0d2 )
	ROM_LOAD( "02a_gs06.bin", 0x3c000, 0x8000, 0x4cafe7a6 )
	ROM_LOAD( "06n_gs22.bin", 0x44000, 0x8000, 0xdc9c508c ) /* Sprites planes 2-3 */
	ROM_LOAD( "04n_gs21.bin", 0x4c000, 0x8000, 0x68883749 ) /* Sprites planes 2-3 */
	ROM_LOAD( "03n_gs20.bin", 0x54000, 0x8000, 0x0be932ed ) /* Sprites planes 2-3 */
	ROM_LOAD( "01n_gs19.bin", 0x5c000, 0x8000, 0x63072f93 ) /* Sprites planes 2-3 */
	ROM_LOAD( "06l_gs18.bin", 0x64000, 0x8000, 0xf69a3c7c ) /* Sprites planes 0-1 */
	ROM_LOAD( "04l_gs17.bin", 0x6c000, 0x8000, 0x4e98562a ) /* Sprites planes 0-1 */
	ROM_LOAD( "03l_gs16.bin", 0x74000, 0x8000, 0x0d99c3b3 ) /* Sprites planes 0-1 */
	ROM_LOAD( "01l_gs15.bin", 0x7c000, 0x8000, 0x7f14270e ) /* Sprites planes 0-1 */

	ROM_REGION(0x0800)	/* color PROMs */
	ROM_LOAD( "03b_g-01.bin", 0x0000, 0x0100, 0x02f55589 )	/* red component */
	ROM_LOAD( "04b_g-02.bin", 0x0100, 0x0100, 0xe1e36dd9 )	/* green component */
	ROM_LOAD( "05b_g-03.bin", 0x0200, 0x0100, 0x989399c0 )	/* blue component */
	ROM_LOAD( "09d_g-04.bin", 0x0300, 0x0100, 0x906612b5 )	/* char lookup table */
	ROM_LOAD( "14a_g-06.bin", 0x0400, 0x0100, 0x4a9da18b )	/* tile lookup table */
	ROM_LOAD( "15a_g-07.bin", 0x0500, 0x0100, 0xcb9394fc )	/* tile palette bank */
	ROM_LOAD( "09f_g-09.bin", 0x0600, 0x0100, 0x3cee181e )	/* sprite lookup table */
	ROM_LOAD( "08f_g-08.bin", 0x0700, 0x0100, 0xef91cdd2 )	/* sprite palette bank */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "14h_gs02.bin", 0x00000, 0x8000, 0xcd7a2c38 )

	ROM_REGION(0x8000)
	ROM_LOAD( "11c_gs14.bin", 0x00000, 0x8000, 0x0af4f7eb ) /* Background tile map */
ROM_END

ROM_START( gunsmrom_rom )
	ROM_REGION(0x20000)     /* 2*64k for code */
	ROM_LOAD( "9n_gs03.bin",  0x00000, 0x8000, 0x592f211b ) /* Code 0000-7fff */
	ROM_LOAD( "10n_gs04.bin", 0x10000, 0x8000, 0x8d4b423f ) /* Paged code */
	ROM_LOAD( "12n_gs05.bin", 0x18000, 0x8000, 0x2b5667fb ) /* Paged code */

	ROM_REGION_DISPOSE(0x84000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "11f_gs01.bin", 0x00000, 0x4000, 0xb61ece9b ) /* Characters */
	ROM_LOAD( "06c_gs13.bin", 0x04000, 0x8000, 0xf6769fc5 ) /* 32x32 tiles planes 2-3 */
	ROM_LOAD( "05c_gs12.bin", 0x0c000, 0x8000, 0xd997b78c )
	ROM_LOAD( "04c_gs11.bin", 0x14000, 0x8000, 0x125ba58e )
	ROM_LOAD( "02c_gs10.bin", 0x1c000, 0x8000, 0xf469c13c )
	ROM_LOAD( "06a_gs09.bin", 0x24000, 0x8000, 0x539f182d ) /* 32x32 tiles planes 0-1 */
	ROM_LOAD( "05a_gs08.bin", 0x2c000, 0x8000, 0xe87e526d )
	ROM_LOAD( "04a_gs07.bin", 0x34000, 0x8000, 0x4382c0d2 )
	ROM_LOAD( "02a_gs06.bin", 0x3c000, 0x8000, 0x4cafe7a6 )
	ROM_LOAD( "06n_gs22.bin", 0x44000, 0x8000, 0xdc9c508c ) /* Sprites planes 2-3 */
	ROM_LOAD( "04n_gs21.bin", 0x4c000, 0x8000, 0x68883749 ) /* Sprites planes 2-3 */
	ROM_LOAD( "03n_gs20.bin", 0x54000, 0x8000, 0x0be932ed ) /* Sprites planes 2-3 */
	ROM_LOAD( "01n_gs19.bin", 0x5c000, 0x8000, 0x63072f93 ) /* Sprites planes 2-3 */
	ROM_LOAD( "06l_gs18.bin", 0x64000, 0x8000, 0xf69a3c7c ) /* Sprites planes 0-1 */
	ROM_LOAD( "04l_gs17.bin", 0x6c000, 0x8000, 0x4e98562a ) /* Sprites planes 0-1 */
	ROM_LOAD( "03l_gs16.bin", 0x74000, 0x8000, 0x0d99c3b3 ) /* Sprites planes 0-1 */
	ROM_LOAD( "01l_gs15.bin", 0x7c000, 0x8000, 0x7f14270e ) /* Sprites planes 0-1 */

	ROM_REGION(0x0800)	/* color PROMs */
	ROM_LOAD( "03b_g-01.bin", 0x0000, 0x0100, 0x02f55589 )	/* red component */
	ROM_LOAD( "04b_g-02.bin", 0x0100, 0x0100, 0xe1e36dd9 )	/* green component */
	ROM_LOAD( "05b_g-03.bin", 0x0200, 0x0100, 0x989399c0 )	/* blue component */
	ROM_LOAD( "09d_g-04.bin", 0x0300, 0x0100, 0x906612b5 )	/* char lookup table */
	ROM_LOAD( "14a_g-06.bin", 0x0400, 0x0100, 0x4a9da18b )	/* tile lookup table */
	ROM_LOAD( "15a_g-07.bin", 0x0500, 0x0100, 0xcb9394fc )	/* tile palette bank */
	ROM_LOAD( "09f_g-09.bin", 0x0600, 0x0100, 0x3cee181e )	/* sprite lookup table */
	ROM_LOAD( "08f_g-08.bin", 0x0700, 0x0100, 0xef91cdd2 )	/* sprite palette bank */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "14h_gs02.bin", 0x00000, 0x8000, 0xcd7a2c38 )

	ROM_REGION(0x8000)
	ROM_LOAD( "11c_gs14.bin", 0x00000, 0x8000, 0x0af4f7eb ) /* Background tile map */
ROM_END

ROM_START( gunsmokj_rom )
	ROM_REGION(0x20000)     /* 2*64k for code */
	ROM_LOAD( "gs03_9n.rom",  0x00000, 0x8000, 0xb56b5df6 ) /* Code 0000-7fff */
	ROM_LOAD( "10n_gs04.bin", 0x10000, 0x8000, 0x8d4b423f ) /* Paged code */
	ROM_LOAD( "12n_gs05.bin", 0x18000, 0x8000, 0x2b5667fb ) /* Paged code */

	ROM_REGION_DISPOSE(0x84000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "11f_gs01.bin", 0x00000, 0x4000, 0xb61ece9b ) /* Characters */
	ROM_LOAD( "06c_gs13.bin", 0x04000, 0x8000, 0xf6769fc5 ) /* 32x32 tiles planes 2-3 */
	ROM_LOAD( "05c_gs12.bin", 0x0c000, 0x8000, 0xd997b78c )
	ROM_LOAD( "04c_gs11.bin", 0x14000, 0x8000, 0x125ba58e )
	ROM_LOAD( "02c_gs10.bin", 0x1c000, 0x8000, 0xf469c13c )
	ROM_LOAD( "06a_gs09.bin", 0x24000, 0x8000, 0x539f182d ) /* 32x32 tiles planes 0-1 */
	ROM_LOAD( "05a_gs08.bin", 0x2c000, 0x8000, 0xe87e526d )
	ROM_LOAD( "04a_gs07.bin", 0x34000, 0x8000, 0x4382c0d2 )
	ROM_LOAD( "02a_gs06.bin", 0x3c000, 0x8000, 0x4cafe7a6 )
	ROM_LOAD( "06n_gs22.bin", 0x44000, 0x8000, 0xdc9c508c ) /* Sprites planes 2-3 */
	ROM_LOAD( "04n_gs21.bin", 0x4c000, 0x8000, 0x68883749 ) /* Sprites planes 2-3 */
	ROM_LOAD( "03n_gs20.bin", 0x54000, 0x8000, 0x0be932ed ) /* Sprites planes 2-3 */
	ROM_LOAD( "01n_gs19.bin", 0x5c000, 0x8000, 0x63072f93 ) /* Sprites planes 2-3 */
	ROM_LOAD( "06l_gs18.bin", 0x64000, 0x8000, 0xf69a3c7c ) /* Sprites planes 0-1 */
	ROM_LOAD( "04l_gs17.bin", 0x6c000, 0x8000, 0x4e98562a ) /* Sprites planes 0-1 */
	ROM_LOAD( "03l_gs16.bin", 0x74000, 0x8000, 0x0d99c3b3 ) /* Sprites planes 0-1 */
	ROM_LOAD( "01l_gs15.bin", 0x7c000, 0x8000, 0x7f14270e ) /* Sprites planes 0-1 */

	ROM_REGION(0x0800)	/* color PROMs */
	ROM_LOAD( "03b_g-01.bin", 0x0000, 0x0100, 0x02f55589 )	/* red component */
	ROM_LOAD( "04b_g-02.bin", 0x0100, 0x0100, 0xe1e36dd9 )	/* green component */
	ROM_LOAD( "05b_g-03.bin", 0x0200, 0x0100, 0x989399c0 )	/* blue component */
	ROM_LOAD( "09d_g-04.bin", 0x0300, 0x0100, 0x906612b5 )	/* char lookup table */
	ROM_LOAD( "14a_g-06.bin", 0x0400, 0x0100, 0x4a9da18b )	/* tile lookup table */
	ROM_LOAD( "15a_g-07.bin", 0x0500, 0x0100, 0xcb9394fc )	/* tile palette bank */
	ROM_LOAD( "09f_g-09.bin", 0x0600, 0x0100, 0x3cee181e )	/* sprite lookup table */
	ROM_LOAD( "08f_g-08.bin", 0x0700, 0x0100, 0xef91cdd2 )	/* sprite palette bank */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "14h_gs02.bin", 0x00000, 0x8000, 0xcd7a2c38 )

	ROM_REGION(0x8000)
	ROM_LOAD( "11c_gs14.bin", 0x00000, 0x8000, 0x0af4f7eb ) /* Background tile map */
ROM_END

ROM_START( gunsmoka_rom )
	ROM_REGION(0x20000)     /* 2*64k for code */
	ROM_LOAD( "gs03.9n",      0x00000, 0x8000, 0x51dc3f76 ) /* Code 0000-7fff */
	ROM_LOAD( "gs04.10n",     0x10000, 0x8000, 0x5ecf31b8 ) /* Paged code */
	ROM_LOAD( "gs05.12n",     0x18000, 0x8000, 0x1c9aca13 ) /* Paged code */

	ROM_REGION_DISPOSE(0x84000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "11f_gs01.bin", 0x00000, 0x4000, 0xb61ece9b ) /* Characters */
	ROM_LOAD( "06c_gs13.bin", 0x04000, 0x8000, 0xf6769fc5 ) /* 32x32 tiles planes 2-3 */
	ROM_LOAD( "05c_gs12.bin", 0x0c000, 0x8000, 0xd997b78c )
	ROM_LOAD( "04c_gs11.bin", 0x14000, 0x8000, 0x125ba58e )
	ROM_LOAD( "02c_gs10.bin", 0x1c000, 0x8000, 0xf469c13c )
	ROM_LOAD( "06a_gs09.bin", 0x24000, 0x8000, 0x539f182d ) /* 32x32 tiles planes 0-1 */
	ROM_LOAD( "05a_gs08.bin", 0x2c000, 0x8000, 0xe87e526d )
	ROM_LOAD( "04a_gs07.bin", 0x34000, 0x8000, 0x4382c0d2 )
	ROM_LOAD( "02a_gs06.bin", 0x3c000, 0x8000, 0x4cafe7a6 )
	ROM_LOAD( "06n_gs22.bin", 0x44000, 0x8000, 0xdc9c508c ) /* Sprites planes 2-3 */
	ROM_LOAD( "04n_gs21.bin", 0x4c000, 0x8000, 0x68883749 ) /* Sprites planes 2-3 */
	ROM_LOAD( "03n_gs20.bin", 0x54000, 0x8000, 0x0be932ed ) /* Sprites planes 2-3 */
	ROM_LOAD( "01n_gs19.bin", 0x5c000, 0x8000, 0x63072f93 ) /* Sprites planes 2-3 */
	ROM_LOAD( "06l_gs18.bin", 0x64000, 0x8000, 0xf69a3c7c ) /* Sprites planes 0-1 */
	ROM_LOAD( "04l_gs17.bin", 0x6c000, 0x8000, 0x4e98562a ) /* Sprites planes 0-1 */
	ROM_LOAD( "03l_gs16.bin", 0x74000, 0x8000, 0x0d99c3b3 ) /* Sprites planes 0-1 */
	ROM_LOAD( "01l_gs15.bin", 0x7c000, 0x8000, 0x7f14270e ) /* Sprites planes 0-1 */

	ROM_REGION(0x0800)	/* color PROMs */
	ROM_LOAD( "03b_g-01.bin", 0x0000, 0x0100, 0x02f55589 )	/* red component */
	ROM_LOAD( "04b_g-02.bin", 0x0100, 0x0100, 0xe1e36dd9 )	/* green component */
	ROM_LOAD( "05b_g-03.bin", 0x0200, 0x0100, 0x989399c0 )	/* blue component */
	ROM_LOAD( "09d_g-04.bin", 0x0300, 0x0100, 0x906612b5 )	/* char lookup table */
	ROM_LOAD( "14a_g-06.bin", 0x0400, 0x0100, 0x4a9da18b )	/* tile lookup table */
	ROM_LOAD( "15a_g-07.bin", 0x0500, 0x0100, 0xcb9394fc )	/* tile palette bank */
	ROM_LOAD( "09f_g-09.bin", 0x0600, 0x0100, 0x3cee181e )	/* sprite lookup table */
	ROM_LOAD( "08f_g-08.bin", 0x0700, 0x0100, 0xef91cdd2 )	/* sprite palette bank */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "14h_gs02.bin", 0x00000, 0x8000, 0xcd7a2c38 )

	ROM_REGION(0x8000)
	ROM_LOAD( "11c_gs14.bin", 0x00000, 0x8000, 0x0af4f7eb ) /* Background tile map */
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0xe683],"\x01\x00\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{

                        osd_fread(f,&RAM[0xe680],16*5);

                        memcpy(&RAM[0xe600], &RAM[0xe680], 8);
                        osd_fclose(f);
		}

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
                osd_fwrite(f,&RAM[0xe680],16*5);
		osd_fclose(f);
	}
}


struct GameDriver gunsmoke_driver =
{
	__FILE__,
	0,
	"gunsmoke",
	"Gunsmoke (World)",
	"1985",
	"Capcom",
	"Paul Leaman (MAME driver)\nRichard Davies\nAnders Nilsson\nMirko Buffoni\nNicola Salmoria\nPaul Swan (color info)",
	0,
	&machine_driver,
	0,

	gunsmoke_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver gunsmrom_driver =
{
	__FILE__,
	&gunsmoke_driver,
	"gunsmrom",
	"Gunsmoke (US set 1)",
	"1985",
	"Capcom (Romstar license)",
	"Paul Leaman (MAME driver)\nRichard Davies\nAnders Nilsson\nMirko Buffoni\nNicola Salmoria\nPaul Swan (color info)",
	0,
	&machine_driver,
	0,

	gunsmrom_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver gunsmoka_driver =
{
	__FILE__,
	&gunsmoke_driver,
	"gunsmoka",
	"Gunsmoke (US set 2)",
	"1986",
	"Capcom",
	"Paul Leaman (MAME driver)\nRichard Davies\nAnders Nilsson\nMirko Buffoni\nNicola Salmoria\nPaul Swan (color info)",
	0,
	&machine_driver,
	0,

	gunsmoka_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver gunsmokj_driver =
{
	__FILE__,
	&gunsmoke_driver,
	"gunsmokj",
	"Gunsmoke (Japan)",
	"1985",
	"Capcom",
	"Paul Leaman (MAME driver)\nRichard Davies\nAnders Nilsson\nMirko Buffoni\nNicola Salmoria\nPaul Swan (color info)",
	0,
	&machine_driver,
	0,

	gunsmokj_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
