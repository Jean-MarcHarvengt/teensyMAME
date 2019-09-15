/***************************************************************************

Mikie memory map (preliminary)

MAIN BOARD:
2800-288f Sprite RAM (288f, not 287f - quite unusual)
3800-3bff Color RAM
3c00-3fff Video RAM
4000-5fff ROM (?)
5ff0	  Watchdog (?)
6000-ffff ROM


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809/M6809.h"



void mikie_palettebank_w(int offset,int data);
void mikie_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void mikie_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



void mikie_init_machine(void)
{
        /* Set optimization flags for M6809 */
        m6809_Flags = M6809_FAST_S | M6809_FAST_U;
}

int mikie_sh_timer_r(int offset)
{
	int clock;

#define TIMER_RATE 512

	clock = cpu_gettotalcycles() / TIMER_RATE;

	return clock;
}

void mikie_sh_irqtrigger_w(int offset,int data)
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
	{ 0x0000, 0x00ff, MRA_RAM },	/* ???? */
	{ 0x2400, 0x2400, input_port_0_r },	/* coins + selftest */
	{ 0x2401, 0x2401, input_port_1_r },	/* player 1 controls */
	{ 0x2402, 0x2402, input_port_2_r },	/* player 2 controls */
	{ 0x2403, 0x2403, input_port_3_r },	/* flip */
	{ 0x2500, 0x2500, input_port_4_r },	/* Dipswitch settings */
	{ 0x2501, 0x2501, input_port_5_r },	/* Dipswitch settings */
	{ 0x2800, 0x2fff, MRA_RAM },	/* RAM BANK 2 */
	{ 0x3000, 0x37ff, MRA_RAM },	/* RAM BANK 3 */
	{ 0x3800, 0x3fff, MRA_RAM },	/* video RAM */
	{ 0x4000, 0x5fff, MRA_ROM },    /* Machine checks for extra rom */
	{ 0x6000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x2002, 0x2002, mikie_sh_irqtrigger_w },
	{ 0x2007, 0x2007, interrupt_enable_w },
	{ 0x2100, 0x2100, MWA_NOP },		/* Watchdog */
	{ 0x2200, 0x2200, mikie_palettebank_w },
	{ 0x2400, 0x2400, soundlatch_w },
	{ 0x2800, 0x288f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x2890, 0x37ff, MWA_RAM },
	{ 0x3800, 0x3bff, colorram_w, &colorram },
	{ 0x3c00, 0x3fff, videoram_w, &videoram, &videoram_size },
	{ 0x6000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x8003, 0x8003, soundlatch_r },
	{ 0x8005, 0x8005, mikie_sh_timer_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x8000, 0x8000, MWA_NOP },	/* sound command latch */
	{ 0x8001, 0x8001, MWA_NOP },	/* ??? */
	{ 0x8002, 0x8002, SN76496_0_w },	/* trigger read of latch */
	{ 0x8004, 0x8004, SN76496_1_w },	/* trigger read of latch */
	{ 0x8079, 0x8079, MWA_NOP },	/* ??? */
