/***************************************************************************

MCR/II memory map (preliminary)

0000-3fff ROM 0
4000-7fff ROM 1
8000-bfff ROM 2
c000-c7ff RAM
f000-f7ff sprite ram
f800-ff7f tiles
ff80-ffff palette ram

IN0

bit 0 : left coin
bit 1 : right coin
bit 2 : 1 player
bit 3 : 2 player
bit 4 : trigger button
bit 5 : tilt
bit 6 : ?
bit 7 : service

IN1, IN2

joystick, sometimes spinner

bit 0: left
bit 1: right
bit 2: up
bit 3: down

IN3

Usually dipswitches. Most game configuration is really done by holding down
the service key (F2) during the startup self-test.

Known issues:

* Trackball support is iffy at the moment.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int journey_vh_start(void);
void journey_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void mcr2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void mcr2_videoram_w(int offset,int data);
void mcr2_paletteram_w(int offset,int data);

void mcr_init_machine(void);
int mcr_interrupt(void);
extern int mcr_loadnvram;

void mcr_writeport(int port,int value);
int mcr_readport(int port);
void mcr_soundstatus_w (int offset,int data);
int mcr_soundlatch_r (int offset);

int kroozr_dial_r(int offset);
int kroozr_trakball_x_r(int offset);
int kroozr_trakball_y_r(int offset);


/***************************************************************************

  Memory maps

***************************************************************************/

static struct MemoryReadAddress mcr2_readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xf000, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress mcr2_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xf7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf800, 0xff7f, mcr2_videoram_w, &videoram, &videoram_size },
	{ 0xff80, 0xffff, mcr2_paletteram_w, &paletteram },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress journey_readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xf000, 0xf1ff, MRA_RAM },
	{ 0xf800, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress journey_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xf1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf800, 0xff7f, mcr2_videoram_w, &videoram, &videoram_size },
	{ 0xff80, 0xffff, mcr2_paletteram_w, &paletteram },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x9000, 0x9003, mcr_soundlatch_r },
	{ 0xa001, 0xa001, AY8910_read_port_0_r },
	{ 0xb001, 0xb001, AY8910_read_port_1_r },
	{ 0xf000, 0xf000, input_port_5_r },
	{ 0xe000, 0xe000, MRA_NOP },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0xa000, 0xa000, AY8910_control_port_0_w },
	{ 0xa002, 0xa002, AY8910_write_port_0_w },
	{ 0xb000, 0xb000, AY8910_control_port_1_w },
	{ 0xb002, 0xb002, AY8910_write_port_1_w },
	{ 0xc000, 0xc000, mcr_soundstatus_w },
	{ 0xe000, 0xe000, MWA_NOP },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct IOReadPort readport[] =
{
   { 0x00, 0x00, input_port_0_r },
   { 0x01, 0x01, input_port_1_r },
   { 0x02, 0x02, input_port_2_r },
   { 0x03, 0x03, input_port_3_r },
   { 0x04, 0x04, input_port_4_r },
   { 0x05, 0xff, mcr_readport },
   { -1 }
};

static struct IOReadPort kroozr_readport[] =
{
   { 0x00, 0x00, input_port_0_r },
   { 0x01, 0x01, kroozr_dial_r }, /* firing spinner */
   { 0x02, 0x02, kroozr_trakball_x_r }, /* x-axis */
   { 0x03, 0x03, input_port_3_r },
   { 0x04, 0x04, kroozr_trakball_y_r }, /* y-axis */
   { 0x05, 0xff, mcr_readport },
   { -1 }
};

static struct IOWritePort writeport[] =
{
   { 0, 0xFF, mcr_writeport },
   { -1 }	/* end of table */
};


/***************************************************************************

  Input port definitions

***************************************************************************/

