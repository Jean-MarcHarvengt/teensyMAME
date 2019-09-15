/***************************************************************************

Jack the Giant Killer memory map (preliminary)

Main CPU
--------
0000-3fff  ROM
4000-62ff  RAM
b000-b07f  sprite ram
b400       command for sound CPU
b500-b505  input ports
b506	   screen flip off
b507	   screen flip on
b600-b61f  palette ram
b800-bbff  video ram
bc00-bfff  color ram
c000-ffff  More ROM

Sound CPU (appears to run in interrupt mode 1)
---------
0000-0fff  ROM
1000-1fff  ROM (Zzyzzyxx only)
4000-43ff  RAM

I/O
---
0x40: Read - ay-8910 port 0
      Write - ay-8910 write
0x80: Write - ay-8910 control

The 2 ay-8910 read ports are responsible for reading the sound commands.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


void jack_paletteram_w(int offset,int data);
void jack_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int  jack_flipscreen_r(int offset);
void jack_flipscreen_w(int offset, int data);

void jack_sh_command_w(int offset,int data)
{
	soundlatch_w(0,data);
	cpu_cause_interrupt(1,0xff);
}



int jack_portB_r(int offset)
{
	#define TIMER_RATE 128

	return cpu_gettotalcycles() / TIMER_RATE;

	#undef TIMER_RATE
}

int zzyzzyxx_portB_r(int offset)
{
	#define TIMER_RATE 16

	return cpu_gettotalcycles() / TIMER_RATE;

	#undef TIMER_RATE
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0xc000, 0xffff, MRA_ROM },
	{ 0x4000, 0x62ff, MRA_RAM },
	{ 0xb500, 0xb500, input_port_0_r },
	{ 0xb501, 0xb501, input_port_1_r },
	{ 0xb502, 0xb502, input_port_2_r },
	{ 0xb503, 0xb503, input_port_3_r },
	{ 0xb504, 0xb504, input_port_4_r },
	{ 0xb505, 0xb505, input_port_5_r },
	{ 0xb506, 0xb507, jack_flipscreen_r },
	{ 0xb000, 0xb07f, MRA_RAM },
	{ 0xb800, 0xbfff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x62ff, MWA_RAM },
	{ 0xb000, 0xb07f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xb400, 0xb400, jack_sh_command_w },
	{ 0xb506, 0xb507, jack_flipscreen_w },
	{ 0xb600, 0xb61f, jack_paletteram_w, &paletteram },
	{ 0xb800, 0xbbff, videoram_w, &videoram, &videoram_size },
	{ 0xbc00, 0xbfff, colorram_w, &colorram },
	{ 0xc000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x6000, 0x6000, MWA_NOP },
	{ -1 }	/* end of table */
};


