/***************************************************************************

Exciting Soccer - (c) 1983 Alpha Denshi Co.

Supported sets:
Exciting Soccer - Alpha Denshi
Exciting Soccer (bootleg) - Kazutomi


Preliminary driver by:
Ernesto Corvi
ernesto@imagina.com

Jarek Parchanski
jpdev@friko6.onet.pl


NOTES:
The game supports Coin 2, but the dip switches used for it are the same
as Coin 1. Basically, this allowed to select an alternative coin table
based on wich Coin input was connected.

KNOWN ISSUES/TODO:
- Cocktail mode is unsupported.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* from vidhrdw */
extern void exctsccr_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern void exctsccr_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern void exctsccr_gfx_bank_w( int offset, int data );
extern int exctsccr_vh_start( void );
extern void exctsccr_vh_stop( void );

/* from machine */
extern unsigned char *exctsccr_mcu_ram;
extern void exctsccr_mcu_w( int offs, int data );
extern void exctsccr_mcu_control_w( int offs, int data );


void exctsccr_DAC_data_w(int offset,int data)
{
	DAC_signed_data_w(offset,data << 2);
}


/***************************************************************************

	Memory definition(s)

***************************************************************************/

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x63ff, MRA_RAM }, /* Alpha mcu (protection) */
	{ 0x7c00, 0x7fff, MRA_RAM }, /* work ram */
	{ 0x8000, 0x83ff, videoram_r },
	{ 0x8400, 0x87ff, colorram_r },
	{ 0x8800, 0x8bff, MRA_RAM }, /* ??? */
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa040, 0xa040, input_port_1_r },
	{ 0xa080, 0xa080, input_port_3_r },
	{ 0xa0c0, 0xa0c0, input_port_2_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x63ff, exctsccr_mcu_w, &exctsccr_mcu_ram }, /* Alpha mcu (protection) */
	{ 0x7c00, 0x7fff, MWA_RAM }, /* work ram */
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, colorram_w, &colorram },
	{ 0x8800, 0x8bff, MWA_RAM }, /* ??? */
	{ 0xa000, 0xa000, MWA_NOP }, /* ??? */
	{ 0xa001, 0xa001, MWA_NOP }, /* ??? */
	{ 0xa002, 0xa002, exctsccr_gfx_bank_w },
	{ 0xa003, 0xa003, MWA_NOP }, /* Cocktail mode ( 0xff = flip screen, 0x00 = normal ) */
	{ 0xa006, 0xa006, exctsccr_mcu_control_w }, /* MCU control */
	{ 0xa007, 0xa007, MWA_NOP }, /* This is also MCU control, but i dont need it */
	{ 0xa040, 0xa06f, MWA_RAM, &spriteram }, /* Sprite pos */
	{ 0xa080, 0xa080, soundlatch_w },
	{ 0xa0c0, 0xa0c0, watchdog_reset_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x8fff, MRA_ROM },
	{ 0xa000, 0xa7ff, MRA_RAM },
	{ 0xc00d, 0xc00d, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x8fff, MWA_ROM },
	{ 0xa000, 0xa7ff, MWA_RAM },
	{ 0xc008, 0xc009, exctsccr_DAC_data_w },
	{ 0xc00c, 0xc00c, soundlatch_w }, /* used to clear the latch */
	{ 0xc00f, 0xc00f, MWA_NOP }, /* ??? */
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
    { 0x82, 0x82, AY8910_write_port_0_w },
    { 0x83, 0x83, AY8910_control_port_0_w },
    { 0x86, 0x86, AY8910_write_port_1_w },
    { 0x87, 0x87, AY8910_control_port_1_w },
    { 0x8a, 0x8a, AY8910_write_port_2_w },
    { 0x8b, 0x8b, AY8910_control_port_2_w },
    { 0x8e, 0x8e, AY8910_write_port_3_w },
    { 0x8f, 0x8f, AY8910_control_port_3_w },
	{ -1 }	/* end of table */
};

