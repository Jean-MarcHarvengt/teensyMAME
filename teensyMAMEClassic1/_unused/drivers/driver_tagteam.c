/***************************************************************************

Tag Team Wrestling hardware description:

This hardware is very similar to the BurgerTime/Lock N Chase family of games
but there are just enough differences to make it a pain to share the
codebase. It looks like this hardware is a bridge between the BurgerTime
family and the later Technos games, like Mat Mania and Mysterious Stones.

The video hardware supports 3 sprite banks instead of 1
The sound hardware appears nearly identical to Mat Mania

TODO:
        * fix hi-score (reset) bug

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6502/m6502.h"

void tagteam_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

int  tagteam_mirrorvideoram_r(int offset);
void tagteam_mirrorvideoram_w(int offset,int data);
int  tagteam_mirrorcolorram_r(int offset);
void tagteam_mirrorcolorram_w(int offset,int data);
void tagteam_video_control_w(int offset,int data);
void tagteam_control_w(int offset,int data);

int  tagteam_vh_start (void);
void tagteam_vh_stop (void);
void tagteam_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh);

static void sound_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(1,M6502_INT_IRQ);
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x2000, 0x2000, input_port_1_r },     /* IN1 */
	{ 0x2001, 0x2001, input_port_0_r },     /* IN0 */
	{ 0x2002, 0x2002, input_port_2_r },     /* DSW2 */
	{ 0x2003, 0x2003, input_port_3_r },     /* DSW1 */
	{ 0x4000, 0x43ff, tagteam_mirrorvideoram_r },
	{ 0x4400, 0x47ff, tagteam_mirrorcolorram_r },
	{ 0x4800, 0x4fff, MRA_RAM },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
//	{ 0x2000, 0x2000, tagteam_unk_w },
	{ 0x2001, 0x2001, tagteam_control_w },
	{ 0x2002, 0x2002, sound_command_w },
//	{ 0x2003, 0x2003, MWA_NOP }, /* Appears to increment when you're out of the ring */
	{ 0x4000, 0x43ff, tagteam_mirrorvideoram_w },
	{ 0x4400, 0x47ff, tagteam_mirrorcolorram_w },
	{ 0x4800, 0x4bff, videoram_w, &videoram, &videoram_size },
	{ 0x4c00, 0x4fff, colorram_w, &colorram },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x2007, 0x2007, soundlatch_r },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x2000, 0x2000, AY8910_write_port_0_w },
	{ 0x2001, 0x2001, AY8910_control_port_0_w },
	{ 0x2002, 0x2002, AY8910_write_port_1_w },
	{ 0x2003, 0x2003, AY8910_control_port_1_w },
	{ 0x2004, 0x2004, DAC_data_w },
	{ 0x2005, 0x2005, interrupt_enable_w },
	{ -1 }  /* end of table */
};



static int tagteam_interrupt(void)
{
	static int coin;
	int port;

	port = readinputport(0) & 0xc0;

	if (port != 0xc0)    /* Coin */
	{
		if (coin == 0)
		{
			coin = 1;
			return nmi_interrupt();
		}
	}
	else coin = 0;

        return ignore_interrupt();
}

