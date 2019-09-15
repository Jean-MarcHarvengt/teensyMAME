/*
Double Dragon, Double Dragon (bootleg) & Double Dragon II

By Carlos A. Lozano & Rob Rosenbrock
Help to do the original drivers from Chris Moore
Sprite CPU support and additional code by Phil Stroffolino
Sprite CPU emulation, vblank support, and partial sound code by Ernesto Corvi.
Dipswitch to dd2 by Marco Cassili.
High Score support by Roberto Fresca.

TODO:
- Find the original MCU code so original Double Dragon ROMs can be supported

NOTES:
The OKI M5205 chip 0 sampling rate is 8000hz (8khz).
The OKI M5205 chip 1 sampling rate is 4000hz (4khz).
Until the ADPCM interface is updated to be able to use
multiple sampling rates, all samples currently play at 8khz.
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "m6809/m6809.h"

/* from vidhrdw */
extern unsigned char *dd_videoram;
extern int dd_scrollx_hi, dd_scrolly_hi;
extern unsigned char *dd_scrollx_lo;
extern unsigned char *dd_scrolly_lo;
int dd_vh_start(void);
void dd_vh_stop(void);
void dd_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void dd_background_w( int offset, int val );
extern unsigned char *dd_spriteram;
extern int dd2_video;
/* end of extern code & data */

/* private globals */
static int dd_sub_cpu_busy;
static int sprite_irq, sound_irq, ym_irq;
/* end of private globals */

static void dd_init_machine( void ) {
	sprite_irq = M6809_INT_NMI;
	sound_irq = M6809_INT_IRQ;
	ym_irq = M6809_INT_FIRQ;
	dd2_video = 0;
	dd_sub_cpu_busy = 0x10;
}

static void dd2_init_machine( void ) {
	sprite_irq = Z80_NMI_INT;
	sound_irq = Z80_NMI_INT;
	ym_irq = -1000;
	dd2_video = 1;
	dd_sub_cpu_busy = 0x10;
}

static void dd_bankswitch_w( int offset, int data )
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	dd_scrolly_hi = ( ( data & 0x02 ) << 7 );
	dd_scrollx_hi = ( ( data & 0x01 ) << 8 );

	if ( ( data & 0x10 ) == 0x10 ) {
		dd_sub_cpu_busy = 0x00;
	} else if ( dd_sub_cpu_busy == 0x00 )
			cpu_cause_interrupt( 1, sprite_irq );

	cpu_setbank( 1,&RAM[ 0x10000 + ( 0x4000 * ( ( data >> 5 ) & 7 ) ) ] );
}

static void dd_forcedIRQ_w( int offset, int data ) {
	cpu_cause_interrupt( 0, M6809_INT_IRQ );
}

static int port4_r( int offset ) {
	int port = readinputport( 4 );

	return port | dd_sub_cpu_busy;
}

static int dd_spriteram_r( int offset ){
	return dd_spriteram[offset];
}

static void dd_spriteram_w( int offset, int data ) {

	if ( cpu_getactivecpu() == 1 && offset == 0 )
		dd_sub_cpu_busy = 0x10;

	dd_spriteram[offset] = data;
}

static void cpu_sound_command_w( int offset, int data ) {
	soundlatch_w( offset, data );
	cpu_cause_interrupt( 2, sound_irq );
}

static void dd_adpcm_w(int offset,int data)
{
	static int start[2],end[2];
	int chip = offset & 1;


	offset >>= 1;

	switch (offset)
	{
		case 3:
			break;

		case 2:
			start[chip] = data & 0x7f;
			break;

		case 1:
			end[chip] = data & 0x7f;
			break;

		case 0:
			ADPCM_play( chip, 0x10000*chip + start[chip]*0x200, (end[chip]-start[chip])*0x400);
			break;
	}
}

