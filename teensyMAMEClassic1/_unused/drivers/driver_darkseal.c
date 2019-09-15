/***************************************************************************

  Dark Seal, (c) 1990 Data East Corporation (Japanese version)
  Gate Of Doom, (c) 1990 Data East Corporation (USA version)

  Basically an improved version of Midnight Resistance hardware:
    More sprites,
    More colours,
    Bigger playfields,
    Better sound,
    Encrypted roms.

  Probably same hardware as Too Crude/Crude Buster.

Sound:
  Unknown CPU, so no music or effects
  1 of the Oki chips can be triggered from sound code without need to emulate
   the sound CPU (this can be removed once sound CPU _is_ emulated...)
  YM2151 (music)
  YM2203C (effects)
  2 Oki ADPCM chips

Driver notes:
  No sound.
  Gate of Doom has no tile or sprite roms dumped as they were soldered to the
  board, it seems quite safe to use the ones from Dark Seal.

  Emulation by Bryan McPhail, mish@tendril.force9.net

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
static unsigned char *ram_drkseal; /* used by high scores */

int  darkseal_vh_start(void);
void darkseal_vh_stop(void);
void darkseal_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void darkseal_pf1_data_w(int offset,int data);
void darkseal_pf2_data_w(int offset,int data);
void darkseal_pf3_data_w(int offset,int data);
void darkseal_pf3b_data_w(int offset,int data);
void darkseal_control_0_w(int offset,int data);
void darkseal_control_1_w(int offset,int data);
void darkseal_palette_24bit_rg(int offset,int data);
void darkseal_palette_24bit_b(int offset,int data);
int darkseal_palette_24bit_rg_r(int offset);
int darkseal_palette_24bit_b_r(int offset);
extern unsigned char *darkseal_sprite;
static unsigned char *darkseal_ram;

/* System prototypes - from machine/dec0.c */
int slyspy_controls_read(int offset);

/******************************************************************************/

static void darkseal_control_w(int offset,int data)
{
	int sound;

	switch (offset) {
  	case 0xa:
    case 6: /* 0 written very frequently */
    	if (errorlog && data) fprintf(errorlog,"Warning - %02x written to control %02x\n",data,offset);
      return;
    case 8: /* Sound CPU write */
      sound=data&0xff;
//    	soundlatch_w(0,sound);
      if (sound>0x46) ADPCM_trigger(0,sound);
    	return;
  }
  if (errorlog) fprintf(errorlog,"Warning - %02x written to control %02x\n",data,offset);
}


/******************************************************************************/

static struct MemoryReadAddress darkseal_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x100000, 0x103fff, MRA_BANK1, &ram_drkseal},
	{ 0x120000, 0x1207ff, MRA_BANK2 },
	{ 0x140000, 0x140fff, darkseal_palette_24bit_rg_r },
	{ 0x141000, 0x141fff, darkseal_palette_24bit_b_r },
	{ 0x180000, 0x18000f, slyspy_controls_read },
	{ 0x220000, 0x220fff, MRA_BANK3 }, /* Palette gets moved here at colour fades */
	{ 0x222000, 0x222fff, MRA_BANK4 }, /* Palette gets moved here at colour fades */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress darkseal_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x103fff, MWA_BANK1, &darkseal_ram },
	{ 0x120000, 0x1207ff, MWA_BANK2, &darkseal_sprite },
	{ 0x140000, 0x140fff, darkseal_palette_24bit_rg, &paletteram },
	{ 0x141000, 0x141fff, darkseal_palette_24bit_b, &paletteram_2 },
	{ 0x180000, 0x18000f, darkseal_control_w },
 	{ 0x200000, 0x200fff, darkseal_pf3b_data_w }, /* 2nd half of pf3, only used on last level */
	{ 0x202000, 0x202fff, darkseal_pf3_data_w },
	{ 0x220000, 0x220fff, MWA_BANK3 }, /* Palette gets moved here at colour fades */
	{ 0x222000, 0x222fff, MWA_BANK4 }, /* Palette gets moved here at colour fades */
	{ 0x240000, 0x24000f, darkseal_control_0_w },
	{ 0x260000, 0x261fff, darkseal_pf2_data_w },
	{ 0x262000, 0x263fff, darkseal_pf1_data_w },
	{ 0x2a0000, 0x2a000f, darkseal_control_1_w },
	{ -1 }  /* end of table */
};

