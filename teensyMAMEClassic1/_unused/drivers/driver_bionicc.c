/********************************************************************

			  Bionic Commando



ToDo:
- optimize the video driver (it currently doesn't use tmpbitmaps)

- get rid of input port hack

	Controls appear to be mapped at 0xFE4000, alongside dip switches, but there
	is something strange going on that I can't (yet) figure out.
	Player controls and coin inputs are supposed to magically appear at
	0xFFFFFB (coin/start)
	0xFFFFFD (player 2)
	0xFFFFFF (player 1)
	This is probably done by an MPU on the board (whose ROM is not
	available).

	The MPU also takes care of the commands for the sound CPU, which are stored
	at FFFFF9.

	IRQ4 seems to be control related.
	On each interrupt, it reads 0xFE4000 (coin/start), shift the bits around
	and move the resulting byte into a dword RAM location. The dword RAM location
	is rotated by 8 bits each time this happens.
	This is probably done to be pedantic about coin insertions (might be protection
	related). In fact, currently coin insertions are not consistently recognized.

********************************************************************/


#include "driver.h"

static unsigned char *ram_bc; /* used by high scores */
static unsigned char *ram_bcvid; /* used by high scores */

extern int bionicc_videoreg_r( int offset );
extern void bionicc_videoreg_w( int offset, int value );

extern unsigned char *bionicc_scroll2;
extern unsigned char *bionicc_scroll1;
extern unsigned char *bionicc_palette;
extern unsigned char *spriteram;
extern unsigned char *videoram;

extern int bionicc_vh_start(void);
extern void bionicc_vh_stop(void);
extern void bionicc_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static void bionicc_readinputs(void);
static void bionicc_sound_cmd(int data);



int bionicc_inputs_r(int offset)
{
if (errorlog) fprintf(errorlog,"%06x: inputs_r %04x\n",cpu_getpc(),offset);
	return (readinputport(offset)<<8) + readinputport(offset+1);
}

static unsigned char bionicc_inp[6];

void hacked_controls_w( int offset, int data )
{
if (errorlog) fprintf(errorlog,"%06x: hacked_controls_w %04x %02x\n",cpu_getpc(),offset,data);
	COMBINE_WORD_MEM( &bionicc_inp[offset], data);
}

static int hacked_controls_r(int offset)
{
if (errorlog) fprintf(errorlog,"%06x: hacked_controls_r %04x %04x\n",cpu_getpc(),offset,READ_WORD( &bionicc_inp[offset] ));
	return READ_WORD( &bionicc_inp[offset] );
}

static void bionicc_mpu_trigger_w(int offset,int data)
{
	data = readinputport(0) >> 4;
	WRITE_WORD(&bionicc_inp[0x00],data ^ 0x0f);

	data = readinputport(5); /* player 2 controls */
	WRITE_WORD(&bionicc_inp[0x02],data ^ 0xff);

	data = readinputport(4); /* player 1 controls */
	WRITE_WORD(&bionicc_inp[0x04],data ^ 0xff);
}



static unsigned char soundcommand[2];

void hacked_soundcommand_w( int offset, int data )
{
	COMBINE_WORD_MEM( &soundcommand[offset], data);
	soundlatch_w(0,data & 0xff);
}

static int hacked_soundcommand_r(int offset)
{
	return READ_WORD( &soundcommand[offset] );
}


/********************************************************************

  INTERRUPT

  The game runs on 2 interrupts.

  IRQ 2 drives the game
  IRQ 4 processes the input ports

  The game is very picky about timing. The following is the only
  way I have found it to work.

********************************************************************/

