/***************************************************************************

EPRoM Memory Map
----------------

EPRoM 68010 MEMORY MAP

Program ROM             000000-05FFFF   R    D15-D0
Program ROM shared      060000-07FFFF   R    D15-D0
Program ROM             080000-09FFFF   R    D15-D0

EEPROM                  0E0001-0E0FFF  R/W   D7-D0    (odd bytes only)
Program RAM             160000-16FFFF  R/W   D15-D0
UNLOCK EEPROM           1Fxxxx          W

Player 1 Input (left)   260000          R    D11-D8 Active lo
Player 2 Input (right)  260010          R    D11-D8 Active lo
      D8:    start
      D9:    fire
      D10:   spare
      D11:   duck

VBLANK                  260010          R    D0 Active lo
Self-test                               R    D1 Active lo
Input buffer full (@260030)             R    D2 Active lo
Output buffer full (@360030)            R    D3 Active lo
ADEOC, end of conversion                R    D4 Active hi

ADC0, analog port       260020          R    D0-D7
ADC1                    260022          R    D0-D7
ADC2                    260024          R    D0-D7
ADC3                    260026          R    D0-D7

Read sound processor    260030          R    D0-D7

Watch Dog               2E0000          W    xx        (128 msec. timeout)

VBLANK Interrupt ack.   360000          W    xx

Video off               360010          W    D5 (active hi)
Video intensity                         W    D1-D4 (0=full on)
EXTRA cpu reset                         W    D0 (lo to reset)

Sound processor reset   360020          W    xx

Write sound processor   360030          W    D0-D7

Color RAM Alpha         3E0000-3E01FF  R/W   D15-D0
Color RAM Motion Object 3E0200-3E03FF  R/W   D15-D0
Color RAM Playfield     3E0400-3E07FE  R/W   D15-D0
Color RAM STAIN         3E0800-3E0FFE  R/W   D15-D0

Playfield Picture RAM   3F0000-3F1FFF  R/W   D15-D0
Motion Object RAM       3F2000-3F3FFF  R/W   D15-D0
Alphanumerics RAM       3F4000-3F4EFF  R/W   D15-D0
Scroll and MOB config   3F4F00-3F4F70  R/W   D15-D0
SLIP pointers           3F4F80-3F4FFF  R/W   D9-D0
Working RAM             3F5000-3F7FFF  R/W   D15-D0
Playfield palette RAM   3F8000-3F9FFF  R/W   D11-D8

-----------------------------------------------------------

EPRoM EXTRA 68010 MEMORY MAP

Program ROM             000000-05FFFF   R    D15-D0
Program ROM shared      060000-07FFFF   R    D15-D0

Program RAM             160000-16FFFF  R/W   D15-D0

Player 1 Input (left)   260000          R    D11-D8 Active lo
Player 2 Input (right)  260010          R    D11-D8 Active lo
      D8:    start
      D9:    fire
      D10:   spare
      D11:   duck

VBLANK                  260010          R    D0 Active lo
Self-test                               R    D1 Active lo
Input buffer full (@260030)             R    D2 Active lo
Output buffer full (@360030)            R    D3 Active lo
ADEOC, end of conversion                R    D4 Active hi

ADC0, analog port       260020          R    D0-D7
ADC1                    260022          R    D0-D7
ADC2                    260024          R    D0-D7
ADC3                    260026          R    D0-D7

Read sound processor    260030          R    D0-D7

VBLANK Interrupt ack.   360000          W    xx

Video off               360010          W    D5 (active hi)
Video intensity                         W    D1-D4 (0=full on)
EXTRA cpu reset                         W    D0 (lo to reset)

Sound processor reset   360020          W    xx

Write sound processor   360030          W    D0-D7

****************************************************************************/



#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/adpcm.h"


extern unsigned char *eprom_playfieldpalram;

extern int eprom_playfieldpalram_size;


int eprom_playfieldram_r (int offset);
int eprom_playfieldpalram_r (int offset);

void eprom_latch_w (int offset, int data);
void eprom_playfieldram_w (int offset, int data);
void eprom_playfieldpalram_w (int offset, int data);

int eprom_interrupt (void);

void eprom_init_machine (void);

int eprom_vh_start (void);
void eprom_vh_stop (void);

void eprom_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

int eprom_update_display_list (int scanline);



/*************************************
 *
 *		Misc. functions
 *
 *************************************/

static void eprom_soundint (void)
{
	cpu_cause_interrupt (0, 6);
}


void eprom_init_machine (void)
{
	atarigen_init_machine (eprom_soundint, 0);
}


