/****************************************************************************
Traverse USA Driver

Lee Taylor
John Clegg

Loosely based on the our previous 10 Yard Fight driver.

****************************************************************************/

#include "driver.h"
#include "Z80/Z80.h"
#include "vidhrdw/generic.h"


extern unsigned char *travrusa_scroll_x_low;
extern unsigned char *travrusa_scroll_x_high;

void travrusa_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int travrusa_vh_start(void);
void travrusa_vh_stop(void);
void travrusa_flipscreen_w(int offset,int data);
void travrusa_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern struct AY8910interface irem_ay8910_interface;
extern struct MSM5205interface irem_msm5205_interface;
void irem_io_w(int offset, int value);
int irem_io_r(int offset);
void irem_sound_cmd_w(int offset, int value);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },        /* Video and Color ram */
	{ 0xd000, 0xd000, input_port_0_r },	/* IN0 */
	{ 0xd001, 0xd001, input_port_1_r },	/* IN1 */
	{ 0xd002, 0xd002, input_port_2_r },	/* IN2 */
	{ 0xd003, 0xd003, input_port_3_r },	/* DSW1 */
	{ 0xd004, 0xd004, input_port_4_r },	/* DSW2 */
	{ 0xe000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, videoram_w, &videoram, &videoram_size },
	{ 0x9000, 0x9000, MWA_RAM, &travrusa_scroll_x_low },
	{ 0xa000, 0xa000, MWA_RAM, &travrusa_scroll_x_high },
	{ 0xc800, 0xc9ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd000, 0xd000, irem_sound_cmd_w },
	{ 0xd001, 0xd001, travrusa_flipscreen_w },
	{ 0xe000, 0xefff, MWA_RAM },
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
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	/* coin input must be active for 19 frames to be consistently recognized */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
		"Coin", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 19 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1, "Accelerate", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON2, "Brake", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x02, "Fuel Reduced on Collision", IP_KEY_NONE )
	PORT_DIPSETTING( 0x03, "Low" )
	PORT_DIPSETTING( 0x02, "Med" )
	PORT_DIPSETTING( 0x01, "Hi" )
	PORT_DIPSETTING( 0x00, "Max" )
	PORT_DIPNAME( 0x04, 0x04, "Fuel Consumption", IP_KEY_NONE )
	PORT_DIPSETTING( 0x04, "Low" )
	PORT_DIPSETTING( 0x00, "Hi" )
	PORT_DIPNAME( 0x08, 0x00, "Continue", IP_KEY_NONE )
	PORT_DIPSETTING( 0x08, "No" )
	PORT_DIPSETTING( 0x00, "Yes" )
	PORT_DIPNAME( 0xf0, 0xf0, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING( 0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING( 0xe0, "2 Coins/1 Credit" )
	PORT_DIPSETTING( 0xd0, "3 Coins/1 Credit" )
	PORT_DIPSETTING( 0xc0, "4 Coins/1 Credit" )
	PORT_DIPSETTING( 0xb0, "5 Coins/1 Credit" )
	PORT_DIPSETTING( 0xa0, "6 Coins/1 Credit" )
	PORT_DIPSETTING( 0x70, "1 Coin/2 Credit" )
	PORT_DIPSETTING( 0x60, "1 Coins/3 Credit" )
	PORT_DIPSETTING( 0x50, "3 Coins/4 Credit" )
	PORT_DIPSETTING( 0x40, "4 Coins/5 Credit" )
	PORT_DIPSETTING( 0x30, "5 Coins/6 Credit" )
	PORT_DIPSETTING( 0x20, "UNKNOWN" )

	/* PORT_DIPSETTING( 0x10, "INVALID" ) */
	/* PORT_DIPSETTING( 0x00, "INVALID" ) */

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING( 0x00, "Upright" )
	PORT_DIPSETTING( 0x02, "Cocktail" )
	PORT_DIPNAME( 0x04, 0x04, "Coin Shute B", IP_KEY_NONE )
	PORT_DIPSETTING( 0x04, "Off" )
	PORT_DIPSETTING( 0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Speed Type", IP_KEY_NONE )
	PORT_DIPSETTING( 0x08, "M/H" )
	PORT_DIPSETTING( 0x00, "KM/H" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "SW6B", IP_KEY_NONE )
	PORT_DIPSETTING( 0x20, "Off" )
	PORT_DIPSETTING( 0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Invulnerability", IP_KEY_NONE )
	PORT_DIPSETTING( 0x40, "Off" )
	PORT_DIPSETTING( 0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
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
	{ 2*256*16*16, 256*16*16, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,      0, 16 },
	{ 1, 0x06000, &spritelayout, 16*8, 16 },
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
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_M6803 | CPU_AUDIO_CPU,
			2000000,	/* 1.0 Mhz ? */
			3,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are generated by the ADPCM hardware */
		}
	},
	57, 1790,	/* accurate frequency, measured on a Moon Patrol board, is 56.75Hz. */
				/* the Lode Runner manual (similar but different hardware) */
				/* talks about 55Hz and 1790ms vblank duration. */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	128+32, 16*8+16*8,
	travrusa_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	travrusa_vh_start,
	travrusa_vh_stop,
	travrusa_vh_screenrefresh,

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


ROM_START( travrusa_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "zippyrac.000", 0x0000, 0x2000, 0xbe066c0a )
	ROM_LOAD( "zippyrac.005", 0x2000, 0x2000, 0x145d6b34 )
	ROM_LOAD( "zippyrac.006", 0x4000, 0x2000, 0xe1b51383 )
	ROM_LOAD( "zippyrac.007", 0x6000, 0x2000, 0x85cd1a51 )

	ROM_REGION_DISPOSE(0x12000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "zippyrac.001", 0x00000, 0x2000, 0xaa8994dd )
	ROM_LOAD( "mr8.3c",       0x02000, 0x2000, 0x3a046dd1 )
	ROM_LOAD( "mr9.3a",       0x04000, 0x2000, 0x1cc3d3f4 )
	ROM_LOAD( "zippyrac.008", 0x06000, 0x2000, 0x3e2c7a6b )
	ROM_LOAD( "zippyrac.009", 0x08000, 0x2000, 0x13be6a14 )
	ROM_LOAD( "zippyrac.010", 0x0a000, 0x2000, 0x6fcc9fdb )

	ROM_REGION(0x0220)	/* color proms */
	ROM_LOAD( "mmi6349.ij",   0x0000, 0x0200, 0xc9724350 ) /* character palette - last $100 are unused */
	ROM_LOAD( "tbp18s.2",     0x0100, 0x0020, 0xa1130007 ) /* sprite palette */
	ROM_LOAD( "tbp24s10.3",   0x0120, 0x0100, 0x76062638 ) /* sprite lookup table */

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "mr10.1a",      0xf000, 0x1000, 0xa02ad8a0 )
ROM_END

ROM_START( motorace_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "mr.cpu",       0x10000, 0x2000, 0x89030b0c )	/* we load the ROM at 10000-11fff, */
														/* it will be decrypted at 0000 */
	ROM_LOAD( "mr1.3l",       0x2000, 0x2000, 0x0904ed58 )
	ROM_LOAD( "mr2.3k",       0x4000, 0x2000, 0x8a2374ec )
	ROM_LOAD( "mr3.3j",       0x6000, 0x2000, 0x2f04c341 )

	ROM_REGION_DISPOSE(0x12000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mr7.3e",       0x00000, 0x2000, 0x492a60be )
	ROM_LOAD( "mr8.3c",       0x02000, 0x2000, 0x3a046dd1 )
	ROM_LOAD( "mr9.3a",       0x04000, 0x2000, 0x1cc3d3f4 )
	ROM_LOAD( "mr4.3n",       0x06000, 0x2000, 0x5cf1a0d6 )
	ROM_LOAD( "mr5.3m",       0x08000, 0x2000, 0xf75f2aad )
	ROM_LOAD( "mr6.3k",       0x0a000, 0x2000, 0x518889a0 )

	ROM_REGION(0x0220)	/* color proms */
	ROM_LOAD( "mmi6349.ij",   0x0000, 0x0200, 0xc9724350 ) /* character palette - last $100 are unused */
	ROM_LOAD( "tbp18s.2",     0x0100, 0x0020, 0xa1130007 ) /* sprite palette */
	ROM_LOAD( "tbp24s10.3",   0x0120, 0x0100, 0x76062638 ) /* sprite lookup table */

	ROM_REGION(0x10000)	/* 64k for sound cpu */
	ROM_LOAD( "mr10.1a",      0xf000, 0x1000, 0xa02ad8a0 )
ROM_END



void motorace_decode(void)
{
	int A,i,j;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* The first CPU ROM has the address and data lines scrambled */
	for (A = 0;A < 0x2000;A++)
	{
		int bit[13];


		for (i = 0;i < 13;i++)
			bit[i] = (A >> i) & 1;

		j =
			(bit[11] <<  0) +
			(bit[ 0] <<  1) +
			(bit[ 2] <<  2) +
			(bit[ 4] <<  3) +
			(bit[ 6] <<  4) +
			(bit[ 8] <<  5) +
			(bit[10] <<  6) +
			(bit[12] <<  7) +
			(bit[ 1] <<  8) +
			(bit[ 3] <<  9) +
			(bit[ 5] << 10) +
			(bit[ 7] << 11) +
			(bit[ 9] << 12);

		for (i = 0;i < 8;i++)
			bit[i] = (RAM[A + 0x10000] >> i) & 1;

		RAM[j] =
			(bit[5] << 0) +
			(bit[0] << 1) +
			(bit[3] << 2) +
			(bit[6] << 3) +
			(bit[1] << 4) +
			(bit[4] << 5) +
			(bit[7] << 6) +
			(bit[2] << 7);
	}
}



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xe080],"\x49\x45\x4f",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xe07c],74);
			RAM[0xE008] = RAM[0xE07C];
			RAM[0xE009] = RAM[0xE07D];
			RAM[0xE00A] = RAM[0xE07E];

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
		osd_fwrite(f,&RAM[0xe07c],74);
		osd_fclose(f);
	}
	memset(&RAM[0xe07c], 0, 74);	/* LT 13-11-97 */
}



struct GameDriver travrusa_driver =
{
	__FILE__,
	0,
	"travrusa",
	"Traverse USA",
	"1983",
	"Irem",
	"Lee Taylor (Driver Code)\nJohn Clegg (Graphics Code)\nAaron Giles (sound)\nThierry Lescot (color info)",
	0,
	&machine_driver,
	0,

	travrusa_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver motorace_driver =
{
	__FILE__,
	&travrusa_driver,
	"motorace",
	"MotoRace USA",
	"1983",
	"Irem (Williams license)",
	"Lee Taylor (Driver Code)\nJohn Clegg (Graphics Code)\nAaron Giles (sound)\nThierry Lescot (color info)\nGerald Vanderick (color info)",
	0,
	&machine_driver,
	0,

	motorace_rom,
	motorace_decode, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};
