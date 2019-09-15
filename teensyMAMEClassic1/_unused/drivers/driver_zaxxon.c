/***************************************************************************

Zaxxon memory map (preliminary)

0000-1fff ROM 3
2000-3fff ROM 2
4000-4fff ROM 1
6000-67ff RAM 1
6800-6fff RAM 2
8000-83ff Video RAM
a000-a0ff sprites

read:
c000      IN0
c001      IN1
c002      DSW0
c003      DSW1
c100      IN2
see the input_ports definition below for details on the input bits

write:
c000      coin A enable
c001      coin B enable
c002      coin aux enable
c003-c004 coin counters
c006      flip screen
ff3c-ff3f sound (see below)
fff0      interrupt enable
fff1      character color bank (not used during the game, but used in test mode)
fff8-fff9 background playfield position (11 bits)
fffa      background color bank (0 = standard  1 = reddish)
fffb      background enable

interrupts:
VBlank triggers IRQ, handled with interrupt mode 1
NMI enters the test mode.

Changes:
25 Jan 98 LBO
	* Added crude support for samples based on Frank's info. As of yet, I don't have
	  a set that matches the names - I need a way to edit the .sam files I have.
	  Hopefully I'll be able to create a good set shortly. I also don't know which
	  sounds "loop".
26 Jan 98 LBO
	* Fixed the sound support. I lack explosion samples and the base missile sample so
	  these are untested. I'm also unsure about the background noise. It seems to have
	  a variable volume so I've tried to reproduce that via just 1 sample.

12 Mar 98 ATJ
        * For the moment replaced Brad's version of the samples with mine from the Mame/P
          release. As yet, no variable volume, but I will be combining the features from
          Brad's driver into mine ASAP.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



int zaxxon_IN2_r(int offset);

extern unsigned char *zaxxon_char_color_bank;
extern unsigned char *zaxxon_background_position;
extern unsigned char *zaxxon_background_color_bank;
extern unsigned char *zaxxon_background_enable;
void zaxxon_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int  zaxxon_vh_start(void);
void zaxxon_vh_stop(void);
void zaxxon_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern int zaxxon_vid_type;

void zaxxon_sound_w(int offset, int data);

/* in machine/segacrpt.c */
void szaxxon_decode(void);
void futspy_decode(void);



void zaxxon_init_machine(void)
{
	zaxxon_vid_type = 0;
}

void futspy_init_machine(void)
{
	zaxxon_vid_type = 2;
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x6000, 0x6fff, MRA_RAM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0xa000, 0xa0ff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r },	/* IN0 */
	{ 0xc001, 0xc001, input_port_1_r },	/* IN1 */
	{ 0xc100, 0xc100, input_port_2_r },	/* IN2 */
	{ 0xc002, 0xc002, input_port_3_r },	/* DSW0 */
	{ 0xc003, 0xc003, input_port_4_r },	/* DSW1 */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x4fff, MWA_ROM },
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0xa000, 0xa0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc000, 0xc002, MWA_NOP },	/* coin enables */
	{ 0xff3c, 0xff3e, zaxxon_sound_w },
	{ 0xfff0, 0xfff0, interrupt_enable_w },
	{ 0xfff1, 0xfff1, MWA_RAM, &zaxxon_char_color_bank },
	{ 0xfff8, 0xfff9, MWA_RAM, &zaxxon_background_position },
	{ 0xfffa, 0xfffa, MWA_RAM, &zaxxon_background_color_bank },
	{ 0xfffb, 0xfffb, MWA_RAM, &zaxxon_background_enable },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress futspy_writemem[] =
{
	{ 0x0000, 0x4fff, MWA_ROM },
	{ 0x6000, 0x6fff, MWA_RAM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0xa000, 0xa0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc000, 0xc002, MWA_NOP },	/* coin enables */
	{ 0xe03c, 0xe03e, zaxxon_sound_w },
	{ 0xe0f0, 0xe0f0, interrupt_enable_w },
	{ 0xe0f1, 0xe0f1, MWA_RAM, &zaxxon_char_color_bank },
	{ 0xe0f8, 0xe0f9, MWA_RAM, &zaxxon_background_position },
	{ 0xe0fa, 0xe0fa, MWA_RAM, &zaxxon_background_color_bank },
	{ 0xe0fb, 0xe0fb, MWA_RAM, &zaxxon_background_enable },
	{ -1 }  /* end of table */
};


