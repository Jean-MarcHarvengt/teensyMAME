/***************************************************************************

Atari Tetris Memory Map (preliminary)

0000-0fff   RAM
1000-1fff   Video RAM
2000-20ff   Palette RAM
2400-25ff   EEPROM
4000-7fff   Paged ROM (Slapstic controlled)
8000-ffff   ROM

I/O

Read

2800-280f Pokey #1
2800-281f Pokey #2

Write

2800-280f Pokey #1
2810-281f Pokey #2
3000      Watchdog
3400      EEPROM enable
3800      ???
3c00      ???

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


void atetris_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int  atetris_slapstic_r(int offset);



static struct MemoryReadAddress readmem[] =
{
        { 0x0000, 0x20ff, MRA_RAM },
        { 0x2400, 0x25ff, MRA_RAM },
        { 0x2800, 0x280f, pokey1_r },
        { 0x2810, 0x281f, pokey2_r },
        { 0x4000, 0x7fff, atetris_slapstic_r },
        { 0x8000, 0xffff, MRA_ROM },
};

static struct MemoryWriteAddress writemem[] =
{
        { 0x0000, 0x0fff, MWA_RAM },
        { 0x1000, 0x1fff, videoram_w, &videoram, &videoram_size },
        { 0x2000, 0x20ff, paletteram_RRRGGGBB_w, &paletteram },
        { 0x2400, 0x25ff, MWA_RAM },
        { 0x2800, 0x280f, pokey1_w },
        { 0x2810, 0x281f, pokey2_w },
        { 0x3000, 0x3000, watchdog_reset_w },
        { 0x3400, 0x3400, MWA_NOP },  // EEPROM enable
        { 0x3800, 0x3800, MWA_NOP },  // ???
        { 0x3c00, 0x3c00, MWA_NOP },  // ???
        { -1 }  /* end of table */
};


INPUT_PORTS_START( ports )
        // These ports are read via the Pokeys
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
        PORT_BITX(0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Freeze", OSD_KEY_5, IP_JOY_NONE, 0 )
        PORT_DIPSETTING(0x00, "Off" )
        PORT_DIPSETTING(0x04, "On" )
        PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_SERVICE, "Freeze Step", OSD_KEY_6, IP_JOY_NONE, 0)
        PORT_BIT( 0x30, IP_ACTIVE_HIGH, IPT_UNUSED )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )
        PORT_BITX(0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
        PORT_DIPSETTING(0x00, "Off" )
        PORT_DIPSETTING(0x80, "On" )

        PORT_START      /* IN1 */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1)
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 | IPF_8WAY )
        PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
        PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 | IPF_8WAY )
        PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2)
        PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 | IPF_8WAY )
        PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
        PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 | IPF_8WAY )
INPUT_PORTS_END


// Same as the regular one except they added a Flip Controls switch
INPUT_PORTS_START( atetcktl_ports )
        // These ports are read via the Pokeys
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
        PORT_BITX(0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Freeze", OSD_KEY_5, IP_JOY_NONE, 0 )
        PORT_DIPSETTING(0x00, "Off" )
        PORT_DIPSETTING(0x04, "On" )
        PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_SERVICE, "Freeze Step", OSD_KEY_6, IP_JOY_NONE, 0)
        PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
        PORT_DIPNAME( 0x20, 0x00, "Flip Controls", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0x20, "On" )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )
        PORT_BITX(0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
        PORT_DIPSETTING(0x00, "Off" )
        PORT_DIPSETTING(0x80, "On" )

        PORT_START      /* IN1 */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1)
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 | IPF_8WAY )
        PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
        PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 | IPF_8WAY )
        PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2)
        PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 | IPF_8WAY )
        PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
        PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 | IPF_8WAY )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
        8,8,    /* 8*8 chars */
        2048,   /* 2048 characters */
        4,      /* 4 bits per pixel */
        { 0,1,2,3 },  // The 4 planes are packed together
        { 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4},
        { 0*4*8, 1*4*8, 2*4*8, 3*4*8, 4*4*8, 5*4*8, 6*4*8, 7*4*8},
        8*8*4     /* every char takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        { 1, 0x0000, &charlayout, 0, 16 },
        { -1 } /* end of array */
};



static struct POKEYinterface pokey_interface =
{
	2,      /* 2 chips */
	1789790,	/* ? */
	50,
	POKEY_DEFAULT_GAIN,
	NO_CLIP,
	/* The 8 pot handlers */
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	/* The allpot handler */
	{ input_port_0_r, input_port_1_r }
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1750000,        /* 1.75 MHz??? */
			0,
			readmem,writemem,0,0,
			interrupt,4
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	42*8, 32*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	atetris_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_POKEY,
			&pokey_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( atetris_rom )
	ROM_REGION(0x14000)     /* 80k for code */
	ROM_LOAD( "1100.45f",     0x0000, 0x10000, 0x2acbdb09 )

