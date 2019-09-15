/***************************************************************************

Pac Man memory map (preliminary)

0000-3fff ROM
4000-43ff Video RAM
4400-47ff Color RAM
4c00-4fff RAM
8000-9fff ROM (Ms Pac Man and Ponpoko only)
a000-bfff ROM (Ponpoko only)

memory mapped ports:

read:
5000      IN0
5040      IN1
5080      DSW 1
50c0	  DSW 2 (Ponpoko only)
see the input_ports definition below for details on the input bits

write:
4ff0-4fff 8 pairs of two bytes:
          the first byte contains the sprite image number (bits 2-7), Y flip (bit 0),
		  X flip (bit 1); the second byte the color
5000      interrupt enable
5001      sound enable
5002      ????
5003      flip screen
5004      1 player start lamp
5005      2 players start lamp
5006      related to the credits. don't know what it was used for.
5007      coin counter
5040-5044 sound voice 1 accumulator (nibbles) (used by the sound hardware only)
5045      sound voice 1 waveform (nibble)
5046-5049 sound voice 2 accumulator (nibbles) (used by the sound hardware only)
504a      sound voice 2 waveform (nibble)
504b-504e sound voice 3 accumulator (nibbles) (used by the sound hardware only)
504f      sound voice 3 waveform (nibble)
5050-5054 sound voice 1 frequency (nibbles)
5055      sound voice 1 volume (nibble)
5056-5059 sound voice 2 frequency (nibbles)
505a      sound voice 2 volume (nibble)
505b-505e sound voice 3 frequency (nibbles)
505f      sound voice 3 volume (nibble)
5060-506f Sprite coordinates, x/y pairs for 8 sprites
50c0      Watchdog reset

I/O ports:
OUT on port $0 sets the interrupt vector

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



void pacman_init_machine(void);
int pacman_interrupt(void);

int pacman_vh_start(void);
void pacman_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void pengo_flipscreen_w(int offset,int data);
void pengo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern unsigned char *pengo_soundregs;
void pengo_sound_enable_w(int offset,int data);
void pengo_sound_w(int offset,int data);

extern void pacplus_decode(void);

void theglob_init_machine(void);
int theglob_decrypt_rom(int offset);
extern unsigned char *theglob_mem_rom;

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },	/* video and color RAM */
	{ 0x4c00, 0x4fff, MRA_RAM },	/* including sprite codes at 4ff0-4fff */
	{ 0x5000, 0x503f, input_port_0_r },	/* IN0 */
	{ 0x5040, 0x507f, input_port_1_r },	/* IN1 */
	{ 0x5080, 0x50bf, input_port_2_r },	/* DSW1 */
	{ 0x50c0, 0x50ff, input_port_3_r },	/* DSW2 */
	{ 0x8000, 0xbfff, MRA_ROM },	/* Ms. Pac Man / Ponpoko only */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4400, 0x47ff, colorram_w, &colorram },
	{ 0x4c00, 0x4fef, MWA_RAM },
	{ 0x4ff0, 0x4fff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5000, 0x5000, interrupt_enable_w },
	{ 0x5001, 0x5001, pengo_sound_enable_w },
	{ 0x5002, 0x5002, MWA_NOP },
	{ 0x5003, 0x5003, pengo_flipscreen_w },
 	{ 0x5004, 0x5005, osd_led_w },
 	{ 0x5006, 0x5006, MWA_NOP },
 	{ 0x5007, 0x5007, coin_counter_w },
	{ 0x5040, 0x505f, pengo_sound_w, &pengo_soundregs },
	{ 0x5060, 0x506f, MWA_RAM, &spriteram_2 },
	{ 0x50c0, 0x50c0, watchdog_reset_w },
	{ 0x8000, 0xbfff, MWA_ROM },	/* Ms. Pac Man / Ponpoko only */
	{ 0xc000, 0xc3ff, videoram_w }, /* mirror address for video ram, */
	{ 0xc400, 0xc7ef, colorram_w }, /* used to display HIGH SCORE and CREDITS */
	{ 0xffff, 0xffff, MWA_NOP },	/* Eyes writes to this location to simplify code */
	{ -1 }	/* end of table */
};



static struct IOWritePort writeport[] =
{
	{ 0, 0, interrupt_vector_w },	/* Pac Man only */
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress theglob_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_BANK1, &theglob_mem_rom },
	{ 0x4000, 0x47ff, MRA_RAM },	/* video and color RAM */
	{ 0x4c00, 0x4fff, MRA_RAM },	/* including sprite codes at 4ff0-4fff */
	{ 0x5000, 0x503f, input_port_0_r },	/* IN0 */
	{ 0x5040, 0x507f, input_port_1_r },	/* IN1 */
	{ 0x5080, 0x50bf, input_port_2_r },	/* DSW1 */
	{ 0x50c0, 0x50ff, input_port_3_r },	/* DSW2 */
	{ -1 }	/* end of table */
};

