/***************************************************************************

Atari System 1 Memory Map
-------------------------

SYSTEM 1 68010 MEMORY MAP

Function                           Address        R/W  DATA
-------------------------------------------------------------
Program ROM                        000000-087FFF  R    D0-D15

Program RAM                        400000-401FFF  R/W  D0-D15

Playfield Horizontal Scroll        800000,800001  W    D0-D8
Playfield Vertical Scroll          820000,820001  W    D0-D8
Playfield Special Priority Color   840000,840001  R    D0-D7

Sound Processor Reset (Low Reset)  860000         W    D7
Trak-Ball Test                                    W    D6
Motion Object Parameter Buffer Select             W    D5-D3
Playfield ROM Bank Select                         W    D2
Trak-Ball Resolution and Test LED                 W    D1
Alphanumerics ROM Bank Select                     W    D0

Watchdog (128 msec. timeout)       880000         W    xx
VBlank Acknowledge                 8A0000         W    xx
Unlock EEPROM                      8C0000         W    xx

Cartridge External                 900000-9FFFFF  R/W  D0-D15

Playfield RAM                      A00000-A01FFF  R/W  D0-D15
Motion Object Vertical Position    A02000-A0207F  R/W  D0-D15
                                   A02200-A0227F  R/W  D0-D15
                                   A02E00-A02E7F  R/W  D0-D15
Motion Object Picture              A02080-A020FF  R/W  D0-D15
                                   A02280-A022FF  R/W  D0-D15
                                   A02E80-A02EFF  R/W  D0-D15
Motion Object Horizontal Position  A02100-A02170  R/W  D0-D15
                                   A02300-A02370  R/W  D0-D15
                                   A02F00-A02F70  R/W  D0-D15
Motion Object Link                 A02180-A021FF  R/W  D0-D15
                                   A02380-A023FF  R/W  D0-D15
                                   A02F80-A02FFF  R/W  D0-D15
Alphanumerics RAM                  A03000-A03FFF  R/W  D0-D15

Color RAM Alpha                    B00000-B001FF  R/W  D0-D15
Color RAM Motion Object            B00200-B003FF  R/W  D0-D15
Color RAM Playfield                B00400-B005FF  R/W  D0-D15
Color RAM Translucent              B00600-B0061F  R/W  D0-D15

EEPROM                             F00001-F00FFF  R/W  D7-D0
Trak-Ball                          F20000-F20006  R    D7-D0
Analog Joystick                    F40000-F4000E  R    D7-D0
Analog Joystick IRQ Disable        F40010         R    D7

Output Buffer Full (@FE0000)       F60000         R    D7
Self-Test                                         R    D6
Switch Input                                      R    D5
Vertical Blank                                    R    D4
Switch Input                                      R    D3
Switch Input                                      R    D2
Switch Input                                      R    D1
Switch Input                                      R    D0

Read Sound Processor (6502)        FC0000         R    D0-D7
Write Sound Processor (6502)       FE0000         W    D0-D7


NOTE: All addresses can be accessed in byte or word mode.


SYSTEM 1 6502 MEMORY MAP

Function                                  Address     R/W  Data
---------------------------------------------------------------
Program RAM                               0000-0FFF   R/W  D0-D7
Cartridge External                        1000-1FFF   R/W  D0-D7

Music                                     1800-1801   R/W  D0-D7
Read 68010 Port (Input Buffer)            1810        R    D0-D7
Write 68010 Port (Outbut Buffer)          1810        W    D0-D7

Self-Test (Active Low)                    1820        R    D7
Output Buffer Full (@1000) (Active High)              R    D4
Data Available (@ 1010) (Active High)                 R    D3
Auxilliary Coin Switch                                R    D2
Left Coin Switch                                      R    D1
Right Coin Switch                                     R    D0

Music Reset (Low Reset)                   1820        W    D0
LED 1                                     1824        W    D0
LED 2                                     1825        W    D0
Coin Counter Right (Active High)          1826        W    D0
Coin Counter Left (Active High)           1827        W    D0

Effects                                   1870-187F   R/W  D0-D7

Program ROM (48K bytes)                   4000-FFFF   R    D0-D7
****************************************************************************/



#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/pokey.h"
#include "sndhrdw/5220intf.h"
#include "sndhrdw/2151intf.h"


extern unsigned char *atarisys1_bankselect;
extern unsigned char *atarisys1_prioritycolor;

extern unsigned char *marble_speedcheck;

int atarisys1_io_r (int offset);
int atarisys1_6502_switch_r (int offset);
int atarisys1_6522_r (int offset);
int atarisys1_int3state_r (int offset);
int atarisys1_trakball_r (int offset);
int atarisys1_joystick_r (int offset);
int atarisys1_playfieldram_r (int offset);
int atarisys1_spriteram_r (int offset);

int marble_speedcheck_r (int offset);

void atarisys1_led_w (int offset, int data);
void atarisys1_6522_w (int offset, int data);
void atarisys1_joystick_w (int offset, int data);
void atarisys1_playfieldram_w (int offset, int data);
void atarisys1_spriteram_w (int offset, int data);
void atarisys1_bankselect_w (int offset, int data);
void atarisys1_hscroll_w (int offset, int data);
void atarisys1_vscroll_w (int offset, int data);

void marble_speedcheck_w (int offset, int data);

int atarisys1_interrupt (void);
void atarisys1_sound_interrupt (void);

void marble_init_machine (void);
void peterpak_init_machine (void);
void indytemp_init_machine (void);
void roadrunn_init_machine (void);
void roadblst_init_machine (void);

int marble_vh_start (void);
int peterpak_vh_start (void);
int indytemp_vh_start (void);
int roadrunn_vh_start (void);
int roadblst_vh_start (void);
void atarisys1_vh_stop (void);

