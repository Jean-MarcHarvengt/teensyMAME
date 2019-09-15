/****************************************************************************/
/*                                                                          */
/* 8080bw.c                                                                 */
/*                                                                          */
/*  Michael Strutts, Nicola Salmoria, Tormod Tjaberg, Mirko Buffoni         */
/*  Lee Taylor, Valerio Verrando, Marco Cassili and others                  */
/*                                                                          */
/*                                                                          */
/* Exidy                                                                    */
/* -----                                                                    */
/* Bandido                               (bandido)                          */
/*                                                                          */
/* Midway                                                                   */
/* ------                                                                   */
/* 596 - Sea Wolf                        (seawolf)                          */
/* 597 - Gunfight                        (gunfight)                         */
/* 605 - Tornado Baseball                (tornbase)                         */
/* 610 - 280-ZZZAP                       (zzzap)                            */
/* 611 - Amazing Maze                    (maze)                             */
/* 612 - Boot Hill                       (boothill)                         */
/* 615 - Checkmate                       (checkmat)                         */
/* 618 - Desert Gun                      NO DRIVER                          */
/* 619 - Double Play                     (dplay)                            */
/* 622 - Laguna Racer                    (lracer)                           */
/* 623 - Guided Missile                  (gmissile)                         */
/* 626 - M-4                             (m4)                               */
/* 630 - Clowns                          (clowns)                           */
/* 640 - Space Walk                      NO DRIVER                          */
/* 642 - Extra Innings                   (einnings)                         */
/* 643 - Shuffleboard                    NO DRIVER                          */
/* 644 - Dog Patch                       (dogpatch)                         */
/* 645 - Space Encounters                (spcenctr)                         */
/* 652 - Phantom II                      (phantom2)                         */
/* 730 - 4 Player Bowling                (midwbowl)                         */
/* 739 - Space Invaders                  (invaders)                         */
/* 742 - Blue Shark                      (blueshrk)                         */
/* 851 - Space Invaders Part II (Midway) (invdelux)                         */
/* 852 - Space Invaders Deluxe           NO DRIVER                          */
/*                                                                          */
/* Taito                                                                    */
/* -----                                                                    */
/* Space Invaders Part II (Taito)        (invadpt2)                         */
/* Space Laser  "GPI Intruder"           (intruder)                         */
/* Galaxy Wars                           (galxwars)                         */
/* Lunar Rescue                          (lrescue)                          */
/* Galaxy Rescue (bootleg?)				 (grescue)							*/
/* Lupin III                             (lupin3)                           */
/* Ozma Wars                             (ozmawars)                         */
/* Space Chaser                          (schaser)                          */
/*                                                                          */
/* Nichibutsu                                                               */
/* ----------                                                               */
/* Rolling Crash - Moon Base             (rollingc)                         */
/*                                                                          */
/* Nintendo                                                                 */
/* --------                                                                 */
/* Heli Fire                             (helifire)                         */
/* Space Fever  (color)                  (sfever)                           */
/* Space Fever  (B&W)                    (sfeverbw)                         */
/*                                                                          */
/* Universal                                                                */
/* ---------                                                                */
/* Cosmic Monsters                       (cosmicmo)                         */
/*                                                                          */
/* Zeltec                                                                   */
/* ------                                                                   */
/* Space Attack II                       (spaceatt)                         */
/* Invaders Revenge                      (invrvnge)                         */
/*                                                                          */
/* Super Earth Invasion                  (earthinv)                         */
/* Destination Earth                     (desterth)                         */
/* Space Phantoms                        (spaceph)                          */
/*                                                                          */
/*                                                                          */
/* Notes:                                                                   */
/* 	Space Chaser writes to $c400-$df9f. Color?                              */
/*                                                                          */
/*                                                                          */
/* Known problems:                                                          */
/* --------------                                                           */
/* 	Space Encounter, Sea Wolf need their movement controls fixed.           */
/* 	The accelerator in 280 Zzzap could be handled better.                   */
/*                                                                          */
/* List of Changes                                                          */
/* ---------------                                                          */
/* 	MJC - 22.01.98 - Boot hill controls/memory mappings                     */
/* 	LBO - 5 May 98 - Cleaned up the code, added Al's extra drivers,         */
/*                   rotation support                                       */
/*                                                                          */
/****************************************************************************/

#include <math.h>
#include "driver.h"
#include "vidhrdw/generic.h"

/* in machine/8080bw.c */

int  invaders_shift_data_r(int offset);
int  midbowl_shift_data_r(int offset);
int  midbowl_shift_data_rev_r(int offset);
int  blueshrk_shift_data_r(int offset);
int  blueshrk_shift_data_rev_r(int offset);
void invaders_shift_amount_w(int offset,int data);
void invaders_shift_data_w(int offset,int data);
int  invaders_interrupt(void);

int  boothill_shift_data_r(int offset);                  /* MJC 310198 */
int  boothill_port_0_r(int offset);                      /* MJC 310198 */
int  boothill_port_1_r(int offset);                      /* MJC 310198 */
void boothill_sh_port3_w(int offset, int data);          /* HSC 4/14/98 */
void boothill_sh_port5_w(int offset, int data);          /* HSC 4/14/98 */

int  gray6bit_controller0_r(int offset);
int  gray6bit_controller1_r(int offset);
int  seawolf_shift_data_r(int offset);
int  seawolf_port_0_r(int offset);

/* in video/8080bw.c */

extern unsigned char *invaders_videoram;

void boothill_videoram_w(int offset,int data);   /* MAB */
void invaders_videoram_w(int offset,int data);
void invdelux_videoram_w(int offset,int data);   /* V.V */
void invrvnge_videoram_w(int offset,int data);   /* V.V */
void lrescue_videoram_w(int offset,int data);    /* V.V */
void rollingc_videoram_w(int offset,int data);   /* L.T */
void astinvad_videoram_w(int offset,int data);   /* HSC used for astinv (z80bw.c)*/

int  invaders_vh_start(void);
void invaders_vh_stop(void);
void invaders_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void invaders_sh_port3_w(int offset,int data);
void invaders_sh_port4_w(int offset,int data);
void invadpt2_sh_port3_w(int offset,int data);
void invaders_sh_port5_w(int offset,int data);
void invaders_sh_update(void);
void seawolf_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void blueshrk_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static unsigned char palette[] = /* V.V */ /* Smoothed pure colors, overlays are not so contrasted */
{
	0x00,0x00,0x00, /* BLACK */
	0xff,0x20,0x20, /* RED */
	0x20,0xff,0x20, /* GREEN */
	0xff,0xff,0x20, /* YELLOW */
	0xff,0xff,0xff, /* WHITE */
	0x20,0xff,0xff, /* CYAN */
	0xff,0x20,0xff  /* PURPLE */
};

enum { BLACK,RED,GREEN,YELLOW,WHITE,CYAN,PURPLE }; /* V.V */

/****************************************************************************/
/*  mix_hiscoreprint: special thanks to: Mirko , John B., Frank, and Mike B */
/*  x 		= x position 													*/
/*	y 		= y Position													*/
/*	value 	= value to display												*/
/*	width 	= number of characters											*/
/*	size 	= size of a character in the character rom						*/
/*	adjust 	= number of rows to adjust the display (line characters up)		*/
/*	romarea	= where into memory the character rom is loaded					*/
/*	offset 	= where in the character rom the 1st number starts				*/
/*	type	= to make this as flexible as possible i've added code for all 	*/
/*				the types of character drawing the current drivers need		*/
/*				to add a new one just add a new case to the case statement	*/
/*				case 0 : plain horizontal 									*/
/*				case 1 : horizontal using colors from lrescue				*/
/*				case 2 : plain vertical										*/
/*				case 3 : inverted vertical									*/
/* 				case 4 : astinvad horizontal (z80bw.c)						*/

/* HSC 11/16/98 */
void mix_hiscoreprint(int x, int y, int value, int width,int size, int adjust,int romarea, int offset, int type)
{
	int i,j;

	j = x+width-1;
	while (j >= x)
	{
		int disp = (value % 10) * size;
		for (i=0; i < size; i++)
	switch (type)
		{
		case 0:
			invaders_videoram_w((31-y) + (i+adjust)*32 + j*256,
			Machine->memory_region[0][romarea+offset+disp+i]);
		break;

		case 1:
			lrescue_videoram_w((31-y) + (i+adjust)*32 + j*256,
			Machine->memory_region[0][romarea+offset+disp+i]);
		break;

		case 2:
			boothill_videoram_w((31-j) + (i+adjust)*32 + y*256,
			Machine->memory_region[0][romarea+offset+disp+i]);
		break;

		case 3:
			boothill_videoram_w((31-j) + (i+adjust)*32 + y*256,
			~(Machine->memory_region[0][romarea+offset+disp+i]));
		break;
		case 4:
			astinvad_videoram_w((31-y) + (i+adjust)*32 + j*256,
			Machine->memory_region[0][romarea+offset+disp+i]);
		break;
		}

		value /= 10;
		j--;
	}
}



/*******************************************************/
/*                                                     */
/* Midway "Space Invaders"                             */
/*                                                     */
/*******************************************************/

ROM_START( invaders_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "invaders.h",   0x0000, 0x0800, 0x734f5ad8 )
	ROM_LOAD( "invaders.g",   0x0800, 0x0800, 0x6bfaca4a )
	ROM_LOAD( "invaders.f",   0x1000, 0x0800, 0x0ccead96 )
	ROM_LOAD( "invaders.e",   0x1800, 0x0800, 0x14e538b0 )
ROM_END

/* invaders, earthinv, spaceatt, invrvnge, invdelux, invadpt2, galxwars, lrescue, */
/* desterth, cosmicmo, spaceph */
static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x57ff, MRA_ROM },
	{ -1 }  /* end of table */
};

/* invaders, earthinv, spaceatt, invdelux, galxwars, desterth, cosmicmo */
static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, invaders_videoram_w, &invaders_videoram },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x01, 0x01, input_port_0_r },
	{ 0x02, 0x02, input_port_1_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};


/* Catch the write to unmapped I/O port 6 */
void invaders_dummy_write(int offset,int data)
{
}

static struct IOWritePort writeport[] =
{
	{ 0x02, 0x02, invaders_shift_amount_w },
	{ 0x03, 0x03, invaders_sh_port3_w },
	{ 0x04, 0x04, invaders_shift_data_w },
	{ 0x05, 0x05, invaders_sh_port5_w },
	{ 0x06, 0x06, invaders_dummy_write },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( invaders_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x80, 0x00, "Coin Info", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_START		/* BSR */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
INPUT_PORTS_END

static struct Samplesinterface samples_interface =
{
	9       /* 9 channels */
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,writemem,readport,writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static int invaders_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */

	if (memcmp(&RAM[0x2008],"\x02\x78\x38",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x20f4],2);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void invaders_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x20f4],2);
		osd_fclose(f);
		RAM[0x2008] = 0;
	}
}

static const char *invaders_sample_names[] =
{
	"*invaders",
	"0.SAM",
	"1.SAM",
	"2.SAM",
	"3.SAM",
	"4.SAM",
	"5.SAM",
	"6.SAM",
	"7.SAM",
	"8.SAM",
	0       /* end of array */
};

struct GameDriver invaders_driver =
{
	__FILE__,
	0,
	"invaders",
	"Space Invaders",
	"1978",
	"Midway",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	0,
	&machine_driver,
	0,

	invaders_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	invaders_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	invaders_hiload, invaders_hisave
};

/*******************************************************/
/*                                                     */
/* Midway "Space Invaders Part II"                     */
/*                                                     */
/*******************************************************/

ROM_START( invaders2_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "pv.01",        0x0000, 0x0800, 0x7288a511 )
	ROM_LOAD( "pv.02",        0x0800, 0x0800, 0x097dd8d5 )
	ROM_LOAD( "pv.03",        0x1000, 0x0800, 0x1766337e )
	ROM_LOAD( "pv.04",        0x1800, 0x0800, 0x8f0e62e0 )
	ROM_LOAD( "pv.05",        0x4000, 0x0800, 0x19b505e9 )
ROM_END

static struct IOReadPort invadpt2_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};

/* LT 20-3-1998 */
static struct IOWritePort invadpt2_writeport[] =
{
	{ 0x02, 0x02, invaders_shift_amount_w },
	{ 0x03, 0x03, invadpt2_sh_port3_w },
	{ 0x04, 0x04, invaders_shift_data_w },
	{ 0x05, 0x05, invaders_sh_port5_w },
	{ 0x06, 0x06, invaders_dummy_write },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress invadpt2_writemem[] =
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, lrescue_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( invadpt2_input_ports )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* N ? */
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Preset Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x80, 0x00, "Coin Info", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_START		/* BSR */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(	0x01, "Cocktail" )
INPUT_PORTS_END

static struct MachineDriver invadpt2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, invadpt2_writemem, invadpt2_readport, invadpt2_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY|VIDEO_MODIFIES_PALETTE,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static int invadpt2_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2340],"\x1b\x1b",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			/* Load the actual score */
			osd_fread(f,&RAM[0x20f4], 0x2);
			/* Load the name */
			osd_fread(f,&RAM[0x2340], 0xa);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void invadpt2_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* Save the actual score */
		osd_fwrite(f,&RAM[0x20f4], 0x2);
		/* Save the name */
		osd_fwrite(f,&RAM[0x2340], 0xa);
		osd_fclose(f);
		RAM[0x2340] = 0;
	}
}

/* LT 24-11-1997 */
/* LT 20-3-1998 UPDATED */
struct GameDriver invadpt2_driver =
{
	__FILE__,
	0,
	"invadpt2",
	"Space Invaders Part II (Taito)",
	"1980",
	"Taito",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nLee Taylor\nMarco Cassili",
	0,
	&invadpt2_machine_driver,
	0,

