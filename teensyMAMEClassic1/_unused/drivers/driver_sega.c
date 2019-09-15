/***************************************************************************

Sega Vector memory map (preliminary)

Most of the info here comes from the wiretap archive at:
http://www.spies.com/arcade/simulation/gameHardware/

 * Sega G80 Vector Simulation

ROM Address Map
---------------
       Eliminator Elim4Player Space Fury  Zektor  TAC/SCAN  Star Trk
-----+-----------+-----------+-----------+-------+---------+---------+
0000 | 969       | 1390      | 969       | 1611  | 1711    | 1873    | CPU u25
-----+-----------+-----------+-----------+-------+---------+---------+
0800 | 1333      | 1347      | 960       | 1586  | 1670    | 1848    | ROM u1
-----+-----------+-----------+-----------+-------+---------+---------+
1000 | 1334      | 1348      | 961       | 1587  | 1671    | 1849    | ROM u2
-----+-----------+-----------+-----------+-------+---------+---------+
1800 | 1335      | 1349      | 962       | 1588  | 1672    | 1850    | ROM u3
-----+-----------+-----------+-----------+-------+---------+---------+
2000 | 1336      | 1350      | 963       | 1589  | 1673    | 1851    | ROM u4
-----+-----------+-----------+-----------+-------+---------+---------+
2800 | 1337      | 1351      | 964       | 1590  | 1674    | 1852    | ROM u5
-----+-----------+-----------+-----------+-------+---------+---------+
3000 | 1338      | 1352      | 965       | 1591  | 1675    | 1853    | ROM u6
-----+-----------+-----------+-----------+-------+---------+---------+
3800 | 1339      | 1353      | 966       | 1592  | 1676    | 1854    | ROM u7
-----+-----------+-----------+-----------+-------+---------+---------+
4000 | 1340      | 1354      | 967       | 1593  | 1677    | 1855    | ROM u8
-----+-----------+-----------+-----------+-------+---------+---------+
4800 | 1341      | 1355      | 968       | 1594  | 1678    | 1856    | ROM u9
-----+-----------+-----------+-----------+-------+---------+---------+
5000 | 1342      | 1356      |           | 1595  | 1679    | 1857    | ROM u10
-----+-----------+-----------+-----------+-------+---------+---------+
5800 | 1343      | 1357      |           | 1596  | 1680    | 1858    | ROM u11
-----+-----------+-----------+-----------+-------+---------+---------+
6000 | 1344      | 1358      |           | 1597  | 1681    | 1859    | ROM u12
-----+-----------+-----------+-----------+-------+---------+---------+
6800 | 1345      | 1359      |           | 1598  | 1682    | 1860    | ROM u13
-----+-----------+-----------+-----------+-------+---------+---------+
7000 |           | 1360      |           | 1599  | 1683    | 1861    | ROM u14
-----+-----------+-----------+-----------+-------+---------+---------+
7800 |                                   | 1600  | 1684    | 1862    | ROM u15
-----+-----------+-----------+-----------+-------+---------+---------+
8000 |                                   | 1601  | 1685    | 1863    | ROM u16
-----+-----------+-----------+-----------+-------+---------+---------+
8800 |                                   | 1602  | 1686    | 1864    | ROM u17
-----+-----------+-----------+-----------+-------+---------+---------+
9000 |                                   | 1603  | 1687    | 1865    | ROM u18
-----+-----------+-----------+-----------+-------+---------+---------+
9800 |                                   | 1604  | 1688    | 1866    | ROM u19
-----+-----------+-----------+-----------+-------+---------+---------+
A000 |                                   | 1605  | 1709    | 1867    | ROM u20
-----+-----------+-----------+-----------+-------+---------+---------+
A800 |                                   | 1606  | 1710    | 1868    | ROM u21
-----+-----------+-----------+-----------+-------+---------+---------+
B000 |                                                     | 1869    | ROM u22
-----+-----------+-----------+-----------+-------+---------+---------+
B800 |                                                     | 1870    | ROM u23
-----+-----------+-----------+-----------+-------+---------+---------+

I/O ports:
read:

write:

These games all have dipswitches, but they are mapped in such a way as to make
using them with MAME extremely difficult. I might try to implement them in the
future.

SWITCH MAPPINGS
---------------

+------+------+------+------+------+------+------+------+
|SW1-8 |SW1-7 |SW1-6 |SW1-5 |SW1-4 |SW1-3 |SW1-2 |SW1-1 |
+------+------+------+------+------+------+------+------+
 F8:08 |F9:08 |FA:08 |FB:08 |F8:04 |F9:04  FA:04  FB:04    Zektor &
       |      |      |      |      |      |                Space Fury
       |      |      |      |      |      |
   1  -|------|------|------|------|------|--------------- upright
   0  -|------|------|------|------|------|--------------- cocktail
       |      |      |      |      |      |
       |  1  -|------|------|------|------|--------------- voice
       |  0  -|------|------|------|------|--------------- no voice
              |      |      |      |      |
              |  1   |  1  -|------|------|--------------- 5 ships
              |  0   |  1  -|------|------|--------------- 4 ships
              |  1   |  0  -|------|------|--------------- 3 ships
              |  0   |  0  -|------|------|--------------- 2 ships
                            |      |      |
                            |  1   |  1  -|--------------- hardest
                            |  0   |  1  -|--------------- hard
1 = Open                    |  1   |  0  -|--------------- medium
0 = Closed                  |  0   |  0  -|--------------- easy

+------+------+------+------+------+------+------+------+
|SW2-8 |SW2-7 |SW2-6 |SW2-5 |SW2-4 |SW2-3 |SW2-2 |SW2-1 |
+------+------+------+------+------+------+------+------+
|F8:02 |F9:02 |FA:02 |FB:02 |F8:01 |F9:01 |FA:01 |FB:01 |
|      |      |      |      |      |      |      |      |
|  1   |  1   |  0   |  0   |  1   | 1    | 0    |  0   | 1 coin/ 1 play
+------+------+------+------+------+------+------+------+

Known problems:

1 The games seem to run too fast. This is most noticable
  with the speech samples in Zektor - they don't match the mouth.
  Slowing down the Z80 doesn't help and in fact hurts performance.

2 Cocktail mode isn't implemented.

Is 1) still valid?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"

extern unsigned char *sega_mem;
extern void sega_security(int chip);
extern void sega_wr(int offset, int data);

int sega_read_ports (int offset);
int sega_IN4_r (int offset);
int elim4_IN4_r (int offset);

int sega_interrupt(void);
int sega_mult_r (int offset);
void sega_mult1_w (int offset, int data);
void sega_mult2_w (int offset, int data);
void sega_switch_w (int offset, int data);

/* Sound hardware prototypes */
int sega_sh_start (void);
int sega_sh_r (int offset);
void sega_sh_speech_w (int offset, int data);
void sega_sh_update(void);

void elim1_sh_w (int offset, int data);
void elim2_sh_w (int offset, int data);
void spacfury1_sh_w (int offset, int data);
void spacfury2_sh_w (int offset, int data);

int tacscan_sh_start (void);
void tacscan_sh_w (int offset, int data);
void tacscan_sh_update(void);

