/***************************************************************************

Loco-Motion memory map (preliminary)

CPU #1
0000-4fff ROM (empty space at 5000-5fff for diagnostic ROM)
8040-83ff video RAM (top 8 rows of screen)
8440-87ff video RAM
8840-8bff color RAM (top 8 rows of screen)
8c40-8fff color RAM
8000-803f sprite RAM #1
8800-883f sprite RAM #2
9800-9fff RAM

read:
a000      IN0
a080      IN1
a100      IN2
a180      DSW

write:
a080      watchdog reset
a100      command for the audio CPU
a180      interrupt trigger on audio CPU
a181      interrupt enable


CPU #2:
2000-23ff RAM

read:
4000      8910 #0 read
6000      8910 #1 read

write:
5000      8910 #0 control
4000      8910 #0 write
7000      8910 #1 control
6000      8910 #1 write

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *rallyx_videoram2,*rallyx_colorram2;
extern unsigned char *rallyx_radarcarx,*rallyx_radarcary,*rallyx_radarcarcolor;
extern int rallyx_radarram_size;
extern unsigned char *rallyx_scrollx,*rallyx_scrolly;
void rallyx_videoram2_w(int offset,int data);
void rallyx_colorram2_w(int offset,int data);
void rallyx_flipscreen_w(int offset,int data);
void rallyx_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int rallyx_vh_start(void);
void rallyx_vh_stop(void);
void locomotn_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void jungler_init(void);
void rallyx_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void commsega_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



/* I am not 100% sure that this timer is correct, but */
/* I'm using the Gyruss wired to the higher 4 bits    */
/* instead of the lower ones, so there is a good      */
/* chance it's the right one. */

/* The timer clock which feeds the lower 4 bits of    */
/* AY-3-8910 port A is based on the same clock        */
/* feeding the sound CPU Z80.  It is a divide by      */
/* 10240, formed by a standard divide by 1024,        */
/* followed by a divide by 10 using a 4 bit           */
/* bi-quinary count sequence. (See LS90 data sheet    */
/* for an example).                                   */
/* Bits 1-3 come directly from the upper three bits   */
/* of the bi-quinary counter. Bit 0 comes from the    */
/* output of the divide by 1024.                      */

static int locomotn_timer[20] = {
0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03, 0x04, 0x05,
0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b, 0x0c, 0x0d
};

static int locomotn_portB_r(int offset)
{
	/* need to protect from totalcycles overflow */
	static int last_totalcycles = 0;

	/* number of Z80 clock cycles to count */
	static int clock;

	int current_totalcycles;

	current_totalcycles = cpu_gettotalcycles();
	clock = (clock + (current_totalcycles-last_totalcycles)) % 10240;

	last_totalcycles = current_totalcycles;

	return locomotn_timer[clock/512] << 4;
}

