/****************************************************************************

Lode Runner

There's two crystals on Kid Kiki. 24.00 MHz and 3.579545 MHz for sound

**************************************************************************/

#include "driver.h"
#include "Z80/Z80.h"
#include "vidhrdw/generic.h"


void ldrun_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int ldrun_vh_start(void);
void ldrun_vh_stop(void);
void ldrun2p_scroll_w(int offset,int data);
void ldrun_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void ldrun2p_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern struct AY8910interface irem_ay8910_interface;
extern struct MSM5205interface irem_msm5205_interface;
void irem_io_w(int offset, int value);
int irem_io_r(int offset);
void irem_sound_cmd_w(int offset, int value);



void ldrun2p_bankswitch_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + ((data & 0x01) * 0x4000);
	cpu_setbank(1,&RAM[bankaddress]);
}



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
	{ 0xc000, 0xc0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd000, 0xdfff, videoram_w, &videoram, &videoram_size },
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
//	{ 0x01, 0x01, kungfum_flipscreen_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress ldrun2p_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xd000, 0xdfff, MRA_RAM },         /* Video and Color ram */
	{ 0xe000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress ldrun2p_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xc800, 0xc800, ldrun2p_bankswitch_w },
	{ 0xd000, 0xdfff, videoram_w, &videoram, &videoram_size },
	{ 0xe000, 0xefff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOWritePort ldrun2p_writeport[] =
{
	{ 0x00, 0x00, irem_sound_cmd_w },
//	{ 0x01, 0x01, kungfum_flipscreen_w },	/* coin counter? */
	{ 0x82, 0x83, ldrun2p_scroll_w },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x001f, irem_io_r },
	{ 0x0080, 0x00ff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x001f, irem_io_w },
	{ 0x0080, 0x00ff, MWA_RAM },
	{ 0x0801, 0x0802, MSM5205_data_w },
	{ 0x9000, 0x9000, MWA_NOP },    /* IACK */
	{ 0x8000, 0xffff, MWA_ROM },
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
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

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
	PORT_DIPNAME( 0x03, 0x03, "Timer Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Slow" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Fast" )
	PORT_DIPSETTING(    0x00, "Fastest" )
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
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
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

INPUT_PORTS_START( ldrun2p_input_ports )
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
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Timer Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x02, 0x02, "2 Players Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "1 Player Lives", IP_KEY_NONE )
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
	PORT_DIPNAME( 0x02, 0x02, "2 Players Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "6" )
/* This activates a different coin mode. Look at the dip switch setting schematic */
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Allow 2 Players Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x10, "Yes" )
	/* In level selection mode, press 1 to select and 2 to restart */
	PORT_BITX   ( 0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Level Selection Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode (must set 2P game to No)", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 2*256*32*8, 256*32*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout ldrun2p_charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	3,	/* 3 bits per pixel */
	{ 2*2048*8*8, 2048*8*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout ldrun2p_spritelayout =
{
	16,16,	/* 16*16 sprites */
	1024,	/* 1024 sprites */
	3,	/* 3 bits per pixel */
	{ 2*1024*32*8, 1024*32*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,      0, 32 },	/* use colors   0-255 */
	{ 1, 0x06000, &spritelayout, 32*8, 32 },	/* use colors 256-511 */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo ldrun2p_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &ldrun2p_charlayout,      0, 32 },	/* use colors   0-255 */
	{ 1, 0x0c000, &ldrun2p_spritelayout, 32*8, 32 },	/* use colors 256-511 */
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
			readmem,writemem,readport,writeport,
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
	55, 1790, /* frames per second and vblank duration from the Lode Runner manual */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 8*8, (64-8)*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	512, 512,
	ldrun_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	ldrun_vh_start,
	ldrun_vh_stop,
	ldrun_vh_screenrefresh,

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

static struct MachineDriver ldrun2p_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			ldrun2p_readmem,ldrun2p_writemem,readport,ldrun2p_writeport,
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
	55, 1790, /* frames per second and vblank duration from the Lode Runner manual */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 8*8, (64-8)*8-1, 0*8, 32*8-1 },
	ldrun2p_gfxdecodeinfo,
	512, 512,
	ldrun_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	ldrun_vh_start,
	ldrun_vh_stop,
	ldrun2p_vh_screenrefresh,

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

ROM_START( ldrun_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "lr-a-4e",      0x0000, 0x2000, 0x5d7e2a4d )
	ROM_LOAD( "lr-a-4d",      0x2000, 0x2000, 0x96f20473 )
	ROM_LOAD( "lr-a-4b",      0x4000, 0x2000, 0xb041c4a9 )
	ROM_LOAD( "lr-a-4a",      0x6000, 0x2000, 0x645e42aa )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lr-e-2d",      0x0000, 0x2000, 0x24f9b58d )	/* characters */
	ROM_LOAD( "lr-e-2j",      0x2000, 0x2000, 0x43175e08 )
	ROM_LOAD( "lr-e-2f",      0x4000, 0x2000, 0xe0317124 )
	ROM_LOAD( "lr-b-4k",      0x6000, 0x2000, 0x8141403e )	/* sprites */
	ROM_LOAD( "lr-b-3n",      0x8000, 0x2000, 0x55154154 )
	ROM_LOAD( "lr-b-4c",      0xa000, 0x2000, 0x924e34d0 )

	ROM_REGION(0x0620)	/* color proms */
	ROM_LOAD( "lr-e-3m",      0x0000, 0x0100, 0x53040416 )	/* character palette red component */
	ROM_LOAD( "lr-b-1m",      0x0100, 0x0100, 0x4bae1c25 )	/* sprite palette red component */
	ROM_LOAD( "lr-e-3l",      0x0200, 0x0100, 0x67786037 )	/* character palette green component */
	ROM_LOAD( "lr-b-1n",      0x0300, 0x0100, 0x9cd3db94 )	/* sprite palette green component */
	ROM_LOAD( "lr-e-3n",      0x0400, 0x0100, 0x5b716837 )	/* character palette blue component */
	ROM_LOAD( "lr-b-1l",      0x0500, 0x0100, 0x08d8cf9a )	/* sprite palette blue component */
	ROM_LOAD( "lr-b-5p",      0x0600, 0x0020, 0xe01f69e2 )	/* sprite height, one entry per 32 */
														/*   sprites. Used at run time! */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "lr-a-3f",      0xc000, 0x2000, 0x7a96accd )	/* samples (ADPCM 4-bit) */
	ROM_LOAD( "lr-a-3h",      0xe000, 0x2000, 0x3f7f3939 )
ROM_END

ROM_START( ldruna_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "roma4c",       0x0000, 0x2000, 0x279421e1 )
	ROM_LOAD( "lr-a-4d",      0x2000, 0x2000, 0x96f20473 )
	ROM_LOAD( "roma4b",       0x4000, 0x2000, 0x3c464bad )
	ROM_LOAD( "roma4a",       0x6000, 0x2000, 0x899df8e0 )

	ROM_REGION_DISPOSE(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lr-e-2d",      0x0000, 0x2000, 0x24f9b58d )	/* characters */
	ROM_LOAD( "lr-e-2j",      0x2000, 0x2000, 0x43175e08 )
	ROM_LOAD( "lr-e-2f",      0x4000, 0x2000, 0xe0317124 )
	ROM_LOAD( "lr-b-4k",      0x6000, 0x2000, 0x8141403e )	/* sprites */
	ROM_LOAD( "lr-b-3n",      0x8000, 0x2000, 0x55154154 )
	ROM_LOAD( "lr-b-4c",      0xa000, 0x2000, 0x924e34d0 )

	ROM_REGION(0x0620)	/* color proms */
	ROM_LOAD( "lr-e-3m",      0x0000, 0x0100, 0x53040416 )	/* character palette red component */
	ROM_LOAD( "lr-b-1m",      0x0100, 0x0100, 0x4bae1c25 )	/* sprite palette red component */
	ROM_LOAD( "lr-e-3l",      0x0200, 0x0100, 0x67786037 )	/* character palette green component */
	ROM_LOAD( "lr-b-1n",      0x0300, 0x0100, 0x9cd3db94 )	/* sprite palette green component */
	ROM_LOAD( "lr-e-3n",      0x0400, 0x0100, 0x5b716837 )	/* character palette blue component */
	ROM_LOAD( "lr-b-1l",      0x0500, 0x0100, 0x08d8cf9a )	/* sprite palette blue component */
	ROM_LOAD( "lr-b-5p",      0x0600, 0x0020, 0xe01f69e2 )	/* sprite height, one entry per 32 */
														/*   sprites. Used at run time! */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "lr-a-3f",      0xc000, 0x2000, 0x7a96accd )	/* samples (ADPCM 4-bit) */
	ROM_LOAD( "lr-a-3h",      0xe000, 0x2000, 0x3f7f3939 )
ROM_END

ROM_START( ldrun2p_rom )
	ROM_REGION(0x18000)	/* 64k for code + 32k for banked ROM */
	ROM_LOAD( "lr4-a-4e",     0x00000, 0x4000, 0x5383e9bf )
	ROM_LOAD( "lr4-a-4d.c",   0x04000, 0x4000, 0x298afa36 )
	ROM_LOAD( "lr4-v-4k",     0x10000, 0x8000, 0x8b248abd )	/* banked at 8000-bfff */

	ROM_REGION_DISPOSE(0x24000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "lr4-v-2b",     0x00000, 0x4000, 0x4118e60a )	/* characters */
	ROM_LOAD( "lr4-v-2d",     0x04000, 0x4000, 0x542bb5b5 )
	ROM_LOAD( "lr4-v-2c",     0x08000, 0x4000, 0xc765266c )
	ROM_LOAD( "lr4-b-4k",     0x0c000, 0x4000, 0xe7fe620c )	/* sprites */
	ROM_LOAD( "lr4-b-4f",     0x10000, 0x4000, 0x6f0403db )
	ROM_LOAD( "lr4-b-3n",     0x14000, 0x4000, 0xad1fba1b )
	ROM_LOAD( "lr4-b-4n",     0x18000, 0x4000, 0x0e568fab )
	ROM_LOAD( "lr4-b-4c",     0x1c000, 0x4000, 0x82c53669 )
	ROM_LOAD( "lr4-b-4e",     0x20000, 0x4000, 0x767a1352 )

	ROM_REGION(0x0620)	/* color proms */
	ROM_LOAD( "lr4-v-1m",     0x0000, 0x0100, 0xfe51bf1d ) /* character palette red component */
	ROM_LOAD( "lr4-b-1m",     0x0100, 0x0100, 0x5d8d17d0 ) /* sprite palette red component */
	ROM_LOAD( "lr4-v-1n",     0x0200, 0x0100, 0xda0658e5 ) /* character palette green component */
	ROM_LOAD( "lr4-b-1n",     0x0300, 0x0100, 0xda1129d2 ) /* sprite palette green component */
	ROM_LOAD( "lr4-v-1p",     0x0400, 0x0100, 0x0df23ebe ) /* character palette blue component */
	ROM_LOAD( "lr4-b-1l",     0x0500, 0x0100, 0x0d89b692 ) /* sprite palette blue component */
	ROM_LOAD( "lr4-b-5p",     0x0600, 0x0020, 0xe01f69e2 )	/* sprite height, one entry per 32 */
														/*   sprites. Used at run time! */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "lr4-a-3d",     0x8000, 0x4000, 0x86c6d445 )
	ROM_LOAD( "lr4-a-3f",     0xc000, 0x4000, 0x097c6c0a )
ROM_END


static int ldrun_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0xE5E5],"\x01\x01\x00",3) == 0 &&
			memcmp(&RAM[0xE6AA],"\x20\x20\x04",3) == 0 )
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xE5E5],200);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void ldrun_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xE5E5],200);
		osd_fclose(f);
	}
}


