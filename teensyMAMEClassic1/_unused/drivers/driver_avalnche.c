/***************************************************************************

Atari Avalanche Driver

Memory Map:
				0000-1FFF				RAM
				2000-2FFF		R		INPUTS
				3000-3FFF		W		WATCHDOG
				4000-4FFF		W		OUTPUTS
				5000-5FFF		W		SOUND LEVEL
				6000-7FFF		R		PROGRAM ROM
				8000-DFFF				UNUSED
				E000-FFFF				PROGRAM ROM (Remapped)

If you have any questions about how this driver works, don't hesitate to
ask.  - Mike Balfour (mab22@po.cwru.edu)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* machine/avalnche.c */
extern int avalnche_input_r(int offset);
extern void avalnche_output_w(int offset, int data);
extern void avalnche_noise_amplitude_w(int offset, int data);
extern int avalnche_interrupt(void);

/* vidhrdw/avalnche.c */
extern void avalnche_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static struct MemoryReadAddress readmem[] =
{
		{ 0x0000, 0x1fff, MRA_RAM }, /* RAM SEL */
		{ 0x2000, 0x2fff, avalnche_input_r }, /* INSEL */
		{ 0x6000, 0x7fff, MRA_ROM }, /* ROM1-ROM2 */
		{ 0xe000, 0xffff, MRA_ROM }, /* ROM2 for 6502 vectors */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
		{ 0x0000, 0x1fff, videoram_w, &videoram, &videoram_size }, /* DISPLAY */
		{ 0x3000, 0x3fff, MWA_NOP }, /* WATCHDOG */
		{ 0x4000, 0x4fff, avalnche_output_w }, /* OUTSEL */
		{ 0x5000, 0x5fff, avalnche_noise_amplitude_w }, /* SOUNDLVL */
		{ 0x6000, 0x7fff, MWA_ROM }, /* ROM1-ROM2 */
		{ 0xe000, 0xffff, MWA_ROM }, /* ROM1-ROM2 */
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( avalnche_input_ports )
		PORT_START		/* IN0 */
		PORT_BIT (0x03, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Spare */
		PORT_DIPNAME( 0x0C, 0x04, "Game Cost", IP_KEY_NONE )
		PORT_DIPSETTING(	0x0C, "Two Plays Per Coin" )
		PORT_DIPSETTING(	0x08, "Two Coins Per Play" )
		PORT_DIPSETTING(	0x04, "One Coin Per Player" )
		PORT_DIPSETTING(	0x00, "Free Play" )
		PORT_DIPNAME( 0x30, 0x00, "Game Language", IP_KEY_NONE )
		PORT_DIPSETTING(	0x30, "German" )
		PORT_DIPSETTING(	0x20, "French" )
		PORT_DIPSETTING(	0x10, "Spanish" )
		PORT_DIPSETTING(	0x00, "English" )
		PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_START2 )
		PORT_BIT (0x80, IP_ACTIVE_HIGH, IPT_START1 )

		PORT_START				/* IN1 */
		PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
		PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
		PORT_DIPNAME( 0x04, 0x04, "Extended Play", IP_KEY_NONE )
		PORT_DIPSETTING(	0x04, "Enabled" )
		PORT_DIPSETTING(	0x00, "Disabled" )
		PORT_DIPNAME( 0x08, 0x00, "Misses/Extended Play", IP_KEY_NONE )
		PORT_DIPSETTING(	0x08, "5/750 points" )
		PORT_DIPSETTING(	0x00, "3/450 points" )
		PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* SLAM */
		PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
		PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )	/* Serve */
		PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )	/* VBLANK */

		PORT_START				/* IN2 */
		PORT_ANALOG ( 0xff, 0x00, IPT_PADDLE, 100, 7, 0, 255 )

INPUT_PORTS_END


static unsigned char palette[] =
{
	0x00,0x00,0x00, /* BLACK */
	0xff,0xff,0xff, /* WHITE */
	0x80,0x80,0x80, /* LT GREY */
	0x55,0x55,0x55, /* DK GREY */
};

static unsigned short colortable[] =
{
	0x00, 0x01,
	0x01, 0x00
};

static struct GfxLayout backlayout =
{
	8,1,	/* 8*1 characters */
	0x2000,	  /* 32*256 characters */
	1,	/* 1 bits per pixel */
	{ 0 }, /* No info needed for bit offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0 },
	1*8 /* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x0000, &backlayout,	0, 2 }, 	/* dynamic bitmapped display */
	{ -1 } /* end of array */
};

static struct DACinterface dac_interface =
{
	2,
	{ 100, 100 },
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			12096000/16, 	   /* clock input is the "2H" signal divided by two */
			0,
			readmem,writemem,0,0,
			avalnche_interrupt,32	/* interrupt at a 4V frequency for sound */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	avalnche_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}

};


static void avalnche_rom_init(void)
{
	int i;

	/* Merge nibble-wide roms together,
	   and load them into 0x6000-0x7fff and e000-ffff */

	for(i=0;i<0x2000;i++)
	{
		ROM[0x6000+i] = (ROM[0x8000+i]<<4)+ROM[0xA000+i];
		ROM[0xE000+i] = (ROM[0x8000+i]<<4)+ROM[0xA000+i];
	}
}


/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( avalnche_rom )
	ROM_REGION(0x10000) /* 64k for code */
	/* Note: These are being loaded into a bogus location, */
	/*		 They are nibble wide rom images which will be */
	/*		 merged and loaded into the proper place by    */
	/*		 orbit_rom_init()							   */
	ROM_LOAD( "30612.d2",     	0x8800, 0x0800, 0x3f975171 )
	ROM_LOAD( "30613.e2",     	0x9000, 0x0800, 0x47a224d3 )
	ROM_LOAD( "30611.c2",     	0x9800, 0x0800, 0x0ad07f85 )

	ROM_LOAD( "30615.d3",     	0xa800, 0x0800, 0x3e1a86b4 )
	ROM_LOAD( "30616.e3",     	0xb000, 0x0800, 0xf620f0f8 )
	ROM_LOAD( "30614.c3",     	0xb800, 0x0800, 0xa12d5d64 )
ROM_END

/***************************************************************************

  Hi Score Routines

***************************************************************************/

static int hiload(void)
{
	static int firsttime = 0;

	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	if (firsttime == 0)
	{
		memset(&RAM[0x009b],0xff,2);
		firsttime = 1;
	}


	/* check for a value written to RAM shortly after hi scores are initialized */
	if (memcmp(&RAM[0x009b],"\x00\x00",2) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x009B],0x2);
			osd_fclose(f);
		}
		firsttime = 0;
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
		osd_fwrite(f,&RAM[0x009B],0x2);
		osd_fclose(f);
	}

}


/***************************************************************************

  Game driver(s)

***************************************************************************/

struct GameDriver avalnche_driver =
{
	__FILE__,
	0,
	"avalnche",
	"Avalanche",
	"1978",
	"Atari",
	"Mike Balfour",
	0,
	&machine_driver,
	0,

	avalnche_rom,
	avalnche_rom_init, 0,
	0,
	0,	/* sound_prom */

	avalnche_input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,
	hiload,hisave
};
