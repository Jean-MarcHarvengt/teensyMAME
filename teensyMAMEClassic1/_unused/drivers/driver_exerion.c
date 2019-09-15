/***************************************************************************

Exerion by Jaleco

Exerion is a unique driver in that it has idiosyncracies that are straight
out of Bizarro World. I submit for your approval:

* The mystery reads from $d802 - timer-based protection?
* The freakish graphics encoding scheme, which no other MAME-supported game uses
* The sprite-ram, and all the funky parameters that go along with it
* The unusual parallaxed background. Is it controlled by the 2nd CPU?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

void exerion_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int exerion_vh_start (void);
void exerion_vh_stop (void);
void exerion_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern void exerion_videoreg_w (int offset,int data);

/* This is the first of many Exerion "features." No clue if it's */
/* protection or some sort of timer. */
int exerion_porta_r (int offset)
{
	static unsigned char val = 0xbe;

	val ^= 0x40;
	return val;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x67ff, MRA_RAM },
	{ 0x8000, 0x8bff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa800, 0xa800, input_port_1_r },
	{ 0xb000, 0xb000, input_port_2_r },
	{ 0xd802, 0xd802, AY8910_read_port_1_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x67ff, MWA_RAM },
	{ 0x8000, 0x87ff, videoram_w, &videoram, &videoram_size },
	{ 0x8800, 0x887f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x8800, 0x8bff, MWA_RAM },
	{ 0xc000, 0xc000, exerion_videoreg_w },
	{ 0xc800, 0xc800, soundlatch_w },
	{ 0xd000, 0xd000, AY8910_control_port_0_w },
	{ 0xd001, 0xd001, AY8910_write_port_0_w },
	{ 0xd800, 0xd800, AY8910_control_port_1_w },
	{ 0xd801, 0xd801, AY8910_write_port_1_w },
	{ -1 }  /* end of table */
};

#ifdef TRY_CPU2
static struct MemoryReadAddress cpu2_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
//	{ 0xa000, 0xa000, MRA_NOP }, /* ??? */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress cpu2_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
//	{ 0x8001, 0x8001, MWA_RAM },
//	{ 0x8003, 0x8003, MWA_RAM },
//	{ 0x8005, 0x8005, MWA_RAM },
//	{ 0x8007, 0x8007, MWA_RAM },
	{ -1 }  /* end of table */
};
#endif


/* Exerion triggers NMIs on vblank */
int exerion_interrupt(void)
{
	if (readinputport(3) & 1)
		return nmi_interrupt();
	else return ignore_interrupt();
}

INPUT_PORTS_START( exerion_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x02, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPSETTING(    0x07, "Infinite" )
	PORT_DIPNAME( 0x18, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x08, "20000" )
	PORT_DIPSETTING(    0x10, "30000" )
	PORT_DIPSETTING(    0x18, "40000" )
	PORT_DIPNAME( 0x20, 0x20, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x40, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* VBLANK */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* FAKE */
	/* The coin slots are not memory mapped. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IPF_IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,            /* 8*8 characters */
	512,          /* total number of chars */
	2,              /* 2 bits per pixel (# of planes) */
	{ 0, 4 },       /* start of every bitplane */
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0 },
	{ 16*0, 16*1, 16*2, 16*3, 16*4, 16*5, 16*6, 16*7 },
	16*8            /* every char takes 16 consecutive bytes */
};

