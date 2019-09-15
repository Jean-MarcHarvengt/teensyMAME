/***************************************************************************

Mr Do! memory map (preliminary)

0000-7fff ROM
8000-83ff color RAM 1
8400-87ff video RAM 1
8800-8bff color RAM 2
8c00-8fff video RAM 2
e000-efff RAM

memory mapped ports:

read:
9803      SECRE 1/6-J2-11
a000      IN0
a001      IN1
a002      DSW1
a003      DSW2

write:
9000-90ff sprites, 64 groups of 4 bytes.
9800      flip (bit 0) playfield priority selector? (bits 1-3)
9801      sound port 1
9802      sound port 2
f000      playfield 0 Y scroll position (not used by Mr. Do!)
f800      playfield 0 X scroll position

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80.h"



extern unsigned char *mrdo_videoram2;
extern unsigned char *mrdo_colorram2;
extern unsigned char *mrdo_scroll_y;
void mrdo_videoram2_w(int offset,int data);
void mrdo_colorram2_w(int offset,int data);
void mrdo_flipscreen_w(int offset,int data);
void mrdo_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int mrdo_vh_start(void);
void mrdo_vh_stop(void);
void mrdo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



/* this looks like some kind of protection. The game doesn't clear the screen */
/* if a read from this address doesn't return the value it expects. */
int mrdo_SECRE_r(int offset)
{
	Z80_Regs regs;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	Z80_GetRegs(&regs);
	return RAM[regs.HL.D];
}



static struct MemoryReadAddress readmem[] =
{
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },	/* video and color RAM */
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa001, 0xa001, input_port_1_r },	/* IN1 */
	{ 0xa002, 0xa002, input_port_2_r },	/* DSW1 */
	{ 0xa003, 0xa003, input_port_3_r },	/* DSW2 */
	{ 0x9803, 0x9803, mrdo_SECRE_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0x8000, 0x83ff, colorram_w, &colorram },
	{ 0x8400, 0x87ff, videoram_w, &videoram, &videoram_size },
	{ 0x8800, 0x8bff, mrdo_colorram2_w, &mrdo_colorram2 },
	{ 0x8c00, 0x8fff, mrdo_videoram2_w, &mrdo_videoram2 },
	{ 0x9000, 0x90ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9800, 0x9800, mrdo_flipscreen_w },	/* screen flip + playfield priority */
	{ 0x9801, 0x9801, SN76496_0_w },
	{ 0x9802, 0x9802, SN76496_1_w },
	{ 0xf800, 0xffff, MWA_RAM, &mrdo_scroll_y },
	{ 0xf000, 0xf7ff, MWA_NOP },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_TILT )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Special", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x10, 0x10, "Extra", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_DIPNAME( 0xc0, 0xc0, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "5" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "Right Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x09, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "Free play" )
	/* settings 0x01 thru 0x05 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, "Left Coin", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x70, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x90, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "Free play" )
	/* settings 0x10 thru 0x50 all give 1 Coin/1 Credit */
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 4, 0 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0,
			16+3, 16+2, 16+1, 16+0, 24+3, 24+2, 24+1, 24+0 },
	{ 0*16, 2*16, 4*16, 6*16, 8*16, 10*16, 12*16, 14*16,
			16*16, 18*16, 20*16, 22*16, 24*16, 26*16, 28*16, 30*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 128 },
	{ 1, 0x2000, &charlayout,       0, 128 },
	{ 1, 0x4000, &spritelayout, 4*128,  16 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	4000000,	/* 4 MHz */
	{ 70, 70 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 4*8, 28*8-1 },
	gfxdecodeinfo,
	257,4*144,
	mrdo_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	mrdo_vh_start,
	mrdo_vh_stop,
	mrdo_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mrdo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "a4-01.bin",    0x0000, 0x2000, 0x03dcfba2 )
	ROM_LOAD( "c4-02.bin",    0x2000, 0x2000, 0x0ecdd39c )
	ROM_LOAD( "e4-03.bin",    0x4000, 0x2000, 0x358f5dc2 )
	ROM_LOAD( "f4-04.bin",    0x6000, 0x2000, 0xf4190cfc )

	ROM_REGION_DISPOSE(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "s8-09.bin",    0x0000, 0x1000, 0xaa80c5b6 )
	ROM_LOAD( "u8-10.bin",    0x1000, 0x1000, 0xd20ec85b )
	ROM_LOAD( "r8-08.bin",    0x2000, 0x1000, 0xdbdc9ffa )
	ROM_LOAD( "n8-07.bin",    0x3000, 0x1000, 0x4b9973db )
	ROM_LOAD( "h5-05.bin",    0x4000, 0x1000, 0xe1218cc5 )
	ROM_LOAD( "k5-06.bin",    0x5000, 0x1000, 0xb1f68b04 )

	ROM_REGION(0x0060)	/* color PROMs */
	ROM_LOAD( "u02--2.bin",   0x0000, 0x0020, 0x238a65d7 )	/* palette (high bits) */
	ROM_LOAD( "t02--3.bin",   0x0020, 0x0020, 0xae263dc0 )	/* palette (low bits) */
	ROM_LOAD( "f10--1.bin",   0x0040, 0x0020, 0x16ee4ca2 )	/* sprite color lookup table */
