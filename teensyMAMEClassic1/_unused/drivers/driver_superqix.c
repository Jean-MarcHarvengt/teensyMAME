/***************************************************************************

Super QIX memory map (preliminary)

CPU:
0000-7fff ROM
8000-bfff BANK 0-1-2-3	(All banks except 2 contain code)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



int superqix_vh_start(void);
void superqix_vh_stop(void);
int superqix_bitmapram_r(int offset);
void superqix_bitmapram_w(int offset,int data);
int superqix_bitmapram2_r(int offset);
void superqix_bitmapram2_w(int offset,int data);
void superqix_0410_w(int offset,int data);
void superqix_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xe000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xe0ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xe100, 0xe7ff, MWA_RAM },
	{ 0xe800, 0xebff, videoram_w, &videoram, &videoram_size },
	{ 0xec00, 0xefff, colorram_w, &colorram },
	{ 0xf000, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x0000, 0x00ff, paletteram_r },
	{ 0x0401, 0x0401, AY8910_read_port_0_r },
	{ 0x0405, 0x0405, AY8910_read_port_1_r },
	{ 0x0418, 0x0418, input_port_4_r },
	{ 0x0800, 0x77ff, superqix_bitmapram_r },
	{ 0x8800, 0xf7ff, superqix_bitmapram2_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x0000, 0x00ff, paletteram_BBGGRRII_w },
	{ 0x0402, 0x0402, AY8910_write_port_0_w },
	{ 0x0403, 0x0403, AY8910_control_port_0_w },
	{ 0x0406, 0x0406, AY8910_write_port_1_w },
	{ 0x0407, 0x0407, AY8910_control_port_1_w },
	{ 0x0410, 0x0410, superqix_0410_w },	/* ROM bank, NMI enable, tile bank */
	{ 0x0800, 0x77ff, superqix_bitmapram_w },
	{ 0x8800, 0xf7ff, superqix_bitmapram2_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )	/* ??? */
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME, "Freeze???", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/3 Credit")
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credit" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/3 Credit")
	PORT_DIPSETTING(    0x04, "1 Coin/2 Credit" )
	PORT_DIPNAME( 0x10, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x20, 0x20, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "30000 100000" )
	PORT_DIPSETTING(    0x08, "30000 50000" )
	PORT_DIPSETTING(    0x04, "50000 100000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0xc0, 0xc0, "Fill Area", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "70%" )
	PORT_DIPSETTING(    0xc0, "75%" )
	PORT_DIPSETTING(    0x40, "80%" )
	PORT_DIPSETTING(    0x00, "85%" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,    /* 1024 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8   /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,    /* 16*16 sprites */
	512,    /* 512 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			32*8+0*4, 32*8+1*4, 32*8+2*4, 32*8+3*4, 32*8+4*4, 32*8+5*4, 32*8+6*4, 32*8+7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	128*8   /* every sprites takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,   0, 16 },	/* Chars */
	{ 1, 0x08000, &charlayout,   0, 16 },	/* Background tiles */
	{ 1, 0x10000, &charlayout,   0, 16 },
	{ 1, 0x18000, &charlayout,   0, 16 },
	{ 1, 0x20000, &charlayout,   0, 16 },
	{ 1, 0x28000, &spritelayout, 0, 16 },	/* Sprites */
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz??? */
	{ 255, 255 },
	{ input_port_0_r, input_port_3_r },		/* port Aread */
	{ input_port_1_r, input_port_2_r },		/* port Bread */
	{ 0 },	/* port Awrite */
	{ 0 }	/* port Bwrite */
};

int sqix_interrupt(void)
{
	static int loop=0;

	loop++;

	if(loop>2) {
		if(loop==6) loop=0;
		return nmi_interrupt();
	}
	else
		return 0;
}

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80 | CPU_16BIT_PORT,
//			10000000,	/* 10 Mhz ? */
			6000000,	/* 6 Mhz ? */
			0,
			readmem,writemem,readport,writeport,
//			nmi_interrupt,3	/* ??? */
			sqix_interrupt,6	/* ??? */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,// | VIDEO_SUPPORTS_DIRTY,
	0,
	superqix_vh_start,
	superqix_vh_stop,
	superqix_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( superqix_rom )
	ROM_REGION(0x20000)	/* 64k for code */
	ROM_LOAD( "sq01.97",      0x00000, 0x08000, 0x0888b7de )
	ROM_LOAD( "sq02.96",      0x10000, 0x10000, 0x9c23cb64 )

	ROM_REGION_DISPOSE(0x38000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sq04.2",       0x00000, 0x08000, 0xf815ef45 )
	ROM_LOAD( "sq03.3",       0x08000, 0x10000, 0x6e8b6a67 )
	ROM_LOAD( "sq06.14",      0x18000, 0x10000, 0x38154517 )
	ROM_LOAD( "sq05.1",       0x28000, 0x10000, 0xdf326540 )

	ROM_REGION(0x1000)	/* Unknown (protection related?) */
	ROM_LOAD( "sq07.108",     0x00000, 0x1000, 0x071a598c )
ROM_END

ROM_START( sqixbl_rom )
	ROM_REGION(0x20000)	/* 64k for code */
	ROM_LOAD( "cpu.2",        0x00000, 0x08000, 0x682e28e3 )
	ROM_LOAD( "sq02.96",      0x10000, 0x10000, 0x9c23cb64 )

	ROM_REGION_DISPOSE(0x38000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sq04.2",       0x00000, 0x08000, 0xf815ef45 )
	ROM_LOAD( "sq03.3",       0x08000, 0x10000, 0x6e8b6a67 )
	ROM_LOAD( "sq06.14",      0x18000, 0x10000, 0x38154517 )
	ROM_LOAD( "sq05.1",       0x28000, 0x10000, 0xdf326540 )
ROM_END

static int superqix_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0xf4c0],"\x00\x00\x32",3) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0xf4c0],40);
			osd_fclose(f);

			/* copy the high score to the work RAM as well */
                        RAM[0xf8f4] = RAM[0xf4c0];
                        RAM[0xf8f3] = RAM[0xf4c1];
                        RAM[0xf8f2] = RAM[0xf4c2];
                        RAM[0xf8f1] = RAM[0xf4c3];

		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void superqix_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0xf4c0],40);
		osd_fclose(f);
	}
}


struct GameDriver superqix_driver =
{
	__FILE__,
	0,
	"superqix",
	"Super Qix",
	"1987",
	"Taito",
	"Mirko Buffoni\nNicola Salmoria",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	superqix_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver sqixbl_driver =
{
	__FILE__,
	&superqix_driver,
	"sqixbl",
	"Super Qix (bootleg)",
	"1987",
	"bootleg",
	"Mirko Buffoni\nNicola Salmoria",
	0,
	&machine_driver,
	0,

	sqixbl_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	superqix_hiload, superqix_hisave
};
