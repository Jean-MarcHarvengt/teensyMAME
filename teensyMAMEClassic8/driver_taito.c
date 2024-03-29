/***************************************************************************

Taito games memory map (preliminary)

MAIN CPU:

0000-7fff ROM (6000-7fff banked in two banks, controlled by bit 7 of d50e)
8000-87ff RAM
9000-bfff Character generator RAM
c400-c7ff Video RAM: front playfield
c800-cbff Video RAM: middle playfield
cc00-cfff Video RAM: back playfield
d100-d17f Sprites
d200-d27f Palette (64 pairs: xxxxxxxR RRGGGBBB. bits are inverted, i.e. 0x01ff = black)
e000-efff ROM (on the protection board with the 68705)

read:

8800      68705 data read
8801      68705 status read
            bit 0 = the 68705 has read data from the Z80
            bit 1 = the 68705 has written data for the Z80
d400-d403 hardware collision detection registers
          d400 ?
          d401 ?
          d402 ?
          d403 bit0 = obj/pf1
               bit1 = obj/pf2
               bit2 = obj/pf3
               bit3 = pf1/pf2
               bit4 = pf1/pf3
               bit5 = pf2/pf3
               bit6 = nc
               bit7 = nc
d404      returns contents of graphic ROM, pointed by d509-d50a
d408      IN0
          bit 5 = jump player 1
          bit 4 = fire player 1
          bit 3 = up player 1
          bit 2 = down player 1
          bit 1 = right player 1
          bit 0 = left player 1
d409      IN1
          bit 5 = jump player 2 (COCKTAIL only)
          bit 4 = fire player 2 (COCKTAIL only)
          bit 3 = up player 2 (COCKTAIL only)
          bit 2 = down player 2 (COCKTAIL only)
          bit 1 = right player 2 (COCKTAIL only)
          bit 0 = left player 2 (COCKTAIL only)
d40a      DSW1
          elevator:
          bit 7   = cocktail / upright (0 = upright)
          bit 6   = flip screen
          bit 5   = ?
          bit 3-4 = lives
		  bit 2   = free play
          bit 0-1 = bonus
          jungle:
          bit 7   = cocktail / upright (0 = upright)
          bit 6   = flip screen
          bit 5   = RAM check
          bit 3-4 = lives
		  bit 2   = ?
          bit 0-1 = finish bonus
d40b      IN2 - can come from a ROM or PAL chip
          bit 7 = start 2
          bit 6 = start 1
          bit 5 = coin 1
          bit 4 = coin 2 (except Alpine Ski Set 1, where it is used for protection)
          bit 0-3 = protection (Alpine Ski); written at d50f (Set 1) or d50e (Set 2)
d40c      COIN
          bit 5 = tilt
          bit 4 = coin
d40d      another input port (use in Front Line for player 2 dial)
d40f      8910 #0 read
            port A DSW2
            port B DSW3

write
8800      68705 data write
d000-d01f front playfield column scroll
d020-d03f middle playfield column scroll
d040-d05f back playfield column scroll
d300      playfield priority control
          bits 0-3 go to A4-A7 of a 256x4 PROM
		  bit 4 selects D0/D1 or D2/D3 of the PROM
		  bit 5-7 n.c.
          A0-A3 of the PROM is fed with a mask of the inactive planes
		    (i.e. all-zero) in the order sprites-front-middle-back
          the 2-bit code which comes out from the PROM selects the plane
		  to display.
d40e      8910 #0 control
d40f      8910 #0 write
d500      front playfield horizontal scroll
d501      front playfield vertical scroll
d502      middle playfield horizontal scroll
d503      middle playfield vertical scroll
d504      back playfield horizontal scroll
d505      back playfield vertical scroll
d506      bits 0-2 = front playfield color code
          bit 3 = front playfield character bank
          bits 4-6 = middle playfield color code
          bit 7 = middle playfield character bank
d507      bits 0-2 = back playfield color code
          bit 3 = back playfield character bank
          bits 4-5 = sprite color bank (1 bank = 2 color codes)
d508      clear hardware collision detection registers
d509-d50a pointer to graphic ROM to read from d404
d50b      command for the audio CPU
d50d      watchdog reset
d50e      bit 7 = ROM bank selector
		  bit 0-4 = protection write (Alpine Ski); result is read from d40b bits 0-3
d50f      can go to a ROM or PAL; the result is read from d40b
		  ==> used in Alpine Ski (Set 1) for protection
d600      bit 0 horizontal screen flip
          bit 1 vertical screen flip
          bit 2 ? sprite related, called OBJEX. It looks like there are 256
                  bytes of sprite RAM, but only 128 can be acessed by the video
                  hardware at a time. This select the high or low bank. The CPU
                  can access all the memory linearly. I don't know if this is
                  ever used.
          bit 3 n.c.
		  bit 4 front playfield enable
		  bit 5 middle playfield enable
		  bit 6 back playfield enable
		  bit 7 sprites enable


SOUND CPU:
0000-3fff ROM (none of the games has this fully populated)
4000-43ff RAM
e000-efff space for diagnostics ROM?

read:
5000      command from CPU board
8101      ?

write:
4800      8910 #1  control
4801      8910 #1  write
            PORT A  digital sound out
4802      8910 #2  control
4803      8910 #2  write
4804      8910 #3  control
4805      8910 #3  write
            port B bit 0 SOUND CPU NMI disable

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80.h"



extern unsigned char *alpine1_protection;
extern unsigned char *alpine2_protection;
void taito_init_machine(void);
void alpine1_protection_w(int offset,int data);
void taito_bankswitch_w(int offset,int data);
int taito_port_2_r(int offset);
int taito_fake_data_r(int offset);
int taito_fake_status_r(int offset);
void taito_fake_data_w(int offset,int data);
int taito_mcu_data_r(int offset);
int taito_mcu_status_r(int offset);
void taito_mcu_data_w(int offset,int data);
int taito_68705_portA_r(int offset);
int taito_68705_portB_r(int offset);
int taito_68705_portC_r(int offset);
void taito_68705_portA_w(int offset,int data);
void taito_68705_portB_w(int offset,int data);

extern unsigned char *taito_videoram2,*taito_videoram3;
extern unsigned char *taito_characterram;
extern unsigned char *taito_scrollx1,*taito_scrollx2,*taito_scrollx3;
extern unsigned char *taito_scrolly1,*taito_scrolly2,*taito_scrolly3;
extern unsigned char *taito_colscrolly1,*taito_colscrolly2,*taito_colscrolly3;
extern unsigned char *taito_gfxpointer;
extern unsigned char *taito_colorbank,*taito_video_priority;
void taito_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int taito_gfxrom_r(int offset);
void taito_videoram2_w(int offset,int data);
void taito_videoram3_w(int offset,int data);
void taito_paletteram_w(int offset,int data);
void taito_colorbank_w(int offset,int data);
void taito_videoenable_w(int offset,int data);
void taito_characterram_w(int offset,int data);
int taito_collision_detection_r(int offset);
void taito_collision_detection_w(int offset,int data);
int taito_vh_start(void);
void taito_vh_stop(void);
void taito_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static int sndnmi_disable = 1;

static void taito_sndnmi_msk(int offset,int data)
{
	sndnmi_disable = data & 0x01;
}

static void taito_soundcommand_w(int offset,int data)
{
	soundlatch_w(offset,data);
	if (!sndnmi_disable) cpu_cause_interrupt(1,Z80_NMI_INT);
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8800, 0x8800, taito_fake_data_r },
	{ 0x8801, 0x8801, taito_fake_status_r },
	{ 0xc400, 0xcfff, MRA_RAM },
	{ 0xd100, 0xd17f, MRA_RAM },
	{ 0xd400, 0xd403, taito_collision_detection_r },
	{ 0xd404, 0xd404, taito_gfxrom_r },
	{ 0xd408, 0xd408, input_port_0_r },	/* IN0 */
	{ 0xd409, 0xd409, input_port_1_r },	/* IN1 */
	{ 0xd40a, 0xd40a, input_port_5_r },	/* DSW1 */
	{ 0xd40b, 0xd40b, taito_port_2_r },	/* IN2 */
	{ 0xd40c, 0xd40c, input_port_3_r },	/* Service */
	{ 0xd40d, 0xd40d, input_port_4_r },
	{ 0xd40f, 0xd40f, AY8910_read_port_0_r },	/* DSW2 and DSW3 */
	{ 0xe000, 0xefff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, taito_fake_data_w },
	{ 0x9000, 0xbfff, taito_characterram_w, &taito_characterram },
	{ 0xc400, 0xc7ff, videoram_w, &videoram, &videoram_size },
	{ 0xc800, 0xcbff, taito_videoram2_w, &taito_videoram2 },
	{ 0xcc00, 0xcfff, taito_videoram3_w, &taito_videoram3 },
	{ 0xd000, 0xd01f, MWA_RAM, &taito_colscrolly1 },
	{ 0xd020, 0xd03f, MWA_RAM, &taito_colscrolly2 },
	{ 0xd040, 0xd05f, MWA_RAM, &taito_colscrolly3 },
	{ 0xd100, 0xd17f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd200, 0xd27f, taito_paletteram_w, &paletteram },
	{ 0xd300, 0xd300, MWA_RAM, &taito_video_priority },
	{ 0xd40e, 0xd40e, AY8910_control_port_0_w },
	{ 0xd40f, 0xd40f, AY8910_write_port_0_w },
	{ 0xd500, 0xd500, MWA_RAM, &taito_scrollx1 },
	{ 0xd501, 0xd501, MWA_RAM, &taito_scrolly1 },
	{ 0xd502, 0xd502, MWA_RAM, &taito_scrollx2 },
	{ 0xd503, 0xd503, MWA_RAM, &taito_scrolly2 },
	{ 0xd504, 0xd504, MWA_RAM, &taito_scrollx3 },
	{ 0xd505, 0xd505, MWA_RAM, &taito_scrolly3 },
	{ 0xd506, 0xd507, taito_colorbank_w, &taito_colorbank },
	{ 0xd508, 0xd508, taito_collision_detection_w },
	{ 0xd509, 0xd50a, MWA_RAM, &taito_gfxpointer },
	{ 0xd50b, 0xd50b, taito_soundcommand_w },
	{ 0xd50d, 0xd50d, watchdog_reset_w },
	{ 0xd50e, 0xd50e, taito_bankswitch_w, &alpine2_protection },
	{ 0xd50f, 0xd50f, alpine1_protection_w, &alpine1_protection },
	{ 0xd600, 0xd600, taito_videoenable_w },
	{ 0xe000, 0xefff, MWA_ROM },
	{ -1 }	/* end of table */
};