static struct IOReadPort sound_readport[] =
{
	{ 0x40, 0x40, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x80, 0x80, AY8910_control_port_0_w },
	{ 0x40, 0x40, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "4 Coins/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x0c, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPNAME( 0x10, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Every 10000" )
	PORT_DIPSETTING(    0x20, "10000 Only" )
	PORT_DIPNAME( 0x40, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Start Level 1" )
	PORT_DIPSETTING(    0x40, "Start Level 5" )
	PORT_DIPNAME( 0x80, 0x00, "Number of Beans", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "x1" )
	PORT_DIPSETTING(    0x80, "x2" )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )  // Most likely unused
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )  // Most likely unused
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )  // Most likely unused
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )  // Most likely unused
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_BITX (   0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING (   0x00, "Off" )
	PORT_DIPSETTING (   0x20, "On" )
	PORT_BITX (   0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING (   0x00, "Off" )
	PORT_DIPSETTING (   0x40, "On" )
	PORT_BITX (   0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "255 Lives", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING (   0x00, "Off" )
	PORT_DIPSETTING (   0x80, "On" )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x1c, IP_ACTIVE_HIGH, IPT_UNKNOWN )  // Most likely unused
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )  // Most likely unused

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // Most likely unused

	PORT_START      /* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // Most likely unused
INPUT_PORTS_END

INPUT_PORTS_START( zzyzzyxx_input_ports )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPNAME( 0x04, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x08, 0x00, "Start with 2 Credits", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Attract Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_BITX (   0x40, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING (   0x00, "Off" )
	PORT_DIPSETTING (   0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Never" )
	PORT_DIPSETTING(    0x00, "10k/50k" )
	PORT_DIPSETTING(    0x01, "25k/100k" )
	PORT_DIPSETTING(    0x03, "100k/300k" )
	PORT_DIPNAME( 0x04, 0x04, "2nd Bonus Given", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Starting Laps", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPNAME( 0x10, 0x00, "Please Lola", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_BITX (   0x20, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Show Intermissions", OSD_KEY_I, IP_JOY_NONE, 0 )
	PORT_DIPSETTING (   0x00, "Off" )
	PORT_DIPSETTING (   0x20, "On" )
	PORT_DIPNAME( 0xc0, 0x00, "Extra Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "None" )
  //PORT_DIPSETTING(    0x40, "None" )
	PORT_DIPSETTING(    0x80, "4 under 4000 pts" )
	PORT_DIPSETTING(    0xc0, "6 under 4000 pts" )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x1c, IP_ACTIVE_HIGH, IPT_UNKNOWN )  // Most likely unused
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNKNOWN )  // Most likely unused

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_2WAY )
	PORT_BIT( 0x0c, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // Most likely unused

	PORT_START      /* IN5 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // Most likely unused
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 0, 1024*8*8 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 16 bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout, 0, 8 },
	{ -1 } /* end of array */
};



/* Sound stuff */
static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1789750,	/* 1.78975 MHz?????? */
	{ 0x10ff },
	{ soundlatch_r },
	{ jack_portB_r },
	{ 0 },
	{ 0 }
};

