/***************************************************************************

Xybots Memory Map
-----------------------------------

XYBOTS 68010 MEMORY MAP

Function                           Address        R/W  DATA
-------------------------------------------------------------
Program ROM                        000000-007FFF  R    D0-D15
Program ROM/SLAPSTIC               008000-00FFFF  R    D0-D15
Program ROM                        010000-02FFFF  R    D0-D15

NOTE: All addresses can be accessed in byte or word mode.


XYBOTS 6502 MEMORY MAP


***************************************************************************/



#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"


int xybots_playfieldram_r (int offset);

void xybots_playfieldram_w (int offset, int data);
void xybots_update_display_list (int scanline);

int xybots_vh_start (void);
void xybots_vh_stop (void);

void xybots_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static int speech_val;
static int last_ctl;


static void xybots_soundint (void)
{
	cpu_cause_interrupt (0, 2);
}


void xybots_init_machine (void)
{
	last_ctl = 0x02;
	atarigen_init_machine (xybots_soundint, 107);
}


void xybots_update (int param)
{
	xybots_update_display_list (param);
	param += 8;
	if (param < 240)
		timer_set (8.0 * cpu_getscanlineperiod (), param, xybots_update);
}


int xybots_interrupt(void)
{
	timer_set (TIME_IN_USEC (Machine->drv->vblank_duration), 0, xybots_update);
	return 1;       /* Interrupt vector 1, used by VBlank */
}


int xybots_sound_interrupt(void)
{
	return interrupt ();
}


int xybots_sw_r (int offset)
{
	return (input_port_0_r (offset) << 8) | input_port_1_r (offset);
}


int xybots_sysin_r (int offset)
{
	static int h256 = 0x0400;

	int result = (input_port_2_r (offset) << 8) | 0xff;

	if (atarigen_cpu_to_sound_ready) result ^= 0x0200;
	result ^= h256 ^= 0x0400;
	return result;
}


void xybots_sound_reset_w (int offset, int data)
{
	atarigen_sound_reset ();
}


int xybots_6502_switch_r (int offset)
{
	int temp = input_port_3_r (offset);

	if (!(input_port_2_r (offset) & 0x01)) temp ^= 0x80;
	if (atarigen_cpu_to_sound_ready) temp ^= 0x40;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x20;
	if (tms5220_ready_r ()) temp ^= 0x10;

	return temp;
}


void xybots_tms5220_w (int offset, int data)
{
	speech_val = data;
}


void xybots_6502_ctl_w (int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];


	if (((data ^ last_ctl) & 0x02) && (data & 0x02))
		tms5220_data_w (0, speech_val);
	last_ctl = data;
	cpu_setbank (8, &RAM[0x10000 + 0x1000 * ((data >> 6) & 3)]);
}




/*************************************
 *
 *		Main CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress xybots_readmem[] =
{
	{ 0x008000, 0x00ffff, atarigen_slapstic_r },
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0xff8000, 0xff8fff, MRA_BANK3 },
	{ 0xff9000, 0xffadff, MRA_BANK2 },
	{ 0xffae00, 0xffafff, MRA_BANK4 },
	{ 0xffb000, 0xffbfff, xybots_playfieldram_r },
	{ 0xffc000, 0xffc7ff, paletteram_word_r },
	{ 0xffd000, 0xffdfff, atarigen_eeprom_r },
	{ 0xffe000, 0xffe0ff, atarigen_sound_r },
	{ 0xffe100, 0xffe1ff, xybots_sw_r },
	{ 0xffe200, 0xffe2ff, xybots_sysin_r },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress xybots_writemem[] =
{
	{ 0x008000, 0x00ffff, atarigen_slapstic_w, &atarigen_slapstic },
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0xff8000, 0xff8fff, MWA_BANK3, &atarigen_alpharam, &atarigen_alpharam_size },
	{ 0xff9000, 0xffadff, MWA_BANK2 },
	{ 0xffae00, 0xffafff, MWA_BANK4, &atarigen_spriteram, &atarigen_spriteram_size },
	{ 0xffb000, 0xffbfff, xybots_playfieldram_w, &atarigen_playfieldram, &atarigen_playfieldram_size },
	{ 0xffc000, 0xffc7ff, paletteram_IIIIRRRRGGGGBBBB_word_w, &paletteram },
	{ 0xffd000, 0xffdfff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xffe800, 0xffe8ff, atarigen_eeprom_enable_w },
	{ 0xffe900, 0xffe9ff, atarigen_sound_w },
	{ 0xffea00, 0xffeaff, MWA_NOP },	/* watchdog */
	{ 0xffeb00, 0xffebff, MWA_NOP },	/* VIRQ ack */
	{ 0xffee00, 0xffeeff, xybots_sound_reset_w },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Sound CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress xybots_sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2001, YM2151_status_port_0_r },
	{ 0x280a, 0x280a, atarigen_6502_sound_r },
	{ 0x280c, 0x280c, xybots_6502_switch_r },
	{ 0x280e, 0x280e, MRA_NOP },	/* IRQ ACK */
	{ 0x2c00, 0x2c0f, pokey1_r },
	{ 0x3000, 0x3fff, MRA_BANK8 },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress xybots_sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x280e, 0x280e, MWA_NOP },	/* IRQ ACK */
	{ 0x2a00, 0x2a00, xybots_tms5220_w },
	{ 0x2a02, 0x2a02, atarigen_6502_sound_w },
	{ 0x2a04, 0x2a04, xybots_6502_ctl_w },
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

