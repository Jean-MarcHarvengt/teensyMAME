/*

TODO: 1943 is almost identical to GunSmoke (one more scrolling playfield). We
      should merge the two drivers.
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80/Z80.h"



extern unsigned char *c1943_scrollx;
extern unsigned char *c1943_scrolly;
extern unsigned char *c1943_bgscrolly;
void c1943_c804_w(int offset,int data);	/* in vidhrdw/c1943.c */
void c1943_d806_w(int offset,int data);	/* in vidhrdw/c1943.c */
void c1943_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void c1943_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int c1943_vh_start(void);
void c1943_vh_stop(void);



/* this is a protection check. The game crashes (thru a jump to 0x8000) */
/* if a read from this address doesn't return the value it expects. */
static int c1943_protection_r(int offset)
{
	Z80_Regs regs;


	Z80_GetRegs(&regs);
	if (errorlog) fprintf(errorlog,"protection read, PC: %04x Result:%02x\n",cpu_getpc(),regs.BC.B.h);
	return regs.BC.B.h;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc002, 0xc002, input_port_2_r },
	{ 0xc003, 0xc003, input_port_3_r },
	{ 0xc004, 0xc004, input_port_4_r },
	{ 0xc007, 0xc007, c1943_protection_r },
	{ 0xe000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xc804, 0xc804, c1943_c804_w },	/* ROM bank switch, screen flip */
	{ 0xc806, 0xc806, watchdog_reset_w },
	{ 0xc807, 0xc807, MWA_NOP }, 	/* protection chip write (we don't emulate it) */
	{ 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
	{ 0xd400, 0xd7ff, colorram_w, &colorram },
	{ 0xd800, 0xd801, MWA_RAM, &c1943_scrolly },
	{ 0xd802, 0xd802, MWA_RAM, &c1943_scrollx },
	{ 0xd803, 0xd804, MWA_RAM, &c1943_bgscrolly },
	{ 0xd806, 0xd806, c1943_d806_w },	/* sprites, bg1, bg2 enable */
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
	{ 0xc000, 0xc7ff, MWA_RAM },
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
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* actually, this is VBLANK */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Button 3, probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Button 3, probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0f, "1 (Easiest)" )
	PORT_DIPSETTING(    0x0e, "2" )
	PORT_DIPSETTING(    0x0d, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x0b, "5" )
	PORT_DIPSETTING(    0x0a, "6" )
	PORT_DIPSETTING(    0x09, "7" )
	PORT_DIPSETTING(    0x08, "8" )
	PORT_DIPSETTING(    0x07, "9" )
	PORT_DIPSETTING(    0x06, "10" )
	PORT_DIPSETTING(    0x05, "11" )
	PORT_DIPSETTING(    0x04, "12" )
	PORT_DIPSETTING(    0x03, "13" )
	PORT_DIPSETTING(    0x02, "14" )
	PORT_DIPSETTING(    0x01, "15" )
	PORT_DIPSETTING(    0x00, "16 (Hardest)" )
	PORT_DIPNAME( 0x10, 0x10, "2 Players Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Credits" )
	PORT_DIPNAME( 0x20, 0x20, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0x40, 0x40, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off")
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/5 Credits" )
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x18, "1 Coin/5 Credits" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x40, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	2048,	/* 2048 sprites */
	4,	/* 4 bits per pixel */
	{ 2048*64*8+4, 2048*64*8+0, 4, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	{ 33*8+3, 33*8+2, 33*8+1, 33*8+0, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
			8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	64*8	/* every sprite takes 64 consecutive bytes */
};
static struct GfxLayout fgtilelayout =
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
static struct GfxLayout bgtilelayout =
{
	32,32,  /* 32*32 tiles */
	128,    /* 128 tiles */
	4,      /* 4 bits per pixel */
	{ 128*256*8+4, 128*256*8+0, 4, 0 },
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
	{ 1, 0x00000, &charlayout,                  0, 32 },
	{ 1, 0x08000, &fgtilelayout,             32*4, 16 },
	{ 1, 0x48000, &bgtilelayout,       32*4+16*16, 16 },
	{ 1, 0x58000, &spritelayout, 32*4+16*16+16*16, 16 },
	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz */
	{ YM2203_VOL(20,25), YM2203_VOL(20,25) },
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
			6000000,	/* 6 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 Mhz */
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
	256,32*4+16*16+16*16+16*16,
	c1943_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	c1943_vh_start,
	c1943_vh_stop,
	c1943_vh_screenrefresh,

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

ROM_START( c1943_rom )
	ROM_REGION(0x30000)	/* 64k for code + 128k for the banked ROMs images */
	ROM_LOAD( "1943.01",      0x00000, 0x08000, 0xc686cc5c )
	ROM_LOAD( "1943.02",      0x10000, 0x10000, 0xd8880a41 )
	ROM_LOAD( "1943.03",      0x20000, 0x10000, 0x3f0ee26c )

	ROM_REGION_DISPOSE(0x98000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1943.04",      0x00000, 0x8000, 0x46cb9d3d )	/* characters */
	ROM_LOAD( "1943.15",      0x08000, 0x8000, 0x6b1a0443 )	/* bg tiles */
	ROM_LOAD( "1943.16",      0x10000, 0x8000, 0x23c908c2 )
	ROM_LOAD( "1943.17",      0x18000, 0x8000, 0x46bcdd07 )
	ROM_LOAD( "1943.18",      0x20000, 0x8000, 0xe6ae7ba0 )
	ROM_LOAD( "1943.19",      0x28000, 0x8000, 0x868ababc )
	ROM_LOAD( "1943.20",      0x30000, 0x8000, 0x0917e5d4 )
	ROM_LOAD( "1943.21",      0x38000, 0x8000, 0x9bfb0d89 )
	ROM_LOAD( "1943.22",      0x40000, 0x8000, 0x04f3c274 )
	ROM_LOAD( "1943.24",      0x48000, 0x8000, 0x11134036 )	/* fg tiles */
	ROM_LOAD( "1943.25",      0x50000, 0x8000, 0x092cf9c1 )
	ROM_LOAD( "1943.06",      0x58000, 0x8000, 0x97acc8af )	/* sprites */
	ROM_LOAD( "1943.07",      0x60000, 0x8000, 0xd78f7197 )
	ROM_LOAD( "1943.08",      0x68000, 0x8000, 0x1a626608 )
	ROM_LOAD( "1943.09",      0x70000, 0x8000, 0x92408400 )
	ROM_LOAD( "1943.10",      0x78000, 0x8000, 0x8438a44a )
	ROM_LOAD( "1943.11",      0x80000, 0x8000, 0x6c69351d )
	ROM_LOAD( "1943.12",      0x88000, 0x8000, 0x5e7efdb7 )
	ROM_LOAD( "1943.13",      0x90000, 0x8000, 0x1143829a )

	ROM_REGION(0x0a00)	/* color PROMs */
	ROM_LOAD( "bmprom.01",    0x0000, 0x0100, 0x74421f18 )	/* red component */
	ROM_LOAD( "bmprom.02",    0x0100, 0x0100, 0xac27541f )	/* green component */
	ROM_LOAD( "bmprom.03",    0x0200, 0x0100, 0x251fb6ff )	/* blue component */
	ROM_LOAD( "bmprom.05",    0x0300, 0x0100, 0x206713d0 )	/* char lookup table */
	ROM_LOAD( "bmprom.10",    0x0400, 0x0100, 0x33c2491c )	/* foreground lookup table */
	ROM_LOAD( "bmprom.09",    0x0500, 0x0100, 0xaeea4af7 )	/* foreground palette bank */
	ROM_LOAD( "bmprom.12",    0x0600, 0x0100, 0xc18aa136 )	/* background lookup table */
	ROM_LOAD( "bmprom.11",    0x0700, 0x0100, 0x405aae37 )	/* background palette bank */
	ROM_LOAD( "bmprom.08",    0x0800, 0x0100, 0xc2010a9e )	/* sprite lookup table */
	ROM_LOAD( "bmprom.07",    0x0900, 0x0100, 0xb56f30c3 )	/* sprite palette bank */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "1943.05",      0x00000, 0x8000, 0xee2bd2d7 )

	ROM_REGION(0x10000)
	ROM_LOAD( "1943.14",      0x0000, 0x8000, 0x4d3c6401 )	/* front background */
	ROM_LOAD( "1943.23",      0x8000, 0x8000, 0xa52aecbd )	/* back background */
ROM_END

ROM_START( c1943jap_rom )
	ROM_REGION(0x30000)	/* 64k for code + 128k for the banked ROMs images */
	ROM_LOAD( "1943jap.001",  0x00000, 0x08000, 0xf6935937 )
	ROM_LOAD( "1943jap.002",  0x10000, 0x10000, 0xaf971575 )
	ROM_LOAD( "1943jap.003",  0x20000, 0x10000, 0x300ec713 )

	ROM_REGION_DISPOSE(0x98000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1943.04",      0x00000, 0x8000, 0x46cb9d3d )	/* characters */
	ROM_LOAD( "1943.15",      0x08000, 0x8000, 0x6b1a0443 )	/* bg tiles */
	ROM_LOAD( "1943.16",      0x10000, 0x8000, 0x23c908c2 )
	ROM_LOAD( "1943.17",      0x18000, 0x8000, 0x46bcdd07 )
	ROM_LOAD( "1943.18",      0x20000, 0x8000, 0xe6ae7ba0 )
	ROM_LOAD( "1943.19",      0x28000, 0x8000, 0x868ababc )
	ROM_LOAD( "1943.20",      0x30000, 0x8000, 0x0917e5d4 )
	ROM_LOAD( "1943.21",      0x38000, 0x8000, 0x9bfb0d89 )
	ROM_LOAD( "1943.22",      0x40000, 0x8000, 0x04f3c274 )
	ROM_LOAD( "1943.24",      0x48000, 0x8000, 0x11134036 )	/* fg tiles */
	ROM_LOAD( "1943.25",      0x50000, 0x8000, 0x092cf9c1 )
	ROM_LOAD( "1943.06",      0x58000, 0x8000, 0x97acc8af )	/* sprites */
	ROM_LOAD( "1943.07",      0x60000, 0x8000, 0xd78f7197 )
	ROM_LOAD( "1943.08",      0x68000, 0x8000, 0x1a626608 )
	ROM_LOAD( "1943.09",      0x70000, 0x8000, 0x92408400 )
	ROM_LOAD( "1943.10",      0x78000, 0x8000, 0x8438a44a )
	ROM_LOAD( "1943.11",      0x80000, 0x8000, 0x6c69351d )
	ROM_LOAD( "1943.12",      0x88000, 0x8000, 0x5e7efdb7 )
	ROM_LOAD( "1943.13",      0x90000, 0x8000, 0x1143829a )

	ROM_REGION(0x0a00)	/* color PROMs */
	ROM_LOAD( "bmprom.01",    0x0000, 0x0100, 0x74421f18 )	/* red component */
	ROM_LOAD( "bmprom.02",    0x0100, 0x0100, 0xac27541f )	/* green component */
	ROM_LOAD( "bmprom.03",    0x0200, 0x0100, 0x251fb6ff )	/* blue component */
	ROM_LOAD( "bmprom.05",    0x0300, 0x0100, 0x206713d0 )	/* char lookup table */
	ROM_LOAD( "bmprom.10",    0x0400, 0x0100, 0x33c2491c )	/* foreground lookup table */
	ROM_LOAD( "bmprom.09",    0x0500, 0x0100, 0xaeea4af7 )	/* foreground palette bank */
	ROM_LOAD( "bmprom.12",    0x0600, 0x0100, 0xc18aa136 )	/* background lookup table */
	ROM_LOAD( "bmprom.11",    0x0700, 0x0100, 0x405aae37 )	/* background palette bank */
	ROM_LOAD( "bmprom.08",    0x0800, 0x0100, 0xc2010a9e )	/* sprite lookup table */
	ROM_LOAD( "bmprom.07",    0x0900, 0x0100, 0xb56f30c3 )	/* sprite palette bank */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "1943.05",      0x00000, 0x8000, 0xee2bd2d7 )

	ROM_REGION(0x10000)
	ROM_LOAD( "1943.14",      0x0000, 0x8000, 0x4d3c6401 )	/* front background */
	ROM_LOAD( "1943.23",      0x8000, 0x8000, 0xa52aecbd )	/* back background */
ROM_END

ROM_START( c1943kai_rom )
	ROM_REGION(0x30000)	/* 64k for code + 128k for the banked ROMs images */
	ROM_LOAD( "1943kai.01",   0x00000, 0x08000, 0x7d2211db )
	ROM_LOAD( "1943kai.02",   0x10000, 0x10000, 0x2ebbc8c5 )
	ROM_LOAD( "1943kai.03",   0x20000, 0x10000, 0x475a6ac5 )

	ROM_REGION_DISPOSE(0x98000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1943kai.04",   0x00000, 0x8000, 0x884a8692 )	/* characters */
	ROM_LOAD( "1943kai.15",   0x08000, 0x8000, 0x6b1a0443 )	/* bg tiles */
	ROM_LOAD( "1943kai.16",   0x10000, 0x8000, 0x9416fe0d )
	ROM_LOAD( "1943kai.17",   0x18000, 0x8000, 0x3d5acab9 )
	ROM_LOAD( "1943kai.18",   0x20000, 0x8000, 0x7b62da1d )
	ROM_LOAD( "1943kai.19",   0x28000, 0x8000, 0x868ababc )
	ROM_LOAD( "1943kai.20",   0x30000, 0x8000, 0xb90364c1 )
	ROM_LOAD( "1943kai.21",   0x38000, 0x8000, 0x8c7fe74a )
	ROM_LOAD( "1943kai.22",   0x40000, 0x8000, 0xd5ef8a0e )
	ROM_LOAD( "1943kai.24",   0x48000, 0x8000, 0xbf186ef2 )	/* fg tiles */
	ROM_LOAD( "1943kai.25",   0x50000, 0x8000, 0xa755faf1 )
	ROM_LOAD( "1943kai.06",   0x58000, 0x8000, 0x5f7e38b3 )	/* sprites */
	ROM_LOAD( "1943kai.07",   0x60000, 0x8000, 0xff3751fd )
	ROM_LOAD( "1943kai.08",   0x68000, 0x8000, 0x159d51bd )
	ROM_LOAD( "1943kai.09",   0x70000, 0x8000, 0x8683e3d2 )
	ROM_LOAD( "1943kai.10",   0x78000, 0x8000, 0x1e0d9571 )
	ROM_LOAD( "1943kai.11",   0x80000, 0x8000, 0xf1fc5ee1 )
	ROM_LOAD( "1943kai.12",   0x88000, 0x8000, 0x0f50c001 )
	ROM_LOAD( "1943kai.13",   0x90000, 0x8000, 0xfd1acf8e )

	ROM_REGION(0x0a00)	/* color PROMs */
	ROM_LOAD( "bmk01.bin",    0x0000, 0x0100, 0xe001ea33 )	/* red component */
	ROM_LOAD( "bmk02.bin",    0x0100, 0x0100, 0xaf34d91a )	/* green component */
	ROM_LOAD( "bmk03.bin",    0x0200, 0x0100, 0x43e9f6ef )	/* blue component */
	ROM_LOAD( "bmk05.bin",    0x0300, 0x0100, 0x41878934 )	/* char lookup table */
	ROM_LOAD( "bmk10.bin",    0x0400, 0x0100, 0xde44b748 )	/* foreground lookup table */
	ROM_LOAD( "bmk09.bin",    0x0500, 0x0100, 0x59ea57c0 )	/* foreground palette bank */
	ROM_LOAD( "bmk12.bin",    0x0600, 0x0100, 0x8765f8b0 )	/* background lookup table */
	ROM_LOAD( "bmk11.bin",    0x0700, 0x0100, 0x87a8854e )	/* background palette bank */
	ROM_LOAD( "bmk08.bin",    0x0800, 0x0100, 0xdad17e2d )	/* sprite lookup table */
	ROM_LOAD( "bmk07.bin",    0x0900, 0x0100, 0x76307f8d )	/* sprite palette bank */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "1943kai.05",   0x00000, 0x8000, 0x25f37957 )

	ROM_REGION(0x10000)
	ROM_LOAD( "1943kai.14",   0x0000, 0x8000, 0xcf0f5a53 )	/* front background */
	ROM_LOAD( "1943kai.23",   0x8000, 0x8000, 0x17f77ef9 )	/* back background */
ROM_END



static int c1943_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xe600],"\x00\x00\x00\x02\x00\x00\x00\x00\x1D\x0A\x0E\x24\x24\x24\x24\x24",16) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			// High score table.
			osd_fread(f,&RAM[0xe600],0x60);

			// High score.
			osd_fread(f,&RAM[0xe110],8);

			// High score screen.
			osd_fread(f,&RAM[0xd1be],1);
			osd_fread(f,&RAM[0xd1de],1);
			osd_fread(f,&RAM[0xd1fe],1);
			osd_fread(f,&RAM[0xd21e],1);
			osd_fread(f,&RAM[0xd23e],1);
			osd_fread(f,&RAM[0xd25e],1);
			osd_fread(f,&RAM[0xd27e],1);

			osd_fclose(f);
		}

		return 1;
	}
	else return 0; /* we can't load the hi scores yet */
}

