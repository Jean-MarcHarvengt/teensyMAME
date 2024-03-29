/***************************************************************************

Exed Exes memory map (preliminary) - RD 29/07/97

MAIN CPU
0000-bfff ROM
d000-d3ff Character Video RAM
d400-d7ff Character Color RAM
e000-efff RAM
f000-ffff Sprites
        Sprite information (4 bytes long) is stored every 32 bytes in the
        regions f000-f5ff and f800-fbff, possibly in other areas as well.

        Sprite Data layout:
                Byte 0 - Sprite number 0-255
                Byte 1 - bits 0 to 3 Sprite color 0-15
                       - bit 4 Sprite x-flip
                       - bit 5 Sprite y-flip
                       - bit 6 ?
                       - bit 7 Sprite Y position MSB
                Byte 2 - Sprite X position
                Byte 3 - Sprite Y position

        The background consists of two vertically scrolling layers, one made
        up of 16x16 tiles similarly to 1942, the other made up of smaller
        8x8 or perhaps 8x16 tiles. The 16x16 tiled foreground is dislayed on
        top of the 8x? tiled background in a similar way to Star Force.

read:
c000      IN0 Coin and start switches
c001      IN1 Joystick 1
c002      IN2 Joystick 2
c003      DSW1 ?
c004      DSW2 ?

write:
c800 Sound command
c804 Coin ack ?
c806 ?
d800-d83f only seems to use:
	d800-d801 near background y-scroll
	d802-d803 near background x-scroll
        d804-d805 8x? tile vertical scroll
        d806-d807 ?

SOUND CPU
0000-3fff ROM
4000-47ff RAM

write:
8000      YM2203 #1 control
8001      YM2203 #1 write
8002      YM2203 #2 control ?
8003      YM2203 #2 write ?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



void c1942_bankswitch_w(int offset,int data);
int c1942_bankedrom_r(int offset);
int c1942_interrupt(void);

extern unsigned char *exedexes_bg_scroll;
extern unsigned char *exedexes_nbg_yscroll;
extern unsigned char *exedexes_nbg_xscroll;
void exedexes_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void exedexes_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc002, 0xc002, input_port_2_r },
	{ 0xc003, 0xc003, input_port_3_r },
	{ 0xc004, 0xc004, input_port_4_r },
	{ 0xe000, 0xefff, MRA_RAM }, /* Work RAM */
	{ 0xf000, 0xffff, MRA_RAM }, /* Sprite RAM */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xc806, 0xc806, MWA_NOP }, /* Watchdog ?? */
	{ 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
	{ 0xd400, 0xd7ff, colorram_w, &colorram },
	{ 0xd800, 0xd801, MWA_RAM, &exedexes_nbg_yscroll },
	{ 0xd802, 0xd803, MWA_RAM, &exedexes_nbg_xscroll },
	{ 0xd804, 0xd805, MWA_RAM, &exedexes_bg_scroll },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xffff, MWA_RAM, &spriteram, &spriteram_size },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8000, AY8910_control_port_0_w },
	{ 0x8001, 0x8001, AY8910_write_port_0_w },
	{ 0x8002, 0x8002, SN76496_0_w },
	{ 0x8003, 0x8003, SN76496_1_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, "Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 8 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x10, 0x10, "2 Players Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Credits" )
	PORT_DIPNAME( 0x20, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English")
	PORT_DIPSETTING(    0x20, "Japanese")
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
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/5 Credits" )
	PORT_DIPNAME( 0x38, 0x38, "Coin A", IP_KEY_NONE )
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
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
        256,    /* 256 sprites */
        4,      /* 4 bits per pixel */
        { 0x4000*8+4, 0x4000*8+0, 4, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	{ 33*8+3, 33*8+2, 33*8+1, 33*8+0, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
			8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	64*8	/* every sprite takes 64 consecutive bytes */
};
static struct GfxLayout tilelayout =
{
	32,32,  /* 32*32 tiles */
	64,    /* 64 tiles */
	2,      /* 2 bits per pixel */
	{ 4, 0 },
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
	{ 1, 0x0000, &charlayout,              0, 64 },
	{ 1, 0x2000, &tilelayout,           64*4, 64 }, /* 32x32 Tiles */
	{ 1, 0x6000, &spritelayout,       2*64*4, 16 }, /* 16x16 Tiles */
	{ 1, 0xe000, &spritelayout, 2*64*4+16*16, 16 }, /* Sprites */
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz ? */
	{ 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	3000000,	/* 3 MHz????? */
	{ 255, 255 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			readmem,writemem,0,0,
			c1942_interrupt,2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,	/* 3 Mhz ??? */
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
	256,64*4+64*4+16*16+16*16,
	exedexes_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	exedexes_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};



ROM_START( exedexes_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "11m_ee04.bin", 0x0000, 0x4000, 0x44140dbd )
	ROM_LOAD( "10m_ee03.bin", 0x4000, 0x4000, 0xbf72cfba )
	ROM_LOAD( "09m_ee02.bin", 0x8000, 0x4000, 0x7ad95e2f )

	ROM_REGION_DISPOSE(0x16000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "05c_ee00.bin", 0x00000, 0x2000, 0xcadb75bd ) /* Characters */
	ROM_LOAD( "h01_ee08.bin", 0x02000, 0x4000, 0x96a65c1d ) /* 32x32 tiles planes 0-1 */
	ROM_LOAD( "a03_ee06.bin", 0x06000, 0x4000, 0x6039bdd1 ) /* 16x16 tiles planes 0-1 */
	ROM_LOAD( "a02_ee05.bin", 0x0a000, 0x4000, 0xb32d8252 ) /* 16x16 tiles planes 2-3 */
	ROM_LOAD( "j11_ee10.bin", 0x0e000, 0x4000, 0xbc83e265 ) /* Sprites planes 0-1 */
	ROM_LOAD( "j12_ee11.bin", 0x12000, 0x4000, 0x0e0f300d ) /* Sprites planes 2-3 */

	ROM_REGION(0x0800)	/* color PROMs */
	ROM_LOAD( "02d_e-02.bin", 0x0000, 0x0100, 0x8d0d5935 )	/* red component */
	ROM_LOAD( "03d_e-03.bin", 0x0100, 0x0100, 0xd3c17efc )	/* green component */
	ROM_LOAD( "04d_e-04.bin", 0x0200, 0x0100, 0x58ba964c )	/* blue component */
	ROM_LOAD( "06f_e-05.bin", 0x0300, 0x0100, 0x35a03579 )	/* char lookup table */
	ROM_LOAD( "l04_e-10.bin", 0x0400, 0x0100, 0x1dfad87a )	/* 32x32 tile lookup table */
	ROM_LOAD( "c04_e-07.bin", 0x0500, 0x0100, 0x850064e0 )	/* 16x16 tile lookup table */
	ROM_LOAD( "l09_e-11.bin", 0x0600, 0x0100, 0x2bb68710 )	/* sprite lookup table */
	ROM_LOAD( "l10_e-12.bin", 0x0700, 0x0100, 0x173184ef )	/* sprite palette bank */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "11e_ee01.bin", 0x00000, 0x4000, 0x73cdf3b2 )

	ROM_REGION(0x6000)      /* For Tile background */
	ROM_LOAD( "c01_ee07.bin", 0x0000, 0x4000, 0x3625a68d )	/* Front Tile Map */
	ROM_LOAD( "h04_ee09.bin", 0x4000, 0x2000, 0x6057c907 )	/* Back Tile map */
ROM_END

ROM_START( savgbees_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ee04e.11m",    0x0000, 0x4000, 0xc0caf442 )
	ROM_LOAD( "ee03e.10m",    0x4000, 0x4000, 0x9cd70ae1 )
	ROM_LOAD( "ee02e.9m",     0x8000, 0x4000, 0xa04e6368 )

	ROM_REGION_DISPOSE(0x16000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ee00e.5c",     0x00000, 0x2000, 0x5972f95f ) /* Characters */
	ROM_LOAD( "h01_ee08.bin", 0x02000, 0x4000, 0x96a65c1d ) /* 32x32 tiles planes 0-1 */
	ROM_LOAD( "a03_ee06.bin", 0x06000, 0x4000, 0x6039bdd1 ) /* 16x16 tiles planes 0-1 */
	ROM_LOAD( "a02_ee05.bin", 0x0a000, 0x4000, 0xb32d8252 ) /* 16x16 tiles planes 2-3 */
	ROM_LOAD( "j11_ee10.bin", 0x0e000, 0x4000, 0xbc83e265 ) /* Sprites planes 0-1 */
	ROM_LOAD( "j12_ee11.bin", 0x12000, 0x4000, 0x0e0f300d ) /* Sprites planes 2-3 */

	ROM_REGION(0x0800)	/* color PROMs */
	ROM_LOAD( "02d_e-02.bin", 0x0000, 0x0100, 0x8d0d5935 )	/* red component */
	ROM_LOAD( "03d_e-03.bin", 0x0100, 0x0100, 0xd3c17efc )	/* green component */
	ROM_LOAD( "04d_e-04.bin", 0x0200, 0x0100, 0x58ba964c )	/* blue component */
	ROM_LOAD( "06f_e-05.bin", 0x0300, 0x0100, 0x35a03579 )	/* char lookup table */
	ROM_LOAD( "l04_e-10.bin", 0x0400, 0x0100, 0x1dfad87a )	/* 32x32 tile lookup table */
	ROM_LOAD( "c04_e-07.bin", 0x0500, 0x0100, 0x850064e0 )	/* 16x16 tile lookup table */
	ROM_LOAD( "l09_e-11.bin", 0x0600, 0x0100, 0x2bb68710 )	/* sprite lookup table */
	ROM_LOAD( "l10_e-12.bin", 0x0700, 0x0100, 0x173184ef )	/* sprite palette bank */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ee01e.11e",    0x00000, 0x4000, 0x93d3f952 )

	ROM_REGION(0x6000)      /* For Tile background */
	ROM_LOAD( "c01_ee07.bin", 0x0000, 0x4000, 0x3625a68d )	/* Front Tile Map */
	ROM_LOAD( "h04_ee09.bin", 0x4000, 0x2000, 0x6057c907 )	/* Back Tile map */
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0xE680],"\x00\x00\x00\x00\x00\x05",6) == 0) &&
			(memcmp(&RAM[0xE6C0],"\x00\x00\x00\x00\x00\x01",6) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xE680],0x50);
			/* fix the score at the top */
			memcpy(&RAM[0xE600],&RAM[0xE680],8);
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
		osd_fwrite(f,&RAM[0xE680],0x50);
		osd_fclose(f);
	}

}



struct GameDriver exedexes_driver =
{
	__FILE__,
	0,
	"exedexes",
	"Exed Exes",
	"1985",
	"Capcom",
	"Richard Davies\nPaul Leaman\nNicola Salmoria\nMirko Buffoni\nPaul Swan (color info)",
	0,
	&machine_driver,
	0,

	exedexes_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver savgbees_driver =
{
	__FILE__,
	&exedexes_driver,
	"savgbees",
	"Savage Bees",
	"1985",
	"Capcom (Memetron license)",
	"Richard Davies\nPaul Leaman\nNicola Salmoria\nMirko Buffoni\nPaul Swan (color info)",
	0,
	&machine_driver,
	0,

	savgbees_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
