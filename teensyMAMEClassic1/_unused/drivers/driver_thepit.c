/***************************************************************************

The Pit/Round Up/Intrepid/Super Mouse memory map (preliminary)


Main CPU:

0000-4fff ROM
8000-87ff RAM
8800-8bff Color RAM        (Not used in Intrepid/Super Mouse)
8c00-8fff Mirror for above (Not used in Intrepid/Super Mouse)
9000-93ff Video RAM
9400-97ff Mirror for above (Color RAM in Intrepid/Super Mouse)
9800-983f Attributes RAM
9840-985f Sprite RAM

Read:

a000      Input Port 0
a800      Input Port 1
b000      DIP Switches
b800      Watchdog Reset

Write:

b000      NMI Enable
b002      Coin Lockout
b003	  Sound Enable
b005      Intrepid graphics bank select
b006      Flip Screen X
b007      Flip Screen Y
b800      Sound Command


Sound CPU:

0000-0fff ROM  (0000-07ff in The Pit)
3800-3bff RAM


Port I/O Read:

8f  AY8910 Read Port


Port I/O Write:

00  Reset Sound Command
8c  AY8910 #2 Control Port    (Intrepid/Super Mouse only)
8d  AY8910 #2 Write Port	  (Intrepid/Super Mouse only)
8e  AY8910 #1 Control Port
8f  AY8910 #1 Write Port


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *galaxian_attributesram;
extern unsigned char *intrepid_sprite_bank_select;
void galaxian_attributes_w(int offset,int data);

void thepit_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void thepit_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void thepit_flipx_w(int offset,int data);
void thepit_flipy_w(int offset,int data);
int  thepit_input_port_0_r(int offset);
void thepit_sound_enable_w(int offset, int data);
void thepit_AY8910_0_w(int offset, int data);
void thepit_AY8910_1_w(int offset, int data);
void intrepid_graphics_bank_select_w(int offset, int data);


static struct MemoryReadAddress thepit_readmem[] =
{
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8800, 0x93ff, MRA_RAM },
	{ 0x9400, 0x97ff, videoram_r },
	{ 0x9800, 0x98ff, MRA_RAM },
	{ 0xa000, 0xa000, thepit_input_port_0_r },
	{ 0xa800, 0xa800, input_port_1_r },
	{ 0xb000, 0xb000, input_port_2_r },
	{ 0xb800, 0xb800, watchdog_reset_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress thepit_writemem[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, colorram_w },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9400, 0x97ff, videoram_w },
	{ 0x9800, 0x983f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9860, 0x98ff, MWA_RAM }, // Probably unused
	{ 0xa000, 0xa000, MWA_NOP }, // Not hooked up according to the schematics
	{ 0xb000, 0xb000, interrupt_enable_w },
	{ 0xb001, 0xb001, MWA_NOP }, // Unused, but initialized
	{ 0xb002, 0xb002, MWA_NOP }, // See memory map
	{ 0xb003, 0xb003, thepit_sound_enable_w },
	{ 0xb004, 0xb005, MWA_NOP }, // Unused, but initialized
	{ 0xb006, 0xb006, thepit_flipx_w, &flip_screen_x },
	{ 0xb007, 0xb007, thepit_flipy_w, &flip_screen_y },
	{ 0xb800, 0xb800, soundlatch_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress intrepid_readmem[] =
{
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x98ff, MRA_RAM },
	{ 0xa000, 0xa000, thepit_input_port_0_r },
	{ 0xa800, 0xa800, input_port_1_r },
	{ 0xb000, 0xb000, input_port_2_r },
	{ 0xb800, 0xb800, watchdog_reset_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress intrepid_writemem[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9400, 0x97ff, colorram_w, &colorram },
	{ 0x9800, 0x983f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9860, 0x98ff, MWA_RAM }, // Probably unused
	{ 0xb000, 0xb000, interrupt_enable_w },
	{ 0xb001, 0xb001, MWA_NOP }, // Unused, but initialized
	{ 0xb002, 0xb002, MWA_NOP }, // See memory map
	{ 0xb003, 0xb003, thepit_sound_enable_w },
	{ 0xb004, 0xb004, MWA_NOP }, // Unused, but initialized
	{ 0xb005, 0xb005, intrepid_graphics_bank_select_w },
	{ 0xb006, 0xb006, thepit_flipx_w, &flip_screen_x },
	{ 0xb007, 0xb007, thepit_flipy_w, &flip_screen_y },
	{ 0xb800, 0xb800, soundlatch_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x3800, 0x3bff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x3800, 0x3bff, MWA_RAM },
	{ -1 }  /* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x8f, 0x8f, AY8910_read_port_0_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, soundlatch_clear_w },
	{ 0x8c, 0x8d, thepit_AY8910_1_w },
	{ 0x8e, 0x8f, thepit_AY8910_0_w },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( thepit_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Coinage P1/P2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1Cr/2Cr" )
	PORT_DIPSETTING(    0x01, "2Cr/3Cr" )
	PORT_DIPSETTING(    0x02, "2Cr/4Cr" )
	PORT_DIPSETTING(    0x03, "Free Play" )
	PORT_DIPNAME( 0x04, 0x00, "Game Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x08, 0x00, "Time Limit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Long" )
	PORT_DIPSETTING(    0x08, "Short" )
	PORT_DIPNAME( 0x10, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_DIPNAME( 0x40, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	/* Since the real inputs are multiplexed, we used this fake port
	   to read the 2nd player controls when the screen is flipped */
	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( roundup_input_ports )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x04, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x10, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Life At", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10,000" )
	PORT_DIPSETTING(    0x20, "30,000" )
	PORT_DIPNAME( 0x40, 0x40, "Gly Boys Wake Up", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	/* Since the real inputs are multiplexed, we used this fake port
	   to read the 2nd player controls when the screen is flipped */
	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( intrepid_input_ports )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Starts a timer, which, */
  												  /* after it runs down, doesn't */
	PORT_START      /* IN2 */                     /* seem to do anything. See $0105 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_BITX(    0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x08, 0x08, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x10, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10,000" )
	PORT_DIPSETTING(    0x10, "30,000" )
	PORT_DIPNAME( 0x20, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0xc0, 0x40, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0xc0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )

	/* Since the real inputs are multiplexed, we used this fake port
	   to read the 2nd player controls when the screen is flipped */
	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( portman_input_ports )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* unused? */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* unused? */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* unused? */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x04, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "100000" )
	PORT_DIPSETTING(    0x10, "300000" )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )	/* not used? */
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )	/* not used? */
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	/* Since the real inputs are multiplexed, we used this fake port
	   to read the 2nd player controls when the screen is flipped */
	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


