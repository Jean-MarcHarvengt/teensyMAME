#include "driver.h"
#include "vidhrdw/generic.h"


static unsigned char *ram;
extern unsigned char *toki_foreground_videoram;
extern unsigned char *toki_background1_videoram;
extern unsigned char *toki_background2_videoram;
extern unsigned char *toki_sprites_dataram;
extern unsigned char *toki_scrollram;

extern int toki_foreground_videoram_size;
extern int toki_background1_videoram_size;
extern int toki_background2_videoram_size;
extern int toki_sprites_dataram_size;

int toki_interrupt(void);
int  toki_vh_start(void);
void toki_vh_stop(void);
void toki_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int  toki_foreground_videoram_r(int offset);
void toki_foreground_videoram_w(int offset,int data);
int  toki_background1_videoram_r(int offset);
void toki_background1_videoram_w(int offset,int data);
int  toki_background2_videoram_r(int offset);
void toki_background2_videoram_w(int offset,int data);
void toki_linescroll_w(int offset,int data);


int toki_read_ports(int offset)
{
    switch(offset)
    {
		case 0:
			return input_port_3_r(0) + (input_port_4_r(0) << 8);
		case 2:
			return input_port_1_r(0) + (input_port_2_r(0) << 8);
		case 4:
			return input_port_0_r(0);
		default:
			return 0;
    }
}

void toki_soundcommand_w(int offset,int data)
{
	soundlatch_w(0,data & 0xff);
	cpu_cause_interrupt(1,0xff);
}

void toki_adpcm_int (int data)
{
	static int toggle;

	toggle = 1 - toggle;
	if (toggle)
		cpu_cause_interrupt(1,Z80_NMI_INT);
}

void toki_adpcm_control_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];


	/* the code writes either 2 or 3 in the bottom two bits */
	bankaddress = 0x10000 + (data & 0x01) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);

	MSM5205_reset_w(0,data & 0x08);
}

void toki_adpcm_data_w(int offset,int data)
{
	MSM5205_data_w(offset,data & 0x0f);
	MSM5205_data_w(offset,data >> 4);
}

static int pip(int offset)
{
	return 0xffff;
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x05ffff, MRA_ROM },
	{ 0x060000, 0x06dfff, MRA_BANK2, &ram },
	{ 0x06e000, 0x06e7ff, paletteram_word_r },
	{ 0x06e800, 0x06efff, toki_background1_videoram_r },
	{ 0x06f000, 0x06f7ff, toki_background2_videoram_r },
	{ 0x06f800, 0x06ffff, toki_foreground_videoram_r },
	{ 0x072000, 0x072001, watchdog_reset_r },	/* probably */
	{ 0x0c0000, 0x0c0005, toki_read_ports },
	{ 0x0c000e, 0x0c000f, pip },	/* sound related, if we return 0 the code writes */
									/* the sound command quickly followed by 0 and the */
									/* sound CPU often misses the command. */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x05ffff, MWA_ROM },
	{ 0x060000, 0x06dfff, MWA_BANK2 },
	{ 0x06e000, 0x06e7ff, paletteram_xxxxBBBBGGGGRRRR_word_w, &paletteram },
	{ 0x06e800, 0x06efff, toki_background1_videoram_w, &toki_background1_videoram, &toki_background1_videoram_size },
	{ 0x06f000, 0x06f7ff, toki_background2_videoram_w, &toki_background2_videoram, &toki_background2_videoram_size },
	{ 0x06f800, 0x06ffff, toki_foreground_videoram_w, &toki_foreground_videoram, &toki_foreground_videoram_size },
	{ 0x071000, 0x071001, MWA_NOP },	/* sprite related? seems another scroll register; */
										/* gets written the same value as 75000a (bg2 scrollx) */
	{ 0x071804, 0x071807, MWA_NOP },	/* sprite related; always 01be0100 */
	{ 0x07180e, 0x071e45, MWA_BANK3, &toki_sprites_dataram, &toki_sprites_dataram_size },
	{ 0x075000, 0x075001, toki_soundcommand_w },
	{ 0x075004, 0x07500b, MWA_BANK4, &toki_scrollram },
	{ 0x0a002a, 0x0a002d, toki_linescroll_w },	/* scroll register used to waggle the title screen */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xec00, 0xec00, YM3812_status_port_0_r },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, soundlatch_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xe000, 0xe000, toki_adpcm_control_w },	/* MSM5205 + ROM bank */
	{ 0xe400, 0xe400, toki_adpcm_data_w },
	{ 0xec00, 0xec00, YM3812_control_port_0_w },
	{ 0xec01, 0xec01, YM3812_write_port_0_w },
	{ 0xec08, 0xec08, YM3812_control_port_0_w },	/* mirror address, it seems */
	{ 0xec09, 0xec09, YM3812_write_port_0_w },	/* mirror address, it seems */
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x1F, 0x1F, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x15, "6 Coins/1 Credit" )
	PORT_DIPSETTING(    0x17, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x19, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x1B, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "8 Coins/3 Credits" )
	PORT_DIPSETTING(    0x1D, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "5 Coins/3 Credits" )
	PORT_DIPSETTING(    0x07, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x1F, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x09, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x13, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x11, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0F, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0D, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0B, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x1E, "A 1/1 B 1/2" )
	PORT_DIPSETTING(    0x14, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x0A, "A 3/1 B 1/5" )
	PORT_DIPSETTING(    0x00, "A 5/1 B 1/6" )
	PORT_DIPSETTING(    0x01, "Free Play" )
	PORT_DIPNAME( 0x20, 0x20, "Joysticks", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x80, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x0C, 0x0C, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "50000 150000" )
	PORT_DIPSETTING(    0x00, "70000 140000 210000" )
	PORT_DIPSETTING(    0x0C, "70000" )
	PORT_DIPSETTING(    0x04, "100000 200000" )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Medium" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x40, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8 by 8 */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
        {4096*8*8*3,4096*8*8*2,4096*8*8*1,4096*8*8*0 },	/* planes */
	{ 0, 1,  2,  3,  4,  5,  6,  7},		/* x bit */
	{ 0, 8, 16, 24, 32, 40, 48, 56},		/* y bit */
	8*8
};