static int ldrun2p_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0xE735],"\x00\x28\x76",3) == 0 &&
			memcmp(&RAM[0xE7FA],"\x20\x20\x06",3) == 0 )
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xE735],200);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void ldrun2p_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xE735],200);
		osd_fclose(f);
	}
}


struct GameDriver ldrun_driver =
{
	__FILE__,
	0,
	"ldrun",
	"Lode Runner (set 1)",
	"1984",
	"Irem (licensed from Broderbund)",
	"Lee Taylor\nJohn Clegg\nAaron Giles (sound)",
	0,
	&machine_driver,
	0,

	ldrun_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	ldrun_hiload, ldrun_hisave
};

struct GameDriver ldruna_driver =
{
	__FILE__,
	&ldrun_driver,
	"ldruna",
	"Lode Runner (set 2)",
	"1984",
	"Irem (licensed from Broderbund)",
	"Lee Taylor\nJohn Clegg\nAaron Giles (sound)",
	0,
	&machine_driver,
	0,

	ldruna_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	ldrun_hiload, ldrun_hisave
};

struct GameDriver ldrun2p_driver =
{
	__FILE__,
	0,
	"ldrun2p",
	"Lode Runner IV - Teikoku Karano Dasshutsu",
	"1986",
	"Irem (licensed from Broderbund)",
	"Lee Taylor\nJohn Clegg\nAaron Giles (sound)\nNicola Salmoria",
	0,
	&ldrun2p_machine_driver,
	0,

