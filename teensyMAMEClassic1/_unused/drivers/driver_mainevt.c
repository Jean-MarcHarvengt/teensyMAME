/***************************************************************************

  The Main Event, (c) 1988 Konami
  Devastators, (c) 1988 Konami

Emulation by Bryan McPhail, mish@tendril.force9.net

Notes:
* Devastators not playable...  Protected?  Unimplemented opcodes?
* Sprite priorities not yet fully correct
* No sound
* Graphics rom test fails - It looks like graphics roms can be read through
  the sprite area - with bank switching at 0x3802/3 (little endian).
  Turned on by setting 0x20 in 0x3800.
  Setting 0x08 in 0x3800 is monitor flip.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80/Z80.h"
#include "M6809/m6809.h"

void mainevt_vh_screenrefresh (struct osd_bitmap *bitmap,int full_refresh);
int mainevt_vh_start (void);
void mainevt_vh_stop (void);
void mainevt_control_w (int offset, int data);
void mainevt_video_control (int offset, int data);

void mainevt_bg_video_w (int offset, int data);
void mainevt_fg_video_w (int offset, int data);
void mainevt_video_w (int offset, int data);

void mainevt_bg_attr_w (int offset, int data);
void mainevt_fg_attr_w (int offset, int data);
void mainevt_attr_w (int offset, int data);

extern unsigned char *mainevt_bg_attr_ram;
extern unsigned char *mainevt_fg_attr_ram;
extern unsigned char *mainevt_attr_ram;

extern unsigned char *bg_videoram;
extern unsigned char *fg_videoram;

extern unsigned char *bg_scrollx_lo;
extern unsigned char *bg_scrollx_hi;
extern unsigned char *bg_scrolly;
extern unsigned char *fg_scrollx_lo;
extern unsigned char *fg_scrollx_hi;
extern unsigned char *fg_scrolly;

static int mainevt_int_type;

/* Unsure about this....  Main Event always uses IRQ, Devastators NMI */
static void mainevt_int_w(int offset, int data)
{
	/* Main Event works with int's enabled from start but Devastors crashes
  	if you try this...  So this turns them on just after self test */
	switch (data) {
		case 0:
			mainevt_int_type=0;
			break;
		case 2:
			mainevt_int_type=M6809_INT_IRQ;
			break;
		case 3: /* Main Event Test mode sets 3.. */
			mainevt_int_type=M6809_INT_IRQ;
			break;
		case 255:
			mainevt_int_type=M6809_INT_NMI;
			break;
	}
	if (errorlog) fprintf(errorlog,"Int Control %02x\n",data);
}

static int mainevt_interrupt(void)
{
	return mainevt_int_type;
}

static int zero_ret(int offset)
{
	return 0xff;
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x1bff, MRA_RAM },
	{ 0x1e00, 0x1e00, zero_ret }, /* ? */
	{ 0x1f94, 0x1f94, input_port_0_r }, /* Coins */
	{ 0x1f95, 0x1f95, input_port_1_r }, /* Player 1 */
	{ 0x1f96, 0x1f96, input_port_2_r }, /* Player 2 */
	{ 0x1f97, 0x1f97, input_port_5_r }, /* Dip 1 */
	{ 0x1f98, 0x1f98, input_port_7_r }, /* Dip 3 */
	{ 0x1f99, 0x1f99, input_port_3_r }, /* Player 3 */
	{ 0x1f9a, 0x1f9a, input_port_4_r }, /* Player 4 */
	{ 0x1f9b, 0x1f9b, input_port_6_r }, /* Dip 2 */
	{ 0x2000, 0x3bff, MRA_RAM },
	{ 0x3c00, 0x3fff, MRA_RAM },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x07ff, mainevt_attr_w, &mainevt_attr_ram },
	{ 0x0800, 0x0fff, mainevt_bg_attr_w, &mainevt_bg_attr_ram },
	{ 0x1000, 0x17ff, mainevt_fg_attr_w, &mainevt_fg_attr_ram },
//	{ 0x0000, 0x1bff, mainevt_attr_w, &mainevt_attr_ram },

	{ 0x180c, 0x180c, MWA_RAM, &bg_scrolly },
	{ 0x1a00, 0x1a00, MWA_RAM, &bg_scrollx_lo },
	{ 0x1a01, 0x1a01, MWA_RAM, &bg_scrollx_hi },
//	{ 0x1800, 0x1bff, MWA_RAM },

	{ 0x1d00, 0x1d00, MWA_NOP },  //??
	{ 0x1e80, 0x1e80, mainevt_int_w },
	{ 0x1f80, 0x1f90, mainevt_control_w },

	{ 0x2000, 0x27ff, mainevt_video_w, &videoram, &videoram_size },
	{ 0x2800, 0x2fff, mainevt_bg_video_w, &bg_videoram },
	{ 0x3000, 0x37ff, mainevt_fg_video_w, &fg_videoram },
