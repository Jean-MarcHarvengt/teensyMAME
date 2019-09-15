/***************************************************************************

Fast Freddie/Jump Coaster memory map (preliminary)

These games run on a modified Galaxian board.

Main CPU

Fast Fred  Jump Coas
0000-7fff             ROM
c000-c7ff             RAM
d000-d3ff  d800-dbff  Video RAM
d400-d7ff  dc00-dfff  Mirrored Video RAM
d800-d83f  d000-d03f  Attributes RAM
d840-d85f  d040-d05f  Sprite RAM

I/O read:

c800-cfff             Custom I/O (Protection)
e000       e802       Input Port 1
e800       e803       Input Port 2
f000       e800       Dip Switches
f800                  Watchdog Reset

I/O write:

c800-cfff             Custom I/O write
e000                  Background color write
fXX1                  NMI Enable
fXX2-fXX3             Palette Select (?)
fXX4-fXX5             Character Bank Select
fXX6                  Screen Flip X
fXX7                  Screen Flip Y

f800                  Sound Command Write (AY8910 #1 Control Port on Jump Coaster)
f801                  AY8910 #1 Write Port on Jump Coaster


Sound CPU (Only on Fast Freddie)

0000-1fff ROM
2000-23ff RAM

I/O read:

3000 Sound Command Read

I/O write:

3000 NMI Enable
4000 Reset PSG's (?)
5000 AY8910 #1 Control Port
5001 AY8910 #1 Write Port
6000 AY8910 #2 Control Port
6001 AY8910 #2 Write Port

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *galaxian_attributesram;
void galaxian_attributes_w(int offset,int data);

void fastfred_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void fastfred_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void fastfred_character_bank_select_w (int offset, int data);
void fastfred_color_bank_select_w (int offset, int data);
void fastfred_background_color_w (int offset, int data);
void fastfred_flipx_w(int offset,int data);
void fastfred_flipy_w(int offset,int data);
void jumpcoas_init_machine(void);

static int use_custom_io;

static int jumpcoas_custom_io_r(int offset)
{
	if (offset == 0x100)
	{
		return 0x63;
	}

	return 0x00;
}

// This routine is a big hack, but the only way I can get the game working
// without knowing anything about the way the protection chip works.
// These values were derived based on disassembly of the code. Usually, it
// was pretty obvious what the values should be. Of course, this will have
// to change if a different ROM set ever surfaces.
static int fastfred_custom_io_r(int offset)
{
	if (!use_custom_io) return 0x00; // Flyboy bootleg

    switch (cpu_getpc())
    {
    case 0x03c0:
        return 0x9d;

    case 0x03e6:
        return 0x9f;

    case 0x0407:
        return 0x00;

    case 0x0446:
        return 0x94;

    case 0x049f:
        return 0x01;
    case 0x04b1:
        return 0x00;

    case 0x0dd2:
        return 0x00;
    case 0x0de4:
        return 0x20;

    case 0x122b:
        return 0x10;
    case 0x123d:
        return 0x00;

    case 0x1a83:
        return 0x10;
    case 0x1a93:
        return 0x00;

    case 0x1b26:
        return 0x00;
    case 0x1b37:
        return 0x80;

    case 0x2491:
        return 0x10;
    case 0x24a2:
        return 0x00;

    case 0x46ce:
        return 0x20;
    case 0x46df:
        return 0x00;

    case 0x7b18:
        return 0x01;
    case 0x7b29:
        return 0x00;

    case 0x7b47:
        return 0x00;
    case 0x7b58:
        return 0x20;

    }

    if (errorlog) fprintf(errorlog, "Uncaught custom I/O read %04X at %04X\n", 0xc800+offset, cpu_getpc());
    return 0x00;
}

static void flyboy_init(void)
{
	use_custom_io = 0;
}

static void fastfred_init(void)
{
	use_custom_io = 1;
}

static struct MemoryReadAddress fastfred_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_NOP }, // There is a bug in Fast Freddie that causes
								 // these locations to be read. See 1b5a
								 // One of the instructions should be ld de,
								 // instead of ld hl,
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xcfff, fastfred_custom_io_r },  // Flyboy is hacked not to use this
	{ 0xd000, 0xd3ff, MRA_RAM },
	{ 0xd800, 0xd8ff, MRA_RAM },
	{ 0xe000, 0xe000, input_port_0_r },
	{ 0xe800, 0xe800, input_port_1_r },
	{ 0xf000, 0xf000, input_port_2_r },
	{ 0xf800, 0xf800, watchdog_reset_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress fastfred_writemem[] =
{
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcfff, MWA_NOP },
	{ 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
	{ 0xd400, 0xd7ff, videoram_w },  // Mirrored for above
	{ 0xd800, 0xd83f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0xd840, 0xd85f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd860, 0xdbff, MWA_RAM }, // Unused, but initialized
	{ 0xe000, 0xe000, fastfred_background_color_w },
	{ 0xf000, 0xf000, MWA_NOP }, // Unused, but initialized
	{ 0xf001, 0xf001, interrupt_enable_w },
	{ 0xf002, 0xf003, fastfred_color_bank_select_w },
	{ 0xf004, 0xf005, fastfred_character_bank_select_w },
	{ 0xf006, 0xf006, fastfred_flipx_w },
	{ 0xf007, 0xf007, fastfred_flipy_w },
	{ 0xf116, 0xf116, fastfred_flipx_w },
	{ 0xf117, 0xf117, fastfred_flipy_w },
	{ 0xf800, 0xf800, soundlatch_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress jumpcoas_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xcfff, jumpcoas_custom_io_r },
	{ 0xd000, 0xd3ff, MRA_RAM },
	{ 0xd800, 0xdbff, MRA_RAM },
	{ 0xe800, 0xe800, input_port_0_r },
	{ 0xe802, 0xe802, input_port_1_r },
	{ 0xe802, 0xe803, input_port_2_r },
	//{ 0xf800, 0xf800, watchdog_reset_r },  // Why doesn't this work???
	{ 0xf800, 0xf800, MRA_NOP },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress jumpcoas_writemem[] =
{
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcfff, MWA_NOP },
	{ 0xd000, 0xd03f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0xd040, 0xd05f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd060, 0xd3ff, MWA_NOP },
	{ 0xd800, 0xdbff, videoram_w, &videoram, &videoram_size },
	{ 0xdc00, 0xdfff, videoram_w },	/* mirror address, used in the name entry screen */
	{ 0xe000, 0xe000, fastfred_background_color_w },
	{ 0xf000, 0xf000, MWA_NOP }, // Unused, but initialized
	{ 0xf001, 0xf001, interrupt_enable_w },
	{ 0xf002, 0xf003, fastfred_color_bank_select_w },
	{ 0xf004, 0xf005, fastfred_character_bank_select_w },
	{ 0xf006, 0xf006, fastfred_flipx_w },
	{ 0xf007, 0xf007, fastfred_flipy_w },
	{ 0xf116, 0xf116, fastfred_flipx_w },
	{ 0xf117, 0xf117, fastfred_flipy_w },
	{ 0xf800, 0xf800, AY8910_control_port_0_w },
	{ 0xf801, 0xf801, AY8910_write_port_0_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x3000, 0x3000, soundlatch_r },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x3000, 0x3000, interrupt_enable_w },
	{ 0x4000, 0x4000, MWA_RAM },  // Reset PSG's
	{ 0x5000, 0x5000, AY8910_control_port_0_w },
	{ 0x5001, 0x5001, AY8910_write_port_0_w },
	{ 0x6000, 0x6000, AY8910_control_port_1_w },
	{ 0x6001, 0x6001, AY8910_write_port_1_w },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( fastfred_input_ports )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BITX(    0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_8WAY | IPF_COCKTAIL )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )

	PORT_START      /* DSW 1 */
	PORT_DIPNAME( 0x0f, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "A 2/1 B 2/1" )
	PORT_DIPSETTING(    0x02, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/1" )
	PORT_DIPSETTING(    0x03, "A 1/1 B 1/2" )
	PORT_DIPSETTING(    0x04, "A 1/1 B 1/3" )
	PORT_DIPSETTING(    0x05, "A 1/1 B 1/4" )
	PORT_DIPSETTING(    0x06, "A 1/1 B 1/5" )
	PORT_DIPSETTING(    0x07, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x08, "A 1/2 B 1/2" )
	PORT_DIPSETTING(    0x09, "A 1/2 B 1/4" )
	PORT_DIPSETTING(    0x0a, "A 1/2 B 1/5" )
	PORT_DIPSETTING(    0x0e, "A 1/2 B 1/6" )
	PORT_DIPSETTING(    0x0b, "A 1/2 B 1/10" )
	PORT_DIPSETTING(    0x0c, "A 1/2 B 1/11" )
	PORT_DIPSETTING(    0x0d, "A 1/2 B 1/12" )
	PORT_DIPSETTING(    0x0f, "Free Play" )
	PORT_DIPNAME( 0x10, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x60, 0x20, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "30000" )
	PORT_DIPSETTING(    0x40, "50000" )
	PORT_DIPSETTING(    0x60, "100000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )
INPUT_PORTS_END

INPUT_PORTS_START( flyboy_input_ports )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_8WAY | IPF_COCKTAIL )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )

	PORT_START      /* DSW 1 */
	PORT_DIPNAME( 0x03, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "6 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "6 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x30, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x20, "7" )
	PORT_BITX( 0,       0x30, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x40, 0x00, "Invincibility", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_BITX( 0,       0x40, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "On", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )
INPUT_PORTS_END

INPUT_PORTS_START( jumpcoas_input_ports )
	PORT_START      /* DSW 0 */
	PORT_DIPNAME( 0x03, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "6 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "6 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x30, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x20, "7" )
	PORT_BITX( 0,       0x30, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "255", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
INPUT_PORTS_END


static struct GfxLayout fastfred_charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	3,      /* 3 bits per pixel */
	{ 0x4000*8, 0x2000*8, 0 }, /* the three bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout jumpcoas_charlayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	3,      /* 3 bits per pixel */
	{ 0x2000*8, 0x1000*8, 0 }, /* the three bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout fastfred_spritelayout =
{
	16,16,  /* 16*16 sprites */
	128,    /* 128 sprites */
	3,      /* 3 bits per pixel */
	{ 0x2000*8, 0x1000*8, 0 }, /* the three bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8     /* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout jumpcoas_spritelayout =
{
	16,16,  /* 16*16 sprites */
	64,     /* 64 sprites */
	3,      /* 3 bits per pixel */
	{ 0x2000*8, 0x1000*8, 0 }, /* the three bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8     /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo fastfred_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &fastfred_charlayout,     0, 32 },
	{ 1, 0x6000, &fastfred_spritelayout,   0, 32 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo jumpcoas_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &jumpcoas_charlayout,     0, 32 },
	{ 1, 0x0800, &jumpcoas_spritelayout,   0, 32 },
	{ -1 } /* end of array */
};


#define CLOCK 18432000  /* The crystal is 18.432Mhz */

static struct AY8910interface fastfred_ay8910_interface =
{
	2,             /* 2 chips */
	CLOCK/6,       /* 3.072 Mhz */
	{ 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct AY8910interface jumpcoas_ay8910_interface =
{
	1,             /* 1 chip */
	CLOCK/6,       /* 3.072 Mhz */
	{ 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static struct MachineDriver fastfred_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			CLOCK/6,     /* 3.072 Mhz */
			0,
			fastfred_readmem,fastfred_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			CLOCK/12,    /* 1.536 Mhz */
			3,
			sound_readmem,sound_writemem,0,0,
			nmi_interrupt,4
		}
	},
	60, CLOCK/16/60,       /* frames per second, vblank duration */
	1,      /* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	fastfred_gfxdecodeinfo,
	256,32*8,
	fastfred_vh_convert_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	fastfred_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&fastfred_ay8910_interface
		}
	}
};

static struct MachineDriver jumpcoas_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			CLOCK/6,     /* 3.072 Mhz */
			0,
			jumpcoas_readmem,jumpcoas_writemem,0,0,
			nmi_interrupt,1
		}
	},
	60, CLOCK/16/60,       /* frames per second, vblank duration */
	1,      /* Single CPU game */
	jumpcoas_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	jumpcoas_gfxdecodeinfo,
	256,32*8,
	fastfred_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	fastfred_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&jumpcoas_ay8910_interface
		}
	}
};

