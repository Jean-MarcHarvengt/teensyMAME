/***************************************************************************

Naughty Boy driver by Sal and John Bugliarisi.
This driver is based largely on MAME's Phoenix driver, since Naughty Boy runs
on similar hardware as Phoenix. Phoenix driver provided by Brad Oliver.
Thanks to Richard Davies for his Phoenix emulator source.


Naughty Boy memory map

0000-3fff 16Kb Program ROM
4000-7fff 1Kb Work RAM (mirrored)
8000-87ff 2Kb Video RAM Charset A (lower priority, mirrored)
8800-8fff 2Kb Video RAM Charset b (higher priority, mirrored)
9000-97ff 2Kb Video Control write-only (mirrored)
9800-9fff 2Kb Video Scroll Register (mirrored)
a000-a7ff 2Kb Sound Control A (mirrored)
a800-afff 2Kb Sound Control B (mirrored)
b000-b7ff 2Kb 8bit Game Control read-only (mirrored)
b800-bfff 1Kb 8bit Dip Switch read-only (mirrored)
c000-0000 16Kb Unused

memory mapped ports:

read-only:
b000-b7ff IN
b800-bfff DSW


Naughty Boy Switch Settings
(C)1982 Cinematronics

 --------------------------------------------------------
|Option |Factory|Descrpt| 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
 ------------------------|-------------------------------
|Lives  |       |2      |on |on |   |   |   |   |   |   |
 ------------------------ -------------------------------
|       |   X   |3      |off|on |   |   |   |   |   |   |
 ------------------------ -------------------------------
|       |       |4      |on |off|   |   |   |   |   |   |
 ------------------------ -------------------------------
|       |       |5      |off|off|   |   |   |   |   |   |
 ------------------------ -------------------------------
|Extra  |       |10000  |   |   |on |on |   |   |   |   |
 ------------------------ -------------------------------
|       |   X   |30000  |   |   |off|on |   |   |   |   |
 ------------------------ -------------------------------
|       |       |50000  |   |   |on |off|   |   |   |   |
 ------------------------ -------------------------------
|       |       |70000  |   |   |off|off|   |   |   |   |
 ------------------------ -------------------------------
|Credits|       |2c, 1p |   |   |   |   |on |on |   |   |
 ------------------------ -------------------------------
|       |   X   |1c, 1p |   |   |   |   |off|on |   |   |
 ------------------------ -------------------------------
|       |       |1c, 2p |   |   |   |   |on |off|   |   |
 ------------------------ -------------------------------
|       |       |4c, 3p |   |   |   |   |off|off|   |   |
 ------------------------ -------------------------------
|Dffclty|   X   |Easier |   |   |   |   |   |   |on |   |
 ------------------------ -------------------------------
|       |       |Harder |   |   |   |   |   |   |off|   |
 ------------------------ -------------------------------
| Type  |       |Upright|   |   |   |   |   |   |   |on |
 ------------------------ -------------------------------
|       |       |Cktail |   |   |   |   |   |   |   |off|
 ------------------------ -------------------------------

*
* Pop Flamer
*

Pop Flamer appears to run on identical hardware as Naughty Boy.
The dipswitches are even identical. Spooky.

                        1   2   3   4   5   6   7   8
-------------------------------------------------------
Number of Mr. Mouse 2 |ON |ON |   |   |   |   |   |   |
                    3 |OFF|ON |   |   |   |   |   |   |
                    4 |ON |OFF|   |   |   |   |   |   |
                    5 |OFF|OFF|   |   |   |   |   |   |
-------------------------------------------------------
Extra Mouse    10,000 |   |   |ON |ON |   |   |   |   |
               30,000 |   |   |OFF|ON |   |   |   |   |
               50,000 |   |   |ON |OFF|   |   |   |   |
               70,000 |   |   |OFF|OFF|   |   |   |   |
-------------------------------------------------------
Credit  2 coin 1 play |   |   |   |   |ON |ON |   |   |
        1 coin 1 play |   |   |   |   |OFF|ON |   |   |
        1 coin 2 play |   |   |   |   |ON |OFF|   |   |
        1 coin 3 play |   |   |   |   |OFF|OFF|   |   |
-------------------------------------------------------
Skill          Easier |   |   |   |   |   |   |ON |   |
               Harder |   |   |   |   |   |   |OFF|   |
-------------------------------------------------------
Game style      Table |   |   |   |   |   |   |   |OFF|
              Upright |   |   |   |   |   |   |   |ON |


 ***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *naughtyb_videoram2;
extern unsigned char *naughtyb_scrollreg;

void naughtyb_videoram2_w(int offset,int data);
void naughtyb_scrollreg_w (int offset,int data);
void naughtyb_videoreg_w (int offset,int data);
void popflame_videoreg_w (int offset,int data);
int naughtyb_vh_start(void);
void naughtyb_vh_stop(void);
void naughtyb_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void naughtyb_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

// Let's skip the sound for now.. ;)
// void naughtyb_sound_control_a_w(int offset, int data);
// void naughtyb_sound_control_b_w(int offset, int data);
// int naughtyb_sh_start(void);
// void naughtyb_sh_update(void);

void phoenix_sound_control_a_w(int offset, int data);
void phoenix_sound_control_b_w(int offset, int data);
int phoenix_sh_start(void);
void phoenix_sh_update(void);
void pleiads_sound_control_a_w(int offset, int data);
void pleiads_sound_control_b_w(int offset, int data);
int pleiads_sh_start(void);
void pleiads_sh_update(void);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x8fff, MRA_RAM },
	{ 0xb000, 0xb7ff, input_port_0_r },     /* IN0 */
	{ 0xb800, 0xbfff, input_port_1_r },     /* DSW */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x7fff, MWA_RAM },
	{ 0x8000, 0x87ff, videoram_w, &videoram, &videoram_size },
	{ 0x8800, 0x8fff, naughtyb_videoram2_w, &naughtyb_videoram2 },
	{ 0x9000, 0x97ff, naughtyb_videoreg_w },
	{ 0x9800, 0x9fff, MWA_RAM, &naughtyb_scrollreg },
	{ 0xa000, 0xa7ff, pleiads_sound_control_a_w },
	{ 0xa800, 0xafff, pleiads_sound_control_b_w },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress popflame_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x7fff, MWA_RAM },
	{ 0x8000, 0x87ff, videoram_w, &videoram, &videoram_size },
	{ 0x8800, 0x8fff, naughtyb_videoram2_w, &naughtyb_videoram2 },
	{ 0x9000, 0x97ff, popflame_videoreg_w },
	{ 0x9800, 0x9fff, MWA_RAM, &naughtyb_scrollreg },
	{ 0xa000, 0xa7ff, pleiads_sound_control_a_w },
	{ 0xa800, 0xafff, pleiads_sound_control_b_w },
	{ -1 }  /* end of table */
};