INPUT_PORTS_START( shollow_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* IN2 unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( tron_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN1 -- controls spinner */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 50, 0, 0, 0, OSD_KEY_Z, OSD_KEY_X, 0, 0, 4 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN3 -- dipswitches */
	PORT_DIPNAME( 0x01, 0x00, "Coin Meters", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )
	PORT_DIPNAME( 0x04, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_BIT( 0xf8, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_COCKTAIL, 50, 0, 0, 0 )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( kroozr_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN1 -- controls firing spinner */
	PORT_BIT( 0x07, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* IN2 -- controls joystick x-axis */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 -- controls joystick y-axis */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* dummy extra port for keyboard movement */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* dummy extra port for dial control */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 40, 0, 0, 0, OSD_KEY_Z, OSD_KEY_X, 0, 0, 4 )
INPUT_PORTS_END

INPUT_PORTS_START( domino_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )

	PORT_START	/* IN2 unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 unused */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( wacko_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN1 -- controls joystick x-axis */
	PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_X, 50, 0, 0, 0 )

	PORT_START	/* IN2 -- controls joystick y-axis */
	PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_Y | IPF_REVERSE, 50, 0, 0, 0 )

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 -- 4-way firing joystick */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_4WAY )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( twotiger_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_START1, "Dogfight Start", OSD_KEY_6, IP_JOY_NONE, 0 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN1 -- player 1 spinner */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 10, 0, 0, 0 )

	PORT_START	/* IN2 -- buttons for player 1 & player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 -- player 2 spinner */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_REVERSE | IPF_PLAYER2, 10, 0, 0, 0 )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


/***************************************************************************

  Graphics layouts

***************************************************************************/

/* note that characters are half the resolution of sprites in each direction, so we generate
   them at double size */

/* 512 characters; used by all the mcr2 games */
static struct GfxLayout mcr2_charlayout_512 =
{
	16, 16,
	512,
	4,
	{ 512*16*8, 512*16*8+1, 0, 1 },
	{ 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14 },
	{ 0, 0, 2*8, 2*8, 4*8, 4*8, 6*8, 6*8, 8*8, 8*8, 10*8, 10*8, 12*8, 12*8, 14*8, 14*8 },
	16*8
};

/* 64 sprites; used by all mcr2 games but Journey */
#define X (64*128*8)
#define Y (2*X)
#define Z (3*X)
static struct GfxLayout mcr2_spritelayout_64 =
{
   32,32,
   64,
   4,
   { 0, 1, 2, 3 },
   {  Z+0, Z+4, Y+0, Y+4, X+0, X+4, 0, 4, Z+8, Z+12, Y+8, Y+12, X+8, X+12, 8, 12,
      Z+16, Z+20, Y+16, Y+20, X+16, X+20, 16, 20, Z+24, Z+28, Y+24, Y+28,
      X+24, X+28, 24, 28 },
   {  0, 32, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7, 32*8, 32*9, 32*10, 32*11,
      32*12, 32*13, 32*14, 32*15, 32*16, 32*17, 32*18, 32*19, 32*20, 32*21,
      32*22, 32*23, 32*24, 32*25, 32*26, 32*27, 32*28, 32*29, 32*30, 32*31 },
   128*8
};
#undef X
#undef Y
#undef Z

/* 128 sprites; used by Journey - it features an mcr3 spriteboard */
#define X (128*128*8)
#define Y (2*X)
#define Z (3*X)
static struct GfxLayout mcr3_spritelayout_128 =
{
   32,32,
   128,
   4,
   { 0, 1, 2, 3 },
   {  Z+0, Z+4, Y+0, Y+4, X+0, X+4, 0, 4, Z+8, Z+12, Y+8, Y+12, X+8, X+12, 8, 12,
      Z+16, Z+20, Y+16, Y+20, X+16, X+20, 16, 20, Z+24, Z+28, Y+24, Y+28,
      X+24, X+28, 24, 28 },
   {  0, 32, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7, 32*8, 32*9, 32*10, 32*11,
      32*12, 32*13, 32*14, 32*15, 32*16, 32*17, 32*18, 32*19, 32*20, 32*21,
      32*22, 32*23, 32*24, 32*25, 32*26, 32*27, 32*28, 32*29, 32*30, 32*31 },
   128*8
};
#undef X
#undef Y
#undef Z

static struct GfxDecodeInfo mcr2_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mcr2_charlayout_512,   0, 4 },	/* colors 0-63 */
	{ 1, 0x4000, &mcr2_spritelayout_64, 16, 1 },	/* colors 16-31 */
	{ -1 } /* end of array */
};