	ldrun2p_rom,
	0, 0,
	0,
	0,

	ldrun2p_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	ldrun2p_hiload, ldrun2p_hisave
};



/***************************************************************************

Kid Niki
1986 Irem (licensed to Data East)

Spelunker (needs fixed colors)
1986 Irem (licensed from Broderbund)

Z80 (main CPU)
sound: 6803 + YM2149(2) + OKI M5205(2)

***************************************************************************/
extern void irem_background_hscroll_w( int offset, int data );
extern void irem_background_vscroll_w( int offset, int data );
extern void irem_text_vscroll_w( int offset, int data );
extern void irem_background_bank_w( int offset, int data );
extern void irem_vh_stop( void );

extern int kidniki_vh_start( void );
extern int spelunk2_vh_start( void );

extern void kidniki_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh );
extern void spelunk2_vh_screenrefresh( struct osd_bitmap *bitmap, int fullrefresh );

extern unsigned char *irem_textram;

/* memory regions */
#define MEM_MAIN_CPU	0
#define MEM_SOUND_CPU	1
#define MEM_TILES		2
#define MEM_CHARS		3
#define MEM_SPRITES		4
#define MEM_COLOR		5
#define MEM_SPR_SIZE	6

/* graphics banks */
#define GFX_TILES		0
#define GFX_CHARS		1
#define GFX_SPRITES		2

