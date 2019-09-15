/*************************************************************************
     Turbo - Sega - 1981

     Memory Map:  ( * not complete * )

     Address Range:     R/W:     Function:
     --------------------------------------------------------------------------
     0000 - 5fff        R        Program ROM
	 a000 - a0ff        W        Sprite RAM
     a800 - a803        W        Lamps / Coin Meters
	 b000 - b1ff        R/W      Collision RAM
     e000 - e7ff        R/W      character RAM
     f000 - f7ff        R/W      RAM
     f202                        coinage 2
     f205                        coinage 1
     f800 - f803        R/W      road drawing
     f900 - f903        R/W      road drawing
     fa00 - fa03        R/W      sound
     fb00 - fb03        R/W      x,DS2,x,x
     fc00 - fc01        R        DS1,x
     fc00 - fc01        W        score
     fd00               R        Coin Inputs, etc.
     fe00               R        DS3,x

 Switch settings:
 Notes:
        1) Facing the CPU board, with the two large IDC connectors at
           the top of the board, and the large and small IDC
           connectors at the bottom, DIP switch #1 is upper right DIP
           switch, DIP switch #2 is the DIP switch to the right of it.

        2) Facing the Sound board, with the IDC connector at the
           bottom of the board, DIP switch #3 (4 bank) can be seen.
 ----------------------------------------------------------------------------

 Option    (DIP Swtich #1) | SW1 | SW2 | SW3 | SW4 | SW5 | SW6 | SW7 | SW8 |
 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
 1 Car On Extended Play    | ON  | ON  |     |     |     |     |     |     |
 2 Car On Extended Play    | OFF | ON  |     |     |     |     |     |     |
 3 Car On Extended Play    | ON  | OFF |     |     |     |     |     |     |
 4 Car On Extended Play    | OFF | OFF |     |     |     |     |     |     |
 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
 Game Time Adjustable      |     |     | ON  |     |     |     |     |     |
 Game Time Fixed (55 Sec.) |     |     | OFF |     |     |     |     |     |
 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
 Hard Game Difficulty      |     |     |     | ON  |     |     |     |     |
 Easy Game Difficulty      |     |     |     | OFF |     |     |     |     |
 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
 Normal Game Mode          |     |     |     |     | ON  |     |     |     |
 No Collisions (cheat)     |     |     |     |     | OFF |     |     |     |
 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
 Initial Entry Off (?)     |     |     |     |     |     | ON  |     |     |
 Initial Entry On  (?)     |     |     |     |     |     | OFF |     |     |
 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
 Not Used                  |     |     |     |     |     |     |  X  |  X  |
 ---------------------------------------------------------------------------

 Option    (DIP Swtich #2) | SW1 | SW2 | SW3 | SW4 | SW5 | SW6 | SW7 | SW8 |
 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
 60 Seconds Game Time      | ON  | ON  |     |     |     |     |     |     |
 70 Seconds Game Time      | OFF | ON  |     |     |     |     |     |     |
 80 Seconds Game Time      | ON  | OFF |     |     |     |     |     |     |
 90 Seconds Game Time      | OFF | OFF |     |     |     |     |     |     |
 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
 Slot 1  1 Coin  1 Credit  |     |     | ON  | ON  | ON  |     |     |     |
 Slot 1  1 Coin  2 Credits |     |     | OFF | ON  | ON  |     |     |     |
 Slot 1  1 Coin  3 Credits |     |     | ON  | OFF | ON  |     |     |     |
 Slot 1  1 Coin  6 Credits |     |     | OFF | OFF | ON  |     |     |     |
 Slot 1  2 Coins 1 Credit  |     |     | ON  | ON  | OFF |     |     |     |
 Slot 1  3 Coins 1 Credit  |     |     | OFF | ON  | OFF |     |     |     |
 Slot 1  4 Coins 1 Credit  |     |     | ON  | OFF | OFF |     |     |     |
 Slot 1  1 Coin  1 Credit  |     |     | OFF | OFF | OFF |     |     |     |
 --------------------------|-----|-----|-----|-----|-----|-----|-----|-----|
 Slot 2  1 Coin  1 Credit  |     |     |     |     |     | ON  | ON  | ON  |
 Slot 2  1 Coin  2 Credits |     |     |     |     |     | OFF | ON  | ON  |
 Slot 2  1 Coin  3 Credits |     |     |     |     |     | ON  | OFF | ON  |
 Slot 2  1 Coin  6 Credits |     |     |     |     |     | OFF | OFF | ON  |
 Slot 2  2 Coins 1 Credit  |     |     |     |     |     | ON  | ON  | OFF |
 Slot 2  3 Coins 1 Credit  |     |     |     |     |     | OFF | ON  | OFF |
 Slot 2  4 Coins 1 Credit  |     |     |     |     |     | ON  | OFF | OFF |
 Slot 2  1 Coins 1 Credit  |     |     |     |     |     | OFF | OFF | OFF |
 ---------------------------------------------------------------------------

 Option    (DIP Swtich #3) | SW1 | SW2 | SW3 | SW4 |
 --------------------------|-----|-----|-----|-----|
 Not Used                  |  X  |  X  |     |     |
 --------------------------|-----|-----|-----|-----|
 Digital (LED) Tachometer  |     |     | ON  |     |
 Analog (Meter) Tachometer |     |     | OFF |     |
 --------------------------|-----|-----|-----|-----|
 Cockpit Sound System      |     |     |     | ON  |
 Upright Sound System      |     |     |     | OFF |
---------------------------------------------------

Here is a complete list of the ROMs:
 Turbo ROMLIST - Frank Palazzolo
 Name    Loc             Function
 -----------------------------------------------------------------------------
 Images Acquired:
 EPR1262,3,4     IC76, IC89, IC103
 EPR1363,4,5
 EPR15xx                 Program ROMS
 EPR1244                 Character Data 1
 EPR1245                 Character Data 2
 EPR-1125                Road ROMS
 EPR-1126
 EPR-1127
 EPR-1238
 EPR-1239
 EPR-1240
 EPR-1241
 EPR-1242
 EPR-1243
 EPR1246-1258            Sprite ROMS
 EPR1288-1300

 PR-1114         IC13    Color 1 (road, etc.)
 PR-1115         IC18    Road gfx
 PR-1116         IC20    Crash (collision detection?)
 PR-1117         IC21    Color 2 (road, etc.)
 PR-1118         IC99    256x4 Character Color PROM
 PR-1119         IC50    512x8 Vertical Timing PROM
 PR-1120         IC62    Horizontal Timing PROM
 PR-1121         IC29    Color PROM
 PR-1122         IC11    Pattern 1
 PR-1123         IC21    Pattern 2

 PA-06R          IC22    Mathbox Timing PAL              (may be needed?)
 PA-06L          IC90    Address Decode PAL


 Issues:

     - it doesn't work!
	 - sprite position and scaling
	 - collision detection
	 - character colors  (mising a PROM I think)
	 - background colors -- this is most likely a bug
	 - speed

**************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* ******************************** */
/* Functions in src/vidhrdw/turbo.c */