/* Wacko uses different colors for the sprites */
static struct GfxDecodeInfo wacko_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mcr2_charlayout_512,   0, 4 },	/* colors 0-63 */
	{ 1, 0x4000, &mcr2_spritelayout_64,  0, 1 },	/* colors 0-15 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo journey_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mcr2_charlayout_512,   0, 4 },	/* colors 0-63 */
	{ 1, 0x4000, &mcr3_spritelayout_128, 0, 4 },	/* colors 0-63 */
	{ -1 } /* end of array */
};


/***************************************************************************

  Sound interfaces

***************************************************************************/

static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	2000000,	/* 2 MHz ?? */
	{ 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


/***************************************************************************

  Machine drivers

***************************************************************************/

static struct MachineDriver mcr2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2500000,	/* 2.5 Mhz */
			0,
			mcr2_readmem,mcr2_writemem,readport,writeport,
			mcr_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,26
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - sound CPU has enough interrupts to handle synchronization */
	mcr_init_machine,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	mcr2_gfxdecodeinfo,
	64, 64,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	mcr2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

static struct MachineDriver wacko_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2500000,	/* 2.5 Mhz */
			0,
			mcr2_readmem,mcr2_writemem,readport,writeport,
			mcr_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,26
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - sound CPU has enough interrupts to handle synchronization */
	mcr_init_machine,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	wacko_gfxdecodeinfo,
	64, 64,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	mcr2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

static struct MachineDriver kroozr_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2500000,	/* 2.5 Mhz */
			0,
			mcr2_readmem,mcr2_writemem,kroozr_readport,writeport,
			mcr_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,26
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - sound CPU has enough interrupts to handle synchronization */
	mcr_init_machine,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	mcr2_gfxdecodeinfo,
	64, 64,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	mcr2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

static struct MachineDriver journey_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			7500000,	/* Looks like it runs at 7.5 Mhz rather than 5 or 2.5 */
			0,
			journey_readmem,journey_writemem,readport,writeport,
			mcr_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,26
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - sound CPU has enough interrupts to handle synchronization */
	mcr_init_machine,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	journey_gfxdecodeinfo,
	64, 64,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	journey_vh_start,
	generic_vh_stop,
	journey_vh_screenrefresh,

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

  High score save/load

***************************************************************************/

static int mcr2_hiload(int addr, int len)
{
   unsigned char *RAM = Machine->memory_region[0];

   /* see if it's okay to load */
   if (mcr_loadnvram)
   {
      void *f;

		f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
      if (f)
      {
			osd_fread(f,&RAM[addr],len);
			osd_fclose (f);
      }
      return 1;
   }
   else return 0;	/* we can't load the hi scores yet */
}

static void mcr2_hisave(int addr, int len)
{
   unsigned char *RAM = Machine->memory_region[0];
   void *f;

	f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1);
   if (f)
   {
      osd_fwrite(f,&RAM[addr],len);
      osd_fclose (f);
   }
}

static int  shollow_hiload(void)  { return mcr2_hiload(0xc600, 0x8a); }
static void shollow_hisave(void)  {        mcr2_hisave(0xc600, 0x8a); }

static int  tron_hiload(void)     { return mcr2_hiload(0xc4f0, 0x97); }
static void tron_hisave(void)     {        mcr2_hisave(0xc4f0, 0x97); }

static int  kroozr_hiload(void)   { return mcr2_hiload(0xc466, 0x95); }
static void kroozr_hisave(void)   {        mcr2_hisave(0xc466, 0x95); }

static int  domino_hiload(void)   { return mcr2_hiload(0xc000, 0x92); }
static void domino_hisave(void)   {        mcr2_hisave(0xc000, 0x92); }

static int  wacko_hiload(void)    { return mcr2_hiload(0xc000, 0x91); }
static void wacko_hisave(void)    {        mcr2_hisave(0xc000, 0x91); }