void startrek_sh_w (int offset, int data);

/* Video hardware prototypes */
void sega_init_colors (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int sega_vh_start (void);
void sega_vh_stop (void);
void sega_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc800, 0xcfff, MRA_RAM },
	{ 0xe000, 0xefff, MRA_RAM, &vectorram, &vectorram_size },
	{ 0xd000, 0xdfff, MRA_RAM },			/* sound ram */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xFFFF, sega_wr, &sega_mem },
	{ -1 }
};

static struct IOReadPort spacfury_readport[] =
{
 	{ 0x3f, 0x3f, sega_sh_r },
 	{ 0xbe, 0xbe, sega_mult_r },
	{ 0xf8, 0xfb, sega_read_ports },
	{ -1 }	/* end of table */
};

static struct IOWritePort spacfury_writeport[] =
{
	{ 0x38, 0x38, sega_sh_speech_w },
	{ 0x3e, 0x3e, spacfury1_sh_w },
	{ 0x3f, 0x3f, spacfury2_sh_w },
  	{ 0xbd, 0xbd, sega_mult1_w },
 	{ 0xbe, 0xbe, sega_mult2_w },
	{ 0xf9, 0xf9, coin_counter_w }, /* 0x80 = enable, 0x00 = disable */
	{ -1 }	/* end of table */
};

