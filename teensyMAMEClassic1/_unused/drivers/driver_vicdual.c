/***************************************************************************

VIC Dual Game board memory map (preliminary)

0000-3fff ROM
4000-7fff ROM mirror image (used in most games)
8000-87ff looks like extra work RAM in Safari
e000-e3ff Video RAM + work RAM
e400-e7ff RAM
e800-efff Character generator RAM

I/O ports:

The memory map is the same for many games, but the I/O ports change. The
following ones are for Carnival, and apply to many other games as well.

read:
00        IN0
          bit 0 = connector
          bit 1 = connector
          bit 2 = dsw
          bit 3 = dsw
          bit 4 = connector
          bit 5 = connector
          bit 6 = seems unused
          bit 7 = seems unused

01        IN1
          bit 0 = connector
          bit 1 = connector
          bit 2 = dsw
          bit 3 = vblank
          bit 4 = connector
          bit 5 = connector
          bit 6 = seems unused
          bit 7 = seems unused

02        IN2
          bit 0 = connector
          bit 1 = connector
          bit 2 = dsw
          bit 3 = timer? is this used?
          bit 4 = connector
          bit 5 = connector
          bit 6 = seems unused
          bit 7 = seems unused

03        IN3
          bit 0 = connector
          bit 1 = connector
          bit 2 = dsw
          bit 3 = COIN (must reset the CPU to make the game acknowledge it)
          bit 4 = connector
          bit 5 = connector
          bit 6 = seems unused
          bit 7 = seems unused

write:
	(ports 1 and 2: see definitions in sound driver)

08        ?

40        palette bank

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "I8039/I8039.h"



#define	PSG_CLOCK_CARNIVAL	( 3579545 / 3 )	/* Hz */



extern unsigned char *vicdual_characterram;
void vicdual_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void vicdual_characterram_w(int offset,int data);
void vicdual_palette_bank_w(int offset, int data);
void vicdual_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void carnival_sh_port1_w(int offset, int data);
void carnival_sh_port2_w(int offset, int data);

int carnival_music_port_t1_r( int offset );

void carnival_music_port_1_w( int offset, int data );
void carnival_music_port_2_w( int offset, int data );


static int protection_data;

void samurai_protection_w(int offset,int data)
{
	protection_data = data;
}

int samurai_input_r(int offset)
{
	int answer = 0;

	if (protection_data == 0xab) answer = 0x02;
	else if (protection_data == 0x1d) answer = 0x0c;

	return (readinputport(1 + offset) & 0xfd) | ((answer >> offset) & 0x02);
}



static struct MemoryReadAddress vicdual_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xe000, 0xefff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress vicdual_writemem[] =
{
	{ 0x7f00, 0x7f00, samurai_protection_w },	/* Samurai only */
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xe000, 0xe3ff, videoram_w, &videoram, &videoram_size },
	{ 0xe400, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xefff, vicdual_characterram_w, &vicdual_characterram },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress samurai_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xe000, 0xefff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress samurai_writemem[] =
{
	{ 0x7f00, 0x7f00, samurai_protection_w },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xe000, 0xe3ff, videoram_w, &videoram, &videoram_size },
	{ 0xe400, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xefff, vicdual_characterram_w, &vicdual_characterram },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress heiankyo_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress heiankyo_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8fff, vicdual_characterram_w, &vicdual_characterram },
	{ -1 }  /* end of table */
};


static struct IOReadPort readport_2Aports[] =
{
	{ 0x01, 0x01, input_port_0_r },
	{ 0x08, 0x08, input_port_1_r },
	{ -1 }  /* end of table */
};

static struct IOReadPort readport_2Bports[] =
{
	{ 0x03, 0x03, input_port_0_r },
	{ 0x08, 0x08, input_port_1_r },
	{ -1 }  /* end of table */
};

static struct IOReadPort readport_3ports[] =
{
	{ 0x01, 0x01, input_port_0_r },
	{ 0x04, 0x04, input_port_1_r },
	{ 0x08, 0x08, input_port_2_r },
	{ -1 }  /* end of table */
};

static struct IOReadPort readport_4ports[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ -1 }  /* end of table */
};

