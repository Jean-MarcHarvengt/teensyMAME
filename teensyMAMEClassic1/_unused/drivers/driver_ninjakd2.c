/***************************************************************************

 *****************************
 *** NINJA KID II hardware ***		(by Roberto Ventura)
 *****************************

General aspect.

The game is driven by a fast Z80 CPU.
Screen resolution is 256x192.
768 colors on screen.
96 sprites.

Rom Contents:

NK2_01.ROM              Main CPU program ROM
NK2_02.ROM              CPU banked data ROM 0 (banks 0 and 1)
NK2_03.ROM              CPU banked data ROM 1 (banks 2 and 3)
NK2_04.ROM              CPU banked data ROM 2 (banks 4 and 5)
NK2_05.ROM              CPU banked data ROM 3 (banks 6 and 7)
NK2_06.ROM              Sound CPU program ROM (encrypted)
NK2_07.ROM              Sprites data ROM 1
NK2_08.ROM              Sprites data ROM 0
NK2_09.ROM              Raw PCM samples (complete?)
NK2_10.ROM              Background data ROM 1
NK2_11.ROM              Background data ROM 0
NK2_12.ROM              Foreground data ROM

*** MEMORY MAP ***

0000-7fff       Main CPU ROM
8000-bfff       Banked CPU ROM
c000-c7ff       I/O
c800-cdff       Color RAM
d000-d7ff       "FRONT" tile map
d800-dfff       "BACK" tile map
e000-efff       Main RAM
f400-f7ff	??? screen frame ???
f800-f9ff	CPU Stack
fa00-ffff       Sprite registers (misc RAM)


1) CPU

1 OSC 12 Mhz
1 OSC 5 Mhz
2 YM 2203C.......CaBBe!.

The Z80 runs in IM0,the game expects execution of RST10 each
frame.

Game level maps,additional code and data are available to main
program via CPU banking at lacations 8000-bf00

The encryption scheme for ROM 6(alternate version) is very simple,
I was surprised by such a large ROM (64k) for sound.
The first half contains opcodes only (see machine 1 pin)
while the second 32k chunk is for immediate data.
Opcode and Data keep their relative positions as well.



2) I/O

c000    "KEYCOIN" button

        76543210
        || |  ||
        || |  |^-COIN 1
        || |  ^--COIN 2
        || ^-----TEST MODE (on the fly,to be pressed during boot)
        |^-------START 1
        ^--------START 2


c001	"PAD1"
c002	"PAD2"

        76543210
          ||||||
          |||||^-RIGHT
          ||||^--LEFT
          |||^---DOWN
          ||^----UP
          |^-----FIRE 0
          ^------FIRE 1


c003    "DIPSW1"

        76543210
        ||||||||
        |||||||^-UNUSED?
        ||||||^-->EXTRA
        |||||^--->LIVES
        ||||^----CONTINUE MODE
        |||^-----DEMO SOUND
        ||^------NORMAL/HARD
        |^-------LIVES 3/4
        ^--------ENGLISH/JAP


c004    "DIPSW2"

        76543210
        ||||||||
        |||||||^-TEST MODE
        ||||||^--TABLE/UPRIGHT
        |||||^---"CREDIT SERVICE"
        ||||^---->
        |||^----->>
        ||^------>>> coin/credit configurations
        |^------->>
        ^-------->

c200	Sound command?
	This byte is written when game plays sound effects...
	it is set when music or sound effects (both pcm and fm) are triggered;
	I guess it is read by another CPU,then.

c201	Unknown,but used.

c202    Bank selector (0 to 7)

c203    Sprite 'overdraw'
        this is the most interesting feature of the game,when set
        the sprites drawn in the previous frame remain on the
        screen,so the game can perform special effects such as the
        'close up' when you die or the "infinite balls" appearing
        when an extra weapon is collected.
        Appears to work like a xor mask,a sprite removes older
        sprite 'bitmap' when present;other memory locations connected to
	this function may be f400-f7ff...should be investigated more.
        -mmmh... I believe this is sci-fiction for a non-bitmap game...

C208    Scroll X  0-7

C209    Scroll X  MSB

C20A    Scroll Y  0-7

C20B    Scroll Y  MSB

C20C    Background ENABLE
        when set to zero the background is totally black,but
        sprites are drawn correctly.


3) COLOR RAM

The palette system is dynamic,the game can show up to 768 different
colors on screen.

Palette depth is 12 bits as usual,two consecutive bytes are used for each
color code.

format: RRRRGGGGBBBB0000

Colors are organized in palettes,since graphics is 4 bits (16 colors)
each palette takes 32 bytes,the three different layers,BACK,SPRITES and
FRONT don't share any color,each has its own 256 color space,hence the
768 colors on screen.

c800-c9ff       Background palettes
ca00-cbff       Sprites palettes
cc00-cdff       Foreground palettes


4) TILE-BASED LAYERS

The tile format for background and foreground is the same,the
only difference is that background tiles are 16x16 pixels while foreground
tiles are only 8x8.

Background virtual screen is 512x512 pixels scrollable.

Two consecutive tiles bytes identify one tile.

        O7 O6 O5 O4 O3 O2 O1 O0         gfx Offset
        O9 O8 FY FX C3 C2 C1 C0         Attibute

        O= GFX offset (1024 tiles)
        F= Flip X and Y
        C= Color palette selector


5) SPRITES

Five bytes identify each sprite,but the registers actually used
are placed at intervals of 16.
Some of the remaining bytes are used (e.g. fa00),their meaning is totally
unknown to me,they seem to be related to the surprising additional sprite
feature of the game,but maybe they're just random writes in RAM.

The first sprite data is located at fa0b,then fa1b and so on.


0b      Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0         Y coord
0c      X7 X6 X5 X4 X3 X2 X1 X0         X coord
0d      O9 O8 FY FX -- -- EN X8         hi gfx - FLIP - Enable - hi X
0e      O7 O6 O5 O4 O3 O2 O1 O0         gfx - offset
0f      -- -- -- -- C3 C2 C1 C0         color

        Y= Y coordinate
        X= X coordinate (X8 is used to clip sprite on the left)
        O= Gfx offset (1024 sprites)
        F= Flip
       EN= Enable (this is maybe the Y8 coordinate bit,but it isn't set
           accordingly in the test mode
        C= Color palette selector

***************************************************************************/