/***************************************************************************

  Zaxxon uses NMI to trigger the self test. We use a fake input port to
  tie that event to a keypress.

***************************************************************************/
static int zaxxon_interrupt(void)
{
	if (readinputport(5) & 1)	/* get status of the F2 key */
		return nmi_interrupt();	/* trigger self test */
	else return interrupt();
}

INPUT_PORTS_START( zaxxon_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )	/* the self test calls this UP */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )	/* the self test calls this DOWN */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* button 2 - unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )	/* the self test calls this UP */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )	/* the self test calls this DOWN */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* button 2 - unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	/* the coin inputs must stay active for exactly one frame, otherwise */
	/* the game will keep inserting coins. */
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			"Coin A", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_COIN2 | IPF_IMPULSE,
			"Coin B", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_COIN3 | IPF_IMPULSE,
			"Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "10000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x02, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	/* The Super Zaxxon manual lists the following as unused. */
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty???", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Easy?" )
	PORT_DIPSETTING(    0x04, "Medium?" )
	PORT_DIPSETTING(    0x08, "Hard?" )
	PORT_DIPSETTING(    0x00, "Hardest?" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x40, 0x40, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START	/* DSW1 */
 	PORT_DIPNAME( 0x0f, 0x03, "B Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0f, "4/1" )
	PORT_DIPSETTING(    0x07, "3/1" )
	PORT_DIPSETTING(    0x0b, "2/1" )
	PORT_DIPSETTING(    0x06, "2/1+Bonus each 5" )
	PORT_DIPSETTING(    0x0a, "2/1+Bonus each 2" )
	PORT_DIPSETTING(    0x03, "1/1" )
	PORT_DIPSETTING(    0x02, "1/1+Bonus each 5" )
	PORT_DIPSETTING(    0x0c, "1/1+Bonus each 4" )
	PORT_DIPSETTING(    0x04, "1/1+Bonus each 2" )
	PORT_DIPSETTING(    0x0d, "1/2" )
	PORT_DIPSETTING(    0x08, "1/2+Bonus each 5" )
	PORT_DIPSETTING(    0x00, "1/2+Bonus each 4" )
	PORT_DIPSETTING(    0x05, "1/3" )
	PORT_DIPSETTING(    0x09, "1/4" )
	PORT_DIPSETTING(    0x01, "1/5" )
	PORT_DIPSETTING(    0x0e, "1/6" )
	PORT_DIPNAME( 0xf0, 0x30, "A Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0xf0, "4/1" )
	PORT_DIPSETTING(    0x70, "3/1" )
	PORT_DIPSETTING(    0xb0, "2/1" )
	PORT_DIPSETTING(    0x60, "2/1+Bonus each 5" )
	PORT_DIPSETTING(    0xa0, "2/1+Bonus each 2" )
	PORT_DIPSETTING(    0x30, "1/1" )
	PORT_DIPSETTING(    0x20, "1/1+Bonus each 5" )
	PORT_DIPSETTING(    0xc0, "1/1+Bonus each 4" )
	PORT_DIPSETTING(    0x40, "1/1+Bonus each 2" )
	PORT_DIPSETTING(    0xd0, "1/2" )
	PORT_DIPSETTING(    0x80, "1/2+Bonus each 5" )
	PORT_DIPSETTING(    0x00, "1/2+Bonus each 4" )
	PORT_DIPSETTING(    0x50, "1/3" )
	PORT_DIPSETTING(    0x90, "1/4" )
	PORT_DIPSETTING(    0x10, "1/5" )
	PORT_DIPSETTING(    0xe0, "1/6" )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( futspy_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )	/* the self test calls this UP */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )	/* the self test calls this DOWN */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )	/* the self test calls this UP */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )	/* the self test calls this DOWN */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	/* the coin inputs must stay active for exactly one frame, otherwise */
	/* the game will keep inserting coins. */
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			"Coin A", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_COIN2 | IPF_IMPULSE,
			"Coin B", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_COIN3 | IPF_IMPULSE,
			"Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )

	PORT_START	/* DSW1 */
 	PORT_DIPNAME( 0x0f, 0x00, "A Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "4/1" )
	PORT_DIPSETTING(    0x07, "3/1" )
	PORT_DIPSETTING(    0x06, "2/1" )
	PORT_DIPSETTING(    0x0a, "2/1+Bonus each 5" )
	PORT_DIPSETTING(    0x0b, "2/1+Bonus each 4" )
	PORT_DIPSETTING(    0x00, "1/1" )
	PORT_DIPSETTING(    0x0c, "1/1+Bonus each 5" )
	PORT_DIPSETTING(    0x0d, "1/1+Bonus each 4" )
	PORT_DIPSETTING(    0x0e, "1/1+Bonus each 2" )
	PORT_DIPSETTING(    0x09, "2/3" )
	PORT_DIPSETTING(    0x01, "1/2" )
	PORT_DIPSETTING(    0x0f, "1/2+Bonus each 5" )
	PORT_DIPSETTING(    0x02, "1/3" )
	PORT_DIPSETTING(    0x03, "1/4" )
	PORT_DIPSETTING(    0x04, "1/5" )
	PORT_DIPSETTING(    0x05, "1/6" )
	PORT_DIPNAME( 0xf0, 0x00, "B Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "4/1" )
	PORT_DIPSETTING(    0x70, "3/1" )
	PORT_DIPSETTING(    0x60, "2/1" )
	PORT_DIPSETTING(    0xa0, "2/1+Bonus each 5" )
	PORT_DIPSETTING(    0xb0, "2/1+Bonus each 4" )
	PORT_DIPSETTING(    0x00, "1/1" )
	PORT_DIPSETTING(    0xc0, "1/1+Bonus each 5" )
	PORT_DIPSETTING(    0xd0, "1/1+Bonus each 4" )
	PORT_DIPSETTING(    0xe0, "1/1+Bonus each 2" )
	PORT_DIPSETTING(    0x90, "2/3" )
	PORT_DIPSETTING(    0x10, "1/2" )
	PORT_DIPSETTING(    0xf0, "1/2+Bonus each 5" )
	PORT_DIPSETTING(    0x20, "1/3" )
	PORT_DIPSETTING(    0x30, "1/4" )
	PORT_DIPSETTING(    0x40, "1/5" )
	PORT_DIPSETTING(    0x50, "1/6" )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x01, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_BITX( 0,       0x0c, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "20k 40k 60k" )
	PORT_DIPSETTING(    0x10, "30k 60k 90k" )
	PORT_DIPSETTING(    0x20, "40k 70k 100k" )
	PORT_DIPSETTING(    0x30, "40k 80k 120k" )
	PORT_DIPNAME( 0xc0, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x80, "Hard" )
	PORT_DIPSETTING(    0xc0, "Hardest" )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
INPUT_PORTS_END



struct GfxLayout zaxxon_charlayout1 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel (actually 2, the third plane is 0) */
	{ 2*256*8*8, 256*8*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

struct GfxLayout zaxxon_charlayout2 =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	32,32,	/* 32*32 sprites */
	64,	/* 64 sprites */
	3,	/* 3 bits per pixel */
	{ 2*64*128*8, 64*128*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 24*8+4, 24*8+5, 24*8+6, 24*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8,
			64*8, 65*8, 66*8, 67*8, 68*8, 69*8, 70*8, 71*8,
			96*8, 97*8, 98*8, 99*8, 100*8, 101*8, 102*8, 103*8 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout futspy_spritelayout =
{
	32,32,	/* 32*32 sprites */
	128,	/* 128 sprites */
	3,	/* 3 bits per pixel */
	{ 2*128*128*8, 128*128*8, 0 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 24*8+4, 24*8+5, 24*8+6, 24*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8,
			64*8, 65*8, 66*8, 67*8, 68*8, 69*8, 70*8, 71*8,
			96*8, 97*8, 98*8, 99*8, 100*8, 101*8, 102*8, 103*8 },
	128*8	/* every sprite takes 128 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &zaxxon_charlayout1,   0, 32 },	/* characters */
	{ 1, 0x1800, &zaxxon_charlayout2,   0, 32 },	/* background tiles */
	{ 1, 0x7800, &spritelayout,  0, 32 },			/* sprites */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo futspy_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &zaxxon_charlayout1,   0, 32 },	/* characters */
	{ 1, 0x1800, &zaxxon_charlayout2,   0, 32 },	/* background tiles */
	{ 1, 0x7800, &futspy_spritelayout,  0, 32 },			/* sprites */
	{ -1 } /* end of array */
};



static struct Samplesinterface zaxxon_samples_interface =
{
	12	/* 12 channels */
};


static struct MachineDriver zaxxon_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz ?? */
			0,
			readmem,writemem,0,0,
			zaxxon_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	zaxxon_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1,2*8, 30*8-1 },
	gfxdecodeinfo,
	256,32*8,
	zaxxon_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	zaxxon_vh_start,
	zaxxon_vh_stop,
	zaxxon_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SAMPLES,
			&zaxxon_samples_interface
		}
	}
};