int turbo_vh_start(void);
void turbo_vh_convert_color_prom(unsigned char *palette,
								 unsigned short *colortable,
								 const unsigned char *color_prom);
void turbo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern unsigned char 	*turbo_sprites_collisionram;
extern int turbo_sprites_collisionram_size;


/* ******************************** */
/* Functions in src/machine/turbo.c */

void turbo_init_machine(void);
int turbo_interrupt(void);
int turbo_fa00_r(int offset);
int turbo_fb00_r(int offset);
int turbo_fc00_r(int offset);
int turbo_fd00_r(int offset);
int turbo_fe00_r(int offset);
void turbo_b800_w(int offset, int data);
void turbo_f800_w(int offset, int data);
void turbo_f900_w(int offset, int data);
void turbo_fa00_w(int offset, int data);
void turbo_fb00_w(int offset, int data);
void turbo_fc00_w(int offset, int data);


/* ******************************** */

static struct Samplesinterface samples_interface =
{
        8       /* eight channels */
};

const char *turbo_sample_names[]=
{
	"*turbo",
	"01.sam", /* Trig1 */
	"02.sam", /* Trig2 */
	"03.sam", /* Trig3 */
	"04.sam", /* Trig4 */
	"10.sam", /* Ambulance */
	0 /*array end*/
};