ROM_END

ROM_START( mrdot_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "d1",           0x0000, 0x2000, 0x3dcd9359 )
	ROM_LOAD( "d2",           0x2000, 0x2000, 0x710058d8 )
	ROM_LOAD( "d3",           0x4000, 0x2000, 0x467d12d8 )
	ROM_LOAD( "d4",           0x6000, 0x2000, 0xfce9afeb )

	ROM_REGION_DISPOSE(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d9",           0x0000, 0x1000, 0xde4cfe66 )
	ROM_LOAD( "d10",          0x1000, 0x1000, 0xa6c2f38b )
	ROM_LOAD( "r8-08.bin",    0x2000, 0x1000, 0xdbdc9ffa )
	ROM_LOAD( "n8-07.bin",    0x3000, 0x1000, 0x4b9973db )
	ROM_LOAD( "h5-05.bin",    0x4000, 0x1000, 0xe1218cc5 )
	ROM_LOAD( "k5-06.bin",    0x5000, 0x1000, 0xb1f68b04 )

	ROM_REGION(0x0060)	/* color PROMs */
	ROM_LOAD( "u02--2.bin",   0x0000, 0x0020, 0x238a65d7 )	/* palette (high bits) */
	ROM_LOAD( "t02--3.bin",   0x0020, 0x0020, 0xae263dc0 )	/* palette (low bits) */
	ROM_LOAD( "f10--1.bin",   0x0040, 0x0020, 0x16ee4ca2 )	/* sprite color lookup table */
ROM_END

ROM_START( mrdofix_rom )
    ROM_REGION(0x10000) /* 64k for code */
    ROM_LOAD( "d1",           0x0000, 0x2000, 0x3dcd9359 )
    ROM_LOAD( "d2",           0x2000, 0x2000, 0x710058d8 )
    ROM_LOAD( "dofix.d3",     0x4000, 0x2000, 0x3a7d039b )
    ROM_LOAD( "dofix.d4",     0x6000, 0x2000, 0x32db845f )

    ROM_REGION_DISPOSE(0x6000)  /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "d9",           0x0000, 0x1000, 0xde4cfe66 )
    ROM_LOAD( "d10",          0x1000, 0x1000, 0xa6c2f38b )
    ROM_LOAD( "r8-08.bin",    0x2000, 0x1000, 0xdbdc9ffa )
    ROM_LOAD( "n8-07.bin",    0x3000, 0x1000, 0x4b9973db )
    ROM_LOAD( "h5-05.bin",    0x4000, 0x1000, 0xe1218cc5 )
    ROM_LOAD( "k5-06.bin",    0x5000, 0x1000, 0xb1f68b04 )

    ROM_REGION(0x0060)  /* color PROMs */
    ROM_LOAD( "u02--2.bin",   0x0000, 0x0020, 0x238a65d7 )  /* palette (high bits) */
    ROM_LOAD( "t02--3.bin",   0x0020, 0x0020, 0xae263dc0 )  /* palette (low bits) */
    ROM_LOAD( "f10--1.bin",   0x0040, 0x0020, 0x16ee4ca2 )  /* sprite color lookup table */
ROM_END

