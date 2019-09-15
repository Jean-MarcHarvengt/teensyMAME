/***************************************************************************

IronHorse memory map (preliminary)

0000-00ff  Work area
2000-23ff  ColorRAM
2400-27ff  VideoRAM
2800-2fff  RAM
3000-30ff  First sprite bank?
3800-38ff  Second sprite bank?
3f00-3fff  Stack AREA
4000-ffff  ROM

Read:
0b01:	Player 2 controls
0b02:	Player 1 controls
0b03:	Coin + selftest
0a00:	DSW1
0b00:	DSW2
0900:	DSW3

Write:
0003:       Bit 1 selects the 1024 char bank, bit 3 the sprite RAM bank, other bits unknown
0004:       Bit 0 controls NMI, bit 3 controls FIRQ, other bits unknown
0020-003f:  Scroll registers

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809/M6809.h"


extern unsigned char *ironhors_scroll;
static unsigned char *ironhors_interrupt_enable;

void ironhors_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void ironhors_palettebank_w(int offset,int data);
void ironhors_charbank_w(int offset,int data);
void ironhors_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



void ironhors_init_machine(void)
{
	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_S | M6809_FAST_U;
}

int ironhors_interrupt(void)
{
	if (cpu_getiloops() == 0)
	{
		if (*ironhors_interrupt_enable & 4) return M6809_INT_FIRQ;
	}
	else if (cpu_getiloops() % 2)
	{
		if (*ironhors_interrupt_enable & 1) return nmi_interrupt();
	}
	return ignore_interrupt();
}

int ironhors_sh_timer_r(int offset)
{
	int clock;

#define TIMER_RATE 128

	clock = cpu_gettotalcycles() / TIMER_RATE;

	return clock;
}

void ironhors_sh_irqtrigger_w(int offset,int data)
{
	cpu_cause_interrupt(1,0xff);
}



static struct MemoryReadAddress ironhors_readmem[] =
{
	{ 0x0020, 0x003f, MRA_RAM },
	{ 0x0b03, 0x0b03, input_port_0_r },	/* coins + selftest */
	{ 0x0b02, 0x0b02, input_port_1_r },	/* player 1 controls */
	{ 0x0b01, 0x0b01, input_port_2_r },	/* player 2 controls */
	{ 0x0a00, 0x0a00, input_port_3_r },	/* Dipswitch settings 0 */
	{ 0x0b00, 0x0b00, input_port_4_r },	/* Dipswitch settings 1 */
	{ 0x0900, 0x0900, input_port_5_r },	/* Dipswitch settings 2 */
	{ 0x2000, 0x2fff, MRA_RAM },
	{ 0x3000, 0x3fff, MRA_RAM },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress ironhors_writemem[] =
{
	{ 0x0003, 0x0003, ironhors_charbank_w },
	{ 0x0004, 0x0004, MWA_RAM, &ironhors_interrupt_enable },
	{ 0x0020, 0x003f, MWA_RAM, &ironhors_scroll },
	{ 0x0800, 0x0800, soundlatch_w },
	{ 0x0900, 0x0900, ironhors_sh_irqtrigger_w },  /* cause interrupt on audio CPU */
	{ 0x0a00, 0x0a00, ironhors_palettebank_w },
	{ 0x0b00, 0x0b00, watchdog_reset_w },
	{ 0x2000, 0x23ff, colorram_w, &colorram },
	{ 0x2400, 0x27ff, videoram_w, &videoram, &videoram_size },
	{ 0x2800, 0x2fff, MWA_RAM },
	{ 0x3000, 0x30ff, MWA_RAM, &spriteram_2 },
	{ 0x3100, 0x37ff, MWA_RAM },
	{ 0x3800, 0x38ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3900, 0x3fff, MWA_RAM },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress ironhors_sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x8000, 0x8000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress ironhors_sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOReadPort ironhors_sound_readport[] =
{
	{ 0x00, 0x00, ironhors_sh_timer_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort ironhors_sound_writeport[] =
{
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress farwest_sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x8000, 0x8000, soundlatch_r },
	{ -1 }	/* end of table */
};
static struct MemoryWriteAddress farwest_sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x8000, 0x8000, YM2203_control_port_0_w },
	{ 0x8001, 0x8001, YM2203_write_port_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( ironhors_input_ports )
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
	/* note that button 3 for player 1 and 2 are exchanged */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
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
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW1 */
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
/* 	PORT_DIPSETTING(    0x00, "Invalid" ) */

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Controls", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Single" )
	PORT_DIPSETTING(    0x00, "Dual" )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout ironhors_charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 0x8000*8+0*4, 0x8000*8+1*4, 2*4, 3*4, 0x8000*8+2*4, 0x8000*8+3*4 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout ironhors_spritelayout =
{
	16,16,	/* 16*16 sprites */
	512,	/* 512 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },	/* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 0x8000*8+0*4, 0x8000*8+1*4, 2*4, 3*4, 0x8000*8+2*4, 0x8000*8+3*4,
           16*8+0*4, 16*8+1*4, 16*8+0x8000*8+0*4, 16*8+0x8000*8+1*4, 16*8+2*4, 16*8+3*4, 16*8+0x8000*8+2*4, 16*8+0x8000*8+3*4 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
           16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo ironhors_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &ironhors_charlayout,         0, 16*8 },
	{ 1, 0x10000, &ironhors_spritelayout, 16*8*16, 16*8 },
	{ 1, 0x10000, &ironhors_charlayout,   16*8*16, 16*8 },  /* to handle 8x8 sprites */
	{ -1 } /* end of array */
};


static struct GfxLayout farwest_charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	4,	/* 4 bits per pixel */
	{ 0, 2, 4, 6 },	/* the four bitplanes are packed in one byte */
	{ 3*8+1, 3*8+0, 0*8+1, 0*8+0, 1*8+1, 1*8+0, 2*8+1, 2*8+0 },
	{ 0*4*8, 1*4*8, 2*4*8, 3*4*8, 4*4*8, 5*4*8, 6*4*8, 7*4*8 },
	32*8	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout farwest_spritelayout =
{
	16,16,	/* 16*16 sprites */
	512,	/* 512 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 512*32*8, 2*512*32*8, 3*512*32*8 },	/* the four bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout farwest_spritelayout2 =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	4,	/* 4 bits per pixel */
	{ 0, 2048*8*8, 2*2048*8*8, 3*2048*8*8 },	/* the four bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo farwest_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &farwest_charlayout,         0, 16*8 },
	{ 1, 0x10000, &farwest_spritelayout, 16*8*16, 16*8 },
	{ 1, 0x10000, &farwest_spritelayout2,16*8*16, 16*8 },  /* to handle 8x8 sprites */
	{ -1 } /* end of array */
};


static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	1500000,	/* 1.5 MHz ? */
	{ YM2203_VOL(255,255) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static struct MachineDriver ironhors_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			2500000,        /* 2.50 Mhz? */
			0,
			ironhors_readmem,ironhors_writemem,0,0,
			ironhors_interrupt,8
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318180/4,	/* ??  */
			3,	/* memory region #3 */
			ironhors_sound_readmem,ironhors_sound_writemem,ironhors_sound_readport,ironhors_sound_writeport,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	ironhors_init_machine,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 2*8, 30*8-1 },
	ironhors_gfxdecodeinfo,
	256,16*8*16+16*8*16,
	ironhors_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	ironhors_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

static struct MachineDriver farwest_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			2500000,        /* 2.50 Mhz? */
			0,
			ironhors_readmem,ironhors_writemem,0,0,
			ironhors_interrupt,8
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318180/4,	/* ??  */
			3,	/* memory region #3 */
			farwest_sound_readmem,farwest_sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	ironhors_init_machine,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 2*8, 30*8-1 },
	farwest_gfxdecodeinfo,
	256,16*8*16+16*8*16,
	ironhors_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	ironhors_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ironhors_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "13c_h03.bin",  0x4000, 0x8000, 0x24539af1 )
	ROM_LOAD( "12c_h02.bin",  0xc000, 0x4000, 0xfab07f86 )

	ROM_REGION_DISPOSE(0x20000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "09f_h07.bin",  0x00000, 0x8000, 0xc761ec73 )
	ROM_LOAD( "06f_h04.bin",  0x08000, 0x8000, 0xc1486f61 )
	ROM_LOAD( "08f_h06.bin",  0x10000, 0x8000, 0xf21d8c93 )
	ROM_LOAD( "07f_h05.bin",  0x18000, 0x8000, 0x60107859 )

	ROM_REGION(0x500)	/* color/lookup proms */
	ROM_LOAD( "03f_h08.bin",  0x0000, 0x0100, 0x9f6ddf83 ) /* palette red */
	ROM_LOAD( "04f_h09.bin",  0x0100, 0x0100, 0xe6773825 ) /* palette green */
	ROM_LOAD( "05f_h10.bin",  0x0200, 0x0100, 0x30a57860 ) /* palette blue */
	ROM_LOAD( "10f_h12.bin",  0x0300, 0x0100, 0x5eb33e73 ) /* character lookup table */
	ROM_LOAD( "10f_h11.bin",  0x0400, 0x0100, 0xa63e37d8 ) /* sprite lookup table */

	ROM_REGION(0x10000)     /* 64k for audio cpu */
	ROM_LOAD( "10c_h01.bin",  0x0000, 0x4000, 0x2b17930f )
