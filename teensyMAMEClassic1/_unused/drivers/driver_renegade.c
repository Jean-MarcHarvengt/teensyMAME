/***************************************************************************

Renegade (preliminary)
(c)1986 Taito

Nekketsu Kouha Kunio Kun (bootleg)
(c)1986 TECHNOS JAPAN

By Carlos A. Lozano & Rob Rosenbrock & Phil Stroffolino

To enter test mode, hold down Player#1 and Player#2 start buttons as the game starts up.

to do:
	68705 emulation (not needed for bootleg)
	cocktail mode not yet supported
	sprites are visible on high score screen
	stage 3 boss's lower body is garbled when she is knocked down

Misc Info:

NMI is used to refresh the sprites
IRQ is used to handle coin inputs

Memory Map (Preliminary):

Working RAM
  $24		used to mirror bankswitch state
  $26		#credits (decimal)
  $2C -  $2D	sprite refresh trigger (used by NMI)
  $31		live/demo (if live, player controls are read from input ports)
  $32		indicates 2 player (alternating) game, or 1 player game
  $33		active player
  $37		stage number
  $38		stage state (for stages with more than one part)
  $40		game status flags; 0x80 indicates time over, 0x40 indicates player dead
 $220		player health
 $222 -  $223	stage timer
 $48a -  $48b	horizontal scroll buffer
 $511 -  $690	sprite RAM buffer
$1002		#lives
$1014 - $1015	stage timer - separated digits
$1017 - $1019	stage timer: (ticks,seconds,minutes)
$1020 - $1048	high score table
$10e5 - $10ff	68705 data buffer

Video RAM
$1800 - $1bff	Text Layer, characters
$1c00 - $1fff	Text Layer, character attributes
$2000 - $217f	MIX RAM (96 sprites)
$2800 - $2bff	BACK LOW MAP RAM (background tiles)
$2C00 - $2fff	BACK HIGH MAP RAM (background attributes)
$3000 - $30ff	COLOR RG RAM
$3100 - $31ff	COLOR B RAM

Registers
$3800w	scroll(0ff)
$3801w	scroll(300)
$3802w	adpcm out?
$3803w	interrupt enable

$3804w	send data from 68705
		sets a flip flop
		clears bit 0 of port C
		generates an interrupt
	a read from (?) sets bit 1 of port C
$3804r	receive data from 68705
$3805w	bankswitch
$3806w	watchdog?
$3807w	?
$3800r	'player1'
		xx		start buttons
		  xx		fire buttons
		    xxxx	joystick state

$3801r	'player2'
		xx		coin inputs
		  xx		fire buttons
		    xxxx	joystick state

$3802r	'DIP2'
		x		unused?
		 x		vblank
		  x		0: 68705 is ready to send information
		   x		1: 68705 is ready to receive information
		    xx		3rd fire buttons for player 2,1
		      xx	difficulty

$3803r 'DIP1'
		x		screen flip
		 x		cabinet type
		  x		bonus (extra life for high score)
		   x		starting lives: 1 or 2
		    xxxx	coins per play

ROM
$4000 - $7fff	bankswitched ROM
$8000 - $ffff	ROM

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6502/m6502.h"
#include "m6809/m6809.h"

static void Write68705( int data );
static void Write68705( int data ){
}

static int Read68705( void );
static int Read68705( void ){
	return 0xda;
}

extern void renegade_vh_screenrefresh(struct osd_bitmap *bitmap, int fullrefresh);
extern int renegade_vh_start( void );
extern void renegade_vh_stop( void );

extern unsigned char *renegade_textram;
extern int renegade_scrollx;
extern int renegade_video_refresh;

static int interrupt_enable = 1;
static unsigned char renegade_waitIRQsound = 0;
static int bank = 0;

static void adpcm_play_w( int offset,int data );
static void adpcm_play_w( int offset,int data ){
	ADPCM_play( 0, 0x2000*(data-0x2c), 0x2000 );
}

static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 channel */
	8000,       		/* 8000Hz playback */
	3,			/* memory region */
	0,			/* init function */
	{ 255 }			/* volume? */
};