static struct AY8910interface zzyzzyxx_ay8910_interface =
{
	1,	/* 1 chip */
	1789750,	/* 1.78975 MHz?????? */
	{ 255 },
	{ soundlatch_r },
	{ zzyzzyxx_portB_r },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			1789750,	/* 1.78975 MHz?????? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,0	/* IRQs are caused by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32, 32,
	0,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	jack_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

static struct MachineDriver zzyzzyxx_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			1789750,	/* 1.78975 MHz?????? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,0	/* IRQs are caused by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32, 32,
	0,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	jack_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&zzyzzyxx_ay8910_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( jack_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "jgk.j8",       0x0000, 0x1000, 0xfe229e20 )
	ROM_LOAD( "jgk.j6",       0x1000, 0x1000, 0x36d7810e )
	ROM_LOAD( "jgk.j7",       0x2000, 0x1000, 0xb15ff3ee )
	ROM_LOAD( "jgk.j5",       0x3000, 0x1000, 0x4a63d242 )
	ROM_LOAD( "jgk.j3",       0xc000, 0x1000, 0x605514a8 )
	ROM_LOAD( "jgk.j4",       0xd000, 0x1000, 0xbce489b7 )
	ROM_LOAD( "jgk.j2",       0xe000, 0x1000, 0xdb21bd55 )
	ROM_LOAD( "jgk.j1",       0xf000, 0x1000, 0x49fffe31 )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "jgk.j12",      0x0000, 0x1000, 0xce726df0 )
	ROM_LOAD( "jgk.j13",      0x1000, 0x1000, 0x6aec2c8d )
	ROM_LOAD( "jgk.j11",      0x2000, 0x1000, 0xfd14c525 )
	ROM_LOAD( "jgk.j10",      0x3000, 0x1000, 0xeab890b2 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "jgk.j9",       0x0000, 0x1000, 0xc2dc1e00 )
ROM_END

ROM_START( jacka_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "jack8",        0x0000, 0x1000, 0x632151d2 )
	ROM_LOAD( "jack6",        0x1000, 0x1000, 0xf94f80d9 )
	ROM_LOAD( "jack7",        0x2000, 0x1000, 0xc830ff1e )
	ROM_LOAD( "jack5",        0x3000, 0x1000, 0x8dea17e7 )
	ROM_LOAD( "jgk.j3",       0xc000, 0x1000, 0x605514a8 )
	ROM_LOAD( "jgk.j4",       0xd000, 0x1000, 0xbce489b7 )
	ROM_LOAD( "jgk.j2",       0xe000, 0x1000, 0xdb21bd55 )
	ROM_LOAD( "jack1",        0xf000, 0x1000, 0x7e75ea3d )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "jack12",       0x0000, 0x1000, 0x80320647 )
	ROM_LOAD( "jgk.j13",      0x1000, 0x1000, 0x6aec2c8d )
	ROM_LOAD( "jgk.j11",      0x2000, 0x1000, 0xfd14c525 )
	ROM_LOAD( "jgk.j10",      0x3000, 0x1000, 0xeab890b2 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "jgk.j9",       0x0000, 0x1000, 0xc2dc1e00 )
ROM_END

ROM_START( treahunt_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "thunt-1.f2",       0x0000, 0x1000, 0x0b35858c )
	ROM_LOAD( "thunt-2.f3",       0x1000, 0x1000, 0x67305a51 )
	ROM_LOAD( "thunt-3.4f",       0x2000, 0x1000, 0xd7a969c3 )
	ROM_LOAD( "thunt-4.6f",       0x3000, 0x1000, 0x2483f14d )
	ROM_LOAD( "thunt-5.7f",       0xc000, 0x1000, 0xc69d5e21 )
	ROM_LOAD( "thunt-6.7e",       0xd000, 0x1000, 0x11bf3d49 )
	ROM_LOAD( "thunt-7.6e",       0xe000, 0x1000, 0x7c2d6279 )
	ROM_LOAD( "thunt-8.4e",       0xf000, 0x1000, 0xf73b86fb )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "thunt-13.a4",      0x0000, 0x1000, 0xe03f1f09 )
	ROM_LOAD( "thunt-12.a3",      0x1000, 0x1000, 0xda4ee9eb )
	ROM_LOAD( "thunt-10.a1",      0x2000, 0x1000, 0x51ec7934 )
	ROM_LOAD( "thunt-11.a2",      0x3000, 0x1000, 0xf9781143 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "thunt-9.4a",       0x0000, 0x1000, 0xc2dc1e00 )
ROM_END

ROM_START( zzyzzyxx_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "zzyzzyxx.a",   0x0000, 0x1000, 0xa9102e34 )
	ROM_LOAD( "zzyzzyxx.b",   0x1000, 0x1000, 0xefa9d4c6 )
	ROM_LOAD( "zzyzzyxx.c",   0x2000, 0x1000, 0xb0a365b1 )
	ROM_LOAD( "zzyzzyxx.d",   0x3000, 0x1000, 0x5ed6dd9a )
	ROM_LOAD( "zzyzzyxx.e",   0xc000, 0x1000, 0x5966fdbf )
	ROM_LOAD( "zzyzzyxx.f",   0xd000, 0x1000, 0x12f24c68 )
	ROM_LOAD( "zzyzzyxx.g",   0xe000, 0x1000, 0x408f2326 )
	ROM_LOAD( "zzyzzyxx.h",   0xf000, 0x1000, 0xf8bbabe0 )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "zzyzzyxx.n",   0x0000, 0x1000, 0x4f64538d )
	ROM_LOAD( "zzyzzyxx.m",   0x1000, 0x1000, 0x217b1402 )
	ROM_LOAD( "zzyzzyxx.k",   0x2000, 0x1000, 0xb8b2b8cc )
	ROM_LOAD( "zzyzzyxx.l",   0x3000, 0x1000, 0xab421a83 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "zzyzzyxx.i",   0x0000, 0x1000, 0xc7742460 )
	ROM_LOAD( "zzyzzyxx.j",   0x1000, 0x1000, 0x72166ccd )
ROM_END

ROM_START( brix_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "a",            0x0000, 0x1000, 0x050e0d70 )
	ROM_LOAD( "b",            0x1000, 0x1000, 0x668118ae )
	ROM_LOAD( "c",            0x2000, 0x1000, 0xff5ed6cf )
	ROM_LOAD( "d",            0x3000, 0x1000, 0xc3ae45a9 )
	ROM_LOAD( "e",            0xc000, 0x1000, 0xdef99fa9 )
	ROM_LOAD( "f",            0xd000, 0x1000, 0xdde717ed )
	ROM_LOAD( "g",            0xe000, 0x1000, 0xadca02d8 )
	ROM_LOAD( "h",            0xf000, 0x1000, 0xbc3b878c )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "n",            0x0000, 0x1000, 0x8064910e )
	ROM_LOAD( "zzyzzyxx.m",   0x1000, 0x1000, 0x217b1402 )
	ROM_LOAD( "k",            0x2000, 0x1000, 0xc7d7e2a0 )
	ROM_LOAD( "zzyzzyxx.l",   0x3000, 0x1000, 0xab421a83 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "zzyzzyxx.i",   0x0000, 0x1000, 0xc7742460 )
	ROM_LOAD( "zzyzzyxx.j",   0x1000, 0x1000, 0x72166ccd )
ROM_END

ROM_START( sucasino_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1",       0x0000, 0x1000, 0xe116e979 )
	ROM_LOAD( "2",       0x1000, 0x1000, 0x2a2635f5 )
	ROM_LOAD( "3",       0x2000, 0x1000, 0x69864d90 )
	ROM_LOAD( "4",       0x3000, 0x1000, 0x174c9373 )
	ROM_LOAD( "5",       0xc000, 0x1000, 0x115bcb1e )
	ROM_LOAD( "6",       0xd000, 0x1000, 0x434caa17 )
	ROM_LOAD( "7",       0xe000, 0x1000, 0x67c68b82 )
	ROM_LOAD( "8",       0xf000, 0x1000, 0xf5b63006 )

	ROM_REGION_DISPOSE(0x4000) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "11",      0x0000, 0x1000, 0xf92c4c5b )
	/* 1000-1fff empty */
	ROM_LOAD( "10",      0x2000, 0x1000, 0x3b0783ce )
	/* 3000-3fff empty */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "9",       0x0000, 0x1000, 0x67cf8aec )
