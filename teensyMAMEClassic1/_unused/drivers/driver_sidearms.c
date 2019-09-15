/***************************************************************************

  Sidearms
  ========

  Driver provided by Paul Leaman

  There is an additional ROM which seems to contain code for a third Z80,
  however the board only has two. The ROM is related to the missing star
  background. At one point, the code jumps to A000, outside of the ROM
  address space.
  This ROM could be something entirely different from Z80 code. In another
  set, it consists of only the second half of the one we have here.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *sidearms_bg_scrollx,*sidearms_bg_scrolly;
extern unsigned char *sidearms_bg2_scrollx,*sidearms_bg2_scrolly;

void sidearms_c804_w(int offset, int data);
int  sidearms_vh_start(void);
void sidearms_vh_stop(void);
void sidearms_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void sidearms_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static void sidearms_bankswitch_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* bits 0 and 1 select the ROM bank */
	bankaddress = 0x10000 + (data & 0x0f) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc800, 0xc800, input_port_0_r },
	{ 0xc801, 0xc801, input_port_1_r },
	{ 0xc802, 0xc802, input_port_2_r },
	{ 0xc803, 0xc803, input_port_3_r },
	{ 0xc804, 0xc804, input_port_4_r },
	{ 0xc805, 0xc805, input_port_5_r },
	{ 0xd000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc3ff, paletteram_xxxxBBBBRRRRGGGG_split1_w, &paletteram },
	{ 0xc400, 0xc7ff, paletteram_xxxxBBBBRRRRGGGG_split2_w, &paletteram_2 },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xc801, 0xc801, sidearms_bankswitch_w },
	{ 0xc802, 0xc802, MWA_NOP },	/* watchdog reset? */
	{ 0xc804, 0xc804, sidearms_c804_w },
	{ 0xc805, 0xc805, MWA_RAM, &sidearms_bg2_scrollx },
	{ 0xc806, 0xc806, MWA_RAM, &sidearms_bg2_scrolly },
	{ 0xc808, 0xc809, MWA_RAM, &sidearms_bg_scrollx },
	{ 0xc80a, 0xc80b, MWA_RAM, &sidearms_bg_scrolly },
	{ 0xd000, 0xd7ff, videoram_w, &videoram, &videoram_size },
	{ 0xd800, 0xdfff, colorram_w, &colorram },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xffff, MWA_RAM, &spriteram, &spriteram_size },
	{ -1 }	/* end of table */
};

#ifdef THIRD_CPU
static void pop(int offset,int data)
{
RAM[0xa000] = 0xc3;
RAM[0xa001] = 0x00;
RAM[0xa002] = 0xa0;
//	interrupt_enable_w(offset,data & 0x80);
}