void renegade_register_w( int offset, int data );
void renegade_register_w( int offset, int data ){
	switch( offset ){
		case 0x0:
		renegade_scrollx = (renegade_scrollx&0xff00)|data;
		break;

		case 0x1:
		renegade_scrollx = (renegade_scrollx&0xFF)|(data<<8);
		break;

		case 0x2:
		renegade_waitIRQsound = 1;
		soundlatch_w(offset,data);
		break;

		case 0x3:
		interrupt_enable = data;
		break;

		case 0x4:
		Write68705( data );
		break;

		case 0x5: /* bankswitch */
		if( (data&1)!=bank ){
			unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
			bank = data&1;
			cpu_setbank(1,&RAM[ bank?0x10000:0x4000 ]);
		}
		break;

		case 0x6: /* unknown - watchdog? */
		case 0x7:
		break;
	}
}

int renegade_register_r( int offset );
int renegade_register_r( int offset ){
	switch( offset ){
		case 0: return input_port_0_r(0);	/* Player#1 controls */
		case 1: return input_port_1_r(0);	/* Player#2 controls */
		case 2: return input_port_2_r(0);	/* DIP2 ; various IO ports */
		case 3: return input_port_3_r(0);	/* DIP1 */
		case 4: return Read68705();
	}
	return 0;
}

static struct MemoryReadAddress main_readmem[] = {
	{ 0x0000, 0x37ff, MRA_RAM },
	{ 0x3800, 0x380f, renegade_register_r },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress main_writemem[] =
{
	{ 0x0000, 0x17ff, MWA_RAM },						/* Working RAM */
	{ 0x1800, 0x1fff, MWA_RAM, &renegade_textram },		/* Text Layer */
	{ 0x2000, 0x27ff, MWA_RAM, &spriteram },			/* MIX RAM (sprites) */
	{ 0x2800, 0x2bff, MWA_RAM, &videoram },				/* BACK LOW MAP RAM */
	{ 0x2C00, 0x2fff, MWA_RAM }, 						/* BACK HIGH MAP RAM */
	{ 0x3000, 0x30ff, paletteram_xxxxBBBBGGGGRRRR_split1_w, &paletteram },
	{ 0x3100, 0x31ff, paletteram_xxxxBBBBGGGGRRRR_split2_w, &paletteram_2 },
	{ 0x3800, 0x380f, renegade_register_w },

	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }
};

static int renegade_interrupt(void);
static int renegade_interrupt(void){
	static int last = 0xC0;
	int current = readinputport(1) & 0xC0;

	static int count = 0;
	count = 1-count;

	/* IRQ is used to handle coin insertion */
	if( count && current!=last ){
		last = current;
		if( current!=0xC0 ) return interrupt();
	}

	if( count && interrupt_enable ) return nmi_interrupt();

	return ignore_interrupt();
}

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1000, soundlatch_r },
	{ 0x2801, 0x2801, YM3526_status_port_0_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },

	/* 0x1800 and 0x3000 also appear to be adpcm-related */
	{ 0x2000, 0x2000, adpcm_play_w },

	{ 0x2800, 0x2800, YM3526_control_port_0_w },
	{ 0x2801, 0x2801, YM3526_write_port_0_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }
};


static int renegade_sound_interrupt(void){
	if (renegade_waitIRQsound){
		renegade_waitIRQsound = 0;
		return (M6809_INT_IRQ);
	}
	return (M6809_INT_FIRQ);
}



