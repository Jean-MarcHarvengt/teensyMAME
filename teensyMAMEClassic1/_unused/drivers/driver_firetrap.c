/***************************************************************************

Fire Trap memory map (preliminary)

Z80:
0000-7fff ROM
8000-bfff Banked ROM (4 banks)
c000-cfff RAM
d000-d7ff bg #1 video/color RAM (alternating pages 0x100 long)
d000-dfff bg #2 video/color RAM (alternating pages 0x100 long)
e000-e3ff fg video RAM
e400-e7ff fg color RAM
e800-e97f sprites RAM

memory mapped ports:
read:
f010      IN0
f011      IN1
f012      IN2
f013      DSW0
f014      DSW1
f015      from pin 10 of 8751 controller
f016      from port #1 of 8751 controller

write:
f000      IRQ acknowledge
f001      sound command (also causes NMI on sound CPU)
f002      ROM bank selection
f003      flip screen
f004      NMI disable
f005      to port #2 of 8751 controller (signal on P3.2)
f008-f009 bg #1 y scroll
f00a-f00b bg #1 x scroll
f00c-f00d bg #2 y scroll
f00e-f00f bg #2 x scroll

interrupts:
VBlank triggers NMI.
the 8751 triggers IRQ

6502:
0000-07ff RAM
4000-7fff Banked ROM (2 banks)
8000-ffff ROM

read:
3400      command from the main cpu

write:
1000-1001 YM3526
2000      ADPCM data for the MSM5205 chip
2400      bit 0 = to sound chip MSM5205 (1 = play sample); bit 1 = IRQ enable
2800      ROM bank select


8751:
Who knows, it's protected. The bootleg doesn't have it.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6502/m6502.h"



extern unsigned char *firetrap_bg1videoram,*firetrap_bg2videoram;
extern unsigned char *firetrap_videoram,*firetrap_colorram;
extern unsigned char *firetrap_scroll1x,*firetrap_scroll1y;
extern unsigned char *firetrap_scroll2x,*firetrap_scroll2y;
extern int firetrap_bgvideoram_size;
extern int firetrap_videoram_size;
void firetrap_bg1videoram_w(int offset,int data);
void firetrap_bg2videoram_w(int offset,int data);
void firetrap_flipscreen_w(int offset,int data);
int firetrap_vh_start(void);
void firetrap_vh_stop(void);
void firetrap_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void firetrap_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static int firetrap_irq_enable = 0;

void firetrap_nmi_disable_w(int offset,int data)
{
	interrupt_enable_w(offset,~data & 1);
}

void firetrap_bankselect_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + (data & 0x03) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);
}

int firetrap_8751_r(int offset)
{
//if (errorlog) fprintf(errorlog,"PC:%04x read from 8751\n",cpu_getpc());

	/* Check for coin insertion */
	/* the following only works in the bootleg version, which doesn't have an */
	/* 8751 - the real thing is much more complicated than that. */
	if ((readinputport(2) & 0x70) != 0x70) return 0xff;
	else return 0;
}

void firetrap_8751_w(int offset,int data)
{
if (errorlog) fprintf(errorlog,"PC:%04x write %02x to 8751\n",cpu_getpc(),data);
	cpu_cause_interrupt(0,0xff);
}

static void firetrap_sound_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(1,M6502_INT_NMI);
}

static void firetrap_sound_2400_w(int offset,int data)
{
	MSM5205_reset_w(offset,~data & 0x01);
	firetrap_irq_enable = data & 0x02;
}

void firetrap_sound_bankselect_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];


	bankaddress = 0x10000 + (data & 0x01) * 0x4000;
	cpu_setbank(2,&RAM[bankaddress]);
}

void firetrap_adpcm_int (int data)
{
	static int toggle;

	toggle = 1 - toggle;
	if (firetrap_irq_enable && toggle)
		cpu_cause_interrupt (1, M6502_INT_IRQ);
}

void firetrap_adpcm_data_w (int offset, int data)
{
	MSM5205_data_w (offset, data >> 4);
	MSM5205_data_w (offset, data);
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xe97f, MRA_RAM },
	{ 0xf010, 0xf010, input_port_0_r },
	{ 0xf011, 0xf011, input_port_1_r },
	{ 0xf012, 0xf012, input_port_2_r },
	{ 0xf013, 0xf013, input_port_3_r },
	{ 0xf014, 0xf014, input_port_4_r },
	{ 0xf016, 0xf016, firetrap_8751_r },
	{ 0xf800, 0xf8ff, MRA_ROM },	/* extra ROM in the bootleg with unprotection code */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd7ff, firetrap_bg1videoram_w, &firetrap_bg1videoram, &firetrap_bgvideoram_size },
	{ 0xd800, 0xdfff, firetrap_bg2videoram_w, &firetrap_bg2videoram },
	{ 0xe000, 0xe3ff, MWA_RAM, &firetrap_videoram, &firetrap_videoram_size },
	{ 0xe400, 0xe7ff, MWA_RAM, &firetrap_colorram },
	{ 0xe800, 0xe97f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xf000, 0xf000, MWA_NOP },	/* IRQ acknowledge */
	{ 0xf001, 0xf001, firetrap_sound_command_w },
	{ 0xf002, 0xf002, firetrap_bankselect_w },
	{ 0xf003, 0xf003, firetrap_flipscreen_w },
	{ 0xf004, 0xf004, firetrap_nmi_disable_w },
