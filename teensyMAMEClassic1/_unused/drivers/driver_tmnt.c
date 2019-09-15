/***************************************************************************

Teenage Mutant Ninja Turtles memory map (preliminary)

000000-05ffff ROM
060000-063fff RAM
080000-080fff Palette RAM
0a0000-0bffff input
100000-105fff Video RAM
140400-1407ff Sprite RAM

memory mapped ports:

read:
0A0000/1  Coin/Service
0A0002/3  Player 1
0A0004/5  Player 2
0A0006/7  Player 3
0A0010/1  DIP Switch 1
0A0012/3  DIP Switch 2
0A0014/5  Player 4
0A0016/7  set to 0xFF
0A0018/9  DIP Switch 3

write:
0a0001    bit 0/1 = coin counters
          bit 3 = trigger irq on sound CPU
		  bit 5 = irq enable
		  bit 7 = enable char ROM reading through the video RAM
0a0009    command for sound CPU
0a0011    watchdog reset
0c0001    bit2 = PRI bit3 = PRI2
          sprite/playfield priority is controlled by these two bits, by bit 3
		  of the background tile color code, and by the SHADOW sprite
		  attribute bit.
		  Priorities are encoded in a PROM (G19 for TMNT). However, in TMNT,
		  the PROM only takes into account the PRI and SHADOW bits.
		  PRI  Priority
		   0   bg fg spr text
		   1   bg spr fg text
		  The SHADOW bit, when set, torns a sprite into a shadow which makes
		  color below it darker (this is done by turning off three resistors
		  in parallel with the RGB output).

		  Note: the background color (color used when all of the four layers
		  are 0) is taken from the *foreground* palette, not the background
		  one as would be more intuitive.

106018    fg y scroll
106019    bg y scroll
106b01    Tile Decoder1
106e01    Tile Decoder2
140007    spritecntrl (not sure of this one)


Punk Shot memory map (preliminary)

000000 03ffff ROM
080000 083fff WRKRAM
090000 090fff COLRAM
100000 100fff FIXRAM
102000 105fff VIDRAM (103000-103fff and 105000-105fff don't seem to be used during the game)
110400 1107ff OBJRAM

read:

write:
0a0021        bit 0 = coin counter
              bit 2 = trigger irq on sound CPU
		      bit 3 = enable char ROM reading through the video RAM
0a0041        command for sound CPU
0a0081        watchdog reset
106018        fg y scroll
106019        bg y scroll
106400-1067ff bg x scroll registers; 4 bytes per scanline:
              0:unknown 1:scroll LSB 2:unknown 3:scroll MSB
              NOTE: the visible area is in the range 106440-1067bf.
			  106400-106403 contains the foreground scroll register:
			  0:scroll LSB 1:unused? 2:scroll MSB 3:unused?
106a00        bit 2 = IRQ enable

interrupts: IRQ level 4.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *punkshot_vidram,*punkshot_scrollram;
extern int punkshot_vidram_size;

int tmnt_interrupt(void);
int punkshot_interrupt(void);
void punkshot_irqenable_w(int offset,int data);

void tmnt_paletteram_w(int offset,int data);
int punkshot_vidram_r(int offset);
void punkshot_vidram_w(int offset,int data);
int punkshot_spriteram_r(int offset);
void punkshot_spriteram_w(int offset,int data);
void punkshot_xscroll_w(int offset,int data);
void punkshot_yscroll_w(int offset,int data);
int punkshot_100000_r(int offset);
void punkshot_charromsubbank_w(int offset,int data);
void tmnt_0a0000_w(int offset,int data);
void punkshot_0a0020_w(int offset,int data);
void tmnt_priority_w(int offset,int data);
void punkshot_priority_w(int offset,int data);
void punkshot_charrombank0_w(int offset,int data);
void punkshot_charrombank1_w(int offset,int data);
int punkshot_vh_start (void);
void punkshot_vh_stop (void);
int tmnt_vh_start (void);
void tmnt_vh_stop (void);
void tmnt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void punkshot_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static int tmnt_soundlatch = 0;


void tmnt_sound_command_w(int offset,int data)
{
	soundlatch_w(0,data & 0xff);
}

int tmnt_sres_r(int offset)
{
	return tmnt_soundlatch;
}

void tmnt_sres_w(int offset,int data)
{
	/* bit 1 resets the UPD7795C sound chip */

	/* bit 2 plays the title music */
	if (data & 4)
	{
		if (!sample_playing(0))	sample_start(0,0,0);
	}
	else sample_stop(0);

	tmnt_soundlatch = data;
}

