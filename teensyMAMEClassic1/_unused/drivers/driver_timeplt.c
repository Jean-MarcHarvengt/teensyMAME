/***************************************************************************

Time Pilot memory map (preliminary)

Main processor memory map.
0000-5fff ROM
a000-a3ff Color RAM
a400-a7ff Video RAM
a800-afff RAM
b000-b7ff sprite RAM (only areas 0xb010 and 0xb410 are used).

memory mapped ports:

read:
c000      video scan line. This is used by the program to multiplex the cloud
          sprites, drawing them twice offset by 128 pixels.
c200      DSW2
c300      IN0
c320      IN1
c340      IN2
c360      DSW1

write:
c000      command for the audio CPU
c200      watchdog reset
c300      interrupt enable
c301      ?
c302      flip screen
c303      ?
c304      trigger interrupt on audio CPU
c305-c307 ?

interrupts:
standard NMI at 0x66


SOUND BOARD:
same as Pooyan

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



void timeplt_flipscreen_w(int offset,int data);
int timeplt_scanline_r(int offset);
void timeplt_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void timeplt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/* I am not 100% sure that this timer is correct, but */
/* I'm using the Gyruss wired to the higher 4 bits    */
/* instead of the lower ones, so there is a good      */
/* chance it's the right one. */
/* I had to change one value in _timer to avoid the */
/* tempo being twice what it should be. */

/* The timer clock which feeds the lower 4 bits of    */
/* AY-3-8910 port A is based on the same clock        */
/* feeding the sound CPU Z80.  It is a divide by      */
/* 10240, formed by a standard divide by 1024,        */
/* followed by a divide by 10 using a 4 bit           */
/* bi-quinary count sequence. (See LS90 data sheet    */
/* for an example).                                   */
/* Bits 1-3 come directly from the upper three bits   */
/* of the bi-quinary counter. Bit 0 comes from the    */
/* output of the divide by 1024.                      */

static int timeplt_timer[20] = {
0x00, 0x01, 0x02, 0x01, 0x02, 0x03, 0x02, 0x03, 0x04, 0x05,
/*            ^^ changed this to make tempo correct */
/* the code waits until (timer & 0xf0) == 0 so we must return 0 only once */
0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b, 0x0c, 0x0d
};

static int timeplt_portB_r(int offset)
{
	/* need to protect from totalcycles overflow */
	static int last_totalcycles = 0;

	/* number of Z80 clock cycles to count */
	static int clock;

	int current_totalcycles;

	current_totalcycles = cpu_gettotalcycles();
	clock = (clock + (current_totalcycles-last_totalcycles)) % 10240;

	last_totalcycles = current_totalcycles;

	return timeplt_timer[clock/512] << 4;
}

