/****************************************************************************

   Bally Astrocade style games

   02.02.98 - New IO port definitions				MJC
              Dirty Rectangle handling
              Sparkle Circuit for Gorf
              errorlog output conditional on MAME_DEBUG

   03/04 98 - Extra Bases driver 				ATJ
	      Wow word driver

   09/20/98 - Added Emulated Sound Support		FMP

 ****************************************************************************
 * To make it easier to differentiate between each machine's settings
 * I have split it into a separate section for each driver.
 *
 * Default settings are declared first, and these can be used by any
 * driver that does not need to customise them differently
 *
 * Memory Maps are at the end (for WOW and Seawolf II)
 ****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/astrocde.h"

extern unsigned char *wow_videoram;

int  wow_intercept_r(int offset);
void wow_videoram_w(int offset,int data);
void wow_magic_expand_color_w(int offset,int data);
void wow_magic_control_w(int offset,int data);
void wow_magicram_w(int offset,int data);
void wow_pattern_board_w(int offset,int data);
void wow_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void wow_vh_screenrefresh_stars(struct osd_bitmap *bitmap,int full_refresh);
int  wow_video_retrace_r(int offset);

void wow_interrupt_enable_w(int offset, int data);
void wow_interrupt_w(int offset, int data);
int  wow_interrupt(void);

int  seawolf2_controller1_r(int offset);
int  seawolf2_controller2_r(int offset);
void seawolf2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void gorf_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int  gorf_vh_start(void);
int  gorf_interrupt(void);
int  gorf_timer_r(int offset);
int  Gorf_IO_r(int offset);

int  wow_vh_start(void);

int  wow_sh_start(void);
void wow_sh_stop(void);
void wow_sh_update(void);

int  wow_speech_r(int offset);
int  wow_port_2_r(int offset);

int  gorf_sh_start(void);
void gorf_sh_stop(void);
void gorf_sh_update(void);
int  gorf_speech_r(int offset);
int  gorf_port_2_r(int offset);
void gorf_sound_control_a_w(int offset,int data);

void colour_register_w(int offset, int data);
void colour_split_w(int offset, int data);

/*
 * Default Settings
 */

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xcfff, MRA_ROM },
	{ 0xd000, 0xdfff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, wow_magicram_w },
	{ 0x4000, 0x7fff, wow_videoram_w, &wow_videoram, &videoram_size },	/* ASG */
	{ 0x8000, 0xcfff, MWA_ROM },
	{ 0xd000, 0xdfff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x08, 0x08, wow_intercept_r },
	{ 0x0e, 0x0e, wow_video_retrace_r },
	{ 0x10, 0x10, input_port_0_r },
	{ 0x11, 0x11, input_port_1_r },
  	{ 0x12, 0x12, wow_port_2_r },
	{ 0x13, 0x13, input_port_3_r },
	{ 0x17, 0x17, wow_speech_r },				/* Actually a Write! */
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x07, colour_register_w },
	{ 0x08, 0x08, MWA_NOP }, /* Gorf uses this */
	{ 0x09, 0x09, colour_split_w },
	{ 0x0a, 0x0b, MWA_NOP }, /* Gorf uses this */
	{ 0x0c, 0x0c, wow_magic_control_w },
	{ 0x0d, 0x0d, interrupt_vector_w },
	{ 0x0e, 0x0e, wow_interrupt_enable_w },
	{ 0x0f, 0x0f, wow_interrupt_w },
	{ 0x10, 0x18, astrocade_sound1_w },
	{ 0x19, 0x19, wow_magic_expand_color_w },
	{ 0x50, 0x58, astrocade_sound2_w },
	{ 0x5b, 0x5b, MWA_NOP }, /* speech board ? Wow always sets this to a5*/
	{ 0x78, 0x7e, wow_pattern_board_w },
/*	{ 0xf8, 0xff, MWA_NOP }, */ /* Gorf uses these */
	{ -1 }	/* end of table */
};