int tmnt_decode_sample(void)
{
	int i;
	signed short *dest;
	unsigned char *source = Machine->memory_region[4];
	struct GameSamples *samples;


	if ((Machine->samples = malloc(sizeof(struct GameSamples))) == NULL)
		return 1;

	samples = Machine->samples;

	if ((samples->sample[0] = malloc(sizeof(struct GameSample) + (0x40000)*sizeof(short))) == NULL)
		return 1;

	samples->sample[0]->length = 0x40000*2;
	samples->sample[0]->volume = 0xff;
	samples->sample[0]->smpfreq = 20000;	/* 20 kHz */
	samples->sample[0]->resolution = 16;
	dest = (signed short *)samples->sample[0]->data;
	samples->total = 1;

	/*	Sound sample for TMNT.D05 is stored in the following mode:
	 *
	 *	Bit 15-13:	Exponent (2 ^ x)
	 *	Bit 12-4 :	Sound data (9 bit)
	 *
	 *	(Sound info courtesy of Dave <dayvee@rocketmail.com>)
	 */

	for (i = 0;i < 0x40000;i++)
	{
		int val = source[2*i] + source[2*i+1] * 256;
		int exp = val >> 13;

	  	val = (val >> 4) & (0x1ff);	/* 9 bit, Max Amplitude 0x200 */
		val -= 0x100;					/* Centralize value	*/

		val <<= exp;

		dest[i] = val;
	}

	/*	The sample is now ready to be used.  It's a 16 bit, 22khz sample.
	 */

	return 0;
}



int tmnt_input_r (int offset)
{
	switch (offset)
	{
		case 0x00:
			return readinputport(0);

		case 0x02:
			return readinputport(1);

		case 0x04:
			return readinputport(2);

		case 0x06:
			return readinputport(3);

		case 0x10:
			return readinputport(4);

		case 0x12:
			return readinputport(5);

		case 0x14:
			return readinputport(6);

		case 0x18:
			return readinputport(7);
	}

if (errorlog) fprintf(errorlog,"CPU #0 PC %06x: warning - read unmapped memory address %06x\n",cpu_getpc(),0x0a0000+offset);

	return 0;
}

int punkshot_input_r(int offset)
{
	switch (offset)
	{
		case 0:	/* DSW1, DSW2 */
			return (readinputport(0) + (readinputport(1) << 8));

		case 2:	/* COIN, DSW3 */
			return (readinputport(2) + (readinputport(3) << 8));

		case 6:	/* IN0, IN1 */
			return (readinputport(4) + (readinputport(5) << 8));
	}

if (errorlog) fprintf(errorlog,"CPU #0 PC %06x: warning - read unmapped memory address %06x\n",cpu_getpc(),0x0a0000+offset);

	return 0;
}



static int punkshot_kludge(int offset)
{
	/* I don't know what's going on here; at one point, the code reads location */
	/* 0xffffff, and returning 0 causes the game to mess up - locking up in a */
	/* loop where the ball is continuously bouncing from the basket. Returning */
	/* a random number seems to prevent that. */
	return rand();
}