static struct GfxLayout bgcharlayout =
{
	32,32,            /* 64*8 characters */
	128,          /* total number of chars */
	2,              /* 2 bits per pixel (# of planes) */
	{ 0, 4 },       /* start of every bitplane */
	{ 3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0,
			2*8+3, 2*8+2, 2*8+1, 2*8+0, 3*8+3, 3*8+2, 3*8+1, 3*8+0,
			4*8+3, 4*8+2, 4*8+1, 4*8+0, 5*8+3, 5*8+2, 5*8+1, 5*8+0,
			6*8+3, 6*8+2, 6*8+1, 6*8+0, 7*8+3, 7*8+2, 7*8+1, 7*8+0 },
	{ 64*0, 64*1, 64*2, 64*3, 64*4, 64*5, 64*6, 64*7,
			64*8, 64*9, 64*10, 64*11, 64*12, 64*13, 64*14, 64*15,
			64*16, 64*17, 64*18, 64*19, 64*20, 64*21, 64*22, 64*23,
			64*24, 64*25, 64*26, 64*27, 64*28, 64*29, 64*30, 64*31 },
	64*32            /* every char takes 16 consecutive bytes */
};

/* 16 x 16 sprites -- requires reorganizing characters in exerion_decode() */
static struct GfxLayout spritelayout =
{
	16,16,          /* 16*16 sprites */
	128*2,          /* total number of sprites in the rom */
	2,              /* 2 bits per pixel (# of planes) */
	{ 0, 4 },       /* start of every bitplane */
	{  3, 2, 1, 0, 8+3, 8+2, 8+1, 8+0,
			128+3, 128+2, 128+1, 128+0, 128+8+3, 128+8+2, 128+8+1, 128+8+0 },
	{ 16*0, 16*1, 16*2, 16*3, 16*4, 16*5, 16*6, 16*7,
			256+16*0, 256+16*1, 256+16*2, 256+16*3, 256+16*4, 256+16*5, 256+16*6, 256+16*7 },
	64*8            /* every sprite takes 64 consecutive bytes */
};

/* Quick and dirty way to emulate pixel-doubled sprites. */
static struct GfxLayout bigspritelayout =
{
	32,32,          /* 32*32 sprites */
	128*2,          /* total number of sprites in the rom */
	2,              /* 2 bits per pixel (# of planes) */
	{ 0, 4 },       /* start of every bitplane */
	{ 3, 3, 2, 2, 1, 1, 0, 0, 8+3, 8+3, 8+2, 8+2, 8+1, 8+1, 8+0, 8+0,
			128+3, 128+3, 128+2, 128+2, 128+1, 128+1, 128+0, 128+0,
			128+8+3, 128+8+3, 128+8+2, 128+8+2, 128+8+1, 128+8+1, 128+8+0, 128+8+0 },
	{ 16*0, 16*0, 16*1, 16*1, 16*2, 16*2, 16*3, 16*3,
			16*4, 16*4, 16*5, 16*5, 16*6, 16*6, 16*7, 16*7,
			256+16*0, 256+16*0, 256+16*1, 256+16*1, 256+16*2, 256+16*2, 256+16*3, 256+16*3,
			256+16*4, 256+16*4, 256+16*5, 256+16*5, 256+16*6, 256+16*6, 256+16*7, 256+16*7 },
	64*8            /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,           0, 64 },
	{ 1, 0x2000, &bgcharlayout,      64*4, 64 },
	{ 1, 0xa000, &spritelayout,    2*64*4, 64 },
	{ 1, 0xa000, &bigspritelayout, 2*64*4, 64 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	10000000/6,	/* 1.666 MHz */
	{ 0x20ff, 0x20ff },
	{ 0, exerion_porta_r },
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
			10000000/3,	/* 3.333 MHz */
			0,
			readmem,writemem,0,0,
			exerion_interrupt,1
		},