static struct IOReadPort readport_samuraiports[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x03, samurai_input_r },	/* bit 1 of these ports returns a protection code */
	{ -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x01, 0x01, carnival_sh_port1_w },
	{ 0x02, 0x02, carnival_sh_port2_w },
	{ 0x40, 0x40, vicdual_palette_bank_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress i8039_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress i8039_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct IOReadPort i8039_readport[] =
{
	{ I8039_t1, I8039_t1, carnival_music_port_t1_r },
	{ -1 }
};

static struct IOWritePort i8039_writeport[] =
{
	{ I8039_p1, I8039_p1, carnival_music_port_1_w },
	{ I8039_p2, I8039_p2, carnival_music_port_2_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( depthch_input_ports )
	PORT_START	/* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_DIPNAME (0x30, 0x30, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING (   0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING (   0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x30, "1 Coin/1 Credit" )
    PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
    PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
INPUT_PORTS_END

INPUT_PORTS_START( safari_input_ports )
	PORT_START	/* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2, "Aim Up", OSD_KEY_A, 0, 0)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON3, "Aim Down", OSD_KEY_Z, 0, 0)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
    PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME (0x30, 0x30, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING (   0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING (   0x20, "2 Coins/1 Credit" )
	PORT_DIPSETTING (   0x30, "1 Coin/1 Credit" )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
INPUT_PORTS_END

INPUT_PORTS_START( frogs_input_ports )
	PORT_START	/* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )	/* maybe Demo Sounds */
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Free Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x10, "Yes" )
	PORT_DIPNAME( 0x20, 0x20, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x20, "90" )
	PORT_DIPNAME( 0x40, 0x40, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "1 Coin/1 Credit" )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
    PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
INPUT_PORTS_END

INPUT_PORTS_START( sspaceat_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )	/* unknown, but used */
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0e, 0x0e, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0e, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x0a, "5" )
	PORT_DIPSETTING(    0x06, "6" )
/* the following are duplicates
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x02, "5" ) */
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )	/* unknown, but used */
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x80, 0x00, "Show Credits", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
INPUT_PORTS_END

