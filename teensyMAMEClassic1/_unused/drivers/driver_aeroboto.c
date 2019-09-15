/****************************************************************************

Aeroboto (Preliminary)

This is the Williams license of Jaleco's Formation-Z.

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *aeroboto_videoram;
extern unsigned char *aeroboto_fgscroll,*aeroboto_bgscroll;
extern unsigned char *aeroboto_charlookup;

void aeroboto_gfxctrl_w(int ofset,int data);
void aeroboto_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static int pip(int offset)
{
	return rand();
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x183f, MRA_RAM },
	{ 0x2000, 0x20ff, MRA_RAM },
	{ 0x2800, 0x28ff, MRA_RAM },
	{ 0x3000, 0x3000, input_port_0_r },
	{ 0x3001, 0x3001, input_port_1_r },
	{ 0x3002, 0x3002, input_port_2_r },
	{ 0x3800, 0x3800, watchdog_reset_r },	/* or IRQ acknowledge */
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x13ff, MWA_RAM, &aeroboto_videoram },
	{ 0x1400, 0x17ff, videoram_w, &videoram, &videoram_size },
	{ 0x1800, 0x181f, MWA_RAM, &aeroboto_fgscroll },
	{ 0x1820, 0x183f, MWA_RAM, &aeroboto_bgscroll },
//	{ 0x2000, 0x20ff, MWA_RAM, &aeroboto_charlookup },
	{ 0x2800, 0x28ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3000, 0x3000, aeroboto_gfxctrl_w },	/* char bank + ? */
	{ 0x3001, 0x3001, soundlatch_w },	/* ? */
	{ 0x3002, 0x3002, soundlatch2_w },	/* ? */
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress readmem_sound[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x9002, 0x9002, AY8910_read_port_0_r },
	{ 0xf000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_sound[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x9000, 0x9000, AY8910_control_port_0_w },
	{ 0x9001, 0x9001, AY8910_write_port_0_w },
	{ 0xa000, 0xa000, AY8910_control_port_1_w },
	{ 0xa001, 0xa001, AY8910_write_port_1_w },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x03, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x04, 0x00, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright?" )
	PORT_DIPSETTING(    0x40, "Cocktail?" )
	/* the coin input must stay low for exactly 2 frames to be consistently recognized. */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE, "Coin A", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2 )

	PORT_START
	PORT_DIPNAME( 0x07, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x06, "1 Coin/4 Credits" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 7", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Flip Screen?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off?" )
	PORT_DIPSETTING(    0x80, "On?" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3, 512*8*8+0, 512*8*8+1, 512*8*8+2, 512*8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	8,16,	/* 8*16 sprites */
	256,	/* 128 sprites */
	3,	/* 3 bits per pixel */
	{ 2*256*16*8, 256*16*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
            8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 64 },	/* chars */
	{ 1, 0x2000, &charlayout,     0, 64 },	/* sky */
	{ 1, 0x4000, &spritelayout,   0, 32 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,      /* 2 chips */
	1500000,        /* 1.5 MHz ? (hand tuned) */
	{ 255, 255 },
	{ soundlatch_r, 0 },	/* ? */
	{ soundlatch2_r, 0 },	/* ? */
	{ 0, 0 },
	{ 0, 0 }
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1250000,        /* 1.25 Mhz ? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ? */
			2,
			readmem_sound,writemem_sound,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,	/* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,256,
	0,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	aeroboto_vh_screenrefresh,

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

ROM_START( formatz_rom )
	ROM_REGION(0x10000)     /* 64k for main CPU */
	ROM_LOAD( "format_z.8",   0x4000, 0x4000, 0x81a2416c )
	ROM_LOAD( "format_z.7",   0x8000, 0x4000, 0x986e6052 )
	ROM_LOAD( "format_z.6",   0xc000, 0x4000, 0xbaa0d745 )

	ROM_REGION_DISPOSE(0x7000)	/* temporary space for graphics */
	ROM_LOAD( "format_z.5",   0x0000, 0x2000, 0xba50be57 )	/* characters */
	ROM_LOAD( "format_z.4",   0x2000, 0x2000, 0x910375a0 )	/* characters */
	ROM_LOAD( "format_z.1",   0x4000, 0x1000, 0x5739afd2 )	/* sprites */
	ROM_LOAD( "format_z.2",   0x5000, 0x1000, 0x3a821391 )	/* sprites */
	ROM_LOAD( "format_z.3",   0x6000, 0x1000, 0x7d1aec79 )	/* sprites */

	ROM_REGION(0x10000)     /* 64k for sound CPU */
	ROM_LOAD( "format_z.9",   0xf000, 0x1000, 0x6b9215ad )
ROM_END

ROM_START( aeroboto_rom )
	ROM_REGION(0x10000)     /* 64k for main CPU */
	ROM_LOAD( "aeroboto.8",   0x4000, 0x4000, 0x4d3fc049 )
	ROM_LOAD( "aeroboto.7",   0x8000, 0x4000, 0x522f51c1 )
	ROM_LOAD( "aeroboto.6",   0xc000, 0x4000, 0x1a295ffb )

	ROM_REGION_DISPOSE(0x7000)	/* temporary space for graphics */
	ROM_LOAD( "aeroboto.5",   0x0000, 0x2000, 0x32fc00f9 )	/* characters */
	ROM_LOAD( "format_z.4",   0x2000, 0x2000, 0x910375a0 )	/* characters */
	ROM_LOAD( "aeroboto.1",   0x4000, 0x1000, 0x7820eeaf )	/* sprites */
	ROM_LOAD( "aeroboto.2",   0x5000, 0x1000, 0xc7f81a3c )	/* sprites */
	ROM_LOAD( "aeroboto.3",   0x6000, 0x1000, 0x5203ad04 )	/* sprites */

	ROM_REGION(0x10000)     /* 64k for sound CPU */
	ROM_LOAD( "format_z.9",   0xf000, 0x1000, 0x6b9215ad )
ROM_END



struct GameDriver formatz_driver =
{
	__FILE__,
	0,
	"formatz",
	"Formation Z",
	"1984",
	"Jaleco",
	"Carlos A. Lozano",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	formatz_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver aeroboto_driver =
{
	__FILE__,
	&formatz_driver,
	"aeroboto",
	"Aeroboto",
	"1984",
	"[Jaleco] (Williams license)",
	"Carlos A. Lozano",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	aeroboto_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
