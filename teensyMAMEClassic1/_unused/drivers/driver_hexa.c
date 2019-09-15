/****************************************************************************

HEXA Memory map (prelim)
0000 7fff ROM
8000 bfff bank switch rom space??
c000 c7ff RAM
e000 e7ff video ram
e800-efff unused RAM

read:
d001      AY8910 read
f000      ???????

write:
d000      AY8910 control
d001      AY8910 write
d008      bit0/1 = flip screen x/y
          bit 4 = ROM bank??
          bit 5 = char bank
		  other bits????????
d010      watchdog reset, or IRQ acknowledge, or both
f000      ????????

NOTES:
        Needs High score save and load (i think the high score is stored
                around 0xc709)

*************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



void hexa_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void hexa_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void hexa_d008_w(int offset,int data);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd001, 0xd001, AY8910_read_port_0_r },
	{ 0xe000, 0xe7ff, MRA_RAM },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xd000, 0xd000, AY8910_control_port_0_w },
	{ 0xd001, 0xd001, AY8910_write_port_0_w },
	{ 0xd008, 0xd008, hexa_d008_w },
	{ 0xd010, 0xd010, watchdog_reset_w },	/* or IRQ acknowledge, or both */
	{ 0xe000, 0xe7ff, videoram_w, &videoram, &videoram_size },
	{ -1 } /* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x03, 0x03, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x04, 0x00, "Naughty Pics", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "Easy?" )
	PORT_DIPSETTING(    0x20, "Medium?" )
	PORT_DIPSETTING(    0x10, "Hard?" )
	PORT_DIPSETTING(    0x00, "Hardest?" )
	PORT_DIPNAME( 0x40, 0x40, "Pobys", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8 by 8 */
	4096,   /* 4096 characters */
	3,		/* 3 bits per pixel */
	{ 2*4096*8*8, 4096*8*8, 0 },	/* plane */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },		/* x bit */
	{ 8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7 }, 	/* y bit */
	8*8
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,  0 , 32 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz ???? */
	{ 255 },
	{ input_port_0_r },
	{ input_port_1_r },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,		/* 4 MHz ??????? */
			0,				/* memory region */
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,					/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 256,
	hexa_vh_convert_color_prom,


	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,					/* vh_init routine */
	generic_vh_start,		/* vh_start routine */
	generic_vh_stop,		/* vh_stop routine */
	hexa_vh_screenrefresh,	/* vh_update routine */

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

ROM_START( hexa_rom )
	ROM_REGION(0x18000)		/* 64k for code + 32k for banked ROM */
	ROM_LOAD( "hexa.20",      0x00000, 0x8000, 0x98b00586 )
	ROM_LOAD( "hexa.21",      0x10000, 0x8000, 0x3d5d006c )

	ROM_REGION_DISPOSE(0x18000)		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "hexa.17",      0x00000, 0x8000, 0xf6911dd6 )
	ROM_LOAD( "hexa.18",      0x08000, 0x8000, 0x6e3d95d2 )
	ROM_LOAD( "hexa.19",      0x10000, 0x8000, 0xffe97a31 )

	ROM_REGION(0x0300)		/* color proms */
	ROM_LOAD( "hexa.001",     0x0000, 0x0100, 0x88a055b4 )
	ROM_LOAD( "hexa.003",     0x0100, 0x0100, 0x3e9d4932 )
	ROM_LOAD( "hexa.002",     0x0200, 0x0100, 0xff15366c )
ROM_END

static int hexa_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];
	static int firsttime = 0;


	/* check if the hi score table has already been initialized */
	/* the high score table is intialized to all 0, so first of all */
	/* we dirty it, then we wait for it to be cleared again */
	if (firsttime == 0)
	{
		RAM[0xc70a] = 1;
		firsttime = 1;
	}

	if (memcmp(&RAM[0xc709],"\x00\x00",2) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xc709],2);
			osd_fclose(f);

		}

		firsttime = 0;
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void hexa_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xc709],2);
		osd_fclose(f);
	}
}


static void hexa_patch(void)
{
	unsigned char *RAM = Machine->memory_region[0];


	/* Hexa is not protected or anything, but it keeps writing 0x3f to register */
	/* 0x07 of the AY8910, to read the input ports. This causes clicks in the */
	/* music since the output channels are continuously disabled and reenabled. */
	/* To avoid that, we just NOP out the 0x3f write. */
	RAM[0x0124] = 0x00;
	RAM[0x0125] = 0x00;
	RAM[0x0126] = 0x00;
}


struct GameDriver hexa_driver =
{
	__FILE__,
	0,
	"hexa",
	"Hexa",
	"????",
	"D. R. Korea",
	"Howie Cohen (driver)\nThierry Lescot (Technical Info) ",
	0,
	&machine_driver,
	0,

	hexa_rom,
	hexa_patch, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hexa_hiload, hexa_hisave
};