INPUT_PORTS_START( suprmous_input_ports )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x06, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/7 Credits" )
	PORT_DIPNAME( 0x18, 0x00, "Lives", IP_KEY_NONE )  /* The game reads these together */
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "5" )
  //PORT_DIPSETTING(    0x10, "5" )
  //PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "5,000" )
	PORT_DIPSETTING(    0x00, "10,000" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	/* Since the real inputs are multiplexed, we used this fake port
	   to read the 2nd player controls when the screen is flipped */
	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	2,      /* 2 bits per pixel */
	{ 0x1000*8, 0 }, /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};


static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	64,     /* 64 sprites */
	2,      /* 2 bits per pixel */
	{ 0x1000*8, 0 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};


static struct GfxLayout suprmous_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	3,	    /* 3 bits per pixel */
	{ 0x2000*8, 0x1000*8, 0 },	/* the three bitplanes for 4 pixels are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	    /* every char takes 8 consecutive bytes */
};


static struct GfxLayout suprmous_spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,		/* 64 sprites */
	3,	    /* 3 bits per pixel */
	{ 0x2000*8, 0x1000*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo thepit_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo intrepid_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ 1, 0x0800, &charlayout,     0, 8 },
	{ 1, 0x0800, &spritelayout,   0, 8 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo suprmous_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &suprmous_charlayout,   0, 8 },
	{ 1, 0x0800, &suprmous_spritelayout, 0, 8 },
	{ -1 } /* end of array */
};