ROM_END



static void treahunt_decode(void)
{
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	int data;

	/* Thanks to Mike Balfour for helping out with the decryption */
	for (A = 0; A < 0x1000; A++)
	{
		data = RAM[A];

		/* unencrypted = !D7 D2 D5 D1 D3 D6 D4 !D0 */
		ROM[A] = (~data & 0x81) |
				 ((data & 0x02) << 3) |
				 ((data & 0x04) << 4) |
				  (data & 0x28) |
				 ((data & 0x10) >> 3) |
				 ((data & 0x40) >> 4);
	}
	for (A = 0x1000; A < 0x2000; A++)
	{
		data = RAM[A];
		if ((A & 0x04) == 0)
		/* unencrypted = !D7 D2 D5 D1 D3 D6 D4 !D0 */
			ROM[A] = (~data & 0x81) |
				 ((data & 0x02) << 3) |
				 ((data & 0x04) << 4) |
				  (data & 0x28) |
				 ((data & 0x10) >> 3) |
				 ((data & 0x40) >> 4);
		else
		/* unencrypted = D0 D2 D5 D1 D3 D6 D4 D7 */
			ROM[A] =
				 ((data & 0x01) << 7) |
				 ((data & 0x02) << 3) |
				 ((data & 0x04) << 4) |
				  (data & 0x28) |
				 ((data & 0x10) >> 3) |
				 ((data & 0x40) >> 4) |
				 ((data & 0x80) >> 7);
	}
	for (A = 0x2000; A < 0x3000; A++)
	{
		data = RAM[A];

		/* unencrypted = !D7 D2 D5 D1 D3 D6 D4 !D0 */
		ROM[A] = (~data & 0x81) |
				 ((data & 0x02) << 3) |
				 ((data & 0x04) << 4) |
				  (data & 0x28) |
				 ((data & 0x10) >> 3) |
				 ((data & 0x40) >> 4);
	}
	for (A = 0x3000; A < 0x10000; A++)
	{
		data = RAM[A];
		if ((A & 0x04) == 0)
		/* unencrypted = !D0 D2 D5 D1 D3 D6 D4 !D7 */
			ROM[A] =
				 ((~data & 0x01) << 7) |
				 ((data & 0x02) << 3) |
				 ((data & 0x04) << 4) |
				  (data & 0x28) |
				 ((data & 0x10) >> 3) |
				 ((data & 0x40) >> 4) |
				 ((~data & 0x80) >> 7);
		else
		/* unencrypted = D0 D2 D5 D1 D3 D6 D4 D7 */
			ROM[A] =
				 ((data & 0x01) << 7) |
				 ((data & 0x02) << 3) |
				 ((data & 0x04) << 4) |
				  (data & 0x28) |
				 ((data & 0x10) >> 3) |
				 ((data & 0x40) >> 4) |
				 ((data & 0x80) >> 7);
	}
}



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x04506],"\x41\x41\x41",3) == 0 &&
			memcmp(&RAM[0x4560],"\x41\x41\x41",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4500],9*11);
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
		osd_fwrite(f,&RAM[0x4500],9*11);
		osd_fclose(f);
	}
}

