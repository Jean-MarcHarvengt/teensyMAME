/***************************************************************************

 Berzerk Driver by Zsolt Vasvari
 Sound Driver by Alex Judd

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char* magicram;

void berzerk_init_machine(void);

int  berzerk_interrupt(void);
void berzerk_interrupt_enable_w(int offset,int data);
void berzerk_enable_nmi_w(int offset,int data);
void berzerk_disable_nmi_w(int offset,int data);
int  berzerk_enable_nmi_r(int offset);
int  berzerk_disable_nmi_r(int offset);
int berzerk_led_on_w(int offset);
int berzerk_led_off_w(int offset);
int berzerk_debug_read(int offset);
int berzerk_voiceboard_read(int offset);
void berzerk_videoram_w(int offset,int data);

void berzerk_videoram_w(int offset,int data);

void berzerk_colorram_w(int offset,int data);

int  frenzy_mirror_r(int offset);
void frenzy_mirror_w(int offset,int data);

void berzerk_magicram_w(int offset,int data);
int  berzerk_magicram_r(int offset);
void berzerk_magicram_control_w(int offset,int data);
int  berzerk_collision_r(int offset);

void berzerk_sound_control_a_w(int offset, int data);
void berzerk_sound_control_b_w(int offset, int data);
int berzerk_sh_start(void);
void berzerk_sh_update(void);

void berzerk_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static struct MemoryReadAddress readmem[] =
{
        { 0x0000, 0x07ff, MRA_ROM},
        { 0x0800, 0x09ff, MRA_RAM},
        { 0x1000, 0x3fff, MRA_ROM},
        { 0x4000, 0x5fff, videoram_r},
        { 0x6000, 0x7fff, berzerk_magicram_r},
        { 0x8000, 0x87ff, colorram_r},
        { -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
        { 0x0800, 0x09ff, MWA_RAM},
        { 0x4000, 0x5fff, berzerk_videoram_w, &videoram, &videoram_size},
        { 0x6000, 0x7fff, berzerk_magicram_w, &magicram},
        { 0x8000, 0x87ff, berzerk_colorram_w, &colorram},
        { -1 }  /* end of table */
};


static struct MemoryReadAddress frenzy_readmem[] =
{
        { 0x0000, 0x3fff, MRA_ROM },
        { 0x4000, 0x5fff, videoram_r },
        { 0x6000, 0x7fff, berzerk_magicram_r },
        { 0x8000, 0x87ff, colorram_r },
        { 0xc000, 0xcfff, MRA_ROM },
        { 0xf800, 0xffff, frenzy_mirror_r },
        { -1 }  /* end of table */
};

static struct MemoryWriteAddress frenzy_writemem[] =
{
        { 0x4000, 0x5fff, berzerk_videoram_w, &videoram, &videoram_size},
        { 0x6000, 0x7fff, berzerk_magicram_w, &magicram},
        { 0x8000, 0x87ff, berzerk_colorram_w, &colorram},
        { 0xf800, 0xffff, frenzy_mirror_w },
        { -1 }  /* end of table */
};


int  frenzy_io62_r(int offset)
{
        return 1;
}

static struct IOReadPort readport[] =
{
        { 0x40, 0x43, IORP_NOP}, /* Sound stuff */
        { 0x44, 0x44, berzerk_voiceboard_read}, /* Sound stuff */
        { 0x45, 0x47, IORP_NOP}, /* Sound stuff */
        { 0x48, 0x48, input_port_0_r},
        { 0x49, 0x49, input_port_1_r},
        { 0x4a, 0x4a, input_port_7_r}, /* Same as 48 for Player 2 */
        { 0x4c, 0x4c, berzerk_enable_nmi_r},
        { 0x4d, 0x4d, berzerk_disable_nmi_r},
        { 0x4e, 0x4e, berzerk_collision_r},
        { 0x50, 0x57, IORP_NOP}, /* Sound stuff */
        { 0x60, 0x60, input_port_2_r},
        { 0x61, 0x61, input_port_3_r},
        { 0x62, 0x62, frenzy_io62_r},   /* I really need 1 more I/O port here */
        { 0x63, 0x63, input_port_4_r},
        { 0x64, 0x64, input_port_5_r},
        { 0x65, 0x65, input_port_6_r},
        { 0x66, 0x66, berzerk_led_off_w},
        { 0x67, 0x67, berzerk_led_on_w},
        { -1 }  /* end of table */
};