INPUT_PORTS_START( headon_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unknown, but used (sound related?) */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
INPUT_PORTS_END

INPUT_PORTS_START ( invho2_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x04, "Head On Lives (1/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "+0" )
	PORT_DIPSETTING(    0x00, "+1" )
	PORT_DIPNAME( 0x08, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Head On Lives (2/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "+0" )
	PORT_DIPSETTING(    0x00, "+1" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Invinco Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x04, "6" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )	/* probably unused */

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	/* There's probably a bug in the code: this would likely be the second */
	/* bit of the Invinco Lives setting, but the game reads bit 3 instead */
	/* of bit 2. */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_TOGGLE, "Game Select", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START ( samurai_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x04, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection, see samurai_input_r() */
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )	/* unknown, but used */
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )	/* seems to be on port 2 instead */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection, see samurai_input_r() */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )	/* either vblank, or a timer. In the */
												/* Carnival schematics, it's a timer. */
//	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* protection, see samurai_input_r() */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( invinco_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x80, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x7e, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
INPUT_PORTS_END

INPUT_PORTS_START ( invds_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Invinco Lives (1/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "+0" )
	PORT_DIPSETTING(    0x04, "+1" )
	PORT_DIPNAME( 0x08, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Invinco Lives (2/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "+0" )
	PORT_DIPSETTING(    0x04, "+2" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Deep Scan Lives (1/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "+0" )
	PORT_DIPSETTING(    0x04, "+1" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	/* +1 and +2 gives 2 lives instead of 6 */
	PORT_DIPNAME( 0x04, 0x00, "Deep Scan Lives (2/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "+0" )
	PORT_DIPSETTING(    0x00, "+2" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_TOGGLE, "Game Select", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START ( tranqgun_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START ( spacetrk_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x04, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x08, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )	/* unknown, but used */
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unknown, but could be used */
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )	/* unknown, but used */
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unknown, but could be used */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START ( sptrekct_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x04, 0x04, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x08, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )	/* unknown, but used */
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unknown, but could be used */
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )	/* unknown, but used */
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* unknown, but could be used */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START ( carnival_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* unknown, but used */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START ( pulsar_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x04, "Lives (1/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "+0" )
	PORT_DIPSETTING(    0x00, "+2" )
	PORT_DIPNAME( 0x08, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x04, "Lives (2/2)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "+0" )
	PORT_DIPSETTING(    0x00, "+1" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_DIPNAME( 0x04, 0x00, "Unused", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START ( heiankyo_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )	/* bonus life? */
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "2 Players Mode", IP_KEY_NONE)
	PORT_DIPSETTING(    0x08, "Alternate")
	PORT_DIPSETTING(    0x00, "Simultaneous")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )	/* bonus life? */
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )	/* bonus life? */
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* timer - unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )	/* probably unused */

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x04, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE | IPF_RESETCPU, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 30 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0xe800, &charlayout, 0, 32 },	/* the game dynamically modifies this */
	{ -1 }	/* end of array */
};



static struct Samplesinterface samples_interface =
{
	10	/* 10 channels */
};



/* There are three version of the machine driver, identical apart from the readport */
/* array and interrupt handler */

#define MACHINEDRIVER(NAME,MEM,PORT)				\
static struct MachineDriver NAME =					\
{													\
	/* basic machine hardware */					\
	{												\
		{											\
			CPU_Z80,								\
			3867120/2,								\
			0,										\
			MEM##_readmem,MEM##_writemem,readport_##PORT,writeport,	\
			ignore_interrupt,1						\
		}											\
	},												\
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */	\
	1,	/* single CPU, no need for interleaving */	\
	0,												\
													\
	/* video hardware */							\
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },		\
	gfxdecodeinfo,									\
	64, 64,											\
	vicdual_vh_convert_color_prom,					\
													\
	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,			\
	0,												\
	generic_vh_start,								\
	generic_vh_stop,								\
	vicdual_vh_screenrefresh,						\
													\
	/* sound hardware */							\
	0,0,0,0,										\
	{												\
		{											\
			SOUND_SAMPLES,							\
			&samples_interface						\
		}											\
	}												\
};

MACHINEDRIVER( vicdual_2Aports_machine_driver, vicdual, 2Aports )
MACHINEDRIVER( vicdual_2Bports_machine_driver, vicdual, 2Bports )
MACHINEDRIVER( vicdual_3ports_machine_driver, vicdual, 3ports )
MACHINEDRIVER( vicdual_4ports_machine_driver, vicdual, 4ports )
MACHINEDRIVER( samurai_machine_driver, samurai, samuraiports )	/* this game has a simple protection */
MACHINEDRIVER( heiankyo_machine_driver, heiankyo, 4ports )


static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chips */
	PSG_CLOCK_CARNIVAL,
	{ 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

/* don't know if any of the other games use the 8048 music board */
/* so, we won't burden those drivers with the extra music handling */
static struct MachineDriver carnival_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120/2,
			0,
			vicdual_readmem,vicdual_writemem,readport_4ports,writeport,
			ignore_interrupt,1
		},
		{
			CPU_I8039 | CPU_AUDIO_CPU,
			( ( 3579545 / 5 ) / 3 ),
			2,
			i8039_readmem,i8039_writemem,i8039_readport,i8039_writeport,
			ignore_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	64, 64,
	vicdual_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	vicdual_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( depthch_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "50a",          0x0000, 0x0400, 0x56c5ffed )
	ROM_LOAD( "51a",          0x0400, 0x0400, 0x695eb81f )
	ROM_LOAD( "52",           0x0800, 0x0400, 0xaed0ba1b )
	ROM_LOAD( "53",           0x0c00, 0x0400, 0x2ccbd2d0 )
	ROM_LOAD( "54a",          0x1000, 0x0400, 0x1b7f6a43 )
	ROM_LOAD( "55a",          0x1400, 0x0400, 0x9fc2eb41 )
ROM_END

ROM_START( safari_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "3160066.u48",  0x0000, 0x0400, 0x2a26b098 )
	ROM_LOAD( "3160065.u47",  0x0400, 0x0400, 0xb776f7db )
	ROM_LOAD( "3160064.u46",  0x0800, 0x0400, 0x19d8c196 )
	ROM_LOAD( "3160063.u45",  0x0c00, 0x0400, 0x028bad25 )
	ROM_LOAD( "3160062.u44",  0x1000, 0x0400, 0x504e0575 )
	ROM_LOAD( "3160061.u43",  0x1400, 0x0400, 0xd4c528e0 )
	ROM_LOAD( "3160060.u42",  0x1800, 0x0400, 0x48c7b0cc )
	ROM_LOAD( "3160059.u41",  0x1c00, 0x0400, 0x3f7baaff )
	ROM_LOAD( "3160058.u40",  0x2000, 0x0400, 0x0d5058f1 )
	ROM_LOAD( "3160057.u39",  0x2400, 0x0400, 0x298e8c41 )
ROM_END

ROM_START( frogs_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "119a.u48",     0x0000, 0x0400, 0xb1d1fce4 )
	ROM_LOAD( "118a.u47",     0x0400, 0x0400, 0x12fdcc05 )
	ROM_LOAD( "117a.u46",     0x0800, 0x0400, 0x8a5be424 )
	ROM_LOAD( "116b.u45",     0x0c00, 0x0400, 0x09b82619 )
	ROM_LOAD( "115a.u44",     0x1000, 0x0400, 0x3d4e4fa8 )
	ROM_LOAD( "114a.u43",     0x1400, 0x0400, 0x04a21853 )
	ROM_LOAD( "113a.u42",     0x1800, 0x0400, 0x02786692 )
	ROM_LOAD( "112a.u41",     0x1c00, 0x0400, 0x0be2a058 )
ROM_END

ROM_START( sspaceat_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "139.u27",      0x0000, 0x0400, 0x9f2112fc )
	ROM_LOAD( "140.u26",      0x0400, 0x0400, 0xddbeed35 )
	ROM_LOAD( "141.u25",      0x0800, 0x0400, 0xb159924d )
	ROM_LOAD( "142.u24",      0x0c00, 0x0400, 0xf2ebfce9 )
	ROM_LOAD( "143.u23",      0x1000, 0x0400, 0xbff34a66 )
	ROM_LOAD( "144.u22",      0x1400, 0x0400, 0xfa062d58 )
	ROM_LOAD( "145.u21",      0x1800, 0x0400, 0x7e950614 )
	ROM_LOAD( "146.u20",      0x1c00, 0x0400, 0x8ba94fbc )

	ROM_REGION(0x0020) /* Color PROMs */
	/* missing! */
ROM_END

ROM_START( headon_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "163a",         0x0000, 0x0400, 0x4bb51259 )
	ROM_LOAD( "164a",         0x0400, 0x0400, 0xaeac8c5f )
	ROM_LOAD( "165a",         0x0800, 0x0400, 0xf1a0cb72 )
	ROM_LOAD( "166c",         0x0c00, 0x0400, 0x65d12951 )
	ROM_LOAD( "167c",         0x1000, 0x0400, 0x2280831e )
	ROM_LOAD( "192a",         0x1400, 0x0400, 0xed4666f2 )
	ROM_LOAD( "193a",         0x1800, 0x0400, 0x37a1df4c )
ROM_END

ROM_START( invho2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "271b.u33",     0x0000, 0x0400, 0x44356a73 )
	ROM_LOAD( "272b.u32",     0x0400, 0x0400, 0xbd251265 )
	ROM_LOAD( "273b.u31",     0x0800, 0x0400, 0x2fc80cd9 )
	ROM_LOAD( "274b.u30",     0x0c00, 0x0400, 0x4fac4210 )
	ROM_LOAD( "275b.u29",     0x1000, 0x0400, 0x85af508e )
	ROM_LOAD( "276b.u28",     0x1400, 0x0400, 0xe305843a )
	ROM_LOAD( "277b.u27",     0x1800, 0x0400, 0xb6b4221e )
	ROM_LOAD( "278b.u26",     0x1c00, 0x0400, 0x74d42250 )
	ROM_LOAD( "279b.u8",      0x2000, 0x0400, 0x8d30a3e0 )
	ROM_LOAD( "280b.u7",      0x2400, 0x0400, 0xb5ee60ec )
	ROM_LOAD( "281b.u6",      0x2800, 0x0400, 0x21a6d4f2 )
	ROM_LOAD( "282b.u5",      0x2c00, 0x0400, 0x07d54f8a )
	ROM_LOAD( "283b.u4",      0x3000, 0x0400, 0xbdbe7ec1 )
	ROM_LOAD( "284b.u3",      0x3400, 0x0400, 0xae9e9f16 )
	ROM_LOAD( "285b.u2",      0x3800, 0x0400, 0x8dc3ec34 )
	ROM_LOAD( "286b.u1",      0x3c00, 0x0400, 0x4bab9ba2 )

	ROM_REGION(0x0020) /* Color PROMs */
	ROM_LOAD( "316-0287.u49", 0x0000, 0x0020, 0xd4374b01 )
ROM_END

ROM_START( samurai_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "epr289.u33",   0x0000, 0x0400, 0xa1a9cb03 )
	ROM_LOAD( "epr290.u32",   0x0400, 0x0400, 0x49fede51 )
	ROM_LOAD( "epr291.u31",   0x0800, 0x0400, 0x6503dd72 )
	ROM_LOAD( "epr292.u30",   0x0c00, 0x0400, 0x179c224f )
	ROM_LOAD( "epr366.u29",   0x1000, 0x0400, 0x3df2abec )
	ROM_LOAD( "epr355.u28",   0x1400, 0x0400, 0xb24517a4 )
	ROM_LOAD( "epr367.u27",   0x1800, 0x0400, 0x992a6e5a )
	ROM_LOAD( "epr368.u26",   0x1c00, 0x0400, 0x403c72ce )
	ROM_LOAD( "epr369.u8",    0x2000, 0x0400, 0x3cfd115b )
	ROM_LOAD( "epr370.u7",    0x2400, 0x0400, 0x2c30db12 )
	ROM_LOAD( "epr299.u6",    0x2800, 0x0400, 0x87c71139 )
	ROM_LOAD( "epr371.u5",    0x2c00, 0x0400, 0x761f56cf )
	ROM_LOAD( "epr301.u4",    0x3000, 0x0400, 0x23de1ff7 )
	ROM_LOAD( "epr372.u3",    0x3400, 0x0400, 0x292cfd89 )

	ROM_REGION(0x0020) /* Color PROMs */
	ROM_LOAD( "pr55.clr",     0x0000, 0x0020, 0x975f5fb0 )
ROM_END

ROM_START( invinco_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "310a.u27",     0x0000, 0x0400, 0xe3931365 )
	ROM_LOAD( "311a.u26",     0x0400, 0x0400, 0xde1a6c4a )
	ROM_LOAD( "312a.u25",     0x0800, 0x0400, 0xe3c08f39 )
	ROM_LOAD( "313a.u24",     0x0c00, 0x0400, 0xb680b306 )
	ROM_LOAD( "314a.u23",     0x1000, 0x0400, 0x790f07d9 )
	ROM_LOAD( "315a.u22",     0x1400, 0x0400, 0x0d13bed2 )
	ROM_LOAD( "316a.u21",     0x1800, 0x0400, 0x88d7eab8 )
	ROM_LOAD( "317a.u20",     0x1c00, 0x0400, 0x75389463 )
	ROM_LOAD( "318a.uxx",     0x2000, 0x0400, 0x0780721d )

	ROM_REGION(0x0020) /* Color PROMs */
	/* missing! */
ROM_END

ROM_START( invds_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "367.u33",      0x0000, 0x0400, 0xe6a33eae )
	ROM_LOAD( "368.u32",      0x0400, 0x0400, 0x421554a8 )
	ROM_LOAD( "369.u31",      0x0800, 0x0400, 0x531e917a )
	ROM_LOAD( "370.u30",      0x0c00, 0x0400, 0x2ad68f8c )
	ROM_LOAD( "371.u29",      0x1000, 0x0400, 0x1b98dc5c )
	ROM_LOAD( "372.u28",      0x1400, 0x0400, 0x3a72190a )
	ROM_LOAD( "373.u27",      0x1800, 0x0400, 0x3d361520 )
	ROM_LOAD( "374.u26",      0x1c00, 0x0400, 0xe606e7d9 )
	ROM_LOAD( "375.u8",       0x2000, 0x0400, 0xadbe8d32 )
	ROM_LOAD( "376.u7",       0x2400, 0x0400, 0x79409a46 )
	ROM_LOAD( "377.u6",       0x2800, 0x0400, 0x3f021a71 )
	ROM_LOAD( "378.u5",       0x2c00, 0x0400, 0x49a542b0 )
	ROM_LOAD( "379.u4",       0x3000, 0x0400, 0xee140e49 )
	ROM_LOAD( "380.u3",       0x3400, 0x0400, 0x688ba831 )
	ROM_LOAD( "381.u2",       0x3800, 0x0400, 0x798ba0c7 )
	ROM_LOAD( "382.u1",       0x3c00, 0x0400, 0x8d195c24 )

	ROM_REGION(0x0020) /* Color PROMs */
	ROM_LOAD( "316-246",      0x0000, 0x0020, 0xfe4406cb )
ROM_END

ROM_START( tranqgun_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "u33.bin",      0x0000, 0x0400, 0x6d50e902 )
	ROM_LOAD( "u32.bin",      0x0400, 0x0400, 0xf0ba0e60 )
	ROM_LOAD( "u31.bin",      0x0800, 0x0400, 0x9fe440d3 )
	ROM_LOAD( "u30.bin",      0x0c00, 0x0400, 0x1041608e )
	ROM_LOAD( "u29.bin",      0x1000, 0x0400, 0xfb5de95f )
	ROM_LOAD( "u28.bin",      0x1400, 0x0400, 0x03fd8727 )
	ROM_LOAD( "u27.bin",      0x1800, 0x0400, 0x3d93239b )
	ROM_LOAD( "u26.bin",      0x1c00, 0x0400, 0x20f64a7f )
	ROM_LOAD( "u8.bin",       0x2000, 0x0400, 0x5121c695 )
	ROM_LOAD( "u7.bin",       0x2400, 0x0400, 0xb13d21f7 )
	ROM_LOAD( "u6.bin",       0x2800, 0x0400, 0x603cee59 )
	ROM_LOAD( "u5.bin",       0x2c00, 0x0400, 0x7f25475f )
	ROM_LOAD( "u4.bin",       0x3000, 0x0400, 0x57dc3123 )
	ROM_LOAD( "u3.bin",       0x3400, 0x0400, 0x7aa7829b )
	ROM_LOAD( "u2.bin",       0x3800, 0x0400, 0xa9b10df5 )
	ROM_LOAD( "u1.bin",       0x3c00, 0x0400, 0x431a7449 )

	ROM_REGION(0x0020) /* Color PROMs */
	/* missing! */
ROM_END

ROM_START( spacetrk_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "u33.bin",      0x0000, 0x0400, 0x9033fe50 )
	ROM_LOAD( "u32.bin",      0x0400, 0x0400, 0x08f61f0d )
	ROM_LOAD( "u31.bin",      0x0800, 0x0400, 0x1088a8c4 )
	ROM_LOAD( "u30.bin",      0x0c00, 0x0400, 0x55560cc8 )
	ROM_LOAD( "u29.bin",      0x1000, 0x0400, 0x71713958 )
	ROM_LOAD( "u28.bin",      0x1400, 0x0400, 0x7bcf5ca3 )
	ROM_LOAD( "u27.bin",      0x1800, 0x0400, 0xad7a2065 )
	ROM_LOAD( "u26.bin",      0x1c00, 0x0400, 0x6060fe77 )
	ROM_LOAD( "u8.bin",       0x2000, 0x0400, 0x75a90624 )
	ROM_LOAD( "u7.bin",       0x2400, 0x0400, 0x7b31a2ab )
	ROM_LOAD( "u6.bin",       0x2800, 0x0400, 0x94135b33 )
	ROM_LOAD( "u5.bin",       0x2c00, 0x0400, 0xcfbf2538 )
	ROM_LOAD( "u4.bin",       0x3000, 0x0400, 0xb4b95129 )
	ROM_LOAD( "u3.bin",       0x3400, 0x0400, 0x03ca1d70 )
	ROM_LOAD( "u2.bin",       0x3800, 0x0400, 0xa968584b )
	ROM_LOAD( "u1.bin",       0x3c00, 0x0400, 0xe6e300e8 )

	ROM_REGION(0x0020) /* Color PROMs */
	ROM_LOAD( "u49.bin",      0x0000, 0x0020, 0xaabae4cd )
ROM_END

ROM_START( sptrekct_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "u33c.bin",     0x0000, 0x0400, 0xb056b928 )
	ROM_LOAD( "u32c.bin",     0x0400, 0x0400, 0xdffb11d9 )
	ROM_LOAD( "u31c.bin",     0x0800, 0x0400, 0x9b25d46f )
	ROM_LOAD( "u30c.bin",     0x0c00, 0x0400, 0x3a612bfe )
	ROM_LOAD( "u29c.bin",     0x1000, 0x0400, 0xd8bb6e0c )
	ROM_LOAD( "u28c.bin",     0x1400, 0x0400, 0x0e367740 )
	ROM_LOAD( "u27c.bin",     0x1800, 0x0400, 0xd59fec86 )
	ROM_LOAD( "u26c.bin",     0x1c00, 0x0400, 0x9deefa0f )
	ROM_LOAD( "u8c.bin",      0x2000, 0x0400, 0x613116c5 )
	ROM_LOAD( "u7c.bin",      0x2400, 0x0400, 0x3bdf2464 )
	ROM_LOAD( "u6c.bin",      0x2800, 0x0400, 0x039d73fa )
	ROM_LOAD( "u5c.bin",      0x2c00, 0x0400, 0x1638344f )
	ROM_LOAD( "u4c.bin",      0x3000, 0x0400, 0xe34443cd )
	ROM_LOAD( "u3c.bin",      0x3400, 0x0400, 0x6f16cbd7 )
	ROM_LOAD( "u2c.bin",      0x3800, 0x0400, 0x94da3cdc )
	ROM_LOAD( "u1c.bin",      0x3c00, 0x0400, 0x2a228bf4 )

	ROM_REGION(0x0020) /* Color PROMs */
	ROM_LOAD( "u49.bin",      0x0000, 0x0020, 0xaabae4cd )
ROM_END

ROM_START( carnival_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "651u33.cpu",   0x0000, 0x0400, 0x9f2736e6 )
	ROM_LOAD( "652u32.cpu",   0x0400, 0x0400, 0xa1f58beb )
	ROM_LOAD( "653u31.cpu",   0x0800, 0x0400, 0x67b17922 )
	ROM_LOAD( "654u30.cpu",   0x0c00, 0x0400, 0xbefb09a5 )
	ROM_LOAD( "655u29.cpu",   0x1000, 0x0400, 0x623fcdad )
	ROM_LOAD( "656u28.cpu",   0x1400, 0x0400, 0x53040332 )
	ROM_LOAD( "657u27.cpu",   0x1800, 0x0400, 0xf2537467 )
	ROM_LOAD( "658u26.cpu",   0x1c00, 0x0400, 0xfcc3854e )
	ROM_LOAD( "659u8.cpu",    0x2000, 0x0400, 0x28be8d69 )
	ROM_LOAD( "660u7.cpu",    0x2400, 0x0400, 0x3873ccdb )
	ROM_LOAD( "661u6.cpu",    0x2800, 0x0400, 0xd9a96dff )
	ROM_LOAD( "662u5.cpu",    0x2c00, 0x0400, 0xd893ca72 )
	ROM_LOAD( "663u4.cpu",    0x3000, 0x0400, 0xdf8c63c5 )
	ROM_LOAD( "664u3.cpu",    0x3400, 0x0400, 0x689a73e8 )
	ROM_LOAD( "665u2.cpu",    0x3800, 0x0400, 0x28e7b2b6 )
	ROM_LOAD( "666u1.cpu",    0x3c00, 0x0400, 0x4eec7fae )

	ROM_REGION(0x0020) /* Color PROMs */
	ROM_LOAD( "316-633",      0x0000, 0x0020, 0xf0084d80 )

	ROM_REGION(0x0800)	/* sound ROM */
	ROM_LOAD( "crvl.snd",     0x0000, 0x0400, 0x0dbaa2b0 )
ROM_END

ROM_START( pulsar_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "790.u33",      0x0000, 0x0400, 0x5e3816da )
	ROM_LOAD( "791.u32",      0x0400, 0x0400, 0xce0aee83 )
	ROM_LOAD( "792.u31",      0x0800, 0x0400, 0x72d78cf1 )
	ROM_LOAD( "793.u30",      0x0c00, 0x0400, 0x42155dd4 )
	ROM_LOAD( "794.u29",      0x1000, 0x0400, 0x11c7213a )
	ROM_LOAD( "795.u28",      0x1400, 0x0400, 0xd2f02e29 )
	ROM_LOAD( "796.u27",      0x1800, 0x0400, 0x67737a2e )
	ROM_LOAD( "797.u26",      0x1c00, 0x0400, 0xec250b24 )
	ROM_LOAD( "798.u8",       0x2000, 0x0400, 0x1d34912d )
	ROM_LOAD( "799.u7",       0x2400, 0x0400, 0xf5695e4c )
	ROM_LOAD( "800.u6",       0x2800, 0x0400, 0xbf91ad92 )
	ROM_LOAD( "801.u5",       0x2C00, 0x0400, 0x1e9721dc )
	ROM_LOAD( "802.u4",       0x3000, 0x0400, 0xd32d2192 )
	ROM_LOAD( "803.u3",       0x3400, 0x0400, 0x3ede44d5 )
	ROM_LOAD( "804.u2",       0x3800, 0x0400, 0x62847b01 )
	ROM_LOAD( "805.u1",       0x3c00, 0x0400, 0xab418e86 )

	ROM_REGION(0x0020) /* Color PROMs */
	ROM_LOAD( "316-0789.u49", 0x0000, 0x0020, 0x7fc1861f )
ROM_END

ROM_START( heiankyo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ha16.u33",     0x0000, 0x0400, 0x1eec8b36 )
	ROM_LOAD( "ha15.u32",     0x0400, 0x0400, 0xc1b9a1a5 )
	ROM_LOAD( "ha14.u31",     0x0800, 0x0400, 0x5b7b582e )
	ROM_LOAD( "ha13.u30",     0x0c00, 0x0400, 0x4aa67e01 )
	ROM_LOAD( "ha12.u29",     0x1000, 0x0400, 0x75889ca6 )
	ROM_LOAD( "ha11.u28",     0x1400, 0x0400, 0xd469226a )
	ROM_LOAD( "ha10.u27",     0x1800, 0x0400, 0x4e203074 )
	ROM_LOAD( "ha9.u26",      0x1c00, 0x0400, 0x9c3a3dd2 )
	ROM_LOAD( "ha8.u8",       0x2000, 0x0400, 0x6cc64878 )
	ROM_LOAD( "ha7.u7",       0x2400, 0x0400, 0x6d2f9527 )
	ROM_LOAD( "ha6.u6",       0x2800, 0x0400, 0xe467c353 )
	ROM_LOAD( "ha3.u3",       0x2c00, 0x0400, 0x6a55eda8 )
	/* 3000-37ff empty */
	ROM_LOAD( "ha2.u2",       0x3800, 0x0400, 0x056b3b8b )
	ROM_LOAD( "ha1.u1",       0x3c00, 0x0400, 0xb8da2b5e )

	ROM_REGION(0x0020) /* Color PROMs */
	ROM_LOAD( "316-138.u49",  0x0000, 0x0020, 0x67104ea9 )
ROM_END



static void vicdual_decode(void)
{
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* copy the ROMs to the mirror image */
	memcpy(&RAM[0x4000],&RAM[0x0000],0x4000);
}



static unsigned char bw_color_prom[] =
{
	/* for b/w games, let's use the Head On PROM */
	0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,
	0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,
};

static unsigned char headon_color_prom[] =
{
	0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,
	0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,0xE1,
};

static unsigned char sspaceat_color_prom[] =
{
	/* Guessed palette! */
	0x60,0x40,0x40,0x20,0xC0,0x80,0xA0,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static unsigned char invinco_color_prom[] =
{
	/* selected palette from the invho2 and invds PROMs */
	0xd1,0xb1,0x31,0x51,0xf1,0xb1,0x71,0x91,0xd1,0xb1,0x31,0x51,0xf1,0xb1,0x71,0x91,
	0xd1,0xb1,0x31,0x51,0xf1,0xb1,0x71,0x91,0xd1,0xb1,0x31,0x51,0xf1,0xb1,0x71,0x91
};

static unsigned char tranqgun_color_prom[] =
{
	/* PR-57: palette */
	/* was a bad read, this is just a guess! */
	0xc0,0xe0,0x60,0x60,0x80,0xa0,0x60,0x20,0xc0,0xe0,0x60,0x60,0x80,0xa0,0x60,0x20,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

#if 0
/* ROMs for this game are not available yet */
static unsigned char ho2ds_color_prom[] =
{
	/* 316-283: palette */
	0x31,0xB1,0x71,0x31,0x31,0x31,0x31,0x31,0x91,0xF1,0x31,0xF1,0x51,0xB1,0x91,0xB1,
	0xF5,0x79,0x31,0xB9,0xF5,0xF5,0xB5,0x95,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31
};
#endif



static const char *carnival_sample_names[] =
{
	"bear.sam",
	"bonus1.sam",
	"bonus2.sam",
	"clang.sam",
	"duck1.sam",
	"duck2.sam",
	"duck3.sam",
	"pipehit.sam",
	"ranking.sam",
	"rifle.sam",
	NULL
};



static int carnival_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xE397],"\x00\x00\x00",3) == 0 &&
			memcmp(&RAM[0xE5A2],"   ",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			/* Read the scores */
			osd_fread(f,&RAM[0xE397],2*30);
			/* Read the initials */
			osd_fread(f,&RAM[0xE5A2],9);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void carnival_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* Save the scores */
		osd_fwrite(f,&RAM[0xE397],2*30);
		/* Save the initials */
		osd_fwrite(f,&RAM[0xE5A2],9);
		osd_fclose(f);
	}

}



struct GameDriver depthch_driver =
{
	__FILE__,
	0,
	"depthch",
	"Depth Charge",
	"1977",
	"Gremlin",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_2Aports_machine_driver,
	0,

	depthch_rom,
	vicdual_decode, 0,
	0,
	0,	/* sound_prom */

	depthch_input_ports,

	bw_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver safari_driver =
{
	__FILE__,
	0,
	"safari",
	"Safari",
	"1977",
	"Gremlin",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_2Bports_machine_driver,
	0,

	safari_rom,
	vicdual_decode, 0,
	0,
	0,	/* sound_prom */

	safari_input_ports,

	bw_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver frogs_driver =
{
	__FILE__,
	0,
	"frogs",
	"Frogs",
	"1978",
	"Gremlin",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_2Aports_machine_driver,
	0,

	frogs_rom,
	vicdual_decode, 0,
	0,
	0,	/* sound_prom */

	frogs_input_ports,

	bw_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver sspaceat_driver =
{
	__FILE__,
	0,
	"sspaceat",
	"Sega Space Attack",
	"1979",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	GAME_IMPERFECT_COLORS,
	&vicdual_3ports_machine_driver,
	0,

	sspaceat_rom,
	vicdual_decode, 0,
	0,
	0,	/* sound_prom */

	sspaceat_input_ports,

	sspaceat_color_prom, 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver headon_driver =
{
	__FILE__,
	0,
	"headon",
	"Head On",
	"1979",
	"Gremlin",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_2Aports_machine_driver,
	0,

	headon_rom,
	vicdual_decode, 0,
	0,
	0,	/* sound_prom */

	headon_input_ports,

	headon_color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

/* No ROMs yet for the following one, but we have the color PROM. */
//GAMEDRIVER( ho2ds,    "Head On 2 / Deep Scan", vicdual_4ports_machine_driver,  0, ORIENTATION_ROTATE_270, 0, 0 )

struct GameDriver invho2_driver =
{
	__FILE__,
	0,
	"invho2",
	"Invinco / Head On 2",
	"1979",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_4ports_machine_driver,
	0,

	invho2_rom,
	vicdual_decode, 0,
	0,
	0,	/* sound_prom */

	invho2_input_ports,

	PROM_MEMORY_REGION(1), 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver samurai_driver =
{
	__FILE__,
	0,
	"samurai",
	"Samurai",
	"1980",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&samurai_machine_driver,
	0,

	samurai_rom,
	vicdual_decode, 0,
	0,
	0,	/* sound_prom */

	samurai_input_ports,

	PROM_MEMORY_REGION(1), 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver invinco_driver =
{
	__FILE__,
	0,
	"invinco",
	"Invinco",
	"1979",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	GAME_IMPERFECT_COLORS,
	&vicdual_3ports_machine_driver,
	0,

	invinco_rom,
	vicdual_decode, 0,
	0,
	0,	/* sound_prom */

	invinco_input_ports,

	invinco_color_prom, 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver invds_driver =
{
	__FILE__,
	0,
	"invds",
	"Invinco / Deep Scan",
	"1979",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_4ports_machine_driver,
	0,

	invds_rom,
	vicdual_decode, 0,
	0,
	0,	/* sound_prom */

	invds_input_ports,

	PROM_MEMORY_REGION(1), 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver tranqgun_driver =
{
	__FILE__,
	0,
	"tranqgun",
	"Tranquilizer Gun",
	"1980",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	GAME_IMPERFECT_COLORS,
	&vicdual_4ports_machine_driver,
	0,

	tranqgun_rom,
	vicdual_decode, 0,
	0,
	0,	/* sound_prom */

	tranqgun_input_ports,

	tranqgun_color_prom, 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver spacetrk_driver =
{
	__FILE__,
	0,
	"spacetrk",
	"Space Trek (Upright)",
	"1980",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_4ports_machine_driver,
	0,

	spacetrk_rom,
	vicdual_decode, 0,
	0,
	0,	/* sound_prom */

	spacetrk_input_ports,

	PROM_MEMORY_REGION(1), 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver sptrekct_driver =
{
	__FILE__,
	&spacetrk_driver,
	"sptrekct",
	"Space Trek (Cocktail)",
	"1980",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_4ports_machine_driver,
	0,

	sptrekct_rom,
	vicdual_decode, 0,
	0,
	0,	/* sound_prom */

	sptrekct_input_ports,

	PROM_MEMORY_REGION(1), 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver carnival_driver =
{
	__FILE__,
	0,
	"carnival",
	"Carnival",
	"1980",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari\nPeter Clare (sound)\nAlan J McCormick (sound)",
	0,
	&carnival_machine_driver,
	0,

	carnival_rom,
	vicdual_decode, 0,
	carnival_sample_names,
	0,	/* sound_prom */

	carnival_input_ports,

	PROM_MEMORY_REGION(1), 0, 0,
	ORIENTATION_ROTATE_270,

	carnival_hiload, carnival_hisave
};

struct GameDriver pulsar_driver =
{
	__FILE__,
	0,
	"pulsar",
	"Pulsar",
	"1981",
	"Sega",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&vicdual_4ports_machine_driver,
	0,

	pulsar_rom,
	vicdual_decode, 0,
	0,
	0,	/* sound_prom */

	pulsar_input_ports,

	PROM_MEMORY_REGION(1), 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver heiankyo_driver =
{
	__FILE__,
	0,
	"heiankyo",
	"Heiankyo Alien",
	"1979",
	"Denki Onkyo",
	"Mike Coates\nRichard Davies\nNicola Salmoria\nZsolt Vasvari",
	0,
	&heiankyo_machine_driver,
	0,

	heiankyo_rom,
	vicdual_decode, 0,
	0,
	0,	/* sound_prom */

	heiankyo_input_ports,

	PROM_MEMORY_REGION(1), 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};