static struct GfxLayout backgroundlayout =
{
	16,16,	/* 16 by 16 */
	4096,	/* 4096 characters */
	4,	/* 4 bits per pixel */
        { 4096*16*16*3,4096*16*16*2,4096*16*16*1,4096*16*16*0 },	/* planes */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  0x8000*8+0, 0x8000*8+1, 0x8000*8+2, 0x8000*8+3, 0x8000*8+4,
	  0x8000*8+5, 0x8000*8+6, 0x8000*8+7 },				/* x bit */
	{
	  0,8,16,24,32,40,48,56,
	  0x10000*8+ 0, 0x10000*8+ 8, 0x10000*8+16, 0x10000*8+24, 0x10000*8+32,
	  0x10000*8+40, 0x10000*8+48, 0x10000*8+56 },			/* y bit */
	8*8
};

static struct GfxLayout spriteslayout =
{
	16,16,	/* 16 by 16 */
	8192,	/* 8192 sprites */
	4,	/* 4 bits per pixel */
        { 8192*16*16*3,8192*16*16*2,8192*16*16*1,8192*16*16*0 },	/* planes */
	{    0,     1,     2,     3,     4,     5,     6,     7,
	 128+0, 128+1, 128+2, 128+3, 128+4, 128+5, 128+6, 128+7 },	/* x bit */
	{ 0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120 },		/* y bit */
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout,			16*16,  16 },
	{ 1, 0x020000, &spriteslayout,		 0*16,  16 },
	{ 1, 0x120000, &backgroundlayout,	32*16,  16 },
	{ 1, 0x1a0000, &backgroundlayout,	48*16,  16 },
	{ -1 } /* end of array */
};



static struct YM3526interface ym3812_interface =
{
	1,			/* 1 chip (no more supported) */
	3600000,	/* 3.600000 MHz ? (partially supported) */
	{ 255 }		/* (not supported) */
};