/***************************************************************************

  Naughty Boy doesn't have VBlank interrupts.
  Interrupts are still used by the game: but they are related to coin
  slots.

***************************************************************************/
int naughtyb_interrupt(void)
{
	if (readinputport(2) & 1)
		return nmi_interrupt();
	else return ignore_interrupt();
}

INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )

	PORT_START	/* DSW0 & VBLANK */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x04, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x04, "30000" )
	PORT_DIPSETTING(    0x08, "50000" )
	PORT_DIPSETTING(    0x0c, "70000" )
	PORT_DIPNAME( 0x30, 0x10, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x20, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x40, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )
	/* This is a bit of a mystery. Bit 0x80 is read as the vblank, but
	   it apparently also controls cocktail/table mode. */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START	/* FAKE */
	/* The coin slots are not memory mapped. */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IPF_IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	2,      /* 2 bits per pixel */
	{ 512*8*8, 0 }, /* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },     /* pretty straightforward layout */
	8*8     /* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,    0, 32 },
	{ 1, 0x2000, &charlayout, 32*4, 32 },
	{ -1 } /* end of array */
};


/* LBO - I'll leave these here since the existing PROM was generated from it and */
/* it's been hand-modified from the original dump */
static unsigned char naughtyb_color_prom[] =
{
	/* 633 - palette low bits */
	/* changed one byte to fix the title screen */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,/*0x06*/0x05,0x02,0x05,0x05,0x05,0x02,0x02,0x02,
	0x02,0x01,0x06,0x06,0x06,0x01,0x01,0x01,0x01,0x04,0x03,0x03,0x03,0x04,0x04,0x04,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x01,0x01,0x01,0x03,0x03,0x03,0x03,
	0x01,0x04,0x06,0x06,0x05,0x01,0x01,0x01,0x07,0x07,0x01,0x01,0x01,0x05,0x05,0x05,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x01,0x01,0x01,0x01,0x01,0x01,
	0x04,0x01,0x02,0x02,0x02,0x02,0x02,0x02,0x01,0x04,0x07,0x07,0x07,0x07,0x07,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x01,0x07,0x07,0x03,0x03,0x03,0x03,
	0x01,0x04,0x01,0x01,0x05,0x01,0x01,0x01,0x07,0x07,0x01,0x01,0x01,0x05,0x05,0x05,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,
	0x02,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x01,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x05,0x03,0x03,0x03,0x01,0x01,
	0x04,0x04,0x06,0x05,0x05,0x01,0x04,0x02,0x07,0x07,0x01,0x06,0x06,0x05,0x02,0x03,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x04,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x01,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x05,0x01,0x01,0x03,0x01,0x01,
	0x04,0x04,0x06,0x02,0x02,0x01,0x04,0x02,0x07,0x07,0x01,0x04,0x04,0x05,0x02,0x03,
	/* h4 - palette high bits */
	/* changed one byte to fix the title screen */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,/*0x06*/0x05,0x06,0x05,0x05,0x05,0x03,0x03,0x03,
	0x02,0x05,0x06,0x06,0x06,0x01,0x01,0x01,0x01,0x04,0x03,0x03,0x03,0x04,0x04,0x04,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x01,0x07,0x07,0x03,0x03,0x03,0x03,
	0x01,0x04,0x07,0x07,0x05,0x07,0x07,0x07,0x07,0x07,0x01,0x01,0x01,0x05,0x05,0x05,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x06,0x01,0x01,0x01,0x01,0x01,0x01,
	0x04,0x05,0x02,0x02,0x02,0x02,0x02,0x02,0x01,0x05,0x07,0x07,0x07,0x07,0x07,0x07,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x01,0x07,0x07,0x03,0x03,0x03,0x03,
	0x01,0x04,0x01,0x01,0x05,0x07,0x07,0x07,0x07,0x07,0x01,0x01,0x01,0x05,0x05,0x05,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,
	0x02,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x01,0x06,0x06,0x06,0x06,0x06,0x06,0x06,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x05,0x03,0x03,0x03,0x05,0x01,
	0x04,0x04,0x07,0x05,0x05,0x07,0x05,0x02,0x07,0x07,0x01,0x06,0x06,0x05,0x03,0x03,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x04,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x01,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x05,0x01,0x01,0x03,0x05,0x01,
	0x04,0x04,0x07,0x02,0x02,0x07,0x05,0x02,0x07,0x07,0x01,0x04,0x04,0x05,0x03,0x03
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1500000,	/* 3 Mhz ? */
			0,
			readmem,writemem,0,0,
			naughtyb_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	28*8, 36*8, { 0*8, 28*8-1, 0*8, 36*8-1 },
	gfxdecodeinfo,
	256,32*4+32*4,
	naughtyb_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	naughtyb_vh_start,
	naughtyb_vh_stop,
	naughtyb_vh_screenrefresh,

	/* sound hardware */
	0,
	pleiads_sh_start,
	0,
	pleiads_sh_update
};

