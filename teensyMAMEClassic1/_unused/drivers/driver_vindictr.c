/***************************************************************************

vindictr Memory Map
----------------

vindictr 68010 MEMORY MAP

Program ROM             000000-05FFFF   R    D15-D0
Program ROM shared      060000-07FFFF   R    D15-D0
Program ROM             080000-09FFFF   R    D15-D0

vindictr                  0E0001-0E0FFF  R/W   D7-D0    (odd bytes only)
Program RAM             160000-16FFFF  R/W   D15-D0
UNLOCK vindictr           1Fxxxx          W

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

****************************************************************************/



#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/adpcm.h"


extern unsigned char *vindictr_playfieldpalram;

extern int vindictr_playfieldpalram_size;


int vindictr_playfieldram_r (int offset);
int vindictr_spriteram_r (int offset);
int vindictr_alpharam_r (int offset);

void vindictr_latch_w (int offset, int data);
void vindictr_playfieldram_w (int offset, int data);
void vindictr_spriteram_w (int offset, int data);
void vindictr_alpharam_w (int offset, int data);

int vindictr_interrupt (void);

void vindictr_init_machine (void);

int vindictr_vh_start (void);
void vindictr_vh_stop (void);

void vindictr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

int vindictr_update_display_list (int scanline);



/*************************************
 *
 *		Misc. functions
 *
 *************************************/

static void vindictr_soundint (void)
{
	cpu_cause_interrupt (0, 6);
}


void vindictr_init_machine (void)
{
	atarigen_init_machine (vindictr_soundint, 0);
}


static int fake_inputs (int real_port, int fake_port)
{
	int result = readinputport (real_port);
	int fake = readinputport (fake_port);

	if (fake & 0x01)			/* up */
	{
		if (fake & 0x04)		/* up and left */
			result &= ~0x20;
		else if (fake & 0x08)	/* up and right */
			result &= ~0x10;
		else					/* up only */
			result &= ~0x30;
	}
	else if (fake & 0x02)		/* down */
	{
		if (fake & 0x04)		/* down and left */
			result &= ~0x80;
		else if (fake & 0x08)	/* down and right */
			result &= ~0x40;
		else					/* down only */
			result &= ~0xc0;
	}
	else if (fake & 0x04)		/* left only */
		result &= ~0x60;
	else if (fake & 0x08)		/* right only */
		result &= ~0x90;

	return result;
}

int vindictr_input_r (int offset)
{
	int result = 0;

	switch (offset & 0x30)
	{
		case 0x00:
			result = 0xff | (fake_inputs (0, 5) << 8);
			break;

		case 0x10:
			result = input_port_2_r (offset) + (fake_inputs (1, 6) << 8);
			if (atarigen_sound_to_cpu_ready) result ^= 0x04;
			if (atarigen_cpu_to_sound_ready) result ^= 0x08;
			result ^= 0x10;
			break;

		case 0x20:
			result = 0xff | (input_port_3_r (offset) << 8);
			break;
	}

	return result;
}


int vindictr_adc_r (int offset)
{
	static int last_offset;
	int result = readinputport (3 + ((last_offset / 2) & 3)) | 0xff00;
	last_offset = offset;
	return result;
}


void vindictr_update (int param)
{
	int yscroll;

	/* update the display list */
	yscroll = vindictr_update_display_list (param);

	/* reset the timer */
	if (!param)
	{
		int next = 8 - (yscroll & 7);
		timer_set (cpu_getscanlineperiod () * (double)next, next, vindictr_update);
	}
	else if (param < 240)
		timer_set (cpu_getscanlineperiod () * 8.0, param + 8, vindictr_update);
}


int vindictr_interrupt (void)
{
	timer_set (TIME_IN_USEC (Machine->drv->vblank_duration), 0, vindictr_update);
	return 4;
}


int vindictr_sound_interrupt (void)
{
	return interrupt ();
}


void vindictr_sound_reset_w (int offset, int data)
{
	atarigen_sound_reset ();
}


int vindictr_6502_switch_r (int offset)
{
	int temp = input_port_4_r (offset);

	if (!(input_port_2_r (offset) & 0x02)) temp ^= 0x80;
	if (atarigen_cpu_to_sound_ready) temp ^= 0x40;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x20;

	return temp;
}