/* Bootleg */
static struct MemoryReadAddress bl_readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x8000, 0x83ff, videoram_r },
	{ 0x8400, 0x87ff, colorram_r },
	{ 0x8800, 0x8fff, MRA_RAM }, /* ??? */
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa040, 0xa040, input_port_1_r },
	{ 0xa080, 0xa080, input_port_3_r },
	{ 0xa0c0, 0xa0c0, input_port_2_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress bl_writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x7000, 0x7000, AY8910_write_port_0_w },
	{ 0x7001, 0x7001, AY8910_control_port_0_w },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, colorram_w, &colorram },
	{ 0x8800, 0x8fff, MWA_RAM }, /* ??? */
	{ 0xa000, 0xa000, MWA_NOP }, /* ??? */
	{ 0xa001, 0xa001, MWA_NOP }, /* ??? */
	{ 0xa002, 0xa002, exctsccr_gfx_bank_w }, /* ??? */
	{ 0xa003, 0xa003, MWA_NOP }, /* Cocktail mode ( 0xff = flip screen, 0x00 = normal ) */
	{ 0xa006, 0xa006, MWA_NOP }, /* no MCU, but some leftover code still writes here */
	{ 0xa007, 0xa007, MWA_NOP }, /* no MCU, but some leftover code still writes here */
	{ 0xa040, 0xa06f, MWA_RAM, &spriteram }, /* Sprite Pos */
	{ 0xa080, 0xa080, soundlatch_w },
	{ 0xa0c0, 0xa0c0, watchdog_reset_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress bl_sound_readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0xe000, 0xe3ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress bl_sound_writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x8000, 0x8000, MWA_NOP }, /* 0 = DAC sound off, 1 = DAC sound on */
	{ 0xa000, 0xa000, soundlatch_w }, /* used to clear the latch */
	{ 0xc000, 0xc000, exctsccr_DAC_data_w },
	{ 0xe000, 0xe3ff, MWA_RAM },
	{ -1 }	/* end of table */
};

/***************************************************************************

	Input port(s)

***************************************************************************/

INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT| IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	/* The next two overlap */
	PORT_DIPNAME( 0x03, 0x03, "Coinage(Coin A)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x03, 0x03, "Coinage(Coin B)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x04, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Table" )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPNAME( 0x10, 0x10, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x60, 0x00, "Game Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "3 Min." )
	PORT_DIPSETTING(    0x40, "4 Min." )
	PORT_DIPSETTING(    0x20, "1 Min." )
	PORT_DIPSETTING(    0x00, "2 Min." )
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Has to be 0 */
INPUT_PORTS_END

/***************************************************************************

	Graphic(s) decoding

***************************************************************************/

static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	3,		/* 3 bits per pixel */
	{ 0x4000*8+4, 0, 4 },	/* plane offset */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	3,		/* 3 bits per pixel */
	{ 0x2000*8, 0, 4 },	/* plane offset */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout1 =
{
	16,16,	    /* 16*16 sprites */
	64,	        /* 64 sprites */
	3,	        /* 3 bits per pixel */
	{ 0x4000*8+4, 0, 4 },	/* plane offset */
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3  },
	{ 0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8,
			32 * 8, 33 * 8, 34 * 8, 35 * 8, 36 * 8, 37 * 8, 38 * 8, 39 * 8 },
	64*8	/* every sprite takes 64 bytes */
};

static struct GfxLayout spritelayout2 =
{
	16,16,	    /* 16*16 sprites */
	64,         /* 64 sprites */
	3,	        /* 3 bits per pixel */
	{ 0x2000*8, 0, 4 },	/* plane offset */
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3  },
	{ 0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8,
			32 * 8, 33 * 8, 34 * 8, 35 * 8, 36 * 8, 37 * 8, 38 * 8, 39 * 8 },
	64*8	/* every sprite takes 64 bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,		/* 16*16 sprites */
	64,	    	/* 64 sprites */
	3,	    	/* 2 bits per pixel */
	{ 0x1000*8+4, 0, 4 },	/* plane offset */
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3  },
	{ 0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8,
			32 * 8, 33 * 8, 34 * 8, 35 * 8, 36 * 8, 37 * 8, 38 * 8, 39 * 8 },
	64*8	/* every sprite takes 64 bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout1,      0, 32 }, /* chars */
	{ 1, 0x2000, &charlayout2,      0, 32 }, /* chars */
	{ 1, 0x1000, &spritelayout1,    16*8, 32 }, /* sprites */
	{ 1, 0x3000, &spritelayout2,    16*8, 32 }, /* sprites */
	{ 1, 0x6000, &spritelayout,    	16*8, 32 }, /* sprites */
	{ -1 } /* end of array */
};

