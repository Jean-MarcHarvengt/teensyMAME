/* Kangaroo driver
Kangaroo (c) Atari Games / Sun Electronics Corp 1982

   Changes:
   97/06/19 - mod to ensure it's really safe to load the scores.

   97/06/17 - added the coin counter output so the error log will
              not get cluttered with so much garbage about unknown locations.
            - added music on/off to dip switch settings.
              Thanks to S. Joe Dunkle for mentioning the game should
              have music. btw. this is not really a dip switch on the PCB,
              it's a pin on the edge connector and the damn manual doesn't
              really tell what it does, so I hadn't ever tried it out...
            - added high score saving/loading. Since I try to avoid disassembling
              the code at all costs I'm not sure it's correct - seems to work tho' ;-) -V-

   97/05/07 - changed to conform the new AUDIO_CPU code
            - changed the audio command read to use the latched version.

   97/04/xx - created, based on the Arabian driver,
            The two games (arabian & kangaroo) are both by Sun Electronics
            and run on very similar hardware.
            Kangaroo PCB is constructed from two boards:
            TVG-1-CPU-B , which houses program ROMS, two Z80 CPUs,
             a GI-AY-8910, a custom microcontroller and the logic chips connecting these.
            TVG-1-VIDEO-B is obviously the video board. On this one
             you can find the graphics ROMS (tvg83 -tvg86), some logic
             chips and 4 banks of 8 4116 RAM chips.

            */
/* TODO */
/* This is still a bit messy after my cut/paste from the arabian driver */
/* will have to clean up && correct the sound problems */

/***************************************************************************

Kangaroo memory map

CPU #0

0000-0fff tvg75
1000-1fff tvg76
2000-2fff tvg77
3000-3fff tvg78
4000-4fff tvg79
5000-5fff tvg80
8000-bfff VIDEO RAM (four banks)
c000-cfff tvg83/84 (banked)
d000-dfff tvg85/86 (banked)
e000-e3ff RAM


memory mapped ports:
read:
e400      DSW 0
ec00      IN 0
ed00      IN 1
ee00      IN 2
efxx      (4 bits wide) security chip in. It seems to work like a clock.

write:
e800-e801 low/high byte start address of data in picture ROM for DMA
e802-e803 low/high byte start address in bitmap RAM (where picture is to be
          written) during DMA
e804-e805 picture size for DMA, and DMA start
e806      vertical start address in bitmap
e807      horizontal start address in bitmap
e808      bank select latch
e809      A & B bitmap control latch (A=playfield B=motion)
          bit 5 FLIP A
          bit 4 FLIP B
          bit 3 EN A
          bit 2 EN B
          bit 1 PRI A
          bit 0 PRI B
e80a      color shading latch
ec00      command to sound CPU
ed00      coin counters
efxx      (4 bits wide) security chip out

---------------------------------------------------------------------------
CPU #1 (sound)

0000 0fff tvg81
4000 43ff RAM
6000      command from main CPU

I/O ports:
7000      AY-3-8910 write
8000      AY-3-8910 control
---------------------------------------------------------------------------

interrupts:
(CPU#0) standard IM 1 interrupt mode (rst #38 every vblank)
(CPU#1) same here
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80/Z80.h"



/* machine */
int  kangaroo_sec_chip_r(int offset);
void kangaroo_sec_chip_w(int offset,int val);

/* vidhrdw */
extern unsigned char *kangaroo_bank_select;
extern unsigned char *kangaroo_blitter;
void kangaroo_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int  kangaroo_vh_start(void);
void kangaroo_vh_stop(void);
void kangaroo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void kangaroo_blitter_w(int offset, int val);
void kangaroo_videoram_w(int offset, int val);
void kangaroo_color_mask_w(int offset,int data);



static void kangaroo_init_machine(void)
{
	/* I think there is a bug in the startup checks of the game. At the very */
	/* beginning, during the RAM check, it goes one byte too far, and ends up */
	/* trying to write, and re-read, location dfff. To the best of my knowledge, */
	/* that is a ROM address, so the test fails and the code keeps jumping back */
	/* at 0000. */
	/* However, a NMI causes a successful reset. Maybe the hardware generates a */
	/* NMI short after power on, therefore masking the bug? The NMI is generated */
	/* by the MB8841 custom microcontroller, so this could be a way to disguise */
	/* the copy protection. */
	/* Anyway, what I do here is just immediately generate the NMI, so the game */
	/* properly starts. */
	cpu_cause_interrupt(0,Z80_NMI_INT);
}