void atarisys1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void roadblst_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/*************************************
 *
 *		Main CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress atarisys1_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x080000, 0x087fff, atarigen_slapstic_r },
	{ 0x2e0000, 0x2e0003, atarisys1_int3state_r },
	{ 0x400000, 0x401fff, MRA_BANK1 },
	{ 0x840000, 0x840003, MRA_BANK4, &atarisys1_prioritycolor },
	{ 0x900000, 0x9fffff, MRA_BANK2 },
	{ 0xa00000, 0xa01fff, atarisys1_playfieldram_r },
	{ 0xa02000, 0xa02fff, atarisys1_spriteram_r },
	{ 0xa03000, 0xa03fff, MRA_BANK6 },
	{ 0xb00000, 0xb007ff, paletteram_word_r },
	{ 0xf00000, 0xf00fff, atarigen_eeprom_r },
	{ 0xf20000, 0xf20007, atarisys1_trakball_r },
	{ 0xf40000, 0xf40013, atarisys1_joystick_r },
	{ 0xf60000, 0xf60003, atarisys1_io_r },
	{ 0xfc0000, 0xfc0003, atarigen_sound_r },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress atarisys1_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x080000, 0x087fff, atarigen_slapstic_w, &atarigen_slapstic },
	{ 0x400000, 0x401fff, MWA_BANK1 },
	{ 0x800000, 0x800003, atarisys1_hscroll_w, &atarigen_hscroll },
	{ 0x820000, 0x820003, atarisys1_vscroll_w, &atarigen_vscroll },
	{ 0x840000, 0x840003, MWA_BANK4 },
	{ 0x860000, 0x860003, atarisys1_bankselect_w, &atarisys1_bankselect },
	{ 0x880000, 0x880003, watchdog_reset_w },
	{ 0x8a0000, 0x8a0003, MWA_NOP },		/* VBLANK ack */
	{ 0x8c0000, 0x8c0003, atarigen_eeprom_enable_w },
	{ 0x900000, 0x9fffff, MWA_BANK2 },
	{ 0xa00000, 0xa01fff, atarisys1_playfieldram_w, &atarigen_playfieldram, &atarigen_playfieldram_size },
	{ 0xa02000, 0xa02fff, atarisys1_spriteram_w, &atarigen_spriteram, &atarigen_spriteram_size },
	{ 0xa03000, 0xa03fff, MWA_BANK6, &atarigen_alpharam, &atarigen_alpharam_size },
	{ 0xb00000, 0xb007ff, paletteram_IIIIRRRRGGGGBBBB_word_w, &paletteram },
	{ 0xf00000, 0xf00fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xf40010, 0xf40013, atarisys1_joystick_w },
	{ 0xfe0000, 0xfe0003, atarigen_sound_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress marble_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },
	{ 0x080000, 0x087fff, atarigen_slapstic_r },
	{ 0x2e0000, 0x2e0003, atarisys1_int3state_r },
	{ 0x400014, 0x400017, marble_speedcheck_r },
	{ 0x400000, 0x401fff, MRA_BANK1 },
	{ 0x840000, 0x840003, MRA_BANK4, &atarisys1_prioritycolor },
	{ 0x900000, 0x9fffff, MRA_BANK2 },
	{ 0xa00000, 0xa01fff, atarisys1_playfieldram_r },
	{ 0xa02000, 0xa02fff, MRA_BANK3 },
	{ 0xa03000, 0xa03fff, MRA_BANK6 },
	{ 0xb00000, 0xb007ff, paletteram_word_r },
	{ 0xf00000, 0xf00fff, atarigen_eeprom_r },
	{ 0xf20000, 0xf20007, atarisys1_trakball_r },
	{ 0xf40000, 0xf40013, atarisys1_joystick_r },
	{ 0xf60000, 0xf60003, atarisys1_io_r },
	{ 0xfc0000, 0xfc0003, atarigen_sound_r },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress marble_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x080000, 0x087fff, atarigen_slapstic_w, &atarigen_slapstic },
	{ 0x400014, 0x400017, marble_speedcheck_w, &marble_speedcheck },
	{ 0x400000, 0x401fff, MWA_BANK1 },
	{ 0x800000, 0x800003, atarisys1_hscroll_w, &atarigen_hscroll },
	{ 0x820000, 0x820003, atarisys1_vscroll_w, &atarigen_vscroll },
	{ 0x840000, 0x840003, MWA_BANK4 },
	{ 0x860000, 0x860003, atarisys1_bankselect_w, &atarisys1_bankselect },
	{ 0x880000, 0x880003, watchdog_reset_w },
	{ 0x8a0000, 0x8a0003, MWA_NOP },		/* VBLANK ack */
	{ 0x8c0000, 0x8c0003, atarigen_eeprom_enable_w },
	{ 0x900000, 0x9fffff, MWA_BANK2 },
	{ 0xa00000, 0xa01fff, atarisys1_playfieldram_w, &atarigen_playfieldram, &atarigen_playfieldram_size },
	{ 0xa02000, 0xa02fff, MWA_BANK3, &atarigen_spriteram, &atarigen_spriteram_size },
	{ 0xa03000, 0xa03fff, MWA_BANK6, &atarigen_alpharam, &atarigen_alpharam_size },
	{ 0xb00000, 0xb007ff, paletteram_IIIIRRRRGGGGBBBB_word_w, &paletteram },
	{ 0xf00000, 0xf00fff, atarigen_eeprom_w, &atarigen_eeprom, &atarigen_eeprom_size },
	{ 0xf40010, 0xf40013, atarisys1_joystick_w },
	{ 0xfe0000, 0xfe0003, atarigen_sound_w },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Sound CPU memory handlers
 *
 *************************************/

