/***************************************************************************

Atari Sprint2 Driver

Memory Map:
        0000-03FF       WRAM
        0400-07FF       R: RAM and DISPLAY, W: DISPLAY
        0800-0BFF       R: SWITCH
        0C00-0FFF       R: SYNC
        0C00-0C0F       W: ATTRACT
        0C10-0C1F       W: SKID1
        0C20-0C2F       W: SKID2
        0C30-0C3F       W: LAMP1
        0C40-0C4F       W: LAMP2
        0C60-0C6F       W: SPARE
        0C80-0CFF       W: TIMER RESET (Watchdog)
        0D00-0D7F       W: COLLISION RESET 1
        0D80-0DFF       W: COLLISION RESET 2
        0E00-0E7F       W: STEERING RESET 1
        0E80-0EFF       W: STEERING RESET 2
        0F00-0F7F       W: NOISE RESET
        1000-13FF       R: COLLISION1
        1400-17FF       R: COLLISION2
        2000-23FF       PROM1 (Playfield ROM1)
        2400-27FF       PROM2 (Playfield ROM1)
        2800-2BFF       PROM3 (Playfield ROM2)
        2C00-2FFF       PROM4 (Playfield ROM2)
        3000-33FF       PROM5 (Program ROM1)
        3400-37FF       PROM6 (Program ROM1)
        3800-3BFF       PROM7 (Program ROM2)
        3C00-3FFF       PROM8 (Program ROM2)
       (FC00-FFFF)      PROM8 (Program ROM2) - only needed for the 6502 vectors

If you have any questions about how this driver works, don't hesitate to
ask.  - Mike Balfour (mab22@po.cwru.edu)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* vidhrdw/sprint2.c */
extern void sprint1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void sprint2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern int sprint2_vh_start(void);
extern void sprint2_vh_stop(void);
extern unsigned char *sprint2_vert_car_ram;
extern unsigned char *sprint2_horiz_ram;

/* machine/sprint2.c */
int sprint1_read_ports(int offset);
int sprint2_read_ports(int offset);
int sprint2_read_sync(int offset);
int sprint2_coins(int offset);
int sprint2_steering1(int offset);
int sprint2_steering2(int offset);
int sprint2_collision1(int offset);
int sprint2_collision2(int offset);
void sprint2_collision_reset1(int offset, int value);
void sprint2_collision_reset2(int offset, int value);
void sprint2_steering_reset1(int offset, int value);
void sprint2_steering_reset2(int offset, int value);
void sprint2_lamp1(int offset, int value);
void sprint2_lamp2(int offset, int value);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0010, 0x0013, MRA_RAM, &sprint2_horiz_ram }, /* WRAM */
	{ 0x0018, 0x001f, MRA_RAM, &sprint2_vert_car_ram }, /* WRAM */
	{ 0x0000, 0x03ff, MRA_RAM }, /* WRAM */
	{ 0x0400, 0x07ff, MRA_RAM }, /* DISPLAY RAM */
	{ 0x0800, 0x083f, sprint2_read_ports }, /* SWITCH */
	{ 0x0840, 0x087f, sprint2_coins },
	{ 0x0880, 0x08bf, sprint2_steering1 },
	{ 0x08c0, 0x08ff, sprint2_steering2 },
	{ 0x0900, 0x093f, sprint2_read_ports }, /* SWITCH */
	{ 0x0940, 0x097f, sprint2_coins },
	{ 0x0980, 0x09bf, sprint2_steering1 },
	{ 0x09c0, 0x09ff, sprint2_steering2 },
	{ 0x0a00, 0x0a3f, sprint2_read_ports }, /* SWITCH */
	{ 0x0a40, 0x0a7f, sprint2_coins },
	{ 0x0a80, 0x0abf, sprint2_steering1 },
	{ 0x0ac0, 0x0aff, sprint2_steering2 },
	{ 0x0b00, 0x0b3f, sprint2_read_ports }, /* SWITCH */
	{ 0x0b40, 0x0b7f, sprint2_coins },
	{ 0x0b80, 0x0bbf, sprint2_steering1 },
	{ 0x0bc0, 0x0bff, sprint2_steering2 },
	{ 0x0c00, 0x0fff, sprint2_read_sync }, /* SYNC */
	{ 0x1000, 0x13ff, sprint2_collision1 }, /* COLLISION 1 */
	{ 0x1400, 0x17ff, sprint2_collision2 }, /* COLLISION 2 */
	{ 0x2000, 0x3fff, MRA_ROM }, /* PROM1-PROM8 */
	{ 0xfff0, 0xffff, MRA_ROM }, /* PROM8 for 6502 vectors */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM }, /* WRAM */
	{ 0x0400, 0x07ff, videoram_w, &videoram, &videoram_size }, /* DISPLAY */
	{ 0x0c00, 0x0c0f, MWA_RAM }, /* ATTRACT */
	{ 0x0c10, 0x0c1f, MWA_RAM }, /* SKID1 */
	{ 0x0c20, 0x0c2f, MWA_RAM }, /* SKID2 */
	{ 0x0c30, 0x0c3f, sprint2_lamp1 }, /* LAMP1 */
	{ 0x0c40, 0x0c4f, sprint2_lamp2 }, /* LAMP2 */
	{ 0x0c60, 0x0c6f, MWA_RAM }, /* SPARE */
	{ 0x0c80, 0x0cff, MWA_NOP }, /* TIMER RESET (watchdog) */
	{ 0x0d00, 0x0d7f, sprint2_collision_reset1 }, /* COLLISION RESET 1 */
	{ 0x0d80, 0x0dff, sprint2_collision_reset2 }, /* COLLISION RESET 2 */
	{ 0x0e00, 0x0e7f, sprint2_steering_reset1 }, /* STEERING RESET 1 */
	{ 0x0e80, 0x0eff, sprint2_steering_reset2 }, /* STEERING RESET 2 */
	{ 0x0f00, 0x0f7f, MWA_RAM }, /* NOISE RESET */
	{ 0x2000, 0x3fff, MWA_ROM }, /* PROM1-PROM8 */
	{ -1 }	/* end of table */
};