static int dd_adpcm_status_r( int offset )
{
	return ( ADPCM_playing( 0 ) + ( ADPCM_playing( 1 ) << 1 ) );
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2fff, dd_spriteram_r, &dd_spriteram },
	{ 0x3000, 0x37ff, MRA_RAM },
	{ 0x3800, 0x3800, input_port_0_r },
	{ 0x3801, 0x3801, input_port_1_r },
	{ 0x3802, 0x3802, port4_r },
	{ 0x3803, 0x3803, input_port_2_r },
	{ 0x3804, 0x3804, input_port_3_r },
	{ 0x3805, 0x3fff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x11ff, paletteram_xxxxBBBBGGGGRRRR_split1_w, &paletteram },
	{ 0x1200, 0x13ff, paletteram_xxxxBBBBGGGGRRRR_split2_w, &paletteram_2 },
	{ 0x1400, 0x17ff, MWA_RAM },
	{ 0x1800, 0x1fff, MWA_RAM, &videoram },
	{ 0x2000, 0x2fff, dd_spriteram_w },
	{ 0x3000, 0x37ff, dd_background_w, &dd_videoram },
	{ 0x3800, 0x3807, MWA_RAM },
	{ 0x3808, 0x3808, dd_bankswitch_w },
	{ 0x3809, 0x3809, MWA_RAM, &dd_scrollx_lo },
	{ 0x380a, 0x380a, MWA_RAM, &dd_scrolly_lo },
	{ 0x380b, 0x380b, MWA_RAM },
	{ 0x380c, 0x380d, MWA_RAM },
	{ 0x380e, 0x380e, cpu_sound_command_w },
	{ 0x380f, 0x380f, dd_forcedIRQ_w },
	{ 0x3810, 0x3fff, MWA_RAM },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress dd2_writemem[] =
{
	{ 0x0000, 0x17ff, MWA_RAM },
	{ 0x1800, 0x1fff, MWA_RAM, &videoram },
	{ 0x2000, 0x2fff, dd_spriteram_w },
	{ 0x3000, 0x37ff, dd_background_w, &dd_videoram },
	{ 0x3800, 0x3807, MWA_RAM },
	{ 0x3808, 0x3808, dd_bankswitch_w },
	{ 0x3809, 0x3809, MWA_RAM, &dd_scrollx_lo },
	{ 0x380a, 0x380a, MWA_RAM, &dd_scrolly_lo },
	{ 0x380b, 0x380b, MWA_RAM },
	{ 0x380c, 0x380d, MWA_RAM },
	{ 0x380e, 0x380e, cpu_sound_command_w },
	{ 0x380f, 0x380f, dd_forcedIRQ_w },
	{ 0x3810, 0x3bff, MWA_RAM },
	{ 0x3c00, 0x3dff, paletteram_xxxxBBBBGGGGRRRR_split1_w, &paletteram },
	{ 0x3e00, 0x3fff, paletteram_xxxxBBBBGGGGRRRR_split2_w, &paletteram_2 },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sub_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x8000, 0x8fff, dd_spriteram_r },
	{ 0xc000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sub_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x8000, 0x8fff, dd_spriteram_w },
	{ 0xc000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1000, soundlatch_r },
	{ 0x1800, 0x1800, dd_adpcm_status_r },
	{ 0x2800, 0x2801, YM2151_status_port_0_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x2800, 0x2800, YM2151_register_port_0_w },
	{ 0x2801, 0x2801, YM2151_data_port_0_w },
	{ 0x3800, 0x3807, dd_adpcm_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress dd2_sub_readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xcfff, dd_spriteram_r },
	{ 0xd000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress dd2_sub_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, dd_spriteram_w },
	{ 0xd000, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress dd2_sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, YM2151_status_port_0_r },
	{ 0x9800, 0x9800, OKIM6295_status_r },
	{ 0xA000, 0xA000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress dd2_sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, YM2151_register_port_0_w },
	{ 0x8801, 0x8801, YM2151_data_port_0_w },
	{ 0x9800, 0x9800, OKIM6295_data_w },
	{ -1 }	/* end of table */
};

