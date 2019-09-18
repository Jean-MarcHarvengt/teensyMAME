/***************************************************************************

Bagman memory map (preliminary)

0000-5fff ROM
6000-67ff RAM
9000-93ff Video RAM
9800-9bff Color RAM
9800-981f Sprites (hidden portion of color RAM)
9c00-9fff ? (filled with 3f, not used otherwise)
c000-ffff ROM (Super Bagman only)

memory mapped ports:

read:
a000      PAL16r6 output.
a800      ? (read only in one place, not used)
b000      DSW
b800      watchdog reset?

write:
a000      interrupt enable
a001      \ horizontal and vertical flip, I don't know which is which
a002      /
a003      video enable??
a004      coin counter
a007      ?
a800-a805 PAL16r6 inputs. This chip is custom logic used for guards controlling.
b000      ?
b800      ?

I/O ports:

I/O 8  ;AY-3-8910 Control Reg.
I/O 9  ;AY-3-8910 Data Write Reg.
I/O C  ;AY-3-8910 Data Read Reg.
        Port A of the 8910 is connected to IN0
        Port B of the 8910 is connected to IN1

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



int bagman_pal16r6_r(int offset);
void bagman_machine_init(void);
void bagman_pal16r6_w(int offset, int data);


extern unsigned char *bagman_video_enable;
void bagman_flipscreen_w(int offset,int data);
void bagman_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void bagman_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x67ff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },
	{ 0x9800, 0x9bff, MRA_RAM },
	{ 0xa000, 0xa000, bagman_pal16r6_r },
	{ 0xa800, 0xa800, MRA_NOP },
	{ 0xb000, 0xb000, input_port_2_r },	/* DSW */
	{ 0xb800, 0xb800, MRA_NOP },
	{ 0xc000, 0xffff, MRA_ROM },	/* Super Bagman only */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x67ff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0x9bff, colorram_w, &colorram },
	{ 0xa000, 0xa000, interrupt_enable_w },
	{ 0xa001, 0xa002, bagman_flipscreen_w },
	{ 0xa003, 0xa003, MWA_RAM, &bagman_video_enable },
	{ 0xc000, 0xffff, MWA_ROM },	/* Super Bagman only */
	{ 0x9800, 0x981f, MWA_RAM, &spriteram, &spriteram_size },	/* hidden portion of color RAM */
									/* here only to initialize the pointer, */
	{ 0xa7ff, 0xa805, bagman_pal16r6_w },	/* PAL16r6 custom logic */
														/* writes are handled by colorram_w */
	{ 0x9c00, 0x9fff, MWA_NOP },	/* written to, but unused */
	{ 0xa004, 0xa004, coin_counter_w },
#if 0
	{ 0xa007, 0xa007, MWA_NOP },	/* ???? */
	{ 0xb000, 0xb000, MWA_NOP },	/* ???? */
	{ 0xb800, 0xb800, MWA_NOP },	/* ???? */
#endif
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress pickin_readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x7000, 0x77ff, MRA_RAM },
	{ 0x8800, 0x8bff, MRA_RAM },
	{ 0x9800, 0x9bff, MRA_RAM },
//	{ 0xa000, 0xa000, bagman_pal16r6_r },
	{ 0xa800, 0xa800, input_port_2_r },
	{ 0xb800, 0xb800, MRA_NOP },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress pickin_writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x7000, 0x77ff, MWA_RAM },
	{ 0x8800, 0x8bff, videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0x9bff, colorram_w, &colorram },
	{ 0xa000, 0xa000, interrupt_enable_w },
	{ 0xa001, 0xa002, bagman_flipscreen_w },
	{ 0xa003, 0xa003, MWA_RAM, &bagman_video_enable },
	{ 0x9800, 0x981f, MWA_RAM, &spriteram, &spriteram_size },	/* hidden portion of color RAM */
									/* here only to initialize the pointer, */
//	{ 0xa7ff, 0xa805, bagman_pal16r6_w },	/* PAL16r6 custom logic */
														/* writes are handled by colorram_w */
	{ 0x9c00, 0x9fff, MWA_NOP },	/* written to, but unused */
	{ 0xa004, 0xa004, coin_counter_w },
