/***************************************************************************

	Pang (Mitchell Corp)
	Buster Bros (Mitchell Corp?) - Encrypted
	Super Pang (Mitchell Corp)   - Encrypted
	Block Block (Capcom)         - Encrypted

	Driver submitted by Paul Leaman (paul@vortexcomputing.demon.co.uk)

	Processor:
	1xZ80 (40006psc)

	Sound:
	1xOKIM6295 (9Z002 VI)   ADPCM chip
	1xYM2413                FM Chip

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern int  pang_vh_start(void);
extern void pang_vh_stop(void);
extern void pang_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern void pang_video_bank_w(int offset, int data);
extern void pang_videoram_w(int offset, int data);
extern int  pang_videoram_r(int offset);
extern void pang_colorram_w(int offset, int data);
extern int  pang_colorram_r(int offset);
extern void pang_palette_bank_w(int offset, int data);
extern void pang_paletteram_w(int offset, int data);
extern int  pang_paletteram_r(int offset);

extern unsigned char * pang_videoram;
extern unsigned char * pang_colorram;

extern int pang_videoram_size;
extern int pang_colorram_size;

#ifdef MAME_DEBUG
void dump_driver(void)
{
	int A;
	unsigned char *RAM;
	FILE *fp=fopen("ROM.DMP", "w+b");
	if (fp)
	{
		RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
		for (A = 0x10000;A < 0x50000;A+=2)

		{
			int value=READ_WORD(RAM+A);
			fputc(value>>8, fp);
			fputc(value&0xff, fp);
		}
		fclose(fp);
	}

	fp=fopen("SOUND.DMP", "w+b");
	if (fp)
	{
			RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
			fwrite(RAM, 0x08000, 1, fp);
			fclose(fp);
	}
}
#else
#define dump_driver()
#endif

#define MACHINE_DRIVER(DRV_NAME, GFX_DECODE) \
static struct MachineDriver DRV_NAME =  \
{                                              \
	{                                           \
		{                                        \
			CPU_Z80,                              \
			4000000,        /* 4 Mhz (?) */       \
			0,                                    \
			readmem,writemem,readport,writeport,  \
			interrupt,1                           \
		},                                        \
	},                                            \
	60, DEFAULT_60HZ_VBLANK_DURATION,             \
	1,                                  \
	0,                                   \
	64*8, 32*8, { 8*8, (64-8)*8-1, 1*8, 31*8-1 }, \
	GFX_DECODE,                            \
	128*16, 128*16,\
	0,          \
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,\
	0,                                          \
	pang_vh_start,                            \
	pang_vh_stop,                             \
	pang_vh_screenrefresh,                    \
	0,0,0,0,\
	{       {   SOUND_OKIM6295,  &okim6295_interface },\
		{   SOUND_YM2413,    &ym2413_interface }, \
	}                                                  \
};

#define CHAR_LAYOUT(LAYOUT_NAME, CHARS)\
static struct GfxLayout LAYOUT_NAME =   \
{                                        \
	8,8,      /* 8*8 characters */       \
	CHARS,  /* 1024 characters */        \
	4,        /* 2 bits per pixel */      \
	{ 0x60000*8+4, 0x60000*8+0,4, 0 },    \
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },   \
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },\
	16*8    /* every char takes 16 consecutive bytes */ \
};

#define SPRITE_LAYOUT(LAYOUT_NAME, SPRITES) \
static struct GfxLayout LAYOUT_NAME = \
{                                      \
	16,16,  /* 16*16 sprites */         \
	SPRITES,   /* 1024 sprites */       \
	4,      /* 4 bits per pixel */       \
	{ 0x60000*8+4, 0x60000*8+0, 4, 0 },   \
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,      \
	  32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },\
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,        \
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 }, \
	64*8    /* every sprite takes 64 consecutive bytes */          \
};