static struct AY8910interface ay8910_interface =
{
	2,      /* 1 or 2 chips */
	18432000/12,     /* 1.536Mhz */
	{ 255, 255 },
	{ soundlatch_r, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 }
};


#define MACHINE_DRIVER(GAMENAME, COLORS)		            \
static struct MachineDriver GAMENAME##_machine_driver =		\
{									  			            \
	/* basic machine hardware */							\
	{														\
		{													\
			CPU_Z80,										\
			18432000/6,     /* 3.072 Mhz */					\
			0,												\
			GAMENAME##_readmem,GAMENAME##_writemem,0,0,		\
			nmi_interrupt,1									\
		},													\
		{													\
			CPU_Z80 | CPU_AUDIO_CPU,						\
			10000000/4,     /* 2.5 Mhz */					\
			3,												\
			sound_readmem,sound_writemem,					\
			sound_readport,sound_writeport,					\
			interrupt,1										\
		}													\
	},														\
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */ \
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */ \
	0,														\
															\
	/* video hardware */									\
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },				\
	GAMENAME##_gfxdecodeinfo,								\
	COLORS+8, COLORS,										\
	thepit_vh_convert_color_prom,							\
															\
	VIDEO_TYPE_RASTER,										\
	0,														\
	generic_vh_start,										\
	generic_vh_stop,										\
	thepit_vh_screenrefresh,								\
															\
	/* sound hardware */									\
	0,0,0,0,												\
	{														\
		{													\
			SOUND_AY8910,									\
			&ay8910_interface								\
		}													\
	}														\
};


#define suprmous_readmem   intrepid_readmem
#define suprmous_writemem  intrepid_writemem

MACHINE_DRIVER(thepit,   4*8)
MACHINE_DRIVER(intrepid, 4*8)
MACHINE_DRIVER(suprmous, 8*8)

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( thepit_rom )
	ROM_REGION(0x10000)     /* 64k for main CPU */
	ROM_LOAD( "p38b",         0x0000, 0x1000, 0x7315e1bc )
	ROM_LOAD( "p39b",         0x1000, 0x1000, 0xc9cc30fe )
	ROM_LOAD( "p40b",         0x2000, 0x1000, 0x986738b5 )
	ROM_LOAD( "p41b",         0x3000, 0x1000, 0x31ceb0a1 )
	ROM_LOAD( "p33b",         0x4000, 0x1000, 0x614ec454 )

	ROM_REGION_DISPOSE(0x1800)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "p9",           0x0000, 0x0800, 0x69502afc )
	ROM_LOAD( "p8",           0x1000, 0x0800, 0x2ddd5045 )

	ROM_REGION(0x0020)      /* Color PROM */
	ROM_LOAD( "pitclr.ic4",   0x0000, 0x0020, 0xa758b567 )

	ROM_REGION(0x10000)     /* 64k for audio CPU */
	ROM_LOAD( "p30",          0x0000, 0x0800, 0x1b79dfb6 )
ROM_END

ROM_START( roundup_rom )
	ROM_REGION(0x10000)     /* 64k for main CPU */
	ROM_LOAD( "roundup.u38",  0x0000, 0x1000, 0xd62c3b7a )
	ROM_LOAD( "roundup.u39",  0x1000, 0x1000, 0x37bf554b )
	ROM_LOAD( "roundup.u40",  0x2000, 0x1000, 0x5109d0c5 )
	ROM_LOAD( "roundup.u41",  0x3000, 0x1000, 0x1c5ed660 )
	ROM_LOAD( "roundup.u33",  0x4000, 0x1000, 0x2fa711f3 )

	ROM_REGION_DISPOSE(0x1800)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "roundup.u9",   0x0000, 0x0800, 0x394676a2 )
	ROM_LOAD( "roundup.u10",  0x1000, 0x0800, 0xa38d708d )

	ROM_REGION(0x0020)      /* Color PROM */
	ROM_LOAD( "roundup.clr",  0x0000, 0x0020, 0xa758b567 )

	ROM_REGION(0x10000)     /* 64k for audio CPU */
	ROM_LOAD( "roundup.u30",  0x0000, 0x0800, 0x1b18faee )
	ROM_LOAD( "roundup.u31",  0x0800, 0x0800, 0x76cf4394 )
