/***************************************************************************

Espial memory map (preliminary)

MAIN CPU:

0000-4fff ROM
5800-5fff RAM
8000-800f sprites code (bits 1-7) and double height (bit 0)
8010-801f sprites X
8400-87ff Video RAM
8800-880f sprites flip x (bit 2) and flip y (bit 3)
8c00-8fff Attribute RAM
9000-900f sprites Y
9010-901f sprites color
9020-903f column scroll
9400-97ff Color RAM
c000-cfff ROM

read:
6081      IN0
6082      IN1
6083      IN2
6084      IN3
6090      read command back from sound CPU
7000      ?

write:
6081      ? - written to twice when the text during the self-test is drawn onscreen
6090      write command to sound CPU
7000      watchdog reset
7100      NMI interrupt acknowledge/enable
7200      ?

Interrupts: VBlank -> NMI.
			IRQ -> send sound commands to sound cpu. Runs in interrupt mode 1

SOUND CPU:
0000-1fff ROM
2000-23ff RAM (?)

read:
6000      read command from main CPU

write:
4000      ? watchdog ?
6000      write command back to main CPU

Interrupts: IRQs are triggered by writes to the sound_command location 0x6000 - im 1
			NMIs occur regularly to process and play the sounds

I/0 ports:
write
00        8910  control
01        8910  write
A ?
B ?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



void espial_init_machine(void);
void espial_interrupt_enable_w(int offset,int data);
int espial_interrupt(void);

extern unsigned char *espial_attributeram;
extern unsigned char *espial_column_scroll;
void espial_attributeram_w(int offset,int data);
void espial_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void espial_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

int espial_sh_interrupt(void);



/* Send sound data to the sound cpu and cause an irq */
void espial_sound_command_w (int offset, int data)
{
	/* The sound cpu runs in interrupt mode 1 */
	soundlatch_w(0,data);
	cpu_cause_interrupt(1,0xff);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x5800, 0x5fff, MRA_RAM },
	{ 0x7000, 0x7000, MRA_RAM },	/* ?? */
	{ 0x8000, 0x803f, MRA_RAM },
	{ 0x8400, 0x87ff, MRA_RAM },
	{ 0x8c00, 0x8fff, MRA_RAM },
	{ 0x9000, 0x903f, MRA_RAM },
	{ 0x9400, 0x97ff, MRA_RAM },
	{ 0xc000, 0xcfff, MRA_ROM },
	{ 0x6081, 0x6081, input_port_0_r },	/* IN0 */
	{ 0x6082, 0x6082, input_port_1_r },	/* IN1 */
	{ 0x6083, 0x6083, input_port_2_r },	/* IN2 */
	{ 0x6084, 0x6084, input_port_3_r },	/* IN3 */
	{ 0x6090, 0x6090, soundlatch_r },	/* the main CPU reads the command back from the slave? */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x4fff, MWA_ROM },
	{ 0x5800, 0x5fff, MWA_RAM },
	{ 0x6090, 0x6090, espial_sound_command_w },
	{ 0x7000, 0x7000, MWA_RAM }, /* watchdog reset */
	{ 0x7100, 0x7100, espial_interrupt_enable_w },
	{ 0x8000, 0x801f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x8400, 0x87ff, videoram_w, &videoram, &videoram_size },
	{ 0x8800, 0x880f, MWA_RAM, &spriteram_3 },
	{ 0x8c00, 0x8fff, espial_attributeram_w, &espial_attributeram },
	{ 0x9000, 0x901f, MWA_RAM, &spriteram_2 },
	{ 0x9020, 0x903f, MWA_RAM, &espial_column_scroll },
	{ 0x9400, 0x97ff, colorram_w, &colorram },
	{ 0xc000, 0xcfff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x4000, 0x4000, MWA_NOP },
	{ 0x6000, 0x6000, soundlatch_w },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( espial_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	768,	/* 768 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },	/* bits are packed in groups of four */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 128*32*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,    0, 64 },
	{ 1, 0x3000, &spritelayout,  0, 64 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHZ?????? */
	{ 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			espial_interrupt,2
		},
		{
			CPU_Z80,
			3072000,	/* 2 Mhz?????? */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,sound_writeport,
			espial_sh_interrupt,4
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	espial_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256,256,
	espial_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	espial_vh_screenrefresh,

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

ROM_START( espial_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "espial.3",     0x0000, 0x2000, 0x10f1da30 )
	ROM_LOAD( "espial.4",     0x2000, 0x2000, 0xd2adbe39 )
	ROM_LOAD( "espial.6",     0x4000, 0x1000, 0xbaa60bc1 )
	ROM_LOAD( "espial.5",     0xc000, 0x1000, 0x6d7bbfc1 )

	ROM_REGION_DISPOSE(0x5000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "espial.8",     0x0000, 0x2000, 0x2f43036f )
	ROM_LOAD( "espial.7",     0x2000, 0x1000, 0xebfef046 )
	ROM_LOAD( "espial.10",    0x3000, 0x1000, 0xde80fbc1 )
	ROM_LOAD( "espial.9",     0x4000, 0x1000, 0x48c258a0 )

	ROM_REGION(0x0200)	/* color proms */
	ROM_LOAD( "espial.1f",    0x0000, 0x0100, 0xd12de557 ) /* palette low 4 bits */
	ROM_LOAD( "espial.1h",    0x0100, 0x0100, 0x4c84fe70 ) /* palette high 4 bits */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "espial.1",     0x0000, 0x1000, 0x1e5ec20b )
	ROM_LOAD( "espial.2",     0x1000, 0x1000, 0x3431bb97 )
ROM_END

ROM_START( espiale_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2764.3",       0x0000, 0x2000, 0x0973c8a4 )
	ROM_LOAD( "2764.4",       0x2000, 0x2000, 0x6034d7e5 )
	ROM_LOAD( "2732.6",       0x4000, 0x1000, 0x357025b4 )
	ROM_LOAD( "2732.5",       0xc000, 0x1000, 0xd03a2fc4 )

	ROM_REGION_DISPOSE(0x5000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "espial.8",     0x0000, 0x2000, 0x2f43036f )
	ROM_LOAD( "espial.7",     0x2000, 0x1000, 0xebfef046 )
	ROM_LOAD( "espial.10",    0x3000, 0x1000, 0xde80fbc1 )
	ROM_LOAD( "espial.9",     0x4000, 0x1000, 0x48c258a0 )

	ROM_REGION(0x0200)	/* color proms */
	ROM_LOAD( "espial.1f",    0x0000, 0x0100, 0xd12de557 ) /* palette low 4 bits */
	ROM_LOAD( "espial.1h",    0x0100, 0x0100, 0x4c84fe70 ) /* palette high 4 bits */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "2732.1",       0x0000, 0x1000, 0xfc7729e9 )
	ROM_LOAD( "2732.2",       0x1000, 0x1000, 0xe4e256da )
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0x584c],"\x81\x00\x13",3) == 0 &&
			memcmp(&RAM[0x58B7],"\x0d\x0a\x27",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x584c],5*22);
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
		osd_fwrite(f,&RAM[0x584c],5*22);
		osd_fclose(f);
	}
}



struct GameDriver espial_driver =
{
	__FILE__,
	0,
	"espial",
	"Espial (US?)",
	"1983",
	"Thunderbolt",
	"Brad Oliver\nNicola Salmoria\nTim Lindquist (color info)",
	0,
	&machine_driver,
	0,

	espial_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	espial_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver espiale_driver =
{
	__FILE__,
	&espial_driver,
	"espiale",
	"Espial (Europe)",
	"1983",
	"Thunderbolt",
	"Brad Oliver\nNicola Salmoria\nTim Lindquist (color info)",
	0,
	&machine_driver,
	0,

	espiale_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	espial_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
