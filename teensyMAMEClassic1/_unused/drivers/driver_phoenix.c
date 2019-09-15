/***************************************************************************
Note:
   pleiads is using another sound driver, sndhrdw\pleiads.c
 Andrew Scott (ascott@utkux.utcc.utk.edu)

Phoenix memory map

0000-3fff 16Kb Program ROM
4000-43ff 1Kb Video RAM Charset A (4340-43ff variables)
4400-47ff 1Kb Work RAM
4800-4bff 1Kb Video RAM Charset B (4840-4bff variables)
4c00-4fff 1Kb Work RAM
5000-53ff 1Kb Video Control write-only (mirrored)
5400-47ff 1Kb Work RAM
5800-5bff 1Kb Video Scroll Register (mirrored)
5c00-5fff 1Kb Work RAM
6000-63ff 1Kb Sound Control A (mirrored)
6400-67ff 1Kb Work RAM
6800-6bff 1Kb Sound Control B (mirrored)
6c00-6fff 1Kb Work RAM
7000-73ff 1Kb 8bit Game Control read-only (mirrored)
7400-77ff 1Kb Work RAM
7800-7bff 1Kb 8bit Dip Switch read-only (mirrored)
7c00-7fff 1Kb Work RAM

memory mapped ports:

read-only:
7000-73ff IN
7800-7bff DSW

 * IN (all bits are inverted)
 * bit 7 : barrier
 * bit 6 : Left
 * bit 5 : Right
 * bit 4 : Fire
 * bit 3 : -
 * bit 2 : Start 2
 * bit 1 : Start 1
 * bit 0 : Coin

 * DSW
 * bit 7 : VBlank
 * bit 6 : free play (pleiads only)
 * bit 5 : attract sound 0 = off 1 = on (pleiads only?)
 * bit 4 : coins per play  0 = 1 coin  1 = 2 coins
 * bit 3 :\ bonus
 * bit 2 :/ 00 = 3000  01 = 4000  10 = 5000  11 = 6000
 * bit 1 :\ number of lives
 * bit 0 :/ 00 = 3  01 = 4  10 = 5  11 = 6

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *phoenix_videoram2;

int phoenix_DSW_r (int offset);

void phoenix_videoreg_w (int offset,int data);
void phoenix_scroll_w (int offset,int data);
void phoenix_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void phoenix_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void phoenix_sound_control_a_w(int offset, int data);
void phoenix_sound_control_b_w(int offset, int data);
int phoenix_sh_start(void);
void phoenix_sh_update(void);
void pleiads_sound_control_a_w(int offset, int data);
void pleiads_sound_control_b_w(int offset, int data);
int pleiads_sh_start(void);
void pleiads_sh_update(void);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4fff, MRA_RAM },	/* video RAM */
	{ 0x5000, 0x6fff, MRA_RAM },
	{ 0x7000, 0x73ff, input_port_0_r },	/* IN0 */
	{ 0x7400, 0x77ff, MRA_RAM },
	{ 0x7800, 0x7bff, input_port_1_r },	/* DSW */
	{ 0x7c00, 0x7fff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM, &phoenix_videoram2 },
	{ 0x4400, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, videoram_w, &videoram, &videoram_size },
	{ 0x4C00, 0x4fff, MWA_RAM },
	{ 0x5000, 0x53ff, phoenix_videoreg_w },
	{ 0x5400, 0x57ff, MWA_RAM },
	{ 0x5800, 0x5bff, phoenix_scroll_w },	/* the game sometimes writes at mirror addresses */
	{ 0x5C00, 0x5fff, MWA_RAM },
	{ 0x6000, 0x63ff, phoenix_sound_control_a_w },
	{ 0x6400, 0x67ff, MWA_RAM },
	{ 0x6800, 0x6bff, phoenix_sound_control_b_w },
	{ 0x6C00, 0x6fff, MWA_RAM },
	{ 0x7400, 0x77ff, MWA_RAM },
	{ 0x7C00, 0x7fff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress pl_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM, &phoenix_videoram2 },
	{ 0x4400, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, videoram_w, &videoram, &videoram_size },
	{ 0x4C00, 0x4fff, MWA_RAM },
	{ 0x5000, 0x53ff, phoenix_videoreg_w },
	{ 0x5400, 0x57ff, MWA_RAM },
	{ 0x5800, 0x5bff, phoenix_scroll_w },
	{ 0x5C00, 0x5fff, MWA_RAM },
	{ 0x6000, 0x63ff, pleiads_sound_control_a_w },
	{ 0x6400, 0x67ff, MWA_RAM },
	{ 0x6800, 0x6bff, pleiads_sound_control_b_w },
	{ 0x6C00, 0x6fff, MWA_RAM },
	{ 0x7400, 0x77ff, MWA_RAM },
	{ 0x7C00, 0x7fff, MWA_RAM },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( phoenix_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x04, "4000" )
	PORT_DIPSETTING(    0x08, "5000" )
	PORT_DIPSETTING(    0x0c, "6000" )
	PORT_DIPNAME( 0x10, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x20, 0x20, "DSW 6 Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "DSW 7 Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END


INPUT_PORTS_START( phoenixt_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x04, "4000" )
	PORT_DIPSETTING(    0x08, "5000" )
	PORT_DIPSETTING(    0x0c, "6000" )
	PORT_DIPNAME( 0x10, 0x10, "DSW 5 Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "DSW 6 Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "DSW 7 Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END

INPUT_PORTS_START( phoenix3_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x04, "4000" )
	PORT_DIPSETTING(    0x08, "5000" )
	PORT_DIPSETTING(    0x0c, "6000" )
	PORT_DIPNAME( 0x10, 0x10, "DSW 5 Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "DSW 6 Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END


INPUT_PORTS_START( pleiads_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x0c, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x04, "4000" )
	PORT_DIPSETTING(    0x08, "5000" )
	PORT_DIPSETTING(    0x0c, "6000" )
	PORT_DIPNAME( 0x10, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x20, 0x20, "DSW 6 Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 256*8*8, 0 },	/* the two bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },	/* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,    0, 16 },
	{ 1, 0x1000, &charlayout, 16*4, 16 },
	{ -1 } /* end of array */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			3072000,	/* 3 Mhz ? */
			0,
			readmem,writemem,0,0,
			ignore_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 26*8-1 },
	gfxdecodeinfo,
	256,16*4+16*4,
	phoenix_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	phoenix_vh_screenrefresh,

	/* sound hardware */
	0,
	phoenix_sh_start,
	0,
	phoenix_sh_update
};

static struct MachineDriver pleiads_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			3072000,	/* 3 Mhz ? */
			0,
                        readmem,pl_writemem,0,0,
			ignore_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 26*8-1 },
	gfxdecodeinfo,
	256,16*4+16*4,
	phoenix_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	phoenix_vh_screenrefresh,

	/* sound hardware */
	0,
	pleiads_sh_start,
	0,
	pleiads_sh_update
};

