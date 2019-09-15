/***************************************************************************

remaining issues:
	busy loop should be patched out

Please contact Phil Stroffolino (phil@maya.com) if there are any questions
regarding this driver.

Tiger Road is (C) 1987 Romstar/Capcom USA

Memory Overview:
	0xfe0800    sprites
	0xfec000    text
	0xfe4000    input ports,dip switches (read); sound out, video reg (write)
	0xfe8000    scroll registers
	0xff8200    palette
	0xffC000    working RAM
	0xffEC70    high scores (not saved)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80/Z80.h"


extern int tigeroad_base_bank;
extern unsigned char *tigeroad_scrollram;

void tigeroad_scrollram_w(int offset,int data);
void tigeroad_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static unsigned char *ram;	/* for hi score save */



static void tigeroad_control_w( int offset, int data )
{
	switch( offset )
	{
		case 0:
			tigeroad_base_bank = (data>>8)&0xF;
			break;
		case 2:
			soundlatch_w(offset,(data >> 8) & 0xff);
			break;
	}
}

static int tigeroad_input_r (int offset);
static int tigeroad_input_r (int offset){
	switch (offset){
		case 0: return (input_port_1_r( offset )<<8) |
						input_port_0_r( offset );

		case 2: return (input_port_3_r( offset )<<8) |
						input_port_2_r( offset );

		case 4: return (input_port_5_r( offset )<<8) |
						input_port_4_r( offset );
	}
	return 0x00;
}

int tigeroad_interrupt(void){
	return 2;
}


/***************************************************************************/