int eprom_input_r (int offset)
{
	int result;

	if (offset & 0x10)
	{
		result = input_port_2_r (offset) + (input_port_1_r (offset) << 8);
		if (atarigen_sound_to_cpu_ready) result ^= 0x04;
		if (atarigen_cpu_to_sound_ready) result ^= 0x08;
		result ^= 0x10;
	}
	else
		result = 0xff + (input_port_0_r (offset) << 8);

	return result;
}


int eprom_adc_r (int offset)
{
	static int last_offset;
	int result = readinputport (3 + ((last_offset / 2) & 3)) | 0xff00;
	last_offset = offset;
	return result;
}


void eprom_update (int param)
{
	/* update the display list */
	int yscroll = eprom_update_display_list (param);

	/* reset the timer */
	if (!param)
	{
		int next = 8 - (yscroll & 7);
		timer_set (cpu_getscanlineperiod () * (double)next, next, eprom_update);
	}
	else if (param < 240)
		timer_set (cpu_getscanlineperiod () * 8.0, param + 8, eprom_update);
}


int eprom_interrupt (void)
{
	timer_set (TIME_IN_USEC (Machine->drv->vblank_duration), 0, eprom_update);
	return 4;
}


int eprom_extra_interrupt (void)
{
	return 4;
}


int eprom_sound_interrupt (void)
{
	return interrupt ();
}


void eprom_sound_reset_w (int offset, int data)
{
	atarigen_sound_reset ();
}


int eprom_6502_switch_r (int offset)
{
	int temp = input_port_7_r (offset);

	if (!(input_port_2_r (offset) & 0x02)) temp ^= 0x80;
	if (atarigen_cpu_to_sound_ready) temp ^= 0x40;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x20;
	if (tms5220_ready_r ()) temp ^= 0x10;

	return temp;
}


static int speech_val;

void eprom_tms5220_w (int offset, int data)
{
	speech_val = data;
}


static int last_ctl;

void eprom_6502_ctl_w (int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];


	if (((data ^ last_ctl) & 0x02) && (data & 0x02))
		tms5220_data_w (0, speech_val);
	last_ctl = data;
	cpu_setbank (8, &RAM[0x10000 + 0x1000 * ((data >> 6) & 3)]);
}


unsigned char *eprom_sync;

int eprom_sync_r (int offset)
{
	return READ_WORD (&eprom_sync[offset]);
}


void eprom_sync_w (int offset, int data)
{
	int oldword = READ_WORD (&eprom_sync[offset]);
	int newword = COMBINE_WORD (oldword, data);
	WRITE_WORD (&eprom_sync[offset], newword);
	if ((oldword & 0xff00) != (newword & 0xff00))
		cpu_yield ();
}


void eprom_vblank_ack_w (int offset, int data)
{
	cpu_clear_pending_interrupts (0);
	cpu_clear_pending_interrupts (2);
}


