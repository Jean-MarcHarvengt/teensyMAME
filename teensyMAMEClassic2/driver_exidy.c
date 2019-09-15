/***************************************************************************

Exidy memory map

0000-00FF R/W Zero Page RAM
0100-01FF R/W Stack RAM
0200-03FF R/W Scratchpad RAM
0800-3FFF  R  Program ROM			   (Targ, Spectar only)
1A00	   R  PX3 (Player 2 inputs)    (Fax only)
			  bit 4  D
			  bit 5  C
			  bit 6  B
			  bit 7  A
1C00	   R  PX2 (Player 1 inputs)    (Fax only)
			  bit 0  2 player start
			  bit 1  1 player start
			  bit 4  D
			  bit 5  C
			  bit 6  B
			  bit 7  A
2000-3FFF  R  Banked question ROM	   (Fax only)
4000-43FF R/W Screen RAM
4800-4FFF R/W Character Generator RAM (except Pepper II and Fax)
5000	   W  Motion Object 1 Horizontal Position Latch (sprite 1 X)
5040	   W  Motion Object 1 Vertical Position Latch	(sprite 1 Y)
5080	   W  Motion Object 2 Horizontal Position Latch (sprite 2 X)
50C0	   W  Motion Object 2 Vertical Position Latch	(sprite 2 Y)
5100	   R  Option Dipswitch Port
			  bit 0  coin 2 (NOT inverted) (must activate together with $5103 bit 5)
			  bit 1-2  bonus
			  bit 3-4  coins per play
			  bit 5-6  lives
			  bit 7  US/UK coins
5100	   W  Motion Objects Image Latch
			  Sprite number  bits 0-3 Sprite #1  4-7 Sprite #2
5101	   R  Control Inputs Port
			  bit 0  start 1
			  bit 1  start 2
			  bit 2  right
			  bit 3  left
			  bit 5  up
			  bit 6  down
			  bit 7  coin 1 (must activate together with $5103 bit 6)
5101	   W  Output Control Latch (not used in PEPPER II upright)
			  bit 7  Enable sprite #1
			  bit 6  Enable sprite #2
5103	   R  Interrupt Condition Latch
			  bit 0  LNG0 - supposedly a language DIP switch
			  bit 1  LNG1 - supposedly a language DIP switch
			  bit 2  different for each game, but generally a collision bit
			  bit 3  TABLE - supposedly a cocktail table DIP switch
			  bit 4  different for each game, but generally a collision bit
			  bit 5  coin 2 (must activate together with $5100 bit 0)
			  bit 6  coin 1 (must activate together with $5101 bit 7)
			  bit 7  L256 - VBlank?
5213	   R  IN2 (Mouse Trap)
			  bit 3  blue button
			  bit 2  free play
			  bit 1  red button
			  bit 0  yellow button
52XX	  R/W Audio/Color Board Communications
6000-6FFF R/W Character Generator RAM (Pepper II, Fax only)
8000-FFF9  R  Program memory space
FFFA-FFFF  R  Interrupt and Reset Vectors

Targ:
5200	  Sound board control
		bit 0 Music
		bit 1 Shoot
		bit 2 unused
		bit 3 Swarn
		bit 4 Sspec
		bit 5 crash
		bit 6 long
		bit 7 game

5201	  Sound board control
		bit 0 note
		bit 1 upper

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/6821pia.h"


/* These are defined in sndhrdw/exidy.c */
void exidy_shriot_w(int offset,int data);
int exidy_shriot_r(int offset);

void exidy_sh8253_w(int offset,int data);
int exidy_sh8253_r(int offset);

extern int exidy_sh_start(void);
extern void exidy_sh_stop(void);

/* These are defined in vidhrdw/exidy.c */

int exidy_vh_start(void);
void exidy_vh_stop(void);
void exidy_characterram_w(int offset,int data);
void exidy_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void exidy_color_w(int offset,int data);

extern unsigned char *exidy_characterram;
extern unsigned char *exidy_sprite_no;
extern unsigned char *exidy_sprite_enable;
extern unsigned char *exidy_sprite1_xpos;
extern unsigned char *exidy_sprite1_ypos;
extern unsigned char *exidy_sprite2_xpos;
extern unsigned char *exidy_sprite2_ypos;
extern unsigned char *exidy_color_latch;

extern unsigned char *exidy_collision;
extern int exidy_collision_counter;

/* These are defined in machine/exidy.c */

extern void fax_bank_select_w(int offset,int data);
extern int exidy_input_port_2_r(int offset);
extern void exidy_init_machine(void);
extern int venture_interrupt(void);
extern int venture_shinterrupt(void);

extern int exidy_interrupt(void);

extern unsigned char exidy_collision_mask;