static int  twotiger_hiload(void) { return mcr2_hiload(0xc000, 0xa0); }
static void twotiger_hisave(void) {        mcr2_hisave(0xc000, 0xa0); }

static int  journey_hiload(void)  { return mcr2_hiload(0xc000, 0x9e); }
static void journey_hisave(void)  {        mcr2_hisave(0xc000, 0x9e); }


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( shollow_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "sh-pro.00",    0x0000, 0x2000, 0x95e2b800 )
	ROM_LOAD( "sh-pro.01",    0x2000, 0x2000, 0xb99f6ff8 )
	ROM_LOAD( "sh-pro.02",    0x4000, 0x2000, 0x1202c7b2 )
	ROM_LOAD( "sh-pro.03",    0x6000, 0x2000, 0x0a64afb9 )
	ROM_LOAD( "sh-pro.04",    0x8000, 0x2000, 0x22fa9175 )
	ROM_LOAD( "sh-pro.05",    0xa000, 0x2000, 0x1716e2bb )

	ROM_REGION_DISPOSE(0x0c000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sh-bg.00",     0x00000, 0x2000, 0x3e2b333c )
	ROM_LOAD( "sh-bg.01",     0x02000, 0x2000, 0xd1d70cc4 )
	ROM_LOAD( "sh-fg.03",     0x04000, 0x2000, 0x37ea9d07 )
	ROM_LOAD( "sh-fg.02",     0x06000, 0x2000, 0x6b57f6da )
	ROM_LOAD( "sh-fg.01",     0x08000, 0x2000, 0xba1a38b4 )
	ROM_LOAD( "sh-fg.00",     0x0a000, 0x2000, 0x33f4554e )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "sh-snd.01",    0x0000, 0x1000, 0x55a297cc )
	ROM_LOAD( "sh-snd.02",    0x1000, 0x1000, 0x46fc31f6 )
	ROM_LOAD( "sh-snd.03",    0x2000, 0x1000, 0xb1f4a6a8 )
ROM_END

ROM_START( tron_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "scpu_pga.bin", 0x0000, 0x2000, 0x5151770b )
	ROM_LOAD( "scpu_pgb.bin", 0x2000, 0x2000, 0x8ddf8717 )
	ROM_LOAD( "scpu_pgc.bin", 0x4000, 0x2000, 0x4241e3a0 )
	ROM_LOAD( "scpu_pgd.bin", 0x6000, 0x2000, 0x035d2fe7 )
	ROM_LOAD( "scpu_pge.bin", 0x8000, 0x2000, 0x24c185d8 )
	ROM_LOAD( "scpu_pgf.bin", 0xA000, 0x2000, 0x38c4bbaf )

	ROM_REGION_DISPOSE(0x0c000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "scpu_bgg.bin", 0x00000, 0x2000, 0x1a9ed2f5 )
	ROM_LOAD( "scpu_bgh.bin", 0x02000, 0x2000, 0x3220f974 )
	ROM_LOAD( "vg_0.bin",     0x04000, 0x2000, 0x9743f873 )
	ROM_LOAD( "vg_1.bin",     0x06000, 0x2000, 0x3329f9d4 )
	ROM_LOAD( "vg_2.bin",     0x08000, 0x2000, 0x58ee14d3 )
	ROM_LOAD( "vg_3.bin",     0x0a000, 0x2000, 0xbc036d1d )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ssi_0a.bin",   0x0000, 0x1000, 0x765e6eba )
	ROM_LOAD( "ssi_0b.bin",   0x1000, 0x1000, 0x1b90ccdd )
	ROM_LOAD( "ssi_0c.bin",   0x2000, 0x1000, 0x3a4bc629 )
ROM_END

