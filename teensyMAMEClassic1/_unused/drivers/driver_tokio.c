/***************************************************************************

Tokio memory map

CPU 1
0000-bfff ROM (8000-bfff is banked)
c000-dcff Graphic RAM. This contains pointers to the video RAM columns and
          to the sprites contained in Object RAM.
dd00-dfff Object RAM (groups of four bytes: X position, code [offset in the
          Graphic RAM], Y position, gfx bank)
e000-f7ff RAM (Shared)
f800-f9ff Palette RAM

fa03 - DSW0
fa04 - DSW1
fa05 - Coins
fa06 - Controls Player 1
fa07 - Controls Player 1

CPU 2
0000-7fff ROM
8000-97ff RAM (Shared)

CPU 3
0000-7fff ROM
8000-8fff RAM


  Here goes a list of known deficiencies of our drivers:

  - The bootleg romset is functional. The original one hangs at
    the title screen. This is because Fredrik and I have worked
    on the first one, and got mostly done. Later Victor added support
    for the original set (mainly sound), which is still deficient.

  - Score saving is still wrong, I think.

  - Sound support is a bit buggy.

  - Gfx bug with large "block" sprites: the big tank on the main
    street, the big mothership and (I guess) the ground turrets later
    in the game.

    I am pretty confident the scrolling is right (it was a bitch to get
    right), and almost sure the normal sprites are handled correctly.
    However, I think there's still some work to be done with the block
    sprites.

    The problem is that the object memory seems to have special
    meaning associated with the higher two bits of the 'map' and the
    'bank' entries. For example, the big tank on the ground should be
    drawn with a height of 64 pixels instead of the usual 32.

    One guess: after some bits are set, a number following entries in
    the object ram could describe aditional sections of the block
    sprite, to be positioned right next to the first one. So in this
    scheme the tank would take two entries, but with just one with
    valid positioning.

  - "fake-r" routine make the "original" roms to restart the game after
    some seconds.

    Well, we know very little about the 0xFE00 address. It could be
    some watchdog or a synchronization timer.

    I remember scanning the main CPU code to find how it was
    used on the bootleg set. Then I just figured out a constant value
    that made the game run (it hang if just set unhandled, that is,
    returning zero).

    Maybe the solution is to patch the bootleg ROMs to skip some tests
    at this location (I remember some of them being in the
    initialization routine of the main CPU).

                       Marcelo de G. Malheiros <malheiro@dca.fee.unicamp.br>
                                                                   1998.9.25

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *tokio_objectram;
extern int tokio_objectram_size;
void bublbobl_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int tokio_vh_start(void);
void tokio_vh_stop(void);
int tokio_videoram_r(int offset);
void tokio_videoram_w(int offset,int data);
int tokio_objectram_r(int offset);
void tokio_objectram_w(int offset,int data);
void tokio_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



unsigned char *tokio_sharedram;

int tokio_sharedram_r(int offset)
{
	return tokio_sharedram[offset];
}

void tokio_sharedram_w(int offset, int data)
{
	tokio_sharedram[offset] = data;
}

void tokio_bankswitch_w(int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

        cpu_setbank(1, &RAM[0x10000 + 0x4000 * (data & 7)]);
}

void tokio_nmitrigger_w(int offset, int data)
{
	cpu_cause_interrupt(1,Z80_NMI_INT);
}

void tokio_sound_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(2,Z80_NMI_INT);
}

static int fake_r(int offset)
{
  return 0xbf; /* ad-hoc value set to pass initial testing */
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdcff, tokio_videoram_r },
	{ 0xdd00, 0xdfff, tokio_objectram_r },
	{ 0xe000, 0xf7ff, tokio_sharedram_r },
	{ 0xf800, 0xf9ff, paletteram_r },
	{ 0xfa03, 0xfa03, input_port_0_r },
	{ 0xfa04, 0xfa04, input_port_1_r },
	{ 0xfa05, 0xfa05, input_port_2_r },
	{ 0xfa06, 0xfa06, input_port_3_r },
	{ 0xfa07, 0xfa07, input_port_4_r },
	{ 0xfe00, 0xfe00, fake_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdcff, tokio_videoram_w, &videoram, &videoram_size },
	{ 0xdd00, 0xdfff, tokio_objectram_w, &tokio_objectram, &tokio_objectram_size },
	{ 0xe000, 0xf7ff, tokio_sharedram_w, &tokio_sharedram },
	{ 0xf800, 0xf9ff, paletteram_RRRRGGGGBBBBxxxx_swap_w, &paletteram },
	{ 0xfa00, 0xfa00, MWA_NOP },
	{ 0xfa80, 0xfa80, tokio_bankswitch_w },
	{ 0xfb00, 0xfb00, MWA_NOP }, /* ??? */
	{ 0xfb80, 0xfb80, tokio_nmitrigger_w },
	{ 0xfc00, 0xfc00, tokio_sound_command_w },
	{ 0xfe00, 0xfe00, MWA_NOP }, /* ??? */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress readmem_video[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x97ff, tokio_sharedram_r },
	{ 0xf800, 0xf9ff, paletteram_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem_video[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x97ff, tokio_sharedram_w, &tokio_sharedram },
	{ 0xf800, 0xf9ff, paletteram_RRRRGGGGBBBBxxxx_swap_w },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9000, soundlatch_r },