static struct IOReadPort zektor_readport[] =
{
 	{ 0x3f, 0x3f, sega_sh_r },
 	{ 0xbe, 0xbe, sega_mult_r },
	{ 0xf8, 0xfb, sega_read_ports },
	{ 0xfc, 0xfc, sega_IN4_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort zektor_writeport[] =
{
	{ 0x38, 0x38, sega_sh_speech_w },
  	{ 0xbd, 0xbd, sega_mult1_w },
 	{ 0xbe, 0xbe, sega_mult2_w },
 	{ 0xf8, 0xf8, sega_switch_w },
	{ 0xf9, 0xf9, coin_counter_w }, /* 0x80 = enable, 0x00 = disable */
	{ -1 }	/* end of table */
};

static struct IOWritePort tacscan_writeport[] =
{
	{ 0x3f, 0x3f, tacscan_sh_w },
  	{ 0xbd, 0xbd, sega_mult1_w },
 	{ 0xbe, 0xbe, sega_mult2_w },
 	{ 0xf8, 0xf8, sega_switch_w },
	{ 0xf9, 0xf9, coin_counter_w }, /* 0x80 = enable, 0x00 = disable */
	{ -1 }	/* end of table */
};

static struct IOReadPort elim2_readport[] =
{
 	{ 0x3f, 0x3f, sega_sh_r },
 	{ 0xbe, 0xbe, sega_mult_r },
	{ 0xf8, 0xfb, sega_read_ports },
	{ 0xfc, 0xfc, input_port_4_r },
	{ -1 }	/* end of table */
};

static struct IOReadPort elim4_readport[] =
{
 	{ 0x3f, 0x3f, sega_sh_r },
 	{ 0xbe, 0xbe, sega_mult_r },
	{ 0xf8, 0xfb, sega_read_ports },
	{ 0xfc, 0xfc, elim4_IN4_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort elim_writeport[] =
{
	{ 0x3e, 0x3e, elim1_sh_w },
	{ 0x3f, 0x3f, elim2_sh_w },
  	{ 0xbd, 0xbd, sega_mult1_w },
 	{ 0xbe, 0xbe, sega_mult2_w },
 	{ 0xf8, 0xf8, sega_switch_w },
	{ 0xf9, 0xf9, coin_counter_w }, /* 0x80 = enable, 0x00 = disable */
	{ -1 }	/* end of table */
};

static struct IOWritePort startrek_writeport[] =
{
	{ 0x38, 0x38, sega_sh_speech_w },
	{ 0x3f, 0x3f, startrek_sh_w },
  	{ 0xbd, 0xbd, sega_mult1_w },
 	{ 0xbe, 0xbe, sega_mult2_w },
 	{ 0xf8, 0xf8, sega_switch_w },
	{ 0xf9, 0xf9, coin_counter_w }, /* 0x80 = enable, 0x00 = disable */
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( spacfury_input_ports )
	PORT_START	/* IN0 - port 0xf8 */
	/* The next bit is referred to as the Service switch in the self test - it just adds a credit */
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )

	PORT_START	/* IN1 - port 0xf9 */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 - port 0xfa */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT ( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 - port 0xfb */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 - FAKE - lazy way to move the self-test fake input port to 5 */
	PORT_BIT ( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN5 - FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

	PORT_START	/* FAKE */
        /* This fake input port is used for DIP Switch 1 */
        PORT_DIPNAME( 0x03, 0x01, "Bonus Ship", IP_KEY_NONE )
        PORT_DIPSETTING(    0x03, "40K Points" )
        PORT_DIPSETTING(    0x01, "30K Points" )
        PORT_DIPSETTING(    0x02, "20K Points" )
        PORT_DIPSETTING(    0x00, "10K Points" )
        PORT_DIPNAME( 0x0c, 0x00, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x0c, "Very Hard" )
        PORT_DIPSETTING(    0x04, "Hard" )
        PORT_DIPSETTING(    0x08, "Moderate" )
        PORT_DIPSETTING(    0x00, "Easy" )
        PORT_DIPNAME( 0x30, 0x30, "Number of Ships", IP_KEY_NONE )
        PORT_DIPSETTING(    0x30, "5 Ships" )
        PORT_DIPSETTING(    0x10, "4 Ships" )
        PORT_DIPSETTING(    0x20, "3 Ships" )
        PORT_DIPSETTING(    0x00, "2 Ships" )
        PORT_DIPNAME( 0x40, 0x00, "Attract Sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPNAME( 0x80, 0x80, "Orientation", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "Upright" )
        PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START	/* FAKE */
        /* This fake input port is used for DIP Switch 2 */
        PORT_DIPNAME( 0x0F, 0x0C, "Coins/Credits (R)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x08, "3 / 1" )
        PORT_DIPSETTING(    0x04, "2 / 1" )
        PORT_DIPSETTING(    0x0C, "1 / 1" )
        PORT_DIPSETTING(    0x02, "1 / 2" )
        PORT_DIPSETTING(    0x0A, "1 / 3" )
        PORT_DIPSETTING(    0x06, "1 / 4" )
        PORT_DIPSETTING(    0x0E, "1 / 5" )
        PORT_DIPSETTING(    0x01, "1 / 6" )
        PORT_DIPSETTING(    0x09, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x05, "2/4 / 1/3" )
        PORT_DIPSETTING(    0x0D, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x03, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0x0B, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x07, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0x0F, "1/2/3/4 / 2/4/6/9" )

        PORT_DIPNAME( 0xF0, 0xC0, "Coins/Credits (L)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x80, "3 / 1" )
        PORT_DIPSETTING(    0x40, "2 / 1" )
        PORT_DIPSETTING(    0xC0, "1 / 1" )
        PORT_DIPSETTING(    0x20, "1 / 2" )
        PORT_DIPSETTING(    0xA0, "1 / 3" )
        PORT_DIPSETTING(    0x60, "1 / 4" )
        PORT_DIPSETTING(    0xE0, "1 / 5" )
        PORT_DIPSETTING(    0x10, "1 / 6" )
        PORT_DIPSETTING(    0x90, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x50, "2/4 / 1/3" )
        PORT_DIPSETTING(    0xD0, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x30, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0xB0, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x70, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0xF0, "1/2/3/4 / 2/4/6/9" )

INPUT_PORTS_END

INPUT_PORTS_START( zektor_input_ports )
	PORT_START	/* IN0 - port 0xf8 */
	/* The next bit is referred to as the Service switch in the self test - it just adds a credit */
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )

	PORT_START	/* IN1 - port 0xf9 */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 - port 0xfa */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 - port 0xfb */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 - port 0xfc - read in machine/sega.c */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT ( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN5 - FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

	PORT_START	/* FAKE */
        /* This fake input port is used for DIP Switch 1 */
        PORT_DIPNAME( 0x0c, 0x00, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x0c, "Very Hard" )
        PORT_DIPSETTING(    0x04, "Hard" )
        PORT_DIPSETTING(    0x08, "Moderate" )
        PORT_DIPSETTING(    0x00, "Easy" )
        PORT_DIPNAME( 0x30, 0x30, "Number of Ships", IP_KEY_NONE )
        PORT_DIPSETTING(    0x30, "5 Ships" )
        PORT_DIPSETTING(    0x10, "4 Ships" )
        PORT_DIPSETTING(    0x20, "3 Ships" )
        PORT_DIPSETTING(    0x00, "2 Ships" )
        PORT_DIPNAME( 0x40, 0x00, "Attract Sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPNAME( 0x80, 0x80, "Orientation", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "Upright" )
        PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START	/* FAKE */
        /* This fake input port is used for DIP Switch 2 */
        PORT_DIPNAME( 0x0F, 0x0C, "Coins/Credits (R)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x08, "3 / 1" )
        PORT_DIPSETTING(    0x04, "2 / 1" )
        PORT_DIPSETTING(    0x0C, "1 / 1" )
        PORT_DIPSETTING(    0x02, "1 / 2" )
        PORT_DIPSETTING(    0x0A, "1 / 3" )
        PORT_DIPSETTING(    0x06, "1 / 4" )
        PORT_DIPSETTING(    0x0E, "1 / 5" )
        PORT_DIPSETTING(    0x01, "1 / 6" )
        PORT_DIPSETTING(    0x09, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x05, "2/4 / 1/3" )
        PORT_DIPSETTING(    0x0D, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x03, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0x0B, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x07, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0x0F, "1/2/3/4 / 2/4/6/9" )

        PORT_DIPNAME( 0xF0, 0xC0, "Coins/Credits (L)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x80, "3 / 1" )
        PORT_DIPSETTING(    0x40, "2 / 1" )
        PORT_DIPSETTING(    0xC0, "1 / 1" )
        PORT_DIPSETTING(    0x20, "1 / 2" )
        PORT_DIPSETTING(    0xA0, "1 / 3" )
        PORT_DIPSETTING(    0x60, "1 / 4" )
        PORT_DIPSETTING(    0xE0, "1 / 5" )
        PORT_DIPSETTING(    0x10, "1 / 6" )
        PORT_DIPSETTING(    0x90, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x50, "2/4 / 1/3" )
        PORT_DIPSETTING(    0xD0, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x30, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0xB0, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x70, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0xF0, "1/2/3/4 / 2/4/6/9" )

        PORT_START      /* IN8 - FAKE port for the dial */
	PORT_ANALOG ( 0xff, 0x00, IPT_DIAL|IPF_CENTER, 100, 0, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( startrek_input_ports )
	PORT_START	/* IN0 - port 0xf8 */
	/* The next bit is referred to as the Service switch in the self test - it just adds a credit */
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )

	PORT_START	/* IN1 - port 0xf9 */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 - port 0xfa */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 - port 0xfb */
	PORT_BIT ( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 - port 0xfc - read in machine/sega.c */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT ( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN5 - FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

	PORT_START	/* FAKE */
        /* This fake input port is used for DIP Switch 1 */
        PORT_DIPNAME( 0x03, 0x01, "Bonus Ship", IP_KEY_NONE )
        PORT_DIPSETTING(    0x03, "40K Points" )
        PORT_DIPSETTING(    0x01, "30K Points" )
        PORT_DIPSETTING(    0x02, "20K Points" )
        PORT_DIPSETTING(    0x00, "10K Points" )
        PORT_DIPNAME( 0x0c, 0x00, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x0c, "Tournament" )
        PORT_DIPSETTING(    0x04, "Hard" )
        PORT_DIPSETTING(    0x08, "Medium" )
        PORT_DIPSETTING(    0x00, "Easy" )
        PORT_DIPNAME( 0x30, 0x30, "Photon Torpedoes", IP_KEY_NONE )
        PORT_DIPSETTING(    0x30, "4" )
        PORT_DIPSETTING(    0x10, "3" )
        PORT_DIPSETTING(    0x20, "2" )
        PORT_DIPSETTING(    0x00, "1" )
        PORT_DIPNAME( 0x40, 0x00, "Attract Sound", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPNAME( 0x80, 0x80, "Orientation", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "Upright" )
        PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START	/* FAKE */
        /* This fake input port is used for DIP Switch 2 */
        PORT_DIPNAME( 0x0F, 0x0C, "Coins/Credits (R)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x08, "3 / 1" )
        PORT_DIPSETTING(    0x04, "2 / 1" )
        PORT_DIPSETTING(    0x0C, "1 / 1" )
        PORT_DIPSETTING(    0x02, "1 / 2" )
        PORT_DIPSETTING(    0x0A, "1 / 3" )
        PORT_DIPSETTING(    0x06, "1 / 4" )
        PORT_DIPSETTING(    0x0E, "1 / 5" )
        PORT_DIPSETTING(    0x01, "1 / 6" )
        PORT_DIPSETTING(    0x09, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x05, "2/4 / 1/3" )
        PORT_DIPSETTING(    0x0D, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x03, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0x0B, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x07, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0x0F, "1/2/3/4 / 2/4/6/9" )

        PORT_DIPNAME( 0xF0, 0xC0, "Coins/Credits (L)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x80, "3 / 1" )
        PORT_DIPSETTING(    0x40, "2 / 1" )
        PORT_DIPSETTING(    0xC0, "1 / 1" )
        PORT_DIPSETTING(    0x20, "1 / 2" )
        PORT_DIPSETTING(    0xA0, "1 / 3" )
        PORT_DIPSETTING(    0x60, "1 / 4" )
        PORT_DIPSETTING(    0xE0, "1 / 5" )
        PORT_DIPSETTING(    0x10, "1 / 6" )
        PORT_DIPSETTING(    0x90, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x50, "2/4 / 1/3" )
        PORT_DIPSETTING(    0xD0, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x30, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0xB0, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x70, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0xF0, "1/2/3/4 / 2/4/6/9" )

        PORT_START      /* IN8 - dummy port for the dial */
	PORT_ANALOG ( 0xff, 0x00, IPT_DIAL|IPF_CENTER, 100, 0, 0, 0 )
INPUT_PORTS_END

INPUT_PORTS_START( elim2_input_ports )
	PORT_START	/* IN0 - port 0xf8 */
	/* The next bit is referred to as the Service switch in the self test - it just adds a credit */
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )

	PORT_START	/* IN1 - port 0xf9 */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 - port 0xfa */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 - port 0xfb */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT   | IPF_PLAYER2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 - port 0xfc - read in machine/sega.c */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2        | IPF_PLAYER2 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT ( 0xf8, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START	/* IN5 - FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

	PORT_START	/* FAKE */
        /* This fake input port is used for DIP Switch 1 */
        PORT_DIPNAME( 0x03, 0x02, "Bonus Ship", IP_KEY_NONE )
        PORT_DIPSETTING(    0x03, "None" )
        PORT_DIPSETTING(    0x00, "30K Points" )
        PORT_DIPSETTING(    0x02, "20K Points" )
        PORT_DIPSETTING(    0x01, "10K Points" )
        PORT_DIPNAME( 0x0c, 0x00, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x0c, "Very Hard" )
        PORT_DIPSETTING(    0x04, "Hard" )
        PORT_DIPSETTING(    0x08, "Moderate" )
        PORT_DIPSETTING(    0x00, "Easy" )
        PORT_DIPNAME( 0x30, 0x30, "Number of Ships", IP_KEY_NONE )
        PORT_DIPSETTING(    0x30, "5 Ships" )
        PORT_DIPSETTING(    0x10, "4 Ships" )
        PORT_DIPSETTING(    0x20, "3 Ships" )
        PORT_DIPSETTING(    0x00, "2 Ships" )
        PORT_DIPNAME( 0x80, 0x80, "Orientation", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "Upright" )
        PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START	/* FAKE */
        /* This fake input port is used for DIP Switch 2 */
        PORT_DIPNAME( 0x0F, 0x0C, "Coins/Credits (R)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x08, "3 / 1" )
        PORT_DIPSETTING(    0x04, "2 / 1" )
        PORT_DIPSETTING(    0x0C, "1 / 1" )
        PORT_DIPSETTING(    0x02, "1 / 2" )
        PORT_DIPSETTING(    0x0A, "1 / 3" )
        PORT_DIPSETTING(    0x06, "1 / 4" )
        PORT_DIPSETTING(    0x0E, "1 / 5" )
        PORT_DIPSETTING(    0x01, "1 / 6" )
        PORT_DIPSETTING(    0x09, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x05, "2/4 / 1/3" )
        PORT_DIPSETTING(    0x0D, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x03, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0x0B, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x07, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0x0F, "1/2/3/4 / 2/4/6/9" )

        PORT_DIPNAME( 0xF0, 0xC0, "Coins/Credits (L)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x80, "3 / 1" )
        PORT_DIPSETTING(    0x40, "2 / 1" )
        PORT_DIPSETTING(    0xC0, "1 / 1" )
        PORT_DIPSETTING(    0x20, "1 / 2" )
        PORT_DIPSETTING(    0xA0, "1 / 3" )
        PORT_DIPSETTING(    0x60, "1 / 4" )
        PORT_DIPSETTING(    0xE0, "1 / 5" )
        PORT_DIPSETTING(    0x10, "1 / 6" )
        PORT_DIPSETTING(    0x90, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x50, "2/4 / 1/3" )
        PORT_DIPSETTING(    0xD0, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x30, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0xB0, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x70, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0xF0, "1/2/3/4 / 2/4/6/9" )

INPUT_PORTS_END

INPUT_PORTS_START( elim4_input_ports )
	PORT_START	/* IN0 - port 0xf8 */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	/* The next bit is referred to as the Service switch in the self test - it just adds a credit */
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )

	PORT_START	/* IN1 - port 0xf9 */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2         | IPF_PLAYER2 )
	PORT_BIT ( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN2 - port 0xfa */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT  | IPF_PLAYER2 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1         | IPF_PLAYER2 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN3 - port 0xfb */
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT   | IPF_PLAYER2 )
	PORT_BIT ( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN4 - port 0xfc - read in machine/sega.c */
	PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1        | IPF_PLAYER3 )
	PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2        | IPF_PLAYER3 )
	PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1        | IPF_PLAYER4 )
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2        | IPF_PLAYER4 )
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )

	PORT_START	/* IN5 - FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

	PORT_START	/* FAKE */
        /* This fake input port is used for DIP Switch 1 */
        PORT_DIPNAME( 0x03, 0x02, "Bonus Ship", IP_KEY_NONE )
        PORT_DIPSETTING(    0x03, "None" )
        PORT_DIPSETTING(    0x00, "30K Points" )
        PORT_DIPSETTING(    0x02, "20K Points" )
        PORT_DIPSETTING(    0x01, "10K Points" )
        PORT_DIPNAME( 0x0c, 0x00, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x0c, "Very Hard" )
        PORT_DIPSETTING(    0x04, "Hard" )
        PORT_DIPSETTING(    0x08, "Moderate" )
        PORT_DIPSETTING(    0x00, "Easy" )
        PORT_DIPNAME( 0x30, 0x30, "Number of Ships", IP_KEY_NONE )
        PORT_DIPSETTING(    0x30, "5 Ships" )
        PORT_DIPSETTING(    0x10, "4 Ships" )
        PORT_DIPSETTING(    0x20, "3 Ships" )
        PORT_DIPSETTING(    0x00, "2 Ships" )
        PORT_DIPNAME( 0x80, 0x80, "Orientation", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "Upright" )
        PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START	/* FAKE */
        /* This fake input port is used for DIP Switch 2 */
        PORT_DIPNAME( 0x0F, 0x0C, "Coins/Credits (R)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x08, "3 / 1" )
        PORT_DIPSETTING(    0x04, "2 / 1" )
        PORT_DIPSETTING(    0x0C, "1 / 1" )
        PORT_DIPSETTING(    0x02, "1 / 2" )
        PORT_DIPSETTING(    0x0A, "1 / 3" )
        PORT_DIPSETTING(    0x06, "1 / 4" )
        PORT_DIPSETTING(    0x0E, "1 / 5" )
        PORT_DIPSETTING(    0x01, "1 / 6" )
        PORT_DIPSETTING(    0x09, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x05, "2/4 / 1/3" )
        PORT_DIPSETTING(    0x0D, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x03, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0x0B, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x07, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0x0F, "1/2/3/4 / 2/4/6/9" )

        PORT_DIPNAME( 0xF0, 0xC0, "Coins/Credits (L)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x80, "3 / 1" )
        PORT_DIPSETTING(    0x40, "2 / 1" )
        PORT_DIPSETTING(    0xC0, "1 / 1" )
        PORT_DIPSETTING(    0x20, "1 / 2" )
        PORT_DIPSETTING(    0xA0, "1 / 3" )
        PORT_DIPSETTING(    0x60, "1 / 4" )
        PORT_DIPSETTING(    0xE0, "1 / 5" )
        PORT_DIPSETTING(    0x10, "1 / 6" )
        PORT_DIPSETTING(    0x90, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x50, "2/4 / 1/3" )
        PORT_DIPSETTING(    0xD0, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x30, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0xB0, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x70, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0xF0, "1/2/3/4 / 2/4/6/9" )

        PORT_START      /* IN8 - FAKE - port 0xfc - read in machine/sega.c */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 | IPF_IMPULSE, IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
	PORT_BIT ( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


static struct GfxLayout fakelayout =
{
	1,1,
	0,
	1,
	{ 0 },
	{ 0 },
	{ 0 },
	0
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0,      &fakelayout,     0, 256 },
	{ -1 } /* end of array */
};


/***************************************************************************

  Security Chips

***************************************************************************/

void spacfury_decode(void)
{
    /* This game uses the 315-0064 security chip */
    sega_security(64);
}

void zektor_decode(void)
{
    /* This game uses the 315-0082 security chip */
    sega_security(82);
}

void elim2_decode(void)
{
    /* This game uses the 315-0070 security chip */
    sega_security(70);
}

void elim4_decode(void)
{
    /* This game uses the 315-0076 security chip */
    sega_security(76);
}

void startrek_decode(void)
{
    /* This game uses the 315-0064 security chip */
    sega_security(64);
}

void tacscan_decode(void)
{
    /* This game uses the 315-0076 security chip */
    sega_security(76);
}

/***************************************************************************

  Game ROMs

***************************************************************************/

ROM_START( spacfury_rom ) /* Revision C */
	ROM_REGION(0x10000)	/* 64k for code */
        ROM_LOAD( "969c.u25",     0x0000, 0x0800, 0x411207f2 )
        ROM_LOAD( "960c.u1",      0x0800, 0x0800, 0xd071ab7e )
        ROM_LOAD( "961c.u2",      0x1000, 0x0800, 0xaebc7b97 )
        ROM_LOAD( "962c.u3",      0x1800, 0x0800, 0xdbbba35e )
        ROM_LOAD( "963c.u4",      0x2000, 0x0800, 0xd9e9eadc )
        ROM_LOAD( "964c.u5",      0x2800, 0x0800, 0x7ed947b6 )
        ROM_LOAD( "965c.u6",      0x3000, 0x0800, 0xd2443a22 )
        ROM_LOAD( "966c.u7",      0x3800, 0x0800, 0x1985ccfc )
        ROM_LOAD( "967c.u8",      0x4000, 0x0800, 0x330f0751 )
        ROM_LOAD( "968c.u9",      0x4800, 0x0800, 0x8366eadb )
ROM_END

ROM_START( spacfura_rom ) /* Revision A */
	ROM_REGION(0x10000)	/* 64k for code */
        ROM_LOAD( "969a.u25",     0x0000, 0x0800, 0x896a615c )
        ROM_LOAD( "960a.u1",      0x0800, 0x0800, 0xe1ea7964 )
        ROM_LOAD( "961a.u2",      0x1000, 0x0800, 0xcdb04233 )
        ROM_LOAD( "962a.u3",      0x1800, 0x0800, 0x5f03e632 )
        ROM_LOAD( "963a.u4",      0x2000, 0x0800, 0x45a77b44 )
        ROM_LOAD( "964a.u5",      0x2800, 0x0800, 0xba008f8b )
        ROM_LOAD( "965a.u6",      0x3000, 0x0800, 0x78677d31 )
        ROM_LOAD( "966a.u7",      0x3800, 0x0800, 0xa8a51105 )
        ROM_LOAD( "967a.u8",      0x4000, 0x0800, 0xd60f667d )
        ROM_LOAD( "968a.u9",      0x4800, 0x0800, 0xaea85b6a )
ROM_END

ROM_START( zektor_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1611.cpu",     0x0000, 0x0800, 0x6245aa23 )
	ROM_LOAD( "1586.rom",     0x0800, 0x0800, 0xefeb4fb5 )
	ROM_LOAD( "1587.rom",     0x1000, 0x0800, 0xdaa6c25c )
	ROM_LOAD( "1588.rom",     0x1800, 0x0800, 0x62b67dde )
	ROM_LOAD( "1589.rom",     0x2000, 0x0800, 0xc2db0ba4 )
	ROM_LOAD( "1590.rom",     0x2800, 0x0800, 0x4d948414 )
	ROM_LOAD( "1591.rom",     0x3000, 0x0800, 0xb0556a6c )
	ROM_LOAD( "1592.rom",     0x3800, 0x0800, 0x750ecadf )
	ROM_LOAD( "1593.rom",     0x4000, 0x0800, 0x34f8850f )
	ROM_LOAD( "1594.rom",     0x4800, 0x0800, 0x52b22ab2 )
	ROM_LOAD( "1595.rom",     0x5000, 0x0800, 0xa704d142 )
	ROM_LOAD( "1596.rom",     0x5800, 0x0800, 0x6975e33d )
	ROM_LOAD( "1597.rom",     0x6000, 0x0800, 0xd48ab5c2 )
	ROM_LOAD( "1598.rom",     0x6800, 0x0800, 0xab54a94c )
	ROM_LOAD( "1599.rom",     0x7000, 0x0800, 0xc9d4f3a5 )
	ROM_LOAD( "1600.rom",     0x7800, 0x0800, 0x893b7dbc )
	ROM_LOAD( "1601.rom",     0x8000, 0x0800, 0x867bdf4f )
	ROM_LOAD( "1602.rom",     0x8800, 0x0800, 0xbd447623 )
	ROM_LOAD( "1603.rom",     0x9000, 0x0800, 0x9f8f10e8 )
	ROM_LOAD( "1604.rom",     0x9800, 0x0800, 0xad2f0f6c )
	ROM_LOAD( "1605.rom",     0xa000, 0x0800, 0xe27d7144 )
	ROM_LOAD( "1606.rom",     0xa800, 0x0800, 0x7965f636 )
ROM_END

ROM_START( tacscan_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1711a",        0x0000, 0x0800, 0x0da13158 )
	ROM_LOAD( "1670c",        0x0800, 0x0800, 0x98de6fd5 )
	ROM_LOAD( "1671a",        0x1000, 0x0800, 0xdc400074 )
	ROM_LOAD( "1672a",        0x1800, 0x0800, 0x2caf6f7e )
	ROM_LOAD( "1673a",        0x2000, 0x0800, 0x1495ce3d )
	ROM_LOAD( "1674a",        0x2800, 0x0800, 0xab7fc5d9 )
	ROM_LOAD( "1675a",        0x3000, 0x0800, 0xcf5e5016 )
	ROM_LOAD( "1676a",        0x3800, 0x0800, 0xb61a3ab3 )
	ROM_LOAD( "1677a",        0x4000, 0x0800, 0xbc0273b1 )
	ROM_LOAD( "1678b",        0x4800, 0x0800, 0x7894da98 )
	ROM_LOAD( "1679a",        0x5000, 0x0800, 0xdb865654 )
	ROM_LOAD( "1680a",        0x5800, 0x0800, 0x2c2454de )
	ROM_LOAD( "1681a",        0x6000, 0x0800, 0x77028885 )
	ROM_LOAD( "1682a",        0x6800, 0x0800, 0xbabe5cf1 )
	ROM_LOAD( "1683a",        0x7000, 0x0800, 0x1b98b618 )
	ROM_LOAD( "1684a",        0x7800, 0x0800, 0xcb3ded3b )
	ROM_LOAD( "1685a",        0x8000, 0x0800, 0x43016a79 )
	ROM_LOAD( "1686a",        0x8800, 0x0800, 0xa4397772 )
	ROM_LOAD( "1687a",        0x9000, 0x0800, 0x002f3bc4 )
	ROM_LOAD( "1688a",        0x9800, 0x0800, 0x0326d87a )
	ROM_LOAD( "1709a",        0xa000, 0x0800, 0xf35ed1ec )
	ROM_LOAD( "1710a",        0xa800, 0x0800, 0x6203be22 )
ROM_END

ROM_START( elim2_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "cpu_u25.969",  0x0000, 0x0800, 0x411207f2 )
	ROM_LOAD( "1333",         0x0800, 0x0800, 0xfd2a2916 )
	ROM_LOAD( "1334",         0x1000, 0x0800, 0x79eb5548 )
	ROM_LOAD( "1335",         0x1800, 0x0800, 0x3944972e )
	ROM_LOAD( "1336",         0x2000, 0x0800, 0x852f7b4d )
	ROM_LOAD( "1337",         0x2800, 0x0800, 0xcf932b08 )
	ROM_LOAD( "1338",         0x3000, 0x0800, 0x99a3f3c9 )
	ROM_LOAD( "1339",         0x3800, 0x0800, 0xd35f0fa3 )
	ROM_LOAD( "1340",         0x4000, 0x0800, 0x8fd4da21 )
	ROM_LOAD( "1341",         0x4800, 0x0800, 0x629c9a28 )
	ROM_LOAD( "1342",         0x5000, 0x0800, 0x643df651 )
	ROM_LOAD( "1343",         0x5800, 0x0800, 0xd29d70d2 )
	ROM_LOAD( "1344",         0x6000, 0x0800, 0xc5e153a3 )
	ROM_LOAD( "1345",         0x6800, 0x0800, 0x40597a92 )
ROM_END

ROM_START( elim4_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "1390_cpu.u25", 0x0000, 0x0800, 0x97010c3e )
	ROM_LOAD( "1347",         0x0800, 0x0800, 0x657d7320 )
	ROM_LOAD( "1348",         0x1000, 0x0800, 0xb15fe578 )
	ROM_LOAD( "1349",         0x1800, 0x0800, 0x0702b586 )
	ROM_LOAD( "1350",         0x2000, 0x0800, 0x4168dd3b )
	ROM_LOAD( "1351",         0x2800, 0x0800, 0xc950f24c )
	ROM_LOAD( "1352",         0x3000, 0x0800, 0xdc8c91cc )
	ROM_LOAD( "1353",         0x3800, 0x0800, 0x11eda631 )
	ROM_LOAD( "1354",         0x4000, 0x0800, 0xb9dd6e7a )
	ROM_LOAD( "1355",         0x4800, 0x0800, 0xc92c7237 )
	ROM_LOAD( "1356",         0x5000, 0x0800, 0x889b98e3 )
	ROM_LOAD( "1357",         0x5800, 0x0800, 0xd79248a5 )
	ROM_LOAD( "1358",         0x6000, 0x0800, 0xc5dabc77 )
	ROM_LOAD( "1359",         0x6800, 0x0800, 0x24c8e5d8 )
	ROM_LOAD( "1360",         0x7000, 0x0800, 0x96d48238 )
ROM_END

ROM_START( startrek_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "cpu1873",      0x0000, 0x0800, 0xbe46f5d9 )
	ROM_LOAD( "1848",         0x0800, 0x0800, 0x65e3baf3 )
	ROM_LOAD( "1849",         0x1000, 0x0800, 0x8169fd3d )
	ROM_LOAD( "1850",         0x1800, 0x0800, 0x78fd68dc )
	ROM_LOAD( "1851",         0x2000, 0x0800, 0x3f55ab86 )
	ROM_LOAD( "1852",         0x2800, 0x0800, 0x2542ecfb )
	ROM_LOAD( "1853",         0x3000, 0x0800, 0x75c2526a )
	ROM_LOAD( "1854",         0x3800, 0x0800, 0x096d75d0 )
	ROM_LOAD( "1855",         0x4000, 0x0800, 0xbc7b9a12 )
	ROM_LOAD( "1856",         0x4800, 0x0800, 0xed9fe2fb )
	ROM_LOAD( "1857",         0x5000, 0x0800, 0x28699d45 )
	ROM_LOAD( "1858",         0x5800, 0x0800, 0x3a7593cb )
	ROM_LOAD( "1859",         0x6000, 0x0800, 0x5b11886b )
	ROM_LOAD( "1860",         0x6800, 0x0800, 0x62eb96e6 )
	ROM_LOAD( "1861",         0x7000, 0x0800, 0x99852d1d )
	ROM_LOAD( "1862",         0x7800, 0x0800, 0x76ce27b2 )
	ROM_LOAD( "1863",         0x8000, 0x0800, 0xdd92d187 )
	ROM_LOAD( "1864",         0x8800, 0x0800, 0xe37d3a1e )
	ROM_LOAD( "1865",         0x9000, 0x0800, 0xb2ec8125 )
	ROM_LOAD( "1866",         0x9800, 0x0800, 0x6f188354 )
	ROM_LOAD( "1867",         0xa000, 0x0800, 0xb0a3eae8 )
	ROM_LOAD( "1868",         0xa800, 0x0800, 0x8b4e2e07 )
	ROM_LOAD( "1869",         0xb000, 0x0800, 0xe5663070 )
	ROM_LOAD( "1870",         0xb800, 0x0800, 0x4340616d )
ROM_END

/***************************************************************************

  Hi Score Routines

***************************************************************************/

static int spacfury_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0xC924],"\x90\x02",2) == 0) &&
			(memcmp(&RAM[0xC95C],"\x10\x00",2) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xC924],2*30);  /* 2 bytes * 30 scores */
			osd_fread(f,&RAM[0xCFD2],3*10);   /* 3 letters * 10 scores */

			osd_fclose(f);
		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}


static void spacfury_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xC924],2*30); /* 2 bytes * 30 scores */
		osd_fwrite(f,&RAM[0xCFD2],3*10);  /* 3 letters * 10 scores */
		osd_fclose(f);
	}
}


static int zektor_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0xC924],"\x90\x02",2) == 0) &&
			(memcmp(&RAM[0xC95C],"\x10\x00",2) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xC924],2*30);  /* 2 bytes * 30 scores? */
			osd_fread(f,&RAM[0xCFD2],3*5);   /* 3 letters * 5 scores */

			osd_fclose(f);
		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}


static void zektor_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xC924],2*30); /* 2 bytes * 30 scores? */
		osd_fwrite(f,&RAM[0xCFD2],3*5);  /* 3 letters * 5 scores */
		osd_fclose(f);
	}
}