/******************************************************************************/

INPUT_PORTS_START( darkseal_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Credits */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x01, "2 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "3 Coin/1 Credit" )
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x18, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x10, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x08, "2 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "3 Coin/1 Credit" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x04, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, "Energy", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x20, "4" )
  	PORT_DIPSETTING(    0x10, "2.5" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x40, "Yes" )
  	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	4096,
	4,		/* 4 bits per pixel  */
	{ 0x00000*8, 0x10000*8, 0x8000*8, 0x18000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout seallayout =
{
	16,16,
	4096,
	4,
	{ 8, 0,  0x40000*8+8, 0x40000*8,},
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxLayout seallayout2 =
{
	16,16,
	4096*2,  /* A lotta sprites.. */
	4,
	{ 8, 0, 0x80000*8+8, 0x80000*8 },
	{ 32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
		0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout, 	0, 16 },	/* Characters 8x8 */
	{ 1, 0x020000, &seallayout, 	768, 16 },	/* Tiles 16x16 */
	{ 1, 0x0a0000, &seallayout, 	1024, 16 },	/* Tiles 16x16 */
	{ 1, 0x120000, &seallayout2,  256, 32 },	/* Sprites 16x16 */
	{ -1 } /* end of array */
};

/******************************************************************************/

static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 chip */
	8000,       /* 8000Hz playback */
	3,			/* memory region 3 */
	0,			/* init function */
	{ 255 }
};

static struct MachineDriver darkseal_machine_driver =
{
	/* basic machine hardware */
	{
	 	{
			CPU_M68000,
			10000000,
			0,
			darkseal_readmem,darkseal_writemem,0,0,
			m68_level6_irq,1 /* VBL */
		}, /*
		{
			CPU_??? | CPU_AUDIO_CPU,
			1250000,
			2,
			darkseal_readmem,darkseal_writemem,0,0,
			interrupt,1
		}   */
	},
	57, 1536, /* frames per second, vblank duration taken from Burger Time */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
 	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	1280, 1280, /* Space for 2048, but video hardware only uses 1280 */
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	darkseal_vh_start,
	darkseal_vh_stop,
	darkseal_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
  	{
  		{
			SOUND_ADPCM,
			&adpcm_interface
  		}
	}
};

/******************************************************************************/

ROM_START( darkseal_rom )
	ROM_REGION(0x80000) /* 68000 code */
  /* Nothing :) */

	ROM_REGION_DISPOSE(0x220000) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "fz-02.rom",    0x000000, 0x10000, 0x3c9c3012 )	/* chars */
	ROM_LOAD( "fz-03.rom",    0x010000, 0x10000, 0x264b90ed )

  	ROM_LOAD( "mac-03.rom",   0x020000, 0x80000, 0x9996f3dc ) /* tiles 1 */
	ROM_LOAD( "mac-02.rom",   0x0a0000, 0x80000, 0x49504e89 ) /* tiles 2 */
  	ROM_LOAD( "mac-00.rom",   0x120000, 0x80000, 0x52acf1d6 ) /* sprites */
  	ROM_LOAD( "mac-01.rom",   0x1a0000, 0x80000, 0xb28f7584 )

	ROM_REGION(0x10000)	/* Unknown sound CPU */
	ROM_LOAD( "fz-06.rom",    0x00000, 0x10000, 0xc4828a6d )

	ROM_REGION(0x40000)	/* ADPCM samples */
  	ROM_LOAD( "fz-08.rom",    0x00000, 0x20000, 0xc9bf68e1 )
	ROM_LOAD( "fz-07.rom",    0x20000, 0x20000, 0x588dd3cb )

	ROM_REGION(0x80000) /* Encrypted code */
  	ROM_LOAD( "ga-04.rom",    0x00000, 0x20000, 0xa1a985a9 ) /* Paired with 1 */
	ROM_LOAD( "ga-00.rom",    0x20000, 0x20000, 0xfbf3ac63 ) /* Paired with 5 */
  	ROM_LOAD( "ga-01.rom",    0x40000, 0x20000, 0x98bd2940 )
 	ROM_LOAD( "ga-05.rom",    0x60000, 0x20000, 0xd5e3ae3f )
ROM_END

ROM_START( gatedoom_rom )
	ROM_REGION(0x80000) /* 68000 code */
  	/* Nothing :) */

	ROM_REGION_DISPOSE(0x220000) /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "fz-02.rom",    0x000000, 0x10000, 0x3c9c3012 )	/* chars */
	ROM_LOAD( "fz-03.rom",    0x010000, 0x10000, 0x264b90ed )

  	/* the following four have not been verified on a real Gate of Doom */
	/* board - might be different from Dark Seal! */
	ROM_LOAD( "mac-03.rom",   0x020000, 0x80000, 0x9996f3dc ) /* tiles 1 */
  	ROM_LOAD( "mac-02.rom",   0x0a0000, 0x80000, 0x49504e89 ) /* tiles 2 */
	ROM_LOAD( "mac-00.rom",   0x120000, 0x80000, 0x52acf1d6 ) /* sprites */
 	ROM_LOAD( "mac-01.rom",   0x1a0000, 0x80000, 0xb28f7584 )

	ROM_REGION(0x10000)	/* Unknown sound CPU */
	ROM_LOAD( "fz-06.rom",    0x00000, 0x10000, 0xc4828a6d )

	ROM_REGION(0x40000)	/* ADPCM samples */
  	ROM_LOAD( "fz-08.rom",    0x00000, 0x20000, 0xc9bf68e1 )
	ROM_LOAD( "fz-07.rom",    0x20000, 0x20000, 0x588dd3cb )

	ROM_REGION(0x80000) /* Encrypted code */
	ROM_LOAD( "gb04.bin",     0x00000, 0x20000, 0x4c3bbd2b ) /* Paired with 1 */
	ROM_LOAD( "gb00.bin",     0x20000, 0x20000, 0xa88c16a1 ) /* Paired with 5 */
	ROM_LOAD( "gb01.bin",     0x40000, 0x20000, 0x59e367f4 )
 	ROM_LOAD( "gb05.bin",     0x60000, 0x20000, 0x252d7e14 )
