/***************************************************************************

Toobin Memory Map (preliminary)
-------------------------------

TOOBIN' 68010 MEMORY MAP

Function                           Address        R/W  DATA
-------------------------------------------------------------
Program ROM                        000000-07FFFF  R    D0-D15

Playfield RAM                      C00000-C07FFF  R/W  D0-D15
Alphanumerics RAM                  C08000-C097FF  R/W  D0-D15
Motion Object RAM                  C09800-C09FFF  R/W  D0-D15

Color RAM Playfield                C10000-C101FF  R/W  D0-D15
Color RAM Motion Object            C10200-C103FF  R/W  D0-D15
Color RAM Alpha                    C10400-C105FF  R/W  D0-D15
Color RAM Extra?                   C10600-C107FF  R/W  D0-D15

???? (Dip switches?)               FF6000         R

Watchdog reset                     FF8000-FF8001  W    xx
68000-to-6502 data                 FF8101         W    D0-D7
Video intensity (0=max, 1f=min)    FF8300         W    D0-D4
Scan line of next interrupt        FF8340         W    D0-D8
Head of motion object list         FF8380         W    D0-D7
Interrupt acknowledge              FF83C0-FF83C1  W    xx
Sound chip reset                   FF8400-FF8401  W    D0
EEPROM output enable               FF8500-FF8501  W    xx

Horizontal scroll register         FF8600-FF8601  W    D6-D15
Vertical scroll register           FF8700-FF8701  W    D6-D15

Player 2 Throw                     FF8800-FF8801  R    D9
Player 1 Throw                                    R    D8
Player 1 Paddle Forward (Right)                   R    D7
Player 1 Paddle Forward (Left)                    R    D6
Player 1 Paddle Backward (Left)                   R    D5
Player 1 Paddle Backward (Right)                  R    D4
Player 2 Paddle Forward (Right)                   R    D3
Player 2 Paddle Forward (Left)                    R    D2
Player 2 Paddle Backward (Left)                   R    D1
Player 2 Paddle Backward (Right)                  R    D0

VBLANK                             FF9000         R    D14
Output buffer full (at FF8101)                    R    D13
Self-test                                         R    D12

6502-to-68000 data                 FF9801         R    D0-D7

EEPROM                             FFA000-FFAFFF  R/W  D0-D7

Program RAM                        FFC000-FFFFFF  R/W  D0-D15



TOOBIN' 6502 MEMORY MAP

Function                                  Address     R/W  Data
---------------------------------------------------------------
Program RAM                               0000-1FFF   R/W  D0-D7

Music (YM-2151)                           2000-2001   R/W  D0-D7

Read 68010 Port (Input Buffer)            280A        R    D0-D7

Self-test                                 280C        R    D7
Output Buffer Full (@2A02) (Active High)              R    D5
Left Coin Switch                                      R    D1
Right Coin Switch                                     R    D0

Interrupt acknowledge                     2A00        W    xx
Write 68010 Port (Outbut Buffer)          2A02        W    D0-D7
Banked ROM select (at 3000-3FFF)          2A04        W    D6-D7
???                                       2A06        W

Effects                                   2C00-2C0F   R/W  D0-D7

Banked Program ROM (4 pages)              3000-3FFF   R    D0-D7
Static Program ROM (48K bytes)            4000-FFFF   R    D0-D7

****************************************************************************/



#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/pokey.h"
#include "sndhrdw/5220intf.h"
#include "sndhrdw/2151intf.h"


extern unsigned char *toobin_interrupt_scan;
extern unsigned char *toobin_intensity;
extern unsigned char *toobin_moslip;

int toobin_io_r (int offset);
int toobin_6502_switch_r (int offset);
int toobin_playfieldram_r (int offset);
int toobin_sound_r (int offset);
int toobin_controls_r (int offset);