void timeplt_sh_irqtrigger_w(int offset,int data)
{
	static int last;


	if (last == 1 && data == 0)
	{
		/* setting bit 0 high then low triggers IRQ on the sound CPU */
		cpu_cause_interrupt(1,0xff);
	}

	last = data;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0xa000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xc000, timeplt_scanline_r },
	{ 0xc200, 0xc200, input_port_4_r },	/* DSW2 */
	{ 0xc300, 0xc300, input_port_0_r },	/* IN0 */
	{ 0xc320, 0xc320, input_port_1_r },	/* IN1 */
	{ 0xc340, 0xc340, input_port_2_r },	/* IN2 */
	{ 0xc360, 0xc360, input_port_3_r },	/* DSW1 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0xa000, 0xa3ff, colorram_w, &colorram },
	{ 0xa400, 0xa7ff, videoram_w, &videoram, &videoram_size },
	{ 0xa800, 0xafff, MWA_RAM },
	{ 0xb010, 0xb03f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xb410, 0xb43f, MWA_RAM, &spriteram_2 },
	{ 0xc000, 0xc000, soundlatch_w },
	{ 0xc200, 0xc200, MWA_NOP },
	{ 0xc300, 0xc300, interrupt_enable_w },
	{ 0xc302, 0xc302, timeplt_flipscreen_w },
	{ 0xc304, 0xc304, timeplt_sh_irqtrigger_w },
	{ -1 }	/* end of table */
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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin 1", IP_KEY_NONE )
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
	PORT_DIPNAME( 0xf0, 0xf0, "Coin 2", IP_KEY_NONE )
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
	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x08, 0x08, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "10000 50000" )
	PORT_DIPSETTING(    0x00, "20000 60000" )
	PORT_DIPNAME( 0x70, 0x70, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x70, "1 (Easiest)" )
	PORT_DIPSETTING(    0x60, "2" )
	PORT_DIPSETTING(    0x50, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x30, "5 (Average)" )
	PORT_DIPSETTING(    0x20, "6" )
	PORT_DIPSETTING(    0x10, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hardest)" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3, 8*8+0,8*8+1,8*8+2,8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3,  8*8, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3,  24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,        0, 32 },
	{ 1, 0x2000, &spritelayout,   32*4, 64 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1789750,	/* 1.78975 MHz ? (same as other Konami games) */
	{ 0x20ff, 0x20ff },
	{ soundlatch_r },
	{ timeplt_portB_r },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz (?) */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
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
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32,32*4+64*4,
	timeplt_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	timeplt_vh_screenrefresh,

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

ROM_START( timeplt_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "tm1",          0x0000, 0x2000, 0x1551f1b9 )
	ROM_LOAD( "tm2",          0x2000, 0x2000, 0x58636cb5 )
	ROM_LOAD( "tm3",          0x4000, 0x2000, 0xff4e0d83 )

	ROM_REGION_DISPOSE(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tm6",          0x0000, 0x2000, 0xc2507f40 )
	ROM_LOAD( "tm4",          0x2000, 0x2000, 0x7e437c3e )
	ROM_LOAD( "tm5",          0x4000, 0x2000, 0xe8ca87b9 )

	ROM_REGION(0x0240)	/* color proms */
	ROM_LOAD( "timeplt.b4",   0x0000, 0x0020, 0x34c91839 ) /* palette */
	ROM_LOAD( "timeplt.b5",   0x0020, 0x0020, 0x463b2b07 ) /* palette */
	ROM_LOAD( "timeplt.e9",   0x0040, 0x0100, 0x4bbb2150 ) /* sprite lookup table */
	ROM_LOAD( "timeplt.e12",  0x0140, 0x0100, 0xf7b7663e ) /* char lookup table */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "tm7",          0x0000, 0x1000, 0xd66da813 )
ROM_END

ROM_START( timepltc_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "cd1y",         0x0000, 0x2000, 0x83ec72c2 )
	ROM_LOAD( "cd2y",         0x2000, 0x2000, 0x0dcf5287 )
	ROM_LOAD( "cd3y",         0x4000, 0x2000, 0xc789b912 )

	ROM_REGION_DISPOSE(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tm6",          0x0000, 0x2000, 0xc2507f40 )
	ROM_LOAD( "tm4",          0x2000, 0x2000, 0x7e437c3e )
	ROM_LOAD( "tm5",          0x4000, 0x2000, 0xe8ca87b9 )

	ROM_REGION(0x0240)	/* color proms */
	ROM_LOAD( "timeplt.b4",   0x0000, 0x0020, 0x34c91839 ) /* palette */
	ROM_LOAD( "timeplt.b5",   0x0020, 0x0020, 0x463b2b07 ) /* palette */
	ROM_LOAD( "timeplt.e9",   0x0040, 0x0100, 0x4bbb2150 ) /* sprite lookup table */
	ROM_LOAD( "timeplt.e12",  0x0140, 0x0100, 0xf7b7663e ) /* char lookup table */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "tm7",          0x0000, 0x1000, 0xd66da813 )
ROM_END

ROM_START( spaceplt_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "sp1",          0x0000, 0x2000, 0xac8ca3ae )
	ROM_LOAD( "sp2",          0x2000, 0x2000, 0x1f0308ef )
	ROM_LOAD( "sp3",          0x4000, 0x2000, 0x90aeca50 )

	ROM_REGION_DISPOSE(0x6000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sp6",          0x0000, 0x2000, 0x76caa8af )
	ROM_LOAD( "sp4",          0x2000, 0x2000, 0x3781ce7a )
	ROM_LOAD( "tm5",          0x4000, 0x2000, 0xe8ca87b9 )

	ROM_REGION(0x0240)	/* color proms */
	ROM_LOAD( "timeplt.b4",   0x0000, 0x0020, 0x34c91839 ) /* palette */
	ROM_LOAD( "timeplt.b5",   0x0020, 0x0020, 0x463b2b07 ) /* palette */
	ROM_LOAD( "timeplt.e9",   0x0040, 0x0100, 0x4bbb2150 ) /* sprite lookup table */
	ROM_LOAD( "timeplt.e12",  0x0140, 0x0100, 0xf7b7663e ) /* char lookup table */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "tm7",          0x0000, 0x1000, 0xd66da813 )
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xab09],"\x00\x00\x01",3) == 0 &&
	    memcmp(&RAM[0xab29],"\x00\x43\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xab08],8*5);
			RAM[0xa98b] = RAM[0xab09];
			RAM[0xa98c] = RAM[0xab0a];
			RAM[0xa98d] = RAM[0xab0b];
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
		osd_fwrite(f,&RAM[0xab08],8*5);
		osd_fclose(f);
	}
}



struct GameDriver timeplt_driver =
{
	__FILE__,
	0,
	"timeplt",
	"Time Pilot",
	"1982",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlan J McCormick (color info)\nPaul Swan (color info)\nMike Cuddy (clouds info)\nEdward Massey (clouds info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	timeplt_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver timepltc_driver =
{
	__FILE__,
	&timeplt_driver,
	"timepltc",
	"Time Pilot (Centuri)",
	"1982",
	"Konami (Centuri license)",
	"Nicola Salmoria (MAME driver)\nAlan J McCormick (color info)\nPaul Swan (color info)\nMike Cuddy (clouds info)\nEdward Massey (clouds info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	timepltc_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver spaceplt_driver =
{
	__FILE__,
	&timeplt_driver,
	"spaceplt",
	"Space Pilot",
	"1982",
	"bootleg",
	"Nicola Salmoria (MAME driver)\nAlan J McCormick (color info)\nPaul Swan (color info)\nMike Cuddy (clouds info)\nEdward Massey (clouds info)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	spaceplt_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};