static unsigned char palette[] =
{
     0x00,0x00,0x00,     /* 0 */
     0x08,0x08,0x08,	/* Gorf back */
     0x10,0x10,0x10,	/* Gorf back */
     0x18,0x18,0x18,	/* Gorf back */
     0x20,0x20,0x20,	/* Gorf back & Stars */
     0x50,0x50,0x50,    /* Gorf Stars */
     0x80,0x80,0x80,    /* Gorf Stars */
     0xA0,0xA0,0xA0,    /* Gorf Stars */
     0x00,0x00,0x28,
     0x00,0x00,0x59,
     0x00,0x00,0x8A,
     0x00,0x00,0xBB,
     0x00,0x00,0xEC,
     0x00,0x00,0xFF,
     0x00,0x00,0xFF,
     0x00,0x00,0xFF,
     0x28,0x00,0x28,     /* 10 */
     0x2F,0x00,0x52,
     0x36,0x00,0x7C,
     0x3D,0x00,0xA6,
     0x44,0x00,0xD0,
     0x4B,0x00,0xFA,
     0x52,0x00,0xFF,
     0x59,0x00,0xFF,
     0x28,0x00,0x28,
     0x36,0x00,0x4B,
     0x44,0x00,0x6E,
     0x52,0x00,0x91,
     0x60,0x00,0xB4,
     0x6E,0x00,0xD7,
     0x7C,0x00,0xFA,
     0x8A,0x00,0xFF,
     0x28,0x00,0x28,     /* 20 */
     0x3D,0x00,0x44,
     0x52,0x00,0x60,
     0x67,0x00,0x7C,
     0x7C,0x00,0x98,
     0x91,0x00,0xB4,
     0xA6,0x00,0xD0,
     0xBB,0x00,0xEC,
     0x28,0x00,0x28,
     0x4B,0x00,0x3D,
     0x6E,0x00,0x52,
     0x91,0x00,0x67,
     0xB4,0x00,0x7C,
     0xD7,0x00,0x91,
     0xFA,0x00,0xA6,
     0xFF,0x00,0xBB,
     0x28,0x00,0x28,     /* 30 */
     0x4B,0x00,0x36,
     0x6E,0x00,0x44,
     0x91,0x00,0x52,
     0xB4,0x00,0x60,
     0xD7,0x00,0x6E,
     0xFA,0x00,0x7C,
     0xFF,0x00,0x8A,
     0x28,0x00,0x28,
     0x52,0x00,0x2F,
     0x7C,0x00,0x36,
     0xA6,0x00,0x3D,
     0xD0,0x00,0x44,
     0xFA,0x00,0x4B,
     0xFF,0x00,0x52,
     0xFF,0x00,0x59,
     0x28,0x00,0x00,     /* 40 */
     0x59,0x00,0x00,
     0x8A,0x00,0x00,
     0xBB,0x00,0x00,
     0xEC,0x00,0x00,
     0xFF,0x00,0x00,
     0xFF,0x00,0x00,
     0xFF,0x00,0x00,
     0x28,0x00,0x00,
     0x59,0x00,0x00,
     0xE0,0x00,0x00,	/* Gorf (8a,00,00) */
     0xBB,0x00,0x00,
     0xEC,0x00,0x00,
     0xFF,0x00,0x00,
     0xFF,0x00,0x00,
     0xFF,0x00,0x00,
     0x28,0x00,0x00,     /* 50 */
     0x80,0x00,0x00,	/* WOW (59,00,00) */
     0xC0,0x00,0x00,	/* WOW (8a,00,00) */
     0xF0,0x00,0x00,	/* WOW (bb,00,00) */
     0xEC,0x00,0x00,
     0xFF,0x00,0x00,
     0xFF,0x00,0x00,
     0xFF,0x00,0x00,
     0xC0,0x00,0x00,	/* Seawolf 2 */
     0x59,0x00,0x00,
     0x8A,0x00,0x00,
     0xBB,0x00,0x00,
     0xEC,0x00,0x00,
     0xFF,0x00,0x00,
     0xFF,0x00,0x00,
     0xFF,0x00,0x00,
     0x28,0x28,0x00,     /* 60 */
     0x59,0x2F,0x00,
     0x8A,0x36,0x00,
     0xBB,0x3D,0x00,
     0xEC,0x44,0x00,
     0xFF,0xC0,0x00,	/* Gorf (ff,4b,00) */
     0xFF,0x52,0x00,
     0xFF,0x59,0x00,
     0x28,0x28,0x00,
     0x59,0x3D,0x00,
     0x8A,0x52,0x00,
     0xBB,0x67,0x00,
     0xEC,0x7C,0x00,
     0xFF,0x91,0x00,
     0xFF,0xA6,0x00,
     0xFF,0xBB,0x00,
     0x28,0x28,0x00,     /* 70 */
     0x59,0x3D,0x00,
     0x8A,0x52,0x00,
     0xBB,0x67,0x00,
     0xEC,0x7C,0x00,
     0xFF,0xE0,0x00,	/* Gorf (ff,91,00) */
     0xFF,0xA6,0x00,
     0xFF,0xBB,0x00,
     0x28,0x28,0x00,
     0x52,0x44,0x00,
     0x7C,0x60,0x00,
     0xA6,0x7C,0x00,
     0xD0,0x98,0x00,
     0xFA,0xB4,0x00,
     0xFF,0xD0,0x00,
     0xFF,0xEC,0x00,
     0x28,0x28,0x00,     /* 80 */
     0x4B,0x52,0x00,
     0x6E,0x7C,0x00,
     0x91,0xA6,0x00,
     0xB4,0xD0,0x00,
     0xD7,0xFA,0x00,
     0xFA,0xFF,0x00,
     0xFF,0xFF,0x00,
     0x28,0x28,0x00,
     0x4B,0x59,0x00,
     0x6E,0x8A,0x00,
     0x91,0xBB,0x00,
     0xB4,0xEC,0x00,
     0xD7,0xFF,0x00,
     0xFA,0xFF,0x00,
     0xFF,0xFF,0x00,
     0x28,0x28,0x00,     /* 90 */
     0x4B,0x59,0x00,
     0x6E,0x8A,0x00,
     0x91,0xBB,0x00,
     0xB4,0xEC,0x00,
     0xD7,0xFF,0x00,
     0xFA,0xFF,0x00,
     0xFF,0xFF,0x00,
     0x28,0x28,0x00,
     0x52,0x59,0x00,
     0x7C,0x8A,0x00,
     0xA6,0xBB,0x00,
     0xD0,0xEC,0x00,
     0xFA,0xFF,0x00,
     0xFF,0xFF,0x00,
     0xFF,0xFF,0x00,
     0x28,0x28,0x00,     /* A0 */
     0x4B,0x59,0x00,
     0x6E,0x8A,0x00,
     0x91,0xBB,0x00,
     0x00,0x00,0xFF,	/* Gorf (b4,ec,00) */
     0xD7,0xFF,0x00,
     0xFA,0xFF,0x00,
     0xFF,0xFF,0x00,
     0x00,0x28,0x28,
     0x00,0x59,0x2F,
     0x00,0x8A,0x36,
     0x00,0xBB,0x3D,
     0x00,0xEC,0x44,
     0x00,0xFF,0x4B,
     0x00,0xFF,0x52,
     0x00,0xFF,0x59,
     0x00,0x28,0x28,     /* B0 */
     0x00,0x59,0x36,
     0x00,0x8A,0x44,
     0x00,0xBB,0x52,
     0x00,0xEC,0x60,
     0x00,0xFF,0x6E,
     0x00,0xFF,0x7C,
     0x00,0xFF,0x8A,
     0x00,0x28,0x28,
     0x00,0x52,0x3D,
     0x00,0x7C,0x52,
     0x00,0xA6,0x67,
     0x00,0xD0,0x7C,
     0x00,0xFA,0x91,
     0x00,0xFF,0xA6,
     0x00,0xFF,0xBB,
     0x00,0x28,0x28,     /* C0 */
     0x00,0x4B,0x44,
     0x00,0x6E,0x60,
     0x00,0x91,0x7C,
     0x00,0xB4,0x98,
     0x00,0xD7,0xB4,
     0x00,0xFA,0xD0,
     0x00,0x00,0x00,	/* WOW Background */
     0x00,0x28,0x28,
     0x00,0x4B,0x4B,
     0x00,0x6E,0x6E,
     0x00,0x91,0x91,
     0x00,0xB4,0xB4,
     0x00,0xD7,0xD7,
     0x00,0xFA,0xFA,
     0x00,0xFF,0xFF,
     0x00,0x28,0x28,     /* D0 */
     0x00,0x4B,0x52,
     0x00,0x6E,0x7C,
     0x00,0x91,0xA6,
     0x00,0xB4,0xD0,
     0x00,0xD7,0xFA,
     0x00,0xFA,0xFF,
     0x00,0xFF,0xFF,
     0x00,0x00,0x30,	/* Gorf (00,00,28)   also   */
     0x00,0x00,0x48,    /* Gorf (00,00,59)   used   */
     0x00,0x00,0x60,	/* Gorf (00,00,8a)    by    */
     0x00,0x00,0x78,	/* Gorf (00,00,bb) seawolf2 */
     0x00,0x00,0x90, 	/* seawolf2 */
     0x00,0x00,0xFF,
     0x00,0x00,0xFF,
     0x00,0x00,0xFF,
     0x00,0x00,0x28,     /* E0 */
     0x00,0x00,0x52,
     0x00,0x00,0x7C,
     0x00,0x00,0xA6,
     0x00,0x00,0xD0,
     0x00,0x00,0xFA,
     0x00,0x00,0xFF,
     0x00,0x00,0xFF,
     0x00,0x00,0x28,
     0x00,0x00,0x4B,
     0x00,0x00,0x6E,
     0x00,0x00,0x91,
     0x00,0x00,0xB4,
     0x00,0x00,0xD7,
     0x00,0x00,0xFA,
     0x00,0x00,0xFF,
     0x00,0x00,0x28,     /* F0 */
     0x00,0x00,0x44,
     0x00,0x00,0x60,
     0x00,0x00,0x7C,
     0x00,0x00,0x98,
     0x00,0x00,0xB4,
     0x00,0x00,0xD0,
     0x00,0x00,0xEC,
     0x00,0x00,0x28,
     0x00,0x00,0x3D,
     0x00,0x00,0x52,
     0x00,0x00,0x67,
     0x00,0x00,0x7C,
     0x00,0x00,0x91,
     0x00,0x00,0xA6,
     0x00,0x00,0xBB,
};