//	{ 0xa003, 0xa003, MWA_RAM },
	{ -1 }
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

	PORT_START
	PORT_DIPNAME( 0x01, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Controls", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Single" )
	PORT_DIPSETTING(    0x02, "Dual" )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
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

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x18, 0x18, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "20000 50000" )
	PORT_DIPSETTING(    0x10, "30000 60000" )
	PORT_DIPSETTING(    0x08, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the bitplanes are packed */
	{ 7*4*8, 6*4*8, 5*4*8, 4*4*8, 3*4*8, 2*4*8, 1*4*8, 0*4*8 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	8*4*8     /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	     /* 16*16 sprites */
	256,	        /* 256 sprites */
	4,	           /* 4 bits per pixel */
	{ 0, 4, 256*128*8+0, 256*128*8+4 },
	{ 39*16, 38*16, 37*16, 36*16, 35*16, 34*16, 33*16, 32*16,
			7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			0, 1, 2, 3, 48*8+0, 48*8+1, 48*8+2, 48*8+3 },
	128*8	/* every sprite takes 64 bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,         0, 16*8 },
	{ 1, 0x4000, &spritelayout, 16*8*16, 16*8 },
	{ 1, 0x4001, &spritelayout, 16*8*16, 16*8 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	1789750,	/* 1.78975 Mhz ??? */
	{ 100, 100 }
};



static struct MachineDriver mikie_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1250000,        /* 1.25 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318180/4,	/* ? */
			3,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	mikie_init_machine,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,16*8*16+16*8*16,
	mikie_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	mikie_vh_screenrefresh,

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


ROM_START( mikie_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "11c_n14.bin",  0x6000, 0x2000, 0xf698e6dd )
	ROM_LOAD( "12a_o13.bin",  0x8000, 0x4000, 0x826e7035 )
	ROM_LOAD( "12d_o17.bin",  0xc000, 0x4000, 0x161c25c8 )

	ROM_REGION_DISPOSE(0x14000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "o11",          0x00000, 0x4000, 0x3c82aaf3 )
	ROM_LOAD( "001",          0x04000, 0x4000, 0xa2ba0df5 )
	ROM_LOAD( "003",          0x08000, 0x4000, 0x9775ab32 )
	ROM_LOAD( "005",          0x0c000, 0x4000, 0xba44aeef )
	ROM_LOAD( "007",          0x10000, 0x4000, 0x31afc153 )

	ROM_REGION(0x0500)	/* color PROMs */
	ROM_LOAD( "01i_d19.bin",  0x0000, 0x0100, 0x8b83e7cf )	/* red component */
	ROM_LOAD( "03i_d21.bin",  0x0100, 0x0100, 0x3556304a )	/* green component */
	ROM_LOAD( "02i_d20.bin",  0x0200, 0x0100, 0x676a0669 )	/* blue component */
	ROM_LOAD( "12h_d22.bin",  0x0300, 0x0100, 0x872be05c )	/* character lookup table */
	ROM_LOAD( "f09_d18.bin",  0x0400, 0x0100, 0x7396b374 )	/* sprite lookup table */

	ROM_REGION(0x10000)
	ROM_LOAD( "06e_n10.bin",  0x0000, 0x2000, 0x2cf9d670 )
ROM_END

ROM_START( mikiej_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "11c_n14.bin",  0x6000, 0x2000, 0xf698e6dd )
	ROM_LOAD( "12a_o13.bin",  0x8000, 0x4000, 0x826e7035 )
	ROM_LOAD( "12d_o17.bin",  0xc000, 0x4000, 0x161c25c8 )

	ROM_REGION_DISPOSE(0x14000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "08i_q11.bin",  0x00000, 0x4000, 0xc48b269b )
	ROM_LOAD( "f01_q01.bin",  0x04000, 0x4000, 0x31551987 )
	ROM_LOAD( "f03_q03.bin",  0x08000, 0x4000, 0x34414df0 )
	ROM_LOAD( "h01_q05.bin",  0x0c000, 0x4000, 0xf9e1ebb1 )
	ROM_LOAD( "h03_q07.bin",  0x10000, 0x4000, 0x15dc093b )

	ROM_REGION(0x0500)	/* color PROMs */
	ROM_LOAD( "01i_d19.bin",  0x0000, 0x0100, 0x8b83e7cf )	/* red component */
	ROM_LOAD( "03i_d21.bin",  0x0100, 0x0100, 0x3556304a )	/* green component */
	ROM_LOAD( "02i_d20.bin",  0x0200, 0x0100, 0x676a0669 )	/* blue component */
	ROM_LOAD( "12h_d22.bin",  0x0300, 0x0100, 0x872be05c )	/* character lookup table */
	ROM_LOAD( "f09_d18.bin",  0x0400, 0x0100, 0x7396b374 )	/* sprite lookup table */

	ROM_REGION(0x10000)
	ROM_LOAD( "06e_n10.bin",  0x0000, 0x2000, 0x2cf9d670 )
ROM_END

ROM_START( mikiehs_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "11c_l14.bin",  0x6000, 0x2000, 0x633f3a6d )
	ROM_LOAD( "12a_m13.bin",  0x8000, 0x4000, 0x9c42d715 )
	ROM_LOAD( "12d_m17.bin",  0xc000, 0x4000, 0xcb5c03c9 )

	ROM_REGION_DISPOSE(0x14000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "08i_l11.bin",  0x00000, 0x4000, 0x5ba9d86b )
	ROM_LOAD( "f01_i01.bin",  0x04000, 0x4000, 0x0c0cab5f )
	ROM_LOAD( "f03_i03.bin",  0x08000, 0x4000, 0x694da32f )
	ROM_LOAD( "h01_i05.bin",  0x0c000, 0x4000, 0x00e357e1 )
	ROM_LOAD( "h03_i07.bin",  0x10000, 0x4000, 0xceeba6ac )

	ROM_REGION(0x0500)	/* color PROMs */
	ROM_LOAD( "01i_d19.bin",  0x0000, 0x0100, 0x8b83e7cf )	/* red component */
	ROM_LOAD( "03i_d21.bin",  0x0100, 0x0100, 0x3556304a )	/* green component */
	ROM_LOAD( "02i_d20.bin",  0x0200, 0x0100, 0x676a0669 )	/* blue component */
	ROM_LOAD( "12h_d22.bin",  0x0300, 0x0100, 0x872be05c )	/* character lookup table */
	ROM_LOAD( "f09_d18.bin",  0x0400, 0x0100, 0x7396b374 )	/* sprite lookup table */

	ROM_REGION(0x10000)
	ROM_LOAD( "06e_h10.bin",  0x0000, 0x2000, 0x4ed887d2 )
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0x2a00],"\x1d\x2c\x1f\x00\x01",5) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{

                        osd_fread(f,&RAM[0x2a00],9*5);

				/* top score display */
                        memcpy(&RAM[0x29f0], &RAM[0x2a05], 4);

				/* 1p score display, which also displays the top score on startup */
				memcpy(&RAM[0x297c], &RAM[0x2a05], 4);

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
                osd_fwrite(f,&RAM[0x2a00],9*5);
		osd_fclose(f);
	}
}



