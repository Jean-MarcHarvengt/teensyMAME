/***************************************************************************

MCR/III memory map (preliminary)

0000-3fff ROM 0
4000-7fff ROM 1
8000-cfff ROM 2
c000-dfff ROM 3
e000-e7ff RAM
e800-e9ff spriteram
f000-f7ff tiles
f800-f8ff palette ram

IN0

bit 0 : left coin
bit 1 : right coin
bit 2 : 1 player
bit 3 : 2 player
bit 4 : ?
bit 5 : tilt
bit 6 : ?
bit 7 : service

IN1,IN2

joystick, sometimes spinner

IN3

Usually dipswitches. Most game configuration is really done by holding down
the service key (F2) during the startup self-test.

IN4

extra inputs; also used to control sound for external boards


The MCR/III games used a plethora of sound boards; all of them seem to do
roughly the same thing.  We have:

* Chip Squeak Deluxe (CSD) - used for music on Spy Hunter
* Turbo Chip Squeak (TCS) - used for all sound effects on Sarge
* Sounds Good (SG) - used for all sound effects on Rampage and Xenophobe
* Squawk & Talk (SNT) - used for speech on Discs of Tron

Known issues:

* Destruction Derby has no sound
* Destruction Derby player 3 and 4 steering wheels are not properly muxed

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/6821pia.h"

void mcr3_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void mcr3_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void mcr3_videoram_w(int offset,int data);
void mcr3_paletteram_w(int offset,int data);

void rampage_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void spyhunt_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int spyhunt_vh_start(void);
void spyhunt_vh_stop(void);
void spyhunt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern unsigned char *spyhunt_alpharam;
extern int spyhunt_alpharam_size;

int crater_vh_start(void);

void mcr_init_machine(void);
int mcr_interrupt(void);
int dotron_interrupt(void);
extern int mcr_loadnvram;

void mcr_writeport(int port,int value);
int mcr_readport(int port);
void mcr_soundstatus_w (int offset,int data);
int mcr_soundlatch_r (int offset);
void mcr_pia_1_w (int offset, int data);
int mcr_pia_1_r (int offset);

int destderb_port_r(int offset);

void spyhunt_init_machine(void);
int spyhunt_port_1_r(int offset);
int spyhunt_port_2_r(int offset);
void spyhunt_writeport(int port,int value);

void rampage_init_machine(void);
void rampage_writeport(int port,int value);

void maxrpm_writeport(int port,int value);
int maxrpm_IN1_r(int offset);
int maxrpm_IN2_r(int offset);

void sarge_init_machine(void);
int sarge_IN1_r(int offset);
int sarge_IN2_r(int offset);
void sarge_writeport(int port,int value);

int dotron_vh_start(void);
void dotron_vh_stop(void);
int dotron_IN2_r(int offset);
void dotron_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void dotron_init_machine(void);
void dotron_writeport(int port,int value);

void crater_writeport(int port,int value);


/***************************************************************************

  Memory maps

***************************************************************************/

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xe9ff, MRA_RAM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xe000, 0xe7ff, MWA_RAM },
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe800, 0xe9ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf000, 0xf7ff, mcr3_videoram_w, &videoram, &videoram_size },
	{ 0xf800, 0xf8ff, mcr3_paletteram_w, &paletteram },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress rampage_readmem[] =
{
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xebff, MRA_RAM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress rampage_writemem[] =
{
	{ 0xe000, 0xe7ff, MWA_RAM },
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe800, 0xebff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf000, 0xf7ff, mcr3_videoram_w, &videoram, &videoram_size },
	{ 0xec00, 0xecff, mcr3_paletteram_w, &paletteram },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress spyhunt_readmem[] =
{
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xebff, MRA_RAM },
	{ 0xf000, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress spyhunt_writemem[] =
{
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xe800, 0xebff, MWA_RAM, &spyhunt_alpharam, &spyhunt_alpharam_size },
	{ 0xe000, 0xe7ff, videoram_w, &videoram, &videoram_size },
	{ 0xf800, 0xf9ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xfa00, 0xfaff, mcr3_paletteram_w, &paletteram },
	{ 0x0000, 0xdfff, MWA_ROM },
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

static struct MemoryReadAddress timber_sound_readmem[] =
{
	{ 0x3000, 0x3fff, MRA_RAM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x9000, 0x9003, mcr_soundlatch_r },
	{ 0xa001, 0xa001, AY8910_read_port_0_r },
	{ 0xb001, 0xb001, AY8910_read_port_1_r },
	{ 0xf000, 0xf000, input_port_5_r },
	{ 0xe000, 0xe000, MRA_NOP },
	{ 0x0000, 0x2fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress timber_sound_writemem[] =
{
	{ 0x3000, 0x3fff, MWA_RAM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0xa000, 0xa000, AY8910_control_port_0_w },
	{ 0xa002, 0xa002, AY8910_write_port_0_w },
	{ 0xb000, 0xb000, AY8910_control_port_1_w },
	{ 0xb002, 0xb002, AY8910_write_port_1_w },
	{ 0xc000, 0xc000, mcr_soundstatus_w },
	{ 0xe000, 0xe000, MWA_NOP },
	{ 0x0000, 0x2fff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress csd_readmem[] =
{
	{ 0x000000, 0x007fff, MRA_ROM },
	{ 0x018000, 0x018007, mcr_pia_1_r },
	{ 0x01c000, 0x01cfff, MRA_BANK1 },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress csd_writemem[] =
{
	{ 0x000000, 0x007fff, MWA_ROM },
	{ 0x018000, 0x018007, mcr_pia_1_w },
	{ 0x01c000, 0x01cfff, MWA_BANK1 },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sg_readmem[] =
{
	{ 0x000000, 0x01ffff, MRA_ROM },
	{ 0x060000, 0x060007, mcr_pia_1_r },
	{ 0x070000, 0x070fff, MRA_BANK1 },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sg_writemem[] =
{
	{ 0x000000, 0x01ffff, MWA_ROM },
	{ 0x060000, 0x060007, mcr_pia_1_w },
	{ 0x070000, 0x070fff, MWA_BANK1 },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress snt_readmem[] =
{
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x0080, 0x0083, pia_1_r },
	{ 0x0090, 0x0093, pia_2_r },
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress snt_writemem[] =
{
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0x0080, 0x0083, pia_1_w },
	{ 0x0090, 0x0093, pia_2_w },
	{ 0xd000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress tcs_readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x6000, 0x6003, pia_1_r },
	{ 0xc000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress tcs_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x6000, 0x6003, pia_1_w },
	{ 0xc000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



/***************************************************************************

  Input port definitions

***************************************************************************/

INPUT_PORTS_START( tapper_input_ports )
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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_DIPNAME( 0x04, 0x04, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_BIT( 0xbb, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( dotron_input_ports )
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

	PORT_START	/* IN1 */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 50, 0, 0, 0, OSD_KEY_Z, OSD_KEY_X, 0, 0, 4 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON3, "Aim Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON4, "Aim Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	/* we default to Environmental otherwise speech is disabled */
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Environmental" )

	PORT_START	/* IN3 -- dipswitches */
	PORT_DIPNAME( 0x01, 0x01, "Coin Meters", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* fake port to make aiming up & down easier */
	PORT_ANALOG ( 0xff, 0x00, IPT_TRACKBALL_Y, 100, 0, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( destderb_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 -- the high 6 bits contain the sterring wheel value */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_ANALOG ( 0xfc, 0x00, IPT_DIAL | IPF_REVERSE, 50, 0, 0, 0 )

	PORT_START	/* IN2 -- the high 6 bits contain the sterring wheel value */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_ANALOG ( 0xfc, 0x00, IPT_DIAL | IPF_REVERSE, 50, 0, 0, 0 )

	PORT_START	/* IN3 -- dipswitches */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "2P Upright" )
	PORT_DIPSETTING(    0x00, "4P Upright" )
	PORT_DIPNAME( 0x02, 0x02, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Harder" )
	PORT_DIPNAME( 0x04, 0x04, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Reward Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Expanded" )
	PORT_DIPSETTING(    0x00, "Limited" )
	PORT_DIPNAME( 0x30, 0x30, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/2 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x10, "1 Coin/2 Credits" )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( timber_input_ports )
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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_DIPNAME( 0x04, 0x04, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0xfb, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( rampage_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x04, 0x04, "Score Option", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Keep score when continuing" )
	PORT_DIPSETTING(    0x00, "Lose score when continuing" )
	PORT_DIPNAME( 0x08, 0x08, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x70, 0x70, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x70, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x60, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x50, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/6 Credits" )
	PORT_BITX( 0x80,    0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Advance", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( maxrpm_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN1 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START	/* IN3 -- dipswitches */
	PORT_DIPNAME( 0x08, 0x08, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x30, 0x30, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x10, "1 Coin/2 Credits" )
/* 0x00 says 2 Coins/2 Credits in service mode, but gives 1 Coin/1 Credit */
	PORT_BIT( 0xc7, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* new fake for acceleration */
	PORT_ANALOGX( 0xff, 0x30, IPT_AD_STICK_Y | IPF_PLAYER2, 100, 0, 0x30, 0xff, OSD_KEY_UP, OSD_KEY_DOWN, IPT_JOYSTICK_UP, IPT_JOYSTICK_DOWN, 1 )

	PORT_START	/* new fake for acceleration */
	PORT_ANALOGX( 0xff, 0x30, IPT_AD_STICK_Y | IPF_PLAYER1, 100, 0, 0x30, 0xff, OSD_KEY_UP, OSD_KEY_DOWN, IPT_JOYSTICK_UP, IPT_JOYSTICK_DOWN, 1 )

	PORT_START	/* new fake for steering */
	PORT_ANALOGX( 0xff, 0x74, IPT_AD_STICK_X | IPF_PLAYER2 | IPF_REVERSE, 80, 0, 0x34, 0xb4, OSD_KEY_LEFT, OSD_KEY_RIGHT, IPT_JOYSTICK_LEFT, IPT_JOYSTICK_RIGHT, 2 )

	PORT_START	/* new fake for steering */
	PORT_ANALOGX( 0xff, 0x74, IPT_AD_STICK_X | IPF_PLAYER1 | IPF_REVERSE, 80, 0, 0x34, 0xb4, OSD_KEY_LEFT, OSD_KEY_RIGHT, IPT_JOYSTICK_LEFT, IPT_JOYSTICK_RIGHT, 2 )

	PORT_START	/* fake for shifting */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( sarge_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_2WAY | IPF_PLAYER1 )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )

	PORT_START	/* IN3 -- dipswitches */
	PORT_DIPNAME( 0x08, 0x08, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x30, 0x30, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x10, "1 Coin/2 Credits" )
/* 0x00 says 2 Coins/2 Credits in service mode, but gives 1 Coin/1 Credit */
	PORT_BIT( 0xc7, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* fake port for single joystick control */
	/* This fake port is handled via sarge_IN1_r and sarge_IN2_r */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
INPUT_PORTS_END


INPUT_PORTS_START( spyhunt_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_TOGGLE, "Gear Shift", OSD_KEY_ENTER, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_KEY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN1 -- various buttons, low 5 bits */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4, "Oil Slick", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_BUTTON5, "Missiles", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3, "Weapon Truck", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2, "Smoke Screen", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1, "Machine Guns", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT(  0x60, IP_ACTIVE_HIGH, IPT_UNUSED ) /* CSD status bits */
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 -- actually not used at all, but read as a trakport */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches -- low 4 bits only */
	PORT_DIPNAME( 0x01, 0x01, "Game Timer", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1:00" )
	PORT_DIPSETTING(    0x01, "1:30" )
	PORT_DIPNAME( 0x02, 0x02, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* new fake for acceleration */
	PORT_ANALOGX( 0xff, 0x30, IPT_AD_STICK_Y | IPF_REVERSE, 100, 0, 0x30, 0xff, OSD_KEY_UP, OSD_KEY_DOWN, OSD_JOY_UP, OSD_JOY_DOWN, 1 )

	PORT_START	/* new fake for steering */
	PORT_ANALOGX( 0xff, 0x74, IPT_AD_STICK_X | IPF_CENTER, 80, 0, 0x34, 0xb4, OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_JOY_LEFT, OSD_JOY_RIGHT, 2 )
INPUT_PORTS_END



INPUT_PORTS_START( crater_input_ports )
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
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 25, 0, 0, 0, OSD_KEY_Z, OSD_KEY_X, 0, 0, 4 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 -- dipswitches */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN4 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* AIN0 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


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

static struct IOReadPort dt_readport[] =
{
   { 0x00, 0x00, input_port_0_r },
   { 0x01, 0x01, input_port_1_r },
   { 0x02, 0x02, dotron_IN2_r },
   { 0x03, 0x03, input_port_3_r },
   { 0x04, 0x04, input_port_4_r },
   { 0x05, 0xff, mcr_readport },
   { -1 }
};

static struct IOReadPort destderb_readport[] =
{
   { 0x00, 0x00, input_port_0_r },
   { 0x01, 0x02, destderb_port_r },
   { 0x03, 0x03, input_port_3_r },
   { 0x04, 0x04, input_port_4_r },
   { 0x05, 0xff, mcr_readport },
   { -1 }
};

static struct IOReadPort maxrpm_readport[] =
{
   { 0x00, 0x00, input_port_0_r },
   { 0x01, 0x01, maxrpm_IN1_r },
   { 0x02, 0x02, maxrpm_IN2_r },
   { 0x03, 0x03, input_port_3_r },
   { 0x04, 0x04, input_port_4_r },
   { 0x05, 0xff, mcr_readport },
   { -1 }
};

static struct IOReadPort sarge_readport[] =
{
   { 0x00, 0x00, input_port_0_r },
   { 0x01, 0x01, sarge_IN1_r },
   { 0x02, 0x02, sarge_IN2_r },
   { 0x03, 0x03, input_port_3_r },
   { 0x04, 0x04, input_port_4_r },
   { 0x05, 0xff, mcr_readport },
   { -1 }
};

static struct IOReadPort spyhunt_readport[] =
{
   { 0x00, 0x00, input_port_0_r },
   { 0x01, 0x01, spyhunt_port_1_r },
   { 0x02, 0x02, spyhunt_port_2_r },
   { 0x03, 0x03, input_port_3_r },
   { 0x04, 0x04, input_port_4_r },
   { 0x05, 0xff, mcr_readport },
   { -1 }
};

static struct IOWritePort writeport[] =
{
   { 0, 0xff, mcr_writeport },
   { -1 }	/* end of table */
};

static struct IOWritePort sh_writeport[] =
{
   { 0, 0xff, spyhunt_writeport },
   { -1 }	/* end of table */
};

static struct IOWritePort cr_writeport[] =
{
   { 0, 0xff, crater_writeport },
   { -1 }	/* end of table */
};

static struct IOWritePort rm_writeport[] =
{
   { 0, 0xff, rampage_writeport },
   { -1 }	/* end of table */
};

static struct IOWritePort mr_writeport[] =
{
   { 0, 0xff, maxrpm_writeport },
   { -1 }	/* end of table */
};

static struct IOWritePort sa_writeport[] =
{
   { 0, 0xff, sarge_writeport },
   { -1 }	/* end of table */
};

static struct IOWritePort dt_writeport[] =
{
   { 0, 0xff, dotron_writeport },
   { -1 }	/* end of table */
};


/***************************************************************************

  Graphics layouts

***************************************************************************/

/* generic character layouts */

/* note that characters are half the resolution of sprites in each direction, so we generate
   them at double size */

/* 1024 characters; used by tapper, timber, rampage */
static struct GfxLayout mcr3_charlayout_1024 =
{
	16, 16,
	1024,
	4,
	{ 1024*16*8, 1024*16*8+1, 0, 1 },
	{ 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14 },
	{ 0, 0, 2*8, 2*8, 4*8, 4*8, 6*8, 6*8, 8*8, 8*8, 10*8, 10*8, 12*8, 12*8, 14*8, 14*8 },
	16*8
};

/* 512 characters; used by dotron, destderb */
static struct GfxLayout mcr3_charlayout_512 =
{
	16, 16,
	512,
	4,
	{ 512*16*8, 512*16*8+1, 0, 1 },
	{ 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14 },
	{ 0, 0, 2*8, 2*8, 4*8, 4*8, 6*8, 6*8, 8*8, 8*8, 10*8, 10*8, 12*8, 12*8, 14*8, 14*8 },
	16*8
};

/* generic sprite layouts */

/* 512 sprites; used by rampage */
#define X (512*128*8)
#define Y (2*X)
#define Z (3*X)
static struct GfxLayout mcr3_spritelayout_512 =
{
   32,32,
   512,
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

/* 256 sprites; used by tapper, timber, destderb, spyhunt */
#define X (256*128*8)
#define Y (2*X)
#define Z (3*X)
static struct GfxLayout mcr3_spritelayout_256 =
{
   32,32,
   256,
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

/* 128 sprites; used by dotron */
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

/***************************** spyhunt layouts **********************************/

/* 128 32x16 characters; used by spyhunt */
static struct GfxLayout spyhunt_charlayout_128 =
{
	32, 32,	/* we pixel double and split in half */
	128,
	4,
	{ 0, 1, 128*128*8, 128*128*8+1  },
	{ 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14, 16, 16, 18, 18, 20, 20, 22, 22, 24, 24, 26, 26, 28, 28, 30, 30 },
	{ 0, 0, 8*8, 8*8, 16*8, 16*8, 24*8, 24*8, 32*8, 32*8, 40*8, 40*8, 48*8, 48*8, 56*8, 56*8,
	  64*8, 64*8, 72*8, 72*8, 80*8, 80*8, 88*8, 88*8, 96*8, 96*8, 104*8, 104*8, 112*8, 112*8, 120*8, 120*8 },
	128*8
};

/* of course, Spy Hunter just *had* to be different than everyone else... */
static struct GfxLayout spyhunt_alphalayout =
{
	16, 16,
	256,
	2,
	{ 0, 1 },
	{ 0, 0, 2, 2, 4, 4, 6, 6, 8, 8, 10, 10, 12, 12, 14, 14 },
	{ 0, 0, 2*8, 2*8, 4*8, 4*8, 6*8, 6*8, 8*8, 8*8, 10*8, 10*8, 12*8, 12*8, 14*8, 14*8 },
	16*8
};



static struct GfxDecodeInfo tapper_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mcr3_charlayout_1024,   0, 4 },
	{ 1, 0x8000, &mcr3_spritelayout_256,  0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo dotron_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mcr3_charlayout_512,   0, 4 },
	{ 1, 0x4000, &mcr3_spritelayout_128, 0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo destderb_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mcr3_charlayout_512,   0, 4 },
	{ 1, 0x4000, &mcr3_spritelayout_256, 0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo timber_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mcr3_charlayout_1024,   0, 4 },
	{ 1, 0x8000, &mcr3_spritelayout_256,  0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo rampage_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mcr3_charlayout_1024,  0, 4 },
	{ 1, 0x8000, &mcr3_spritelayout_512, 0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo sarge_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &mcr3_charlayout_512,   0, 4 },
	{ 1, 0x4000, &mcr3_spritelayout_256, 0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo spyhunt_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &spyhunt_charlayout_128, 1*16, 1 },	/* top half */
	{ 1, 0x0004, &spyhunt_charlayout_128, 1*16, 1 },	/* bottom half */
	{ 1, 0x8000, &mcr3_spritelayout_256,  0*16, 1 },
	{ 1, 0x28000, &spyhunt_alphalayout,   8*16, 1 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo crater_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &spyhunt_charlayout_128, 3*16, 1 },	/* top half */
	{ 1, 0x0004, &spyhunt_charlayout_128, 3*16, 1 },	/* bottom half */
	{ 1, 0x8000, &mcr3_spritelayout_256,  0*16, 4 },
	{ 1, 0x28000, &spyhunt_alphalayout,   8*16, 1 },
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

static struct DACinterface dac_interface =
{
	1,
	{ 255 }
};

static struct TMS5220interface tms5220_interface =
{
	640000,
	192,
	0
};



/***************************************************************************

  Machine drivers

***************************************************************************/

static struct MachineDriver tapper_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5 Mhz */
			0,
			readmem,writemem,readport,writeport,
			mcr_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,
			sound_readmem,sound_writemem,0,0,
			interrupt,26
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - sound CPU has enough interrupts to handle synchronization */
	mcr_init_machine,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	tapper_gfxdecodeinfo,
	8*16, 8*16,
	mcr3_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	mcr3_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

static struct MachineDriver dotron_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5 Mhz */
			0,
			readmem,writemem,dt_readport,dt_writeport,
			dotron_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,
			sound_readmem,sound_writemem,0,0,
			interrupt,26
		},
		{
			CPU_M6802 | CPU_AUDIO_CPU,
			3580000/4,	/* .8 Mhz */
			3,
			snt_readmem,snt_writemem,0,0,
			ignore_interrupt,1
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - sound CPU has enough interrupts to handle synchronization */
	dotron_init_machine,

	/* video hardware */
/*	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 }, - MAB 09/30/98, changed the screen size for the backdrop */
	800, 600, { 0, 800-1, 0, 600-1 },
	dotron_gfxdecodeinfo,
	254, 4*16,		/* The extra colors are for the backdrop */
	mcr3_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	dotron_vh_start,
	dotron_vh_stop,
	dotron_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_TMS5220,
			&tms5220_interface
		}
	}
};

static struct MachineDriver destderb_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5 Mhz */
			0,
			readmem,writemem,destderb_readport,writeport,
			mcr_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,
			sound_readmem,sound_writemem,0,0,
			interrupt,26
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - sound CPU has enough interrupts to handle synchronization */
	mcr_init_machine,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	destderb_gfxdecodeinfo,
	8*16, 8*16,
	mcr3_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	mcr3_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

static struct MachineDriver timber_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5 Mhz */
			0,
			readmem,writemem,readport,writeport,
			mcr_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,
			timber_sound_readmem,timber_sound_writemem,0,0,
			interrupt,26
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - sound CPU has enough interrupts to handle synchronization */
	mcr_init_machine,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	timber_gfxdecodeinfo,
	8*16, 8*16,
	mcr3_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	mcr3_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

static struct MachineDriver rampage_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5 Mhz */
			0,
			rampage_readmem,rampage_writemem,readport,rm_writeport,
			mcr_interrupt,1
		},
		{
			CPU_M68000 | CPU_AUDIO_CPU,
			7500000,	/* 7.5 Mhz */
			2,
			sg_readmem,sg_writemem,0,0,
			ignore_interrupt,1
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - sound CPU synchronization is done via timers */
	rampage_init_machine,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	rampage_gfxdecodeinfo,
	8*16, 8*16,
	mcr3_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	rampage_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver maxrpm_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5 Mhz */
			0,
			rampage_readmem,rampage_writemem,maxrpm_readport,mr_writeport,
			mcr_interrupt,1
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - sound CPU synchronization is done via timers */
	rampage_init_machine,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	rampage_gfxdecodeinfo,
	8*16, 8*16,
	mcr3_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	rampage_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver sarge_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5 Mhz */
			0,
			rampage_readmem,rampage_writemem,sarge_readport,sa_writeport,
			mcr_interrupt,1
		},
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			2250000,	/* 2.25 Mhz??? */
			2,
			tcs_readmem,tcs_writemem,0,0,
			ignore_interrupt,1
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - sound CPU synchronization is done via timers */
	sarge_init_machine,

	/* video hardware */
	32*16, 30*16, { 0, 32*16-1, 0, 30*16-1 },
	sarge_gfxdecodeinfo,
	8*16, 8*16,
	mcr3_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	rampage_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

static struct MachineDriver spyhunt_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5 Mhz */
			0,
			spyhunt_readmem,spyhunt_writemem,spyhunt_readport,sh_writeport,
			mcr_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,
			sound_readmem,sound_writemem,0,0,
			interrupt,26
		},
		{
			CPU_M68000 | CPU_AUDIO_CPU,
			7500000,	/* Actually 7.5 Mhz, but the 68000 emulator isn't accurate */
			3,
			csd_readmem,csd_writemem,0,0,
			ignore_interrupt,1
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - sound CPU has enough interrupts to handle synchronization */
	spyhunt_init_machine,

	/* video hardware */
	31*16, 30*16, { 0, 31*16-1, 0, 30*16-1 },
	spyhunt_gfxdecodeinfo,
	8*16+4, 8*16+4,
	spyhunt_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	spyhunt_vh_start,
	spyhunt_vh_stop,
	spyhunt_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
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


static struct MachineDriver crater_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5 Mhz */
			0,
			spyhunt_readmem,spyhunt_writemem,readport,cr_writeport,
			mcr_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			2,
			sound_readmem,sound_writemem,0,0,
			interrupt,26
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - sound CPU has enough interrupts to handle synchronization */
	mcr_init_machine,

	/* video hardware */
	30*16, 30*16, { 0, 30*16-1, 0, 30*16-1 },
	crater_gfxdecodeinfo,
	8*16+4, 8*16+4,
	spyhunt_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	crater_vh_start,
	spyhunt_vh_stop,
	spyhunt_vh_screenrefresh,

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

static int mcr3_hiload(int addr, int len)
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

static void mcr3_hisave(int addr, int len)
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

static int  tapper_hiload(void)   { return mcr3_hiload(0xe000, 0x9d); }
static void tapper_hisave(void)   {        mcr3_hisave(0xe000, 0x9d); }

static int  dotron_hiload(void)   { return mcr3_hiload(0xe543, 0xac); }
static void dotron_hisave(void)   {        mcr3_hisave(0xe543, 0xac); }

static int  destderb_hiload(void) { return mcr3_hiload(0xe4e6, 0x153); }
static void destderb_hisave(void) {        mcr3_hisave(0xe4e6, 0x153); }

static int  timber_hiload(void)   { return mcr3_hiload(0xe000, 0x9f); }
static void timber_hisave(void)   {        mcr3_hisave(0xe000, 0x9f); }

static int  rampage_hiload(void)  { return mcr3_hiload(0xe631, 0x3f); }
static void rampage_hisave(void)  {        mcr3_hisave(0xe631, 0x3f); }

static int  spyhunt_hiload(void)  { return mcr3_hiload(0xf42b, 0xfb); }
static void spyhunt_hisave(void)  {        mcr3_hisave(0xf42b, 0xfb); }

static int  crater_hiload(void)   { return mcr3_hiload(0xf5fb, 0xa3); }
static void crater_hisave(void)   {        mcr3_hisave(0xf5fb, 0xa3); }



/***************************************************************************

  ROM decoding

***************************************************************************/

static void spyhunt_decode (void)
{
   unsigned char *RAM = Machine->memory_region[0];


	/* some versions of rom 11d have the top and bottom 8k swapped; to enable us to work with either
	   a correct set or a swapped set (both of which pass the checksum!), we swap them here */
	if (RAM[0xa000] != 0x0c)
	{
		int i;
		unsigned char temp;

		for (i = 0;i < 0x2000;i++)
		{
			temp = RAM[0xa000 + i];
			RAM[0xa000 + i] = RAM[0xc000 + i];
			RAM[0xc000 + i] = temp;
		}
	}
}


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tapper_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "tappg0.bin",   0x0000, 0x4000, 0x127171d1 )
	ROM_LOAD( "tappg1.bin",   0x4000, 0x4000, 0x9d6a47f7 )
	ROM_LOAD( "tappg2.bin",   0x8000, 0x4000, 0x3a1f8778 )
	ROM_LOAD( "tappg3.bin",   0xc000, 0x2000, 0xe8dcdaa4 )

	ROM_REGION_DISPOSE(0x28000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tapbg1.bin",   0x00000, 0x4000, 0x2a30238c )
	ROM_LOAD( "tapbg0.bin",   0x04000, 0x4000, 0x394ab576 )
	ROM_LOAD( "tapfg7.bin",   0x08000, 0x4000, 0x070b4c81 )
	ROM_LOAD( "tapfg6.bin",   0x0c000, 0x4000, 0xa37aef36 )
	ROM_LOAD( "tapfg5.bin",   0x10000, 0x4000, 0x800f7c8a )
	ROM_LOAD( "tapfg4.bin",   0x14000, 0x4000, 0x32674ee6 )
	ROM_LOAD( "tapfg3.bin",   0x18000, 0x4000, 0x818fffd4 )
	ROM_LOAD( "tapfg2.bin",   0x1c000, 0x4000, 0x67e37690 )
	ROM_LOAD( "tapfg1.bin",   0x20000, 0x4000, 0x32509011 )
	ROM_LOAD( "tapfg0.bin",   0x24000, 0x4000, 0x8412c808 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "tapsnda7.bin", 0x0000, 0x1000, 0x0e8bb9d5 )
	ROM_LOAD( "tapsnda8.bin", 0x1000, 0x1000, 0x0cf0e29b )
	ROM_LOAD( "tapsnda9.bin", 0x2000, 0x1000, 0x31eb6dc6 )
	ROM_LOAD( "tapsda10.bin", 0x3000, 0x1000, 0x01a9be6a )
ROM_END

ROM_START( sutapper_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "5791",         0x0000, 0x4000, 0x87119cc4 )
	ROM_LOAD( "5792",         0x4000, 0x4000, 0x4c23ad89 )
	ROM_LOAD( "5793",         0x8000, 0x4000, 0xfecbf683 )
	ROM_LOAD( "5794",         0xc000, 0x2000, 0x5bdc1916 )

	ROM_REGION_DISPOSE(0x28000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5790",         0x00000, 0x4000, 0xac1558c1 )
	ROM_LOAD( "5789",         0x04000, 0x4000, 0xfa66cab5 )
	ROM_LOAD( "5801",         0x08000, 0x4000, 0xd70defa7 )
	ROM_LOAD( "5802",         0x0c000, 0x4000, 0xd4f114b9 )
	ROM_LOAD( "5799",         0x10000, 0x4000, 0x02c69432 )
	ROM_LOAD( "5800",         0x14000, 0x4000, 0xebf1f948 )
	ROM_LOAD( "5797",         0x18000, 0x4000, 0xf10a1d05 )
	ROM_LOAD( "5798",         0x1c000, 0x4000, 0x614990cd )
	ROM_LOAD( "5795",         0x20000, 0x4000, 0x5d987c92 )
	ROM_LOAD( "5796",         0x24000, 0x4000, 0xde5700b4 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5788",         0x0000, 0x1000, 0x5c1d0982 )
	ROM_LOAD( "5787",         0x1000, 0x1000, 0x09e74ed8 )
	ROM_LOAD( "5786",         0x2000, 0x1000, 0xc3e98284 )
	ROM_LOAD( "5785",         0x3000, 0x1000, 0xced2fd47 )
ROM_END

ROM_START( rbtapper_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "rbtpg0.bin",   0x0000, 0x4000, 0x20b9adf4 )
	ROM_LOAD( "rbtpg1.bin",   0x4000, 0x4000, 0x87e616c2 )
	ROM_LOAD( "rbtpg2.bin",   0x8000, 0x4000, 0x0b332c97 )
	ROM_LOAD( "rbtpg3.bin",   0xc000, 0x2000, 0x698c06f2 )

	ROM_REGION_DISPOSE(0x28000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "rbtbg1.bin",   0x00000, 0x4000, 0x44dfa483 )
	ROM_LOAD( "rbtbg0.bin",   0x04000, 0x4000, 0x510b13de )
	ROM_LOAD( "rbtfg7.bin",   0x08000, 0x4000, 0x8dbf0c36 )
	ROM_LOAD( "rbtfg6.bin",   0x0c000, 0x4000, 0x441201a0 )
	ROM_LOAD( "rbtfg5.bin",   0x10000, 0x4000, 0x9eeca46e )
	ROM_LOAD( "rbtfg4.bin",   0x14000, 0x4000, 0x8c79e7d7 )
	ROM_LOAD( "rbtfg3.bin",   0x18000, 0x4000, 0x3e725e77 )
	ROM_LOAD( "rbtfg2.bin",   0x1c000, 0x4000, 0x4ee8b624 )
	ROM_LOAD( "rbtfg1.bin",   0x20000, 0x4000, 0x1c0b8791 )
	ROM_LOAD( "rbtfg0.bin",   0x24000, 0x4000, 0xe99f6018 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "rbtsnda7.bin", 0x0000, 0x1000, 0x5c1d0982 )
	ROM_LOAD( "rbtsnda8.bin", 0x1000, 0x1000, 0x09e74ed8 )
	ROM_LOAD( "rbtsnda9.bin", 0x2000, 0x1000, 0xc3e98284 )
	ROM_LOAD( "rbtsda10.bin", 0x3000, 0x1000, 0xced2fd47 )
ROM_END

struct GameDriver tapper_driver =
{
	__FILE__,
	0,
	"tapper",
	"Tapper (Budweiser)",
	"1983",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria",
	0,
	&tapper_machine_driver,
	0,

	tapper_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tapper_input_ports,

	0, 0,0,
	ORIENTATION_DEFAULT,

	tapper_hiload, tapper_hisave
};

struct GameDriver sutapper_driver =
{
	__FILE__,
	&tapper_driver,
	"sutapper",
	"Tapper (Suntory)",
	"1983",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria",
	0,
	&tapper_machine_driver,
	0,

	sutapper_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tapper_input_ports,

	0, 0,0,
	ORIENTATION_DEFAULT,

	tapper_hiload, tapper_hisave
};

struct GameDriver rbtapper_driver =
{
	__FILE__,
	&tapper_driver,
	"rbtapper",
	"Tapper (Root Beer)",
	"1984",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria",
	0,
	&tapper_machine_driver,
	0,

	rbtapper_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tapper_input_ports,

	0, 0,0,
	ORIENTATION_DEFAULT,

	tapper_hiload, tapper_hisave
};


ROM_START( dotron_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "loc-pg0.1c",   0x0000, 0x4000, 0xba0da15f )
	ROM_LOAD( "loc-pg1.2c",   0x4000, 0x4000, 0xdc300191 )
	ROM_LOAD( "loc-pg2.3c",   0x8000, 0x4000, 0xab0b3800 )
	ROM_LOAD( "loc-pg1.4c",   0xc000, 0x2000, 0xf98c9f8e )

	ROM_REGION_DISPOSE(0x14000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "loc-bg2.6f",   0x00000, 0x2000, 0x40167124 )
	ROM_LOAD( "loc-bg1.5f",   0x02000, 0x2000, 0xbb2d7a5d )
	ROM_LOAD( "loc-a.cp0",    0x04000, 0x2000, 0xb35f5374 )
	ROM_LOAD( "loc-b.cp9",    0x06000, 0x2000, 0x565a5c48 )
	ROM_LOAD( "loc-c.cp8",    0x08000, 0x2000, 0xef45d146 )
	ROM_LOAD( "loc-d.cp7",    0x0a000, 0x2000, 0x5e8a3ef3 )
	ROM_LOAD( "loc-e.cp6",    0x0c000, 0x2000, 0xce957f1a )
	ROM_LOAD( "loc-f.cp5",    0x0e000, 0x2000, 0xd26053ce )
	ROM_LOAD( "loc-g.cp4",    0x10000, 0x2000, 0x57a2b1ff )
	ROM_LOAD( "loc-h.cp3",    0x12000, 0x2000, 0x3bb4d475 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "sound0.a7",    0x0000, 0x1000, 0x6d39bf19 )
	ROM_LOAD( "sound1.a8",    0x1000, 0x1000, 0xac872e1d )
	ROM_LOAD( "sound2.a9",    0x2000, 0x1000, 0xe8ef6519 )
	ROM_LOAD( "sound3.a10",   0x3000, 0x1000, 0x6b5aeb02 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "pre.u3",       0xd000, 0x1000, 0xc3d0f762 )
	ROM_LOAD( "pre.u4",       0xe000, 0x1000, 0x7ca79b43 )
	ROM_LOAD( "pre.u5",       0xf000, 0x1000, 0x24e9618e )
ROM_END

ROM_START( dotrone_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "loc-cpu1",     0x0000, 0x4000, 0xeee31b8c )
	ROM_LOAD( "loc-cpu2",     0x4000, 0x4000, 0x75ba6ad3 )
	ROM_LOAD( "loc-cpu3",     0x8000, 0x4000, 0x94bb1a0e )
	ROM_LOAD( "loc-cpu4",     0xc000, 0x2000, 0xc137383c )

	ROM_REGION_DISPOSE(0x14000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "loc-bg2.6f",   0x00000, 0x2000, 0x40167124 )
	ROM_LOAD( "loc-bg1.5f",   0x02000, 0x2000, 0xbb2d7a5d )
	ROM_LOAD( "loc-a.cp0",    0x04000, 0x2000, 0xb35f5374 )
	ROM_LOAD( "loc-b.cp9",    0x06000, 0x2000, 0x565a5c48 )
	ROM_LOAD( "loc-c.cp8",    0x08000, 0x2000, 0xef45d146 )
	ROM_LOAD( "loc-d.cp7",    0x0a000, 0x2000, 0x5e8a3ef3 )
	ROM_LOAD( "loc-e.cp6",    0x0c000, 0x2000, 0xce957f1a )
	ROM_LOAD( "loc-f.cp5",    0x0e000, 0x2000, 0xd26053ce )
	ROM_LOAD( "loc-g.cp4",    0x10000, 0x2000, 0x57a2b1ff )
	ROM_LOAD( "loc-h.cp3",    0x12000, 0x2000, 0x3bb4d475 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "loc-a",        0x0000, 0x1000, 0x2de6a8a8 )
	ROM_LOAD( "loc-b",        0x1000, 0x1000, 0x4097663e )
	ROM_LOAD( "loc-c",        0x2000, 0x1000, 0xf576b9e7 )
	ROM_LOAD( "loc-d",        0x3000, 0x1000, 0x74b0059e )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "pre.u3",       0xd000, 0x1000, 0xc3d0f762 )
	ROM_LOAD( "pre.u4",       0xe000, 0x1000, 0x7ca79b43 )
	ROM_LOAD( "pre.u5",       0xf000, 0x1000, 0x24e9618e )
ROM_END


struct GameDriver dotron_driver =
{
	__FILE__,
	0,
	"dotron",
	"Discs of Tron (Upright)",
	"1983",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria\nAlan J. McCormick (speech info)\nMathis Rosenhauer(backdrop support)\nMike Balfour(backdrop support)\nBrandon Kirkpatrick (backdrop)",
	0,
	&dotron_machine_driver,
	0,

	dotron_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	dotron_input_ports,

	0, 0,0,
	ORIENTATION_FLIP_X,

	dotron_hiload, dotron_hisave
};

struct GameDriver dotrone_driver =
{
	__FILE__,
	&dotron_driver,
	"dotrone",
	"Discs of Tron (Environmental)",
	"1983",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria\nAlan J. McCormick (speech info)\nMathis Rosenhauer(backdrop support)\nMike Balfour(backdrop support)\nBrandon Kirkpatrick (backdrop)",
	0,
	&dotron_machine_driver,
	0,

	dotrone_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	dotron_input_ports,

	0, 0,0,
	ORIENTATION_FLIP_X,

	dotron_hiload, dotron_hisave
};



ROM_START( destderb_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "dd_pro",       0x0000, 0x4000, 0x8781b367 )
	ROM_LOAD( "dd_pro1",      0x4000, 0x4000, 0x4c713bfe )
	ROM_LOAD( "dd_pro2",      0x8000, 0x4000, 0xc2cbd2a4 )

	ROM_REGION_DISPOSE(0x24000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "dd_bg0.6f",    0x00000, 0x2000, 0xcf80be19 )
	ROM_LOAD( "dd_bg1.5f",    0x02000, 0x2000, 0x4e173e52 )
	ROM_LOAD( "dd_fg-3.a10",  0x04000, 0x4000, 0x801d9b86 )
	ROM_LOAD( "dd_fg-7.a9",   0x08000, 0x4000, 0x0ec3f60a )
	ROM_LOAD( "dd_fg-2.a8",   0x0c000, 0x4000, 0x6cab7b95 )
	ROM_LOAD( "dd_fg-6.a7",   0x10000, 0x4000, 0xabfb9a8b )
	ROM_LOAD( "dd_fg-1.a6",   0x14000, 0x4000, 0x70259651 )
	ROM_LOAD( "dd_fg-5.a5",   0x18000, 0x4000, 0x5fe99007 )
	ROM_LOAD( "dd_fg-0.a4",   0x1c000, 0x4000, 0xe57a4de6 )
	ROM_LOAD( "dd_fg-4.a3",   0x20000, 0x4000, 0x55aa667f )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "dd_ssio.a7",   0x0000, 0x1000, 0xc95cf31e )
	ROM_LOAD( "dd_ssio.a8",   0x1000, 0x1000, 0x12aaa48e )
ROM_END

struct GameDriver destderb_driver =
{
	__FILE__,
	0,
	"destderb",
	"Demolition Derby",
	"1984",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria\nBrad Oliver",
	0,
	&destderb_machine_driver,
	0,

	destderb_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	destderb_input_ports,

	0, 0,0,
	ORIENTATION_DEFAULT,

	destderb_hiload, destderb_hisave
};


ROM_START( timber_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "timpg0.bin",   0x0000, 0x4000, 0x377032ab )
	ROM_LOAD( "timpg1.bin",   0x4000, 0x4000, 0xfd772836 )
	ROM_LOAD( "timpg2.bin",   0x8000, 0x4000, 0x632989f9 )
	ROM_LOAD( "timpg3.bin",   0xc000, 0x2000, 0xdae8a0dc )

	ROM_REGION_DISPOSE(0x28000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "timbg1.bin",   0x00000, 0x4000, 0xb1cb2651 )
	ROM_LOAD( "timbg0.bin",   0x04000, 0x4000, 0x2ae352c4 )
	ROM_LOAD( "timfg7.bin",   0x08000, 0x4000, 0xd9c27475 )
	ROM_LOAD( "timfg6.bin",   0x0c000, 0x4000, 0x244778e8 )
	ROM_LOAD( "timfg5.bin",   0x10000, 0x4000, 0xeb636216 )
	ROM_LOAD( "timfg4.bin",   0x14000, 0x4000, 0xb7105eb7 )
	ROM_LOAD( "timfg3.bin",   0x18000, 0x4000, 0x37c03272 )
	ROM_LOAD( "timfg2.bin",   0x1c000, 0x4000, 0xe2c2885c )
	ROM_LOAD( "timfg1.bin",   0x20000, 0x4000, 0x81de4a73 )
	ROM_LOAD( "timfg0.bin",   0x24000, 0x4000, 0x7f3a4f59 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "tima7.bin",    0x0000, 0x1000, 0xc615dc3e )
	ROM_LOAD( "tima8.bin",    0x1000, 0x1000, 0x83841c87 )
	ROM_LOAD( "tima9.bin",    0x2000, 0x1000, 0x22bcdcd3 )
ROM_END

struct GameDriver timber_driver =
{
	__FILE__,
	0,
	"timber",
	"Timber",
	"1984",
	"Bally Midway",
	"Christopher Kirmse\nAaron Giles\nNicola Salmoria\nBrad Oliver",
	0,
	&timber_machine_driver,
	0,

	timber_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	timber_input_ports,

	0, 0,0,
	ORIENTATION_DEFAULT,

	timber_hiload, timber_hisave
};


ROM_START( rampage_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pro-0.rv3",    0x0000, 0x8000, 0x2f7ca03c )
	ROM_LOAD( "pro-1.rv3",    0x8000, 0x8000, 0xd89bd9a4 )

	ROM_REGION_DISPOSE(0x48000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bg-0",         0x00000, 0x04000, 0xc0d8b7a5 )
	ROM_LOAD( "bg-1",         0x04000, 0x04000, 0x2f6e3aa1 )
	ROM_LOAD( "fg-3",         0x08000, 0x10000, 0x81e1de40 )
	ROM_LOAD( "fg-2",         0x18000, 0x10000, 0x9489f714 )
	ROM_LOAD( "fg-1",         0x28000, 0x10000, 0x8728532b )
	ROM_LOAD( "fg-0",         0x38000, 0x10000, 0x0974be5d )

	ROM_REGION(0x20000)  /* 128k for the Sounds Good board */
	ROM_LOAD_EVEN( "ramp_u7.snd",  0x00000, 0x8000, 0xcffd7fa5 )
	ROM_LOAD_ODD ( "ramp_u17.snd", 0x00000, 0x8000, 0xe92c596b )
	ROM_LOAD_EVEN( "ramp_u8.snd",  0x10000, 0x8000, 0x11f787e4 )
	ROM_LOAD_ODD ( "ramp_u18.snd", 0x10000, 0x8000, 0x6b8bf5e1 )
ROM_END

void rampage_rom_decode (void)
{
	int i;

	/* Rampage tile graphics are inverted */
	for (i = 0; i < 0x8000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}

struct GameDriver rampage_driver =
{
	__FILE__,
	0,
	"rampage",
	"Rampage",
	"1986",
	"Bally Midway",
	"Aaron Giles\nChristopher Kirmse\nNicola Salmoria\nBrad Oliver",
	0,
	&rampage_machine_driver,
	0,

	rampage_rom,
	rampage_rom_decode, 0,
	0,
	0,	/* sound_prom */

	rampage_input_ports,

	0, 0,0,
	ORIENTATION_DEFAULT,

	rampage_hiload, rampage_hisave
};


ROM_START( powerdrv_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pdrv3b.bin",   0x0000, 0x8000, 0xd870b704 )
	ROM_LOAD( "pdrv5b.bin",   0x8000, 0x8000, 0xfa0544ad )

	ROM_REGION_DISPOSE(0x48000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pdrv15a.bin",  0x00000, 0x04000, 0xb858b5a8 )
	ROM_LOAD( "pdrv14b.bin",  0x04000, 0x04000, 0x12ee7fc2 )
	ROM_LOAD( "pdrv4e.bin",   0x08000, 0x10000, 0xde400335 )
	ROM_LOAD( "pdrv5e.bin",   0x18000, 0x10000, 0x4cb4780e )
	ROM_LOAD( "pdrv6e.bin",   0x28000, 0x10000, 0x1a1f7f81 )
	ROM_LOAD( "pdrv8e.bin",   0x38000, 0x10000, 0xdd3a2adc )

	ROM_REGION(0x20000)  /* 128k for the Sounds Good board */
	ROM_LOAD_EVEN( "pdsndu7.bin",  0x00000, 0x8000, 0x78713e78 )
	ROM_LOAD_ODD ( "pdsndu17.bin", 0x00000, 0x8000, 0xc41de6e4 )
	ROM_LOAD_EVEN( "pdsndu8.bin",  0x10000, 0x8000, 0x15714036 )
	ROM_LOAD_ODD ( "pdsndu18.bin", 0x10000, 0x8000, 0xcae14c70 )
ROM_END


struct GameDriver powerdrv_driver =
{
	__FILE__,
	0,
	"powerdrv",
	"Power Drive",
	"????",
	"Bally Midway",
	"Aaron Giles\nChristopher Kirmse\nNicola Salmoria\nBrad Oliver",
	0,
	&rampage_machine_driver,
	0,

	powerdrv_rom,
	rampage_rom_decode, 0,
	0,
	0,	/* sound_prom */

	rampage_input_ports,

	0, 0,0,
	ORIENTATION_DEFAULT,

	0, 0
};


ROM_START( maxrpm_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "pro.0",        0x00000, 0x8000, 0x3f9ec35f )
	ROM_LOAD( "pro.1",        0x08000, 0x6000, 0xf628bb30 )
	ROM_CONTINUE(             0x10000, 0x2000 )	/* unused? but there seems to be stuff in here */
								/* loading it at e000 causes rogue sprites to appear on screen */

	ROM_REGION_DISPOSE(0x48000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bg-0",         0x00000, 0x4000, 0xe3fb693a )
	ROM_LOAD( "bg-1",         0x04000, 0x4000, 0x50d1db6c )
	ROM_LOAD( "fg-3",         0x08000, 0x8000, 0x9ae3eb52 )
	ROM_LOAD( "fg-2",         0x18000, 0x8000, 0x38be8505 )
	ROM_LOAD( "fg-1",         0x28000, 0x8000, 0xe54b7f2a )
	ROM_LOAD( "fg-0",         0x38000, 0x8000, 0x1d1435c1 )
ROM_END

struct GameDriver maxrpm_driver =
{
	__FILE__,
	0,
	"maxrpm",
	"Max RPM",
	"1986",
	"Bally Midway",
	"Aaron Giles\nChristopher Kirmse\nNicola Salmoria\nBrad Oliver",
	0,
	&maxrpm_machine_driver,
	0,

	maxrpm_rom,
	rampage_rom_decode, 0,
	0,
	0,	/* sound_prom */

	maxrpm_input_ports,

	0, 0,0,
	ORIENTATION_DEFAULT,

	rampage_hiload, rampage_hisave
};


ROM_START( sarge_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "cpu_3b.bin",   0x0000, 0x8000, 0xda31a58f )
	ROM_LOAD( "cpu_5b.bin",   0x8000, 0x8000, 0x6800e746 )

	ROM_REGION_DISPOSE(0x24000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "til_15a.bin",  0x00000, 0x2000, 0x685001b8 )
	ROM_LOAD( "til_14b.bin",  0x02000, 0x2000, 0x8449eb45 )
	ROM_LOAD( "spr_4e.bin",   0x04000, 0x8000, 0xc382267d )
	ROM_LOAD( "spr_5e.bin",   0x0c000, 0x8000, 0xc832375c )
	ROM_LOAD( "spr_6e.bin",   0x14000, 0x8000, 0x7cc6fb28 )
	ROM_LOAD( "spr_8e.bin",   0x1c000, 0x8000, 0x93fac29d )

	ROM_REGION(0x10000)  /* 64k for the Turbo Cheap Squeak */
	ROM_LOAD( "tcs_u5.bin",   0xc000, 0x2000, 0xa894ef8a )
	ROM_LOAD( "tcs_u4.bin",   0xe000, 0x2000, 0x6ca6faf3 )
ROM_END

void sarge_rom_decode (void)
{
	int i;

	/* Sarge tile graphics are inverted */
	for (i = 0; i < 0x4000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}

struct GameDriver sarge_driver =
{
	__FILE__,
	0,
	"sarge",
	"Sarge",
	"1985",
	"Bally Midway",
	"Aaron Giles\nChristopher Kirmse\nNicola Salmoria\nBrad Oliver",
	0,
	&sarge_machine_driver,
	0,

	sarge_rom,
	sarge_rom_decode, 0,
	0,
	0,	/* sound_prom */

	sarge_input_ports,

	0, 0,0,
	ORIENTATION_DEFAULT,

	0, 0
};


ROM_START( spyhunt_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "cpu_pg0.6d",   0x0000, 0x2000, 0x1721b88f )
	ROM_LOAD( "cpu_pg1.7d",   0x2000, 0x2000, 0x909d044f )
	ROM_LOAD( "cpu_pg2.8d",   0x4000, 0x2000, 0xafeeb8bd )
	ROM_LOAD( "cpu_pg3.9d",   0x6000, 0x2000, 0x5e744381 )
	ROM_LOAD( "cpu_pg4.10d",  0x8000, 0x2000, 0xa3033c15 )
	ROM_LOAD( "cpu_pg5.11d",  0xA000, 0x4000, 0x88aa1e99 )

	ROM_REGION_DISPOSE(0x29000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cpu_bg2.5a",   0x0000, 0x2000, 0xba0fd626 )
	ROM_LOAD( "cpu_bg3.6a",   0x2000, 0x2000, 0x7b482d61 )
	ROM_LOAD( "cpu_bg0.3a",   0x4000, 0x2000, 0xdea34fed )
	ROM_LOAD( "cpu_bg1.4a",   0x6000, 0x2000, 0x8f64525f )
	ROM_LOAD( "vid_6fg.a2",   0x8000, 0x4000, 0x8cb8a066 )
	ROM_LOAD( "vid_7fg.a1",   0xc000, 0x4000, 0x940fe17e )
	ROM_LOAD( "vid_4fg.a4",   0x10000, 0x4000, 0x7ca4941b )
	ROM_LOAD( "vid_5fg.a3",   0x14000, 0x4000, 0x2d9fbcec )
	ROM_LOAD( "vid_2fg.a6",   0x18000, 0x4000, 0x62c8bfa5 )
	ROM_LOAD( "vid_3fg.a5",   0x1c000, 0x4000, 0xb894934d )
	ROM_LOAD( "vid_0fg.a8",   0x20000, 0x4000, 0x292c5466 )
	ROM_LOAD( "vid_1fg.a7",   0x24000, 0x4000, 0x9fe286ec )
	ROM_LOAD( "cpu_alph.10g", 0x28000, 0x1000, 0x936dc87f )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "snd_0sd.a8",   0x0000, 0x1000, 0xc95cf31e )
	ROM_LOAD( "snd_1sd.a7",   0x1000, 0x1000, 0x12aaa48e )

	ROM_REGION(0x8000)  /* 32k for the Chip Squeak Deluxe */
	ROM_LOAD_EVEN( "csd_u7a.u7",   0x00000, 0x2000, 0x6e689fe7 )
	ROM_LOAD_ODD ( "csd_u17b.u17", 0x00000, 0x2000, 0x0d9ddce6 )
	ROM_LOAD_EVEN( "csd_u8c.u8",   0x04000, 0x2000, 0x35563cd0 )
	ROM_LOAD_ODD ( "csd_u18d.u18", 0x04000, 0x2000, 0x63d3f5b1 )
ROM_END


struct GameDriver spyhunt_driver =
{
	__FILE__,
	0,
	"spyhunt",
	"Spy Hunter",
	"1983",
	"Bally Midway",
	"Aaron Giles\nChristopher Kirmse\nNicola Salmoria\nBrad Oliver\nLawnmower Man",
	0,
	&spyhunt_machine_driver,
	0,

	spyhunt_rom,
	spyhunt_decode, 0,
	0,
	0,	/* sound_prom */

	spyhunt_input_ports,

	0, 0,0,
	ORIENTATION_ROTATE_90,

	spyhunt_hiload, spyhunt_hisave
};



ROM_START( crater_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "crcpu.6d",     0x0000, 0x2000, 0xad31f127 )
	ROM_LOAD( "crcpu.7d",     0x2000, 0x2000, 0x3743c78f )
	ROM_LOAD( "crcpu.8d",     0x4000, 0x2000, 0xc95f9088 )
	ROM_LOAD( "crcpu.9d",     0x6000, 0x2000, 0xa03c4b11 )
	ROM_LOAD( "crcpu.10d",    0x8000, 0x2000, 0x44ae4cbd )

	ROM_REGION_DISPOSE(0x29000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "crcpu.5a",     0x00000, 0x2000, 0x2fe4a6e1 )
	ROM_LOAD( "crcpu.6a",     0x02000, 0x2000, 0xd0659042 )
	ROM_LOAD( "crcpu.3a",     0x04000, 0x2000, 0x9d73504a )
	ROM_LOAD( "crcpu.4a",     0x06000, 0x2000, 0x42a47dff )
	ROM_LOAD( "crvid.a9",     0x08000, 0x4000, 0x811f152d )
	ROM_LOAD( "crvid.a10",    0x0c000, 0x4000, 0x7a22d6bc )
	ROM_LOAD( "crvid.a7",     0x10000, 0x4000, 0x9fa307d5 )
	ROM_LOAD( "crvid.a8",     0x14000, 0x4000, 0x4b913498 )
	ROM_LOAD( "crvid.a5",     0x18000, 0x4000, 0x9bdec312 )
	ROM_LOAD( "crvid.a6",     0x1c000, 0x4000, 0x5bf954e0 )
	ROM_LOAD( "crvid.a3",     0x20000, 0x4000, 0x2c2f5b29 )
	ROM_LOAD( "crvid.a4",     0x24000, 0x4000, 0x579a8e36 )
	ROM_LOAD( "crcpu.10g",    0x28000, 0x1000, 0x6fe53c8d )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "crsnd4.a7",    0x0000, 0x1000, 0xfd666cb5 )
	ROM_LOAD( "crsnd1.a8",    0x1000, 0x1000, 0x90bf2c4c )
	ROM_LOAD( "crsnd2.a9",    0x2000, 0x1000, 0x3b8deef1 )
	ROM_LOAD( "crsnd3.a10",   0x3000, 0x1000, 0x05803453 )
ROM_END

struct GameDriver crater_driver =
{
	__FILE__,
	0,
	"crater",
	"Crater Raider",
	"1984",
	"Bally Midway",
	"Aaron Giles\nChristopher Kirmse\nNicola Salmoria\nBrad Oliver\nLawnmower Man",
	0,
	&crater_machine_driver,
	0,

	crater_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	crater_input_ports,

	0, 0,0,
	ORIENTATION_FLIP_X,

	crater_hiload, crater_hisave
};