#ifdef TRY_CPU2
		{
			CPU_Z80,
			10000000/3,	/* 3.333 MHz */
			3,      /* memory region #3 */
			cpu2_readmem,cpu2_writemem,0,0,
			ignore_interrupt,0
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 12*8, 52*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32,64*4+64*4+64*4,
	exerion_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	exerion_vh_start,
	exerion_vh_stop,
	exerion_vh_screenrefresh,

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

ROM_START( exerion_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "exerion.07",   0x0000, 0x2000, 0x4c78d57d )
	ROM_LOAD( "exerion.08",   0x2000, 0x2000, 0xdcadc1df )
	ROM_LOAD( "exerion.09",   0x4000, 0x2000, 0x34cc4d14 )

	ROM_REGION_DISPOSE(0x1c000)     /* temporary space for graphics (disposed after conversion) */
	/* Set aside 0xe000 for the unscrambled graphics */
	ROM_LOAD( "exerion.06",   0x0e000, 0x2000, 0x435a85a4 )	/* fg chars */
	ROM_LOAD( "exerion.03",   0x10000, 0x2000, 0x790595b8 )	/* bg chars */
	ROM_LOAD( "exerion.04",   0x12000, 0x2000, 0xd7abd0b9 )
	ROM_LOAD( "exerion.01",   0x14000, 0x2000, 0x5bb755cb )
	ROM_LOAD( "exerion.02",   0x16000, 0x2000, 0xa7ecbb70 )
	ROM_LOAD( "exerion.11",   0x18000, 0x2000, 0xf0633a09 )	/* sprites */
	ROM_LOAD( "exerion.10",   0x1a000, 0x2000, 0x80312de0 )

	ROM_REGION(0x420)       /* color/lookup proms */
	ROM_LOAD( "exerion.e1",   0x0000, 0x020, 0x2befcc20 ) /* palette */
	ROM_LOAD( "exerion.i8",   0x0020, 0x100, 0x31db0e08 ) /* fg char lookup table */
	ROM_LOAD( "exerion.h10",  0x0120, 0x100, 0xcdd23f3e ) /* sprite lookup table */
	ROM_LOAD( "exerion.i3",   0x0220, 0x100, 0xfe72ab79 ) /* bg char lookup table */
	ROM_LOAD( "exerion.k4",   0x0320, 0x100, 0xffc2ba43 ) /* bg char mixer */

	ROM_REGION(0x10000)     /* 64k for the second CPU */
	ROM_LOAD( "exerion.05",   0x0000, 0x2000, 0x32f6bff5 )
ROM_END

ROM_START( exeriont_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "prom5.4p",     0x0000, 0x4000, 0x58b4dc1b )
	ROM_LOAD( "prom6.4s",     0x4000, 0x2000, 0xfca18c2d )

	ROM_REGION_DISPOSE(0x1c000)     /* temporary space for graphics (disposed after conversion) */
	/* Set aside 0xe000 for the unscrambled graphics */
	ROM_LOAD( "exerion.06",   0x0e000, 0x2000, 0x435a85a4 )	/* fg chars */
	ROM_LOAD( "exerion.03",   0x10000, 0x2000, 0x790595b8 )	/* bg chars */
	ROM_LOAD( "exerion.04",   0x12000, 0x2000, 0xd7abd0b9 )
	ROM_LOAD( "exerion.01",   0x14000, 0x2000, 0x5bb755cb )
	ROM_LOAD( "exerion.02",   0x16000, 0x2000, 0xa7ecbb70 )
	ROM_LOAD( "exerion.11",   0x18000, 0x2000, 0xf0633a09 )	/* sprites */
	ROM_LOAD( "exerion.10",   0x1a000, 0x2000, 0x80312de0 )

	ROM_REGION(0x420)       /* color/lookup proms */
	ROM_LOAD( "exerion.e1",   0x0000, 0x020, 0x2befcc20 ) /* palette */
	ROM_LOAD( "exerion.i8",   0x0020, 0x100, 0x31db0e08 ) /* fg char lookup table */
	ROM_LOAD( "exerion.h10",  0x0120, 0x100, 0xcdd23f3e ) /* sprite lookup table */
	ROM_LOAD( "exerion.i3",   0x0220, 0x100, 0xfe72ab79 ) /* bg char lookup table */
	ROM_LOAD( "exerion.k4",   0x0320, 0x100, 0xffc2ba43 ) /* bg char mixer */

	ROM_REGION(0x10000)     /* 64k for the second CPU */
	ROM_LOAD( "exerion.05",   0x0000, 0x2000, 0x32f6bff5 )
ROM_END

ROM_START( exerionb_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "eb5.bin",      0x0000, 0x4000, 0xda175855 )
	ROM_LOAD( "eb6.bin",      0x4000, 0x2000, 0x0dbe2eff )

	ROM_REGION_DISPOSE(0x1c000)     /* temporary space for graphics (disposed after conversion) */
	/* Set aside 0xe000 for the unscrambled graphics */
	ROM_LOAD( "exerion.06",   0x0e000, 0x2000, 0x435a85a4 )	/* fg chars */
	ROM_LOAD( "exerion.03",   0x10000, 0x2000, 0x790595b8 )	/* bg chars */
	ROM_LOAD( "exerion.04",   0x12000, 0x2000, 0xd7abd0b9 )
	ROM_LOAD( "exerion.01",   0x14000, 0x2000, 0x5bb755cb )
	ROM_LOAD( "exerion.02",   0x16000, 0x2000, 0xa7ecbb70 )
	ROM_LOAD( "exerion.11",   0x18000, 0x2000, 0xf0633a09 )	/* sprites */
	ROM_LOAD( "exerion.10",   0x1a000, 0x2000, 0x80312de0 )

	ROM_REGION(0x420)       /* color/lookup proms */
	ROM_LOAD( "exerion.e1",   0x0000, 0x020, 0x2befcc20 ) /* palette */
	ROM_LOAD( "exerion.i8",   0x0020, 0x100, 0x31db0e08 ) /* fg char lookup table */
	ROM_LOAD( "exerion.h10",  0x0120, 0x100, 0xcdd23f3e ) /* sprite lookup table */
	ROM_LOAD( "exerion.i3",   0x0220, 0x100, 0xfe72ab79 ) /* bg char lookup table */
	ROM_LOAD( "exerion.k4",   0x0320, 0x100, 0xffc2ba43 ) /* bg char mixer */

	ROM_REGION(0x10000)     /* 64k for the second CPU */
	ROM_LOAD( "exerion.05",   0x0000, 0x2000, 0x32f6bff5 )
ROM_END



static void exerion_decode(void)
{
	int i, x, offs;
	unsigned char *RAM;
	unsigned char *src;

	/*
		The gfx ROMs are stored in a half-assed fashion that makes using them with
		MAME challenging. We shuffle the bytes around to make it easier.

		They have been loaded at 0xe000, we write them at 0x0000.
	*/

	RAM = Machine->memory_region[1];
	src = RAM + 0xe000;

	for(i = 0;i < 512*7;i++)
	{
		offs = (i/16)*2048 + (i%16)*16;

		for (x=0; x<8; x++)
		{
			int d, s;

			d = i*16 + x*2;
			s = (offs + 256*x) / 8;

			RAM[d] = src[s];
			RAM[d+1] = src[s+1];
		}
	}

	/* reorganize bg chars */
	for(i = 0;i < 0x8000;i++)
	{
		int s;

		s = (i & 0x6000) | ((i & 0x1fe0) >> 2) | ((i & 0x0018) << 8) | (i & 0x0007);

		RAM[0x2000+s] = src[0x2000+i];
	}

	/* now reorganize the characters that form the sprites, and the sprites */

	/* get a fresh copy to work with */
	memcpy (&RAM[0x18000], &RAM[0xa000], 0x4000);

	/* for each of the 256 sprites, make the 4 characters contiguous */
	for (i=0; i<(128*2); i++)
	{
		int d, s;

		/* this assumes that sprite 1 is offset by 32 characters (32*16 bytes) from sprite 0 */
		offs = 16 * ((i/32)*2 + (i%32)*32);     // byte offset of sprite

		/* this assumes that sprite 1 is right next to sprite 0 (informational only) */
		//offs = 16 * i * 2; // byte offset of sprite

		d = 0xa000 + i*64;
		s = 0x18000 + offs;

		memcpy (&RAM[d], &RAM[s], 32);  // first 32 bytes
		memcpy (&RAM[d+32], &RAM[s+256], 32); // next 32 bytes
	}
}

static void exerionb_decode(void)
{
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* the program ROMs have data lines D1 and D2 swapped. Decode them. */
	for (A = 0;A < 0x6000;A++)
		RAM[A] = (RAM[A] & 0xf9) | ((RAM[A] & 2) << 1) | ((RAM[A] & 4) >> 1);

	/* also convert the gfx as in Exerion */
	exerion_decode();
}



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	static int firsttime;


	/* check if the hi score table has already been initialized */
	/* the high score table is intialized to all 0, so first of all */
	/* we dirty it, then we wait for it to be cleared again */
	if (firsttime == 0)
	{
		memset(&RAM[0x6600],0xff,10);	/* high score */
		memset(&RAM[0x6700],0xff,40);
		firsttime = 1;
	}



	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x6600],"\x00\x00\x00",3) == 0 &&
		memcmp(&RAM[0x6700],"\x00\x00\x00",3) == 0 &&
		memcmp(&RAM[0x6725],"\x00\x00\x00",3) == 0)
	{
		void *f;
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int writing = 0;


			osd_fread(f,&RAM[0x6600],10);
			osd_fread(f,&RAM[0x6700],40);
			osd_fclose(f);

			/* this is a little gross, but necessary to get the high score on-screen */
			if (!writing) writing = (RAM[0x6600] >> 4);
			videoram_w (0x48d, writing ? (RAM[0x6600] >> 4) | 0x30 : ' ');
			if (!writing) writing = (RAM[0x6600] & 0x0f);
			videoram_w (0x44d, writing ? (RAM[0x6600] & 0x0f) | 0x30 : ' ');
			if (!writing) writing = (RAM[0x6601] >> 4);
			videoram_w (0x40d, writing ? (RAM[0x6601] >> 4) | 0x30 : ' ');
			if (!writing) writing = (RAM[0x6601] & 0x0f);
			videoram_w (0x3cd, writing ? (RAM[0x6601] & 0x0f) | 0x30 : ' ');
		}
		firsttime = 0;
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
		osd_fwrite(f,&RAM[0x6600],10);
		osd_fwrite(f,&RAM[0x6700],40);
		osd_fclose(f);
	}
}