static struct MSM5205interface msm5205_interface =
{
	1,			/* 1 chips */
	4000,       /* 8000Hz playback? */
	toki_adpcm_int,	/* interrupt function */
	{ 255 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			16000000,	/* the real speed should be 8MHz or less (68000-8 CPU), */
						/* but with less than 14MHz there are slowdowns and the */
						/* title screen doesn't wave correctly */
			0,
			readmem,writemem,
			0,0,
			toki_interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 MHz ??? */
			2,
			sound_readmem,sound_writemem,
			0,0,
			ignore_interrupt,0	/* IRQs are caused by the main CPU?? */
								/* NMIs are caused by the ADPCM chip */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,
	/* video hardware */
	32*8, 32*8,
	{ 0*8, 32*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	4*256, 4*256,
	0,
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	toki_vh_start,
	toki_vh_stop,
	toki_vh_screenrefresh,

	/* sound hardware; only samples because we still haven't YM 3812 */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_MSM5205,
			&msm5205_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( toki_rom )
	ROM_REGION(0x60000)	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "toki.e3",      0x00000, 0x20000, 0xae9b3da4 )
	ROM_LOAD_ODD ( "toki.e5",      0x00000, 0x20000, 0x66a5a1d6 )
	ROM_LOAD_EVEN( "toki.e2",      0x40000, 0x10000, 0xd6a82808 )
	ROM_LOAD_ODD ( "toki.e4",      0x40000, 0x10000, 0xa01a5b10 )

	ROM_REGION_DISPOSE(0x220000)	/* 2*64k for foreground tiles */
	ROM_LOAD( "toki.e21",     0x000000, 0x08000, 0xbb8cacbd )
	ROM_LOAD( "toki.e13",     0x008000, 0x08000, 0x052ad275 )
	ROM_LOAD( "toki.e22",     0x010000, 0x08000, 0x04dcdc21 )
	ROM_LOAD( "toki.e7",      0x018000, 0x08000, 0x70729106 )
							/* 16*64k for sprites */
	ROM_LOAD( "toki.e26",     0x020000, 0x20000, 0xa8ba71fc )
	ROM_LOAD( "toki.e28",     0x040000, 0x20000, 0x29784948 )
	ROM_LOAD( "toki.e34",     0x060000, 0x20000, 0xe5f6e19b )
	ROM_LOAD( "toki.e36",     0x080000, 0x20000, 0x96e8db8b )
	ROM_LOAD( "toki.e30",     0x0a0000, 0x20000, 0x770d2b1b )
	ROM_LOAD( "toki.e32",     0x0c0000, 0x20000, 0xc289d246 )
	ROM_LOAD( "toki.e38",     0x0e0000, 0x20000, 0x87f4e7fb )
	ROM_LOAD( "toki.e40",     0x100000, 0x20000, 0x96e87350 )
							/* 8*64k for background #1 tiles */
	ROM_LOAD( "toki.e23",     0x120000, 0x10000, 0xfeb13d35 )
	ROM_LOAD( "toki.e24",     0x130000, 0x10000, 0x5b365637 )
	ROM_LOAD( "toki.e15",     0x140000, 0x10000, 0x617c32e6 )
	ROM_LOAD( "toki.e16",     0x150000, 0x10000, 0x2a11c0f0 )
	ROM_LOAD( "toki.e17",     0x160000, 0x10000, 0xfbc3d456 )
	ROM_LOAD( "toki.e18",     0x170000, 0x10000, 0x4c2a72e1 )
	ROM_LOAD( "toki.e8",      0x180000, 0x10000, 0x46a1b821 )
	ROM_LOAD( "toki.e9",      0x190000, 0x10000, 0x82ce27f6 )
							/* 8*64k for background #2 tiles */
	ROM_LOAD( "toki.e25",     0x1a0000, 0x10000, 0x63026cad )
	ROM_LOAD( "toki.e20",     0x1b0000, 0x10000, 0xa7f2ce26 )
	ROM_LOAD( "toki.e11",     0x1c0000, 0x10000, 0x48989aa0 )
	ROM_LOAD( "toki.e12",     0x1d0000, 0x10000, 0xc2ad9342 )
	ROM_LOAD( "toki.e19",     0x1e0000, 0x10000, 0x6cd22b18 )
	ROM_LOAD( "toki.e14",     0x1f0000, 0x10000, 0x859e313a )
	ROM_LOAD( "toki.e10",     0x200000, 0x10000, 0xe15c1d0f )
	ROM_LOAD( "toki.e6",      0x210000, 0x10000, 0x6f4b878a )

	ROM_REGION(0x18000)	/* 64k for code + 32k for banked data */
	ROM_LOAD( "toki.e1",      0x00000, 0x8000, 0x2832ef75 )
	ROM_CONTINUE(        0x10000, 0x8000 )	/* banked at 8000-bfff */
ROM_END



static int hiload(void)
{
	void *f;

	/* check if the hi score table has already been initialized */

	if (memcmp(&ram[0x6BB6],"D TA",4) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&ram[0x6B66],180);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}


static void hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&ram[0x6B66],180);
		osd_fclose(f);
	}
}


void toki_rom_decode (void)
{
	unsigned char *temp = malloc (65536 * 2);
	int i, offs;

	/* invert the sprite data in the ROMs */
	for (i = 0x020000; i < 0x120000; i++)
		Machine->memory_region[1][i] ^= 0xff;

	/* merge background tile graphics together */
	if (temp)
	{
		for (offs = 0x120000; offs < 0x220000; offs += 0x20000)
		{
			unsigned char *base = &Machine->memory_region[1][offs];
			memcpy (temp, base, 65536 * 2);
			for (i = 0; i < 16; i++)
			{
				memcpy (&base[0x00000 + i * 0x800], &temp[0x0000 + i * 0x2000], 0x800);
				memcpy (&base[0x10000 + i * 0x800], &temp[0x0800 + i * 0x2000], 0x800);
				memcpy (&base[0x08000 + i * 0x800], &temp[0x1000 + i * 0x2000], 0x800);
				memcpy (&base[0x18000 + i * 0x800], &temp[0x1800 + i * 0x2000], 0x800);
			}
		}

		free (temp);
	}
}



struct GameDriver toki_driver =
{
	__FILE__,
	0,
	"toki",
	"Toki (bootleg)",
	"1990",
	"Datsu",
	"Jarek Parchanski (MAME driver)\nRichard Bush (hardware info)", //Game authors:\n\nKitahara Haruki\nNishizawa Takashi\nSakuma Akira\nAoki Tsukasa\nKakiuchi Hiroyuki",
	0,
	&machine_driver,
	0,

	toki_rom,
	toki_rom_decode, 0,
	0,
	0,			/* sound_prom */
	input_ports,
	0, 0, 0, 	  	/* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};