/* These are defined in sndhrdw/targ.c */
extern unsigned char targ_spec_flag;
extern void targ_sh_w(int offset,int data);
extern int targ_sh_start(void);
extern void targ_sh_stop(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x0800, 0x3fff, MRA_ROM }, /* Targ, Spectar only */
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x4800, 0x4fff, MRA_RAM },
	{ 0x5100, 0x5100, input_port_0_r }, /* DSW */
	{ 0x5101, 0x5101, input_port_1_r }, /* IN0 */
	{ 0x5103, 0x5103, exidy_input_port_2_r, &exidy_collision }, /* IN1 */
	{ 0x5105, 0x5105, input_port_4_r }, /* IN3 - Targ, Spectar only */
	{ 0x5200, 0x520F, pia_1_r },
	{ 0x5213, 0x5213, input_port_3_r }, 	/* IN2 */
	{ 0x6000, 0x6fff, MRA_RAM }, /* Pepper II only */
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0800, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4800, 0x4fff, exidy_characterram_w, &exidy_characterram },
	{ 0x5000, 0x5000, MWA_RAM, &exidy_sprite1_xpos },
	{ 0x5040, 0x5040, MWA_RAM, &exidy_sprite1_ypos },
	{ 0x5080, 0x5080, MWA_RAM, &exidy_sprite2_xpos },
	{ 0x50C0, 0x50C0, MWA_RAM, &exidy_sprite2_ypos },
	{ 0x5100, 0x5100, MWA_RAM, &exidy_sprite_no },
	{ 0x5101, 0x5101, MWA_RAM, &exidy_sprite_enable },
	{ 0x5200, 0x520F, pia_1_w },
	{ 0x5210, 0x5212, exidy_color_w, &exidy_color_latch },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress targ_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0800, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4800, 0x4fff, exidy_characterram_w, &exidy_characterram },
	{ 0x5000, 0x5000, MWA_RAM, &exidy_sprite1_xpos },
	{ 0x5040, 0x5040, MWA_RAM, &exidy_sprite1_ypos },
	{ 0x5080, 0x5080, MWA_RAM, &exidy_sprite2_xpos },
	{ 0x50C0, 0x50C0, MWA_RAM, &exidy_sprite2_ypos },
	{ 0x5100, 0x5100, MWA_RAM, &exidy_sprite_no },
	{ 0x5101, 0x5101, MWA_RAM, &exidy_sprite_enable },
	{ 0x5200, 0x5201, targ_sh_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress pepper2_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x5000, 0x5000, MWA_RAM, &exidy_sprite1_xpos },
	{ 0x5040, 0x5040, MWA_RAM, &exidy_sprite1_ypos },
	{ 0x5080, 0x5080, MWA_RAM, &exidy_sprite2_xpos },
	{ 0x50C0, 0x50C0, MWA_RAM, &exidy_sprite2_ypos },
	{ 0x5100, 0x5100, MWA_RAM, &exidy_sprite_no },
	{ 0x5101, 0x5101, MWA_RAM, &exidy_sprite_enable },
	{ 0x5200, 0x520F, pia_1_w },
	{ 0x5210, 0x5212, exidy_color_w, &exidy_color_latch },
	{ 0x5213, 0x5217, MWA_NOP }, /* empty control lines on color/sound board */
	{ 0x6000, 0x6fff, exidy_characterram_w, &exidy_characterram }, /* two 6116 character RAMs */
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress fax_readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x0400, 0x07ff, MRA_RAM }, /* Fax only */
	{ 0x1a00, 0x1a00, input_port_4_r }, /* IN3 - Fax only */
	{ 0x1c00, 0x1c00, input_port_3_r }, /* IN2 - Fax only */
	{ 0x2000, 0x3fff, MRA_BANK1 }, /* Fax only */
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x5100, 0x5100, input_port_0_r }, /* DSW */
	{ 0x5101, 0x5101, input_port_1_r }, /* IN0 */
	{ 0x5103, 0x5103, exidy_input_port_2_r, &exidy_collision }, /* IN1 */
	{ 0x5200, 0x520F, pia_1_r },
	{ 0x5213, 0x5213, input_port_3_r }, 	/* IN2 */
	{ 0x6000, 0x6fff, MRA_RAM }, /* Fax, Pepper II only */
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress fax_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, MWA_RAM }, /* Fax only */
	{ 0x2000, 0x2000, fax_bank_select_w }, /* Fax only */
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x5000, 0x5000, MWA_RAM, &exidy_sprite1_xpos },
	{ 0x5040, 0x5040, MWA_RAM, &exidy_sprite1_ypos },
	{ 0x5080, 0x5080, MWA_RAM, &exidy_sprite2_xpos },
	{ 0x50C0, 0x50C0, MWA_RAM, &exidy_sprite2_ypos },
	{ 0x5100, 0x5100, MWA_RAM, &exidy_sprite_no },
	{ 0x5101, 0x5101, MWA_RAM, &exidy_sprite_enable },
	{ 0x5200, 0x520F, pia_1_w },
	{ 0x5210, 0x5212, exidy_color_w, &exidy_color_latch },
	{ 0x5213, 0x5217, MWA_NOP }, /* empty control lines on color/sound board */
	{ 0x6000, 0x6fff, exidy_characterram_w, &exidy_characterram }, /* two 6116 character RAMs */
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
    { 0x0000, 0x07ff, MRA_RAM },
    { 0x0800, 0x0FFF, exidy_shriot_r },
    { 0x1000, 0x100F, pia_2_r },
    { 0x1800, 0x1FFF, exidy_sh8253_r },
    { 0x2000, 0x3FFF, MRA_RAM },
	{ 0x5800, 0x7fff, MRA_ROM },
	{ 0x8000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
    { 0x0000, 0x07FF, MWA_RAM },
    { 0x0800, 0x0FFF, exidy_shriot_w },
    { 0x1000, 0x100F, pia_2_w },
    { 0x1800, 0x1FFF, exidy_sh8253_w },
    { 0x2000, 0x3FFF, MWA_RAM },
    { 0x5800, 0x7fff, MWA_ROM },
	{ 0x8000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

/***************************************************************************
Input Ports
***************************************************************************/

INPUT_PORTS_START( sidetrac_input_ports )
	PORT_START		/* DSW0 */
	PORT_DIPNAME(0x03, 0x00, "Trains", IP_KEY_NONE )
	PORT_DIPSETTING( 0x00, "01")
	PORT_DIPSETTING( 0x01, "02")
	PORT_DIPSETTING( 0x02, "03")
	PORT_DIPSETTING( 0x03, "04")
	PORT_DIPNAME( 0x0c, 0x04, "Coinage", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "1C / 2P" )
	PORT_DIPSETTING(	0x04, "1C / 1P" )
	PORT_DIPSETTING(	0x08, "2C / 1P" )
	PORT_DIPSETTING(    0x0c, "2C / 1P")
	PORT_DIPNAME(0x10, 0x10, "Top Score Award", IP_KEY_NONE )
	PORT_DIPSETTING(   0x00, "Off" )
    PORT_DIPSETTING(   0x10, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0xFF, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( targ_input_ports )
	PORT_START		/* DSW0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN2 ) /* upright/cocktail switch? */
	PORT_DIPNAME( 0x02, 0x00, "P Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "10P/1 C 50P Coin/6 Cs" )
	PORT_DIPSETTING(	0x02, "2x10P/1 C 50P Coin/3 Cs" )
	PORT_DIPNAME( 0x04, 0x00, "Top Score Award", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "Credit" )
	PORT_DIPSETTING(	0x04, "Extended Play" )
	PORT_DIPNAME( 0x18, 0x08, "Q Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(	0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(	0x08, "1 Coin/1 Credit" )
	PORT_DIPSETTING(	0x00, "1C/1C (no display)" )
	PORT_DIPSETTING(	0x18, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x60, 0x40, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(	0x60, "2" )
	PORT_DIPSETTING(	0x40, "3" )
	PORT_DIPSETTING(	0x20, "4" )
	PORT_DIPSETTING(	0x00, "5" )
	PORT_DIPNAME( 0x80, 0x80, "Currency", IP_KEY_NONE )
	PORT_DIPSETTING(	0x80, "Quarters" )
	PORT_DIPSETTING(	0x00, "Pence" )

	PORT_START	/* IN0 */
	PORT_BIT( 0x7F, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x1f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0xFF, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* identical to Targ, the only difference is the additional Language dip switch */
INPUT_PORTS_START( spectar_input_ports )
	PORT_START		/* DSW0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_COIN2 ) /* upright/cocktail switch? */
	PORT_DIPNAME( 0x02, 0x00, "P Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "10P/1 C 50P Coin/6 Cs" )
	PORT_DIPSETTING(	0x02, "2x10P/1 C 50P Coin/3 Cs" )
	PORT_DIPNAME( 0x04, 0x00, "Top Score Award", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "Credit" )
	PORT_DIPSETTING(	0x04, "Extended Play" )
	PORT_DIPNAME( 0x18, 0x08, "Q Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(	0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(	0x08, "1 Coin/1 Credit" )
	PORT_DIPSETTING(	0x00, "1C/1C (no display)" )
	PORT_DIPSETTING(	0x18, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x60, 0x40, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(	0x60, "2" )
	PORT_DIPSETTING(	0x40, "3" )
	PORT_DIPSETTING(	0x20, "4" )
	PORT_DIPSETTING(	0x00, "5" )
	PORT_DIPNAME( 0x80, 0x80, "Currency", IP_KEY_NONE )
	PORT_DIPSETTING(	0x80, "Quarters" )
	PORT_DIPSETTING(	0x00, "Pence" )

	PORT_START	/* IN0 */
	PORT_BIT( 0x7F, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "English" )
	PORT_DIPSETTING(	0x01, "French" )
	PORT_DIPSETTING(	0x02, "German" )
	PORT_DIPSETTING(	0x03, "Spanish" )
	PORT_BIT( 0x1c, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0xFF, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	  | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( mtrap_input_ports )
	PORT_START	/* DSW0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(	0x06, "30000" )
	PORT_DIPSETTING(	0x04, "40000" )
	PORT_DIPSETTING(	0x02, "50000" )
	PORT_DIPSETTING(	0x00, "60000" )
	PORT_DIPNAME( 0x98, 0x98, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(	0x90, "2 Coins/1 Credit" )
	PORT_DIPSETTING(	0x98, "1 Coin/1 Credit" )
	PORT_DIPSETTING(	0x88, "1 Coin/2 Credits" )
	PORT_DIPSETTING(	0x80, "1 Coin/4 Credits" )
	PORT_DIPSETTING(	0x00, "Coin A 2/1 Coin B 1/3" )
	PORT_DIPSETTING(	0x08, "Coin A 1/3 Coin B 2/7" )
	PORT_DIPSETTING(	0x10, "Coin A 1/1 Coin B 1/4" )
	PORT_DIPSETTING(	0x18, "Coin A 1/1 Coin B 1/5" )
	PORT_DIPNAME( 0x60, 0x40, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(	0x60, "2" )
	PORT_DIPSETTING(	0x40, "3" )
	PORT_DIPSETTING(	0x20, "4" )
	PORT_DIPSETTING(	0x00, "5" )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON1, "Dog Button", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
/*
	The schematics claim these exist, but there's nothing in
	the ROMs to support that claim (as far as I can see):

	PORT_DIPNAME( 0x03, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "English" )
	PORT_DIPSETTING(	0x01, "French" )
	PORT_DIPSETTING(	0x02, "German" )
	PORT_DIPSETTING(	0x03, "Spanish" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "Upright" )
	PORT_DIPSETTING(	0x08, "Cocktail" )
*/

	PORT_BIT( 0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON2, "Yellow Button", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON3, "Red Button", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(	  0x04, 0x04, IPT_DIPSWITCH_NAME, "Free Play", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(	0x04, "Off" )
	PORT_DIPSETTING(	0x00, "On" )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON4, "Blue Button", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( venture_input_ports )
	PORT_START	/* DSW0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(	0x06, "20000" )
	PORT_DIPSETTING(	0x04, "30000" )
	PORT_DIPSETTING(	0x02, "40000" )
	PORT_DIPSETTING(	0x00, "50000" )
	PORT_DIPNAME( 0x98, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(	0x88, "2 Coins/1 Credit" )
	PORT_DIPSETTING(	0x80, "1 Coin/1 Credit" )
	PORT_DIPSETTING(	0x98, "1 Coin/2 Credits" )
	PORT_DIPSETTING(	0x00, "Pence: LC 2C/1C - RC 1C/3C" )
	PORT_DIPSETTING(	0x18, "Pence: LC 1C/1C - RC 1C/6C" )
	/*0x10 same as 0x00 */
	/*0x90 same as 0x80 */
	PORT_DIPNAME( 0x60, 0x20, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "2" )
	PORT_DIPSETTING(	0x20, "3" )
	PORT_DIPSETTING(	0x40, "4" )
	PORT_DIPSETTING(	0x60, "5" )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
/*
	The schematics claim these exist, but there's nothing in
	the ROMs to support that claim (as far as I can see):

	PORT_DIPNAME( 0x03, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "English" )
	PORT_DIPSETTING(	0x01, "French" )
	PORT_DIPSETTING(	0x02, "German" )
	PORT_DIPSETTING(	0x03, "Spanish" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "Upright" )
	PORT_DIPSETTING(	0x08, "Cocktail" )
*/

	PORT_BIT( 0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( pepper2_input_ports )
	PORT_START		/* DSW */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x06, 0x06, "Bonus Turns", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "70000" )
	PORT_DIPSETTING(	0x02, "60000" )
	PORT_DIPSETTING(	0x04, "50000" )
	PORT_DIPSETTING(	0x06, "40000" )
	PORT_DIPNAME( 0x60, 0x20, "Number of Turns", IP_KEY_NONE )
	PORT_DIPSETTING(	0x60, "5 Turns" )
	PORT_DIPSETTING(	0x40, "4 Turns" )
	PORT_DIPSETTING(	0x20, "3 Turns" )
	PORT_DIPSETTING(	0x00, "2 Turns" )
	PORT_DIPNAME( 0x98, 0x98, "Coins/Credit", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "L-2/1 R-1/3" )
	PORT_DIPSETTING(	0x08, "L-1/1 R-1/4" )
	PORT_DIPSETTING(	0x10, "L-1/1 R-1/5" )
	PORT_DIPSETTING(	0x18, "1/3 or 2/7" )
	PORT_DIPSETTING(	0x90, "1/4" )
	PORT_DIPSETTING(	0x80, "1/2" )
	PORT_DIPSETTING(	0x88, "2/1" )
	PORT_DIPSETTING(	0x98, "1/1" )

	PORT_START		/* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
/*
	The schematics claim these exist, but there's nothing in
	the ROMs to support that claim (as far as I can see):

	PORT_DIPNAME( 0x03, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "English" )
	PORT_DIPSETTING(	0x01, "French" )
	PORT_DIPSETTING(	0x02, "German" )
	PORT_DIPSETTING(	0x03, "Spanish" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "Upright" )
	PORT_DIPSETTING(	0x08, "Cocktail" )
*/

	PORT_BIT( 0x1F, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( fax_input_ports )
	PORT_START		/* DSW */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x06, 0x04, "Bonus Time", IP_KEY_NONE )
	PORT_DIPSETTING(	0x06, "8000 pts" )
	PORT_DIPSETTING(	0x04, "13000 pts" )
	PORT_DIPSETTING(	0x02, "18000 pts" )
	PORT_DIPSETTING(	0x00, "25000 pts" )
	PORT_DIPNAME( 0x60, 0x40, "Game/Bonus Times", IP_KEY_NONE )
	PORT_DIPSETTING(	0x60, ":32/:24" )
	PORT_DIPSETTING(	0x40, ":48/:36" )
	PORT_DIPSETTING(	0x20, "1:04/:48" )
	PORT_DIPSETTING(	0x00, "1:12/1:04" )
	PORT_DIPNAME( 0x98, 0x98, "Coins/Credit", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "L-2/1 R-1/3" )
	PORT_DIPSETTING(	0x08, "L-1/1 R-1/4" )
	PORT_DIPSETTING(	0x10, "L-1/1 R-1/5" )
	PORT_DIPSETTING(	0x18, "1/3 or 2/7" )
	PORT_DIPSETTING(	0x90, "1/4" )
	PORT_DIPSETTING(	0x80, "1/2" )
	PORT_DIPSETTING(	0x88, "2/1" )
	PORT_DIPSETTING(	0x98, "1/1" )

	PORT_START		/* IN0 */
	PORT_BIT ( 0x7F, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* IN1 */
/*
	The schematics claim these exist, but there's nothing in
	the ROMs to support that claim (as far as I can see):

	PORT_DIPNAME( 0x03, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "English" )
	PORT_DIPSETTING(	0x01, "French" )
	PORT_DIPSETTING(	0x02, "German" )
	PORT_DIPSETTING(	0x03, "Spanish" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "Upright" )
	PORT_DIPSETTING(	0x08, "Cocktail" )
*/

	PORT_BIT( 0x1B, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* Set when motion object 1 is drawn? */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* VBlank */

	PORT_START /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START /* IN3 */
	PORT_BIT( 0x0F, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

INPUT_PORTS_END


/***************************************************************************
Graphics Layout
***************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 1 bits per pixel */
	{ 0 }, /* No info needed for bit offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

/* Pepper II and Fax used a special Plane Generation Board for 2-bit graphics */

static struct GfxLayout pepper2_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 }, /* 2 bits separated by 0x0800 bytes */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};


static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	16*4,	/* 64 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8},
	8*32	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout targ_spritelayout =
{
	16,16,	/* 16*16 sprites */
	16*2,	/* 32 characters for Targ/Spectar */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8},
	8*32	/* every char takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x4800, &charlayout,	0, 4 }, 	/* the game dynamically modifies this */
	{ 1, 0x0000, &spritelayout, 8, 2 },  /* Sprites */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo pepper2_gfxdecodeinfo[] =
{
	{ 0, 0x6000, &pepper2_charlayout,	0, 4 },    /* the game dynamically modifies this */
	{ 1, 0x0000, &spritelayout, 16, 2 },  /* Angel/Devil/Zipper Ripper */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo targ_gfxdecodeinfo[] =
{
	{ 0, 0x4800, &charlayout,	0, 4 }, 	/* the game dynamically modifies this */
	{ 1, 0x0000, &targ_spritelayout, 8, 2 },  /* Sprites */
	{ -1 } /* end of array */
};



/* Arbitrary starting colors, modified by the game */

static unsigned char sidetrac_palette[] =
{
	0x00,0x00,0x00,   /* BACKGND */
	0x00,0x00,0x00,   /* CSPACE0 */
	0x00,0xff,0x00,   /* CSPACE1 */
	0xff,0xff,0xff,   /* CSPACE2 */
	0xff,0xff,0xff,   /* CSPACE3 */
	0xff,0x00,0xff,   /* 5LINES (unused?) */
	0xff,0xff,0x00,   /* 5MO2VID  */
	0xff,0xff,0xff	  /* 5MO1VID  */
};

static unsigned short sidetrac_colortable[] =
{
	/* one-bit characters */
	0, 4,  /* chars 0x00-0x3F */
	0, 3,  /* chars 0x40-0x7F */
	0, 2,  /* chars 0x80-0xBF */
	0, 1,  /* chars 0xC0-0xFF */

	/* Motion Object 1 */
	0, 7,

	/* Motion Object 2 */
	0, 6,

};

static unsigned char palette[] =
{
	0x00,0x00,0x00,   /* BACKGND */
	0x00,0x00,0xff,   /* CSPACE0 */
	0x00,0xff,0x00,   /* CSPACE1 */
	0x00,0xff,0xff,   /* CSPACE2 */
	0xff,0x00,0x00,   /* CSPACE3 */
	0xff,0x00,0xff,   /* 5LINES (unused?) */
	0xff,0xff,0x00,   /* 5MO2VID */
	0xff,0xff,0xff	  /* 5MO1VID */
};

/* Targ doesn't have a color PROM; colors are changed by the means of 8x3 */
/* dip switches on the board. Here are the colors they map to. */
static unsigned char targ_palette[] =
{
					/* color   use				  */
	0x00,0x00,0xFF, /* blue    background		  */
	0x00,0xFF,0xFF, /* cyan    characters 192-255 */
	0xFF,0xFF,0x00, /* yellow  characters 128-191 */
	0xFF,0xFF,0xFF, /* white   characters  64-127 */
	0xFF,0x00,0x00, /* red	   characters	0- 63 */
	0x00,0xFF,0xFF, /* cyan    not used 		  */
	0xFF,0xFF,0xFF, /* white   bullet sprite	  */
	0x00,0xFF,0x00, /* green   wummel sprite	  */
};

/* Spectar has different colors */
static unsigned char spectar_palette[] =
{
					/* color   use				  */
	0x00,0x00,0xFF, /* blue    background		  */
	0x00,0xFF,0x00, /* green   characters 192-255 */
	0x00,0xFF,0x00, /* green   characters 128-191 */
	0xFF,0xFF,0xFF, /* white   characters  64-127 */
	0xFF,0x00,0x00, /* red	   characters	0- 63 */
	0x00,0xFF,0x00, /* green   not used 		  */
	0xFF,0xFF,0x00, /* yellow  bullet sprite	  */
	0x00,0xFF,0x00, /* green   wummel sprite	  */
};


static unsigned short colortable[] =
{
	/* one-bit characters */
	0, 4,  /* chars 0x00-0x3F */
	0, 3,  /* chars 0x40-0x7F */
	0, 2,  /* chars 0x80-0xBF */
	0, 1,  /* chars 0xC0-0xFF */

	/* Motion Object 1 */
	0, 7,

	/* Motion Object 2 */
	0, 6,

};

static unsigned short pepper2_colortable[] =
{
	/* two-bit characters */
	/* (Because this is 2-bit color, the colorspace is only divided
		in half instead of in quarters.  That's why 00-3F = 40-7F and
		80-BF = C0-FF) */
	0, 0, 4, 3,  /* chars 0x00-0x3F */
	0, 0, 4, 3,  /* chars 0x40-0x7F */
	0, 0, 2, 1,  /* chars 0x80-0xBF */
	0, 0, 2, 1,  /* chars 0xC0-0xFF */

	/* Motion Object 1 */
	0, 7,

	/* Motion Object 2 */
	0, 6,

};



/***************************************************************************
  Game drivers
***************************************************************************/

static const char *targ_sample_names[] =
{
	"*targ",
	"expl.sam",
	"shot.sam",
	"sexpl.sam",
	"spslow.sam",
	"spfast.sam",
	0	/* end of array */
};

static struct Samplesinterface targ_samples_interface=
{
	3	/* 3 Channels */
};

static struct CustomSound_interface targ_custom_interface =
{
	targ_sh_start,
	targ_sh_stop,
	0
};

static struct DACinterface targ_DAC_interface =
{
	1,
    { 100 }
};



static struct MachineDriver targ_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			11289000/16,	/* .705562 MHz */
			0,
			readmem,targ_writemem,0,0,
			exidy_interrupt,1
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	exidy_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	targ_gfxdecodeinfo,
	sizeof(targ_palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	exidy_vh_start,
	exidy_vh_stop,
	exidy_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_CUSTOM,
			&targ_custom_interface
		},
		{
			SOUND_SAMPLES,
			&targ_samples_interface
        },
        {
            SOUND_DAC,
            &targ_DAC_interface
        }
	}
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			11289000/16,
			0,
			readmem,writemem,0,0,
			exidy_interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			3579545/4,
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
    32, /* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	exidy_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	exidy_vh_start,
	exidy_vh_stop,
	exidy_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,

};

static struct CustomSound_interface exidy_custom_interface =
{
    exidy_sh_start,
    exidy_sh_stop,
	0
};

static struct Samplesinterface venture_samples_interface=
{
	3	/* 3 Channels */
};

static struct MachineDriver venture_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			11289000/16,
			0,
			readmem,writemem,0,0,
			venture_interrupt,32 /* Need to have multiple IRQs per frame if there's a collision */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			3579545/4,
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10, /* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	exidy_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	exidy_vh_start,
	exidy_vh_stop,
	exidy_vh_screenrefresh,

	/* sound hardware */
        0,0,0,0,
	{
		{
			SOUND_SAMPLES,
                        &venture_samples_interface
                },
		{
			SOUND_CUSTOM,
                        &exidy_custom_interface
                }
        }
};


static struct MachineDriver pepper2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			11289000/16,
			0,
			readmem,pepper2_writemem,0,0,
			exidy_interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			3579545/4,
			2,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10, /* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	exidy_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	pepper2_gfxdecodeinfo,
	sizeof(palette)/3,sizeof(pepper2_colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	exidy_vh_start,
	exidy_vh_stop,
	exidy_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SAMPLES,
                        &venture_samples_interface
                },
		{
			SOUND_CUSTOM,
                        &exidy_custom_interface
                }
        }

};


static struct MachineDriver fax_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			11289000/16,
			0,
			fax_readmem,fax_writemem,0,0,
			exidy_interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			3579545/4,
			2,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10, /* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	exidy_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 31*8-1, 0*8, 32*8-1 },
	pepper2_gfxdecodeinfo,
	sizeof(palette)/3,sizeof(pepper2_colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	exidy_vh_start,
	exidy_vh_stop,
	exidy_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SAMPLES,
                        &venture_samples_interface
                },
		{
			SOUND_CUSTOM,
                        &exidy_custom_interface
                }
        }

};



/***************************************************************************
  Game ROMs
***************************************************************************/

ROM_START( sidetrac_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "stl8a-1",      0x2800, 0x0800, 0xe41750ff )
	ROM_LOAD( "stl7a-2",      0x3000, 0x0800, 0x57fb28dc )
	ROM_LOAD( "stl6a-2",      0x3800, 0x0800, 0x4226d469 )
	ROM_RELOAD(          0xf800, 0x0800 ) /* for the reset/interrupt vectors */
	ROM_LOAD( "stl9c-1",      0x4800, 0x0400, 0x08710a84 ) /* prom instead of ram chr gen*/

	ROM_REGION_DISPOSE(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "stl11d",       0x0000, 0x0200, 0x3bd1acc1 )
ROM_END

ROM_START( targ_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "targ10a1",     0x1800, 0x0800, 0x969744e1 )
	ROM_LOAD( "targ09a1",     0x2000, 0x0800, 0xa177a72d )
	ROM_LOAD( "targ08a1",     0x2800, 0x0800, 0x6e6928a5 )
	ROM_LOAD( "targ07a4",     0x3000, 0x0800, 0xe2f37f93 )
	ROM_LOAD( "targ06a3",     0x3800, 0x0800, 0xa60a1bfc )
	ROM_RELOAD(           0xf800, 0x0800 ) /* for the reset/interrupt vectors */

	ROM_REGION_DISPOSE(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "targ11d1",     0x0000, 0x0400, 0x9f03513e )
ROM_END

ROM_START( spectar_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "spl11a-3",     0x1000, 0x0800, 0x08880aff )
	ROM_LOAD( "spl10a-2",     0x1800, 0x0800, 0xfca667c1 )
	ROM_LOAD( "spl9a-3",      0x2000, 0x0800, 0x9d4ce8ba )
	ROM_LOAD( "spl8a-2",      0x2800, 0x0800, 0xcfacbadf )
	ROM_LOAD( "spl7a-2",      0x3000, 0x0800, 0x4c4741ff )
	ROM_LOAD( "spl6a-2",      0x3800, 0x0800, 0x0cb46b25 )
	ROM_RELOAD(           0xf800, 0x0800 )	/* for the reset/interrupt vectors */

	ROM_REGION_DISPOSE(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "hrl11d-2",     0x0000, 0x0400, 0xc55b645d )	/* this is actually not used (all FF) */
	ROM_CONTINUE(         0x0000, 0x0400 )	/* overwrite with the real one */
ROM_END

ROM_START( spectar1_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "spl12a1",      0x0800, 0x0800, 0x7002efb4 )
	ROM_LOAD( "spl11a1",      0x1000, 0x0800, 0x8eb8526a )
	ROM_LOAD( "spl10a1",      0x1800, 0x0800, 0x9d169b3d )
	ROM_LOAD( "spl9a1",       0x2000, 0x0800, 0x40e3eba1 )
	ROM_LOAD( "spl8a1",       0x2800, 0x0800, 0x64d8eb84 )
	ROM_LOAD( "spl7a1",       0x3000, 0x0800, 0xe08b0d8d )
	ROM_LOAD( "spl6a1",       0x3800, 0x0800, 0xf0e4e71a )
	ROM_RELOAD(          0xf800, 0x0800 )	/* for the reset/interrupt vectors */

	ROM_REGION_DISPOSE(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "hrl11d-2",     0x0000, 0x0400, 0xc55b645d )	/* this is actually not used (all FF) */
	ROM_CONTINUE(         0x0000, 0x0400 )	/* overwrite with the real one */
ROM_END

ROM_START( mtrap_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "mtl11a.bin",   0xA000, 0x1000, 0xbd6c3eb5 )
	ROM_LOAD( "mtl10a.bin",   0xB000, 0x1000, 0x75b0593e )
	ROM_LOAD( "mtl9a.bin",    	0xC000, 0x1000, 0x28dd20ff )
	ROM_LOAD( "mtl8a.bin",    	0xD000, 0x1000, 0xcc09f7a4 )
	ROM_LOAD( "mtl7a.bin",    	0xE000, 0x1000, 0xcaafbb6d )
	ROM_LOAD( "mtl6a.bin",    	0xF000, 0x1000, 0xd85e52ca )

	ROM_REGION_DISPOSE(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mtl11d.bin",   0x0000, 0x0800, 0xc6e4d339 )

	ROM_REGION(0x10000) /* 64k for audio */
	ROM_LOAD( "mta5a.bin",    0x6800, 0x0800, 0xdbe4ec02 )
	ROM_LOAD( "mta6a.bin",    0x7000, 0x0800, 0xc00f0c05 )
	ROM_LOAD( "mta7a.bin",    0x7800, 0x0800, 0xf3f16ca7 )
	ROM_RELOAD(            0xf800, 0x0800 )
ROM_END

ROM_START( venture_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "13a-cpu",      0x8000, 0x1000, 0xf4e4d991 )
	ROM_LOAD( "12a-cpu",      0x9000, 0x1000, 0xc6d8cb04 )
	ROM_LOAD( "11a-cpu",      0xA000, 0x1000, 0x3bdb01f4 )
	ROM_LOAD( "10a-cpu",      0xB000, 0x1000, 0x0da769e9 )
	ROM_LOAD( "9a-cpu",       0xC000, 0x1000, 0x0ae05855 )
	ROM_LOAD( "8a-cpu",       0xD000, 0x1000, 0x4ae59676 )
	ROM_LOAD( "7a-cpu",       0xE000, 0x1000, 0x48d66220 )
	ROM_LOAD( "6a-cpu",       0xF000, 0x1000, 0x7b78cf49 )

	ROM_REGION_DISPOSE(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "11d-cpu",      0x0000, 0x0800, 0xb4bb2503 )

	ROM_REGION(0x10000) /* 64k for audio */
	ROM_LOAD( "3a-ac",        0x5800, 0x0800, 0x4ea1c3d9 )
	ROM_LOAD( "4a-ac",        0x6000, 0x0800, 0x5154c39e )
	ROM_LOAD( "5a-ac",        0x6800, 0x0800, 0x1e1e3916 )
	ROM_LOAD( "6a-ac",        0x7000, 0x0800, 0x80f3357a )
	ROM_LOAD( "7a-ac",        0x7800, 0x0800, 0x466addc7 )
	ROM_RELOAD(        0xF800, 0x0800 )
ROM_END

ROM_START( venture2_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "vent_a13.cpu", 0x8000, 0x1000, 0x4c833f99 )
	ROM_LOAD( "vent_a12.cpu", 0x9000, 0x1000, 0x8163cefc )
	ROM_LOAD( "vent_a11.cpu", 0xA000, 0x1000, 0x324a5054 )
	ROM_LOAD( "vent_a10.cpu", 0xB000, 0x1000, 0x24358203 )
	ROM_LOAD( "vent_a9.cpu",  0xC000, 0x1000, 0x04428165 )
	ROM_LOAD( "vent_a8.cpu",  0xD000, 0x1000, 0x4c1a702a )
	ROM_LOAD( "vent_a7.cpu",  0xE000, 0x1000, 0x1aab27c2 )
	ROM_LOAD( "vent_a6.cpu",  0xF000, 0x1000, 0x767bdd71 )

	ROM_REGION_DISPOSE(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "11d-cpu",      0x0000, 0x0800, 0xb4bb2503 )

	ROM_REGION(0x10000) /* 64k for audio */
	ROM_LOAD( "3a-ac",        0x5800, 0x0800, 0x4ea1c3d9 )
	ROM_LOAD( "4a-ac",        0x6000, 0x0800, 0x5154c39e )
	ROM_LOAD( "5a-ac",        0x6800, 0x0800, 0x1e1e3916 )
	ROM_LOAD( "6a-ac",        0x7000, 0x0800, 0x80f3357a )
	ROM_LOAD( "7a-ac",        0x7800, 0x0800, 0x466addc7 )
	ROM_RELOAD(        0xF800, 0x0800 )
ROM_END

ROM_START( venture4_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "vel13a-4",     0x8000, 0x1000, 0x1c5448f9 )
	ROM_LOAD( "vel12a-4",     0x9000, 0x1000, 0xe62491cc )
	ROM_LOAD( "vel11a-4",     0xA000, 0x1000, 0xe91faeaf )
	ROM_LOAD( "vel10a-4",     0xB000, 0x1000, 0xda3a2991 )
	ROM_LOAD( "vel9a-4",      0xC000, 0x1000, 0xd1887b11 )
	ROM_LOAD( "vel8a-4",      0xD000, 0x1000, 0x8e8153fc )
	ROM_LOAD( "vel7a-4",      0xE000, 0x1000, 0x0a091701 )
	ROM_LOAD( "vel6a-4",      0xF000, 0x1000, 0x7b165f67 )

	ROM_REGION_DISPOSE(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "vel11d-2",     0x0000, 0x0800, 0xea6fd981 )

	ROM_REGION(0x10000) /* 64k for audio */
	ROM_LOAD( "vea3a-2",      0x5800, 0x0800, 0x83b8836f )
	ROM_LOAD( "4a-ac",        0x6000, 0x0800, 0x5154c39e )
	ROM_LOAD( "5a-ac",        0x6800, 0x0800, 0x1e1e3916 )
	ROM_LOAD( "6a-ac",        0x7000, 0x0800, 0x80f3357a )
	ROM_LOAD( "7a-ac",        0x7800, 0x0800, 0x466addc7 )
	ROM_RELOAD(          0xF800, 0x0800 )
ROM_END

ROM_START( pepper2_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "main_12a",     0x9000, 0x1000, 0x33db4737 )
	ROM_LOAD( "main_11a",     0xA000, 0x1000, 0xa1f43b1f )
	ROM_LOAD( "main_10a",     0xB000, 0x1000, 0x4d7d7786 )
	ROM_LOAD( "main_9a",      0xC000, 0x1000, 0xb3362298 )
	ROM_LOAD( "main_8a",      0xD000, 0x1000, 0x64d106ed )
	ROM_LOAD( "main_7a",      0xE000, 0x1000, 0xb1c6f07c )
	ROM_LOAD( "main_6a",      0xF000, 0x1000, 0x515b1046 )

	ROM_REGION_DISPOSE(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "main_11d",     0x0000, 0x0800, 0xb25160cd )

	ROM_REGION(0x10000) /* 64k for audio */
	ROM_LOAD( "audio_5a",     0x6800, 0x0800, 0x90e3c781 )
	ROM_LOAD( "audio_6a",     0x7000, 0x0800, 0xdd343e34 )
	ROM_LOAD( "audio_7a",     0x7800, 0x0800, 0xe02b4356 )
	ROM_RELOAD(           0xF800, 0x0800 )
ROM_END

ROM_START( hardhat_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "hhl-2.11a",    0xA000, 0x1000, 0x7623deea )
	ROM_LOAD( "hhl-2.10a",    0xB000, 0x1000, 0xe6bf2fb1 )
	ROM_LOAD( "hhl-2.9a",     0xC000, 0x1000, 0xacc2bce5 )
	ROM_LOAD( "hhl-2.8a",     0xD000, 0x1000, 0x23c7a2f8 )
	ROM_LOAD( "hhl-2.7a",     0xE000, 0x1000, 0x6f7ce1c2 )
	ROM_LOAD( "hhl-2.6a",     0xF000, 0x1000, 0x2a20cf10 )

	ROM_REGION_DISPOSE(0x0800) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "hhl-1.11d",    0x0000, 0x0800, 0xdbcdf353 )

	ROM_REGION(0x10000) /* 64k for audio */
	ROM_LOAD( "hha-1.5a",     0x6800, 0x0800, 0x16a5a183 )
	ROM_LOAD( "hha-1.6a",     0x7000, 0x0800, 0xbde64021 )
	ROM_LOAD( "hha-1.7a",     0x7800, 0x0800, 0x505ee5d3 )
	ROM_RELOAD(           0xF800, 0x0800 )
ROM_END

ROM_START( fax_rom )
	ROM_REGION(0x40000) /* 64k for code + 192k for extra memory */
	ROM_LOAD( "fxl8-13a.32",  0x8000, 0x1000, 0x8e30bf6b )
	ROM_LOAD( "fxl8-12a.32",  0x9000, 0x1000, 0x60a41ff1 )
	ROM_LOAD( "fxl8-11a.32",  0xA000, 0x1000, 0x2c9cee8a )
	ROM_LOAD( "fxl8-10a.32",  0xB000, 0x1000, 0x9b03938f )
	ROM_LOAD( "fxl8-9a.32",   0xC000, 0x1000, 0xfb869f62 )
	ROM_LOAD( "fxl8-8a.32",   0xD000, 0x1000, 0xdb3470bc )
	ROM_LOAD( "fxl8-7a.32",   0xE000, 0x1000, 0x1471fef5 )
	ROM_LOAD( "fxl8-6a.32",   0xF000, 0x1000, 0x812e39f3 )
	/* Banks of question ROMs */
	ROM_LOAD( "fxd-1c.64",    	0x10000, 0x2000, 0xfd7e3137 )
	ROM_LOAD( "fxd-2c.64",    	0x12000, 0x2000, 0xe78cb16f )
	ROM_LOAD( "fxd-3c.64",    	0x14000, 0x2000, 0x57a94c6f )
	ROM_LOAD( "fxd-4c.64",    	0x16000, 0x2000, 0x9036c5a2 )
	ROM_LOAD( "fxd-5c.64",    	0x18000, 0x2000, 0x38c03405 )
	ROM_LOAD( "fxd-6c.64",    	0x1A000, 0x2000, 0xf48fc308 )
	ROM_LOAD( "fxd-7c.64",    	0x1C000, 0x2000, 0xcf93b924 )
	ROM_LOAD( "fxd-8c.64",    	0x1E000, 0x2000, 0x607b48da )
	ROM_LOAD( "fxd-1b.64",    	0x20000, 0x2000, 0x62872d4f )
	ROM_LOAD( "fxd-2b.64",    	0x22000, 0x2000, 0x625778d0 )
	ROM_LOAD( "fxd-3b.64",    	0x24000, 0x2000, 0xc3473dee )
	ROM_LOAD( "fxd-4b.64",    	0x26000, 0x2000, 0xe39a15f5 )
	ROM_LOAD( "fxd-5b.64",    	0x28000, 0x2000, 0x101a9d70 )
	ROM_LOAD( "fxd-6b.64",    	0x2A000, 0x2000, 0x374a8f05 )
	ROM_LOAD( "fxd-7b.64",    	0x2C000, 0x2000, 0xf7e7f824 )
	ROM_LOAD( "fxd-8b.64",    	0x2E000, 0x2000, 0x8f1a5287 )
	ROM_LOAD( "fxd-1a.64",    	0x30000, 0x2000, 0xfc5e6344 )
	ROM_LOAD( "fxd-2a.64",    	0x32000, 0x2000, 0x43cf60b3 )
	ROM_LOAD( "fxd-3a.64",    	0x34000, 0x2000, 0x6b7d29cb )
	ROM_LOAD( "fxd-4a.64",    	0x36000, 0x2000, 0xb9de3c2d )
	ROM_LOAD( "fxd-5a.64",    	0x38000, 0x2000, 0x67285bc6 )
	ROM_LOAD( "fxd-6a.64",    	0x3A000, 0x2000, 0xba67b7b2 )
	/* The last two ROM sockets were apparently never populated */
//	ROM_LOAD( "fxd-7a.64",	0x3C000, 0x2000, 0x00000000 )
//	ROM_LOAD( "fxd-8a.64",	0x3E000, 0x2000, 0x00000000 )

	ROM_REGION_DISPOSE(0x0800)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "fxl1-11d.32",  0x0000, 0x0800, 0x54fc873d )
	ROM_CONTINUE(            0x0000, 0x0800 )	/* overwrite with the real one - should be a 2716? */

	ROM_REGION(0x10000) /* 64k for audio */
	/* 5a missing! */
	ROM_LOAD( "fxa2-6a.16",   0x7000, 0x0800, 0x2b3bfc44 )
	ROM_LOAD( "fxa2-7a.16",   0x7800, 0x0800, 0x578c62b7 )
	ROM_RELOAD(             0xF800, 0x0800 )
ROM_END



/***************************************************************************
  Hi Score Routines
***************************************************************************/

static int mtrap_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	static int firsttime;
		/* the high score table is intialized to all 0, so first of all */
		/* we dirty it, then we wait for it to be cleared again */
		if (firsttime == 0)
		{
			memset(&RAM[0x0380],0xff,5+6*5);	/* high score */
			firsttime = 1;
		}


	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x0380],"\x00\x06\x0C\x12\x18",5) == 0) &&
		(memcmp(&RAM[0x03A0],"LWH",3) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0380],5+6*5);
			osd_fclose(f);
		}
		firsttime = 0;
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void mtrap_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* 5 bytes for score order, 6 bytes per score/initials */
		osd_fwrite(f,&RAM[0x0380],5+6*5);
		osd_fclose(f);
	}

}

static int venture_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x0380],"\x00\x06\x0C\x12\x18",5) == 0) &&
		(memcmp(&RAM[0x03A0],"DJS",3) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0380],5+6*5);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void venture_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* 5 bytes for score order, 6 bytes per score/initials */
		osd_fwrite(f,&RAM[0x0380],5+6*5);
		osd_fclose(f);
	}

}