/***************************************************************************

   Sound interfaces

***************************************************************************/

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	8000,           /* 8000Hz ??? TODO: find out the real frequency */
	2,              /* memory region 2 */
	{ 255 }
};

/***************************************************************************

 Horrible bankswitch routine. Need to be able to read decrypted opcodes
 from ROM. There must be a better way than this.

***************************************************************************/

static void pang_bankswitch_w(int offset,int data)
{
	static int olddata=-1;

	if (data != olddata)
	{
		int bankaddress;
		unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

		bankaddress = 0x10000 + (data & 0x0f) * 0x4000;

		memcpy(ROM+0x8000, RAM+bankaddress, 0x4000);
		memcpy(RAM+0x8000, RAM+bankaddress+0x20000, 0x4000);
		olddata=data;
	}
}

static struct YM2413interface ym2413_interface=
{
    1,              /* 1 chip */
    3579580,        /* FRQ */
    {255},          /* Volume */
    NULL,         /* IRQ handler */
};


/***************************************************************************

  Memory handlers

***************************************************************************/

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },            /* CODE */
	{ 0xc000, 0xc7ff, pang_paletteram_r },  /* Banked palette RAM */
	{ 0xc800, 0xcfff, pang_colorram_r },          /* Attribute RAM */
	{ 0xd000, 0xdfff, pang_videoram_r },    /* Banked char / OBJ RAM */
	{ 0xe000, 0xffff, MRA_RAM },            /* Work RAM */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, pang_paletteram_w },
	{ 0xc800, 0xcfff, pang_colorram_w, &pang_colorram, &pang_colorram_size },
	{ 0xd000, 0xdfff, pang_videoram_w, &pang_videoram, &pang_videoram_size },
	{ 0xe000, 0xffff, MWA_RAM },

	{ -1 }  /* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ 0x04, 0x04, input_port_4_r },
	{ 0x05, 0x05, input_port_5_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, pang_palette_bank_w },    /* Palette bank */
	{ 0x02, 0x02, pang_bankswitch_w },      /* Code bank register */
	{ 0x03, 0x03, YM2413_data_port_0_w  },  /* Sound ?? */
	{ 0x04, 0x04, YM2413_register_port_0_w},/* Sound ?? */
	{ 0x05, 0x05, OKIM6295_data_w },        /* ADPCM */
	{ 0x06, 0x06, MWA_NOP },                /* Watchdog ? */
	{ 0x07, 0x07, pang_video_bank_w },      /* Video RAM bank register */
	{ 0x08, 0x08, MWA_NOP },                /* ???? */
	{ 0x10, 0x10, MWA_NOP },                /* ???? */
	{ 0x18, 0x18, MWA_NOP },                /* ???? */

	{ -1 }  /* end of table */
};

/***************************************************************************

  PANG

***************************************************************************/

