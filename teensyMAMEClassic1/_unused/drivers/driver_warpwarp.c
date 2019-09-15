/***************************************************************************

Warp Warp memory map (preliminary)

  Memory Map figured out by Chris Hardy (chrish@kcbbs.gen.nz)
  Initial Driver code by Mirko


0000-37FF ROM		Code
4800-4FFF ROM		Graphics rom which must be included in memory space

memory mapped ports:

read:

All Read ports only use bit 0

C000      Coin slot 2
C001      ???
C002      Start P1
C003      Start P2
C004      Fire
C005      Test Mode
C006      ???
C007      Coin Slot 2
C010      Joystick (read like an analog one, but it's digital)
          0->23 = DOWN
          24->63 = UP
          64->111 = LEFT
          112->167 = RIGHT
          168->255 = NEUTRAL
C020-C027 Dipswitch 1->8 in bit 0

write:
c000-c001 bullet x/y pos
C002      Sound
C003      WatchDog reset
C010      Music 1
C020      Music 2
c030-c032 lamps
c034      coin lock out
c035      coin counter
c036      IRQ enable _and_ bullet enable (both on bit 0) (currently ignored)
C037      flip screen (currently ignored)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *warpwarp_bulletsram;
void warpwarp_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void warpwarp_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

int warpwarp_input_c000_7_r(int offset);
int warpwarp_input_c020_27_r(int offset);
int warpwarp_input_controller_r(int offset);
int warpwarp_interrupt(void);



int warpwarp_input_c000_7_r(int offset)
{
	return (readinputport(0) >> offset) & 1;
}

/* Read the Dipswitches */
int warpwarp_input_c020_27_r(int offset)
{
	return (readinputport(1) >> offset) & 1;
}

int warpwarp_input_controller_r(int offset)
{
	int res;

	res = readinputport(2);
	if (res & 1) return 23;
	if (res & 2) return 63;
	if (res & 4) return 111;
	if (res & 8) return 167;
	return 255;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x37ff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x4800, 0x4fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0xc000, 0xc007, warpwarp_input_c000_7_r },
	{ 0xc010, 0xc010, warpwarp_input_controller_r },
	{ 0xc020, 0xc027, warpwarp_input_c020_27_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x37ff, MWA_ROM },
	{ 0x4000, 0x43ff, videoram_w, &videoram, &videoram_size },
	{ 0x4400, 0x47ff, colorram_w, &colorram },
	{ 0x4800, 0x4fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0xC000, 0xC001, MWA_RAM, &warpwarp_bulletsram },
	{ 0xC003, 0xC003, watchdog_reset_w },
	{ 0xc030, 0xc032, osd_led_w },
	{ 0xc035, 0xc035, coin_counter_w },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x0c, 0x04, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	/* TODO: The bonus setting changes for 5 lives */
	PORT_DIPNAME( 0x30, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "8000 30000" )
	PORT_DIPSETTING(    0x10, "10000 40000" )
	PORT_DIPSETTING(    0x20, "15000 60000" )
	PORT_DIPSETTING(    0x30, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "High Score Initials", IP_KEY_NONE ) /* Probably unused */
	PORT_DIPSETTING(    0x80, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )

	PORT_START      /* FAKE - used by input_controller_r to simulate an analog stick */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
INPUT_PORTS_END




static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	1,	/* 1 bit per pixel */
	{ 0 },
	{  0, 1, 2, 3, 4, 5, 6, 7 ,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x4800, &charlayout,   0, 256 },
	{ 0, 0x4800, &spritelayout, 0, 256 },
	{ -1 } /* end of array */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2048000,	/* 3 Mhz? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
  	34*8, 32*8, { 0*8, 34*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,

	256, 2*256,
	warpwarp_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	warpwarp_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( warpwarp_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "g-09601.2r",   0x0000, 0x1000, 0x916ffa35 )
	ROM_LOAD( "g-09602.2m",   0x1000, 0x1000, 0x398bb87b )
	ROM_LOAD( "g-09603.1p",   0x2000, 0x1000, 0x6b962fc4 )
	ROM_LOAD( "g-09613.1t",   0x3000, 0x0800, 0x60a67e76 )
	ROM_LOAD( "g-9611.4c",    0x4800, 0x0800, 0x00e6a326 )
ROM_END

ROM_START( warpwar2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "g-09601.2r",   0x0000, 0x1000, 0x916ffa35 )
	ROM_LOAD( "g-09602.2m",   0x1000, 0x1000, 0x398bb87b )
	ROM_LOAD( "g-09603.1p",   0x2000, 0x1000, 0x6b962fc4 )
	ROM_LOAD( "g-09612.1t",   0x3000, 0x0800, 0xb91e9e79 )
	ROM_LOAD( "g-9611.4c",    0x4800, 0x0800, 0x00e6a326 )
ROM_END



static int hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0x8358],"\x00\x30\x00",3) == 0 &&
			memcmp(&RAM[0x8373],"\x18\x18\x18",3) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x8358],3*10);
			RAM[0x831d] = RAM[0x8364];
			RAM[0x831e] = RAM[0x8365];
			RAM[0x831f] = RAM[0x8366];
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;
}

static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8358],3*10);
		osd_fclose(f);
	}
}



struct GameDriver warpwarp_driver =
{
	__FILE__,
	0,
	"warpwarp",
	"Warp Warp (set 1)",
	"1981",
	"[Namco] (Rock-ola license)",
	"Chris Hardy (MAME driver)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	warpwarp_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};

struct GameDriver warpwar2_driver =
{
	__FILE__,
	&warpwarp_driver,
	"warpwar2",
	"Warp Warp (set 2)",
	"1981",
	"[Namco] (Rock-ola license)",
	"Chris Hardy (MAME driver)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	warpwar2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};