int bionicc_interrupt(void)
{
	if (cpu_getiloops() == 0) return 2;
	else return 4;
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },                /* 68000 ROM */
	{ 0xfe0000, 0xfe07ff, MRA_BANK1 },              /* RAM? */
	{ 0xfe0800, 0xfe0cff, MRA_BANK2 },              /* sprites */
	{ 0xfe0d00, 0xfe3fff, MRA_BANK3 },              /* RAM? */
	{ 0xfe4000, 0xfe4003, bionicc_inputs_r },       /* dipswitches */
	{ 0xfe8010, 0xfe8017, bionicc_videoreg_r },     /* scroll registers */
	{ 0xfec000, 0xfecfff, MRA_BANK4, &ram_bcvid },              /* fixed text layer */
	{ 0xff0000, 0xff3fff, MRA_BANK5 },              /* SCROLL1 layer */
	{ 0xff4000, 0xff7fff, MRA_BANK6 },              /* SCROLL2 layer */
	{ 0xff8000, 0xff86ff, MRA_BANK7 },              /* palette RAM */
	{ 0xffc000, 0xfffff7, MRA_BANK8, &ram_bc },               /* working RAM */
	{ 0xfffff8, 0xfffff9, hacked_soundcommand_r },      /* hack */
	{ 0xfffffa, 0xffffff, hacked_controls_r },      /* hack */
	{ -1 }
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0xfe0000, 0xfe07ff, MWA_BANK1 },	/* RAM? */
	{ 0xfe0800, 0xfe3fff, MWA_BANK2, &spriteram },
	{ 0xfe8010, 0xfe8017, bionicc_videoreg_w },
	{ 0xfe801a, 0xfe801b, bionicc_mpu_trigger_w },	/* ??? not sure, but looks like it */
	{ 0xfec000, 0xfecfff, MWA_BANK4, &videoram },
	{ 0xff0000, 0xff3fff, MWA_BANK5, &bionicc_scroll1 },
	{ 0xff4000, 0xff7fff, MWA_BANK6, &bionicc_scroll2 },
	{ 0xff8000, 0xff86ff, MWA_BANK7, &bionicc_palette },
	{ 0xffc000, 0xfffff7, MWA_BANK8 },	/* working RAM */
	{ 0xfffff8, 0xfffff9, hacked_soundcommand_w },      /* hack */
	{ 0xfffffa, 0xffffff, hacked_controls_w },	/* hack */
	{ -1 }
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8001, 0x8001, YM2151_status_port_0_r },
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8000, YM2151_register_port_0_w },
	{ 0x8001, 0x8001, YM2151_data_port_0_w },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( bionicc_input_ports )
	PORT_START
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x04, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x18, 0x18, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "20K, 40K, every 60K")
	PORT_DIPSETTING(    0x10, "30K, 50K, every 70K" )
	PORT_DIPSETTING(    0x08, "20K and 60K only")
	PORT_DIPSETTING(    0x00, "30K and 70K only" )
	PORT_DIPNAME( 0x60, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x60, "Medium")
	PORT_DIPSETTING(    0x20, "Hard")
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x18, "1 Coin/6 Credits" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off")
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



/********************************************************************

  GRAPHICS

********************************************************************/


static struct GfxLayout spritelayout_bionicc=
{
	16,16,  /* 16*16 sprites */
	2048,   /* 2048 sprites */
	4,      /* 4 bits per pixel */
	{ 0x30000*8,0x20000*8,0x10000*8,0 },
	{
		0,1,2,3,4,5,6,7,
		(16*8)+0,(16*8)+1,(16*8)+2,(16*8)+3,
		(16*8)+4,(16*8)+5,(16*8)+6,(16*8)+7
	},
	{
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
	},
	256   /* every sprite takes 256 consecutive bytes */
};