static struct IOWritePort writeport[] =
{
        { 0x40, 0x46, berzerk_sound_control_a_w}, /* First sound board */
        { 0x47, 0x47, IOWP_NOP}, /* not used sound stuff */
        { 0x4b, 0x4b, berzerk_magicram_control_w},
        { 0x4c, 0x4c, berzerk_enable_nmi_w},
        { 0x4d, 0x4d, berzerk_disable_nmi_w},
        { 0x4f, 0x4f, berzerk_interrupt_enable_w},
        { 0x50, 0x57, IOWP_NOP}, /* Second sound board but not used */
        { -1 }  /* end of table */
};


INPUT_PORTS_START( input_ports )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
        PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

        PORT_START      /* IN1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x7c, IP_ACTIVE_LOW, IPT_UNUSED )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

        PORT_START      /* IN2 */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* Has to do with */
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* test modes */
        PORT_BIT( 0x3c, IP_ACTIVE_LOW,  IPT_UNUSED )
        PORT_DIPNAME( 0xC0, 0x00, "Language", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "English" )
        PORT_DIPSETTING(    0x40, "German" )
        PORT_DIPSETTING(    0x80, "French" )
        PORT_DIPSETTING(    0xc0, "Spanish" )

        PORT_START      /* IN3 */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* Has to do with */
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* test modes */
        PORT_BIT( 0x3c, IP_ACTIVE_LOW,  IPT_UNUSED )
        PORT_DIPNAME( 0x40, 0x40, "Extra Man at  5,000 Pts", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0x40, "On" )
        PORT_DIPNAME( 0x80, 0x80, "Extra Man at 10,000 Pts", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0x80, "On" )

        PORT_START      /* IN4 - Coin Chute 2*/
        PORT_BIT( 0xff, IP_ACTIVE_LOW,  IPT_UNUSED )

        PORT_START      /* IN5 */
        PORT_DIPNAME( 0x0f, 0x00, "Credits/Coins", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, " 1/1" )
        PORT_DIPSETTING(    0x01, " 2/1" )
        PORT_DIPSETTING(    0x02, " 3/1" )
        PORT_DIPSETTING(    0x03, " 4/1" )
        PORT_DIPSETTING(    0x04, " 5/1" )
        PORT_DIPSETTING(    0x05, " 6/1" )
        PORT_DIPSETTING(    0x06, " 7/1" )
        PORT_DIPSETTING(    0x07, "10/1" )
        PORT_DIPSETTING(    0x08, "14/1" )
        PORT_DIPSETTING(    0x09, " 1/2" )
        PORT_DIPSETTING(    0x0a, " 3/2" )
        PORT_DIPSETTING(    0x0b, " 5/2" )
        PORT_DIPSETTING(    0x0c, " 7/2" )
        PORT_DIPSETTING(    0x0d, " 3/4" )
        PORT_DIPSETTING(    0x0e, " 5/4" )
        PORT_DIPSETTING(    0x0f, " 7/4" )
        PORT_BIT( 0xf0, IP_ACTIVE_LOW,  IPT_UNUSED )

        PORT_START      /* IN6 */
        PORT_DIPNAME( 0x01, 0x00, "Free Play", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0x01, "On" )
        PORT_BIT( 0x7e, IP_ACTIVE_LOW,  IPT_UNUSED )
        PORT_BITX(0x80, IP_ACTIVE_HIGH, 0, "Stats", OSD_KEY_F1, IP_JOY_NONE, 0 )

        PORT_START      /* IN7 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
        PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNUSED )
        PORT_DIPNAME( 0x80, 0x80, "Cocktail Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "Off" )
        PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( frenzy_input_ports )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
        PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

        PORT_START      /* IN1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x7c, IP_ACTIVE_LOW, IPT_UNUSED )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

        PORT_START      /* IN2 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* Has to do with */
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* test modes */
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* test modes */
        PORT_BIT( 0x38, IP_ACTIVE_HIGH,  IPT_UNUSED )
        PORT_DIPNAME( 0xC0, 0x00, "Language", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "English" )
        PORT_DIPSETTING(    0x40, "German" )
        PORT_DIPSETTING(    0x80, "French" )
        PORT_DIPSETTING(    0xc0, "Spanish" )

        PORT_START      /* IN3 */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* If low, do a self-test? */
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* ?? */
        PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* If low, do dipswitch test */
        PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* If low, do grid test */
        PORT_BIT( 0x30, IP_ACTIVE_HIGH,  IPT_UNUSED )
        PORT_DIPNAME( 0x40, 0x40, "Extra Man at  5,000 Pts", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0x40, "On" )
        PORT_DIPNAME( 0x80, 0x80, "Extra Man at 10,000 Pts", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0x80, "On" )

        PORT_START      /* IN4 - Coin Chute 2 */
        PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_COIN2 )

        PORT_START      /* IN5 - 0x01 */
        PORT_DIPNAME( 0x0f, 0x01, "Credits/Coins", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, " 1/1" )
        PORT_DIPSETTING(    0x01, " 2/1" )
        PORT_DIPSETTING(    0x02, " 3/1" )
        PORT_DIPSETTING(    0x03, " 4/1" )
        PORT_DIPSETTING(    0x04, " 5/1" )
        PORT_DIPSETTING(    0x05, " 6/1" )
        PORT_DIPSETTING(    0x06, " 7/1" )
        PORT_DIPSETTING(    0x07, "10/1" )
        PORT_DIPSETTING(    0x08, "14/1" )
        PORT_DIPSETTING(    0x09, " 1/2" )
        PORT_DIPSETTING(    0x0a, " 3/2" )
        PORT_DIPSETTING(    0x0b, " 5/2" )
        PORT_DIPSETTING(    0x0c, " 7/2" )
        PORT_DIPSETTING(    0x0d, " 3/4" )
        PORT_DIPSETTING(    0x0e, " 5/4" )
        PORT_DIPSETTING(    0x0f, " 7/4" )
        PORT_BIT( 0xf0, IP_ACTIVE_LOW,  IPT_UNUSED )

        PORT_START      /* IN6 */
        PORT_DIPNAME( 0x01, 0x00, "Free Play", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPSETTING(    0x01, "On" )
        PORT_BIT( 0x7e, IP_ACTIVE_LOW,  IPT_UNUSED )
        PORT_BITX(0x80, IP_ACTIVE_HIGH, 0, "Stats", OSD_KEY_F1, IP_JOY_NONE, 0 )

        PORT_START      /* IN7 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
        PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNUSED )
        PORT_DIPNAME( 0x80, 0x80, "Cocktail Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "Off" )
        PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



/* Simple 1-bit RGBI palette */
unsigned char berzerk_palette[16 * 3] =
{
        0x00, 0x00, 0x00,
        0xff, 0x00, 0x00,
        0x00, 0xff, 0x00,
        0xff, 0xff, 0x00,
        0x00, 0x00, 0xff,
        0xff, 0x00, 0xff,
        0x00, 0xff, 0xff,
        0xff, 0xff, 0xff,
        0x40, 0x40, 0x40,
        0xff, 0x40, 0x40,
        0x40, 0xff, 0x40,
        0xff, 0xff, 0x40,
        0x40, 0x40, 0xff,
        0xff, 0x40, 0xff,
        0x40, 0xff, 0xff,
        0xff, 0xff, 0xff
};

static struct Samplesinterface berzerk_samples_interface =
{
	8	/* 8 channels */
};

static struct MachineDriver berzerk_machine_driver =
{
	/* basic machine hardware */
	{
			{
					CPU_Z80,
					2500000,        /* 2.5 MHz */
					0,
					readmem,writemem,readport,writeport,
					berzerk_interrupt,8
			},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	berzerk_init_machine,

	/* video hardware */
	256, 256, { 0, 256-1, 0, 256-1 },
	0,
	sizeof(berzerk_palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	berzerk_vh_screenrefresh,

	/* sound hardware */
	0,
	berzerk_sh_start,
	0,
	berzerk_sh_update,
	{
		{
			SOUND_SAMPLES,
			&berzerk_samples_interface
		}
	}
};


static struct MachineDriver frenzy_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2500000,        /* 2.5 MHz */
			0,
			frenzy_readmem,frenzy_writemem,readport,writeport,
			berzerk_interrupt,8
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	256, 256, { 0, 256-1, 0, 256-1 },
	0,
	sizeof(berzerk_palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	berzerk_vh_screenrefresh,

	/* sound hardware */
	0,
	berzerk_sh_start,
	0,
	berzerk_sh_update,
	{
		{
			SOUND_SAMPLES,
			&berzerk_samples_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( berzerk1_rom )
        ROM_REGION(0x10000)
        ROM_LOAD( "rom0.1c",      0x0000, 0x0800, 0x5b7eb77d )
        ROM_LOAD( "rom1.1d",      0x1000, 0x0800, 0xe58c8678 )
        ROM_LOAD( "rom2.3d",      0x1800, 0x0800, 0x705bb339 )
        ROM_LOAD( "rom3.5d",      0x2000, 0x0800, 0x6a1936b4 )
        ROM_LOAD( "rom4.6d",      0x2800, 0x0800, 0xfa5dce40 )
        ROM_LOAD( "rom5.5c",      0x3000, 0x0800, 0x2579b9f4 )
ROM_END

ROM_START( berzerk_rom )
        ROM_REGION(0x10000)
        ROM_LOAD( "1c-0",         0x0000, 0x0800, 0xca566dbc )
        ROM_LOAD( "1d-1",         0x1000, 0x0800, 0x7ba69fde )
        ROM_LOAD( "3d-2",         0x1800, 0x0800, 0xa1d5248b )
        ROM_LOAD( "5d-3",         0x2000, 0x0800, 0xfcaefa95 )
        ROM_LOAD( "6d-4",         0x2800, 0x0800, 0x1e35b9a0 )
        ROM_LOAD( "5c-5",         0x3000, 0x0800, 0xc8c665e5 )
ROM_END

ROM_START( frenzy_rom )
        ROM_REGION(0x10000)
        ROM_LOAD( "1c-0",         0x0000, 0x1000, 0xabdd25b8 )
        ROM_LOAD( "1d-1",         0x1000, 0x1000, 0x536e4ae8 )
        ROM_LOAD( "3d-2",         0x2000, 0x1000, 0x3eb9bc9b )
        ROM_LOAD( "5d-3",         0x3000, 0x1000, 0xe1d3133c )
        ROM_LOAD( "6d-4",         0xc000, 0x1000, 0x5581a7b1 )
        /* 1c & 2c are the voice ROMs */
ROM_END

ROM_START( frenzy1_rom )
        ROM_REGION(0x10000)
        ROM_LOAD( "frenzy01.bin", 0x0000, 0x4000, 0xeb70f964 )
        ROM_LOAD( "frenzy02.bin", 0xc000, 0x1000, 0x5581a7b1 )
ROM_END

static const char *berzerk_sample_names[] =
{
	"*berzerk", /* universal samples directory */
	"",
	"01.sam", // "kill"
	"02.sam", // "attack"
	"03.sam", // "charge"
	"04.sam", // "got"
	"05.sam", // "to"
	"06.sam", // "get"
	"",
	"08.sam", // "alert"
	"09.sam", // "detected"
	"10.sam", // "the"
	"11.sam", // "in"
	"12.sam", // "it"
	"",
	"",
	"15.sam", // "humanoid"
	"16.sam", // "coins"
	"17.sam", // "pocket"
	"18.sam", // "intruder"
	"",
	"20.sam", // "escape"
	"21.sam", // "destroy"
	"22.sam", // "must"
	"23.sam", // "not"
	"24.sam", // "chicken"
	"25.sam", // "fight"
	"26.sam", // "like"
	"27.sam", // "a"
	"28.sam", // "robot"
	"",
	"30.sam", // player fire
	"31.sam", // baddie fire
	"32.sam", // kill baddie
	"33.sam", // kill human (real)
	"34.sam", // kill human (cheat)
	0	/* end of array */
};



static int berzerk_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[0];
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0x0800],0x0800);
		osd_fclose(f);
	}

	return 1;
}

static void berzerk_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0800],0x800);
		osd_fclose(f);
	}
}

