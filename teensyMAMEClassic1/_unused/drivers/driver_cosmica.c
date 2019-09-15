/*************************************************************************
 Universal Cosmic Alien Driver
 (c)Lee Taylor May/June 1998, All rights reserved.

 For use only in offical Mame releases.
 Not to be distrabuted as part of any commerical work.
**************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"




extern unsigned char *cosmicalien_videoram;

void cosmicalien_flipx_w(int offset,int data);
void cosmicalien_flipy_w(int offset,int data);
void cosmicalien_attributes_w(int offset,int data);
void cosmicalien_stars_w(int offset,int data);
int  cosmicalien_vh_start(void);
void cosmicalien_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void cosmicalien_vh_stop(void);


void cosmicalien_videoram_w(int offset,int data);

static struct MemoryReadAddress cosmicalien_readmem[] =
{
        { 0x5f00, 0x5fff, MRA_RAM},
        { 0x4000, 0x5c10, MRA_RAM},
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x6800, 0x6800, input_port_0_r },	/* IN0 */
	{ 0x6801, 0x6801, input_port_1_r },	/* IN0 */
	{ 0x6802, 0x6802, input_port_2_r },	/* IN1 */
	{ 0x6803, 0x6803, input_port_3_r },	/* DSW */
	{ -1 }	/* end of table */
};


static struct MemoryWriteAddress cosmicalien_writemem[] =
{
        { 0x5F00, 0x5fff, MWA_RAM},
	{ 0x4400, 0x5c00, cosmicalien_videoram_w, &cosmicalien_videoram},
        { 0x4000, 0x44ff, MWA_RAM},
        { 0x5c00, 0x5c10, MWA_RAM},
	{ 0x6000, 0x601f, MWA_RAM, &spriteram, &spriteram_size },
        { 0x7000, 0x7008, MWA_RAM}, /* Sound???? */
	{ -1 }	/* end of table */
};



int cosmicalien_interrupt(void)
{
	if (readinputport(4) & 1)	/* Left Coin */
		return nmi_interrupt();
	else return ignore_interrupt();
}


INPUT_PORTS_START( cosmicalien_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x02, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x0c, 0x00, "Coins", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin 1 Play" )
	PORT_DIPSETTING(    0x08, "1 Coin 2 Plays" )
	PORT_DIPSETTING(    0x04, "2 Coins 1 Play" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "5000 pts" )
	PORT_DIPSETTING(    0x20, "10000 pts" )
	PORT_DIPSETTING(    0x10, "15000 pts" )
	PORT_DIPSETTING(    0x00, "No Bonus" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START  /* IN3 */
        PORT_BIT( 0x1E, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_START	/* FAKE */

	/* The coin slots are not memory mapped. Coin  causes a NMI, */
	/* This fake input port is used by the interrupt */
	/* handler to be notified of coin insertions. We use IPF_IMPULSE to */
	/* trigger exactly one interrupt, without having to check when the */
	/* user releases the key. */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			"Coin", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
INPUT_PORTS_END


static struct GfxLayout cosmicalien_spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 64*16*16 },	/* the two bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
        { 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
          7, 6, 5, 4, 3, 2, 1, 0},
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo cosmicalien_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &cosmicalien_spritelayout,  0,  8 },
	{ -1 } /* end of array */
};


static unsigned char palette[] =
{
	0x00,0x00,0x00,   /* black  */
	0x00,0x00,0xd8,   /* blue   */
	0xf8,0x00,0x00,   /* red    */
	0xff,0x00,0xff,   /* purple */
   	0x00,0xf8,0x00,   /* green  */
	0x00,0xff,0xff,   /* cyan   */
	0xf8,0xf8,0x00,   /* yellow */
	0xff,0xff,0xff,   /* white  */
	0xf8,0x94,0x44,   /* orange */
};

static unsigned short spritelookup_prom[] =
{
  0x00,0x06,0x03,0x01,
  0x00,0x01,0x07,0x03,
  0x00,0x07,0x05,0x01,
  0x00,0x07,0x07,0x07,
  0x00,0x01,0x03,0x05,
  0x00,0x03,0x06,0x01,
  0x00,0x01,0x03,0x07,
  0x00,0x07,0x07,0x07,

  0x00,0x06,0x01,0x03,
  0x00,0x05,0x03,0x04,
  0x00,0x02,0x06,0x01,
  0x00,0x07,0x01,0x03,
  0x00,0x01,0x03,0x04,
  0x00,0x03,0x06,0x01,
  0x00,0x07,0x07,0x07,
  0x00,0x07,0x07,0x07
};


static struct MachineDriver cosmicalien_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			1081600,
			0,
			cosmicalien_readmem,cosmicalien_writemem,0,0,
			cosmicalien_interrupt,1
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
  	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	cosmicalien_gfxdecodeinfo,
	sizeof(palette)/3,sizeof(spritelookup_prom)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER,
	0,
	cosmicalien_vh_start,
	cosmicalien_vh_stop,
	cosmicalien_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0
};




/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( cosmicalien_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "r1",           0x0000, 0x0800, 0x535ee0c5 )
	ROM_LOAD( "r2",           0x0800, 0x0800, 0xed3cf8f7 )
	ROM_LOAD( "r3",           0x1000, 0x0800, 0x6a111e5e )
	ROM_LOAD( "r4",           0x1800, 0x0800, 0xc9b5ca2a )
	ROM_LOAD( "r5",           0x2000, 0x0800, 0x43666d68 )


	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "r6",           0x0000, 0x0800, 0x431e866c )
	ROM_LOAD( "r7",           0x0800, 0x0800, 0xaa6c6079 )
ROM_END

/* HSC 12/02/98 */
static int hiload(void)
{
	static int firsttime = 0;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* check if the hi score table has already been initialized */
	/* the high score table is intialized to all 0, so first of all */
	/* we dirty it, then we wait for it to be cleared again */
	if (firsttime == 0)
	{
		memset(&RAM[0x400e],0xff,4);	/* high score */
		firsttime = 1;
	}

    /* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0x400e],"\x00\x00\x00",3) == 0 )
    {
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
			osd_fread(f,&RAM[0x400e],3);
			osd_fclose(f);
        }

 		firsttime = 0;
 		return 1;
    }
    else return 0;  /* we can't load the hi scores yet */
}

static void hisave(void)
{
    void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

    if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
       osd_fwrite(f,&RAM[0x400e],3);
	   osd_fclose(f);
    }
}



struct GameDriver cosmica_driver =
{
	__FILE__,
	0,
	"cosmica",
	"Cosmic Alien",
	"1980",
	"Universal",
	"Lee Taylor",
	GAME_WRONG_COLORS,
	&cosmicalien_machine_driver,
	0,

	cosmicalien_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	cosmicalien_input_ports,

	0, palette, spritelookup_prom,
	ORIENTATION_DEFAULT,

	hiload, hisave /* hsc 12/02/98 */
};