void vindictr_6502_ctl_w (int offset, int data)
{
	cpu_setbank (8, &Machine->memory_region[2][0x10000 + 0x1000 * ((data >> 6) & 3)]);
}


void vindictr_vblank_ack_w (int offset, int data)
{
	cpu_clear_pending_interrupts (0);
}


static unsigned char *vindictr_ram;

int vindictr_ram_r (int offset)
{
	return READ_WORD (&vindictr_ram[offset]);
}


void vindictr_ram_w (int offset, int data)
{
	COMBINE_WORD_MEM (&vindictr_ram[offset], data);
}


/*************************************
 *
 *		Main CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress vindictr_readmem[] =
{
	{ 0x000000, 0x09ffff, MRA_ROM },
	{ 0x0e0000, 0x0e0fff, atarigen_eeprom_r },
	{ 0x260000, 0x26002f, vindictr_input_r },
	{ 0x260030, 0x260033, atarigen_sound_r },
	{ 0x3e0000, 0x3e0fff, paletteram_word_r },
	{ 0x3f0000, 0x3f1fff, vindictr_playfieldram_r },
	{ 0x3f2000, 0x3f3fff, vindictr_spriteram_r },
	{ 0x3f4000, 0x3f4fff, vindictr_alpharam_r },
	{ 0x3f5000, 0x3f7fff, vindictr_ram_r },
	{ 0xff8000, 0xff9fff, vindictr_playfieldram_r },
	{ 0xffa000, 0xffbfff, vindictr_spriteram_r },
	{ 0xffc000, 0xffcfff, vindictr_alpharam_r },
	{ 0xffd000, 0xffffff, vindictr_ram_r },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress vindictr_writemem[] =
{
	{ 0x000000, 0x09ffff, MWA_ROM },
	{ 0x0e0000, 0x0e0fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0x1f0000, 0x1fffff, atarigen_eeprom_enable_w },
	{ 0x2e0000, 0x2e0003, watchdog_reset_w },
	{ 0x360000, 0x360003, vindictr_vblank_ack_w },
	{ 0x360010, 0x360013, vindictr_latch_w },
	{ 0x360020, 0x360023, vindictr_sound_reset_w },
	{ 0x360030, 0x360033, atarigen_sound_w },
	{ 0x3e0000, 0x3e0fff, paletteram_IIIIRRRRGGGGBBBB_word_w, &paletteram },
	{ 0x3f0000, 0x3f1fff, vindictr_playfieldram_w },
	{ 0x3f2000, 0x3f3fff, vindictr_spriteram_w },
	{ 0x3f4000, 0x3f4fff, vindictr_alpharam_w },
	{ 0x3f5000, 0x3f7fff, vindictr_ram_w },
	{ 0xff8000, 0xff9fff, vindictr_playfieldram_w, &atarigen_playfieldram, &atarigen_playfieldram_size },
	{ 0xffa000, 0xffbfff, vindictr_spriteram_w, &atarigen_spriteram, &atarigen_spriteram_size },
	{ 0xffc000, 0xffcfff, vindictr_alpharam_w, &atarigen_alpharam, &atarigen_alpharam_size },
	{ 0xffd000, 0xffffff, vindictr_ram_w, &vindictr_ram },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Sound CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress vindictr_sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2001, YM2151_status_port_0_r },
	{ 0x280a, 0x280a, atarigen_6502_sound_r },
	{ 0x280c, 0x280c, vindictr_6502_switch_r },
	{ 0x280e, 0x280e, MRA_NOP },	/* IRQ ACK */
	{ 0x2c00, 0x2c0f, pokey1_r },
	{ 0x3000, 0x3fff, MRA_BANK8 },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress vindictr_sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x280e, 0x280e, MWA_NOP },	/* IRQ ACK */
	{ 0x2a02, 0x2a02, atarigen_6502_sound_w },
	{ 0x2a04, 0x2a04, vindictr_6502_ctl_w },
	{ 0x2a06, 0x2a06, MWA_NOP },	/* mixer */
	{ 0x2c00, 0x2c0f, pokey1_w },
	{ 0x3000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Port definitions
 *
 *************************************/

INPUT_PORTS_START( vindictr_ports )
	PORT_START		/* 0x26000 high */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP   | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_PLAYER1 | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER1 | IPF_2WAY )

	PORT_START		/* 0x26010 high */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP    | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP   | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN  | IPF_PLAYER2 | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER2 | IPF_2WAY )

	PORT_START      /* 0x26010 low */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BITX(    0x02, 0x02, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x02, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )	/* Input buffer full (@260030) */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED ) /* Output buffer full (@360030) */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* ADEOC, end of conversion */
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* 0x26020 high */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* sound switch */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* speech chip ready */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) /* output buffer full */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* input buffer full */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* self test */

	PORT_START	/* single joystick */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER1 )

	PORT_START	/* single joystick */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT | IPF_PLAYER2 )
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

