/***************************************************************************

Crazy Balloon memory map (preliminary)

0000-2fff ROM
4000-43ff RAM
4800-4bff Video RAM
5000-53ff Color RAM

I/O:

read:
00        dsw
01        joystick
02        bit 0-3 from chip PC3259 (bit 3 is the sprite/char collision detection)
          bit 4-7 dsw
03        bit 0 dsw
          bit 1 high score name reset
		  bit 2 service
		  bit 3 tilt
		  bit 4-7 from chip PC3092; coin inputs & start buttons
06-0a-0e  mirror addresses for 02; address lines 2 and 3 go to the PC3256 chip
          so they probably alter its output, while the dsw bits (4-7) stay the same.

write:
01        ?
02        bit 0-3 sprite code bit 4-7 sprite color
03        sprite X pos
04        sprite Y pos
05        music?? to a counter?
06        sound
          bit 0 IRQ enable/acknowledge
          bit 1 sound enable
          bit 2 sound related (to amplifier)
          bit 3 explosion (to 76477)
          bit 4 breath (to 76477)
          bit 5 appear (to 76477)
          bit 6 sound related (to 555)
          bit 7 to chip PC3259
07        to chip PC3092 (bits 0-3)
08        to chip PC3092 (bits 0-3)
          bit 0 seems to be flip screen
          bit 1 might enable coin input
09        to chip PC3092 (bits 0-3)
0a        to chip PC3092 (bits 0-3)
0b        to chip PC3092 (bits 0-3)
0c        MSK (to chip PC3259)
0d        CC (not used)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



void crbaloon_spritectrl_w(int offset,int data);
void crbaloon_flipscreen_w(int offset,int data);
void crbaloon_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void crbaloon_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


int val06,val08,val0a;

void crbaloon_06_w(int offset,int data)
{
	val06 = data;

	interrupt_enable_w(offset,data & 1);
}

void crbaloon_08_w(int offset,int data)
{
	val08 = data;

	crbaloon_flipscreen_w(offset,data & 1);
}

void crbaloon_0a_w(int offset,int data)
{
	val0a = data;
}

int crbaloon_IN2_r(int offset)
{
	extern int BalloonCollision;

	if (BalloonCollision != 0)
	{
		return (input_port_2_r(0) & 0xf0) | 0x08;
    }

	/* the following is needed for the game to boot up */
	if (val06 & 0x80)
	{
if (errorlog) fprintf(errorlog,"PC %04x: %02x high\n",cpu_getpc(),offset);
		return (input_port_2_r(0) & 0xf0) | 0x07;
	}
	else
	{
if (errorlog) fprintf(errorlog,"PC %04x: %02x low\n",cpu_getpc(),offset);
		return (input_port_2_r(0) & 0xf0) | 0x07;
	}
}

int crbaloon_IN3_r(int offset)
{
	if (val08 & 0x02)
		/* enable coin & start input? Wild guess!!! */
		return input_port_3_r(0);

	/* the following is needed for the game to boot up */
	if (val0a & 0x01)
	{
if (errorlog) fprintf(errorlog,"PC %04x: 03 high\n",cpu_getpc());
		return (input_port_3_r(0) & 0x0f) | 0x00;
	}
	else
	{
if (errorlog) fprintf(errorlog,"PC %04x: 03 low\n",cpu_getpc());
		return (input_port_3_r(0) & 0x0f) | 0x00;
	}
}