/* bit 0x10 is sprite CPU busy signal */
#define COMMON_PORT4	PORT_START \
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 ) \
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 ) \
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 ) \
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK ) \
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN ) \
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define COMMON_INPUT_PORTS PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_START \
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE ) \
	PORT_DIPSETTING(	0x00, "4 Coins/1 Credit" ) \
	PORT_DIPSETTING(	0x01, "3 Coins/1 Credit" ) \
	PORT_DIPSETTING(	0x02, "2 Coins/1 Credit" ) \
	PORT_DIPSETTING(	0x07, "1 Coin/1 Credit" ) \
	PORT_DIPSETTING(	0x06, "1 Coin/2 Credits" ) \
	PORT_DIPSETTING(	0x05, "1 Coin/3 Credits" ) \
	PORT_DIPSETTING(	0x04, "1 Coin/4 Credits" ) \
	PORT_DIPSETTING(	0x03, "1 Coin/5 Credits" ) \
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE ) \
	PORT_DIPSETTING(	0x00, "4 Coins/1 Credit" ) \
	PORT_DIPSETTING(	0x08, "3 Coins/1 Credit" ) \
	PORT_DIPSETTING(	0x10, "2 Coins/1 Credit" ) \
	PORT_DIPSETTING(	0x38, "1 Coin/1 Credit" ) \
	PORT_DIPSETTING(	0x30, "1 Coin/2 Credits" ) \
	PORT_DIPSETTING(	0x28, "1 Coin/3 Credits" ) \
	PORT_DIPSETTING(	0x20, "1 Coin/4 Credits" ) \
	PORT_DIPSETTING(	0x18, "1 Coin/5 Credits" ) \
	PORT_DIPNAME( 0x40, 0x40, "Screen Orientation", IP_KEY_NONE ) \
	PORT_DIPSETTING(	0x00, "On" ) \
	PORT_DIPSETTING(	0x40, "Off") \
	PORT_DIPNAME( 0x80, 0x80, "Screen Reverse", IP_KEY_NONE ) \
	PORT_DIPSETTING(	0x00, "On" ) \
	PORT_DIPSETTING(	0x80, "Off")

INPUT_PORTS_START( dd1_input_ports )
	COMMON_INPUT_PORTS

    PORT_START      /* DSW1 */
    PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
    PORT_DIPSETTING(    0x02, "Easy")
    PORT_DIPSETTING(    0x03, "Normal")
    PORT_DIPSETTING(    0x01, "Hard")
    PORT_DIPSETTING(    0x00, "Very Hard")
    PORT_DIPNAME( 0x04, 0x04, "Attract Mode Sound", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Off" )
    PORT_DIPSETTING(    0x04, "On")
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
    PORT_DIPSETTING(    0x10, "20K")
    PORT_DIPSETTING(    0x00, "40K" )
    PORT_DIPSETTING(    0x30, "30K and every 60K")
    PORT_DIPSETTING(    0x20, "40K and every 80K" )
    PORT_DIPNAME( 0xc0, 0xc0, "Lives", IP_KEY_NONE )
    PORT_DIPSETTING(    0xc0, "2")
    PORT_DIPSETTING(    0x80, "3" )
    PORT_DIPSETTING(    0x40, "4")
    PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )

    COMMON_PORT4
INPUT_PORTS_END

INPUT_PORTS_START( dd2_input_ports )
	COMMON_INPUT_PORTS

  PORT_START      /* DSW1 */
    PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
    PORT_DIPSETTING(    0x02, "Easy")
    PORT_DIPSETTING(    0x03, "Normal")
    PORT_DIPSETTING(    0x01, "Medium")
    PORT_DIPSETTING(    0x00, "Hard")
    PORT_DIPNAME( 0x04, 0x04, "Attract Mode Sound", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Off" )
    PORT_DIPSETTING(    0x04, "On")
    PORT_DIPNAME( 0x08, 0x08, "Hurricane Kick", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Easy" )
    PORT_DIPSETTING(    0x08, "Normal")
    PORT_DIPNAME( 0x30, 0x30, "Timer", IP_KEY_NONE )
    PORT_DIPSETTING(    0x20, "80" )
    PORT_DIPSETTING(    0x30, "70")
    PORT_DIPSETTING(    0x10, "65")
    PORT_DIPSETTING(    0x00, "60" )
    PORT_DIPNAME( 0xc0, 0xc0, "Lives", IP_KEY_NONE )
    PORT_DIPSETTING(    0xc0, "1")
    PORT_DIPSETTING(    0x80, "2" )
    PORT_DIPSETTING(    0x40, "3")
    PORT_DIPSETTING(    0x00, "4")

	COMMON_PORT4
INPUT_PORTS_END

#undef COMMON_INPUT_PORTS
#undef COMMON_PORT4

#define CHAR_LAYOUT( name, num ) \
	static struct GfxLayout name = \
	{ \
		8,8, /* 8*8 chars */ \
		num, /* 'num' characters */ \
		4, /* 4 bits per pixel */ \
		{ 0, 2, 4, 6 }, /* plane offset */ \
		{ 1, 0, 65, 64, 129, 128, 193, 192 }, \
		{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },	\
		32*8 /* every char takes 32 consecutive bytes */ \
	};

#define TILE_LAYOUT( name, num, planeoffset ) \
	static struct GfxLayout name = \
	{ \
		16,16, /* 16x16 chars */ \
		num, /* 'num' characters */ \
		4, /* 4 bits per pixel */ \
		{ planeoffset*8+0, planeoffset*8+4, 0,4 }, /* plane offset */ \
		{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0, \
	          32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 }, \
		{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, \
	          8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 }, \
		64*8 /* every char takes 64 consecutive bytes */ \
	};

CHAR_LAYOUT( char_layout, 1024 ) /* foreground chars */
TILE_LAYOUT( tile_layout, 2048, 0x20000 ) /* background tiles */
TILE_LAYOUT( sprite_layout, 2048*2, 0x40000 ) /* sprites */

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0xc0000, &char_layout,	    0, 8 },	/* 8x8 text */
	{ 1, 0x40000, &sprite_layout, 128, 8 },	/* 16x16 sprites */
	{ 1, 0x00000, &tile_layout,	  256, 8 },		/* 16x16 background tiles */
	{ -1 }
};