static int pepper2_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	static int firsttime;
		/* the high score table is intialized to all 0, so first of all */
		/* we dirty it, then we wait for it to be cleared again */
		if (firsttime == 0)
		{
			memset(&RAM[0x0360],0xff,5+6*5);	/* high score */
			firsttime = 1;
		}



	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x0365],"\x00\x07\x65",3) == 0) &&
		(memcmp(&RAM[0x0380],"\x15\x20\x11",3) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0360],5+6*5);
			osd_fclose(f);
		}
		firsttime = 0;
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void pepper2_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* 5 bytes for score order, 6 bytes per score/initials */
		osd_fwrite(f,&RAM[0x0360],5+6*5);
		osd_fclose(f);
	}

}

static int targ_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x00AE],"\x00\x10",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x00AE],2);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void targ_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x00AE],2);
		osd_fclose(f);
	}
}

static int fax_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	static int firsttime;
		/* the high score table is intialized to all 0, so first of all */
		/* we dirty it, then we wait for it to be cleared again */
		if (firsttime == 0)
		{
			memset(&RAM[0x02b4],0xff,7*50);	/* high score */
			firsttime = 1;
		}



	if (memcmp(&RAM[0x0360],"\x00\x00\x00",3) == 0 && memcmp(&RAM[0x040f],"\x00\x00\x00",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x02B4],7*50);
			osd_fclose(f);
		}
		firsttime =0;
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void fax_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* 7 characters per hi score, 50 hi scores */
		osd_fwrite(f,&RAM[0x02B4],7*50);
		osd_fclose(f);
	}
}