static void c1943_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		// High score table.
		osd_fwrite(f,&RAM[0xe600],0x60);

		// High score.
		osd_fwrite(f,&RAM[0xe110],8);

		// High score screen.
		osd_fwrite(f,&RAM[0xd1be],1);
		osd_fwrite(f,&RAM[0xd1de],1);
		osd_fwrite(f,&RAM[0xd1fe],1);
		osd_fwrite(f,&RAM[0xd21e],1);
		osd_fwrite(f,&RAM[0xd23e],1);
		osd_fwrite(f,&RAM[0xd25e],1);
		osd_fwrite(f,&RAM[0xd27e],1);

		osd_fclose(f);
	}
}


static int c1943kai_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xe600],"\x00\x00\x02\x00\x00\x00\x00\x00\x1D\x0A\x0E\x24\x24\x24\x24\x24",16) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			// High score table.
			osd_fread(f,&RAM[0xe600],0x60);

			// High score.
			osd_fread(f,&RAM[0xe110],8);

			// High score screen.
			osd_fread(f,&RAM[0xd1be],1);
			osd_fread(f,&RAM[0xd1de],1);
			osd_fread(f,&RAM[0xd1fe],1);
			osd_fread(f,&RAM[0xd21e],1);
			osd_fread(f,&RAM[0xd23e],1);
			osd_fread(f,&RAM[0xd25e],1);
			osd_fread(f,&RAM[0xd27e],1);

			osd_fclose(f);
		}

		return 1;
	}
	else return 0; /* we can't load the hi scores yet */
}