static struct MachineDriver futspy_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz ?? */
			0,
			readmem,futspy_writemem,0,0,
			zaxxon_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	futspy_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1,2*8, 30*8-1 },
	futspy_gfxdecodeinfo,
	256,32*8,
	zaxxon_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	zaxxon_vh_start,
	zaxxon_vh_stop,
	zaxxon_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SAMPLES,
			&zaxxon_samples_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

static const char *zaxxon_sample_names[] =
{
	"*zaxxon",
	"03.sam",	/* Homing Missile */
	"02.sam",	/* Base Missile */
	"01.sam",	/* Laser (force field) */
	"00.sam",	/* Battleship (end of level boss) */
	"11.sam",	/* S-Exp (enemy explosion) */
	"10.sam",	/* M-Exp (ship explosion) */
	"08.sam", 	/* Cannon (ship fire) */
	"23.sam",	/* Shot (enemy fire) */
	"21.sam",	/* Alarm 2 (target lock) */
	"20.sam",	/* Alarm 3 (low fuel) */
	"05.sam",	/* initial background noise */
	"04.sam",	/* looped asteroid noise */
	0
};

ROM_START( zaxxon_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "zaxxon.3",     0x0000, 0x2000, 0x6e2b4a30 )
	ROM_LOAD( "zaxxon.2",     0x2000, 0x2000, 0x1c9ea398 )
	ROM_LOAD( "zaxxon.1",     0x4000, 0x1000, 0x1c123ef9 )

	ROM_REGION_DISPOSE(0xd800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "zaxxon.14",    0x0000, 0x0800, 0x07bf8c52 )	/* characters */
	ROM_LOAD( "zaxxon.15",    0x0800, 0x0800, 0xc215edcb )
	/* 1000-17ff empty space to convert the characters as 3bpp instead of 2 */
	ROM_LOAD( "zaxxon.6",     0x1800, 0x2000, 0x6e07bb68 )	/* background tiles */
	ROM_LOAD( "zaxxon.5",     0x3800, 0x2000, 0x0a5bce6a )
	ROM_LOAD( "zaxxon.4",     0x5800, 0x2000, 0xa5bf1465 )
	ROM_LOAD( "zaxxon.11",    0x7800, 0x2000, 0xeaf0dd4b )	/* sprites */
	ROM_LOAD( "zaxxon.12",    0x9800, 0x2000, 0x1c5369c7 )
	ROM_LOAD( "zaxxon.13",    0xb800, 0x2000, 0xab4e8a9a )

	ROM_REGION(0x8000)	/* background graphics */
	ROM_LOAD( "zaxxon.8",     0x0000, 0x2000, 0x28d65063 )
	ROM_LOAD( "zaxxon.7",     0x2000, 0x2000, 0x6284c200 )
	ROM_LOAD( "zaxxon.10",    0x4000, 0x2000, 0xa95e61fd )
	ROM_LOAD( "zaxxon.9",     0x6000, 0x2000, 0x7e42691f )

	ROM_REGION(0x0200)	/* color proms */
	ROM_LOAD( "zaxxon.u98",   0x0000, 0x0100, 0x6cc6695b ) /* palette */
	ROM_LOAD( "zaxxon.u72",   0x0100, 0x0100, 0xdeaa21f7 ) /* char lookup table */