static struct IOReadPort theglob_readport[] =
{
	{ 0x00, 0xff, theglob_decrypt_rom },	/* Switch protection logic */
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( pacman_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START	/* DSW 1 */
 	PORT_DIPNAME( 0x03, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x0c, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x30, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_DIPSETTING(    0x20, "20000" )
	PORT_DIPSETTING(    0x30, "Never" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Ghost Names", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Normal" )
	PORT_DIPSETTING(    0x00, "Alternate" )

	PORT_START	/* DSW 2 */
 	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the fire button */
	/* and activate the speedup cheat if it is. */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Speedup Cheat", OSD_KEY_LCONTROL, OSD_JOY_FIRE1, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
INPUT_PORTS_END

/* Ms. Pac Man input ports are identical to Pac Man, the only difference is */
/* the missing Ghost Names dip switch. */
INPUT_PORTS_START( mspacman_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START	/* DSW 1 */
 	PORT_DIPNAME( 0x03, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x0c, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x30, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_DIPSETTING(    0x20, "20000" )
	PORT_DIPSETTING(    0x30, "Never" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW 2 */
 	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the fire button */
	/* and activate the speedup cheat if it is. */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Speedup Cheat", OSD_KEY_LCONTROL, OSD_JOY_FIRE1, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( crush_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_DIPNAME( 0x10, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x10, "Cocktail" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW 1 */
 	PORT_DIPNAME( 0x03, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x0c, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_DIPNAME( 0x10, 0x10, "First Pattern", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, "Teleport holes", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW 2 */
 	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( ponpoko_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	/* The 2nd player controls are used even in upright mode */
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x03, 0x01, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "10000" )
	PORT_DIPSETTING(    0x02, "30000" )
	PORT_DIPSETTING(    0x03, "50000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x0c, 0x00, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPNAME( 0x30, 0x20, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x30, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW 2 */
 	PORT_DIPNAME( 0x0f, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "A 3/1 B 3/1" )
	PORT_DIPSETTING(    0x0e, "A 3/1 B 1/2" )
	PORT_DIPSETTING(    0x0f, "A 3/1 B 1/4" )
	PORT_DIPSETTING(    0x02, "A 2/1 B 2/1" )
	PORT_DIPSETTING(    0x0d, "A 2/1 B 1/1" )
	PORT_DIPSETTING(    0x07, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x0b, "A 2/1 B 1/5" )
	PORT_DIPSETTING(    0x0c, "A 2/1 B 1/6" )
	PORT_DIPSETTING(    0x01, "A 1/1 B 1/1" )
	PORT_DIPSETTING(    0x06, "A 1/1 B 4/5" )
	PORT_DIPSETTING(    0x05, "A 1/1 B 2/3" )
	PORT_DIPSETTING(    0x0a, "A 1/1 B 1/3" )
	PORT_DIPSETTING(    0x08, "A 1/1 B 1/5" )
	PORT_DIPSETTING(    0x09, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x03, "A 1/2 B 1/2" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 3", IP_KEY_NONE )  /* Most likely unused */
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 4", IP_KEY_NONE )  /* Most likely unused */
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 5", IP_KEY_NONE )  /* Most likely unused */
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( eyes_input_ports )
    PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BITX(0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(0x10, "Off" )
	PORT_DIPSETTING(0x00, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START	/* DSW 1 */
 	PORT_DIPNAME( 0x03, 0x03, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x0c, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "50000" )
	PORT_DIPSETTING(    0x20, "75000" )
	PORT_DIPSETTING(    0x10, "100000" )
	PORT_DIPSETTING(    0x00, "125000" )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )  /* Not accessed */
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW 2 */
 	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( lizwiz_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BITX(0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(0x10, "Off" )
	PORT_DIPSETTING(0x00, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START	/* DSW 1 */
 	PORT_DIPNAME( 0x03, 0x03, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x0c, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "75000" )
	PORT_DIPSETTING(    0x20, "100000" )
	PORT_DIPSETTING(    0x10, "125000" )
	PORT_DIPSETTING(    0x00, "150000" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW 2 */
 	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( theglob_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START	/* DSW 1 */
 	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x1c, 0x1c, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x1c, "1 (Easiest)" )
	PORT_DIPSETTING(    0x18, "2" )
	PORT_DIPSETTING(    0x14, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPSETTING(    0x08, "6" )
	PORT_DIPSETTING(    0x04, "7" )
	PORT_DIPSETTING(    0x00, "8 (Hardest)" )
	PORT_DIPNAME( 0x20, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW 2 */
 	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


static struct GfxLayout tilelayout =
{
	8,8,	/* 8*8 characters */
    256,    /* 256 characters */
    2,  /* 2 bits per pixel */
    { 0, 4 },   /* the two bitplanes for 4 pixels are packed into one byte */
    { 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 }, /* bits are packed in groups of four */
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    16*8    /* every char takes 16 bytes */
};


static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &tilelayout,	0, 32 },
	{ 1, 0x1000, &spritelayout, 0, 32 },
	{ -1 } /* end of array */
};


static struct namco_interface namco_interface =
{
	3072000/32,	/* sample rate */
	3,			/* number of voices */
	32,			/* gain adjustment */
	255,		/* playback volume */
	3			/* memory region */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,writeport,
			pacman_interrupt,1
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	pacman_init_machine,

	/* video hardware */
	36*8, 28*8, { 0*8, 36*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	16, 4*32,
	pacman_vh_convert_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	pacman_vh_start,
	generic_vh_stop,
	pengo_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_NAMCO,
			&namco_interface
		}
	}
};


static struct MachineDriver theglob_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			theglob_readmem,writemem,theglob_readport,writeport,
			pacman_interrupt,1
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	theglob_init_machine,

	/* video hardware */
	36*8, 28*8, { 0*8, 36*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	16, 4*32,
	pacman_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	pacman_vh_start,
	generic_vh_stop,
	pengo_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_NAMCO,
			&namco_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( pacman_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pacman.6e",    0x0000, 0x1000, 0xc1e6ab10 )
	ROM_LOAD( "pacman.6f",    0x1000, 0x1000, 0x1a6fb2d4 )
	ROM_LOAD( "pacman.6h",    0x2000, 0x1000, 0xbcdd1beb )
	ROM_LOAD( "pacman.6j",    0x3000, 0x1000, 0x817d94e3 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pacman.5e",    0x0000, 0x1000, 0x0c944964 )
	ROM_LOAD( "pacman.5f",    0x1000, 0x1000, 0x958fedf9 )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, 0x2fc650bd )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, 0x3eb3a8e4 )

	ROM_REGION(0x0200)	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, 0xa9cc86bf )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, 0x77245b66 )	/* timing - not used */
ROM_END

ROM_START( namcopac_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "namcopac.6e",  0x0000, 0x1000, 0xfee263b3 )
	ROM_LOAD( "namcopac.6f",  0x1000, 0x1000, 0x39d1fc83 )
	ROM_LOAD( "namcopac.6h",  0x2000, 0x1000, 0x02083b03 )
	ROM_LOAD( "namcopac.6j",  0x3000, 0x1000, 0x7a36fe55 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "namcopac.5e",  0x0000, 0x1000, 0x0c944964 )
	ROM_LOAD( "namcopac.5f",  0x1000, 0x1000, 0x958fedf9 )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, 0x2fc650bd )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, 0x3eb3a8e4 )

	ROM_REGION(0x0200)	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, 0xa9cc86bf )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, 0x77245b66 )	/* timing - not used */
ROM_END

ROM_START( pacmanjp_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "prg1",         0x0000, 0x0800, 0xf36e88ab )
	ROM_LOAD( "prg2",         0x0800, 0x0800, 0x618bd9b3 )
	ROM_LOAD( "prg3",         0x1000, 0x0800, 0x7d177853 )
	ROM_LOAD( "prg4",         0x1800, 0x0800, 0xd3e8914c )
	ROM_LOAD( "prg5",         0x2000, 0x0800, 0x6bf4f625 )
	ROM_LOAD( "prg6",         0x2800, 0x0800, 0xa948ce83 )
	ROM_LOAD( "prg7",         0x3000, 0x0800, 0xb6289b26 )
	ROM_LOAD( "prg8",         0x3800, 0x0800, 0x17a88c13 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "chg1",         0x0000, 0x0800, 0x2066a0b7 )
	ROM_LOAD( "chg2",         0x0800, 0x0800, 0x3591b89d )
	ROM_LOAD( "chg3",         0x1000, 0x0800, 0x9e39323a )
	ROM_LOAD( "chg4",         0x1800, 0x0800, 0x1b1d9096 )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, 0x2fc650bd )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, 0x3eb3a8e4 )

	ROM_REGION(0x0200)	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, 0xa9cc86bf )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, 0x77245b66 )	/* timing - not used */
ROM_END

ROM_START( pacmod_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pacmanh.6e",   0x0000, 0x1000, 0x3b2ec270 )
	ROM_LOAD( "pacmanh.6f",   0x1000, 0x1000, 0x1a6fb2d4 )
	ROM_LOAD( "pacmanh.6h",   0x2000, 0x1000, 0x18811780 )
	ROM_LOAD( "pacmanh.6j",   0x3000, 0x1000, 0x5c96a733 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pacmanh.5e",   0x0000, 0x1000, 0x299fb17a )
	ROM_LOAD( "pacmanh.5f",   0x1000, 0x1000, 0x958fedf9 )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, 0x2fc650bd )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, 0x3eb3a8e4 )

	ROM_REGION(0x0200)	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, 0xa9cc86bf )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, 0x77245b66 )	/* timing - not used */
ROM_END

ROM_START( hangly_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "hangly.6e",    0x0000, 0x1000, 0x5fe8610a )
	ROM_LOAD( "hangly.6f",    0x1000, 0x1000, 0x73726586 )
	ROM_LOAD( "hangly.6h",    0x2000, 0x1000, 0x4e7ef99f )
	ROM_LOAD( "hangly.6j",    0x3000, 0x1000, 0x7f4147e6 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "hangly.5e",    0x0000, 0x1000, 0x0c944964 )
	ROM_LOAD( "hangly.5f",    0x1000, 0x1000, 0x958fedf9 )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, 0x2fc650bd )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, 0x3eb3a8e4 )