/* only difference is taito_fake_ replaced with taito_mcu_ */
static struct MemoryReadAddress mcu_readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8800, 0x8800, taito_mcu_data_r },
	{ 0x8801, 0x8801, taito_mcu_status_r },
	{ 0xc400, 0xcfff, MRA_RAM },
	{ 0xd100, 0xd17f, MRA_RAM },
	{ 0xd400, 0xd403, taito_collision_detection_r },
	{ 0xd404, 0xd404, taito_gfxrom_r },
	{ 0xd408, 0xd408, input_port_0_r },	/* IN0 */
	{ 0xd409, 0xd409, input_port_1_r },	/* IN1 */
	{ 0xd40a, 0xd40a, input_port_5_r },	/* DSW1 */
	{ 0xd40b, 0xd40b, input_port_2_r },	/* IN2 */
	{ 0xd40c, 0xd40c, input_port_3_r },	/* Service */
	{ 0xd40d, 0xd40d, input_port_4_r },
	{ 0xd40f, 0xd40f, AY8910_read_port_0_r },	/* DSW2 and DSW3 */
	{ 0xe000, 0xefff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress mcu_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8800, taito_mcu_data_w },
	{ 0x9000, 0xbfff, taito_characterram_w, &taito_characterram },
	{ 0xc400, 0xc7ff, videoram_w, &videoram, &videoram_size },
	{ 0xc800, 0xcbff, taito_videoram2_w, &taito_videoram2 },
	{ 0xcc00, 0xcfff, taito_videoram3_w, &taito_videoram3 },
	{ 0xd000, 0xd01f, MWA_RAM, &taito_colscrolly1 },
	{ 0xd020, 0xd03f, MWA_RAM, &taito_colscrolly2 },
	{ 0xd040, 0xd05f, MWA_RAM, &taito_colscrolly3 },
	{ 0xd100, 0xd17f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd200, 0xd27f, taito_paletteram_w, &paletteram },
	{ 0xd300, 0xd300, MWA_RAM, &taito_video_priority },
	{ 0xd40e, 0xd40e, AY8910_control_port_0_w },
	{ 0xd40f, 0xd40f, AY8910_write_port_0_w },
	{ 0xd500, 0xd500, MWA_RAM, &taito_scrollx1 },
	{ 0xd501, 0xd501, MWA_RAM, &taito_scrolly1 },
	{ 0xd502, 0xd502, MWA_RAM, &taito_scrollx2 },
	{ 0xd503, 0xd503, MWA_RAM, &taito_scrolly2 },
	{ 0xd504, 0xd504, MWA_RAM, &taito_scrollx3 },
	{ 0xd505, 0xd505, MWA_RAM, &taito_scrolly3 },
	{ 0xd506, 0xd507, taito_colorbank_w, &taito_colorbank },
	{ 0xd508, 0xd508, taito_collision_detection_w },
	{ 0xd509, 0xd50a, MWA_RAM, &taito_gfxpointer },
	{ 0xd50b, 0xd50b, taito_soundcommand_w },
	{ 0xd50d, 0xd50d, watchdog_reset_w },
	{ 0xd50e, 0xd50e, taito_bankswitch_w },
	{ 0xd600, 0xd600, taito_videoenable_w },
	{ 0xe000, 0xefff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x4801, 0x4801, AY8910_read_port_1_r },
	{ 0x4803, 0x4803, AY8910_read_port_2_r },
	{ 0x4805, 0x4805, AY8910_read_port_3_r },
	{ 0x5000, 0x5000, soundlatch_r },
	{ 0xe000, 0xefff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x4800, 0x4800, AY8910_control_port_1_w },
	{ 0x4801, 0x4801, AY8910_write_port_1_w },
	{ 0x4802, 0x4802, AY8910_control_port_2_w },
	{ 0x4803, 0x4803, AY8910_write_port_2_w },
	{ 0x4804, 0x4804, AY8910_control_port_3_w },
	{ 0x4805, 0x4805, AY8910_write_port_3_w },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress m68705_readmem[] =
{
	{ 0x0000, 0x0000, taito_68705_portA_r },
	{ 0x0001, 0x0001, taito_68705_portB_r },
	{ 0x0002, 0x0002, taito_68705_portC_r },
	{ 0x0003, 0x007f, MRA_RAM },
	{ 0x0080, 0x07ff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress m68705_writemem[] =
{
	{ 0x0000, 0x0000, taito_68705_portA_w },
	{ 0x0001, 0x0001, taito_68705_portB_w },
	{ 0x0003, 0x007f, MWA_RAM },
	{ 0x0080, 0x07ff, MWA_ROM },
	{ -1 }	/* end of table */
};



#define DSW2_PORT \
	PORT_DIPNAME( 0x0f, 0x00, "Coin A", IP_KEY_NONE ) \
	PORT_DIPSETTING(    0x0f, "9 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0x0e, "8 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0x0d, "7 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0x0c, "6 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0x0b, "5 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0x0a, "4 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0x09, "3 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )  \
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" ) \
	PORT_DIPSETTING(    0x02, "1 Coin/3 Credits" ) \
	PORT_DIPSETTING(    0x03, "1 Coin/4 Credits" ) \
	PORT_DIPSETTING(    0x04, "1 Coin/5 Credits" ) \
	PORT_DIPSETTING(    0x05, "1 Coin/6 Credits" ) \
	PORT_DIPSETTING(    0x06, "1 Coin/7 Credits" ) \
	PORT_DIPSETTING(    0x07, "1 Coin/8 Credits" ) \
	PORT_DIPNAME( 0xf0, 0x00, "Coin B", IP_KEY_NONE ) \
	PORT_DIPSETTING(    0xf0, "9 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0xe0, "8 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0xd0, "7 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0xc0, "6 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0xb0, "5 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0xa0, "4 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0x90, "3 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" ) \
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )  \
	PORT_DIPSETTING(    0x10, "1 Coin/2 Credits" ) \
	PORT_DIPSETTING(    0x20, "1 Coin/3 Credits" ) \
	PORT_DIPSETTING(    0x30, "1 Coin/4 Credits" ) \
	PORT_DIPSETTING(    0x40, "1 Coin/5 Credits" ) \
	PORT_DIPSETTING(    0x50, "1 Coin/6 Credits" ) \
	PORT_DIPSETTING(    0x60, "1 Coin/7 Credits" ) \
	PORT_DIPSETTING(    0x70, "1 Coin/8 Credits" )


INPUT_PORTS_START( spaceskr_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x18, 0x18, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPSETTING(    0x18, "6" )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x20, "Yes" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( junglek_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Finish Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "None" )
	PORT_DIPSETTING(    0x02, "Timer x1" )
	PORT_DIPSETTING(    0x01, "Timer x2" )
	PORT_DIPSETTING(    0x00, "Timer x3" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown DSW A 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x18, 0x18, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x03, 0x03, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "10000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x00, "30000" )
	PORT_DIPSETTING(    0x03, "None" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown DSW C 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown DSW C 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown DSW C 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x20, "Yes" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Infinite Lives", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( alpine_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL, "2 Fast", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON1, "Fast", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x1e, 0x00, IPT_UNUSED )				/* protection read */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )		/* flips screen */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Jump Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "500-1500" )
	PORT_DIPSETTING(    0x01, "800-2000" )
	PORT_DIPSETTING(    0x02, "1000-2500" )
	PORT_DIPSETTING(    0x03, "2000-4000" )
	PORT_BIT( 			0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x18, 0x18, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1:00" )
	PORT_DIPSETTING(    0x08, "1:30" )
	PORT_DIPSETTING(    0x10, "2:00" )
	PORT_DIPSETTING(    0x18, "2:30" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x03, 0x03, "1st Extended Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10k" )
	PORT_DIPSETTING(    0x01, "15k" )
	PORT_DIPSETTING(    0x02, "20k" )
	PORT_DIPSETTING(    0x03, "25k" )
	PORT_DIPNAME( 0x1c, 0x1c, "Extended Time Every", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5k" )
	PORT_DIPSETTING(    0x04, "6k" )
	PORT_DIPSETTING(    0x08, "7k" )
	PORT_DIPSETTING(    0x0c, "8k" )
	PORT_DIPSETTING(    0x10, "9k" )
	PORT_DIPSETTING(    0x14, "10k" )
	PORT_DIPSETTING(    0x18, "11k" )
	PORT_DIPSETTING(    0x1c, "12k" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x20, "Yes" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( alpinea_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL, "2 Fast", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON1, "Fast", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x0f, 0x00, IPT_UNUSED )				/* protection read */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )		/* flips screen */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Jump Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "500-1500" )
	PORT_DIPSETTING(    0x01, "800-2000" )
	PORT_DIPSETTING(    0x02, "1000-2500" )
	PORT_DIPSETTING(    0x03, "2000-4000" )
	PORT_BIT( 			0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x18, 0x18, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1:00" )
	PORT_DIPSETTING(    0x08, "1:30" )
	PORT_DIPSETTING(    0x10, "2:00" )
	PORT_DIPSETTING(    0x18, "2:30" )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x03, 0x03, "1st Extended Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10k" )
	PORT_DIPSETTING(    0x01, "15k" )
	PORT_DIPSETTING(    0x02, "20k" )
	PORT_DIPSETTING(    0x03, "25k" )
	PORT_DIPNAME( 0x1c, 0x1c, "Extended Time Every", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5k" )
	PORT_DIPSETTING(    0x04, "6k" )
	PORT_DIPSETTING(    0x08, "7k" )
	PORT_DIPSETTING(    0x0c, "8k" )
	PORT_DIPSETTING(    0x10, "9k" )
	PORT_DIPSETTING(    0x14, "10k" )
	PORT_DIPSETTING(    0x18, "11k" )
	PORT_DIPSETTING(    0x1c, "12k" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x20, "Yes" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( timetunl_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x18, 0x18, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x20, "Yes" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( frontlin_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "10000" )
	PORT_DIPSETTING(    0x02, "20000" )
	PORT_DIPSETTING(    0x01, "30000" )
	PORT_DIPSETTING(    0x00, "50000" )
	PORT_DIPNAME( 0x04, 0x04, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x18, 0x18, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_BITX(    0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, "Unknown DSW C 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown DSW C 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown DSW C 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown DSW C 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Coins/Credits" )
	PORT_DIPSETTING(    0x00, "Insert Coin" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x20, "Yes" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( elevator_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "10000" )
	PORT_DIPSETTING(    0x02, "15000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x00, "24000" )
	PORT_DIPNAME( 0x04, 0x04, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x18, 0x18, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown DSW A 6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easiest" )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x01, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown DSW C 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown DSW C 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Coins/Credits" )
	PORT_DIPSETTING(    0x00, "Insert Coin" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x20, "Yes" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( tinstar_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Bonus Life?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "10000?" )
	PORT_DIPSETTING(    0x02, "20000?" )
	PORT_DIPSETTING(    0x01, "30000?" )
	PORT_DIPSETTING(    0x00, "50000?" )
	PORT_DIPNAME( 0x04, 0x04, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x18, 0x10, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, "Unknown DSW C 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown DSW C 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown DSW C 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown DSW C 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Coins/Credits" )
	PORT_DIPSETTING(    0x00, "Insert Coin" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x20, "Yes" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END

INPUT_PORTS_START( waterski_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON1, "Slow", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON2, "Jump", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL, "2 Slow", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL, "2 Jump", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* Service */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x18, 0x18, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2:00" )
	PORT_DIPSETTING(    0x08, "2:10" )
	PORT_DIPSETTING(    0x10, "2:20" )
	PORT_DIPSETTING(    0x18, "2:30" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START      /* DSW2 Coinage */
	DSW2_PORT

	PORT_START      /* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Coinage Display", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Coins/Credits" )
	PORT_DIPSETTING(    0x00, "Insert Coin" )
	PORT_DIPNAME( 0x20, 0x20, "Year Display", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x20, "Yes" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "A and B" )
	PORT_DIPSETTING(    0x00, "A only" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel */
	{ 512*8*8, 256*8*8, 0 },	/* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	3,	/* 3 bits per pixel */
	{ 128*16*16, 64*16*16, 0 },	/* the bitplanes are separated */
	{ 7, 6, 5, 4, 3, 2, 1, 0,
		8*8+7, 8*8+6, 8*8+5, 8*8+4, 8*8+3, 8*8+2, 8*8+1, 8*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0, 0x9000, &charlayout,   0, 16 },	/* the game dynamically modifies this */
	{ 0, 0x9000, &spritelayout, 0, 16 },	/* the game dynamically modifies this */
	{ 0, 0xa800, &charlayout,   0, 16 },	/* the game dynamically modifies this */
	{ 0, 0xa800, &spritelayout, 0, 16 },	/* the game dynamically modifies this */
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	4,	/* 4 chips */
	6000000/4,	/* 1.5 MHz */
	{ 255, 255, 255, 0x20ff },
	{ input_port_6_r, 0, 0, 0 },		/* port Aread */
	{ input_port_7_r, 0, 0, 0 },		/* port Bread */
	{ 0, DAC_data_w, 0, 0 },		/* port Awrite */
	{ 0, 0, 0, taito_sndnmi_msk }	/* port Bwrite */
};

static struct DACinterface dac_interface =
{
	1,
	{ 255 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			8000000/2,	/* 4 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			6000000/2,	/* 3 Mhz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			/* interrupts: */
			/* - no interrupts synced with vblank */
			/* - NMI triggered by the main CPU */
			/* - periodic IRQ, with frequency 6000000/(4*16*16*10*16) = 36.621 Hz, */
			/*   that is a period of 27306666.6666 ns */
			0,0,
			interrupt,27306667
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	taito_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	64, 16*8,
	taito_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	taito_vh_start,
	taito_vh_stop,
	taito_vh_screenrefresh,

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

/* same as above, but with additional 68705 MCU */
static struct MachineDriver mcu_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			8000000/2,	/* 4 Mhz */
			0,
			mcu_readmem,mcu_writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			6000000/2,	/* 3 Mhz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			/* interrupts: */
			/* - no interrupts synced with vblank */
			/* - NMI triggered by the main CPU */
			/* - periodic IRQ, with frequency 6000000/(4*16*16*10*16) = 36.621 Hz, */
			/*   that is a period of 27306666.6666 ns */
			0,0,
			interrupt,27306667
		},
		{
			CPU_M68705,
			3000000,	/* xtal is 3MHz, is it demultiplied internally? */
			4,
			m68705_readmem,m68705_writemem,0,0,
			ignore_interrupt,0	/* IRQs are caused by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	100,	/* 100 CPU slices per frame - an high value to ensure proper */
			/* synchronization of the CPUs */
	taito_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	64, 16*8,
	taito_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,
	taito_vh_start,
	taito_vh_stop,
	taito_vh_screenrefresh,

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



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( spaceskr_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "eb01",         0x0000, 0x1000, 0x92345b05 )
	ROM_LOAD( "eb02",         0x1000, 0x1000, 0xa3e21420 )
	ROM_LOAD( "eb03",         0x2000, 0x1000, 0xa077c52f )
	ROM_LOAD( "eb04",         0x3000, 0x1000, 0x440030cf )
	ROM_LOAD( "eb05",         0x4000, 0x1000, 0xb0d396ab )
	ROM_LOAD( "eb06",         0x5000, 0x1000, 0x371d2f7a )
	ROM_LOAD( "eb07",         0x6000, 0x1000, 0x13e667c4 )
	ROM_LOAD( "eb08",         0x7000, 0x1000, 0xf2e84015 )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "eb09",         0x0000, 0x1000, 0x77af540e )
	ROM_LOAD( "eb10",         0x1000, 0x1000, 0xb10073de )
	ROM_LOAD( "eb11",         0x2000, 0x1000, 0xc7954bd1 )
	ROM_LOAD( "eb12",         0x3000, 0x1000, 0xcd6c087b )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "eb13",         0x0000, 0x1000, 0x192f6536 )
	ROM_LOAD( "eb14",         0x1000, 0x1000, 0xd04d0a21 )
	ROM_LOAD( "eb15",         0x2000, 0x1000, 0x88194305 )
ROM_END

ROM_START( junglek_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "kn41.bin",     0x00000, 0x1000, 0x7e4cd631 )
	ROM_LOAD( "kn42.bin",     0x01000, 0x1000, 0xbade53af )
	ROM_LOAD( "kn43.bin",     0x02000, 0x1000, 0xa20e5a48 )
	ROM_LOAD( "kn44.bin",     0x03000, 0x1000, 0x44c770d3 )
	ROM_LOAD( "kn45.bin",     0x04000, 0x1000, 0xf60a3d06 )
	ROM_LOAD( "kn46.bin",     0x05000, 0x1000, 0x27a95fd5 )
	ROM_LOAD( "kn47.bin",     0x06000, 0x1000, 0x5c3199e0 )
	ROM_LOAD( "kn48.bin",     0x07000, 0x1000, 0xe690b36e )
	/* 10000-10fff space for another banked ROM (not used) */
	ROM_LOAD( "kn60.bin",     0x11000, 0x1000, 0x1a9c0a26 )	/* banked at 7000 */

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "kn49.bin",     0x0000, 0x1000, 0xfe275213 )
	ROM_LOAD( "kn50.bin",     0x1000, 0x1000, 0xd9f93c55 )
	ROM_LOAD( "kn51.bin",     0x2000, 0x1000, 0x70e8fc12 )
	ROM_LOAD( "kn52.bin",     0x3000, 0x1000, 0xbcbac1a3 )
	ROM_LOAD( "kn53.bin",     0x4000, 0x1000, 0xb946c87d )
	ROM_LOAD( "kn54.bin",     0x5000, 0x1000, 0xf757d8f0 )
	ROM_LOAD( "kn55.bin",     0x6000, 0x1000, 0x70aef58f )
	ROM_LOAD( "kn56.bin",     0x7000, 0x1000, 0x932eb667 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "kn57-1.bin",   0x0000, 0x1000, 0x62f6763a )
	ROM_LOAD( "kn58-1.bin",   0x1000, 0x1000, 0x9ef46c7f )
	ROM_LOAD( "kn59-1.bin",   0x2000, 0x1000, 0xcee485fc )
ROM_END

ROM_START( jhunt_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "kn41a",        0x00000, 0x1000, 0x6bf118d8 )
	ROM_LOAD( "kn42.bin",     0x01000, 0x1000, 0xbade53af )
	ROM_LOAD( "kn43.bin",     0x02000, 0x1000, 0xa20e5a48 )
	ROM_LOAD( "kn44.bin",     0x03000, 0x1000, 0x44c770d3 )
	ROM_LOAD( "kn45.bin",     0x04000, 0x1000, 0xf60a3d06 )
	ROM_LOAD( "kn46a",        0x05000, 0x1000, 0xac89c155 )
	ROM_LOAD( "kn47.bin",     0x06000, 0x1000, 0x5c3199e0 )
	ROM_LOAD( "kn48a",        0x07000, 0x1000, 0xef80e931 )
	/* 10000-10fff space for another banked ROM (not used) */
	ROM_LOAD( "kn60.bin",     0x11000, 0x1000, 0x1a9c0a26 )	/* banked at 7000 */

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "kn49a",        0x0000, 0x1000, 0xb139e792 )
	ROM_LOAD( "kn50a",        0x1000, 0x1000, 0x1046019f )
	ROM_LOAD( "kn51a",        0x2000, 0x1000, 0xda50c8a4 )
	ROM_LOAD( "kn52a",        0x3000, 0x1000, 0x0444f06c )
	ROM_LOAD( "kn53a",        0x4000, 0x1000, 0x6a17803e )
	ROM_LOAD( "kn54a",        0x5000, 0x1000, 0xd41428c7 )
	ROM_LOAD( "kn55.bin",     0x6000, 0x1000, 0x70aef58f )
	ROM_LOAD( "kn56a",        0x7000, 0x1000, 0x679c1101 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "kn57-1.bin",   0x0000, 0x1000, 0x62f6763a )
	ROM_LOAD( "kn58-1.bin",   0x1000, 0x1000, 0x9ef46c7f )
	ROM_LOAD( "kn59-1.bin",   0x2000, 0x1000, 0xcee485fc )
ROM_END

ROM_START( alpine_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "rh16.069",     0x0000, 0x1000, 0x6b2a69b7 )
	ROM_LOAD( "rh17.068",     0x1000, 0x1000, 0xe344b0b7 )
	ROM_LOAD( "rh18.067",     0x2000, 0x1000, 0x753bdd87 )
	ROM_LOAD( "rh19.066",     0x3000, 0x1000, 0x3efb3fcd )
	ROM_LOAD( "rh20.065",     0x4000, 0x1000, 0xc2cd4e79 )
	ROM_LOAD( "rh21.064",     0x5000, 0x1000, 0x74109145 )
	ROM_LOAD( "rh22.055",     0x6000, 0x1000, 0xefa82a57 )
	ROM_LOAD( "rh23.054",     0x7000, 0x1000, 0x77c25acf )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "rh24.001",     0x0000, 0x1000, 0x4b1d9455 )
	ROM_LOAD( "rh25.002",     0x1000, 0x1000, 0xbf71e278 )
	ROM_LOAD( "rh26.003",     0x2000, 0x1000, 0x13da2a9b )
	ROM_LOAD( "rh27.004",     0x3000, 0x1000, 0x425b52b0 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "rh13.070",     0x0000, 0x1000, 0xdcad1794 )
ROM_END

ROM_START( alpinea_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "rh01-1.69",    0x0000, 0x1000, 0x7fbcb635 )
	ROM_LOAD( "rh02.68",      0x1000, 0x1000, 0xc83f95af )
	ROM_LOAD( "rh03.67",      0x2000, 0x1000, 0x211102bc )
	ROM_LOAD( "rh04-1.66",    0x3000, 0x1000, 0x494a91b0 )
	ROM_LOAD( "rh05.65",      0x4000, 0x1000, 0xd85588be )
	ROM_LOAD( "rh06.64",      0x5000, 0x1000, 0x521fddb9 )
	ROM_LOAD( "rh07.55",      0x6000, 0x1000, 0x51f369a4 )
	ROM_LOAD( "rh08.54",      0x7000, 0x1000, 0xe0af9cb2 )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "rh24.001",     0x0000, 0x1000, 0x4b1d9455 )
	ROM_LOAD( "rh25.002",     0x1000, 0x1000, 0xbf71e278 )
	ROM_LOAD( "rh26.003",     0x2000, 0x1000, 0x13da2a9b )
	ROM_LOAD( "rh12.4",       0x3000, 0x1000, 0x0ff0d1fe )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "rh13.070",     0x0000, 0x1000, 0xdcad1794 )
ROM_END

ROM_START( timetunl_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "un01.69",     0x00000, 0x1000, 0x2e56d946 )
	ROM_LOAD( "un02.68",     0x01000, 0x1000, 0xf611d852 )
	ROM_LOAD( "un03.67",     0x02000, 0x1000, 0x144b5e7f )
	ROM_LOAD( "un04.66",     0x03000, 0x1000, 0xb6767eba )
	ROM_LOAD( "un05.65",     0x04000, 0x1000, 0x91e3c558 )
	ROM_LOAD( "un06.64",     0x05000, 0x1000, 0xaf5a7d2a )
	ROM_LOAD( "un07.55",     0x06000, 0x1000, 0x4ee50999 )
	ROM_LOAD( "un08.54",     0x07000, 0x1000, 0x97259b57 )
	ROM_LOAD( "un09.53",     0x10000, 0x1000, 0x771d0fb0 )	/* banked at 6000 */
	ROM_LOAD( "un10.52",     0x11000, 0x1000, 0x8b6afad2 )	/* banked at 7000 */

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "un11.1",     0x0000, 0x1000, 0x3be4fed6 )
	ROM_LOAD( "un12.2",     0x1000, 0x1000, 0x2dee1cf3 )
	ROM_LOAD( "un13.3",     0x2000, 0x1000, 0x72b491a8 )
	ROM_LOAD( "un14.4",     0x3000, 0x1000, 0x5f695369 )
	ROM_LOAD( "un15.5",     0x4000, 0x1000, 0x001df94b )
	ROM_LOAD( "un16.6",     0x5000, 0x1000, 0xe33b9019 )
	ROM_LOAD( "un17.7",     0x6000, 0x1000, 0xd66025b8 )
	ROM_LOAD( "un18.8",     0x7000, 0x1000, 0xe67ff377 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "un19.70",     0x0000, 0x1000, 0xdbf726c6 )
ROM_END

ROM_START( wwestern_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "ww01.bin",     0x0000, 0x1000, 0xbfe10753 )
	ROM_LOAD( "ww02d.bin",    0x1000, 0x1000, 0x20579e90 )
	ROM_LOAD( "ww03d.bin",    0x2000, 0x1000, 0x0e65be37 )
	ROM_LOAD( "ww04d.bin",    0x3000, 0x1000, 0xb3565a31 )
	ROM_LOAD( "ww05d.bin",    0x4000, 0x1000, 0xbe46feb2 )
	ROM_LOAD( "ww06d.bin",    0x5000, 0x1000, 0xc81c9736 )
	ROM_LOAD( "ww07.bin",     0x6000, 0x1000, 0x1937cc17 )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "ww08.bin",     0x0000, 0x1000, 0x041a5a1c )
	ROM_LOAD( "ww09.bin",     0x1000, 0x1000, 0x07982ac5 )
	ROM_LOAD( "ww10.bin",     0x2000, 0x1000, 0xf32ae203 )
	ROM_LOAD( "ww11.bin",     0x3000, 0x1000, 0x7ff1431f )
	ROM_LOAD( "ww12.bin",     0x4000, 0x1000, 0xbe1b563a )
	ROM_LOAD( "ww13.bin",     0x5000, 0x1000, 0x092cd9e5 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ww14.bin",     0x0000, 0x1000, 0x23776870 )
ROM_END

ROM_START( frontlin_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "fl69.u69",     0x00000, 0x1000, 0x93b64599 )
	ROM_LOAD( "fl68.u68",     0x01000, 0x1000, 0x82dccdfb )
	ROM_LOAD( "fl67.u67",     0x02000, 0x1000, 0x3fa1ba12 )
	ROM_LOAD( "fl66.u66",     0x03000, 0x1000, 0x4a3db285 )
	ROM_LOAD( "fl65.u65",     0x04000, 0x1000, 0xda00ec70 )
	ROM_LOAD( "fl64.u64",     0x05000, 0x1000, 0x9fc90a20 )
	ROM_LOAD( "fl55.u55",     0x06000, 0x1000, 0x359242c2 )
	ROM_LOAD( "fl54.u54",     0x07000, 0x1000, 0xd234c60f )
	ROM_LOAD( "aa1_10.8",     0x0e000, 0x1000, 0x2704aa4c )
	ROM_LOAD( "fl53.u53",     0x10000, 0x1000, 0x67429975 )	/* banked at 6000 */
	ROM_LOAD( "fl52.u52",     0x11000, 0x1000, 0xcb223d34 )	/* banked at 7000 */

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "fl1.u1",       0x0000, 0x1000, 0xe82c9f46 )
	ROM_LOAD( "fl2.u2",       0x1000, 0x1000, 0x123055d3 )
	ROM_LOAD( "fl3.u3",       0x2000, 0x1000, 0x7ea46347 )
	ROM_LOAD( "fl4.u4",       0x3000, 0x1000, 0x9e2cff10 )
	ROM_LOAD( "fl5.u5",       0x4000, 0x1000, 0x630b4be1 )
	ROM_LOAD( "fl6.u6",       0x5000, 0x1000, 0x9e092d58 )
	ROM_LOAD( "fl7.u7",       0x6000, 0x1000, 0x613682a3 )
	ROM_LOAD( "fl8.u8",       0x7000, 0x1000, 0xf73b0d5e )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "fl70.u70",     0x0000, 0x1000, 0x15f4ed8c )
	ROM_LOAD( "fl71.u71",     0x1000, 0x1000, 0xc3eb38e7 )

	ROM_REGION(0x0800)	/* 8k for the microcontroller */
	ROM_LOAD( "aa1.13",       0x0080, 0x0780, 0xa5273366 )
ROM_END

ROM_START( elevator_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "ea-ic69.bin",  0x0000, 0x1000, 0x24e277ef )
	ROM_LOAD( "ea-ic68.bin",  0x1000, 0x1000, 0x13702e39 )
	ROM_LOAD( "ea-ic67.bin",  0x2000, 0x1000, 0x46f52646 )
	ROM_LOAD( "ea-ic66.bin",  0x3000, 0x1000, 0xe22fe57e )
	ROM_LOAD( "ea-ic65.bin",  0x4000, 0x1000, 0xc10691d7 )
	ROM_LOAD( "ea-ic64.bin",  0x5000, 0x1000, 0x8913b293 )
	ROM_LOAD( "ea-ic55.bin",  0x6000, 0x1000, 0x1cabda08 )
	ROM_LOAD( "ea-ic54.bin",  0x7000, 0x1000, 0xf4647b4f )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "ea-ic1.bin",   0x0000, 0x1000, 0xbbbb3fba )
	ROM_LOAD( "ea-ic2.bin",   0x1000, 0x1000, 0x639cc2fd )
	ROM_LOAD( "ea-ic3.bin",   0x2000, 0x1000, 0x61317eea )
	ROM_LOAD( "ea-ic4.bin",   0x3000, 0x1000, 0x55446482 )
	ROM_LOAD( "ea-ic5.bin",   0x4000, 0x1000, 0x77895c0f )
	ROM_LOAD( "ea-ic6.bin",   0x5000, 0x1000, 0x9a1b6901 )
	ROM_LOAD( "ea-ic7.bin",   0x6000, 0x1000, 0x839112ec )
	ROM_LOAD( "ea-ic8.bin",   0x7000, 0x1000, 0xdb7ff692 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ea-ic70.bin",  0x0000, 0x1000, 0x6d5f57cb )
	ROM_LOAD( "ea-ic71.bin",  0x1000, 0x1000, 0xf0a769a1 )

	ROM_REGION(0x0800)	/* 8k for the microcontroller */
	ROM_LOAD( "ba3.11",       0x0080, 0x0780, 0x47b8d449 )
ROM_END

ROM_START( elevatob_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "ea69.bin",     0x0000, 0x1000, 0x66baa214 )
	ROM_LOAD( "ea-ic68.bin",  0x1000, 0x1000, 0x13702e39 )
	ROM_LOAD( "ea-ic67.bin",  0x2000, 0x1000, 0x46f52646 )
	ROM_LOAD( "ea66.bin",     0x3000, 0x1000, 0xb88f3383 )
	ROM_LOAD( "ea-ic65.bin",  0x4000, 0x1000, 0xc10691d7 )
	ROM_LOAD( "ea-ic64.bin",  0x5000, 0x1000, 0x8913b293 )
	ROM_LOAD( "ea55.bin",     0x6000, 0x1000, 0xd546923e )
	ROM_LOAD( "ea54.bin",     0x7000, 0x1000, 0x963ec5a5 )
	/* 10000-10fff space for another banked ROM (not used) */
	ROM_LOAD( "ea52.bin",     0x11000, 0x1000, 0x44b1314a )	/* protection crack, bank switched at 7000 */

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "ea-ic1.bin",   0x0000, 0x1000, 0xbbbb3fba )
	ROM_LOAD( "ea-ic2.bin",   0x1000, 0x1000, 0x639cc2fd )
	ROM_LOAD( "ea-ic3.bin",   0x2000, 0x1000, 0x61317eea )
	ROM_LOAD( "ea-ic4.bin",   0x3000, 0x1000, 0x55446482 )
	ROM_LOAD( "ea-ic5.bin",   0x4000, 0x1000, 0x77895c0f )
	ROM_LOAD( "ea-ic6.bin",   0x5000, 0x1000, 0x9a1b6901 )
	ROM_LOAD( "ea-ic7.bin",   0x6000, 0x1000, 0x839112ec )
	ROM_LOAD( "ea08.bin",     0x7000, 0x1000, 0x67ebf7c1 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ea-ic70.bin",  0x0000, 0x1000, 0x6d5f57cb )
	ROM_LOAD( "ea-ic71.bin",  0x1000, 0x1000, 0xf0a769a1 )
ROM_END

ROM_START( tinstar_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "ts.69",        0x0000, 0x1000, 0xa930af60 )
	ROM_LOAD( "ts.68",        0x1000, 0x1000, 0x7f2714ca )
	ROM_LOAD( "ts.67",        0x2000, 0x1000, 0x49170786 )
	ROM_LOAD( "ts.66",        0x3000, 0x1000, 0x3766f130 )
	ROM_LOAD( "ts.65",        0x4000, 0x1000, 0x41251246 )
	ROM_LOAD( "ts.64",        0x5000, 0x1000, 0x812285d5 )
	ROM_LOAD( "ts.55",        0x6000, 0x1000, 0x6b80ac51 )
	ROM_LOAD( "ts.54",        0x7000, 0x1000, 0xb352360f )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "ts.1",         0x0000, 0x1000, 0xf1160718 )
	ROM_LOAD( "ts.2",         0x1000, 0x1000, 0x39dc6dbb )
	ROM_LOAD( "ts.3",         0x2000, 0x1000, 0x079df429 )
	ROM_LOAD( "ts.4",         0x3000, 0x1000, 0xe61105d4 )
	ROM_LOAD( "ts.5",         0x4000, 0x1000, 0xffab5d15 )
	ROM_LOAD( "ts.6",         0x5000, 0x1000, 0xf1d8ca36 )
	ROM_LOAD( "ts.7",         0x6000, 0x1000, 0x894f6332 )
	ROM_LOAD( "ts.8",         0x7000, 0x1000, 0x519aed19 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ts.70",        0x0000, 0x1000, 0x4771838d )
	ROM_LOAD( "ts.71",        0x1000, 0x1000, 0x03c91332 )
	ROM_LOAD( "ts.72",        0x2000, 0x1000, 0xbeeed8f3 )

	ROM_REGION(0x0800)	/* 8k for the microcontroller */
	ROM_LOAD( "a10-12",       0x0080, 0x0780, 0x53c1617c )
ROM_END

ROM_START( waterski_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "a03-01",       0x0000, 0x1000, 0x322c4c2c )
	ROM_LOAD( "a03-02",       0x1000, 0x1000, 0x8df176d1 )
	ROM_LOAD( "a03-03",       0x2000, 0x1000, 0x420bd04f )
	ROM_LOAD( "a03-04",       0x3000, 0x1000, 0x5c081a94 )
	ROM_LOAD( "a03-05",       0x4000, 0x1000, 0x1fae90d2 )
	ROM_LOAD( "a03-06",       0x5000, 0x1000, 0x55b7c151 )
	ROM_LOAD( "a03-07",       0x6000, 0x1000, 0x8abc7522 )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "a03-08",       0x0000, 0x1000, 0xc206d870 )	/* minor bit rot */
	ROM_LOAD( "a03-09",       0x1000, 0x1000, 0x48ac912a )
	ROM_LOAD( "a03-10",       0x2000, 0x1000, 0xa8cbb3e5 )	/* corrupt! */
	ROM_LOAD( "a03-11",       0x3000, 0x1000, 0xf06cddd6 )
	ROM_LOAD( "a03-12",       0x4000, 0x1000, 0x27dfd8c2 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "a03-13",       0x0000, 0x1000, 0x78c7d37f )
	ROM_LOAD( "a03-14",       0x1000, 0x1000, 0x31f991ca )
ROM_END

ROM_START( bioatack_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "aa8-01.69",     0x0000, 0x1000, 0xe5abc211 )
	ROM_LOAD( "aa8-02.68",     0x1000, 0x1000, 0xb5bfde00 )
	ROM_LOAD( "aa8-03.67",     0x2000, 0x1000, 0xe4e46e69 )
	ROM_LOAD( "aa8-04.66",     0x3000, 0x1000, 0x86e0af8c )
	ROM_LOAD( "aa8-05.65",     0x4000, 0x1000, 0xc6248608 )
	ROM_LOAD( "aa8-06.64",     0x5000, 0x1000, 0x685a0383 )
	ROM_LOAD( "aa8-07.55",     0x6000, 0x1000, 0x9d58e2b7 )
	ROM_LOAD( "aa8-08.54",     0x7000, 0x1000, 0xdec5271f )
	/* 10000-11fff space for banked ROMs (not used) */

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "aa8-09.1",     0x0000, 0x1000, 0x1fee5fd6 )
	ROM_LOAD( "aa8-10.2",     0x1000, 0x1000, 0xe0133423 )
	ROM_LOAD( "aa8-11.3",     0x2000, 0x1000, 0x0f5715c6 )
	ROM_LOAD( "aa8-12.4",     0x3000, 0x1000, 0x71126dd0 )
	ROM_LOAD( "aa8-13.5",     0x4000, 0x1000, 0xadcdd2f0 )
	ROM_LOAD( "aa8-14.6",     0x5000, 0x1000, 0x2fe18680 )
	ROM_LOAD( "aa8-15.7",     0x6000, 0x1000, 0xff5aad4b )
	ROM_LOAD( "aa8-16.8",     0x7000, 0x1000, 0xceba4036 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "aa8-17.70",     0x0000, 0x1000, 0x36eb95b5 )
ROM_END

ROM_START( sfposeid_rom )
	ROM_REGION(0x12000)	/* 64k for code */
	ROM_LOAD( "a14-01.1",     0x00000, 0x2000, 0xaa779fbb )
	ROM_LOAD( "a14-02.2",     0x02000, 0x2000, 0xecec9dc3 )
	ROM_LOAD( "a14-03.3",     0x04000, 0x2000, 0x469498c1 )
	ROM_LOAD( "a14-04.6",     0x06000, 0x2000, 0x1db4bc02 )
	ROM_LOAD( "a14-05.7",     0x10000, 0x2000, 0x95e2f903 )	/* banked at 6000 */

	ROM_REGION_DISPOSE(0x1000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x8000)	/* graphic ROMs */
	ROM_LOAD( "a14-06.4",     0x0000, 0x2000, 0x9740493b )
	ROM_LOAD( "a14-07.5",     0x2000, 0x2000, 0x1c93de97 )
	ROM_LOAD( "a14-08.9",     0x4000, 0x2000, 0x4367e65a )
	ROM_LOAD( "a14-09.10",    0x6000, 0x2000, 0x677cffd5 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "a14-10.70",    0x0000, 0x1000, 0xf1365f35 )
	ROM_LOAD( "a14-11.71",    0x1000, 0x1000, 0x74a12fe2 )
ROM_END



static int junglek_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0x816B],"\x00\x50\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0x816B],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void junglek_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x816B],3);
		osd_fclose(f);
	}

}

static int alpine_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8082],"\x00\x50\x00",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x8082],3);
			osd_fclose(f);

			/* WE MUST ALSO COPY THE SCORE TO THE SCREEN (skip leading zeros) */
			if (RAM[0x8084] > 0xf)
				RAM[0xc5be]=(RAM[0x8084] >> 4);
			if (RAM[0x8084] != 0)
				RAM[0xc5de]=(RAM[0x8084] & 0x0f);
			RAM[0xc5fe]=(RAM[0x8083] >> 4);
			RAM[0xc61e]=(RAM[0x8083] & 0x0f);
			RAM[0xc63e]=(RAM[0x8082] >> 4);
			RAM[0xc65e]=(RAM[0x8082] & 0x0f);
		}

		return 1;
	 }
	 else return 0; /* we can't load the hi scores yet */
}