INPUT_PORTS_START( xybots_ports )
	PORT_START	/* /SW (hi) */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON2, "P1 Twist Left", OSD_KEY_Z, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3, "P1 Twist Right", OSD_KEY_X, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* /SW (lo) */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2, "P2 Twist Left", OSD_KEY_Q, IP_JOY_DEFAULT, 0 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "P2 Twist Right", OSD_KEY_W, IP_JOY_DEFAULT, 0 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* /SYSIN (hi) */
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_VBLANK )	/* VBLANK */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )	/* 256H */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED ) 	/* /AUDBUSY */
	PORT_BITX(    0x01, 0x01, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x01, "Off")
	PORT_DIPSETTING(    0x00, "On")

	PORT_START	/* IN2 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* self test */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* input buffer full */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) /* output buffer full */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* speech chip ready */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
INPUT_PORTS_END



/*************************************
 *
 *		Graphics definitions
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 chars */
	512,	/* 512 chars */
	2,		/* 2 bits per pixel */
	{ 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16	/* every char takes 16 consecutive bytes */
};


static struct GfxLayout pflayout =
{
	8,8,	/* 8*8 chars */
	8192,	/* 8192 chars */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8	/* every char takes 32 consecutive bytes */
};


static struct GfxLayout molayout =
{
	8,8,	/* 8*8 chars */
	16384,	/* 16384 chars */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*8, 4*8, 8*8, 12*8, 16*8, 20*8, 24*8, 28*8 },
	32*8	/* every char takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0xc0000, &charlayout,      0, 64 },		/* characters 8x8 */
	{ 1, 0x00000, &pflayout,      512, 16 },		/* playfield */
	{ 1, 0x40000, &molayout,      256, 16 },		/* sprites */
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


static struct TMS5220interface tms5220_interface =
{
    640000,     /* clock speed (80*samplerate) */
    100,        /* volume */
    0           /* irq handler */
};


static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4MHz */
	{ 80 | YM2151_STEREO_REVERSE },
	{ 0 }
};



/*************************************
 *
 *		Machine driver
 *
 *************************************/

static struct MachineDriver xybots_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,
			0,
			xybots_readmem,xybots_writemem,0,0,
			xybots_interrupt,1
		},
		{
			CPU_M6502,
			1789790,
			2,
			xybots_sound_readmem,xybots_sound_writemem,0,0,
			0,0,
			xybots_sound_interrupt,250
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	xybots_init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	1024,1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_SUPPORTS_DIRTY | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	xybots_vh_start,
	xybots_vh_stop,
	xybots_vh_screenrefresh,

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
		},
		{
			SOUND_POKEY,
			&pokey_interface
		}
	}
};



/*************************************
 *
 *		ROM definition(s)
 *
 *************************************/

ROM_START( xybots_rom )
	ROM_REGION(0x90000)	/* 8*64k for 68000 code */
	ROM_LOAD_EVEN( "2112.c17",     0x00000, 0x10000, 0x16d64748 )
	ROM_LOAD_ODD ( "2113.c19",     0x00000, 0x10000, 0x2677d44a )
	ROM_LOAD_EVEN( "2114.b17",     0x20000, 0x08000, 0xd31890cb )
	ROM_LOAD_ODD ( "2115.b19",     0x20000, 0x08000, 0x750ab1b0 )

	ROM_REGION_DISPOSE(0xc2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "2102.l13",     0x00000, 0x08000, 0xc1309674 )
	ROM_RELOAD(           0x08000, 0x08000 )
	ROM_LOAD( "2103.l11",     0x10000, 0x10000, 0x907c024d )
	ROM_LOAD( "2117.l7",      0x30000, 0x10000, 0x0cc9b42d )
	ROM_LOAD( "1105.de1",     0x40000, 0x10000, 0x315a4274 )
	ROM_LOAD( "1106.e1",      0x50000, 0x10000, 0x3d8c1dd2 )
	ROM_LOAD( "1107.f1",      0x60000, 0x10000, 0xb7217da5 )
	ROM_LOAD( "1108.fj1",     0x70000, 0x10000, 0x77ac65e1 )
	ROM_LOAD( "1109.j1",      0x80000, 0x10000, 0x1b482c53 )
	ROM_LOAD( "1110.k1",      0x90000, 0x10000, 0x99665ff4 )
	ROM_LOAD( "1111.kl1",     0xa0000, 0x10000, 0x416107ee )
	ROM_LOAD( "1101.c4",      0xc0000, 0x02000, 0x59c028a2 )

	ROM_REGION(0x14000)	/* 64k for 6502 code */
	ROM_LOAD( "xybots.snd",   0x10000, 0x4000, 0x3b9f155d )
	ROM_CONTINUE(           0x04000, 0xc000 )
ROM_END



/*************************************
 *
 *		Game driver(s)
 *
 *************************************/

struct GameDriver xybots_driver =
{
	__FILE__,
	0,
	"xybots",
	"Xybots",
	"1987",
	"Atari Games",
	"Aaron Giles (MAME driver)\nAlan J. McCormick (hardware info)\nFrank Palazzolo (Slapstic decoding)",
	0,
	&xybots_machine_driver,
	0,

	xybots_rom,
	0,
	0,
	0,
	0,	/* sound_prom */

	xybots_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};