	ROM_REGION(0x0200)	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, 0xa9cc86bf )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, 0x77245b66 )	/* timing - not used */
ROM_END

ROM_START( puckman_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "puckman.6e",   0x0000, 0x1000, 0xa8ae23c5 )
	ROM_LOAD( "puckman.6f",   0x1000, 0x1000, 0x1a6fb2d4 )
	ROM_LOAD( "puckman.6h",   0x2000, 0x1000, 0x197443f8 )
	ROM_LOAD( "puckman.6j",   0x3000, 0x1000, 0x2e64a3ba )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "puckman.5e",   0x0000, 0x1000, 0x0c944964 )
	ROM_LOAD( "puckman.5f",   0x1000, 0x1000, 0x958fedf9 )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, 0x2fc650bd )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, 0x3eb3a8e4 )

	ROM_REGION(0x0200)	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, 0xa9cc86bf )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, 0x77245b66 )	/* timing - not used */
ROM_END

ROM_START( piranha_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pr1.cpu",      0x0000, 0x1000, 0xbc5ad024 )
	ROM_LOAD( "pr2.cpu",      0x1000, 0x1000, 0x1a6fb2d4 )
	ROM_LOAD( "pr3.cpu",      0x2000, 0x1000, 0x473c379d )
	ROM_LOAD( "pr4.cpu",      0x3000, 0x1000, 0x63fbf895 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pr5.cpu",      0x0000, 0x0800, 0x3fc4030c )
	ROM_LOAD( "pr7.cpu",      0x0800, 0x0800, 0x30b9a010 )
	ROM_LOAD( "pr6.cpu",      0x1000, 0x0800, 0xf3e9c9d5 )
	ROM_LOAD( "pr8.cpu",      0x1800, 0x0800, 0x133d720d )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, 0x2fc650bd )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, 0x3eb3a8e4 )

	ROM_REGION(0x0200)	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, 0xa9cc86bf )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, 0x77245b66 )	/* timing - not used */
