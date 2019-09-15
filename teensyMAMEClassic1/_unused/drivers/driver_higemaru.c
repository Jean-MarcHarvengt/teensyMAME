#include "driver.h"
#include "vidhrdw/generic.h"


void higemaru_c800_w(int offset,int data);
void higemaru_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void higemaru_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



int higemaru_interrupt(void)
{
	if (cpu_getiloops() == 0) return 0x00cf;	/* RST 08h */
	else return 0x00d7;	/* RST 10h */
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc002, 0xc002, input_port_2_r },
	{ 0xc003, 0xc003, input_port_3_r },
	{ 0xc004, 0xc004, input_port_4_r },
	{ 0xd000, 0xd7ff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc800, 0xc800, higemaru_c800_w },
	{ 0xc801, 0xc801, AY8910_control_port_0_w },
	{ 0xc802, 0xc802, AY8910_write_port_0_w },
	{ 0xc803, 0xc803, AY8910_control_port_1_w },
	{ 0xc804, 0xc804, AY8910_write_port_1_w },
	{ 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
	{ 0xd400, 0xd7ff, colorram_w, &colorram },
	{ 0xd880, 0xd9ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe000, 0xefff, MWA_RAM },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x04, 0x04, "Freeze", IP_KEY_NONE )	/* could be Tilt? */
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x18, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0xc0, 0xc0, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "1" )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x0e, 0x0e, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0e, "10000 50000 50000" )
	PORT_DIPSETTING(    0x0c, "10000 60000 60000" )
	PORT_DIPSETTING(    0x0a, "20000 60000 60000" )
	PORT_DIPSETTING(    0x08, "20000 70000 70000" )
	PORT_DIPSETTING(    0x06, "30000 70000 70000" )
	PORT_DIPSETTING(    0x04, "30000 80000 80000" )
	PORT_DIPSETTING(    0x02, "40000 100000 100000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x10, 0x10, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Demo Music", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	4,		/* 4 bits per pixel */
	{ 128*64*8+4, 128*64*8, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,       0, 32 },
	{ 1, 0x02000, &spritelayout,  32*4, 16 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	2000000,	/* 2 MHz ? Main xtal is 12MHz */
	{ 255, 255 },
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
			4000000,	/* 4 MHz ? Main xtal is 12MHz */
			0,
			readmem,writemem,0,0,
			higemaru_interrupt,2
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32, 32*4+16*16,
	higemaru_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	higemaru_vh_screenrefresh,

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

ROM_START( higemaru_rom )
	ROM_REGION(0x1c000)	/* 64k for code */
	ROM_LOAD( "hg4",          0x0000, 0x2000, 0xdc67a7f9 )
	ROM_LOAD( "hg5",          0x2000, 0x2000, 0xf65a4b68 )
	ROM_LOAD( "hg6",          0x4000, 0x2000, 0x5f5296aa )
	ROM_LOAD( "hg7",          0x6000, 0x2000, 0xdc5d455d )

	ROM_REGION_DISPOSE(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "hg3",          0x00000, 0x2000, 0xb37b88c8 )	/* characters */
	ROM_LOAD( "hg1",          0x02000, 0x2000, 0xef4c2f5d )	/* tiles */
	ROM_LOAD( "hg2",          0x04000, 0x2000, 0x9133f804 )

	ROM_REGION(0x0220)	/* color PROMs */
	ROM_LOAD( "hgb3",         0x0000, 0x0020, 0x629cebd8 )	/* palette */
	ROM_LOAD( "hgb5",         0x0020, 0x0100, 0xdbaa4443 )	/* char lookup table */
	ROM_LOAD( "hgb1",         0x0120, 0x0100, 0x07c607ce )	/* sprite lookup table */
ROM_END

static int higemaru_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
        if ((memcmp(&RAM[0xee00],"\x00\x20\x00",3) == 0) &&
            (memcmp(&RAM[0xee75],"\x00\x10\x00",3) == 0))
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0xee00],130);
			osd_fclose(f);

			/* copy the high score to the work RAM as well */
                        RAM[0xee97] = RAM[0xee00];
                        RAM[0xee98] = RAM[0xee01];
                        RAM[0xee99] = RAM[0xee02];

		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void higemaru_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0xee00],130);
		osd_fclose(f);
	}
}


struct GameDriver higemaru_driver =
{
	__FILE__,
	0,
	"higemaru",
	"HigeMaru",
	"1984",
	"Capcom",
	"Mirko Buffoni\nNicola Salmoria",
	0,
	&machine_driver,
	0,

	higemaru_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	higemaru_hiload, higemaru_hisave
};