	ROM_REGION_DISPOSE(0x10000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1101.35a",     0x0000, 0x10000, 0x84a1939f )
ROM_END

ROM_START( atetrisa_rom )
	ROM_REGION(0x14000)     /* 80k for code */
	ROM_LOAD( "d1",           0x0000, 0x10000, 0x2bcab107 )

	ROM_REGION_DISPOSE(0x10000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1101.35a",     0x0000, 0x10000, 0x84a1939f )
ROM_END

ROM_START( atetrisb_rom )
	ROM_REGION(0x14000)     /* 80k for code */
	ROM_LOAD( "tetris.01",    0x0000, 0x10000, 0x944d15f6 )

	ROM_REGION_DISPOSE(0x10000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tetris.02",    0x0000, 0x10000, 0x5c4e7258 )

	/* there's an extra EEPROM, maybe used for protection crack, which */
	/* however doesn't seem to be required to run the game in this driver. */
ROM_END

ROM_START( atetcktl_rom )
	ROM_REGION(0x14000)     /* 80k for code */
	ROM_LOAD( "tetcktl1.rom", 0x0000, 0x10000, 0x9afd1f4a )

	ROM_REGION_DISPOSE(0x10000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1103.35a",     0x0000, 0x10000, 0xec2a7f93 )
ROM_END

ROM_START( atetckt2_rom )
	ROM_REGION(0x14000)     /* 80k for code */
	ROM_LOAD( "1102.45f",     0x0000, 0x10000, 0x1bd28902 )

	ROM_REGION_DISPOSE(0x10000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1103.35a",     0x0000, 0x10000, 0xec2a7f93 )
ROM_END



static void atetris_rom_move (void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    // Move the lower 16k to 0x10000
    memcpy(&RAM[0x10000], &RAM[0x00000], 0x4000);
    memset(&RAM[0x00000], 0, 0x4000);
}


static int hiload (void)
{
   void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


   f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
   if (f)
   {
        // Load EEPROM
        osd_fread (f, &RAM[0x2400], 0x200);
        osd_fclose (f);
   }
   else
   {
        // Initialize EEPROM so the game can reset it
        memset(&RAM[0x2400], 0xff, 0x200);
   }

   return 1;
}


static void hisave (void)
{
   void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


   f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 1);
   if (f)
   {
      osd_fwrite (f, &RAM[0x2400], 0x200);
      osd_fclose (f);
   }
}


struct GameDriver atetris_driver =
{
	__FILE__,
	0,
	"atetris",
	"Tetris (set 1)",
	"1988",
	"Atari Games",
	"Zsolt Vasvari",
	0,
	&machine_driver,
	0,

	atetris_rom,
	atetris_rom_move,
	0,
	0,
	0,      /* sound_prom */

	ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver atetrisa_driver =
{
	__FILE__,
	&atetris_driver,
	"atetrisa",
	"Tetris (set 2)",
	"1988",
	"Atari Games",
	"Zsolt Vasvari",
	0,
	&machine_driver,
	0,

	atetrisa_rom,
	atetris_rom_move,
	0,
	0,
	0,      /* sound_prom */

	ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver atetrisb_driver =
{
	__FILE__,
	&atetris_driver,
	"atetrisb",
	"Tetris (bootleg)",
	"1988",
	"bootleg",
	"Zsolt Vasvari",
	0,
	&machine_driver,
	0,

	atetrisb_rom,
	atetris_rom_move,
	0,
	0,
	0,      /* sound_prom */

	ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver atetcktl_driver =
{
	__FILE__,
	&atetris_driver,
	"atetcktl",
	"Tetris (Cocktail set 1)",
	"1989",
	"Atari Games",
	"Zsolt Vasvari",
	0,
	&machine_driver,
	0,

	atetcktl_rom,
	atetris_rom_move,
	0,
	0,
	0,      /* sound_prom */

	atetcktl_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_ROTATE_270,
	hiload, hisave
};

struct GameDriver atetckt2_driver =
{
	__FILE__,
	&atetris_driver,
	"atetckt2",
	"Tetris (Cocktail set 2)",
	"1989",
	"Atari Games",
	"Zsolt Vasvari",
	0,
	&machine_driver,
	0,

	atetckt2_rom,
	atetris_rom_move,
	0,
	0,
	0,      /* sound_prom */

	atetcktl_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_ROTATE_270,
	hiload, hisave
};
