/***************************************************************************

to do:
-Better sound emulation
-Speed!!!
 -The Z80 spends most of its time reading the timer, we should trap that.


Time Pilot 84 Memory Map (preliminary)

The schematics are available on the net.

There is 3 CPU for this game.
 Two 68A09E for the game.
 A Z80A for the sound

As I understand it, the second 6809 is for displaying
 the sprites. If we do not emulate him, all work well, except
 that the player cannot die.
 Address 57ff must read 0 to pass the RAM test if the second CPU
 is not emulated.


---- Master 6809 ------

Write
 2000-27ff MAFR Watch dog ?
 2800      COL0 a register that index the colors Proms
 3000      reset IRQ
 3001      OUT2  Coin Counter 2
 3002      OUT1  Coin Counter 1
 3003      MUT
 3004      HREV
 3005      VREV
 3006      -
 3007      GMED
 3800      SON   Sound on
 3A00      SDA   Sound data
 3C00      SHF0 SHF1 J2 J3 J4 J5 J6 J7  background Y position
 3E00      L0 - L7                      background X position

Read:
 2800      in0  Buttons 1
 2820      in1  Buttons 2
 2840      in2  Buttons 3
 2860      in3  Dip switches 1
 3000      in4  Dip switches 2
 3800      in5  Dip switches 3 (not used)

Read/Write
 4000-47ff Char ram, 2 pages
 4800-4fff Background character ram, 2 pages
 5000-57ff Ram (Common for the Master and Slave 6809)  0x5000-0x517f sprites data
 6000-ffff Rom (only from $8000 to $ffff is used in this game)


------ Slave 6809 --------
 0000-1fff SAFR Watch dog ?
 2000      seem to be the beam position (if always 0, no player collision is detected)
 4000      enable or reset IRQ
 6000-7fff DRA ?
 8000-87ff Ram (Common for the Master and Slave 6809)
 E000-ffff Rom


------ Sound CPU (Z80) -----
There are 3 or 4 76489AN chips driven by the Z80

0000-1fff Rom program (A6)
2000-3fff Rom Program (A4) (not used or missing?)
4000-43ff Ram
6000-7fff Sound data in
8000-9fff Timer
A000-Bfff
C000      Store Data that will go to one of the 76489AN
C001      76489 #1 trigger
C002      76489 #2 (optional) trigger
C003      76489 #3 trigger
C004      76489 #4 trigger

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


/* In Machine */
extern int tp84_beam_r(int offset);
extern int tp84_interrupt(void);
void tp84_catchloop_w(int offset,int data); /* JB 970829 */
int tp84_catchloop_r(int offset); /* JB 970829 */
void tp84_init_machine(void); /* JB 970829 */

/* In Vidhrdw */
extern unsigned char *tp84_sharedram;
int tp84_sharedram_r(int offset);
void tp84_sharedram_w(int offset,int data);

extern unsigned char *tp84_videoram2;
extern unsigned char *tp84_colorram2;
extern unsigned char *tp84_scrollx;
extern unsigned char *tp84_scrolly;
void tp84_videoram2_w(int offset,int data);
void tp84_colorram2_w(int offset,int data);
void tp84_col0_w(int offset,int data);
void tp84_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int tp84_vh_start(void);
void tp84_vh_stop(void);
void tp84_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



int tp84_sh_timer_r(int offset)
{
	int clock;

#define TIMER_RATE 256

	clock = cpu_gettotalcycles() / TIMER_RATE;

	return clock;
}

void tp84_sh_irqtrigger_w(int offset,int data)
{
	cpu_cause_interrupt(2,0xff);
}