/*************************************
 *
 *		Main CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress eprom_readmem[] =
{
	{ 0x000000, 0x09ffff, MRA_ROM },
	{ 0x0e0000, 0x0e0fff, atarigen_eeprom_r },
	{ 0x16cc00, 0x16cc01, eprom_sync_r },
	{ 0x160000, 0x16ffff, MRA_BANK1 },
	{ 0x260000, 0x26001f, eprom_input_r },
	{ 0x260020, 0x26002f, eprom_adc_r },
	{ 0x260030, 0x260033, atarigen_sound_r },
	{ 0x3e0000, 0x3e0fff, paletteram_word_r },
	{ 0x3f0000, 0x3f1fff, eprom_playfieldram_r },
	{ 0x3f2000, 0x3f3fff, MRA_BANK3 },
	{ 0x3f4000, 0x3f4fff, MRA_BANK4 },
	{ 0x3f5000, 0x3f7fff, MRA_BANK5 },
	{ 0x3f8000, 0x3f9fff, eprom_playfieldpalram_r },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress eprom_writemem[] =
{
	{ 0x000000, 0x09ffff, MWA_ROM },
	{ 0x0e0000, 0x0e0fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x16cc00, 0x16cc01, eprom_sync_w, &eprom_sync },
	{ 0x160000, 0x16ffff, MWA_BANK1 },
	{ 0x1f0000, 0x1fffff, atarigen_eeprom_enable_w },
/*	{ 0x2e0000, 0x2e0003, watchdog_reset_w },*/
	{ 0x2e0000, 0x2e0003, MWA_NOP },
	{ 0x360000, 0x360003, eprom_vblank_ack_w },
	{ 0x360010, 0x360013, eprom_latch_w },
	{ 0x360020, 0x360023, eprom_sound_reset_w },
	{ 0x360030, 0x360033, atarigen_sound_w },
	{ 0x3e0000, 0x3e0fff, paletteram_IIIIRRRRGGGGBBBB_word_w, &paletteram },
	{ 0x3f0000, 0x3f1fff, eprom_playfieldram_w, &atarigen_playfieldram, &atarigen_playfieldram_size },
	{ 0x3f2000, 0x3f3fff, MWA_BANK3, &atarigen_spriteram, &atarigen_spriteram_size },
	{ 0x3f4000, 0x3f4fff, MWA_BANK4, &atarigen_alpharam, &atarigen_alpharam_size },
	{ 0x3f5000, 0x3f7fff, MWA_BANK5 },
	{ 0x3f8000, 0x3f9fff, eprom_playfieldpalram_w, &eprom_playfieldpalram, &eprom_playfieldpalram_size },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Sound CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress eprom_sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2001, YM2151_status_port_0_r },
	{ 0x280a, 0x280a, atarigen_6502_sound_r },
	{ 0x280c, 0x280c, eprom_6502_switch_r },
	{ 0x280e, 0x280e, MRA_NOP },	/* IRQ ACK */
	{ 0x3000, 0x3fff, MRA_BANK8 },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress eprom_sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x280e, 0x280e, MWA_NOP },	/* IRQ ACK */
	{ 0x2a00, 0x2a00, eprom_tms5220_w },
	{ 0x2a02, 0x2a02, atarigen_6502_sound_w },
	{ 0x2a04, 0x2a04, eprom_6502_ctl_w },
	{ 0x2a06, 0x2a06, MWA_NOP },	/* mixer */
	{ 0x3000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Extra CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress eprom_extra_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x16cc00, 0x16cc01, eprom_sync_r, &eprom_sync },
	{ 0x160000, 0x16ffff, MRA_BANK1 },
	{ 0x260000, 0x26001f, eprom_input_r },
	{ 0x260020, 0x26002f, eprom_adc_r },
	{ 0x260030, 0x260033, atarigen_sound_r },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress eprom_extra_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x16cc00, 0x16cc01, eprom_sync_w },
	{ 0x160000, 0x16ffff, MWA_BANK1 },
	{ 0x360000, 0x360003, eprom_vblank_ack_w },
	{ 0x360010, 0x360013, eprom_latch_w },
	{ 0x360020, 0x360023, eprom_sound_reset_w },
	{ 0x360030, 0x360033, atarigen_sound_w },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Port definitions
 *
 *************************************/

INPUT_PORTS_START( eprom_ports )
	PORT_START		/* 0x26000 high */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* 0x26010 high */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* 0x26010 low */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BITX(    0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x02, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )	/* Input buffer full (@260030) */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* Output buffer full (@360030) */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* ADEOC, end of conversion */
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* ADC0 @ 0x260020 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER1, 100, 0, 0x10, 0xf0 )

	PORT_START	/* ADC1 @ 0x260022 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER1, 100, 0, 0x10, 0xf0 )

	PORT_START	/* ADC0 @ 0x260024 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER2, 100, 0, 0x10, 0xf0 )

	PORT_START	/* ADC1 @ 0x260026 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER2, 100, 0, 0x10, 0xf0 )

	PORT_START	/* sound switch */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* speech chip ready */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) /* output buffer full */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* input buffer full */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* self test */
INPUT_PORTS_END



/*************************************
 *
 *		Graphics definitions
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	1024,	/* 1024 chars */
	2,		/* 2 bits per pixel */
	{ 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16	/* every char takes 16 consecutive bytes */
};


static struct GfxLayout spritelayout =
{
	8,8,	/* 8*8 sprites */
	32768,	/* 32768 of them */
	4,		/* 4 bits per pixel */
	{ 0*8*0x40000, 1*8*0x40000, 2*8*0x40000, 3*8*0x40000 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x100000, &charlayout,      0, 64 },		/* characters 8x8 */
	{ 1, 0x000000, &spritelayout,  256, 32 },		/* sprites & playfield */
	{ -1 } /* end of array */
};



/*************************************
 *
 *		Sound definitions
 *
 *************************************/

static struct TMS5220interface tms5220_interface =
{
    640000,     /* clock speed (80*samplerate) */
    100,        /* volume */
    0 /* irq handler */
};


static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579580,	/* 3.58 MHZ ? */
	{ 30 },
	{ 0 }
};



/*************************************
 *
 *		Machine driver
 *
 *************************************/

static struct MachineDriver eprom_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,		/* 7.159 Mhz */
			0,
			eprom_readmem,eprom_writemem,0,0,
			eprom_interrupt,1
		},
		{
			CPU_M6502,
			1789790,		/* 1.791 Mhz */
			2,
			eprom_sound_readmem,eprom_sound_writemem,0,0,
			0,0,
			eprom_sound_interrupt,250
		},
		{
			CPU_M68000,
			7159160,		/* 7.159 Mhz */
			3,
			eprom_extra_readmem,eprom_extra_writemem,0,0,
			eprom_extra_interrupt,1
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	eprom_init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	eprom_vh_start,
	eprom_vh_stop,
	eprom_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_TMS5220,
			&tms5220_interface
		}
	}
};