//	{ 0xa000, 0xa000, YM3526_status_port_0_r },
	{ 0xb000, 0xb000, YM2203_status_port_0_r },
	{ 0xb001, 0xb001, YM2203_read_port_0_r },
	{ 0xe000, 0xefff, MRA_ROM },	/* space for diagnostic ROM? */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
//	{ 0x9000, 0x9001, MWA_NOP },
//	{ 0x9800, 0x9800, MWA_NOP },
//	{ 0xa000, 0xa000, YM3526_control_port_0_w },
//	{ 0xa001, 0xa001, YM3526_write_port_0_w },
//	{ 0xa800, 0xa801, MWA_NOP },
	{ 0xb000, 0xb000, YM2203_control_port_0_w },
	{ 0xb001, 0xb001, YM2203_write_port_0_w },
	{ 0xe000, 0xefff, MWA_ROM },	/* space for diagnostic ROM? */
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( tokio_input_ports )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x02, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sounds?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x02, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x08, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0C, "100000 400000" )
	PORT_DIPSETTING(    0x08, "200000 400000" )
	PORT_DIPSETTING(    0x04, "300000 400000" )
	PORT_DIPSETTING(    0x00, "400000 400000" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "A" )
	PORT_DIPSETTING(    0x40, "B" )
	PORT_DIPNAME( 0x80, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x80, "Japanese" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 ) /* service */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( tokiob_input_ports )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x02, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sounds?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x02, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x08, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0C, "100000 400000" )
	PORT_DIPSETTING(    0x08, "200000 400000" )
	PORT_DIPSETTING(    0x04, "300000 400000" )
	PORT_DIPSETTING(    0x00, "400000 400000" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x00, "99" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "A" )
	PORT_DIPSETTING(    0x40, "B" )
	PORT_DIPNAME( 0x80, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x80, "Japanese" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 ) /* service */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN)

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_2WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,	/* the characters are 8x8 pixels */
	256*8*8,/* 256 chars per bank * 8 banks per ROM pair * 8 ROM pairs */
	4,	/* 4 bits per pixel */
	{ 0, 4, 8*0x8000*8, 8*0x8000*8+4 },
	{ 3, 2, 1, 0, 11, 10, 9, 8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 bytes in two ROMs */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* read all graphics into one big graphics region */
	{ 1, 0x00000, &charlayout, 0, 16 },
	{ -1 }	/* end of array */
};

static void irqhandler(void)
{
	cpu_cause_interrupt(2,0xff);
}