ROM_END

/******************************************************************************/

static void darkseal_decrypt(void)
{
	unsigned char *RAM = Machine->memory_region[4];
	unsigned char *OUT = Machine->memory_region[0];
	int newword,i;

	/* Read each byte, decrypt it, and make it a word so 68000 core can read it */
	for (i=0x00000; i<0x40000; i++) {
		int swap1=RAM[i]&0x40;
		int swap2=RAM[i]&0x2;

		/* Mask to 0xbd, misses bits 2 & 7 */
		RAM[i]=(RAM[i]&0xbd);
		if (swap1) RAM[i]+=0x2;
		if (swap2) RAM[i]+=0x40;

		swap1=RAM[i+0x40000]&0x40;
		swap2=RAM[i+0x40000]&0x2;

		/* Mask to 0xbd, misses bits 2 & 7 */
		RAM[i+0x40000]=(RAM[i+0x40000]&0xbd);
		if (swap1) RAM[i+0x40000]+=0x2;
		if (swap2) RAM[i+0x40000]+=0x40;

		newword=((RAM[i])<<8) + RAM[i+0x40000];
		WRITE_WORD (&OUT[i*2],newword);
		WRITE_WORD (&ROM[i*2],newword);
	}

	/* Free encrypted memory region */
	free(Machine->memory_region[4]);
	Machine->memory_region[4] = 0;
}

/******************************************************************************/