	invaders2_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	invadpt2_input_ports,

	0,palette, 0,
	ORIENTATION_ROTATE_270,

	invadpt2_hiload, invadpt2_hisave
};

/*******************************************************/
/*                                                     */
/* ?????? "Super Earth Invasion"                       */
/*                                                     */
/*******************************************************/

ROM_START( earthinv_rom )
	ROM_REGION(0x10000)             /* 64k for code */
	ROM_LOAD( "earthinv.h",   0x0000, 0x0800, 0x58a750c8 )
	ROM_LOAD( "earthinv.g",   0x0800, 0x0800, 0xb91742f1 )
	ROM_LOAD( "earthinv.f",   0x1000, 0x0800, 0x4acbbc60 )
	ROM_LOAD( "earthinv.e",   0x1800, 0x0800, 0xdf397b12 )
ROM_END

INPUT_PORTS_START( earthinv_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown DSW 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Unknown DSW 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/1 Credit" )
INPUT_PORTS_END


struct GameDriver earthinv_driver =
{
	__FILE__,
	&invaders_driver,
	"earthinv",
	"Super Earth Invasion",
	"1980",
	"bootleg",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nMarco Cassili",
	0,
	&machine_driver,
	0,

	earthinv_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	earthinv_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,
	invaders_hiload, invaders_hisave

};

/*******************************************************/
/*                                                     */
/* ?????? "Space Attack II"                            */
/*                                                     */
/*******************************************************/

ROM_START( spaceatt_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "spaceatt.h",   0x0000, 0x0800, 0xa31d0756 )
	ROM_LOAD( "spaceatt.g",   0x0800, 0x0800, 0xf41241f7 )
	ROM_LOAD( "spaceatt.f",   0x1000, 0x0800, 0x4c060223 )
	ROM_LOAD( "spaceatt.e",   0x1800, 0x0800, 0x7cf6f604 )
ROM_END

INPUT_PORTS_START( spaceatt_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
INPUT_PORTS_END

struct GameDriver spaceatt_driver =
{
	__FILE__,
	&invaders_driver,
	"spaceatt",
	"Space Attack II",
	"1980",
	"Zenitone Microsec",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	0,
	&machine_driver,
	0,

	spaceatt_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	spaceatt_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	invaders_hiload, invaders_hisave
};

/*******************************************************/
/*                                                     */
/* Zenitone Microsec "Invaders Revenge"                */
/*                                                     */
/*******************************************************/

ROM_START( invrvnge_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "invrvnge.h",   0x0000, 0x0800, 0xaca41bbb )
	ROM_LOAD( "invrvnge.g",   0x0800, 0x0800, 0xcfe89dad )
	ROM_LOAD( "invrvnge.f",   0x1000, 0x0800, 0xe350de2c )
	ROM_LOAD( "invrvnge.e",   0x1800, 0x0800, 0x1ec8dfc8 )
ROM_END

ROM_START( invrvnga_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "5m.bin",   0x0000, 0x0800, 0xb145cb71 )
	ROM_LOAD( "5n.bin",   0x0800, 0x0800, 0x660e8af3 )
	ROM_LOAD( "5p.bin",   0x1000, 0x0800, 0x6ec5a9ad )
	ROM_LOAD( "5r.bin",   0x1800, 0x0800, 0x74516811 )
ROM_END


static struct MemoryWriteAddress invrvnge_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, invrvnge_videoram_w, &invaders_videoram },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( invrvnge_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
INPUT_PORTS_END

static struct MachineDriver invrvnge_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, invrvnge_writemem, readport, writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static int invrvnge_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2003],"\xce\x00",2) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x2019],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}


static void invrvnge_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x2019],3);
		osd_fclose(f);
	}
}

struct GameDriver invrvnge_driver =
{
	__FILE__,
	0,
	"invrvnge",
	"Invader's Revenge",
	"????",
	"Zenitone Microsec",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni",
	0,
	&invrvnge_machine_driver,
	0,

	invrvnge_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	invrvnge_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	invrvnge_hiload, invrvnge_hisave
};

struct GameDriver invrvnga_driver =
{
	__FILE__,
	&invrvnge_driver,
	"invrvnga",
	"Invader's Revenge (Dutchford)",
	"????",
	"Zenitone Microsec (Dutchford license)",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni",
	0,
	&invrvnge_machine_driver,
	0,

	invrvnga_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	invrvnge_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	invrvnge_hiload, invrvnge_hisave
};


/*******************************************************/
/*                                                     */
/* Midway "Space Invaders Part II"                     */
/*                                                     */
/*******************************************************/

ROM_START( invdelux_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "invdelux.h",   0x0000, 0x0800, 0xe690818f )
	ROM_LOAD( "invdelux.g",   0x0800, 0x0800, 0x4268c12d )
	ROM_LOAD( "invdelux.f",   0x1000, 0x0800, 0xf4aa1880 )
	ROM_LOAD( "invdelux.e",   0x1800, 0x0800, 0x408849c1 )
	ROM_LOAD( "invdelux.d",   0x4000, 0x0800, 0xe8d5afcd )
ROM_END

static struct IOReadPort invdelux_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress invdelux_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, invdelux_videoram_w, &invaders_videoram },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( invdelux_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* N ? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Preset Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x80, 0x00, "Coin Info", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_START		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(	0x01, "Cocktail" )
INPUT_PORTS_END


static struct MachineDriver invdelux_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, invdelux_writemem, invdelux_readport, writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static int invdelux_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2340],"\x1b\x1b",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			/* Load the actual score */
			osd_fread(f,&RAM[0x20f4], 0x2);
			/* Load the name */
			osd_fread(f,&RAM[0x2340], 0xa);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void invdelux_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* Save the actual score */
		osd_fwrite(f,&RAM[0x20f4], 0x2);
		/* Save the name */
		osd_fwrite(f,&RAM[0x2340], 0xa);
		osd_fclose(f);
		RAM[0x2340] = 0;
	}
}

struct GameDriver invdelux_driver =
{
	__FILE__,
	&invadpt2_driver,
	"invdelux",
	"Space Invaders Part II (Midway)",
	"1980",
	"Midway",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	0,
	&invdelux_machine_driver,
	0,

	invdelux_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	invdelux_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	invdelux_hiload, invdelux_hisave
};


/*******************************************************/
/*                                                     */
/* ?????? "Astro Laser"                                */
/*                                                     */
/*******************************************************/

ROM_START( astlaser_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "1.u36",        0x0000, 0x0800, 0xb44e2c41 )
	ROM_LOAD( "2.u35",        0x0800, 0x0800, 0x9876f331 )
	ROM_LOAD( "3.u34",        0x1000, 0x0800, 0xed79000b )
	ROM_LOAD( "4.u33",        0x1800, 0x0800, 0x10a160a1 )
ROM_END

INPUT_PORTS_START( astlaser_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* N ? */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Preset Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x80, 0x00, "Coin Info", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_START		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(	0x01, "Cocktail" )
INPUT_PORTS_END


static int astlaser_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


			/* check if the hi score table has already been initialized */
	 if (memcmp(&RAM[0x2f7c],"\x3e\x3e",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			/* Load the actual score */
			int hi;
			osd_fread(f,&RAM[0x2034], 0x2);
			osd_fclose(f);
			hi = (RAM[0x2034] & 0x0f) +
				 (RAM[0x2034] >> 4) * 10 +
				 (RAM[0x2035] & 0x0f) * 100 +
				 (RAM[0x2035] >> 4) * 1000;
			mix_hiscoreprint(10, 1, hi, 4, 5,  3, 0x1000, 0x12d,  0);

		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void astlaser_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* Save the actual score */
		osd_fwrite(f,&RAM[0x2034], 0x2);
		osd_fclose(f);
		RAM[0x2f7c] = 0;
	}
}



struct GameDriver astlaser_driver =
{
	__FILE__,
	0,
	"astlaser",
	"Astro Laser",
	"1980",
	"?????",
	"The Space Invaders Team",
	0,
	&invdelux_machine_driver,
	0,

	astlaser_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	astlaser_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	astlaser_hiload, astlaser_hisave
};

/*******************************************************/
/*                                                     */
/* Game Plan "Intruder"                                */
/*                                                     */
/*******************************************************/

ROM_START( intruder_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "la01",         0x0000, 0x0800, 0xbedc0078 )
	ROM_LOAD( "la02",         0x0800, 0x0800, 0x43bc65c5 )
	ROM_LOAD( "la03",         0x1000, 0x0800, 0x1083e9cc )
	ROM_LOAD( "la04",         0x1800, 0x0800, 0x5116b234 )
ROM_END

INPUT_PORTS_START( intruder_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* N ? */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Preset Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x80, 0x00, "Coin Info", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_START		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(	0x01, "Cocktail" )
INPUT_PORTS_END

struct GameDriver intruder_driver =
{
	__FILE__,
	&astlaser_driver,
	"intruder",
	"Intruder",
	"1980",
	"Game Plan, Inc. (Taito)",
	"The Space Invaders Team",
	0,
	&invdelux_machine_driver,
	0,

	intruder_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	intruder_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	astlaser_hiload, astlaser_hisave
};

/*******************************************************/
/*                                                     */
/* Taito "Galaxy Wars"                                 */
/*                                                     */
/*******************************************************/

ROM_START( galxwars_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "galxwars.0",   0x0000, 0x0400, 0x608bfe7f )
	ROM_LOAD( "galxwars.1",   0x0400, 0x0400, 0xa810b258 )
	ROM_LOAD( "galxwars.2",   0x0800, 0x0400, 0x74f31781 )
	ROM_LOAD( "galxwars.3",   0x0c00, 0x0400, 0xc88f886c )
	ROM_LOAD( "galxwars.4",   0x4000, 0x0400, 0xae4fe8fb )
	ROM_LOAD( "galxwars.5",   0x4400, 0x0400, 0x37708a35 )
ROM_END

INPUT_PORTS_START( galxwars_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x08, "5000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
INPUT_PORTS_END


static int galxwars_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2000],"\x07\x00",2) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int hi;
			char length[7];
			int digits;
			osd_fread(f,&RAM[0x2005],6);
			osd_fclose(f);

				hi = (RAM[0x2005] & 0x0f) * 100000 +
				 	 (RAM[0x2006] & 0x0f) * 10000 +
				 	 (RAM[0x2007] & 0x0f) * 1000 +
				 	 (RAM[0x2008] & 0x0f) * 100 +
					 (RAM[0x2009] & 0x0f) * 10 +
					 (RAM[0x200a] & 0x0f) * 1;


				sprintf(length,"%i",hi);
				digits = strlen(length);
				mix_hiscoreprint(11+(6-digits), 2, hi, digits, 5, 1,0x0000, 0x2f0, 0);

		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void galxwars_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x2005],6);
		osd_fclose(f);
		RAM[0x2000] = 0;
	}
}

struct GameDriver galxwars_driver =
{
	__FILE__,
	0,
	"galxwars",
	"Galaxy Wars",
	"1979",
	"Taito",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	0,
	&machine_driver,
	0,

	galxwars_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	galxwars_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,
	galxwars_hiload, galxwars_hisave
};

/*******************************************************/
/*                                                     */
/* Taito "Lunar Rescue"                                */
/*                                                     */
/*******************************************************/

ROM_START( lrescue_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "lrescue.1",    0x0000, 0x0800, 0x2bbc4778 )
	ROM_LOAD( "lrescue.2",    0x0800, 0x0800, 0x49e79706 )
	ROM_LOAD( "lrescue.3",    0x1000, 0x0800, 0x1ac969be )
	ROM_LOAD( "lrescue.4",    0x1800, 0x0800, 0x782fee3c )
	ROM_LOAD( "lrescue.5",    0x4000, 0x0800, 0x58fde8bc )
	ROM_LOAD( "lrescue.6",    0x4800, 0x0800, 0xbfb0f65d )
ROM_END

ROM_START( grescue_rom )
	ROM_REGION(0x10000)     /* 64k for code */
        ROM_LOAD( "lrescue.1",    0x0000, 0x0800, 0x2bbc4778 )
        ROM_LOAD( "lrescue.2",    0x0800, 0x0800, 0x49e79706 )
        ROM_LOAD( "lrescue.3",    0x1000, 0x0800, 0x1ac969be )
        ROM_LOAD( "grescue.4",    0x1800, 0x0800, 0xca412991 )
        ROM_LOAD( "grescue.5",    0x4000, 0x0800, 0xa419a4d6 )
        ROM_LOAD( "lrescue.6",    0x4800, 0x0800, 0xbfb0f65d )
ROM_END


/* lrescue, invadpt2, spaceph */
static struct MemoryWriteAddress lrescue_writemem[] =
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, lrescue_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( lrescue_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
INPUT_PORTS_END

static struct MachineDriver lrescue_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, lrescue_writemem, readport, writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static int lrescue_hiload(void)     /* V.V */ /* Whole function */
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x20CF],"\x1b\x1b",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			/* Load the actual score */
			osd_fread(f,&RAM[0x20F4], 0x2);
			/* Load the name */
			osd_fread(f,&RAM[0x20CF], 0xa);
			/* Load the high score length */
			osd_fread(f,&RAM[0x20DB], 0x1);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */

}

static void lrescue_hisave(void)    /* V.V */ /* Whole function */
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		/* Save the actual score */
		osd_fwrite(f,&RAM[0x20F4],0x02);
		/* Save the name */
		osd_fwrite(f,&RAM[0x20CF],0xa);
		/* Save the high score length */
		osd_fwrite(f,&RAM[0x20DB],0x1);
		osd_fclose(f);
		RAM[0x20CF] = 0;
	}
}

struct GameDriver lrescue_driver =
{
	__FILE__,
	0,
	"lrescue",
	"Lunar Rescue",
	"1979",
	"Taito",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	0,
	&lrescue_machine_driver,
	0,