INPUT_PORTS_START( input_ports )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* attack left */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* jump */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START /* DIP2 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING( 0x00, "Hard" )
	PORT_DIPSETTING( 0x01, "Very Hard" )
	PORT_DIPSETTING( 0x02, "Easy" )
	PORT_DIPSETTING( 0x03, "Normal" )

	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* attack right */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED ) 	/* 68705 status */
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) 	/* 68705 status */
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DIP1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(	0x01, "1 Coin/3 Credits" )
	PORT_DIPSETTING(	0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(	0x03, "1 Coin/1 Credits" )

	PORT_DIPNAME( 0x0C, 0x0C, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(	0x04, "1 Coin/3 Credits" )
	PORT_DIPSETTING(	0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(	0x0C, "1 Coin/1 Credits" )

 	PORT_DIPNAME (0x10, 0x10, "Lives", IP_KEY_NONE )
 	PORT_DIPSETTING (   0x10, "1" )
 	PORT_DIPSETTING (   0x00, "2" )

 	PORT_DIPNAME (0x20, 0x20, "Bonus", IP_KEY_NONE )
 	PORT_DIPSETTING (   0x20, "30k" )
 	PORT_DIPSETTING (   0x00, "None" )

 	PORT_DIPNAME (0x40, 0x40, "Cabinet Type", IP_KEY_NONE )
 	PORT_DIPSETTING (   0x40, "Table" )
 	PORT_DIPSETTING (   0x00, "Upright" )

 	PORT_DIPNAME (0x80, 0x80, "Video Flip", IP_KEY_NONE )
 	PORT_DIPSETTING (   0x80, "Off" )
 	PORT_DIPSETTING (   0x00, "On" )
INPUT_PORTS_END

static struct GfxLayout charlayout1 =
{
	8,8, /* 8x8 characters */
	1024, /* 1024 characters */
	3, /* bits per pixel */
	{ 2, 4, 6 },	/* plane offsets; bit 0 is always clear */
	{ 1, 0, 65, 64, 129, 128, 193, 192 }, /* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 }, /* y offsets */
	32*8 /* offset to next character */
};
static struct GfxLayout charlayout =
{
	8,8, /* 8x8 characters */
	1024, /* 1024 characters */
	3, /* bits per pixel */
	{ 2, 4, 6 },	/* plane offsets; bit 0 is always clear */
	{ 1, 0, 65, 64, 129, 128, 193, 192 }, /* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 }, /* y offsets */
	32*8 /* offset to next character */
};

static struct GfxLayout tileslayout1 =
{
	16,16, /* tile size */
	256, /* number of tiles */
	3, /* bits per pixel */

	/* plane offsets */
	{ 4, 0x8000*8+0, 0x8000*8+4 },

	/* x offsets */
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
		32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 },

	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },

	64*8 /* offset to next tile */
};

static struct GfxLayout tileslayout2 =
{
	16,16, /* tile size */
	256, /* number of tiles */
	3, /* bits per pixel */

	/* plane offsets */
	{ 0, 0xC000*8+0, 0xC000*8+4 },

	/* x offsets */
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
		32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 },

	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },

	64*8 /* offset to next tile */
};

static struct GfxLayout tileslayout3 =
{
	16,16, /* tile size */
	256, /* number of tiles */
	3, /* bits per pixel */

	/* plane offsets */
	{ 0x4000*8+4, 0x10000*8+0, 0x10000*8+4 },

	/* x offsets */
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
		32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 },

	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },

	64*8 /* offset to next tile */
};

static struct GfxLayout tileslayout4 =
{
	16,16, /* tile size */
	256, /* number of tiles */
	3, /* bits per pixel */

	/* plane offsets */
	{ 0x4000*8+0, 0x14000*8+0, 0x14000*8+4 },

	/* x offsets */
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
		32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 },

	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },

	64*8 /* offset to next tile */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* 8x8 text, 8 colors */
	{ 1, 0x00000, &charlayout,     0, 4 },	/* colors   0- 32 */

	/* 16x16 background tiles, 8 colors */
	{ 1, 0x08000, &tileslayout1, 192, 8 },	/* colors 192-255 */
	{ 1, 0x08000, &tileslayout2, 192, 8 },
	{ 1, 0x08000, &tileslayout3, 192, 8 },
	{ 1, 0x08000, &tileslayout4, 192, 8 },

	{ 1, 0x20000, &tileslayout1, 192, 8 },
	{ 1, 0x20000, &tileslayout2, 192, 8 },
	{ 1, 0x20000, &tileslayout3, 192, 8 },
	{ 1, 0x20000, &tileslayout4, 192, 8 },

	/* 16x16 sprites, 8 colors */
	{ 1, 0x38000, &tileslayout1, 128, 4 },	/* colors 128-159 */
	{ 1, 0x38000, &tileslayout2, 128, 4 },
	{ 1, 0x38000, &tileslayout3, 128, 4 },
	{ 1, 0x38000, &tileslayout4, 128, 4 },

	{ 1, 0x68000, &tileslayout1, 128, 4 },
	{ 1, 0x68000, &tileslayout2, 128, 4 },
	{ 1, 0x68000, &tileslayout3, 128, 4 },
	{ 1, 0x68000, &tileslayout4, 128, 4 },

	{ 1, 0x50000, &tileslayout1, 128, 4 },
	{ 1, 0x50000, &tileslayout2, 128, 4 },
	{ 1, 0x50000, &tileslayout3, 128, 4 },
	{ 1, 0x50000, &tileslayout4, 128, 4 },

	{ 1, 0x80000, &tileslayout1, 128, 4 },
	{ 1, 0x80000, &tileslayout2, 128, 4 },
	{ 1, 0x80000, &tileslayout3, 128, 4 },
	{ 1, 0x80000, &tileslayout4, 128, 4 },
	{ -1 }
};



static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip (no more supported) */
	3250000,	/* 3.25 MHz ? (hand tuned) */
	{ 255 }		/* (not supported) */
};