#include "driver.h"
#include "vidhrdw/generic.h"

void ninjakd2_bgvideoram_w(int offset, int data);
void ninjakd2_fgvideoram_w(int offset, int data);
void ninjakd2_sprite_overdraw_w(int offset, int data);
void ninjakd2_background_enable_w(int offset, int data);
int  ninjakd2_vh_start(void);
void ninjakd2_vh_stop(void);
void ninjakd2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern unsigned char 	*ninjakd2_scrolly_ram;
extern unsigned char 	*ninjakd2_scrollx_ram;
extern unsigned char 	*ninjakd2_bgenable_ram;
extern unsigned char 	*ninjakd2_spoverdraw_ram;
extern unsigned char 	*ninjakd2_spriteram;
extern unsigned char 	*ninjakd2_background_videoram;
extern unsigned char 	*ninjakd2_foreground_videoram;
extern int 	ninjakd2_spriteram_size;
extern int	ninjakd2_backgroundram_size;
extern int 	ninjakd2_foregroundram_size;

static int ninjakd2_bank_latch = 255, main_cpu_num;

void ninjakd2_init_machine(void)
{
	main_cpu_num = 0;
}

void ninjak2a_init_machine(void)
{
	main_cpu_num = 1;
}

int ninjakd2_interrupt(void)
{
	return 0x00d7;	/* RST 10h */
}

int ninjakd2_bankselect_r(int offset)
{
	return ninjakd2_bank_latch;
}