static struct MemoryReadAddress tmnt_readmem[] =
{
	{ 0x000000, 0x05ffff, MRA_ROM },
	{ 0x060000, 0x063fff, MRA_BANK1 },	/* main RAM */
	{ 0x080000, 0x080fff, paletteram_word_r },
	{ 0x0a0000, 0x0a001b, tmnt_input_r },
	{ 0x100000, 0x106fff, punkshot_100000_r },	/* either video RAMs or character ROMs, */
												/* depending on bit 3 of a0021. */
												/* The area of the char ROMs appearing here */
												/* is controlled by 106b00 and 106e00 */
	{ 0x140400, 0x1407ff, punkshot_spriteram_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress tmnt_writemem[] =
{
	{ 0x000000, 0x05ffff, MWA_ROM },
	{ 0x060000, 0x063fff, MWA_BANK1 },	/* main RAM */
	{ 0x080000, 0x080fff, tmnt_paletteram_w, &paletteram },
	{ 0x0a0000, 0x0a0003, tmnt_0a0000_w },
	{ 0x0a0008, 0x0a0009, tmnt_sound_command_w },
	{ 0x0a0010, 0x0a0013, watchdog_reset_w },
	{ 0x0c0000, 0x0c0003, tmnt_priority_w },
	{ 0x100000, 0x105fff, punkshot_vidram_w, &punkshot_vidram, &punkshot_vidram_size },
	{ 0x106018, 0x10601b, punkshot_yscroll_w },
	{ 0x106400, 0x1067ff, punkshot_xscroll_w, &punkshot_scrollram },
	{ 0x106b00, 0x106b03, punkshot_charrombank0_w },
//	{ 0x106c00, 0x106c03, punkshot_charromsubbank_w },
	{ 0x106e00, 0x106e03, punkshot_charrombank1_w },
	{ 0x140400, 0x1407ff, punkshot_spriteram_w, &spriteram, &spriteram_size },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress punkshot_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x080000, 0x083fff, MRA_BANK1 },	/* main RAM */
	{ 0x090000, 0x090fff, paletteram_word_r },
	{ 0x0a0000, 0x0a0007, punkshot_input_r },
	{ 0x100000, 0x106fff, punkshot_100000_r },	/* either video RAMs or character ROMs, */
												/* depending on bit 3 of a0021. */
												/* The area of the char ROMs appearing here */
												/* is controlled by 106b00 and 106e00 */
	{ 0x110400, 0x1107ff, punkshot_spriteram_r },
	{ 0xfffffc, 0xffffff, punkshot_kludge },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress punkshot_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x080000, 0x083fff, MWA_BANK1 },	/* main RAM */
	{ 0x090000, 0x090fff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x0a0020, 0x0a0023, punkshot_0a0020_w },
	{ 0x0a0040, 0x0a0041, tmnt_sound_command_w },
	{ 0x0a0068, 0x0a006b, punkshot_priority_w },
	{ 0x0a0080, 0x0a0083, watchdog_reset_w },
	{ 0x100000, 0x105fff, punkshot_vidram_w, &punkshot_vidram, &punkshot_vidram_size },
	{ 0x106018, 0x10601b, punkshot_yscroll_w },
	{ 0x106400, 0x1067ff, punkshot_xscroll_w, &punkshot_scrollram },
	{ 0x106a00, 0x106a03, punkshot_irqenable_w },
	{ 0x106b00, 0x106b03, punkshot_charrombank0_w },
	{ 0x106c00, 0x106c03, punkshot_charromsubbank_w },
	{ 0x106e00, 0x106e03, punkshot_charrombank1_w },
	{ 0x110400, 0x1107ff, punkshot_spriteram_w, &spriteram, &spriteram_size },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress tmnt_s_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x9000, tmnt_sres_r },	/* title music & UPD7759C reset */
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xc001, 0xc001, YM2151_status_port_0_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress tmnt_s_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x9000, tmnt_sres_w },	/* title music & UPD7759C reset */
	{ 0xc000, 0xc000, YM2151_register_port_0_w },
	{ 0xc001, 0xc001, YM2151_data_port_0_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress punkshot_s_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xf7ff, MRA_RAM },
	{ 0xf801, 0xf801, YM2151_status_port_0_r },
	{ 0xfc00, 0xfc00, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress punkshot_s_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf000, 0xf7ff, MWA_RAM },
	{ 0xf800, 0xf800, YM2151_register_port_0_w },
	{ 0xf801, 0xf801, YM2151_data_port_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( tmnt_input_ports )
	PORT_START      /* COINS */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE )

	PORT_START      /* PLAYER 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* PLAYER 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* PLAYER 3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER3 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x01, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x02, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x60, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* PLAYER 4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER4 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START	/* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( tmnt2p_input_ports )
	PORT_START      /* COINS */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE )

	PORT_START      /* PLAYER 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* PLAYER 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* PLAYER 3 */
