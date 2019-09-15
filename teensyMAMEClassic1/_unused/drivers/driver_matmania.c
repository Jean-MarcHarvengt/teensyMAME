/***************************************************************************

Mat Mania
Memetron, 1985
(copyright Taito, licensed by Technos, distributed by Memetron)

MAIN BOARD:

0000-0fff RAM
1000-13ff Video RAM
1400-17ff Attribute RAM
1800-1fff ?? Only used in self-test ??
2000-21ff Background video RAM #1
2200-23ff Background attribute RAM #1
2400-25ff Background video RAM #2
2600-27ff Background attribute RAM #2
4000-ffff ROM

Changes:
25 Jan 98 LBO
	* Split off from mystston.c
	* Added Mania Challenge driver

TODO:
	* Mania Challenge needs the audio CPU working. However, it's not clear how
	  it's set up. There is some extra 6502 code, but it appears to be for the main CPU.
	  I also understood that it was somehow using a 6809 for the audio CPU but I
	  can find no program ROMs that are 6809 code.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6502/m6502.h"

extern unsigned char *matmania_videoram2,*matmania_colorram2;
extern int matmania_videoram2_size;
extern unsigned char *matmania_videoram3,*matmania_colorram3;
extern int matmania_videoram3_size;
extern unsigned char *matmania_scroll;
extern unsigned char *matmania_pageselect;

void matmania_paletteram_w(int offset,int data);
void matmania_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void matmania_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void maniach_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void matmania_videoram3_w(int offset,int data);
void matmania_colorram3_w(int offset,int data);
int matmania_vh_start(void);
void matmania_vh_stop(void);
void matmania_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



void matmania_sh_command_w (int offset, int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(1,M6502_INT_IRQ);
}

void matmania_dac_w(int offset,int data)
{
	DAC_data_w(0,data);
}


static int maniach_3040;

void maniach_3040_w (int offset, int data)
{
	maniach_3040 = data + 4;
}

int maniach_3040_r (int offset)
{
	if (errorlog) fprintf (errorlog, "3040_r @ pc:%04x\n", cpu_getpc());
/*	return 0x4a; */
	return maniach_3040;
}

int maniach_3041_r (int offset)
{
	if (errorlog) fprintf (errorlog, "3041_r @ pc:%04x\n", cpu_getpc());
	return 0x02;
}