ROM_END

ROM_START( pacplus_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pacplus.6e",   0x0000, 0x1000, 0xd611ef68 )
	ROM_LOAD( "pacplus.6f",   0x1000, 0x1000, 0xc7207556 )
	ROM_LOAD( "pacplus.6h",   0x2000, 0x1000, 0xae379430 )
	ROM_LOAD( "pacplus.6j",   0x3000, 0x1000, 0x5a6dff7b )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pacplus.5e",   0x0000, 0x1000, 0x022c35da )
	ROM_LOAD( "pacplus.5f",   0x1000, 0x1000, 0x4de65cdd )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "pacplus.7f",   0x0000, 0x0020, 0x063dd53a )
	ROM_LOAD( "pacplus.4a",   0x0020, 0x0100, 0xe271a166 )

	ROM_REGION(0x0200)	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, 0xa9cc86bf )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, 0x77245b66 )	/* timing - not used */
ROM_END

ROM_START( mspacman_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "boot1",        0x0000, 0x1000, 0xd16b31b7 )
	ROM_LOAD( "boot2",        0x1000, 0x1000, 0x0d32de5e )
	ROM_LOAD( "boot3",        0x2000, 0x1000, 0x1821ee0b )
	ROM_LOAD( "boot4",        0x3000, 0x1000, 0x165a9dd8 )
	ROM_LOAD( "boot5",        0x8000, 0x1000, 0x8c3e6de6 )
	ROM_LOAD( "boot6",        0x9000, 0x1000, 0x368cb165 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5e",           0x0000, 0x1000, 0x5c281d01 )
	ROM_LOAD( "5f",           0x1000, 0x1000, 0x615af909 )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, 0x2fc650bd )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, 0x3eb3a8e4 )

	ROM_REGION(0x0200)	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, 0xa9cc86bf )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, 0x77245b66 )	/* timing - not used */
ROM_END

ROM_START( mspacatk_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "boot1",        0x0000, 0x1000, 0xd16b31b7 )
	ROM_LOAD( "mspacatk.2",   0x1000, 0x1000, 0x0af09d31 )
	ROM_LOAD( "boot3",        0x2000, 0x1000, 0x1821ee0b )
	ROM_LOAD( "boot4",        0x3000, 0x1000, 0x165a9dd8 )
	ROM_LOAD( "mspacatk.5",   0x8000, 0x1000, 0xe6e06954 )
	ROM_LOAD( "mspacatk.6",   0x9000, 0x1000, 0x3b5db308 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5e",           0x0000, 0x1000, 0x5c281d01 )
	ROM_LOAD( "5f",           0x1000, 0x1000, 0x615af909 )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, 0x2fc650bd )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, 0x3eb3a8e4 )

	ROM_REGION(0x0200)	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, 0xa9cc86bf )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, 0x77245b66 )	/* timing - not used */
ROM_END

ROM_START( crush_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "tp1",          0x0000, 0x0800, 0xf276592e )
	ROM_LOAD( "tp5a",         0x0800, 0x0800, 0x3d302abe )
	ROM_LOAD( "tp2",          0x1000, 0x0800, 0x25f42e70 )
	ROM_LOAD( "tp6",          0x1800, 0x0800, 0x98279cbe )
	ROM_LOAD( "tp3",          0x2000, 0x0800, 0x8377b4cb )
	ROM_LOAD( "tp7",          0x2800, 0x0800, 0xd8e76c8c )
	ROM_LOAD( "tp4",          0x3000, 0x0800, 0x90b28fa3 )
	ROM_LOAD( "tp8",          0x3800, 0x0800, 0x10854e1b )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tpa",          0x0000, 0x0800, 0xc7617198 )
	ROM_LOAD( "tpc",          0x0800, 0x0800, 0xe129d76a )
	ROM_LOAD( "tpb",          0x1000, 0x0800, 0xd1899f05 )
	ROM_LOAD( "tpd",          0x1800, 0x0800, 0xd35d1caf )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "crush.7f",     0x0000, 0x0020, 0x2fc650bd )
	ROM_LOAD( "crush.4a",     0x0020, 0x0100, 0x2bc5d339 )

	ROM_REGION(0x0200)	/* sound PROMs */
	ROM_LOAD( "crush.1m",     0x0000, 0x0100, 0xa9cc86bf )
	ROM_LOAD( "crush.3m",     0x0100, 0x0100, 0x77245b66 )	/* timing - not used */
