/***************************************************************************

  Vigilante

If you have any questions about how this driver works, don't hesitate to
ask.  - Mike Balfour (mab22@po.cwru.edu)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* vidhrdw/vigilant.c */
extern int vigilant_vh_start(void);
extern void vigilant_vh_stop(void);
extern void vigilant_vh_convert_color_prom (unsigned char *palette, unsigned char *colortable, const unsigned char *color_prom);
extern void vigilant_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void vigilant_paletteram_w(int offset, int data);
extern void vigilant_sprite_paletteram_w(int offset, int data);
extern void vigilant_horiz_scroll_w(int offset, int data);
extern void vigilant_rear_horiz_scroll_w(int offset, int data);
extern void vigilant_rear_color_w(int offset, int data);



void vigilant_bank_select_w (int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* This is bad because ROM 7-8K is unpopulated */
	if ((errorlog) && (data & 0x04))
		fprintf(errorlog,"switching to ROM 7-8K!!\n");

	bankaddress = 0x10000 + (data & 0x07) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);
}

/***************************************************************************
 vigilant_out2_w
 **************************************************************************/
void vigilant_out2_w(int offset,int data)
{
	/* D0 = FILP = Flip screen? */
	/* D1 = COA1 = Coin Counter A? */
	/* D2 = COB1 = Coin Counter B? */
}


static int sample_rom_offset;

void vigilant_sample_offset_low_w(int offset,int data)
{
	sample_rom_offset = (sample_rom_offset & 0xFF00) | (data & 0x00FF);
}

void vigilant_sample_offset_high_w(int offset,int data)
{
	sample_rom_offset = (sample_rom_offset & 0x00FF) | ((data & 0x00FF) << 8);
}

int vigilant_sample_rom_r(int offset)
{
	unsigned char *dac_ROM = Machine->memory_region[3];

	return dac_ROM[sample_rom_offset];
}

void vigilant_count_up_w(int offset,int data)
{
	unsigned char *dac_ROM = Machine->memory_region[3];

	sample_rom_offset++;
	DAC_data_w(0,dac_ROM[sample_rom_offset]);
}


void vigilant_soundcmd_w(int offset,int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(1,0x00df);	/* RST 18h (could be RST 08h instead) */
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc020, 0xc0df, MRA_RAM },
	{ 0xc800, 0xcbff, MRA_RAM },
	{ 0xcc00, 0xcfff, MRA_RAM },
	{ 0xd000, 0xdfff, videoram_r },
	{ 0xe000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc020, 0xc0df, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc800, 0xcfff, vigilant_paletteram_w, &paletteram },
	{ 0xd000, 0xdfff, videoram_w, &videoram, &videoram_size },
	{ 0xe000, 0xefff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ 0x04, 0x04, input_port_4_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, vigilant_soundcmd_w },  /* SD */
	{ 0x01, 0x01, vigilant_out2_w }, /* OUT2 */
	{ 0x04, 0x04, vigilant_bank_select_w }, /* PBANK */
	{ 0x80, 0x81, vigilant_horiz_scroll_w }, /* HSPL, HSPH */
	{ 0x82, 0x83, vigilant_rear_horiz_scroll_w }, /* RHSPL, RHSPH */
	{ 0x84, 0x84, vigilant_rear_color_w }, /* RCOD */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xf000, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0x80, 0x80, soundlatch_r },  /* SDRE */
	{ 0x84, 0x84, vigilant_sample_rom_r },  /* S ROM C */
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x80, 0x80, vigilant_sample_offset_low_w }, /* STL */
	{ 0x81, 0x81, vigilant_sample_offset_high_w }, /* STH */
	{ 0x82, 0x82, vigilant_count_up_w }, /* COUNT UP */