ROM_END

ROM_START( farwest_rom )
	ROM_REGION(0x12000)	/* 64k for code + 8k for extra ROM */
	ROM_LOAD( "ironhors.008", 0x04000, 0x4000, 0xb1c8246c )
	ROM_LOAD( "ironhors.009", 0x08000, 0x8000, 0xea34ecfc )
	ROM_LOAD( "ironhors.007", 0x10000, 0x2000, 0x471182b7 )	/* don't know what this is for */

	ROM_REGION_DISPOSE(0x20000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ironhors.005", 0x00000, 0x8000, 0xf77e5b83 )
	ROM_LOAD( "ironhors.006", 0x08000, 0x8000, 0x7bbc0b51 )
	ROM_LOAD( "ironhors.001", 0x10000, 0x4000, 0xa8fc21d3 )
	ROM_LOAD( "ironhors.002", 0x14000, 0x4000, 0x9c1e5593 )
	ROM_LOAD( "ironhors.003", 0x18000, 0x4000, 0x3a0bf799 )
	ROM_LOAD( "ironhors.004", 0x1c000, 0x4000, 0x1fab18a3 )

	ROM_REGION(0x500)	/* color/lookup proms */
	ROM_LOAD( "ironcol.003",  0x0000, 0x0100, 0x3e3fca11 ) /* palette red */
	ROM_LOAD( "ironcol.001",  0x0100, 0x0100, 0xdfb13014 ) /* palette green */
	ROM_LOAD( "ironcol.002",  0x0200, 0x0100, 0x77c88430 ) /* palette blue */
	ROM_LOAD( "ironcol.004",  0x0300, 0x0100, 0x5eb33e73 ) /* character lookup table */
	ROM_LOAD( "ironcol.005",  0x0400, 0x0100, 0x15077b9c ) /* sprite lookup table */

	ROM_REGION(0x10000)     /* 64k for audio cpu */
	ROM_LOAD( "ironhors.010", 0x0000, 0x4000, 0xa28231a6 )
ROM_END



struct GameDriver ironhors_driver =
{
	__FILE__,
	0,
	"ironhors",
	"Iron Horse",
	"1986",
	"Konami",
	"Mirko Buffoni (MAME driver)\nPaul Swan (color info)",
	0,
	&ironhors_machine_driver,
	0,

	ironhors_rom,
	0, 0,
	0,
	0,

	ironhors_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver farwest_driver =
{
	__FILE__,
	&ironhors_driver,
	"farwest",
	"Far West",
	"1986",
	"bootleg?",
	"Mirko Buffoni (MAME driver)\nGerald Vanderick (color info)",
	GAME_NOT_WORKING,
	&farwest_machine_driver,
	0,

	farwest_rom,
	0, 0,
	0,
	0,

	ironhors_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
