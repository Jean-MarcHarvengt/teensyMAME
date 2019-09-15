/***************************************************************************

68000:

00000-2ffff ROM
30000-33fff RAM part of this (starting from 30000) is shared with the
                TI TMS320C10NL-14 protection microcontroller
40000-40fff RAM sprite character display properties (co-ordinates, character, color - etc)
50000-50dff Palette RAM
60000-60003 RAM
7a000-7abff RAM shared with Z80; 16-bit on this side, 8-bit on Z80 side

read:
78004       Player 1 Joystick and Buttons input port
78006       Player 2 Joystick and Buttons input port
78009       bit 7 vblank
7e000-7e005 read data from video RAM (see below)

write:
70000-70001 scroll   y   for character page (centre normally 0x01c9)
70002-70003 scroll < x > for character page (centre normally 0x00e2)
70004-70005 offset in character page to write character (7e000)

72000-72001 scroll   y   for foreground page (starts from     0x03c9)
72002-72003 scroll < x > for foreground page (centre normally 0x002a)
72004-72005 offset in character page to write character (7e002)

74000-74001 scroll   y   for background page (starts from     0x03c9)
74002-74003 scroll < x > for background page (centre normally 0x002a)
74004-74005 offset in character page to write character (7e004)

76000-76003 enable video ???
7800c       goes to the TI TMS320C10NL-14 protection microcontroller. Bit 0
            might be interrupt enable, but might also be a signal to the chip
            which then triggers the IRQ. The game writes 0c-0d in succession
            to this register when it expects the protection chip to do its
            work with the shared RAM at 30000.
7e000-7e001 data to write in text video RAM (70000)
7e002-7e003 data to write in bg video RAM (72004)
7e004-7e005 data to write in fg video RAM (74004)

Z80:
0000-7fff ROM
8000-87ff shared with 68000; 8-bit on this side, 16-bit on 68000 side

in:
00        YM3812 status
10        Coin inputs and control/service inputs
40        DSW1
50        DSW2

out:
00        YM3812 control
01        YM3812 data
20        ??

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M68000/M68000.h"


static unsigned char *twincobr_sharedram;

static unsigned char *twincobr_60000_3;
void twincobr_70004_w(int offset,int data);
int twincobr_7e000_r(int offset);
void twincobr_7e000_w(int offset,int data);
void twincobr_72004_w(int offset,int data);
int twincobr_7e002_r(int offset);
void twincobr_7e002_w(int offset,int data);
void twincobr_74004_w(int offset,int data);
int twincobr_7e004_r(int offset);
void twincobr_7e004_w(int offset,int data);
void twincobr_txscroll_w(int offset,int data);
void twincobr_bgscroll_w(int offset,int data);
void twincobr_fgscroll_w(int offset,int data);
int twincobr_vh_start(void);
void twincobr_vh_stop(void);
void twincobr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static int intenable;

int twincobr_interrupt(void)
{
	if (intenable) return MC68000_IRQ_4;
	else return MC68000_INT_NONE;
}


int twincobr_input_r(int offset)
{
	return readinputport(1 + offset / 2);
}

int twincobr_60000_3_r(int offset)
{
 unsigned short int temp6000x = 0;
 temp6000x = READ_WORD(&twincobr_60000_3[offset]);
 if (errorlog) fprintf(errorlog,"PC - read %04x to 6000%01x\n",temp6000x,offset);
 return temp6000x;
}
void twincobr_60000_3_w(int offset,int data)
{
 WRITE_WORD(&twincobr_60000_3[offset],data);
 if (errorlog) fprintf(errorlog,"PC - write %04x to 6000%01x\n",data,offset);
}



void twincobr_7800c_w(int offset,int data)
{
//// if (errorlog) fprintf(errorlog,"PC %06x - write %04x to 7800c\n",cpu_getpc(),data);
	intenable = data & 1;
}

int twincobr_sharedram_r(int offset)
{
	return twincobr_sharedram[offset / 2];
}

void twincobr_sharedram_w(int offset,int data)
{
	twincobr_sharedram[offset / 2] = data;
}

static unsigned char *ppp;
static int plap(int offset)
{
//// if (errorlog) fprintf(errorlog,"plap %06x %04x\n",cpu_getpc(),offset);
if (cpu_getpc() == 0x23a9e)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
}
if (cpu_getpc() == 0x23aac)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
}
if (cpu_getpc() == 0x23ad0)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
}
if (cpu_getpc() == 0x23c1a)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
}
if (cpu_getpc() == 0x23c3e)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
}
if (cpu_getpc() == 0x23c62)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
}
if (cpu_getpc() == 0x23c80)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
	WRITE_WORD(&ppp[0x000e],0x0000);	/* ??? */
}
if (cpu_getpc() == 0x23cb6)
{
	WRITE_WORD(&ppp[0x0000],0x0000);
	WRITE_WORD(&ppp[0x000e],0x0076);
}
	return READ_WORD(&ppp[offset]);
}
static void plop(int offset,int data)
{
//// if (errorlog) fprintf(errorlog,"plop %06x %04x %02x\n",cpu_getpc(),offset,data);
	COMBINE_WORD_MEM(&ppp[offset],data);
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x02ffff, MRA_ROM },
	{ 0x030000, 0x03001f, plap },
	{ 0x030020, 0x033fff, MRA_BANK1 },
	{ 0x040000, 0x040fff, MRA_BANK2 },  /* sprite ram data */
    { 0x050000, 0x050dff, paletteram_word_r },
	{ 0x060000, 0x060003, twincobr_60000_3_r, &twincobr_60000_3 },
	{ 0x078004, 0x078007, twincobr_input_r },
	{ 0x078008, 0x07800b, input_port_0_r },	/* vblank??? */
	{ 0x07a000, 0x07abff, twincobr_sharedram_r },     /* 16-bit on 68000 side, 8-bit on Z80 side */
	{ 0x07e000, 0x07e001, twincobr_7e000_r },	/* data from text video RAM */
	{ 0x07e002, 0x07e003, twincobr_7e002_r },	/* data from bg video RAM */
	{ 0x07e004, 0x07e005, twincobr_7e004_r },	/* data from fg video RAM */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x02ffff, MWA_ROM },
	{ 0x030000, 0x03001f, plop, &ppp },
	{ 0x030020, 0x033fff, MWA_BANK1 },
	{ 0x040000, 0x040fff, MWA_BANK2, &spriteram, &spriteram_size },  /* sprite ram data */
    { 0x050000, 0x050dff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x060000, 0x060003, twincobr_60000_3_w, &twincobr_60000_3 },
	{ 0x070000, 0x070003, twincobr_txscroll_w },	/* scroll */
	{ 0x070004, 0x070005, twincobr_70004_w },	/* offset in text video RAM */
	{ 0x072000, 0x072003, twincobr_bgscroll_w },	/* scroll */
	{ 0x072004, 0x072005, twincobr_72004_w },	/* offset in bg video RAM */
	{ 0x074000, 0x074003, twincobr_fgscroll_w },	/* scroll */
	{ 0x074004, 0x074005, twincobr_74004_w },	/* offset in fg video RAM */
	{ 0x07800c, 0x07800f, twincobr_7800c_w },
	{ 0x07a000, 0x07abff, twincobr_sharedram_w },	/* 16-bit on 68000 side, 8-bit on Z80 side */
	{ 0x07e000, 0x07e001, twincobr_7e000_w },	/* data for text video RAM */
	{ 0x07e002, 0x07e003, twincobr_7e002_w },	/* data for bg video RAM */
	{ 0x07e004, 0x07e005, twincobr_7e004_w },	/* data for fg video RAM */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM, &twincobr_sharedram },
	{ -1 }	/* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x00, 0x00, YM3812_status_port_0_r },
	{ 0x10, 0x10, input_port_3_r },
	{ 0x40, 0x40, input_port_4_r },
	{ 0x50, 0x50, input_port_5_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM3812_control_port_0_w },
	{ 0x01, 0x01, YM3812_write_port_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )     /* ? could be wrong */

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Cross Hatch Pattern", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x30, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x20, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPNAME( 0xc0, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x80, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/6 Credits" )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x03, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "50000 & every 150000" )
	PORT_DIPSETTING(    0x04, "70000 & every 200000" )
	PORT_DIPSETTING(    0x08, "50000" )
	PORT_DIPSETTING(    0x0c, "100000" )
	PORT_DIPNAME( 0x30, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x40, 0x00, "Show DIP SW Settings", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x40, "Yes" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	3,	/* 3 bits per pixel */
	{ 0*2048*8*8, 1*2048*8*8, 2*2048*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout fgtilelayout =
{
	8,8,	/* 8*8 tiles */
	8192,	/* 8192 tiles */
	4,	/* 4 bits per pixel */
	{ 0*8192*8*8, 1*8192*8*8, 2*8192*8*8, 3*8192*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8    /* every tile takes 8 consecutive bytes */
};

static struct GfxLayout bgtilelayout =
{
	8,8,	/* 8*8 tiles */
	4096,	/* 4096 tiles */
	4,	/* 4 bits per pixel */
	{ 0*4096*8*8, 1*4096*8*8, 2*4096*8*8, 3*4096*8*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8    /* every tile takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	2048,	/* 2048 sprites */
	4,	/* 4 bits per pixel */
	{ 0*2048*32*8, 1*2048*32*8, 2*2048*32*8, 3*2048*32*8 }, /* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8	/* every sprite takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,   1536, 32 },	/* colors 1536-1791 */
	{ 1, 0x0c000, &fgtilelayout, 1280, 16 },	/* colors 1280-1535 */
	{ 1, 0x4c000, &bgtilelayout, 1024, 16 },	/* colors 1024-1079 */
	{ 1, 0x6c000, &spritelayout,    0, 64 },	/* colors    0-1023 */
	{ -1 } /* end of array */
};



/* handler called by the 3812 emulator when the internal timers cause an IRQ */
static void irqhandler(void)
{
	cpu_cause_interrupt(1,0xff);
}

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip (no more supported) */
	3500000,	/* 3.5 MHz ? (partially supported) */
	{ 255 },		/* (not supported) */
	irqhandler,
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7000000,       /* 7 MHz */
			0,
			readmem,writemem,0,0,
			twincobr_interrupt,1
		},
		{
			CPU_Z80,
			3500000,        /* 3.5 MHz  */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,0	/* IRQs are caused by the YM3812 */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	100,	/* 100 CPU slices per frame */
	0,

	/* video hardware */
	64*8, 32*8, { 0*8, 40*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	1792, 1792,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	twincobr_vh_start,
	twincobr_vh_stop,
	twincobr_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		}
	}
};




/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( twincobr_rom )
	ROM_REGION(0x30000)	/* 3*64k for code */
	ROM_LOAD_EVEN( "tc16",         0x00000, 0x10000, 0x07f64d13 )
	ROM_LOAD_ODD ( "tc14",         0x00000, 0x10000, 0x41be6978 )
	ROM_LOAD_EVEN( "tc15",         0x20000, 0x08000, 0x3a646618 )
	ROM_LOAD_ODD ( "tc13",         0x20000, 0x08000, 0xd7d1e317 )

	ROM_REGION_DISPOSE(0xac000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tc11",         0x00000, 0x04000, 0x0a254133 )	/* chars */
	ROM_LOAD( "tc03",         0x04000, 0x04000, 0xe9e2d4b1 )
	ROM_LOAD( "tc04",         0x08000, 0x04000, 0xa599d845 )
	ROM_LOAD( "tc01",         0x0c000, 0x10000, 0x15b3991d )	/* fg tiles */
	ROM_LOAD( "tc02",         0x1c000, 0x10000, 0xd9e2e55d )
	ROM_LOAD( "tc05",         0x2c000, 0x10000, 0x8cc79357 )
	ROM_LOAD( "tc06",         0x3c000, 0x10000, 0x13daeac8 )
	ROM_LOAD( "tc07",         0x4c000, 0x08000, 0xb5d48389 )	/* bg tiles */
	ROM_LOAD( "tc08",         0x54000, 0x08000, 0x97f20fdc )
	ROM_LOAD( "tc09",         0x5c000, 0x08000, 0x170c01db )
	ROM_LOAD( "tc10",         0x64000, 0x08000, 0x44f5accd )
	ROM_LOAD( "tc20",         0x6c000, 0x10000, 0xcb4092b8 )	/* sprites */
	ROM_LOAD( "tc19",         0x7c000, 0x10000, 0x9cb8675e )
	ROM_LOAD( "tc18",         0x8c000, 0x10000, 0x806fb374 )
	ROM_LOAD( "tc17",         0x9c000, 0x10000, 0x4264bff8 )

	ROM_REGION(0x10000)     /* 32k for second CPU */
	ROM_LOAD( "tc12",         0x00000, 0x08000, 0xe37b3c44 )	/* slightly different from the other two sets */

	ROM_REGION(0x01000)     /* 4k for TI TMS320C10NL-14 Microcontroller */
	ROM_LOAD_EVEN( "tc1b",         0x0000, 0x0800, 0x1757cc33 )
	ROM_LOAD_ODD ( "tc2a",         0x0000, 0x0800, 0xd6d878c9 )
ROM_END

ROM_START( twincobu_rom )
	ROM_REGION(0x30000)	/* 3*64k for code */
	ROM_LOAD_EVEN( "tc16",         0x00000, 0x10000, 0x07f64d13 )
	ROM_LOAD_ODD ( "tc14",         0x00000, 0x10000, 0x41be6978 )
	ROM_LOAD_EVEN( "tcbra26.bin",  0x20000, 0x08000, 0xbdd00ba4 )
	ROM_LOAD_ODD ( "tcbra27.bin",  0x20000, 0x08000, 0xed600907 )

	ROM_REGION_DISPOSE(0xac000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tc11",         0x00000, 0x04000, 0x0a254133 )	/* chars */
	ROM_LOAD( "tc03",         0x04000, 0x04000, 0xe9e2d4b1 )
	ROM_LOAD( "tc04",         0x08000, 0x04000, 0xa599d845 )
	ROM_LOAD( "tc01",         0x0c000, 0x10000, 0x15b3991d )	/* fg tiles */
	ROM_LOAD( "tc02",         0x1c000, 0x10000, 0xd9e2e55d )
	ROM_LOAD( "tc05",         0x2c000, 0x10000, 0x8cc79357 )
	ROM_LOAD( "tc06",         0x3c000, 0x10000, 0x13daeac8 )
	ROM_LOAD( "tc07",         0x4c000, 0x08000, 0xb5d48389 )	/* bg tiles */
	ROM_LOAD( "tc08",         0x54000, 0x08000, 0x97f20fdc )
	ROM_LOAD( "tc09",         0x5c000, 0x08000, 0x170c01db )
	ROM_LOAD( "tc10",         0x64000, 0x08000, 0x44f5accd )
	ROM_LOAD( "tc20",         0x6c000, 0x10000, 0xcb4092b8 )	/* sprites */
	ROM_LOAD( "tc19",         0x7c000, 0x10000, 0x9cb8675e )
	ROM_LOAD( "tc18",         0x8c000, 0x10000, 0x806fb374 )
	ROM_LOAD( "tc17",         0x9c000, 0x10000, 0x4264bff8 )

	ROM_REGION(0x10000)     /* 32k for second CPU */
	ROM_LOAD( "b30-05",       0x00000, 0x08000, 0x1a8f1e10 )

	ROM_REGION(0x01000)     /* 4k for TI TMS320C10NL-14 Microcontroller */
	ROM_LOAD_EVEN( "tc1b",         0x0000, 0x0800, 0x1757cc33 )
	ROM_LOAD_ODD ( "tc2a",         0x0000, 0x0800, 0xd6d878c9 )
ROM_END

ROM_START( ktiger_rom )
	ROM_REGION(0x30000)	/* 3*64k for code */
	ROM_LOAD_EVEN( "tc16",         0x00000, 0x10000, 0x07f64d13 )
	ROM_LOAD_ODD ( "tc14",         0x00000, 0x10000, 0x41be6978 )
	ROM_LOAD_EVEN( "b30-02",       0x20000, 0x08000, 0x1d63e9c4 )
	ROM_LOAD_ODD ( "b30-04",       0x20000, 0x08000, 0x03957a30 )

	ROM_REGION_DISPOSE(0xac000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tc11",         0x00000, 0x04000, 0x0a254133 )	/* chars */
	ROM_LOAD( "tc03",         0x04000, 0x04000, 0xe9e2d4b1 )
	ROM_LOAD( "tc04",         0x08000, 0x04000, 0xa599d845 )
	ROM_LOAD( "tc01",         0x0c000, 0x10000, 0x15b3991d )	/* fg tiles */
	ROM_LOAD( "tc02",         0x1c000, 0x10000, 0xd9e2e55d )
	ROM_LOAD( "tc05",         0x2c000, 0x10000, 0x8cc79357 )
	ROM_LOAD( "tc06",         0x3c000, 0x10000, 0x13daeac8 )
	ROM_LOAD( "tc07",         0x4c000, 0x08000, 0xb5d48389 )	/* bg tiles */
	ROM_LOAD( "tc08",         0x54000, 0x08000, 0x97f20fdc )
	ROM_LOAD( "tc09",         0x5c000, 0x08000, 0x170c01db )
	ROM_LOAD( "tc10",         0x64000, 0x08000, 0x44f5accd )
	ROM_LOAD( "tc20",         0x6c000, 0x10000, 0xcb4092b8 )	/* sprites */
	ROM_LOAD( "tc19",         0x7c000, 0x10000, 0x9cb8675e )
	ROM_LOAD( "tc18",         0x8c000, 0x10000, 0x806fb374 )
	ROM_LOAD( "tc17",         0x9c000, 0x10000, 0x4264bff8 )

	ROM_REGION(0x10000)     /* 32k for second CPU */
	ROM_LOAD( "b30-05",       0x00000, 0x08000, 0x1a8f1e10 )

	ROM_REGION(0x01000)     /* 4k for TI TMS320C10NL-14 Microcontroller */
	ROM_LOAD_EVEN( "tc1b",         0x0000, 0x0800, 0x1757cc33 )
	ROM_LOAD_ODD ( "tc2a",         0x0000, 0x0800, 0xd6d878c9 )
ROM_END

ROM_START( fshark_rom )
	ROM_REGION(0x20000)     /* 2*64k for code */
	ROM_LOAD_EVEN( "b02_18-1.rom", 0x00000, 0x10000, 0x0 )
	ROM_LOAD_ODD ( "b02_17-1.rom", 0x00000, 0x10000, 0x0 )

	ROM_REGION_DISPOSE(0xac000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "b02_05-1.rom", 0x00000, 0x04000, 0x0 )        /* chars */
	ROM_LOAD( "b02_06-1.rom", 0x04000, 0x04000, 0x0 )
	ROM_LOAD( "b02_07-1.rom", 0x08000, 0x04000, 0x0 )
	ROM_LOAD( "b02_01.256",   0x0c000, 0x08000, 0x0 )        /* fg tiles */
	ROM_LOAD( "b02_02.256",   0x14000, 0x08000, 0x0 )
	ROM_LOAD( "b02_03.256",   0x1c000, 0x08000, 0x0 )
	ROM_LOAD( "b02_04.256",   0x24000, 0x08000, 0x0 )
	ROM_LOAD( "b02_08.rom",   0x2c000, 0x08000, 0x0 )
	ROM_LOAD( "b02_09.rom",   0x34000, 0x08000, 0x0 )
	ROM_LOAD( "b02_10.rom",   0x3c000, 0x08000, 0x0 )
	ROM_LOAD( "b02_11.rom",   0x44000, 0x08000, 0x0 )
	ROM_LOAD( "b02_12.rom",   0x4c000, 0x08000, 0x0 )        /* bg tiles */
	ROM_LOAD( "b02_13.rom",   0x54000, 0x08000, 0x0 )
	ROM_LOAD( "b02_14.rom",   0x5c000, 0x08000, 0x0 )
	ROM_LOAD( "b02_15.rom",   0x64000, 0x08000, 0x0 )
	ROM_LOAD( "b02_01.512",   0x6c000, 0x10000, 0x0 )        /* sprites */
	ROM_LOAD( "b02_02.512",   0x7c000, 0x10000, 0x0 )
	ROM_LOAD( "b02_03.512",   0x8c000, 0x10000, 0x0 )
	ROM_LOAD( "b02_04.512",   0x9c000, 0x10000, 0x0 )

	ROM_REGION(0x10000)     /* 64k for second CPU */
	ROM_LOAD( "b02_16.rom",   0x0000, 0x8000, 0x0 )
ROM_END




struct GameDriver twincobr_driver =
{
	__FILE__,
	0,
	"twincobr",
	"Twin Cobra (Taito)",
	"1987",
	"Taito",
	"Quench\nNicola Salmoria",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	twincobr_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver twincobu_driver =
{
	__FILE__,
	&twincobr_driver,
	"twincobu",
	"Twin Cobra (Romstar)",
	"1987",
	"Taito of America (Romstar license)",
	"Quench\nNicola Salmoria",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	twincobu_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};

/* This doesn't work yet */
struct GameDriver ktiger_driver =
{
	__FILE__,
	&twincobr_driver,
	"ktiger",
	"Kyukyoku Tiger",
	"1987",
	"Taito",
	"Quench\nNicola Salmoria",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	ktiger_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver fshark_driver =
{
	__FILE__,
	0,
	"fshark",
	"Flying Shark",
	"????",
	"?????",
	"Quench\nNicola Salmoria",
	0,
	&machine_driver,
	0,

	fshark_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};