static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0xfe0800, 0xfe0cff, MRA_BANK1 },
	{ 0xfe0d00, 0xfe1807, MRA_BANK2 },
	{ 0xfe4000, 0xfe4007, tigeroad_input_r },
	{ 0xfec000, 0xfec7ff, MRA_BANK3 },
	{ 0xff8200, 0xff867f, paletteram_word_r },
	{ 0xffc000, 0xffffff, MRA_BANK5 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0xfe0800, 0xfe0cff, MWA_BANK1, &spriteram, &spriteram_size },
	{ 0xfe0d00, 0xfe1807, MWA_BANK2 },  /* still part of OBJ RAM */
	{ 0xfe4000, 0xfe4003, tigeroad_control_w },
	{ 0xfec000, 0xfec7ff, MWA_BANK3, &videoram, &videoram_size },
	{ 0xfe8000, 0xfe8003, MWA_BANK4, &tigeroad_scrollram },
	{ 0xfe800c, 0xfe800f, MWA_NOP },    /* fe800e = watchdog or IRQ acknowledge */
	{ 0xff8200, 0xff867f, paletteram_xxxxRRRRGGGGBBBB_word_w, &paletteram },
	{ 0xffc000, 0xffffff, MWA_BANK5, &ram },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8000, YM2203_status_port_0_r },
	{ 0xa000, 0xa000, YM2203_status_port_1_r },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xe000, 0xe000, soundlatch_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8000, YM2203_control_port_0_w },
	{ 0x8001, 0x8001, YM2203_write_port_0_w },
	{ 0xa000, 0xa000, YM2203_control_port_1_w },
	{ 0xa001, 0xa001, YM2203_write_port_1_w },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ -1 }  /* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x01, IOWP_NOP },   /* ??? */
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( tigeroad_input_ports )
	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START  /* dipswitch A */
	PORT_DIPNAME( 0x07, 0x07, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x38, 0x38, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x18, "1 Coin/6 Credits" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START /* dipswitch B */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright")
	PORT_DIPSETTING(    0x04, "Cocktail")
	PORT_DIPNAME( 0x18, 0x18, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "20000 70000 70000" )
	PORT_DIPSETTING(    0x10, "20000 80000 80000" )
	PORT_DIPSETTING(    0x08, "30000 80000 80000" )
	PORT_DIPSETTING(    0x00, "30000 90000 90000" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Very Easy" )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x60, "Normal" )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x80, "Yes" )
INPUT_PORTS_END

INPUT_PORTS_START( f1dream_input_ports )
	PORT_START  /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START  /* dipswitch A */
	PORT_DIPNAME( 0x07, 0x07, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x38, 0x38, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x18, "1 Coin/6 Credits" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START /* dipswitch B */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright")
	PORT_DIPSETTING(    0x04, "Cocktail")
	PORT_DIPNAME( 0x18, 0x18, "F1 Up Point", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "12" )
	PORT_DIPSETTING(    0x10, "16" )
	PORT_DIPSETTING(    0x08, "18" )
	PORT_DIPSETTING(    0x00, "20" )
	PORT_DIPNAME( 0x20, 0x20, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Normal" )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPNAME( 0x40, 0x00, "Version", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "International" )
	PORT_DIPSETTING(    0x40, "Japan" )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x80, "Yes" )
INPUT_PORTS_END



static struct GfxLayout text_layout =
{
	8,8,    /* character size */
	2048,   /* number of characters */
	2,      /* bits per pixel */
	{ 4, 0 }, /* plane offsets */
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 }, /* x offsets */
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 }, /* y offsets */
	16*8
};

static struct GfxLayout tile_layout =
{
	32,32,  /* tile size */
	2048,   /* number of tiles */
	4,      /* bits per pixel */

	{ 2048*256*8+4, 2048*256*8+0, 4, 0 },

	{ /* x offsets */
		0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
		64*8+0, 64*8+1, 64*8+2, 64*8+3, 64*8+8+0, 64*8+8+1, 64*8+8+2, 64*8+8+3,
		2*64*8+0, 2*64*8+1, 2*64*8+2, 2*64*8+3, 2*64*8+8+0, 2*64*8+8+1, 2*64*8+8+2, 2*64*8+8+3,
		3*64*8+0, 3*64*8+1, 3*64*8+2, 3*64*8+3, 3*64*8+8+0, 3*64*8+8+1, 3*64*8+8+2, 3*64*8+8+3,
	},

	{ /* y offsets */
		0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16,
		16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16,
		24*16, 25*16, 26*16, 27*16, 28*16, 29*16, 30*16, 31*16
	},

	256*8
};

static struct GfxLayout sprite_layout =
{
	16,16,  /* tile size */
	4096,   /* number of tiles */
	4,      /* bits per pixel */
	{ 3*4096*32*8, 2*4096*32*8, 1*4096*32*8, 0*4096*32*8 }, /* plane offsets */
	{ /* x offsets */
		0, 1, 2, 3, 4, 5, 6, 7,
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7
	},
	{ /* y offsets */
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8
	},
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x000000, &tile_layout,     0, 16 },   /* colors   0-255 */
	{ 1, 0x100000, &sprite_layout, 256, 16 },   /* colors 256-511 */
	{ 1, 0x180000, &text_layout,   512, 16 },   /* colors 512-575 */
	{ -1 } /* end of array */
};



/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(void)
{
	cpu_cause_interrupt(1,0xff);
}

static struct YM2203interface ym2203_interface =
{
	2,          /* 2 chips */
	3500000,    /* 3.5 MHz ? */
	{ YM2203_VOL(255,255), YM2203_VOL(255,255) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ irqhandler }
};



static struct MachineDriver machine_driver =
{
	{
		{
			CPU_M68000,
			6000000, /* ? Main clock is 24MHz */
			0,
			readmem,writemem,0,0,
			tigeroad_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,    /* 4 Mhz ??? */
			3,  /* memory region #3 */
			sound_readmem,sound_writemem,0,sound_writeport,
			ignore_interrupt,0  /* NMIs are triggered by the main CPU */
								/* IRQs are triggered by the YM2203 */
		}
	},
	60, 2500,   /* frames per second, vblank duration */
				/* vblank duration hand tuned to get proper sprite/background alignment */
	1,  /* CPU slices per frame */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	576, 576,
	0, /* convert color prom routine */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	0,
	0,
	tigeroad_vh_screenrefresh,

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

ROM_START( tigeroad_rom )
	ROM_REGION(0x40000) /* 256K for 68000 code */
	ROM_LOAD_EVEN( "tru02.bin",    0x00000, 0x20000, 0x8d283a95 )
	ROM_LOAD_ODD( "tru04.bin",    0x00000, 0x20000, 0x72e2ef20 )

