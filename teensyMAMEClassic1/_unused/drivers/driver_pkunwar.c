/***************************************************************************

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


void nova2001_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void pkunwar_flipscreen_w(int offset,int data);
void pkunwar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0xa001, 0xa001, AY8910_read_port_0_r },
	{ 0xa003, 0xa003, AY8910_read_port_1_r },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x8800, 0x8bff, videoram_w, &videoram, &videoram_size },
	{ 0x8c00, 0x8fff, colorram_w, &colorram },
	{ 0xa000, 0xa000, &AY8910_control_port_0_w },
	{ 0xa001, 0xa001, &AY8910_write_port_0_w },
	{ 0xa002, 0xa002, &AY8910_control_port_1_w },
	{ 0xa003, 0xa003, &AY8910_write_port_1_w },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xe000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, pkunwar_flipscreen_w },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START	/* IN1 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x30, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 8192*8, 8192*8 + 4, 2*4,  3*4, 8192*8 + 8, 8192*8 + 12 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8 },
	16*8
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 8192*8, 8192*8 + 4, 2*4,  3*4, 8192*8 + 8, 8192*8 + 12,
			0*4+16*8, 1*4+16*8, 8192*8+16*8, 8192*8 + 4+16*8, 2*4+16*8,  3*4+16*8, 8192*8 + 8 + 16*8, 8192*8 + 12 + 16*8 },
	{ 0*8, 2*8, 4*8, 6*8, 8*8, 10*8, 12*8, 14*8,
			0*8+32*8, 2*8+32*8, 4*8+32*8, 6*8+32*8, 8*8+32*8, 10*8+32*8, 12*8+32*8, 14*8 + 32*8  },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   16*16, 16 },
	{ 1, 0x1000, &charlayout,   16*16, 16 },
	{ 1, 0x4000, &charlayout,   16*16, 16 },
	{ 1, 0x5000, &charlayout,   16*16, 16 },
	{ 1, 0x8000, &charlayout,   16*16, 16 },
	{ 1, 0x9000, &charlayout,   16*16, 16 },
	{ 1, 0xc000, &charlayout,   16*16, 16 },
	{ 1, 0xd000, &charlayout,   16*16, 16 },
	{ 1, 0x0000, &spritelayout,	    0, 16 },
	{ 1, 0x1000, &spritelayout,	    0, 16 },
	{ 1, 0x4000, &spritelayout,	    0, 16 },
	{ 1, 0x5000, &spritelayout,	    0, 16 },
	{ 1, 0x8000, &spritelayout,	    0, 16 },
	{ 1, 0x9000, &spritelayout,	    0, 16 },
	{ 1, 0xc000, &spritelayout,	    0, 16 },
	{ 1, 0xd000, &spritelayout,	    0, 16 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface = {
	2,		/* 2 chips */
	1250000,	/* 1.25 MHz? */
	{ 255, 255 },
	{ input_port_0_r, input_port_2_r },
	{ input_port_1_r, input_port_3_r },
	{ 0, 0 },
	{ 0, 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,
			0,
			readmem,writemem,0,writeport,
			interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 4*8, 28*8-1 },
	gfxdecodeinfo,
	32,32*16,
	nova2001_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	pkunwar_vh_screenrefresh,

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

ROM_START( pkunwar_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pkwar.01r",    0x0000, 0x4000, 0xce2d2c7b )
	ROM_LOAD( "pkwar.02r",    0x4000, 0x4000, 0xabc1f661 )
	ROM_LOAD( "pkwar.03r",    0xE000, 0x2000, 0x56faebea )

	ROM_REGION_DISPOSE(0x10000)	/* 64K for graphics */
	ROM_LOAD( "pkwar.01y",    0x0000, 0x4000, 0x428d3b92 )
	ROM_LOAD( "pkwar.02y",    0x4000, 0x4000, 0xce1da7bc )
	ROM_LOAD( "pkwar.03y",    0x8000, 0x4000, 0x63204400 )
	ROM_LOAD( "pkwar.04y",    0xC000, 0x4000, 0x061dfca8 )

	ROM_REGION(0x0020)	/* color PROMs */
	ROM_LOAD( "pkwar.col",    0x0000, 0x0020, 0xaf0fc5e2 )
ROM_END

ROM_START( pkunwarj_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pgunwar.6",    0x0000, 0x4000, 0x357f3ef3 )
	ROM_LOAD( "pgunwar.5",    0x4000, 0x4000, 0x0092e49e )
	ROM_LOAD( "pkwar.03r",    0xE000, 0x2000, 0x56faebea )

	ROM_REGION_DISPOSE(0x10000)	/* 64K for graphics */
	ROM_LOAD( "pkwar.01y",    0x0000, 0x4000, 0x428d3b92 )
	ROM_LOAD( "pkwar.02y",    0x4000, 0x4000, 0xce1da7bc )
	ROM_LOAD( "pgunwar.2",    0x8000, 0x4000, 0xa2a43443 )
	ROM_LOAD( "pkwar.04y",    0xC000, 0x4000, 0x061dfca8 )

	ROM_REGION(0x0020)	/* color PROMs */
	ROM_LOAD( "pkwar.col",    0x0000, 0x0020, 0xaf0fc5e2 )
ROM_END

static int pkunwar_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xc187],"\x00\x20\x00",3) == 0)

	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xc187],12*5);
			RAM[0xc1d8] = RAM[0xc187]; /* update the HS on screen writting it backward*/
			RAM[0xc1d7] = RAM[0xc188];
			RAM[0xc1d6] = RAM[0xc189];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;		/* can't load hi scores yet */
}

static void pkunwar_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xc187],12*5);
		osd_fclose(f);
	}
}



struct GameDriver pkunwar_driver =
{
	__FILE__,
	0,
	"pkunwar",
	"Penguin-Kun Wars (US)",
	"1985?",
	"UPL",
	"Allard van der Bas\nNicola Salmoria",
	0,
	&machine_driver,
	0,

	pkunwar_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	pkunwar_hiload, pkunwar_hisave
};

struct GameDriver pkunwarj_driver =
{
	__FILE__,
	&pkunwar_driver,
	"pkunwarj",
	"Penguin-Kun Wars (Japan)",
	"1985?",
	"UPL",
	"Allard van der Bas\nNicola Salmoria",
	0,
	&machine_driver,
	0,

	pkunwarj_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	pkunwar_hiload, pkunwar_hisave
};