/* The only difference is that we use "sprint1_read_ports" */
static struct MemoryReadAddress sprint1_readmem[] =
{
	{ 0x0010, 0x0013, MRA_RAM, &sprint2_horiz_ram }, /* WRAM */
	{ 0x0018, 0x001f, MRA_RAM, &sprint2_vert_car_ram }, /* WRAM */
	{ 0x0000, 0x03ff, MRA_RAM }, /* WRAM */
	{ 0x0400, 0x07ff, MRA_RAM }, /* DISPLAY RAM */
	{ 0x0800, 0x083f, sprint1_read_ports }, /* SWITCH */
	{ 0x0840, 0x087f, sprint2_coins },
	{ 0x0880, 0x08bf, sprint2_steering1 },
	{ 0x08c0, 0x08ff, sprint2_steering2 },
	{ 0x0900, 0x093f, sprint1_read_ports }, /* SWITCH */
	{ 0x0940, 0x097f, sprint2_coins },
	{ 0x0980, 0x09bf, sprint2_steering1 },
	{ 0x09c0, 0x09ff, sprint2_steering2 },
	{ 0x0a00, 0x0a3f, sprint1_read_ports }, /* SWITCH */
	{ 0x0a40, 0x0a7f, sprint2_coins },
	{ 0x0a80, 0x0abf, sprint2_steering1 },
	{ 0x0ac0, 0x0aff, sprint2_steering2 },
	{ 0x0b00, 0x0b3f, sprint1_read_ports }, /* SWITCH */
	{ 0x0b40, 0x0b7f, sprint2_coins },
	{ 0x0b80, 0x0bbf, sprint2_steering1 },
	{ 0x0bc0, 0x0bff, sprint2_steering2 },
	{ 0x0c00, 0x0fff, sprint2_read_sync }, /* SYNC */
	{ 0x1000, 0x13ff, sprint2_collision1 }, /* COLLISION 1 */
	{ 0x1400, 0x17ff, sprint2_collision2 }, /* COLLISION 2 */
	{ 0x2000, 0x3fff, MRA_ROM }, /* PROM1-PROM8 */
	{ 0xfff0, 0xffff, MRA_ROM }, /* PROM8 for 6502 vectors */
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( sprint2_input_ports )
	PORT_START      /* DSW - fake port, gets mapped to Sprint2 ports */
	PORT_DIPNAME( 0x01, 0x01, "Misc.", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Cycle 12 Tracks" )
	PORT_DIPSETTING(    0x00, "Show Easy Track" )
	PORT_DIPNAME( 0x02, 0x00, "Misc.", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "No Oil Slicks" )
	PORT_DIPSETTING(    0x00, "Oil Slicks" )
	PORT_DIPNAME( 0x0C, 0x00, "Cost", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0C, "Free" )
	PORT_DIPSETTING(    0x08, "2 coins/player" )
	PORT_DIPSETTING(    0x04, "1 coin/2 players" )
	PORT_DIPSETTING(    0x00, "1 coin/player" )
	PORT_DIPNAME( 0x10, 0x10, "Ext. Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "None" )
	PORT_DIPSETTING(    0x00, "30% of game length" )
	PORT_DIPNAME( 0xC0, 0xC0, "Game Length", IP_KEY_NONE )
	PORT_DIPSETTING(    0xC0, "60 seconds" )
	PORT_DIPSETTING(    0x80, "90 seconds" )
	PORT_DIPSETTING(    0x40, "120 seconds" )
	PORT_DIPSETTING(    0x00, "150 seconds" )

	PORT_START      /* IN0 - fake port, gets mapped to Sprint2 ports */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN, "1st gear (P1)", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT, "2nd gear (P1)", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT, "3rd gear (P1)", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP, "4th gear (P1)", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2, "1st gear (P2)", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2, "2nd gear (P2)", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2, "3rd gear (P2)", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2, "4th gear (P2)", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )

	PORT_START      /* IN1 - fake port, gets mapped to Sprint2 ports */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON1, "Gas (P1)", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "Gas (P2)", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON2, "Track Select", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )

