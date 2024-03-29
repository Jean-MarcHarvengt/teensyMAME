/***************************************************************************

Based on drivers from Juno First emulator by Chris Hardy (chrish@kcbbs.gen.nz)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809.h"



void rocnrope_flipscreen_w(int offset,int data);
void rocnrope_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void rocnrope_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

unsigned char KonamiDecode( unsigned char opcode, unsigned short address );


void rocnrope_init_machine(void)
{
	/* Set optimization flags for M6809 */
//	m6809_Flags = M6809_FAST_S | M6809_FAST_U;
}

/* Roc'n'Rope has the IRQ vectors in RAM. The rom contains $FFFF at this address! */
void rocnrope_interrupt_vector_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	RAM[0xFFF2+offset] = data;
}

/* I am not 100% sure that this timer is correct, but */
/* I'm using the Gyruss wired to the higher 4 bits    */
/* instead of the lower ones, so there is a good      */
/* chance it's the right one. */

/* The timer clock which feeds the lower 4 bits of    */
/* AY-3-8910 port A is based on the same clock        */
/* feeding the sound CPU Z80.  It is a divide by      */
/* 10240, formed by a standard divide by 1024,        */
/* followed by a divide by 10 using a 4 bit           */
/* bi-quinary count sequence. (See LS90 data sheet    */
/* for an example).                                   */
/* Bits 1-3 come directly from the upper three bits   */
/* of the bi-quinary counter. Bit 0 comes from the    */
/* output of the divide by 1024.                      */

static int rocnrope_timer[20] = {
0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03, 0x04, 0x05,
0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b, 0x0c, 0x0d
};

static int rocnrope_portB_r(int offset)
{
	/* need to protect from totalcycles overflow */
	static int last_totalcycles = 0;

	/* number of Z80 clock cycles to count */
	static int clock;

	int current_totalcycles;

	current_totalcycles = cpu_gettotalcycles();
	clock = (clock + (current_totalcycles-last_totalcycles)) % 10240;

	last_totalcycles = current_totalcycles;

	return rocnrope_timer[clock/512] << 4;
}

void rocnrope_sh_irqtrigger_w(int offset,int data)
{
	static int last;


	if (last == 0 && data == 1)
	{
		/* setting bit 0 low then high triggers IRQ on the sound CPU */
		cpu_cause_interrupt(1,0xff);
	}

	last = data;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x3080, 0x3080, input_port_0_r }, /* IO Coin */
	{ 0x3081, 0x3081, input_port_1_r }, /* P1 IO */
	{ 0x3082, 0x3082, input_port_2_r }, /* P2 IO */
	{ 0x3083, 0x3083, input_port_3_r }, /* DSW 0 */
	{ 0x3000, 0x3000, input_port_4_r }, /* DSW 1 */
	{ 0x3100, 0x3100, input_port_5_r }, /* DSW 2 */
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x6000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x4000, 0x403f, MWA_RAM, &spriteram_2 },
	{ 0x4040, 0x43ff, MWA_RAM },
	{ 0x4400, 0x443f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x4440, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, colorram_w, &colorram },
	{ 0x4c00, 0x4fff, videoram_w, &videoram, &videoram_size },
	{ 0x5000, 0x5fff, MWA_RAM },
	{ 0x8000, 0x8000, watchdog_reset_w },
	{ 0x8080, 0x8080, rocnrope_flipscreen_w },
	{ 0x8081, 0x8081, rocnrope_sh_irqtrigger_w },  /* cause interrupt on audio CPU */
	{ 0x8082, 0x8082, MWA_NOP },	/* interrupt acknowledge??? */
	{ 0x8083, 0x8083, MWA_NOP },	/* Coin counter 1 */
	{ 0x8084, 0x8084, MWA_NOP },	/* Coin counter 2 */
	{ 0x8087, 0x8087, interrupt_enable_w },
	{ 0x8100, 0x8100, soundlatch_w },
	{ 0x8182, 0x818d, rocnrope_interrupt_vector_w },
	{ 0x6000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x3000, 0x33ff, MRA_RAM },
	{ 0x4000, 0x4000, AY8910_read_port_0_r },
	{ 0x6000, 0x6000, AY8910_read_port_1_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x3000, 0x33ff, MWA_RAM },
	{ 0x4000, 0x4000, AY8910_write_port_0_w },
	{ 0x5000, 0x5000, AY8910_control_port_0_w },
	{ 0x6000, 0x6000, AY8910_write_port_1_w },
	{ 0x7000, 0x7000, AY8910_control_port_1_w },
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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
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
	PORT_DIPSETTING(    0x00, "Disabled" )