void kangaroo_bank_select_w(int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* this is a VERY crude way to handle the banked ROMs - but it's */
	/* correct enough to pass the self test */
	if (data & 0x05) { cpu_setbank(1,&RAM[0x10000]); }
	else { cpu_setbank(1,&RAM[0x12000]); }

	*kangaroo_bank_select = data;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0xc000, 0xdfff, MRA_BANK1 },
	{ 0xe000, 0xe3ff, MRA_RAM },
	{ 0xe400, 0xe400, input_port_3_r },
	{ 0xec00, 0xec00, input_port_0_r },
	{ 0xed00, 0xed00, input_port_1_r },
	{ 0xee00, 0xee00, input_port_2_r },
	{ 0xef00, 0xef00, kangaroo_sec_chip_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x8000, 0xbfff, kangaroo_videoram_w },
	{ 0xc000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xe3ff, MWA_RAM },
	{ 0xe800, 0xe807, kangaroo_blitter_w, &kangaroo_blitter },
	{ 0xe808, 0xe808, kangaroo_bank_select_w, &kangaroo_bank_select },
	{ 0xe809, 0xe809, MWA_RAM },	/* playfield flip, enable, priority */
	{ 0xe80a, 0xe80a, kangaroo_color_mask_w },
	{ 0xec00, 0xec00, soundlatch_w },
	{ 0xed00, 0xed00, MWA_NOP },
	{ 0xef00, 0xefff, kangaroo_sec_chip_w },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sh_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sh_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOReadPort sh_readport[] =
{
	{ -1 }	/* end of table */
};