static void background_tilemap_w( int offset, int data ){
	videoram[offset] = data;
	dirtybuffer[offset/2] = 1;
}

INPUT_PORTS_START( kidniki_input_ports )
	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT(0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL  )
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_COCKTAIL )
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_COCKTAIL )
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNKNOWN | IPF_COCKTAIL )
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNKNOWN | IPF_COCKTAIL )
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START /* coinage not yet verified - copied from Vigilante */
	PORT_DIPNAME( 0xF0, 0xF0, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(	0xA0, "6 Coins/1 Credit" )
	PORT_DIPSETTING(	0xB0, "5 Coins/1 Credit" )
	PORT_DIPSETTING(	0xC0, "4 Coins/1 Credit" )
	PORT_DIPSETTING(	0xD0, "3 Coins/1 Credit" )
	PORT_DIPSETTING(	0x10, "8 Coins/3 Credits" )
	PORT_DIPSETTING(	0xE0, "2 Coins/1 Credit" )
	PORT_DIPSETTING(	0x20, "5 Coins/3 Credits" )
	PORT_DIPSETTING(	0x30, "3 Coins/2 Credits" )
	PORT_DIPSETTING(	0xF0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(	0x40, "2 Coins/3 Credits" )
	PORT_DIPSETTING(	0x90, "1 Coin/2 Credits" )
	PORT_DIPSETTING(	0x80, "1 Coin/3 Credits" )
	PORT_DIPSETTING(	0x70, "1 Coin/4 Credits" )
	PORT_DIPSETTING(	0x60, "1 Coin/5 Credits" )
	PORT_DIPSETTING(	0x50, "1 Coin/6 Credits" )
	PORT_DIPSETTING(	0x00, "Free Play" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(	0x01, "Off" )
	PORT_DIPSETTING(	0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode", IP_KEY_NONE )
	PORT_DIPSETTING(	0x04, "Mode 1" )
	PORT_DIPSETTING(	0x00, "Mode 2" )
	PORT_DIPNAME( 0x08, 0x08, "Game Repeats", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "Yes" )
	PORT_DIPSETTING(	0x08, "No" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "No" )
	PORT_DIPSETTING(	0x10, "Yes" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

static struct IOReadPort irem_readport[] = {
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ 0x04, 0x04, input_port_4_r },
	{ -1 }	/* end of table */
};

static void kidniki_bankswitch_w(int offset,int data){
	static int bank_table[16] = {
		0,1,2,3, 4,5,6,7,
		12,12,12,12, 8,9,10,11
	};

	int base = bank_table[data&0xf] * 0x2000;
	unsigned char *mem = &Machine->memory_region[MEM_MAIN_CPU][0x10000];
	cpu_setbank( 1,&mem[base] );
}

static struct IOWritePort kidniki_writeport[] =
{
	{ 0x00, 0x00, irem_sound_cmd_w },
	/* 0x01 is most likely screen flip */
	{ 0x80, 0x81, irem_background_hscroll_w },
	{ 0x82, 0x83, irem_text_vscroll_w },
	{ 0x84, 0x84, irem_background_bank_w },
	{ 0x85, 0x85, kidniki_bankswitch_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress kidniki_readmem[] = {
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_BANK1 },
	{ 0xa000, 0xafff, MRA_RAM, &videoram },
	{ 0xc000, 0xc0ff, MRA_RAM },
	{ 0xd000, 0xdfff, MRA_RAM, &irem_textram },
	{ 0xe000, 0xe3ff, MRA_RAM },
	{ 0xe400, 0xffff, MRA_RAM, &spriteram },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress kidniki_writemem[] = {
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xafff, background_tilemap_w },
	{ 0xc000, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};

/*************************************************************/

static struct GfxLayout kidniki_tile_layout =
{
	8,8, /* character size */
	0x1000, /* number of characters */
	3, /* 3 bits per pixel */
	{ 0, 0x8000*8,0x8000*8*2 },
	{ 0, 1, 2, 3, 4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	64	/* character offset */
};

static struct GfxLayout kidniki_char_layout =
{
	12,8, /* character size */
	0x400, /* number of characters */
	3, /* bits per pixel */
	{ 0, 0x4000*8,0x4000*8*2 },
	{ 0, 1, 2, 3, 64+0,64+1,64+2,64+3,64+4,64+5,64+6,64+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	128	/* character offset */
};

static struct GfxLayout kidniki_sprite_layout =
{
	16,16,	/* character size */
	0x800,	/* number of characters */
	3,	/* bits per pixel */
	{ 0,0x80000,0x80000*2 },
	{ 0, 1, 2, 3, 4,5,6,7,
		128+0,128+1,128+2,128+3,128+4,128+5,128+6,128+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },

	256 /* sprite offset */
};

static struct GfxDecodeInfo kidniki_gfxdecodeinfo[] =
{
	{ MEM_TILES, 	0, &kidniki_tile_layout,		256,	32 },
	{ MEM_CHARS,	0, &kidniki_char_layout,		256,	32 },
	{ MEM_SPRITES,	0, &kidniki_sprite_layout,		0,		32 },
	{ -1 } /* end of array */
};

static struct MachineDriver kidniki_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			6000000,	/* unknown, but 4Mhz causes crashes when fighting the final boss */
			MEM_MAIN_CPU,
			kidniki_readmem,kidniki_writemem,
			irem_readport,kidniki_writeport,
			interrupt,1
		},
		{
			CPU_M6803 | CPU_AUDIO_CPU,
			1000000,	/* 1.0 Mhz ? */
			MEM_SOUND_CPU,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are generated by the ADPCM hardware */
		}
	},
	60, 0, /* frames per second and vblank duration from the Lode Runner manual */
	1, /* cpu slices */
	0, /* init machine */

	/* video hardware */
	32*12,	/* screen width */
	32*8,	/* screen height */
	{ 0*8, 32*12-1, 0*8, 32*8-1 }, /* viewport clip */
	kidniki_gfxdecodeinfo,
	512,512,
	ldrun_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	kidniki_vh_start,
	irem_vh_stop,
	kidniki_vh_screenrefresh,

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
		},
	}
};



ROM_START( kidniki_rom )
	ROM_REGION( 0x30000 )	/* main CPU */
	ROM_LOAD( "dr04.4e",	0x0000, 0x4000, 0x80431858 )
	ROM_LOAD( "dr03.4cd",	0x4000, 0x4000, 0xdba20934 )

	ROM_LOAD( "dr11.8k",	0x10000, 0x08000, 0x04d82d93 )
	ROM_LOAD( "dr12.8l",	0x18000, 0x10000, 0xc0b255fd )

	ROM_REGION( 0x10000 )	/* sound CPU */
	ROM_LOAD( "dr00.3a",	0x4000, 0x04000, 0x458309f7 )
	ROM_LOAD( "dr01.3cd",	0x8000, 0x04000, 0xe66897bd )
	ROM_LOAD( "dr02.3f",	0xc000, 0x04000, 0xf9e31e26 ) /* 6803 code */

	ROM_REGION_DISPOSE( 0x18000 ) /* tiles */
	ROM_LOAD( "dr07.2dc",	0x08000, 0x8000, 0xab59a4c4 )
	ROM_LOAD( "dr06.2b",	0x10000, 0x8000, 0x4d9a970f )
	ROM_LOAD( "dr05.2a",	0x00000, 0x8000, 0x2e6dad0c )

	ROM_REGION_DISPOSE( 0xc000 ) /* characters */
	ROM_LOAD( "dr08.4l",	0x0000, 0x4000, 0x32d50643 )
	ROM_LOAD( "dr09.4m",	0x4000, 0x4000, 0x17df6f95 )
	ROM_LOAD( "dr10.4n",	0x8000, 0x4000, 0x820ce252 )

	ROM_REGION_DISPOSE( 0x30000 ) /* sprites */
	ROM_LOAD( "dr16.4cb",	0x00000, 0x4000, 0xf1d1bb93 ) /* plane0 */
	ROM_LOAD( "dr18.4e",	0x04000, 0x4000, 0xedb7f25b )
	ROM_LOAD( "dr17.4dc",	0x08000, 0x4000, 0x4fb87868 )
	ROM_LOAD( "dr15.4a",	0x0c000, 0x4000, 0xe0b88de5 )
	ROM_LOAD( "dr14.3p",	0x10000, 0x4000, 0x76cfbcbc ) /* plane 1 */
	ROM_LOAD( "dr24.4p",	0x14000, 0x4000, 0xd51c8db5 )
	ROM_LOAD( "dr23.4nm",	0x18000, 0x4000, 0x03469df8 )
	ROM_LOAD( "dr13.3nm",	0x1c000, 0x4000, 0xd5c3dfe0 )
	ROM_LOAD( "dr21.4k",	0x20000, 0x4000, 0xa06cea9a ) /* plane 2 */
	ROM_LOAD( "dr19.4f",	0x24000, 0x4000, 0xb34605ad )
	ROM_LOAD( "dr22.4l",	0x28000, 0x4000, 0x41303de8 )
	ROM_LOAD( "dr20.4jh",	0x2c000, 0x4000, 0x5fbe6f61 )

	ROM_REGION( 3*512 ) /* color proms */
	/* sprites */
	ROM_LOAD( "dr30.1m",	0*256,	256, 0x28c73263 )//red
	ROM_LOAD( "dr31.1n",	2*256,	256, 0x3529210e )//green
	ROM_LOAD( "dr29.1l",	4*256,	256, 0x1173a754 )//blue
	/* tile colors */
	ROM_LOAD( "dr25.3f",	1*256,	256, 0x8e91430b )//red
	ROM_LOAD( "dr26.3h",	3*256,	256, 0xb563b93f )//green
	ROM_LOAD( "dr27.3j",	5*256,	256, 0x70d668ef )//blue

	ROM_REGION( 32 ) /* sprite height table */
	ROM_LOAD( "dr32.5p",	0, 32,	0x11cd1f2e )

/* unknown:
	ROM_LOAD( "dr28.8f",	0, 512, 0x6cef0fbd )
	ROM_LOAD( "dr33.6f",	0, 256, 0x34d88d3c )
*/
ROM_END


static int kidniki_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0xE062],"\x00\x10\x00",3) == 0 &&
			memcmp(&RAM[0xE0CE],"\x20\x20\x00",3) == 0 )
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xE062],111);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void kidniki_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xE062],111);
		osd_fclose(f);
	}
}