//	{ 0x2000, 0x3bff, mainevt_video_w, &videoram, &videoram_size },

	{ 0x380c, 0x380c, MWA_RAM, &fg_scrolly },
	{ 0x3a00, 0x3a00, MWA_RAM, &fg_scrollx_lo },
	{ 0x3a01, 0x3a01, MWA_RAM, &fg_scrollx_hi },
//	{ 0x3800, 0x3bff, MWA_RAM },

	{ 0x3c00, 0x3fff, MWA_RAM, &spriteram },
	{ 0x4000, 0x5dff, MWA_RAM },
	{ 0x5e00, 0x5fff, paletteram_xBBBBBGGGGGRRRRR_swap_w, &paletteram },
 	{ 0x6000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress dv_readmem[] =
{
	{ 0x0000, 0x1bff, MRA_RAM },

	{ 0x1e00, 0x1e00, zero_ret },
	{ 0x1f94, 0x1f94, input_port_0_r }, /* Coins */
	{ 0x1f95, 0x1f95, input_port_1_r }, /* Player 1 */
	{ 0x1f96, 0x1f96, input_port_2_r }, /* Player 2 */
	{ 0x1f97, 0x1f97, input_port_5_r }, /* Dip 1 */
	{ 0x1f98, 0x1f98, input_port_7_r }, /* Dip 3 */
	{ 0x1f9b, 0x1f9b, input_port_6_r }, /* Dip 2 */

	{ 0x1fa6, 0x1fa6, zero_ret },
//	{ 0x1fa1, 0x1fa1, zero_ret },
//	{ 0x1fa3, 0x1fa3, zero_ret },
 //	{ 0x1fa7, 0x1fa7, zero_ret },

	{ 0x1e80, 0x1e80, zero_ret },  /* Needs to return 0xff */

	{ 0x2000, 0x3bff, MRA_RAM },
	{ 0x3c00, 0x3fff, MRA_RAM },
	{ 0x4000, 0x5fff, MRA_RAM },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress dv_writemem[] =
{
	{ 0x0000, 0x07ff, mainevt_attr_w, &mainevt_attr_ram },
	{ 0x0800, 0x0fff, mainevt_bg_attr_w, &mainevt_bg_attr_ram },
	{ 0x1000, 0x17ff, mainevt_fg_attr_w, &mainevt_fg_attr_ram },
//	{ 0x0000, 0x1bff, mainevt_attr_w, &mainevt_attr_ram },
	{ 0x180c, 0x180c, MWA_RAM, &bg_scrolly },
	{ 0x1a00, 0x1a00, MWA_RAM, &bg_scrollx_lo },
	{ 0x1a01, 0x1a01, MWA_RAM, &bg_scrollx_hi },
//	{ 0x1800, 0x1bff, MWA_RAM },

//  { 0x1d00, 0x1d00, MWA_NOP },  //??
	{ 0x1e80, 0x1e80, mainevt_int_w },
	{ 0x1f80, 0x1f90, mainevt_control_w },
	{ 0x1fb2, 0x1fb2, MWA_NOP },

	{ 0x2000, 0x27ff, mainevt_video_w, &videoram, &videoram_size },
	{ 0x2800, 0x2fff, mainevt_bg_video_w, &bg_videoram },
	{ 0x3000, 0x37ff, mainevt_fg_video_w, &fg_videoram },
//	{ 0x2000, 0x3bff, mainevt_video_w, &videoram, &videoram_size },

	{ 0x380c, 0x380c, MWA_RAM, &fg_scrolly },
	{ 0x3a00, 0x3a00, MWA_RAM, &fg_scrollx_lo },
	{ 0x3a01, 0x3a01, MWA_RAM, &fg_scrollx_hi },
//	{ 0x3800, 0x3bff, MWA_RAM },

	{ 0x3c00, 0x3fff, MWA_RAM, &spriteram },
	{ 0x4000, 0x5dff, MWA_RAM },
	{ 0x5e00, 0x5fff, paletteram_xBBBBBGGGGGRRRRR_swap_w, &paletteram },
	{ 0x6000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

#if 0
static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0xa000, 0xa000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ -1 }	/* end of table */
};
#endif

/*****************************************************************************/

INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) /* Service 1 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* Service 2 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) /* Service 3 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) /* Service 4 */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, "Coins", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x01, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

 	PORT_START
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x18, 0x18, "Bonus Energy", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "90" )
	PORT_DIPSETTING(    0x10, "80" )
	PORT_DIPSETTING(    0x08, "70" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Video Flip", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0xF8, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( dv_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE ) /* Service 1 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE ) /* Service 2 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) /* Service 3 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) /* Service 4 */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, "Coins", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x01, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

 	PORT_START
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x18, 0x18, "Bonus Energy", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "90" )
	PORT_DIPSETTING(    0x10, "80" )
	PORT_DIPSETTING(    0x08, "70" )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Video Flip", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0xF8, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/*****************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,
	8192,
	4,
	{ 3*8192*8*8,2*8192*8*8,8192*8*8,0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16, /* 16*16 sprites */
	8192,
	4,
	{ 0,8,0x80000*8,(0x80000*8)+8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*16+0, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	8*64	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,   0,    12 },
	{ 1, 0x40000, &spritelayout, 16*12, 4 },
	{ -1 } /* end of array */
};