//	{ 0xf005, 0xf005, firetrap_8751_w },
	{ 0xf008, 0xf009, MWA_RAM, &firetrap_scroll1y },
	{ 0xf00a, 0xf00b, MWA_RAM, &firetrap_scroll1x },
	{ 0xf00c, 0xf00d, MWA_RAM, &firetrap_scroll2y },
	{ 0xf00e, 0xf00f, MWA_RAM, &firetrap_scroll2x },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x3400, 0x3400, soundlatch_r },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x1000, 0x1000, YM3526_control_port_0_w },
	{ 0x1001, 0x1001, YM3526_write_port_0_w },
	{ 0x2000, 0x2000, firetrap_adpcm_data_w },	/* ADPCM data for the MSM5205 chip */
	{ 0x2400, 0x2400, firetrap_sound_2400_w },
	{ 0x2800, 0x2800, firetrap_sound_bankselect_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_4WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_4WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_4WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_4WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_4WAY | IPF_COCKTAIL )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )	/* bootleg only */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )	/* bootleg only */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )	/* bootleg only */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x07, "Coin A", IP_KEY_NONE )
//	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
//	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
//	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x18, 0x18, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x18, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_DIPNAME( 0x40, 0x40, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "50000 70000" )
	PORT_DIPSETTING(    0x20, "60000 80000" )
	PORT_DIPSETTING(    0x10, "80000 100000" )
	PORT_DIPSETTING(    0x00, "50000" )
	PORT_DIPNAME( 0x40, 0x40, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Yes" )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 3, 2, 1, 0, 0x1000*8+3, 0x1000*8+2, 0x1000*8+1, 0x1000*8+0 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 characters */
	256,	/* 256 characters */
	4,	/* 4 bits per pixel */
	{ 0, 4, 0x8000*8+0, 0x8000*8+4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 3, 2, 1, 0, 0x2000*8+3, 0x2000*8+2, 0x2000*8+1, 0x2000*8+0,
			16*8+3, 16*8+2, 16*8+1, 16*8+0, 16*8+0x2000*8+3, 16*8+0x2000*8+2, 16*8+0x2000*8+1, 16*8+0x2000*8+0 },
	32*8	/* every char takes 32 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	1024,	/* 1024 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1024*16*16, 2*1024*16*16, 3*1024*16*16 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0,
			16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,	        0, 16 },
	{ 1, 0x02000, &tilelayout,	     16*4,  4 },
	{ 1, 0x06000, &tilelayout,	     16*4,  4 },
	{ 1, 0x12000, &tilelayout,	     16*4,  4 },
	{ 1, 0x16000, &tilelayout,	     16*4,  4 },
	{ 1, 0x22000, &tilelayout,	16*4+4*16,  4 },
	{ 1, 0x26000, &tilelayout,	16*4+4*16,  4 },
	{ 1, 0x32000, &tilelayout,	16*4+4*16,  4 },
	{ 1, 0x36000, &tilelayout,	16*4+4*16,  4 },
	{ 1, 0x42000, &spritelayout, 16*4+4*16+4*16,  4 },
	{ -1 } /* end of array */
};



static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip (no more supported) */
	3600000,	/* 3.600000 MHz ? (partially supported) */
	{ 255 }		/* (not supported) */
};