#undef CLOCK

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( fastfred_rom )
	ROM_REGION(0x10000)     /* 64k for main CPU */
	ROM_LOAD( "ffr.01",       0x0000, 0x1000, 0x15032c13 )
	ROM_LOAD( "ffr.02",       0x1000, 0x1000, 0xf9642744 )
	ROM_LOAD( "ffr.03",       0x2000, 0x1000, 0xf0919727 )
	ROM_LOAD( "ffr.04",       0x3000, 0x1000, 0xc778751e )
	ROM_LOAD( "ffr.05",       0x4000, 0x1000, 0xcd6e160a )
	ROM_LOAD( "ffr.06",       0x5000, 0x1000, 0x67f7f9b3 )
	ROM_LOAD( "ffr.07",       0x6000, 0x1000, 0x2935c76a )
	ROM_LOAD( "ffr.08",       0x7000, 0x1000, 0x0fb79e7b )

	ROM_REGION_DISPOSE(0x9000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ffr.14",       0x0000, 0x1000, 0xe8a00e81 )
	ROM_LOAD( "ffr.17",       0x1000, 0x1000, 0x701e0f01 )
	ROM_LOAD( "ffr.15",       0x2000, 0x1000, 0xb49b053f )
	ROM_LOAD( "ffr.18",       0x3000, 0x1000, 0x4b208c8b )
	ROM_LOAD( "ffr.16",       0x4000, 0x1000, 0x8c686bc2 )
	ROM_LOAD( "ffr.19",       0x5000, 0x1000, 0x75b613f6 )
	ROM_LOAD( "ffr.11",       0x6000, 0x1000, 0x0e1316d4 )
	ROM_LOAD( "ffr.12",       0x7000, 0x1000, 0x94c06686 )
	ROM_LOAD( "ffr.13",       0x8000, 0x1000, 0x3fcfaa8e )

	ROM_REGION(0x300)       /* color proms */
	ROM_LOAD( "flyboy.red",   0x0000, 0x100, 0xcbff2caa )
	ROM_LOAD( "flyboy.grn",   0x0100, 0x100, 0x7da063d0 )
	ROM_LOAD( "flyboy.blu",   0x0200, 0x100, 0x448f9399 )

	ROM_REGION(0x10000)     /* 64k for audio CPU */
	ROM_LOAD( "ffr.09",       0x0000, 0x1000, 0xa1ec8d7e )
	ROM_LOAD( "ffr.10",       0x1000, 0x1000, 0x460ca837 )
ROM_END

ROM_START( flyboy_rom )
	ROM_REGION(0x10000)     /* 64k for main CPU */
	ROM_LOAD( "rom1.cpu",     0x0000, 0x1000, 0xe9e1f527 )
	ROM_LOAD( "rom2.cpu",     0x1000, 0x1000, 0x07fbe78c )
	ROM_LOAD( "rom3.cpu",     0x2000, 0x1000, 0xd2f8f085 )
	ROM_LOAD( "rom4.cpu",     0x3000, 0x1000, 0x19e5e15c )
	ROM_LOAD( "rom5.cpu",     0x4000, 0x1000, 0xd56872ea )
	ROM_LOAD( "rom6.cpu",     0x5000, 0x1000, 0xf5464c72 )
	ROM_LOAD( "rom7.cpu",     0x6000, 0x1000, 0x50a1baff )
	ROM_LOAD( "rom8.cpu",     0x7000, 0x1000, 0xfe2ae95d )

	ROM_REGION_DISPOSE(0x9000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "rom14.rom",    0x0000, 0x1000, 0xaeb07260 )
	ROM_LOAD( "rom17.rom",    0x1000, 0x1000, 0xa834325b )
	ROM_LOAD( "rom15.rom",    0x2000, 0x1000, 0xc10c7ce2 )
	ROM_LOAD( "rom18.rom",    0x3000, 0x1000, 0x2f196c80 )
	ROM_LOAD( "rom16.rom",    0x4000, 0x1000, 0x719246b1 )
	ROM_LOAD( "rom19.rom",    0x5000, 0x1000, 0x00c1c5d2 )
	ROM_LOAD( "rom11.rom",    0x6000, 0x1000, 0xee7ec342 )
	ROM_LOAD( "rom12.rom",    0x7000, 0x1000, 0x84d03124 )
	ROM_LOAD( "rom13.rom",    0x8000, 0x1000, 0xfcb33ff4 )

	ROM_REGION(0x300)       /* color proms */
	ROM_LOAD( "flyboy.red",   0x0000, 0x100, 0xcbff2caa )
	ROM_LOAD( "flyboy.grn",   0x0100, 0x100, 0x7da063d0 )
	ROM_LOAD( "flyboy.blu",   0x0200, 0x100, 0x448f9399 )

	ROM_REGION(0x10000)     /* 64k for audio CPU */
	ROM_LOAD( "rom9.cpu",     0x0000, 0x1000, 0x5d05d1a0 )
	ROM_LOAD( "rom10.cpu",    0x1000, 0x1000, 0x7a28005b )
ROM_END

ROM_START( jumpcoas_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "jumpcoas.001", 0x0000, 0x2000, 0x0778c953 )
	ROM_LOAD( "jumpcoas.002", 0x2000, 0x2000, 0x57f59ce1 )
	ROM_LOAD( "jumpcoas.003", 0x4000, 0x2000, 0xd9fc93be )
	ROM_LOAD( "jumpcoas.004", 0x6000, 0x2000, 0xdc108fc1 )

	ROM_REGION_DISPOSE(0x3000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "jumpcoas.005", 0x0000, 0x1000, 0x2dce6b07 )
	ROM_LOAD( "jumpcoas.006", 0x1000, 0x1000, 0x0d24aa1b )
	ROM_LOAD( "jumpcoas.007", 0x2000, 0x1000, 0x14c21e67 )

	ROM_REGION(0x300)       /* color proms */
	ROM_LOAD( "jumpcoas.red", 0x0000, 0x100, 0x13714880 )
	ROM_LOAD( "jumpcoas.gre", 0x0100, 0x100, 0x05354848 )
	ROM_LOAD( "jumpcoas.blu", 0x0200, 0x100, 0xf4662db7 )

ROM_END

static int fastfred_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xc41c],"\x10",1) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xc400],0x45);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void fastfred_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xc400],0x45);
		osd_fclose(f);
	}
}