ROM_END

ROM_START( szaxxon_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "suzaxxon.3",   0x0000, 0x2000, 0xaf7221da )
	ROM_LOAD( "suzaxxon.2",   0x2000, 0x2000, 0x1b90fb2a )
	ROM_LOAD( "suzaxxon.1",   0x4000, 0x1000, 0x07258b4a )

	ROM_REGION_DISPOSE(0xd800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "suzaxxon.14",  0x0000, 0x0800, 0xbccf560c )	/* characters */
	ROM_LOAD( "suzaxxon.15",  0x0800, 0x0800, 0xd28c628b )
	/* 1000-17ff empty space to convert the characters as 3bpp instead of 2 */
	ROM_LOAD( "suzaxxon.6",   0x1800, 0x2000, 0xf51af375 )	/* background tiles */
	ROM_LOAD( "suzaxxon.5",   0x3800, 0x2000, 0xa7de021d )
	ROM_LOAD( "suzaxxon.4",   0x5800, 0x2000, 0x5bfb3b04 )
	ROM_LOAD( "suzaxxon.11",  0x7800, 0x2000, 0x1503ae41 )	/* sprites */
	ROM_LOAD( "suzaxxon.12",  0x9800, 0x2000, 0x3b53d83f )
	ROM_LOAD( "suzaxxon.13",  0xb800, 0x2000, 0x581e8793 )

	ROM_REGION(0x8000)	/* background graphics */
	ROM_LOAD( "suzaxxon.8",   0x0000, 0x2000, 0xdd1b52df )
	ROM_LOAD( "suzaxxon.7",   0x2000, 0x2000, 0xb5bc07f0 )
	ROM_LOAD( "suzaxxon.10",  0x4000, 0x2000, 0x68e84174 )
	ROM_LOAD( "suzaxxon.9",   0x6000, 0x2000, 0xa509994b )

	ROM_REGION(0x0200)	/* color proms */
	ROM_LOAD( "suzaxxon.u98", 0x0000, 0x0100, 0x15727a9f ) /* palette */
	ROM_LOAD( "suzaxxon.u72", 0x0100, 0x0100, 0xdeaa21f7 ) /* char lookup table */