static struct MachineDriver renegade_machine_driver =
{
	{
		{
 			CPU_M6502,
			3000000,	/* should be 1.5 MHz? */
			0,
			main_readmem,main_writemem,0,0,
			renegade_interrupt,2
		},
		{
 			CPU_M6809 | CPU_AUDIO_CPU,
			1200000,	/* ? */
			2,
			sound_readmem,sound_writemem,0,0,
			renegade_sound_interrupt,16
		}
	},
	60,

	DEFAULT_REAL_60HZ_VBLANK_DURATION*2,

	1, /* cpu slices */
	0, /* init machine */

	32*8, 32*8,
	{ 8, 31*8-1, 0, 30*8-1 },
	gfxdecodeinfo,
	256,256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	renegade_vh_start,
	renegade_vh_stop,
	renegade_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3526,
			&ym3526_interface
		},
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};

ROM_START( renegade_rom )
	ROM_REGION(0x14000)	/* 64k for code + bank switched ROM */
	ROM_LOAD( "nb-5.bin",     0x8000, 0x8000, 0xba683ddf )
	ROM_LOAD( "na-5.bin",     0x4000, 0x4000, 0xde7e7df4 )
	ROM_CONTINUE( 0x10000, 0x4000 )

	ROM_REGION_DISPOSE(0x98000) /* temporary space for graphics (disposed after conversion) */

	/* characters */
	ROM_LOAD( "nc-5.bin",     0x00000, 0x8000, 0x9adfaa5d )	/* 4 bitplanes */

	/* tiles */
	ROM_LOAD( "n1-5.bin",     0x08000, 0x8000, 0x4a9f47f3 )	/* bitplane 3 */
	ROM_LOAD( "n6-5.bin",     0x10000, 0x8000, 0xd62a0aa8 )	/* bitplanes 1,2 */
	ROM_LOAD( "n7-5.bin",     0x18000, 0x8000, 0x7ca5a532 )	/* bitplanes 1,2 */

	ROM_LOAD( "n2-5.bin",     0x20000, 0x8000, 0x8d2e7982 )	/* bitplane 3 */
	ROM_LOAD( "n8-5.bin",     0x28000, 0x8000, 0x0dba31d3 )	/* bitplanes 1,2 */
	ROM_LOAD( "n9-5.bin",     0x30000, 0x8000, 0x5b621b6a )	/* bitplanes 1,2 */

	/* sprites */
	ROM_LOAD( "nh-5.bin",     0x38000, 0x8000, 0xdcd7857c )	/* bitplane 3 */
	ROM_LOAD( "nd-5.bin",     0x40000, 0x8000, 0x2de1717c )	/* bitplanes 1,2 */
	ROM_LOAD( "nj-5.bin",     0x48000, 0x8000, 0x0f96a18e )	/* bitplanes 1,2 */

	ROM_LOAD( "ni-5.bin",     0x50000, 0x8000, 0x6f597ed2 )	/* bitplane 3 */
	ROM_LOAD( "nf-5.bin",     0x58000, 0x8000, 0x0efc8d45 )	/* bitplanes 1,2 */
	ROM_LOAD( "nl-5.bin",     0x60000, 0x8000, 0x14778336 )	/* bitplanes 1,2 */

	ROM_LOAD( "nn-5.bin",     0x68000, 0x8000, 0x1bf15787 )	/* bitplane 3 */
	ROM_LOAD( "ne-5.bin",     0x70000, 0x8000, 0x924c7388 )	/* bitplanes 1,2 */
	ROM_LOAD( "nk-5.bin",     0x78000, 0x8000, 0x69499a94 )	/* bitplanes 1,2 */

	ROM_LOAD( "no-5.bin",     0x80000, 0x8000, 0x147dd23b )	/* bitplane 3 */
	ROM_LOAD( "ng-5.bin",     0x88000, 0x8000, 0xa8ee3720 )	/* bitplanes 1,2 */
	ROM_LOAD( "nm-5.bin",     0x90000, 0x8000, 0xc100258e )	/* bitplanes 1,2 */

	ROM_REGION(0x10000) /* audio CPU (M6809) */
	ROM_LOAD( "n0-5.bin",     0x08000, 0x08000, 0x3587de3b )

	ROM_REGION(0x20000) /* adpcm */
	ROM_LOAD( "n5-5.bin",     0x00000, 0x8000, 0x7ee43a3c )
	ROM_LOAD( "n4-5.bin",     0x10000, 0x8000, 0x6557564c )
	ROM_LOAD( "n3-5.bin",     0x18000, 0x8000, 0x78fd6190 )