struct GameDriver exerion_driver =
{
	__FILE__,
	0,
	"exerion",
	"Exerion",
	"1983",
	"Jaleco",
	"Brad Oliver\nJohn Butler\nGerald Vanderick (color info)",
	GAME_WRONG_COLORS,
	&machine_driver,
	0,

	exerion_rom,
	exerion_decode, 0,
	0,
	0,      /* sound_prom */

	exerion_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver exeriont_driver =
{
	__FILE__,
	&exerion_driver,
	"exeriont",
	"Exerion (Taito)",
	"1983",
	"Jaleco (Taito America license)",
	"Brad Oliver\nJohn Butler\nGerald Vanderick (color info)",
	GAME_WRONG_COLORS,
	&machine_driver,
	0,

	exeriont_rom,
	exerion_decode, 0,
	0,
	0,      /* sound_prom */

	exerion_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver exerionb_driver =
{
	__FILE__,
	&exerion_driver,
	"exerionb",
	"Exerion (bootleg)",
	"1983",
	"Jaleco",
	"Brad Oliver\nJohn Butler\nGerald Vanderick (color info)",
	GAME_WRONG_COLORS,
	&machine_driver,
	0,

	exerionb_rom,
	exerionb_decode, 0,
	0,
	0,      /* sound_prom */

	exerion_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};