void locomotn_sh_irqtrigger_w(int offset,int data)
{
	static int last;


	if (last == 0 && data == 1)
	{
		/* setting bit 0 low then high triggers IRQ on the sound CPU */
		cpu_cause_interrupt(1,0xff);
	}

	last = data;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9800, 0x9fff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa080, 0xa080, input_port_1_r },	/* IN1 */
	{ 0xa100, 0xa100, input_port_2_r },	/* IN2 */
	{ 0xa180, 0xa180, input_port_3_r },	/* DSW */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress jungler_writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, rallyx_videoram2_w, &rallyx_videoram2 },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, rallyx_colorram2_w, &rallyx_colorram2 },
	{ 0x9800, 0x9fff, MWA_RAM },
	{ 0xa034, 0xa03f, MWA_RAM, &rallyx_radarcarcolor },
	{ 0xa080, 0xa080, watchdog_reset_w },
	{ 0xa100, 0xa100, soundlatch_w },
	{ 0xa130, 0xa130, MWA_RAM, &rallyx_scrollx },
	{ 0xa140, 0xa140, MWA_RAM, &rallyx_scrolly },
	{ 0xa170, 0xa170, MWA_NOP },	/* ????? */
	{ 0xa180, 0xa180, locomotn_sh_irqtrigger_w },
	{ 0xa181, 0xa181, interrupt_enable_w },
	{ 0xa183, 0xa183, rallyx_flipscreen_w },
	{ 0xa184, 0xa185, osd_led_w },
	{ 0xa186, 0xa186, MWA_NOP },
	{ 0x8014, 0x801f, MWA_RAM, &spriteram, &spriteram_size },	/* these are here just to initialize */
	{ 0x8814, 0x881f, MWA_RAM, &spriteram_2 },	/* the pointers. */
	{ 0x8034, 0x803f, MWA_RAM, &rallyx_radarcarx, &rallyx_radarram_size },	/* ditto */
	{ 0x8834, 0x883f, MWA_RAM, &rallyx_radarcary },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, rallyx_videoram2_w, &rallyx_videoram2 },
	{ 0x8800, 0x8bff, colorram_w, &colorram },
	{ 0x8c00, 0x8fff, rallyx_colorram2_w, &rallyx_colorram2 },
	{ 0x9800, 0x9fff, MWA_RAM },
	{ 0xa000, 0xa00f, MWA_RAM, &rallyx_radarcarcolor },
	{ 0xa080, 0xa080, watchdog_reset_w },
	{ 0xa100, 0xa100, soundlatch_w },
	{ 0xa130, 0xa130, MWA_RAM, &rallyx_scrollx },
	{ 0xa140, 0xa140, MWA_RAM, &rallyx_scrolly },
	{ 0xa170, 0xa170, MWA_NOP },	/* ????? */
	{ 0xa180, 0xa180, locomotn_sh_irqtrigger_w },
	{ 0xa181, 0xa181, interrupt_enable_w },
	{ 0xa183, 0xa183, rallyx_flipscreen_w },
	{ 0xa184, 0xa185, osd_led_w },
	{ 0xa186, 0xa186, MWA_NOP },
	{ 0x8000, 0x801f, MWA_RAM, &spriteram, &spriteram_size },	/* these are here just to initialize */
	{ 0x8800, 0x881f, MWA_RAM, &spriteram_2 },	/* the pointers. */
	{ 0x8020, 0x803f, MWA_RAM, &rallyx_radarcarx, &rallyx_radarram_size },	/* ditto */
	{ 0x8820, 0x883f, MWA_RAM, &rallyx_radarcary },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x4000, 0x4000, AY8910_read_port_0_r },
	{ 0x6000, 0x6000, AY8910_read_port_1_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x4000, 0x4000, AY8910_write_port_0_w },
	{ 0x5000, 0x5000, AY8910_control_port_0_w },
	{ 0x6000, 0x6000, AY8910_write_port_1_w },
	{ 0x7000, 0x7000, AY8910_control_port_1_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( locomotn_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x08, "Cocktail" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "255?" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x08, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x0c, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x0e, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x80, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0xc0, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0xe0, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x70, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x50, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Disable" )
INPUT_PORTS_END

INPUT_PORTS_START( jungler_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* DSW0 */
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x18, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( commsega_input_ports )
/* the direction controls are probably wrong */
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x07, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x06, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x1c, 0x1c, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x14, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x18, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x1c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x08, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters (256 in Jungler) */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 8*8+0,8*8+1,8*8+2,8*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites (64 in Jungler) */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout radardotlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	2,2,	/* 2*2 square */
	1,	/* just one */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0x15f*8, 0x15f*8 },	/* I "know" that this bit of the */
	{ 4, 4 },	/* graphics ROMs is 1 */
	0	/* no use */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,        0, 64 },
	{ 1, 0x0000, &spritelayout,      0, 64 },
	{ 1, 0x0000, &radardotlayout, 64*4,  4 },
	{ -1 } /* end of array */
};