static struct MemoryReadAddress turbo_readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM }, // ROM
	{ 0xb000, 0xb1ff, MRA_RAM }, // gets cleared at start
	{ 0xe000, 0xe7ff, videoram_r }, // VRAM
	{ 0xf000, 0xf7ff, MRA_RAM }, // RAM
	{ 0xfa00, 0xfa03, turbo_fa00_r }, // SOUND
	{ 0xfb00, 0xfbff, turbo_fb00_r },
	{ 0xfc00, 0xfcff, turbo_fc00_r },
	{ 0xfd00, 0xfdff, turbo_fd00_r },
	{ 0xfe00, 0xfeff, turbo_fe00_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress turbo_writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM }, // main program
	{ 0xa000, 0xa0ff, MWA_RAM, &spriteram, &spriteram_size }, // sprites
	{ 0xa800, 0xa803, MWA_RAM }, // coin meters/start lamp
	{ 0xb000, 0xb1ff, MWA_RAM, &turbo_sprites_collisionram, &turbo_sprites_collisionram_size }, // collision ram
	{ 0xb800, 0xb800, turbo_b800_w }, // not sure
	{ 0xe000, 0xe7ff, videoram_w, &videoram, &videoram_size }, // VRAM
	{ 0xe800, 0xe800, MWA_NOP }, // not sure
	{ 0xf000, 0xf7ff, MWA_RAM }, // RAM
	{ 0xf800, 0xf803, turbo_f800_w }, // road drawing
	{ 0xf900, 0xf903, turbo_f900_w }, // road drawing
	{ 0xfa00, 0xfa03, turbo_fa00_w }, // SOUND
	{ 0xfb00, 0xfbff, turbo_fb00_w },
	{ 0xfc00, 0xfcff, turbo_fc00_w }, // score
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( turbo_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 ) //SERVICE
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_SERVICE, "Test Mode", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_TOGGLE ) // SHIFT
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) //ACCEL A
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) //ACCEL B

	PORT_START  /* DSW 1 */
	PORT_DIPNAME( 0x03, 0x00, "Car On Extended Play", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "1")
	PORT_DIPSETTING(    0x01, "2")
	PORT_DIPSETTING(    0x02, "3")
	PORT_DIPSETTING(    0x03, "4")
	PORT_DIPNAME( 0x04, 0x00, "Game Time", IP_KEY_NONE)
	PORT_DIPSETTING(    0x04, "Fixed (55 sec)")
	PORT_DIPSETTING(    0x00, "Adjustable")
	PORT_DIPNAME( 0x08, 0x00, "Difficulty", IP_KEY_NONE)
	PORT_DIPSETTING(    0x08, "Easy")
	PORT_DIPSETTING(    0x00, "Hard")
	PORT_DIPNAME( 0x10, 0x00, "Game Mode", IP_KEY_NONE)
	PORT_DIPSETTING(    0x10, "No Collisions (cheat)")
	PORT_DIPSETTING(    0x00, "Normal")
	PORT_DIPNAME( 0x20, 0x20, "Initial Entry", IP_KEY_NONE)
	PORT_DIPSETTING(    0x20, "On")
	PORT_DIPSETTING(    0x00, "Off")
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE) //unused?
	PORT_DIPSETTING(    0x40, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE) //unused?
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On")

	PORT_START  /* DSW 2 */
	PORT_DIPNAME( 0x03, 0x03, "Game Time", IP_KEY_NONE)
	PORT_DIPSETTING(    0x01, "70 seconds")
	PORT_DIPSETTING(    0x00, "60 seconds")
	PORT_DIPSETTING(    0x02, "80 seconds")
	PORT_DIPSETTING(    0x03, "90 seconds")
	PORT_DIPNAME( 0xe0, 0xe0, "Coinage 1", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "1 coin/1 credits")
	PORT_DIPSETTING(    0x20, "1 coin/2 credits")
	PORT_DIPSETTING(    0x40, "1 coin/3 credits")
	PORT_DIPSETTING(    0x60, "1 coin/6 credits")
	PORT_DIPSETTING(    0x80, "2 coin/1 credits")
	PORT_DIPSETTING(    0xa0, "3 coin/1 credits")
	PORT_DIPSETTING(    0xc0, "4 coin/1 credits")
	PORT_DIPSETTING(    0xe0, "1 coin/1 credits")
	PORT_DIPNAME( 0x1c, 0x1c, "Coinage 2", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "1 coin/1 credits")
	PORT_DIPSETTING(    0x04, "1 coin/2 credits")
	PORT_DIPSETTING(    0x08, "1 coin/3 credits")
	PORT_DIPSETTING(    0x0c, "1 coin/6 credits")
	PORT_DIPSETTING(    0x10, "2 coin/1 credits")
	PORT_DIPSETTING(    0x14, "3 coin/1 credits")
	PORT_DIPSETTING(    0x18, "4 coin/1 credits")
	PORT_DIPSETTING(    0x1c, "1 coin/1 credits")

	PORT_START  /* DSW 3 */
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE) //unused
	PORT_DIPSETTING(    0x10, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE) //unused
	PORT_DIPSETTING(    0x20, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0x40, 0x40, "Tachometer", IP_KEY_NONE)
	PORT_DIPSETTING(    0x40, "Analog (Meter)")
	PORT_DIPSETTING(    0x00, "Digital (led)")
	PORT_DIPNAME( 0x80, 0x80, "Sound System", IP_KEY_NONE)
	PORT_DIPSETTING(    0x80, "upright")
	PORT_DIPSETTING(    0x00, "cockpit")
	PORT_DIPNAME( 0x0f, 0x00, "Unused", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "Unused")

	PORT_START      /* IN0 */
	PORT_ANALOGX( 0xff, 0, IPT_DIAL | IPF_CENTER, 1, 0, 0, 0,
				OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_JOY_LEFT, OSD_JOY_RIGHT, 16)