static int sidetrac_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	static int firsttime;
		/* the high score table is intialized to all 0, so first of all */
		/* we dirty it, then we wait for it to be cleared again */
		if (firsttime == 0)
		{
			memset(&RAM[0x000a],0xff,8);	/* high score */
			firsttime = 1;
		}



	/* Check for high score init. */
	if (memcmp(&RAM[0x000f],"\x00\x00",2) == 0 && memcmp(&RAM[0x000a],"\x00\x00",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x000f],2);
			osd_fclose(f);
		}
		firsttime = 0;
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void sidetrac_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x000f],2);
		osd_fclose(f);
	}
}


/***************************************************************************
Game Driver
***************************************************************************/

struct GameDriver sidetrac_driver =
{
	__FILE__,
	0,
	"sidetrac",
	"Side Track",
	"1979",
	"Exidy",
	"Marc LaFontaine\nBrian Levine\nMike Balfour",
	0,
	&targ_machine_driver,
	0,

	sidetrac_rom,
	0, 0,
	targ_sample_names,
	0,

	sidetrac_input_ports,

	0, sidetrac_palette, sidetrac_colortable,

	ORIENTATION_DEFAULT,

	sidetrac_hiload, sidetrac_hisave
};

struct GameDriver targ_driver =
{
	__FILE__,
	0,
	"targ",
	"Targ",
	"1980",
	"Exidy",
	"Neil Bradley (hardware info)\nDan Boris (adaptation of Venture driver)",
	0,
	&targ_machine_driver,
	0,