/*****************************************************************************/

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
 		{
			CPU_M6809,
			1500000,
			0,
			readmem,writemem,0,0,
			mainevt_interrupt,1
		}/*,
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,
			2,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1
		} */
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 14*8, 50*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	mainevt_vh_start,
	mainevt_vh_stop,
	mainevt_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};

static struct MachineDriver dv_machine_driver =
{
	/* basic machine hardware */
	{
 		{
			CPU_M6309,
			1000000,
			0,
			dv_readmem,dv_writemem,0,0,
			mainevt_interrupt,1
		}/*,
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,
			2,
			sound_readmem,sound_writemem,0,0,
			interrupt,4
		}  */
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 14*8, 50*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	mainevt_vh_start,
	mainevt_vh_stop,
	mainevt_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mainevt_rom )
	ROM_REGION(0x40000)
	ROM_LOAD( "799c02.k11",   0x10000, 0x08000, 0xe2e7dbd5 )
	ROM_CONTINUE(0x8000,0x8000)

	ROM_REGION_DISPOSE(0x140000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "799c06.f22",   0x00000, 0x08000, 0xf839cb58 )
	ROM_RELOAD(               0x08000, 0x08000 )
	ROM_LOAD( "799c07.h22",   0x10000, 0x08000, 0x176df538 )
	ROM_RELOAD(               0x18000, 0x08000 )
	ROM_LOAD( "799c08.j22",   0x20000, 0x08000, 0xd01e0078 )
	ROM_RELOAD(               0x28000, 0x08000 )
	ROM_LOAD( "799c09.k22",   0x30000, 0x08000, 0x9baec75e )
	ROM_RELOAD(               0x38000, 0x08000 )

	ROM_LOAD( "799b04.h4",    0x40000, 0x80000, 0x323e0c2b )
	ROM_LOAD( "799b05.k4",    0xc0000, 0x80000, 0x571c5831 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "799c01.f7",    0x00000, 0x08000, 0x447c4c5c )

	ROM_REGION(0x80000) /* 2 sample roms? */
//  ROM_LOAD( "799b03.d4", 0x00000, 0x80000, 0xbef2b882 )
// ROM_LOAD( "799b06.c22",0x80000, 0x80000, 0xbef2b882 )
ROM_END

ROM_START( devstors_rom )
	ROM_REGION(0x40000)
	ROM_LOAD( "dev-x02.rom",  0x10000, 0x08000, 0xe58ebb35 )
	ROM_CONTINUE(0x8000,0x8000)

	ROM_REGION_DISPOSE(0x140000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "dev-f06.rom",  0x00000, 0x10000, 0x26592155 )
	ROM_LOAD( "dev-f07.rom",  0x10000, 0x10000, 0x6c74fa2e )
	ROM_LOAD( "dev-f08.rom",  0x20000, 0x10000, 0x29e12e80 )
	ROM_LOAD( "dev-f09.rom",  0x30000, 0x10000, 0x67ca40d5 )

	ROM_LOAD( "dev-f04.rom",  0x40000, 0x80000, 0xf16cd1fa )
	ROM_LOAD( "dev-f05.rom",  0xc0000, 0x80000, 0xda37db05 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "dev-k01.rom",  0x00000, 0x08000, 0xd44b3eb0 )

	ROM_REGION(0x80000)
 	ROM_LOAD( "dev-f03.rom",  0x00000, 0x80000, 0x19065031 )
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if  (memcmp(&RAM[0x415C],"\xFF\xAE\xCA",3) == 0 &&
			memcmp(&RAM[0x419F],"\x00\x05\x77",3) == 0 )
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x415C],70);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x415C],70);
		osd_fclose(f);
	}
}



struct GameDriver mainevt_driver =
{
	__FILE__,
	0,
	"mainevt",
	"The Main Event",
	"1988",
	"Konami",
	"Bryan McPhail\n",
	0,
	&machine_driver,
	0,

	mainevt_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver devstors_driver =
{
	__FILE__,
	0,
 	"devstors",
	"Devastators",
	"1988",
	"Konami",
	"Bryan McPhail\n",
	GAME_NOT_WORKING,
	&dv_machine_driver,
	0,

	devstors_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	dv_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	0,0
};