ROM_END

ROM_START( futspy_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "fs_snd.u27",   0x0000, 0x2000, 0x7578fe7f )
	ROM_LOAD( "fs_snd.u28",   0x2000, 0x2000, 0x8ade203c )
	ROM_LOAD( "fs_snd.u29",   0x4000, 0x1000, 0x734299c3 )

	ROM_REGION_DISPOSE(0x13800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "fs_snd.u68",   0x0000, 0x0800, 0x305fae2d )	/* characters */
	ROM_LOAD( "fs_snd.u69",   0x0800, 0x0800, 0x3c5658c0 )
	/* 1000-17ff empty space to convert the characters as 3bpp instead of 2 */
	ROM_LOAD( "fs_vid.113",   0x1800, 0x2000, 0x36d2bdf6 )	/* background tiles */
	ROM_LOAD( "fs_vid.112",   0x3800, 0x2000, 0x3740946a )
	ROM_LOAD( "fs_vid.111",   0x5800, 0x2000, 0x4cd4df98 )
	ROM_LOAD( "fs_vid.u77",   0x7800, 0x4000, 0x1b93c9ec )	/* sprites */
	ROM_LOAD( "fs_vid.u78",   0xb800, 0x4000, 0x50e55262 )
	ROM_LOAD( "fs_vid.u79",   0xf800, 0x4000, 0xbfb02e3e )

	ROM_REGION(0x8000)	/* background graphics */
	ROM_LOAD( "fs_vid.u91",   0x0000, 0x2000, 0x86da01f4 )
	ROM_LOAD( "fs_vid.u90",   0x2000, 0x2000, 0x2bd41d2d )
	ROM_LOAD( "fs_vid.u93",   0x4000, 0x2000, 0xb82b4997 )
	ROM_LOAD( "fs_vid.u92",   0x6000, 0x2000, 0xaf4015af )

	ROM_REGION(0x0200)	/* color proms */
	ROM_LOAD( "futrprom.u98", 0x0000, 0x0100, 0x9ba2acaa ) /* palette */
	ROM_LOAD( "futrprom.u72", 0x0100, 0x0100, 0xf9e26790 ) /* char lookup table */
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x6110],"\x00\x89\x00",3) == 0 &&
			memcmp(&RAM[0x6179],"\x00\x37\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x6100],21*6);
			RAM[0x6038] = RAM[0x6110];
			RAM[0x6039] = RAM[0x6111];
			RAM[0x603a] = RAM[0x6112];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* make sure that the high score table is still valid (entering the */
	/* test mode corrupts it) */
	if (memcmp(&RAM[0x6110],"\x00\x00\x00",3) != 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
		{
			osd_fwrite(f,&RAM[0x6100],21*6);
			osd_fclose(f);
		}
	}
}

static int futspy_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0x0427],"\x00\x00\x01",3) == 0 &&
			memcmp(&RAM[0x0460],"\x49\x44\x41",3) == 0 &&
			memcmp(&RAM[0x6419],"\x00\x00\x01",3) == 0 &&
			memcmp(&RAM[0x6450],"\x10\x00\x49",3) == 0 )
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x6419],60);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void futspy_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x6419],60);
		osd_fclose(f);
	}
}