static int tacscan_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0xCB44],"MCL",3) == 0) &&
		(memcmp(&RAM[0xCB95],"\x02\x03\x00",3) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xCB44],3*5);  /* initials */
			osd_fread(f,&RAM[0xCB95],3*5);  /* scores */
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}


static void tacscan_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xCB44],3*5);  /* initials */
		osd_fwrite(f,&RAM[0xCB95],3*5);  /* scores */
		osd_fclose(f);
	}
}


static int elim2_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0xC99F],"\x0C\x0B\x07",3) == 0) &&
			(memcmp(&RAM[0xC9BA],"\x0A\x08\x03",3) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xC924],2*30);  /* 2 bytes * 30 scores */
			osd_fread(f,&RAM[0xC99F],3*10);  /* 3 letters * 10 scores */

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}


static void elim2_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xC924],2*30); /* 2 bytes * 30 scores */
		osd_fwrite(f,&RAM[0xC99F],3*10); /* 3 letters * 10 scores */
		osd_fclose(f);
	}
}

static int elim4_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0xCC4D],"\x0C\x0B\x07",3) == 0) &&
			(memcmp(&RAM[0xCC68],"\x0A\x08\x03",3) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xC928],2*30);  /* 2 bytes * 30 scores */
			osd_fread(f,&RAM[0xCC4D],3*10);  /* 3 letters * 10 scores */

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}


