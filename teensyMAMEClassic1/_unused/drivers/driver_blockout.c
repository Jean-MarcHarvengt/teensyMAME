/***************************************************************************


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *blockout_videoram;
extern unsigned char *blockout_frontvideoram;
extern unsigned char *blockout_frontcolor;

static unsigned char *ram_blockout; /* used by high scores */

void blockout_videoram_w(int offset, int data);
int blockout_videoram_r(int offset);
void blockout_frontvideoram_w(int offset, int data);
int blockout_frontvideoram_r(int offset);
void blockout_paletteram_w(int offset, int data);
void blockout_frontcolor_w(int offset, int data);
int blockout_vh_start(void);
void blockout_vh_stop(void);
void blockout_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


int blockout_interrupt(void)
{
	/* interrupt 6 is vblank */
	/* interrupt 5 reads coin inputs - might have to be triggered only */
	/* when a coin is inserted */
	return 6 - cpu_getiloops();
}

int blockout_input_r(int offset)
{
	switch (offset)
	{
		case 0:
			return input_port_0_r(offset);
		case 2:
			return input_port_1_r(offset);
		case 4:
			return input_port_2_r(offset);
		case 6:
			return input_port_3_r(offset);
		case 8:
			return input_port_4_r(offset);
		default:
if (errorlog) fprintf(errorlog,"PC %06x - read input port %06x\n",cpu_getpc(),0x100000+offset);
			return 0;
	}
}

void blockout_sound_command_w(int offset,int data)
{
	switch (offset)
	{
		case 0:
			soundlatch_w(offset,data);
			cpu_cause_interrupt(1,Z80_NMI_INT);
			break;
		case 2:
			/* don't know, maybe reset sound CPU */
			break;
	}
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x100000, 0x10000b, blockout_input_r },
	{ 0x180000, 0x1bffff, blockout_videoram_r },
	{ 0x1d4000, 0x1dffff, MRA_BANK1, &ram_blockout },	/* work RAM */
	{ 0x1f4000, 0x1fffff, MRA_BANK2 },	/* work RAM */
	{ 0x200000, 0x207fff, blockout_frontvideoram_r },
	{ 0x208000, 0x21ffff, MRA_BANK3 },	/* ??? */
	{ 0x280200, 0x2805ff, paletteram_word_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x100014, 0x100017, blockout_sound_command_w },
	{ 0x180000, 0x1bffff, blockout_videoram_w, &blockout_videoram },
	{ 0x1d4000, 0x1dffff, MWA_BANK1 },	/* work RAM */
	{ 0x1f4000, 0x1fffff, MWA_BANK2 },	/* work RAM */
	{ 0x200000, 0x207fff, blockout_frontvideoram_w, &blockout_frontvideoram },
	{ 0x208000, 0x21ffff, MWA_BANK3 },	/* ??? */
	{ 0x280002, 0x280003, blockout_frontcolor_w },
	{ 0x280200, 0x2805ff, blockout_paletteram_w, &paletteram },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8801, 0x8801, YM2151_status_port_0_r },
	{ 0x9800, 0x9800, OKIM6295_status_r },
	{ 0xa000, 0xa000, soundlatch_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, YM2151_register_port_0_w },
	{ 0x8801, 0x8801, YM2151_data_port_0_w },
	{ 0x9800, 0x9800, OKIM6295_data_w },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START
	PORT_DIPNAME( 0x03, 0x03, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )	/* unused? */
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )	/* unused? */
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )	/* unused? */
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )	/* unused? */
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Rotate Buttons", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
INPUT_PORTS_END




/* handler called by the 2151 emulator when the internal timers cause an IRQ */
static void blockout_irq_handler(void)
{
	cpu_cause_interrupt(1,0xff);
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3582071,	/* seems to be the standard */
	{ 60 },
	{ blockout_irq_handler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,              /* 1 chip */
	8000,           /* 8000Hz frequency */
	3,              /* memory region 3 */
	{ 50 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000,	/* 10 Mhz ??? */
			0,
			readmem,writemem,0,0,
			blockout_interrupt,2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* 4 Mhz ??? */
			2,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* NMIs are triggered by the main CPU, IRQs by the YM2151 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	320, 256, { 0, 319, 8, 247 },
	0,
	513, 0,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	blockout_vh_start,
	blockout_vh_stop,
	blockout_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151_ALT,
			&ym2151_interface
		},
		{
			SOUND_OKIM6295,
			&okim6295_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( blockout_rom )
	ROM_REGION(0x40000)	/* 2*128k for 68000 code */
	ROM_LOAD_EVEN( "bo29a0-2.bin", 0x00000, 0x20000, 0xb0103427 )
	ROM_LOAD_ODD ( "bo29a1-2.bin", 0x00000, 0x20000, 0x5984d5a2 )

	ROM_REGION_DISPOSE(0x800)
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "bo29e3-0.bin", 0x0000, 0x8000, 0x3ea01f78 )

	ROM_REGION(0x20000)	/* 128k for ADPCM samples - sound chip is OKIM6295 */
	ROM_LOAD( "bo29e2-0.bin", 0x0000, 0x20000, 0x15c5a99d )
ROM_END


/* hi load / save added 12/01/98 HSC */

static int hiload(void)
{

    void *f;
    /* check if the hi score table has already been initialized */
    if (READ_WORD(&ram_blockout[0x1fa4])==0x322d && READ_WORD(&ram_blockout[0x1fa6])==0x2b00 && READ_WORD(&ram_blockout[0x2012])==0x15 && READ_WORD(&ram_blockout[0x2016])==0x10)
    {
        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
			osd_fread_msbfirst(f,&ram_blockout[0x1fa4],120);
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
		osd_fwrite_msbfirst(f,&ram_blockout[0x1fa4],120);
		osd_fclose(f);
	}
}



struct GameDriver blockout_driver =
{
	__FILE__,
	0,
	"blockout",
	"Block Out",
	"1989",
	"Technos + California Dreams",
	"Nicola Salmoria\nAaron Giles (ADPCM sound)",
	0,
	&machine_driver,
	0,

	blockout_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	hiload, hisave /* hsc 12/1/98 */
};