static unsigned char wrong_color_prom[] =
{
	/* 8b.cpu - palette */
	0x00,0x07,0xA0,0x97,0x67,0x3F,0x7D,0x38,0xF0,0xA8,0xC0,0x18,0x5E,0xA8,0x1B,0xFF,
	0x00,0xFF,0xB7,0x00,0x00,0xFF,0xB7,0x00,0x00,0xFF,0xB7,0x00,0x00,0xFF,0xB7,0x00,
	/* 9d.cpu - lookup table */
	0x00,0x09,0x05,0x0C,0x00,0x01,0x08,0x06,0x00,0x06,0x0F,0x01,0x00,0x01,0x02,0x03,
	0x00,0x04,0x05,0x07,0x00,0x07,0x08,0x09,0x00,0x0A,0x0B,0x0C,0x00,0x08,0x05,0x0F,
	0x00,0x08,0x0F,0x01,0x00,0x05,0x04,0x0E,0x00,0x01,0x07,0x08,0x00,0x0B,0x0C,0x0D,
	0x00,0x01,0x01,0x01,0x00,0x02,0x02,0x02,0x00,0x03,0x03,0x03,0x00,0x04,0x04,0x04,
	0x00,0x05,0x05,0x05,0x00,0x06,0x06,0x06,0x00,0x07,0x07,0x07,0x00,0x08,0x08,0x08,
	0x00,0x09,0x09,0x09,0x00,0x0A,0x0A,0x0A,0x00,0x0B,0x0B,0x0B,0x00,0x0C,0x0C,0x0C,
	0x00,0x0D,0x0D,0x0D,0x00,0x0E,0x0E,0x0E,0x00,0x0F,0x0F,0x0F,0x00,0x00,0x00,0x00,
	0x00,0x02,0x05,0x0C,0x00,0x09,0x05,0x0C,0x00,0x0B,0x05,0x0C,0x00,0x0E,0x05,0x0C,
	0x00,0x09,0x05,0x0C,0x00,0x01,0x08,0x06,0x00,0x06,0x0F,0x01,0x00,0x01,0x02,0x03,
	0x00,0x04,0x05,0x07,0x00,0x07,0x08,0x09,0x00,0x0A,0x0B,0x0C,0x00,0x08,0x05,0x0F,
	0x00,0x08,0x00,0x01,0x00,0x05,0x04,0x0E,0x00,0x01,0x07,0x08,0x00,0x0B,0x0C,0x0D,
	0x00,0x01,0x01,0x01,0x00,0x02,0x02,0x02,0x00,0x03,0x03,0x03,0x00,0x04,0x04,0x04,
	0x00,0x05,0x05,0x05,0x00,0x06,0x06,0x06,0x00,0x07,0x07,0x07,0x00,0x08,0x08,0x08,
	0x00,0x09,0x09,0x09,0x00,0x0A,0x0A,0x0A,0x00,0x0B,0x0B,0x0B,0x00,0x0C,0x0C,0x0C,
	0x00,0x0D,0x0D,0x0D,0x00,0x0E,0x0E,0x0E,0x00,0x0F,0x0F,0x0F,0x00,0x03,0x06,0x0D,
	0x00,0x04,0x05,0x0C,0x00,0x03,0x05,0x0C,0x00,0x0B,0x08,0x09,0x00,0x09,0x04,0x0D,
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1789750,	/* 1.78975 MHz ? (same as other Konami games) */
	{ 0x20ff, 0x20ff },
	{ soundlatch_r },
	{ locomotn_portB_r },
	{ 0 },
	{ 0 }
};



#define MACHINE_DRIVER(GAMENAME)   \
																	\
static struct MachineDriver GAMENAME##_machine_driver =             \
{                                                                   \
	/* basic machine hardware */                                    \
	{                                                               \
		{                                                           \
			CPU_Z80,                                                \
			3072000,	/* 3.072 Mhz ? */                           \
			0,                                                      \
			readmem,GAMENAME##_writemem,0,0,                        \
			nmi_interrupt,1                                         \
		},                                                          \
		{                                                           \
			CPU_Z80 | CPU_AUDIO_CPU,                                \
			14318180/4,	/* ???? same as other Konami games */       \
			3,	/* memory region #3 */                              \
			sound_readmem,sound_writemem,0,0,                       \
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */ \
		}                                                           \
	},                                                              \
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */ \
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */ \
	0,                                                              \
                                                                    \
	/* video hardware */                                            \
	36*8, 28*8, { 0*8, 36*8-1, 0*8, 28*8-1 },                       \
	gfxdecodeinfo,                                                  \
	32,64*4+4*2,                                                    \
	rallyx_vh_convert_color_prom,                                   \
                                                                    \
	VIDEO_TYPE_RASTER,                                              \
	0,                                                              \
	rallyx_vh_start,                                                \
	rallyx_vh_stop,                                                 \
	GAMENAME##_vh_screenrefresh,                                    \
                                                                    \
	/* sound hardware */                                            \
	0,0,0,0,                                                        \
	{                                                               \
		{                                                           \
			SOUND_AY8910,                                           \
			&ay8910_interface                                       \
		}                                                           \
	}                                                               \
};