static const char *phoenix_sample_names[] =
{
	"*phoenix",
	"shot8.sam",
	"death8.sam",
	"phoenix1.sam",
	"phoenix2.sam",
	0	/* end of array */
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( phoenix_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic45",         0x0000, 0x0800, 0x9f68086b )
	ROM_LOAD( "ic46",         0x0800, 0x0800, 0x273a4a82 )
	ROM_LOAD( "ic47",         0x1000, 0x0800, 0x3d4284b9 )
	ROM_LOAD( "ic48",         0x1800, 0x0800, 0xcb5d9915 )
	ROM_LOAD( "ic49",         0x2000, 0x0800, 0xa105e4e7 )
	ROM_LOAD( "ic50",         0x2800, 0x0800, 0xac5e9ec1 )
	ROM_LOAD( "ic51",         0x3000, 0x0800, 0x2eab35b4 )
	ROM_LOAD( "ic52",         0x3800, 0x0800, 0xaff8e9c5 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic23",         0x0000, 0x0800, 0x3c7e623f )
	ROM_LOAD( "ic24",         0x0800, 0x0800, 0x59916d3b )
	ROM_LOAD( "ic39",         0x1000, 0x0800, 0x53413e8f )
	ROM_LOAD( "ic40",         0x1800, 0x0800, 0x0be2ba91 )

	ROM_REGION(0x0200)	/* color PROMs */
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, 0x79350b25 )	/* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, 0xe176b768 )	/* palette high bits */
ROM_END

ROM_START( phoenixt_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "phoenix.45",   0x0000, 0x0800, 0x5b8c55a8 )
	ROM_LOAD( "phoenix.46",   0x0800, 0x0800, 0xdbc942fa )
	ROM_LOAD( "phoenix.47",   0x1000, 0x0800, 0xcbbb8839 )
	ROM_LOAD( "phoenix.48",   0x1800, 0x0800, 0xcb65eff8 )
	ROM_LOAD( "phoenix.49",   0x2000, 0x0800, 0xc8a5d6d6 )
	ROM_LOAD( "ic50",         0x2800, 0x0800, 0xac5e9ec1 )
	ROM_LOAD( "ic51",         0x3000, 0x0800, 0x2eab35b4 )
	ROM_LOAD( "phoenix.52",   0x3800, 0x0800, 0xb9915263 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic23",         0x0000, 0x0800, 0x3c7e623f )
	ROM_LOAD( "ic24",         0x0800, 0x0800, 0x59916d3b )
	ROM_LOAD( "ic39",         0x1000, 0x0800, 0x53413e8f )
	ROM_LOAD( "ic40",         0x1800, 0x0800, 0x0be2ba91 )

	ROM_REGION(0x0200)	/* color PROMs */
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, 0x79350b25 )	/* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, 0xe176b768 )	/* palette high bits */
ROM_END

ROM_START( phoenix3_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "phoenix3.45",  0x0000, 0x0800, 0xa362cda0 )
	ROM_LOAD( "phoenix3.46",  0x0800, 0x0800, 0x5748f486 )
	ROM_LOAD( "phoenix3.47",  0x1000, 0x0800, 0xcbbb8839 )
	ROM_LOAD( "phoenix3.48",  0x1800, 0x0800, 0xb5d97a4d )
	ROM_LOAD( "phoenix3.49",  0x2000, 0x0800, 0xa105e4e7 )
	ROM_LOAD( "ic50",         0x2800, 0x0800, 0xac5e9ec1 )
	ROM_LOAD( "ic51",         0x3000, 0x0800, 0x2eab35b4 )
	ROM_LOAD( "phoenix3.52",  0x3800, 0x0800, 0xd2c5c984 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic23",         0x0000, 0x0800, 0x3c7e623f )
	ROM_LOAD( "ic24",         0x0800, 0x0800, 0x59916d3b )
	ROM_LOAD( "ic39",         0x1000, 0x0800, 0x53413e8f )
	ROM_LOAD( "ic40",         0x1800, 0x0800, 0x0be2ba91 )

	ROM_REGION(0x0200)	/* color PROMs */
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, 0x79350b25 )	/* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, 0xe176b768 )	/* palette high bits */
ROM_END

ROM_START( phoenixc_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "phoenix.45",   0x0000, 0x0800, 0x5b8c55a8 )
	ROM_LOAD( "phoenix.46",   0x0800, 0x0800, 0xdbc942fa )
	ROM_LOAD( "phoenix.47",   0x1000, 0x0800, 0xcbbb8839 )
	ROM_LOAD( "phoenixc.48",  0x1800, 0x0800, 0x5ae0b215 )
	ROM_LOAD( "phoenixc.49",  0x2000, 0x0800, 0x1a1ce0d0 )
	ROM_LOAD( "ic50",         0x2800, 0x0800, 0xac5e9ec1 )
	ROM_LOAD( "ic51",         0x3000, 0x0800, 0x2eab35b4 )
	ROM_LOAD( "phoenixc.52",  0x3800, 0x0800, 0x8424d7c4 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic23",         0x0000, 0x0800, 0x3c7e623f )
	ROM_LOAD( "ic24",         0x0800, 0x0800, 0x59916d3b )
	ROM_LOAD( "phoenixc.39",  0x1000, 0x0800, 0xbb0525ed )
	ROM_LOAD( "phoenixc.40",  0x1800, 0x0800, 0x4178aa4f )

	ROM_REGION(0x0200)	/* color PROMs */
	ROM_LOAD( "ic40_b.bin",   0x0000, 0x0100, 0x79350b25 )	/* palette low bits */
	ROM_LOAD( "ic41_a.bin",   0x0100, 0x0100, 0xe176b768 )	/* palette high bits */
ROM_END

ROM_START( pleiads_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic45.bin",     0x0000, 0x0800, 0x93fc2958 )
	ROM_LOAD( "ic46.bin",     0x0800, 0x0800, 0xe2b5b8cd )
	ROM_LOAD( "ic47.bin",     0x1000, 0x0800, 0x87e700bb )
	ROM_LOAD( "ic48.bin",     0x1800, 0x0800, 0x2d5198d0 )
	ROM_LOAD( "ic49.bin",     0x2000, 0x0800, 0x9dc73e63 )
	ROM_LOAD( "ic50.bin",     0x2800, 0x0800, 0xf1a8a00d )
	ROM_LOAD( "ic51.bin",     0x3000, 0x0800, 0x6f56f317 )
	ROM_LOAD( "ic52.bin",     0x3800, 0x0800, 0xb1b5a8a6 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic23.bin",     0x0000, 0x0800, 0x4e30f9e7 )
	ROM_LOAD( "ic24.bin",     0x0800, 0x0800, 0x5188fc29 )
	ROM_LOAD( "ic39.bin",     0x1000, 0x0800, 0x85866607 )
	ROM_LOAD( "ic40.bin",     0x1800, 0x0800, 0xa841d511 )

	ROM_REGION(0x0200)	/* color PROMs */
	ROM_LOAD( "7611-5.26",   0x0000, 0x0100, 0x7a1bcb1e )	/* palette low bits */
	ROM_LOAD( "7611-5.33",   0x0100, 0x0100, 0xe38eeb83 )	/* palette high bits */
ROM_END

ROM_START( pleiadce_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pleiades.47",  0x0000, 0x0800, 0x711e2ba0 )
	ROM_LOAD( "pleiades.48",  0x0800, 0x0800, 0x93a36943 )
	ROM_LOAD( "ic47.bin",     0x1000, 0x0800, 0x87e700bb )
	ROM_LOAD( "pleiades.50",  0x1800, 0x0800, 0x5a9beba0 )
	ROM_LOAD( "pleiades.51",  0x2000, 0x0800, 0x1d828719 )
	ROM_LOAD( "ic50.bin",     0x2800, 0x0800, 0xf1a8a00d )
	ROM_LOAD( "pleiades.53",  0x3000, 0x0800, 0x037b319c )
	ROM_LOAD( "pleiades.54",  0x3800, 0x0800, 0xca264c7c )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pleiades.45",  0x0000, 0x0800, 0x8dbd3785 )
	ROM_LOAD( "pleiades.44",  0x0800, 0x0800, 0x0db3e436 )
	ROM_LOAD( "ic39.bin",     0x1000, 0x0800, 0x85866607 )
	ROM_LOAD( "ic40.bin",     0x1800, 0x0800, 0xa841d511 )

	ROM_REGION(0x0200)	/* color PROMs */
	ROM_LOAD( "7611-5.26",   0x0000, 0x0100, 0x7a1bcb1e )	/* palette low bits */
	ROM_LOAD( "7611-5.33",   0x0100, 0x0100, 0xe38eeb83 )	/* palette high bits */
ROM_END



static int hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x438a],"\x00\x00\x0f",3) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4388],4);

			/* copy the high score to the screen */
			/*       // I suppose noone can do such an HISCORE!!! ;)
			phoenix_videoram2_w(0x0221, (RAM[0x4388] >> 4)+0x20);
			phoenix_videoram2_w(0x0201, (RAM[0x4388] & 0xf)+0x20);
			*/
			RAM[0x49e1] = (RAM[0x4389] >> 4) + 0x20;
			RAM[0x49c1] = (RAM[0x4389] & 0xf) + 0x20;
			RAM[0x49a1] = (RAM[0x438a] >> 4) + 0x20;
			RAM[0x4981] = (RAM[0x438a] & 0xf) + 0x20;
			RAM[0x4961] = (RAM[0x438b] >> 4) + 0x20;
			RAM[0x4941] = (RAM[0x438b] & 0xf) + 0x20;
			osd_fclose(f);
		}

		return 1;
	}
	else return 0; /* we can't load the hi scores yet */
}



