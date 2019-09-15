#include "driver.h"
#include "vidhrdw/generic.h"


void pingpong_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void pingpong_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

static unsigned char *intenable;


static int pingpong_interrupt(void)
{
	if (cpu_getiloops() == 0)
	{
		if (*intenable & 0x04) return interrupt();
	}
	else if (cpu_getiloops() % 2)
	{
		if (*intenable & 0x08) return nmi_interrupt();
	}

	return ignore_interrupt();
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7FFF, MRA_ROM },
	{ 0x8000, 0x87FF, MRA_RAM },
	{ 0x9000, 0x97FF, MRA_RAM },
	{ 0xA800, 0xA800, input_port_0_r },
	{ 0xA880, 0xA880, input_port_1_r },
	{ 0xA900, 0xA900, input_port_2_r },
	{ 0xA980, 0xA980, input_port_3_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7FFF, MWA_ROM },
	{ 0x8000, 0x83FF, colorram_w, &colorram },
	{ 0x8400, 0x87FF, videoram_w, &videoram, &videoram_size },
	{ 0x9000, 0x9002, MWA_RAM },
	{ 0x9003, 0x9052, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9053, 0x97FF, MWA_RAM },
	{ 0xA000, 0xA000, MWA_RAM, &intenable },	/* bit 2 = irq enable, bit 3 = nmi enable */
												/* bit 0/1 = coin counters */
												/* other bits unknown */
	{ 0xA200, 0xA200, MWA_NOP },		/* SN76496 data latch */
	{ 0xA400, 0xA400, SN76496_0_w },	/* trigger read */
	{ 0xA600, 0xA600, watchdog_reset_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX( 0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING( 0x00, "On" )
	PORT_DIPSETTING( 0x04, "Off" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT| IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0F, 0x0F, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0A, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x08, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0F, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x0C, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x0E, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0B, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0D, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0xF0, 0xF0, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0xA0, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x80, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xF0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0xC0, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0xE0, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x70, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0xB0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xD0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x50, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x06, 0x06, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x04, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,		/* 8*8 characters */
	512,		/* 512 characters */
	2,		/* 2 bits per pixel */
	{ 4, 0 },	/* the bitplanes are packed in one nibble */
	{ 3, 2, 1, 0, 8*8+3, 8*8+2, 8*8+1, 8*8+0 },	/* x bit */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8   },     /* y bit */
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,		/* 16*16 sprites */
	128,		/* 128 sprites */
	2,		/* 2 bits per pixel */
	{ 4, 0 },	/* the bitplanes are packed in one nibble */
	{ 12*16+3,12*16+2,12*16+1,12*16+0,
	   8*16+3, 8*16+2, 8*16+1, 8*16+0,
	   4*16+3, 4*16+2, 4*16+1, 4*16+0,
	        3,      2,      1,      0 },			/* x bit */
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
	  32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8  },    /* y bit */
	64*8	/* every char takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,         0, 64 },
	{ 1, 0x2000, &spritelayout,    64*4, 64 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	1,			/* 1 chip */
	18432000/8,	/* 2.304 MHz */
	{ 100 }
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,		/* 3.072 MHz (probably) */
			0,
			readmem,writemem,0,0,
			pingpong_interrupt,16	/* 1 IRQ + 8 NMI */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,
	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32,64*4+64*4,
	pingpong_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	pingpong_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,

	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};


static int hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x94B1],"\x02\x00\x00",3) == 0 &&
	    memcmp(&RAM[0x94cc],"\x00\x20\x00",3) == 0 &&
	    memcmp(&RAM[0x949e],"\x02\x00\x00",3) == 0 &&	/* high score */
	    memcmp(&RAM[0x846E],"\x02\x00\x00",3) == 0)	/* high score in video RAM */
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x94B1],6*10);

			RAM[0x949E] = RAM[0x94B1];
			RAM[0x949F] = RAM[0x94B2];
			RAM[0x94A0] = RAM[0x94B3];

			RAM[0x846D] = (RAM[0x94B1]>>4) ? RAM[0x94B1]>>4 : 0x10;
			RAM[0x846E] = RAM[0x94B1] & 0xF;
			RAM[0x846F] = RAM[0x94B2] >> 4;
			RAM[0x8470] = RAM[0x94B2] & 0xF;
			RAM[0x8471] = RAM[0x94B3] >> 4;
			RAM[0x8472] = RAM[0x94B3] & 0xF;

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
		osd_fwrite(f,&RAM[0x94B1],6*10);
		osd_fclose(f);
	}
}



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( pingpong_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pp_e04.rom",   0x0000, 0x4000, 0x18552f8f )
	ROM_LOAD( "pp_e03.rom",   0x4000, 0x4000, 0xae5f01e8 )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pp_e01.rom",   0x0000, 0x2000, 0xd1d6f090 )
	ROM_LOAD( "pp_e02.rom",   0x2000, 0x2000, 0x33c687e0 )

	ROM_REGION(0x0220)	/* color proms */
	ROM_LOAD( "pingpong.3j",  0x0000, 0x0020, 0x3e04f06e ) /* palette (this might be bad) */
	ROM_LOAD( "pingpong.11j", 0x0020, 0x0100, 0x09d96b08 ) /* sprites */
	ROM_LOAD( "pingpong.5h",  0x0120, 0x0100, 0x8456046a ) /* characters */
ROM_END



struct GameDriver pingpong_driver =
{
	__FILE__,
	0,
	"pingpong",
	"Ping Pong",
	"1985",
	"Konami",
	"Jarek Parchanski (MAME driver)\nMartin Binder (color info)",
	0,
	&machine_driver,
	0,

	pingpong_rom,
	0, 0,
	0,
	0,	/* sound_prom */
	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
