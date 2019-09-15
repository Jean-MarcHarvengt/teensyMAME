/***************************************************************************

	POW - Prisoners Of War, SNK 1988

 	Emulation by Bryan McPhail, mish@tendril.force9.net

Issues:
	Wrong ADPCM samples are sometimes played.
	Priority is sometimes wrong, eg, end of first level - player goes behind
	ladder instead of in front...
	Title screen is wrong (view bottom layer with TRANSPARENCY_NONE to see).
	Japanese text is wrong (I can't find bank select).
	A bit slow...

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

void pow_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

static unsigned char *pow_ram;

/******************************************************************************/

static int control_1_r(int offset)
{
	return (readinputport(0) + (readinputport(1) << 8));
}

static int control_2_r(int offset)
{
	return readinputport(2);
}

static int dip_1_r(int offset)
{
	return (readinputport(3) << 8);
}

static int dip_2_r(int offset)
{
	return (readinputport(4) << 8);
}

static void pow_sound_w(int offset, int data)
{
	int mybyte=(data>>8)&0xff;

	if (mybyte>0xbf) {
                ADPCM_trigger(0,mybyte);
	}
	else {
		soundlatch_w(0,mybyte);
		cpu_cause_interrupt(1,Z80_NMI_INT);
	}
}

static int high_ret(int offset) { return 0xffff; }

/* Idle cpu detection for POW */
static int cycle_r(int offset)
{
	int c=READ_WORD(&pow_ram[0x3e7c]);
	if ((c&0xff)!=0xf) cpu_spinuntil_int();
	return c;
}

/******************************************************************************

Save States - shall be used someday ;)

extern int MC68000_State_Load(void *f);
extern int MC68000_State_Save(void *f);

static void LoadState(void)
{
	void *f;
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0) {
		MC68000_State_Load(f);
		osd_fread(f,pow_ram,0x4000);
		osd_fread(f,pow_sprites,0x8000);
		osd_fread(f,videoram,0x1000);
		osd_fread(f,paletteram,0x1000);
		osd_fclose(f);
	}
}

static void SaveState(void)
{
	void *f;
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0) {
		MC68000_State_Save(f);
		osd_fwrite(f,pow_ram,0x4000);
		osd_fwrite(f,pow_sprites,0x8000);
		osd_fwrite(f,videoram,0x1000);
		osd_fwrite(f,paletteram,0x1000);
		osd_fclose(f);
	}
}

*******************************************************************************/

static struct MemoryReadAddress pow_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x043e7c, 0x043e7d, cycle_r },
	{ 0x040000, 0x043fff, MRA_BANK1 },
	{ 0x080000, 0x080001, control_1_r },
	{ 0x0c0000, 0x0c0001, control_2_r },
	{ 0x0e0000, 0x0e0001, high_ret }, /* Not sure */
	{ 0x0e8000, 0x0e8001, high_ret }, /* Not sure */
	{ 0x0f0000, 0x0f0001, dip_1_r },
	{ 0x0f0008, 0x0f0009, dip_2_r },
	{ 0x100000, 0x100fff, MRA_BANK2 },
	{ 0x200000, 0x207fff, MRA_BANK3 },
	{ 0x400000, 0x400fff, paletteram_word_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress pow_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x040000, 0x043fff, MWA_BANK1, &pow_ram },
	{ 0x080000, 0x080001, pow_sound_w },
	{ 0x0f0008, 0x0f0009, MWA_NOP },
	{ 0x100000, 0x100fff, MWA_BANK2, &videoram },
	{ 0x200000, 0x207fff, MWA_BANK3, &spriteram },
	{ 0x400000, 0x400fff, paletteram_xxxxRRRRGGGGBBBB_word_w, &paletteram },
	{ -1 }  /* end of table */
};

/******************************************************************************/