enum { BLACK,YELLOW,BLUE,RED,WHITE };

static unsigned short colortable[] =
{
	BLACK,YELLOW,BLUE,RED,
	BLACK,WHITE,BLACK,RED	/* not used by the game, here only for the dip switch menu */
};


/****************************************************************************
 * Wizard of Wor
 ****************************************************************************/

ROM_START( wow_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "wow.x1", 0x0000, 0x1000, 0xc1295786 )
	ROM_LOAD( "wow.x2", 0x1000, 0x1000, 0x9be93215 )
	ROM_LOAD( "wow.x3", 0x2000, 0x1000, 0x75e5a22e )
	ROM_LOAD( "wow.x4", 0x3000, 0x1000, 0xef28eb84 )
	ROM_LOAD( "wow.x5", 0x8000, 0x1000, 0x16912c2b )
	ROM_LOAD( "wow.x6", 0x9000, 0x1000, 0x35797f82 )
	ROM_LOAD( "wow.x7", 0xa000, 0x1000, 0xce404305 )
/*	ROM_LOAD( "wow.x8", 0xc000, 0x1000, ? )	here would go the foreign language ROM */
ROM_END

static const char *wow_sample_names[] =
{
	"*wow",
	"a.sam", "again.sam", "ahh.sam", "am.sam", "and.sam",
	"anew.sam", "another.sam", "any.sam", "anyone.sam", "appear.sam", "are.sam", "are.sam", "babies.sam", "back.sam",
	"beat.sam", "become.sam", "best.sam", "better.sam", "bite.sam", "bones.sam", "breath.sam", "but.sam", "can.sam", "cant.sam",
	"chance.sam", "chest.sam", "coin.sam", "dance.sam", "destroy.sam",
	"develop.sam", "do.sam", "dont.sam", "doom.sam", "door.sam", "draw.sam", "dungeon.sam", "dungeons.sam",
	"each.sam", "eaten.sam", "escape.sam", "explode.sam", "fear.sam", "find.sam", "find.sam", "fire.sam", "for.sam", "from.sam",
	"garwor.sam", "get.sam", "get.sam", "get.sam", "getting.sam", "good.sam", "hahahaha.sam", "harder.sam",
	"hasnt.sam", "have.sam", "heavyw.sam", "hey.sam", "hope.sam",
	"hungry.sam", "hungry.sam", "hurry.sam", "i.sam", "i.sam", "if.sam", "if.sam", "im.sam", "i1.sam", "ill.sam", "in.sam",
	"insert.sam", "invisibl.sam", "it.sam", "lie.sam", "magic.sam",
	"magical.sam", "me.sam", "meet.sam", "months.sam",
	"my.sam", "my.sam", "my.sam", "my.sam", "my.sam", "myself.sam", "near.sam", "never.sam",
	"now.sam", "of.sam", "off.sam", "one.sam", "only.sam", "oven.sam", "pause.sam", "pets.sam", "powerful.sam", "pop.sam",
	"radar.sam", "ready.sam",
	"rest.sam", "say.sam", "science.sam", "see.sam", "spause.sam", "start.sam", "the.sam", "the.sam", "the.sam", "the.sam", "then.sam",
	"through.sam", "thurwor.sam", "time.sam", "to.sam", "to.sam", "to.sam", "treasure.sam", "try.sam", "very.sam", "wait.sam",
	"war.sam", "warrior.sam", "watch.sam", "we.sam", "welcome.sam",
	"were.sam", "while.sam", "will.sam", "with.sam", "wizard.sam", "wont.sam",
	"wor.sam", "world.sam", "worlings.sam", "worlock.sam",
	"you.sam", "you.sam", "you.sam", "you.sam", "you.sam", "youl.sam", "youl.sam", "youd.sam", "your.sam",0
};