static struct IOWritePort sh_writeport[] =
{
	{ 0x7000, 0x7000, AY8910_write_port_0_w },
	{ 0x8000, 0x8000, AY8910_control_port_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x20, 0x00, "Music", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x80, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x02, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPNAME( 0x0c, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "10000 30000" )
	PORT_DIPSETTING(    0x04, "10000" )
	PORT_DIPSETTING(    0x0c, "20000 40000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xf0, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "A 2/1 B 2/1" )
	PORT_DIPSETTING(    0x20, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/1" )
	PORT_DIPSETTING(    0x30, "A 1/1 B 1/2" )
	PORT_DIPSETTING(    0x40, "A 1/1 B 1/3" )
	PORT_DIPSETTING(    0x50, "A 1/1 B 1/4" )
	PORT_DIPSETTING(    0x60, "A 1/1 B 1/5" )
	PORT_DIPSETTING(    0x70, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x80, "A 1/2 B 1/2" )
	PORT_DIPSETTING(    0x90, "A 1/2 B 1/4" )
	PORT_DIPSETTING(    0xa0, "A 1/2 B 1/5" )
	PORT_DIPSETTING(    0xe0, "A 1/2 B 1/6" )
	PORT_DIPSETTING(    0xb0, "A 1/2 B 1/10" )
	PORT_DIPSETTING(    0xc0, "A 1/2 B 1/11" )
	PORT_DIPSETTING(    0xd0, "A 1/2 B 1/12" )
	/* 0xe0 gives A 1/2 B 1/6 */
	PORT_DIPSETTING(    0xf0, "Free Play" )
INPUT_PORTS_END



static struct GfxLayout sprlayout =
{
	1,4,    /* 4 * 1 lines */
	8192,   /* 8192 "characters" */
	4,      /* 4 bpp        */
	{ 0, 4, 0x2000*8, 0x2000*8+4 },       /* 4 and 8192 bytes between planes */
	{ 0 },
	{ 0,1,2,3 },
	1*8
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x10000, &sprlayout, 0, 2 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	10000000/8,     /* 1.25 MHz */
	{ 255 },
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
			10000000/4,	/* 2.5 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_16BIT_PORT | CPU_AUDIO_CPU,
			10000000/4,	/* 2.5 MHz */
			2,
			sh_readmem,sh_writemem,sh_readport,sh_writeport,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	kangaroo_init_machine,

	/* video hardware */
	32*8, 32*8, { 0x0b, 0xfa, 0, 32*8-1 },
	gfxdecodeinfo,
	8+8+8,2*16,
	kangaroo_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,
	kangaroo_vh_start,
	kangaroo_vh_stop,
	kangaroo_vh_screenrefresh,

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

ROM_START( kangaroo_rom )
	ROM_REGION(0x14000)	/* 64k for code + 16k for banked ROMs */
	ROM_LOAD( "tvg75.bin",    0x0000, 0x1000, 0x0d18c581 )
	ROM_LOAD( "tvg76.bin",    0x1000, 0x1000, 0x5978d37a )
	ROM_LOAD( "tvg77.bin",    0x2000, 0x1000, 0x522d1097 )
	ROM_LOAD( "tvg78.bin",    0x3000, 0x1000, 0x063da970 )
	ROM_LOAD( "tvg79.bin",    0x4000, 0x1000, 0x82a26c7d )
	ROM_LOAD( "tvg80.bin",    0x5000, 0x1000, 0x3dead542 )

	ROM_LOAD( "tvg83.bin",    0x10000, 0x1000, 0xc0446ca6 )	/* graphics ROMs */
	ROM_LOAD( "tvg85.bin",    0x11000, 0x1000, 0x72c52695 )
	ROM_LOAD( "tvg84.bin",    0x12000, 0x1000, 0xe4cb26c2 )
	ROM_LOAD( "tvg86.bin",    0x13000, 0x1000, 0x9e6a599f )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000) /* sound */
	ROM_LOAD( "tvg81.bin",    0x0000, 0x1000, 0xfb449bfd )

	ROM_REGION(0x0800)	/* 8k for the MB8841 custom microcontroller (currently not emulated) */
	ROM_LOAD( "tvg82.bin",    0x0000, 0x0800, 0x57766f69 )
ROM_END

ROM_START( kangarob_rom )
	ROM_REGION(0x14000)	/* 64k for code + 16k for banked ROMs */
	ROM_LOAD( "tvg75.bin",    0x0000, 0x1000, 0x0d18c581 )
	ROM_LOAD( "tvg76.bin",    0x1000, 0x1000, 0x5978d37a )
	ROM_LOAD( "tvg77.bin",    0x2000, 0x1000, 0x522d1097 )
	ROM_LOAD( "tvg78.bin",    0x3000, 0x1000, 0x063da970 )
	ROM_LOAD( "k5",           0x4000, 0x1000, 0x9e5cf8ca )
	ROM_LOAD( "k6",           0x5000, 0x1000, 0x7644504a )

	ROM_LOAD( "tvg83.bin",    0x10000, 0x1000, 0xc0446ca6 )	/* graphics ROMs */
	ROM_LOAD( "tvg85.bin",    0x11000, 0x1000, 0x72c52695 )
	ROM_LOAD( "tvg84.bin",    0x12000, 0x1000, 0xe4cb26c2 )
	ROM_LOAD( "tvg86.bin",    0x13000, 0x1000, 0x9e6a599f )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000) /* sound */
	ROM_LOAD( "tvg81.bin",    0x0000, 0x1000, 0xfb449bfd )
ROM_END



static int kangaroo_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xe1a3],"\x00\x50\x00",3) == 0 &&
			memcmp(&RAM[0xe1d9],"\x00\x50\x00",3) == 0)
	{
		void *f;
		if (( f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xe1a0],6*10);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void kangaroo_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xe1a0],6*10);
		osd_fclose(f);
	}
}



struct GameDriver kangaroo_driver =
{
	__FILE__,
	0,
	"kangaroo",
	"Kangaroo",
	"1982",
	"Atari",
	"Ville Laitinen (MAME driver)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	kangaroo_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	kangaroo_hiload, kangaroo_hisave
};

struct GameDriver kangarob_driver =
{
	__FILE__,
	&kangaroo_driver,
	"kangarob",
	"Kangaroo (bootleg)",
	"1982",
	"bootleg",
	"Ville Laitinen (MAME driver)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	kangarob_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	kangaroo_hiload, kangaroo_hisave
};