static int frenzy_hiload(void)
{
	static int firsttime = 0;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	/* the high score table is intialized to all 0, so first of all */
	/* we dirty it, then we wait for it to be cleared again */
	if (firsttime == 0)
	{
		memset(&RAM[0x406f],0xff,59);
		firsttime = 1;
	}

	if (memcmp(&RAM[0x406F],"\x00\x00\x00",3) == 0 &&
			memcmp(&RAM[0x40A7],"\x00\x00\x00",3) == 0 &&
			memcmp (&RAM[0x5c81],"\x04\x10\x80",3)==0 )
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x406f],59);
			osd_fclose(f);
		}

		firsttime = 0;
		return 1;
	}
	else return 0;   /* we can't load the hi scores yet */
}

static void frenzy_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x406f],59);
		osd_fclose(f);
	}
}



struct GameDriver berzerk_driver =
{
	__FILE__,
	0,
	"berzerk",
	"Berzerk",
	"1980",
	"Stern",
	"Zsolt Vasvari\nChristopher Kirmse\nMirko Buffoni\nValerio Verrando\nDouglas Silfen\nAlex Judd (Sound Programming)",
	0,
	&berzerk_machine_driver,
	0,

	berzerk_rom,
	0, 0,
	berzerk_sample_names,
	0,	/* sound_prom */