/*	{ 0x83, 0x83, interrupt acknowledge? */
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( vigilant_input_ports )
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(0xF0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(	0x02, "2" )
	PORT_DIPSETTING(	0x03, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(	0x04, "Normal" )
	PORT_DIPSETTING(	0x00, "Hard" )
	PORT_DIPNAME( 0x08, 0x08, "Decrease of Energy", IP_KEY_NONE )
	PORT_DIPSETTING(	0x08, "Slow" )
	PORT_DIPSETTING(	0x00, "Fast" )
	/* TODO: support the different settings which happen in Coin Mode 2 */
	PORT_DIPNAME( 0xF0, 0xF0, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(	0xA0, "6 Coins/1 Credit" )
	PORT_DIPSETTING(	0xB0, "5 Coins/1 Credit" )
	PORT_DIPSETTING(	0xC0, "4 Coins/1 Credit" )
	PORT_DIPSETTING(	0xD0, "3 Coins/1 Credit" )
	PORT_DIPSETTING(	0x10, "8 Coins/3 Credits" )
	PORT_DIPSETTING(	0xE0, "2 Coins/1 Credit" )
	PORT_DIPSETTING(	0x20, "5 Coins/3 Credits" )
	PORT_DIPSETTING(	0x30, "3 Coins/2 Credits" )
	PORT_DIPSETTING(	0xF0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(	0x40, "2 Coins/3 Credits" )
	PORT_DIPSETTING(	0x90, "1 Coin/2 Credits" )
	PORT_DIPSETTING(	0x80, "1 Coin/3 Credits" )
	PORT_DIPSETTING(	0x70, "1 Coin/4 Credits" )
	PORT_DIPSETTING(	0x60, "1 Coin/5 Credits" )
	PORT_DIPSETTING(	0x50, "1 Coin/6 Credits" )
	PORT_DIPSETTING(	0x00, "Free Play" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(	0x01, "Off" )
	PORT_DIPSETTING(	0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )
/* This activates a different coin mode. Look at the dip switch setting schematic */
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode", IP_KEY_NONE )
	PORT_DIPSETTING(	0x04, "Mode 1" )
	PORT_DIPSETTING(	0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "Off" )
	PORT_DIPSETTING(	0x08, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "No" )
	PORT_DIPSETTING(	0x10, "Yes" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(	0x80, "Off" )
	PORT_DIPSETTING(	0x00, "On" )
INPUT_PORTS_END


static struct GfxLayout text_layout =
{
	8,8, /* tile size */
	4096, /* number of tiles */
	4, /* bits per pixel */
	{0,4,64*1024*8,64*1024*8+4}, /* plane offsets */
	{ 0,1,2,3, 64+0,64+1,64+2,64+3 }, /* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 }, /* y offsets */
	128
};

static struct GfxLayout sprite_layout =
{
	16,16,	/* tile size */
	4096,	/* number of sprites ($1000) */
	4,		/* bits per pixel */
	{0,4,0x40000*8,0x40000*8+4}, /* plane offsets */
	{ /* x offsets */
		0x00*8+0,0x00*8+1,0x00*8+2,0x00*8+3,
		0x10*8+0,0x10*8+1,0x10*8+2,0x10*8+3,
		0x20*8+0,0x20*8+1,0x20*8+2,0x20*8+3,
		0x30*8+0,0x30*8+1,0x30*8+2,0x30*8+3
	},
	{ /* y offsets */
		0x00*8, 0x01*8, 0x02*8, 0x03*8,
		0x04*8, 0x05*8, 0x06*8, 0x07*8,
		0x08*8, 0x09*8, 0x0A*8, 0x0B*8,
		0x0C*8, 0x0D*8, 0x0E*8, 0x0F*8
	},
	0x40*8
};

static struct GfxLayout back_layout =
{
	32,1, /* tile size */
	512*8, /* number of tiles */
	4, /* bits per pixel */
	{0,2,4,6}, /* plane offsets */
	{ 0*8+1, 0*8,  1*8+1, 1*8, 2*8+1, 2*8, 3*8+1, 3*8, 4*8+1, 4*8, 5*8+1, 5*8,
	6*8+1, 6*8, 7*8+1, 7*8, 8*8+1, 8*8, 9*8+1, 9*8, 10*8+1, 10*8, 11*8+1, 11*8,
	12*8+1, 12*8, 13*8+1, 13*8, 14*8+1, 14*8, 15*8+1, 15*8 }, /* x offsets */
	{ 0 }, /* y offsets */
	16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &text_layout,   256, 16 },	/* colors 256-511 */
	{ 1, 0x20000, &sprite_layout,   0, 16 },	/* colors   0-255 */
	{ 1, 0xA0000, &back_layout,   512,  2 },	/* actually the background uses colors */
	{ 1, 0xB0000, &back_layout,   512,  2 },	/* 256-511, but giving it exclusive */
	{ 1, 0xC0000, &back_layout,   512,  2 },	/* pens we can handle it more easily. */
	{ -1 } /* end of array */
};



/* handler called by the 2151 emulator when the internal timers cause an IRQ */
static void vigilant_irq_handler(void)
{
	cpu_cause_interrupt(1,0x00ef);	/* RST 28h */
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579645,	/* 3.579645 MHz */
	{ 60 },
	{ vigilant_irq_handler }
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
			CPU_Z80,
			3579645,		   /* 3.579645 MHz */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579645,		   /* 3.579645 MHz */
			2,
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			nmi_interrupt,128	/* clocked by V1 */
								/* RST 18h (RST 08h?) is caused by the main CPU */
								/* RST 28h is caused by the YM2151 */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* no need for interleaving */
	0,

	/* video hardware */
	64*8, 32*8, { 16*8, 48*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	512+32,512+32,
	0, /* no color prom */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	vigilant_vh_start,
	vigilant_vh_stop,
	vigilant_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}

};



/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( vigilant_rom )
	ROM_REGION(0x20000) /* region#0: 64k for code + 64k for bankswitching */
	ROM_LOAD( "g07_c03.bin",  0x00000, 0x08000, 0x9dcca081 )
	ROM_LOAD( "j07_c04.bin",  0x10000, 0x10000, 0xe0159105 )

	ROM_REGION_DISPOSE(0xD0000) /* region #1: graphics (disposed after conversion) */
	ROM_LOAD( "h05_c09.bin",  0x00000, 0x10000, 0x4f5872f0 )
	ROM_LOAD( "f05_c08.bin",  0x10000, 0x10000, 0x01579d20 )
	ROM_LOAD( "t07_c16.bin",  0x20000, 0x10000, 0xf5425e42 )
	ROM_LOAD( "v07_c17.bin",  0x30000, 0x10000, 0x959ba3c7 )
	ROM_LOAD( "p07_c14.bin",  0x40000, 0x10000, 0xcb50a17c )
	ROM_LOAD( "s07_c15.bin",  0x50000, 0x10000, 0x7f2e91c5 )
	ROM_LOAD( "n07_c12.bin",  0x60000, 0x10000, 0x10af8eb2 )
	ROM_LOAD( "o07_c13.bin",  0x70000, 0x10000, 0xb1d9d4dc )
	ROM_LOAD( "k07_c10.bin",  0x80000, 0x10000, 0x9576f304 )
	ROM_LOAD( "l07_c11.bin",  0x90000, 0x10000, 0x4598be4a )
	ROM_LOAD( "d01_c05.bin",  0xA0000, 0x10000, 0x81b1ee5c )
	ROM_LOAD( "e01_c06.bin",  0xB0000, 0x10000, 0xd0d33673 )
	ROM_LOAD( "f01_c07.bin",  0xC0000, 0x10000, 0xaae81695 )

	ROM_REGION(0x10000) /* region#2: 64k for sound */
	ROM_LOAD( "g05_c02.bin",  0x00000, 0x10000, 0x10582b2d )

	ROM_REGION(0x10000) /* region#3: 64k for sample ROM */
	ROM_LOAD( "d04_c01.bin",  0x00000, 0x10000, 0x9b85101d )
ROM_END

ROM_START( vigilntj_rom )
	ROM_REGION(0x20000) /* region#0: 64k for code + 64k for bankswitching */
	ROM_LOAD( "vg_a-8h.rom",  0x00000, 0x08000, 0xba848713 )
	ROM_LOAD( "vg_a-8l.rom",  0x10000, 0x10000, 0x3b12b1d8 )

	ROM_REGION_DISPOSE(0xD0000) /* region #1: graphics (disposed after conversion) */
	ROM_LOAD( "h05_c09.bin",  0x00000, 0x10000, 0x4f5872f0 )
	ROM_LOAD( "f05_c08.bin",  0x10000, 0x10000, 0x01579d20 )
	ROM_LOAD( "t07_c16.bin",  0x20000, 0x10000, 0xf5425e42 )
	ROM_LOAD( "v07_c17.bin",  0x30000, 0x10000, 0x959ba3c7 )
	ROM_LOAD( "p07_c14.bin",  0x40000, 0x10000, 0xcb50a17c )
	ROM_LOAD( "s07_c15.bin",  0x50000, 0x10000, 0x7f2e91c5 )
	ROM_LOAD( "n07_c12.bin",  0x60000, 0x10000, 0x10af8eb2 )
	ROM_LOAD( "o07_c13.bin",  0x70000, 0x10000, 0xb1d9d4dc )
	ROM_LOAD( "k07_c10.bin",  0x80000, 0x10000, 0x9576f304 )
	ROM_LOAD( "l07_c11.bin",  0x90000, 0x10000, 0x4598be4a )
	ROM_LOAD( "d01_c05.bin",  0xA0000, 0x10000, 0x81b1ee5c )
	ROM_LOAD( "e01_c06.bin",  0xB0000, 0x10000, 0xd0d33673 )
	ROM_LOAD( "f01_c07.bin",  0xC0000, 0x10000, 0xaae81695 )

	ROM_REGION(0x10000) /* region#2: 64k for sound */
	ROM_LOAD( "g05_c02.bin",  0x00000, 0x10000, 0x10582b2d )

	ROM_REGION(0x10000) /* region#3: 64k for sample ROM */
	ROM_LOAD( "d04_c01.bin",  0x00000, 0x10000, 0x9b85101d )
ROM_END

static int vigilant_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0xe04b],"\x00\x45\x11",3) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0xe04b],60);
			osd_fclose(f);

			/* copy the high score to the work RAM as well */
                        RAM[0xe048] = RAM[0xe04b];
                        RAM[0xe049] = RAM[0xe04c];
                        RAM[0xe04a] = RAM[0xe04d];

		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void vigilant_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0xe04b],60);
		osd_fclose(f);
	}
}


struct GameDriver vigilant_driver =
{
	__FILE__,
	0,
	"vigilant",
	"Vigilante (US)",
	"1988",
	"Irem",
	"Mike Balfour\nPhil Stroffolino\nNicola Salmoria",
	0,
	&machine_driver,
	0,

	vigilant_rom,
	0,0,
	0,
	0,	/* sound_prom */

	vigilant_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
        vigilant_hiload, vigilant_hisave
};

struct GameDriver vigilntj_driver =
{
	__FILE__,
	&vigilant_driver,
	"vigilntj",
	"Vigilante (Japan)",
	"1988",
	"Irem",
	"Mike Balfour\nPhil Stroffolino\nNicola Salmoria",
	0,
	&machine_driver,
	0,

	vigilntj_rom,
	0,0,
	0,
	0,	/* sound_prom */

	vigilant_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
        vigilant_hiload, vigilant_hisave
};