static void elim4_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xC928],2*30); /* 2 bytes * 30 scores */
		osd_fwrite(f,&RAM[0xCC4D],3*10); /* 3 letters * 10 scores */
		osd_fclose(f);
	}
}

static int startrek_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if ((memcmp(&RAM[0xC98B],"SLP",3) == 0) &&
			(memcmp(&RAM[0xC910],"\x25\x06\x09",3) == 0))
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xC98B],0x1B);
			osd_fread(f,&RAM[0xC910],0x24);
			// osd_fread(f,&RAM[0xC98B],0xF0); /* longer ? */

			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}


static void startrek_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xC98B],0x1B);
		osd_fwrite(f,&RAM[0xC910],0x24);
		osd_fclose(f);
	}
}




/***************************************************************************

  Game driver(s)

***************************************************************************/

static const char *spacfury_sample_names[] =
{
	"*spacfury",
	/* Speech samples */
	"sf01.sam",
	"sf02.sam",
	"sf03.sam",
	"sf04.sam",
	"sf05.sam",
	"sf06.sam",
	"sf07.sam",
	"sf08.sam",
	"sf09.sam",
	"sf0a.sam",
	"sf0b.sam",
	"sf0c.sam",
	"sf0d.sam",
	"sf0e.sam",
	"sf0f.sam",
	"sf10.sam",
	"sf11.sam",
	"sf12.sam",
	"sf13.sam",
	"sf14.sam",
	"sf15.sam",
	/* Sound samples */
	"sfury1.sam",
	"sfury2.sam",
	"sfury3.sam",
	"sfury4.sam",
	"sfury5.sam",
	"sfury6.sam",
	"sfury7.sam",
	"sfury8.sam",
	"sfury9.sam",
	"sfury10.sam",
    0	/* end of array */
};