/*************************************
 *
 *		ROM decoding
 *
 *************************************/

static void eprom_rom_decode (void)
{
	int i;

	/* invert the graphics bits on the playfield and motion objects */
	for (i = 0; i < 0x100000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}



/*************************************
 *
 *		ROM definition(s)
 *
 *************************************/

ROM_START( eprom_rom )
	ROM_REGION(0xa0000)	/* 10*64k for 68000 code */
	ROM_LOAD_EVEN( "136069.50a",   0x00000, 0x10000, 0x08888dec )
	ROM_LOAD_ODD ( "136069.40a",   0x00000, 0x10000, 0x29cb1e97 )
	ROM_LOAD_EVEN( "136069.50b",   0x20000, 0x10000, 0x702241c9 )
	ROM_LOAD_ODD ( "136069.40b",   0x20000, 0x10000, 0xfecbf9e2 )
	ROM_LOAD_EVEN( "136069.50d",   0x40000, 0x10000, 0x0f2f1502 )
	ROM_LOAD_ODD ( "136069.40d",   0x40000, 0x10000, 0xbc6f6ae8 )
	ROM_LOAD_EVEN( "136069.40k",   0x60000, 0x10000, 0x130650f6 )
	ROM_LOAD_ODD ( "136069.50k",   0x60000, 0x10000, 0x1da21ed8 )

	ROM_REGION_DISPOSE(0x104000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136069.47s",   0x00000, 0x10000, 0x0de9d98d )
	ROM_LOAD( "136069.43s",   0x10000, 0x10000, 0x8eb106ad )
	ROM_LOAD( "136069.38s",   0x20000, 0x10000, 0xbf3d0e18 )
	ROM_LOAD( "136069.32s",   0x30000, 0x10000, 0x48fb2e42 )
	ROM_LOAD( "136069.76s",   0x40000, 0x10000, 0x602d939d )
	ROM_LOAD( "136069.70s",   0x50000, 0x10000, 0xf6c973af )
	ROM_LOAD( "136069.64s",   0x60000, 0x10000, 0x9cd52e30 )
	ROM_LOAD( "136069.57s",   0x70000, 0x10000, 0x4e2c2e7e )
	ROM_LOAD( "136069.47u",   0x80000, 0x10000, 0xe7edcced )
	ROM_LOAD( "136069.43u",   0x90000, 0x10000, 0x9d3e144d )
	ROM_LOAD( "136069.38u",   0xa0000, 0x10000, 0x23f40437 )
	ROM_LOAD( "136069.32u",   0xb0000, 0x10000, 0x2a47ff7b )
	ROM_LOAD( "136069.76u",   0xc0000, 0x10000, 0xb0cead58 )
	ROM_LOAD( "136069.70u",   0xd0000, 0x10000, 0xfbc3934b )
	ROM_LOAD( "136069.64u",   0xe0000, 0x10000, 0x0e07493b )
	ROM_LOAD( "136069.57u",   0xf0000, 0x10000, 0x34f8f0ed )
	ROM_LOAD( "1360691.25d",  0x100000, 0x04000, 0x409d818e )

	ROM_REGION(0x14000)	/* 64k + 16k for 6502 code */
	ROM_LOAD( "136069.7b",    0x10000, 0x4000, 0x86e93695 )
	ROM_CONTINUE(             0x04000, 0xc000 )

	ROM_REGION(0x80000)	/* 8*64k for 68000 code */
	ROM_LOAD_EVEN( "136069.10s",   0x00000, 0x10000, 0xdeff6469 )
	ROM_LOAD_ODD ( "136069.10u",   0x00000, 0x10000, 0x5d7afca2 )
	ROM_LOAD_EVEN( "136069.40k",   0x60000, 0x10000, 0x130650f6 )
	ROM_LOAD_ODD ( "136069.50k",   0x60000, 0x10000, 0x1da21ed8 )
ROM_END



/*************************************
 *
 *		Game driver(s)
 *
 *************************************/

struct GameDriver eprom_driver =
{
	__FILE__,
	0,
	"eprom",
	"Escape from the Planet of the Robot Monsters",
	"1989",
	"Atari Games",
	"Aaron Giles (MAME driver)\nTim Lindquist (hardware information)",
	0,
	&eprom_machine_driver,
	0,

	eprom_rom,
	eprom_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	eprom_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};
