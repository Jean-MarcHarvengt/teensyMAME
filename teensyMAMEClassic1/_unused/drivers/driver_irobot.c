/****************************************************************************
I-Robot Memory Map

0000 - 07FF  R/W    RAM
0800 - 0FFF  R/W    Banked RAM
1000 - 1000  INRD1  Bit 7 = Right Coin
                    Bit 6 = Left Coin
                    Bit 5 = Aux Coin
                    Bit 4 = Self Test
                    Bit 3 = ?
                    Bit 2 = ?
                    Bit 1 = ?
                    Bit 0 = ?
1040 - 1040  INRD2  Bit 7 = Start 1
                    Bit 6 = Start 2
                    Bit 5 = ?
                    Bit 4 = Fire
                    Bit 3 = ?
                    Bit 2 = ?
                    Bit 1 = ?
                    Bit 0 = ?
1080 - 1080  STATRD Bit 7 = VBLANK
                    Bit 6 = Polygon generator done
                    Bit 5 = Mathbox done
                    Bit 4 = Unused
                    Bit 3 = ?
                    Bit 2 = ?
                    Bit 1 = ?
                    Bit 0 = ?
10C0 - 10C0  INRD3  Dip switch
1140 - 1140  STATWR Bit 7 = Select Polygon RAM banks
                    Bit 6 = BFCALL
                    Bit 5 = Cocktail Flip
                    Bit 4 = Start Mathbox
                    Bit 3 = Connect processor bus to mathbox bus
                    Bit 2 = Start polygon generator
                    Bit 1 = Select polygon image RAM bank
                    Bit 0 = Erase polygon image memory
1180 - 1180  OUT0   Bit 7 = Alpha Map 1
                    Bit 6,5 = RAM bank select
                    Bit 4,3 = Mathbox memory select
                    Bit 2,1 = Mathbox bank select
11C0 - 11C0  OUT1   Bit 7 = Coin Counter R
                    Bit 6 = Coin Counter L
                    Bit 5 = LED2
                    Bit 4 = LED1
                    Bit 3,2,1 = ROM bank select
1200 - 12FF  R/W    NVRAM (bits 0..3 only)
1300 - 13FF  W      Select analog controller
1300 - 13FF  R      Read analog controller
1400 - 143F  R/W    Quad Pokey
1800 - 18FF         Palette RAM
1900 - 1900  W      Watchdog reset
1A00 - 1A00  W      FIREQ Enable
1B00 - 1BFF  W      Start analog controller ADC
1C00 - 1FFF  R/W    Character RAM
2000 - 3FFF  R/W    Mathbox/Vector Gen Shared RAM
4000 - 5FFF  R      Banked ROM
6000 - FFFF  R      Fixed ROM

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern int  irobot_vh_start(void);
extern void irobot_vh_stop(void);
extern void irobot_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern void irobot_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void irobot_paletteram_w(int offset,int data);

extern void irobot_nvram_w(int offset,int data);
extern unsigned char *irobot_nvRAM;
void irobot_init_machine (void);

int irobot_status_r(int offset);
void irobot_statwr_w(int offset, int data);
void irobot_out0_w(int offset, int data);
void irobot_rom_banksel( int offset, int data);
int irobot_control_r (int offset);
void irobot_control_w (int offset, int data);
int irobot_sharedmem_r(int offset);
void irobot_sharedmem_w(int offset,int data);


static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0x07ff, MRA_RAM },
    { 0x0800, 0x0fff, MRA_BANK2 },
    { 0x1200, 0x12FF, MRA_RAM, &irobot_nvRAM },
    { 0x1300, 0x13FF, irobot_control_r },
    { 0x1400, 0x143f, quad_pokey_r },       /* Quad Pokey read  */
    { 0x1C00, 0x1fff, MRA_RAM },
    { 0x1000, 0x1000, input_port_0_r }, /* INRD1 */
    { 0x1040, 0x1040, input_port_1_r }, /* INRD2 */
    { 0x1080, 0x1080, irobot_status_r }, /* STATRD */
    { 0x10C0, 0x10c0, input_port_3_r }, /* INRD3 */
    { 0x2000, 0x3fff, irobot_sharedmem_r },
    { 0x4000, 0x5FFF, MRA_BANK1 },
    { 0x6000, 0xFFFF, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
    { 0x0000, 0x07ff, MWA_RAM },
    { 0x0800, 0x0fff, MWA_BANK2 },
    { 0x1100, 0x1100, MWA_RAM },
    { 0x1200, 0x12FF, irobot_nvram_w },
    { 0x1400, 0x143f, quad_pokey_w },       /* Quad Pokey write  */
    { 0x1140, 0x1140, irobot_statwr_w },
    { 0x1180, 0x1180, irobot_out0_w },
    { 0x11C0, 0x11c0, irobot_rom_banksel },
    { 0x1800, 0x18FF, irobot_paletteram_w },
    { 0x1900, 0x19FF, MWA_RAM },            /* Watchdog reset */
    { 0x1A00, 0x1A00, MWA_RAM },
    { 0x1B00, 0x1BFF, irobot_control_w },
    { 0x1C00, 0x1FFF, videoram_w, &videoram, &videoram_size },
    { 0x2000, 0x3fff, irobot_sharedmem_w},
    { 0x4000, 0xffff, MWA_ROM },
    { -1 }  /* end of table */
};