static struct MemoryReadAddress readmem2[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe3ff, MRA_RAM },
	{ 0xe400, 0xe7ff, MRA_RAM },
	{ 0xe800, 0xebff, MRA_RAM },
	{ 0xec00, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem2[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe3ff, MWA_RAM },
	{ 0xe400, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xebff, MWA_RAM },
	{ 0xec00, 0xefff, MWA_RAM },
	{ 0xf80e, 0xf80e, pop },	/* ROM bank selector? (to appear at 8000) */
	{ -1 }	/* end of table */
};
#endif

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd000, 0xd000, soundlatch_r },
	{ 0xf000, 0xf000, YM2203_status_port_0_r },
	{ 0xf002, 0xf002, YM2203_status_port_1_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xf000, YM2203_control_port_0_w },
	{ 0xf001, 0xf001, YM2203_write_port_0_w },
	{ 0xf002, 0xf002, YM2203_control_port_1_w },
	{ 0xf003, 0xf003, YM2203_write_port_1_w },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x08, "Freeze", IP_KEY_NONE )	/* I don't think it's really a dip switch */
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x07, 0x07, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "0 (Easiest)")
	PORT_DIPSETTING(    0x06, "1")
	PORT_DIPSETTING(    0x05, "2")
	PORT_DIPSETTING(    0x04, "3")
	PORT_DIPSETTING(    0x03, "4")
	PORT_DIPSETTING(    0x02, "5")
	PORT_DIPSETTING(    0x01, "6")
	PORT_DIPSETTING(    0x00, "7 (Hardest)")
	PORT_DIPNAME( 0x08, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "5")
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "100000" )
	PORT_DIPSETTING(    0x20, "100000 100000" )
	PORT_DIPSETTING(    0x10, "150000 150000" )
	PORT_DIPSETTING(    0x00, "200000 200000" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x01, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x03, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit")
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x18, "1 Coin/6 Credits")
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit")
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No")
	PORT_DIPSETTING(    0x40, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, "Demo sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off")
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* DSW1 */
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )	/* not sure, but likely */
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	2048,	/* 2048 sprites */
	4,	/* 4 bits per pixel */
	{ 2048*64*8+4, 2048*64*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	32,32,  /* 32*32 tiles */
	512,    /* 512 tiles */
	4,      /* 4 bits per pixel */
	{ 512*256*8+4, 512*256*8+0, 4, 0 },
	{
		      0,       1,       2,       3,       8+0,       8+1,       8+2,       8+3,
		32*16+0, 32*16+1, 32*16+2, 32*16+3, 32*16+8+0, 32*16+8+1, 32*16+8+2, 32*16+8+3,
		64*16+0, 64*16+1, 64*16+2, 64*16+3, 64*16+8+0, 64*16+8+1, 64*16+8+2, 64*16+8+3,
		96*16+0, 96*16+1, 96*16+2, 96*16+3, 96*16+8+0, 96*16+8+1, 96*16+8+2, 96*16+8+3,
	},
	{
		 0*16,  1*16,  2*16,  3*16,  4*16,  5*16,  6*16,  7*16,
		 8*16,  9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
		16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
		24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16
	},
	256*8	/* every tile takes 256 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/*   start    pointer       colour start   number of colours */
	{ 1, 0x00000, &charlayout,   768, 64 },	/* colors 768-1023 */
	{ 1, 0x08000, &tilelayout,     0, 32 },	/* colors   0-511 */
	{ 1, 0x48000, &spritelayout, 512, 16 },	/* colors 512-767 */
	{ -1 } /* end of array */
};



/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(void)
{
	cpu_cause_interrupt(1,0xff);
}

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	3500000,	/* 3.5 MHz ? (hand tuned) */
	{ YM2203_VOL(15,35), YM2203_VOL(15,35) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};



static struct MachineDriver sidearms_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 Mhz (?) */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the YM2203 */
		},
#ifdef THIRD_CPU
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			4,	/* memory region #4 */
			readmem2,writemem2,0,0,
			nmi_interrupt,1
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	48*8, 32*8, { 0*8, 48*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	sidearms_vh_start,
	sidearms_vh_stop,
	sidearms_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};