INPUT_PORTS_START( pang_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL)

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0xFF, 0xFF, "Dip A", IP_KEY_NONE )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0xFF, 0xFF, "Dip A", IP_KEY_NONE )
	PORT_DIPSETTING(    0xff, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW2 */
	/* All these settings (screenstop) are needed for the game to boot */
	PORT_DIPNAME( 0x89, 0x80, "Screen Stop", IP_KEY_NONE )
	PORT_DIPSETTING(    0x09, "On" )
	PORT_DIPSETTING(    0x80, "Off" )

	PORT_DIPNAME( 0x02, 0x02, "Service Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x02, "Off" )

	PORT_DIPNAME( 0x74, 0x00, "DIP C", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x74, "Off" )
INPUT_PORTS_END

CHAR_LAYOUT(pang_charlayout, 256*0x41)
SPRITE_LAYOUT(pang_spritelayout, 2048)

static struct GfxDecodeInfo gfxdecodeinfo_pang[] =
{
	{ 1, 0x00000, &pang_charlayout,     0, 128 }, /* colors  */
	{ 1, 0x40000, &pang_spritelayout,   0, 128 }, /* colors  */
	{ -1 } /* end of array */
};

MACHINE_DRIVER(pang_machine_driver, gfxdecodeinfo_pang)

ROM_START( pang_rom )
	ROM_REGION(0x60000)     /* 64k for code */
	ROM_LOAD( "pang_02.bin",  0x10000, 0x20000, 0x3f15bb61 )   /* Decrypted op codes */
	ROM_LOAD( "pang_03.bin",  0x30000, 0x20000, 0x0c8477ae )   /* Decrypted data */
	ROM_LOAD( "pang_04.bin",  0x50000, 0x10000, 0xf68f88a5 )   /* Decrypted opcode + data */

	ROM_REGION_DISPOSE(0xc0000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pang_09.bin",  0x00000, 0x20000, 0x3a5883f5 )
	ROM_LOAD( "pang_10.bin",  0x20000, 0x20000, 0x79a8ed08 )
	ROM_LOAD( "pang_08.bin",  0x40000, 0x10000, 0xf3188aa1 )
	ROM_LOAD( "pang_07.bin",  0x50000, 0x10000, 0x011da14b )

	ROM_LOAD( "pang_11.bin",  0x60000, 0x20000, 0x166a16ae )
	ROM_LOAD( "pang_12.bin",  0x80000, 0x20000, 0x2fb3db6c )
	ROM_LOAD( "pang_06.bin",  0xa0000, 0x10000, 0x0e25e797 )
	ROM_LOAD( "pang_05.bin",  0xb0000, 0x10000, 0x6daa4e27 )

	ROM_REGION(0x10000)     /* OKIM */
	ROM_LOAD( "pang_01.bin",  0x00000, 0x10000, 0xb6463907 )
ROM_END

static void pang_decode(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	memcpy(ROM, RAM+0x50000, 0x8000);   /* OP codes */
	memcpy(RAM, RAM+0x58000, 0x8000);   /* Data */
}

static int pang_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
        if ((memcmp(&RAM[0xf9e3],"\x01\x00\x00",3) == 0) &&
            (memcmp(&RAM[0xfa73],"\x00\x05\x00",3) == 0))
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0xf9e3],160);
			osd_fclose(f);

			/* copy the high score to the work RAM as well */
                        RAM[0xe00d] = RAM[0xf9e3];
                        RAM[0xe00e] = RAM[0xf9e4];
                        RAM[0xe00f] = RAM[0xf9e5];

		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void pang_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0xf9e3],160);
		osd_fclose(f);
	}
}

struct GameDriver pang_driver =
{
	__FILE__,
	0,
	"pang",
	"Pang",
	"1990",
	"Mitchell",
	"Paul Leaman (MAME driver)\nMario Silva",
	0,
	&pang_machine_driver,
	0,

	pang_rom,
	0, pang_decode,
	0,
	0,      /* sound_prom */

	pang_input_ports,

	NULL, 0, 0,
	ORIENTATION_DEFAULT,
        pang_hiload, pang_hisave
};

/***************************************************************************


  Non-working games  using the same hardware


***************************************************************************/

static void decode_main_cpu(unsigned int *decode_table, int invert)
{
	/* Oh yeah! As if.... */
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	for (A = 0x10000;A < 0x50000;A++)
	{
		int decode=decode_table[A&0xff];
		if (invert)
		{
			decode=~decode;
		}
		RAM[A]=RAM[A] ^ decode;
		ROM[A]=RAM[A];
	}

	/* Copy resident page */
	memcpy(RAM, RAM+0x4c000, 0x1000);
	memcpy(ROM, RAM+0x4c000, 0x1000);
}


/***************************************************************************

  Buster Bros.

***************************************************************************/

