/****************************************************************************

Kung Fu Master
Data East<Irem, 1984
memory map (Doc by Ishmair)

ROMS mapping

KF1 ?

KF2yKF3   Adpcm  4bit samples

0000h->KF4      Z80   IM 1.   It doesn't use NMIs.
4000h->KF5      Z80


Gfx KF6,7,8->Background CharSet 1024 Chars.3 planes 8 colors.
    KF9,11,14->Sprites 1   ;3 planes
    KF10,12,13->Sprites 2  ;3 planes

    Sprites are in the same easy format that chars.

; Video RAM  (Char window)
D000h-D7FFh character number (0-255)  64x32 Size (two screens for scroll)

                              HFlip
                               |
D800h-DFFFh character ATTR   11000000
                             \/
                             banco
;                            char

;sprites
4096 char sprites (16 veces 256) de 8 colores.


SCROLL ;  Scroll Byte 1  AT (A000h) -> two's complement.
          Scroll Byte 2  AT (B000h) -> This seems to be scroll related,
                                       but i don't know how it works.

SPRITES
           00   01   02   03   04   05   06   07
           |         Y     ?   AnL  AnH  X     ?
           |              YH    |             Parte alta de XH?
           |                         |
           0F color                 03h bank
                                     |
           F0 ? 1 en silvia         40h Flip

All the X and Y are two's complements , i think.

Start at C000h
I use 256 sprites.      Maybe all my sprites handle is wrong.



Ports IN ;                    ;(0=On 1=Off)
              (00) INTERFACE 0          7 6 5 4 3 2 1 0
                                                | | | |
                                                | | | 1Up Start
                                                | | 2Up Start
                                                | Coin
                                                Coin
                                        Fire1
                                        |
              (01) INTERFACE 1          7 6 5 4 3 2 1 0
                                            |   | | | |
                                            |   | | | Right
                                            |   | | Left
                                            |   | Down
                                            |   Up
                                            Fire2

              (02) INTERFACE 2          7 6 5 4 3 2 1 0
                                        2nd player controls

              (03) DSW 1                7 6 5 4 3 2 1 0

              (04) DSW 2                7 6 5 4 3 2 1 0
                                        |
                                        Test Mode

Puertos OUT;  (00)      ;Sound related. (Look at subrutine 0DE5h)
                        ;Bit 7 at 1 , 7 low bits  .. sound number.
                        ;digital sound will be easy to implement.
              (01)       ?


|-------------------------------------------------------------------------|
|                           Kung-Fu Master                                |
|                                                                         |
|                      DIP Switch #1 Settings                             |
|-------------------------------------------------------------------------|
|  Function:                    | SW#1 SW#2 SW#3 SW#4 SW#5 SW#6 SW#7 SW#8 |
|-------------------------------|-----------------------------------------|
|                               |                                         |
| Difficulty:                   |                                         |
|   Easy                        | Off                                     |
|   Difficult                   | On                                      |
| Energy:                       |                                         |
|   Slow                        |      Off                                |
|   Fast                        |      On                                 |
| Fighters:                     |                                         |
|   2                           |           On   Off                      |
|   3                           |           Off  Off                      |
|   4                           |           Off  On                       |
|   5                           |           On   On                       |
|                               |                                         |
|                               |               Coin Mode 2               |
|                               |           (DIP SW2 #3 = "On")           |
|   Selector A:                 |                                         |
|   -----------                 |                                         |
|   1 Coin  1 Credit            |                     Off  Off            |
|   2 Coins 1 Credit            |                     On   Off            |
|   3 Coins 1 Credit            |                     Off  On             |
|   Free Play                   |                     On   On             |
|                               |                                         |
|   Selector B:                 |                                         |
|   -----------                 |                                         |
|   1 Coin  2 Credits           |                               Off  Off  |
|   1 Coin  3 Credits           |                               On   Off  |
|   1 Coin  5 Credits           |                               Off  On   |
|   1 Coin  6 Credits           |                               On   On   |
|                               |               Coin Mode 1               |
|                               |           (DIP SW2 #3 = "Off")          |
|   1 Coin  1 Credit            |                     Off  Off  Off  Off  |
|   2 Coins 1 Credit            |                     On   Off  Off  Off  |
|   3 Coins 1 Credit            |                     Off  On   Off  Off  |
|   4 Coins 1 Credit            |                     On   On   Off  Off  |
|   5 Coins 1 Credit            |                     Off  Off  On   Off  |
|   6 Coins 1 Credit            |                     On   Off  On   Off  |
|                               |                                         |
|   1 Coin  2 Credits           |                     Off  Off  Off  On   |
|   1 Coin  3 Credits           |                     On   Off  Off  On   |
|   1 Coin  4 Credits           |                     Off  On   Off  On   |
|   1 Coin  5 Credits           |                     On   On   Off  On   |
|   1 Coin  6 Credits           |                     Off  Off  On   On   |
|   Free Play                   |                     On   On   On   On   |
|-------------------------------------------------------------------------|
|                                                                         |
|                      DIP Switch #2 Settings                             |
|-------------------------------------------------------------------------|
|  Function:                    | SW#1 SW#2 SW#3 SW#4 SW#5 SW#6 SW#7 SW#8 |
|-------------------------------|-----------------------------------------|
|                               |                                         |
| Picture:                      |                                         |
|   Normal                      | Off                                     |
|   Inverted                    | On                                      |
| Table Type:                   |                                         |
|   Cocktail Table              |      Off                                |
|   Upright                     |      On                                 |
| Coin Mode:                    |                                         |
|   Mode 1                      |           Off                           |
|   Mode 2                      |           On                            |
| Stop Mode:                    |                                         |
|   Without                     |                     Off                 |
|   With                        |                     On                  |
| Hit Mode:                     |                                         |
|   No Hit                      |                               Off       |
|   Hit                         |                               On        |
| Test Mode:                    |                                         |
|   Off                         |                                    Off  |
|   On                          |                                    On   |
|-------------------------------------------------------------------------|

Note #1: SW#4 and SW#6 are supposed to be in the "Off" position.

Note #2: Initiate "Stop Mode" with 2P Start button.
         Start again with 1P Start button.

**************************************************************************/