static struct MemoryReadAddress atarisys1_sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x100f, atarisys1_6522_r },
	{ 0x1800, 0x1801, YM2151_status_port_0_r },
	{ 0x1810, 0x1810, atarigen_6502_sound_r },
	{ 0x1820, 0x1820, atarisys1_6502_switch_r },
	{ 0x1870, 0x187f, pokey1_r },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress atarisys1_sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x100f, atarisys1_6522_w },
	{ 0x1800, 0x1800, YM2151_register_port_0_w },
	{ 0x1801, 0x1801, YM2151_data_port_0_w },
	{ 0x1810, 0x1810, atarigen_6502_sound_w },
	{ 0x1824, 0x1825, atarisys1_led_w },
	{ 0x1820, 0x1827, MWA_NOP },
	{ 0x1870, 0x187f, pokey1_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*************************************
 *
 *		Port definitions
 *
 *************************************/

INPUT_PORTS_START( marble_ports )
	PORT_START      /* IN0 */
    PORT_ANALOGX ( 0xff, 0, IPT_TRACKBALL_X | IPF_REVERSE | IPF_CENTER | IPF_PLAYER1, 100, 0x7f, 0, 0, OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_JOY_LEFT, OSD_JOY_RIGHT, 32 )

	PORT_START      /* IN1 */
    PORT_ANALOGX ( 0xff, 0, IPT_TRACKBALL_Y | IPF_CENTER | IPF_PLAYER1, 100, 0x7f, 0, 0, OSD_KEY_UP, OSD_KEY_DOWN, OSD_JOY_UP, OSD_JOY_DOWN, 32 )

	PORT_START      /* IN2 */
    PORT_ANALOGX ( 0xff, 0, IPT_TRACKBALL_X | IPF_CENTER | IPF_REVERSE | IPF_PLAYER2, 100, 0x7f, 0, 0, OSD_KEY_D, OSD_KEY_G, 0, 0, 32 )

	PORT_START      /* IN3 */
    PORT_ANALOGX ( 0xff, 0, IPT_TRACKBALL_Y | IPF_CENTER | IPF_PLAYER2, 100, 0x7f, 0, 0, OSD_KEY_R, OSD_KEY_F, 0, 0, 32 )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x78, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
INPUT_PORTS_END


INPUT_PORTS_START( peterpak_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x78, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
INPUT_PORTS_END


INPUT_PORTS_START( indytemp_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x0f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x78, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
INPUT_PORTS_END


INPUT_PORTS_START( roadrunn_ports )
	PORT_START	/* IN0 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_X | IPF_REVERSE | IPF_PLAYER1, 100, 0, 0x10, 0xf0 )

	PORT_START	/* IN1 */
	PORT_ANALOG ( 0xff, 0x80, IPT_AD_STICK_Y | IPF_PLAYER1, 100, 0, 0x10, 0xf0 )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x78, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
INPUT_PORTS_END


INPUT_PORTS_START( roadblst_ports )
	PORT_START	/* IN0 */
	PORT_ANALOG ( 0xff, 0x40, IPT_DIAL | IPF_REVERSE, 25, 0, 0x00, 0x7f )

	PORT_START	/* IN1 */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x7f, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN3 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x78, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Self Test", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off")
	PORT_DIPSETTING(    0x00, "On")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
INPUT_PORTS_END



/*************************************
 *
 *		Graphics definitions
 *
 *************************************/

static struct GfxLayout atarisys1_charlayout =
{
	8,8,	/* 8*8 chars */
	512,	/* 512 chars */
	2,		/* 2 bits per pixel */
	{ 0, 4 },
	{ 0, 1, 2, 3, 8, 9, 10, 11 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16	/* every char takes 16 consecutive bytes */
};


static struct GfxLayout atarisys1_objlayout_3bpp =
{
	8,8,	/* 8*8 sprites */
	4096,	/* 4096 of them */
	3,		/* 3 bits per pixel */
	{ 2*8*0x08000, 1*8*0x08000, 0*8*0x08000 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};


static struct GfxLayout atarisys1_objlayout_4bpp =
{
	8,8,	/* 8*8 sprites */
	4096,	/* 4096 of them */
	4,		/* 4 bits per pixel */
	{ 3*8*0x08000, 2*8*0x08000, 1*8*0x08000, 0*8*0x08000 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};


static struct GfxLayout atarisys1_objlayout_5bpp =
{
	8,8,	/* 8*8 sprites */
	4096,	/* 4096 of them */
	5,		/* 5 bits per pixel */
	{ 4*8*0x08000, 3*8*0x08000, 2*8*0x08000, 1*8*0x08000, 0*8*0x08000 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};


static struct GfxLayout atarisys1_objlayout_6bpp =
{
	8,8,	/* 8*8 sprites */
	4096,	/* 4096 of them */
	6,		/* 6 bits per pixel */
	{ 5*8*0x08000, 4*8*0x08000, 3*8*0x08000, 2*8*0x08000, 1*8*0x08000, 0*8*0x08000 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};


static struct GfxDecodeInfo marble_gfxdecodeinfo[] =
{
	{ 1, 0x40000, &atarisys1_charlayout,       0, 64 },
	{ 1, 0x00000, &atarisys1_objlayout_5bpp, 256, 24+1 },
	{ 1, 0x28000, &atarisys1_objlayout_3bpp, 256, 96+1 },
	{ -1 } /* end of array */
};


static struct GfxDecodeInfo peterpak_gfxdecodeinfo[] =
{
	{ 1, 0x60000, &atarisys1_charlayout,       0, 64 },
	{ 1, 0x00000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x20000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x40000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ -1 } /* end of array */
};


static struct GfxDecodeInfo indytemp_gfxdecodeinfo[] =
{
	{ 1, 0x80000, &atarisys1_charlayout,       0, 64 },
	{ 1, 0x00000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x20000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x40000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x60000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ -1 } /* end of array */
};


static struct GfxDecodeInfo roadrunn_gfxdecodeinfo[] =
{
	{ 1, 0xc0000, &atarisys1_charlayout,       0, 64 },
	{ 1, 0x00000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x20000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x40000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x60000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x80000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0xa0000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ -1 } /* end of array */
};


static struct GfxDecodeInfo roadblst_gfxdecodeinfo[] =
{
	{ 1, 0xf0000, &atarisys1_charlayout,       0, 64 },
	{ 1, 0x00000, &atarisys1_objlayout_6bpp, 256, 12+1 },
	{ 1, 0x00000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x30000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x50000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x70000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0x90000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0xb0000, &atarisys1_objlayout_4bpp, 256, 48+1 },
	{ 1, 0xd0000, &atarisys1_objlayout_4bpp, 256, 48+1 },
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
	1789790,	/* 1.78 MHz */
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
	650826, /*640000,     * clock speed (80*samplerate) */
	100,        /* volume */
	0 /* irq handler */
};


static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579580,	/* 3.58 MHZ ? */
	{ 40 },
	{ atarisys1_sound_interrupt }
};



/*************************************
 *
 *		Machine driver
 *
 *************************************/

static struct MachineDriver marble_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,		/* 7.159 Mhz */
			0,
			marble_readmem,marble_writemem,0,0,
			atarisys1_interrupt,1
		},
		{
			CPU_M6502,
			1789790,		/* 1.791 Mhz */
			2,
			atarisys1_sound_readmem,atarisys1_sound_writemem,0,0,
			ignore_interrupt,1	/* IRQ generated by the YM2151 */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	marble_init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	marble_gfxdecodeinfo,
	1024+64,1024+64,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	marble_vh_start,
	atarisys1_vh_stop,
	atarisys1_vh_screenrefresh,

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


static struct MachineDriver peterpak_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,		/* 7.159 Mhz */
			0,
			atarisys1_readmem,atarisys1_writemem,0,0,
			atarisys1_interrupt,1
		},
		{
			CPU_M6502,
			1789790,		/* 1.791 Mhz */
			2,
			atarisys1_sound_readmem,atarisys1_sound_writemem,0,0,
			ignore_interrupt,1	/* IRQ generated by the YM2151 */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	peterpak_init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	peterpak_gfxdecodeinfo,
	1024+64,1024+64,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	peterpak_vh_start,
	atarisys1_vh_stop,
	atarisys1_vh_screenrefresh,

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


static struct MachineDriver indytemp_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,		/* 7.159 Mhz */
			0,
			atarisys1_readmem,atarisys1_writemem,0,0,
			atarisys1_interrupt,1
		},
		{
			CPU_M6502,
			1789790,		/* 1.791 Mhz */
			2,
			atarisys1_sound_readmem,atarisys1_sound_writemem,0,0,
			ignore_interrupt,1	/* IRQ generated by the YM2151 */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	indytemp_init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	indytemp_gfxdecodeinfo,
	1024+64,1024+64,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	indytemp_vh_start,
	atarisys1_vh_stop,
	atarisys1_vh_screenrefresh,

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


static struct MachineDriver roadrunn_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,		/* 7.159 Mhz */
			0,
			atarisys1_readmem,atarisys1_writemem,0,0,
			atarisys1_interrupt,1
		},
		{
			CPU_M6502,
			1789790,		/* 1.791 Mhz */
			2,
			atarisys1_sound_readmem,atarisys1_sound_writemem,0,0,
			ignore_interrupt,1	/* IRQ generated by the YM2151 */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	roadrunn_init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	roadrunn_gfxdecodeinfo,
	1024+64,1024+64,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	roadrunn_vh_start,
	atarisys1_vh_stop,
	atarisys1_vh_screenrefresh,

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


static struct MachineDriver roadblst_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			7159160,		/* 7.159 Mhz */
			0,
			atarisys1_readmem,atarisys1_writemem,0,0,
			atarisys1_interrupt,1
		},
		{
			CPU_M6502,
			1789790,		/* 1.791 Mhz */
			2,
			atarisys1_sound_readmem,atarisys1_sound_writemem,0,0,
			ignore_interrupt,1	/* IRQ generated by the YM2151 */
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	10,
	roadblst_init_machine,

	/* video hardware */
	42*8, 30*8, { 0*8, 42*8-1, 0*8, 30*8-1 },
	roadblst_gfxdecodeinfo,
	1024+64,1024+64,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	roadblst_vh_start,
	atarisys1_vh_stop,
	roadblst_vh_screenrefresh,

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
 *		ROM decoders
 *
 *************************************/

static void marble_rom_decode (void)
{
	int i;

	/* invert the graphics bits on the playfield and motion objects */
	for (i = 0; i < 0x40000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}


static void peterpak_rom_decode (void)
{
	int i;

	/* invert the graphics bits on the playfield and motion objects */
	for (i = 0; i < 0x60000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}


static void indytemp_rom_decode (void)
{
	int i;

	/* invert the graphics bits on the playfield and motion objects */
	for (i = 0; i < 0x80000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}


static void roadrunn_rom_decode (void)
{
	int i;

	/* invert the graphics bits on the playfield and motion objects */
	for (i = 0; i < 0xc0000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}


static void roadblst_rom_decode (void)
{
	int i;

	/* ROMs 39+40 load the lower half at 10000 and the upper half at 50000 */
	/* ROMs 55+56 load the lower half at 20000 and the upper half at 60000 */
	/* However, we load 39+40 into 10000 and 20000, and 55+56 into 50000+60000 */
	/* We need to swap the memory at 20000 and 50000 */
	for (i = 0; i < 0x10000; i++)
	{
		int temp = Machine->memory_region[0][0x20000 + i];
		Machine->memory_region[0][0x20000 + i] = Machine->memory_region[0][0x50000 + i];
		Machine->memory_region[0][0x50000 + i] = temp;
	}

	/* invert the graphics bits on the playfield and motion objects */
	for (i = 0; i < 0xf0000; i++)
		Machine->memory_region[1][i] ^= 0xff;
}



/*************************************
 *
 *		ROM definition(s)
 *
 *************************************/

ROM_START( marble_rom )
	ROM_REGION(0x88000)	/* 8.5*64k for 68000 code & slapstic ROM */
	ROM_LOAD_EVEN( "136032.205",   0x00000, 0x04000, 0x88d0be26 )
	ROM_LOAD_ODD ( "136032.206",   0x00000, 0x04000, 0x3c79ef05 )
	ROM_LOAD_EVEN( "136033.401",   0x10000, 0x08000, 0xecfc25a2 )
	ROM_LOAD_ODD ( "136033.402",   0x10000, 0x08000, 0x7ce9bf53 )
	ROM_LOAD_EVEN( "136033.403",   0x20000, 0x08000, 0xdafee7a2 )
	ROM_LOAD_ODD ( "136033.404",   0x20000, 0x08000, 0xb59ffcf6 )
	ROM_LOAD_EVEN( "136033.107",   0x80000, 0x04000, 0xf3b8745b )
	ROM_LOAD_ODD ( "136033.108",   0x80000, 0x04000, 0xe51eecaa )

	ROM_REGION_DISPOSE(0x42000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136033.137",   0x00000, 0x04000, 0x7a45f5c1 )  /* bank 0 (5 bpp)*/
	ROM_LOAD( "136033.138",   0x04000, 0x04000, 0x7e954a88 )
	ROM_LOAD( "136033.139",   0x08000, 0x04000, 0x1eb1bb5f )
	ROM_LOAD( "136033.140",   0x0c000, 0x04000, 0x8a82467b )
	ROM_LOAD( "136033.141",   0x10000, 0x04000, 0x52448965 )
	ROM_LOAD( "136033.142",   0x14000, 0x04000, 0xb4a70e4f )
	ROM_LOAD( "136033.143",   0x18000, 0x04000, 0x7156e449 )
	ROM_LOAD( "136033.144",   0x1c000, 0x04000, 0x4c3e4c79 )
	ROM_LOAD( "136033.145",   0x20000, 0x04000, 0x9062be7f )
	ROM_LOAD( "136033.146",   0x24000, 0x04000, 0x14566dca )
	ROM_LOAD( "136033.149",   0x2c000, 0x04000, 0xb6658f06 )  /* bank 1 (3 bpp) */
	ROM_LOAD( "136033.151",   0x34000, 0x04000, 0x84ee1c80 )
	ROM_LOAD( "136033.153",   0x3c000, 0x04000, 0xdaa02926 )
	ROM_LOAD( "136032.107",   0x40000, 0x02000, 0x7a29dc07 )  /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "136033.421",   0x8000, 0x4000, 0x78153dc3 )
	ROM_LOAD( "136033.422",   0xc000, 0x4000, 0x2e66300e )
ROM_END


ROM_START( marble2_rom )
	ROM_REGION(0x88000)	/* 8.5*64k for 68000 code & slapstic ROM */
	ROM_LOAD_EVEN( "136032.205",   0x00000, 0x04000, 0x88d0be26 )
	ROM_LOAD_ODD ( "136032.206",   0x00000, 0x04000, 0x3c79ef05 )
	ROM_LOAD_EVEN( "136033.201",   0x10000, 0x08000, 0x9395804d )
	ROM_LOAD_ODD ( "136033.202",   0x10000, 0x08000, 0xedd313f5 )
	ROM_LOAD_EVEN( "136033.203",   0x20000, 0x08000, 0xdafee7a2 )
	ROM_LOAD_ODD ( "136033.204",   0x20000, 0x08000, 0x4d621731 )
	ROM_LOAD_EVEN( "136033.107",   0x80000, 0x04000, 0xf3b8745b )
	ROM_LOAD_ODD ( "136033.108",   0x80000, 0x04000, 0xe51eecaa )

	ROM_REGION_DISPOSE(0x42000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136033.137",   0x00000, 0x04000, 0x7a45f5c1 )  /* bank 0 (5 bpp)*/
	ROM_LOAD( "136033.138",   0x04000, 0x04000, 0x7e954a88 )
	ROM_LOAD( "136033.139",   0x08000, 0x04000, 0x1eb1bb5f )
	ROM_LOAD( "136033.140",   0x0c000, 0x04000, 0x8a82467b )
	ROM_LOAD( "136033.141",   0x10000, 0x04000, 0x52448965 )
	ROM_LOAD( "136033.142",   0x14000, 0x04000, 0xb4a70e4f )
	ROM_LOAD( "136033.143",   0x18000, 0x04000, 0x7156e449 )
	ROM_LOAD( "136033.144",   0x1c000, 0x04000, 0x4c3e4c79 )
	ROM_LOAD( "136033.145",   0x20000, 0x04000, 0x9062be7f )
	ROM_LOAD( "136033.146",   0x24000, 0x04000, 0x14566dca )
	ROM_LOAD( "136033.149",   0x2c000, 0x04000, 0xb6658f06 )  /* bank 1 (3 bpp) */
	ROM_LOAD( "136033.151",   0x34000, 0x04000, 0x84ee1c80 )
	ROM_LOAD( "136033.153",   0x3c000, 0x04000, 0xdaa02926 )
	ROM_LOAD( "136032.107",   0x40000, 0x02000, 0x7a29dc07 )  /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "136033.121",   0x8000, 0x4000, 0x73fe2b46 )
	ROM_LOAD( "136033.122",   0xc000, 0x4000, 0x03bf65c3 )
ROM_END


ROM_START( marblea_rom )
	ROM_REGION(0x88000)	/* 8.5*64k for 68000 code & slapstic ROM */
	ROM_LOAD_EVEN( "136032.205",   0x00000, 0x04000, 0x88d0be26 )
	ROM_LOAD_ODD ( "136032.206",   0x00000, 0x04000, 0x3c79ef05 )
	ROM_LOAD_EVEN( "136033.323",   0x10000, 0x04000, 0x4dc2987a )
	ROM_LOAD_ODD ( "136033.324",   0x10000, 0x04000, 0xe22e6e11 )
	ROM_LOAD_EVEN( "136033.225",   0x18000, 0x04000, 0x743f6c5c )
	ROM_LOAD_ODD ( "136033.226",   0x18000, 0x04000, 0xaeb711e3 )
	ROM_LOAD_EVEN( "136033.227",   0x20000, 0x04000, 0xd06d2c22 )
	ROM_LOAD_ODD ( "136033.228",   0x20000, 0x04000, 0xe69cec16 )
	ROM_LOAD_EVEN( "136033.229",   0x28000, 0x04000, 0xc81d5c14 )
	ROM_LOAD_ODD ( "136033.230",   0x28000, 0x04000, 0x526ce8ad )
	ROM_LOAD_EVEN( "136033.107",   0x80000, 0x04000, 0xf3b8745b )
	ROM_LOAD_ODD ( "136033.108",   0x80000, 0x04000, 0xe51eecaa )

	ROM_REGION_DISPOSE(0x42000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136033.137",   0x00000, 0x04000, 0x7a45f5c1 )  /* bank 0 (5 bpp)*/
	ROM_LOAD( "136033.138",   0x04000, 0x04000, 0x7e954a88 )
	ROM_LOAD( "136033.139",   0x08000, 0x04000, 0x1eb1bb5f )
	ROM_LOAD( "136033.140",   0x0c000, 0x04000, 0x8a82467b )
	ROM_LOAD( "136033.141",   0x10000, 0x04000, 0x52448965 )
	ROM_LOAD( "136033.142",   0x14000, 0x04000, 0xb4a70e4f )
	ROM_LOAD( "136033.143",   0x18000, 0x04000, 0x7156e449 )
	ROM_LOAD( "136033.144",   0x1c000, 0x04000, 0x4c3e4c79 )
	ROM_LOAD( "136033.145",   0x20000, 0x04000, 0x9062be7f )
	ROM_LOAD( "136033.146",   0x24000, 0x04000, 0x14566dca )
	ROM_LOAD( "136033.149",   0x2c000, 0x04000, 0xb6658f06 )  /* bank 1 (3 bpp) */
	ROM_LOAD( "136033.151",   0x34000, 0x04000, 0x84ee1c80 )
	ROM_LOAD( "136033.153",   0x3c000, 0x04000, 0xdaa02926 )
	ROM_LOAD( "136032.107",   0x40000, 0x02000, 0x7a29dc07 )  /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "136033.257",   0x8000, 0x4000, 0x2e2e0df8 )
	ROM_LOAD( "136033.258",   0xc000, 0x4000, 0x1b9655cd )
ROM_END


ROM_START( peterpak_rom )
	ROM_REGION(0x88000)	/* 8.5*64k for 68000 code & slapstic ROM */
	ROM_LOAD_EVEN( "136032.205",   0x00000, 0x04000, 0x88d0be26 )
	ROM_LOAD_ODD ( "136032.206",   0x00000, 0x04000, 0x3c79ef05 )
	ROM_LOAD_EVEN( "136028.142",   0x10000, 0x04000, 0x4f9fc020 )
	ROM_LOAD_ODD ( "136028.143",   0x10000, 0x04000, 0x9fb257cc )
	ROM_LOAD_EVEN( "136028.144",   0x18000, 0x04000, 0x50267619 )
	ROM_LOAD_ODD ( "136028.145",   0x18000, 0x04000, 0x7b6a5004 )
	ROM_LOAD_EVEN( "136028.146",   0x20000, 0x04000, 0x4183a67a )
	ROM_LOAD_ODD ( "136028.147",   0x20000, 0x04000, 0x14e2d97b )
	ROM_LOAD_EVEN( "136028.148",   0x80000, 0x04000, 0x230e8ba9 )
	ROM_LOAD_ODD ( "136028.149",   0x80000, 0x04000, 0x0ff0c13a )

	ROM_REGION_DISPOSE(0x62000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136028.138",   0x00000, 0x08000, 0x53eaa018 )  /* bank 0 (4 bpp) */
	ROM_LOAD( "136028.139",   0x08000, 0x08000, 0x354a19cb )
	ROM_LOAD( "136028.140",   0x10000, 0x08000, 0x8d2c4717 )
	ROM_LOAD( "136028.141",   0x18000, 0x08000, 0xbf59ea19 )
	ROM_LOAD( "136028.150",   0x20000, 0x08000, 0x83362483 )  /* bank 1 (4 bpp) */
	ROM_LOAD( "136028.151",   0x28000, 0x08000, 0x6e95094e )
	ROM_LOAD( "136028.152",   0x30000, 0x08000, 0x9553f084 )
	ROM_LOAD( "136028.153",   0x38000, 0x08000, 0xc2a9b028 )
	ROM_LOAD( "136028.105",   0x44000, 0x04000, 0xac9a5a44 )  /* bank 2 (4 bpp) */
	ROM_LOAD( "136028.108",   0x4c000, 0x04000, 0x51941e64 )
	ROM_LOAD( "136028.111",   0x54000, 0x04000, 0x246599f3 )
	ROM_LOAD( "136028.114",   0x5c000, 0x04000, 0x918a5082 )
	ROM_LOAD( "136032.107",   0x60000, 0x02000, 0x7a29dc07 )  /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "136028.101",   0x8000, 0x4000, 0xff712aa2 )
	ROM_LOAD( "136028.102",   0xc000, 0x4000, 0x89ea21a1 )
ROM_END


ROM_START( indytemp_rom )
	ROM_REGION(0x88000)	/* 8.5*64k for 68000 code & slapstic ROM */
	ROM_LOAD_EVEN( "136032.205",   0x00000, 0x04000, 0x88d0be26 )
	ROM_LOAD_ODD ( "136032.206",   0x00000, 0x04000, 0x3c79ef05 )
	ROM_LOAD_EVEN( "136036.432",   0x10000, 0x08000, 0xd888cdf1 )
	ROM_LOAD_ODD ( "136036.431",   0x10000, 0x08000, 0xb7ac7431 )
	ROM_LOAD_EVEN( "136036.434",   0x20000, 0x08000, 0x802495fd )
	ROM_LOAD_ODD ( "136036.433",   0x20000, 0x08000, 0x3a914e5c )
	ROM_LOAD_EVEN( "136036.456",   0x30000, 0x04000, 0xec146b09 )
	ROM_LOAD_ODD ( "136036.457",   0x30000, 0x04000, 0x6628de01 )
	ROM_LOAD_EVEN( "136036.358",   0x80000, 0x04000, 0xd9351106 )
	ROM_LOAD_ODD ( "136036.359",   0x80000, 0x04000, 0xe731caea )

	ROM_REGION_DISPOSE(0x82000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136036.135",   0x00000, 0x08000, 0x07593e5e )
	ROM_LOAD( "136036.139",   0x08000, 0x08000, 0xbcb3b517 )
	ROM_LOAD( "136036.143",   0x10000, 0x08000, 0x4ca6704c )
	ROM_LOAD( "136036.147",   0x18000, 0x08000, 0x12b2673e )
	ROM_LOAD( "136036.136",   0x20000, 0x08000, 0xb2732404 )
	ROM_LOAD( "136036.140",   0x28000, 0x08000, 0x9b00f1a2 )
	ROM_LOAD( "136036.144",   0x30000, 0x08000, 0xb3d095dc )
	ROM_LOAD( "136036.148",   0x38000, 0x08000, 0x3980416e )
	ROM_LOAD( "136036.137",   0x40000, 0x08000, 0xd3e2b325 )
	ROM_LOAD( "136036.141",   0x48000, 0x08000, 0x83166acb )
	ROM_LOAD( "136036.145",   0x50000, 0x08000, 0x7897cde6 )
	ROM_LOAD( "136036.149",   0x58000, 0x08000, 0x81503a23 )
	ROM_LOAD( "136036.138",   0x60000, 0x08000, 0x00a2c186 )
	ROM_LOAD( "136036.142",   0x68000, 0x08000, 0x7faae75f )
	ROM_LOAD( "136036.146",   0x70000, 0x08000, 0x96f69b50 )
	ROM_LOAD( "136036.150",   0x78000, 0x08000, 0x62a4a20e )
	ROM_LOAD( "136032.107",   0x80000, 0x02000, 0x7a29dc07 )        /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "136036.153",   0x4000, 0x4000, 0x95294641 )
	ROM_LOAD( "136036.154",   0x8000, 0x4000, 0xcbfc6adb )
	ROM_LOAD( "136036.155",   0xc000, 0x4000, 0x4c8233ac )
ROM_END


ROM_START( roadrunn_rom )
	ROM_REGION(0x88000)	/* 8.5*64k for 68000 code & slapstic ROM */
	ROM_LOAD_EVEN( "136032.205",   0x00000, 0x04000, 0x88d0be26 )
	ROM_LOAD_ODD ( "136032.206",   0x00000, 0x04000, 0x3c79ef05 )
	ROM_LOAD_EVEN( "136040.228",   0x10000, 0x08000, 0xb66c629a )
	ROM_LOAD_ODD ( "136040.229",   0x10000, 0x08000, 0x5638959f )
	ROM_LOAD_EVEN( "136040.230",   0x20000, 0x08000, 0xcd7956a3 )

	ROM_LOAD_ODD ( "136040.231",   0x20000, 0x08000, 0x722f2d3b )
	ROM_LOAD_EVEN( "136040.134",   0x50000, 0x08000, 0x18f431fe )
	ROM_LOAD_ODD ( "136040.135",   0x50000, 0x08000, 0xcb06f9ab )
	ROM_LOAD_EVEN( "136040.136",   0x60000, 0x08000, 0x8050bce4 )
	ROM_LOAD_ODD ( "136040.137",   0x60000, 0x08000, 0x3372a5cf )
	ROM_LOAD_EVEN( "136040.138",   0x70000, 0x08000, 0xa83155ee )
	ROM_LOAD_ODD ( "136040.139",   0x70000, 0x08000, 0x23aead1c )
	ROM_LOAD_EVEN( "136040.140",   0x80000, 0x04000, 0xd1464c88 )
	ROM_LOAD_ODD ( "136040.141",   0x80000, 0x04000, 0xf8f2acdf )

	ROM_REGION_DISPOSE(0xc2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "136040.101",   0x00000, 0x08000, 0x0b094fbc )
	ROM_LOAD( "136040.107",   0x08000, 0x08000, 0x4f908670 )
	ROM_LOAD( "136040.113",   0x10000, 0x08000, 0xfd5a0f76 )
	ROM_LOAD( "136040.119",   0x18000, 0x08000, 0xce71348e )
	ROM_LOAD( "136040.102",   0x20000, 0x08000, 0x1779c3ac )
	ROM_LOAD( "136040.108",   0x28000, 0x08000, 0x6d05127f )
	ROM_LOAD( "136040.114",   0x30000, 0x08000, 0x8220a1aa )
	ROM_LOAD( "136040.120",   0x38000, 0x08000, 0x9161a73e )
	ROM_LOAD( "136040.103",   0x40000, 0x08000, 0xdb0519b7 )
	ROM_LOAD( "136040.109",   0x48000, 0x08000, 0x700a49ef )
	ROM_LOAD( "136040.115",   0x50000, 0x08000, 0xd12f922d )
	ROM_LOAD( "136040.121",   0x58000, 0x08000, 0xc946c1f2 )
	ROM_LOAD( "136040.104",   0x60000, 0x08000, 0x248e3218 )
	ROM_LOAD( "136040.110",   0x68000, 0x08000, 0x04d87a0e )
	ROM_LOAD( "136040.116",   0x70000, 0x08000, 0x52b23a36 )
	ROM_LOAD( "136040.122",   0x78000, 0x08000, 0x5e482c43 )
	ROM_LOAD( "136040.105",   0x80000, 0x08000, 0xbd95d695 )
	ROM_LOAD( "136040.111",   0x88000, 0x08000, 0xb2ca7e1f )
	ROM_LOAD( "136040.117",   0x90000, 0x08000, 0xdf1a558a )
	ROM_LOAD( "136040.123",   0x98000, 0x08000, 0xf3dabfb0 )
	ROM_LOAD( "136040.106",   0xa0000, 0x08000, 0xa1736550 )
	ROM_LOAD( "136040.112",   0xa8000, 0x08000, 0x8556ae56 )
	ROM_LOAD( "136040.118",   0xb0000, 0x08000, 0xc84b42a5 )
	ROM_LOAD( "136040.124",   0xb8000, 0x08000, 0x3d38d30c )
	ROM_LOAD( "136032.107",   0xc0000, 0x02000, 0x7a29dc07 )        /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "136040.143",   0x8000, 0x4000, 0x62b9878e )
	ROM_LOAD( "136040.144",   0xc000, 0x4000, 0x6ef1b804 )
ROM_END


ROM_START( roadblst_rom )
	ROM_REGION(0x88000)	/* 8.5*64k for 68000 code & slapstic ROM */
	ROM_LOAD_EVEN( "136032.205",   0x00000, 0x04000, 0x88d0be26 )
	ROM_LOAD_ODD ( "136032.206",   0x00000, 0x04000, 0x3c79ef05 )
	ROM_LOAD_EVEN( "048-1139.rom", 0x10000, 0x10000, 0xb73c1bd5 )
	ROM_LOAD_ODD ( "048-1140.rom", 0x10000, 0x10000, 0x6305429b )
	ROM_LOAD_EVEN( "048-1155.rom", 0x50000, 0x10000, 0xe95fc7d2 )
	ROM_LOAD_ODD ( "048-1156.rom", 0x50000, 0x10000, 0x727510f9 )
	ROM_LOAD_EVEN( "048-1167.rom", 0x70000, 0x08000, 0xc6d30d6f )
	ROM_LOAD_ODD ( "048-1168.rom", 0x70000, 0x08000, 0x16951020 )
	ROM_LOAD_EVEN( "048-2147.rom", 0x80000, 0x04000, 0x5c1adf67 )
	ROM_LOAD_ODD ( "048-2148.rom", 0x80000, 0x04000, 0xd9ac8966 )

	ROM_REGION_DISPOSE(0xf2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "048-1101.rom", 0x00000, 0x08000, 0xfe342d27 )
	ROM_LOAD( "048-1102.rom", 0x08000, 0x08000, 0x17c7e780 )
	ROM_LOAD( "048-1103.rom", 0x10000, 0x08000, 0x39688e01 )
	ROM_LOAD( "048-1104.rom", 0x18000, 0x08000, 0xc8f9bd8e )
	ROM_LOAD( "048-1105.rom", 0x20000, 0x08000, 0xc69e439e )
	ROM_LOAD( "048-1106.rom", 0x28000, 0x08000, 0x4ee55796 )
	ROM_LOAD( "048-1107.rom", 0x30000, 0x08000, 0x02117c58 )
	ROM_CONTINUE(             0x50000, 0x08000 )
	ROM_LOAD( "048-1108.rom", 0x38000, 0x08000, 0x1e148525 )
	ROM_CONTINUE(             0x58000, 0x08000 )
	ROM_LOAD( "048-1109.rom", 0x40000, 0x08000, 0x110ce07e )
	ROM_CONTINUE(             0x60000, 0x08000 )
	ROM_LOAD( "048-1110.rom", 0x48000, 0x08000, 0xc00aa0f4 )
	ROM_CONTINUE(             0x68000, 0x08000 )
	ROM_LOAD( "048-1111.rom", 0x70000, 0x08000, 0xc951d014 )
	ROM_CONTINUE(             0x90000, 0x08000 )
	ROM_LOAD( "048-1112.rom", 0x78000, 0x08000, 0x95c5a006 )
	ROM_CONTINUE(             0x98000, 0x08000 )
	ROM_LOAD( "048-1113.rom", 0x80000, 0x08000, 0xf61f2370 )
	ROM_CONTINUE(             0xa0000, 0x08000 )
	ROM_LOAD( "048-1114.rom", 0x88000, 0x08000, 0x774a36a8 )
	ROM_CONTINUE(             0xa8000, 0x08000 )
	ROM_LOAD( "048-1115.rom", 0xb0000, 0x08000, 0xa47bc79d )
	ROM_CONTINUE(             0xd0000, 0x08000 )
	ROM_LOAD( "048-1116.rom", 0xb8000, 0x08000, 0xb8a5c215 )
	ROM_CONTINUE(             0xd8000, 0x08000 )
	ROM_LOAD( "048-1117.rom", 0xc0000, 0x08000, 0x2d1c1f64 )
	ROM_CONTINUE(             0xe0000, 0x08000 )
	ROM_LOAD( "048-1118.rom", 0xc8000, 0x08000, 0xbe879b8e )
	ROM_CONTINUE(             0xe8000, 0x08000 )
	ROM_LOAD( "136032.107",   0xf0000, 0x02000, 0x7a29dc07 )        /* alpha font */

	ROM_REGION(0x10000)	/* 64k for 6502 code */
	ROM_LOAD( "048-1149.rom", 0x4000, 0x4000, 0x2e54f95e )
	ROM_LOAD( "048-1169.rom", 0x8000, 0x4000, 0xee318052 )
	ROM_LOAD( "048-1170.rom", 0xc000, 0x4000, 0x75dfec33 )
ROM_END



/*************************************
 *
 *		Game driver(s)
 *
 *************************************/

struct GameDriver marble_driver =
{
	__FILE__,
	0,
	"marble",
	"Marble Madness (set 1)",
	"1984",
	"Atari Games",
	"Aaron Giles (MAME driver)\nFrank Palazzolo (Slapstic decoding)\nTim Lindquist (Hardware Info)",
	0,
	&marble_machine_driver,
	0,

	marble_rom,
	marble_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	marble_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver marble2_driver =
{
	__FILE__,
	&marble_driver,
	"marble2",
	"Marble Madness (set 2)",
	"1984",
	"Atari Games",
	"Aaron Giles (MAME driver)\nFrank Palazzolo (Slapstic decoding)\nTim Lindquist (Hardware Info)",
	0,
	&marble_machine_driver,
	0,

	marble2_rom,
	marble_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	marble_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver marblea_driver =
{
	__FILE__,
	&marble_driver,
	"marblea",
	"Marble Madness (set 3)",
	"1984",
	"Atari Games",
	"Aaron Giles (MAME driver)\nFrank Palazzolo (Slapstic decoding)\nTim Lindquist (Hardware Info)",
	0,
	&marble_machine_driver,
	0,

	marblea_rom,
	marble_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	marble_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver peterpak_driver =
{
	__FILE__,
	0,
	"peterpak",
	"Peter Pack-Rat",
	"1984",
	"Atari Games",
	"Aaron Giles (MAME driver)\nFrank Palazzolo (Slapstic decoding)\nTim Lindquist (Hardware Info)",
	0,
	&peterpak_machine_driver,
	0,

	peterpak_rom,
	peterpak_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	peterpak_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver indytemp_driver =
{
	__FILE__,
	0,
	"indytemp",
	"Indiana Jones and the Temple of Doom",
	"1985",
	"Atari Games",
	"Aaron Giles (MAME driver)\nFrank Palazzolo (Slapstic decoding)\nTim Lindquist (Hardware Info)",
	0,
	&indytemp_machine_driver,
	0,

	indytemp_rom,
	indytemp_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	indytemp_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver roadrunn_driver =
{
	__FILE__,
	0,
	"roadrunn",
	"Road Runner",
	"1985",
	"Atari Games",
	"Aaron Giles (MAME driver)\nFrank Palazzolo (Slapstic decoding)\nTim Lindquist (Hardware Info)",
	0,
	&roadrunn_machine_driver,
	0,

	roadrunn_rom,
	roadrunn_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	roadrunn_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};


struct GameDriver roadblst_driver =
{
	__FILE__,
	0,
	"roadblst",
	"Road Blasters",
	"1987",
	"Atari Games",
	"Aaron Giles (MAME driver)\nFrank Palazzolo (Slapstic decoding)\nTim Lindquist (Hardware Info)",
	0,
	&roadblst_machine_driver,
	0,

	roadblst_rom,
	roadblst_rom_decode,
	0,
	0,
	0,	/* sound_prom */

	roadblst_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	atarigen_hiload, atarigen_hisave
};