ROM_END

ROM_START( ponpoko_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ppoko1.bin",   0x0000, 0x1000, 0x49077667 )
	ROM_LOAD( "ppoko2.bin",   0x1000, 0x1000, 0x5101781a )
	ROM_LOAD( "ppoko3.bin",   0x2000, 0x1000, 0xd790ed22 )
	ROM_LOAD( "ppoko4.bin",   0x3000, 0x1000, 0x4e449069 )
	ROM_LOAD( "ppoko5.bin",   0x8000, 0x1000, 0x54ca3d7d )
	ROM_LOAD( "ppoko6.bin",   0x9000, 0x1000, 0x3055c7e0 )
	ROM_LOAD( "ppoko7.bin",   0xa000, 0x1000, 0x3cbe47ca )
	ROM_LOAD( "ppoko8.bin",   0xb000, 0x1000, 0xb39be27d )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ppoko9.bin",   0x0000, 0x1000, 0xb73e1a06 )
	ROM_LOAD( "ppoko10.bin",  0x1000, 0x1000, 0x62069b5d )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "82s123.7f",    0x0000, 0x0020, 0x2fc650bd )
	ROM_LOAD( "82s126.4a",    0x0020, 0x0100, 0x3eb3a8e4 )

	ROM_REGION(0x0200)	/* sound PROMs */
	ROM_LOAD( "82s126.1m",    0x0000, 0x0100, 0xa9cc86bf )
	ROM_LOAD( "82s126.3m",    0x0100, 0x0100, 0x77245b66 )	/* timing - not used */
ROM_END

ROM_START( eyes_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "d7",           0x0000, 0x1000, 0x3b09ac89 )
	ROM_LOAD( "e7",           0x1000, 0x1000, 0x97096855 )
	ROM_LOAD( "f7",           0x2000, 0x1000, 0x731e294e )
	ROM_LOAD( "h7",           0x3000, 0x1000, 0x22f7a719 )

	ROM_REGION_DISPOSE(0x2000)  /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "d5",           0x0000, 0x1000, 0xd6af0030 )
	ROM_LOAD( "e5",           0x1000, 0x1000, 0xa42b5201 )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "82s123.8h",    0x0000, 0x0020, 0x2fc650bd )
	ROM_LOAD( "82s129.4a",    0x0020, 0x0100, 0xd8d78829 )

	ROM_REGION(0x0200)	/* sound PROMs */
	ROM_LOAD( "82s129.1k",    0x0000, 0x0100, 0xa9cc86bf )
	ROM_LOAD( "82s129.3k",    0x0100, 0x0100, 0x77245b66 )	/* timing - not used */
ROM_END

ROM_START( mrtnt_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "tnt.1",        0x0000, 0x1000, 0x0e836586 )
	ROM_LOAD( "tnt.2",        0x1000, 0x1000, 0x779c4c5b )
	ROM_LOAD( "tnt.3",        0x2000, 0x1000, 0xad6fc688 )
	ROM_LOAD( "tnt.4",        0x3000, 0x1000, 0xd77557b3 )

	ROM_REGION_DISPOSE(0x2000)  /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tnt.5",        0x0000, 0x1000, 0x3038cc0e )
	ROM_LOAD( "tnt.6",        0x1000, 0x1000, 0x97634d8b )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "mrtnt08.bin",  0x0000, 0x0020, 0x2fc650bd )	/* wrong! from Pac Man */
	ROM_LOAD( "mrtnt04.bin",  0x0020, 0x0100, 0x3eb3a8e4 )	/* wrong! from Pac Man */

	ROM_REGION(0x0200)	/* sound PROMs */
	ROM_LOAD( "mrtnt01.bin",  0x0000, 0x0100, 0xa9cc86bf )
	ROM_LOAD( "mrtnt03.bin",  0x0100, 0x0100, 0x77245b66 )	/* timing - not used */
ROM_END

ROM_START( lizwiz_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "6e.cpu",       0x0000, 0x1000, 0x32bc1990 )
	ROM_LOAD( "6f.cpu",       0x1000, 0x1000, 0xef24b414 )
	ROM_LOAD( "6h.cpu",       0x2000, 0x1000, 0x30bed83d )
	ROM_LOAD( "6j.cpu",       0x3000, 0x1000, 0xdd09baeb )
	ROM_LOAD( "wiza",         0x8000, 0x1000, 0xf6dea3a6 )
	ROM_LOAD( "wizb",         0x9000, 0x1000, 0xf27fb5a8 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5e.cpu",       0x0000, 0x1000, 0x45059e73 )
	ROM_LOAD( "5f.cpu",       0x1000, 0x1000, 0xd2469717 )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "7f.cpu",       0x0000, 0x0020, 0x7549a947 )
	ROM_LOAD( "4a.cpu",       0x0020, 0x0100, 0x5fdca536 )

	ROM_REGION(0x0100)	/* sound prom */
	ROM_LOAD( "pacman.spr",   0x0000, 0x0100, 0xa9cc86bf )
