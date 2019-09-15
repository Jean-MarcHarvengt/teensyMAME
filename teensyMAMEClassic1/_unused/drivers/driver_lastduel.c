/**************************************************************************

* Last Duel ('American' set)      - Capcom, 1988
* Last Duel (Bootleg set)         - Capcom, 1988
* Mad Gear (aka Led Storm)        - Capcom, 1989

Emulation by Bryan McPhail, mish@tendril.force9.net

Issues:

* American Last Duel set is missing 1 set of tile roms, the roms from
the bootleg set are used.

* Strange bug in Last Duel, American set - the 'y' at the far right of
the screen from the copyright message is not cleared. Weird...

* Wrong dip switches for Mad Gear (service mode is in there somewhere..)

* No tile roms for Mad Gear!

**************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "z80/z80.h"

extern int lastduel_vram_r(int offset);
extern void lastduel_vram_w(int offset,int value);
extern int lastduel_scroll2_r(int offset);
extern void lastduel_scroll2_w(int offset,int value);
extern int lastduel_scroll1_r(int offset);
extern void lastduel_scroll1_w(int offset,int value);
extern int lastduel_sprites_r(int offset);
extern void lastduel_sprites_w(int offset,int value);
extern int lastduel_vh_start(void);
extern void lastduel_vh_stop(void);
extern void lastduel_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void lastduel_scroll_w( int offset, int data );
#if 0
extern void lastduel_set_dirty(void);
#endif

extern unsigned char *lastduel_ram;
extern unsigned char *lastduel_vram;
extern unsigned char *lastduel_scroll2;
extern unsigned char *lastduel_scroll1;
extern unsigned char *lastduel_sprites;

/******************************************************************************/

static int madgear_inputs_r(int offset)
{
	switch (offset) {
  	case 4: /* Player 1 & Player 2 controls */
    	return(readinputport(0)<<8)+readinputport(1);

    case 2: /* COINS in top two bits of byte 3 */

   //  return 0;

    	return(readinputport(2)<<8)+readinputport(3);

    case 6: /* Start + coins */
    	return(readinputport(2)<<8)+readinputport(3);

    case 0: // Return 0 for test mode
      return(readinputport(0)<<8)+readinputport(1);

  }

   if (errorlog) fprintf(errorlog,"Unknown read\n" );
	return 0xffff;
}

static int lastduel_inputs_r(int offset)
{

  switch (offset) {
  	case 0: /* Player 1 & Player 2 controls */
    	return(readinputport(0)<<8)+readinputport(1);

    case 2: /* Coins & Service switch */
      return readinputport(2);

    case 4: /* Dips */
    	return (readinputport(3)<<8)+readinputport(4);

    case 6: /* Dips, flip */
      return readinputport(5);
  }

  if (errorlog) fprintf(errorlog,"Unknown read\n" );
  return 0xffff;
}

static void lastduel_sound_w( int offset, int data )
{
	switch( offset )
	{
		case 0:
		  // Unknown - looks like a watchdog
			break;
		case 2:
			soundlatch_w(offset,data & 0xff);
			break;
	}
}

/******************************************************************************/

static struct MemoryReadAddress lastduel_readmem[] =
{
  { 0x000000, 0x05ffff, MRA_ROM },
  { 0xfc4000, 0xfc4007, lastduel_inputs_r },
  { 0xfc0800, 0xfc0cff, lastduel_sprites_r },
  { 0xfcc000, 0xfcdfff, lastduel_vram_r },
  { 0xfd0000, 0xfd3fff, lastduel_scroll1_r },
  { 0xfd4000, 0xfd7fff, lastduel_scroll2_r },
  { 0xfd8000, 0xfd87ff, paletteram_word_r },
  { 0xfe0000, 0xffffff, MRA_BANK1 },
  { -1 }	/* end of table */
};