//	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER3 | IPF_8WAY )
//	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 | IPF_8WAY )
//	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER3 | IPF_8WAY )
//	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER3 | IPF_8WAY )
//	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
//	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
//	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
//	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x01, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x02, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x60, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* PLAYER 4 */
//	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER4 | IPF_8WAY )
//	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 | IPF_8WAY )
//	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER4 | IPF_8WAY )
//	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER4 | IPF_8WAY )
//	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
//	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
//	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* button 3 - unused */
//	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START	/* DSW3 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( punkshot_input_ports )
	PORT_START	/* DSW 1 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x05, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x01, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x03, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x07, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/7 Credits" )
	PORT_DIPNAME( 0x10, 0x10, "Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x00, "1 Coin" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, "Energy", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "40" )
	PORT_DIPSETTING(    0x02, "50" )
	PORT_DIPSETTING(    0x01, "60" )
	PORT_DIPSETTING(    0x00, "70" )
	PORT_DIPNAME( 0x0c, 0x0c, "Period Length", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "3 Minutes" )
	PORT_DIPSETTING(    0x08, "4 Minutes" )
	PORT_DIPSETTING(    0x04, "5 Minutes" )
	PORT_DIPSETTING(    0x00, "6 Minutes" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x60, 0x60, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x10, 0x10, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout tmnt_charlayout =
{
	8,8,	/* 8*8 characters */
	32768,	/* 32768 characters */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 32768*16*8+2*4, 32768*16*8+3*4, 32768*16*8+0*4, 32768*16*8+1*4, 2*4, 3*4, 0*4, 1*4},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};

/* The peculiar way in which the sprites are encoded forces us to draw them in 16x1 */
/* blocks - otherwise we would have to decode separately the 16xN, 32xN 64xN and 128xN */
static struct GfxLayout tmnt_spritelayout =
{
	16,1,	/* 16*1 sprites */
	16384,	/* 16384 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 16384*64*8+2*4, 16384*64*8+3*4, 16384*64*8+0*4, 16384*64*8+1*4, 2*4, 3*4, 0*4, 1*4,
			16+16384*64*8+2*4, 16+16384*64*8+3*4, 16+16384*64*8+0*4, 16+16384*64*8+1*4, 16+2*4, 16+3*4, 16+0*4, 16+1*4 },
	{ 0 },
	4*8	/* every sprite takes 4 consecutive bytes */
};

static struct GfxLayout punkshot_charlayout =
{
	8,8,	/* 8*8 characters */
	16384,	/* 16384 characters */
	4,	/* 4 bits per pixel */
	{ 16384*16*8+8, 16384*16*8+0, 8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout punkshot_spritelayout =
{
	16,16,	/* 16*16 sprites */
	16384,	/* 16384 sprites */
	4,	/* 4 bits per pixel */
	{ 16384*64*8+0, 16384*64*8+8, 0, 8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*16+0, 8*16+1, 8*16+2, 8*16+3, 8*16+4, 8*16+5, 8*16+6, 8*16+7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			16*16, 17*16, 18*16, 19*16, 20*16, 21*16, 22*16, 23*16 },
	64*8	/* every sprite takes 64 consecutive bytes */
};


static struct GfxDecodeInfo tmnt_gfxdecodeinfo[] =
{
	{ 2, 0x000000, &tmnt_charlayout,   0, 64 },
	{ 2, 0x100000, &tmnt_spritelayout, 0, 64 },
	{ 2, 0x110000, &tmnt_spritelayout, 0, 64 },
	{ 2, 0x120000, &tmnt_spritelayout, 0, 64 },
	{ 2, 0x130000, &tmnt_spritelayout, 0, 64 },
	{ 2, 0x140000, &tmnt_spritelayout, 0, 64 },
	{ 2, 0x150000, &tmnt_spritelayout, 0, 64 },
	{ 2, 0x160000, &tmnt_spritelayout, 0, 64 },
	{ 2, 0x170000, &tmnt_spritelayout, 0, 64 },
	{ 2, 0x180000, &tmnt_spritelayout, 0, 64 },
	{ 2, 0x190000, &tmnt_spritelayout, 0, 64 },
	{ 2, 0x1a0000, &tmnt_spritelayout, 0, 64 },
	{ 2, 0x1b0000, &tmnt_spritelayout, 0, 64 },
	{ 2, 0x1c0000, &tmnt_spritelayout, 0, 64 },
	{ 2, 0x1d0000, &tmnt_spritelayout, 0, 64 },
	{ 2, 0x1e0000, &tmnt_spritelayout, 0, 64 },
	{ 2, 0x1f0000, &tmnt_spritelayout, 0, 64 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo punkshot_gfxdecodeinfo[] =
{
	{ 2, 0x000000, &punkshot_charlayout,   0, 128 },
	{ 2, 0x080000, &punkshot_spritelayout, 0, 128 },
	{ -1 } /* end of array */
};



static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3579545,	/* 3.579545 MHz */
	{ 60 },
	{ 0 }
};

static struct Samplesinterface samples_interface =
{
	1	/* 1 channel for the title music */
};



static struct MachineDriver tmnt_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 MHz */
			0,
			tmnt_readmem,tmnt_writemem,0,0,
			tmnt_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 MHz */
			3,
			tmnt_s_readmem,tmnt_s_writemem,0,0,
			ignore_interrupt,0	/* IRQs are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 13*8, (64-13)*8-1, 2*8, 30*8-1 },	/* not sure about the horizontal visible area */
	tmnt_gfxdecodeinfo,
	64*16,64*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	tmnt_vh_start,
	tmnt_vh_stop,
	tmnt_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,
	tmnt_decode_sample,
	0,
	0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		},
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

static struct MachineDriver punkshot_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			12000000,	/* CPU is 68000/12, but this doesn't necessarily mean it's */
						/* running at 12MHz. TMNT uses 8MHz */
			0,
			punkshot_readmem,punkshot_writemem,0,0,
			punkshot_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3579545,	/* 3.579545 MHz */
			3,
			punkshot_s_readmem,punkshot_s_writemem,0,0,
			nmi_interrupt,1	/* IRQs are triggered by the main CPU */
							/* I don't know who generates NMIs, probably a sound chip */
							/* because the code requires them to get out from HALT (the. */
							/* NMI handler is just RETN) */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 13*8, (64-13)*8-1, 2*8, 30*8-1 },	/* not sure about the horizontal visible area */
	punkshot_gfxdecodeinfo,
	128*16,128*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	punkshot_vh_start,
	punkshot_vh_stop,
	punkshot_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
};