CHAR_LAYOUT( dd2_char_layout, 2048 ) /* foreground chars */
TILE_LAYOUT( dd2_sprite_layout, 2048*3, 0x60000 ) /* sprites */

/* background tiles encoding for dd2 is the same as dd1 */

static struct GfxDecodeInfo dd2_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &dd2_char_layout,	    0, 8 },	/* 8x8 chars */
	{ 1, 0x50000, &dd2_sprite_layout, 128, 8 },	/* 16x16 sprites */
	{ 1, 0x10000, &tile_layout,       256, 8 },         /* 16x16 background tiles */
	{ -1 } // end of array
};

static void dd_irq_handler(void) {
	cpu_cause_interrupt( 2, ym_irq );
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3582071,	/* seems to be the standard */
	{ 60 },
	{ dd_irq_handler }
};

static struct ADPCMinterface adpcm_interface =
{
	2,			/* 2 channels */
	8000,       /* 8000Hz playback */
	4,			/* memory region 4 */
	0,			/* init function */
	{ 50, 50 }
};

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	8000,           /* frequency (Hz) */
	4,              /* memory region */
	{ 15 }
};

static int dd_interrupt(void)
{
	return ( M6809_INT_FIRQ | M6809_INT_NMI );
}



static struct MachineDriver ddragon_machine_driver =
{
	/* basic machine hardware */
	{
		{
 			CPU_M6309,
			3579545,	/* 3.579545 Mhz */
			0,
			readmem,writemem,0,0,
			dd_interrupt,1
		},
		{
 			CPU_HD63701, /* we're missing the code for this one */
			12000000 / 3, /* 4 Mhz */
			2,
			sub_readmem,sub_writemem,0,0,
			ignore_interrupt,0
		},
		{
 			CPU_M6809 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 Mhz */
			3,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0 /* irq on command */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	100, /* heavy interleaving to sync up sprite<->main cpu's */
	dd_init_machine,

	/* video hardware */
	32*8, 32*8,{ 1*8, 31*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	384, 384,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dd_vh_start,
	dd_vh_stop,
	dd_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151_ALT,
			&ym2151_interface
		},
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};

static struct MachineDriver ddragonb_machine_driver =
{
	/* basic machine hardware */
	{
		{
 			CPU_M6309,
			3579545,	/* 3.579545 Mhz */
			0,
			readmem,writemem,0,0,
			dd_interrupt,1
		},
		{
 			CPU_M6809,
			12000000 / 3, /* 4 Mhz */
			2,
			sub_readmem,sub_writemem,0,0,
			ignore_interrupt,0
		},
		{
 			CPU_M6809 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 Mhz */
			3,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0 /* irq on command */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	100, /* heavy interleaving to sync up sprite<->main cpu's */
	dd_init_machine,

	/* video hardware */
	32*8, 32*8,{ 1*8, 31*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	384, 384,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dd_vh_start,
	dd_vh_stop,
	dd_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151_ALT,
			&ym2151_interface
		},
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};

static struct MachineDriver ddragon2_machine_driver =
{
	/* basic machine hardware */
	{
		{
 			CPU_M6309,
			3579545,	/* 3.579545 Mhz */
			0,
			readmem,dd2_writemem,0,0,
			dd_interrupt,1
		},
		{
			CPU_Z80,
			12000000 / 3, /* 4 Mhz */
			2,	/* memory region */
			dd2_sub_readmem,dd2_sub_writemem,0,0,
			ignore_interrupt,0
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 Mhz */
			3,	/* memory region */
			dd2_sound_readmem,dd2_sound_writemem,0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	100, /* heavy interleaving to sync up sprite<->main cpu's */
	dd2_init_machine,

	/* video hardware */
	32*8, 32*8,{ 1*8, 31*8-1, 2*8, 30*8-1 },
	dd2_gfxdecodeinfo,
	384, 384,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dd_vh_start,
	dd_vh_stop,
	dd_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151_ALT,
			&ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/


ROM_START( ddragon_rom )
	ROM_REGION(0x28000)	/* 64k for code + bankswitched memory */
	ROM_LOAD( "a_m2_d02.bin", 0x08000, 0x08000, 0x668dfa19 )
	ROM_LOAD( "a_k2_d03.bin", 0x10000, 0x08000, 0x5779705e ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "a_h2_d04.bin", 0x18000, 0x08000, 0x3bdea613 ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "a_g2_d05.bin", 0x20000, 0x08000, 0x728f87b9 ) /* banked at 0x4000-0x8000 */

	ROM_REGION_DISPOSE(0xC8000) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a_a2_d06.bin", 0xC0000, 0x08000, 0x7a8b8db4 ) /* 0,1,2,3 */ /* text */
	ROM_LOAD( "b_c5_d09.bin", 0x00000, 0x10000, 0x7c435887 ) /* 0,1 */ /* tiles */
	ROM_LOAD( "b_a5_d10.bin", 0x10000, 0x10000, 0xc6640aed ) /* 0,1 */ /* tiles */
	ROM_LOAD( "b_c7_d19.bin", 0x20000, 0x10000, 0x5effb0a0 ) /* 2,3 */ /* tiles */
	ROM_LOAD( "b_a7_d20.bin", 0x30000, 0x10000, 0x5fb42e7c ) /* 2,3 */ /* tiles */
	ROM_LOAD( "b_r7_d11.bin", 0x40000, 0x10000, 0x574face3 ) /* 0,1 */ /* sprites */
	ROM_LOAD( "b_p7_d12.bin", 0x50000, 0x10000, 0x40507a76 ) /* 0,1 */ /* sprites */
	ROM_LOAD( "b_m7_d13.bin", 0x60000, 0x10000, 0xbb0bc76f ) /* 0,1 */ /* sprites */
	ROM_LOAD( "b_l7_d14.bin", 0x70000, 0x10000, 0xcb4f231b ) /* 0,1 */ /* sprites */
	ROM_LOAD( "b_j7_d15.bin", 0x80000, 0x10000, 0xa0a0c261 ) /* 2,3 */ /* sprites */
	ROM_LOAD( "b_h7_d16.bin", 0x90000, 0x10000, 0x6ba152f6 ) /* 2,3 */ /* sprites */
	ROM_LOAD( "b_f7_d17.bin", 0xA0000, 0x10000, 0x3220a0b6 ) /* 2,3 */ /* sprites */
	ROM_LOAD( "b_d7_d18.bin", 0xB0000, 0x10000, 0x65c7517d ) /* 2,3 */ /* sprites */

	ROM_REGION(0x10000) /* sprite cpu */
	/* missing mcu code */
	/* currently load the audio cpu code in this location */
	/* because otherwise mame will loop indefinately in cpu_run */
	ROM_LOAD( "a_s2_d01.bin", 0x08000, 0x08000, 0x9efa95bb )

	ROM_REGION(0x10000) /* audio cpu */
	ROM_LOAD( "a_s2_d01.bin", 0x08000, 0x08000, 0x9efa95bb )

	ROM_REGION(0x20000) /* adpcm samples */
	ROM_LOAD( "a_s6_d07.bin", 0x00000, 0x10000, 0x34755de3 )
	ROM_LOAD( "a_r6_d08.bin", 0x10000, 0x10000, 0x904de6f8 )
ROM_END

ROM_START( ddragonb_rom )
	ROM_REGION(0x28000)	/* 64k for code + bankswitched memory */
	ROM_LOAD( "ic26",         0x08000, 0x08000, 0xae714964 )
	ROM_LOAD( "a_k2_d03.bin", 0x10000, 0x08000, 0x5779705e ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "ic24",         0x18000, 0x08000, 0xdbf24897 ) /* banked at 0x4000-0x8000 */
	ROM_LOAD( "ic23",         0x20000, 0x08000, 0x6c9f46fa ) /* banked at 0x4000-0x8000 */

	ROM_REGION_DISPOSE(0xc8000) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a_a2_d06.bin", 0xC0000, 0x08000, 0x7a8b8db4 ) /* 0,1,2,3 */ /* text */
	ROM_LOAD( "b_c5_d09.bin", 0x00000, 0x10000, 0x7c435887 ) /* 0,1 */ /* tiles */
	ROM_LOAD( "b_a5_d10.bin", 0x10000, 0x10000, 0xc6640aed ) /* 0,1 */ /* tiles */
	ROM_LOAD( "b_c7_d19.bin", 0x20000, 0x10000, 0x5effb0a0 ) /* 2,3 */ /* tiles */
	ROM_LOAD( "b_a7_d20.bin", 0x30000, 0x10000, 0x5fb42e7c ) /* 2,3 */ /* tiles */
	ROM_LOAD( "b_r7_d11.bin", 0x40000, 0x10000, 0x574face3 ) /* 0,1 */ /* sprites */
	ROM_LOAD( "b_p7_d12.bin", 0x50000, 0x10000, 0x40507a76 ) /* 0,1 */ /* sprites */
	ROM_LOAD( "b_m7_d13.bin", 0x60000, 0x10000, 0xbb0bc76f ) /* 0,1 */ /* sprites */
	ROM_LOAD( "b_l7_d14.bin", 0x70000, 0x10000, 0xcb4f231b ) /* 0,1 */ /* sprites */
	ROM_LOAD( "b_j7_d15.bin", 0x80000, 0x10000, 0xa0a0c261 ) /* 2,3 */ /* sprites */
	ROM_LOAD( "b_h7_d16.bin", 0x90000, 0x10000, 0x6ba152f6 ) /* 2,3 */ /* sprites */
	ROM_LOAD( "b_f7_d17.bin", 0xA0000, 0x10000, 0x3220a0b6 ) /* 2,3 */ /* sprites */
	ROM_LOAD( "b_d7_d18.bin", 0xB0000, 0x10000, 0x65c7517d ) /* 2,3 */ /* sprites */

	ROM_REGION(0x10000) /* sprite cpu */
	ROM_LOAD( "ic38",         0x0c000, 0x04000, 0x6a6a0325 )

	ROM_REGION(0x10000) /* audio cpu */
	ROM_LOAD( "a_s2_d01.bin", 0x08000, 0x08000, 0x9efa95bb )

	ROM_REGION(0x20000) /* adpcm samples */
	ROM_LOAD( "a_s6_d07.bin", 0x00000, 0x10000, 0x34755de3 )
	ROM_LOAD( "a_r6_d08.bin", 0x10000, 0x10000, 0x904de6f8 )
ROM_END

ROM_START( ddragon2_rom )
	ROM_REGION(0x28000)	/* region#0: 64k for code */
	ROM_LOAD( "26a9-04.bin",  0x08000, 0x8000, 0xf2cfc649 )
	ROM_LOAD( "26aa-03.bin",  0x10000, 0x8000, 0x44dd5d4b )
	ROM_LOAD( "26ab-0.bin",   0x18000, 0x8000, 0x49ddddcd )
	ROM_LOAD( "26ac-02.bin",  0x20000, 0x8000, 0x097eaf26 )

	ROM_REGION_DISPOSE(0x110000) /* region#1: graphics (disposed after conversion) */
	ROM_LOAD( "26a8-0.bin",   0x00000, 0x10000, 0x3ad1049c ) /* 0,1,2,3 */ /* text */
    ROM_LOAD( "26j4-0.bin",   0x10000, 0x20000, 0xa8c93e76 ) /* 0,1 */ /* tiles */
    ROM_LOAD( "26j5-0.bin",   0x30000, 0x20000, 0xee555237 ) /* 2,3 */ /* tiles */
    ROM_LOAD( "26j0-0.bin",   0x50000, 0x20000, 0xdb309c84 ) /* 0,1 */ /* sprites */
    ROM_LOAD( "26j1-0.bin",   0x70000, 0x20000, 0xc3081e0c ) /* 0,1 */ /* sprites */
	ROM_LOAD( "26af-0.bin",   0x90000, 0x20000, 0x3a615aad ) /* 0,1 */ /* sprites */
	ROM_LOAD( "26j2-0.bin",   0xb0000, 0x20000, 0x589564ae ) /* 2,3 */ /* sprites */
	ROM_LOAD( "26j3-0.bin",   0xd0000, 0x20000, 0xdaf040d6 ) /* 2,3 */ /* sprites */
	ROM_LOAD( "26a10-0.bin",  0xf0000, 0x20000, 0x6d16d889 ) /* 2,3 */ /* sprites */

	ROM_REGION(0x10000) /* region#2: sprite CPU 64kb (Upper 16kb = 0) */
    ROM_LOAD( "26ae-0.bin",   0x00000, 0x10000, 0xea437867 )

	ROM_REGION(0x10000) /* region#3: music CPU, 64kb */
    ROM_LOAD( "26ad-0.bin",   0x00000, 0x8000, 0x75e36cd6 )

	ROM_REGION(0x40000) /* region#4: adpcm */
    ROM_LOAD( "26j6-0.bin",   0x00000, 0x20000, 0xa84b2a29 )
    ROM_LOAD( "26j7-0.bin",   0x20000, 0x20000, 0xbc6a48d5 )
ROM_END



static int ddragonb_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((RAM[0x0e73] == 0x02) && (RAM[0x0e76] == 0x27) && (RAM[0x0023] == 0x02))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0e73],6*5);
			RAM[0x0023] = RAM[0x0e73];
			RAM[0x0024] = RAM[0x0e74];
			RAM[0x0025] = RAM[0x0e75];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void ddragonb_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0e73],6*5);
		osd_fclose(f);
	}
}


static int ddragon2_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((RAM[0x0f91] == 0x02) && (RAM[0x0f94] == 0x25) && (RAM[0x0023] == 0x02))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0f91],6*5);
			RAM[0x0023] = RAM[0x0f91];
			RAM[0x0024] = RAM[0x0f92];
			RAM[0x0025] = RAM[0x0f93];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void ddragon2_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0f91],6*5);
		osd_fclose(f);
	}

}