ROM_START( tron2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pro0.d2",      0x0000, 0x2000, 0x0de0471a )
	ROM_LOAD( "scpu_pgb.bin", 0x2000, 0x2000, 0x8ddf8717 )
	ROM_LOAD( "scpu_pgc.bin", 0x4000, 0x2000, 0x4241e3a0 )
	ROM_LOAD( "scpu_pgd.bin", 0x6000, 0x2000, 0x035d2fe7 )
	ROM_LOAD( "scpu_pge.bin", 0x8000, 0x2000, 0x24c185d8 )
	ROM_LOAD( "scpu_pgf.bin", 0xA000, 0x2000, 0x38c4bbaf )

	ROM_REGION_DISPOSE(0x0c000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "scpu_bgg.bin", 0x00000, 0x2000, 0x1a9ed2f5 )
	ROM_LOAD( "scpu_bgh.bin", 0x02000, 0x2000, 0x3220f974 )
	ROM_LOAD( "vg_0.bin",     0x04000, 0x2000, 0x9743f873 )
	ROM_LOAD( "vg_1.bin",     0x06000, 0x2000, 0x3329f9d4 )
	ROM_LOAD( "vg_2.bin",     0x08000, 0x2000, 0x58ee14d3 )
	ROM_LOAD( "vg_3.bin",     0x0a000, 0x2000, 0xbc036d1d )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ssi_0a.bin",   0x0000, 0x1000, 0x765e6eba )
	ROM_LOAD( "ssi_0b.bin",   0x1000, 0x1000, 0x1b90ccdd )
	ROM_LOAD( "ssi_0c.bin",   0x2000, 0x1000, 0x3a4bc629 )
ROM_END

ROM_START( kroozr_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "kozmkcpu.2d",  0x0000, 0x2000, 0x61e02045 )
	ROM_LOAD( "kozmkcpu.3d",  0x2000, 0x2000, 0xcaabed57 )
	ROM_LOAD( "kozmkcpu.4d",  0x4000, 0x2000, 0x2bc83fc7 )
	ROM_LOAD( "kozmkcpu.5d",  0x6000, 0x2000, 0xa0ec38c1 )
	ROM_LOAD( "kozmkcpu.6d",  0x8000, 0x2000, 0x7044f2b6 )

	ROM_REGION_DISPOSE(0x0c000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "kozmkcpu.3g",  0x00000, 0x2000, 0xeda6ed2d )
	ROM_LOAD( "kozmkcpu.4g",  0x02000, 0x2000, 0xddde894b )
	ROM_LOAD( "kozmkvid.1a",  0x04000, 0x2000, 0xb57fb0ff )
	ROM_LOAD( "kozmkvid.1b",  0x06000, 0x2000, 0xc6041ba7 )
	ROM_LOAD( "kozmkvid.1d",  0x08000, 0x2000, 0x4e23b35b )
	ROM_LOAD( "kozmkvid.1e",  0x0a000, 0x2000, 0xca60e2cc )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "kozmksnd.7a",  0x0000, 0x1000, 0x6736e433 )
	ROM_LOAD( "kozmksnd.8a",  0x1000, 0x1000, 0xea9cd919 )
	ROM_LOAD( "kozmksnd.9a",  0x2000, 0x1000, 0x9dfa7994 )
ROM_END

ROM_START( domino_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "dmanpg0.bin",  0x0000, 0x2000, 0x3bf3bb1c )
	ROM_LOAD( "dmanpg1.bin",  0x2000, 0x2000, 0x85cf1d69 )
	ROM_LOAD( "dmanpg2.bin",  0x4000, 0x2000, 0x7dd2177a )
	ROM_LOAD( "dmanpg3.bin",  0x6000, 0x2000, 0xf2e0aa44 )

	ROM_REGION_DISPOSE(0x0c000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "dmanbg0.bin",  0x00000, 0x2000, 0x9163007f )
	ROM_LOAD( "dmanbg1.bin",  0x02000, 0x2000, 0x28615c56 )
	ROM_LOAD( "dmanfg3.bin",  0x04000, 0x2000, 0x1f39257e )
	ROM_LOAD( "dmanfg2.bin",  0x06000, 0x2000, 0x4a8e76b8 )
	ROM_LOAD( "dmanfg1.bin",  0x08000, 0x2000, 0x16aa4b9b )
	ROM_LOAD( "dmanfg0.bin",  0x0a000, 0x2000, 0x0b1f9f9e )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "dm-a7.snd",    0x0000, 0x1000, 0xfa982dcc )
	ROM_LOAD( "dm-a8.snd",    0x1000, 0x1000, 0x72839019 )
	ROM_LOAD( "dm-a9.snd",    0x2000, 0x1000, 0xad760da7 )
	ROM_LOAD( "dm-a10.snd",   0x3000, 0x1000, 0x958c7287 )