static struct YM2203interface ym2203_interface =
{
	1,		/* 1 chip */
	3000000,	/* 3 MHz ??? */
	{ YM2203_VOL(100,20) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};


static struct MachineDriver tokio_machine_driver =
{
	/* basic machine hardware */
	{		/* MachineCPU */
		{       /* Main CPU */
			CPU_Z80,
			4000000,		/* 4 Mhz??? */
			0,			/* memory_region */
			readmem,writemem,0,0,
			interrupt,1
		},
		{       /* Video CPU */
			CPU_Z80,
			4000000,		/* 4 Mhz??? */
			2,			/* memory_region */
			readmem_video,writemem_video,0,0,
			interrupt,1
		},
		{       /* Audio CPU */
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	        /* 4 Mhz ??? */
			3,	                /* memory region */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
						/* NMIs are triggered by the main CPU */
						/* IRQs are triggered by the YM2203 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION, /* frames/second, vblank duration */
	100,      /* 100 CPU slices per frame */
	0,	/* init_machine() */

	/* video hardware */
	32*8, 32*8,			/* screen width, height */
	{ 0, 32*8-1, 2*8, 30*8-1 },   /* visible area */
	gfxdecodeinfo,
	256, 256,
	bublbobl_vh_convert_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	tokio_vh_start,
	tokio_vh_stop,
	tokio_vh_screenrefresh,

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

ROM_START( tokio_rom )
    ROM_REGION(0x30000)	/* main CPU */
    ROM_LOAD( "a7127-1.256", 0x00000, 0x8000, 0x8c180896 )
    /* ROMs banked at 8000-bfff */
    ROM_LOAD( "a7128-1.256", 0x10000, 0x8000, 0x1b447527 )
    ROM_LOAD( "a7104.256",   0x18000, 0x8000, 0xa0a4ce0e )
    ROM_LOAD( "a7105.256",   0x20000, 0x8000, 0x6da0b945 )
    ROM_LOAD( "a7106-1.256", 0x28000, 0x8000, 0x56927b3f )

    ROM_REGION_DISPOSE(0x80000)	/* temporary space for graphics */
    /* 1st plane */
    ROM_LOAD( "a7108.256",   0x00000, 0x8000, 0x0439ab13 )
    ROM_LOAD( "a7109.256",   0x08000, 0x8000, 0xedb3d2ff )
    ROM_LOAD( "a7110.256",   0x10000, 0x8000, 0x69f0888c )
    ROM_LOAD( "a7111.256",   0x18000, 0x8000, 0x4ae07c31 )
    ROM_LOAD( "a7112.256",   0x20000, 0x8000, 0x3f6bd706 )
    ROM_LOAD( "a7113.256",   0x28000, 0x8000, 0xf2c92aaa )
    ROM_LOAD( "a7114.256",   0x30000, 0x8000, 0xc574b7b2 )
    ROM_LOAD( "a7115.256",   0x38000, 0x8000, 0x12d87e7f )
    /* 2nd plane */
    ROM_LOAD( "a7116.256",   0x40000, 0x8000, 0x0bce35b6 )
    ROM_LOAD( "a7117.256",   0x48000, 0x8000, 0xdeda6387 )
    ROM_LOAD( "a7118.256",   0x50000, 0x8000, 0x330cd9d7 )
    ROM_LOAD( "a7119.256",   0x58000, 0x8000, 0xfc4b29e0 )
    ROM_LOAD( "a7120.256",   0x60000, 0x8000, 0x65acb265 )
    ROM_LOAD( "a7121.256",   0x68000, 0x8000, 0x33cde9b2 )
    ROM_LOAD( "a7122.256",   0x70000, 0x8000, 0xfb98eac0 )
    ROM_LOAD( "a7123.256",   0x78000, 0x8000, 0x30bd46ad )

    ROM_REGION(0x10000)	/* video CPU */
    ROM_LOAD( "a7101.256",   0x00000, 0x8000, 0x0867c707 )

    ROM_REGION(0x10000)	/* audio CPU */
    ROM_LOAD( "a7107.256",   0x0000, 0x08000, 0xf298cc7b )
ROM_END

ROM_START( tokiob_rom )
    ROM_REGION(0x30000) /* main CPU */
    ROM_LOAD( "2",           0x00000, 0x8000, 0xf583b1ef )
    /* ROMs banked at 8000-bfff */
    ROM_LOAD( "3",           0x10000, 0x8000, 0x69dacf44 )
    ROM_LOAD( "a7104.256",   0x18000, 0x8000, 0xa0a4ce0e )
    ROM_LOAD( "a7105.256",   0x20000, 0x8000, 0x6da0b945 )
    ROM_LOAD( "6",           0x28000, 0x8000, 0x1490e95b )

    ROM_REGION_DISPOSE(0x80000)	/* temporary space for graphics */
    /* 1st plane */
    ROM_LOAD( "a7108.256",   0x00000, 0x8000, 0x0439ab13 )
    ROM_LOAD( "a7109.256",   0x08000, 0x8000, 0xedb3d2ff )
    ROM_LOAD( "a7110.256",   0x10000, 0x8000, 0x69f0888c )
    ROM_LOAD( "a7111.256",   0x18000, 0x8000, 0x4ae07c31 )
    ROM_LOAD( "a7112.256",   0x20000, 0x8000, 0x3f6bd706 )
    ROM_LOAD( "a7113.256",   0x28000, 0x8000, 0xf2c92aaa )
    ROM_LOAD( "a7114.256",   0x30000, 0x8000, 0xc574b7b2 )
    ROM_LOAD( "a7115.256",   0x38000, 0x8000, 0x12d87e7f )
    /* 2nd plane */
    ROM_LOAD( "a7116.256",   0x40000, 0x8000, 0x0bce35b6 )
    ROM_LOAD( "a7117.256",   0x48000, 0x8000, 0xdeda6387 )
    ROM_LOAD( "a7118.256",   0x50000, 0x8000, 0x330cd9d7 )
    ROM_LOAD( "a7119.256",   0x58000, 0x8000, 0xfc4b29e0 )
    ROM_LOAD( "a7120.256",   0x60000, 0x8000, 0x65acb265 )
    ROM_LOAD( "a7121.256",   0x68000, 0x8000, 0x33cde9b2 )
    ROM_LOAD( "a7122.256",   0x70000, 0x8000, 0xfb98eac0 )
    ROM_LOAD( "a7123.256",   0x78000, 0x8000, 0x30bd46ad )

    ROM_REGION(0x10000)	/* video CPU */
    ROM_LOAD( "a7101.256",   0x00000, 0x8000, 0x0867c707 )

    ROM_REGION(0x10000)	/* audio CPU */
    ROM_LOAD( "a7107.256",   0x0000, 0x08000, 0xf298cc7b )
ROM_END



void tokio_patch(void)
{
        Machine->memory_region[0][0x0c13] = 0x18; /* patch protection */
        Machine->memory_region[0][0x0c14] = 0x20;
}



static int hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if (memcmp(&RAM[0xf4c0],"\x61,\x28",2) == 0 )
        {
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xf4c0],3);
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
		osd_fwrite(f,&RAM[0xf4c0],3);
		osd_fclose(f);
	}
}



struct GameDriver tokio_driver =
{
	__FILE__,
	0,
	"tokio",
	"Tokio / Scramble Formation",
	"1986",
	"Taito",
	"Victor Trucco\nMarcelo de G. Malheiros\nFredrik Sjostedt\n---------------\nChris Moore\nOliver White\nNicola Salmoria",
	GAME_NOT_WORKING,
	&tokio_machine_driver,
	0,

	tokio_rom,
	tokio_patch, 0,	/* remove protection */
	0,
	0,	/* sound_prom */

	tokio_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver tokiob_driver =
{
	__FILE__,
	&tokio_driver,
	"tokiob",
	"Tokio / Scramble Formation (bootleg)",
	"1986",
	"bootleg",
	"Marcelo de G. Malheiros\nFredrik Sjostedt\nVictor Trucco\n---------------\nChris Moore\nOliver White\nNicola Salmoria",
	0,
	&tokio_machine_driver,
	0,

	tokiob_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tokiob_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};