#include "driver.h"
#include "Z80/Z80.h"
#include "vidhrdw/generic.h"


extern unsigned char *kungfum_scroll_low;
extern unsigned char *kungfum_scroll_high;
void kungfum_flipscreen_w(int offset,int value);
void kungfum_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int kungfum_vh_start(void);
void kungfum_vh_stop(void);
void kungfum_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


extern struct AY8910interface irem_ay8910_interface;
extern struct MSM5205interface irem_msm5205_interface;
void irem_io_w(int offset, int value);
int irem_io_r(int offset);
void irem_sound_cmd_w(int offset, int value);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xd000, 0xdfff, MRA_RAM },         /* Video and Color ram */
	{ 0xe000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xa000, 0xa000, MWA_RAM, &kungfum_scroll_low },
	{ 0xb000, 0xb000, MWA_RAM, &kungfum_scroll_high },
	{ 0xc020, 0xc0df, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd000, 0xd7ff, videoram_w, &videoram, &videoram_size },
	{ 0xd800, 0xdfff, colorram_w, &colorram },
	{ 0xe000, 0xefff, MWA_RAM },
	{ -1 }	/* end of table */
};


static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },   /* coin */
	{ 0x01, 0x01, input_port_1_r },   /* player 1 control */
	{ 0x02, 0x02, input_port_2_r },   /* player 2 control */
	{ 0x03, 0x03, input_port_3_r },   /* DSW 1 */
	{ 0x04, 0x04, input_port_4_r },   /* DSW 2 */
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, irem_sound_cmd_w },
	{ 0x01, 0x01, kungfum_flipscreen_w },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x001f, irem_io_r },
	{ 0x0080, 0x00ff, MRA_RAM },
	{ 0xa000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x001f, irem_io_w },
	{ 0x0080, 0x00ff, MWA_RAM },
	{ 0x0801, 0x0802, MSM5205_data_w },
	{ 0x9000, 0x9000, MWA_NOP },    /* IACK */
	{ 0xa000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
/* Start 1 & 2 also restarts and freezes the game with stop mode on
   and are used in test mode to enter and esc the various tests */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	/* coin input must be active for 19 frames to be consistently recognized */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE,
		"Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 19 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x02, 0x02, "Energy Loss", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	/* TODO: support the different settings which happen in Coin Mode 2 */
	PORT_DIPNAME( 0xf0, 0xf0, "Coinage", IP_KEY_NONE ) /* mapped on coin mode 1 */
	PORT_DIPSETTING(    0xa0, "6 Coins/1 Credit" )
	PORT_DIPSETTING(    0xb0, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0xd0, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xe0, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x70, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x50, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	/* settings 0x10, 0x20, 0x80, 0x90 all give 1 Coin/1 Credit */

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )
/* This activates a different coin mode. Look at the dip switch setting schematic */
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	/* In slowmo mode, press 2 to slow game speed */
	PORT_BITX   ( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Slow Motion Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* In level selection mode, press 1 to select and 2 to restart */
	PORT_BITX   ( 0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Level Selection Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 2 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	1024,	/* 1024 sprites */
	3,	/* 3 bits per pixel */
	{ 2*1024*32*8, 1024*32*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8},
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,      0, 32 },	/* use colors   0-255 */
	{ 1, 0x06000, &spritelayout, 32*8, 32 },	/* use colors 256-511 */
	{ -1 } /* end of array */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			readmem,writemem,readport, writeport,
			interrupt,1
		},
		{
			CPU_M6803 | CPU_AUDIO_CPU,
			1000000,	/* 1.0 Mhz ? */
			3,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are generated by the ADPCM hardware */
		}
	},
	57, 1790,	/* accurate frequency, measured on a Moon Patrol board, is 56.75Hz. */
				/* the Lode Runner manual (similar but different hardware) */
				/* talks about 55Hz and 1790ms vblank duration. */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 31*8-1 },
	gfxdecodeinfo,
	512, 512,
	kungfum_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	kungfum_vh_start,
	kungfum_vh_stop,
	kungfum_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&irem_ay8910_interface
		},
		{
			SOUND_MSM5205,
			&irem_msm5205_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( kungfum_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "a-4e-c.bin",   0x0000, 0x4000, 0xb6e2d083 )
	ROM_LOAD( "a-4d-c.bin",   0x4000, 0x4000, 0x7532918e )

	ROM_REGION_DISPOSE(0x1e000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "g-4c-a.bin",   0x00000, 0x2000, 0x6b2cc9c8 )	/* characters */
	ROM_LOAD( "g-4d-a.bin",   0x02000, 0x2000, 0xc648f558 )
	ROM_LOAD( "g-4e-a.bin",   0x04000, 0x2000, 0xfbe9276e )
	ROM_LOAD( "b-4k-.bin",    0x06000, 0x2000, 0x16fb5150 )	/* sprites */
	ROM_LOAD( "b-4f-.bin",    0x08000, 0x2000, 0x67745a33 )
	ROM_LOAD( "b-4l-.bin",    0x0a000, 0x2000, 0xbd1c2261 )
	ROM_LOAD( "b-4h-.bin",    0x0c000, 0x2000, 0x8ac5ed3a )
	ROM_LOAD( "b-3n-.bin",    0x0e000, 0x2000, 0x28a213aa )
	ROM_LOAD( "b-4n-.bin",    0x10000, 0x2000, 0xd5228df3 )
	ROM_LOAD( "b-4m-.bin",    0x12000, 0x2000, 0xb16de4f2 )
	ROM_LOAD( "b-3m-.bin",    0x14000, 0x2000, 0xeba0d66b )
	ROM_LOAD( "b-4c-.bin",    0x16000, 0x2000, 0x01298885 )
	ROM_LOAD( "b-4e-.bin",    0x18000, 0x2000, 0xc77b87d4 )
	ROM_LOAD( "b-4d-.bin",    0x1a000, 0x2000, 0x6a70615f )
	ROM_LOAD( "b-4a-.bin",    0x1c000, 0x2000, 0x6189d626 )

	ROM_REGION(0x0620)	/* color PROMs */
	ROM_LOAD( "g-1j-.bin",    0x0000, 0x0100, 0x668e6bca )	/* character palette red component */
	ROM_LOAD( "b-1m-.bin",    0x0100, 0x0100, 0x76c05a9c )	/* sprite palette red component */
	ROM_LOAD( "g-1f-.bin",    0x0200, 0x0100, 0x964b6495 )	/* character palette green component */
	ROM_LOAD( "b-1n-.bin",    0x0300, 0x0100, 0x23f06b99 )	/* sprite palette green component */
	ROM_LOAD( "g-1h-.bin",    0x0400, 0x0100, 0x550563e1 )	/* character palette blue component */
	ROM_LOAD( "b-1l-.bin",    0x0500, 0x0100, 0x35e45021 )	/* sprite palette blue component */
	ROM_LOAD( "b-5f-.bin",    0x0600, 0x0020, 0x7a601c3d )	/* sprite height, one entry per 32 */
														/*   sprites. Used at run time! */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "a-3e-.bin",    0xa000, 0x2000, 0x58e87ab0 )	/* samples (ADPCM 4-bit) */
	ROM_LOAD( "a-3f-.bin",    0xc000, 0x2000, 0xc81e31ea )	/* samples (ADPCM 4-bit) */
	ROM_LOAD( "a-3h-.bin",    0xe000, 0x2000, 0xd99fb995 )
