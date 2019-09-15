/***************************************************************************

Jump Bug memory map (preliminary)

0000-3fff ROM
4000-47ff RAM
4800-4bff Video RAM
4c00-4fff mirror address for video RAM
5000-50ff Object RAM
  5000-503f  screen attributes
  5040-505f  sprites
  5060-507f  bullets?
  5080-50ff  unused?
8000-afff ROM

read:
6000      IN0
6800      IN1
7000      IN2

write:
5800      8910 write port
5900      8910 control port
6002-6006 gfx bank select - see vidhrdw/jumpbug.c for details (6005 seems to be unused)
7001      interrupt enable
7002      coin counter ????
7003      ?
7004      stars on (?)
7005      ?
7006      screen vertical flip ????
7007      screen horizontal flip ????
7800      ?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *jumpbug_attributesram;
extern unsigned char *jumpbug_gfxbank;
extern unsigned char *jumpbug_stars;
void jumpbug_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void jumpbug_attributes_w(int offset,int data);
void jumpbug_gfxbank_w(int offset,int data);
void jumpbug_stars_w(int offset,int data);
int jumpbug_vh_start(void);
void jumpbug_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static struct MemoryReadAddress readmem[] =
{
	{ 0x4000, 0x4bff, MRA_RAM },	/* RAM, Video RAM */
	{ 0x4c00, 0x4fff, videoram_r },	/* mirror address for Video RAM*/
	{ 0x5000, 0x507f, MRA_RAM },	/* screen attributes, sprites */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0xafff, MRA_ROM },
	{ 0x6000, 0x6000, input_port_0_r },	/* IN0 */
	{ 0x6800, 0x6800, input_port_1_r },	/* IN1 */
	{ 0x7000, 0x7000, input_port_2_r },	/* DSW0 */
	{ 0xeff0, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, videoram_w, &videoram, &videoram_size },
	{ 0x4c00, 0x4fff, videoram_w },	/* mirror address for Video RAM */
	{ 0x5000, 0x503f, jumpbug_attributes_w, &jumpbug_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x7001, 0x7001, interrupt_enable_w },
	{ 0x7004, 0x7004, MWA_RAM, &jumpbug_stars },
	{ 0x6002, 0x6006, jumpbug_gfxbank_w, &jumpbug_gfxbank },
	{ 0x5900, 0x5900, AY8910_control_port_0_w },
	{ 0x5800, 0x5800, AY8910_write_port_0_w },
	{ 0xeff0, 0xefff, MWA_RAM },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0xafff, MWA_ROM },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x40, 0x00, "Difficulty ?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Hard?" )
	PORT_DIPSETTING(    0x40, "Easy?" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x0c, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2/1 2/1" )
	PORT_DIPSETTING(    0x08, "2/1 1/3" )
	PORT_DIPSETTING(    0x00, "1/1 1/1" )
	PORT_DIPSETTING(    0x0c, "1/1 1/6" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	768,	/* 768 characters */
	2,	/* 2 bits per pixel */
	{ 0, 768*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	192,	/* 192 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 192*16*16 },	/* the two bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz????? */
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
			nmi_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	32+64, 32,	/* 32 for the characters, 64 for the stars */
	jumpbug_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	jumpbug_vh_start,
	generic_vh_stop,
	jumpbug_vh_screenrefresh,

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

ROM_START( jumpbug_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "jb1",          0x0000, 0x1000, 0x415aa1b7 )
	ROM_LOAD( "jb2",          0x1000, 0x1000, 0xb1c27510 )
	ROM_LOAD( "jb3",          0x2000, 0x1000, 0x97c24be2 )
	ROM_LOAD( "jb4",          0x3000, 0x1000, 0x66751d12 )
	ROM_LOAD( "jb5",          0x8000, 0x1000, 0xe2d66faf )
	ROM_LOAD( "jb6",          0x9000, 0x1000, 0x49e0bdfd )
	ROM_LOAD( "jb7",          0xa000, 0x1000, 0x690025ee )

	ROM_REGION_DISPOSE(0x3000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "jbi",          0x0000, 0x0800, 0x7749b111 )
	ROM_LOAD( "jbj",          0x0800, 0x0800, 0x06e8d7df )
	ROM_LOAD( "jbk",          0x1000, 0x0800, 0xb8dbddf3 )
	ROM_LOAD( "jbl",          0x1800, 0x0800, 0x9a091b0a )
	ROM_LOAD( "jbm",          0x2000, 0x0800, 0x8a0fc082 )
	ROM_LOAD( "jbn",          0x2800, 0x0800, 0x155186e0 )

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "6331-1.11r",   0x0000, 0x0020, 0x6a0c7d87 )
ROM_END

ROM_START( jbugsega_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "jb1.prg",      0x0000, 0x4000, 0xb223fca9 )
	ROM_LOAD( "jb2.prg",      0x8000, 0x2800, 0x1b2eb25f )

	ROM_REGION_DISPOSE(0x3000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "jbi",          0x0000, 0x0800, 0x7749b111 )
	ROM_LOAD( "jbj",          0x0800, 0x0800, 0x06e8d7df )
	ROM_LOAD( "jbk",          0x1000, 0x0800, 0xb8dbddf3 )
	ROM_LOAD( "jbl",          0x1800, 0x0800, 0x9a091b0a )
	ROM_LOAD( "jbm",          0x2000, 0x0800, 0x8a0fc082 )
	ROM_LOAD( "jbn",          0x2800, 0x0800, 0x155186e0 )

	ROM_REGION(0x0020)	/* color prom */
	ROM_LOAD( "6331-1.11r",   0x0000, 0x0020, 0x6a0c7d87 )
ROM_END



static void jumpbug_decode(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* this is not a "decryption", it is just a protection removal */
	RAM[0x265a] = 0xc9;
	RAM[0x8a16] = 0xc9;
	RAM[0x8dae] = 0xc9;
	RAM[0x8dbe] = 0xc3;
	RAM[0x8dd7] = 0x18;
	RAM[0x9f3d] = 0x18;
	RAM[0x9f53] = 0xc3;
}



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0x4208],"\x00\x00\x00\x05",4) == 0 &&
		memcmp(&RAM[0x4233],"\x97\x97\x97\x97",4) ==0)

	{

			void *f;

			if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
			{
					osd_fread(f,&RAM[0x4208],6);
					osd_fread(f,&RAM[0x4222],3*7);
					osd_fclose(f);
			}
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
			osd_fwrite(f,&RAM[0x4208],6);
			osd_fwrite(f,&RAM[0x4222],3*7);
			osd_fclose(f);
	}
}



struct GameDriver jumpbug_driver =
{
	__FILE__,
	0,
	"jumpbug",
	"Jump Bug",
	"1981",
	"Rock-ola",
	"Richard Davies\nBrad Oliver\nNicola Salmoria\nJuan Carlos Lorente\nMarco Cassili",
	0,
	&machine_driver,
	0,

	jumpbug_rom,
	jumpbug_decode, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver jbugsega_driver =
{
	__FILE__,
	&jumpbug_driver,
	"jbugsega",
	"Jump Bug (bootleg)",
	"1981",
	"bootleg",
	"Richard Davies\nBrad Oliver\nNicola Salmoria\nJuan Carlos Lorente\nMarco Cassili",
	0,
	&machine_driver,
	0,

	jbugsega_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