INPUT_PORTS_START( wow_input_ports )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* speech status */

	PORT_START /* Dip Switch */
	PORT_DIPNAME( 0x01, 0x01, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x06, 0x06, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/5 Credits" )
	PORT_DIPNAME( 0x08, 0x08, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "English" )
	PORT_DIPSETTING(    0x00, "Foreign (NEED ROM)" )
	PORT_DIPNAME( 0x10, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "2 / 5" )
	PORT_DIPSETTING(    0x00, "3 / 7" )
	PORT_DIPNAME( 0x20, 0x20, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "3rd Level" )
	PORT_DIPSETTING(    0x00, "4th Level" )
	PORT_DIPNAME( 0x40, 0x40, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END

static struct Samplesinterface wow_samples_interface =
{
	8	/* 8 channels */
};

static struct astrocade_interface astrocade_2chip_interface =
{
	2,			/* Number of chips */
	1789773,	/* Clock speed */
	255			/* Volume */
};

static struct astrocade_interface astrocade_1chip_interface =
{
	1,			/* Number of chips */
	1789773,	/* Clock speed */
	255			/* Volume */
};

static struct MachineDriver wow_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1789773,	/* 1.789 Mhz */
			0,
			readmem,writemem,readport,writeport,
			wow_interrupt,256
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	320, 204, { 0, 320-1, 0, 204-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	wow_vh_start,
	generic_vh_stop,
	wow_vh_screenrefresh_stars,

	/* sound hardware */
	0,             				/* Initialise audio hardware */
	wow_sh_start,   			/* Start audio  */
	wow_sh_stop,     			/* Stop audio   */
	wow_sh_update,              /* Update audio */
	{
		{
			SOUND_SAMPLES,
			&wow_samples_interface
		},
		{
			SOUND_ASTROCADE,
			&astrocade_2chip_interface
		}
 	}
};

static int wow_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xD004],"\x00\x00",2) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
	    osd_fread(f,&RAM[0xD004],20);
			/* stored twice in memory??? */
			memcpy(&RAM[0xD304],&RAM[0xD004],20);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void wow_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
	osd_fwrite(f,&RAM[0xD004],20);
		osd_fclose(f);
	}

}

struct GameDriver wow_driver =
{
	__FILE__,
	0,
	"wow",
	"Wizard of Wor",
	"1980",
	"Midway",
	"Nicola Salmoria (MAME driver)\nSteve Scavone (info and code)\nJim Hernandez (hardware info)\nMike Coates (additional code)\nKevin Estep (samples)\nAlex Judd (sound programming)\nFrank Palazzolo",
	0,
	&wow_machine_driver,
	0,

	wow_rom,
	0, 0,
	wow_sample_names,
	0,	/* sound_prom */

	wow_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	wow_hiload, wow_hisave,
};

/****************************************************************************
 * Robby Roto
 ****************************************************************************/

ROM_START( robby_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "rotox1.bin",  0x0000, 0x1000, 0xa431b85a )
	ROM_LOAD( "rotox2.bin",  0x1000, 0x1000, 0x33cdda83 )
	ROM_LOAD( "rotox3.bin",  0x2000, 0x1000, 0xdbf97491 )
	ROM_LOAD( "rotox4.bin",  0x3000, 0x1000, 0xa3b90ac8 )
	ROM_LOAD( "rotox5.bin",  0x8000, 0x1000, 0x46ae8a94 )
	ROM_LOAD( "rotox6.bin",  0x9000, 0x1000, 0x7916b730 )
	ROM_LOAD( "rotox7.bin",  0xa000, 0x1000, 0x276dc4a5 )
	ROM_LOAD( "rotox8.bin",  0xb000, 0x1000, 0x1ef13457 )
  	ROM_LOAD( "rotox9.bin",  0xc000, 0x1000, 0x370352bf )
	ROM_LOAD( "rotox10.bin", 0xd000, 0x1000, 0xe762cbda )