ROM_START( mrlo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "mrlo01.bin",   0x0000, 0x2000, 0x6f455e7d )
	ROM_LOAD( "d2",           0x2000, 0x2000, 0x710058d8 )
	ROM_LOAD( "dofix.d3",     0x4000, 0x2000, 0x3a7d039b )
	ROM_LOAD( "mrlo04.bin",   0x6000, 0x2000, 0x49c10274 )

	ROM_REGION_DISPOSE(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mrlo09.bin",   0x0000, 0x1000, 0xfdb60d0d )
	ROM_LOAD( "mrlo10.bin",   0x1000, 0x1000, 0x0492c10e )
	ROM_LOAD( "r8-08.bin",    0x2000, 0x1000, 0xdbdc9ffa )
	ROM_LOAD( "n8-07.bin",    0x3000, 0x1000, 0x4b9973db )
	ROM_LOAD( "h5-05.bin",    0x4000, 0x1000, 0xe1218cc5 )
	ROM_LOAD( "k5-06.bin",    0x5000, 0x1000, 0xb1f68b04 )

	ROM_REGION(0x0060)	/* color PROMs */
	ROM_LOAD( "u02--2.bin",   0x0000, 0x0020, 0x238a65d7 )	/* palette (high bits) */
	ROM_LOAD( "t02--3.bin",   0x0020, 0x0020, 0xae263dc0 )	/* palette (low bits) */
	ROM_LOAD( "f10--1.bin",   0x0040, 0x0020, 0x16ee4ca2 )	/* sprite color lookup table */
ROM_END

ROM_START( mrdu_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "d1",           0x0000, 0x2000, 0x3dcd9359 )
	ROM_LOAD( "d2",           0x2000, 0x2000, 0x710058d8 )
	ROM_LOAD( "d3",           0x4000, 0x2000, 0x467d12d8 )
	ROM_LOAD( "du4.bin",      0x6000, 0x2000, 0x893bc218 )

	ROM_REGION_DISPOSE(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "du9.bin",      0x0000, 0x1000, 0x4090dcdc )
	ROM_LOAD( "du10.bin",     0x1000, 0x1000, 0x1e63ab69 )
	ROM_LOAD( "r8-08.bin",    0x2000, 0x1000, 0xdbdc9ffa )
	ROM_LOAD( "n8-07.bin",    0x3000, 0x1000, 0x4b9973db )
	ROM_LOAD( "h5-05.bin",    0x4000, 0x1000, 0xe1218cc5 )
	ROM_LOAD( "k5-06.bin",    0x5000, 0x1000, 0xb1f68b04 )

	ROM_REGION(0x0060)	/* color PROMs */
	ROM_LOAD( "u02--2.bin",   0x0000, 0x0020, 0x238a65d7 )	/* palette (high bits) */
	ROM_LOAD( "t02--3.bin",   0x0020, 0x0020, 0xae263dc0 )	/* palette (low bits) */
	ROM_LOAD( "f10--1.bin",   0x0040, 0x0020, 0x16ee4ca2 )	/* sprite color lookup table */
ROM_END

ROM_START( mrdoy_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "dosnow.1",     0x0000, 0x2000, 0xd3454e2c )
	ROM_LOAD( "dosnow.2",     0x2000, 0x2000, 0x5120a6b2 )
	ROM_LOAD( "dosnow.3",     0x4000, 0x2000, 0x96416dbe )
	ROM_LOAD( "dosnow.4",     0x6000, 0x2000, 0xc05051b6 )

	ROM_REGION_DISPOSE(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "dosnow.9",     0x0000, 0x1000, 0x85d16217 )
	ROM_LOAD( "dosnow.10",    0x1000, 0x1000, 0x61a7f54b )
	ROM_LOAD( "dosnow.8",     0x2000, 0x1000, 0x2bd1239a )
	ROM_LOAD( "dosnow.7",     0x3000, 0x1000, 0xac8ffddf )
	ROM_LOAD( "dosnow.5",     0x4000, 0x1000, 0x7662d828 )
	ROM_LOAD( "dosnow.6",     0x5000, 0x1000, 0x413f88d1 )

	ROM_REGION(0x0060)	/* color PROMs */
	ROM_LOAD( "u02--2.bin",   0x0000, 0x0020, 0x238a65d7 )	/* palette (high bits) */
	ROM_LOAD( "t02--3.bin",   0x0020, 0x0020, 0xae263dc0 )	/* palette (low bits) */
	ROM_LOAD( "f10--1.bin",   0x0040, 0x0020, 0x16ee4ca2 )	/* sprite color lookup table */
ROM_END

ROM_START( yankeedo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "a4-01.bin",    0x0000, 0x2000, 0x03dcfba2 )
	ROM_LOAD( "yd_d2.c4",     0x2000, 0x2000, 0x7c9d7ce0 )
	ROM_LOAD( "e4-03.bin",    0x4000, 0x2000, 0x358f5dc2 )
	ROM_LOAD( "f4-04.bin",    0x6000, 0x2000, 0xf4190cfc )

	ROM_REGION_DISPOSE(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "s8-09.bin",    0x0000, 0x1000, 0xaa80c5b6 )
	ROM_LOAD( "u8-10.bin",    0x1000, 0x1000, 0xd20ec85b )
	ROM_LOAD( "r8-08.bin",    0x2000, 0x1000, 0xdbdc9ffa )
	ROM_LOAD( "n8-07.bin",    0x3000, 0x1000, 0x4b9973db )
	ROM_LOAD( "yd_d5.h5",     0x4000, 0x1000, 0xf530b79b )
	ROM_LOAD( "yd_d6.k5",     0x5000, 0x1000, 0x790579aa )

	ROM_REGION(0x0060)	/* color PROMs */
	ROM_LOAD( "u02--2.bin",   0x0000, 0x0020, 0x238a65d7 )	/* palette (high bits) */
	ROM_LOAD( "t02--3.bin",   0x0020, 0x0020, 0xae263dc0 )	/* palette (low bits) */
	ROM_LOAD( "f10--1.bin",   0x0040, 0x0020, 0x16ee4ca2 )	/* sprite color lookup table */
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xe017],"\x01\x00\x00",3) == 0 &&
			memcmp(&RAM[0xe071],"\x01\x00\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xe017],10*10);
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
		osd_fwrite(f,&RAM[0xe017],10*10);
		osd_fclose(f);
	}
}