ROM_END

ROM_START( wacko_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "wackocpu.2d",  0x0000, 0x2000, 0xc98e29b6 )
	ROM_LOAD( "wackocpu.3d",  0x2000, 0x2000, 0x90b89774 )
	ROM_LOAD( "wackocpu.4d",  0x4000, 0x2000, 0x515edff7 )
	ROM_LOAD( "wackocpu.5d",  0x6000, 0x2000, 0x9b01bf32 )

	ROM_REGION_DISPOSE(0x0c000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "wackocpu.3g",  0x00000, 0x2000, 0x33160eb1 )
	ROM_LOAD( "wackocpu.4g",  0x02000, 0x2000, 0xdaf37d7c )
	ROM_LOAD( "wackovid.1a",  0x04000, 0x2000, 0x080be3ad )
	ROM_LOAD( "wackovid.1b",  0x06000, 0x2000, 0x7d899790 )
	ROM_LOAD( "wackovid.1d",  0x08000, 0x2000, 0xa02f1672 )
	ROM_LOAD( "wackovid.1e",  0x0a000, 0x2000, 0xdca59be7 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "wackosnd.7a",  0x0000, 0x1000, 0x1a58763f )
	ROM_LOAD( "wackosnd.8a",  0x1000, 0x1000, 0xa4e3c771 )
	ROM_LOAD( "wackosnd.9a",  0x2000, 0x1000, 0x155ba3dd )
ROM_END