ROM_END

ROM_START( theglob_rom )
	ROM_REGION(0x20000)	/* 64k for code */
	ROM_LOAD( "glob.u2",      0x0000, 0x2000, 0x829d0bea )
	ROM_LOAD( "glob.u3",      0x2000, 0x2000, 0x31de6628 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "glob.5e",      0x0000, 0x1000, 0x53688260 )
	ROM_LOAD( "glob.5f",      0x1000, 0x1000, 0x051f59c7 )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "glob.7f",      0x0000, 0x0020, 0x1f617527 )
	ROM_LOAD( "glob.4a",      0x0020, 0x0100, 0x28faa769 )

	ROM_REGION(0x0100)	/* sound prom */
	ROM_LOAD( "pacman.spr",   0x0000, 0x0100, 0xa9cc86bf )
ROM_END

ROM_START( beastf_rom )
	ROM_REGION(0x20000)	/* 64k for code */
	ROM_LOAD( "bf-u2.bin",    0x0000, 0x2000, 0x3afc517b )
	ROM_LOAD( "bf-u3.bin",    0x2000, 0x2000, 0x8dbd76d0 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "beastf.5e",    0x0000, 0x1000, 0x5654dc34 )
	ROM_LOAD( "beastf.5f",    0x1000, 0x1000, 0x1b30ca61 )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "glob.7f",      0x0000, 0x0020, 0x1f617527 )
	ROM_LOAD( "glob.4a",      0x0020, 0x0100, 0x28faa769 )

	ROM_REGION(0x0100)	/* sound prom */
	ROM_LOAD( "pacman.spr",   0x0000, 0x0100, 0xa9cc86bf )
ROM_END

ROM_START( jumpshot_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "6e",           0x0000, 0x1000, 0xf00def9a )
	ROM_LOAD( "6f",           0x1000, 0x1000, 0xf70deae2 )
	ROM_LOAD( "6h",           0x2000, 0x1000, 0x894d6f68 )
	ROM_LOAD( "6j",           0x3000, 0x1000, 0xf15a108a )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5e",           0x0000, 0x1000, 0xd9fa90f5 )
	ROM_LOAD( "5f",           0x1000, 0x1000, 0x2ec711c1 )

	ROM_REGION(0x0120)	/* color PROMs */
	ROM_LOAD( "prom.7f",      0x0000, 0x0020, 0x872b42f3 )
	ROM_LOAD( "prom.4a",      0x0020, 0x0100, 0x0399f39f )

	ROM_REGION(0x0200)	/* sound PROMs */
	ROM_LOAD( "prom.1m",      0x0000, 0x0100, 0xa9cc86bf )
	ROM_LOAD( "prom.3m",      0x0100, 0x0100, 0x77245b66 )	/* timing - not used */
ROM_END



static void ponpoko_decode(void)
{
	int i, j;
	unsigned char *RAM, temp;

	/* The gfx data is swapped wrt the other Pac Man hardware games. */
	/* Here we revert it to the usual format. */

	/* Characters */
	RAM = Machine->memory_region[1];
	for (i = 0; i < 0x1000; i += 0x10)
	{
		for (j = 0; j < 8; j++)
		{
			temp          = RAM[i+j+0x08];
			RAM[i+j+0x08] = RAM[i+j+0x00];
			RAM[i+j+0x00] = temp;
		}
	}

	/* Sprites */
	for (; i < 0x2000; i += 0x20)
	{
		for (j = 0; j < 8; j++)
		{
			temp          = RAM[i+j+0x18];
			RAM[i+j+0x18] = RAM[i+j+0x10];
			RAM[i+j+0x10] = RAM[i+j+0x08];
			RAM[i+j+0x08] = RAM[i+j+0x00];
			RAM[i+j+0x00] = temp;
		}
	}
}


static void eyes_decode(void)
{
	int i;
	unsigned char *RAM;

	/* CPU ROMs */

	/* Data lines D3 and D5 swapped */
	RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	for (i = 0; i < 0x4000; i++)
	{
		RAM[i] =  (RAM[i] & 0xc0) | ((RAM[i] & 0x08) << 2) |
				  (RAM[i] & 0x10) | ((RAM[i] & 0x20) >> 2) | (RAM[i] & 0x07);
	}


	/* Graphics ROMs */

	/* Data lines D4 and D6 and address lines A0 and A2 are swapped */
	RAM = Machine->memory_region[1];
	for (i = 0; i < 0x2000; i += 8)
	{
		int j;
		unsigned char swapbuffer[8];

		for (j = 0; j < 8; j++)
		{
			swapbuffer[j] = RAM[i + (j >> 2) + (j & 2) + ((j & 1) << 2)];
		}

		for (j = 0; j < 8; j++)
		{
			char ch = swapbuffer[j];

			RAM[i + j] = (ch & 0x80) | ((ch & 0x10) << 2) |
				         (ch & 0x20) | ((ch & 0x40) >> 2) | (ch & 0x0f);
		}
	}
}