INPUT_PORTS_END


static struct GfxLayout turbo_charlayout =
{
	8,8,	/* 8*8 sprites */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */

	{ 256*8*8, 0 },	/* bitplane offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7},		/* x bit */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every chares 8 consecutive bytes */
};

static struct GfxLayout turbo_numlayout =
{
	10,8,	/* 8*8 sprites */
	16,	/* 16 characters */
	1,	/* 1 bit per pixel */

	{ 0 },	/* bitplane offsets */
	{ 9*8, 8*8, 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7},		/* x bit */
	10*8	/* every chares 10 consecutive bytes */
};


static struct GfxDecodeInfo turbo_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &turbo_charlayout,     0, 256 },
	{ 1, 0x0000, &turbo_numlayout,  4*256,   1 }, /* replaced in vh_start */
	{ -1 } /* end of array */
};


static struct MachineDriver turbo_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,
			0,
			turbo_readmem,turbo_writemem,0,0,
			turbo_interrupt,1
		}
	},
	30, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	turbo_init_machine,

	/* video hardware */
  	32*8, 36*8, { 0*8, 32*8-1, 0*8, 36*8-1 },
	turbo_gfxdecodeinfo,
	512+2,4*256+2,
	turbo_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	turbo_vh_start,
	generic_vh_stop,
	turbo_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( turbo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "epr1513.bin",  0x0000, 0x2000, 0x0326adfc )
	ROM_LOAD( "epr1514.bin",  0x2000, 0x2000, 0x25af63b0 )
	ROM_LOAD( "epr1515.bin",  0x4000, 0x2000, 0x059c1c36 )

    ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr1244.rom", 0x0000, 0x0800, 0x17f67424 )
	ROM_LOAD( "epr1245.rom", 0x0800, 0x0800, 0x2ba0b46b )

	ROM_REGION(0x1000) /* PROMs */
	ROM_LOAD( "pr1121.bin",  0x0000, 0x0200, 0x7692f497 )	/* Color PROM */
	ROM_LOAD( "pr-1118.bin", 0x0200, 0x0100, 0x07324cfd )	/* 256x4 Character Color PROM */
	ROM_LOAD( "pr1114.bin",  0x0300, 0x0020, 0x78aded46 )	/* Color 1 (road, etc.) */
	ROM_LOAD( "pr1117.bin",  0x0320, 0x0020, 0xf06d9907 )	/* Color 2 (road, etc.) */
	ROM_LOAD( "sndprom.bin", 0x0340, 0x0020, 0xb369a6ae )
	ROM_LOAD( "pr1122.bin",  0x0400, 0x0400, 0x0afed679 )	/* Pattern 1 */
	ROM_LOAD( "pr1123.bin",  0x0800, 0x0400, 0x02d2cb52 )	/* Pattern 2 */
	ROM_LOAD( "pr-1119.bin", 0x0c00, 0x0200, 0x628d3f1d )	/* timing - not used */
	ROM_LOAD( "pr-1120.bin", 0x0e00, 0x0200, 0x591b6a68 )	/* timing - not used */

    ROM_REGION(0x20000) /* the game will need access to this part */
	ROM_LOAD( "epr1246.rom", 0x00000, 0x2000, 0x555bfe9a )
	ROM_RELOAD(              0x02000, 0x2000 )
	ROM_LOAD( "epr1247.rom", 0x04000, 0x2000, 0xc8c5e4d5 )
	ROM_RELOAD(              0x06000, 0x2000 )
	ROM_LOAD( "epr1248.rom", 0x08000, 0x2000, 0x82fe5b94 )
	ROM_RELOAD(              0x0a000, 0x2000 )
	ROM_LOAD( "epr1249.rom", 0x0c000, 0x2000, 0xe258e009 )
	ROM_LOAD( "epr1250.rom", 0x0e000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1251.rom", 0x10000, 0x2000, 0x292573de )
	ROM_LOAD( "epr1252.rom", 0x12000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1253.rom", 0x14000, 0x2000, 0x92783626 )
	ROM_LOAD( "epr1254.rom", 0x16000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1255.rom", 0x18000, 0x2000, 0x485dcef9 )
	ROM_LOAD( "epr1256.rom", 0x1a000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1257.rom", 0x1c000, 0x2000, 0x4ca984ce )
	ROM_LOAD( "epr1258.rom", 0x1e000, 0x2000, 0xaee6e05e )

	ROM_REGION(0x4840) /* the game will need access to this part - road */
	ROM_LOAD( "epr1125.rom", 0x0000, 0x0800, 0x65b5d44b )
	ROM_LOAD( "epr1126.rom", 0x0800, 0x0800, 0x685ace1b )
	ROM_LOAD( "epr1127.rom", 0x1000, 0x0800, 0x9233c9ca )
	ROM_LOAD( "epr1238.rom", 0x1800, 0x0800, 0xd94fd83f )
	ROM_LOAD( "epr1239.rom", 0x2000, 0x0800, 0x4c41124f )
	ROM_LOAD( "epr1240.rom", 0x2800, 0x0800, 0x371d6282 )
	ROM_LOAD( "epr1241.rom", 0x3000, 0x0800, 0x1109358a )
	ROM_LOAD( "epr1242.rom", 0x3800, 0x0800, 0x04866769 )
	ROM_LOAD( "epr1243.rom", 0x4000, 0x0800, 0x29854c48 )
	ROM_LOAD( "pr1115.bin",  0x4800, 0x0020, 0x5394092c )
	ROM_LOAD( "pr1116.bin",  0x4820, 0x0020, 0x3956767d )
ROM_END

ROM_START( turboa_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "epr1262.rom",  0x0000, 0x2000, 0x1951b83a )
	ROM_LOAD( "epr1263.rom",  0x2000, 0x2000, 0x45e01608 )
	ROM_LOAD( "epr1264.rom",  0x4000, 0x2000, 0x1802f6c7 )

    ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr1244.rom", 0x0000, 0x0800, 0x17f67424 )
	ROM_LOAD( "epr1245.rom", 0x0800, 0x0800, 0x2ba0b46b )

	ROM_REGION(0x1000) /* PROMs */
	ROM_LOAD( "pr1121.bin",  0x0000, 0x0200, 0x7692f497 )	/* Color PROM */
	ROM_LOAD( "pr-1118.bin", 0x0200, 0x0100, 0x07324cfd )	/* 256x4 Character Color PROM */
	ROM_LOAD( "pr1114.bin",  0x0300, 0x0020, 0x78aded46 )	/* Color 1 (road, etc.) */
	ROM_LOAD( "pr1117.bin",  0x0320, 0x0020, 0xf06d9907 )	/* Color 2 (road, etc.) */
	ROM_LOAD( "sndprom.bin", 0x0340, 0x0020, 0xb369a6ae )
	ROM_LOAD( "pr1122.bin",  0x0400, 0x0400, 0x0afed679 )	/* Pattern 1 */
	ROM_LOAD( "pr1123.bin",  0x0800, 0x0400, 0x02d2cb52 )	/* Pattern 2 */
	ROM_LOAD( "pr-1119.bin", 0x0c00, 0x0200, 0x628d3f1d )	/* timing - not used */
	ROM_LOAD( "pr-1120.bin", 0x0e00, 0x0200, 0x591b6a68 )	/* timing - not used */

    ROM_REGION(0x20000) /* the game will need access to this part */
	ROM_LOAD( "epr1246.rom", 0x00000, 0x2000, 0x555bfe9a )
	ROM_RELOAD(              0x02000, 0x2000 )
	ROM_LOAD( "epr1247.rom", 0x04000, 0x2000, 0xc8c5e4d5 )
	ROM_RELOAD(              0x06000, 0x2000 )
	ROM_LOAD( "epr1248.rom", 0x08000, 0x2000, 0x82fe5b94 )
	ROM_RELOAD(              0x0a000, 0x2000 )
	ROM_LOAD( "epr1249.rom", 0x0c000, 0x2000, 0xe258e009 )
	ROM_LOAD( "epr1250.rom", 0x0e000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1251.rom", 0x10000, 0x2000, 0x292573de )
	ROM_LOAD( "epr1252.rom", 0x12000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1253.rom", 0x14000, 0x2000, 0x92783626 )
	ROM_LOAD( "epr1254.rom", 0x16000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1255.rom", 0x18000, 0x2000, 0x485dcef9 )
	ROM_LOAD( "epr1256.rom", 0x1a000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1257.rom", 0x1c000, 0x2000, 0x4ca984ce )
	ROM_LOAD( "epr1258.rom", 0x1e000, 0x2000, 0xaee6e05e )

	ROM_REGION(0x4840) /* the game will need access to this part - road */
	ROM_LOAD( "epr1125.rom", 0x0000, 0x0800, 0x65b5d44b )
	ROM_LOAD( "epr1126.rom", 0x0800, 0x0800, 0x685ace1b )
	ROM_LOAD( "epr1127.rom", 0x1000, 0x0800, 0x9233c9ca )
	ROM_LOAD( "epr1238.rom", 0x1800, 0x0800, 0xd94fd83f )
	ROM_LOAD( "epr1239.rom", 0x2000, 0x0800, 0x4c41124f )
	ROM_LOAD( "epr1240.rom", 0x2800, 0x0800, 0x371d6282 )
	ROM_LOAD( "epr1241.rom", 0x3000, 0x0800, 0x1109358a )
	ROM_LOAD( "epr1242.rom", 0x3800, 0x0800, 0x04866769 )
	ROM_LOAD( "epr1243.rom", 0x4000, 0x0800, 0x29854c48 )
	ROM_LOAD( "pr1115.bin",  0x4800, 0x0020, 0x5394092c )
	ROM_LOAD( "pr1116.bin",  0x4820, 0x0020, 0x3956767d )
ROM_END

ROM_START( turbob_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "epr-1363.cpu",  0x0000, 0x2000, 0x5c110fb6 )
	ROM_LOAD( "epr-1364.cpu",  0x2000, 0x2000, 0x6a341693 )
	ROM_LOAD( "epr-1365.cpu",  0x4000, 0x2000, 0x3b6b0dc8 )

    ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "epr1244.rom", 0x0000, 0x0800, 0x17f67424 )
	ROM_LOAD( "epr1245.rom", 0x0800, 0x0800, 0x2ba0b46b )

	ROM_REGION(0x1000) /* PROMs */
	ROM_LOAD( "pr1121.bin",  0x0000, 0x0200, 0x7692f497 )	/* Color PROM */
	ROM_LOAD( "pr-1118.bin", 0x0200, 0x0100, 0x07324cfd )	/* 256x4 Character Color PROM */
	ROM_LOAD( "pr1114.bin",  0x0300, 0x0020, 0x78aded46 )	/* Color 1 (road, etc.) */
	ROM_LOAD( "pr1117.bin",  0x0320, 0x0020, 0xf06d9907 )	/* Color 2 (road, etc.) */
	ROM_LOAD( "sndprom.bin", 0x0340, 0x0020, 0xb369a6ae )
	ROM_LOAD( "pr1122.bin",  0x0400, 0x0400, 0x0afed679 )	/* Pattern 1 */
	ROM_LOAD( "pr1123.bin",  0x0800, 0x0400, 0x02d2cb52 )	/* Pattern 2 */
	ROM_LOAD( "pr-1119.bin", 0x0c00, 0x0200, 0x628d3f1d )	/* timing - not used */
	ROM_LOAD( "pr-1120.bin", 0x0e00, 0x0200, 0x591b6a68 )	/* timing - not used */

    ROM_REGION(0x20000) /* the game will need access to this part */
	ROM_LOAD( "epr1246.rom", 0x00000, 0x2000, 0x555bfe9a )
	ROM_RELOAD(              0x02000, 0x2000 )
	ROM_LOAD( "mpr1290.rom", 0x04000, 0x2000, 0x95182020 )	/* is this good? */
	ROM_RELOAD(              0x06000, 0x2000 )
	ROM_LOAD( "epr1248.rom", 0x08000, 0x2000, 0x82fe5b94 )
	ROM_RELOAD(              0x0a000, 0x2000 )
	ROM_LOAD( "mpr1291.rom", 0x0c000, 0x2000, 0x0e857f82 )	/* is this good? */
	ROM_LOAD( "epr1250.rom", 0x0e000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1251.rom", 0x10000, 0x2000, 0x292573de )
	ROM_LOAD( "epr1252.rom", 0x12000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1253.rom", 0x14000, 0x2000, 0x92783626 )
	ROM_LOAD( "epr1254.rom", 0x16000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1255.rom", 0x18000, 0x2000, 0x485dcef9 )
	ROM_LOAD( "epr1256.rom", 0x1a000, 0x2000, 0xaee6e05e )
	ROM_LOAD( "epr1257.rom", 0x1c000, 0x2000, 0x4ca984ce )
	ROM_LOAD( "epr1258.rom", 0x1e000, 0x2000, 0xaee6e05e )

	ROM_REGION(0x4840) /* the game will need access to this part - road */
	ROM_LOAD( "epr1125.rom", 0x0000, 0x0800, 0x65b5d44b )
	ROM_LOAD( "epr1126.rom", 0x0800, 0x0800, 0x685ace1b )
	ROM_LOAD( "epr1127.rom", 0x1000, 0x0800, 0x9233c9ca )
	ROM_LOAD( "epr1238.rom", 0x1800, 0x0800, 0xd94fd83f )
	ROM_LOAD( "epr1239.rom", 0x2000, 0x0800, 0x4c41124f )
	ROM_LOAD( "epr1240.rom", 0x2800, 0x0800, 0x371d6282 )
	ROM_LOAD( "epr1241.rom", 0x3000, 0x0800, 0x1109358a )
	ROM_LOAD( "epr1242.rom", 0x3800, 0x0800, 0x04866769 )
	ROM_LOAD( "epr1243.rom", 0x4000, 0x0800, 0x29854c48 )
	ROM_LOAD( "pr1115.bin",  0x4800, 0x0020, 0x5394092c )
	ROM_LOAD( "pr1116.bin",  0x4820, 0x0020, 0x3956767d )
ROM_END



static void turbo_decode(void)
{
/*
 * The table is arranged this way (second half is mirror image of first)
 *
 *      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
 *
 * 0   00 00 00 00 01 01 01 01 02 02 02 02 03 03 03 03
 * 1   04 04 04 04 05 05 05 05 06 06 06 06 07 07 07 07
 * 2   08 08 08 08 09 09 09 09 0A 0A 0A 0A 0B 0B 0B 0B
 * 3   0C 0C 0C 0C 0D 0D 0D 0D 0E 0E 0E 0E 0F 0F 0F 0F
 * 4   10 10 10 10 11 11 11 11 12 12 12 12 13 13 13 13
 * 5   14 14 14 14 15 15 15 15 16 16 16 16 17 17 17 17
 * 6   18 18 18 18 19 19 19 19 1A 1A 1A 1A 1B 1B 1B 1B
 * 7   1C 1C 1C 1C 1D 1D 1D 1D 1E 1E 1E 1E 1F 1F 1F 1F
 * 8   1F 1F 1F 1F 1E 1E 1E 1E 1D 1D 1D 1D 1C 1C 1C 1C
 * 9   1B 1B 1B 1B 1A 1A 1A 1A 19 19 19 19 18 18 18 18
 * A   17 17 17 17 16 16 16 16 15 15 15 15 14 14 14 14
 * B   13 13 13 13 12 12 12 12 11 11 11 11 10 10 10 10
 * C   0F 0F 0F 0F 0E 0E 0E 0E 0D 0D 0D 0D 0C 0C 0C 0C
 * D   0B 0B 0B 0B 0A 0A 0A 0A 09 09 09 09 08 08 08 08
 * E   07 07 07 07 06 06 06 06 05 05 05 05 04 04 04 04
 * F   03 03 03 03 02 02 02 02 01 01 01 01 00 00 00 00
 *
 */

	int offs,i,j;
	unsigned char *RAM;
	unsigned char src;
	static unsigned char xortable[4][32]=
	{
		/* Table 0 */
		/* 0x0000-0x3ff */
		/* 0x0800-0xbff */
		/* 0x4000-0x43ff */
		/* 0x4800-0x4bff */
		{ 0x00,0x44,0x0c,0x48,0x00,0x44,0x0c,0x48,
		  0xa0,0xe4,0xac,0xe8,0xa0,0xe4,0xac,0xe8,
		  0x60,0x24,0x6c,0x28,0x60,0x24,0x6c,0x28,
		  0xc0,0x84,0xcc,0x88,0xc0,0x84,0xcc,0x88 },

		/* Table 1 */
		/* 0x0400-0x07ff */
		/* 0x0c00-0x0fff */
		/* 0x1400-0x17ff */
		/* 0x1c00-0x1fff */
		/* 0x2400-0x27ff */
		/* 0x2c00-0x2fff */
		/* 0x3400-0x37ff */
		/* 0x3c00-0x3fff */
		/* 0x4400-0x47ff */
		/* 0x4c00-0x4fff */
		/* 0x5400-0x57ff */
		/* 0x5c00-0x5fff */
		{ 0x00,0x44,0x18,0x5c,0x14,0x50,0x0c,0x48,
		  0x28,0x6c,0x30,0x74,0x3c,0x78,0x24,0x60,
		  0x60,0x24,0x78,0x3c,0x74,0x30,0x6c,0x28,
		  0x48,0x0c,0x50,0x14,0x5c,0x18,0x44,0x00 }, //0x00 --> 0x10 ?

		/* Table 2 */
		/* 0x1000-0x13ff */
		/* 0x1800-0x1bff */
		/* 0x5000-0x53ff */
		/* 0x5800-0x5bff */
		{ 0x00,0x00,0x28,0x28,0x90,0x90,0xb8,0xb8,
		  0x28,0x28,0x00,0x00,0xb8,0xb8,0x90,0x90,
		  0x00,0x00,0x28,0x28,0x90,0x90,0xb8,0xb8,
		  0x28,0x28,0x00,0x00,0xb8,0xb8,0x90,0x90 },

		/* Table 3 */
		/* 0x2000-0x23ff */
		/* 0x2800-0x2bff */
		/* 0x3000-0x33ff */
		/* 0x3800-0x3bff */
		{ 0x00,0x14,0x88,0x9c,0x30,0x24,0xb8,0xac,
		  0x24,0x30,0xac,0xb8,0x14,0x00,0x9c,0x88,
		  0x48,0x5c,0xc0,0xd4,0x78,0x6c,0xf0,0xe4,
		  0x6c,0x78,0xe4,0xf0,0x5c,0x48,0xd4,0xc0 }
	};

	int findtable[]=
	{
		0,1,0,1, /* 0x0000-0x0fff */
		2,1,2,1, /* 0x1000-0x1fff */
		3,1,3,1, /* 0x2000-0x2fff */
		3,1,3,1, /* 0x3000-0x3fff */
		0,1,0,1, /* 0x4000-0x4fff */
		2,1,2,1  /* 0x5000-0x5fff */
	};

	RAM = Machine->memory_region[0];

	for (offs = 0x0000;offs < 0x6000; offs++)
	{
		src=RAM[offs];
		i=findtable[offs>>10];
		j=src>>2;
		if (src&0x80) j = 0x3f - j;
		RAM[offs] = src ^ xortable[i][j];
	}

}

struct GameDriver turbo_driver =
{
	__FILE__,
	0,
	"turbo",
	"Turbo",
	"1981",
	"Sega",
	"Alex Pasadyn\nHowie Cohen\nFrank Palazzolo",
	GAME_NOT_WORKING,
	&turbo_machine_driver,
	0,

	turbo_rom,
	0, 0,	/* rom decode and opcode decode functions */
	turbo_sample_names,
	0,      /* sound_prom */

	turbo_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

struct GameDriver turboa_driver =
{
	__FILE__,
	&turbo_driver,
	"turboa",
	"Turbo (encrypted set 1)",
	"1981",
	"Sega",
	"Alex Pasadyn\nHowie Cohen\nFrank Palazzolo",
	GAME_NOT_WORKING,
	&turbo_machine_driver,
	0,

	turboa_rom,
	turbo_decode, 0,	/* rom decode and opcode decode functions */
	turbo_sample_names,
	0,      /* sound_prom */

	turbo_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

struct GameDriver turbob_driver =
{
	__FILE__,
	&turbo_driver,
	"turbob",
	"Turbo (encrypted set 2)",
	"1981",
	"Sega",
	"Alex Pasadyn\nHowie Cohen\nFrank Palazzolo",
	GAME_NOT_WORKING,
	&turbo_machine_driver,
	0,

	turbob_rom,
	turbo_decode, 0,	/* rom decode and opcode decode functions */
	turbo_sample_names,
	0,      /* sound_prom */

	turbo_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	0,0
};