struct GameDriver zaxxon_driver =
{
	__FILE__,
	0,
	"zaxxon",
	"Zaxxon",
	"1982",
	"Sega",
	"Mirko Buffoni (MAME driver)\nNicola Salmoria (MAME driver)\nAlex Judd (sound)\nGerald Vanderick (color info)\nFrank Palazzolo (sound info)\nRiek Gladys (sound info)\nJohn Butler (video)",
	0,
	&zaxxon_machine_driver,
	0,

	zaxxon_rom,
	0, 0,
	zaxxon_sample_names,
	0,	/* sound_prom */

	zaxxon_input_ports,

	PROM_MEMORY_REGION(3), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver szaxxon_driver =
{
	__FILE__,
	0,
	"szaxxon",
	"Super Zaxxon",
	"1982",
	"Sega",
	"Mirko Buffoni (MAME driver)\nNicola Salmoria (MAME driver)\nAlex Judd (sound)\nTim Lindquist (encryption and color info)\nFrank Palazzolo (sound info)\nRiek Gladys (sound info)",
	0,
	&zaxxon_machine_driver,
	0,

	szaxxon_rom,
	0, szaxxon_decode,
	zaxxon_sample_names,
	0,	/* sound_prom */

	zaxxon_input_ports,

	PROM_MEMORY_REGION(3), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver futspy_driver =
{
	__FILE__,
	0,
	"futspy",
	"Future Spy",
	"1984",
	"Sega",
	"Nicola Salmoria",
	0,
	&futspy_machine_driver,
	0,

	futspy_rom,
	0, futspy_decode,
	0,
	0,	/* sound_prom */

	futspy_input_ports,

	PROM_MEMORY_REGION(3), 0, 0,
	ORIENTATION_ROTATE_270,

	futspy_hiload, futspy_hisave
};