#if 0
	{ 0xa007, 0xa007, MWA_NOP },	/* ???? */
	{ 0xb000, 0xb000, MWA_NOP },	/* ???? */
	{ 0xb800, 0xb800, MWA_NOP },	/* ???? */
#endif
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x0c, 0x0c, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x08, 0x08, AY8910_control_port_0_w },
	{ 0x09, 0x09, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( bagman_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x03, 0x02, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x18, 0x18, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "Easy" )
	PORT_DIPSETTING(    0x10, "Medium" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x20, 0x20, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "English" )
	PORT_DIPSETTING(    0x00, "French" )
	PORT_DIPNAME( 0x40, 0x40, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
INPUT_PORTS_END

/* EXACTLY the same as bagman, the only difference is that the START1 button */
/* also acts as the shoot button. */
INPUT_PORTS_START( sbagman_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* double-function button, start and shoot */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )	/* double-function button, start and shoot */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x03, 0x02, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x18, 0x18, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "Easy" )
	PORT_DIPSETTING(    0x10, "Medium" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x20, 0x20, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "English" )
	PORT_DIPSETTING(    0x00, "French" )
	PORT_DIPNAME( 0x40, 0x40, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
INPUT_PORTS_END

INPUT_PORTS_START( pickin_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x01, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2C/1C 1C/1C 1C/3C 1C/7C" )
	PORT_DIPSETTING(    0x01, "1C/1C 1C/2C 1C/6C 1C/14C" )
	PORT_DIPNAME( 0x06, 0x04, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x08, 0x08, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "English" )
	PORT_DIPSETTING(    0x00, "French" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 128*16*16 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,      0, 16 },	/* char set #1 */
	{ 1, 0x2000, &charlayout,      0, 16 },	/* char set #2 */
	{ 1, 0x0000, &spritelayout,    0, 16 },	/* sprites */
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz??? */
	{ 255 },
	{ input_port_0_r },
	{ input_port_1_r },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz (?) */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	bagman_machine_init,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	64,64,
	bagman_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	bagman_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

static struct MachineDriver pickin_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz (?) */
			0,
			pickin_readmem,pickin_writemem,readport,writeport,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	bagman_machine_init,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	64,64,
	bagman_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	bagman_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( bagman_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "e9_b05.bin",   0x0000, 0x1000, 0xe0156191 )
	ROM_LOAD( "f9_b06.bin",   0x1000, 0x1000, 0x7b758982 )
	ROM_LOAD( "f9_b07.bin",   0x2000, 0x1000, 0x302a077b )
	ROM_LOAD( "k9_b08.bin",   0x3000, 0x1000, 0xf04293cb )
	ROM_LOAD( "m9_b09s.bin",  0x4000, 0x1000, 0x68e83e4f )
	ROM_LOAD( "n9_b10.bin",   0x5000, 0x1000, 0x1d6579f7 )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "e1_b02.bin",   0x0000, 0x1000, 0x4a0a6b55 )
	ROM_LOAD( "j1_b04.bin",   0x1000, 0x1000, 0xc680ef04 )
	ROM_LOAD( "c1_b01.bin",   0x2000, 0x1000, 0x705193b2 )
	ROM_LOAD( "f1_b03s.bin",  0x3000, 0x1000, 0xdba1eda7 )

	ROM_REGION(0x0040)	/* color PROMs */
	ROM_LOAD( "p3.bin",       0x0000, 0x0020, 0x2a855523 )
	ROM_LOAD( "r3.bin",       0x0020, 0x0020, 0xae6f1019 )

	ROM_REGION(0x2000)	/* data for the TMS5110 speech chip */
	ROM_LOAD( "r9_b11.bin",   0x0000, 0x1000, 0x2e0057ff )
	ROM_LOAD( "t9_b12.bin",   0x1000, 0x1000, 0xb2120edd )
ROM_END

ROM_START( bagmans_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "a4_9e.bin",    0x0000, 0x1000, 0x5fb0a1a3 )
	ROM_LOAD( "a5-9f",        0x1000, 0x1000, 0x2ddf6bb9 )
	ROM_LOAD( "a4_9j.bin",    0x2000, 0x1000, 0xb2da8b77 )
	ROM_LOAD( "a5-9k",        0x3000, 0x1000, 0xf91d617b )
	ROM_LOAD( "a4_9m.bin",    0x4000, 0x1000, 0xb8e75eb6 )
	ROM_LOAD( "a5-9n",        0x5000, 0x1000, 0x68e4b64d )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a2_1e.bin",    0x0000, 0x1000, 0xf217ac09 )
	ROM_LOAD( "j1_b04.bin",   0x1000, 0x1000, 0xc680ef04 )
	ROM_LOAD( "a2_1c.bin",    0x2000, 0x1000, 0xf3e11bd7 )
	ROM_LOAD( "a2_1f.bin",    0x3000, 0x1000, 0xd0f7105b )

	ROM_REGION(0x0040)	/* color PROMs */
	ROM_LOAD( "p3.bin",       0x0000, 0x0020, 0x2a855523 )
	ROM_LOAD( "r3.bin",       0x0020, 0x0020, 0xae6f1019 )

	ROM_REGION(0x2000)	/* data for the TMS5110 speech chip */
	ROM_LOAD( "r9_b11.bin",   0x0000, 0x1000, 0x2e0057ff )
	ROM_LOAD( "t9_b12.bin",   0x1000, 0x1000, 0xb2120edd )
ROM_END

ROM_START( bagmans2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "a4_9e.bin",    0x0000, 0x1000, 0x5fb0a1a3 )
	ROM_LOAD( "a4_9f.bin",    0x1000, 0x1000, 0x7871206e )
	ROM_LOAD( "a4_9j.bin",    0x2000, 0x1000, 0xb2da8b77 )
	ROM_LOAD( "a4_9k.bin",    0x3000, 0x1000, 0x36b6a944 )
	ROM_LOAD( "a4_9m.bin",    0x4000, 0x1000, 0xb8e75eb6 )
	ROM_LOAD( "a4_9n.bin",    0x5000, 0x1000, 0x83fccb1c )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a2_1e.bin",    0x0000, 0x1000, 0xf217ac09 )
	ROM_LOAD( "a2_1j.bin",    0x1000, 0x1000, 0xc680ef04 )
	ROM_LOAD( "a2_1c.bin",    0x2000, 0x1000, 0xf3e11bd7 )
	ROM_LOAD( "a2_1f.bin",    0x3000, 0x1000, 0xd0f7105b )

	ROM_REGION(0x0040)	/* color PROMs */
	ROM_LOAD( "p3.bin",       0x0000, 0x0020, 0x2a855523 )
	ROM_LOAD( "r3.bin",       0x0020, 0x0020, 0xae6f1019 )

	ROM_REGION(0x2000)	/* data for the TMS5110 speech chip */
	ROM_LOAD( "r9_b11.bin",   0x0000, 0x1000, 0x2e0057ff )
	ROM_LOAD( "t9_b12.bin",   0x1000, 0x1000, 0xb2120edd )
ROM_END

ROM_START( sbagman_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "5.9e",         0x0000, 0x1000, 0x1b1d6b0a )
	ROM_LOAD( "6.9f",         0x1000, 0x1000, 0xac49cb82 )
	ROM_LOAD( "7.9j",         0x2000, 0x1000, 0x9a1c778d )
	ROM_LOAD( "8.9k",         0x3000, 0x1000, 0xb94fbb73 )
	ROM_LOAD( "9.9m",         0x4000, 0x1000, 0x601f34ba )
	ROM_LOAD( "10.9n",        0x5000, 0x1000, 0x5f750918 )
	ROM_LOAD( "13.8d",        0xc000, 0x0e00, 0x944a4453 )
	ROM_CONTINUE(             0xfe00, 0x0200 )
	ROM_LOAD( "14.8f",        0xd000, 0x0400, 0x83b10139 )
	ROM_CONTINUE(             0xe400, 0x0200 )
	ROM_CONTINUE(             0xd600, 0x0a00 )
	ROM_LOAD( "15.8j",        0xe000, 0x0400, 0xfe924879 )
	ROM_CONTINUE(             0xd400, 0x0200 )
	ROM_CONTINUE(             0xe600, 0x0a00 )
	ROM_LOAD( "16.8k",        0xf000, 0x0e00, 0xb77eb1f5 )
	ROM_CONTINUE(             0xce00, 0x0200 )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "2.1e",         0x0000, 0x1000, 0xf4d3d4e6 )
	ROM_LOAD( "4.1j",         0x1000, 0x1000, 0x2c6a510d )
	ROM_LOAD( "1.1c",         0x2000, 0x1000, 0xa046ff44 )
	ROM_LOAD( "3.1f",         0x3000, 0x1000, 0xa4422da4 )

	ROM_REGION(0x0040)	/* color PROMs */
	ROM_LOAD( "p3.bin",       0x0000, 0x0020, 0x2a855523 )
	ROM_LOAD( "r3.bin",       0x0020, 0x0020, 0xae6f1019 )

	ROM_REGION(0x2000)	/* data for the TMS5110 speech chip */
	ROM_LOAD( "11.9r",        0x0000, 0x1000, 0x2e0057ff )
	ROM_LOAD( "12.9t",        0x1000, 0x1000, 0xb2120edd )
ROM_END

ROM_START( sbagmans_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "sbag_9e.bin",  0x0000, 0x1000, 0xc19696f2 )
	ROM_LOAD( "6.9f",         0x1000, 0x1000, 0xac49cb82 )
	ROM_LOAD( "7.9j",         0x2000, 0x1000, 0x9a1c778d )
	ROM_LOAD( "8.9k",         0x3000, 0x1000, 0xb94fbb73 )
	ROM_LOAD( "sbag_9m.bin",  0x4000, 0x1000, 0xb21e246e )
	ROM_LOAD( "10.9n",        0x5000, 0x1000, 0x5f750918 )
	ROM_LOAD( "13.8d",        0xc000, 0x0e00, 0x944a4453 )
	ROM_CONTINUE(             0xfe00, 0x0200 )
	ROM_LOAD( "sbag_f8.bin",  0xd000, 0x0400, 0x0f3e6de4 )
	ROM_CONTINUE(             0xe400, 0x0200 )
	ROM_CONTINUE(             0xd600, 0x0a00 )
	ROM_LOAD( "15.8j",        0xe000, 0x0400, 0xfe924879 )
	ROM_CONTINUE(             0xd400, 0x0200 )
	ROM_CONTINUE(             0xe600, 0x0a00 )
	ROM_LOAD( "16.8k",        0xf000, 0x0e00, 0xb77eb1f5 )
	ROM_CONTINUE(             0xce00, 0x0200 )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "2.1e",         0x0000, 0x1000, 0xf4d3d4e6 )
	ROM_LOAD( "4.1j",         0x1000, 0x1000, 0x2c6a510d )
	ROM_LOAD( "sbag_1c.bin",  0x2000, 0x1000, 0x262f870a )
	ROM_LOAD( "sbag_1f.bin",  0x3000, 0x1000, 0x350ed0fb )

	ROM_REGION(0x0040)	/* color PROMs */
	ROM_LOAD( "p3.bin",       0x0000, 0x0020, 0x2a855523 )
	ROM_LOAD( "r3.bin",       0x0020, 0x0020, 0xae6f1019 )

	ROM_REGION(0x2000)	/* data for the TMS5110 speech chip */
	ROM_LOAD( "11.9r",        0x0000, 0x1000, 0x2e0057ff )
	ROM_LOAD( "12.9t",        0x1000, 0x1000, 0xb2120edd )