struct GameDriver mikie_driver =
{
	__FILE__,
	0,
	"mikie",
	"Mikie",
	"1984",
	"Konami",
	"Allard Van Der Bas (MAME driver)\nMirko Buffoni (MAME driver)\nStefano Mozzi (MAME driver)\nMarco Cassili (dip switches)\nAl Kossow (color info)",
	0,
	&mikie_machine_driver,
	0,

	mikie_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver mikiej_driver =
{
	__FILE__,
	&mikie_driver,
	"mikiej",
	"Shinnyuushain Tooru-kun",
	"1984",
	"Konami",
	"Allard Van Der Bas (MAME driver)\nMirko Buffoni (MAME driver)\nStefano Mozzi (MAME driver)\nMarco Cassili (dip switches)\nAl Kossow (color info)",
	0,
	&mikie_machine_driver,
	0,

	mikiej_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver mikiehs_driver =
{
	__FILE__,
	&mikie_driver,
	"mikiehs",
	"Mikie (High School Graffiti)",
	"1984",
	"Konami",
	"Allard Van Der Bas (MAME driver)\nMirko Buffoni (MAME driver)\nStefano Mozzi (MAME driver)\nMarco Cassili (dip switches)\nAl Kossow (color info)",
	0,
	&mikie_machine_driver,
	0,

	mikiehs_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
