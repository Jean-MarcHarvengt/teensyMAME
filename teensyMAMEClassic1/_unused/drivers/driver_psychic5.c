/**************************
 *** PSYCHIC 5 hardware ***		(by Roberto Ventura)
 **************************

Psychic 5 (c) JALECO (early 1987)


0) GENERAL.

The game has two Z80s.
The second CPU controls the two YM2203 sound chips.
Screen resolution is 224x256 (vertical CRT).
768 colors on screen.
96 sprites (16x16 or 32x32).

Some hardware features description is arbitrary since I
guessed their hardware implementation trusting what
I can recall of the 'real' arcade game.


1) ROM CONTENTS.

P5A	256 Kbit	Sound program ROM
P5B	512 Kbit	Sprites data ROM 0
P5C	512 Kbit	Sprites data ROM 1
P5D	256 Kbit	Main CPU program ROM
P5E	512 Kbit	Banked CPU data ROM (banks 0,1,2 and 3)
P5F	256 Kbit	Foreground data ROM
P5G	512 Kbit	Background data ROM 0
P5H	512 Kbit	Background data ROM 1

ROM banks 2 and 3 contain the eight level maps only.

Graphics format is pretty simple,each nibble contains information
for one pixel (4 planes packed).

All graphics is made of 8x8 tiles (32 consecutive bytes),16x16 tiles
are composed of 4 consecutive 8x8 tiles,while 32x32 sprites can be
considered as four 16x16 tiles assembled in the same way 8x8 tiles produce
one 16x16 tile.
Tile composition follows this scheme:

These are 4 consecutive tiles as stored in ROM - increasing memory
(the symbol ">" means the tiles are rotated 270 degrees):

A>;B>;C>;D>

This is the composite tile.

A> C>
B> D>

This is the way tile looks on the effective CRT after 90 deg rotation.

C^ D^
A^ B^


2) CPU.

The board mounts two crystals, 12.000 Mhz and 5.000 Mhz.
The possible effective main CPU clock may be 6 Mhz (12/2).

The main Z80 runs in Interrupt mode 0 (IM0),the game program expects
execution of two different restart (RST) instructions.
RST 10,the main IRQ,is to be triggered each time the screen is refreshed.
RST 08 must be triggered in order to make the game work properly
(e.g. demo game),I guess sound has something to do with this IRQ,but I
don't know whether it is sinchronized with video beam neither I know whether
it can be disabled.

Sound CPU runs in IM1.

The main CPU lies idle waiting the external IRQ occurrence executing code
from 012d to 0140.

Game data is paged,level maps and other data fit in ROM space
8000-bfff.

Video RAM is also banked at locations from c000 to dfff.



3) MAIN CPU MEMORY MAP.

0000-7fff       ROM
8000-bfff       paged ROM
c000-dfff       paged RAM (RAM/VRAM/IO)
f000-f1ff	I/O
f200-f7ff	Sprites registers (misc RAM)
f800-ffff       Work RAM

-paged RAM memory map

Bank 0

c000-cfff	Background tile buffer
e000-dfff	RAM (dummy background image for software collisions)

Bank 1

c000-c3ff	I/O
c400-cbff	Palette RAM
d000-d800	Foreground tile buffer


4) I/O

-VRAM bank 1

c000    COIN SLOTS and START BUTTONS

        76543210
        ||    ||
        ||    |^-- coin 1
        ||    ^--- coin 2
        ||
        |^-------- start 1
        ^--------- start 2


c001    PLAYER 1 CONTROLS
c002    PLAYER 2 CONTROLS

        76543210
          ||||||
          |||||^-- right
          ||||^--- left
          |||^---- down
          ||^----- up
          |^------ fire 0
          ^------- fire 1


c003    DIPSWITCH 0

        76543210
        ||
        ^^-------- number of player's espers


c004    DIPSWITCH 1

        76543210
        |||    |
        |||    ^-- player's immortality
        ^^^------- coin/credit configurations

c308    BACKGROUND SCROLL Y  least significant 8 bits

c309    BACKGROUND SCROLL Y  most significant 2 bits

	76543210
	      ||
	      |^-- Y scroll bit 8
	      ^--- Y scroll bit 9

c30A    BACKGROUND SCROLL X  least significant 8 bits

c30B    BACKGROUND SCROLL X  MSB

	76543210
	||||||||
	|||||||^-- X scroll bit 8
	||||||^--- Unknown (title screen)
	^^^^^^---- Unknown (title screen: 0xff)

c30C	SCREEN MODE

	76543210
	      ||
	      |^-- background enable bit (0 means black BG)
	      ^--- grey background enable bit

c5fe	BACKGROUND PALETTE INTENSITY (red and green)

	76543210
	||||||||
	||||^^^^-- green intensity
	^^^^------ red intensity

c5ff	BACKGROUND PALETTE INTENSITY (blue)

	76543210
	||||||||
        ||||^^^^-- unknown (?)
	^^^^------ blue intensity

-RAM f000-f1ff

f000    SOUND COMMAND (?)

f001	UNKNOWN
	maybe some external HW like a flashing light
	when a coin falls in slot (?)

f002	ROM PAGE SELECTOR
	select four (0-3) ROM pages at 8000-bfff.

f003	VRAM PAGE SELECTOR
	selects two (0-1) VRAM pages at c000-dfff.

f004	UNKNOWN

f005	UNKNOWN


5) COLOR RAM

The palette system is dynamic,the game can show up to 768 different
colors on screen.

Each color component (RGB) depth is 4 bits,two consecutive bytes are used
for each color code (12 bits).

format: RRRRGGGGBBBB0000

Colors are organized in palettes,since graphics is 4 bits (16 colors)
each palette takes 32 bytes;the three different layers (background,sprites
and foreground) don't share any color,each has its own 256 color space,hence
the 768 colors on screen.

c400-c5ff       Sprites palettes
c800-c9ff       Background palettes
ca00-cbff       Foreground palettes

The last palette colors for sprites and foreground are transparent
colors,these colors aren't displayed on screen so the actual maximum
color output is 736 colors.

Some additional palette effects are provided for background.
Sprite palette 15's transparent color (c5fe-c5ff) is the global
background intensity.
Background Intensity is encoded in the same way of colors,and
affects the intensity of each color component (BGR).
The least significant nibble of c5ff is unknown,it assumes value
0xf occasionaly when the other nibbles are changed.The only
value which is not 0 neither 0xf is 2 and it is assumed by this nibble
during the ride on the witches' broom.

When bit 1 of c30c (VRAM bank 1) is set background screen turns
grey.Notice the output of this function is passed to the palette
intensity process.
When you hit the witch the screen gets purple,this is done via
setting grey screen and scaling color components.
(BTW,I haven't seen a Psychic5 machine for about 10 years but
I think the resulting purple tonality in my emulator is way off...)

Palette intensity acts badly during title screen,when the game
scenario rapidly shows up under the golden logo;the problem is value
0xffff causes the background to be displayed as black.


6) TILE-BASED LAYERS

The tile format for background and foreground is the same,the
only difference is that background tiles are 16x16 pixels while foreground
tiles are only 8x8.

Background virtual screen is 1024 pixels tall and 512 pixel wide.

Two consecutive bytes identify one tile.

        O7 O6 O5 O4 O3 O2 O1 O0         gfx Offset
        O9 O8 FX FY C3 C2 C1 C0         Attibute

        O= GFX offset (1024 tiles)
        F= Flip X and Y
        C= Color palette selector

Tile layers are to be displayed 'bottom-up' (eg. c000-c040
is row 63 of background screen)

Both background and foreground playfields can scroll; background tiles
are arranged in a rotational (wrap-around) buffer; window can be scrolled
vertically by an eight pixel offset (i.e. enough to scroll a single tile
smoothly),the game has to copy in new tiles every time a row is scrolled
(this feature is only used in the credits section at the end of the game).
I haven't provided the address for this feature, since I don't
know where it is mapped.


7) SPRITES

Five bytes identify each sprite,but the registers actually used
are placed at intervals of 16.
The remaining bytes are used as backup sprite coordinates and
attributes,not real sprites.
Sprites are provided in 2 different sizes,16x16 and 32x32.
Larger sprites are addressed as 16x16 sprites,but they are all
aligned by 4.

The first sprite data is located at f20b,then f21b and so on.

0b      X7 X6 X5 X4 X3 X2 X1 X0         X coord (on screen)
0c      Y7 Y6 Y5 Y5 Y3 Y2 Y1 Y0         X coord (on screen)
0d      O9 O8 FX FY SZ X8 -- Y8         hi gfx - FLIP - hi X - hi Y
0e      O7 O6 O5 O4 O3 O2 O1 O0         gfx - offset
0f      -- -- -- -- C3 C2 C1 C0         color

	Y= Y coordinate (two's complemented) (Y8 is used to clip sprite on top border)
        X= X coordinate (X8 is used to clip 32x32 sprites on left border)
        O= Gfx offset (1024 sprites)
        F= Flip
	SZ=Size 0=16x16 sprite,1=32x32 sprite
        C= Color palette selector
*/