struct GameDriver kidniki_driver =
{
	__FILE__,
	0,
	"kidniki",
	"Kid Niki, Radical Ninja",
	"1986",
	"Irem (Data East USA license)",
	"Phil Stroffolino\nAaron Giles (sound)",
	0,
	&kidniki_machine_driver,
	0,

	kidniki_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	kidniki_input_ports,

	PROM_MEMORY_REGION(MEM_COLOR), 0, 0,
	ORIENTATION_DEFAULT,

	kidniki_hiload, kidniki_hisave
};

static struct GfxLayout spelunk2_char_layout =
{
	12,8, /* character size */
	256, /* number of characters */
	3, /* bits per pixel */
	{ 0, 0x2000*8, 0x2000*8*2 },
	{
		0,1,2,3,
		0x800*8+0,0x800*8+1,0x800*8+2,0x800*8+3,
		0x800*8+4,0x800*8+5,0x800*8+6,0x800*8+7
	},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	64	/* character offset */
};
static struct GfxDecodeInfo spelunk2_gfxdecodeinfo[] =
{
	{ MEM_TILES, 	0,		&kidniki_tile_layout,	256, 32 },
	{ MEM_CHARS,	0x0000,	&spelunk2_char_layout,	256,32 },
	{ MEM_SPRITES,	0,		&kidniki_sprite_layout,	0, 32 },
	{ MEM_CHARS,	0x1000, &spelunk2_char_layout,	256,32 },
	{ -1 } /* end of array */
};

