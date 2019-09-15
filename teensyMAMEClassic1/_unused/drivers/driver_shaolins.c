/***************************************************************************

Shaolin's Road memory map (preliminary)



***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *shaolins_nmi_enable;
extern unsigned char *shaolins_scroll;

void shaolins_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void shaolins_palettebank_w(int offset,int data);
void shaolins_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


int shaolins_interrupt(void)
{
	if (cpu_getiloops() == 0) return interrupt();
	else if (cpu_getiloops() % 2)
	{
		if (*shaolins_nmi_enable & 0x02) return nmi_interrupt();
	}

	return ignore_interrupt();
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0500, 0x0500, input_port_3_r },	/* Dipswitch settings */
	{ 0x0600, 0x0600, input_port_4_r },	/* Dipswitch settings */
	{ 0x0700, 0x0700, input_port_0_r },	/* coins + service */
	{ 0x0701, 0x0701, input_port_1_r },	/* player 1 controls */
	{ 0x0702, 0x0702, input_port_2_r },	/* player 2 controls */
	{ 0x0703, 0x0703, input_port_5_r },	/* selftest */
	{ 0x2800, 0x2bff, MRA_RAM },	/* RAM BANK 2 */
	{ 0x3000, 0x33ff, MRA_RAM },	/* RAM BANK 1 */
	{ 0x3800, 0x3fff, MRA_RAM },	/* video RAM */
	{ 0x4000, 0x5fff, MRA_ROM },    /* Machine checks for extra rom */
	{ 0x6000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0000, MWA_RAM, &shaolins_nmi_enable },	/* bit 1 = nmi enable, bit 2 = ? */
														/* bit 3, bit 4 = coin counters */
	{ 0x0100, 0x0100, watchdog_reset_w },
	{ 0x0300, 0x0300, SN76496_0_w }, 	/* trigger chip to read from latch. The program always */
	{ 0x0400, 0x0400, SN76496_1_w }, 	/* writes the same number as the latch, so we don't */
										/* bother emulating them. */
	{ 0x0800, 0x0800, MWA_NOP },	/* latch for 76496 #0 */
	{ 0x1000, 0x1000, MWA_NOP },	/* latch for 76496 #1 */
	{ 0x1800, 0x1800, shaolins_palettebank_w },
	{ 0x2000, 0x2000, MWA_RAM, &shaolins_scroll },
	{ 0x2800, 0x2bff, MWA_RAM },	/* RAM BANK 2 */
	{ 0x3000, 0x30ff, MWA_RAM },	/* RAM BANK 1 */
	{ 0x3100, 0x33ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3800, 0x3bff, colorram_w, &colorram },
	{ 0x3c00, 0x3fff, videoram_w, &videoram, &videoram_size },
	{ 0x6000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( shaolins_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x02, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x18, 0x18, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "30000 70000" )
	PORT_DIPSETTING(    0x10, "40000 80000" )
	PORT_DIPSETTING(    0x08, "40000" )
	PORT_DIPSETTING(    0x00, "50000" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown DSW2 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown DSW2 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown DSW2 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown DSW2 6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown DSW2 7", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown DSW2 8", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW2 */
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
INPUT_PORTS_END



static struct GfxLayout shaolins_charlayout =
{
	8,8,	/* 8*8 chars */
	512,	/* 512 characters */
	4,	/* 4 bits per pixel */
	{ 512*16*8+4, 512*16*8+0, 4, 0 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout shaolins_spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel */
	{ 256*64*8+4, 256*64*8+0, 4, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	{ 24*8+3, 24*8+2, 24*8+1, 24*8+0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
			8*8+3, 8*8+2, 8*8+1, 8*8+0 , 3, 2, 1, 0 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo shaolins_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &shaolins_charlayout,         0, 16*8 },
	{ 1, 0x4000, &shaolins_spritelayout, 16*8*16, 16*8 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	3072000,	/* 3.072 Mhz???? */
	{ 100, 100 }
};



static struct MachineDriver shaolins_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1250000,        /* 1.25 Mhz */
			0,
			readmem,writemem,0,0,
			shaolins_interrupt,16	/* 1 IRQ + 8 NMI */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	shaolins_gfxdecodeinfo,
	256,16*8*16+16*8*16,
	shaolins_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	shaolins_vh_screenrefresh,

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

ROM_START( kicker_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "kikrd8.bin",   0x6000, 0x2000, 0x2598dfdd )
	ROM_LOAD( "kikrd9.bin",   0x8000, 0x4000, 0x0cf0351a )
	ROM_LOAD( "kikrd11.bin",  0xC000, 0x4000, 0x654037f8 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "kikra10.bin",  0x0000, 0x2000, 0x4d156afc )
	ROM_LOAD( "kikra11.bin",  0x2000, 0x2000, 0xff6ca5df )
	ROM_LOAD( "kikrh14.bin",  0x4000, 0x4000, 0xb94e645b )
	ROM_LOAD( "kikrh13.bin",  0x8000, 0x4000, 0x61bbf797 )

	ROM_REGION(0x0500)	/* color proms */
	ROM_LOAD( "kicker.a12",   0x0000, 0x0100, 0xb09db4b4 ) /* palette red component */
	ROM_LOAD( "kicker.a13",   0x0100, 0x0100, 0x270a2bf3 ) /* palette green component */
	ROM_LOAD( "kicker.a14",   0x0200, 0x0100, 0x83e95ea8 ) /* palette blue component */
	ROM_LOAD( "kicker.b8",    0x0300, 0x0100, 0xaa900724 ) /* character lookup table */
	ROM_LOAD( "kicker.f16",   0x0400, 0x0100, 0x80009cf5 ) /* sprite lookup table */
ROM_END

ROM_START( shaolins_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "kikrd8.bin",   0x6000, 0x2000, 0x2598dfdd )
	ROM_LOAD( "kikrd9.bin",   0x8000, 0x4000, 0x0cf0351a )
	ROM_LOAD( "kikrd11.bin",  0xC000, 0x4000, 0x654037f8 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "shaolins.6",   0x0000, 0x2000, 0xff18a7ed )
	ROM_LOAD( "shaolins.7",   0x2000, 0x2000, 0x5f53ae61 )
	ROM_LOAD( "kikrh14.bin",  0x4000, 0x4000, 0xb94e645b )
	ROM_LOAD( "kikrh13.bin",  0x8000, 0x4000, 0x61bbf797 )

	ROM_REGION(0x0500)	/* color proms */
	ROM_LOAD( "kicker.a12",   0x0000, 0x0100, 0xb09db4b4 ) /* palette red component */
	ROM_LOAD( "kicker.a13",   0x0100, 0x0100, 0x270a2bf3 ) /* palette green component */
	ROM_LOAD( "kicker.a14",   0x0200, 0x0100, 0x83e95ea8 ) /* palette blue component */
	ROM_LOAD( "kicker.b8",    0x0300, 0x0100, 0xaa900724 ) /* character lookup table */
	ROM_LOAD( "kicker.f16",   0x0400, 0x0100, 0x80009cf5 ) /* sprite lookup table */
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2b00],"\x1d\x2c\x1f\x01\x00",5) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x2b00],0x40);

			/* top score display */
			memcpy(&RAM[0x2af0], &RAM[0x2b04], 4);

			/* 1p score display, which also displays the top score on startup */
			memcpy(&RAM[0x2a81], &RAM[0x2b04], 4);

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x2b00],0x40);
		osd_fclose(f);
	}
}



struct GameDriver kicker_driver =
{
	__FILE__,
	0,
	"kicker",
	"Kicker",
	"1985",
	"Konami",
	"Allard Van Der Bas (MAME driver)\nMirko Buffoni (additional code)\nPhil Stroffolino (additional code)\nGerald Vanderick (color info)",
	0,
	&shaolins_machine_driver,
	0,

	kicker_rom,
	0, 0,
	0,
	0,

	shaolins_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver shaolins_driver =
{
	__FILE__,
	&kicker_driver,
	"shaolins",
	"Shao-Lin's Road",
	"1985",
	"Konami",
	"Allard Van Der Bas (MAME driver)\nMirko Buffoni (additional code)\nPhil Stroffolino (additional code)\nGerald Vanderick (color info)",
	0,
	&shaolins_machine_driver,
	0,

	shaolins_rom,
	0, 0,
	0,
	0,

	shaolins_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