static void c1943kai_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		// High score table.
		osd_fwrite(f,&RAM[0xe600],0x60);

		// High score.
		osd_fwrite(f,&RAM[0xe110],8);

		// High score screen.
		osd_fwrite(f,&RAM[0xd1be],1);
		osd_fwrite(f,&RAM[0xd1de],1);
		osd_fwrite(f,&RAM[0xd1fe],1);
		osd_fwrite(f,&RAM[0xd21e],1);
		osd_fwrite(f,&RAM[0xd23e],1);
		osd_fwrite(f,&RAM[0xd25e],1);
		osd_fwrite(f,&RAM[0xd27e],1);

		osd_fclose(f);
	}
}



struct GameDriver c1943_driver =
{
	__FILE__,
	0,
	"1943",
	"1943 (US)",
	"1987",
	"Capcom",
	"Mirko Buffoni (MAME driver)\nPaul Leaman (MAME driver)\nNicola Salmoria (MAME driver)\nTim Lindquist (color info)",
	0,
	&machine_driver,
	0,

	c1943_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	c1943_hiload, c1943_hisave
};

struct GameDriver c1943jap_driver =
{
	__FILE__,
	&c1943_driver,
	"1943jap",
	"1943 (Japan)",
	"1987",
	"Capcom",
	"Mirko Buffoni (MAME driver)\nPaul Leaman (MAME driver)\nNicola Salmoria (MAME driver)\nTim Lindquist (color info)",
	0,
	&machine_driver,
	0,

	c1943jap_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	c1943_hiload, c1943_hisave
};

struct GameDriver c1943kai_driver =
{
	__FILE__,
	0,
	"1943kai",
	"1943 Kai",
	"1987",
	"Capcom",
	"Mirko Buffoni (MAME driver)\nPaul Leaman (MAME driver)\nNicola Salmoria (MAME driver)\nTim Lindquist (color info)",
	0,
	&machine_driver,
	0,

	c1943kai_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	c1943kai_hiload, c1943kai_hisave
};