static void alpine_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8082],3);
		osd_fclose(f);
	}
}

static int frontlin_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8640],"\x01\x00\x00",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x8640],3);
			osd_fclose(f);

			/* WE MUST ALSO COPY THE SCORE TO THE SCREEN*/
			RAM[0xc5be]=(RAM[0x8640] >> 4);
			RAM[0xc5de]=(RAM[0x8640] & 0x0f);
			RAM[0xc5fe]=(RAM[0x8641] >> 4);
			RAM[0xc61e]=(RAM[0x8641] & 0x0f);
			RAM[0xc63e]=(RAM[0x8642] >> 4);
			RAM[0xc65e]=(RAM[0x8642] & 0x0f);
		}

		return 1;
	 }
	 else return 0; /* we can't load the hi scores yet */
}

static void frontlin_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8640],3);
		osd_fclose(f);
	}
}

static int elevator_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* wait for the high score to initialize */
	if (memcmp(&RAM[0x8350],"\x00\x00\x01",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x8350],3);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void elevator_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x8350],3);
		osd_fclose(f);
	}
}



struct GameDriver spaceskr_driver =
{
	__FILE__,
	0,
	"spaceskr",
	"Space Seeker",
	"1981",
	"Taito",
	"Nicola Salmoria (MAME driver)\nTatsuyuki Satoh (additional code)",
	0,
	&machine_driver,
	0,