ROM_END

ROM_START( kungfud_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "a-4e-d",       0x0000, 0x4000, 0xfc330a46 )
	ROM_LOAD( "a-4d-d",       0x4000, 0x4000, 0x1b2fd32f )

	ROM_REGION_DISPOSE(0x1e000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "g-4c-a.bin",   0x00000, 0x2000, 0x6b2cc9c8 )	/* characters */
	ROM_LOAD( "g-4d-a.bin",   0x02000, 0x2000, 0xc648f558 )
	ROM_LOAD( "g-4e-a.bin",   0x04000, 0x2000, 0xfbe9276e )
	ROM_LOAD( "b-4k-.bin",    0x06000, 0x2000, 0x16fb5150 )	/* sprites */
	ROM_LOAD( "b-4f-.bin",    0x08000, 0x2000, 0x67745a33 )
	ROM_LOAD( "b-4l-.bin",    0x0a000, 0x2000, 0xbd1c2261 )
	ROM_LOAD( "b-4h-.bin",    0x0c000, 0x2000, 0x8ac5ed3a )
	ROM_LOAD( "b-3n-.bin",    0x0e000, 0x2000, 0x28a213aa )
	ROM_LOAD( "b-4n-.bin",    0x10000, 0x2000, 0xd5228df3 )
	ROM_LOAD( "b-4m-.bin",    0x12000, 0x2000, 0xb16de4f2 )
	ROM_LOAD( "b-3m-.bin",    0x14000, 0x2000, 0xeba0d66b )
	ROM_LOAD( "b-4c-.bin",    0x16000, 0x2000, 0x01298885 )
	ROM_LOAD( "b-4e-.bin",    0x18000, 0x2000, 0xc77b87d4 )
	ROM_LOAD( "b-4d-.bin",    0x1a000, 0x2000, 0x6a70615f )
	ROM_LOAD( "b-4a-.bin",    0x1c000, 0x2000, 0x6189d626 )

	ROM_REGION(0x0620)	/* color PROMs */
	ROM_LOAD( "g-1j-.bin",    0x0000, 0x0100, 0x668e6bca )	/* character palette red component */
	ROM_LOAD( "b-1m-.bin",    0x0100, 0x0100, 0x76c05a9c )	/* sprite palette red component */
	ROM_LOAD( "g-1f-.bin",    0x0200, 0x0100, 0x964b6495 )	/* character palette green component */
	ROM_LOAD( "b-1n-.bin",    0x0300, 0x0100, 0x23f06b99 )	/* sprite palette green component */
	ROM_LOAD( "g-1h-.bin",    0x0400, 0x0100, 0x550563e1 )	/* character palette blue component */
	ROM_LOAD( "b-1l-.bin",    0x0500, 0x0100, 0x35e45021 )	/* sprite palette blue component */
	ROM_LOAD( "b-5f-.bin",    0x0600, 0x0020, 0x7a601c3d )	/* sprite height, one entry per 32 */
														/*   sprites. Used at run time! */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "a-3e-.bin",    0xa000, 0x2000, 0x58e87ab0 )	/* samples (ADPCM 4-bit) */
	ROM_LOAD( "a-3f-.bin",    0xc000, 0x2000, 0xc81e31ea )	/* samples (ADPCM 4-bit) */
	ROM_LOAD( "a-3h-.bin",    0xe000, 0x2000, 0xd99fb995 )