/* Exactly the same but for the writemem handler */
static struct MachineDriver popflame_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1500000,	/* 3 Mhz ? */
			0,
			readmem,popflame_writemem,0,0,
			naughtyb_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	28*8, 36*8, { 0*8, 28*8-1, 0*8, 36*8-1 },
	gfxdecodeinfo,
	256,32*4+32*4,
	naughtyb_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	naughtyb_vh_start,
	naughtyb_vh_stop,
	naughtyb_vh_screenrefresh,

	/* sound hardware */
	0,
	pleiads_sh_start,
	0,
	pleiads_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( naughtyb_rom )
	ROM_REGION(0x10000)      /* 64k for code */
	ROM_LOAD( "nb1ic30",      0x0000, 0x0800, 0x3f482fa3 )
	ROM_LOAD( "nb2ic29",      0x0800, 0x0800, 0x7ddea141 )
	ROM_LOAD( "nb3ic28",      0x1000, 0x0800, 0x8c72a069 )
	ROM_LOAD( "nb4ic27",      0x1800, 0x0800, 0x30feae51 )
	ROM_LOAD( "nb5ic26",      0x2000, 0x0800, 0x05242fd0 )
	ROM_LOAD( "nb6ic25",      0x2800, 0x0800, 0x7a12ffea )
	ROM_LOAD( "nb7ic24",      0x3000, 0x0800, 0x9cc287df )
	ROM_LOAD( "nb8ic23",      0x3800, 0x0800, 0x4d84ff2c )

	ROM_REGION_DISPOSE(0x4000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "nb15ic44",     0x0000, 0x0800, 0xd692f9c7 )
	ROM_LOAD( "nb16ic43",     0x0800, 0x0800, 0xd3ba8b27 )
	ROM_LOAD( "nb13ic46",     0x1000, 0x0800, 0xc1669cd5 )
	ROM_LOAD( "nb14ic45",     0x1800, 0x0800, 0xeef2c8e5 )
	ROM_LOAD( "nb11ic48",     0x2000, 0x0800, 0x23271a13 )
	ROM_LOAD( "nb12ic47",     0x2800, 0x0800, 0xef0706c3 )
	ROM_LOAD( "nb9ic50",      0x3000, 0x0800, 0xd6949c27 )
	ROM_LOAD( "nb10ic49",     0x3800, 0x0800, 0xc97c97b9 )

	ROM_REGION(0x0200)      /* color proms */
	ROM_LOAD( "naughtyb.633", 0x0000, 0x0100, 0x7838b973 ) /* palette low bits */
	ROM_LOAD( "naughtyb.h4",  0x0100, 0x0100, 0x70043706 ) /* palette high bits */
