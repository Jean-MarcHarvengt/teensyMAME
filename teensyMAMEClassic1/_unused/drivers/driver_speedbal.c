/***************************************************************************

Speed Ball map

Z80 MAIN CPU
0000-7fff ROM 1 32k
8000-dbff ROM 2 data
dc00-dfff for both CPU&SOUND (Shared)
e000-e1ff Video RAM background tiles 16*16 (2 bytes: 1 for tile char + 1 for color)
e800-efff Video RAM characters 32*32 (2 bytes: 1 for char and 1 for color)
f000-feff RAM
ff00-ffff Sprites (64 total: four bytes to one)

in:
00	DSW ONE
10	DSW TWO
20	IN1  JOY 1 STATUS & COIN & START
30	IN2  JOY 2 STATUS & TILT

out:
40	????
50	????  maybe VSYNC ????

 ======================================================================

Z80 SOUND
0000-7fff ROM
dc00-dfff shared with MAIN CPU
f000-dfff RAM

out:
00	YM3812 control
01	YM3812 data
20	??

 ======================================================================

  Colors :   2 bits for foreground characters =  4 colors * 16 palettes
	     4 bits for background tiles      = 16 colors * 16 palettes
	     4 bits for sprites	       = 16 colors * 16 palettes

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80/Z80.h"


unsigned char *speedbal_foreground_videoram;
unsigned char *speedbal_background_videoram;
unsigned char *speedbal_sprites_dataram;

int speedbal_foreground_videoram_size;
int speedbal_background_videoram_size;
int speedbal_sprites_dataram_size;

int  speedbal_vh_start(void);
void speedbal_vh_stop(void);
void speedbal_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int  speedbal_foreground_videoram_r(int offset);
void speedbal_foreground_videoram_w(int offset,int data);
int  speedbal_background_videoram_r(int offset);
void speedbal_background_videoram_w(int offset,int data);


unsigned char *speedbal_sharedram;

int speedbal_sharedram_r(int offset)
{
//  if (offset==0x0) speedbal_sharedram[offset]+=1;
  return speedbal_sharedram[offset];
}


void speedbal_sharedram_w(int offset,int data)
{
    speedbal_sharedram[offset] = data;
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xdbff, MRA_ROM },
	{ 0xdc00, 0xdfff, speedbal_sharedram_r, &speedbal_sharedram },  // shared with SOUND
	{ 0xe000, 0xe1ff, speedbal_background_videoram_r },
	{ 0xe800, 0xefff, speedbal_foreground_videoram_r },
	{ 0xf000, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xdbff, MWA_ROM },
	{ 0xdc00, 0xdfff, speedbal_sharedram_w, &speedbal_sharedram },  // shared with SOUND
	{ 0xe000, 0xe1ff, speedbal_background_videoram_w, &speedbal_background_videoram, &speedbal_background_videoram_size },
	{ 0xe800, 0xefff, speedbal_foreground_videoram_w, &speedbal_foreground_videoram, &speedbal_foreground_videoram_size },
	{ 0xf000, 0xf5ff, paletteram_RRRRGGGGBBBBxxxx_swap_w, &paletteram },
	{ 0xf600, 0xfeff, MWA_RAM },
	{ 0xff00, 0xffff, MWA_RAM, &speedbal_sprites_dataram, &speedbal_sprites_dataram_size },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xdc00, 0xdfff, speedbal_sharedram_r }, // shared with MAIN CPU
	{ 0xf000, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xdc00, 0xdfff, speedbal_sharedram_w }, // shared with MAIN CPU
	{ 0xf000, 0xffff, MWA_RAM },
	{ -1 }  /* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x10, 0x10, input_port_1_r },
	{ 0x20, 0x20, input_port_2_r },
	{ 0x30, 0x30, input_port_3_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ -1 }  /* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x00, 0x00, YM3812_status_port_0_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM3812_control_port_0_w },
	{ 0x01, 0x01, YM3812_write_port_0_w },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x07, 0x07, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "70000 200000 1M" )
	PORT_DIPSETTING(    0x07, "70000 200000" )
	PORT_DIPSETTING(    0x03, "100000 300000 1M" )
	PORT_DIPSETTING(    0x04, "100000 300000" )
	PORT_DIPSETTING(    0x01, "200000 1M" )
	PORT_DIPSETTING(    0x05, "200000" )
/*	PORT_DIPSETTING(    0x02, "200000" ) */
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "Very Easy" )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x10, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "Very Easy" )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x40, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/5 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW , IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW , IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW , IPT_BUTTON4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW , IPT_BUTTON3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW , IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW , IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW , IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW , IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW , IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW , IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW , IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW , IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW , IPT_TILT    )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	4,      /* actually 2 bits per pixel - two of the planes are empty */
	{ 1024*16*8+4, 1024*16*8+0, 4, 0 },
	{ 8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },   /* characters are rotated 90 degrees */
	16*8	   /* every char takes 16 bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,  /* 16*16 tiles */
	1024,   /* 1024 tiles */
	4,      /* 4 bits per pixel */
	{ 0, 2, 4, 6 }, /* the bitplanes are packed in one nibble */
	{ 0*8+0, 0*8+1, 7*8+0, 7*8+1, 6*8+0, 6*8+1, 5*8+0, 5*8+1,
			4*8+0, 4*8+1, 3*8+0, 3*8+1, 2*8+0, 2*8+1, 1*8+0, 1*8+1 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8  /* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	512,    /* 512 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 2, 4, 6 }, /* the bitplanes are packed in one nibble */
	{ 7*8+1, 7*8+0, 6*8+1, 6*8+0, 5*8+1, 5*8+0, 4*8+1, 4*8+0,
			3*8+1, 3*8+0, 2*8+1, 2*8+0, 1*8+1, 1*8+0, 0*8+1, 0*8+0 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8  /* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,	 256, 16 },
	{ 1, 0x08000, &tilelayout,	 512, 16 },
	{ 1, 0x28000, &spritelayout,   0, 16 },
	{ -1 } /* end of array */
};



static struct YM3812interface ym3812_interface =
{
	1,		      /* 1 chip (no more supported) */
	3600000,	/* 3.600000 MHz ? (partially supported) */
	{ 255 }	 /* (not supported) */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 MHz ??? */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		},
		{
			CPU_Z80,
			2660000,	/* 2.66 MHz ???  Maybe yes */
			2,
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			interrupt,8
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	1,    /* 1 CPU slices per frame */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	768, 768,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	speedbal_vh_start,
	speedbal_vh_stop,
	speedbal_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( speedbal_rom )
	ROM_REGION(0x10000)     /* 64K for code: main */
	ROM_LOAD( "sb1.bin",  0x0000,  0x8000, 0x1c242e34 )
	ROM_LOAD( "sb3.bin",  0x8000,  0x8000, 0x7682326a )

	ROM_REGION_DISPOSE(0x38000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD("sb10.bin", 0x00000, 0x08000, 0x36dea4bf )    /* chars */
	ROM_LOAD( "sb9.bin", 0x08000, 0x08000, 0xb567e85e )    /* bg tiles */
	ROM_LOAD( "sb5.bin", 0x10000, 0x08000, 0xb0eae4ba )
	ROM_LOAD( "sb8.bin", 0x18000, 0x08000, 0xd2bfbdb6 )
	ROM_LOAD( "sb4.bin", 0x20000, 0x08000, 0x1d23a130 )
	ROM_LOAD( "sb6.bin", 0x28000, 0x08000, 0x0e2506eb )    /* sprites */
	ROM_LOAD( "sb7.bin", 0x30000, 0x08000, 0x9f1b33d1 )

	ROM_REGION(0x10000)     /* 64K for second CPU: sound */
	ROM_LOAD( "sb2.bin",  0x0000, 0x8000, 0xe6a6d9b7 )
ROM_END


static void speedbal_decode (void)
{
	int i;

	/* invert the graphics bits on the sprites */
	for (i = 0x28000; i < 0x38000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}


static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if  (memcmp(&RAM[0xF800],"\x20\x38\x76",3) == 0 &&
			memcmp(&RAM[0xF843],"\x56\x41\x50",3) == 0 )
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xF800],70);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xF800],70);
		osd_fclose(f);
	}
}


struct GameDriver speedbal_driver =
{
	__FILE__,
	0,
	"speedbal",
	"Speed Ball",
	"1987",
	"Tecfri",
	"Joseba Epalza",
	0,
	&machine_driver,
	0,

	speedbal_rom,
	speedbal_decode, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};