ROM_END

ROM_START( kungfub_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "c5.5h",        0x0000, 0x4000, 0x5d8e791d )
	ROM_LOAD( "c4.5k",        0x4000, 0x4000, 0x4000e2b8 )

	ROM_REGION_DISPOSE(0x1e000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "g-4c-a.bin",   0x00000, 0x2000, 0x6b2cc9c8 )	/* characters */
	ROM_LOAD( "g-4d-a.bin",   0x02000, 0x2000, 0xc648f558 )
	ROM_LOAD( "g-4e-a.bin",   0x04000, 0x2000, 0xfbe9276e )
	ROM_LOAD( "b-4k-.bin",    0x06000, 0x2000, 0x16fb5150 )	/* sprites */
	ROM_LOAD( "b-4f-.bin",    0x08000, 0x2000, 0x67745a33 )
	ROM_LOAD( "b-4l-.bin",    0x0a000, 0x2000, 0xbd1c2261 )
	ROM_LOAD( "b-4h-.bin",    0x0c000, 0x2000, 0x8ac5ed3a )
	ROM_LOAD( "b-3n-.bin",    0x0e000, 0x2000, 0x28a213aa )
	ROM_LOAD( "b-4n-.bin",    0x10000, 0x2000, 0xd5228df3 )
	ROM_LOAD( "b-4m-.bin",    0x12000, 0x2000, 0xb16de4f2 )
	ROM_LOAD( "b-3m-.bin",    0x14000, 0x2000, 0xeba0d66b )
	ROM_LOAD( "b-4c-.bin",    0x16000, 0x2000, 0x01298885 )
	ROM_LOAD( "b-4e-.bin",    0x18000, 0x2000, 0xc77b87d4 )
	ROM_LOAD( "b-4d-.bin",    0x1a000, 0x2000, 0x6a70615f )
	ROM_LOAD( "b-4a-.bin",    0x1c000, 0x2000, 0x6189d626 )

	ROM_REGION(0x0620)	/* color PROMs */
	ROM_LOAD( "g-1j-.bin",    0x0000, 0x0100, 0x668e6bca )	/* character palette red component */
	ROM_LOAD( "b-1m-.bin",    0x0100, 0x0100, 0x76c05a9c )	/* sprite palette red component */
	ROM_LOAD( "g-1f-.bin",    0x0200, 0x0100, 0x964b6495 )	/* character palette green component */
	ROM_LOAD( "b-1n-.bin",    0x0300, 0x0100, 0x23f06b99 )	/* sprite palette green component */
	ROM_LOAD( "g-1h-.bin",    0x0400, 0x0100, 0x550563e1 )	/* character palette blue component */
	ROM_LOAD( "b-1l-.bin",    0x0500, 0x0100, 0x35e45021 )	/* sprite palette blue component */
	ROM_LOAD( "b-5f-.bin",    0x0600, 0x0020, 0x7a601c3d )	/* sprite height, one entry per 32 */
														/*   sprites. Used at run time! */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "a-3e-.bin",    0xa000, 0x2000, 0x58e87ab0 )	/* samples (ADPCM 4-bit) */
	ROM_LOAD( "a-3f-.bin",    0xc000, 0x2000, 0xc81e31ea )	/* samples (ADPCM 4-bit) */
	ROM_LOAD( "a-3h-.bin",    0xe000, 0x2000, 0xd99fb995 )