void ninjakd2_bankselect_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[main_cpu_num].memory_region];
	int bankaddress;

	if (data != ninjakd2_bank_latch)
	{
		ninjakd2_bank_latch = data;

		bankaddress = 0x10000 + ((data & 0x7) * 0x4000);
		cpu_setbank(1,&RAM[bankaddress]);	 /* Select 8 banks of 16k */
	}
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc000, input_port_2_r },
	{ 0xc001, 0xc001, input_port_0_r },
	{ 0xc002, 0xc002, input_port_1_r },
	{ 0xc003, 0xc003, input_port_3_r },
	{ 0xc004, 0xc004, input_port_4_r },
	{ 0xc200, 0xc200, MRA_RAM },
	{ 0xc201, 0xc201, MRA_RAM },		// unknown but used
	{ 0xc202, 0xc202, ninjakd2_bankselect_r },
	{ 0xc203, 0xc203, MRA_RAM },
	{ 0xc208, 0xc209, MRA_RAM, &ninjakd2_scrollx_ram },
	{ 0xc20a, 0xc20b, MRA_RAM, &ninjakd2_scrolly_ram },
	{ 0xc20c, 0xc20c, MRA_RAM },
	{ 0xc800, 0xf9ff, MRA_RAM },
	{ 0xfa00, 0xffff, MRA_RAM, &ninjakd2_spriteram, &ninjakd2_spriteram_size },
	{ -1 }	/* end of table */
};


static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc200, 0xc200, soundlatch_w },
	{ 0xc201, 0xc201, MWA_RAM },		// unknown but used
	{ 0xc202, 0xc202, ninjakd2_bankselect_w },
	{ 0xc203, 0xc203, ninjakd2_sprite_overdraw_w, &ninjakd2_spoverdraw_ram },
	{ 0xc208, 0xc20b, MWA_RAM },
	{ 0xc20c, 0xc20c, ninjakd2_background_enable_w, &ninjakd2_bgenable_ram },
	{ 0xc800, 0xcdff, paletteram_RRRRGGGGBBBBxxxx_swap_w, &paletteram },
	{ 0xd000, 0xd7ff, ninjakd2_fgvideoram_w, &ninjakd2_foreground_videoram, &ninjakd2_foregroundram_size },
	{ 0xd800, 0xdfff, ninjakd2_bgvideoram_w, &ninjakd2_background_videoram, &ninjakd2_backgroundram_size },
	{ 0xe000, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress snd_readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe000, 0xe000, soundlatch_r },
	{ 0xf000, 0xf000, MRA_RAM },	 // UNKNOWN ??????
	{ -1 }	/* end of table */
};


static struct MemoryWriteAddress snd_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xf000, 0xf000, MWA_RAM },	// UNKNOWN ??????
	{ -1 }	/* end of table */
};

static struct IOWritePort snd_writeport[] =
{
	{ 0x0000, 0x0000, YM2203_control_port_0_w },
	{ 0x0001, 0x0001, YM2203_write_port_0_w },
	{ 0x0080, 0x0080, YM2203_control_port_1_w },
	{ 0x0081, 0x0081, YM2203_write_port_1_w },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( input_ports )

    PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START    /* player 2 controls */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_COCKTAIL )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )	/* keep pressed during boot to enter service mode */
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

    PORT_START  /* dsw0 */
    PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE )
    PORT_DIPSETTING(    0x01, "Off" )
    PORT_DIPSETTING(    0x00, "On" )
    PORT_DIPNAME( 0x06, 0x06, "Bonus Life", IP_KEY_NONE )
    PORT_DIPSETTING(    0x04, "20000 50000" )
    PORT_DIPSETTING(    0x06, "30000 50000" )
    PORT_DIPSETTING(    0x02, "50000 100000" )
    PORT_DIPSETTING(    0x00, "None" )
    PORT_DIPNAME( 0x08, 0x08, "Allow Continue", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "No" )
    PORT_DIPSETTING(    0x08, "Yes"  )
    PORT_DIPNAME( 0x10, 0x00, "Demo Sounds", IP_KEY_NONE )
    PORT_DIPSETTING(    0x10, "Off" )
    PORT_DIPSETTING(    0x00, "On"  )
    PORT_DIPNAME( 0x20, 0x20, "Difficulty", IP_KEY_NONE )
    PORT_DIPSETTING(    0x20, "Normal" )
    PORT_DIPSETTING(    0x00, "Hard" )
    PORT_DIPNAME( 0x40, 0x40, "Lives", IP_KEY_NONE )
    PORT_DIPSETTING(    0x40, "3" )
    PORT_DIPSETTING(    0x00, "4" )
    PORT_DIPNAME( 0x80, 0x00, "Language", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "English" )
    PORT_DIPSETTING(    0x80, "Japanese" )

    PORT_START  /* dsw1 */
    PORT_BITX(    0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
    PORT_DIPSETTING(    0x01, "Off" )
    PORT_DIPSETTING(    0x00, "On" )
    PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Upright" )
    PORT_DIPSETTING(    0x02, "Cocktail" )
    PORT_DIPNAME( 0x04, 0x00, "Credit Service", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "Off" )
    PORT_DIPSETTING(    0x04, "On" )
    PORT_DIPNAME( 0x18, 0x18, "Coin B", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
    PORT_DIPSETTING(    0x18, "1 Coin/1 Credit" )
    PORT_DIPSETTING(    0x10, "1 Coin/2 Credits" )
    PORT_DIPSETTING(    0x08, "1 Coin/3 Credits" )
    PORT_DIPNAME( 0xe0, 0xe0, "Coin A", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "5 Coins/1 Credit" )
    PORT_DIPSETTING(    0x20, "4 Coins/1 Credit" )
    PORT_DIPSETTING(    0x40, "3 Coins/1 Credit" )
    PORT_DIPSETTING(    0x60, "2 Coins/1 Credit" )
    PORT_DIPSETTING(    0xe0, "1 Coin/1 Credit" )
    PORT_DIPSETTING(    0xc0, "1 Coin/2 Credits" )
    PORT_DIPSETTING(    0xa0, "1 Coin/3 Credits" )
    PORT_DIPSETTING(    0x80, "1 Coin/4 Credits" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,     /* 8*8 characters */
	1024,    /* 1024 characters */
	4,       /* 4 bits per pixel */
	{0,1,2,3 }, /* the bitplanes are packed in one nibble */
	{0, 4, 16384*8+0, 16384*8+4, 8, 12, 16384*8+8, 16384*8+12 },
	{16*0, 16*1, 16*2, 16*3, 16*4, 16*5, 16*6, 16*7 },
	8*16
};

static struct GfxLayout spritelayout =
{
	16,16,   /* 16*16 characters */
	1024,    /* 1024 sprites */
	4,       /* 4 bits per pixel */
	{0,1,2,3}, /* the bitplanes are packed in one nibble */
	{0,  4,  65536*8+0,  65536*8+4,  8, 12,  65536*8+8, 65536*8+12,
		16*8+0, 16*8+4, 16*8+65536*8+0, 16*8+65536*8+4, 16*8+8, 16*8+12, 16*8+65536*8+8, 16*8+65536*8+12},
	{16*0, 16*1, 16*2, 16*3, 16*4, 16*5, 16*6, 16*7,
		32*8+16*0, 32*8+16*1, 32*8+16*2, 32*8+16*3, 32*8+16*4, 32*8+16*5, 32*8+16*6, 32*8+16*7},
	8*64
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x20000, &spritelayout,  0*16, 16 },
	{ 1, 0x00000, &spritelayout, 16*16, 16 },
	{ 1, 0x40000, &charlayout,   32*16, 16 },
	{ -1 } /* end of array */
};

static struct YM2203interface ym2203_interface =
{
	2, /* 2 chips */
	1250000, /* 5000000/4 MHz ???? */
	{ YM2203_VOL(255,255), YM2203_VOL(255,255) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static struct MachineDriver ninjakd2_machine_driver =
{
	{
		{
			CPU_Z80,
			6000000,		/* 12000000/2 ??? */
			0,			/* & vbl duration since sprites are */
			readmem,writemem,0,0,	/* very sensitive to these settings */
			ninjakd2_interrupt,1
		}
	},
	60, 10000,			/* frames per second, vblank duration */
	10,				/* single CPU, no need for interleaving */
	ninjakd2_init_machine,
	32*8, 32*8,
	{ 0*8, 32*8-1, 4*8, 28*8-1 },
	gfxdecodeinfo,
	48*16,48*16,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	ninjakd2_vh_start,
	ninjakd2_vh_stop,
	ninjakd2_vh_screenrefresh,

	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};

static struct MachineDriver ninjak2a_machine_driver =
{
	{
/* TODO: currently I have to put the sound CPU first because the main core supports */
/* opcode decryption only on the first CPU */
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,		/* 12000000/3 ??? */
			0,
			snd_readmem,snd_writemem,
			0,snd_writeport,
			interrupt,2
		},
		{
			CPU_Z80,
			6000000,		/* 12000000/2 ??? */
			2,			/* & vbl duration since sprites are */
			readmem,writemem,0,0,	/* very sensitive to these settings */
			ninjakd2_interrupt,1
		}
	},
	60, 10000,	/* frames per second, vblank duration */
	10,
	ninjak2a_init_machine,
	32*8, 32*8,
	{ 0*8, 32*8-1, 4*8, 28*8-1 },
	gfxdecodeinfo,
	48*16,48*16,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	ninjakd2_vh_start,
	ninjakd2_vh_stop,
	ninjakd2_vh_screenrefresh,

	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};



ROM_START( ninjakd2_rom )
	ROM_REGION(0x30000)
	ROM_LOAD( "nk2_01.rom",   0x00000, 0x8000, 0x3cdbb906 )
	ROM_LOAD( "nk2_02.rom",   0x10000, 0x8000, 0xb5ce9a1a )
	ROM_LOAD( "nk2_03.rom",   0x18000, 0x8000, 0xad275654 )
	ROM_LOAD( "nk2_04.rom",   0x20000, 0x8000, 0xe7692a77 )
	ROM_LOAD( "nk2_05.rom",   0x28000, 0x8000, 0x5dac9426 )

	ROM_REGION_DISPOSE(0x48000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "nk2_08.rom",   0x00000, 0x4000, 0x1b79c50a )   // sprites tiles
	ROM_CONTINUE(	        0x10000, 0x4000)
	ROM_CONTINUE(	        0x04000, 0x4000)
	ROM_CONTINUE(	        0x14000, 0x4000)
	ROM_LOAD( "nk2_07.rom",   0x08000, 0x4000, 0x0be5cd13 )
	ROM_CONTINUE(	        0x18000, 0x4000)
	ROM_CONTINUE(	        0x0C000, 0x4000)
	ROM_CONTINUE(	        0x1C000, 0x4000)
	ROM_LOAD( "nk2_11.rom",   0x20000, 0x4000, 0x41a714b3 )   // background tiles
	ROM_CONTINUE(	        0x30000, 0x4000)
	ROM_CONTINUE(	        0x24000, 0x4000)
	ROM_CONTINUE(	        0x34000, 0x4000)
	ROM_LOAD( "nk2_10.rom",   0x28000, 0x4000, 0xc913c4ab )
	ROM_CONTINUE(	        0x38000, 0x4000)
	ROM_CONTINUE(	        0x2C000, 0x4000)
	ROM_CONTINUE(	        0x3C000, 0x4000)
	ROM_LOAD( "nk2_12.rom",   0x40000, 0x02000, 0xdb5657a9 )  // foreground tiles
	ROM_CONTINUE(	        0x44000, 0x02000)
	ROM_CONTINUE(	        0x42000, 0x02000)
	ROM_CONTINUE(	        0x46000, 0x02000)

	ROM_REGION(0x10000)
	ROM_LOAD( "nk2_06.rom",   0x0000, 0x10000, 0xd3a18a79 )  // sound z80 code encrypted

	ROM_REGION(0x10000)
	ROM_LOAD( "nk2_09.rom",   0x0000, 0x10000, 0xc1d2d170 )  // raw pcm samples
ROM_END

ROM_START( ninjak2a_rom )
	ROM_REGION(0x10000)
	ROM_LOAD( "nk2_06.bin",   0x0000, 0x10000, 0x7bfe6c9e )  // sound z80 code encrypted

	ROM_REGION_DISPOSE(0x48000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "nk2_08.rom",   0x00000, 0x4000, 0x1b79c50a )   // sprites tiles
	ROM_CONTINUE(	        0x10000, 0x4000)
	ROM_CONTINUE(	        0x04000, 0x4000)
	ROM_CONTINUE(	        0x14000, 0x4000)
	ROM_LOAD( "nk2_07.rom",   0x08000, 0x4000, 0x0be5cd13 )
	ROM_CONTINUE(	        0x18000, 0x4000)
	ROM_CONTINUE(	        0x0C000, 0x4000)
	ROM_CONTINUE(	        0x1C000, 0x4000)
	ROM_LOAD( "nk2_11.rom",   0x20000, 0x4000, 0x41a714b3 )   // background tiles
	ROM_CONTINUE(	        0x30000, 0x4000)
	ROM_CONTINUE(	        0x24000, 0x4000)
	ROM_CONTINUE(	        0x34000, 0x4000)
	ROM_LOAD( "nk2_10.rom",   0x28000, 0x4000, 0xc913c4ab )
	ROM_CONTINUE(	        0x38000, 0x4000)
	ROM_CONTINUE(	        0x2C000, 0x4000)
	ROM_CONTINUE(	        0x3C000, 0x4000)
	ROM_LOAD( "nk2_12.rom",   0x40000, 0x02000, 0xdb5657a9 )  // foreground tiles
	ROM_CONTINUE(	        0x44000, 0x02000)
	ROM_CONTINUE(	        0x42000, 0x02000)
	ROM_CONTINUE(	        0x46000, 0x02000)

	ROM_REGION(0x30000)
	ROM_LOAD( "nk2_01.bin",   0x00000, 0x8000, 0xe6adca65 )
	ROM_LOAD( "nk2_02.bin",   0x10000, 0x8000, 0xd9284bd1 )
	ROM_LOAD( "nk2_03.rom",   0x18000, 0x8000, 0xad275654 )
	ROM_LOAD( "nk2_04.rom",   0x20000, 0x8000, 0xe7692a77 )
	ROM_LOAD( "nk2_05.bin",   0x28000, 0x8000, 0x960725fb )

	ROM_REGION(0x10000)
	ROM_LOAD( "nk2_09.rom",   0x0000, 0x10000, 0xc1d2d170 )  // raw pcm samples
ROM_END



static int hiload(void)
{
    void *f;
    unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[main_cpu_num].memory_region];

    /* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0xe0f4],"\x00\x30\x00",3) == 0 )
    {
        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
            osd_fread(f,&RAM[0xe04e],100);
	    osd_fread(f,&RAM[0xe0f4],3);
            osd_fclose(f);
        }
        return 1;
    }
    else
        return 0;  /* we can't load the hi scores yet */
}

static void hisave(void)
{
    void *f;
    unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[main_cpu_num].memory_region];

    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
        osd_fwrite(f,&RAM[0xe04e],100);
	osd_fwrite(f,&RAM[0xe0f4],3);
        osd_fclose(f);
    }
}