	ROM_REGION_DISPOSE(0x188000) /* temporary space for graphics */
	ROM_LOAD( "tr-01a.bin",   0x000000, 0x20000, 0xa8aa2e59 ) /* tiles */
	ROM_LOAD( "tr-04a.bin",   0x020000, 0x20000, 0x8863a63c )
	ROM_LOAD( "tr-02a.bin",   0x040000, 0x20000, 0x1a2c5f89 )
	ROM_LOAD( "tr05.bin",     0x060000, 0x20000, 0x5bf453b3 )
	ROM_LOAD( "tr-03a.bin",   0x080000, 0x20000, 0x1e0537ea )
	ROM_LOAD( "tr-06a.bin",   0x0A0000, 0x20000, 0xb636c23a )
	ROM_LOAD( "tr-07a.bin",   0x0C0000, 0x20000, 0x5f907d4d )
	ROM_LOAD( "tr08.bin",     0x0E0000, 0x20000, 0xadee35e2 )
	ROM_LOAD( "tr-09a.bin",   0x100000, 0x20000, 0x3d98ad1e ) /* sprites */
	ROM_LOAD( "tr-10a.bin",   0x120000, 0x20000, 0x8f6f03d7 )
	ROM_LOAD( "tr-11a.bin",   0x140000, 0x20000, 0xcd9152e5 )
	ROM_LOAD( "tr-12a.bin",   0x160000, 0x20000, 0x7d8a99d0 )
	ROM_LOAD( "tr01.bin",     0x180000, 0x08000, 0x74a9f08c ) /* 8x8 text */

	ROM_REGION( 0x08000 ) /* tilemap for background */
	ROM_LOAD( "tr13.bin",     0x0000, 0x8000, 0xa79be1eb )

	ROM_REGION( 0x10000 ) /* audio CPU */
	ROM_LOAD( "tru05.bin",    0x0000, 0x8000, 0xf9a7c9bf )
ROM_END

ROM_START( f1dream_rom )
	ROM_REGION(0x40000) /* 256K for 68000 code */
	ROM_LOAD_EVEN( "06j_02.bin",   0x00000, 0x20000, 0x3c2ec697 )
	ROM_LOAD_ODD( "06k_03.bin",   0x00000, 0x20000, 0x85ebad91 )

	ROM_REGION_DISPOSE(0x188000) /* temporary space for graphics */
	ROM_LOAD( "03f_12.bin",   0x000000, 0x10000, 0xbc13e43c ) /* tiles */
	ROM_LOAD( "01f_10.bin",   0x010000, 0x10000, 0xf7617ad9 )
	ROM_LOAD( "03h_14.bin",   0x020000, 0x10000, 0xe33cd438 )
	/* 30000-7ffff empty */
	ROM_LOAD( "02f_11.bin",   0x080000, 0x10000, 0x4aa49cd7 )
	ROM_LOAD( "17f_09.bin",   0x090000, 0x10000, 0xca622155 )
	ROM_LOAD( "02h_13.bin",   0x0a0000, 0x10000, 0x2a63961e )
	/* b0000-fffff empty */
	ROM_LOAD( "03b_06.bin",   0x100000, 0x10000, 0x5e54e391 ) /* sprites */
	/* 110000-11ffff empty */
	ROM_LOAD( "02b_05.bin",   0x120000, 0x10000, 0xcdd119fd )
	/* 130000-13ffff empty */
	ROM_LOAD( "03d_08.bin",   0x140000, 0x10000, 0x811f2e22 )
	/* 150000-15ffff empty */
	ROM_LOAD( "02d_07.bin",   0x160000, 0x10000, 0xaa9a1233 )
	/* 170000-17ffff empty */
	ROM_LOAD( "10d_01.bin",   0x180000, 0x08000, 0x361caf00 ) /* 8x8 text */

	ROM_REGION( 0x08000 ) /* tilemap for background */
	ROM_LOAD( "07l_15.bin",   0x0000, 0x8000, 0x978758b7 )