struct GameDriver mrdo_driver =
{
	__FILE__,
	0,
	"mrdo",
	"Mr. Do! (Universal)",
	"1982",
	"Universal",
	"Nicola Salmoria (MAME driver)\nPaul Swan (color info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	mrdo_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver mrdot_driver =
{
	__FILE__,
	&mrdo_driver,
	"mrdot",
	"Mr. Do! (Taito)",
	"1982",
	"Universal (Taito license)",
	"Nicola Salmoria (MAME driver)\nPaul Swan (color info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	mrdot_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver mrdofix_driver =
{
    __FILE__,
    &mrdo_driver,
    "mrdofix",
    "Mr. Do! (bugfixed)",
    "1982",
    "Universal (Taito license)",
    "Nicola Salmoria (MAME driver)\nPaul Swan (color info)\nMarco Cassili",
    0,
    &machine_driver,
    0,

    mrdofix_rom,
    0, 0,
    0,
    0,  /* sound_prom */

    input_ports,

    PROM_MEMORY_REGION(2), 0, 0,
    ORIENTATION_ROTATE_270,

    hiload, hisave
};

struct GameDriver mrlo_driver =
{
	__FILE__,
	&mrdo_driver,
	"mrlo",
	"Mr. Lo!",
	"1982",
	"bootleg",
	"Nicola Salmoria (MAME driver)\nPaul Swan (color info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	mrlo_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver mrdu_driver =
{
	__FILE__,
	&mrdo_driver,
	"mrdu",
	"Mr. Du!",
	"1982",
	"bootleg",
	"Nicola Salmoria (MAME driver)\nPaul Swan (color info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	mrdu_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};


struct GameDriver mrdoy_driver =
{
	__FILE__,
	&mrdo_driver,
	"mrdoy",
	"Mr. Do! (Yukidaruma)",
	"1982",
	"bootleg",
	"Nicola Salmoria (MAME driver)\nPaul Swan (color info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	mrdoy_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver yankeedo_driver =
{
	__FILE__,
	&mrdo_driver,
	"yankeedo",
	"Yankee DO! (Two Bit Score)",
	"1982",
	"bootleg",
	"Nicola Salmoria (MAME driver)\nPaul Swan (color info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	yankeedo_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};