	input_ports,

	0, berzerk_palette, 0,

	ORIENTATION_DEFAULT,

	berzerk_hiload, berzerk_hisave
};


struct GameDriver berzerk1_driver =
{
	__FILE__,
	&berzerk_driver,
	"berzerk1",
	"Berzerk (version 1)",
	"1980",
	"Stern",
	"Zsolt Vasvari\nChristopher Kirmse\nMirko Buffoni\nValerio Verrando\nDouglas Silfen\nAlex Judd (Sound Programming)",
	0,
	&berzerk_machine_driver,
	0,

	berzerk1_rom,
	0, 0,
	berzerk_sample_names,
	0,	/* sound_prom */

	input_ports,

	0, berzerk_palette, 0,

	ORIENTATION_DEFAULT,

	berzerk_hiload, berzerk_hisave
};


struct GameDriver frenzy_driver =
{
	__FILE__,
	0,
	"frenzy",
	"Frenzy",
	"1982",
	"Stern",
	"Keith Gerdes\nMirko Buffoni\nMike Cuddy\nBrad Oliver\nZsolt Vasvari\nChristopher Kirmse",
	0,
	&frenzy_machine_driver,
	0,

	frenzy_rom,
	0, 0,
	berzerk_sample_names,
	0,	/* sound_prom */

	frenzy_input_ports,

	0, berzerk_palette, 0,

	ORIENTATION_DEFAULT,

	frenzy_hiload, frenzy_hisave
};

struct GameDriver frenzy1_driver =
{
	__FILE__,
	&frenzy_driver,
	"frenzy1",
	"Frenzy (version 1)",
	"1982",
	"Stern",
	"Keith Gerdes\nMirko Buffoni\nMike Cuddy\nBrad Oliver\nZsolt Vasvari\nChristopher Kirmse",
	0,
	&frenzy_machine_driver,
	0,

	frenzy1_rom,
	0, 0,
	berzerk_sample_names,
	0,	/* sound_prom */

	frenzy_input_ports,

	0, berzerk_palette, 0,

	ORIENTATION_DEFAULT,

	frenzy_hiload, frenzy_hisave
};