	PORT_START      /* IN2 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN4 */
	PORT_ANALOG ( 0xff, 0x00, IPT_DIAL, 100, 0, 0, 0 )


	PORT_START      /* IN5 */
	PORT_ANALOG ( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 100, 0, 0, 0 )
INPUT_PORTS_END


INPUT_PORTS_START( sprint1_input_ports )
	PORT_START      /* DSW - fake port, gets mapped to Sprint2 ports */
	PORT_DIPNAME( 0x01, 0x01, "Misc.", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Cycle Tracks Every Lap" )
	PORT_DIPSETTING(    0x00, "Cycle Tracks Every 2 Laps" )
	PORT_DIPNAME( 0x02, 0x00, "Misc.", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "No Oil Slicks" )
	PORT_DIPSETTING(    0x00, "Oil Slicks" )
	PORT_DIPNAME( 0x0C, 0x00, "Cost", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0C, "Free" )
	PORT_DIPSETTING(    0x08, "2 coins/game" )
	PORT_DIPSETTING(    0x04, "2 games/coin" )
	PORT_DIPSETTING(    0x00, "1 coin/game" )
	PORT_DIPNAME( 0x10, 0x10, "Ext. Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "None" )
	PORT_DIPSETTING(    0x00, "30% of game length" )
	PORT_DIPNAME( 0xC0, 0xC0, "Game Length", IP_KEY_NONE )
	PORT_DIPSETTING(    0xC0, "60 seconds" )
	PORT_DIPSETTING(    0x80, "90 seconds" )
	PORT_DIPSETTING(    0x40, "120 seconds" )
	PORT_DIPSETTING(    0x00, "150 seconds" )

	PORT_START      /* IN0 - fake port, gets mapped to Sprint2 ports */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN, "1st gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT, "2nd gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT, "3rd gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP, "4th gear", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT (0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 - fake port, gets mapped to Sprint2 ports */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON1, "Gas", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN2 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN4 */
	PORT_ANALOG ( 0xff, 0x00, IPT_DIAL, 100, 0, 0, 0 )


	PORT_START      /* IN5 */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
        64,     /* 64 characters */
        1,      /* 1 bit per pixel */
        { 0 },        /* no separation in 1 bpp */
        { 4, 5, 6, 7, 0x200*8 + 4, 0x200*8 + 5, 0x200*8 + 6, 0x200*8 + 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout carlayout =
{
        16,8,    /* 16*8 characters */
        32,      /* 32 characters */
        1,       /* 1 bit per pixel */
        { 0 },        /* no separation in 1 bpp */
        { 7, 6, 5, 4, 0x200*8 + 7, 0x200*8 + 6, 0x200*8 + 5, 0x200*8 + 4,
          15, 14, 13, 12, 0x200*8 + 15, 0x200*8 + 14, 0x200*8 + 13, 0x200*8 + 12  },
        { 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
        16*8     /* every char takes 16 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        { 1, 0x0000, &charlayout, 0x00, 0x02 }, /* offset into colors, # of colors */
        { 1, 0x0400, &carlayout, 0x04, 0x04 }, /* offset into colors, # of colors */
	{ -1 } /* end of array */
};

static unsigned char palette[] =
{
        0x00,0x00,0x00, /* BLACK - oil slicks, text, black car */
        0x55,0x55,0x55, /* DK GREY - default background */
        0x80,0x80,0x80, /* LT GREY - grey cars */
        0xff,0xff,0xff, /* WHITE - track, text, white car */
};

static unsigned short colortable[] =
{
        0x01, 0x00, /* Black playfield */
        0x01, 0x03, /* White playfield */
        0x01, 0x03, /* White car */
        0x01, 0x00, /* Black car */
        0x01, 0x02, /* Grey car 1 */
        0x01, 0x02  /* Grey car 2 */
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			333333,        /* 0.3 Mhz ???? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 }, /* it's actually 32x28, but we'll leave room for our gear indicator */
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER,
	0,
	sprint2_vh_start,
	sprint2_vh_stop,
	sprint2_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0
};

static struct MachineDriver sprint1_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			333333,        /* 0.3 Mhz ???? */
			0,
			sprint1_readmem,writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 }, /* it's actually 32x28, but we'll leave room for our gear indicator */
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER,
	0,
	sprint2_vh_start,
	sprint2_vh_stop,
	sprint1_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0

};





/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( sprint1_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "6290-01.b1",   0x2000, 0x0800, 0x41fc985e )
	ROM_LOAD( "6291-01.c1",   0x2800, 0x0800, 0x07f7a920 )
	ROM_LOAD( "6442-01.d1",   0x3000, 0x0800, 0xe9ff0124 )
	ROM_RELOAD(             0xF000, 0x0800 )
	ROM_LOAD( "6443-01.e1",   0x3800, 0x0800, 0xd6bb00d0 )
	ROM_RELOAD(             0xF800, 0x0800 )

	ROM_REGION_DISPOSE(0x800)     /* 2k for graphics */
	ROM_LOAD( "6396-01.p4",   0x0000, 0x0200, 0x801b42dd )
	ROM_LOAD( "6397-01.r4",   0x0200, 0x0200, 0x135ba1aa )
	ROM_LOAD( "6398-01.k6",   0x0400, 0x0200, 0xc9e1017e )
	ROM_LOAD( "6399-01.j6",   0x0600, 0x0200, 0x63d685b2 )
ROM_END

ROM_START( sprint2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "6290-01.b1",   0x2000, 0x0800, 0x41fc985e )
	ROM_LOAD( "6291-01.c1",   0x2800, 0x0800, 0x07f7a920 )
	ROM_LOAD( "6404sp2.d1",   0x3000, 0x0800, 0xd2878ff6 )
	ROM_RELOAD(             0xF000, 0x0800 )
	ROM_LOAD( "6405sp2.e1",   0x3800, 0x0800, 0x6c991c80 )
	ROM_RELOAD(             0xF800, 0x0800 )

	ROM_REGION_DISPOSE(0x800)     /* 2k for graphics */
	ROM_LOAD( "6396-01.p4",   0x0000, 0x0200, 0x801b42dd )
	ROM_LOAD( "6397-01.r4",   0x0200, 0x0200, 0x135ba1aa )
	ROM_LOAD( "6398-01.k6",   0x0400, 0x0200, 0xc9e1017e )
	ROM_LOAD( "6399-01.j6",   0x0600, 0x0200, 0x63d685b2 )
ROM_END

/***************************************************************************

  Hi Score Routines

***************************************************************************/

static int sprint1_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0x0057],"000",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0x0057],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void sprint1_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0x0057],3);
		osd_fclose(f);
	}

}


/***************************************************************************

  Game driver(s)

***************************************************************************/

struct GameDriver sprint1_driver =
{
	__FILE__,
	0,
	"sprint1",
	"Sprint 1",
	"1978",
	"Atari",
	"Mike Balfour",
	0,
	&sprint1_machine_driver,
	0,

	sprint1_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	sprint1_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,
	sprint1_hiload,sprint1_hisave
};

struct GameDriver sprint2_driver =
{
	__FILE__,
	&sprint1_driver,
	"sprint2",
	"Sprint 2",
	"1976",
	"Atari",
	"Mike Balfour",
	0,
	&machine_driver,
	0,

	sprint2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	sprint2_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,
	0,0
};