static int flyboy_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xc41c],"\x50",1) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xc400],0x94);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void flyboy_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xc400],0x94);
		osd_fclose(f);
	}
}

static int jumpcoas_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* Note: the game doesn't seem to initialize the high score display
	   at the top of the screen. Even without high score loading, the
	   screen says that the high score is 0, even though the high score
	   table shows all 5000's */

	/* check if the hi score table has already been initialized */
	if (RAM[0xc421] == 0x11)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xc400],0x45);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void jumpcoas_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xc400],0x45);
		osd_fclose(f);
	}
}


struct GameDriver flyboy_driver =
{
	__FILE__,
	0,
	"flyboy",
	"Fly-Boy (bootleg?)",
	"1982",
	"Kaneko",
	"Zsolt Vasvari\nBrad Oliver (additional code)\nMarco Cassili (additional code)",
	0,
	&fastfred_machine_driver,
	flyboy_init,

	flyboy_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	flyboy_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	flyboy_hiload, flyboy_hisave
};

struct GameDriver fastfred_driver =
{
	__FILE__,
	&flyboy_driver,
	"fastfred",
	"Fast Freddie",
	"1982",
	"Atari",
	"Zsolt Vasvari",
	0,
	&fastfred_machine_driver,
	fastfred_init,

	fastfred_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	fastfred_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	fastfred_hiload, fastfred_hisave
};

struct GameDriver jumpcoas_driver =
{
	__FILE__,
	0,
	"jumpcoas",
	"Jump Coaster",
	"1983",
	"Kaneko",
	"Zsolt Vasvari",
	0,
	&jumpcoas_machine_driver,
	0,

	jumpcoas_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	jumpcoas_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	jumpcoas_hiload, jumpcoas_hisave
};