	targ_rom,
	0, 0,
	targ_sample_names,
	0,

	targ_input_ports,

	0, targ_palette, colortable,

	ORIENTATION_DEFAULT,

	targ_hiload,targ_hisave
};

struct GameDriver spectar_driver =
{
	__FILE__,
	0,
	"spectar",
	"Spectar (revision 3)",
	"1980",
	"Exidy",
	"Neil Bradley (hardware info)\nDan Boris (adaptation of Venture driver)",
	0,
	&targ_machine_driver,
	0,

	spectar_rom,
	0, 0,
	targ_sample_names,
	0,

	spectar_input_ports,

	0, spectar_palette, colortable,

	ORIENTATION_DEFAULT,

	targ_hiload,targ_hisave
};

struct GameDriver spectar1_driver =
{
	__FILE__,
	&spectar_driver,
	"spectar1",
	"Spectar (revision 1?)",
	"1980",
	"Exidy",
	"Neil Bradley (hardware info)\nDan Boris (adaptation of Venture driver)",
	0,
	&targ_machine_driver,
	0,

	spectar1_rom,
	0, 0,
	targ_sample_names,
	0,

	spectar_input_ports,

	0, spectar_palette, colortable,

	ORIENTATION_DEFAULT,

	targ_hiload,targ_hisave
};