static struct MemoryReadAddress pow_sound_readmem[] =
{
	{ 0x0000, 0xefff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf800, 0xf800, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress pow_sound_writemem[] =
{
	{ 0x0000, 0xefff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct IOReadPort pow_sound_readport[] =
{
   { 0x00, 0x00, YM3812_status_port_0_r },
   { -1 }
};

static struct IOWritePort pow_sound_writeport[] =
{
   { 0x00, 0x00, YM3812_control_port_0_w },
   { 0x20, 0x20, YM3812_write_port_0_w },
   /* 0x40 & 0x80 - adpcm ports, an unknown chip */
   { -1 }
};

/******************************************************************************/

INPUT_PORTS_START( pow_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 ) /* Service button */
/* there's a service mode dip switch as well
	PORT_BITX(    0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
*/	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1, all active high */
	PORT_DIPNAME( 0x03, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPNAME( 0x0c, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x10, 0x10, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Occurence", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Second Bonus" )
	PORT_DIPSETTING(    0x20, "Every Bonus" )
	PORT_DIPNAME( 0x40, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x40, "Japanese" )
	PORT_DIPNAME( 0x80, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START	/* Dip switch bank 2, all active high */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "20000 50000" )
	PORT_DIPSETTING(    0x08, "40000 100000" )
	PORT_DIPSETTING(    0x04, "60000 150000" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, "Game Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Demo Sounds On" )
	PORT_DIPSETTING(    0x20, "Demo Sounds Off" )
	PORT_DIPSETTING(    0x30, "Freeze Game" )
	PORT_BITX( 0,       0x10, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0xc0, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0xc0, "Hardest" )
INPUT_PORTS_END

/* Identical to pow, but the Language dip switch has no effect */
INPUT_PORTS_START( powj_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2  )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 ) /* Service button */
/* there's a service mode dip switch as well
	PORT_BITX(    0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
*/	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dip switch bank 1, all active high */
	PORT_DIPNAME( 0x03, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPNAME( 0x0c, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x10, 0x10, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Occurence", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Second Bonus" )
	PORT_DIPSETTING(    0x20, "Every Bonus" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START	/* Dip switch bank 2, all active high */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "20000 50000" )
	PORT_DIPSETTING(    0x08, "40000 100000" )
	PORT_DIPSETTING(    0x04, "60000 150000" )
	PORT_DIPSETTING(    0x0c, "None" )
	PORT_DIPNAME( 0x30, 0x00, "Game Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Demo Sounds On" )
	PORT_DIPSETTING(    0x20, "Demo Sounds Off" )
	PORT_DIPSETTING(    0x30, "Freeze Game" )
	PORT_BITX( 0,       0x10, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0xc0, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0xc0, "Hardest" )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	2048,
	4,		/* 4 bits per pixel  */
	{ 0, 4, 0x8000*8, (0x8000*8)+4 },
	{ 8*8+3, 8*8+2, 8*8+1, 8*8+0, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	4096*4,
	4,		/* 4 bits per pixel */
	{ 0, 0x80000*8, 0x100000*8, 0x180000*8 },
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*32	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout,   0, 128 },
	{ 1, 0x010000, &spritelayout, 0, 128 },
	{ -1 } /* end of array */
};

/******************************************************************************/

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip (no more supported) */
	4000000,	/* 4 MHz ? (hand tuned) */
	{ 255 }		/* (not supported) */
};

static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 chip */
	8000,   /* 8000Hz playback */
	3,			/* memory region 3 */
	0,			/* init function */
	{ 255 }
};

/******************************************************************************/

static int pow_interrupt(void)
{

/* I use these to trigger save states:
	if ((readinputport(1)&0x1)!=0x1) SaveState();
	if ((readinputport(1)&0x2)!=0x2) LoadState();
*/
	return 1;
}

static struct MachineDriver pow_machine_driver =
{
	/* basic machine hardware */
	{
 		{
			CPU_M68000,
			10000000,
			0,
			pow_readmem,pow_writemem,0,0,
			pow_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,	/* ?? */
			2,
			pow_sound_readmem,pow_sound_writemem,
			pow_sound_readport,pow_sound_writeport,
			interrupt,3	/* ?? hand tuned */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
 	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	0,
	0,
	pow_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_ADPCM,
			&adpcm_interface
		}
	}
};

/******************************************************************************/

ROM_START( pow_rom )
	ROM_REGION(0x40000)
	ROM_LOAD_EVEN( "dg1", 0x00000, 0x20000, 0x8e71a8af )
	ROM_LOAD_ODD ( "dg2", 0x00000, 0x20000, 0x4287affc )

	ROM_REGION_DISPOSE(0x210000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "dg9",  0x000000, 0x08000, 0xdf864a08 )
	ROM_LOAD( "dg10", 0x008000, 0x08000, 0x9e470d53 )
	ROM_LOAD( "snk880.11a",0x010000, 0x20000, 0xe70fd906 )
	ROM_LOAD( "snk880.12a",0x030000, 0x20000, 0x628b1aed )
	ROM_LOAD( "snk880.13a",0x050000, 0x20000, 0x19dc8868 )
	ROM_LOAD( "snk880.14a",0x070000, 0x20000, 0x47cd498b )
	ROM_LOAD( "snk880.15a",0x090000, 0x20000, 0x7a90e957 )
	ROM_LOAD( "snk880.16a",0x0b0000, 0x20000, 0xe40a6c13 )
 	ROM_LOAD( "snk880.17a",0x0d0000, 0x20000, 0xc7931cc2 )
	ROM_LOAD( "snk880.18a",0x0f0000, 0x20000, 0xeed72232 )
	ROM_LOAD( "snk880.19a",0x110000, 0x20000, 0x1775b8dd )
	ROM_LOAD( "snk880.20a",0x130000, 0x20000, 0xf8e752ec )
	ROM_LOAD( "snk880.21a",0x150000, 0x20000, 0x27e9fffe )
	ROM_LOAD( "snk880.22a",0x170000, 0x20000, 0xaa9c00d8 )
	ROM_LOAD( "snk880.23a",0x190000, 0x20000, 0xadb6ad68 )
	ROM_LOAD( "snk880.24a",0x1b0000, 0x20000, 0xdd41865a )
	ROM_LOAD( "snk880.25a",0x1d0000, 0x20000, 0x055759ad )
	ROM_LOAD( "snk880.26a",0x1f0000, 0x20000, 0x9bc261c5 )

	ROM_REGION(0x10000)	/* Sound CPU */
	ROM_LOAD( "dg8",  0x000000, 0x10000, 0xd1d61da3 )

	ROM_REGION(0x10000)	/* ADPCM samples */
	ROM_LOAD( "dg7",  0x000000, 0x10000, 0xaba9a9d3 )
ROM_END

ROM_START( powj_rom )
	ROM_REGION(0x40000)
	ROM_LOAD_EVEN( "1-2", 0x00000, 0x20000, 0x2f17bfb0 )
	ROM_LOAD_ODD ( "2-2", 0x00000, 0x20000, 0xbaa32354 )

	ROM_REGION_DISPOSE(0x210000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "dg9",       0x000000, 0x08000, 0xdf864a08 )
	ROM_LOAD( "dg10",      0x008000, 0x08000, 0x9e470d53 )
	ROM_LOAD( "snk880.11a",0x010000, 0x20000, 0xe70fd906 )
	ROM_LOAD( "snk880.12a",0x030000, 0x20000, 0x628b1aed )
	ROM_LOAD( "snk880.13a",0x050000, 0x20000, 0x19dc8868 )
	ROM_LOAD( "snk880.14a",0x070000, 0x20000, 0x47cd498b )
	ROM_LOAD( "snk880.15a",0x090000, 0x20000, 0x7a90e957 )
	ROM_LOAD( "snk880.16a",0x0b0000, 0x20000, 0xe40a6c13 )
 	ROM_LOAD( "snk880.17a",0x0d0000, 0x20000, 0xc7931cc2 )
	ROM_LOAD( "snk880.18a",0x0f0000, 0x20000, 0xeed72232 )
	ROM_LOAD( "snk880.19a",0x110000, 0x20000, 0x1775b8dd )
	ROM_LOAD( "snk880.20a",0x130000, 0x20000, 0xf8e752ec )
	ROM_LOAD( "snk880.21a",0x150000, 0x20000, 0x27e9fffe )
	ROM_LOAD( "snk880.22a",0x170000, 0x20000, 0xaa9c00d8 )
	ROM_LOAD( "snk880.23a",0x190000, 0x20000, 0xadb6ad68 )
	ROM_LOAD( "snk880.24a",0x1b0000, 0x20000, 0xdd41865a )
	ROM_LOAD( "snk880.25a",0x1d0000, 0x20000, 0x055759ad )
	ROM_LOAD( "snk880.26a",0x1f0000, 0x20000, 0x9bc261c5 )

	ROM_REGION(0x10000)	/* Sound CPU */
	ROM_LOAD( "dg8",  0x000000, 0x10000, 0xd1d61da3 )

	ROM_REGION(0x10000)	/* ADPCM samples */
	ROM_LOAD( "dg7",  0x000000, 0x10000, 0xaba9a9d3 )
ROM_END

/******************************************************************************/

ADPCM_SAMPLES_START(pow_samples)
ADPCM_SAMPLE(0xc0,0x0033,0x0573*2)
ADPCM_SAMPLE(0xc1,0x05a6,0x0e4d*2)
ADPCM_SAMPLE(0xc2,0x13f3,0x14a4*2)
ADPCM_SAMPLE(0xc3,0x2897,0x01e1*2)
ADPCM_SAMPLE(0xc4,0x2a78,0x00ff*2)
ADPCM_SAMPLE(0xc5,0x2b77,0x0034*2)
ADPCM_SAMPLE(0xc6,0x2bab,0x0001*2)
ADPCM_SAMPLE(0xc7,0x2bac,0x033c*2)
ADPCM_SAMPLE(0xc8,0x2ee8,0x07e7*2)
ADPCM_SAMPLE(0xc9,0x36cf,0x0cd4*2)
ADPCM_SAMPLE(0xca,0x43a3,0x1455*2)
ADPCM_SAMPLE(0xcb,0x57f8,0x005d*2)
ADPCM_SAMPLE(0xcc,0x5855,0x1b42*2)
ADPCM_SAMPLE(0xcd,0x7397,0x0316*2)
ADPCM_SAMPLE(0xce,0x76ad,0x1079*2)
ADPCM_SAMPLE(0xcf,0x8726,0x0684*2)
ADPCM_SAMPLE(0xd0,0x8daa,0x0307*2)
ADPCM_SAMPLE(0xd1,0x90b1,0x0ac2*2)
ADPCM_SAMPLE(0xd2,0x9b73,0x0c27*2)
ADPCM_SAMPLE(0xd3,0xa79a,0x0209*2)
ADPCM_SAMPLE(0xd4,0xa9a3,0x07e5*2)
ADPCM_SAMPLE(0xd5,0xb188,0x0001*2)
ADPCM_SAMPLE(0xd6,0xb189,0x032a*2)
ADPCM_SAMPLE(0xd7,0xb4b3,0x1748*2)
ADPCM_SAMPLE(0xd8,0xcbfb,0x03d4*2)
ADPCM_SAMPLE(0xd9,0xcfcf,0x1836*2)
ADPCM_SAMPLES_END

struct GameDriver pow_driver =
{
	__FILE__,
	0,
	"pow",
	"P.O.W. (US)",
	"1988",
	"SNK",
	"Bryan McPhail",
	0,
	&pow_machine_driver,
	0,

	pow_rom,
	0, 0,
	0,
	(void *)pow_samples,	/* sound_prom */

	pow_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver powj_driver =
{
	__FILE__,
	&pow_driver,
	"powj",
	"P.O.W. (Japan)",
	"1988",
	"SNK",
	"Bryan McPhail",
	0,
	&pow_machine_driver,
	0,

	powj_rom,
	0, 0,
	0,
	(void *)pow_samples,	/* sound_prom */

	powj_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0, 0
};