static struct POKEYinterface pokey_interface =
{
	1,	/* 1 chip */
	1789790,	/* 1.5 MHz??? */
	40,
	POKEY_DEFAULT_GAIN,
	NO_CLIP,
	/* The 8 pot handlers */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	/* The allpot handler */
	{ 0 }
};


static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579580,	/* 3.58 MHZ ? */
	{ 80 },
	{ 0 }
};



/*************************************
 *
 *		Machine driver
 *
 *************************************/

static struct MachineDriver vindictr_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,		/* 7.159 Mhz */
			0,
			vindictr_readmem,vindictr_writemem,0,0,
			vindictr_interrupt,1
		},
		{
			CPU_M6502,
			1789790,		/* 1.791 Mhz */
			2,
			vindictr_sound_readmem,vindictr_sound_writemem,0,0,
			0,0,
			vindictr_sound_interrupt,250
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	vindictr_init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	vindictr_vh_start,
	vindictr_vh_stop,
	vindictr_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_POKEY,
			&pokey_interface
		}
	}
};



/*************************************
 *
 *		ROM decoding
 *
 *************************************/

static void vindictr_rom_decode (void)
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

ROM_START( vindictr_rom )
	ROM_REGION(0x60000)	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "vin.d1",       0x00000, 0x10000, 0x2e5135e4 )
	ROM_LOAD_ODD ( "vin.d3",       0x00000, 0x10000, 0xe357fa79 )
	ROM_LOAD_EVEN( "vin.j1",       0x20000, 0x10000, 0x44c77ee0 )
	ROM_LOAD_ODD ( "vin.j3",       0x20000, 0x10000, 0x4deaa77f )
	ROM_LOAD_EVEN( "vin.k1",       0x40000, 0x10000, 0x9a0444ee )
	ROM_LOAD_ODD ( "vin.k3",       0x40000, 0x10000, 0xd5022d78 )

	ROM_REGION_DISPOSE(0x104000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "vin.p13",      0x00000, 0x20000, 0x062f8e52 )
	ROM_LOAD( "vin.p14",      0x20000, 0x10000, 0x0e4366fa )
	ROM_RELOAD(          0x30000, 0x10000 )
	ROM_LOAD( "vin.p8",       0x40000, 0x20000, 0x09123b57 )
	ROM_LOAD( "vin.p6",       0x60000, 0x10000, 0x6b757bca )
	ROM_RELOAD(          0x70000, 0x10000 )
	ROM_LOAD( "vin.r13",      0x80000, 0x20000, 0xa5268c4f )
	ROM_LOAD( "vin.r14",      0xa0000, 0x10000, 0x609f619e )
	ROM_RELOAD(          0xb0000, 0x10000 )
	ROM_LOAD( "vin.r8",       0xc0000, 0x20000, 0x2d07fdaa )
	ROM_LOAD( "vin.r6",       0xe0000, 0x10000, 0x0a2aba63 )
	ROM_RELOAD(          0xf0000, 0x10000 )
	ROM_LOAD( "vin.n17",      0x100000, 0x04000, 0xf99b631a )        /* alpha font */

	ROM_REGION(0x14000)	/* 64k + 16k for 6502 code */
	ROM_LOAD( "vin.snd",      0x10000, 0x4000, 0xd2212c0a )
	ROM_CONTINUE(        0x04000, 0xc000 )
ROM_END



/*************************************
 *
 *		Game driver(s)
 *
 *************************************/

struct GameDriver vindictr_driver =
{
	__FILE__,
	0,
	"vindictr",
	"Vindicators",
	"1988",
	"Atari Games",
	"Aaron Giles (MAME driver)\nNeil Bradley (hardware information)",
	0,
	&vindictr_machine_driver,
	0,

	vindictr_rom,
	vindictr_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	vindictr_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};