static struct MemoryWriteAddress lastduel_writemem[] =
{
  { 0x000000, 0x05ffff, MWA_ROM },
  { 0xfc0000, 0xfc0003, MWA_NOP }, /* Written rarely */
  { 0xfc4000, 0xfc4003, lastduel_sound_w },
  { 0xfc8000, 0xfc800f, lastduel_scroll_w },
  { 0xfc0800, 0xfc0d07, lastduel_sprites_w, &lastduel_sprites },
  { 0xfcc000, 0xfcdfff, lastduel_vram_w,    &lastduel_vram },
  { 0xfd0000, 0xfd3fff, lastduel_scroll1_w, &lastduel_scroll1 },
  { 0xfd4000, 0xfd7fff, lastduel_scroll2_w, &lastduel_scroll2 },
  { 0xfd8000, 0xfd87ff, paletteram_RRRRGGGGBBBBIIII_word_w, &paletteram },
  { 0xfe0000, 0xffffff, MWA_BANK1, &lastduel_ram },
  { -1 }	/* end of table */
};

static struct MemoryReadAddress madgear_readmem[] =
{
  { 0x000000, 0x07ffff, MRA_ROM },
  { 0xfc4000, 0xfc4007, madgear_inputs_r },
  { 0xfc1800, 0xfc1fff, lastduel_sprites_r },
  { 0xfc8000, 0xfc9fff, lastduel_vram_r },
  { 0xfcc000, 0xfcc7ff, paletteram_word_r },
  { 0xfd4000, 0xfd7fff, lastduel_scroll1_r },
  { 0xfd8000, 0xfdffff, lastduel_scroll2_r },
  { 0xff0000, 0xffffff, MRA_BANK1 },
  { -1 }	/* end of table */
};