	ROM_REGION( 0x10000 ) /* audio CPU */
	ROM_LOAD( "12k_04.bin",   0x0000, 0x8000, 0x4b9a7524 )
ROM_END

ROM_START( f1dreamb_rom )
	ROM_REGION(0x40000) /* 256K for 68000 code */
	ROM_LOAD_EVEN( "f1d_04.bin",   0x00000, 0x10000, 0x903febad )
	ROM_LOAD_ODD( "f1d_05.bin",   0x00000, 0x10000, 0x666fa2a7 )
	ROM_LOAD_EVEN( "f1d_02.bin",   0x20000, 0x10000, 0x98973c4c )
	ROM_LOAD_ODD( "f1d_03.bin",   0x20000, 0x10000, 0x3d21c78a )

	ROM_REGION_DISPOSE(0x188000) /* temporary space for graphics */
	ROM_LOAD( "03f_12.bin",   0x000000, 0x10000, 0xbc13e43c ) /* tiles */
	ROM_LOAD( "01f_10.bin",   0x010000, 0x10000, 0xf7617ad9 )
	ROM_LOAD( "03h_14.bin",   0x020000, 0x10000, 0xe33cd438 )
	/* 30000-7ffff empty */
	ROM_LOAD( "02f_11.bin",   0x080000, 0x10000, 0x4aa49cd7 )
	ROM_LOAD( "17f_09.bin",   0x090000, 0x10000, 0xca622155 )
	ROM_LOAD( "02h_13.bin",   0x0a0000, 0x10000, 0x2a63961e )
	/* b0000-fffff empty */
	ROM_LOAD( "03b_06.bin",   0x100000, 0x10000, 0x5e54e391 ) /* sprites */
	/* 110000-11ffff empty */
	ROM_LOAD( "02b_05.bin",   0x120000, 0x10000, 0xcdd119fd )
	/* 130000-13ffff empty */
	ROM_LOAD( "03d_08.bin",   0x140000, 0x10000, 0x811f2e22 )
	/* 150000-15ffff empty */
	ROM_LOAD( "02d_07.bin",   0x160000, 0x10000, 0xaa9a1233 )
	/* 170000-17ffff empty */
	ROM_LOAD( "10d_01.bin",   0x180000, 0x08000, 0x361caf00 ) /* 8x8 text */

	ROM_REGION( 0x08000 ) /* tilemap for background */
	ROM_LOAD( "07l_15.bin",   0x0000, 0x8000, 0x978758b7 )

	ROM_REGION( 0x10000 ) /* audio CPU */
	ROM_LOAD( "12k_04.bin",   0x0000, 0x8000, 0x4b9a7524 )
ROM_END



static int tigeroad_hiload(void)
{
	void *f;


	/* check if the hi score table has already been initialized */
	if (memcmp(&ram[0x2c70],"UYIK",4) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&ram[0x2c70],100);
			ram[0x0092]=ram[0x2cac];
			ram[0x0093]=ram[0x2cad];
			ram[0x0094]=ram[0x2cae];
			ram[0x0095]=ram[0x2caf];
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}


static void tigeroad_hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&ram[0x2c70],100);
		osd_fclose(f);
	}
}



struct GameDriver tigeroad_driver =
{
	__FILE__,
	0,
	"tigeroad",
	"Tiger Road",
	"1987",
	"Capcom (Romstar license)",
	"Phil Stroffolino (MAME driver)\nTim Lindquist",
	0,
	&machine_driver,
	0,

	tigeroad_rom,
	0,0,0,0,

	tigeroad_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	tigeroad_hiload, tigeroad_hisave
};

/* F1 Dream has an Intel 8751 microcontroller for protection */
struct GameDriver f1dream_driver =
{
	__FILE__,
	0,
	"f1dream",
	"F1 Dream",
	"1988",
	"Capcom (Romstar license)",
	"Paul Leaman\nPhil Stroffolino (MAME driver)\nTim Lindquist",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	f1dream_rom,
	0,0,0,0,

	f1dream_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver f1dreamb_driver =
{
	__FILE__,
	&f1dream_driver,
	"f1dreamb",
	"F1 Dream (bootleg)",
	"1988",
	"bootleg",
	"Paul Leaman\nPhil Stroffolino (MAME driver)\nTim Lindquist",
	0,
	&machine_driver,
	0,

	f1dreamb_rom,
	0,0,0,0,

	f1dream_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