	lrescue_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	lrescue_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	lrescue_hiload, lrescue_hisave  /* V.V */
};

struct GameDriver grescue_driver =
{
	__FILE__,
	&lrescue_driver,
	"grescue",
	"Galaxy Rescue (bootleg?)",
	"1979",
	"bootleg?",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	0,
	&lrescue_machine_driver,
	0,

	grescue_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	lrescue_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	lrescue_hiload, lrescue_hisave  /* V.V */
};



/*******************************************************/
/*                                                     */
/* ?????? "Destination Earth"                          */
/*                                                     */
/*******************************************************/

ROM_START( desterth_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "36_h.bin",     0x0000, 0x0800, 0xf86923e5 )
	ROM_LOAD( "35_g.bin",     0x0800, 0x0800, 0x797f440d )
	ROM_LOAD( "34_f.bin",     0x1000, 0x0800, 0x993d0846 )
	ROM_LOAD( "33_e.bin",     0x1800, 0x0800, 0x8d155fc5 )
	ROM_LOAD( "32_d.bin",     0x4000, 0x0800, 0x3f531b6f )
	ROM_LOAD( "31_c.bin",     0x4800, 0x0800, 0xab019c30 )
	ROM_LOAD( "42_b.bin",     0x5000, 0x0800, 0xed9dbac6 )
ROM_END

INPUT_PORTS_START( desterth_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
INPUT_PORTS_END

static int desterth_hiload(void)     /* V.V */ /* Whole function */
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x20CF],"\x1b\x07",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			/* Load the actual score */
			osd_fread(f,&RAM[0x20F4], 0x2);
			/* Load the name */
			osd_fread(f,&RAM[0x20CF], 0xa);
			/* Load the high score length */
			osd_fread(f,&RAM[0x20DB], 0x1);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

struct GameDriver desterth_driver =
{
	__FILE__,
	&lrescue_driver,
	"desterth",
	"Destination Earth",
	"1979",
	"bootleg",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	0,
	&machine_driver,
	0,

	desterth_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	desterth_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	desterth_hiload, lrescue_hisave
};

/*******************************************************/
/*                                                     */
/* Universal "Cosmic Monsters"                         */
/*                                                     */
/*******************************************************/

ROM_START( cosmicmo_rom )  /* L.T */
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "cosmicmo.1",   0x0000, 0x0400, 0xd6e4e5da )
	ROM_LOAD( "cosmicmo.2",   0x0400, 0x0400, 0x8f7988e6 )
	ROM_LOAD( "cosmicmo.3",   0x0800, 0x0400, 0x2d2e9dc8 )
	ROM_LOAD( "cosmicmo.4",   0x0c00, 0x0400, 0x26cae456 )
	ROM_LOAD( "cosmicmo.5",   0x4000, 0x0400, 0xb13f228e )
	ROM_LOAD( "cosmicmo.6",   0x4400, 0x0400, 0x4ae1b9c4 )
	ROM_LOAD( "cosmicmo.7",   0x4800, 0x0400, 0x6a13b15b )
ROM_END

INPUT_PORTS_START( cosmicmo_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )
INPUT_PORTS_END

struct GameDriver cosmicmo_driver =
{
	__FILE__,
	&invaders_driver,
	"cosmicmo",
	"Cosmic Monsters",
	"1979",
	"Universal",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nLee Taylor\nMarco Cassili",
	0,
	&machine_driver,
	0,

	cosmicmo_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	cosmicmo_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	invaders_hiload, invaders_hisave
};


/*******************************************************/
/*                                                     */
/* Zilec "Space Phantoms"                              */
/*                                                     */
/*******************************************************/

ROM_START( spaceph_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "sv01.bin",     0x0000, 0x0400, 0xde84771d )
	ROM_LOAD( "sv02.bin",     0x0400, 0x0400, 0x957fc661 )
	ROM_LOAD( "sv03.bin",     0x0800, 0x0400, 0xdbda38b9 )
	ROM_LOAD( "sv04.bin",     0x0c00, 0x0400, 0xf51544a5 )
	ROM_LOAD( "sv05.bin",     0x1000, 0x0400, 0x98d02683 )
	ROM_LOAD( "sv06.bin",     0x1400, 0x0400, 0x4ec390fd )
	ROM_LOAD( "sv07.bin",     0x1800, 0x0400, 0x170862fd )
	ROM_LOAD( "sv08.bin",     0x1c00, 0x0400, 0x511b12cf )
	ROM_LOAD( "sv09.bin",     0x4000, 0x0400, 0xaf1cd1af )
	ROM_LOAD( "sv10.bin",     0x4400, 0x0400, 0x31b7692e )
	ROM_LOAD( "sv11.bin",     0x4800, 0x0400, 0x50257351 )
	ROM_LOAD( "sv12.bin",     0x4c00, 0x0400, 0xa2a3366a )
ROM_END

INPUT_PORTS_START( spaceph_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x03, 0x00, "Fuel", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "35000" )
	PORT_DIPSETTING(    0x02, "25000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x00, "15000" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Tilt */
	PORT_DIPNAME( 0x08, 0x00, "Bonus Fuel", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "10000" )
	PORT_DIPSETTING(    0x00, "15000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Fire */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Left */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Right */
	PORT_DIPNAME( 0x80, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
INPUT_PORTS_END

static int spaceph_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */

	if (memcmp(&RAM[0x275d],"\x45\x41",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int hi;
			osd_fread(f,&RAM[0x2043],2);
			osd_fclose(f);

			hi = (RAM[0x2043] & 0x0f) +
				 (RAM[0x2043] >> 4) * 10 +
				 (RAM[0x2044] & 0x0f) * 100 +
				 (RAM[0x2044] >> 4) * 1000;
			mix_hiscoreprint(12, 2, hi, 4, 8, 1,0x1400, 0x25c, 1);

		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void spaceph_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x2043],2);
		osd_fclose(f);
		RAM[0x275d] = 0;
	}
}



/* LT 3-12-1997 */
struct GameDriver spaceph_driver =
{
	__FILE__,
	0,
	"spaceph",
	"Space Phantoms",
	"????",
	"Zilec Games",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nLee Taylor\nPaul Swan\nMarco Cassili",
	0,
	&lrescue_machine_driver,
	0,

	spaceph_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	spaceph_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	spaceph_hiload,spaceph_hisave
};

/*******************************************************/
/*                                                     */
/* Nichibutsu "Rolling Crash"                          */
/*                                                     */
/*******************************************************/

ROM_START( rollingc_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "rc01.bin",     0x0000, 0x0400, 0x66fa50bf )
	ROM_LOAD( "rc02.bin",     0x0400, 0x0400, 0x61c06ae4 )
	ROM_LOAD( "rc03.bin",     0x0800, 0x0400, 0x77e39fa0 )
	ROM_LOAD( "rc04.bin",     0x0c00, 0x0400, 0x3fdfd0f3 )
	ROM_LOAD( "rc05.bin",     0x1000, 0x0400, 0xc26a8f5b )
	ROM_LOAD( "rc06.bin",     0x1400, 0x0400, 0x0b98dbe5 )
	ROM_LOAD( "rc07.bin",     0x1800, 0x0400, 0x6242145c )
	ROM_LOAD( "rc08.bin",     0x1c00, 0x0400, 0xd23c2ef1 )
	ROM_LOAD( "rc09.bin",     0x4000, 0x0800, 0x2e2c5b95 )
	ROM_LOAD( "rc10.bin",     0x4800, 0x0800, 0xef94c502 )
	ROM_LOAD( "rc11.bin",     0x5000, 0x0800, 0xa3164b18 )
	ROM_LOAD( "rc12.bin",     0x5800, 0x0800, 0x2052f6d9 )
ROM_END

static struct MemoryReadAddress rollingc_readmem[] =
{
//      { 0x2000, 0x2002, MRA_RAM },
//      { 0x2003, 0x2003, hack },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x5fff, MRA_ROM },
	{ 0xa400, 0xbfff, MRA_RAM },
	{ 0xe400, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress rollingc_writemem[] =
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, rollingc_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x5fff, MWA_ROM },
	{ 0xa400, 0xbfff, MWA_RAM },
	{ 0xe400, 0xffff, MWA_RAM },
	{ -1 }  /* end of table */
};
INPUT_PORTS_START( rollingc_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) /* Game Select */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) /* Game Select */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* Player 1 Controls */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY ) /* Player 1 Controls */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY ) /* Player 1 Controls */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Tilt ? */
	PORT_DIPNAME( 0x08, 0x00, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* Player 2 Controls */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) /* Player 2 Controls */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) /* Player 2 Controls */
	PORT_DIPNAME( 0x80, 0x00, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
INPUT_PORTS_END

static unsigned char rollingc_palette[] =
{
	0xff,0xff,0xff, /* WHITE */
	0xa0,0xa0,0xa0, /* ?????? */
	0x00,0xff,0x00, /* GREEN */
	0x00,0xa0,0xa0, /* GREEN */
	0xff,0x00,0xff, /* PURPLE */
	0x00,0xff,0xff, /* CYAN */
	0xa0,0x00,0xa0, /* WHITE */
	0xff,0x00,0x00, /* RED */
	0xff,0xff,0x00, /* YELLOW */
	0x80,0x40,0x20, /* ?????? */
	0x80,0x80,0x00, /* GREEN */
	0x00,0x80,0x80, /* GREEN */
	0xa0,0xa0,0xff, /* PURPLE */
	0xa0,0xff,0x80, /* CYAN */
	0xff,0xff,0x00, /* WHITE */
	0xff,0x00,0xa0, /* RED */
	0x00,0x00,0x00  /* BLACK */
};

static struct MachineDriver rollingc_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			rollingc_readmem,rollingc_writemem,invdelux_readport,writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(rollingc_palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static int rollingc_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */

	if (memcmp(&RAM[0x2b7c],"\x43\x00\x49	",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{

			osd_fread(f,&RAM[0x2302],15);
			osd_fclose(f);

					}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void rollingc_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{

	    osd_fwrite(f,&RAM[0x2302],15);
		osd_fclose(f);
		RAM[0x2b7c] = 0;
	}
}



/* LT 3-12-1997 */
struct GameDriver rollingc_driver =
{
	__FILE__,
	0,
	"rollingc",
	"Rolling Crash / Moon Base",
	"1979",
	"Nichibutsu",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nLee Taylor\nPaul Swan",    /*L.T */
	0,
	&rollingc_machine_driver,
	0,

	rollingc_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	rollingc_input_ports,

	0,rollingc_palette, 0,
	ORIENTATION_ROTATE_270,

	rollingc_hiload,rollingc_hisave
};

/*******************************************************/
/*                                                     */
/* Exidy "Bandido"                                     */
/*                                                     */
/* Nintendo game which was sold outside of the US as   */
/* "Sheriff"                                           */
/*                                                     */
/*******************************************************/

ROM_START( bandido_rom )                                                                                /* MJC */
	ROM_REGION(0x10000)             /* 64k for code */
	ROM_LOAD( "baf1-3",       0x0000, 0x0400, 0xaec94829 )
	ROM_LOAD( "baf2-1",       0x0400, 0x0400, 0xda67721a )
	ROM_LOAD( "bag1-1",       0x0800, 0x0400, 0x3fb7888e )
	ROM_LOAD( "bag2-1",       0x0C00, 0x0400, 0x585fcfee )
	ROM_LOAD( "bah1-1",       0x1000, 0x0400, 0x5cb63677 )
	ROM_LOAD( "bah2-1",       0x1400, 0x0400, 0x79e69a6a )
	ROM_LOAD( "bai1-1",       0x1800, 0x0400, 0xdda7d1e8 )
	ROM_LOAD( "bai2-1",       0x1C00, 0x0400, 0x5c5f3f86 )
	ROM_LOAD( "baj1-1",       0x2000, 0x0400, 0x0aa8b79a )
	ROM_LOAD( "baj2-2",       0x2400, 0x0400, 0xa10b848a )

#if 0
	ROM_REGION(0x0010)              /* Not Used */

    ROM_REGION(0x0800)                  /* Sound 8035 + 76477 Sound Generator */
    ROM_LOAD( "basnd.u2",     0x0000, 0x0400, 0x0 )
#endif

ROM_END

static struct MemoryReadAddress bandido_readmem[] =
{
	{ 0x0000, 0x27ff, MRA_ROM },
	{ 0x4200, 0x7FFF, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress bandido_writemem[] =
{
	{ 0x4200, 0x5dff, boothill_videoram_w, &invaders_videoram },
	{ 0x5E00, 0x7fff, MWA_RAM },
	{ 0x0000, 0x27ff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct IOReadPort bandido_readport[] =                                   /* MJC */
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ 0x04, 0x04, input_port_3_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort bandido_writeport[] =                 /* MJC */
{
	{ 0x02, 0x02, invaders_shift_amount_w },
	{ 0x03, 0x03, invaders_shift_data_w },
	{ -1 }  /* end of table */
};

/* All of the controls/dips for cocktail mode are as per the schematic */
/* BUT a coffee table version was never manufactured and support was   */
/* probably never completed.                                           */
/* e.g. cocktail players button will give 6 credits!                   */

INPUT_PORTS_START( bandido_input_ports )                        /* MJC */

	PORT_START      /* 00 Main Controls */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY )

	PORT_START      /* 01 Player 2 Controls */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY | IPF_COCKTAIL )

	PORT_START      /* 02 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )           /* Marked for   */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )           /* Expansion    */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )           /* on Schematic */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* 04 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail (SEE NOTES)" )
INPUT_PORTS_END

static struct MachineDriver bandido_machine_driver =                    /* MJC */
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			bandido_readmem, bandido_writemem, bandido_readport, bandido_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0
};

static int bandido_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x6003],"\xff\x01\xb8",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
		    osd_fread(f,&RAM[0x60f1],0x3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void bandido_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
	    osd_fwrite(f,&RAM[0x60f1],0x3);
		osd_fclose(f);
		RAM[0x6003] = 0;
	}
}