int crbaloon_IN_r(int offset)
{
	switch (offset & 0x03)
	{
		case 0:
			return input_port_0_r(offset);

		case 1:
			return input_port_1_r(offset);

		case 2:
			return crbaloon_IN2_r(offset);

		case 3:
			return crbaloon_IN3_r(offset);
	}

	return 0;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x2fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x4800, 0x4bff, MRA_RAM },
	{ 0x5000, 0x53ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x2fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x4800, 0x4bff, videoram_w, &videoram, &videoram_size },
	{ 0x5000, 0x53ff, colorram_w, &colorram },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x00, 0x0f, crbaloon_IN_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x02, 0x04, crbaloon_spritectrl_w },
	{ 0x06, 0x06, crbaloon_06_w },
	{ 0x08, 0x08, crbaloon_08_w },
	{ 0x0a, 0x0a, crbaloon_0a_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Test?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "I/O Check?" )
	PORT_DIPSETTING(    0x00, "RAM Check?" )
	PORT_DIPNAME( 0x02, 0x02, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x0c, 0x04, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x10, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5000" )
	PORT_DIPSETTING(    0x10, "10000" )
	PORT_DIPNAME( 0xe0, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x60, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0xa0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x00, "Disable" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* from chip PC3259 */
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1, "High Score Name Reset", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )	/* should be COIN2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	/* the following four bits come from chip PC3092 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	32,32,	/* 32*32 sprites */
	16,	/* 16 sprites */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 3*32*8+0, 3*32*8+1, 3*32*8+2, 3*32*8+3, 3*32*8+4, 3*32*8+5, 3*32*8+6, 3*32*8+7,
			2*32*8+0, 2*32*8+1, 2*32*8+2, 2*32*8+3, 2*32*8+4, 2*32*8+5, 2*32*8+6, 2*32*8+7,
			1*32*8+0, 1*32*8+1, 1*32*8+2, 1*32*8+3, 1*32*8+4, 1*32*8+5, 1*32*8+6, 1*32*8+7,
			0*32*8+0, 0*32*8+1, 0*32*8+2, 0*32*8+3, 0*32*8+4, 0*32*8+5, 0*32*8+6, 0*32*8+7 },
	{ 31*8, 30*8, 29*8, 28*8, 27*8, 26*8, 25*8, 24*8,
			23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	32*4*8  /* every sprite takes 128 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 16 },
	{ 1, 0x0800, &spritelayout, 0, 16 },
	{ -1 } /* end of array */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz ????? */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 4*8, 32*8-1 },
	gfxdecodeinfo,
	16, 16*2,
	crbaloon_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	crbaloon_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( crbaloon_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "cl01.bin",     0x0000, 0x0800, 0x9d4eef0b )
	ROM_LOAD( "cl02.bin",     0x0800, 0x0800, 0x10f7a6f7 )
	ROM_LOAD( "cl03.bin",     0x1000, 0x0800, 0x44ed6030 )
	ROM_LOAD( "cl04.bin",     0x1800, 0x0800, 0x62f66f6c )
	ROM_LOAD( "cl05.bin",     0x2000, 0x0800, 0xc8f1e2be )
	ROM_LOAD( "cl06.bin",     0x2800, 0x0800, 0x7d465691 )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cl07.bin",     0x0000, 0x0800, 0x2c1fbea8 )
	ROM_LOAD( "cl08.bin",     0x0800, 0x0800, 0xba898659 )
ROM_END

ROM_START( crbalon2_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "cl01.bin",     0x0000, 0x0800, 0x9d4eef0b )
	ROM_LOAD( "crazybal.ep2", 0x0800, 0x0800, 0x87572086 )
	ROM_LOAD( "crazybal.ep3", 0x1000, 0x0800, 0x575fe995 )
	ROM_LOAD( "cl04.bin",     0x1800, 0x0800, 0x62f66f6c )
	ROM_LOAD( "cl05.bin",     0x2000, 0x0800, 0xc8f1e2be )
	ROM_LOAD( "crazybal.ep6", 0x2800, 0x0800, 0xfed6ff5c )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "cl07.bin",     0x0000, 0x0800, 0x2c1fbea8 )
	ROM_LOAD( "cl08.bin",     0x0800, 0x0800, 0xba898659 )
ROM_END

/* HSC 12/02/98 */
static int hiload(void)
{

	static int firsttime =0;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	if (firsttime == 0)
		{
			memset(&RAM[0x4014],0xff,5); /* hi score */
			memset(&RAM[0x417d],0xff,12); /* name */
			firsttime = 1;
		}

    /* check if the hi score table has already been initialized */
    if (memcmp(&RAM[0x4014],"\x00\x00\x00\x00\x00",5) == 0 &&
		memcmp(&RAM[0x417f],"\x11\x12\x29\x1c\x0c\x18\x1b\x0e\x00\x00",10) == 0  )
    {
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
            osd_fread(f,&RAM[0x4014],5);
			osd_fread(f,&RAM[0x417d],12);
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
	 	osd_fwrite(f,&RAM[0x4014],5);
		osd_fwrite(f,&RAM[0x417d],12);
        osd_fclose(f);
    	memset(&RAM[0x4014],0xff,5); /* hi score */
		memset(&RAM[0x417d],0xff,12); /* name */

	}
}



struct GameDriver crbaloon_driver =
{
	__FILE__,
	0,
	"crbaloon",
	"Crazy Balloon (set 1)",
	"1980",
	"Taito",
	"Nicola Salmoria",
	0,
	&machine_driver,
	0,

	crbaloon_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	hiload,hisave /* hsc 12/02/98 */
};

struct GameDriver crbalon2_driver =
{
	__FILE__,
	&crbaloon_driver,
	"crbalon2",
	"Crazy Balloon (set 2)",
	"1980",
	"Taito",
	"Nicola Salmoria",
	0,
	&machine_driver,
	0,

	crbalon2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	hiload , hisave /* hsc 12/02/98 */
};