struct GameDriver ddragon_driver =
{
	__FILE__,
	0,
	"ddragon",
	"Double Dragon",
	"1987",
	"bootleg?",
	"Carlos A. Lozano\nRob Rosenbrock\nChris Moore\nPhil Stroffolino\nErnesto Corvi",
	GAME_NOT_WORKING,
	&ddragon_machine_driver,
	0,

	ddragon_rom,
	0, 0,
	0,
	0,

	dd1_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	ddragonb_hiload, ddragonb_hisave
};

struct GameDriver ddragonb_driver =
{
	__FILE__,
	&ddragon_driver,
	"ddragonb",
	"Double Dragon (bootleg)",
	"1987",
	"bootleg",
	"Carlos A. Lozano\nRob Rosenbrock\nChris Moore\nPhil Stroffolino\nErnesto Corvi\n",
	0,
	&ddragonb_machine_driver,
	0,

	ddragonb_rom,
	0, 0,
	0,
	0,

	dd1_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	ddragonb_hiload, ddragonb_hisave
};

struct GameDriver ddragon2_driver =
{
	__FILE__,
	0,
	"ddragon2",
	"Double Dragon 2",
	"1988",
	"Technos",
	"Carlos A. Lozano\nRob Rosenbrock\nPhil Stroffolino\nErnesto Corvi\n",
	0,
	&ddragon2_machine_driver,
	0,

	ddragon2_rom,
	0, 0,
	0,
	0,

	dd2_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	ddragon2_hiload, ddragon2_hisave
};