struct GameDriver bandido_driver =                                                              /* MJC */
{
	__FILE__,
	0,
	"bandido",
	"Bandido",
	"1980",
	"Exidy",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nMirko Buffoni\nValerio Verrando\nMike Coates\n",
	0,
	&bandido_machine_driver,
	0,

	bandido_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	bandido_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	bandido_hiload, bandido_hisave
};

/*******************************************************/
/*                                                     */
/* Midway "Boot Hill"                                  */
/*                                                     */
/*******************************************************/

ROM_START( boothill_rom )
	ROM_REGION(0x10000)     /* 64k for code */
    ROM_LOAD( "romh.cpu",     0x0000, 0x0800, 0x1615d077 )
    ROM_LOAD( "romg.cpu",     0x0800, 0x0800, 0x65a90420 )
    ROM_LOAD( "romf.cpu",     0x1000, 0x0800, 0x3fdafd79 )
    ROM_LOAD( "rome.cpu",     0x1800, 0x0800, 0x374529f4 )
ROM_END

static struct MemoryWriteAddress boothill_writemem[] =
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, boothill_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct IOReadPort boothill_readport[] =                                  /* MJC 310198 */
{
	{ 0x00, 0x00, boothill_port_0_r },
	{ 0x01, 0x01, boothill_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, boothill_shift_data_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort boothill_writeport[] =                                /* MJC 310198 */
{
	{ 0x01, 0x01, invaders_shift_amount_w },
	{ 0x02, 0x02, invaders_shift_data_w },
	{ 0x03, 0x03, boothill_sh_port3_w },
	{ 0x05, 0x05, boothill_sh_port5_w },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( boothill_input_ports )                                       /* MJC 310198 */
    /* Gun position uses bits 4-6, handled using fake paddles */
	PORT_START      /* IN0 - Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )        /* Move Man */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )              /* Fire */

	PORT_START      /* IN1 - Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )              /* Move Man */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )                    /* Fire */

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0C, 0x00, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "64" )
	PORT_DIPSETTING(    0x04, "74" )
	PORT_DIPSETTING(    0x08, "84" )
	PORT_DIPSETTING(    0x0C, "94" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE )                   /* Test */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

    PORT_START                                                                                          /* Player 2 Gun */
	PORT_ANALOGX ( 0xff, 0x00, IPT_PADDLE | IPF_PLAYER2, 100, 7, 1, 255, 0, 0, 0, 0, 1 )

    PORT_START                                                                                          /* Player 1 Gun */
	PORT_ANALOGX ( 0xff, 0x00, IPT_PADDLE, 100, 7, 1, 255, OSD_KEY_Z, OSD_KEY_A, 0, 0, 1 )
INPUT_PORTS_END

static struct MachineDriver boothill_machine_driver =                   /* MJC 310198 */
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, boothill_writemem, boothill_readport, boothill_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static const char *boothill_sample_names[] =
{
	"*boothill", /* in case we ever find any bootlegs hehehe */
	"addcoin.sam",
	"endgame.sam",
	"gunshot.sam",
	"killed.sam",
	0       /* end of array */
};

struct GameDriver boothill_driver =                                                     /* MJC 310198 */
{
	__FILE__,
	0,
	"boothill",
	"Boot Hill",
	"1977",
	"Midway",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili\nMike Balfour\nMike Coates\n\n(added samples)\n Howie Cohen \n Douglas Silfen \n Eric Lundquist",
	0,
	&boothill_machine_driver,
	0,

	boothill_rom,
	0, 0,
	boothill_sample_names,
	0,      /* sound_prom */

	boothill_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};


/*******************************************************/
/*                                                     */
/* Taito "Space Chaser"                                */
/*                                                     */
/*******************************************************/

ROM_START( schaser_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "rt13.bin",     0x0000, 0x0400, 0x0dfbde68 )
	ROM_LOAD( "rt14.bin",     0x0400, 0x0400, 0x5a508a25 )
	ROM_LOAD( "rt15.bin",     0x0800, 0x0400, 0x2ac43a93 )
	ROM_LOAD( "rt16.bin",     0x0c00, 0x0400, 0xf5583afc )
	ROM_LOAD( "rt17.bin",     0x1000, 0x0400, 0x51cf1155 )
	ROM_LOAD( "rt18.bin",     0x1400, 0x0400, 0x3f0fc73a )
	ROM_LOAD( "rt19.bin",     0x1800, 0x0400, 0xb66ea369 )
	ROM_LOAD( "rt20.bin",     0x1c00, 0x0400, 0xe3a7466a )
	ROM_LOAD( "rt21.bin",     0x4000, 0x0400, 0xb368ac98 )
	ROM_LOAD( "rt22.bin",     0x4400, 0x0400, 0x6e060dfb )
ROM_END

INPUT_PORTS_START( schaser_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_START      /* IN1 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Level", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Tilt  */
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END

static int schaser_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2010],"\x93\x1c\x40",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{

		    osd_fread(f,&RAM[0x233f],3);
			osd_fclose(f);

					}

	return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void schaser_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{

	    osd_fwrite(f,&RAM[0x233f],3);
		osd_fclose(f);
		RAM[0x2010] =0;
	}
}

/* LT 20-2-1998 */
struct GameDriver schaser_driver =
{
	__FILE__,
	0,
	"schaser",
	"Space Chaser",
	"1980",
	"Taito",
	"Lee Taylor\n",
	0,
	&lrescue_machine_driver,
	0,

	schaser_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	schaser_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	schaser_hiload, schaser_hisave
};

/*******************************************************/
/*                                                     */
/* Midway "Space Encounters"                           */
/*                                                     */
/* To Do : 'trench' video overlay                      */
/*                                                     */
/*******************************************************/

ROM_START( spcenctr_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "4m33.cpu",     0x0000, 0x0800, 0x7458b2db )
	ROM_LOAD( "4m32.cpu",     0x0800, 0x0800, 0x1b873788 )
	ROM_LOAD( "4m31.cpu",     0x1000, 0x0800, 0xd4319c91 )
	ROM_LOAD( "4m30.cpu",     0x1800, 0x0800, 0x9b9a1a45 )
	ROM_LOAD( "4m29.cpu",     0x4000, 0x0800, 0x294d52ce )
	ROM_LOAD( "4m28.cpu",     0x4800, 0x0800, 0xce44c923 )
	ROM_LOAD( "4m27.cpu",     0x5000, 0x0800, 0x098070ab )
	ROM_LOAD( "4m26.cpu",     0x5800, 0x0800, 0x7f1d1f44 )
ROM_END

static struct IOWritePort spcenctr_writeport[] =
{
	{ 0x01, 0x01, invaders_shift_amount_w },
	{ 0x02, 0x02, invaders_shift_data_w },
	{ -1 }  /* end of table */
};

static struct IOReadPort spcenctr_readport[] =
{
	{ 0x00, 0x00, gray6bit_controller0_r }, /* These 2 ports use Gray's binary encoding */
	{ 0x01, 0x01, gray6bit_controller1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( spcenctr_input_ports )
	PORT_START      /* IN0 */
	PORT_ANALOG ( 0x3f, 0x1f, IPT_AD_STICK_X, 25, 0, 0x01, 0x3e) /* 6 bit horiz encoder - Gray's binary? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )    /* fire */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_ANALOG ( 0x3f, 0x1f, IPT_AD_STICK_Y, 25, 0, 0x01, 0x3e) /* 6 bit vert encoder - Gray's binary? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2000, 4000, 8000" )
	PORT_DIPSETTING(    0x01, "3000, 6000, 12000" )
	PORT_DIPSETTING(    0x02, "4000, 8000, 16000" )
	PORT_DIPSETTING(    0x03, "5000, 10000, 20000" )
	PORT_DIPNAME( 0x0c, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x30, 0x00, "Bonus/Test Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Bonus On" )
	PORT_DIPSETTING(    0x30, "Bonus Off" )
	PORT_DIPSETTING(    0x20, "Cross Hatch" )
	PORT_DIPSETTING(    0x10, "Ram/Rom Test" )
	PORT_DIPNAME( 0xc0, 0x00, "Play Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "45 Sec" )
	PORT_DIPSETTING(    0x40, "60 Sec" )
	PORT_DIPSETTING(    0x80, "75 Sec" )
	PORT_DIPSETTING(    0xC0, "90 Sec" )
INPUT_PORTS_END

static struct MachineDriver spcenctr_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, boothill_writemem, spcenctr_readport, spcenctr_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};


static int spcenctr_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x26f0],"\x0e\x0e",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
		    osd_fread(f,&RAM[0x202b],2);
			osd_fclose(f);


			RAM[0x202e] = RAM[0x202b];
			RAM[0x202f] = RAM[0x202c];
			}

	return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void spcenctr_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x202b],2);
		osd_fclose(f);
		RAM[0x26f0] = 0;
	}
}

struct GameDriver spcenctr_driver =
{
	__FILE__,
	0,
	"spcenctr",
	"Space Encounters",
	"1980",
	"Midway",
	"Space Invaders Team\nLee Taylor\n",
	0,
	&spcenctr_machine_driver,
	0,

	spcenctr_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	spcenctr_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	spcenctr_hiload, spcenctr_hisave
};

/*******************************************************/
/*                                                     */
/* Midway "Clowns"                                     */
/*                                                     */
/*******************************************************/

ROM_START( clowns_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "h2.cpu",       0x0000, 0x0400, 0xff4432eb )
	ROM_LOAD( "g2.cpu",       0x0400, 0x0400, 0x676c934b )
	ROM_LOAD( "f2.cpu",       0x0800, 0x0400, 0x00757962 )
	ROM_LOAD( "e2.cpu",       0x0c00, 0x0400, 0x9e506a36 )
	ROM_LOAD( "d2.cpu",       0x1000, 0x0400, 0xd61b5b47 )
	ROM_LOAD( "c2.cpu",       0x1400, 0x0400, 0x154d129a )
ROM_END

/*
 * Clowns (EPROM version)
 */
INPUT_PORTS_START( clowns_input_ports )
	PORT_START      /* IN0 */
	PORT_ANALOG ( 0xff, 0x7f, IPT_PADDLE, 100, 0, 0x01, 0xfe)

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 or 2 Players Game" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 or 2 Players Game" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Balloon Resets", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Each row" )
	PORT_DIPSETTING(    0x10, "All rows" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3000" )
	PORT_DIPSETTING(    0x20, "4000" )
	PORT_DIPNAME( 0x40, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3")
	PORT_DIPSETTING(    0x40, "4")
	PORT_DIPNAME( 0x80, 0x00, "Test Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "On")
	PORT_DIPSETTING(    0x00, "Off")
INPUT_PORTS_END

static struct MachineDriver clowns_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, boothill_writemem, invdelux_readport, spcenctr_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static int clowns_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];



	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2000],"\x38\x0e",2) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int hi;
			char length[5];
			int digits;
		    osd_fread(f,&RAM[0x2015],2);
			osd_fclose(f);

				hi = (RAM[0x2015] >> 4  ) * 1 +
				 	 (RAM[0x2015] & 0x0f) * 10 +
				 	 (RAM[0x2016] >> 4	) * 100 +
				 	 (RAM[0x2016] & 0x0f) * 1000 ;


				sprintf(length,"%i",hi);
				digits = strlen(length);
				mix_hiscoreprint(15, 24, hi, digits,10, 1,0x400, 0x15e, 2);
				}
	return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void clowns_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{

	    osd_fwrite(f,&RAM[0x2015],2);
		osd_fclose(f);
		RAM[0x2000]=0;
	}
}


struct GameDriver clowns_driver =
{
	__FILE__,
	0,
	"clowns",
	"Clowns",
	"1978",
	"Midway",
	"Space Invaders Team\nLee Taylor\n",
	0,
	&clowns_machine_driver,
	0,

	clowns_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	clowns_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	clowns_hiload, clowns_hisave
};

/*******************************************************/
/*                                                     */
/* Midway "Guided Missile"                             */
/*                                                     */
/*******************************************************/

ROM_START( gmissile_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "gm_623.h",     0x0000, 0x0800, 0xa3ebb792 )
	ROM_LOAD( "gm_623.g",     0x0800, 0x0800, 0xa5e740bb )
	ROM_LOAD( "gm_623.f",     0x1000, 0x0800, 0xda381025 )
	ROM_LOAD( "gm_623.e",     0x1800, 0x0800, 0xf350146b )
ROM_END

INPUT_PORTS_START( gmissile_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 ) /* Start 2 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 ) /* Start 1 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x03, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x08, "70" )
	PORT_DIPSETTING(    0x04, "80" )
	PORT_DIPSETTING(    0x0c, "90" )
	PORT_DIPNAME( 0x30, 0x00, "Extra Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "500" )
	PORT_DIPSETTING(    0x20, "700" )
	PORT_DIPSETTING(    0x10, "1000" )
	PORT_DIPSETTING(    0x30, "None" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x80, 0x80, "Test Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

struct GameDriver gmissile_driver =
{
	__FILE__,
	0,
	"gmissile",
	"Guided Missile",
	"1977",
	"Midway",
	"Lee Taylor\n",
	0,
	&boothill_machine_driver,
	0,

	gmissile_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	gmissile_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

/*******************************************************/
/*                                                     */
/* Midway "Sea Wolf"                                   */
/*                                                     */
/*******************************************************/

ROM_START( seawolf_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "sw0041.h",     0x0000, 0x0400, 0x8f597323 )
	ROM_LOAD( "sw0042.g",     0x0400, 0x0400, 0xdb980974 )
	ROM_LOAD( "sw0043.f",     0x0800, 0x0400, 0xe6ffa008 )
	ROM_LOAD( "sw0044.e",     0x0c00, 0x0400, 0xc3557d6a )