static struct Samplesinterface spacfury_samples_interface =
{
	9	/* 9 channels */
};

static struct MachineDriver spacfury_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120,	/* 3.86712 Mhz */
			0,
			readmem, writemem, spacfury_readport, spacfury_writeport,
			0, 0, /* no vblank interrupt */
			sega_interrupt, 40 /* 40 Hz */
		}
	},
	40, 0,	/* frames per second, vblank duration (vector game, so no vblank) */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	400, 300, { 512, 1536, 552, 1464 },
	gfxdecodeinfo,
	256,256,
	sega_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	sega_vh_start,
	sega_vh_stop,
	sega_vh_screenrefresh,

	/* sound hardware */
	0,
	sega_sh_start,
	0,
	sega_sh_update,
	{
		{
			SOUND_SAMPLES,
			&spacfury_samples_interface
		}
	}
};



struct GameDriver spacfury_driver =
{
	__FILE__,
	0,
	"spacfury",
	"Space Fury (revision C)",
	"1981",
	"Sega",
	"Al Kossow (G80 Emu)\nBrad Oliver (MAME driver)\n"VECTOR_TEAM,
	0,
	&spacfury_machine_driver,
	0,

	spacfury_rom,
	spacfury_decode, 0,
	spacfury_sample_names,
	0,

