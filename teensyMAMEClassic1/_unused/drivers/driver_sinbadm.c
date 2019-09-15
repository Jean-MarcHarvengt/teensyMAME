#include "driver.h"
#include "vidhrdw/generic.h"

/* in machine/segacrpt.c */
void sinbadm_decode(void);


void blueprnt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc800, 0xcfff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
{ 0xc800, 0xcbff, videoram_w, &videoram, &videoram_size },
{ 0xc800, 0xcbff, colorram_w, &colorram },
	{ 0xc800, 0xcfff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8 by 8 */
	1024,	/* 1024 characters */
	2,		/* 2 bits per pixel */
	{ 0, 1024*8*8 },			/* plane */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};
static struct GfxLayout charlayout1 =
{
	8,8,	/* 8 by 8 */
	1024,	/* 1024 characters */
	1,		/* 1 bits per pixel */
	{ 0 },			/* plane */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout, 0, 64 },
	{ 1, 0x4000, &charlayout1, 0, 64 },
	{ 1, 0x6000, &charlayout1, 0, 64 },
	{ 1, 0x8000, &charlayout1, 0, 64 },
	{ 1, 0xa000, &charlayout1, 0, 64 },
	{ -1 } /* end of array */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz ? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3072000,	/* 3.072 Mhz ? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	blueprnt_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0
};



ROM_START( sinbadm_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "epr5393.new",  0x0000, 0x2000, 0x0 )
	ROM_LOAD( "epr5394.new",  0x2000, 0x2000, 0x0 )
	ROM_LOAD( "epr5395.new",  0x4000, 0x2000, 0x0 )
	ROM_LOAD( "epr5396.new",  0x6000, 0x2000, 0x0 )
	ROM_LOAD( "epr5397.new",  0x8000, 0x2000, 0x0 )	/* ?? contains gfx */
	ROM_LOAD( "epr5398.new",  0xa000, 0x2000, 0x0 )	/* ?? contains gfx */

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr5428.new",  0x0000, 0x2000, 0x0 )
	ROM_LOAD( "epr5429.new",  0x2000, 0x2000, 0x0 )
	ROM_LOAD( "epr5424.new",  0x4000, 0x2000, 0x0 )
	ROM_LOAD( "epr5425.new",  0x6000, 0x2000, 0x0 )
	ROM_LOAD( "epr5426.new",  0x8000, 0x2000, 0x0 )
	ROM_LOAD( "epr5427.new",  0xa000, 0x2000, 0x0 )

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "epr5400.new",  0x0000, 0x2000, 0x0 )
ROM_END



struct GameDriver sinbadm_driver =
{
	__FILE__,
	0,
	"sinbadm",
	"Sinbad Mystery",
	"????",
	"?????",
	"Nicola Salmoria",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	sinbadm_rom,
	0, sinbadm_decode,
	0,
	0,

	0,//input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};