/***************************************************************************

	Sound interface(s)

***************************************************************************/

static struct AY8910interface ay8910_interface =
{
	4,	/* 4 chips */
	1500000,	/* 1.5 MHz ? */
	{ 15, 15, 15, 15 }, /* volume */
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 }, /* it writes 0s thru port A, no clue what for */
	{ 0, 0, 0, 0 }
};

static struct DACinterface dac_interface =
{
	2,
	{ 50, 50 }
};

/* Bootleg */
static struct AY8910interface bl_ay8910_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz ? */
	{ 50 }, /* volume */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct DACinterface bl_dac_interface =
{
	1,
	{ 100 }
};

/***************************************************************************

	Machine driver(s)

***************************************************************************/

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			3,
			sound_readmem,sound_writemem,0,sound_writeport,
			ignore_interrupt,0,
			nmi_interrupt, 4000 /* 4 khz, updates the dac */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32, 64*8,
	exctsccr_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	exctsccr_vh_start,
	exctsccr_vh_stop,
	exctsccr_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

/* Bootleg */
static struct MachineDriver bl_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4.0 Mhz (?) */
			0,
			bl_readmem,bl_writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz ? */
			3,
			bl_sound_readmem,bl_sound_writemem,0,0,
			ignore_interrupt,0
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32, 64*8,
	exctsccr_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	exctsccr_vh_stop,
	exctsccr_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_AY8910,
			&bl_ay8910_interface
		},
		{
			SOUND_DAC,
			&bl_dac_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( exctsccr_rom )
    ROM_REGION(0x10000)     /* 64k for code */
    ROM_LOAD( "1_g10.bin", 0x0000, 0x2000, 0xaa68df66 )
    ROM_LOAD( "2_h10.bin", 0x2000, 0x2000, 0x2d8f8326 )
    ROM_LOAD( "3_j10.bin", 0x4000, 0x2000, 0xdce4a04d )

    ROM_REGION(0x08000)
    ROM_LOAD( "4_a5.bin", 0x0000, 0x2000, 0xc342229b )
    ROM_LOAD( "5_b5.bin", 0x2000, 0x2000, 0x35f4f8c9 )
    ROM_LOAD( "6_c5.bin", 0x4000, 0x2000, 0xeda40e32 )
    ROM_LOAD( "2_k5.bin", 0x6000, 0x1000, 0x7f9cace2 )
    ROM_LOAD( "3_l5.bin", 0x7000, 0x1000, 0xdb2d9e0d )

	ROM_REGION(0x220)	/* color proms */
	ROM_LOAD( "Prom1.e1", 0x000, 0x020, 0xd9b10bf0 ) /* palette */
	ROM_LOAD( "Prom2.8r", 0x020, 0x100, 0x8a9c0edf ) /* lookup table */
	ROM_LOAD( "Prom3.k5", 0x120, 0x100, 0xb5db1c2c ) /* lookup table */

    ROM_REGION(0x10000)     /* 64k for code */
    ROM_LOAD( "0_h6.bin", 0x0000, 0x2000, 0x3babbd6b )
    ROM_LOAD( "9_f6.bin", 0x2000, 0x2000, 0x639998f5 )
    ROM_LOAD( "8_d6.bin", 0x4000, 0x2000, 0x88651ee1 )
    ROM_LOAD( "7_c6.bin", 0x6000, 0x2000, 0x6d51521e )
    ROM_LOAD( "1_a6.bin", 0x8000, 0x1000, 0x20f2207e )
ROM_END

/* Bootleg */
ROM_START( exctsccb_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ES-1.E2", 0x0000, 0x2000, 0x997c6a82 )
	ROM_LOAD( "ES-2.G2", 0x2000, 0x2000, 0x5c66e792 )
	ROM_LOAD( "ES-3.H2", 0x4000, 0x2000, 0xe0d504c0 )

	ROM_REGION(0x8000)	/* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "4_a5.bin", 0x0000, 0x2000, 0xc342229b ) /* ES-4.E5 */
    ROM_LOAD( "5_b5.bin", 0x2000, 0x2000, 0x35f4f8c9 ) /* ES-5.5G */
    ROM_LOAD( "6_c5.bin", 0x4000, 0x2000, 0xeda40e32 ) /* ES-6.S  */
	ROM_LOAD( "2_k5.bin", 0x6000, 0x1000, 0x7f9cace2 ) /* ES-7.S - bad read on the bootleg set */
	ROM_LOAD( "3_l5.bin", 0x7000, 0x1000, 0xdb2d9e0d ) /* ES-8.S - bad read on the bootleg set */

	ROM_REGION(0x220)	/* color proms */
	ROM_LOAD( "Prom1.e1", 0x000, 0x020, 0xd9b10bf0 ) /* palette */
	ROM_LOAD( "Prom2.8r", 0x020, 0x100, 0x8a9c0edf ) /* lookup table */
	ROM_LOAD( "Prom3.k5", 0x120, 0x100, 0xb5db1c2c ) /* lookup table */

	ROM_REGION(0x10000)	/* sound */
	ROM_LOAD( "ES-A.K2", 0x0000, 0x2000, 0x99e87b78 )
	ROM_LOAD( "ES-B.L2", 0x2000, 0x2000, 0x8b3db794 )
	ROM_LOAD( "ES-C.M2", 0x4000, 0x2000, 0x7bed2f81 )
ROM_END


static int hiload_es(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if (memcmp(&RAM[0x7c60],"\x02\x00\x00",3) == 0)
	{
		void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
		if (f)
		{
			osd_fread(f,&RAM[0x7c90],48);
			osd_fclose(f);

			/* Copy the high score to the work ram as well */

			RAM[0x7c60] = RAM[0x7c93];
			RAM[0x7c61] = RAM[0x7c94];
			RAM[0x7c62] = RAM[0x7c95];
		}
		return 1;
	}
	return 0;  /* we can't load the hi scores yet */
}

static void hisave_es(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */

	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1);

	if (f)
	{
		osd_fwrite(f,&RAM[0x7c90],48);
		osd_fclose(f);
	}
}

static int hiload_esb(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if (memcmp(&RAM[0x8c60],"\x02\x00\x00",3) == 0)
	{
		void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
		if (f)
		{
			osd_fread(f,&RAM[0x8c90],48);
			osd_fclose(f);

			/* Copy the high score to the work ram as well */

			RAM[0x8c60] = RAM[0x8c93];
			RAM[0x8c61] = RAM[0x8c94];
			RAM[0x8c62] = RAM[0x8c95];
		}
		return 1;
	}
	return 0;  /* we can't load the hi scores yet */
}

static void hisave_esb(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */

	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1);

	if (f)
	{
		osd_fwrite(f,&RAM[0x8c90],48);
		osd_fclose(f);
	}
}




struct GameDriver exctsccr_driver =
{
	__FILE__,
	0,
	"exctsccr",
	"Exciting Soccer",
	"1983",
	"Alpha Denshi Co.",
	"Ernesto Corvi\nJarek Parchanski\nDedicated to Paolo Nicoletti",
	0,
	&machine_driver,
	0,
	exctsccr_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload_es, hisave_es
};

/* Bootleg */
struct GameDriver exctsccb_driver =
{
	__FILE__,
	&exctsccr_driver,
	"exctsccb",
	"Exciting Soccer (bootleg)",
	"1984",
	"Kazutomi",
	"Ernesto Corvi\nJarek Parchanski\n\nDedicated to Paolo Nicoletti",
	0,
	&bl_machine_driver,
	0,
	exctsccb_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,
	hiload_esb, hisave_esb
};