/* 0x00 disables Coin 2. It still accepts coins and makes the sound, but
   it doesn't give you any credit */

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x78, 0x78, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x78, "Easy 1" )
	PORT_DIPSETTING(    0x70, "Easy 2" )
	PORT_DIPSETTING(    0x68, "Easy 3" )
	PORT_DIPSETTING(    0x60, "Easy 4" )
	PORT_DIPSETTING(    0x58, "Normal 1" )
	PORT_DIPSETTING(    0x50, "Normal 2" )
	PORT_DIPSETTING(    0x48, "Normal 3" )
	PORT_DIPSETTING(    0x40, "Normal 4" )
	PORT_DIPSETTING(    0x38, "Normal 5" )
	PORT_DIPSETTING(    0x30, "Normal 6" )
	PORT_DIPSETTING(    0x28, "Normal 7" )
	PORT_DIPSETTING(    0x20, "Normal 8" )
	PORT_DIPSETTING(    0x18, "Difficult 1" )
	PORT_DIPSETTING(    0x10, "Difficult 2" )
	PORT_DIPSETTING(    0x08, "Difficult 3" )
	PORT_DIPSETTING(    0x00, "Difficult 4" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x07, 0x07, "First Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "20000" )
	PORT_DIPSETTING(    0x05, "30000" )
	PORT_DIPSETTING(    0x04, "40000" )
	PORT_DIPSETTING(    0x03, "50000" )
	PORT_DIPSETTING(    0x02, "60000" )
	PORT_DIPSETTING(    0x01, "70000" )
	PORT_DIPSETTING(    0x00, "80000" )
	/* 0x06 gives 20000 */
	PORT_DIPNAME( 0x38, 0x38, "Repeated Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x38, "40000" )
	PORT_DIPSETTING(    0x18, "50000" )
	PORT_DIPSETTING(    0x10, "60000" )
	PORT_DIPSETTING(    0x08, "70000" )
	PORT_DIPSETTING(    0x00, "80000" )
	/* 0x20, 0x28 and 0x30 all gives 40000 */
	PORT_DIPNAME( 0x40, 0x00, "Grant Repeated Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown DSW 8", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 sprites */
	512,	/* 512 characters */
	4,	/* 4 bits per pixel */
	{ 0x2000*8+4, 0x2000*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel */
	{ 256*64*8+4, 256*64*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 16 },
	{ 1, 0x4000, &spritelayout, 16*16, 16 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1789750,	/* 1.78975 MHz ? (same as other Konami games) */
	{ 0x20ff, 0x20ff },
	{ soundlatch_r },
	{ rocnrope_portB_r },
	{ 0 },
	{ 0 }
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
			14318180/4,	/* ???? same as other Konami games */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	rocnrope_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32,16*16+16*16,
	rocnrope_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	rocnrope_vh_screenrefresh,

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

ROM_START( rocnrope_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "rnr_h1.vid",   0x6000, 0x2000, 0x0fddc1f6 )
	ROM_LOAD( "rnr_h2.vid",   0x8000, 0x2000, 0xce9db49a )
	ROM_LOAD( "rnr_h3.vid",   0xA000, 0x2000, 0x6d278459 )
	ROM_LOAD( "rnr_h4.vid",   0xC000, 0x2000, 0x9b2e5f2a )
	ROM_LOAD( "rnr_h5.vid",   0xE000, 0x2000, 0x150a6264 )

	ROM_REGION_DISPOSE(0xc000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "rnr_h12.vid",  0x0000, 0x2000, 0xe2114539 )
	ROM_LOAD( "rnr_h11.vid",  0x2000, 0x2000, 0x169a8f3f )
	ROM_LOAD( "rnr_a11.vid",  0x4000, 0x2000, 0xafdaba5e )
	ROM_LOAD( "rnr_a12.vid",  0x6000, 0x2000, 0x054cafeb )
	ROM_LOAD( "rnr_a9.vid",   0x8000, 0x2000, 0x9d2166b2 )
	ROM_LOAD( "rnr_a10.vid",  0xa000, 0x2000, 0xaff6e22f )

	ROM_REGION(0x0220)    /* color proms */
	ROM_LOAD( "a17_prom.bin", 0x0000, 0x0020, 0x22ad2c3e )
	ROM_LOAD( "b16_prom.bin", 0x0020, 0x0100, 0x750a9677 )
	ROM_LOAD( "rocnrope.pr3", 0x0120, 0x0100, 0xb5c75a27 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "rnr_7a.snd",   0x0000, 0x1000, 0x75d2c4e2 )
	ROM_LOAD( "rnr_8a.snd",   0x1000, 0x1000, 0xca4325ae )
ROM_END

ROM_START( ropeman_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "j01_rm01.bin", 0x6000, 0x2000, 0x6310a1fe )
	ROM_LOAD( "j02_rm02.bin", 0x8000, 0x2000, 0x75af8697 )
	ROM_LOAD( "j03_rm03.bin", 0xA000, 0x2000, 0xb21372b1 )
	ROM_LOAD( "j04_rm04.bin", 0xC000, 0x2000, 0x7acb2a05 )
	ROM_LOAD( "rnr_h5.vid",   0xE000, 0x2000, 0x150a6264 )

	ROM_REGION_DISPOSE(0xc000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "j12_rm07.bin", 0x0000, 0x2000, 0xcd8ac4bf )
	ROM_LOAD( "j11_rm06.bin", 0x2000, 0x2000, 0x35891835 )
	ROM_LOAD( "rnr_a11.vid",  0x4000, 0x2000, 0xafdaba5e )
	ROM_LOAD( "rnr_a12.vid",  0x6000, 0x2000, 0x054cafeb )
	ROM_LOAD( "rnr_a9.vid",   0x8000, 0x2000, 0x9d2166b2 )
	ROM_LOAD( "rnr_a10.vid",  0xa000, 0x2000, 0xaff6e22f )

	ROM_REGION(0x0220)    /* color proms */
	ROM_LOAD( "a17_prom.bin", 0x0000, 0x0020, 0x22ad2c3e )
	ROM_LOAD( "b16_prom.bin", 0x0020, 0x0100, 0x750a9677 )
	ROM_LOAD( "rocnrope.pr3", 0x0120, 0x0100, 0xb5c75a27 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "rnr_7a.snd",   0x0000, 0x1000, 0x75d2c4e2 )
	ROM_LOAD( "rnr_8a.snd",   0x1000, 0x1000, 0xca4325ae )
ROM_END



static void rocnrope_decode(void)
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


	if (memcmp(&RAM[0x5160],"\x01\x00\x00",3) == 0 &&
		memcmp(&RAM[0x50A6],"\x01\x00\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x5160],0x40);
			RAM[0x50A6] = RAM[0x5160];
			RAM[0x50A7] = RAM[0x5161];
			RAM[0x50A8] = RAM[0x5162];
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
		osd_fwrite(f,&RAM[0x5160],0x40);
		osd_fclose(f);
	}
}



struct GameDriver rocnrope_driver =
{
	__FILE__,
	0,
	"rocnrope",
	"Roc'n Rope",
	"1983",
	"Konami + Kosuka",
	"Chris Hardy (MAME driver)\nPaul Swan (color info)",
	0,
	&machine_driver,
	0,

	rocnrope_rom,
	0, rocnrope_decode,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver ropeman_driver =
{
	__FILE__,
	&rocnrope_driver,
	"ropeman",
	"Rope Man",
	"1983",
	"bootleg",
	"Chris Hardy (MAME driver)\nPaul Swan (color info)",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	ropeman_rom,
	0, rocnrope_decode,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};