	spacfury_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

    spacfury_hiload, spacfury_hisave
};

struct GameDriver spacfura_driver =
{
	__FILE__,
	&spacfury_driver,
	"spacfura",
	"Space Fury (revision A)",
	"1981",
	"Sega",
	"Al Kossow (G80 Emu)\nBrad Oliver (MAME driver)\n"VECTOR_TEAM,
	0,
	&spacfury_machine_driver,
	0,

	spacfura_rom,
	spacfury_decode, 0,
	spacfury_sample_names,
	0,

	spacfury_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	spacfury_hiload, spacfury_hisave
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

static const char *zektor_sample_names[] =
{
	"*zektor",
	"zk01.sam",
	"zk02.sam",
	"zk03.sam",
	"zk04.sam",
	"zk05.sam",
	"zk06.sam",
	"zk07.sam",
	"zk08.sam",
	"zk09.sam",
	"zk0a.sam",
	"zk0b.sam",
	"zk0c.sam",
	"zk0d.sam",
	"zk0e.sam",
	"zk0f.sam",
	"zk10.sam",
	"zk11.sam",
	"zk12.sam",
	"zk13.sam",
    0	/* end of array */
};

static struct Samplesinterface zektor_samples_interface =
{
	1 /* only speech for now */
};

static struct MachineDriver zektor_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120,	/* 3.86712 Mhz */
			0,
			readmem,writemem,zektor_readport,zektor_writeport,

			0, 0, /* no vblank interrupt */
			sega_interrupt, 40 /* 40 Hz */
		}
	},
	40, 0,	/* frames per second, vblank duration (vector game, so no vblank) */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	400, 300, { 512, 1536, 624, 1432 },
	gfxdecodeinfo,
	256,256,
	sega_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	sega_vh_start,
	sega_vh_stop,
	sega_vh_screenrefresh,

	/* sound hardware */
	0,
	sega_sh_start,
	0,
	sega_sh_update,
	{
		{
			SOUND_SAMPLES,
			&zektor_samples_interface
		}
	}
};



struct GameDriver zektor_driver =
{
	__FILE__,
	0,
	"zektor",
	"Zektor",
	"1982",
	"Sega",
	"Al Kossow (G80 Emu)\nBrad Oliver (MAME driver)\n"VECTOR_TEAM,
	0,
	&zektor_machine_driver,
	0,

	zektor_rom,
	zektor_decode, 0,
	zektor_sample_names,
	0,

	zektor_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	zektor_hiload, zektor_hisave
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

static const char *tacscan_sample_names[] =
{
	"*tacscan",
	/* Player ship thrust sounds */
	"01.sam",
	"02.sam",
	"03.sam",
	"plaser.sam",
	"pexpl.sam",
	"pship.sam",
	"tunnelh.sam",
	"sthrust.sam",
	"slaser.sam",
	"sexpl.sam",
	"eshot.sam",
	"eexpl.sam",
    0	/* end of array */
};

static struct Samplesinterface tacscan_samples_interface =
{
	6	/* 6 channels */
};

static struct MachineDriver tacscan_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120,	/* 3.86712 Mhz */
			0,
			readmem,writemem,zektor_readport,tacscan_writeport,

