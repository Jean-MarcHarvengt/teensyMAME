#include "driver.h"
#include "vidhrdw/generic.h"

static int dataoffset=0;

extern unsigned char *goldstar_video1, *goldstar_video2, *goldstar_video3;
extern int goldstar_video_size;
extern unsigned char *goldstar_scroll1, *goldstar_scroll2, *goldstar_scroll3;

void goldstar_video1_w(int offset, int data);
void goldstar_video2_w(int offset, int data);
void goldstar_video3_w(int offset, int data);
void goldstar_fa00_w(int offset, int data);
int  goldstar_vh_start(void);
void goldstar_vh_stop(void);
void goldstar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


void protection_w(int address, int data)
{
	if (data == 0x2a)
		dataoffset = 0;
}

int protection_r(int address)
{
	static int data[4] = { 0x47, 0x4f, 0x4c, 0x44 };

	dataoffset %= 4;
	return data[dataoffset++];
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xb7ff, MRA_ROM },
	{ 0xb800, 0xbfff, MRA_RAM },
	{ 0xc000, 0xc7ff, MRA_ROM },
	{ 0xc800, 0xd9ff, MRA_RAM },
	{ 0xe000, 0xe1ff, MRA_RAM },
	{ 0xd800, 0xd9ff, MRA_RAM },
	{ 0xf800, 0xf800, input_port_0_r },	/* PLAYER */
	{ 0xf801, 0xf801, input_port_1_r },	/* Test Mode */
	{ 0xf802, 0xf802, input_port_2_r },	/* DSW 1 */