ROM_END

static struct MemoryReadAddress robby_readmem[] =
{
	{ 0xe000, 0xffff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0xdfff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress robby_writemem[] =
{
	{ 0xe000, 0xffff, MWA_RAM },
	{ 0x4000, 0x7fff, wow_videoram_w, &wow_videoram, &videoram_size },
	{ 0x0000, 0x3fff, wow_magicram_w },
	{ 0x8000, 0xdfff, MWA_ROM },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( robby_input_ports )

	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* Dip Switch */
	PORT_DIPNAME( 0x01, 0x00, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

INPUT_PORTS_END

static struct MachineDriver robby_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1789773,	/* 1.789 Mhz */
			0,
			robby_readmem,robby_writemem,readport,writeport,
			wow_interrupt,256
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	320, 204, { 1, 320-1, 0, 204-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	wow_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	{
		{
			SOUND_ASTROCADE,
			&astrocade_2chip_interface
		}
	}
};

static int robby_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0xE13B],"\x10\x27",2) == 0) &&
		(memcmp(&RAM[0xE1E4],"COCK",4) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
	    osd_fread(f,&RAM[0xE13B],0xAD);
			/* appears twice in memory??? */
			memcpy(&RAM[0xE33B],&RAM[0xE13B],0xAD);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void robby_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
	osd_fwrite(f,&RAM[0xE13B],0xAD);
		osd_fclose(f);
	}

}

struct GameDriver robby_driver =
{
	__FILE__,
	0,
	"robby",
	"Robby Roto",
	"1981",
	"Bally Midway",
	"Nicola Salmoria (MAME driver)\nSteve Scavone (info and code)\nMike Coates (additional code)\nFrank Palazzolo",
	0,
	&robby_machine_driver,
	0,

	robby_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	robby_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	robby_hiload, robby_hisave
};

/****************************************************************************
 * Gorf
 ****************************************************************************/

ROM_START( gorf_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "gorf-a.bin", 0x0000, 0x1000, 0x5b348321 )
	ROM_LOAD( "gorf-b.bin", 0x1000, 0x1000, 0x62d6de77 )
	ROM_LOAD( "gorf-c.bin", 0x2000, 0x1000, 0x1d3bc9c9 )
	ROM_LOAD( "gorf-d.bin", 0x3000, 0x1000, 0x70046e56 )
	ROM_LOAD( "gorf-e.bin", 0x8000, 0x1000, 0x2d456eb5 )
	ROM_LOAD( "gorf-f.bin", 0x9000, 0x1000, 0xf7e4e155 )
	ROM_LOAD( "gorf-g.bin", 0xa000, 0x1000, 0x4e2bd9b9 )
	ROM_LOAD( "gorf-h.bin", 0xb000, 0x1000, 0xfe7b863d )
ROM_END

ROM_START( gorfpgm1_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "873a", 0x0000, 0x1000, 0x97cb4a6a )
	ROM_LOAD( "873b", 0x1000, 0x1000, 0x257236f8 )
	ROM_LOAD( "873c", 0x2000, 0x1000, 0x16b0638b )
	ROM_LOAD( "873d", 0x3000, 0x1000, 0xb5e821dc )
	ROM_LOAD( "873e", 0x8000, 0x1000, 0x8e82804b )
	ROM_LOAD( "873f", 0x9000, 0x1000, 0x715fb4d9 )
	ROM_LOAD( "873g", 0xa000, 0x1000, 0x8a066456 )
	ROM_LOAD( "873h", 0xb000, 0x1000, 0x56d40c7c )
ROM_END

/* Here's the same words in English : Missing bite, dust, conquer, another, galaxy, try, again, devour, attack, power */

static const char *gorf_sample_names[] =
{
 "*gorf","a.sam","a.sam","again.sam","am.sam","am.sam","and.sam","anhilatn.sam",
 "another.sam","another.sam","are.sam","are.sam",
 "avenger.sam","bad.sam","bad.sam","be.sam",
 "been.sam","but.sam","button.sam","cadet.sam",
 "cannot.sam","captain.sam","chronicl.sam","coin.sam","coins.sam","colonel.sam",
 "consciou.sam","defender.sam","destroy.sam","destroyd.sam",
 "doom.sam","draws.sam","empire.sam","end.sam",
 "enemy.sam","escape.sam","flagship.sam","for.sam","galactic.sam",
 "general.sam","gorf.sam","gorphian.sam","gorphian.sam","gorphins.sam",
 "hahahahu.sam","hahaher.sam","harder.sam","have.sam",
 "hitting.sam","i.sam","i.sam","impossib.sam","in.sam","insert.sam",
 "is.sam","live.sam","long.sam","meet.sam","move.sam",
 "my.sam","my.sam",
 "near.sam","next.sam","nice.sam","no.sam",
 "now.sam","pause.sam","player.sam","prepare.sam","prisonrs.sam",
 "promoted.sam","push.sam","robot.sam","robots.sam","robots.sam",
 "seek.sam","ship.sam","shot.sam","some.sam","space.sam","spause.sam",
 "survival.sam","take.sam","the.sam","the.sam","the.sam","time.sam",
 "to.sam","to.sam","unbeatab.sam",
 "warrior.sam","warriors.sam","will.sam",
 "you.sam","you.sam","you.sam","you.sam","your.sam","your.sam","yourself.sam",
 "s.sam","for.sam","for.sam","will.sam","Gorph.sam",
// Missing Samples
 "coin", "attack.sam","bite.sam","conquer.sam","devour.sam","dust.sam",
 "galaxy.sam","got.sam","power.sam","try.sam","supreme.sam","all.sam",
 "hail.sam","emperor.sam",
 0
} ;

