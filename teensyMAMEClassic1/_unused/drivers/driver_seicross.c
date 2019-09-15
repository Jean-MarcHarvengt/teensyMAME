/***************************************************************************

Seicross memory map (preliminary)

0000-77ff ROM
7800-7fff RAM
9000-93ff videoram
9c00-9fff colorram

Read:
A000      Joystick + Players start button
A800      player #2 controls + coin + ?
B000      test switches
B800      watchdog reset?

Write:
8820-887f Sprite ram
9800-981f Scroll control
9880-989f ? (always 0?)

I/O ports:
0         8910 control
1         8910 write
4         8910 read


There is a microcontroller on the board. Nichibutsu custom part marked
NSC81050-102  8127 E37 and labeled No. 00363.  It's a 40-pin IC at location 4F
on the (Seicross-) board. Looks like it is linked to the dips (and those are
on a very small daughterboard).
It's on the schematic, and it has a VMA pin, so I assume it must be a 68xx family part

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



int seicross_protection_r(int offset);

extern unsigned char *seicross_row_scroll;
void seicross_colorram_w(int offset,int data);
void seicross_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void seicross_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static int portb;

static int seicross_portB_r(int offset)
{
	return (portb & 1) | (readinputport(3) & 0xfe);
}

static void friskyt_kludge(int param)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	RAM[0x7f70] = 0x00;
}

static void seicross_portB_w(int offset,int data)
{
if (errorlog) fprintf(errorlog,"PC %04x: 8910 port B = %02x\n",cpu_getpc(),data);
	/* bit 0 is IRQ enable */
	interrupt_enable_w(0,data & 1);

	/* bit 2 seems to trigger a curtom IC or microcontroller */
	if (data & 4)
	{
		unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

		if (cpu_getpc() == 0x2ea9)	/* Seicross */
		{
			RAM[0x7814] = 0x02;
			RAM[0x7815] = 0x03;
		}

		if (cpu_getpc() == 0x0425)	/* Frisky Tom */
			timer_set(TIME_IN_USEC(50),0,friskyt_kludge);
	}

	/* other bits unknown */
	portb = data;
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x77ff, MRA_ROM },
/* there is probably a microcontroller on board, which writes to memory */
/* 7816 and 7817 are read if you keep F1 pressed */
	{ 0x7800, 0x7fff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },	/* video RAM */
	{ 0x9800, 0x981f, MRA_RAM },
	{ 0x9c00, 0x9fff, MRA_RAM },	/* color RAM */
	{ 0xa000, 0xa000, input_port_0_r },	/* IN0 */
	{ 0xa800, 0xa800, input_port_1_r },	/* IN1 */
	{ 0xb000, 0xb000, input_port_2_r },	/* test */
	{ 0xb800, 0xb800, watchdog_reset_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x77ff, MWA_ROM },
	{ 0x7800, 0x7fff, MWA_RAM },
	{ 0x8820, 0x887f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0x981f, MWA_RAM, &seicross_row_scroll },
	{ 0x9880, 0x989f, MWA_RAM },	/* ? */
	{ 0x9c00, 0x9fff, seicross_colorram_w, &colorram },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x04, 0x04, AY8910_read_port_0_r },
	{ 0x0c, 0x0c, AY8910_read_port_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ 0x08, 0x08, AY8910_control_port_0_w },
	{ 0x09, 0x09, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( seicross_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START      /* Test */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
/* actually it looks like bit 1 is not a test switch, but an error report from */
/* a microcontroller on board. */
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Test 2", OSD_KEY_F1, 0, 0 )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */

	PORT_START
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x20, 0x20, "Debug Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Demo Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "SW7B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x80, "1" )
INPUT_PORTS_END

INPUT_PORTS_START( friskyt_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )

	PORT_START      /* Test */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
/* actually it looks like bit 1 is not a test switch, but an error report from */
/* a microcontroller on board. */
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Test 2", OSD_KEY_F1, 0, 0 )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */

	PORT_START
	PORT_BIT( 0x1f, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x20, 0x20, "Debug Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Demo Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "SW7B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x80, "1" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes are packed in one byte */
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes are packed in one byte */
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 17*8+0, 17*8+1, 17*8+2, 17*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 16 },
	{ 1, 0x2000, &spritelayout, 0, 16 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1536000,	/* 1.536 MHz ?? */
	{ 255 },
	{ 0 },
	{ seicross_portB_r },
	{ 0 },
	{ seicross_portB_w }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	64, 64,
	seicross_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	seicross_vh_screenrefresh,

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

ROM_START( seicross_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "smc1",         0x0000, 0x1000, 0xf6c3aeca )
	ROM_LOAD( "smc2",         0x1000, 0x1000, 0x0ec6c218 )
	ROM_LOAD( "smc3",         0x2000, 0x1000, 0xceb3c8f4 )
	ROM_LOAD( "smc4",         0x3000, 0x1000, 0x3112af59 )
	ROM_LOAD( "smc5",         0x4000, 0x1000, 0xb494a993 )
	ROM_LOAD( "smc6",         0x5000, 0x1000, 0x09d5b9da )
	ROM_LOAD( "smc7",         0x6000, 0x1000, 0x13052b03 )
	ROM_LOAD( "smc8",         0x7000, 0x0800, 0x2093461d )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "smcc",         0x0000, 0x1000, 0xfbd9b91d )
	ROM_LOAD( "smcd",         0x1000, 0x1000, 0xc3c953c4 )
	ROM_LOAD( "smca",         0x2000, 0x1000, 0x4819f0cd )
	ROM_LOAD( "smcb",         0x3000, 0x1000, 0x4c268778 )

	ROM_REGION(0x0040)	/* color PROMs */
	ROM_LOAD( "sz73",         0x0000, 0x0020, 0x4d218a3c )
	ROM_LOAD( "sz74",         0x0020, 0x0020, 0xc550531c )
ROM_END

ROM_START( friskyt_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ftom.01",      0x0000, 0x1000, 0xbce5d486 )
	ROM_LOAD( "ftom.02",      0x1000, 0x1000, 0x63157d6e )
	ROM_LOAD( "ftom.03",      0x2000, 0x1000, 0xc8d9ef2c )
	ROM_LOAD( "ftom.04",      0x3000, 0x1000, 0x23a01aac )
	ROM_LOAD( "ftom.05",      0x4000, 0x1000, 0xbfaf702a )
	ROM_LOAD( "ftom.06",      0x5000, 0x1000, 0xbce70b9c )
	ROM_LOAD( "ftom.07",      0x6000, 0x1000, 0xb2ef303a )
	ROM_LOAD( "ft8_8.rom",    0x7000, 0x0800, 0x10461a24 )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ftom.11",      0x0000, 0x1000, 0x1ec6ff65 )
	ROM_LOAD( "ftom.12",      0x1000, 0x1000, 0x3b8f40b5 )
	ROM_LOAD( "ftom.09",      0x2000, 0x1000, 0x60642f25 )
	ROM_LOAD( "ftom.10",      0x3000, 0x1000, 0x07b9dcfc )

	ROM_REGION(0x0040)	/* color PROMs */
	ROM_LOAD( "ft.9c",        0x0000, 0x0020, 0x0032167e )
	ROM_LOAD( "ft.9b",        0x0020, 0x0020, 0x6b364e69 )
ROM_END



static int seicross_hiload(void)
{
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0x7ad4],"\x00\x50\x02",3) == 0)
        {
                void *f;


                if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
                {
                        osd_fread(f,&RAM[0x7ad4],6*5);
                        osd_fclose(f);
                }

                return 1;
        }
        else return 0;  /* we can't load the hi scores yet */
}

static void seicross_hisave(void)
{
        void *f;
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
        {
                osd_fwrite(f,&RAM[0x7ad4],6*5);
                osd_fclose(f);
        }
}



struct GameDriver friskyt_driver =
{
	__FILE__,
	0,
	"friskyt",
	"Frisky Tom",
	"1981",
	"Nichibutsu",
	"Mirko Buffoni\nNicola Salmoria",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	friskyt_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	friskyt_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver seicross_driver =
{
	__FILE__,
	0,
	"seicross",
	"Seicross",
	"1984",
	"Nichibutsu + Alice",
	"Mirko Buffoni\nNicola Salmoria",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	seicross_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	seicross_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	seicross_hiload, seicross_hisave
};