/* CPU 1 read addresses */
static struct MemoryReadAddress readmem[] =
{
	{ 0x2800, 0x2800, input_port_0_r },
	{ 0x2820, 0x2820, input_port_1_r },
	{ 0x2840, 0x2840, input_port_2_r },
	{ 0x2860, 0x2860, input_port_3_r },
	{ 0x3000, 0x3000, input_port_4_r },
	{ 0x4000, 0x4fff, MRA_RAM },
	{ 0x523a, 0x523a, tp84_catchloop_r }, /* JB 970829 */
	{ 0x5000, 0x57ff, tp84_sharedram_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

/* CPU 1 write addresses */
static struct MemoryWriteAddress writemem[] =
{
	{ 0x2000, 0x2000, MWA_RAM }, /*Watch dog?*/
	{ 0x2800, 0x2800, tp84_col0_w },
	{ 0x3000, 0x3000, MWA_RAM },
	{ 0x3800, 0x3800, tp84_sh_irqtrigger_w },
	{ 0x3a00, 0x3a00, soundlatch_w },
	{ 0x3c00, 0x3c00, MWA_RAM, &tp84_scrollx }, /* Y scroll */
	{ 0x3e00, 0x3e00, MWA_RAM, &tp84_scrolly }, /* X scroll */
	{ 0x4000, 0x43ff, videoram_w, &videoram , &videoram_size},
	{ 0x4400, 0x47ff, tp84_videoram2_w, &tp84_videoram2 },
	{ 0x4800, 0x4bff, colorram_w, &colorram },
	{ 0x4c00, 0x4fff, tp84_colorram2_w, &tp84_colorram2 },
	{ 0x5000, 0x57ff, tp84_sharedram_w, &tp84_sharedram },   /* 0x5000-0x517f sprites definitions*/
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};


/* CPU 2 read addresses */
static struct MemoryReadAddress readmem_cpu2[] =
{
	{ 0x0000, 0x0000, MRA_RAM },
	{ 0x2000, 0x2000, tp84_beam_r }, /* beam position */
	{ 0x6000, 0x7fff, MRA_RAM },
	{ 0x8000, 0x87ff, tp84_sharedram_r },  /* shared RAM with the main CPU */
	{ 0xe000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

/* CPU 2 write addresses */
static struct MemoryWriteAddress writemem_cpu2[] =
{
	{ 0x0000, 0x0000, MWA_RAM }, /* Watch dog ?*/
	{ 0x4000, 0x4000, tp84_catchloop_w }, /* IRQ enable */ /* JB 970829 */
	{ 0x6000, 0x7fff, MWA_RAM },
	{ 0x8000, 0x87ff, tp84_sharedram_w },    /* shared RAM with the main CPU */
	{ 0xe000, 0xffff, MWA_ROM },              /* ROM code */
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0x8000, 0x8000, tp84_sh_timer_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0xc000, 0xc000, MWA_NOP },
	{ 0xc001, 0xc001, SN76496_0_w },
	{ 0xc003, 0xc003, SN76496_1_w },
	{ 0xc004, 0xc004, SN76496_2_w },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x01, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x50, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x10, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x70, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Invalid" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x02, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x18, 0x18, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "10000 50000" )
	PORT_DIPSETTING(    0x10, "20000 60000" )
	PORT_DIPSETTING(    0x08, "30000 70000" )
	PORT_DIPSETTING(    0x00, "40000 80000" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Medium" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{  0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },	/* bits are packed in groups of four */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel */
	{ 256*64*8+4, 256*64*8+0, 4 ,0 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,        0, 64*8 },
	{ 1, 0x4000, &spritelayout, 64*4*8, 16*8 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	3,	/* 3 chips */
	1789750,	/* 1.78975 Mhz ? */
	{ 75, 75, 75 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1500000,			/* ? Mhz 1.1 seem too slow */
			0,					/* memory region */
			readmem,			/* MemoryReadAddress */
			writemem,			/* MemoryWriteAddress */
			0,					/* IOReadPort */
			0,					/* IOWritePort */
			interrupt,			/* interrupt routine */
			1					/* interrupts per frame */
		},
		{
			CPU_M6809,
			1100000,			/* ? Mhz */
			3,	/* memory region #3 */
			readmem_cpu2,
			writemem_cpu2,
			0,
			0,
			tp84_interrupt,1	/* JB 970829 */ /*256*/
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318180/4,	/* ? */
			4,	/* memory region #4 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	100,	/* 100 CPU slices per frame - an high value to ensure proper */
			/* synchronization of the CPUs */
	tp84_init_machine,		/* init machine routine */ /* JB 970829 */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,				/* GfxDecodeInfo * */
	256,4096, /* see tp84_vh_convert_color_prom for explanation */
	tp84_vh_convert_color_prom,							/* convert color prom routine */

	VIDEO_TYPE_RASTER,
	0,							/* vh_init routine */
	tp84_vh_start,			/* vh_start routine */
	tp84_vh_stop,			/* vh_stop routine */
	tp84_vh_screenrefresh,	/* vh_update routine */

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

ROM_START( tp84_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "tp84_7j.bin",  0x8000, 0x2000, 0x605f61c7 )
	ROM_LOAD( "tp84_8j.bin",  0xa000, 0x2000, 0x4b4629a4 )
	ROM_LOAD( "tp84_9j.bin",  0xc000, 0x2000, 0xdbd5333b )
	ROM_LOAD( "tp84_10j.bin", 0xe000, 0x2000, 0xa45237c4 )

	ROM_REGION_DISPOSE(0xc000)	/* Temporary */
	ROM_LOAD( "tp84_2j.bin",  0x0000, 0x2000, 0x05c7508f ) /* chars */
	ROM_LOAD( "tp84_1j.bin",  0x2000, 0x2000, 0x498d90b7 )
	ROM_LOAD( "tp84_12a.bin", 0x4000, 0x2000, 0xcd682f30 ) /* sprites */
	ROM_LOAD( "tp84_13a.bin", 0x6000, 0x2000, 0x888d4bd6 )
	ROM_LOAD( "tp84_14a.bin", 0x8000, 0x2000, 0x9a220b39 )
	ROM_LOAD( "tp84_15a.bin", 0xa000, 0x2000, 0xfac98397 )

	ROM_REGION(0x0500)	/* color/loookup proms */
	ROM_LOAD( "tp84_2c.bin",  0x0000, 0x0100, 0xd737eaba ) /* palette red component */
	ROM_LOAD( "tp84_2d.bin",  0x0100, 0x0100, 0x2f6a9a2a ) /* palette green component */
	ROM_LOAD( "tp84_1e.bin",  0x0200, 0x0100, 0x2e21329b ) /* palette blue component */
	ROM_LOAD( "tp84_1f.bin",  0x0300, 0x0100, 0x61d2d398 ) /* char lookup table */
	ROM_LOAD( "tp84_16c.bin", 0x0400, 0x0100, 0x13c4e198 ) /* sprite lookup table */

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "tp84_10d.bin", 0xe000, 0x2000, 0x36462ff1 )

	ROM_REGION(0x10000)	/* 64k for code of sound cpu Z80 */
	ROM_LOAD( "tp84s_6a.bin", 0x0000, 0x2000, 0xc44414da )
ROM_END

ROM_START( tp84a_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "tp84_7j.bin",  0x8000, 0x2000, 0x605f61c7 )
	ROM_LOAD( "f05",          0xa000, 0x2000, 0xe97d5093 )
	ROM_LOAD( "tp84_9j.bin",  0xc000, 0x2000, 0xdbd5333b )
	ROM_LOAD( "f07",          0xe000, 0x2000, 0x8fbdb4ef )

	ROM_REGION_DISPOSE(0xc000)	/* Temporary */
	ROM_LOAD( "tp84_2j.bin",  0x0000, 0x2000, 0x05c7508f ) /* chars */
	ROM_LOAD( "tp84_1j.bin",  0x2000, 0x2000, 0x498d90b7 )
	ROM_LOAD( "tp84_12a.bin", 0x4000, 0x2000, 0xcd682f30 ) /* sprites */
	ROM_LOAD( "tp84_13a.bin", 0x6000, 0x2000, 0x888d4bd6 )
	ROM_LOAD( "tp84_14a.bin", 0x8000, 0x2000, 0x9a220b39 )
	ROM_LOAD( "tp84_15a.bin", 0xa000, 0x2000, 0xfac98397 )

	ROM_REGION(0x0500)	/* color/loookup proms */
	ROM_LOAD( "tp84_2c.bin",  0x0000, 0x0100, 0xd737eaba ) /* palette red component */
	ROM_LOAD( "tp84_2d.bin",  0x0100, 0x0100, 0x2f6a9a2a ) /* palette green component */
	ROM_LOAD( "tp84_1e.bin",  0x0200, 0x0100, 0x2e21329b ) /* palette blue component */
	ROM_LOAD( "tp84_1f.bin",  0x0300, 0x0100, 0x61d2d398 ) /* char lookup table */
	ROM_LOAD( "tp84_16c.bin", 0x0400, 0x0100, 0x13c4e198 ) /* sprite lookup table */

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "tp84_10d.bin", 0xe000, 0x2000, 0x36462ff1 )

	ROM_REGION(0x10000)	/* 64k for code of sound cpu Z80 */
	ROM_LOAD( "tp84s_6a.bin", 0x0000, 0x2000, 0xc44414da )
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[0];
	void *f;

	/* Wait for hiscore table initialization to be done. */
	if (memcmp(&RAM[0x57a0], "\x00\x02\x00\x47\x53\x58", 6) != 0)
		return 0;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		/* Load and set hiscore table. */
		osd_fread(f,&RAM[0x57a0],5*6);
		RAM[0x5737] = RAM[0x57a1];
		RAM[0x5738] = RAM[0x57a2];
		osd_fclose(f);
	}

	return 1;
}



static void hisave(void)
{
	unsigned char *RAM = Machine->memory_region[0];
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* Write hiscore table. */
		osd_fwrite(f,&RAM[0x57a0],5*6);
		osd_fclose(f);
	}
}



struct GameDriver tp84_driver =
{
	__FILE__,
	0,
	"tp84",
	"Time Pilot 84 (set 1)",
	"1984",
	"Konami",
	"Marc Lafontaine (MAME driver)\nMarco Cassili",
	0,
	&machine_driver,	/* MachineDriver */
	0,

	tp84_rom,			/* RomModule */
	0, 0,				/* ROM decrypt routines */
	0,					/* samplenames */
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2),	/* color prom */
	0, 	          		/* palette */
	0, 	          		/* color table */
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver tp84a_driver =
{
	__FILE__,
	&tp84_driver,
	"tp84a",
	"Time Pilot 84 (set 2)",
	"1984",
	"Konami",
	"Marc Lafontaine (MAME driver)\nMarco Cassili",
	0,
	&machine_driver,	/* MachineDriver */
	0,

	tp84a_rom,			/* RomModule */
	0, 0,				/* ROM decrypt routines */
	0,					/* samplenames */
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2),	/* color prom */
	0, 	          		/* palette */
	0, 	          		/* color table */
	ORIENTATION_ROTATE_90,

	hiload, hisave
};