INPUT_PORTS_START( tagteam_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_TILT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_START2 )

	PORT_START      /* DSW1 - 7 not used?, 8 = VBLANK! */
	PORT_DIPNAME( 0x0F, 0x0F, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0F, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x0A, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPNAME( 0x10, 0x00, "Monitor", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x10, "Cocktail" )
	PORT_DIPNAME( 0x20, 0x00, "Control Panel", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_DIPNAME( 0x40, 0x00, "Unused?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK  )

	PORT_START      /* DSW2 - 3,4,5,6,7,8 = not used? */
	PORT_DIPNAME( 0x01, 0x01, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Unused?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unused?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unused?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unused?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unused?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unused?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	3072,   /* 3072 characters */
	3,      /* 3 bits per pixel */
	{ 2*3072*8*8, 3072*8*8, 0 },    /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};


static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	768,    /* 768 sprites */
	3,      /* 3 bits per pixel */
	{ 2*768*16*16, 768*16*16, 0 },  /* the bitplanes are separated */
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo tagteam_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 4 }, /* chars */
	{ 1, 0x0000, &spritelayout, 0, 4 }, /* sprites */
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,      /* 2 chips */
	1500000,        /* 1.5 MHz ?? */
	{ 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct DACinterface dac_interface =
{
	1,
	{ 255 }
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz ?? */
			0,
			readmem,writemem,0,0,
			tagteam_interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			975000,  /* 975 kHz ?? */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			nmi_interrupt,16   /* IRQs are triggered by the main CPU */
		}
	},
	57, 3072,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
	tagteam_gfxdecodeinfo,
	32, 32,
	tagteam_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	tagteam_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

ROM_START( tagteam_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "prowbf0.bin",  0x08000, 0x2000, 0x6ec3afae )
	ROM_LOAD( "prowbf1.bin",  0x0a000, 0x2000, 0xb8fdd176 )
	ROM_LOAD( "prowbf2.bin",  0x0c000, 0x2000, 0x3d33a923 )
	ROM_LOAD( "prowbf3.bin",  0x0e000, 0x2000, 0x518475d2 )

	ROM_REGION_DISPOSE(0x12000) /* temporary space for graphics (disposed after conversion) */
	/* Character ROMs - 1024 chars in 3 banks, 3 bpp */
	ROM_LOAD( "prowbf10.bin", 0x00000, 0x2000, 0x48165902 )
	ROM_LOAD( "prowbf11.bin", 0x02000, 0x2000, 0xc3fe99c1 )
	ROM_LOAD( "prowbf12.bin", 0x04000, 0x2000, 0x69de1ea2 )
	ROM_LOAD( "prowbf13.bin", 0x06000, 0x2000, 0xecfa581d )
	ROM_LOAD( "prowbf14.bin", 0x08000, 0x2000, 0xa6721142 )
	ROM_LOAD( "prowbf15.bin", 0x0a000, 0x2000, 0xd0de7e03 )
	ROM_LOAD( "prowbf16.bin", 0x0c000, 0x2000, 0x75ee5705 )
	ROM_LOAD( "prowbf17.bin", 0x0e000, 0x2000, 0xccf42380 )
	ROM_LOAD( "prowbf18.bin", 0x10000, 0x2000, 0xe73a4bba )

	ROM_REGION(0x10000)	/* 64k for audio code */
	ROM_LOAD( "prowbf4.bin",  0x04000, 0x2000, 0x0558e1d8 )
	ROM_LOAD( "prowbf5.bin",  0x06000, 0x2000, 0xc1073f24 )
	ROM_LOAD( "prowbf6.bin",  0x08000, 0x2000, 0x208cd081 )
	ROM_LOAD( "prowbf7.bin",  0x0a000, 0x2000, 0x34a033dc )
	ROM_LOAD( "prowbf8.bin",  0x0c000, 0x2000, 0xeafe8056 )
	ROM_LOAD( "prowbf9.bin",  0x0e000, 0x2000, 0xd589ce1b )

	ROM_REGION(0x40)    /* color PROMs */
	ROM_LOAD( "fko-76.bin",   0x0000, 0x0020, 0xb6ee1483 )
	ROM_LOAD( "fjo-25.bin",   0x0020, 0x0020, 0x24da2b63 ) /* What is this prom for? */
ROM_END



static int tagteam_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0x0513],"\x19",1)==0 && memcmp(&RAM[0x0034],"\x05",1)== 0)
	{

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			/* The game ranks you on a scale of 1-50, but only displays the top 3 scores */
			osd_fread(f,&RAM[0x0406],50*3);
                        /* Game keeps copy of highest score here */
			RAM[0x0032] = RAM[0x0406];
			RAM[0x0033] = RAM[0x0407];
			RAM[0x0034] = RAM[0x0408];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void tagteam_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x406],50*3);
		osd_fclose(f);
	}
}

struct GameDriver tagteam_driver =
{
	__FILE__,
	0,
	"tagteam",
	"Tag Team Wrestling",
	"1983",
	"Data East (Technos license)",
	"Steve Ellenoff\nBrad Oliver\nCamilty (hardware info)",
	0,
	&machine_driver,
	0,

	tagteam_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tagteam_input_ports,

	PROM_MEMORY_REGION(3),
	0, 0,
	ORIENTATION_ROTATE_270,

	tagteam_hiload, tagteam_hisave
};