#define locomotn_writemem writemem
#define commsega_writemem writemem
#define jungler_vh_screenrefresh rallyx_vh_screenrefresh
MACHINE_DRIVER(locomotn)
MACHINE_DRIVER(jungler)
MACHINE_DRIVER(commsega)


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( jungler_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "jungr1",       0x0000, 0x1000, 0x5bd6ad15 )
	ROM_LOAD( "jungr2",       0x1000, 0x1000, 0xdc99f1e3 )
	ROM_LOAD( "jungr3",       0x2000, 0x1000, 0x3dcc03da )
	ROM_LOAD( "jungr4",       0x3000, 0x1000, 0xf92e9940 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5k",           0x0000, 0x0800, 0x924262bf )
	ROM_LOAD( "5m",           0x0800, 0x0800, 0x131a08ac )
	/* 1000-1fff empty for my convenience */

	ROM_REGION(0x0260)	/* color PROMs */
	ROM_LOAD( "18s030.8b",    0x0000, 0x0020, 0x55a7e6d1 ) /* palette */
	ROM_LOAD( "tbp24s10.9d",  0x0020, 0x0100, 0xd223f7b8 ) /* loookup table */
	ROM_LOAD( "18s030.7a",    0x0120, 0x0020, 0x8f574815 ) /* ?? */
	ROM_LOAD( "6331-1.10a",   0x0140, 0x0020, 0xb8861096 ) /* ?? */
	ROM_LOAD( "82s129.10g",   0x0160, 0x0100, 0x2ef89356 ) /* ?? */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "1b",           0x0000, 0x1000, 0xf86999c3 )
ROM_END

ROM_START( locomotn_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1a.cpu",       0x0000, 0x1000, 0xb43e689a )
	ROM_LOAD( "2a.cpu",       0x1000, 0x1000, 0x529c823d )
	ROM_LOAD( "3.cpu",        0x2000, 0x1000, 0xc9dbfbd1 )
	ROM_LOAD( "4.cpu",        0x3000, 0x1000, 0xcaf6431c )
	ROM_LOAD( "5.cpu",        0x4000, 0x1000, 0x64cf8dd6 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c1.cpu",       0x0000, 0x1000, 0x5732eda9 )
	ROM_LOAD( "c2.cpu",       0x1000, 0x1000, 0xc3035300 )

	ROM_REGION(0x0120)	/* color proms */
	ROM_LOAD( "8b.cpu",       0x0000, 0x0020, 0x75b05da0 ) /* palette */
	ROM_LOAD( "9d.cpu",       0x0020, 0x0100, 0xaa6cf063 ) /* loookup table */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "s1.snd",       0x0000, 0x1000, 0xa1105714 )
ROM_END

ROM_START( junglers_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "5c",           0x0000, 0x1000, 0xedd71b28 )
	ROM_LOAD( "5a",           0x1000, 0x1000, 0x61ea4d46 )
	ROM_LOAD( "4d",           0x2000, 0x1000, 0x557c7925 )
	ROM_LOAD( "4c",           0x3000, 0x1000, 0x51aac9a5 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5k",           0x0000, 0x0800, 0x924262bf )
	ROM_LOAD( "5m",           0x0800, 0x0800, 0x131a08ac )
	/* 1000-1fff empty for my convenience */

	ROM_REGION(0x0260)	/* color PROMs */
	ROM_LOAD( "18s030.8b",    0x0000, 0x0020, 0x55a7e6d1 ) /* palette */
	ROM_LOAD( "tbp24s10.9d",  0x0020, 0x0100, 0xd223f7b8 ) /* loookup table */
	ROM_LOAD( "18s030.7a",    0x0120, 0x0020, 0x8f574815 ) /* ?? */
	ROM_LOAD( "6331-1.10a",   0x0140, 0x0020, 0xb8861096 ) /* ?? */
	ROM_LOAD( "82s129.10g",   0x0160, 0x0100, 0x2ef89356 ) /* ?? */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "1b",           0x0000, 0x1000, 0xf86999c3 )
ROM_END

ROM_START( commsega_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "csega1",       0x0000, 0x1000, 0x92de3405 )
	ROM_LOAD( "csega2",       0x1000, 0x1000, 0xf14e2f9a )
	ROM_LOAD( "csega3",       0x2000, 0x1000, 0x941dbf48 )
	ROM_LOAD( "csega4",       0x3000, 0x1000, 0xe0ac69b4 )
	ROM_LOAD( "csega5",       0x4000, 0x1000, 0xbc56ebd0 )

	ROM_REGION_DISPOSE(0x2000) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "csega7",       0x0000, 0x1000, 0xe8e374f9 )
	ROM_LOAD( "csega6",       0x1000, 0x1000, 0xcf07fd5e )

	ROM_REGION(0x0120)	/* color proms */

	ROM_REGION(0x10000) /* 64k for the audio CPU */
	ROM_LOAD( "csega8",       0x0000, 0x1000, 0x588b4210 )
