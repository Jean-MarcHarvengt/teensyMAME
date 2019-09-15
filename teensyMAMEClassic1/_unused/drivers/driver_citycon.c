/***************************************************************************


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *citycon_scroll;
extern unsigned char *citycon_charlookup;
void citycon_charlookup_w(int offset,int data);
void citycon_background_w(int offset,int data);

void citycon_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int  citycon_vh_start(void);
void citycon_vh_stop(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x3000, 0x3000, input_port_0_r },
	{ 0x3001, 0x3001, input_port_1_r },
	{ 0x3002, 0x3002, input_port_2_r },
	{ 0x3007, 0x3007, watchdog_reset_r },	/* ? */
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x1fff, videoram_w, &videoram, &videoram_size },
	{ 0x2000, 0x20ff, citycon_charlookup_w, &citycon_charlookup },
	{ 0x2800, 0x28ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3000, 0x3000, citycon_background_w },
	{ 0x3001, 0x3001, soundlatch_w },
	{ 0x3002, 0x3002, soundlatch2_w },
	{ 0x3004, 0x3005, MWA_RAM, &citycon_scroll },
	{ 0x3800, 0x3cff, paletteram_RRRRGGGGBBBBxxxx_swap_w, &paletteram },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress readmem_sound[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
//	{ 0x4002, 0x4002, YM2203_read_port_1_r },	/* ?? */
	{ 0x6001, 0x6001, YM2203_read_port_0_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_sound[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x4000, 0x4000, YM2203_control_port_1_w },
	{ 0x4001, 0x4001, YM2203_write_port_1_w },
	{ 0x6000, 0x6000, YM2203_control_port_0_w },
	{ 0x6001, 0x6001, YM2203_write_port_0_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x04, 0x00, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	/* the coin input must stay low for exactly 2 frames to be consistently recognized. */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE, "Coin A", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )

	PORT_START
	PORT_DIPNAME( 0x07, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x06, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 7", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Flip Screen?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3, 256*8*8+0, 256*8*8+1, 256*8*8+2, 256*8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	4,	/* 4 bits per pixel */
	{ 4, 0, 0xc000*8+4, 0xc000*8+0 },
	{ 0, 1, 2, 3, 256*8*8+0, 256*8*8+1, 256*8*8+2, 256*8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	8,16,	/* 8*16 sprites */
	128,	/* 128 sprites */
	4,	/* 4 bits per pixel */
	{ 4, 0, 0x2000*8+4, 0x2000*8+0 },
	{ 0, 1, 2, 3, 128*16*8+0, 128*16*8+1, 128*16*8+2, 128*16*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
            8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8	/* every sprite takes 16 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout, 512, 32 },	/* colors 512-639 */
	{ 1, 0x02000, &spritelayout, 0, 16 },	/* colors 0-255 */
	{ 1, 0x03000, &spritelayout, 0, 16 },
	{ 1, 0x06000, &tilelayout, 256, 16 },	/* colors 256-511 */
	{ 1, 0x07000, &tilelayout, 256, 16 },
	{ 1, 0x08000, &tilelayout, 256, 16 },
	{ 1, 0x09000, &tilelayout, 256, 16 },
	{ 1, 0x0a000, &tilelayout, 256, 16 },
	{ 1, 0x0b000, &tilelayout, 256, 16 },
	{ 1, 0x0c000, &tilelayout, 256, 16 },
	{ 1, 0x0d000, &tilelayout, 256, 16 },
	{ 1, 0x0e000, &tilelayout, 256, 16 },
	{ 1, 0x0f000, &tilelayout, 256, 16 },
	{ 1, 0x10000, &tilelayout, 256, 16 },
	{ 1, 0x11000, &tilelayout, 256, 16 },
	{ -1 } /* end of array */
};



static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	2000000,	/* 2.0 MHz ??? */
	{ YM2203_VOL(20,0x2014), YM2203_VOL(20,0x2014) },
	{ soundlatch_r },
	{ soundlatch2_r },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			2048000,        /* 2.048 Mhz ??? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			640000,        /* 0.640 Mhz ??? */
			3,
			readmem_sound,writemem_sound,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	640, 640,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	citycon_vh_start,
	citycon_vh_stop,
	citycon_vh_screenrefresh,

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

ROM_START( citycon_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "c10",          0x4000, 0x4000, 0xae88b53c )
	ROM_LOAD( "c11",          0x8000, 0x8000, 0x139eb1aa )

	ROM_REGION_DISPOSE(0x1e000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c4",           0x00000, 0x2000, 0xa6b32fc6 )	/* Characters */
	ROM_LOAD( "c12",          0x02000, 0x2000, 0x08eaaccd )	/* Sprites    */
	ROM_LOAD( "c13",          0x04000, 0x2000, 0x1819aafb )
	ROM_LOAD( "c9",           0x06000, 0x8000, 0x8aeb47e6 )	/* Background tiles */
	ROM_LOAD( "c8",           0x0e000, 0x4000, 0x0d7a1eeb )
	ROM_LOAD( "c6",           0x12000, 0x8000, 0x2246fe9d )
	ROM_LOAD( "c7",           0x1a000, 0x4000, 0xe8b97de9 )

	ROM_REGION(0xe000)
	ROM_LOAD( "c2",           0x0000, 0x8000, 0xf2da4f23 )	/* background maps */
	ROM_LOAD( "c3",           0x8000, 0x4000, 0x7ef3ac1b )
	ROM_LOAD( "c5",           0xc000, 0x2000, 0xc03d8b1b )	/* color codes for the background */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "c1",           0x8000, 0x8000, 0x1fad7589 )
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0900],"\x53\x48\x4f",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0900],24*10);
			osd_fread(f,&RAM[0x0043],3);
			osd_fread(f,&RAM[0x0055],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0900],24*10);
		osd_fwrite(f,&RAM[0x0043],3);
		osd_fwrite(f,&RAM[0x0055],3);
		osd_fclose(f);
	}
}



struct GameDriver citycon_driver =
{
	__FILE__,
	0,
	"citycon",
	"City Connection",
	"1985",
	"Jaleco",
	"Mirko Buffoni (MAME driver)\nNicola Salmoria (MAME driver)",
	0,
	&machine_driver,
	0,

	citycon_rom,
	0, 0,
	0,

	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