ROM_START( bbros_rom )
	ROM_REGION(0x30000)     /* 64k for code */

	/* Encrypted!!!! */
	ROM_LOAD( "bb7.bin",      0x10000, 0x20000, 0x0 )
	ROM_LOAD( "bb6.bin",      0x10000, 0x08000, 0x0 )

	ROM_REGION_DISPOSE(0xc0000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bb2.bin",      0x00000, 0x20000, 0x0 )
	ROM_LOAD( "bb3.bin",      0x20000, 0x20000, 0x0 )
	ROM_LOAD( "bb10.bin",     0x40000, 0x20000, 0x0 )

	ROM_LOAD( "bb4.bin",      0x60000, 0x20000, 0x0 )
	ROM_LOAD( "bb5.bin",      0x80000, 0x20000, 0x0 )
	ROM_LOAD( "bb9.bin",      0xa0000, 0x20000, 0x0 )

	ROM_REGION(0x20000)     /* OKIM */
	ROM_LOAD( "bb1.bin",      0x0000, 0x20000, 0x0 )
ROM_END

static void bbros_decode(void)
{
	static unsigned int sp_decode [256]={
	0xED,0xEB,0xEB,0xF5,0xF5,0xF3,0xF3,0xED,0xED,0xEB,0xEB,0xF5,0xF5,0xF3,0xF3,0xED,
	0xED,0xEB,0xEB,0xF5,0xF5,0xF3,0xF3,0xDD,0xDD,0xDB,0xDB,0xF5,0xF5,0xF3,0xF3,0xDD,
	0xDD,0xDB,0xDB,0xF5,0xF5,0xF3,0xF3,0xDD,0xDD,0xDB,0xDB,0xF5,0xF5,0xF3,0xF3,0xDD,
	0xDD,0xDB,0xDB,0xF5,0xF5,0xF3,0xF3,0xED,0xED,0xE7,0xE7,0xF9,0xF9,0xF3,0xF3,0xED,
	0xEE,0xE7,0xE7,0xFA,0xFA,0xF3,0xF3,0xEE,0xEE,0xE7,0xE7,0xFA,0xFA,0xF3,0xF3,0xEE,
	0xEE,0xE7,0xE7,0xFA,0xFA,0xF3,0xF3,0xDE,0xDE,0xD7,0xD7,0xFA,0xFA,0xF3,0xF3,0xDE,
	0xDE,0xD7,0xD7,0xFA,0xFA,0xF3,0xF3,0xDE,0xDE,0xD7,0xD7,0xFA,0xFA,0xF3,0xF3,0xDE,
	0xDE,0xD7,0xD7,0xFA,0xFA,0xF3,0xF3,0xED,0xED,0xEB,0xEB,0xF5,0xF5,0xF3,0xF3,0xED,
	0xEE,0xEB,0xEB,0xF6,0xF6,0xF3,0xF3,0xEE,0xEE,0xEB,0xEB,0xF6,0xF6,0xF3,0xF3,0xEE,
	0xEE,0xEB,0xEB,0xF6,0xF6,0xF3,0xF3,0xDE,0xDE,0xDB,0xDB,0xF6,0xF6,0xF3,0xF3,0xDE,
	0xDE,0xDB,0xDB,0xF6,0xF6,0xF3,0xF3,0xDE,0xDE,0xDB,0xDB,0xF6,0xF6,0xF3,0xF3,0xDE,
	0xDE,0xDB,0xDB,0xF6,0xF6,0xF3,0xF3,0xEE,0xEE,0xE7,0xE7,0xFA,0xFA,0xF3,0xF3,0xEE,
	0xED,0xE7,0xE7,0xF9,0xF9,0xF3,0xF3,0xED,0xED,0xE7,0xE7,0xF9,0xF9,0xF3,0xF3,0xED,
	0xED,0xE7,0xE7,0xF9,0xF9,0xF3,0xF3,0xDD,0xDD,0xD7,0xD7,0xF9,0xF9,0xF3,0xF3,0xDD,
	0xDD,0xD7,0xD7,0xF9,0xF9,0xF3,0xF3,0xDD,0xDD,0xD7,0xD7,0xF9,0xF9,0xF3,0xF3,0xDD,
	0xDD,0xD7,0xD7,0xF9,0xF9,0xF3,0xF3,0xEE,0xEE,0xEB,0xEB,0xF6,0xF6,0xF3,0xF3,0xEE,
	};

	decode_main_cpu(sp_decode, 1);

	dump_driver();
}



struct GameDriver bbros_driver =
{
	__FILE__,
	0,
	"bbros",
	"Buster Bros",
	"????",
	"?????",
	"Paul Leaman\n",
	GAME_NOT_WORKING,
	&pang_machine_driver,
	0,

	bbros_rom,
	0, bbros_decode,
	0,
	0,      /* sound_prom */

	pang_input_ports,

	NULL, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************

  Super Pang

***************************************************************************/

ROM_START( spang_rom )
	ROM_REGION(0x60000)     /* 64k for code */

	/* Encrypted!!!! */
	ROM_LOAD( "spe_08.rom",   0x10000, 0x20000, 0x0 )
	ROM_LOAD( "spe_07.rom",   0x30000, 0x20000, 0x0 )
	ROM_LOAD( "spe_06.rom",   0x50000, 0x08000, 0x0 )

	ROM_REGION_DISPOSE(0xc0000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "spe_04.rom",   0x00000, 0x20000, 0x0 )
	ROM_LOAD( "spe_03.rom",   0x20000, 0x20000, 0x0 )
	ROM_LOAD( "spe_09.rom",   0x40000, 0x20000, 0x0 )

	ROM_LOAD( "spe_02.rom",   0x60000, 0x20000, 0x0 )
	ROM_LOAD( "spe_05.rom",   0x80000, 0x20000, 0x0 )
	ROM_LOAD( "spe_10.rom",   0xa0000, 0x20000, 0x0 )

	ROM_REGION(0x20000)     /* OKIM */
	ROM_LOAD( "spe_01.rom",   0x0000, 0x20000, 0x0 )
ROM_END


static void spang_decode(void)
{
	static unsigned int sp_decode [256]={
	0xAE,0x5E,0x9E,0x6E,0xAE,0x5D,0x9D,0x6D,0xAD,0x5D,0x9D,0x6D,0xAD,0x5E,0x9E,0x6E,
	0xAE,0x5E,0x9E,0x6E,0xAE,0x5D,0x9D,0x6D,0xAD,0x5D,0x9D,0x6D,0xAD,0x5E,0x9E,0x6E,
	0xAE,0x5E,0x9E,0x6E,0xAE,0x5D,0x9D,0x6D,0xAD,0x5D,0x9D,0x6D,0xAD,0x5E,0x9E,0x6E,
	0xAE,0x5E,0x9E,0x6E,0xAE,0x5D,0x9D,0x6D,0xAD,0x5D,0x9D,0x6D,0xAD,0x5E,0x9E,0x6E,
	0x3E,0x3E,0x3E,0x3E,0x3E,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3E,0x3E,0x3E,
	0x3E,0x3E,0x3E,0x3E,0x3E,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3E,0x3E,0x3E,
	0x3E,0x3E,0x3E,0x3E,0x3E,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x5E,0x9E,0x6E,
	0xAE,0x5E,0x9E,0x6E,0xAE,0x5D,0x9D,0x6D,0xAD,0x5D,0x9D,0x6D,0xAD,0x5E,0x9E,0x6E,
	0x3E,0x3E,0x3E,0x3E,0x3E,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3E,0x3E,0x3E,
	0x3E,0x3E,0x3E,0x3E,0x3E,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3E,0x3E,0x3E,
	0x3E,0x3E,0x3E,0x3E,0x3E,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3E,0x3E,0x3E,
	0x3E,0x3E,0x3E,0x3E,0x3E,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3E,0x3E,0x3E,
	0xAE,0x5E,0x9E,0x6E,0xAE,0x5D,0x9D,0x6D,0xAD,0x5D,0x9D,0x6D,0xAD,0x5E,0x9E,0x6E,
	0xAE,0x5E,0x9E,0x6E,0xAE,0x5D,0x9D,0x6D,0xAD,0x5D,0x9D,0x6D,0xAD,0x5E,0x9E,0x6E,
	0xAE,0x5E,0x9E,0x6E,0xAE,0x5D,0x9D,0x6D,0xAD,0x5D,0x9D,0x6D,0xAD,0x3E,0x3E,0x3E,
	0x3E,0x3E,0x3E,0x3E,0x3E,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3D,0x3E,0x3E,0x3E
	};

	decode_main_cpu(sp_decode, 1);

	dump_driver();
}

struct GameDriver spang_driver =
{
	__FILE__,
	0,
	"spang",
	"Super Pang",
	"????",
	"?????",
	"Paul Leaman\n",
	GAME_NOT_WORKING,
	&pang_machine_driver,
	0,

	spang_rom,
	0, spang_decode,
	0,
	0,      /* sound_prom */

	pang_input_ports,

	NULL, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};


/***************************************************************************

  Block Block

***************************************************************************/


CHAR_LAYOUT(block_charlayout, 4096*2)
SPRITE_LAYOUT(block_spritelayout, 2048*2)

static struct GfxDecodeInfo gfxdecodeinfo_block[] =
{
	{ 1, 0x00000, &block_charlayout,     0, 128 },
	{ 1, 0x20000, &block_spritelayout,   0, 128 },
	{ -1 } /* end of array */
};

MACHINE_DRIVER(block_machine_driver, gfxdecodeinfo_block)

ROM_START( block_rom )
	ROM_REGION(0x60000)

	/* Encrypted!!!! */
	ROM_LOAD( "ble_07.rom",   0x10000, 0x20000, 0x0 )
	ROM_LOAD( "ble_06.rom",   0x30000, 0x20000, 0x0 )
	ROM_LOAD( "ble_05.rom",   0x00000, 0x08000, 0x0 )

	ROM_REGION_DISPOSE(0xc0000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "bl_18.rom",    0x00000, 0x20000, 0x0 )
	ROM_LOAD( "bl_17.rom",    0x20000, 0x20000, 0x0 )
	ROM_LOAD( "bl_19.rom",    0x40000, 0x20000, 0x0 )

	ROM_LOAD( "bl_08.rom",    0x60000, 0x20000, 0x0 )
	ROM_LOAD( "bl_16.rom",    0x80000, 0x20000, 0x0 )
	ROM_LOAD( "bl_09.rom",    0xa0000, 0x20000, 0x0 )

	ROM_REGION(0x20000)     /* OKIM */
	ROM_LOAD( "bl_01.rom",    0x0000, 0x20000, 0x0 )
ROM_END


static void block_decode(void)
{
	static unsigned int bl_decode [256]={
	0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,
	0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,
	0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,
	0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x01,0x40,
	0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,
	0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,
	0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,
	0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,0x02,0x80,0x02,0x40,
	0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,
	0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,
	0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,
	0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x02,0x40,
	0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,
	0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,
	0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,
	0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,0x01,0x80,0x01,0x40,
	};
	unsigned char *RAM;
	int A;

	decode_main_cpu(bl_decode, 0);

	/* Block Block's sound CPU is also encrypted */
	RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	for (A = 0;A < 0x08000;A++)
	{
		RAM[A] = RAM[A] ^ (~bl_decode[A&0xff]);
	}

	dump_driver();
}

struct GameDriver block_driver =
{
	__FILE__,
	0,
	"block",
	"Block Block",
	"????",
	"?????",
	"Paul Leaman\n",
	GAME_NOT_WORKING,
	&block_machine_driver,
	0,

	block_rom,
	0, block_decode,
	0,
	0,      /* sound_prom */

	pang_input_ports,

	NULL, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};