static struct MemoryWriteAddress madgear_writemem[] =
{
  { 0x000000, 0x07ffff, MWA_ROM },
  { 0xfc4000, 0xfc4003, lastduel_sound_w },
  { 0xfd0000, 0xfd000f, lastduel_scroll_w },
  { 0xfc1800, 0xfc1fff, lastduel_sprites_w, &lastduel_sprites },
  { 0xfc8000, 0xfc9fff, lastduel_vram_w,    &lastduel_vram },
  { 0xfcc000, 0xfcc7ff, paletteram_RRRRGGGGBBBBIIII_word_w, &paletteram },
  { 0xfd4000, 0xfd7fff, lastduel_scroll1_w, &lastduel_scroll1 },
  { 0xfd8000, 0xfdffff, lastduel_scroll2_w, &lastduel_scroll2 },
  { 0xff0000, 0xffffff, MWA_BANK1 },
  { -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ 0xe800, 0xe800, YM2203_status_port_0_r },
	{ 0xf000, 0xf000, YM2203_status_port_1_r },
	{ 0xf800, 0xf800, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xe800, YM2203_control_port_0_w },
	{ 0xe801, 0xe801, YM2203_write_port_0_w },
	{ 0xf000, 0xf000, YM2203_control_port_1_w },
	{ 0xf001, 0xf001, YM2203_write_port_1_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress mg_sound_readmem[] =
{
	{ 0x0000, 0xcfff, MRA_ROM },
	{ 0xd000, 0xd7ff, MRA_RAM },
 	{ 0xf000, 0xf000, YM2203_status_port_0_r },
 	{ 0xf002, 0xf002, YM2203_status_port_1_r },
 	{ 0xf006, 0xf006, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress mg_sound_writemem[] =
{
	{ 0x0000, 0xcfff, MWA_ROM },
	{ 0xd000, 0xd7ff, MWA_RAM },
 	{ 0xf000, 0xf000, YM2203_control_port_0_w },
 	{ 0xf001, 0xf001, YM2203_write_port_0_w },
 	{ 0xf002, 0xf002, YM2203_control_port_1_w },
	{ 0xf003, 0xf003, YM2203_write_port_1_w },
	{ 0xf00a, 0xf00a, MWA_NOP },
	{ -1 }	/* end of table */
};

/******************************************************************************/

static struct GfxLayout sprites =
{
  16,16,  /* 16*16 sprites */
  4096,   /* 32 bytes per sprite, 0x20000 per plane so 4096 sprites */
  4,      /* 4 bits per pixel */
  { 0x00000*8, 0x20000*8, 0x40000*8, 0x60000*8  },
  {
    0,1,2,3,4,5,6,7,
    (16*8)+0,(16*8)+1,(16*8)+2,(16*8)+3,
    (16*8)+4,(16*8)+5,(16*8)+6,(16*8)+7
  },
  {
    0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
    8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
  },
  32*8   /* every sprite takes 32 consecutive bits */
};

static struct GfxLayout text_layout =
{
  8,8,    /* 8*8 characters */
  2048,   /* 2048 character */
  2,      /* 2 bitplanes */
  { 4,0 },
  {
    0,1,2,3,8,9,10,11
  },
  {
    0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16
  },
  16*8   /* every character takes 16 consecutive bytes */
};

static struct GfxLayout scroll1layout =
{
  16,16,  /* 16*16 tiles */
  2048,   /* 2048 tiles */
  4,      /* 4 bits per pixel */
  { 4,0,(0x020000*8)+4,0x020000*8, },
  {
    0,1,2,3,8,9,10,11,
    (8*4*8)+0,(8*4*8)+1,(8*4*8)+2,(8*4*8)+3,
    (8*4*8)+8,(8*4*8)+9,(8*4*8)+10,(8*4*8)+11
  },
  {
    0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
    8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16
  },
  64*8   /* each tile takes 64 consecutive bytes */
};

static struct GfxLayout scroll2layout =
{
  16,16,  /* 16*16 tiles */
  4096,   /* 4096 tiles */
  4,      /* 4 bits per pixel */
  { 4,0,(0x040000*8)+4,0x040000*8, },
  {
    0,1,2,3,8,9,10,11,
    (8*4*8)+0,(8*4*8)+1,(8*4*8)+2,(8*4*8)+3,
    (8*4*8)+8,(8*4*8)+9,(8*4*8)+10,(8*4*8)+11
  },
  {
    0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
    8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16
  },
  64*8   /* each tile takes 64 consecutive bytes */
};

static struct GfxDecodeInfo lastduel_gfxdecodeinfo[] =
{
  {  1, 0x40000,&sprites,       512, 16 },	/* colors 512-767 */
  {  1, 0xc0000,&text_layout,   768, 16 },	/* colors 768-831 */
  {  1, 0x00000,&scroll1layout,   0, 16 },	/* colors   0-255 */
  {  1, 0xc8000,&scroll2layout,	256, 16 },	/* colors 256-511 */
  { -1 }
};

/******************************************************************************

Save States - shall be used someday ;) */

#if 0
extern int MC68000_State_Load(void *f);
extern int MC68000_State_Save(void *f);

static void LoadState(void)
{
	void *f;
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0) {
		MC68000_State_Load(f);
		osd_fread(f,lastduel_ram,0x20000);
		osd_fread(f,lastduel_sprites,0x506);
		osd_fread(f,lastduel_vram,0x2000);
		osd_fread(f,lastduel_scroll1,0x4000);
		osd_fread(f,lastduel_scroll2,0x4000);
		osd_fread(f,paletteram,0x800);
		osd_fclose(f);
		lastduel_set_dirty();
	}
}

static void SaveState(void)
{
	void *f;
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0) {
		MC68000_State_Save(f);
		osd_fwrite(f,lastduel_ram,0x20000);
		osd_fwrite(f,lastduel_sprites,0x506);
		osd_fwrite(f,lastduel_vram,0x2000);
		osd_fwrite(f,lastduel_scroll1,0x4000);
		osd_fwrite(f,lastduel_scroll2,0x4000);
		osd_fwrite(f,paletteram,0x800);
		osd_fclose(f);
	}
}
#endif

/******************************************************************************/

/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(void)
{
	cpu_cause_interrupt(1,0xff);
}

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	3500000,	/* 3.5 MHz ? */
	{ YM2203_VOL(140,255), YM2203_VOL(140,255) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};

int lastduel_interrupt(void)
{
#if 0
	/* I use these to trigger save states:*/
	if ((readinputport(0)&0x1)!=0x1) SaveState();
	if ((readinputport(0)&0x2)!=0x2) LoadState();
#endif

	if (cpu_getiloops() == 0) return 2;
	else return 4;
}

int madgear_interrupt(void)
{
	if (cpu_getiloops() == 0) return 5;
	else return 6;
}

static struct MachineDriver lastduel_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,
			0,
			lastduel_readmem, lastduel_writemem, 0,0,
			lastduel_interrupt,6	/* ??? */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579000,
			2,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are caused by the YM2203 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	512, 256, { 63, 455, 0, 255 },

	lastduel_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	lastduel_vh_start,
	lastduel_vh_stop,
	lastduel_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

static struct MachineDriver madgear_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,
			0,
			madgear_readmem, madgear_writemem, 0,0,
			madgear_interrupt,6	/* ??? */
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579000,
			2,
			mg_sound_readmem,mg_sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are caused by the YM2203 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	512, 256, { 63, 455, 0, 255 },

	lastduel_gfxdecodeinfo,
	1024, 1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	lastduel_vh_start,
	lastduel_vh_stop,
	lastduel_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

/******************************************************************************/

INPUT_PORTS_START( lastduel_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )	/* Could be cabinet type? */
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "20000 60000 80000" )
	PORT_DIPSETTING(    0x30, "30000 80000 80000" )
	PORT_DIPSETTING(    0x10, "40000 80000 80000" )
	PORT_DIPSETTING(    0x00, "40000 80000 100000" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x18, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "6" )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPNAME( 0x04, 0x04, "Type", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Car" )
	PORT_DIPSETTING(    0x00, "Plane" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( madgear_input_ports )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 ) //TEST
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 ) //TEST

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY ) //TEST
	PORT_BITX(    0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )      //TEST
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 )     //TEST
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x04, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x18, 0x18, "Bonus Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "20K, 40K, every 60K" )
	PORT_DIPSETTING(    0x10, "30K, 50K, every 70K" )
	PORT_DIPSETTING(    0x08, "20K and 60K only" )
	PORT_DIPSETTING(    0x00, "30K and 70K only" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x80, "???", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, "P1 Credits/Coins", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1/4" )
	PORT_DIPSETTING(    0x01, "1/3" )
	PORT_DIPSETTING(    0x02, "1/2" )
	PORT_DIPSETTING(    0x03, "6/1" )
	PORT_DIPSETTING(    0x04, "4/1" )
	PORT_DIPSETTING(    0x05, "3/1" )
	PORT_DIPSETTING(    0x06, "2/1" )
	PORT_DIPSETTING(    0x07, "1/1" )
	PORT_DIPNAME( 0x38, 0x38, "P2 Credits/Coins", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1/4" )
	PORT_DIPSETTING(    0x08, "1/3" )
	PORT_DIPSETTING(    0x10, "1/2" )
	PORT_DIPSETTING(    0x18, "6/1" )
	PORT_DIPSETTING(    0x20, "4/1" )
	PORT_DIPSETTING(    0x28, "3/1" )
	PORT_DIPSETTING(    0x30, "2/1" )
	PORT_DIPSETTING(    0x38, "1/1" )
	PORT_DIPNAME( 0x40, 0x40, "Test Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPNAME( 0x80, 0x80, "Flip", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
INPUT_PORTS_END

/******************************************************************************/

ROM_START( lastduel_rom )
	ROM_REGION(0x60000)	/* 68000 code */
	ROM_LOAD_EVEN( "ldu-06.rom",   0x00000, 0x20000, 0x4228a00b )
	ROM_LOAD_ODD( "ldu-05.rom",   0x00000, 0x20000, 0x7260434f )
	ROM_LOAD_EVEN( "ldu-04.rom",   0x40000, 0x10000, 0x429fb964 )
	ROM_LOAD_ODD( "ldu-03.rom",   0x40000, 0x10000, 0x5aa4df72 )

	ROM_REGION_DISPOSE(0x148000) /* temporary space for graphics */
	ROM_LOAD( "ld_17.bin",    0x000000, 0x10000, 0x7188bfdd ) /* tiles */
	ROM_LOAD( "ld_18.bin",    0x010000, 0x10000, 0xa62af66a )
	ROM_LOAD( "ld_19.bin",    0x020000, 0x10000, 0x4b762e50 )
	ROM_LOAD( "ld_20.bin",    0x030000, 0x10000, 0xb140188e )
	ROM_LOAD( "ld_09.bin",    0x040000, 0x10000, 0xf8fd5243 ) /* sprites */
	ROM_LOAD( "ld_10.bin",    0x050000, 0x10000, 0xb49ad746 )
	ROM_LOAD( "ld_11.bin",    0x060000, 0x10000, 0x1a0d180e )
	ROM_LOAD( "ld_12.bin",    0x070000, 0x10000, 0xb2745e26 )
	ROM_LOAD( "ld_15.bin",    0x080000, 0x10000, 0x96b13bbc )
	ROM_LOAD( "ld_16.bin",    0x090000, 0x10000, 0x9d80f7e6 )
	ROM_LOAD( "ld_13.bin",    0x0a0000, 0x10000, 0xa1a598ac )
	ROM_LOAD( "ld_14.bin",    0x0b0000, 0x10000, 0xedf515cc )
	ROM_LOAD( "ld_01.bin",    0x0c0000, 0x08000, 0xad3c6f87 ) /* 8x8 text */
	ROM_LOAD( "ld_28.bin",    0x0c8000, 0x10000, 0x06778248 ) /* tiles */
	ROM_LOAD( "ld_26.bin",    0x0d8000, 0x10000, 0xb0edac81 )
	ROM_LOAD( "ld_24.bin",    0x0e8000, 0x10000, 0x66eac4df )
	ROM_LOAD( "ld_22.bin",    0x0f8000, 0x10000, 0xf80f8812 )
	ROM_LOAD( "ld_27.bin",    0x108000, 0x10000, 0x48c78675 )
	ROM_LOAD( "ld_25.bin",    0x118000, 0x10000, 0xc541ae9a )
	ROM_LOAD( "ld_23.bin",    0x128000, 0x10000, 0xd817332c )
	ROM_LOAD( "ld_21.bin",    0x138000, 0x10000, 0xb74f0c0e )

	ROM_REGION( 0x10000 ) /* audio CPU */
	ROM_LOAD( "ld_02.bin",    0x0000, 0x10000, 0x91834d0c )
ROM_END

ROM_START( lstduelb_rom )
	ROM_REGION(0x60000)	/* 68000 code */
	ROM_LOAD_EVEN( "ld_08.bin",    0x00000, 0x10000, 0x43811a96 )
	ROM_LOAD_ODD( "ld_07.bin",    0x00000, 0x10000, 0x63c30946 )
	ROM_LOAD_EVEN( "ld_04.bin",    0x20000, 0x10000, 0x46a4e0f8 )
	ROM_LOAD_ODD( "ld_03.bin",    0x20000, 0x10000, 0x8d5f204a )
	ROM_LOAD_EVEN( "ldu-04.rom",   0x40000, 0x10000, 0x429fb964 )
	ROM_LOAD_ODD( "ldu-03.rom",   0x40000, 0x10000, 0x5aa4df72 )

	ROM_REGION_DISPOSE(0x148000) /* temporary space for graphics */
	ROM_LOAD( "ld_17.bin",    0x000000, 0x10000, 0x7188bfdd ) /* tiles */
	ROM_LOAD( "ld_18.bin",    0x010000, 0x10000, 0xa62af66a )
	ROM_LOAD( "ld_19.bin",    0x020000, 0x10000, 0x4b762e50 )
	ROM_LOAD( "ld_20.bin",    0x030000, 0x10000, 0xb140188e )
	ROM_LOAD( "ld_09.bin",    0x040000, 0x10000, 0xf8fd5243 ) /* sprites */
	ROM_LOAD( "ld_10.bin",    0x050000, 0x10000, 0xb49ad746 )
	ROM_LOAD( "ld_11.bin",    0x060000, 0x10000, 0x1a0d180e )
	ROM_LOAD( "ld_12.bin",    0x070000, 0x10000, 0xb2745e26 )
	ROM_LOAD( "ld_15.bin",    0x080000, 0x10000, 0x96b13bbc )
	ROM_LOAD( "ld_16.bin",    0x090000, 0x10000, 0x9d80f7e6 )
	ROM_LOAD( "ld_13.bin",    0x0a0000, 0x10000, 0xa1a598ac )
	ROM_LOAD( "ld_14.bin",    0x0b0000, 0x10000, 0xedf515cc )
	ROM_LOAD( "ld_01.bin",    0x0c0000, 0x08000, 0xad3c6f87 ) /* 8x8 text */
	ROM_LOAD( "ld_28.bin",    0x0c8000, 0x10000, 0x06778248 ) /* tiles */
	ROM_LOAD( "ld_26.bin",    0x0d8000, 0x10000, 0xb0edac81 )
	ROM_LOAD( "ld_24.bin",    0x0e8000, 0x10000, 0x66eac4df )
	ROM_LOAD( "ld_22.bin",    0x0f8000, 0x10000, 0xf80f8812 )
	ROM_LOAD( "ld_27.bin",    0x108000, 0x10000, 0x48c78675 )
	ROM_LOAD( "ld_25.bin",    0x118000, 0x10000, 0xc541ae9a )
	ROM_LOAD( "ld_23.bin",    0x128000, 0x10000, 0xd817332c )
	ROM_LOAD( "ld_21.bin",    0x138000, 0x10000, 0xb74f0c0e )

	ROM_REGION( 0x10000 ) /* audio CPU */
	ROM_LOAD( "ld_02.bin",    0x0000, 0x10000, 0x91834d0c )
ROM_END

ROM_START( madgear_rom )
	ROM_REGION(0x80000)	/* 256K for 68000 code */
	ROM_LOAD_EVEN( "mg_04.rom",    0x00000, 0x20000, 0xb112257d )
	ROM_LOAD_ODD( "mg_03.rom",    0x00000, 0x20000, 0xb2672465 )
	ROM_LOAD_EVEN( "mg_02.rom",    0x40000, 0x20000, 0x9f5ebe16 )
	ROM_LOAD_ODD( "mg_01.rom",    0x40000, 0x20000, 0x1cea2af0 )

	ROM_REGION_DISPOSE(0x148000) /* temporary space for graphics */

	/* No tile roms :( :( */

	ROM_LOAD( "mg_m07.rom",   0x050000, 0x10000, 0xe5c0b211 ) /* Interleaved sprites */
	ROM_LOAD( "mg_m11.rom",   0x040000, 0x10000, 0xee319a64 )
	ROM_LOAD( "mg_m08.rom",   0x070000, 0x10000, 0x59709aa3 )
	ROM_LOAD( "mg_m12.rom",   0x060000, 0x10000, 0x887ef120 )
	ROM_LOAD( "mg_m09.rom",   0x090000, 0x10000, 0x40ee83eb )
	ROM_LOAD( "mg_m13.rom",   0x080000, 0x10000, 0xeae07db4 )
	ROM_LOAD( "mg_m10.rom",   0x0b0000, 0x10000, 0xb64afb54 )
	ROM_LOAD( "mg_m14.rom",   0x0a0000, 0x10000, 0x21e5424c )

	ROM_LOAD( "mg_06.rom",    0x0c0000, 0x08000, 0x382ee59b ) /* 8x8 text */

	ROM_REGION( 0x10000 ) /* audio CPU */
	ROM_LOAD( "mg_05.rom",    0x0000, 0x10000, 0x2fbfc945 )
ROM_END

/******************************************************************************/

struct GameDriver lastduel_driver =
{
	__FILE__,
	0,
	"lastduel",
	"Last Duel",
	"1988",
	"Capcom",
	"Bryan McPhail",
	0,
	&lastduel_machine_driver,
	0,

	lastduel_rom,
	0,0,0,0,

	lastduel_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver lstduelb_driver =
{
	__FILE__,
	&lastduel_driver,
	"lstduelb",
	"Last Duel (bootleg)",
	"1988",
	"bootleg",
	"Bryan McPhail",
	0,
	&lastduel_machine_driver,
	0,

	lstduelb_rom,
	0,0,0,0,

	lastduel_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver madgear_driver =
{
	__FILE__,
	0,
	"madgear",
	"Mad Gear",
	"1989",
	"Capcom",
	"Bryan McPhail\n\nDriver Notes: \n  Tile roms missing!\n",
	GAME_NOT_WORKING,
	&madgear_machine_driver,
	0,

	madgear_rom,
	0,0,0,0,

	madgear_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};
