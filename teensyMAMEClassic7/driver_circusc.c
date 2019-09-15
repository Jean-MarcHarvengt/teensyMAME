/***************************************************************************

Based on drivers from Juno First emulator by Chris Hardy (chrish@kcbbs.gen.nz)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809.h"



extern unsigned char *circusc_spritebank;
extern unsigned char *circusc_scroll;

void circusc_flipscreen_w(int offset,int data);
void circusc_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void circusc_sprite_bank_select_w(int offset, int data);
void circusc_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

unsigned char KonamiDecode( unsigned char opcode, unsigned short address );



void circusc_init_machine(void)
{
	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_S | M6809_FAST_U;
}

int circusc_sh_timer_r(int offset)
{
	int clock;
#define CIRCUSCHALIE_TIMER_RATE (14318180/6144)

	clock = (cpu_gettotalcycles()*4) / CIRCUSCHALIE_TIMER_RATE;

	return clock & 0xF;
}

void circusc_sh_irqtrigger_w(int offset,int data)
{
	cpu_cause_interrupt(1,0xff);
}

void circusc_dac_w(int offset,int data)
{
	DAC_data_w(0,data);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x1000, 0x1000, input_port_0_r }, /* IO Coin */
	{ 0x1001, 0x1001, input_port_1_r }, /* P1 IO */
	{ 0x1002, 0x1002, input_port_2_r }, /* P2 IO */
	{ 0x1400, 0x1400, input_port_3_r }, /* DIP 1 */
	{ 0x1800, 0x1800, input_port_4_r }, /* DIP 2 */
	{ 0x2000, 0x39ff, MRA_RAM },
	{ 0x6000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0000, circusc_flipscreen_w },
	{ 0x0001, 0x0001, interrupt_enable_w },
	{ 0x0003, 0x0004, coin_counter_w },  /* Coin counters */
	{ 0x0005, 0x0005, MWA_RAM, &circusc_spritebank },
	{ 0x0400, 0x0400, watchdog_reset_w },
	{ 0x0800, 0x0800, soundlatch_w },
	{ 0x0c00, 0x0c00, circusc_sh_irqtrigger_w },  /* cause interrupt on audio CPU */
	{ 0x1c00, 0x1c00, MWA_RAM, &circusc_scroll },
	{ 0x2000, 0x2fff, MWA_RAM },
	{ 0x3000, 0x33ff, colorram_w, &colorram },
	{ 0x3400, 0x37ff, videoram_w, &videoram, &videoram_size },
	{ 0x3800, 0x38ff, MWA_RAM, &spriteram_2 },
	{ 0x3900, 0x39ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3a00, 0x3fff, MWA_RAM },
	{ 0x6000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0x8000, 0x8000, circusc_sh_timer_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0xa000, 0xa000, MWA_NOP },	/* latch command for the 76496. We should buffer this */
									/* command and send it to the chip, but we just use */
									/* the triggers below because the program always writes */
									/* the same number here and there. */
	{ 0xa001, 0xa001, SN76496_0_w },	/* trigger the 76496 to read the latch */
	{ 0xa002, 0xa002, SN76496_1_w },	/* trigger the 76496 to read the latch */
	{ 0xa003, 0xa003, circusc_dac_w },
	{ 0xa004, 0xa004, MWA_NOP },		/* ??? */
	{ 0xa07c, 0xa07c, MWA_NOP },		/* ??? */
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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
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
	PORT_DIPSETTING(    0x00, "Free Play" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x08, 0x08, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "20000 70000" )
	PORT_DIPSETTING(    0x00, "30000 80000" )
	PORT_DIPNAME( 0x10, 0x10, "Dip Sw2 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Dip Sw2 6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Dip Sw2 7", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	384,	/* 384 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },        /* the bitplanes are packed */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*4*16, 1*4*16, 2*4*16, 3*4*16, 4*4*16, 5*4*16, 6*4*16, 7*4*16,
			8*4*16, 9*4*16, 10*4*16, 11*4*16, 12*4*16, 13*4*16, 14*4*16, 15*4*16 },
	32*4*8    /* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 16 },
	{ 1, 0x4000, &spritelayout, 16*16, 16 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	14318180/8,	/*  1.7897725 Mhz */
	{ 100, 100 }
};

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			2048000,        /* 2 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318180/4,	/* Z80 Clock is derived from a 14.31818 Mhz crystal */
			3,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	circusc_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32,16*16+16*16,
	circusc_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	circusc_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( circusc_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "h03_r05.bin",  0x6000, 0x2000, 0xed52c60f )
	ROM_LOAD( "h04_n04.bin",  0x8000, 0x2000, 0xfcc99e33 )
	ROM_LOAD( "h05_n03.bin",  0xA000, 0x2000, 0x5ef5b3b5 )
	ROM_LOAD( "h06_n02.bin",  0xC000, 0x2000, 0xa5a5e796 )
	ROM_LOAD( "h07_n01.bin",  0xE000, 0x2000, 0x70d26721 )

	ROM_REGION_DISPOSE(0x10000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a04_j12.bin",  0x0000, 0x2000, 0x56e5b408 )
	ROM_LOAD( "a05_k13.bin",  0x2000, 0x2000, 0x5aca0193 )
	ROM_LOAD( "e11_j06.bin",  0x4000, 0x2000, 0xdf0405c6 )
	ROM_LOAD( "e12_j07.bin",  0x6000, 0x2000, 0x23dfe3a6 )
	ROM_LOAD( "e13_j08.bin",  0x8000, 0x2000, 0x3ba95390 )
	ROM_LOAD( "e14_j09.bin",  0xa000, 0x2000, 0xa9fba85a )
	ROM_LOAD( "e15_j10.bin",  0xc000, 0x2000, 0x0532347e )
	ROM_LOAD( "e16_j11.bin",  0xe000, 0x2000, 0xe1725d24 )

	ROM_REGION(0x220)      /* color proms */
	ROM_LOAD( "a02_j18.bin",  0x0000, 0x020, 0x10dd4eaa ) /* palette */
	ROM_LOAD( "c10_j16.bin",  0x0020, 0x100, 0xc244f2aa ) /* character lookup table */
	ROM_LOAD( "b07_j17.bin",  0x0120, 0x100, 0x13989357 ) /* sprite lookup table */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "cd05_l14.bin", 0x0000, 0x2000, 0x607df0fb )
	ROM_LOAD( "cd07_l15.bin", 0x2000, 0x2000, 0xa6ad30e1 )