struct GameDriver mtrap_driver =
{
	__FILE__,
	0,
	"mtrap",
	"Mouse Trap",
	"1981",
	"Exidy",
	"Marc LaFontaine\nBrian Levine\nMike Balfour\nMarco Cassili",
	0,
	&machine_driver,
	0,

	mtrap_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	mtrap_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	mtrap_hiload,mtrap_hisave
};

struct GameDriver venture_driver =
{
	__FILE__,
	0,
	"venture",
	"Venture (set 1)",
	"1981",
	"Exidy",
	"Marc LaFontaine\nNicola Salmoria\nBrian Levine\nMike Balfour\nBryan Smith (hardware info)",
	0,
	&venture_machine_driver,
	0,

	venture_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	venture_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	venture_hiload,venture_hisave
};

struct GameDriver venture2_driver =
{
	__FILE__,
	&venture_driver,
	"venture2",
	"Venture (set 2)",
	"1981",
	"Exidy",
	"Marc LaFontaine\nNicola Salmoria\nBrian Levine\nMike Balfour\nBryan Smith (hardware info)",
	0,
	&venture_machine_driver,
	0,

	venture2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	venture_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	venture_hiload,venture_hisave
};

struct GameDriver venture4_driver =
{
	__FILE__,
	&venture_driver,
	"venture4",
	"Venture (version 4)",
	"1981",
	"Exidy",
	"Marc LaFontaine\nNicola Salmoria\nBrian Levine\nMike Balfour\nBryan Smith (hardware info)",
	0,
	&venture_machine_driver,
	0,