/***************************************************************************

  High score save/load

***************************************************************************/

static int tmnt_hiload(void)
{
	void *f;

	/* check if the hi score table has already been initialized */


if ((READ_WORD(&(cpu_bankbase[1][0x03500])) == 0x0312) && (READ_WORD(&(cpu_bankbase[1][0x03502])) == 0x0257) && (READ_WORD(&(cpu_bankbase[1][0x03512])) == 0x0101) && (READ_WORD(&(cpu_bankbase[1][0x035C8])) == 0x4849) && (READ_WORD(&(cpu_bankbase[1][0x035CA])) == 0x4400) && (READ_WORD(&(cpu_bankbase[1][0x035EC])) == 0x4D49))

{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread_msbfirst(f,&(cpu_bankbase[1][0x03500]),0x14);
			osd_fread_msbfirst(f,&(cpu_bankbase[1][0x035C8]),0x27);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;
}

static void tmnt_hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite_msbfirst(f,&(cpu_bankbase[1][0x03500]),0x14);
		osd_fwrite_msbfirst(f,&(cpu_bankbase[1][0x035C8]),0x27);


		osd_fclose(f);
	}
}

static int punkshot_hiload(void)
{
	void *f;

	/* check if the hi score table has already been initialized */


if ((READ_WORD(&(cpu_bankbase[1][0x00708])) == 0x0007) && (READ_WORD(&(cpu_bankbase[1][0x0070A])) == 0x2020) && (READ_WORD(&(cpu_bankbase[1][0x0072C])) == 0x464C))

{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread_msbfirst(f,&(cpu_bankbase[1][0x00708]),0x28);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;
}

static void punkshot_hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite_msbfirst(f,&(cpu_bankbase[1][0x00708]),0x28);
		osd_fclose(f);
	}
}

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( tmnt_rom )
	ROM_REGION(0x60000)	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD_EVEN( "963-r23",      0x00000, 0x20000, 0xa7f61195 )
	ROM_LOAD_ODD ( "963-r24",      0x00000, 0x20000, 0x661e056a )
	ROM_LOAD_EVEN( "963-r21",      0x40000, 0x10000, 0xde047bb6 )
	ROM_LOAD_ODD ( "963-r22",      0x40000, 0x10000, 0xd86a0888 )

	ROM_REGION_DISPOSE(0x1000)
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x300000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, 0xdb4769a8 )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, 0x8069cd2e )        /* 8x8 tiles */
	ROM_LOAD( "963-a17",      0x100000, 0x80000, 0xb5239a44 )        /* sprites */
	ROM_LOAD( "963-a18",      0x180000, 0x80000, 0xdd51adef )        /* sprites */
	ROM_LOAD( "963-a15",      0x200000, 0x80000, 0x1f324eed )        /* sprites */
	ROM_LOAD( "963-a16",      0x280000, 0x80000, 0xd4bd9984 )        /* sprites */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, 0x1692a6d6 )

	ROM_REGION(0x80000)	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, 0xfca078c7 )

	ROM_REGION(0x40000)	/* 128k+128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, 0xe2ac3063 ) /* samples for 007232 */
	ROM_LOAD( "963-a27",      0x20000, 0x20000, 0x2dfd674b ) /* samples for UPD7759C */