	spaceskr_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	spaceskr_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_180,

	0, 0
};

struct GameDriver junglek_driver =
{
	__FILE__,
	0,
	"junglek",
	"Jungle King",
	"1982",
	"Taito",
	"Nicola Salmoria (MAME driver)\nTatsuyuki Satoh (additional code)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	junglek_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	junglek_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	junglek_hiload, junglek_hisave
};

struct GameDriver jhunt_driver =
{
	__FILE__,
	&junglek_driver,
	"jhunt",
	"Jungle Hunt",
	"1982",
	"Taito of America",
	"Nicola Salmoria (MAME driver)\nTatsuyuki Satoh (additional code)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	jhunt_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	junglek_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	junglek_hiload, junglek_hisave
};

struct GameDriver alpine_driver =
{
	__FILE__,
	0,
	"alpine",
	"Alpine Ski (set 1)",
	"1982",
	"Taito",
	"Nicola Salmoria (MAME driver)\nTatsuyuki Satoh (additional code)\nJohn Butler (additional code)",
	0,
	&machine_driver,
	0,

	alpine_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	alpine_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	alpine_hiload, alpine_hisave
};

struct GameDriver alpinea_driver =
{
	__FILE__,
	&alpine_driver,
	"alpinea",
	"Alpine Ski (set 2)",
	"1982",
	"Taito",
	"Nicola Salmoria (MAME driver)\nTatsuyuki Satoh (additional code)\nJohn Butler (additional code)",
	0,
	&machine_driver,
	0,