void toobin_interrupt_scan_w (int offset, int data);
void toobin_interrupt_ack_w (int offset, int data);
void toobin_sound_reset_w (int offset, int data);
void toobin_moslip_w (int offset, int data);
void toobin_6502_bank_w (int offset, int data);
void toobin_paletteram_w (int offset, int data);
void toobin_playfieldram_w (int offset, int data);

int toobin_interrupt (void);
int toobin_sound_interrupt (void);

void toobin_init_machine (void);

int toobin_vh_start (void);
void toobin_vh_stop (void);
void toobin_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/*************************************
 *
 *		Main CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress toobin_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0xc00000, 0xc07fff, toobin_playfieldram_r },
	{ 0xc08000, 0xc097ff, MRA_BANK2 },
	{ 0xc09800, 0xc09fff, MRA_BANK3 },
	{ 0xc10000, 0xc107ff, paletteram_word_r },
	{ 0xff6000, 0xff6003, MRA_NOP },		/* who knows? read at controls time */
	{ 0xff8800, 0xff8803, toobin_controls_r },
	{ 0xff9000, 0xff9003, toobin_io_r },
	{ 0xff9800, 0xff9803, atarigen_sound_r },
	{ 0xffa000, 0xffafff, atarigen_eeprom_r },
	{ 0xffc000, 0xffffff, MRA_BANK1 },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress toobin_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0xc00000, 0xc07fff, toobin_playfieldram_w, &atarigen_playfieldram, &atarigen_playfieldram_size },
	{ 0xc08000, 0xc097ff, MWA_BANK2, &atarigen_alpharam, &atarigen_alpharam_size },
	{ 0xc09800, 0xc09fff, MWA_BANK3, &atarigen_spriteram, &atarigen_spriteram_size },
	{ 0xc10000, 0xc107ff, toobin_paletteram_w, &paletteram },
	{ 0xff8000, 0xff8003, watchdog_reset_w },
	{ 0xff8100, 0xff8103, atarigen_sound_w },
	{ 0xff8300, 0xff8303, MWA_BANK7, &toobin_intensity },
	{ 0xff8340, 0xff8343, toobin_interrupt_scan_w, &toobin_interrupt_scan },
	{ 0xff8380, 0xff8383, toobin_moslip_w, &toobin_moslip },
	{ 0xff83c0, 0xff83c3, toobin_interrupt_ack_w },
	{ 0xff8400, 0xff8403, toobin_sound_reset_w },
	{ 0xff8500, 0xff8503, atarigen_eeprom_enable_w },
	{ 0xff8600, 0xff8603, MWA_BANK4, &atarigen_hscroll },
	{ 0xff8700, 0xff8703, MWA_BANK5, &atarigen_vscroll },
	{ 0xffa000, 0xffafff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xffc000, 0xffffff, MWA_BANK1 },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Sound CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress toobin_sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x2001, YM2151_status_port_0_r },
	{ 0x280a, 0x280a, atarigen_6502_sound_r },
	{ 0x280c, 0x280c, toobin_6502_switch_r },
	{ 0x280e, 0x280e, MRA_NOP },
	{ 0x2c00, 0x2c0f, pokey1_r },
	{ 0x3000, 0x3fff, MRA_BANK8 },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress toobin_sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM },
	{ 0x2000, 0x2000, YM2151_register_port_0_w },
	{ 0x2001, 0x2001, YM2151_data_port_0_w },
	{ 0x2a00, 0x2a00, MWA_NOP },
	{ 0x2a02, 0x2a02, atarigen_6502_sound_w },
	{ 0x2a04, 0x2a04, toobin_6502_bank_w },
	{ 0x2a06, 0x2a06, MWA_NOP },
	{ 0x2c00, 0x2c0f, pokey1_w },
	{ 0x3000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Port definitions
 *
 *************************************/

INPUT_PORTS_START( toobin_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2, "P2 Throw", OSD_KEY_COMMA, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1, "P1 Throw", OSD_KEY_LCONTROL, IP_JOY_DEFAULT, 0)

	PORT_START	/* IN1 */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1, "P1 Right Hand", OSD_KEY_E, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1, "P1 Left Hand", OSD_KEY_Q, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1, "P1 Left Foot", OSD_KEY_A, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1, "P1 Right Foot", OSD_KEY_D, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2, "P2 Right Hand", OSD_KEY_O, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2, "P2 Left Hand", OSD_KEY_U, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2, "P2 Left Foot", OSD_KEY_J, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2, "P2 Right Foot", OSD_KEY_L, IP_JOY_DEFAULT, 0)

	PORT_START	/* IN2 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED ) /* self test */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED ) /* input buffer full */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED ) /* output buffer full */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED ) /* speech chip ready */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START	/* DSW */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x03, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