ROM_END

ROM_START( tmntj_rom )
	ROM_REGION(0x60000)	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD_EVEN( "963-x23",      0x00000, 0x20000, 0xa9549004 )
	ROM_LOAD_ODD ( "963-x24",      0x00000, 0x20000, 0xe5cc9067 )
	ROM_LOAD_EVEN( "963-x21",      0x40000, 0x10000, 0x5789cf92 )
	ROM_LOAD_ODD ( "963-x22",      0x40000, 0x10000, 0x0a74e277 )

	ROM_REGION_DISPOSE(0x1000)
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x300000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, 0xdb4769a8 )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, 0x8069cd2e )        /* 8x8 tiles */
	ROM_LOAD( "963-a17",      0x100000, 0x80000, 0xb5239a44 )        /* sprites */
	ROM_LOAD( "963-a18",      0x180000, 0x80000, 0xdd51adef )        /* sprites */
	ROM_LOAD( "963-a15",      0x200000, 0x80000, 0x1f324eed )        /* sprites */
	ROM_LOAD( "963-a16",      0x280000, 0x80000, 0xd4bd9984 )        /* sprites */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, 0x1692a6d6 )

	ROM_REGION(0x80000)	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, 0xfca078c7 )

	ROM_REGION(0x40000)	/* 128k+128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, 0xe2ac3063 ) /* samples for 007232 */
	ROM_LOAD( "963-a27",      0x20000, 0x20000, 0x2dfd674b ) /* samples for UPD7759C */
ROM_END

ROM_START( tmht2p_rom )
	ROM_REGION(0x60000)	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD_EVEN( "963-u23",      0x00000, 0x20000, 0x58bec748 )
	ROM_LOAD_ODD ( "963-u24",      0x00000, 0x20000, 0xdce87c8d )
	ROM_LOAD_EVEN( "963-u21",      0x40000, 0x10000, 0xabce5ead )
	ROM_LOAD_ODD ( "963-u22",      0x40000, 0x10000, 0x4ecc8d6b )

	ROM_REGION_DISPOSE(0x1000)
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x300000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, 0xdb4769a8 )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, 0x8069cd2e )        /* 8x8 tiles */
	ROM_LOAD( "963-a17",      0x100000, 0x80000, 0xb5239a44 )        /* sprites */
	ROM_LOAD( "963-a18",      0x180000, 0x80000, 0xdd51adef )        /* sprites */
	ROM_LOAD( "963-a15",      0x200000, 0x80000, 0x1f324eed )        /* sprites */
	ROM_LOAD( "963-a16",      0x280000, 0x80000, 0xd4bd9984 )        /* sprites */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, 0x1692a6d6 )

	ROM_REGION(0x80000)	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, 0xfca078c7 )

	ROM_REGION(0x40000)	/* 128k+128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, 0xe2ac3063 ) /* samples for 007232 */
	ROM_LOAD( "963-a27",      0x20000, 0x20000, 0x2dfd674b ) /* samples for UPD7759C */
ROM_END