ROM_END

static struct IOReadPort seawolf_readport[] =
{
	{ 0x00, 0x00, seawolf_shift_data_r },
	{ 0x01, 0x01, seawolf_port_0_r },
	{ 0x02, 0x02, input_port_1_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort seawolf_writeport[] =
{
	{ 0x03, 0x03, invaders_shift_data_w },
	{ 0x04, 0x04, invaders_shift_amount_w },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( seawolf_input_ports )
	PORT_START      /* IN0 */
	PORT_ANALOG ( 0x1f, 0x01, IPT_PADDLE, 100, 0, 0x01, 0xfe)
//	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN ) // x movement
//	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) // x movement
//	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) // x movement
//	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) // x movement
//	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) // x movement
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_DIPNAME( 0xc0, 0xc0, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "61" )
	PORT_DIPSETTING(    0x40, "71" )
	PORT_DIPSETTING(    0x80, "81" )
	PORT_DIPSETTING(    0xc0, "91" )

	PORT_START      /* IN1 Dips & Coins */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_DIPNAME( 0x0c, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" ) //
	PORT_DIPSETTING(    0x0c, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" ) //
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_TILT ) // Reset High Scores
	PORT_DIPNAME( 0xe0, 0x00, "Extended Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "2000" )
	PORT_DIPSETTING(    0x40, "3000" )
	PORT_DIPSETTING(    0x60, "4000" )
	PORT_DIPSETTING(    0x80, "5000" )
	PORT_DIPSETTING(    0xa0, "6000" )
	PORT_DIPSETTING(    0xc0, "7000" )
	PORT_DIPSETTING(    0xe0, "8000" )
	PORT_DIPSETTING(    0x00, "None" )
INPUT_PORTS_END

static struct MachineDriver seawolf_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,boothill_writemem,seawolf_readport,seawolf_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    seawolf_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};



static int seawolf_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x23f0],"\x78\x00\x54",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int hi;
			char length[3];
			int digits;
		    osd_fread(f,&RAM[0x2006],1);
			osd_fclose(f);


				hi = (RAM[0x2006] >> 4  ) * 1 +
				 	 (RAM[0x2006] & 0x0f) * 10 ;

				sprintf(length,"%i",hi);
				digits = strlen(length);
				mix_hiscoreprint(25, 26, hi, digits,10, 1,0x800,0x38f,  2);
			}

	return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void seawolf_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{

	    osd_fwrite(f,&RAM[0x2006],1);
		osd_fclose(f);
		RAM[0x23f0] = 0;
	}
}


struct GameDriver seawolf_driver =
{
	__FILE__,
	0,
	"seawolf",
	"Sea Wolf",
	"1976",
	"Midway",
	"Lee Taylor\n",
	0,
	&seawolf_machine_driver,
	0,

	seawolf_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	seawolf_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	seawolf_hiload, seawolf_hisave
};

/*******************************************************/
/*                                                     */
/* Midway "Gun Fight"                                  */
/*                                                     */
/*******************************************************/

ROM_START( gunfight_rom)
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "7609h.bin",    0x0000, 0x0400, 0x0b117d73 )
	ROM_LOAD( "7609g.bin",    0x0400, 0x0400, 0x57bc3159 )
	ROM_LOAD( "7609f.bin",    0x0800, 0x0400, 0x8049a6bd )
	ROM_LOAD( "7609e.bin",    0x0c00, 0x0400, 0x773264e2 )
ROM_END

static struct IOWritePort gunfight_writeport[] =
{
	{ 0x02, 0x02, invaders_shift_amount_w },
	{ 0x04, 0x04, invaders_shift_data_w },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( gunfight_input_ports )
    /* Gun position uses bits 4-6, handled using fake paddles */
	PORT_START      /* IN0 - Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )        /* Move Man */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT| IPF_8WAY  | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )              /* Fire */

	PORT_START      /* IN1 - Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )              /* Move Man */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 )                    /* Fire */

#ifdef NOTDEF
	PORT_START      /* IN2 Dips & Coins */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_DIPNAME( 0x0C, 0x00, "Plays", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0C, "4" )
	PORT_DIPNAME( 0x30, 0x00, "Time", IP_KEY_NONE ) /* These are correct */
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x10, "70" )
	PORT_DIPSETTING(    0x20, "80" )
	PORT_DIPSETTING(    0x30, "90" )
	PORT_DIPNAME( 0xc0, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin - 1 Player" )
	PORT_DIPSETTING(    0x40, "1 Coin - 2 Players" )
	PORT_DIPSETTING(    0x80, "1 Coin - 3 Players" )
	PORT_DIPSETTING(    0xc0, "1 Coin - 4 Players" )
#endif

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin" )
	PORT_DIPSETTING(    0x01, "2 Coins" )
	PORT_DIPSETTING(    0x02, "3 Coins" )
	PORT_DIPSETTING(    0x03, "4 Coins" )
	PORT_DIPNAME( 0x0C, 0x00, "Plays", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0C, "4" )
	PORT_DIPNAME( 0x30, 0x00, "Time", IP_KEY_NONE ) /* These are correct */
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x10, "70" )
	PORT_DIPSETTING(    0x20, "80" )
	PORT_DIPSETTING(    0x30, "90" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

    PORT_START                                                                                          /* Player 2 Gun */
	PORT_ANALOGX ( 0xff, 0x00, IPT_PADDLE | IPF_PLAYER2, 100, 7, 1, 255, 0, 0, 0, 0, 1 )

    PORT_START                                                                                          /* Player 1 Gun */
	PORT_ANALOGX ( 0xff, 0x00, IPT_PADDLE, 100, 7, 1, 255, OSD_KEY_Z, OSD_KEY_A, 0, 0, 1 )
INPUT_PORTS_END

static struct MachineDriver gunfight_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,boothill_writemem,boothill_readport,gunfight_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

struct GameDriver gunfight_driver =
{
	__FILE__,
	0,
	"gunfight",
	"Gunfight",
	"1975",
	"Midway",
	"Lee Taylor\n",
	0,
	&gunfight_machine_driver,
	0,

	gunfight_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	gunfight_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

/*******************************************************/
/*                                                     */
/* Midway "280 ZZZAP"                                  */
/*                                                     */
/*******************************************************/

ROM_START( zzzap_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "zzzaph",       0x0000, 0x0400, 0x1fa86e1c )
	ROM_LOAD( "zzzapg",       0x0400, 0x0400, 0x9639bc6b )
	ROM_LOAD( "zzzapf",       0x0800, 0x0400, 0xadc6ede1 )
	ROM_LOAD( "zzzape",       0x0c00, 0x0400, 0x472493d6 )
	ROM_LOAD( "zzzapd",       0x1000, 0x0400, 0x4c240ee1 )
	ROM_LOAD( "zzzapc",       0x1400, 0x0400, 0x6e85aeaf )
ROM_END

static struct IOWritePort zzzap_writeport[] =
{
	/*{ 0x02, 0x02, zzzap_snd2_w}, */
	/*{ 0x05, 0x05, zzzap_snd5_w}, */
	{ 0x03, 0x03, invaders_shift_data_w },
	{ 0x04, 0x04, invaders_shift_amount_w },
	/*{ 0x07, 0x07, zzzap_wdog }, */
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( zzzap_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* 4 bit accel */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* Crude approximation using 2 buttons */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_TOGGLE ) /* shift */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START1 )

	PORT_START      /* IN1 - Steering Wheel */
	PORT_ANALOG ( 0xff, 0x7f, IPT_PADDLE | IPF_REVERSE, 100, 0, 0x01, 0xfe)

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x00, "Play Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "60 Sec" )
	PORT_DIPSETTING(    0x00, "80 Sec" )
	PORT_DIPSETTING(    0x08, "99 Sec" )
	PORT_DIPSETTING(    0x04, "Test" )
	PORT_DIPNAME( 0x30, 0x00, "Extended Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Score >= 2.5" )
	PORT_DIPSETTING(    0x10, "Score >= 2" )
	PORT_DIPSETTING(    0x20, "None" )
/* 0x30 same as 0x20 */
	PORT_DIPNAME( 0xc0, 0x00, "Language", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "English")
	PORT_DIPSETTING(    0x40, "German")
	PORT_DIPSETTING(    0x80, "French")
	PORT_DIPSETTING(    0xc0, "Spanish")
INPUT_PORTS_END

static struct MachineDriver zzzap_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,boothill_writemem,invdelux_readport,zzzap_writeport,
			invaders_interrupt, 2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0
};


static int zzzap_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];



		if (memcmp(&RAM[0x2411],"\x3c\x3c\x3c",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int hi;
			char length[3];
			int digits;
		    osd_fread(f,&RAM[0x2029],4);
			osd_fread(f,&RAM[0x203d],2);
			osd_fclose(f);

				hi = (RAM[0x2029] >> 4  ) * 1  +
				 	 (RAM[0x2029] & 0x0f) * 10;

				sprintf(length,"%i",hi);
				digits = strlen(length);

				mix_hiscoreprint(5, 26, hi, digits,10, 4,0x000, 0x186, 3);

				hi = (RAM[0x202a] >> 4  ) * 1 +
				 	 (RAM[0x202a] & 0x0f) * 10 ;

				sprintf(length,"%i",hi);
				digits = strlen(length);

				mix_hiscoreprint(2, 26, hi, digits,10, 4,0x000, 0x186, 3);

				hi = (RAM[0x202b] >> 4  ) * 1  +
					 (RAM[0x202b] & 0x0f) * 10;

				sprintf(length,"%i",hi);
				digits = strlen(length);

				mix_hiscoreprint(5, 24, hi, digits,10, 5,0x000, 0x186, 3);

				hi = (RAM[0x202c] >> 4  ) * 1 +
					 (RAM[0x202c] & 0x0f) * 10 ;

					 sprintf(length,"%i",hi);
					 digits = strlen(length);
				mix_hiscoreprint(2, 24, hi, digits,10, 5,0x000, 0x186, 3);
			}
	return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void zzzap_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{

	    osd_fwrite(f,&RAM[0x2029],4);
	    osd_fwrite(f,&RAM[0x203d],2);
		osd_fclose(f);
		RAM[0x2411] = 0;
	}
}


struct GameDriver zzzap_driver =
{
	__FILE__,
	0,
	"280zzzap",
	"280-ZZZAP",
	"1976",
	"Midway",
	"Space Invaders Team\nLee Taylor\n",
	0,
	&zzzap_machine_driver,
	0,

	zzzap_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	zzzap_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	zzzap_hiload, zzzap_hisave
};

/*******************************************************/
/*                                                     */
/* Taito "Lupin III"                                   */
/*                                                     */
/*******************************************************/

ROM_START( lupin3_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "lp12.bin",     0x0000, 0x0800, 0x68a7f47a )
	ROM_LOAD( "lp13.bin",     0x0800, 0x0800, 0xcae9a17b )
	ROM_LOAD( "lp14.bin",     0x1000, 0x0800, 0x3553b9e4 )
	ROM_LOAD( "lp15.bin",     0x1800, 0x0800, 0xacbeef64 )
	ROM_LOAD( "lp16.bin",     0x4000, 0x0800, 0x19fcdc54 )
	ROM_LOAD( "lp17.bin",     0x4800, 0x0800, 0x66289ab2 )
	ROM_LOAD( "lp18.bin",     0x5000, 0x0800, 0x2f07b4ba )
ROM_END

static struct MemoryWriteAddress lupin3_writemem[] =
{
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, lrescue_videoram_w, &invaders_videoram },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x57ff, MWA_ROM },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( lupin3_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP | IPF_4WAY )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP | IPF_4WAY )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x08, 0x00, "Check", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "8" )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPNAME( 0x10, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x10, "Japan" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH,  IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH,  IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x00, "Invulnerability", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "Onn" )
INPUT_PORTS_END


static struct MachineDriver lupin3_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,lupin3_writemem,invdelux_readport,writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static int lupin3_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

    /* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0x2000],"\x18\x41",2) == 0 )
    {
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
			osd_fread(f,&RAM[0x20dc],3);
			osd_fclose(f);
        }

        return 1;
    }
    else
        return 0;  /* we can't load the hi scores yet */
}

static void lupin3_hisave(void)
{
    void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
        osd_fwrite(f,&RAM[0x20dc],3);
        osd_fclose(f);
    }
}



struct GameDriver lupin3_driver =
{
	__FILE__,
	0,
	"lupin3",
	"Lupin III",
	"1980",
	"Taito",
	"Space Invaders Team\nLee Taylor\n",
	0,
	&lupin3_machine_driver,
	0,

	lupin3_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	lupin3_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	lupin3_hiload, lupin3_hisave
};

/*******************************************************/
/*                                                     */
/* Nintendo "Heli Fire"                                */
/*                                                     */
/*******************************************************/

ROM_START( helifire_rom )                                                                                /* MJC */
	ROM_REGION(0x10000)             /* 64k for code */
	ROM_LOAD( "tub.f1b",      0x0000, 0x0400, 0x032f89ca )
	ROM_LOAD( "tub.f2b",      0x0400, 0x0400, 0x2774e70f )
	ROM_LOAD( "tub.g1b",      0x0800, 0x0400, 0xb5ad6e8a )
	ROM_LOAD( "tub.g2b",      0x0C00, 0x0400, 0x5e015bf4 )
	ROM_LOAD( "tub.h1b",      0x1000, 0x0400, 0x23bb4e5a )
	ROM_LOAD( "tub.h2b",      0x1400, 0x0400, 0x358227c6 )
	ROM_LOAD( "tub.i1b",      0x1800, 0x0400, 0x0c679f44 )
	ROM_LOAD( "tub.i2b",      0x1C00, 0x0400, 0xd8b7a398 )
	ROM_LOAD( "tub.j1b",      0x2000, 0x0400, 0x98ef24db )
	ROM_LOAD( "tub.j2b",      0x2400, 0x0400, 0x5e2b5877 )