/*************************************
 *
 *		Graphics definitions
 *
 *************************************/

static struct GfxLayout toobin_charlayout =
{
	8,8,	/* 8*8 chars */
	1024,	/* 1024 chars */
	2,		/* 2 bits per pixel */
	{ 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16	/* every char takes 16 consecutive bytes */
};


static struct GfxLayout toobin_pflayout =
{
	8,8,	/* 8*8 sprites */
	16384,/* 16384 of them */
	4,		/* 4 bits per pixel */
	{ 256*1024*8, 256*1024*8+4, 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16	/* every sprite takes 16 consecutive bytes */
};


static struct GfxLayout toobin_spritelayout =
{
	16,16,/* 16*16 sprites */
	16384,/* 16384 of them */
	4,		/* 4 bits per pixel */
	{ 1024*1024*8, 1024*1024*8+4, 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	8*64	/* every sprite takes 64 consecutive bytes */
};


static struct GfxDecodeInfo toobin_gfxdecodeinfo[] =
{
	{ 1, 0x280000, &toobin_charlayout,     512, 64 },
	{ 1, 0x000000, &toobin_pflayout,         0, 16 },
	{ 1, 0x080000, &toobin_spritelayout,   256, 16 },
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
	1789790,	/* ? */
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

static struct MachineDriver toobin_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,
			0,
			toobin_readmem,toobin_writemem,0,0,
			toobin_interrupt,1
		},
		{
			CPU_M6502,
			1789790,
			2,
			toobin_sound_readmem,toobin_sound_writemem,0,0,
			0,0,
			toobin_sound_interrupt,250
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,    /* we need some interleave since the sound CPU talks to the main CPU */
	toobin_init_machine,

	/* video hardware */
	64*8, 48*8, { 0*8, 64*8-1, 0*8, 48*8-1 },
	toobin_gfxdecodeinfo,
	1024,1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	toobin_vh_start,
	toobin_vh_stop,
	toobin_vh_screenrefresh,

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
 *		ROM definition(s)
 *
 *************************************/

ROM_START( toobin_rom )
	ROM_REGION(0x80000)	/* 8*64k for 68000 code */
	ROM_LOAD_EVEN( "061-3133.bin", 0x00000, 0x10000, 0x79a92d02 )
	ROM_LOAD_ODD ( "061-3137.bin", 0x00000, 0x10000, 0xe389ef60 )
	ROM_LOAD_EVEN( "061-3134.bin", 0x20000, 0x10000, 0x3dbe9a48 )
	ROM_LOAD_ODD ( "061-3138.bin", 0x20000, 0x10000, 0xa17fb16c )
	ROM_LOAD_EVEN( "061-3135.bin", 0x40000, 0x10000, 0xdc90b45c )
	ROM_LOAD_ODD ( "061-3139.bin", 0x40000, 0x10000, 0x6f8a719a )
	ROM_LOAD_EVEN( "061-1136.bin", 0x60000, 0x10000, 0x5ae3eeac )
	ROM_LOAD_ODD ( "061-1140.bin", 0x60000, 0x10000, 0xdacbbd94 )

	ROM_REGION_DISPOSE(0x284000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "061-1101.bin", 0x000000, 0x10000, 0x02696f15 )  /* bank 0 (4 bpp)*/
	ROM_LOAD( "061-1102.bin", 0x010000, 0x10000, 0x4bed4262 )
	ROM_LOAD( "061-1103.bin", 0x020000, 0x10000, 0xe62b037f )
	ROM_LOAD( "061-1104.bin", 0x030000, 0x10000, 0xfa05aee6 )
	ROM_LOAD( "061-1105.bin", 0x040000, 0x10000, 0xab1c5578 )
	ROM_LOAD( "061-1106.bin", 0x050000, 0x10000, 0x4020468e )
	ROM_LOAD( "061-1107.bin", 0x060000, 0x10000, 0xfe6f6aed )
	ROM_LOAD( "061-1108.bin", 0x070000, 0x10000, 0x26fe71e1 )
	ROM_LOAD( "061-1143.bin", 0x080000, 0x20000, 0x211c1049 )  /* bank 0 (4 bpp)*/
	ROM_LOAD( "061-1144.bin", 0x0a0000, 0x20000, 0xef62ed2c )
	ROM_LOAD( "061-1145.bin", 0x0c0000, 0x20000, 0x067ecb8a )
	ROM_LOAD( "061-1146.bin", 0x0e0000, 0x20000, 0xfea6bc92 )
	ROM_LOAD( "061-1125.bin", 0x100000, 0x10000, 0xc37f24ac )
	ROM_RELOAD(               0x140000, 0x10000 )
	ROM_LOAD( "061-1126.bin", 0x110000, 0x10000, 0x015257f0 )
	ROM_RELOAD(               0x150000, 0x10000 )
	ROM_LOAD( "061-1127.bin", 0x120000, 0x10000, 0xd05417cb )
	ROM_RELOAD(               0x160000, 0x10000 )
	ROM_LOAD( "061-1128.bin", 0x130000, 0x10000, 0xfba3e203 )
	ROM_RELOAD(               0x170000, 0x10000 )
	ROM_LOAD( "061-1147.bin", 0x180000, 0x20000, 0xca4308cf )
	ROM_LOAD( "061-1148.bin", 0x1a0000, 0x20000, 0x23ddd45c )
	ROM_LOAD( "061-1149.bin", 0x1c0000, 0x20000, 0xd77cd1d0 )
	ROM_LOAD( "061-1150.bin", 0x1e0000, 0x20000, 0xa37157b8 )
	ROM_LOAD( "061-1129.bin", 0x200000, 0x10000, 0x294aaa02 )
	ROM_RELOAD(               0x240000, 0x10000 )
	ROM_LOAD( "061-1130.bin", 0x210000, 0x10000, 0xdd610817 )
	ROM_RELOAD(               0x250000, 0x10000 )
	ROM_LOAD( "061-1131.bin", 0x220000, 0x10000, 0xe8e2f919 )
	ROM_RELOAD(               0x260000, 0x10000 )
	ROM_LOAD( "061-1132.bin", 0x230000, 0x10000, 0xc79f8ffc )
	ROM_RELOAD(               0x270000, 0x10000 )
	ROM_LOAD( "061-1142.bin", 0x280000, 0x04000, 0xa6ab551f )  /* alpha font */

	ROM_REGION(0x14000)	/* 64k for 6502 code */
	ROM_LOAD( "061-1114.bin", 0x10000, 0x4000, 0xc0dcce1a )
	ROM_CONTINUE(             0x04000, 0xc000 )
ROM_END



/*************************************
 *
 *		Game driver(s)
 *
 *************************************/

struct GameDriver toobin_driver =
{
	__FILE__,
	0,
	"toobin",
	"Toobin'",
	"1988",
	"Atari Games",
	"Aaron Giles (MAME driver)\nTim Lindquist (hardware info)",
	0,
	&toobin_machine_driver,
	0,

	toobin_rom,
	0,
	0,
	0,
	0,	/* sound_prom */

	toobin_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_ROTATE_270,
	atarigen_hiload, atarigen_hisave
};
