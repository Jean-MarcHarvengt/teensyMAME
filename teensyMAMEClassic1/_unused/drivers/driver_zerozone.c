/***************************************************************************

Zero Zone memory map

CPU 1 : 68000, uses irq 1

0x000000 - 0x01ffff : ROM
0x080000 - 0x08000f : input ports and dipswitches
0x088000 - 0x0881ff : palette RAM, 256 total colors
0x09ce00 - 0x09d9ff : video ram, 48x32
0x0c0000 - 0x0cffff : RAM
0x0f8000 - 0x0f87ff : RAM (unused?)

TODO:
	* adpcm samples don't seem to be playing at the proper tempo - too fast?
	* There are a lot of unknown dipswitches

***************************************************************************/
#include "driver.h"
#include "M68000/M68000.h"
#include "Z80/Z80.h"
#include "vidhrdw/generic.h"

void zerozone_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
int zerozone_vh_start(void);
void zerozone_vh_stop(void);
int zerozone_videoram_r(int offset);
void zerozone_videoram_w(int offset, int data);

extern unsigned char *zerozone_videoram;
static unsigned char *ram; /* for high score save */

static int zerozone_input_r (int offset)
{
	switch (offset)
	{
		case 0x00:
			return readinputport(0); /* IN0 */
		case 0x02:
			return (readinputport(1) | (readinputport(2) << 8)); /* IN1 & IN2 */
		case 0x08:
			return (readinputport(4) << 8);
		case 0x0a:
			return readinputport(3);
	}

if (errorlog) fprintf(errorlog,"CPU #0 PC %06x: warning - read unmapped memory address %06x\n",cpu_getpc(),0x800000+offset);

	return 0x00;
}


void zerozone_sound_w (int offset, int data)
{
	soundlatch_w (offset, (data >> 8) & 0xff);
	cpu_cause_interrupt (1, 0xff);
}

static int zerozone_adpcm_status_r (int offset)
{
	return (ADPCM_playing (0));
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x01ffff, MRA_ROM },
	{ 0x080000, 0x08000f, zerozone_input_r },
	{ 0x088000, 0x0881ff, paletteram_word_r },
//	{ 0x098000, 0x098001, MRA_RAM }, /* watchdog? */
	{ 0x09ce00, 0x09d9ff, zerozone_videoram_r },
	{ 0x0c0000, 0x0cffff, MRA_BANK1, &ram },
	{ 0x0f8000, 0x0f87ff, MRA_BANK2 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x01ffff, MWA_ROM },
	{ 0x084000, 0x084001, zerozone_sound_w },
	{ 0x088000, 0x0881ff, paletteram_BBBBGGGGRRRRxxxx_word_w, &paletteram },
	{ 0x09ce00, 0x09d9ff, zerozone_videoram_w, &zerozone_videoram, &videoram_size },
	{ 0x0c0000, 0x0cffff, MWA_BANK1 }, /* RAM */
	{ 0x0f8000, 0x0f87ff, MWA_BANK2 },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
//	{ 0x9800, 0x9800, OKIM6295_status_r },
	{ 0x9800, 0x9800, zerozone_adpcm_status_r },
	{ 0xa000, 0xa000, soundlatch_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9800, 0x9800, OKIM6295_data_w },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START /* DSW A */
	PORT_DIPNAME( 0x07, 0x07, "Coinage", IP_KEY_NONE)
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit")
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits")
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits")
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits")
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit")
	PORT_DIPSETTING(    0x02, "3 Coins/1 Credit")
	PORT_DIPSETTING(    0x01, "4 Coins/1 Credit")
	PORT_DIPSETTING(    0x00, "6 Coins/1 Credit")
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE)
	PORT_DIPSETTING(    0x08, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE)
	PORT_DIPSETTING(    0x10, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE)
	PORT_DIPSETTING(    0x20, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0xc0, 0xc0, "Unknown", IP_KEY_NONE)
	PORT_DIPSETTING(    0xc0, "1")
	PORT_DIPSETTING(    0x80, "2")
	PORT_DIPSETTING(    0x40, "3")
	PORT_DIPSETTING(    0x00, "4")

	PORT_START /* DSW B */
	PORT_DIPNAME( 0x01, 0x01, "Unknown",IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0x02, 0x02, "Unknown",IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0x0c, 0x0c, "Unknown",IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "1")
	PORT_DIPSETTING(    0x04, "2")
	PORT_DIPSETTING(    0x08, "3")
	PORT_DIPSETTING(    0x00, "4")
	PORT_DIPNAME( 0x10, 0x10, "Unknown",IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE)
	PORT_DIPSETTING(    0x20, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE)
	PORT_DIPSETTING(    0x40, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8+0, 8+4, 16+0, 16+4, 24+0, 24+4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8	/* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout, 0, 256 },         /* sprites & playfield */
	{ -1 } /* end of array */
};


static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	8000,           /* 8000Hz ??? TODO: find out the real frequency */
	3,              /* memory region 3 */
	{ 255 }
};

static struct MachineDriver machine_driver =
{
	{
		{
			CPU_M68000,
			10000000,	/* 10 MHz */
			0,
			readmem,writemem,0,0,
			m68_level1_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			1000000,	/* 1 MHz ??? */
			2,
			sound_readmem, sound_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the main cpu */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	0,

	/* video hardware */
	48*8, 32*8, { 0*8, 48*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE,
	0,
	zerozone_vh_start,
	zerozone_vh_stop,
	zerozone_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};

/***************************************************************************

  High score save/load

***************************************************************************/

static int hiload(void)
{
	void *f;

	/* check if the hi score table has already been initialized */

    if (READ_WORD(&ram[0x17cc]) == 0x0053 && READ_WORD(&ram[0x1840]) == 0x0700 && READ_WORD(&ram[0x23da]) == 0x03 )
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread_msbfirst(f,&ram[0x17cc],0x78);
			osd_fclose(f);
			memcpy(&ram[0x23da],&ram[0x17d2],6);	/* copy high score */
		}
		return 1;
	}
	else return 0;
}

static void hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite_msbfirst(f,&ram[0x17cc],0x78);
		osd_fclose(f);
	}
}


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( zerozone_rom )
	ROM_REGION(0x20000)     /* 128k for 68000 code */
	ROM_LOAD_EVEN( "zz-4.rom", 0x0000, 0x10000, 0x83718b9b )
	ROM_LOAD_ODD ( "zz-5.rom", 0x0000, 0x10000, 0x18557f41 )

	ROM_REGION_DISPOSE(0x080000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "zz-6.rom", 0x00000, 0x80000, 0xc8b906b9 )

	ROM_REGION(0x10000)      /* sound cpu */
	ROM_LOAD( "zz-1.rom", 0x00000, 0x08000, 0x223ccce5 )

	ROM_REGION(0x40000)      /* ADPCM samples */
	ROM_LOAD( "zz-2.rom", 0x00000, 0x20000, 0xc7551e81 )
	ROM_LOAD( "zz-3.rom", 0x20000, 0x20000, 0xe348ff5e )
ROM_END



struct GameDriver zerozone_driver =
{
	__FILE__,
	0,
	"zerozone",
	"Zero Zone",
	"1993",
	"Comad",
	"Brad Oliver",
	0,
	&machine_driver,
	0,

	zerozone_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};