static void copytoscreen(int mem, int len, int screen, int direction)
{
	char buf[10];
	int hi;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	hi =      (RAM[mem + direction*3] & 0x0f) +
		      (RAM[mem + direction*3] >> 4)   * 10 +
		      (RAM[mem + direction*2] & 0x0f) * 100 +
		      (RAM[mem + direction*2] >> 4)   * 1000 +
		      (RAM[mem + direction*1] & 0x0f) * 10000 +
		      (RAM[mem + direction*1] >> 4)   * 100000;

	if (len == 4)
	{
		hi += (RAM[mem + 0*direction] & 0x0f) * 1000000 +
		      (RAM[mem + 0*direction] >> 4)   * 10000000;
	}

	if (hi)
	{
		sprintf(buf,"%8d",hi);
		if (buf[2] != ' ') videoram_w(screen + direction*0, buf[2]-'0');
		if (buf[3] != ' ') videoram_w(screen + direction*1, buf[3]-'0');
		if (buf[4] != ' ') videoram_w(screen + direction*2, buf[4]-'0');
		if (buf[5] != ' ') videoram_w(screen + direction*3, buf[5]-'0');
		if (buf[6] != ' ') videoram_w(screen + direction*4, buf[6]-'0');
		                   videoram_w(screen + direction*5, buf[7]-'0');
	}
}

static int pacman_hiload(void)
{
	static int resetcount;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* during a reset, leave time to the game to clear the screen */
	if (++resetcount < 60) return 0;

	/* wait for "HIGH SCORE" to be on screen */
	if (memcmp(&RAM[0x43d1],"\x48\x47",2) == 0)
	{
		void *f;


		resetcount = 0;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4e88],4);

			/* also copy the high score to the screen, otherwise it won't be */
			/* updated */
			copytoscreen(0x4e8b, 4, 0x3f2, -1);

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void pacman_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4e88],4);
		osd_fclose(f);
	}
}



static int crush_hiload(void)
{
	static int resetcount;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* during a reset, leave time to the game to clear the screen */
	if (++resetcount < 60) return 0;

	/* wait for "HI SCORE" to be on screen */
	if (memcmp(&RAM[0x43d0],"\x53\x40",2) == 0)
	{
		void *f;


		resetcount = 0;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4c80],3);

			/* also copy the high score to the screen, otherwise it won't be */
			/* updated */
			copytoscreen(0x4c83, 3, 0x3f3, -1);

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void crush_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4c80],3);
		osd_fclose(f);
	}
}



static int eyes_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0x4d30],"\x90\x52\x00",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4cf7],0x3c);

			/* also copy the high score to the screen, otherwise it won't be */
			/* updated */
			copytoscreen(0x4cfd, 3, 0x3f2, -1);

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}


static void eyes_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4cf7],0x3c);
		osd_fclose(f);
	}
}

static int mrtnt_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0x4cec],"\x40\x86\x01",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4cb3], 6 * 10);

			/* also copy the high score to the screen, otherwise it won't be */
			/* updated */
			copytoscreen(0x4cb9, 3, 0x3f2, -1);

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}


static void mrtnt_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4cb3], 6 * 10);
		osd_fclose(f);
	}
}


static int ponpoko_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0x406c],"\x0f\x0f\x0f\x0f\x0f\x00",6) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x4e5a],0x14);
			memcpy(&RAM[0x4c40], &RAM[0x4e5a], 3);

			/* also copy the high score to the screen, otherwise it won't be */
			/* updated */
			copytoscreen(0x4e59, 3, 0x6c, 1);

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void ponpoko_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4e5a],0x14);
		osd_fclose(f);
	}
}


static int theglob_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	int address = 0x4c48;
	if (strcmp(Machine->gamedrv->name, "beastf") == 0) address = 0x4c46;

	if (memcmp(&RAM[address],"MOB",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[address],0x50);

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void theglob_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	int address = 0x4c48;
	if (strcmp(Machine->gamedrv->name, "beastf") == 0) address = 0x4c46;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[address],0x50);
		osd_fclose(f);
	}
}


#define BASE_CREDITS "Allard van der Bas (original code)\nNicola Salmoria (MAME driver)"


struct GameDriver pacman_driver =
{
	__FILE__,
	0,
	"pacman",
	"Pac Man (Midway)",
	"1980",
	"[Namco] (Midway license)",
	BASE_CREDITS,
	0,
	&machine_driver,
	0,

	pacman_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	pacman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	pacman_hiload, pacman_hisave
};

struct GameDriver namcopac_driver =
{
	__FILE__,
	&pacman_driver,
	"namcopac",
	"Pac Man (Namco)",
	"1980",
	"Namco",
	BASE_CREDITS,
	0,
	&machine_driver,
	0,

	namcopac_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	pacman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	pacman_hiload, pacman_hisave
};

struct GameDriver pacmanjp_driver =
{
	__FILE__,
	&pacman_driver,
	"pacmanjp",
	"Pac Man (Namco, alternate)",
	"1980",
	"Namco",
	BASE_CREDITS,
	0,
	&machine_driver,
	0,

	pacmanjp_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	pacman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	pacman_hiload, pacman_hisave
};

struct GameDriver pacmod_driver =
{
	__FILE__,
	&pacman_driver,
	"pacmod",
	"Pac Man (harder)",
	"1981",
	"[Namco] (Midway license)",
	BASE_CREDITS,
	0,
	&machine_driver,
	0,

	pacmod_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	pacman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	pacman_hiload, pacman_hisave
};