			0, 0, /* no vblank interrupt */
			sega_interrupt, 40 /* 40 Hz */
		}
	},
	40, 0,	/* frames per second, vblank duration (vector game, so no vblank) */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	400, 300, { 496, 1552, 592, 1456 },
	gfxdecodeinfo,
	256,256,
	sega_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	sega_vh_start,
	sega_vh_stop,
	sega_vh_screenrefresh,

	/* sound hardware */
	0,
	tacscan_sh_start,
	0,
	tacscan_sh_update,
	{
		{
			SOUND_SAMPLES,
			&tacscan_samples_interface
		}
	}
};

struct GameDriver tacscan_driver =
{
	__FILE__,
	0,
	"tacscan",
	"Tac/Scan",
	"1982",
	"Sega",
	"Al Kossow (G80 Emu)\nBrad Oliver (MAME driver)\n"VECTOR_TEAM,
	0,
	&tacscan_machine_driver,
	0,

	tacscan_rom,
	tacscan_decode, 0,
	tacscan_sample_names,
	0,

	zektor_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	tacscan_hiload, tacscan_hisave
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

/* Eliminator sound samples (all versions) */
static const char *elim_sample_names[] =
{
	"*elim2",
	"elim1.sam",
	"elim2.sam",
	"elim3.sam",
	"elim4.sam",
	"elim5.sam",
	"elim6.sam",
	"elim7.sam",
	"elim8.sam",
	"elim9.sam",
	"elim10.sam",
	"elim11.sam",
	"elim12.sam",
    0	/* end of array */
};

static struct Samplesinterface elim2_samples_interface =
{
	8	/* 8 channels */
};

static struct MachineDriver elim2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120,	/* 3.86712 Mhz */
			0,
			readmem,writemem,elim2_readport,elim_writeport,

			0, 0, /* no vblank interrupt */
			sega_interrupt, 40 /* 40 Hz */
		}
	},
	40, 0,	/* frames per second, vblank duration (vector game, so no vblank) */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	400, 300, { 512, 1536, 600, 1440 },
	gfxdecodeinfo,
	256,256,
	sega_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	sega_vh_start,
	sega_vh_stop,
	sega_vh_screenrefresh,

	/* sound hardware */
	0,
	sega_sh_start,
	0,
	sega_sh_update,
	{
		{
			SOUND_SAMPLES,
			&elim2_samples_interface
		}
	}
};

struct GameDriver elim2_driver =
{
	__FILE__,
	0,
	"elim2",
	"Eliminator (2 Players)",
	"1981",
	"Gremlin",
	"Al Kossow (G80 Emu)\nBrad Oliver (MAME driver)\n"VECTOR_TEAM,
	0,
	&elim2_machine_driver,
	0,

	elim2_rom,
	elim2_decode, 0,
	elim_sample_names,
	0,

	elim2_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	elim2_hiload, elim2_hisave
};

static struct MachineDriver elim4_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120,	/* 3.86712 Mhz */
			0,
			readmem,writemem,elim4_readport,elim_writeport,

			0, 0, /* no vblank interrupt */
			sega_interrupt, 40 /* 40 Hz */
		}
	},
	40, 0,	/* frames per second, vblank duration (vector game, so no vblank) */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	400, 300, { 512, 1536, 600, 1440 },
	gfxdecodeinfo,
	256,256,
	sega_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	sega_vh_start,
	sega_vh_stop,
	sega_vh_screenrefresh,

	/* sound hardware */
	0,
	sega_sh_start,
	0,
	sega_sh_update,
	{
		{
			SOUND_SAMPLES,
			&elim2_samples_interface
		}
	}
};

struct GameDriver elim4_driver =
{
	__FILE__,
	&elim2_driver,
	"elim4",
	"Eliminator (4 Players)",
	"1981",
	"Gremlin",
	"Al Kossow (G80 Emu)\nBrad Oliver (MAME driver)\n"VECTOR_TEAM,
	0,
	&elim4_machine_driver,
	0,

	elim4_rom,
	elim4_decode, 0,
	elim_sample_names,
	0,

	elim4_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	elim4_hiload, elim4_hisave
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

static const char *startrek_sample_names[] =
{
	"*startrek",
	/* Speech samples */
	"st01.sam",
	"st02.sam",
	"st03.sam",
	"st04.sam",
	"st05.sam",
	"st06.sam",
	"st07.sam",
	"st08.sam",
	"st09.sam",
	"st0a.sam",
	"st0b.sam",
	"st0c.sam",
	"st0d.sam",
	"st0e.sam",
	"st0f.sam",
	"st10.sam",
	"st11.sam",
	"st12.sam",
	"st13.sam",
	"st14.sam",
	"st15.sam",
	"st16.sam",
	"st17.sam",
	/* Sound samples */
	"trek1.sam",
	"trek2.sam",
	"trek3.sam",
	"trek4.sam",
	"trek5.sam",
	"trek6.sam",
	"trek7.sam",
	"trek8.sam",
	"trek9.sam",
	"trek10.sam",
	"trek11.sam",
	"trek12.sam",
	"trek13.sam",
	"trek14.sam",
	"trek15.sam",
	"trek16.sam",
	"trek17.sam",
	"trek18.sam",
	"trek19.sam",
	"trek20.sam",
	"trek21.sam",
	"trek22.sam",
	"trek23.sam",
	"trek24.sam",
	"trek25.sam",
	"trek26.sam",
	"trek27.sam",
	"trek28.sam",
    0	/* end of array */
};

static struct Samplesinterface startrek_samples_interface =
{
	6	/* 6 channels */
};

static struct MachineDriver startrek_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3867120,	/* 3.86712 Mhz */
			0,
			readmem,writemem,zektor_readport,startrek_writeport,

			0, 0, /* no vblank interrupt */
			sega_interrupt, 40 /* 40 Hz */
		}
	},
	40, 0,	/* frames per second, vblank duration (vector game, so no vblank) */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	400, 300, { 512, 1536, 616, 1464 },
	gfxdecodeinfo,
	256,256,
	sega_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	sega_vh_start,
	sega_vh_stop,
	sega_vh_screenrefresh,

	/* sound hardware */
	0,
	sega_sh_start,
	0,
	sega_sh_update,
	{
		{
			SOUND_SAMPLES,
			&startrek_samples_interface
		}
	}
};

struct GameDriver startrek_driver =
{
	__FILE__,
	0,
	"startrek",
	"Star Trek",
	"1982",
	"Sega",
	"Al Kossow (G80 Emu)\nBrad Oliver (MAME driver)\n"VECTOR_TEAM,
	0,
	&startrek_machine_driver,
	0,

	startrek_rom,
	startrek_decode, 0,
	startrek_sample_names,
	0,

	startrek_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	startrek_hiload, startrek_hisave
};