static int zzyzzyxx_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x5100],"\x00\x01\x50",3) == 0 &&
			memcmp(&RAM[0x541b],"\x91\x9d\xa3",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x5100],4*10);
			osd_fread(f,&RAM[0x5400],3*10);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void zzyzzyxx_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x5100],4*10);
		osd_fwrite(f,&RAM[0x5400],3*10);
		osd_fclose(f);
	}
}



struct GameDriver jack_driver =
{
	__FILE__,
	0,
	"jack",
	"Jack the Giantkiller (set 1)",
	"1982",
	"Cinematronics",
	"Brad Oliver",
	0,
	&machine_driver,
	0,

	jack_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver jacka_driver =
{
	__FILE__,
	&jack_driver,
	"jacka",
	"Jack the Giantkiller (set 2)",
	"1982",
	"Cinematronics",
	"Brad Oliver",
	0,
	&machine_driver,
	0,

	jacka_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver treahunt_driver =
{
	__FILE__,
	0,
	"treahunt",
	"Treasure Hunt",
	"????",
	"?????",
	"Brad Oliver\nMike Balfour",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	treahunt_rom,
	0, treahunt_decode,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,
	hiload, hisave
};

struct GameDriver zzyzzyxx_driver =
{
	__FILE__,
	0,
	"zzyzzyxx",
	"Zzyzzyxx",
	"1982",
	"Cinematronics",
	"Brad Oliver",
	0,
	&zzyzzyxx_machine_driver,
	0,

	zzyzzyxx_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	zzyzzyxx_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	zzyzzyxx_hiload, zzyzzyxx_hisave
};

struct GameDriver brix_driver =
{
	__FILE__,
	&zzyzzyxx_driver,
	"brix",
	"Brix",
	"1982",
	"Cinematronics",
	"Brad Oliver",
	0,
	&zzyzzyxx_machine_driver,
	0,

	brix_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	zzyzzyxx_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	zzyzzyxx_hiload, zzyzzyxx_hisave
};

struct GameDriver sucasino_driver =
{
	__FILE__,
	0,
	"sucasino",
	"Super Casino",
	"1982",
	"Data Amusement",
	"Brad Oliver",
	0,
	&machine_driver,
	0,

	sucasino_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,
	0, 0
};