#if 0
	ROM_REGION(0x0010)              /* Not Used */

	ROM_REGION(0x0800)   /* Sound 8035 + 76477 Sound Generator */
	ROM_LOAD( "tub.snd",      0x0000, 0x0400, 0x0 )
#endif
ROM_END

ROM_START( helifira_rom )
	ROM_REGION(0x10000)             /* 64k for code */
	ROM_LOAD( "f1a.bin",      0x0000, 0x0400, 0x92c9d6c1 )
	ROM_LOAD( "f2a.bin",      0x0400, 0x0400, 0xa264dde8 )
	ROM_LOAD( "tub.g1b",      0x0800, 0x0400, 0xb5ad6e8a )
	ROM_LOAD( "g2a.bin",      0x0C00, 0x0400, 0xa987ebcd )
	ROM_LOAD( "h1a.bin",      0x1000, 0x0400, 0x25abcaf0 )
	ROM_LOAD( "tub.h2b",      0x1400, 0x0400, 0x358227c6 )
	ROM_LOAD( "tub.i1b",      0x1800, 0x0400, 0x0c679f44 )
	ROM_LOAD( "i2a.bin",      0x1C00, 0x0400, 0x296610fd )
	ROM_LOAD( "tub.j1b",      0x2000, 0x0400, 0x98ef24db )
	ROM_LOAD( "tub.j2b",      0x2400, 0x0400, 0x5e2b5877 )

#if 0
	ROM_REGION(0x0010)              /* Not Used */

	ROM_REGION(0x0800)   /* Sound 8035 + 76477 Sound Generator */
	ROM_LOAD( "tub.snd",      0x0000, 0x0400, 0x0 )
#endif
ROM_END


INPUT_PORTS_START( helifire_input_ports )
        PORT_START      /* 00 Main Controls */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY  )
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY )
        PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY )
        PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY )
        PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
        PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
        PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
        PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

        PORT_START      /* 01 Player 2 Controls */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_4WAY | IPF_COCKTAIL )
        PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_4WAY | IPF_COCKTAIL )
        PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_4WAY | IPF_COCKTAIL )
        PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
        PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
        PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
        PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

        PORT_START      /* Start and Coin Buttons */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
        PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
        PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
        PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Marked for   */
        PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) /* Expansion    */
        PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED ) /* on Schematic */
        PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

        PORT_START      /* DSW */
        PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "3" )
        PORT_DIPSETTING(    0x01, "4" )
        PORT_DIPSETTING(    0x02, "5" )
        PORT_DIPSETTING(    0x03, "6" )
        PORT_DIPNAME( 0x0c, 0x00, "Bonus Life", IP_KEY_NONE )
        PORT_DIPSETTING(    0x0c, "5000")
        PORT_DIPSETTING(    0x04, "6000")
        PORT_DIPSETTING(    0x08, "8000")
        PORT_DIPSETTING(    0x00, "10000")
        PORT_DIPNAME( 0x10, 0x00, "Coinage", IP_KEY_NONE )
        PORT_DIPSETTING(    0x10, "2 Coins/1 Credit")
        PORT_DIPSETTING(    0x00, "1 Coin/1 Credit")
        PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "Upright" )
        PORT_DIPSETTING(    0x00, "Cocktail" )
INPUT_PORTS_END

static int helifire_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    /* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0x6600],"\x82\x65",2) == 0 )
    {
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
            int hi;

			osd_fread(f,&RAM[0x6008],3);
			osd_fclose(f);

			hi = (RAM[0x6008] >> 4  ) * 100000 +
			 	 (RAM[0x6008] & 0x0f) * 10000 +
				 (RAM[0x6009] >> 4  ) * 1000 +
				 (RAM[0x6009] & 0x0f) * 100 +
				 (RAM[0x600a] >> 4  ) * 10 +
				 (RAM[0x600a] & 0x0f) * 1;

				mix_hiscoreprint(11, 1, hi, 5, 8, 0,0x1c00, 0x1e, 0);

		}
        return 1;
    }
    else
        return 0;  /* we can't load the hi scores yet */
}

static void helifire_hisave(void)
{
    void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
	    osd_fwrite(f,&RAM[0x6008],3);
		osd_fclose(f);
    	RAM[0x6600] = 0;
	}
}




struct GameDriver helifire_driver =
{
	__FILE__,
	0,
	"helifire",
	"Heli Fire (revision B)",
	"1980",
	"Nintendo",
	"The Space Invaders Team",
	0,
	&bandido_machine_driver,
	0,

	helifire_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	helifire_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	helifire_hiload, helifire_hisave
};

struct GameDriver helifira_driver =
{
	__FILE__,
	&helifire_driver,
	"helifira",
	"Heli Fire (revision A)",
	"1980",
	"Nintendo",
	"The Space Invaders Team",
	0,
	&bandido_machine_driver,
	0,

	helifira_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	helifire_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	helifire_hiload, helifire_hisave
};

/*******************************************************/
/*                                                     */
/* Nintendo "Space Fever (Color)"                      */
/*                                                     */
/*******************************************************/


ROM_START( spacefev_rom )                                                                                /* MJC */
ROM_REGION(0x10000)             /* 64k for code */
        ROM_LOAD( "tsf.f1",       0x0000, 0x0400, 0x35f295bd )
        ROM_LOAD( "tsf.f2",       0x0400, 0x0400, 0x0c633f4c )
        ROM_LOAD( "tsf.g1",       0x0800, 0x0400, 0xf3d851cb )
        ROM_LOAD( "tsf.g2",       0x0C00, 0x0400, 0x1faef63a )
        ROM_LOAD( "tsf.h1",       0x1000, 0x0400, 0xb365389d )
        ROM_LOAD( "tsf.h2",       0x1400, 0x0400, 0xa36c61c9 )
        ROM_LOAD( "tsf.i1",       0x1800, 0x0400, 0xd4f3b50d )

#if 0
	ROM_REGION(0x0010)              /* Not Used */
       ROM_REGION(0x0800)    /* Sound 8035 + 76477 Sound Generator */
       ROM_LOAD( "tsf.snd",      0x0000, 0x0400, 0x0 )
#endif

ROM_END

INPUT_PORTS_START( spacefev_input_ports )

	PORT_START      /* 00 Main Controls */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START      /* 01 Player 2 Controls */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BITX(0x08, 0x00, 0, "Start Game A", OSD_KEY_Q, IP_JOY_NONE, 0 )
	PORT_BITX(0x10, 0x00, 0, "Start Game B", OSD_KEY_W, IP_JOY_NONE, 0 )
	PORT_BITX(0x20, 0x00, 0, "Start Game C", OSD_KEY_E, IP_JOY_NONE, 0 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* If on low the game doesn't start */

	PORT_START      /* DSW */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
//	PORT_DIPNAME( 0xfc, 0x00, "Unknown", IP_KEY_NONE )
//	PORT_DIPSETTING(    0x00, "On" )
//	PORT_DIPSETTING(    0xfc, "Off" )
INPUT_PORTS_END


static int spacefev_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
    /* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0x6013],"\xd0\x04",2) == 0 )
    {
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
        	osd_fread(f,&RAM[0x60ec],3);
			osd_fread(f,&RAM[0x60f1],3);
			osd_fread(f,&RAM[0x60e7],3);
			osd_fclose(f);
        }
        return 1;
    }
    else
        return 0;  /* we can't load the hi scores yet */
}

static void spacefev_hisave(void)
{
    void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
        osd_fwrite(f,&RAM[0x60ec],3);
		osd_fwrite(f,&RAM[0x60f1],3);
		osd_fwrite(f,&RAM[0x60e7],3);
		osd_fclose(f);
		RAM[0x6013]=0;
    }
}


struct GameDriver spacefev_driver =
{
	__FILE__,
	0,
	"spacefev",
	"Space Fever",
	"1980",
	"Nintendo",
	"The Space Invaders Team",
	0,
	&bandido_machine_driver,
	0,

	spacefev_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	spacefev_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	spacefev_hiload, spacefev_hisave
};

/*******************************************************/
/*                                                     */
/* Nintendo "Space Fever" (B&W)                        */
/*                                                     */
/*******************************************************/
ROM_START( sfeverbw_rom )
ROM_REGION(0x10000)             /* 64k for code */
        ROM_LOAD( "spacefev.f1", 0x0000, 0x0400, 0xb8887351 )
        ROM_LOAD( "spacefev.f2", 0x0400, 0x0400, 0xcda933a7 )
        ROM_LOAD( "spacefev.g1", 0x0800, 0x0400, 0xde17578a )
        ROM_LOAD( "spacefev.g2", 0x0C00, 0x0400, 0xf1a90948 )
        ROM_LOAD( "spacefev.h1", 0x1000, 0x0400, 0xeefb4273 )
        ROM_LOAD( "spacefev.h2", 0x1400, 0x0400, 0xe91703e8 )
        ROM_LOAD( "spacefev.i1", 0x1800, 0x0400, 0x41e18df9 )
        ROM_LOAD( "spacefev.i2", 0x1c00, 0x0400, 0xeff9f82d )

#if 0
        ROM_REGION(0x0010)              /* Not Used */

       ROM_REGION(0x0800)    /* Sound 8035 + 76477 Sound Generator */
       ROM_LOAD( "TSF.SND", 0x0000, 0x0400, 0x0 )
#endif

ROM_END

static int sfeverbw_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
    /* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0x6070],"\xB0\x04",2) == 0 )
    {
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
		osd_fread(f,&RAM[0x609d],3);
		osd_fread(f,&RAM[0x60a3],3);
		osd_fread(f,&RAM[0x60a9],3);
		osd_fread(f,&RAM[0x60af],3);
		osd_fread(f,&RAM[0x60b5],3);
		osd_fread(f,&RAM[0x60bb],3);
		osd_fread(f,&RAM[0x60c1],3);
		osd_fread(f,&RAM[0x60c7],3);
		osd_fread(f,&RAM[0x60cd],3);
		osd_fread(f,&RAM[0x60e7],3);
		osd_fread(f,&RAM[0x60ec],3);
		osd_fread(f,&RAM[0x60f1],3);
		osd_fclose(f);
        }
        return 1;
    }
    else
        return 0;  /* we can't load the hi scores yet */
}

static void sfeverbw_hisave(void)
{
    void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
        osd_fwrite(f,&RAM[0x609d],3);
		osd_fwrite(f,&RAM[0x60a3],3);
		osd_fwrite(f,&RAM[0x60a9],3);
		osd_fwrite(f,&RAM[0x60af],3);
		osd_fwrite(f,&RAM[0x60b5],3);
		osd_fwrite(f,&RAM[0x60bb],3);
		osd_fwrite(f,&RAM[0x60c1],3);
		osd_fwrite(f,&RAM[0x60c7],3);
		osd_fwrite(f,&RAM[0x60cd],3);
		osd_fwrite(f,&RAM[0x60e7],3);
		osd_fwrite(f,&RAM[0x60ec],3);
		osd_fwrite(f,&RAM[0x60f1],3);
		osd_fclose(f);
		RAM[0x6070] = 0;

    }
}


struct GameDriver sfeverbw_driver =
{
        __FILE__,
        0,
        "sfeverbw",
        "Space Fever (B&W)",
        "1980",
        "Nintendo",
        "The Space Invaders Team",
        0,
        &bandido_machine_driver,
		0,

        sfeverbw_rom,
        0, 0,
        0,
        0,      /* sound_prom */

        spacefev_input_ports,

        0, palette, 0,
        ORIENTATION_ROTATE_270,

        sfeverbw_hiload, sfeverbw_hisave
};

/*******************************************************/
/*                                                     */
/* Taito "Polaris"                                     */
/*                                                     */
/*******************************************************/


ROM_START( polaris_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ps-01",        0x0000, 0x0800, 0xc04ce5a9 )
	ROM_LOAD( "ps-09",        0x0800, 0x0800, 0x9a5c8cb2 )
	ROM_LOAD( "ps-08",        0x1000, 0x0800, 0x8680d7ea )
	ROM_LOAD( "ps-04",        0x1800, 0x0800, 0x65694948 )
	ROM_LOAD( "ps-05",        0x4000, 0x0800, 0x772e31f3 )
	ROM_LOAD( "ps-10",        0x4800, 0x0800, 0x3df77bac )
ROM_END

ROM_START( polarisa_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ps01-1",      0x0000, 0x0800, 0x7d41007c )
	ROM_LOAD( "ps-09",       0x0800, 0x0800, 0x9a5c8cb2 )
	ROM_LOAD( "ps03-1",      0x1000, 0x0800, 0x21f32415 )
	ROM_LOAD( "ps-04",       0x1800, 0x0800, 0x65694948 )
	ROM_LOAD( "ps-05",       0x4000, 0x0800, 0x772e31f3 )
	ROM_LOAD( "ps-10",       0x4800, 0x0800, 0x3df77bac )
	ROM_LOAD( "ps26",        0x5000, 0x0800, 0x9d5c3d50 )
//	ROM_LOAD( "ps07",        0x5800, 0x0400, 0x164aa05d )
//	ROM_LOAD( "ps08",        0x5c00, 0x0100, 0x2953253b )
ROM_END

static struct MemoryReadAddress polaris_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x4fff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress polaris_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x2400, 0x3fff, boothill_videoram_w, &invaders_videoram },
	{ 0x4000, 0x4fff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct IOReadPort polaris_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort polaris_writeport[] =
{
	{ 0x01, 0x01, invaders_shift_amount_w },
	{ 0x03, 0x03, invaders_shift_data_w },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( polaris_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )   // Tilt
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )


	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_DIPNAME( 0x80, 0x00, "Preset Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END


static struct MachineDriver polaris_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			polaris_readmem,polaris_writemem,polaris_readport,polaris_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0
};


static int polaris_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    /* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0x2040],"\x28\xE0",2) == 0 )
    {
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
			osd_fread(f,&RAM[0x2314],2);
            osd_fclose(f);
        }

        return 1;
    }
    else
        return 0;  /* we can't load the hi scores yet */
}

static void polaris_hisave(void)
{
    void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
        osd_fwrite(f,&RAM[0x2314],2);
        osd_fclose(f);
    	RAM[0x2040] = 0;
	}
}