static struct MSM5205interface msm5205_interface =
{
	1,		/* 1 chip */
	8000,	/* 8000Hz playback ? */
	firetrap_adpcm_int,		/* interrupt function */
	{ 255 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			6000000,	/* 6 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			3072000/2,	/* 1.536 Mhz? */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,0
							/* IRQs are caused by the ADPCM chip */
							/* NMIs are caused by the main CPU */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256+1,16*4+4*16+4*16+4*16,
	firetrap_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	firetrap_vh_start,
	firetrap_vh_stop,
	firetrap_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3526,
			&ym3526_interface
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

ROM_START( firetrap_rom )
	ROM_REGION(0x20000)	/* 64k for code + 64k for banked ROMs */
	ROM_LOAD( "di02.bin",     0x00000, 0x8000, 0x3d1e4bf7 )
	ROM_LOAD( "di01.bin",     0x10000, 0x8000, 0x9bbae38b )
	ROM_LOAD( "di00.bin",     0x18000, 0x8000, 0xd0dad7de )

	ROM_REGION_DISPOSE(0x62000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "di03.bin",     0x00000, 0x2000, 0x46721930 )	/* characters */
	ROM_LOAD( "di06.bin",     0x02000, 0x8000, 0x441d9154 )	/* tiles */
	ROM_LOAD( "di07.bin",     0x0a000, 0x8000, 0xef0a7e23 )
	ROM_LOAD( "di04.bin",     0x12000, 0x8000, 0x8e6e7eec )
	ROM_LOAD( "di05.bin",     0x1a000, 0x8000, 0xec080082 )
	ROM_LOAD( "di09.bin",     0x22000, 0x8000, 0xd11e28e8 )
	ROM_LOAD( "di11.bin",     0x2a000, 0x8000, 0x6424d5c3 )
	ROM_LOAD( "di08.bin",     0x32000, 0x8000, 0xc32a21d8 )
	ROM_LOAD( "di10.bin",     0x3a000, 0x8000, 0x9b89300a )
	ROM_LOAD( "di16.bin",     0x42000, 0x8000, 0x0de055d7 )	/* sprites */
	ROM_LOAD( "di13.bin",     0x4a000, 0x8000, 0x869219da )
	ROM_LOAD( "di14.bin",     0x52000, 0x8000, 0x6b65812e )
	ROM_LOAD( "di15.bin",     0x5a000, 0x8000, 0x3e27f77d )

	ROM_REGION(0x200)	/* color proms */
	ROM_LOAD( "firetrap.3b",  0x0000, 0x100, 0x8bb45337 ) /* palette red and green component */
	ROM_LOAD( "firetrap.4b",  0x0100, 0x100, 0xd5abfc64 ) /* palette blue component */

	ROM_REGION(0x18000)	/* 64k for the sound CPU + 32k for banked ROMs */
	ROM_LOAD( "di17.bin",     0x08000, 0x8000, 0x8605f6b9 )
	ROM_LOAD( "di18.bin",     0x10000, 0x8000, 0x49508c93 )

	/* there's also a protected 8751 microcontroller with ROM onboard */
ROM_END

ROM_START( firetpbl_rom )
	ROM_REGION(0x28000)	/* 64k for code + 96k for banked ROMs */
	ROM_LOAD( "ft0d.bin",     0x00000, 0x8000, 0x793ef849 )
	ROM_LOAD( "ft0c.bin",     0x10000, 0x8000, 0x5c8a0562 )
	ROM_LOAD( "ft0b.bin",     0x18000, 0x8000, 0xf2412fe8 )
	ROM_LOAD( "ft0a.bin",     0x08000, 0x8000, 0x613313ee )	/* unprotection code */

	ROM_REGION_DISPOSE(0x62000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ft0e.bin",     0x00000, 0x2000, 0xa584fc16 )	/* characters */
	ROM_LOAD( "di06.bin",     0x02000, 0x8000, 0x441d9154 )	/* tiles */
	ROM_LOAD( "di07.bin",     0x0a000, 0x8000, 0xef0a7e23 )
	ROM_LOAD( "di04.bin",     0x12000, 0x8000, 0x8e6e7eec )
	ROM_LOAD( "di05.bin",     0x1a000, 0x8000, 0xec080082 )
	ROM_LOAD( "di09.bin",     0x22000, 0x8000, 0xd11e28e8 )
	ROM_LOAD( "di11.bin",     0x2a000, 0x8000, 0x6424d5c3 )
	ROM_LOAD( "di08.bin",     0x32000, 0x8000, 0xc32a21d8 )
	ROM_LOAD( "di10.bin",     0x3a000, 0x8000, 0x9b89300a )
	ROM_LOAD( "di16.bin",     0x42000, 0x8000, 0x0de055d7 )	/* sprites */
	ROM_LOAD( "di13.bin",     0x4a000, 0x8000, 0x869219da )
	ROM_LOAD( "di14.bin",     0x52000, 0x8000, 0x6b65812e )
	ROM_LOAD( "di15.bin",     0x5a000, 0x8000, 0x3e27f77d )

	ROM_REGION(0x200)	/* color proms */
	ROM_LOAD( "firetrap.3b",  0x0000, 0x100, 0x8bb45337 ) /* palette red and green component */
	ROM_LOAD( "firetrap.4b",  0x0100, 0x100, 0xd5abfc64 ) /* palette blue component */

	ROM_REGION(0x18000)	/* 64k for the sound CPU + 32k for banked ROMs */
	ROM_LOAD( "di17.bin",     0x08000, 0x8000, 0x8605f6b9 )
	ROM_LOAD( "di18.bin",     0x10000, 0x8000, 0x49508c93 )
ROM_END


static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (memcmp(&RAM[0xca47],"\x02\x14\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xca47],93);
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
		osd_fwrite(f,&RAM[0xca47],93);
		osd_fclose(f);
	}
}



struct GameDriver firetrap_driver =
{
	__FILE__,
	0,
	"firetrap",
	"Fire Trap",
	"1986",
	"Data East USA",
	"Nicola Salmoria (MAME driver)\nTim Lindquist (color and hardware info)",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	firetrap_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver firetpbl_driver =
{
	__FILE__,
	&firetrap_driver,
	"firetpbl",
	"Fire Trap (Japan bootleg)",
	"1986",
	"bootleg",
	"Nicola Salmoria (MAME driver)\nTim Lindquist (color and hardware info)",
	0,
	&machine_driver,
	0,

	firetpbl_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