ROM_START( twotiger_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2tgrpg0.bin",  0x0000, 0x2000, 0xe77a924b )
	ROM_LOAD( "2tgrpg1.bin",  0x2000, 0x2000, 0x2699ebdc )
	ROM_LOAD( "2tgrpg2.bin",  0x4000, 0x2000, 0xb5ca3f17 )
	ROM_LOAD( "2tgrpg3.bin",  0x6000, 0x2000, 0x8aa82049 )

	ROM_REGION_DISPOSE(0x0c000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "2tgrbg0.bin",  0x00000, 0x2000, 0x52f69068 )
	ROM_LOAD( "2tgrbg1.bin",  0x02000, 0x2000, 0x758d4f7d )
	ROM_LOAD( "2tgrfg3.bin",  0x04000, 0x2000, 0x9b22697b )
	ROM_LOAD( "2tgrfg2.bin",  0x06000, 0x2000, 0x08e3e1a6 )
	ROM_LOAD( "2tgrfg1.bin",  0x08000, 0x2000, 0xfbcaffa5 )
	ROM_LOAD( "2tgrfg0.bin",  0x0a000, 0x2000, 0x4abf3ca0 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "2tgra7.bin",   0x0000, 0x1000, 0x4620d970 )
	ROM_LOAD( "2tgra8.bin",   0x1000, 0x1000, 0xe95d8cfe )
	ROM_LOAD( "2tgra9.bin",   0x2000, 0x1000, 0x81e6ce0e )
ROM_END



struct GameDriver shollow_driver =
{
	__FILE__,
	0,
	"shollow",
	"Satan's Hollow",
	"1981",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria\nBrad Oliver",
	0,
	&mcr2_machine_driver,
	0,

	shollow_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	shollow_input_ports,

	0, 0,0,
	ORIENTATION_ROTATE_90,

	shollow_hiload,shollow_hisave
};

struct GameDriver tron_driver =
{
	__FILE__,
	0,
	"tron",
	"Tron (set 1)",
	"1982",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria\nBrad Oliver",
	0,
	&mcr2_machine_driver,
	0,

	tron_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tron_input_ports,

	0, 0,0,
	ORIENTATION_ROTATE_90,

	tron_hiload,tron_hisave
};

struct GameDriver tron2_driver =
{
	__FILE__,
	&tron_driver,
	"tron2",
	"Tron (set 2)",
	"1982",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria\nBrad Oliver",
	0,
	&mcr2_machine_driver,
	0,

	tron2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tron_input_ports,

	0, 0,0,
	ORIENTATION_ROTATE_90,

	tron_hiload,tron_hisave
};

struct GameDriver kroozr_driver =
{
	__FILE__,
	0,
	"kroozr",
	"Kozmik Kroozr",
	"1982",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria\nBrad Oliver",
	0,
	&kroozr_machine_driver,
	0,

	kroozr_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	kroozr_input_ports,

	0,0,0,
	ORIENTATION_DEFAULT,

	kroozr_hiload,kroozr_hisave
};

struct GameDriver domino_driver =
{
	__FILE__,
	0,
	"domino",
	"Domino Man",
	"1982",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria\nBrad Oliver",
	0,
	&mcr2_machine_driver,
	0,

	domino_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	domino_input_ports,

	0, 0,0,
	ORIENTATION_DEFAULT,

	domino_hiload,domino_hisave
};

struct GameDriver wacko_driver =
{
	__FILE__,
	0,
	"wacko",
	"Wacko",
	"1982",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria\nBrad Oliver\nJohn Butler",
	0,
	&wacko_machine_driver,
	0,

	wacko_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	wacko_input_ports,

	0,0,0,
	ORIENTATION_DEFAULT,

	wacko_hiload,wacko_hisave
};

struct GameDriver twotiger_driver =
{
	__FILE__,
	0,
	"twotiger",
	"Two Tigers",
	"1984",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria\nBrad Oliver",
	0,
	&mcr2_machine_driver,
	0,

	twotiger_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	twotiger_input_ports,

	0, 0,0,
	ORIENTATION_DEFAULT,

	twotiger_hiload,twotiger_hisave
};



ROM_START( journey_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "d2",           0x0000, 0x2000, 0xf2618913 )
	ROM_LOAD( "d3",           0x2000, 0x2000, 0x2f290d2e )
	ROM_LOAD( "d4",           0x4000, 0x2000, 0xcc6c0150 )
	ROM_LOAD( "d5",           0x6000, 0x2000, 0xc3023931 )
	ROM_LOAD( "d6",           0x8000, 0x2000, 0x5d445c99 )

	ROM_REGION_DISPOSE(0x14000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "g3",           0x00000, 0x2000, 0xc14558de )
	ROM_LOAD( "g4",           0x02000, 0x2000, 0x9104c1d0 )
	ROM_LOAD( "a1",           0x04000, 0x2000, 0x4af986f8 )
	ROM_LOAD( "a2",           0x06000, 0x2000, 0xb30cd2a7 )
	ROM_LOAD( "a3",           0x08000, 0x2000, 0xf295afda )
	ROM_LOAD( "a4",           0x0a000, 0x2000, 0x765876a7 )
	ROM_LOAD( "a5",           0x0c000, 0x2000, 0x560c474f )
	ROM_LOAD( "a6",           0x0e000, 0x2000, 0xb1f31583 )
	ROM_LOAD( "a7",           0x10000, 0x2000, 0x4ca2bb2d )
	ROM_LOAD( "a8",           0x12000, 0x2000, 0x4fb7925d )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "a",            0x0000, 0x1000, 0x2524a2aa )
	ROM_LOAD( "b",            0x1000, 0x1000, 0xb8e35814 )
	ROM_LOAD( "c",            0x2000, 0x1000, 0x09c488cf )
	ROM_LOAD( "d",            0x3000, 0x1000, 0x3d627bee )
ROM_END

struct GameDriver journey_driver =
{
	__FILE__,
	0,
	"journey",
	"Journey",
	"1983",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria\nBrad Oliver",
	0,
	&journey_machine_driver,
	0,

	journey_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	domino_input_ports,

	0,0,0,
	ORIENTATION_ROTATE_90,

	journey_hiload,journey_hisave
};