INPUT_PORTS_START( irobot_input_ports )
	PORT_START	/* IN0 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN)
    PORT_BITX(    0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
    PORT_DIPSETTING(    0x10, "Off" )
    PORT_DIPSETTING(    0x00, "On" )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1)
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* IN2 */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* MB DONE */
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* EXT DONE */
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START      /* DSW1 */
    PORT_DIPNAME( 0x03, 0x02, "Coins Per Credit", IP_KEY_NONE )
    PORT_DIPSETTING(    0x03, "4 Coins=1 Credit" )
    PORT_DIPSETTING(    0x02, "3 Coins=1 Credit " )
    PORT_DIPSETTING(    0x01, "2 Coins=1 Credit" )
    PORT_DIPSETTING(    0x00, "1 Coin=1 Credit" )

    PORT_DIPNAME( 0x0C, 0x00, "Right Coin",IP_KEY_NONE)
    PORT_DIPSETTING(    0x00, "1 Coin for 1 Coin Unit" )
    PORT_DIPSETTING(    0x04, "1 Coin for 4 Coin Units" )
    PORT_DIPSETTING(    0x08, "1 Coin for 5 Coin Units" )
    PORT_DIPSETTING(    0x0C, "1 Coin for 6 Coin Units" )

    PORT_DIPNAME( 0x10, 0x00, "Left Coin",IP_KEY_NONE)
    PORT_DIPSETTING(    0x00, "1 Coin for 1 Credit" )
    PORT_DIPSETTING(    0x10, "1 Coin for 2 Credits" )

    PORT_DIPNAME( 0xE0, 0x00, "Bonus Adder",IP_KEY_NONE)
    PORT_DIPSETTING(    0x00, "No Bonus" )
    PORT_DIPSETTING(    0x20, "2 Coin Units for 1 Credit" )
    PORT_DIPSETTING(    0xA0, "3 Coin Units for 1 Credit" )
    PORT_DIPSETTING(    0x40, "4 Coin Units for 1 Credit" )
    PORT_DIPSETTING(    0x80, "5 Coin Units for 1 Credit" )
    PORT_DIPSETTING(    0xC0, "No Bonus" )
    PORT_DIPSETTING(    0xE0, "Free Play" )

    PORT_START      /* DSW2 */
    PORT_DIPNAME( 0x01, 0x01, "Language", IP_KEY_NONE )
    PORT_DIPSETTING(    0x00, "German" )
    PORT_DIPSETTING(    0x01, "English" )

    PORT_DIPNAME( 0x02, 0x00, "Difficulty",IP_KEY_NONE)
    PORT_DIPSETTING(    0x00, "Medium" )
    PORT_DIPSETTING(    0x02, "Easy" )

    PORT_DIPNAME( 0x0C, 0x00, "Bonus lives per coin",IP_KEY_NONE)
    PORT_DIPSETTING(    0x00, "3" )
    PORT_DIPSETTING(    0x40, "2" )
    PORT_DIPSETTING(    0x80, "5" )
    PORT_DIPSETTING(    0xC0, "4" )

    PORT_DIPNAME( 0x30, 0x00, "Bonus life",IP_KEY_NONE)
    PORT_DIPSETTING(    0x20, "None" )
    PORT_DIPSETTING(    0x00, "20,000" )
    PORT_DIPSETTING(    0x30, "30,000" )
    PORT_DIPSETTING(    0x10, "50,000" )

    PORT_DIPNAME( 0x40, 0x00, "Min Game Time",IP_KEY_NONE)
    PORT_DIPSETTING(    0x40, "90 Sec" )
    PORT_DIPSETTING(    0x00, "3 Lives" )

    PORT_DIPNAME( 0x80, 0x00, "Doodle City Time",IP_KEY_NONE)
    PORT_DIPSETTING(    0x80, "3 min 5 sec" )
    PORT_DIPSETTING(    0x00, "2 min 10 sec" )

	PORT_START	/* IN4 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y, 70, 0, 0, 255 )

	PORT_START	/* IN5 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X, 50, 0, 0, 255 )

INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
    64,    /* 64 characters */
    1,      /* 1 bit per pixel */
    { 0 }, /* the bitplanes are packed in one nibble */
    { 4, 5, 6, 7, 12, 13, 14, 15},
    { 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16},
    16*8   /* every char takes 16 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
    { 1, 0x0000, &charlayout,     64, 16 },
	{ -1 }
};

unsigned char irobot_color_prom[] =
{
	0x00, 0x00, 0x00, 0x00, 0xF3, 0x33, 0x0F, 0xC3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xF3, 0x33, 0x0F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static struct POKEYinterface pokey_interface =
{
	4,	/* 4 chips */
	1250000,	/* 1.25 MHz??? */
	25,
	POKEY_DEFAULT_GAIN,
	NO_CLIP,
	/* The 8 pot handlers */
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	/* The allpot handler */
    { input_port_4_r, 0, 0, 0 },
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
            CPU_M6809,
            1500000,    /* 1.5 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,4
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
    1,
    irobot_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
    64 + 32,64 + 32, /* 64 for polygons, 32 for text */
    irobot_vh_convert_color_prom,

    VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
    irobot_vh_start,
    irobot_vh_stop,
    irobot_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_POKEY,
			&pokey_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( irobot_rom )
    ROM_REGION(0x20000) /* 64k for code + 48K Banked ROM*/
    ROM_LOAD( "136029.208",   0x6000, 0x2000, 0xb4d0be59 )
    ROM_LOAD( "136029.209",   0x8000, 0x4000, 0xf6be3cd0 )
    ROM_LOAD( "136029.210",   0xC000, 0x4000, 0xc0eb2133 )
    ROM_LOAD( "136029.405",   0x10000, 0x4000, 0x9163efe4 )
    ROM_LOAD( "136029.206",   0x14000, 0x4000, 0xe114a526 )
    ROM_LOAD( "136029.207",   0x18000, 0x4000, 0xb4556cb0 )

    ROM_REGION_DISPOSE(0x800)  /* temporary space for graphics (disposed after conversion) */
    ROM_LOAD( "136029.124",   0x0000, 0x800, 0x848948b6 )

    ROM_REGION(0x10000)  /* temoporary space for mathbox roms */
    ROM_LOAD( "ir103.bin",    0x0000, 0x2000, 0x0c83296d )
    ROM_LOAD( "ir104.bin",    0x2000, 0x2000, 0x0a6cdcca )
    ROM_LOAD( "ir101.bin",    0x4000, 0x4000, 0x62a38c08 )
    ROM_LOAD( "ir102.bin",    0x8000, 0x4000, 0x9d588f22 )
    ROM_LOAD( "ir111.bin",    0xC000, 0x400, 0x9fbc9bf3 )
    ROM_LOAD( "ir112.bin",    0xC400, 0x400, 0xb2713214 )
    ROM_LOAD( "ir113.bin",    0xC800, 0x400, 0x7875930a )
    ROM_LOAD( "ir114.bin",    0xCC00, 0x400, 0x51d29666 )
    ROM_LOAD( "ir115.bin",    0xD000, 0x400, 0x00f9b304 )
    ROM_LOAD( "ir116.bin",    0xD400, 0x400, 0x326aba54 )
    ROM_LOAD( "ir117.bin",    0xD800, 0x400, 0x98efe8d0 )
    ROM_LOAD( "ir118.bin",    0xDC00, 0x400, 0x4a6aa7f9 )
    ROM_LOAD( "ir119.bin",    0xE000, 0x400, 0xa5a13ad8 )
    ROM_LOAD( "ir120.bin",    0xE400, 0x400, 0x2a083465 )
    ROM_LOAD( "ir121.bin",    0xE800, 0x400, 0xadebcb99 )
    ROM_LOAD( "ir122.bin",    0xEC00, 0x400, 0xda7b6f79 )
    ROM_LOAD( "ir123.bin",    0xF000, 0x400, 0x39fff18f )

ROM_END

static int novram_load(void)
{
unsigned char *RAM = Machine->memory_region[0];

	void *f;
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
        osd_fread(f,&RAM[0x1200],256);
		osd_fclose(f);
	}
	return 1;
}

static void novram_save(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
        osd_fwrite(f,&RAM[0x1200],256);
		osd_fclose(f);
	}
}

struct GameDriver irobot_driver =
{
	__FILE__,
	0,
    "irobot",
    "I, Robot",
	"1984?",
	"Atari",
    "Dan Boris\nMike Balfour\nFrank Palazzolo\nBryan Smith (Tech info)",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

    irobot_rom,
	0, 0,
	0,
	0,	/* sound_prom */

    irobot_input_ports,

    irobot_color_prom, 0, 0,
	ORIENTATION_DEFAULT,
	novram_load, novram_save /* Highscore load, save */
};