static struct GfxLayout vramlayout_bionicc=
{
	8,8,    /* 8*8 characters */
	2048,   /* 2048 character */
	2,      /* 2 bitplanes */
	{ 4,0 },
	{ 0,1,2,3,8,9,10,11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	128   /* every character takes 128 consecutive bytes */
};

static struct GfxLayout scroll2layout_bionicc=
{
	8,8,    /* 8*8 tiles */
	2048,   /* 2048 tiles */
	4,      /* 4 bits per pixel */
	{ (0x08000*8)+4,0x08000*8,4,0 },
	{ 0,1,2,3, 8,9,10,11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	128   /* every tile takes 128 consecutive bytes */
};

static struct GfxLayout scroll1layout_bionicc=
{
	16,16,  /* 16*16 tiles */
	2048,   /* 2048 tiles */
	4,      /* 4 bits per pixel */
	{ (0x020000*8)+4,0x020000*8,4,0 },
	{
		0,1,2,3, 8,9,10,11,
		(8*4*8)+0,(8*4*8)+1,(8*4*8)+2,(8*4*8)+3,
		(8*4*8)+8,(8*4*8)+9,(8*4*8)+10,(8*4*8)+11
	},
	{
		0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
		8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16
	},
	512   /* each tile takes 512 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo_bionicc[] =
{
	{ 1, 0x48000, &scroll2layout_bionicc, 0,    4 },
	{ 1, 0x58000, &scroll1layout_bionicc, 64,   4 },
	{ 1, 0x00000, &spritelayout_bionicc,  128, 16 },
	{ 1, 0x40000, &vramlayout_bionicc,    384, 16 },
	{ -1 }
};



static struct YM2151interface ym2151_interface =
{
	1,                      /* 1 chip */
	3579580,                /* 3.579580 MHz ? */
	{ 60 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			10000000, /* ?? MHz ? */
			0,
			readmem,writemem,0,0,
			bionicc_interrupt,8
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,  /* 4 Mhz ??? TODO: find real FRQ */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			nmi_interrupt,4	/* ??? */
		}
	},
	60, 5000, //DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo_bionicc,
	64*3+256, /* colours */
	64*3+256, /* Colour table length */
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	bionicc_vh_start,
	bionicc_vh_stop,
	bionicc_vh_screenrefresh,

	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		 { SOUND_YM2151,  &ym2151_interface },
	}
};