#include "driver.h"
#include "vidhrdw/generic.h"

int  psychic5_vh_start(void);
void psychic5_vh_stop(void);
void psychic5_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void psychic5_paged_ram_w(int offset, int data);
int  psychic5_paged_ram_r(int offset);
void psychic5_vram_page_select_w(int offset,int data);
int  psychic5_vram_page_select_r(int offset);


static int psychic5_bank_latch = 0x0;


int psychic5_bankselect_r(int offset)
{
	return psychic5_bank_latch;
}

void psychic5_bankselect_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	int bankaddress;

	if (data != psychic5_bank_latch)
	{
		psychic5_bank_latch = data;
		bankaddress = 0x10000 + ((data & 3) * 0x4000);
		cpu_setbank(1,&RAM[bankaddress]);	 /* Select 4 banks of 16k */
	}
}

int psychic5_interrupt(void)
{
	if (cpu_getiloops() == 0)
	   return 0xd7;		/* RST 10h */
	else
   	   return 0xcf;		/* RST 08h */
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, psychic5_paged_ram_r },
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0xf000, 0xf000, MRA_RAM },
	{ 0xf001, 0xf001, MRA_RAM },			// unknown
	{ 0xf002, 0xf002, psychic5_bankselect_r },
	{ 0xf003, 0xf003, psychic5_vram_page_select_r },
	{ 0xf004, 0xf004, MRA_RAM },			// unknown
	{ 0xf005, 0xf005, MRA_RAM },			// unknown
	{ 0xf006, 0xf1ff, MRA_NOP },
	{ 0xf200, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xbfff, MWA_BANK1 },
	{ 0xc000, 0xdfff, psychic5_paged_ram_w },
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xf000, 0xf000, soundlatch_w },
	{ 0xf001, 0xf001, MWA_RAM },			// unknown
	{ 0xf002, 0xf002, psychic5_bankselect_w },
	{ 0xf003, 0xf003, psychic5_vram_page_select_w },
	{ 0xf004, 0xf004, MWA_RAM },			// unknown
	{ 0xf005, 0xf005, MWA_RAM },			// unknown
	{ 0xf006, 0xf1ff, MWA_NOP },
	{ 0xf200, 0xf7ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf800, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe000, 0xe000, soundlatch_r },
	{ -1 }
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ -1 }
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x0000, 0x0000, YM2203_control_port_0_w },
	{ 0x0001, 0x0001, YM2203_write_port_0_w },
	{ 0x0080, 0x0080, YM2203_control_port_1_w },
	{ 0x0081, 0x0081, YM2203_write_port_1_w },
        { -1 }
};