static struct IOReadPort Gorf_readport[] =
{
	{ 0x08, 0x08, wow_intercept_r },
	{ 0x0E, 0x0E, wow_video_retrace_r },
	{ 0x10, 0x10, input_port_0_r },
	{ 0x11, 0x11, input_port_1_r },
	{ 0x12, 0x12, gorf_port_2_r },
	{ 0x13, 0x13, input_port_3_r },
	{ 0x15, 0x16, Gorf_IO_r },				/* Actually a Write! */
	{ 0x17, 0x17, gorf_speech_r },				/* Actually a Write! */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress Gorf_readmem[] =
{
	{ 0xd000, 0xd0a4, MRA_RAM },
	{ 0xd0a5, 0xd0a5, gorf_timer_r },
	{ 0xd0a6, 0xdfff, MRA_RAM },
	{ 0xd9d5, 0xdfff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0xcfff, MRA_ROM },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( gorf_input_ports )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* speech status */

	PORT_START /* Dip Switch */
	PORT_DIPNAME( 0x01, 0x01, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x06, 0x06, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/5 Credits" )
	PORT_DIPNAME( 0x08, 0x08, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "English" )
	PORT_DIPSETTING(    0x00, "Foreign (NEED ROM)" )
	PORT_DIPNAME( 0x10, 0x00, "Lives per Credit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Mission 5" )
	PORT_DIPSETTING(    0x20, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END

static struct Samplesinterface gorf_samples_interface =
{
	8	/* 8 channels */
};

static struct MachineDriver gorf_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1789773,	/* 1.789 Mhz */
			0,
			Gorf_readmem,writemem,Gorf_readport,writeport,		/* ASG */
			gorf_interrupt,256
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	/* it may look like the right hand side of the screen needs clipping, but */
	/* this isn't the case: cocktail mode would be clipped on the wrong side */
	204, 320, { 0, 204-1, 0, 320-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	gorf_vh_start,
	generic_vh_stop,
	gorf_vh_screenrefresh,

	/* sound hardware */
	0,
	gorf_sh_start,
	gorf_sh_stop,
	gorf_sh_update,
	{
		{
			SOUND_SAMPLES,
			&gorf_samples_interface
		},
		{
			SOUND_ASTROCADE,
			&astrocade_2chip_interface
		}
	}
};

static int gorf_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((RAM[0xD00B]==0xFF) && (RAM[0xD03D]==0x33))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
	    osd_fread(f,&RAM[0xD010],0x22);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void gorf_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
	osd_fwrite(f,&RAM[0xD010],0x22);
		osd_fclose(f);
	}

}

struct GameDriver gorf_driver =
{
	__FILE__,
	0,
	"gorf",
    "Gorf",
	"1981",
	"Midway",
    "Nicola Salmoria (MAME driver)\nSteve Scavone (info and code)\nMike Coates (game support)\nKevin Estep (samples)\nAlex Judd (word sound driver)\nFrank Palazzolo",
	0,
	&gorf_machine_driver,
	0,

	gorf_rom,
	0, 0,
	gorf_sample_names,
	0,	/* sound_prom */

	gorf_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	gorf_hiload, gorf_hisave
};

struct GameDriver gorfpgm1_driver =
{
	__FILE__,
	&gorf_driver,
	"gorfpgm1",
    "Gorf (Program 1)",
	"1981",
	"Midway",
    "Nicola Salmoria (MAME driver)\nSteve Scavone (info and code)\nMike Coates (game support)\nKevin Estep (samples)\nAlex Judd (word sound driver)\nFrank Palazzolo",
	0,
	&gorf_machine_driver,
	0,

	gorfpgm1_rom,
	0, 0,
	gorf_sample_names,
	0,	/* sound_prom */

	gorf_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	gorf_hiload, gorf_hisave
};

/****************************************************************************
 * Space Zap
 ****************************************************************************/

ROM_START( spacezap_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "0662.01", 0x0000, 0x1000, 0xa92de312 )
	ROM_LOAD( "0663.xx", 0x1000, 0x1000, 0x4836ebf1 )
	ROM_LOAD( "0664.xx", 0x2000, 0x1000, 0xd8193a80 )
	ROM_LOAD( "0665.xx", 0x3000, 0x1000, 0x3784228d )
ROM_END

INPUT_PORTS_START( spacezap_input_ports )


	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* IN1 */

	PORT_START /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* Dip Switch */
	PORT_DIPNAME( 0x01, 0x01, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x06, 0x06, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/5 Credits" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END

static struct MachineDriver spacezap_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1789773,	/* 1.789 Mhz */
			0,
			readmem,writemem,readport,writeport,
			wow_interrupt,256
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	320, 204, { 0, 320-1, 0, 204-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	wow_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	{
		{
				SOUND_ASTROCADE,
				&astrocade_2chip_interface
		}
	}
};

static int spacezap_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if memory has already been initialized */
	    if (memcmp(&RAM[0xD024],"\x01\x01",2) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
	        osd_fread(f,&RAM[0xD01D],6);
			/* Appears twice in memory??? */
			memcpy(&RAM[0xD041],&RAM[0xD01D],6);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void spacezap_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
	    osd_fwrite(f,&RAM[0xD01D],6);
		osd_fclose(f);
	}

}

struct GameDriver spacezap_driver =
{
	__FILE__,
	0,
	"spacezap",
	"Space Zap",
	"1980",
	"Midway",
	"Nicola Salmoria (MAME driver)\nSteve Scavone (info and code)\nMike Coates (game support)\nFrank Palazzolo",
	0,
	&spacezap_machine_driver,
	0,

	spacezap_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	spacezap_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	spacezap_hiload, spacezap_hisave
};

/****************************************************************************
 * Seawolf II
 ****************************************************************************/

ROM_START( seawolf2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "sw2x1.bin", 0x0000, 0x0800, 0xad0103f6 )
	ROM_LOAD( "sw2x2.bin", 0x0800, 0x0800, 0xe0430f0a )
	ROM_LOAD( "sw2x3.bin", 0x1000, 0x0800, 0x05ad1619 )
	ROM_LOAD( "sw2x4.bin", 0x1800, 0x0800, 0x1a1a14a2 )
ROM_END

static struct MemoryReadAddress seawolf2_readmem[] =
{
	{ 0xc000, 0xcfff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress seawolf2_writemem[] =
{
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0x4000, 0x7fff, wow_videoram_w, &wow_videoram, &videoram_size },
	{ 0x0000, 0x3fff, wow_magicram_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( seawolf2_input_ports )
	PORT_START /* IN0 */
	/* bits 0-5 come from a dial - we fake it */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x40, 0x00, "Language 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Language 2" )
	PORT_DIPSETTING(    0x40, "French" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 )

	PORT_START /* IN1 */
	/* bits 0-5 come from a dial - we fake it */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT| IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME( 0x08, 0x00, "Language 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x08, "German" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START /* Dip Switch */
	PORT_DIPNAME( 0x01, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x06, 0x00, "Play Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "40" )
	PORT_DIPSETTING(    0x04, "50" )
	PORT_DIPSETTING(    0x02, "60" )
	PORT_DIPSETTING(    0x00, "70" )
	PORT_DIPNAME( 0x08, 0x08, "2 Players Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Credits" )
	PORT_DIPNAME( 0x30, 0x00, "Extended Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "5000" )
	PORT_DIPSETTING(    0x20, "6000" )
	PORT_DIPSETTING(    0x30, "7000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Colour" )
	PORT_DIPSETTING(    0x00, "Mono" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct IOReadPort seawolf2_readport[] =
{
	{ 0x08, 0x08, wow_intercept_r },
	{ 0x0E, 0x0E, wow_video_retrace_r },
	{ 0x10, 0x10, seawolf2_controller2_r },
	{ 0x11, 0x11, seawolf2_controller1_r },
	{ 0x12, 0x12, input_port_2_r },
	{ 0x13, 0x13, input_port_3_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort seawolf2_writeport[] =
{
	{ 0x00, 0x07, colour_register_w },
	{ 0x08, 0x08, MWA_NOP }, /* Gorf uses this */
	{ 0x09, 0x09, colour_split_w },
	{ 0x0a, 0x0b, MWA_NOP }, /* Gorf uses this */
	{ 0x0c, 0x0c, wow_magic_control_w },
	{ 0x0d, 0x0d, interrupt_vector_w },
	{ 0x0e, 0x0e, wow_interrupt_enable_w },
	{ 0x0f, 0x0f, wow_interrupt_w },
	{ 0x19, 0x19, wow_magic_expand_color_w },

	{ -1 }	/* end of table */
};
static struct MachineDriver seawolf_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1789773,	/* 1.789 Mhz */
			0,
			seawolf2_readmem,seawolf2_writemem,seawolf2_readport,seawolf2_writeport,
			wow_interrupt,256
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	320, 204, { 1, 320-1, 0, 204-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	seawolf2_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
};

static int seawolf_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	    if (memcmp(&RAM[0xC20D],"\xD8\x19",2) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
	        osd_fread(f,&RAM[0xC208],2);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void seawolf_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
	    osd_fwrite(f,&RAM[0xC208],2);
		osd_fclose(f);
	}

}

struct GameDriver seawolf2_driver =
{
	__FILE__,
	0,
	"seawolf2",
	"Sea Wolf II",
	"1978",
	"Midway",
	"Nicola Salmoria (MAME driver)\nSteve Scavone (info and code)\nMike Coates (game support)",
	0,
	&seawolf_machine_driver,
	0,

	seawolf2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	seawolf2_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	seawolf_hiload, seawolf_hisave
};

/****************************************************************************
 * Extra Bases (Bally/Midway)
 ****************************************************************************/

ROM_START( ebases_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "m761a", 0x0000, 0x1000, 0x34422147 )
	ROM_LOAD( "m761b", 0x1000, 0x1000, 0x4f28dfd6 )
	ROM_LOAD( "m761c", 0x2000, 0x1000, 0xbff6c97e )
	ROM_LOAD( "m761d", 0x3000, 0x1000, 0x5173781a )
ROM_END

static struct MemoryReadAddress ebases_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xcfff, MRA_ROM },
	{ 0xd000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress ebases_writemem[] =
{
	{ 0x0000, 0x3fff, wow_magicram_w },
	{ 0x4000, 0x7fff, wow_videoram_w, &wow_videoram, &videoram_size },
	{ 0x8000, 0xbfff, MWA_RAM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( ebases_input_ports )
	PORT_START /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_TILT )
	PORT_DIPNAME( 0x08, 0x08, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x10, 0x00, "Colour / Mono", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Colour" )
	PORT_DIPSETTING(    0x10, "Mono" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )


	PORT_START /* Dip Switch */
	PORT_DIPNAME( 0x01, 0x00, "2 Players Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Credits" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START /* Dip Switch */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END


static struct IOReadPort ebases_readport[] =
{
	{ 0x08, 0x08, wow_intercept_r },
	{ 0x0E, 0x0E, wow_video_retrace_r },
	{ 0x10, 0x10, input_port_0_r },
	{ 0x11, 0x11, input_port_1_r },
	{ 0x12, 0x12, input_port_2_r },
	{ 0x13, 0x13, input_port_3_r },
	{ -1 }	/* end of table */
};

static struct MachineDriver ebases_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1789773,	/* 1.789 Mhz */
			0,
			ebases_readmem,ebases_writemem,ebases_readport,writeport,
			wow_interrupt,256
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	320, 210, { 1, 320-1, 0, 192-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	seawolf2_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	{
		{
				SOUND_ASTROCADE,
				&astrocade_1chip_interface
		}
	}
};

struct GameDriver ebases_driver =
{
	__FILE__,
	0,
	"ebases",
	"Extra Bases",
	"1980",
	"Midway",
	"Alex Judd (MAME driver)\nSteve Scavone (info and code)\nMike Coates (game support)\nFrank Palazzolo",
	GAME_NOT_WORKING,
	&ebases_machine_driver,
	0,

	ebases_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	ebases_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	0, 0
};

/***************************************************************************

Wizard of Wor memory map (preliminary)

0000-3fff ROM A/B/C/D but also "magic" RAM (which processes data and copies it to the video RAM)
4000-7fff SCREEN RAM (bitmap)
8000-afff ROM E/F/G
c000-cfff ROM X (Foreign language - not required)
d000-d3ff STATIC RAM

I/O ports:
IN:
08        intercept register (collision detector)
	      bit 0: intercept in pixel 3 in an OR or XOR write since last reset
	      bit 1: intercept in pixel 2 in an OR or XOR write since last reset
	      bit 2: intercept in pixel 1 in an OR or XOR write since last reset
	      bit 3: intercept in pixel 0 in an OR or XOR write since last reset
	      bit 4: intercept in pixel 3 in last OR or XOR write
	      bit 5: intercept in pixel 2 in last OR or XOR write
	      bit 6: intercept in pixel 1 in last OR or XOR write
	      bit 7: intercept in pixel 0 in last OR or XOR write
10        IN0
11        IN1
12        IN2
13        DSW
14		  Video Retrace
15        ?
17        Speech Synthesizer (Output)

*
 * IN0 (all bits are inverted)
 * bit 7 : flip screen
 * bit 6 : START 2
 * bit 5 : START 1
 * bit 4 : SLAM
 * bit 3 : TEST
 * bit 2 : COIN 3
 * bit 1 : COIN 2
 * bit 0 : COIN 1
 *
*
 * IN1 (all bits are inverted)
 * bit 7 : ?
 * bit 6 : ?
 * bit 5 : FIRE player 1
 * bit 4 : MOVE player 1
 * bit 3 : RIGHT player 1
 * bit 2 : LEFT player 1
 * bit 1 : DOWN player 1
 * bit 0 : UP player 1
 *
*
 * IN2 (all bits are inverted)
 * bit 7 : ?
 * bit 6 : ?
 * bit 5 : FIRE player 2
 * bit 4 : MOVE player 2
 * bit 3 : RIGHT player 2
 * bit 2 : LEFT player 2
 * bit 1 : DOWN player 2
 * bit 0 : UP player 2
 *
*
 * DSW (all bits are inverted)
 * bit 7 : Attract mode sound  0 = Off  1 = On
 * bit 6 : 1 = Coin play  0 = Free Play
 * bit 5 : Bonus  0 = After fourth level  1 = After third level
 * bit 4 : Number of lives  0 = 3/7  1 = 2/5
 * bit 3 : language  1 = English  0 = Foreign
 * bit 2 : \ right coin slot  00 = 1 coin 5 credits  01 = 1 coin 3 credits
 * bit 1 : /                  10 = 2 coins 1 credit  11 = 1 coin 1 credit
 * bit 0 : left coin slot 0 = 2 coins 1 credit  1 = 1 coin 1 credit
 *


OUT:
00-07     palette (00-03 = left part of screen; 04-07 right part)
09        position where to switch from the "left" to the "right" palette.
08        select video mode (0 = low res 160x102, 1 = high res 320x204)
0a        screen height
0b        color block transfer
0c        magic RAM control
	      bit 7: ?
	      bit 6: flip
	      bit 5: draw in XOR mode
	      bit 4: draw in OR mode
	      bit 3: "expand" mode (convert 1bpp data to 2bpp)
	      bit 2: ?
	      bit 1:\ shift amount to be applied before copying
	      bit 0:/
0d        set interrupt vector
10-18     sound
19        magic RAM expand mode color
78-7e     pattern board (see vidhrdw.c for details)

***************************************************************************

Seawolf Specific Bits

IN
10		Controller Player 2
		bit 0-5	= rotary controller
	    bit 7   = fire button

11		Controller Player 1
		bit 0-5 = rotary controller
	    bit 6   = Language Select 1  (On = French, Off = language 2)
	    bit 7   = fire button

12      bit 0   = Coin
		bit 1   = Start 1 Player
	    bit 2   = Start 2 Player
	    bit 3   = Language 2         (On = German, Off = English)

13      bit 0   = Coinage
		bit 1   = \ Length of time	 (40, 50, 60 or 70 Seconds)
	    bit 2   = / for Game
	    bit 3   = ?
	    bit 4   = ?
	    bit 5   = ?
	    bit 6   = Colour / Mono
	    bit 7   = Test

***************************************************************************/