ROM_START( bionicc_rom )
	ROM_REGION(0x40000)      /* 68000 code */
	ROM_LOAD_EVEN( "tsu_02b.rom",  0x00000, 0x10000, 0xcf965a0a ) /* 68000 code */
	ROM_LOAD_ODD ( "tsu_04b.rom",  0x00000, 0x10000, 0xc9884bfb ) /* 68000 code */
	ROM_LOAD_EVEN( "tsu_03b.rom",  0x20000, 0x10000, 0x4e157ae2 ) /* 68000 code */
	ROM_LOAD_ODD ( "tsu_05b.rom",  0x20000, 0x10000, 0xe66ca0f9 ) /* 68000 code */

	ROM_REGION_DISPOSE(0x098000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tsu_10.rom",   0x000000, 0x08000, 0xf1180d02 )	/* Sprites */
	ROM_LOAD( "tsu_09.rom",   0x008000, 0x08000, 0x6a049292 )
	ROM_LOAD( "tsu_15.rom",   0x010000, 0x08000, 0xea912701 )
	ROM_LOAD( "tsu_14.rom",   0x018000, 0x08000, 0x46b2ad83 )
	ROM_LOAD( "tsu_20.rom",   0x020000, 0x08000, 0x17857ad2 )
	ROM_LOAD( "tsu_19.rom",   0x028000, 0x08000, 0xb5c82722 )
	ROM_LOAD( "tsu_22.rom",   0x030000, 0x08000, 0x5ee1ae6a )
	ROM_LOAD( "tsu_21.rom",   0x038000, 0x08000, 0x98777006 )
	ROM_LOAD( "tsu_08.rom",   0x040000, 0x08000, 0x9bf0b7a2 )	/* VIDEORAM (text layer) tiles */
	ROM_LOAD( "tsu_07.rom",   0x048000, 0x08000, 0x9469efa4 )	/* SCROLL2 Layer Tiles */
	ROM_LOAD( "tsu_06.rom",   0x050000, 0x08000, 0x40bf0eb4 )
	ROM_LOAD( "ts_12.rom",    0x058000, 0x08000, 0xe4b4619e )	/* SCROLL1 Layer Tiles */
	ROM_LOAD( "ts_11.rom",    0x060000, 0x08000, 0xab30237a )
	ROM_LOAD( "ts_17.rom",    0x068000, 0x08000, 0xdeb657e4 )
	ROM_LOAD( "ts_16.rom",    0x070000, 0x08000, 0xd363b5f9 )
	ROM_LOAD( "ts_13.rom",    0x078000, 0x08000, 0xa8f5a004 )
	ROM_LOAD( "ts_18.rom",    0x080000, 0x08000, 0x3b36948c )
	ROM_LOAD( "ts_23.rom",    0x088000, 0x08000, 0xbbfbe58a )
	ROM_LOAD( "ts_24.rom",    0x090000, 0x08000, 0xf156e564 )

	ROM_REGION(0x10000) /* 64k for the audio CPU */
	ROM_LOAD( "tsu_01b.rom",  0x00000, 0x8000, 0xa9a6cafa )
ROM_END

ROM_START( bionicc2_rom )
	ROM_REGION(0x40000)      /* 68000 code */
	ROM_LOAD_EVEN( "02",      0x00000, 0x10000, 0xf2528f08 ) /* 68000 code */
	ROM_LOAD_ODD ( "04",      0x00000, 0x10000, 0x38b1c7e4 ) /* 68000 code */
	ROM_LOAD_EVEN( "03",      0x20000, 0x10000, 0x72c3b76f ) /* 68000 code */
	ROM_LOAD_ODD ( "05",      0x20000, 0x10000, 0x70621f83 ) /* 68000 code */

	ROM_REGION_DISPOSE(0x098000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tsu_10.rom",   0x000000, 0x08000, 0xf1180d02 )	/* Sprites */
	ROM_LOAD( "tsu_09.rom",   0x008000, 0x08000, 0x6a049292 )
	ROM_LOAD( "tsu_15.rom",   0x010000, 0x08000, 0xea912701 )
	ROM_LOAD( "tsu_14.rom",   0x018000, 0x08000, 0x46b2ad83 )
	ROM_LOAD( "tsu_20.rom",   0x020000, 0x08000, 0x17857ad2 )
	ROM_LOAD( "tsu_19.rom",   0x028000, 0x08000, 0xb5c82722 )
	ROM_LOAD( "tsu_22.rom",   0x030000, 0x08000, 0x5ee1ae6a )
	ROM_LOAD( "tsu_21.rom",   0x038000, 0x08000, 0x98777006 )
	ROM_LOAD( "tsu_08.rom",   0x040000, 0x08000, 0x9bf0b7a2 )	/* VIDEORAM (text layer) tiles */
	ROM_LOAD( "tsu_07.rom",   0x048000, 0x08000, 0x9469efa4 )	/* SCROLL2 Layer Tiles */
	ROM_LOAD( "tsu_06.rom",   0x050000, 0x08000, 0x40bf0eb4 )
	ROM_LOAD( "ts_12.rom",    0x058000, 0x08000, 0xe4b4619e )	/* SCROLL1 Layer Tiles */
	ROM_LOAD( "ts_11.rom",    0x060000, 0x08000, 0xab30237a )
	ROM_LOAD( "ts_17.rom",    0x068000, 0x08000, 0xdeb657e4 )
	ROM_LOAD( "ts_16.rom",    0x070000, 0x08000, 0xd363b5f9 )
	ROM_LOAD( "ts_13.rom",    0x078000, 0x08000, 0xa8f5a004 )
	ROM_LOAD( "ts_18.rom",    0x080000, 0x08000, 0x3b36948c )
	ROM_LOAD( "ts_23.rom",    0x088000, 0x08000, 0xbbfbe58a )
	ROM_LOAD( "ts_24.rom",    0x090000, 0x08000, 0xf156e564 )

	ROM_REGION(0x10000) /* 64k for the audio CPU */
	ROM_LOAD( "tsu_01b.rom",  0x00000, 0x8000, 0xa9a6cafa )
ROM_END

/* hi load / save added 11/20/98 HSC */

#ifdef LSB_FIRST
#define ENDIAN_ALIGN(a)	(a)
#else
#define ENDIAN_ALIGN(a) (a^1)
#endif

static int hiload(void)
{

    void *f;
    /* check if the hi score table has already been initialized */
    if (READ_WORD(&ram_bc[0x39e2])==0x2 && READ_WORD(&ram_bc[0x39e4])==0 && READ_WORD(&ram_bc[0x3a2e])==0x434f && READ_WORD(&ram_bc[0x3a30])==0x4d20)
    {
        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
        {
			int hi;
			osd_fread_msbfirst(f,&ram_bc[0x39e2],10*8);
			osd_fclose(f);
			ram_bc[0x57a]=ram_bc[0x39e2];
            ram_bc[0x57b]=ram_bc[0x39e3];
            ram_bc[0x57c]=ram_bc[0x39e4];
            ram_bc[0x57d]=ram_bc[0x39e5];

			hi =(ram_bc[ENDIAN_ALIGN(0x57c)] & 0x0f) +
				(ram_bc[ENDIAN_ALIGN(0x57c)] >> 4) * 10 +
				(ram_bc[ENDIAN_ALIGN(0x57d)] & 0x0f) * 100 +
				(ram_bc[ENDIAN_ALIGN(0x57d)] >> 4) * 1000 +
				(ram_bc[ENDIAN_ALIGN(0x57a)] & 0x0f) * 10000 +
				(ram_bc[ENDIAN_ALIGN(0x57a)] >> 4) * 100000 +
				(ram_bc[ENDIAN_ALIGN(0x57b)] & 0x0f) * 1000000 +
				(ram_bc[ENDIAN_ALIGN(0x57b)] >> 4) * 10000000;

			if (hi >= 10000000)
				ram_bcvid[ENDIAN_ALIGN(0x0d8)] = ram_bc[ENDIAN_ALIGN(0x57b)] >> 4;
			if (hi >= 1000000)
				ram_bcvid[ENDIAN_ALIGN(0x0da)] = ram_bc[ENDIAN_ALIGN(0x57b)] & 0x0F;
			if (hi >= 100000)
				ram_bcvid[ENDIAN_ALIGN(0x0dc)] = ram_bc[ENDIAN_ALIGN(0x57a)] >> 4;
			if (hi >= 10000)
				ram_bcvid[ENDIAN_ALIGN(0x0de)] = ram_bc[ENDIAN_ALIGN(0x57a)] & 0x0F;
			if (hi >= 1000)
				ram_bcvid[ENDIAN_ALIGN(0x0e0)] = ram_bc[ENDIAN_ALIGN(0x57d)] >> 4;
			if (hi >= 100)
				ram_bcvid[ENDIAN_ALIGN(0x0e2)] = ram_bc[ENDIAN_ALIGN(0x57d)] & 0x0F;
			if (hi >= 10)
				ram_bcvid[ENDIAN_ALIGN(0x0e4)] = ram_bc[ENDIAN_ALIGN(0x57c)] >> 4;
			if (hi >= 0)
				ram_bcvid[ENDIAN_ALIGN(0x0e6)] = ram_bc[ENDIAN_ALIGN(0x57c)] & 0x0F;

        	if (errorlog)
				fprintf(errorlog,"hi score: %i\n", hi);
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
		osd_fwrite_msbfirst(f,&ram_bc[0x39e2],10*8);
		osd_fclose(f);
	}
}




struct GameDriver bionicc_driver =
{
	__FILE__,
	0,
	"bionicc",
	"Bionic Commando (set 1)",
	"1987",
	"Capcom",
	"Steven Frew\nPhil Stroffolino\nPaul Leaman",
	0,
	&machine_driver,
	0,

	bionicc_rom,
	0,
	0,0,
	0,

	bionicc_input_ports,
	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	hiload,hisave
};

struct GameDriver bionicc2_driver =
{
	__FILE__,
	&bionicc_driver,
	"bionicc2",
	"Bionic Commando (set 2)",
	"1987",
	"Capcom",
	"Steven Frew\nPhil Stroffolino\nPaul Leaman",
	0,
	&machine_driver,
	0,

	bionicc2_rom,
	0,
	0,0,
	0,

	bionicc_input_ports,
	NULL, 0, 0,

	ORIENTATION_DEFAULT,
	hiload, hisave
};