ROM_END

ROM_START( pickin_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "9e",           0x0000, 0x1000, 0xefd0bd43 )
	ROM_LOAD( "9f",           0x1000, 0x1000, 0xb5785a23 )
	ROM_LOAD( "9j",           0x2000, 0x1000, 0x65ee9fd4 )
	ROM_LOAD( "9k",           0x3000, 0x1000, 0x7b23350e )
	ROM_LOAD( "9m",           0x4000, 0x1000, 0x935a7248 )
	ROM_LOAD( "9n",           0x5000, 0x1000, 0x52485d1d )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1f",           0x0000, 0x1000, 0xc5e96ac6 )
	ROM_LOAD( "1j",           0x1000, 0x1000, 0x41c4ac1c )
	/* 2000-3fff empty for my convenience */

	ROM_REGION(0x0040)	/* color PROMs */
	ROM_LOAD( "6331-1.3p",    0x0000, 0x0020, 0xfac81668 )
	ROM_LOAD( "6331-1.3r",    0x0020, 0x0020, 0x14ee1603 )
ROM_END



static int hiload(void)
{


	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

		/* wait for "HIGH SCORE" to be on screen */
        if (memcmp(&RAM[0x6257],"\x00\x89\x01",3) == 0&& memcmp(&RAM[0x6217],"\x00\x42\x01",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{

                        /* Bagman allows for 13 letters */
						/* The rest only allow to enter three letters but
                         * we could enter until thirteen by accesing to memory
                         * directly. In Super Bagman the default high-score
                         * names have more than three letters */
                        osd_fread(f,&RAM[0x6217],5*16);


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

                osd_fwrite(f,&RAM[0x6217],5*16);
				osd_fclose(f);
				memset(&RAM[0x6217],0xff,5*16);
	}
}

static int pickin_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

		/* wait for "HIGH SCORE" to be on screen */

        if (memcmp(&RAM[0x71da],"\x00\x89\x01",3) == 0&& memcmp(&RAM[0x719a],"\x00\x42\x01",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{

                        /* Bagman allows for 13 letters */
						/* The rest only allow to enter three letters but
                         * we could enter until thirteen by accesing to memory
                         * directly. In Super Bagman the default high-score
                         * names have more than three letters */
                        osd_fread(f,&RAM[0x719a],80);


			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void pickin_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{

                osd_fwrite(f,&RAM[0x719a],80);
				osd_fclose(f);
				memset(&RAM[0x719a],0xff,80);
	}
}


struct GameDriver bagman_driver =
{
	__FILE__,
	0,
	"bagman",
	"Bagman",
	"1982",
	"Valadon Automation",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nJarek Burczynski (additional code)\nTim Lindquist (color info)\nAndrew Deschenes (protection info)",
	0,
	&machine_driver,
	0,

	bagman_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	bagman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver bagmans_driver =
{
	__FILE__,
	&bagman_driver,
	"bagmans",
	"Bagman (Stern set 1)",
	"1982",
	"Valadon Automation (Stern license)",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nJarek Burczynski (additional code)\nTim Lindquist (color info)\nAndrew Deschenes (protection info)",
	0,
	&machine_driver,
	0,

	bagmans_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	bagman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver bagmans2_driver =
{
	__FILE__,
	&bagman_driver,
	"bagmans2",
	"Bagman (Stern set 2)",
	"1982",
	"Valadon Automation (Stern license)",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nJarek Burczynski (additional code)\nTim Lindquist (color info)\nAndrew Deschenes (protection info)",
	0,
	&machine_driver,
	0,

	bagmans2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	bagman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver sbagman_driver =
{
	__FILE__,
	0,
	"sbagman",
	"Super Bagman",
	"1984",
	"Valadon Automation",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (Bagman driver)\nJarek Burczynski (MAME driver)\nTim Lindquist (color info)\nAndrew Deschenes (protection info)",
	0,
	&machine_driver,
	0,

	sbagman_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	sbagman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver sbagmans_driver =
{
	__FILE__,
	&sbagman_driver,
	"sbagmans",
	"Super Bagman (Stern)",
	"1984",
	"Valadon Automation (Stern license)",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (Bagman driver)\nJarek Burczynski (MAME driver)\nTim Lindquist (color info)\nAndrew Deschenes (protection info)",
	0,
	&machine_driver,
	0,

	sbagmans_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	sbagman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver pickin_driver =
{
	__FILE__,
	0,
	"pickin",
	"Pickin'",
	"1983",
	"Valadon Automation",
	"Robert Anschuetz (Arcade emulator)\nNicola Salmoria (MAME driver)\nJarek Burczynski (additional code)",
	0,
	&pickin_machine_driver,
	0,

	pickin_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	pickin_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	pickin_hiload,pickin_hisave
};