ROM_END

ROM_START( intrepid_rom )
	ROM_REGION(0x10000)     /* 64k for main CPU */
	ROM_LOAD( "ic19.1",       0x0000, 0x1000, 0x7d927b23 )
	ROM_LOAD( "ic18.2",       0x1000, 0x1000, 0xdcc22542 )
	ROM_LOAD( "ic17.3",       0x2000, 0x1000, 0xfd11081e )
	ROM_LOAD( "ic16.4",       0x3000, 0x1000, 0x74a51841 )
	ROM_LOAD( "ic15.5",       0x4000, 0x1000, 0x4fef643d )

	ROM_REGION_DISPOSE(0x2000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic9.9",        0x0000, 0x1000, 0x8c70d18d )
	ROM_LOAD( "ic8.8",        0x1000, 0x1000, 0x04d067d3 )

	ROM_REGION(0x0020)      /* Color PROM */
	ROM_LOAD( "ic3.prm",      0x0000, 0x0020, 0x927ff40a )

	ROM_REGION(0x10000)     /* 64k for audio CPU */
	ROM_LOAD( "ic22.7",       0x0000, 0x0800, 0x1a7cc392 )
	ROM_LOAD( "ic23.6",       0x0800, 0x0800, 0x91ca7097 )
ROM_END

ROM_START( portman_rom )
	ROM_REGION(0x10000)     /* 64k for main CPU */
	ROM_LOAD( "pe1",       0x0000, 0x1000, 0xa5cf6083 )
	ROM_LOAD( "pe2",       0x1000, 0x1000, 0x0b53d48a )
	ROM_LOAD( "pe3",       0x2000, 0x1000, 0x1c923057 )
	ROM_LOAD( "pe4",       0x3000, 0x1000, 0x555c71ef )
	ROM_LOAD( "pe5",       0x4000, 0x1000, 0xf749e2d4 )

	ROM_REGION_DISPOSE(0x2000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pe8",       0x0000, 0x1000, 0x4d8c2974 )
	ROM_LOAD( "pe9",       0x1000, 0x1000, 0x4e4ea162 )

	ROM_REGION(0x0020)      /* Color PROM */
	ROM_LOAD( "ic3",       0x0000, 0x0020, 0x6440dc61 )

	ROM_REGION(0x10000)     /* 64k for audio CPU */
	ROM_LOAD( "pe7",       0x0000, 0x0800, 0xd2094e4a )
	ROM_LOAD( "pe6",       0x0800, 0x0800, 0x1cf447f4 )
ROM_END

ROM_START( suprmous_rom )
	ROM_REGION(0x10000)	    /* 64k for main CPU */
	ROM_LOAD( "sm.1",         0x0000, 0x1000, 0x9db2b786 )
	ROM_LOAD( "sm.2",         0x1000, 0x1000, 0x0a3d91d3 )
	ROM_LOAD( "sm.3",         0x2000, 0x1000, 0x32af6285 )
	ROM_LOAD( "sm.4",         0x3000, 0x1000, 0x46091524 )
	ROM_LOAD( "sm.5",         0x4000, 0x1000, 0xf15fd5d2 )

	ROM_REGION_DISPOSE(0x3000)	    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sm.7",         0x0000, 0x1000, 0x1d476696 )
	ROM_LOAD( "sm.8",         0x1000, 0x1000, 0x2f81ab5f )
	ROM_LOAD( "sm.9",         0x2000, 0x1000, 0x8463af89 )

	ROM_REGION(0x0040)      /* Color PROM */
	/* We don't have this yet */

	ROM_REGION(0x10000)	   /* 64k for audio CPU */
	ROM_LOAD( "sm.6",         0x0000, 0x1000, 0xfba71785 )
ROM_END



static int thepit_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8297],"\x16",1) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x8039],0x0f);
			osd_fread(f,&RAM[0x8280],0x20);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void thepit_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8039],0x0f);
		osd_fwrite(f,&RAM[0x8280],0x20);
		osd_fclose(f);
	}
}