INPUT_PORTS_START( input_ports )
    PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

    PORT_START		/* player 1 controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START		/* player 2 controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_COCKTAIL )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START  /* dsw0 */
    PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
    PORT_DIPSETTING(    0x01, "Off" )
    PORT_DIPSETTING(    0x00, "On" )
    PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
    PORT_DIPSETTING(    0x02, "Off" )
    PORT_DIPSETTING(    0x00, "On" )
    PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
    PORT_DIPSETTING(    0x04, "Off" )
    PORT_DIPSETTING(    0x00, "On" )
    PORT_DIPNAME( 0x08, 0x08, "Difficulty", IP_KEY_NONE )
    PORT_DIPSETTING(    0x08, "Normal" )
    PORT_DIPSETTING(    0x00, "Difficult" )
    PORT_DIPNAME( 0x10, 0x00, "Cabinet", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Upright" )
    PORT_DIPSETTING(    0x10, "Cocktail" )
    PORT_DIPNAME( 0x20, 0x20, "Demo Sounds", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Off" )
    PORT_DIPSETTING(    0x20, "On" )
    PORT_DIPNAME( 0xC0, 0xC0, "Lives", IP_KEY_NONE )
    PORT_DIPSETTING(    0x80, "2" )
    PORT_DIPSETTING(    0xC0, "3" )
    PORT_DIPSETTING(    0x40, "4" )
    PORT_DIPSETTING(    0x00, "5" )

    PORT_START  /* dsw1 */
    PORT_BITX(    0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
    PORT_DIPSETTING(    0x01, "Off" )
    PORT_DIPSETTING(    0x00, "On" )
    PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
    PORT_DIPSETTING(    0x02, "Off" )
    PORT_DIPSETTING(    0x00, "On" )
    PORT_DIPNAME( 0xe0, 0xe0, "Coin A", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "5 Coins/1 Credit" )
    PORT_DIPSETTING(    0x20, "4 Coins/1 Credit" )
    PORT_DIPSETTING(    0x40, "3 Coins/1 Credit" )
    PORT_DIPSETTING(    0x60, "2 Coins/1 Credit" )
    PORT_DIPSETTING(    0xe0, "1 Coin/1 Credit" )
    PORT_DIPSETTING(    0xc0, "1 Coin/2 Credits" )
    PORT_DIPSETTING(    0xa0, "1 Coin/3 Credits" )
    PORT_DIPSETTING(    0x80, "1 Coin/4 Credits" )
    PORT_DIPNAME( 0x1c, 0x1c, "Coin B", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "5 Coins/1 Credit" )
    PORT_DIPSETTING(    0x04, "4 Coins/1 Credit" )
    PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
    PORT_DIPSETTING(    0x0c, "2 Coins/1 Credit" )
    PORT_DIPSETTING(    0x1c, "1 Coin/1 Credit" )
    PORT_DIPSETTING(    0x18, "1 Coin/2 Credits" )
    PORT_DIPSETTING(    0x14, "1 Coin/3 Credits" )
    PORT_DIPSETTING(    0x10, "1 Coin/4 Credits" )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
        8,8,    /* 8x8 characters */
        2048,	/* 2048 characters */
        4,      /* 4 bits per pixel */
        { 0, 1, 2, 3 }, /* the four bitplanes for pixel are packed into one nibble */
        { 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
        32*8   	/* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
        16,16,  /* 16x16 characters */
        1024,	/* 1024 characters */
        4,      /* 4 bits per pixel */
        { 0, 1, 2, 3 },	/* the four bitplanes for pixel are packed into one nibble */
        { 0, 4, 8, 12, 16, 20, 24, 28, 64*8, 64*8+4, 64*8+8, 64*8+12, 64*8+16, 64*8+20, 64*8+24, 64*8+28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8, 32*8, 36*8, 40*8, 44*8, 48*8, 52*8, 56*8, 60*8 },
        128*8	/* every char takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        /* character set */
        { 1, 0x00000, &spritelayout,  0*16, 16 },
        { 1, 0x20000, &spritelayout, 16*16, 16 },
        { 1, 0x40000, &charlayout,   32*16, 16 },
	{ -1 } /* end of array */
};

static struct YM2203interface ym2203_interface =
{
	2,		/* 2 chips   */
	1500000,    /* 6000000/4 */
	{ YM2203_VOL(255,127), YM2203_VOL(255,127) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			6000000,
			0,
			readmem,writemem,0,0,
			psychic5_interrupt,2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4250000,	/* hand tuned to fix the tempo */
			2,
			sound_readmem,sound_writemem,0,sound_writeport,
			interrupt,3
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,                                     /* Allow time for 2nd cpu to interleave*/
	0,
	/* video hardware */
	32*8, 32*8,
	{ 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	48*16,48*16,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	psychic5_vh_start,
	psychic5_vh_stop,
	psychic5_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}

};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( psychic5_rom )
	ROM_REGION( 0x20000 ) 				/* 2*64K for main CPU, Z80 */
	ROM_LOAD( "p5d",          0x00000, 0x08000, 0x90259249 )
	ROM_LOAD( "p5e",          0x10000, 0x10000, 0x72298f34 )

	ROM_REGION_DISPOSE( 0x48000 )   			/* graphics (disposed after conversion) */
	ROM_LOAD( "p5b",          0x00000, 0x10000, 0x7e3f87d4 )	/* sprite tiles */
	ROM_LOAD( "p5c",          0x10000, 0x10000, 0x8710fedb )
	ROM_LOAD( "p5g",          0x20000, 0x10000, 0xf9262f32 )	/* background tiles */
	ROM_LOAD( "p5h",          0x30000, 0x10000, 0xc411171a )
	ROM_LOAD( "p5f",          0x40000, 0x08000, 0x04d7e21c ) /* foreground tiles */

	ROM_REGION( 0x10000 ) 				/*64K for 2nd z80 CPU*/
	ROM_LOAD( "p5a",          0x00000, 0x08000, 0x50060ecd )
ROM_END

static int hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if (memcmp(&RAM[0xfc84],"\x00\x07\x53",3) == 0)
	{
		void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
		if (f)
		{
			osd_fread(f,&RAM[0xfd00],49);
			osd_fclose(f);

			/* Copy the high score to the work ram as well */

			RAM[0xfc84] = RAM[0xfd00];
			RAM[0xfc85] = RAM[0xfd01];
			RAM[0xfc86] = RAM[0xfd02];
		}
		return 1;
	}
	return 0;  /* we can't load the hi scores yet */
}

static void hisave(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */

	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1);

	if (f)
	{
		osd_fwrite(f,&RAM[0xfd00],49);
		osd_fclose(f);
	}
}

struct GameDriver psychic5_driver =
{
	__FILE__,
	0,
	"psychic5",
	"Psychic 5",
	"1987",
	"Jaleco",
	"Jarek Parchanski\nRoberto Ventura",
	0,
	&machine_driver,
	0,

	psychic5_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	hiload, hisave
};
