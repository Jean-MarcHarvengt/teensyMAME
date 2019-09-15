/***************************************************************************

Space Panic memory map

0000-3FFF ROM
4000-5BFF Video RAM (Bitmap)
5C00-5FFF RAM
6000-601F Sprite Controller
          4 bytes per sprite

          byte 1 - 80 = ?
                   40 = Rotate sprite left/right
                   3F = Sprite Number (Conversion to my layout via table)

          byte 2 - X co-ordinate
          byte 3 - Y co-ordinate

          byte 4 - 08 = Switch sprite bank
                   07 = Colour

6800      IN1 - Player controls. See input port setup for mappings
6801      IN2 - Player 2 controls for cocktail mode. See input port setup for mappings.
6802      DSW - Dip switches
6803      IN0 - Coinage and player start
42FC-42FE Colour Map Selector
700C      Alternate colour mapping

(Not Implemented)

7000-700B Various triggers, Sound etc
700D-700F Various triggers
7800      80 = Flash Screen?

I/O ports:
read:

write:

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

void panic_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void panic_videoram_w(int offset,int data);
int panic_interrupt(void);

extern unsigned char *panic_videoram;

int  panic_vh_start(void);
void panic_vh_stop(void);
void panic_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static struct MemoryReadAddress readmem[] =
{
	{ 0x4000, 0x5FFF, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x6803, 0x6803, input_port_0_r }, /* IN0 */
	{ 0x6800, 0x6800, input_port_1_r }, /* IN1 */
	{ 0x6801, 0x6801, input_port_2_r }, /* IN2 */
	{ 0x6802, 0x6802, input_port_3_r }, /* DSW */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x5C00, 0x5FFF, MWA_RAM },
	{ 0x4000, 0x5BFF, panic_videoram_w, &panic_videoram },
	{ 0x6000, 0x601F, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START      /* DSW */
	PORT_DIPNAME( 0x07, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x05, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/5 Credits" )
/* 0x06 and 0x07 disabled */
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_DIPNAME( 0x10, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x10, "5000" )
	PORT_DIPNAME( 0x20, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0xc0, 0x40, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/3 Credits" )
INPUT_PORTS_END



/* Main Sprites */

static struct GfxLayout spritelayout0 =
{
	16,16,	/* 16*16 sprites */
	48 ,	/* 64 sprites */
	2,	    /* 2 bits per pixel */
	{ 4096*8, 0 },	/* the two bitplanes are separated */
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8, 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
  	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0, 7, 6, 5, 4, 3, 2, 1, 0 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

/* Man Top */

static struct GfxLayout spritelayout1 =
{
	16,16,	/* 16*16 sprites */
	16 ,	/* 16 sprites */
	2,	    /* 2 bits per pixel */
	{ 4096*8, 0 },	/* the two bitplanes are separated */
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8, 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0, 7, 6, 5, 4, 3, 2, 1, 0,  },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0A00, &spritelayout0, 0, 8 },	/* Monsters             */
	{ 1, 0x0200, &spritelayout0, 0, 8 },	/* Monsters eating Man  */
	{ 1, 0x0800, &spritelayout1, 0, 8 },	/* Man                  */
	{ -1 } /* end of array */
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2000000,	/* 2 Mhz? */
			0,
			readmem,writemem,0,0,
			panic_interrupt,2
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
  	32*8, 32*8, { 6*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	16, 8*4,
	panic_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	panic_vh_start,
	panic_vh_stop,
	panic_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

static int panic_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* wait for default to be copied */
	if (RAM[0x40c1] == 0x00 && RAM[0x40c2] == 0x03 && RAM[0x40c3] == 0x04)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
        	RAM[0x4004] = 0x01;	/* Prevent program resetting high score */

			osd_fread(f,&RAM[0x40C1],5);
                osd_fread(f,&RAM[0x5C00],12);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void panic_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x40C1],5);
        osd_fwrite(f,&RAM[0x5C00],12);
		osd_fclose(f);
	}
}