static int roundup_suprmous_common_hiload(int address)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (videoram_r(0x0181) == 0x00 &&
		videoram_r(0x01a1) == 0x24)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int pos;

			osd_fread(f,&RAM[address],0x03);
			osd_fclose(f);

			/* Copy it to the screen and remove leading zeroes */
			videoram_w(0x0221, RAM[address  ] >> 4);
			videoram_w(0x0201, RAM[address  ] & 0x0f);
			videoram_w(0x01e1, RAM[address+1] >> 4);
			videoram_w(0x01c1, RAM[address+1] & 0x0f);
			videoram_w(0x01a1, RAM[address+2] >> 4);
			videoram_w(0x0181, RAM[address+2] & 0x0f);

			for (pos = 0x0221; pos >= 0x01a1; pos -= 0x20 )
			{
				if (videoram_r(pos) != 0x00) break;

				videoram_w(pos, 0x24);
			}
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static int roundup_hiload(void)
{
	return roundup_suprmous_common_hiload(0x8050);
}

static void roundup_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8050],0x03);
		osd_fclose(f);
	}
}


static int intrepid_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8078],"\xfd",1) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x803d],0x3c);
			RAM[0x8035] = RAM[0x803d];
			RAM[0x8036] = RAM[0x803e];
			RAM[0x8037] = RAM[0x803f];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void intrepid_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x803d],0x3c);
		osd_fclose(f);
	}
}


static int suprmous_hiload(void)
{
	return roundup_suprmous_common_hiload(0x804a);
}

static void suprmous_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x804a],0x03);
		osd_fclose(f);
	}
}

struct GameDriver thepit_driver =
{
	__FILE__,
	0,
	"thepit",
	"The Pit",
	"1982",
	"Centuri",
	"Zsolt Vasvari",
	GAME_IMPERFECT_COLORS,
	&thepit_machine_driver,
	0,

	thepit_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	thepit_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	thepit_hiload, thepit_hisave
};


struct GameDriver roundup_driver =
{
	__FILE__,
	0,
	"roundup",
	"Round-Up",
	"1981",
	"Amenip/Centuri",
	"Zsolt Vasvari",
	GAME_IMPERFECT_COLORS,
	&thepit_machine_driver,
	0,

	roundup_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	roundup_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	roundup_hiload, roundup_hisave
};


struct GameDriver intrepid_driver =
{
	__FILE__,
	0,
	"intrepid",
	"Intrepid",
	"1983",
	"Nova Games Ltd.",
	"Zsolt Vasvari",
	GAME_IMPERFECT_COLORS,
	&intrepid_machine_driver,
	0,

	intrepid_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	intrepid_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	intrepid_hiload, intrepid_hisave
};

struct GameDriver portman_driver =
{
	__FILE__,
	0,
	"portman",
	"Port Man",
	"1982",
	"Nova Games Ltd.",
	"Zsolt Vasvari",
	GAME_IMPERFECT_COLORS,
	&intrepid_machine_driver,
	0,

	portman_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	portman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver suprmous_driver =
{
	__FILE__,
	0,
	"suprmous",
	"Super Mouse",
	"1982",
	"Taito",
	"Brad Oliver",
	GAME_WRONG_COLORS,
	&suprmous_machine_driver,
	0,

	suprmous_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	suprmous_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	suprmous_hiload, suprmous_hisave
};