void spelunk2_port_w( int offset, int data ){
	switch( offset ){
		case 0:
		irem_background_vscroll_w(0,data);
		break;

		case 1:
		irem_background_hscroll_w(0,data);
		break;

		case 2:
		irem_background_hscroll_w(1,(data&2)>>1);
		irem_background_vscroll_w(1,(data&1));
		break;

		case 3:
		{
			unsigned char *mem = Machine->memory_region[MEM_MAIN_CPU];
			cpu_setbank( 1,&mem[0x20000+0x1000*(data>>6)] );
			cpu_setbank( 2,&mem[0x10000+0x400*(data&0x3c)] );
		}
		break;
	}
}

static struct MemoryReadAddress spelunk2_readmem[] = {
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_BANK1 },
	{ 0x9000, 0x9fff, MRA_BANK2 },
	{ 0xa000, 0xafff, MRA_RAM, &videoram },
	{ 0xb000, 0xc7ff, MRA_RAM },
	{ 0xc800, 0xdfff, MRA_RAM, &irem_textram },
	{ 0xe000, 0xe3ff, MRA_RAM },
	{ 0xe400, 0xffff, MRA_RAM, &spriteram },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress spelunk2_writemem[] = {
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xa000, 0xbfff, background_tilemap_w },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd003, spelunk2_port_w },
	{ 0xe000, 0xefff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOWritePort spelunk2_writeport[] =
{
	{ 0x00, 0x00, irem_sound_cmd_w },
	/* 0x01: flip? */
	{ -1 }	/* end of table */
};

static struct MachineDriver spelunk2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			6000000,	/* 4.0 Mhz (?) */
			MEM_MAIN_CPU,
			spelunk2_readmem,spelunk2_writemem,
			irem_readport,spelunk2_writeport,
			interrupt,1
		},
		{
			CPU_M6803 | CPU_AUDIO_CPU,
			1000000,	/* 1.0 Mhz ? */
			MEM_SOUND_CPU,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are generated by the ADPCM hardware */
		}
	},
	60, 0, /* frames per second and vblank duration */
	1, /* cpu slices */
	0, /* init machine */

	/* video hardware */
	32*12,	/* screen width */
	32*8,	/* screen height */
	{ 0*8, 32*12-1, 0*8, 32*8-1 }, /* viewport clip */
	spelunk2_gfxdecodeinfo,
	512,512,
	ldrun_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	spelunk2_vh_start,
	irem_vh_stop,
	spelunk2_vh_screenrefresh,

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
		},
	}
};