ROM_START( panic_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "spcpanic.1",   0x0000, 0x0800, 0x405ae6f9 )         /* Code */
	ROM_LOAD( "spcpanic.2",   0x0800, 0x0800, 0xb6a286c5 )
	ROM_LOAD( "spcpanic.3",   0x1000, 0x0800, 0x85ae8b2e )
	ROM_LOAD( "spcpanic.4",   0x1800, 0x0800, 0xb6d4f52f )
	ROM_LOAD( "spcpanic.5",   0x2000, 0x0800, 0x5b80f277 )
	ROM_LOAD( "spcpanic.6",   0x2800, 0x0800, 0xb73babf0 )
	ROM_LOAD( "spcpanic.7",   0x3000, 0x0800, 0xfc27f4e5 )
	ROM_LOAD( "spcpanic.8",   0x3800, 0x0800, 0x7da0b321 )         /* Colour Map */

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "spcpanic.9",   0x0000, 0x0800, 0xeec78b4c )
	ROM_LOAD( "spcpanic.10",  0x0800, 0x0800, 0xc9631c2d )
	ROM_LOAD( "spcpanic.12",  0x1000, 0x0800, 0xe83423d0 )
	ROM_LOAD( "spcpanic.11",  0x1800, 0x0800, 0xacea9df4 )

	ROM_REGION(0x0080)	/* color PROM */
	ROM_LOAD( "82S123.SP",    0x0000, 0x0020, 0x35d43d2f )
ROM_END

ROM_START( panica_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "panica.1",     0x0000, 0x0800, 0x289720ce )         /* Code */
	ROM_LOAD( "spcpanic.2",   0x0800, 0x0800, 0xb6a286c5 )
	ROM_LOAD( "spcpanic.3",   0x1000, 0x0800, 0x85ae8b2e )
	ROM_LOAD( "spcpanic.4",   0x1800, 0x0800, 0xb6d4f52f )
	ROM_LOAD( "spcpanic.5",   0x2000, 0x0800, 0x5b80f277 )
	ROM_LOAD( "spcpanic.6",   0x2800, 0x0800, 0xb73babf0 )
	ROM_LOAD( "panica.7",     0x3000, 0x0800, 0x3641cb7f )
	ROM_LOAD( "spcpanic.8",   0x3800, 0x0800, 0x7da0b321 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "spcpanic.9",   0x0000, 0x0800, 0xeec78b4c )
	ROM_LOAD( "spcpanic.10",  0x0800, 0x0800, 0xc9631c2d )
	ROM_LOAD( "spcpanic.12",  0x1000, 0x0800, 0xe83423d0 )
	ROM_LOAD( "spcpanic.11",  0x1800, 0x0800, 0xacea9df4 )

	ROM_REGION(0x0080)	/* color PROM */
	ROM_LOAD( "82S123.SP",    0x0000, 0x0020, 0x35d43d2f )
ROM_END

ROM_START( panicger_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "spacepan.001", 0x0000, 0x0800, 0xa6d9515a )         /* Code */
	ROM_LOAD( "spacepan.002", 0x0800, 0x0800, 0xcfc22663 )
	ROM_LOAD( "spacepan.003", 0x1000, 0x0800, 0xe1f36893 )
	ROM_LOAD( "spacepan.004", 0x1800, 0x0800, 0x01be297c )
	ROM_LOAD( "spacepan.005", 0x2000, 0x0800, 0xe0d54805 )
	ROM_LOAD( "spacepan.006", 0x2800, 0x0800, 0xaae1458e )
	ROM_LOAD( "spacepan.007", 0x3000, 0x0800, 0x14e46e70 )
	ROM_LOAD( "spcpanic.8",   0x3800, 0x0800, 0x7da0b321 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "spcpanic.9",   0x0000, 0x0800, 0xeec78b4c )
	ROM_LOAD( "spcpanic.10",  0x0800, 0x0800, 0xc9631c2d )
	ROM_LOAD( "spcpanic.12",  0x1000, 0x0800, 0xe83423d0 )
	ROM_LOAD( "spcpanic.11",  0x1800, 0x0800, 0xacea9df4 )

	ROM_REGION(0x0020)	/* color PROMs */
	ROM_LOAD( "82S123.SP",    0x0000, 0x0020, 0x35d43d2f )
ROM_END


struct GameDriver panic_driver =
{
	__FILE__,
	0,
	"panic",
	"Space Panic (set 1)",
	"1980",
	"Universal",
	"Mike Coates (MAME driver)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	panic_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	panic_hiload, panic_hisave
};

struct GameDriver panica_driver =
{
	__FILE__,
	&panic_driver,
	"panica",
	"Space Panic (set 2)",
	"1980",
	"Universal",
	"Mike Coates (MAME driver)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	panica_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	panic_hiload, panic_hisave
};

struct GameDriver panicger_driver =
{
	__FILE__,
	&panic_driver,
	"panicger",
	"Space Panic (German)",
	"1980",
	"Universal (ADP Automaten license)",
	"Mike Coates (MAME driver)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	panicger_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	panic_hiload, panic_hisave
};