ROM_END

ROM_START( circusc2_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "s05",          0x6000, 0x2000, 0x48feafcf )
	ROM_LOAD( "q04",          0x8000, 0x2000, 0xc283b887 )
	ROM_LOAD( "q03",          0xA000, 0x2000, 0xe90c0e86 )
	ROM_LOAD( "q02",          0xC000, 0x2000, 0x4d847dc6 )
	ROM_LOAD( "q01",          0xE000, 0x2000, 0x18c20adf )

	ROM_REGION_DISPOSE(0x10000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a04_j12.bin",  0x0000, 0x2000, 0x56e5b408 )
	ROM_LOAD( "a05_k13.bin",  0x2000, 0x2000, 0x5aca0193 )
	ROM_LOAD( "e11_j06.bin",  0x4000, 0x2000, 0xdf0405c6 )
	ROM_LOAD( "e12_j07.bin",  0x6000, 0x2000, 0x23dfe3a6 )
	ROM_LOAD( "e13_j08.bin",  0x8000, 0x2000, 0x3ba95390 )
	ROM_LOAD( "e14_j09.bin",  0xa000, 0x2000, 0xa9fba85a )
	ROM_LOAD( "e15_j10.bin",  0xc000, 0x2000, 0x0532347e )
	ROM_LOAD( "e16_j11.bin",  0xe000, 0x2000, 0xe1725d24 )

	ROM_REGION(0x220)      /* color proms */
	ROM_LOAD( "a02_j18.bin",  0x0000, 0x020, 0x10dd4eaa ) /* palette */
	ROM_LOAD( "c10_j16.bin",  0x0020, 0x100, 0xc244f2aa ) /* character lookup table */
	ROM_LOAD( "b07_j17.bin",  0x0120, 0x100, 0x13989357 ) /* sprite lookup table */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "cd05_l14.bin", 0x0000, 0x2000, 0x607df0fb )
	ROM_LOAD( "cd07_l15.bin", 0x2000, 0x2000, 0xa6ad30e1 )
ROM_END

static void circusc_decode(void)
{
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	for (A = 0x6000;A < 0x10000;A++)
	{
		ROM[A] = KonamiDecode(RAM[A],A);
	}
}



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0x2163],"CBR",3) == 0 &&
		memcmp(&RAM[0x20A6],"\x01\x98\x30",3) == 0 &&
		memcmp(&RAM[0x3627],"\x01",1) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x2160],0x32);
			RAM[0x20A6] = RAM[0x2160];
			RAM[0x20A7] = RAM[0x2161];
			RAM[0x20A8] = RAM[0x2162];

			/* Copy high score to videoram */
			RAM[0x35A7] = RAM[0x2162] & 0x0F;
			RAM[0x35C7] = (RAM[0x2162] & 0xF0) >> 4;
			RAM[0x35E7] = RAM[0x2161] & 0x0F;
			RAM[0x3607] = (RAM[0x2161] & 0xF0) >> 4;
			RAM[0x3627] = RAM[0x2160] & 0x0F;
			if ((RAM[0x2160] & 0xF0) != 0)
				RAM[0x3647] = (RAM[0x2160] & 0xF0) >> 4;

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
		osd_fwrite(f,&RAM[0x2160],0x32);
		osd_fclose(f);
	}
}



struct GameDriver circusc_driver =
{
	__FILE__,
	0,
	"circusc",
	"Circus Charlie",
	"1984",
	"Konami",
	"Chris Hardy (MAME driver)\nPaul Swan (color info)",
	0,
	&machine_driver,
	0,

	circusc_rom,
	0, circusc_decode,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver circusc2_driver =
{
	__FILE__,
	&circusc_driver,
	"circusc2",
	"Circus Charlie (level select)",
	"1984",
	"Konami",
	"Chris Hardy (MAME driver)\nPaul Swan (color info)",
	0,
	&machine_driver,
	0,

	circusc2_rom,
	0, circusc_decode,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};