ROM_START( spelunk2_rom )
	ROM_REGION( 0x24000 )	/* main CPU */
	ROM_LOAD( "sp2-a.4e",	0x0000, 0x4000, 0x96c04bbb )
	ROM_LOAD( "sp2-a.4d",	0x4000, 0x4000, 0xcb38c2ff )
	/* banked switched ROMs */
	ROM_LOAD( "sp2-r.7d",	0x10000, 0x8000, 0x558837ea )
	ROM_LOAD( "sp2-r.7c",	0x18000, 0x8000, 0x4b380162 )
	ROM_LOAD( "sp2-r.7b",	0x20000, 0x4000, 0x7709a1fe )

	ROM_REGION( 0x10000 )	/* sound CPU */
	ROM_LOAD( "sp2-a.3d",	0x8000, 0x04000, 0x839ec7e2 ) /* adpcm data */
	ROM_LOAD( "sp2-a.3f",	0xc000, 0x04000, 0xad3ce898 ) /* 6803 code */

	ROM_REGION_DISPOSE( 0x18000 ) /* tiles */
	ROM_LOAD( "sp2-r.1b",	0x00000, 0x8000, 0x3a0c4d47 )
	ROM_LOAD( "sp2-r.1d",	0x08000, 0x8000, 0xc19fa4c9 )
	ROM_LOAD( "sp2-r.3b",	0x10000, 0x8000, 0x366604af )

	ROM_REGION_DISPOSE( 0x10000 ) /* characters */
	ROM_LOAD( "sp2-r.4m",	0x00000, 0x2000, 0x5591f22c )
	ROM_LOAD( "sp2-r.4p",	0x02000, 0x2000, 0x32967081 )
	ROM_LOAD( "sp2-r.4l",	0x04000, 0x2000, 0xb12f939a )

	ROM_REGION_DISPOSE( 0x30000 ) /* sprites */
	ROM_LOAD( "sp2-b.3n",	0x00000, 0x4000, 0xf59e8b76 ) // plane0
	ROM_LOAD( "sp2-b.4e",	0x04000, 0x4000, 0x780a463b )

	ROM_LOAD( "sp2-b.4c",	0x10000, 0x4000, 0x1caf7013 ) // plane1
	ROM_LOAD( "sp2-b.4f",	0x14000, 0x4000, 0xe4a1166f )

	ROM_LOAD( "sp2-b.4k",	0x20000, 0x4000, 0x6cb67a17 ) // plane2
	ROM_LOAD( "sp2-b.4n",	0x24000, 0x4000, 0xfa65bac9 )

	ROM_REGION( 3*512 ) /* color proms */
	/* sprites */
	ROM_LOAD( "sp2-b.1m",	0*256,	256, 0x906104c7 )//red
	ROM_LOAD( "sp2-b.1n",	2*256,	256, 0x5a564c06 )//green
	ROM_LOAD( "sp2-b.1l",	4*256,	256, 0x8f4a2e3c )//blue
	/* tile colors */
	/* red - missing? */
	ROM_LOAD( "sp2-r.2k",	3*256,	256, 0x1cf5987e )//green
	ROM_LOAD( "sp2-r.2j",	5*256,	256, 0x1acbe2a5 )//blue

	ROM_REGION( 32 ) /* sprite height table */
	ROM_LOAD( "sp2-b.5p",	0, 32,	0xcd126f6a )

/*
	unknown:
	ROM_LOAD( "sp2-r.8j",	0, 512, 0x875cc442 )
	ROM_LOAD( "sp2-r.1k",	0,	512, 0 )
*/
ROM_END


static int spelunk2_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0xE066],"\x99\x11\x59",3) == 0 &&
			memcmp(&RAM[0xE0C7],"\x49\x4e\x3f",3) == 0 )
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xE066],100);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void spelunk2_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xE066],100);
		osd_fclose(f);
	}
}


struct GameDriver spelunk2_driver =
{
	__FILE__,
	0,
	"spelunk2",
	"Spelunker II",
	"1986",
	"Irem (licensed from Broderbund)",
	"Phil Stroffolino\nAaron Giles (sound)",
	GAME_WRONG_COLORS,
	&spelunk2_machine_driver,
	0,

	spelunk2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(MEM_COLOR), 0, 0,
	ORIENTATION_DEFAULT,

	spelunk2_hiload, spelunk2_hisave
};