ROM_START( tmnt2pj_rom )
	ROM_REGION(0x60000)	/* 2*128k and 2*64k for 68000 code */
	ROM_LOAD_EVEN( "963-123",      0x00000, 0x20000, 0x6a3527c9 )
	ROM_LOAD_ODD ( "963-124",      0x00000, 0x20000, 0x2c4bfa15 )
	ROM_LOAD_EVEN( "963-121",      0x40000, 0x10000, 0x4181b733 )
	ROM_LOAD_ODD ( "963-122",      0x40000, 0x10000, 0xc64eb5ff )

	ROM_REGION_DISPOSE(0x1000)
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x300000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "963-a28",      0x000000, 0x80000, 0xdb4769a8 )        /* 8x8 tiles */
	ROM_LOAD( "963-a29",      0x080000, 0x80000, 0x8069cd2e )        /* 8x8 tiles */
	ROM_LOAD( "963-a17",      0x100000, 0x80000, 0xb5239a44 )        /* sprites */
	ROM_LOAD( "963-a18",      0x180000, 0x80000, 0xdd51adef )        /* sprites */
	ROM_LOAD( "963-a15",      0x200000, 0x80000, 0x1f324eed )        /* sprites */
	ROM_LOAD( "963-a16",      0x280000, 0x80000, 0xd4bd9984 )        /* sprites */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "963-e20",      0x00000, 0x08000, 0x1692a6d6 )

	ROM_REGION(0x80000)	/* 512k for the title music sample */
	ROM_LOAD( "963-a25",      0x00000, 0x80000, 0xfca078c7 )

	ROM_REGION(0x40000)	/* 128k+128k for the samples */
	ROM_LOAD( "963-a26",      0x00000, 0x20000, 0xe2ac3063 ) /* samples for 007232 */
	ROM_LOAD( "963-a27",      0x20000, 0x20000, 0x2dfd674b ) /* samples for UPD7759C */
ROM_END

ROM_START( punkshot_rom )
	ROM_REGION(0x40000)	/* 4*64k for 68000 code */
	ROM_LOAD_EVEN( "907m02.i7",    0x00000, 0x20000, 0x59e14575 )
	ROM_LOAD_ODD ( "907m03.i10",   0x00000, 0x20000, 0xadb14b1e )

	ROM_REGION_DISPOSE(0x1000)
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x280000)	/* graphics (addressable by the main CPU) */
	ROM_LOAD( "907d06.e23",   0x000000, 0x40000, 0xf5cc38f4 )
	ROM_LOAD( "907d05.e22",   0x040000, 0x40000, 0xe25774c1 )
	ROM_LOAD( "907d08l.k7",   0x080000, 0x80000, 0x05f3d196 )
	ROM_LOAD( "907d08h.k7",   0x100000, 0x80000, 0xeaf18c22 )
	ROM_LOAD( "907d07l.k2",   0x180000, 0x80000, 0xfeeb345a )
	ROM_LOAD( "907d07h.k2",   0x200000, 0x80000, 0x0bff4383 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "907f01.e8",    0x0000, 0x8000, 0xf040c484 )

	ROM_REGION(0x80000)	/* ADPCM samples */
	ROM_LOAD( "907d04.d3",    0x0000, 0x80000, 0x090feb5e )
ROM_END



struct GameDriver tmnt_driver =
{
	__FILE__,
	0,
	"tmnt",
	"TMNT (4 Players USA)",
	"1989",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)\nDan Boris (hardware info)",
	0,
	&tmnt_machine_driver,
	0,

	tmnt_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tmnt_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	tmnt_hiload, tmnt_hisave
};

struct GameDriver tmntj_driver =
{
	__FILE__,
	&tmnt_driver,
	"tmntj",
	"TMNT (4 Players Japanese)",
	"1989",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)\nDan Boris (hardware info)",
	0,
	&tmnt_machine_driver,
	0,

	tmntj_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tmnt_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	tmnt_hiload, tmnt_hisave
};

struct GameDriver tmht2p_driver =
{
	__FILE__,
	&tmnt_driver,
	"tmht2p",
	"TMHT (2 Players UK)",
	"1989",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)\nDan Boris (hardware info)\nAlex Simmons (2 player version)",
	0,
	&tmnt_machine_driver,
	0,

	tmht2p_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tmnt2p_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	tmnt_hiload, tmnt_hisave
};

struct GameDriver tmnt2pj_driver =
{
	__FILE__,
	&tmnt_driver,
	"tmnt2pj",
	"TMNT (2 Players Japanese)",
	"1990",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)\nDan Boris (hardware info)\nAlex Simmons (2 player version)",
	0,
	&tmnt_machine_driver,
	0,

	tmnt2pj_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	tmnt2p_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	tmnt_hiload, tmnt_hisave
};

struct GameDriver punkshot_driver =
{
	__FILE__,
	0,
	"punkshot",
	"Punk Shot",
	"1990",
	"Konami",
	"Nicola Salmoria (MAME driver)\nAlex Pasadyn (MAME driver)\nJeff Slutter (hardware info)\nHowie Cohen (hardware info)\nDan Boris (hardware info)",
	0,
	&punkshot_machine_driver,
	0,

	punkshot_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	punkshot_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	punkshot_hiload, punkshot_hisave
};