ROM_END

ROM_START( kungfub2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "kf4",          0x0000, 0x4000, 0x3f65313f )
	ROM_LOAD( "kf5",          0x4000, 0x4000, 0x9ea325f3 )

	ROM_REGION_DISPOSE(0x1e000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "g-4c-a.bin",   0x00000, 0x2000, 0x6b2cc9c8 )	/* characters */
	ROM_LOAD( "g-4d-a.bin",   0x02000, 0x2000, 0xc648f558 )
	ROM_LOAD( "g-4e-a.bin",   0x04000, 0x2000, 0xfbe9276e )
	ROM_LOAD( "b-4k-.bin",    0x06000, 0x2000, 0x16fb5150 )	/* sprites */
	ROM_LOAD( "b-4f-.bin",    0x08000, 0x2000, 0x67745a33 )
	ROM_LOAD( "b-4l-.bin",    0x0a000, 0x2000, 0xbd1c2261 )
	ROM_LOAD( "b-4h-.bin",    0x0c000, 0x2000, 0x8ac5ed3a )
	ROM_LOAD( "b-3n-.bin",    0x0e000, 0x2000, 0x28a213aa )
	ROM_LOAD( "b-4n-.bin",    0x10000, 0x2000, 0xd5228df3 )
	ROM_LOAD( "b-4m-.bin",    0x12000, 0x2000, 0xb16de4f2 )
	ROM_LOAD( "b-3m-.bin",    0x14000, 0x2000, 0xeba0d66b )
	ROM_LOAD( "b-4c-.bin",    0x16000, 0x2000, 0x01298885 )
	ROM_LOAD( "b-4e-.bin",    0x18000, 0x2000, 0xc77b87d4 )
	ROM_LOAD( "b-4d-.bin",    0x1a000, 0x2000, 0x6a70615f )
	ROM_LOAD( "b-4a-.bin",    0x1c000, 0x2000, 0x6189d626 )

	ROM_REGION(0x0620)	/* color PROMs */
	ROM_LOAD( "g-1j-.bin",    0x0000, 0x0100, 0x668e6bca )	/* character palette red component */
	ROM_LOAD( "b-1m-.bin",    0x0100, 0x0100, 0x76c05a9c )	/* sprite palette red component */
	ROM_LOAD( "g-1f-.bin",    0x0200, 0x0100, 0x964b6495 )	/* character palette green component */
	ROM_LOAD( "b-1n-.bin",    0x0300, 0x0100, 0x23f06b99 )	/* sprite palette green component */
	ROM_LOAD( "g-1h-.bin",    0x0400, 0x0100, 0x550563e1 )	/* character palette blue component */
	ROM_LOAD( "b-1l-.bin",    0x0500, 0x0100, 0x35e45021 )	/* sprite palette blue component */
	ROM_LOAD( "b-5f-.bin",    0x0600, 0x0020, 0x7a601c3d )	/* sprite height, one entry per 32 */
														/*   sprites. Used at run time! */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "a-3e-.bin",    0xa000, 0x2000, 0x58e87ab0 )	/* samples (ADPCM 4-bit) */
	ROM_LOAD( "a-3f-.bin",    0xc000, 0x2000, 0xc81e31ea )	/* samples (ADPCM 4-bit) */
	ROM_LOAD( "a-3h-.bin",    0xe000, 0x2000, 0xd99fb995 )
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xea06],"\x00\x14\x95",3) == 0 &&
			memcmp(&RAM[0xea78],"\x00\x48\x52",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xea06],6*20);
			RAM[0xe980] = RAM[0xea7a];
			RAM[0xe981] = RAM[0xea79];
			RAM[0xe982] = RAM[0xea78];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xea06],6*20);
		osd_fclose(f);
	}
}