	alpinea_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	alpinea_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	alpine_hiload, alpine_hisave
};

struct GameDriver timetunl_driver =
{
	__FILE__,
	0,
	"timetunl",
	"Time Tunnel",
	"1982",
	"Taito",
	"Nicola Salmoria (MAME driver)\nTatsuyuki Satoh (additional code)",
	0,
	&machine_driver,
	0,

	timetunl_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	timetunl_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_180,

	0, 0
};

struct GameDriver wwestern_driver =
{
	__FILE__,
	0,
	"wwestern",
	"Wild Western",
	"1982",
	"Taito",
	"Nicola Salmoria (MAME driver)\nTatsuyuki Satoh (additional code)",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	wwestern_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	elevator_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	0, 0
};

struct GameDriver frontlin_driver =
{
	__FILE__,
	0,
	"frontlin",
	"Front Line",
	"1982",
	"Taito",
	"Nicola Salmoria (MAME driver)\nTatsuyuki Satoh (additional code)\nMarco Cassili",
	0,
	&mcu_machine_driver,
	0,

	frontlin_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	frontlin_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	frontlin_hiload, frontlin_hisave
};

struct GameDriver elevator_driver =
{
	__FILE__,
	0,
	"elevator",
	"Elevator Action",
	"1983",
	"Taito",
	"Nicola Salmoria (MAME driver)\nTatsuyuki Satoh (additional code)\nMarco Cassili",
	0,
	&mcu_machine_driver,
	0,