ROM_END



static int locomotn_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x9f00],"\x00\x00\x01",3) == 0 &&
	    memcmp(&RAM[0x99c6],"\x00\x00\x01",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x9f00],12*10);
			RAM[0x99c6] = RAM[0x9f00];
			RAM[0x99c7] = RAM[0x9f01];
			RAM[0x99c8] = RAM[0x9f02];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void locomotn_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x9f00],12*10);
		osd_fclose(f);
	}
}

static int jungler_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x991c],"\x00\x00\x02",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x9940],16*10);
			RAM[0x991c] = RAM[0x9940];
			RAM[0x991d] = RAM[0x9941];
			RAM[0x991e] = RAM[0x9942];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void jungler_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x9940],16*10);
		osd_fclose(f);
	}
}

static int commsega_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x9c60],"\x1d\x1c",2) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x9c6d],6);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void commsega_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x9c6d],6);
		osd_fclose(f);
	}
}



struct GameDriver jungler_driver =
{
	__FILE__,
	0,
	"jungler",
	"Jungler (Konami)",
	"1981",
	"Konami",
	"Nicola Salmoria",
	0,
	&jungler_machine_driver,
	jungler_init,

	jungler_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	jungler_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	jungler_hiload, jungler_hisave
};

struct GameDriver junglers_driver =
{
	__FILE__,
	&jungler_driver,
	"junglers",
	"Jungler (Stern)",
	"1981",
	"Stern",
	"Nicola Salmoria",
	0,
	&jungler_machine_driver,
	jungler_init,

	junglers_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	jungler_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	jungler_hiload, jungler_hisave
};

struct GameDriver locomotn_driver =
{
	__FILE__,
	0,
	"locomotn",
	"Loco-Motion",
	"1982",
	"Konami (Centuri license)",
	"Nicola Salmoria\nKevin Klopp (color info)",
	0,
	&locomotn_machine_driver,
	0,

	locomotn_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	locomotn_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	locomotn_hiload, locomotn_hisave
};

struct GameDriver commsega_driver =
{
	__FILE__,
	0,
	"commsega",
	"Commando (Sega)",
	"1983",
	"Sega",
	"Nicola Salmoria\nBrad Oliver",
	GAME_WRONG_COLORS,
	&commsega_machine_driver,
	0,

	commsega_rom,
	0, 0,
	0,
	0, /* sound_prom */

	commsega_input_ports,

	wrong_color_prom, 0, 0,	/* wrong */
	ORIENTATION_ROTATE_90,

	commsega_hiload, commsega_hisave
};