/* Why do I have to double length???  Bug in ADPCM code? */
ADPCM_SAMPLES_START(darkseal_samples)
ADPCM_SAMPLE(0x47,0x00f8,0x1000*2)
ADPCM_SAMPLE(0x48,0x10f8,0x0600*2)
ADPCM_SAMPLE(0x49,0x16f8,0x1000*2)
ADPCM_SAMPLE(0x4a,0x26f8,0x1600*2)
ADPCM_SAMPLE(0x4b,0x3cf8,0x1200*2)
ADPCM_SAMPLE(0x4c,0x4ef8,0x0800*2)
ADPCM_SAMPLE(0x4d,0x56f8,0x1200*2)
ADPCM_SAMPLE(0x4e,0x68f8,0x1000*2)
ADPCM_SAMPLE(0x4f,0x78f8,0x1200*2)
ADPCM_SAMPLE(0x50,0x8af8,0x0600*2)
ADPCM_SAMPLE(0x51,0x90f8,0x1200*2)
ADPCM_SAMPLE(0x52,0xa2f8,0x1000*2)
ADPCM_SAMPLE(0x53,0xb2f8,0x1200*2)
ADPCM_SAMPLE(0x54,0xc4f8,0x0800*2)
ADPCM_SAMPLE(0x55,0xccf8,0x0e00*2)
ADPCM_SAMPLE(0x56,0xdaf8,0x1a00*2)
ADPCM_SAMPLE(0x57,0xf4f8,0x3000*2)
ADPCM_SAMPLE(0x58,0x124f8,0x1a00*2)
ADPCM_SAMPLE(0x59,0x13ef8,0x1600*2)
ADPCM_SAMPLE(0x5a,0x154f8,0x1000*2)
ADPCM_SAMPLE(0x5b,0x164f8,0x0600*2)
ADPCM_SAMPLE(0x5c,0x16af8,0x1200*2)
ADPCM_SAMPLE(0x5d,0x17cf8,0x0600*2)
ADPCM_SAMPLE(0x5e,0x182f8,0x0a00*2)
ADPCM_SAMPLE(0x5f,0x18cf8,0x1600*2)
ADPCM_SAMPLE(0x60,0x1a2f8,0x0800*2)
ADPCM_SAMPLE(0x61,0x1aaf8,0x0400*2)
ADPCM_SAMPLE(0x62,0x1aef8,0x0800*2)
ADPCM_SAMPLE(0x63,0x1b6f8,0x1000*2)
ADPCM_SAMPLE(0x64,0x1c6f8,0x0600*2)
ADPCM_SAMPLE(0x65,0x1ccf8,0x2000*2)
ADPCM_SAMPLES_END



/* hi load / save added 12/02/98 HSC */

static int hiload(void)
{
    void *f;
    /* check if the hi score table has already been initialized */
    if (READ_WORD(&ram_drkseal[0x3e00])==0x50 && READ_WORD(&ram_drkseal[0x3e04])==0x50 && READ_WORD(&ram_drkseal[0x3e32])==0x4800 && READ_WORD(&ram_drkseal[0x3e34])==0x462e)
    {
        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
			osd_fread_msbfirst(f,&ram_drkseal[0x3e00],56);
			osd_fclose(f);

		}

		return 1;
	}
    else return 0;  /* we can't load the hi scores yet */
}

static void hisave(void)
{
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
        {
				osd_fwrite_msbfirst(f,&ram_drkseal[0x3e00],56);
				osd_fclose(f);
        }
}


struct GameDriver darkseal_driver =
{
	__FILE__,
	0,
	"darkseal",
	"Dark Seal",
	"1990",
	"Data East Corporation",
	"Bryan McPhail",
	0,
	&darkseal_machine_driver,
	0,

	darkseal_rom,
	0, darkseal_decrypt,
	0,
	(void *)darkseal_samples,

	darkseal_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	hiload , hisave  /* hsc 12/02/98 */
};

struct GameDriver gatedoom_driver =
{
	__FILE__,
	&darkseal_driver,
	"gatedoom",
	"Gate Of Doom",
	"1990",
	"Data East Corporation",
	"Bryan McPhail",
	0,
	&darkseal_machine_driver,
	0,

	gatedoom_rom,
	0, darkseal_decrypt,
	0,
	(void *)darkseal_samples,

	darkseal_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	hiload , hisave /* hsc 12/02/98 */
};