ROM_END

ROM_START( popflame_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic86.pop",     0x0000, 0x1000, 0x5e32bbdf )
	ROM_LOAD( "ic80.pop",     0x1000, 0x1000, 0xb77abf3d )
	ROM_LOAD( "ic94.pop",     0x2000, 0x1000, 0x945a3c0f )
	ROM_LOAD( "ic100.pop",    0x3000, 0x1000, 0xf9f2343b )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic13.pop",     0x0000, 0x1000, 0x2367131e )
	ROM_LOAD( "ic3.pop",      0x1000, 0x1000, 0xdeed0a8b )
	ROM_LOAD( "ic29.pop",     0x2000, 0x1000, 0x7b54f60f )
	ROM_LOAD( "ic38.pop",     0x3000, 0x1000, 0xdd2d9601 )

	ROM_REGION(0x0200)      /* color proms */
	ROM_LOAD( "ic53",         0x0000, 0x0100, 0x6e66057f ) /* palette low bits */
	ROM_LOAD( "ic54",         0x0100, 0x0100, 0x236bc771 ) /* palette high bits */
ROM_END



static int naughtyb_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score has already been written to screen */
	if((RAM[0x874a] == 8) && (RAM[0x8746] == 9) && (RAM[0x8742] == 7) && (RAM[0x873e] == 8) &&  /* HIGH */
	   (RAM[0x8743] == 0x20) && (RAM[0x873f] == 0x20) && (RAM[0x873b] == 0x20) && (RAM[0x8737] == 0x20))
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			char buf[10];
			int hi;

			osd_fread(f,&RAM[0x4003],4);

			/* also copy the high score to the screen, otherwise it won't be */
			/* updated */
			hi = (RAM[0x4006] & 0x0f) +
			     (RAM[0x4006] >> 4) * 10 +
			     (RAM[0x4005] & 0x0f) * 100 +
			     (RAM[0x4005] >> 4) * 1000 +
			     (RAM[0x4004] & 0x0f) * 10000 +
			     (RAM[0x4004] >> 4) * 100000 +
			     (RAM[0x4003] & 0x0f) * 1000000 +
			     (RAM[0x4003] >> 4) * 10000000;
			if (hi)
			{
				sprintf(buf,"%8d",hi);
				if (buf[2] != ' ') videoram_w(0x0743,buf[2]-'0'+0x20);
				if (buf[3] != ' ') videoram_w(0x073f,buf[3]-'0'+0x20);
				if (buf[4] != ' ') videoram_w(0x073b,buf[4]-'0'+0x20);
				if (buf[5] != ' ') videoram_w(0x0737,buf[5]-'0'+0x20);
				if (buf[6] != ' ') videoram_w(0x0733,buf[6]-'0'+0x20);
				if (buf[7] != ' ') videoram_w(0x072f,buf[7]-'0'+0x20);
			}
			osd_fclose(f);
		}

		return 1;
	}
	else return 0; /* we can't load the hi scores yet */
}