	elevator_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	elevator_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	elevator_hiload, elevator_hisave
};

struct GameDriver elevatob_driver =
{
	__FILE__,
	&elevator_driver,
	"elevatob",
	"Elevator Action (bootleg)",
	"1983",
	"bootleg",
	"Nicola Salmoria (MAME driver)\nTatsuyuki Satoh (additional code)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	elevatob_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	elevator_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	elevator_hiload, elevator_hisave
};

struct GameDriver tinstar_driver =
{
	__FILE__,
	0,
	"tinstar",
	"The Tin Star",
	"1983",
	"Taito",
	"Nicola Salmoria (MAME driver)\nTatsuyuki Satoh (additional code)",
	0,
	&mcu_machine_driver,
	0,

	tinstar_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tinstar_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver waterski_driver =
{
	__FILE__,
	0,
	"waterski",
	"Water Ski",
	"1983",
	"Taito",
	"Nicola Salmoria (MAME driver)\nTatsuyuki Satoh (additional code)",
	0,
	&machine_driver,
	0,

	waterski_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	waterski_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver bioatack_driver =
{
	__FILE__,
	0,
	"bioatack",
	"Bio Attack",
	"1983",
	"Taito (Fox Video Games license)",
	"Nicola Salmoria (MAME driver)\nTatsuyuki Satoh (additional code)",
	0,
	&machine_driver,
	0,

	bioatack_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	alpine_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver sfposeid_driver =
{
	__FILE__,
	0,
	"sfposeid",
	"Sea Fighter Poseidon",
	"1984",
	"Taito",
	"Nicola Salmoria (MAME driver)\nTatsuyuki Satoh (additional code)",
	GAME_NOT_WORKING,
	&machine_driver,
	0,

	sfposeid_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	alpine_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