struct GameDriver kungfum_driver =
{
	__FILE__,
	0,
	"kungfum",
	"Kung Fu Master",
	"1984",
	"Irem",
	"Mirko Buffoni\nNicola Salmoria\nIshmair\nPaul Swan\nAaron Giles (sound)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	kungfum_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver kungfud_driver =
{
	__FILE__,
	&kungfum_driver,
	"kungfud",
	"Kung Fu Master (Data East)",
	"1984",
	"Irem (Data East license)",
	"Mirko Buffoni\nNicola Salmoria\nIshmair\nPaul Swan\nAaron Giles (sound)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	kungfud_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver kungfub_driver =
{
	__FILE__,
	&kungfum_driver,
	"kungfub",
	"Kung Fu Master (bootleg set 1)",
	"1984",
	"bootleg",
	"Mirko Buffoni\nNicola Salmoria\nIshmair\nPaul Swan\nAaron Giles (sound)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	kungfub_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver kungfub2_driver =
{
	__FILE__,
	&kungfum_driver,
	"kungfub2",
	"Kung Fu Master (bootleg set 2)",
	"1984",
	"bootleg",
	"Mirko Buffoni\nNicola Salmoria\nIshmair\nPaul Swan\nAaron Giles (sound)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	kungfub2_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