static unsigned long get_score(unsigned char *score)
{
   return (score[3])+(154*score[2])+((unsigned long)(39322)*score[1])+((unsigned long)(39322)*154*score[0]);
}

static void naughtyb_hisave(void)
{
	unsigned long score1,score2,hiscore;
	void *f;

	unsigned char *RAM = Machine->memory_region[0];

	score1 = get_score(&RAM[0x4020]);
	score2 = get_score(&RAM[0x4030]);
	hiscore = get_score(&RAM[0x4003]);

	if (score1 > hiscore) RAM += 0x4020;
	else if (score2 > hiscore) RAM += 0x4030;
	else RAM += 0x4003;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0],4);
		osd_fclose(f);
	}
}



static int popflame_hiload (void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score has already been written to screen */
	if ((RAM[0x874a] == 8) && (RAM[0x8746] == 9) && (RAM[0x8742] == 7) &&
	    (RAM[0x873e] == 8) &&  /* HIGH */
	    (RAM[0x8743] == 0x20) && (RAM[0x873f] == 0x20) && (RAM[0x873b] == 0x20) &&
	    (RAM[0x8737] == 0x20)) /* 0000 */
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread (f,&RAM[0x4004],3);
			osd_fclose (f);

			videoram_w (0x743, (RAM[0x4004] >> 4) | 0x20);
			videoram_w (0x73f, (RAM[0x4004] & 0x0f) | 0x20);
			videoram_w (0x73b, (RAM[0x4005] >> 4) | 0x20);
			videoram_w (0x737, (RAM[0x4005] & 0x0f) | 0x20);
			videoram_w (0x733, (RAM[0x4006] >> 4) | 0x20);
			videoram_w (0x72f, (RAM[0x4006] & 0x0f) | 0x20);
		}

		return 1;
	}
	else return 0; /* we can't load the hi scores yet */
}

static void popflame_hisave (void)
{
	unsigned long score1,score2,hiscore;
	void *f;

	unsigned char *RAM = Machine->memory_region[0];

	score1 = get_score (&RAM[0x4021]);
	score2 = get_score (&RAM[0x4031]);
	hiscore = get_score (&RAM[0x4004]);

	if (score1 > hiscore) RAM += 0x4021;
	else if (score2 > hiscore) RAM += 0x4031;
	else RAM += 0x4004;

	if ((f = osd_fopen (Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite (f,&RAM[0],3);
		osd_fclose (f);
	}
}



struct GameDriver naughtyb_driver =
{
	__FILE__,
	0,
	"naughtyb",
	"Naughty Boy",
	"1982",
	"Jaleco (Cinematronics license)",
	"Sal and John Bugliarisi (MAME driver)\nMirko Buffoni (additional code)\nNicola Salmoria (additional code)\nAlan J. McCormick (color info)",
	0,
	&machine_driver,
	0,

	naughtyb_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	naughtyb_hiload, naughtyb_hisave
};

struct GameDriver popflame_driver =
{
	__FILE__,
	0,
	"popflame",
	"Pop Flamer",
	"1982",
	"Jaleco",
	"Brad Oliver (MAME driver)\nSal and John Bugliarisi (Naughty Boy driver)\nMirko Buffoni (additional code)\nNicola Salmoria (additional code)\nTim Lindquist (color info)",
	0,
	&popflame_machine_driver,
	0,

	popflame_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	popflame_hiload, popflame_hisave
};