ROM_START( sidearms_rom )
	ROM_REGION(0x20000)	/* 64k for code + banked ROMs images */
	ROM_LOAD( "sa03.bin",     0x00000, 0x08000, 0xe10fe6a0 )	/* CODE */
	ROM_LOAD( "a_14e.rom",    0x10000, 0x08000, 0x4925ed03 )	/* 0+1 */
	ROM_LOAD( "a_12e.rom",    0x18000, 0x08000, 0x81d0ece7 )	/* 2+3 */

	ROM_REGION_DISPOSE(0x88000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a_10j.rom",    0x00000, 0x4000, 0x651fef75 )	/* characters */
	ROM_LOAD( "b_13d.rom",    0x08000, 0x8000, 0x3c59afe1 )	/* tiles */
	ROM_LOAD( "b_13e.rom",    0x10000, 0x8000, 0x64bc3b77 )
	ROM_LOAD( "b_13f.rom",    0x18000, 0x8000, 0xe6bcea6f )
	ROM_LOAD( "b_13g.rom",    0x20000, 0x8000, 0xc71a3053 )
	ROM_LOAD( "b_14d.rom",    0x28000, 0x8000, 0x826e8a97 )
	ROM_LOAD( "b_14e.rom",    0x30000, 0x8000, 0x6cfc02a4 )
	ROM_LOAD( "b_14f.rom",    0x38000, 0x8000, 0x9b9f6730 )
	ROM_LOAD( "b_14g.rom",    0x40000, 0x8000, 0xef6af630 )
	ROM_LOAD( "b_11b.rom",    0x48000, 0x8000, 0xeb6f278c )	/* sprites */
	ROM_LOAD( "b_13b.rom",    0x50000, 0x8000, 0xe91b4014 )
	ROM_LOAD( "b_11a.rom",    0x58000, 0x8000, 0x2822c522 )
	ROM_LOAD( "b_13a.rom",    0x60000, 0x8000, 0x3e8a9f75 )
	ROM_LOAD( "b_12b.rom",    0x68000, 0x8000, 0x86e43eda )
	ROM_LOAD( "b_14b.rom",    0x70000, 0x8000, 0x076e92d1 )
	ROM_LOAD( "b_12a.rom",    0x78000, 0x8000, 0xce107f3c )
	ROM_LOAD( "b_14a.rom",    0x80000, 0x8000, 0xdba06076 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "a_04k.rom",    0x0000, 0x8000, 0x34efe2d2 )

	ROM_REGION(0x08000)	/* 32k tile map */
	ROM_LOAD( "b_03d.rom",    0x0000, 0x8000, 0x6f348008 )

#ifdef THIRD_CPU
	ROM_REGION(0x10000)	/* 64k for CPU */
	ROM_LOAD( "b_11j.rom",    0x0000, 0x8000, 0x0 )
#endif
ROM_END

ROM_START( sidearmr_rom )
	ROM_REGION(0x20000)	/* 64k for code + banked ROMs images */
	ROM_LOAD( "03",           0x00000, 0x08000, 0x9a799c45 )	/* CODE */
	ROM_LOAD( "a_14e.rom",    0x10000, 0x08000, 0x4925ed03 )	/* 0+1 */
	ROM_LOAD( "a_12e.rom",    0x18000, 0x08000, 0x81d0ece7 )	/* 2+3 */

	ROM_REGION_DISPOSE(0x88000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a_10j.rom",    0x00000, 0x4000, 0x651fef75 )	/* characters */
	ROM_LOAD( "b_13d.rom",    0x08000, 0x8000, 0x3c59afe1 )	/* tiles */
	ROM_LOAD( "b_13e.rom",    0x10000, 0x8000, 0x64bc3b77 )
	ROM_LOAD( "b_13f.rom",    0x18000, 0x8000, 0xe6bcea6f )
	ROM_LOAD( "b_13g.rom",    0x20000, 0x8000, 0xc71a3053 )
	ROM_LOAD( "b_14d.rom",    0x28000, 0x8000, 0x826e8a97 )
	ROM_LOAD( "b_14e.rom",    0x30000, 0x8000, 0x6cfc02a4 )
	ROM_LOAD( "b_14f.rom",    0x38000, 0x8000, 0x9b9f6730 )
	ROM_LOAD( "b_14g.rom",    0x40000, 0x8000, 0xef6af630 )
	ROM_LOAD( "b_11b.rom",    0x48000, 0x8000, 0xeb6f278c )	/* sprites */
	ROM_LOAD( "b_13b.rom",    0x50000, 0x8000, 0xe91b4014 )
	ROM_LOAD( "b_11a.rom",    0x58000, 0x8000, 0x2822c522 )
	ROM_LOAD( "b_13a.rom",    0x60000, 0x8000, 0x3e8a9f75 )
	ROM_LOAD( "b_12b.rom",    0x68000, 0x8000, 0x86e43eda )
	ROM_LOAD( "b_14b.rom",    0x70000, 0x8000, 0x076e92d1 )
	ROM_LOAD( "b_12a.rom",    0x78000, 0x8000, 0xce107f3c )
	ROM_LOAD( "b_14a.rom",    0x80000, 0x8000, 0xdba06076 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "a_04k.rom",    0x0000, 0x8000, 0x34efe2d2 )

	ROM_REGION(0x08000)	/* 32k tile map */
	ROM_LOAD( "b_03d.rom",    0x0000, 0x8000, 0x6f348008 )

#ifdef THIRD_CPU
	ROM_REGION(0x10000)	/* 64k for CPU */
	ROM_LOAD( "b_11j.rom",    0x0000, 0x8000, 0x0 )
#endif
ROM_END

ROM_START( sidearjp_rom )
	ROM_REGION(0x20000)	/* 64k for code + banked ROMs images */
	ROM_LOAD( "a_15e.rom",    0x00000, 0x08000, 0x61ceb0cc )	/* CODE */
	ROM_LOAD( "a_14e.rom",    0x10000, 0x08000, 0x4925ed03 )	/* 0+1 */
	ROM_LOAD( "a_12e.rom",    0x18000, 0x08000, 0x81d0ece7 )	/* 2+3 */

	ROM_REGION_DISPOSE(0x88000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a_10j.rom",    0x00000, 0x4000, 0x651fef75 )	/* characters */
	ROM_LOAD( "b_13d.rom",    0x08000, 0x8000, 0x3c59afe1 )	/* tiles */
	ROM_LOAD( "b_13e.rom",    0x10000, 0x8000, 0x64bc3b77 )
	ROM_LOAD( "b_13f.rom",    0x18000, 0x8000, 0xe6bcea6f )
	ROM_LOAD( "b_13g.rom",    0x20000, 0x8000, 0xc71a3053 )
	ROM_LOAD( "b_14d.rom",    0x28000, 0x8000, 0x826e8a97 )
	ROM_LOAD( "b_14e.rom",    0x30000, 0x8000, 0x6cfc02a4 )
	ROM_LOAD( "b_14f.rom",    0x38000, 0x8000, 0x9b9f6730 )
	ROM_LOAD( "b_14g.rom",    0x40000, 0x8000, 0xef6af630 )
	ROM_LOAD( "b_11b.rom",    0x48000, 0x8000, 0xeb6f278c )	/* sprites */
	ROM_LOAD( "b_13b.rom",    0x50000, 0x8000, 0xe91b4014 )
	ROM_LOAD( "b_11a.rom",    0x58000, 0x8000, 0x2822c522 )
	ROM_LOAD( "b_13a.rom",    0x60000, 0x8000, 0x3e8a9f75 )
	ROM_LOAD( "b_12b.rom",    0x68000, 0x8000, 0x86e43eda )
	ROM_LOAD( "b_14b.rom",    0x70000, 0x8000, 0x076e92d1 )
	ROM_LOAD( "b_12a.rom",    0x78000, 0x8000, 0xce107f3c )
	ROM_LOAD( "b_14a.rom",    0x80000, 0x8000, 0xdba06076 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "a_04k.rom",    0x0000, 0x8000, 0x34efe2d2 )

	ROM_REGION(0x08000)	/* 32k tile map */
	ROM_LOAD( "b_03d.rom",    0x0000, 0x8000, 0x6f348008 )

#ifdef THIRD_CPU
	ROM_REGION(0x10000)	/* 64k for CPU */
	ROM_LOAD( "b_11j.rom",    0x0000, 0x8000, 0x0 )
#endif
ROM_END


static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0xe680],"\x00\x00\x00\x01\x00\x00\x00\x00",8) == 0)
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



struct GameDriver sidearms_driver =
{
	__FILE__,
	0,
	"sidearms",
	"Sidearms (World)",
	"1986",
	"Capcom",
	"Paul Leaman (MAME driver)\nNicola Salmoria (additional code)",
	0,
	&sidearms_machine_driver,
	0,

	sidearms_rom,
	0,0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver sidearmr_driver =
{
	__FILE__,
	&sidearms_driver,
	"sidearmr",
	"Sidearms (US)",
	"1986",
	"Capcom (Romstar license)",
	"Paul Leaman (MAME driver)\nNicola Salmoria (additional code)",
	0,
	&sidearms_machine_driver,
	0,

	sidearmr_rom,
	0,0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver sidearjp_driver =
{
	__FILE__,
	&sidearms_driver,
	"sidearjp",
	"Sidearms (Japan)",
	"1986",
	"Capcom",
	"Paul Leaman (MAME driver)\nNicola Salmoria (additional code)",
	0,
	&sidearms_machine_driver,
	0,

	sidearjp_rom,
	0,0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	hiload, hisave
};
