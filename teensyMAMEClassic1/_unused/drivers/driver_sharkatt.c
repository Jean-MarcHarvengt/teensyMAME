/***************************************************************************

 Shark Attack driver

 TODO:
  - Finish 8255 emulation and separate into new source file.
  - Add support for audio cassette for diver sounds (port C of 8255)
  - Verify that nothing needs to be done for the TMS9927 VTAC.
  - Locate the "cocktail" set of ROMs.
  - Figure out what the write to port 0 is used for.

 ***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern void sharkatt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern void sharkatt_videoram_w(int offset,int data);
extern void sharkatt_color_map_w(int offset, int data);
extern void sharkatt_color_plane_w(int offset, int data);
extern void sharkatt_vtcsel_w(int offset, int data);

static int PA_8255 = 0;
static int PB_8255 = 0;
static int PC_8255 = 0;

static int sharkatt_8255_r(int offset)
{
	switch (offset & 0x03)
	{
		case 1:
			/* SWITCH BANK 1 - B6 */
			if (PA_8255 & 0x01)
				return input_port_0_r(offset);
			/* SWITCH BANK 0 - B5 */
			if (PA_8255 & 0x02)
				return input_port_1_r(offset);
			/* PFPCONN - B8 */
			if (PA_8255 & 0x04)
				return input_port_2_r(offset);
			/* PFDCONN - B7 */
			if (PA_8255 & 0x08)
				return input_port_3_r(offset);

			if (errorlog)
				fprintf(errorlog,"8255: read from port B, PA = %X\n",PA_8255);
			break;
		default:
			if (errorlog)
				fprintf(errorlog,"8255: read from port<>B, offset %d\n",offset);
			break;
	}

	return 0;
}

static void sharkatt_8255_w(int offset, int data)
{
	switch (offset & 0x03)
	{
		case 0:		PA_8255 = data;	break;
		case 1:		PB_8255 = data;	break;
		case 2:		PC_8255 = data;	break;
		case 3:		if (errorlog) fprintf(errorlog,"8255 Control = %02X\n",data);
	}
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },	/* ROMSEL - ROMs at 6800-7fff are unpopulated */
	{ 0x8000, 0x9fff, MRA_RAM },	/* RAMSEL */
	{ 0xc000, 0xdfff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },	/* ROMSEL - ROMs at 6800-7fff are unpopulated */
	{ 0x8000, 0x9fff, MWA_RAM },	/* RAMSEL */
	{ 0xc000, 0xdfff, sharkatt_videoram_w, &videoram, &videoram_size },
	{ -1 }  /* end of table */
};


static struct IOReadPort readport[] =
{
	{ 0x30, 0x3f, sharkatt_8255_r },
	{ 0x41, 0x41, AY8910_read_port_0_r },
	{ 0x43, 0x43, AY8910_read_port_1_r },
	{ -1 }  /* end of table */
};

/* TODO: figure out what this does!!! */
static void fake_w(int offset, int data)
{
}

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, fake_w },
	{ 0x30, 0x3f, sharkatt_8255_w },
	{ 0x40, 0x40, AY8910_control_port_0_w },
	{ 0x41, 0x41, AY8910_write_port_0_w },
	{ 0x42, 0x42, AY8910_control_port_1_w },
	{ 0x43, 0x43, AY8910_write_port_1_w },
	{ 0x50, 0x5f, sharkatt_color_plane_w },
	{ 0x60, 0x6f, sharkatt_vtcsel_w },
	{ 0x70, 0x7f, sharkatt_color_map_w },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( sharkatt_input_ports )

	PORT_START      /* IN0 */
	PORT_DIPNAME( 0x01, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(	0x01, "1 Coin/1 Credit" )
	PORT_BIT( 0xFE, IP_ACTIVE_HIGH, IPT_UNKNOWN )  // Unlabelled DIPs

	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x03, "5" )
	PORT_BIT( 0x7C, IP_ACTIVE_HIGH, IPT_UNKNOWN )  // Unlabelled DIPs
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_SERVICE | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON1, "Munch",  IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON2, "Thrust", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL, "Munch",  IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL, "Thrust", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )

INPUT_PORTS_END

static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	4000000/4,	/* Z80 Clock / 4 */
	{ 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static int sharkatt_interrupt(void)
{
	/* SLAM switch causes an NMI if it's pressed */
	if ((input_port_3_r(0) & 0x10) == 0)
		return nmi_interrupt();

	return interrupt();
}

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,        /* 4 Mhz? */
			0,
			readmem,writemem,readport,writeport,
			sharkatt_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	16, 16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	sharkatt_vh_screenrefresh,

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

ROM_START( sharkatt_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "sharkatt.0",   0x0000, 0x0800, 0xc71505e9 )
	ROM_LOAD( "sharkatt.1",   0x0800, 0x0800, 0x3e3abf70 )
	ROM_LOAD( "sharkatt.2",   0x1000, 0x0800, 0x96ded944 )
	ROM_LOAD( "sharkatt.3",   0x1800, 0x0800, 0x007283ae )
	ROM_LOAD( "sharkatt.4a",  0x2000, 0x0800, 0x5cb114a7 )
	ROM_LOAD( "sharkatt.5",   0x2800, 0x0800, 0x1d88aaad )
	ROM_LOAD( "sharkatt.6",   0x3000, 0x0800, 0xc164bad4 )
	ROM_LOAD( "sharkatt.7",   0x3800, 0x0800, 0xd78c4b8b )
	ROM_LOAD( "sharkatt.8",   0x4000, 0x0800, 0x5958476a )
	ROM_LOAD( "sharkatt.9",   0x4800, 0x0800, 0x4915eb37 )
	ROM_LOAD( "sharkatt.10",  0x5000, 0x0800, 0x9d07cb68 )
	ROM_LOAD( "sharkatt.11",  0x5800, 0x0800, 0x21edc962 )
	ROM_LOAD( "sharkatt.12a", 0x6000, 0x0800, 0x5dd8785a )

ROM_END

/***************************************************************************

  Hi Score Routines

***************************************************************************/

static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x806E],"05000",5) == 0) &&
		(memcmp(&RAM[0x80BB],"PL ",3) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x806E],0x50);
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
		osd_fwrite(f,&RAM[0x806E],0x50);
		osd_fclose(f);
	}

}


struct GameDriver sharkatt_driver =
{
	__FILE__,
	0,
	"sharkatt",
	"Shark Attack",
	"1980",
	"Pacific Novelty",
	"Victor Trucco\nMike Balfour",
	0,
	&machine_driver,
	0,

	sharkatt_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	sharkatt_input_ports,

	0, 0, 0,    /* colors, palette, colortable */
	ORIENTATION_DEFAULT,

	hiload, hisave
};