ROM_END

ROM_START( kuniokun_rom )
	ROM_REGION(0x14000)	/* 64k for code + bank switched ROM */
	ROM_LOAD( "ta18-10.bin",  0x8000, 0x8000, 0xa90cf44a )
	ROM_LOAD( "ta18-11.bin",  0x4000, 0x4000, 0xf240f5cd )
	ROM_CONTINUE( 0x10000, 0x4000 )

	ROM_REGION_DISPOSE(0x98000) /* temporary space for graphics (disposed after conversion) */

	/* characters */
	ROM_LOAD( "ta18-25.bin",  0x00000, 0x8000, 0x9bd2bea3 )	/* 4 bitplanes */

	ROM_LOAD( "ta18-01.bin",  0x08000, 0x8000, 0xdaf15024 )
	ROM_LOAD( "ta18-06.bin",  0x10000, 0x8000, 0x1f59a248 )
	ROM_LOAD( "ta18-05.bin",  0x18000, 0x8000, 0x7ca5a532 )

	ROM_LOAD( "ta18-02.bin",  0x20000, 0x8000, 0x994c0021 )
	ROM_LOAD( "ta18-04.bin",  0x28000, 0x8000, 0x00000000 )
	ROM_LOAD( "ta18-03.bin",  0x30000, 0x8000, 0x0475c99a )

	/* sprites */
	ROM_LOAD( "ta18-20.bin",  0x38000, 0x8000, 0xc7d54139 )
	ROM_LOAD( "ta18-24.bin",  0x40000, 0x8000, 0x84677d45 )
	ROM_LOAD( "ta18-18.bin",  0x48000, 0x8000, 0x1c770853 )

	ROM_LOAD( "ta18-19.bin",  0x50000, 0x8000, 0xc8795fd7 )
	ROM_LOAD( "ta18-22.bin",  0x58000, 0x8000, 0xdf3a2ff5 )
	ROM_LOAD( "ta18-16.bin",  0x60000, 0x8000, 0x7244bad0 )

	ROM_LOAD( "ta18-14.bin",  0x68000, 0x8000, 0xaf656017 )
	ROM_LOAD( "ta18-23.bin",  0x70000, 0x8000, 0x3fd19cf7 )
	ROM_LOAD( "ta18-17.bin",  0x78000, 0x8000, 0x74c64c6e )

	ROM_LOAD( "ta18-13.bin",  0x80000, 0x8000, 0xb6b14d46 )
	ROM_LOAD( "ta18-21.bin",  0x88000, 0x8000, 0xc95e009b )
	ROM_LOAD( "ta18-15.bin",  0x90000, 0x8000, 0xa5d61d01 )

	ROM_REGION(0x10000) /* audio CPU (M6809) */
	ROM_LOAD( "ta18-00.bin",  0x08000, 0x08000, 0x3587de3b )

	ROM_REGION(0x20000) /* adpcm */
	ROM_LOAD( "ta18-07.bin",  0x00000, 0x8000, 0x02e3f3ed )
	ROM_LOAD( "ta18-08.bin",  0x10000, 0x8000, 0xc9312613 )
	ROM_LOAD( "ta18-09.bin",  0x18000, 0x8000, 0x07ed4705 )
ROM_END

/*
static int hiload(void){
	if( osd_key_pressed( OSD_KEY_L ) ){
		void *f;
		unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x1020],8*5);
			osd_fclose(f);
		}
		return 1;
	}

	return 0;
}

static void hisave(void){
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x1020],8*5);
		osd_fclose(f);
	}
}
*/

struct GameDriver renegade_driver =
{
	__FILE__,
	0,
	"renegade",
	"Renegade (US)",
	"1986",
	"Taito",
	"Phil Stroffolino\nCarlos A. Lozano\nRob Rosenbrock",
	GAME_NOT_WORKING,
	&renegade_machine_driver,
	0,

	renegade_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0,0 /*hiload, hisave*/
};

struct GameDriver kuniokub_driver =
{
	__FILE__,
	&renegade_driver,
	"kuniokub",
	"Nekketsu Kouha Kunio Kun (Jap bootleg)",
	"1986",
	"bootleg",
	"Phil Stroffolino\nCarlos A. Lozano\nRob Rosenbrock",
	0,
	&renegade_machine_driver,
	0,

	kuniokun_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0,0 /*hiload, hisave*/
};