static unsigned long get_score(unsigned char *score)
{
     return (score[3])+(256*score[2])+((unsigned long)(65536)*score[1])+((unsigned long)(65536)*256*score[0]);
}

static void hisave(void)
{
   unsigned long score1,score2,hiscore;
   void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


   score1 = get_score(&RAM[0x4380]);
   score2 = get_score(&RAM[0x4384]);
   hiscore = get_score(&RAM[0x4388]);

   if (score1 > hiscore) RAM += 0x4380;         /* check consistency */
   else if (score2 > hiscore) RAM += 0x4384;
   else RAM += 0x4388;

   if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
   {
      osd_fwrite(f,&RAM[0],4);
      osd_fclose(f);
   }
}



struct GameDriver phoenix_driver =
{
	__FILE__,
	0,
	"phoenix",
	"Phoenix (Amstar)",
	"1980",
	"Amstar",
	"Richard Davies\nBrad Oliver\nMirko Buffoni\nNicola Salmoria\nShaun Stephenson\nAndrew Scott\nTim Lindquist (color info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	phoenix_rom,
	0, 0,
	phoenix_sample_names,
	0,	/* sound_prom */

	phoenix_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver phoenixt_driver =
{
	__FILE__,
	&phoenix_driver,
	"phoenixt",
	"Phoenix (Taito)",
	"1980",
	"Taito",
	"Richard Davies\nBrad Oliver\nMirko Buffoni\nNicola Salmoria\nShaun Stephenson\nAndrew Scott\nTim Lindquist (color info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	phoenixt_rom,
	0, 0,
	phoenix_sample_names,
	0,	/* sound_prom */

	phoenixt_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver phoenix3_driver =
{
	__FILE__,
	&phoenix_driver,
	"phoenix3",
	"Phoenix (T.P.N.)",
	"1980",
	"bootleg",
	"Richard Davies\nBrad Oliver\nMirko Buffoni\nNicola Salmoria\nShaun Stephenson\nAndrew Scott\nTim Lindquist (color info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	phoenix3_rom,
	0, 0,
	phoenix_sample_names,
	0,	/* sound_prom */

	phoenix3_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver phoenixc_driver =
{
	__FILE__,
	&phoenix_driver,
	"phoenixc",
	"Phoenix (IRECSA, G.G.I Corp)",
	"1981",
	"bootleg?",
	"Richard Davies\nBrad Oliver\nMirko Buffoni\nNicola Salmoria\nShaun Stephenson\nAndrew Scott\nTim Lindquist (color info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	phoenixc_rom,
	0, 0,
	phoenix_sample_names,
	0,	/* sound_prom */

	phoenixt_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};


struct GameDriver pleiads_driver =
{
	__FILE__,
	0,
	"pleiads",
	"Pleiads (Tehkan)",
	"1981",
	"Tehkan",
	"Richard Davies\nBrad Oliver\nMirko Buffoni\nNicola Salmoria\nShaun Stephenson\nAndrew Scott\nMarco Cassili",
	0,
	&pleiads_machine_driver,
	0,

	pleiads_rom,
	0, 0,
	phoenix_sample_names,
	0,	/* sound_prom */

	pleiads_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver pleiadce_driver =
{
	__FILE__,
	&pleiads_driver,
	"pleiadce",
	"Pleiads (Centuri)",
	"1981",
	"Tehkan (Centuri license)",
	"Richard Davies\nBrad Oliver\nMirko Buffoni\nNicola Salmoria\nShaun Stephenson\nAndrew Scott\nMarco Cassili",
	0,
	&pleiads_machine_driver,
	0,

	pleiadce_rom,
	0, 0,
	phoenix_sample_names,
	0,	/* sound_prom */

	pleiads_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};