void ninjak2a_sound_decode(void)
{
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	memcpy(ROM,RAM,0xffff);

	for (A = 0x0000;A < 0x7FFF;A++)
		RAM[A] = RAM[A+0x8000];
}

struct GameDriver ninjakd2_driver =
{
	__FILE__,
	0,
	"ninjakd2",
	"Ninja Kid II",
	"1987",
	"UPL",
	"Jarek Parchanski (MAME driver)\nRoberto Ventura (hardware info)",
#if 0
"\n\n\nGame authors:\n\n \
Game design:    Tsutomu Fuzisawa\n \
Program design: Satoru Kinjo\n \
Char. design:   Tsutomu Fizisawa\n \
Char. design:   Akemi Tsunoda\n \
Sound compose:  Tsutomu Fuzisawa\n \
Bgm create:     Mecano Associate\n \
Data make:      Takashi Hayashi\n",
#endif
	GAME_NOT_WORKING,
	&ninjakd2_machine_driver,
	0,

	ninjakd2_rom,
	0,0,
	0,
	0, /* sound prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload,hisave
};

struct GameDriver ninjak2a_driver =
{
	__FILE__,
	&ninjakd2_driver,
	"ninjak2a",
	"Ninja Kid II (alternate)",
	"1987",
	"UPL",
	"Jarek Parchanski (MAME driver)\nRoberto Ventura (hardware info)",
#if 0
"\n\n\nGame authors:\n\n \
Game design:    Tsutomu Fuzisawa\n \
Program design: Satoru Kinjo\n \
Char. design:   Tsutomu Fizisawa\n \
Char. design:   Akemi Tsunoda\n \
Sound compose:  Tsutomu Fuzisawa\n \
Bgm create:     Mecano Associate\n \
Data make:      Takashi Hayashi\n",
#endif
	0,
	&ninjak2a_machine_driver,
	0,

	ninjak2a_rom,
	0,ninjak2a_sound_decode,
	0,
	0, /* sound prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload,hisave
};