struct GameDriver hangly_driver =
{
	__FILE__,
	&pacman_driver,
	"hangly",
	"Hangly Man",
	"1981",
	"hack",
	BASE_CREDITS,
	0,
	&machine_driver,
	0,

	hangly_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	pacman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	pacman_hiload, pacman_hisave
};

struct GameDriver puckman_driver =
{
	__FILE__,
	&pacman_driver,
	"puckman",
	"Puck Man",
	"1980",
	"hack",
	BASE_CREDITS,
	0,
	&machine_driver,
	0,

	puckman_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	pacman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	pacman_hiload, pacman_hisave
};

struct GameDriver piranha_driver =
{
	__FILE__,
	&pacman_driver,
	"piranha",
	"Piranha",
	"1981",
	"hack",
	BASE_CREDITS,
	0,
	&machine_driver,
	0,

	piranha_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	mspacman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	pacman_hiload, pacman_hisave
};

struct GameDriver pacplus_driver =
{
	__FILE__,
	0,
	"pacplus",
	"Pac Man Plus",
	"1982",
	"[Namco] (Midway license)",
	BASE_CREDITS"\nClay Cowgill (information)\nMike Balfour (information)",
	0,
	&machine_driver,
	0,

	pacplus_rom,
	pacplus_decode, 0,
	0,
	0,	/* sound_prom */

	pacman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	pacman_hiload, pacman_hisave
};

struct GameDriver mspacman_driver =
{
	__FILE__,
	0,
	"mspacman",
	"Ms. Pac Man",
	"1981",
	"bootleg",
	BASE_CREDITS,
	0,
	&machine_driver,
	0,

	mspacman_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	mspacman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	pacman_hiload, pacman_hisave
};

struct GameDriver mspacatk_driver =
{
	__FILE__,
	&mspacman_driver,
	"mspacatk",
	"Miss Pac Plus",
	"1981",
	"hack",
	BASE_CREDITS,
	0,
	&machine_driver,
	0,

	mspacatk_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	mspacman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	pacman_hiload, pacman_hisave
};

extern struct GameDriver maketrax_driver;
struct GameDriver crush_driver =
{
	__FILE__,
	&maketrax_driver,
	"crush",
	"Crush Roller",
	"1981",
	"bootleg",
	BASE_CREDITS"\nGary Walton (color info)\nSimon Walls (color info)",
	0,
	&machine_driver,
	0,

	crush_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	crush_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	crush_hiload, crush_hisave
};

struct GameDriver ponpoko_driver =
{
	__FILE__,
	0,
	"ponpoko",
	"Ponpoko",
	"1982",
	"Sigma Ent. Inc.",
	BASE_CREDITS,
	0,
	&machine_driver,
	0,

	ponpoko_rom,
	ponpoko_decode, 0,
	0,
	0,	/* sound_prom */

	ponpoko_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,	/* probably correct */
	ORIENTATION_DEFAULT,

	ponpoko_hiload, ponpoko_hisave
};

struct GameDriver eyes_driver =
{
	__FILE__,
	0,
	"eyes",
	"Eyes",
	"1982",
	"Digitrex Techstar (Rock-ola license)",
	"Zsolt Vasvari\n"BASE_CREDITS,
	0,
	&machine_driver,
	0,

	eyes_rom,
	eyes_decode, 0,
	0,
	0,	/* sound_prom */

	eyes_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,	/* wrong!! */
	ORIENTATION_ROTATE_90,

	eyes_hiload, eyes_hisave
};

struct GameDriver mrtnt_driver =
{
	__FILE__,
	0,
	"mrtnt",
	"Mr. TNT",
	"1983",
	"Telko",
	"Zsolt Vasvari\n"BASE_CREDITS,
	0,
	&machine_driver,
	0,

	mrtnt_rom,
	eyes_decode, 0,
	0,
	0,	/* sound_prom */

	eyes_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,	/* wrong!! */
	ORIENTATION_ROTATE_90,

	mrtnt_hiload, mrtnt_hisave
};

struct GameDriver lizwiz_driver =
{
	__FILE__,
	0,
	"lizwiz",
	"Lizard Wizard",
	"1985",
	"Techstar (Sunn license)",
	BASE_CREDITS,
	0,
	&machine_driver,
	0,

	lizwiz_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	lizwiz_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver theglob_driver =
{
	__FILE__,
	0,
	"theglob",
	"The Glob (Pac Man hardware)",
	"1983",
	"Epos Corporation",
	BASE_CREDITS"\nClay Cowgill (information)\nMike Balfour (information)",
	0,
	&theglob_machine_driver,
	0,

	theglob_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	theglob_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	theglob_hiload,theglob_hisave
};

struct GameDriver beastf_driver =
{
	__FILE__,
	&theglob_driver,
	"beastf",
	"Beastie Feastie (Pac Man hardware)",
	"1984",
	"Epos Corporation",
	BASE_CREDITS"\nClay Cowgill (information)\nMike Balfour (information)",
	0,
	&theglob_machine_driver,
	0,

	beastf_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	theglob_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	theglob_hiload,theglob_hisave
};

/* not working, encrypted */
struct GameDriver jumpshot_driver =
{
	__FILE__,
	0,
	"jumpshot",
	"Jump Shot",
	"????",
	"?????",
	BASE_CREDITS,
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	jumpshot_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	pacman_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};