	venture4_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	venture_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	venture_hiload,venture_hisave
};

struct GameDriver pepper2_driver =
{
	__FILE__,
	0,
	"pepper2",
	"Pepper II",
	"1982",
	"Exidy",
	"Marc LaFontaine\nBrian Levine\nMike Balfour",
	0,
	&pepper2_machine_driver,
	0,

	pepper2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	pepper2_input_ports,

	0, palette, pepper2_colortable,
	ORIENTATION_DEFAULT,

	pepper2_hiload,pepper2_hisave
};

struct GameDriver hardhat_driver =
{
	__FILE__,
	0,
	"hardhat",
	"Hard Hat",
	"1982",
	"Exidy",
	"Marc LaFontaine\nBrian Levine\nMike Balfour",
	0,
	&pepper2_machine_driver,
	0,

	hardhat_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	pepper2_input_ports,

	0, palette, pepper2_colortable,
	ORIENTATION_DEFAULT,

	pepper2_hiload,pepper2_hisave
};

struct GameDriver fax_driver =
{
	__FILE__,
	0,
	"fax",
	"Fax",
	"1983",
	"Exidy",
	"Marc LaFontaine\nBrian Levine\nMike Balfour",
	0,
	&fax_machine_driver,
	0,

	fax_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	fax_input_ports,

	0, palette, pepper2_colortable,
	ORIENTATION_DEFAULT,

	fax_hiload,fax_hisave
};