static struct MemoryReadAddress matmania_readmem[] =
{
	{ 0x0000, 0x077f, MRA_RAM },
	{ 0x1000, 0x17ff, MRA_RAM },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x3000, 0x3000, input_port_0_r },
	{ 0x3010, 0x3010, input_port_1_r },
	{ 0x3020, 0x3020, input_port_2_r },
	{ 0x3030, 0x3030, input_port_3_r },
	{ 0x3040, 0x3040, maniach_3040_r },
	{ 0x3041, 0x3041, maniach_3041_r },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress matmania_writemem[] =
{
	{ 0x0000, 0x077f, MWA_RAM },
	{ 0x0780, 0x07df, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x1000, 0x13ff, MWA_RAM, &matmania_videoram2, &matmania_videoram2_size },
	{ 0x1400, 0x17ff, MWA_RAM, &matmania_colorram2 },
	{ 0x2000, 0x21ff, videoram_w, &videoram, &videoram_size },
	{ 0x2200, 0x23ff, colorram_w, &colorram },
	{ 0x2400, 0x25ff, matmania_videoram3_w, &matmania_videoram3, &matmania_videoram3_size },
	{ 0x2600, 0x27ff, matmania_colorram3_w, &matmania_colorram3 },
	{ 0x3000, 0x3000, MWA_RAM, &matmania_pageselect },
	{ 0x3010, 0x3010, matmania_sh_command_w },
	{ 0x3020, 0x3020, MWA_RAM, &matmania_scroll },
	{ 0x3030, 0x3030, MWA_NOP },	/* ?? */
	{ 0x3050, 0x307f, matmania_paletteram_w, &paletteram },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress maniach_writemem[] =
{
	{ 0x0000, 0x077f, MWA_RAM },
	{ 0x0780, 0x07df, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x1000, 0x13ff, MWA_RAM, &matmania_videoram2, &matmania_videoram2_size },
	{ 0x1400, 0x17ff, MWA_RAM, &matmania_colorram2 },
	{ 0x2000, 0x21ff, videoram_w, &videoram, &videoram_size },
	{ 0x2200, 0x23ff, colorram_w, &colorram },
	{ 0x2400, 0x25ff, matmania_videoram3_w, &matmania_videoram3, &matmania_videoram3_size },
	{ 0x2600, 0x27ff, matmania_colorram3_w, &matmania_colorram3 },
	{ 0x3000, 0x3000, MWA_RAM, &matmania_pageselect },
	{ 0x3010, 0x3010, matmania_sh_command_w },
	{ 0x3020, 0x3020, MWA_RAM, &matmania_scroll },
	{ 0x3030, 0x3030, MWA_NOP },	/* ?? */
	{ 0x3040, 0x3040, maniach_3040_w },	/* ??? */
	{ 0x3050, 0x307f, matmania_paletteram_w, &paletteram },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x01ff, MRA_RAM },
	{ 0x2007, 0x2007, soundlatch_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x01ff, MWA_RAM },
	{ 0x2000, 0x2000, AY8910_write_port_0_w },
	{ 0x2001, 0x2001, AY8910_control_port_0_w },
	{ 0x2002, 0x2002, AY8910_write_port_1_w },
	{ 0x2003, 0x2003, AY8910_control_port_1_w },
	{ 0x2004, 0x2004, matmania_dac_w },
	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress maniach_sound_readmem[] =
{
	{ 0x0000, 0x01ff, MRA_RAM },
//	{ 0x2007, 0x2007, soundlatch_r },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress maniach_sound_writemem[] =
{
	{ 0x0000, 0x01ff, MWA_RAM },
//	{ 0x2000, 0x2000, AY8910_write_port_0_w },
//	{ 0x2001, 0x2001, AY8910_control_port_0_w },
//	{ 0x2002, 0x2002, AY8910_write_port_1_w },
//	{ 0x2003, 0x2003, AY8910_control_port_1_w },
//	{ 0x2004, 0x2004, AY8910_write_port_2_w },
//	{ 0x2005, 0x2005, AY8910_control_port_2_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( matmania_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* DSW1 */
	PORT_DIPNAME (0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME (0x04, 0x04, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME (0x08, 0x08, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME (0x10, 0x10, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME (0x20, 0x20, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME (0x40, 0x40, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME (0x80, 0x80, "Unknown 6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME (0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/3 Credits" )
	PORT_DIPNAME (0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credits" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/3 Credits" )
	PORT_DIPNAME (0x10, 0x10, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME (0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout tilelayout =
{
	16,16,  /* 16*16 tiles */
	512,    /* 512 tiles */
	3,	/* 3 bits per pixel */
	{ 2*512*16*16, 512*16*16, 0 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8	/* every tile takes 16 consecutive bytes */
};

static struct GfxLayout matmania_spritelayout =
{
	16,16,  /* 16*16 sprites */
	3584,    /* 3584 sprites */
	3,	/* 3 bits per pixel */
	{ 2*3584*16*16, 3584*16*16, 0 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxLayout maniach_spritelayout =
{
	16,16,  /* 16*16 sprites */
	3584,    /* 3584 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 3584*16*16, 2*3584*16*16 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8	/* every sprite takes 16 consecutive bytes */
};

static struct GfxLayout maniach_tilelayout =
{
	16,16,  /* 16*16 tiles */
	1024,    /* 1024 tiles */
	3,	/* 3 bits per pixel */
	{ 2*1024*16*16, 1024*16*16, 0 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8	/* every tile takes 16 consecutive bytes */
};

static struct GfxDecodeInfo matmania_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,              0, 4 },
	{ 1, 0x06000, &tilelayout,            4*8, 4 },
	{ 1, 0x12000, &matmania_spritelayout, 8*8, 2 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo maniach_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,             0, 4 },
	{ 1, 0x06000, &maniach_tilelayout,   4*8, 4 },
	{ 1, 0x1e000, &maniach_spritelayout, 8*8, 2 },
	{ -1 } /* end of array */
};


static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz?????? */
	{ 50, 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};



static struct MachineDriver matmania_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz ???? */
			0,
			matmania_readmem,matmania_writemem,0,0,
			interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1200000,	/* 1.2 Mhz ???? */
			3,
			sound_readmem,sound_writemem,0,0,
			nmi_interrupt,15	/* ???? */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,	/* enough time for the audio CPU to get all commands */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	matmania_gfxdecodeinfo,
	64+16, 64+16,
	matmania_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	matmania_vh_start,
	matmania_vh_stop,
	matmania_vh_screenrefresh,

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

static struct MachineDriver maniach_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1500000,	/* 1.5 Mhz ???? */
			0,
			matmania_readmem,maniach_writemem,0,0,
			interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1200000,	/* 1.2 Mhz ???? */
			3,
			maniach_sound_readmem,maniach_sound_writemem,0,0,
			nmi_interrupt,15	/* ???? */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	maniach_gfxdecodeinfo,
	64+16, 64+16,
	matmania_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	matmania_vh_start,
	matmania_vh_stop,
	maniach_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0
};

/***************************************************************************

  Mat Mania driver

***************************************************************************/

ROM_START( matmania_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "k0-03",        0x4000, 0x4000, 0x314ab8a4 )
	ROM_LOAD( "k1-03",        0x8000, 0x4000, 0x3b3c3f08 )
	ROM_LOAD( "k2-03",        0xc000, 0x4000, 0x286c0917 )

	ROM_REGION_DISPOSE(0x66000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ku-02",        0x00000, 0x2000, 0x613c8698 )	/* Character ROMs - 1024 chars, 3 bpp */
	ROM_LOAD( "kv-02",        0x02000, 0x2000, 0x274ce14b )
	ROM_LOAD( "kw-02",        0x04000, 0x2000, 0x7588a9c4 )
	ROM_LOAD( "kt-02",        0x06000, 0x4000, 0x5d817c70 )	/* tile set */
	ROM_LOAD( "ks-02",        0x0a000, 0x4000, 0x2e9f3ba0 )
	ROM_LOAD( "kr-02",        0x0e000, 0x4000, 0xb057d3e3 )
	ROM_LOAD( "k6-00",        0x12000, 0x4000, 0x294d0878 )	/* sprites */
	ROM_LOAD( "k7-00",        0x16000, 0x4000, 0x0908c2f5 )
	ROM_LOAD( "k8-00",        0x1a000, 0x4000, 0xae8341e1 )
	ROM_LOAD( "k9-00",        0x1e000, 0x4000, 0x752ac2c6 )
	ROM_LOAD( "ka-00",        0x22000, 0x4000, 0x46a9cb16 )
	ROM_LOAD( "kb-00",        0x26000, 0x4000, 0xbf016772 )
	ROM_LOAD( "kc-00",        0x2a000, 0x4000, 0x8d08bce7 )
	ROM_LOAD( "kd-00",        0x2e000, 0x4000, 0xaf1d6a60 )
	ROM_LOAD( "ke-00",        0x32000, 0x4000, 0x614f19b0 )
	ROM_LOAD( "kf-00",        0x36000, 0x4000, 0xbdf58c18 )
	ROM_LOAD( "kg-00",        0x3a000, 0x4000, 0x2189f5cf )
	ROM_LOAD( "kh-00",        0x3e000, 0x4000, 0x6b11ed1f )
	ROM_LOAD( "ki-00",        0x42000, 0x4000, 0xd7ac4ec5 )
	ROM_LOAD( "kj-00",        0x46000, 0x4000, 0x2caee05d )
	ROM_LOAD( "kk-00",        0x4a000, 0x4000, 0xeb54f010 )
	ROM_LOAD( "kl-00",        0x4e000, 0x4000, 0xfa4c7e0c )
	ROM_LOAD( "km-00",        0x52000, 0x4000, 0x6d2369b6 )
	ROM_LOAD( "kn-00",        0x56000, 0x4000, 0xc55733e2 )
	ROM_LOAD( "ko-00",        0x5a000, 0x4000, 0xed3c3476 )
	ROM_LOAD( "kp-00",        0x5e000, 0x4000, 0x9c84a969 )
	ROM_LOAD( "kq-00",        0x62000, 0x4000, 0xfa2f0003 )

	ROM_REGION(0x0080)	/* color proms */
	ROM_LOAD( "matmania.1",   0x0000, 0x0020, 0x1b58f01f ) /* char palette red and green components */
	ROM_LOAD( "matmania.5",   0x0020, 0x0020, 0x2029f85f ) /* tile palette red and green components */
	ROM_LOAD( "matmania.2",   0x0040, 0x0020, 0xb6ac1fd5 ) /* char palette blue component */
	ROM_LOAD( "matmania.16",  0x0060, 0x0020, 0x09325dc2 ) /* tile palette blue component */

	ROM_REGION(0x10000)	/* 64k for audio code */
	ROM_LOAD( "k4-0",         0x8000, 0x4000, 0x86dab489 )
	ROM_LOAD( "k5-0",         0xc000, 0x4000, 0x4c41cdba )
ROM_END


ROM_START( excthour_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "e29",          0x04000, 0x4000, 0xc453e855 )
	ROM_LOAD( "e28",          0x08000, 0x4000, 0x17b63708 )
	ROM_LOAD( "e27",          0x0c000, 0x4000, 0x269ab3bc )

	ROM_REGION_DISPOSE(0x66000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "e30",          0x00000, 0x2000, 0xb2875329 )	/* Character ROMs - 1024 chars, 3 bpp */
	ROM_LOAD( "e31",          0x02000, 0x2000, 0xc9506de8 )
	ROM_LOAD( "e32",          0x04000, 0x2000, 0x00d1635f )
	ROM_LOAD( "e5",           0x06000, 0x4000, 0x0604dc55 )	/* tile set */
	ROM_LOAD( "ks-02",        0x0a000, 0x4000, 0x2e9f3ba0 )
	ROM_LOAD( "e3",           0x0e000, 0x4000, 0xebd273c6 )
	ROM_LOAD( "k6-00",        0x12000, 0x4000, 0x294d0878 )	/* sprites */
	ROM_LOAD( "k7-00",        0x16000, 0x4000, 0x0908c2f5 )
	ROM_LOAD( "k8-00",        0x1a000, 0x4000, 0xae8341e1 )
	ROM_LOAD( "k9-00",        0x1e000, 0x4000, 0x752ac2c6 )
	ROM_LOAD( "ka-00",        0x22000, 0x4000, 0x46a9cb16 )
	ROM_LOAD( "kb-00",        0x26000, 0x4000, 0xbf016772 )
	ROM_LOAD( "kc-00",        0x2a000, 0x4000, 0x8d08bce7 )
	ROM_LOAD( "kd-00",        0x2e000, 0x4000, 0xaf1d6a60 )
	ROM_LOAD( "ke-00",        0x32000, 0x4000, 0x614f19b0 )
	ROM_LOAD( "kf-00",        0x36000, 0x4000, 0xbdf58c18 )
	ROM_LOAD( "kg-00",        0x3a000, 0x4000, 0x2189f5cf )
	ROM_LOAD( "kh-00",        0x3e000, 0x4000, 0x6b11ed1f )
	ROM_LOAD( "ki-00",        0x42000, 0x4000, 0xd7ac4ec5 )
	ROM_LOAD( "kj-00",        0x46000, 0x4000, 0x2caee05d )
	ROM_LOAD( "kk-00",        0x4a000, 0x4000, 0xeb54f010 )
	ROM_LOAD( "kl-00",        0x4e000, 0x4000, 0xfa4c7e0c )
	ROM_LOAD( "km-00",        0x52000, 0x4000, 0x6d2369b6 )
	ROM_LOAD( "kn-00",        0x56000, 0x4000, 0xc55733e2 )
	ROM_LOAD( "ko-00",        0x5a000, 0x4000, 0xed3c3476 )
	ROM_LOAD( "kp-00",        0x5e000, 0x4000, 0x9c84a969 )
	ROM_LOAD( "kq-00",        0x62000, 0x4000, 0xfa2f0003 )

	ROM_REGION(0x0080)	/* color proms */
	ROM_LOAD( "matmania.1",   0x0000, 0x0020, 0x1b58f01f ) /* char palette red and green components */
	ROM_LOAD( "matmania.5",   0x0020, 0x0020, 0x2029f85f ) /* tile palette red and green components */
	ROM_LOAD( "matmania.2",   0x0040, 0x0020, 0xb6ac1fd5 ) /* char palette blue component */
	ROM_LOAD( "matmania.16",  0x0060, 0x0020, 0x09325dc2 ) /* tile palette blue component */

	ROM_REGION(0x10000)	/* 64k for audio code */
	ROM_LOAD( "k4-0",         0x8000, 0x4000, 0x86dab489 )
	ROM_LOAD( "k5-0",         0xc000, 0x4000, 0x4c41cdba )
ROM_END

ROM_START( maniach_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "mc-mb2.bin",   0x04000, 0x4000, 0xa6da1ba8 )
	ROM_LOAD( "mc-ma2.bin",   0x08000, 0x4000, 0x84583323 )
	ROM_LOAD( "mc-m92.bin",   0x0c000, 0x4000, 0xe209a500 )

	ROM_REGION_DISPOSE(0x72000)	/* temporary space for graphics (disposed after conversion) */
	/* Character ROMs - 1024 chars, 3 bpp */
	ROM_LOAD( "mc-m60.bin",   0x00000, 0x2000, 0x1cdbb117 )
	ROM_LOAD( "mc-m70.bin",   0x02000, 0x2000, 0x553f0780 )
	ROM_LOAD( "mc-m80.bin",   0x04000, 0x2000, 0x9392ecb7 )
	/* tile set */
	ROM_LOAD( "mc-m01.bin",   0x06000, 0x8000, 0xda558e4d )
	ROM_LOAD( "mc-m10.bin",   0x0e000, 0x8000, 0x619a02f8 )
	ROM_LOAD( "mc-m20.bin",   0x16000, 0x8000, 0xa617c6c1 )
	/* sprites */
	ROM_LOAD( "mc-mc0.bin",   0x1e000, 0x4000, 0x133d644f )
	ROM_LOAD( "mc-md0.bin",   0x22000, 0x4000, 0xe387b036 )
	ROM_LOAD( "mc-me0.bin",   0x26000, 0x4000, 0xb36b1283 )
	ROM_LOAD( "mc-mf0.bin",   0x2a000, 0x4000, 0x2584d8a9 )
	ROM_LOAD( "mc-mg0.bin",   0x2e000, 0x4000, 0xcf31a714 )
	ROM_LOAD( "mc-mh0.bin",   0x32000, 0x4000, 0x6292d589 )
	ROM_LOAD( "mc-mi0.bin",   0x36000, 0x4000, 0xee2e06e3 )
	ROM_LOAD( "mc-mj0.bin",   0x3a000, 0x4000, 0x7e73895b )
	ROM_LOAD( "mc-mk0.bin",   0x3e000, 0x4000, 0x66c8bf75 )
	ROM_LOAD( "mc-ml0.bin",   0x42000, 0x4000, 0x88138a1d )
	ROM_LOAD( "mc-mm0.bin",   0x46000, 0x4000, 0xa1a4260d )
	ROM_LOAD( "mc-mn0.bin",   0x4a000, 0x4000, 0x6bc61b58 )
	ROM_LOAD( "mc-mo0.bin",   0x4e000, 0x4000, 0xf96ef600 )
	ROM_LOAD( "mc-mp0.bin",   0x52000, 0x4000, 0x1259618e )
	ROM_LOAD( "mc-mq0.bin",   0x56000, 0x4000, 0x102a1666 )
	ROM_LOAD( "mc-mr0.bin",   0x5a000, 0x4000, 0x1e854453 )
	ROM_LOAD( "mc-ms0.bin",   0x5e000, 0x4000, 0x7bc9d878 )
	ROM_LOAD( "mc-mt0.bin",   0x62000, 0x4000, 0x09cea985 )
	ROM_LOAD( "mc-mu0.bin",   0x66000, 0x4000, 0x5421769e )
	ROM_LOAD( "mc-mv0.bin",   0x6a000, 0x4000, 0x36fc3e2d )
	ROM_LOAD( "mc-mw0.bin",   0x6e000, 0x4000, 0x135dce4c )

	ROM_REGION(0x0080)	/* color proms */
	ROM_LOAD( "prom.2",       0x0000, 0x0020, 0x32db2cf4 ) /* char palette red and green components */
	ROM_LOAD( "prom.16",      0x0020, 0x0020, 0x18836d26 ) /* tile palette red and green components */
	ROM_LOAD( "prom.3",       0x0040, 0x0020, 0xc7925311 ) /* char palette blue component */
	ROM_LOAD( "prom.17",      0x0060, 0x0020, 0x41f51d49 ) /* tile palette blue component */

	ROM_REGION(0x10000)	/* 64k for audio code */
	ROM_LOAD( "mc-m50.bin",   0x4000, 0x4000, 0xba415d68 )
	ROM_LOAD( "mc-m40.bin",   0x8000, 0x4000, 0x2a217ed0 )
	ROM_LOAD( "mc-m30.bin",   0xc000, 0x4000, 0x95af1723 ) /* definitely 6502 code, for 1st CPU? */
ROM_END

ROM_START( maniach2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic40-mb1",     0x04000, 0x4000, 0xb337a867 )
	ROM_LOAD( "ic41-ma1",     0x08000, 0x4000, 0x85ec8279 )
	ROM_LOAD( "ic42-m91",     0x0c000, 0x4000, 0xa14b86dd )

	ROM_REGION_DISPOSE(0x72000)	/* temporary space for graphics (disposed after conversion) */
	/* Character ROMs - 1024 chars, 3 bpp */
	ROM_LOAD( "mc-m60.bin",   0x00000, 0x2000, 0x1cdbb117 )
	ROM_LOAD( "mc-m70.bin",   0x02000, 0x2000, 0x553f0780 )
	ROM_LOAD( "mc-m80.bin",   0x04000, 0x2000, 0x9392ecb7 )
	/* tile set */
	ROM_LOAD( "mc-m01.bin",   0x06000, 0x8000, 0xda558e4d )
	ROM_LOAD( "mc-m10.bin",   0x0e000, 0x8000, 0x619a02f8 )
	ROM_LOAD( "mc-m20.bin",   0x16000, 0x8000, 0xa617c6c1 )
	/* sprites */
	ROM_LOAD( "mc-mc0.bin",   0x1e000, 0x4000, 0x133d644f )
	ROM_LOAD( "mc-md0.bin",   0x22000, 0x4000, 0xe387b036 )
	ROM_LOAD( "mc-me0.bin",   0x26000, 0x4000, 0xb36b1283 )
	ROM_LOAD( "mc-mf0.bin",   0x2a000, 0x4000, 0x2584d8a9 )
	ROM_LOAD( "mc-mg0.bin",   0x2e000, 0x4000, 0xcf31a714 )
	ROM_LOAD( "mc-mh0.bin",   0x32000, 0x4000, 0x6292d589 )
	ROM_LOAD( "mc-mi0.bin",   0x36000, 0x4000, 0xee2e06e3 )
	ROM_LOAD( "mc-mj0.bin",   0x3a000, 0x4000, 0x7e73895b )
	ROM_LOAD( "mc-mk0.bin",   0x3e000, 0x4000, 0x66c8bf75 )
	ROM_LOAD( "mc-ml0.bin",   0x42000, 0x4000, 0x88138a1d )
	ROM_LOAD( "mc-mm0.bin",   0x46000, 0x4000, 0xa1a4260d )
	ROM_LOAD( "mc-mn0.bin",   0x4a000, 0x4000, 0x6bc61b58 )
	ROM_LOAD( "mc-mo0.bin",   0x4e000, 0x4000, 0xf96ef600 )
	ROM_LOAD( "mc-mp0.bin",   0x52000, 0x4000, 0x1259618e )
	ROM_LOAD( "mc-mq0.bin",   0x56000, 0x4000, 0x102a1666 )
	ROM_LOAD( "mc-mr0.bin",   0x5a000, 0x4000, 0x1e854453 )
	ROM_LOAD( "mc-ms0.bin",   0x5e000, 0x4000, 0x7bc9d878 )
	ROM_LOAD( "mc-mt0.bin",   0x62000, 0x4000, 0x09cea985 )
	ROM_LOAD( "mc-mu0.bin",   0x66000, 0x4000, 0x5421769e )
	ROM_LOAD( "mc-mv0.bin",   0x6a000, 0x4000, 0x36fc3e2d )
	ROM_LOAD( "mc-mw0.bin",   0x6e000, 0x4000, 0x135dce4c )

	ROM_REGION(0x0080)	/* color proms */
	ROM_LOAD( "prom.2",       0x0000, 0x0020, 0x32db2cf4 ) /* char palette red and green components */
	ROM_LOAD( "prom.16",      0x0020, 0x0020, 0x18836d26 ) /* tile palette red and green components */
	ROM_LOAD( "prom.3",       0x0040, 0x0020, 0xc7925311 ) /* char palette blue component */
	ROM_LOAD( "prom.17",      0x0060, 0x0020, 0x41f51d49 ) /* tile palette blue component */

	ROM_REGION(0x10000)	/* 64k for audio code */
	ROM_LOAD( "mc-m50.bin",   0x4000, 0x4000, 0xba415d68 )
	ROM_LOAD( "mc-m40.bin",   0x8000, 0x4000, 0x2a217ed0 )
	ROM_LOAD( "mc-m30.bin",   0xc000, 0x4000, 0x95af1723 ) /* definitely 6502 code, for 1st CPU? */
ROM_END



static int matmania_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x0700],"\x00\x30\x00",3) == 0) &&
	    (memcmp(&RAM[0x074d],"\xb0\xb0\xb0",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0700],16*5);
			RAM[0x0028] = RAM[0x0700];
			RAM[0x0029] = RAM[0x0701];
			RAM[0x002a] = RAM[0x0702];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static int excthour_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0x0700],"\x00\x30\x00",3) == 0) &&
	    (memcmp(&RAM[0x074d],"\xc9\xcd\xb0",3) == 0))
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0700],16*5);
			RAM[0x0028] = RAM[0x0700];
			RAM[0x0029] = RAM[0x0701];
			RAM[0x002a] = RAM[0x0702];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void matmania_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x0700],16*5);
		osd_fclose(f);
	}
}


struct GameDriver matmania_driver =
{
	__FILE__,
	0,
	"matmania",
	"Mat Mania",
	"1985",
	"Technos (Taito America license)",
	"Brad Oliver (MAME driver)\nTim Lindquist (color info)",
	0,
	&matmania_machine_driver,
	0,

	matmania_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	matmania_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	matmania_hiload, matmania_hisave
};


struct GameDriver excthour_driver =
{
	__FILE__,
	&matmania_driver,
	"excthour",
	"Exciting Hour",
	"1985",
	"Technos (Taito license)",
	"Brad Oliver (MAME driver)\nTim Lindquist (color info)",
	0,
	&matmania_machine_driver,
	0,

	excthour_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	matmania_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	excthour_hiload, matmania_hisave
};

struct GameDriver maniach_driver =
{
	__FILE__,
	0,
	"maniach",
	"Mania Challenge (set 1)",
	"1986",
	"Technos (Taito America license)",
	"Brad Oliver (MAME driver)\nTim Lindquist (color info)",
	GAME_NOT_WORKING | GAME_WRONG_COLORS,
	&maniach_machine_driver,
	0,

	maniach_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	matmania_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver maniach2_driver =
{
	__FILE__,
	&maniach_driver,
	"maniach2",
	"Mania Challenge (set 2)",
	"1986",
	"Technos (Taito America license)",
	"Brad Oliver (MAME driver)\nTim Lindquist (color info)",
	GAME_NOT_WORKING | GAME_WRONG_COLORS,
	&maniach_machine_driver,
	0,

	maniach2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	matmania_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