//	{ 0xf803, 0xf803, },
//	{ 0xf804, 0xf804, },
	{ 0xf805, 0xf805, input_port_7_r },	/* DSW 4 (also appears in 8910 port) */
	{ 0xf806, 0xf806, input_port_9_r },	/* (don't know to which one of the */
										/* service mode dip switches it should map) */
	{ 0xf810, 0xf810, input_port_3_r },	/* COIN */
	{ 0xf811, 0xf811, input_port_4_r },	/* TEST */
	{ 0xf820, 0xf820, input_port_5_r },	/* DSW 2 */
	{ 0xf830, 0xf830, AY8910_read_port_0_r },
	{ 0xfb00, 0xfb00, OKIM6295_status_r },
	{ 0xfd00, 0xfdff, MRA_RAM },
	{ 0xfe00, 0xfe00, protection_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xb7ff, MWA_ROM },
	{ 0xb800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc7ff, MWA_ROM },
	{ 0xc800, 0xcfff, videoram_w, &videoram, &videoram_size },
	{ 0xd000, 0xd7ff, colorram_w, &colorram },
	{ 0xd800, 0xd9ff, goldstar_video1_w, &goldstar_video1, &goldstar_video_size },
	{ 0xe000, 0xe1ff, goldstar_video2_w, &goldstar_video2 },
	{ 0xe800, 0xe9ff, goldstar_video3_w, &goldstar_video3 },
	{ 0xf040, 0xf07f, MWA_RAM, &goldstar_scroll1 },
	{ 0xf080, 0xf0bf, MWA_RAM, &goldstar_scroll2 },
	{ 0xf0c0, 0xf0ff, MWA_RAM, &goldstar_scroll3 },
	{ 0xf830, 0xf830, AY8910_write_port_0_w },
	{ 0xf840, 0xf840, AY8910_control_port_0_w },
	{ 0xfa00, 0xfa00, goldstar_fa00_w },
	{ 0xfb00, 0xfb00, OKIM6295_data_w },
	{ 0xfd00, 0xfdff, paletteram_BBGGGRRR_w, &paletteram },
	{ 0xfe00, 0xfe00, protection_w },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x10, 0x10, input_port_8_r },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* PLAYER */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON4, "Bet Red / 2", OSD_KEY_V, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON3, "Stop 3 / Small / 1 / Info", OSD_KEY_C, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON5, "Bet Blue / Double / 3", OSD_KEY_B, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1, "Stop 1 / Take", OSD_KEY_Z, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON2, "Stop 2 / Big / Ticket", OSD_KEY_X, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON6, "Start / Stop All / 4", OSD_KEY_N, IP_JOY_DEFAULT, 0)

	PORT_START	/* */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )	/* this is not a coin, not sure what it is */
												/* maybe it's used to buy tickets. */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_SERVICE, "Statistics", OSD_KEY_F1, IP_JOY_NONE, 0)

	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x01, 0x00, "Game Style", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Gettoni" )
	PORT_DIPSETTING(    0x00, "Ticket" )
	PORT_DIPNAME( 0x02, 0x02, "Hopper Out", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Active Low" )
	PORT_DIPSETTING(    0x00, "Active High" )
	PORT_DIPNAME( 0x04, 0x04, "Payout Automatic?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "W-Up '7'", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Loss" )
	PORT_DIPSETTING(    0x00, "Even" )
	PORT_DIPNAME( 0x10, 0x10, "W-Up Pay Rate", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "60%" )
	PORT_DIPSETTING(    0x00, "70%" )
	PORT_DIPNAME( 0x20, 0x20, "W-Up Game", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPNAME( 0xc0, 0x00, "Bet Max", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "8 Bet" )
	PORT_DIPSETTING(    0x80, "16 Bet" )
	PORT_DIPSETTING(    0x40, "32 Bet" )
	PORT_DIPSETTING(    0x00, "50 Bet" )

	PORT_START	/* COIN */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* TEST */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW 2 */
	PORT_DIPNAME( 0x07, 0x00, "Main Game Pay Rate", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "75 %" )
	PORT_DIPSETTING(    0x01, "70 %" )
	PORT_DIPSETTING(    0x02, "65 %" )
	PORT_DIPSETTING(    0x03, "60 %" )
	PORT_DIPSETTING(    0x04, "55 %" )
	PORT_DIPSETTING(    0x05, "50 %" )
	PORT_DIPSETTING(    0x06, "45 %" )
	PORT_DIPSETTING(    0x07, "40 %" )
	PORT_DIPNAME( 0x18, 0x00, "Hopper Limit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "300" )
	PORT_DIPSETTING(    0x10, "500" )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_DIPSETTING(    0x00, "Unlimited" )
	PORT_DIPNAME( 0x20, 0x00, "100 Odds Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPNAME( 0x40, 0x00, "Key-In Type", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "B-Type" )
	PORT_DIPSETTING(    0x00, "A-Type" )
	PORT_DIPNAME( 0x80, 0x00, "Center Super 7 Bet Limit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Unlimited" )
	PORT_DIPSETTING(    0x00, "Limited" )

	PORT_START	/* DSW 3 */
	PORT_DIPNAME( 0x0c, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "1 Coin/100 Credits" )
	PORT_DIPSETTING(    0x08, "1 Coin/50 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/20 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/10 Credits" )
	PORT_DIPNAME( 0xc0, 0x40, "Coin C", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "1 Coin/10 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credits" )

	PORT_START	/* DSW 4 */
	PORT_DIPNAME( 0x07, 0x06, "Credit Limited", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "5000" )
	PORT_DIPSETTING(    0x06, "10000" )
	PORT_DIPSETTING(    0x05, "20000" )
	PORT_DIPSETTING(    0x04, "30000" )
	PORT_DIPSETTING(    0x03, "40000" )
	PORT_DIPSETTING(    0x02, "50000" )
	PORT_DIPSETTING(    0x01, "100000" )
	PORT_DIPSETTING(    0x00, "Unlimited" )
	PORT_DIPNAME( 0x08, 0x00, "Display Credit Limit", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Type of Coin D", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Play Min Bet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "16 Bet" )
	PORT_DIPSETTING(    0x00, "8 Bet" )
	PORT_DIPNAME( 0x40, 0x00, "Reel Speed", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Low" )
	PORT_DIPSETTING(    0x00, "High" )
	PORT_DIPNAME( 0x80, 0x00, "Ticket Payment", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "1 Ticket/100" )
	PORT_DIPSETTING(    0x00, "Pay All" )

	PORT_START	/* DSW 5 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* ??? */
	PORT_BIT( 0xdf, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x20, 0x00, "Show Woman", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	4096,    /* 4096 characters */
	3,      /* 3 bits per pixel */
	{ 2, 4, 6 }, /* the bitplanes are packed in one byte */
	{ 0*8+0, 0*8+1, 1*8+0, 1*8+1, 2*8+0, 2*8+1, 3*8+0, 3*8+1 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8   /* every char takes 32 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	8,32,    /* 8*32 characters */
	256,    /* 256 tiles */
	4,      /* 4 bits per pixel */
	{ 0, 2, 4, 6 },
	{ 0, 1, 1*8+0, 1*8+1, 2*8+0, 2*8+1, 3*8+0, 3*8+1 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8,
			32*8, 36*8, 40*8, 44*8, 48*8, 52*8, 56*8, 60*8,
			64*8, 68*8, 72*8, 76*8, 80*8, 84*8, 88*8, 92*8,
			96*8, 100*8, 104*8, 108*8, 112*8, 116*8, 120*8, 124*8 },
	128*8   /* every char takes 128 consecutive bytes */
};


static struct GfxLayout tilelayoutbl =
{
	8,32,    /* 8*32 characters */
	256,    /* 256 tiles */
	4,      /* 4 bits per pixel */
	{ 0, 2, 4, 6 },
	{ 0, 1, 2*8+0, 2*8+1, 1*8+0, 1*8+1, 3*8+0, 3*8+1 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8,
			32*8, 36*8, 40*8, 44*8, 48*8, 52*8, 56*8, 60*8,
			64*8, 68*8, 72*8, 76*8, 80*8, 84*8, 88*8, 92*8,
			96*8, 100*8, 104*8, 108*8, 112*8, 116*8, 120*8, 124*8 },
	128*8   /* every char takes 128 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,   0, 16 },
	{ 1, 0x20000, &tilelayout, 128,  8 },
	{ -1 } /* end of array */
};
static struct GfxDecodeInfo gfxdecodeinfobl[] =
{
	{ 1, 0x00000, &charlayout,   0, 16 },
	{ 1, 0x20000, &tilelayoutbl, 128,  8 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1500000,	/* ????? */
	{ 255 },
	{ input_port_7_r },	/* DSW 4 */
	{ input_port_6_r },	/* DSW 3 */
	{ 0 },
	{ 0 }
};

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	8000,           /* 8000Hz frequency */
	2,              /* memory region 2 */
	{ 255 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz ? */
			0,
			readmem,writemem,readport,0,
			interrupt,1
		},
	},
	60, 0,
	1,
	0,

	/* video hardware */
	64*8, 32*8, { 0*8, 64*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_PIXEL_ASPECT_RATIO_1_2,
	0,
	goldstar_vh_start,
	goldstar_vh_stop,
	goldstar_vh_screenrefresh,

	/* sound hardware */
	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};


static struct MachineDriver goldstbl_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz ? */
			0,
			readmem,writemem,readport,0,
			interrupt,1
		},
	},
	60, 0,
	1,
	0,

	/* video hardware */
	64*8, 32*8, { 0*8, 64*8-1, 2*8, 30*8-1 },
	gfxdecodeinfobl,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_PIXEL_ASPECT_RATIO_1_2,
	0,
	goldstar_vh_start,
	goldstar_vh_stop,
	goldstar_vh_screenrefresh,

	/* sound hardware */
	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
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

ROM_START( goldstar_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "gs4-cpu.bin",  0x0000, 0x10000, 0x73e47d4d )

	ROM_REGION_DISPOSE(0x28000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "gs2.bin",      0x00000, 0x20000, 0xa2d5b898 )
	ROM_LOAD( "gs3.bin",      0x20000, 0x08000, 0x8454ce3c )

	ROM_REGION(0x20000) /* Audio ADPCM */
	ROM_LOAD( "gs1-snd.bin",  0x0000, 0x20000, 0x9d58960f )
ROM_END


ROM_START( goldstbl_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "gsb-cpu.bin",  0x0000, 0x10000, 0x82b238c3 )

	ROM_REGION_DISPOSE(0x28000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "gs2.bin",      0x00000, 0x20000, 0xa2d5b898 )
	ROM_LOAD( "gsb-spr.bin",  0x20000, 0x08000, 0x52ecd4c7 )

	ROM_REGION(0x20000) /* Audio ADPCM */
	ROM_LOAD( "gs1-snd.bin",  0x0000, 0x20000, 0x9d58960f )
ROM_END


static void goldstar_decode(void)
{
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	for (A = 0;A < 0x10000;A++)
	{
		if ((A & 0x30) == 0)
			RAM[A] ^= 0x82;
		else
			RAM[A] ^= 0xcc;
	}
}



int nvram_load(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* Try loading static RAM */
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		/* just load in everything, the game will overwrite what it doesn't want */
		osd_fread(f,&RAM[0xb800],0x0800);
		osd_fclose(f);
	}
	/* Invalidate the static RAM to force reset to factory settings */
	else memset(&RAM[0xb800],0xff,0x0800);

	return 1;
}

void nvram_save(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xb800],0x0800);
		osd_fclose(f);
	}
}



struct GameDriver goldstar_driver =
{
	__FILE__,
	0,
	"goldstar",
	"Golden Star",
	"????",
	"????",
	"Mirko Buffoni\nNicola Salmoria",
	0,
	&machine_driver,
	0,

	goldstar_rom,
	goldstar_decode, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	nvram_load, nvram_save
};


struct GameDriver goldstbl_driver =
{
	__FILE__,
	&goldstar_driver,
	"goldstbl",
	"Golden Star (Blue version)",
	"????",
	"????",
	"Mirko Buffoni\nNicola Salmoria",
	0,
	&goldstbl_machine_driver,
	0,

	goldstbl_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	nvram_load, nvram_save
};