struct GameDriver polaris_driver =
{
	__FILE__,
	0,
	"polaris",
	"Polaris (set 1)",
	"1980",
	"Taito",
	"Space Invaders Team",
	0,
	&polaris_machine_driver,
	0,

	polaris_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	polaris_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	polaris_hiload, polaris_hisave
};

struct GameDriver polarisa_driver =
{
	__FILE__,
	&polaris_driver,
	"polarisa",
	"Polaris (set 2)",
	"1980",
	"Taito",
	"Space Invaders Team",
	0,
	&polaris_machine_driver,
	0,

	polarisa_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	polaris_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	polaris_hiload, polaris_hisave
};


/*******************************************************/
/*                                                     */
/* Midway "Laguna Racer"                               */
/*                                                     */
/*******************************************************/


ROM_START( lagunar_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "lagunar.h",    0x0000, 0x0800, 0x0cd5a280 )
	ROM_LOAD( "lagunar.g",    0x0800, 0x0800, 0x824cd6f5 )
	ROM_LOAD( "lagunar.f",    0x1000, 0x0800, 0x62692ca7 )
	ROM_LOAD( "lagunar.e",    0x1800, 0x0800, 0x20e098ed )
ROM_END

INPUT_PORTS_START( lagunar_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* 4 bit accel */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) /* Crude approximation using 2 buttons */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_TOGGLE ) /* shift */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START1 )

	PORT_START      /* IN1 - Steering Wheel */
	PORT_ANALOG ( 0xff, 0x7f, IPT_PADDLE | IPF_REVERSE, 100, 0, 0x01, 0xfe)

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Play Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "45 Sec" )
	PORT_DIPSETTING(    0x04, "60 Sec" )
	PORT_DIPSETTING(    0x08, "75 Sec" )
	PORT_DIPSETTING(    0x0c, "90 Sec" )
	PORT_DIPNAME( 0x30, 0x00, "Extended Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "350" )
	PORT_DIPSETTING(    0x10, "400" )
	PORT_DIPSETTING(    0x20, "450" )
	PORT_DIPSETTING(    0x30, "500" )
	PORT_DIPNAME( 0xc0, 0x00, "Test Modes", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "Play Mode")
	PORT_DIPSETTING(    0x40, "RAM/ROM")
	PORT_DIPSETTING(    0x80, "Steering")
	PORT_DIPSETTING(    0xc0, "No Extended Play")
INPUT_PORTS_END


static int lagunar_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
    /* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0x2030],"\x04\x0d",2) == 0 )
    {
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
        	osd_fread(f,&RAM[0x2005],3);
			osd_fread(f,&RAM[0x2011],3);
			osd_fread(f,&RAM[0x2014],3);
			osd_fread(f,&RAM[0x2017],3);
		    osd_fclose(f);
        }
        return 1;
    }
    else
        return 0;  /* we can't load the hi scores yet */
}

static void lagunar_hisave(void)
{
    void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
        //osd_fwrite(f,&RAM[0x2000],0x3ff);
        osd_fwrite(f,&RAM[0x2005],3);
		osd_fwrite(f,&RAM[0x2011],3);
		osd_fwrite(f,&RAM[0x2014],3);
		osd_fwrite(f,&RAM[0x2017],3);


		osd_fclose(f);
    }
}


struct GameDriver lagunar_driver =
{
	__FILE__,
	0,
	"lagunar",
	"Laguna Racer",
	"1977",
	"Midway",
	"The Space Invaders Team",
	0,
	&zzzap_machine_driver,
	0,

	lagunar_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	lagunar_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_90,

	lagunar_hiload, lagunar_hisave
};

/*******************************************************/
/*                                                     */
/* Midway "M-4"                                        */
/*                                                     */
/*******************************************************/


ROM_START( m4_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "m4.h",         0x0000, 0x0800, 0x9ee2a0b5 )
	ROM_LOAD( "m4.g",         0x0800, 0x0800, 0x0e84b9cb )
	ROM_LOAD( "m4.f",         0x1000, 0x0800, 0x9ded9956 )
	ROM_LOAD( "m4.e",         0x1800, 0x0800, 0xb6983238 )
ROM_END

static struct IOReadPort m4_readport[] =                                  /* MJC 310198 */
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, boothill_shift_data_r },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( m4_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_JOYSTICKLEFT_UP | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICKLEFT_DOWN | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON1 ) /* Left trigger */
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 ) /* Left reload */
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_JOYSTICKRIGHT_UP | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_START1 )  /* 1 player */
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_JOYSTICKRIGHT_DOWN | IPF_2WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_BUTTON3 ) /* Right trigger */
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON4 ) /* Right reload */
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_START2 )  /* 2 player */
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin  / Player" )
	PORT_DIPSETTING(    0x01, "1 Coin  / 2 Players" )
	PORT_DIPSETTING(    0x02, "2 Coins / Player" )
	PORT_DIPSETTING(    0x03, "2 Coins / 2 Players" )
	PORT_DIPNAME( 0x0C, 0x0C, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "60" )
	PORT_DIPSETTING(    0x04, "70" )
	PORT_DIPSETTING(    0x08, "80" )
	PORT_DIPSETTING(    0x0C, "90" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static struct MachineDriver m4_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, boothill_writemem, m4_readport, boothill_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

struct GameDriver m4_driver =
{
	__FILE__,
	0,
	"m4",
	"M-4",
	"1977",
	"Midway",
	"The Space Invaders Team",
	0,
	&m4_machine_driver,
	0,

	m4_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	m4_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};

/*******************************************************/
/*                                                     */
/* Midway "Phantom II"                                 */
/*                                                     */
/* To Do : little fluffy clouds                        */
/*         you still see them sometimes in the desert  */
/*                                                     */
/*******************************************************/


ROM_START( phantom2_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "phantom2.h",   0x0000, 0x0800, 0x0e3c2439 )
	ROM_LOAD( "phantom2.g",   0x0800, 0x0800, 0xe8df3e52 )
	ROM_LOAD( "phantom2.f",   0x1000, 0x0800, 0x30e83c6d )
	ROM_LOAD( "phantom2.e",   0x1800, 0x0800, 0x8c641cac )
ROM_END

static struct IOReadPort phantom2_readport[] =                                  /* MJC 310198 */
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, boothill_shift_data_r },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( phantom2_input_ports )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )                    /* Fire */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x01, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x06, 0x06, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "45sec 20sec 20" )
	PORT_DIPSETTING(    0x02, "60sec 25sec 25" )
	PORT_DIPSETTING(    0x04, "75sec 30sec 30" )
	PORT_DIPSETTING(    0x06, "90sec 35sec 35" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE )                   /* Test */

INPUT_PORTS_END

static struct MachineDriver phantom2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem, boothill_writemem, phantom2_readport, boothill_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};


static int phantom2_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    /* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0x2000],"\x36\x20",2) == 0 )
    {
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
	        osd_fread(f,&RAM[0x2161],3);
			osd_fread(f,&RAM[0x2168],3);
			osd_fread(f,&RAM[0x216f],3);
            osd_fclose(f);
        }

        return 1;
    }
    else
        return 0;  /* we can't load the hi scores yet */
}

static void phantom2_hisave(void)
{
    void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
        osd_fwrite(f,&RAM[0x2161],3);
		osd_fwrite(f,&RAM[0x2168],3);
		osd_fwrite(f,&RAM[0x216f],3);
		osd_fclose(f);
		RAM[0x2000] = 0;
    }
}

struct GameDriver phantom2_driver =
{
	__FILE__,
	0,
	"phantom2",
	"Phantom II",
	"1979",
	"Midway",
	"The Space Invaders Team",
	0,
	&phantom2_machine_driver,
	0,

	phantom2_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	phantom2_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

phantom2_hiload, phantom2_hisave
};

/*******************************************************/
/*                                                     */
/* Midway "Dog Patch"                                  */
/*                                                     */
/*******************************************************/


static struct IOReadPort dogpatch_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, invaders_shift_data_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort dogpatch_writeport[] =
{
	{ 0x01, 0x01, invaders_shift_amount_w },
	{ 0x02, 0x02, invaders_shift_data_w },
	{ 0x03, 0x03, invaders_dummy_write },
	{ 0x04, 0x04, invaders_dummy_write },
	{ 0x05, 0x05, invaders_dummy_write },
	{ 0x06, 0x06, invaders_dummy_write },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( dogpatch_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START      /* IN1 */
	PORT_ANALOG ( 0x3f, 0x1f, IPT_AD_STICK_X, 25, 0, 0x01, 0x3e) /* 6 bit horiz encoder - Gray's binary? */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x03, 0x00, "# Cans", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "10" )
	PORT_DIPSETTING(    0x02, "15" )
	PORT_DIPSETTING(    0x01, "20" )
	PORT_DIPSETTING(    0x00, "25" )
	PORT_DIPNAME( 0x0c, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "2 Coins/1 Credit (1 or 2 Players)" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/1 Credit (1 or 2 Players)" )
	PORT_DIPNAME( 0x10, 0x00, "Extended Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "3 extra cans" )
	PORT_DIPSETTING(    0x00, "5 extra cans" )
	PORT_DIPNAME( 0x20, 0x20, "Test", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPSETTING(    0x20, "Off")
	PORT_DIPNAME( 0xc0, 0x00, "Extended Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "150 Pts")
	PORT_DIPSETTING(    0x80, "175 Pts")
	PORT_DIPSETTING(    0x40, "225 Pts")
	PORT_DIPSETTING(    0x00, "275 Pts")
INPUT_PORTS_END

static struct MachineDriver dogpatch_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,boothill_writemem,dogpatch_readport,dogpatch_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

ROM_START( dogpatch_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "dogpatch.h",   0x0000, 0x0800, 0x74ebdf4d )
	ROM_LOAD( "dogpatch.g",   0x0800, 0x0800, 0xac246f70 )
	ROM_LOAD( "dogpatch.f",   0x1000, 0x0800, 0xa975b011 )
	ROM_LOAD( "dogpatch.e",   0x1800, 0x0800, 0xc12b1f60 )
ROM_END

struct GameDriver dogpatch_driver =
{
	__FILE__,
	0,
	"dogpatch",
	"Dog Patch",
	"1977",
	"Midway",
	"The Space Invaders Team",
	0,
	&dogpatch_machine_driver,
	0,

	dogpatch_rom,
	0, 0,
	0, 0,      /* sound_prom */

	dogpatch_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};

/*******************************************************/
/*                                                     */
/* Midway "4 Player Bowling"                           */
/*                                                     */
/*******************************************************/


static struct IOReadPort midwbowl_readport[] =
{
	{ 0x01, 0x01, midbowl_shift_data_r },
	{ 0x02, 0x02, input_port_0_r },				/* dip switch */
	{ 0x03, 0x03, midbowl_shift_data_rev_r },
	{ 0x04, 0x04, input_port_1_r },				/* coins / switches */
	{ 0x05, 0x05, input_port_2_r },				/* ball vert */
	{ 0x06, 0x06, input_port_3_r },				/* ball horz */
	{ -1 }  /* end of table */
};

static struct IOWritePort midwbowl_writeport[] =
{
	{ 0x01, 0x01, invaders_shift_amount_w },
	{ 0x02, 0x02, invaders_shift_data_w },
	{ -1 }  /* end of table */
};

ROM_START( midwbowl_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "h.cpu",        0x0000, 0x0800, 0x74c29b93 )
	ROM_LOAD( "g.cpu",        0x0800, 0x0800, 0xca26d8b4 )
	ROM_LOAD( "f.cpu",        0x1000, 0x0800, 0xba8a0bfa )
	ROM_LOAD( "e.cpu",        0x1800, 0x0800, 0x4da65a40 )
	ROM_LOAD( "d.cpu",        0x4000, 0x0800, 0xe7dbc9d9 )
ROM_END

INPUT_PORTS_START( midwbowl_input_ports )
	PORT_START      /* IN2 */
	PORT_DIPNAME( 0x03, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x01, "French" )
	PORT_DIPSETTING(    0x02, "German" )
/* 0x03 same as 0x02 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x10, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* effects button 1 */
	PORT_DIPNAME( 0x80, 0x00, "Test", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off")
	PORT_DIPSETTING(    0x80, "On")

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN5 */
	PORT_ANALOG ( 0xff, 0, IPT_TRACKBALL_Y | IPF_REVERSE, 10, 3, 0, 0)

	PORT_START      /* IN6 */
	PORT_ANALOG ( 0xff, 0, IPT_TRACKBALL_X, 10, 3, 0, 0)
INPUT_PORTS_END

static struct MachineDriver midwbowl_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz */
			0,
			readmem, boothill_writemem, midwbowl_readport, midwbowl_writeport,
			invaders_interrupt, 2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
};



static int midbowl_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    /* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0x2213],"\xbb\x11",2) == 0 )
    {
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
		osd_fread(f,&RAM[0x22a0],3);
        osd_fclose(f);
        }
        return 1;
    }
    else
        return 0;  /* we can't load the hi scores yet */
}

static void midbowl_hisave(void)
{
    void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
        osd_fwrite(f,&RAM[0x22a0],3);
        osd_fclose(f);
		RAM[0x2213]=0;
    }
}


struct GameDriver midwbowl_driver =
{
	__FILE__,
	0,
	"bowler",
	"4 Player Bowling",
	"1978",
	"Midway",
	"The Space Invaders Team",
	0,
	&midwbowl_machine_driver,
	0,

	midwbowl_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	midwbowl_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_90,

	midbowl_hiload, midbowl_hisave
};

/*******************************************************/
/*                                                     */
/* Midway "Blue Shark"                                 */
/*                                                     */
/*******************************************************/


ROM_START( blueshrk_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "blueshrk.h",   0x0000, 0x0800, 0x4ff94187 )
	ROM_LOAD( "blueshrk.g",   0x0800, 0x0800, 0xe49368fd )
	ROM_LOAD( "blueshrk.f",   0x1000, 0x0800, 0x86cca79d )
ROM_END

static struct IOReadPort blueshrk_readport[] =
{
	{ 0x00, 0x00, blueshrk_shift_data_rev_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_0_r },
	{ 0x03, 0x03, blueshrk_shift_data_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort blueshrk_writeport[] =
{
	{ 0x01, 0x01, invaders_shift_amount_w },
	{ 0x02, 0x02, invaders_shift_data_w },
	{ 0x03, 0x03, invaders_dummy_write },
	{ 0x04, 0x04, invaders_dummy_write },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( blueshrk_input_ports )

	PORT_START      /* IN2 Dips & Coins */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x60, 0x20, "Replay", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "14000" )
	PORT_DIPSETTING(    0x40, "18000" )
	PORT_DIPSETTING(    0x60, "22000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x80, 0x80, "Test", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On")

	PORT_START      /* IN1 */
	PORT_ANALOG ( 0x7f, 0x45, IPT_PADDLE, 100, 0, 0xf, 0x7f)

INPUT_PORTS_END

static struct MachineDriver blueshrk_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,
			0,
			readmem, boothill_writemem, blueshrk_readport, blueshrk_writeport,
			invaders_interrupt, 2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	blueshrk_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
};

static int blueshrk_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
    /* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0x2450],"\xc3\x7e",2) == 0 )
	{
        void *f;
        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
			int hi;
			osd_fread(f,&RAM[0x200a],5);
            osd_fclose(f);

			hi = (RAM[0x200a] & 0x0f) +
				 (RAM[0x200b] & 0x0f) * 10 +
        		 (RAM[0x200c] & 0x0f) * 100 +
				 (RAM[0x200d] & 0x0f) * 1000 +
				 (RAM[0x200e] & 0x0f) * 10000 ;
			mix_hiscoreprint(21, 1, hi, 5,10, 6,0x800, 0x7fa, 2);

		}
        return 1;
    }
    else
        return 0;  /* we can't load the hi scores yet */
}

static void blueshrk_hisave(void)
{
    void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
        osd_fwrite(f,&RAM[0x200a],5);
        osd_fclose(f);
		RAM[0x2450]=0;
    }
}


struct GameDriver blueshrk_driver =
{
	__FILE__,
	0,
	"blueshrk",
	"Blue Shark",
	"1978",
	"Midway",
	"The Space Invaders Team",
	0,
	&blueshrk_machine_driver,
	0,

	blueshrk_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	blueshrk_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	blueshrk_hiload, blueshrk_hisave
};


/*******************************************************/
/*                                                     */
/* Midway "Extra Innings"                              */
/*                                                     */
/*******************************************************/


ROM_START( einnings_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "ei.h",         0x0000, 0x0800, 0xeff9c7af )
	ROM_LOAD( "ei.g",         0x0800, 0x0800, 0x5d1e66cb )
	ROM_LOAD( "ei.f",         0x1000, 0x0800, 0xed96785d )
	ROM_LOAD( "ei.e",         0x1800, 0x0800, 0xad096a5d )
	ROM_LOAD( "ei.b",         0x5000, 0x0800, 0x56b407d4 )
ROM_END

/*
 * The cocktail version has independent bat, pitch, and field controls
 * while the upright version ties the pairs of inputs together through
 * jumpers in the wiring harness.
 */
INPUT_PORTS_START( einnings_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )			/* home bat */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT )	/* home fielders left */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT)	/* home fielders right */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT)	/* home pitch left */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT)	/* home pitch right */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN)  /* home pitch slow */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP)    /* home pitch fast */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )			/* vistor bat */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT )	/* vistor fielders left */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT) 	/* visitor fielders right */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT)	/* visitor pitch left */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT)	/* visitor pitch right */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN)  /* visitor pitch slow */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP)    /* visitor pitch fast */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x07, 0x00, "Coins / Inning", IP_KEY_NONE)
	PORT_DIPSETTING(    0x02, "2 Coins/1 inning")
	PORT_DIPSETTING(    0x00, "1 Coin/1 inning")
	PORT_DIPSETTING(    0x01, "1 C/1 inning 2Cs/3 innings")
	PORT_DIPNAME( 0x40, 0x40, "Test", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "On")
	PORT_DIPSETTING(    0x40, "Off")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )
INPUT_PORTS_END

struct GameDriver einnings_driver =
{
	__FILE__,
	0,
	"einnings",
	"Extra Innings",
	"1978",
	"Midway",
	"The Space Invaders Team",
	0,
	&boothill_machine_driver,
	0,

	einnings_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	einnings_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};

/*******************************************************/
/*                                                     */
/* Midway "Double Play"                                */
/*                                                     */
/*******************************************************/


ROM_START( dplay_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "dplay619.h",   0x0000, 0x0800, 0x6680669b )
	ROM_LOAD( "dplay619.g",   0x0800, 0x0800, 0x0eec7e01 )
	ROM_LOAD( "dplay619.f",   0x1000, 0x0800, 0x3af4b719 )
	ROM_LOAD( "dplay619.e",   0x1800, 0x0800, 0x65cab4fc )
ROM_END


struct GameDriver dplay_driver =
{
	__FILE__,
	0,
	"dplay",
	"Double Play",
	"1977",
	"Midway",
	"The Space Invaders Team",
	0,
	&boothill_machine_driver,
	0,

	dplay_rom,
	0, 0,
	0, 0,      /* sound_prom */

	einnings_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};


/*******************************************************/
/*                                                     */
/* Midway "Amazing Maze"                               */
/*                                                     */
/*******************************************************/

ROM_START( maze_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "invaders.h",   0x0000, 0x0800, 0xf2860cff )
	ROM_LOAD( "invaders.g",   0x0800, 0x0800, 0x65fad839 )
ROM_END

INPUT_PORTS_START( maze_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1  )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END

static struct MachineDriver maze_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,boothill_writemem,invdelux_readport,writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0
};

/* L.T 11/3/1998 */
struct GameDriver maze_driver =
{
	__FILE__,
	0,
	"maze",
	"Amazing Maze",
	"1976",
	"Midway",
	"Space Invaders Team\nLee Taylor\n",
	0,
	&maze_machine_driver,
	0,

	maze_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	maze_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};

/*******************************************************/
/*                                                     */
/* Midway "Tornado Baseball"                           */
/*                                                     */
/*******************************************************/

ROM_START( tornbase_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "tb.h",         0x0000, 0x0800, 0x653f4797 )
	ROM_LOAD( "tb.g",         0x0800, 0x0800, 0x33468006 )
	ROM_LOAD( "tb.f",         0x1000, 0x0800, 0x215e070c )
ROM_END

INPUT_PORTS_START( tornbase_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_START      /* DSW0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x78, 0x40, "Coins / Rounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 / 1" )
	PORT_DIPSETTING(    0x08, "2 / 1" )
	PORT_DIPSETTING(    0x10, "3 / 1" )
	PORT_DIPSETTING(    0x18, "4 / 1" )
	PORT_DIPSETTING(    0x20, "1 / 2" )
	PORT_DIPSETTING(    0x28, "2 / 2" )
	PORT_DIPSETTING(    0x30, "3 / 2" )
	PORT_DIPSETTING(    0x38, "4 / 2" )
	PORT_DIPSETTING(    0x40, "1 / 4" )
	PORT_DIPSETTING(    0x48, "2 / 4" )
	PORT_DIPSETTING(    0x50, "3 / 4" )
	PORT_DIPSETTING(    0x58, "4 / 4" )
	PORT_DIPSETTING(    0x60, "1 / 9" )
	PORT_DIPSETTING(    0x68, "2 / 9" )
	PORT_DIPSETTING(    0x70, "3 / 9" )
	PORT_DIPSETTING(    0x78, "4 / 9" )
	PORT_DIPNAME( 0x80, 0x00, "Test Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END

static struct MachineDriver tornbase_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,boothill_writemem,invdelux_readport,writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
	invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0
};

/* LT 20-2-1998 */
struct GameDriver tornbase_driver =
{
	__FILE__,
	0,
	"tornbase",
	"Tornado Baseball",
	"1976",
	"Midway",
	"Lee Taylor\n",
	0,
	&tornbase_machine_driver,
	0,

	tornbase_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	tornbase_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};

/*******************************************************/
/*                                                     */
/* Midway "Checkmate"                                  */
/*                                                     */
/*******************************************************/

ROM_START( checkmat_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "checkmat.h", 0x0000, 0x0400, 0x3481a6d1 )
	ROM_LOAD( "checkmat.g", 0x0400, 0x0400, 0xdf5fa551 )
	ROM_LOAD( "checkmat.f", 0x0800, 0x0400, 0x25586406 )
	ROM_LOAD( "checkmat.e", 0x0c00, 0x0400, 0x59330d84 )
ROM_END

static struct IOReadPort checkmat_readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort checkmat_writeport[] =
{
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( checkmat_input_ports )
	PORT_START      /* IN0  */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )

	PORT_START      /* IN1  */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )

	PORT_START      /* IN2 Dips & Coins */
	PORT_DIPNAME( 0x01, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 or 2 Player" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 to 4 Players" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPNAME( 0x0c, 0x00, "Rounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPNAME( 0x60, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x20, "German" )
	PORT_DIPSETTING(    0x40, "French" )
	PORT_DIPSETTING(    0x60, "Spanish" )
	PORT_DIPNAME( 0x80, 0x00, "Test", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

    PORT_START       /* IN3  */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN1 )

INPUT_PORTS_END

static struct MachineDriver checkmat_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,        /* 2 Mhz? */
			0,
			readmem,boothill_writemem,checkmat_readport,checkmat_writeport,
			invaders_interrupt,2    /* two interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	invaders_vh_start,
	invaders_vh_stop,
    invaders_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

struct GameDriver checkmat_driver =
{
	__FILE__,
	0,
	"checkmat",
	"Checkmate",
	"1977",
	"Midway",
	"The Space Invaders Team\n",
	0,
	&checkmat_machine_driver,
	0,

	checkmat_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	checkmat_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

/*******************************************************/
/*                                                     */
/* Taito "Ozma Wars"                                   */
/*                                                     */
/*******************************************************/

ROM_START( ozmawars_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "mw01", 0x0000, 0x0800, 0x31f4397d )
	ROM_LOAD( "mw02", 0x0800, 0x0800, 0xd8e77c62 )
	ROM_LOAD( "mw03", 0x1000, 0x0800, 0x3bfa418f )
	ROM_LOAD( "mw04", 0x1800, 0x0800, 0xe190ce6c )
	ROM_LOAD( "mw05", 0x4000, 0x0800, 0x3bc7d4c7 )
	ROM_LOAD( "mw06", 0x4800, 0x0800, 0x99ca2eae )
ROM_END

INPUT_PORTS_START( ozmawars_input_ports )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* N ? */
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START		/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Preset Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x80, 0x00, "Coin Info", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_START		/* BSR */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
INPUT_PORTS_END

struct GameDriver ozmawars_driver =
{
	__FILE__,
	0,
	"ozmawars",
	"Ozma Wars",
	"1979",
	"SNK",
	"The Space Invaders Team",
	0,
	&invadpt2_machine_driver,
	0,

	ozmawars_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	ozmawars_input_ports,

	0,palette, 0,
	ORIENTATION_ROTATE_270,
	spaceph_hiload, spaceph_hisave

};


/*******************************************************/
/*                                                     */
/* Emag "Super Invaders"                               */
/*                                                     */
/*******************************************************/

INPUT_PORTS_START( sinvemag_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x08, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1500" )
	PORT_DIPSETTING(    0x08, "1000" )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_START		/* BSR */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START		/* Dummy port for cocktail mode */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
INPUT_PORTS_END

ROM_START( sinvemag_rom )
	ROM_REGION(0x10000)     /* 64k for code */
        ROM_LOAD( "emag_si.a",   0x0000, 0x0400, 0x86bb8cb6 )
        ROM_LOAD( "emag_si.b",   0x0400, 0x0400, 0xfebe6d1a )
        ROM_LOAD( "emag_si.c",   0x0800, 0x0400, 0xaafb24f7 )
        ROM_LOAD( "emag_si.d",   0x1400, 0x0400, 0x68c4b9da )
        ROM_LOAD( "emag_si.e",   0x1800, 0x0400, 0xc4e80586 )
        ROM_LOAD( "emag_si.f",   0x1c00, 0x0400, 0x077f5ef2 )
ROM_END

struct GameDriver sinvemag_driver =
{
	__FILE__,
	&invaders_driver,
	"sinvemag",
	"Super Invaders",
	"????",
	"bootleg",
	"Michael Strutts (Space Invaders emulator)\nNicola Salmoria\nTormod Tjaberg (sound)\nMirko Buffoni\nValerio Verrando\nMarco Cassili",
	0,
	&machine_driver,
	0,

	sinvemag_rom,
	0, 0,
	invaders_sample_names,
	0,      /* sound_prom */

	sinvemag_input_ports,

	0, palette, 0,
	ORIENTATION_ROTATE_270,

	invaders_hiload, invaders_hisave
};
