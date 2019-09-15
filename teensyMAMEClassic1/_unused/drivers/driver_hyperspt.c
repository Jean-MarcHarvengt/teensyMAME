/***************************************************************************

Based on drivers from Juno First emulator by Chris Hardy (chrish@kcbbs.gen.nz)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809/M6809.h"



extern unsigned char *hyperspt_scroll;

void hyperspt_flipscreen_w(int offset,int data);
void hyperspt_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int hyperspt_vh_start(void);
void hyperspt_vh_stop(void);
void hyperspt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void roadf_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern unsigned char *konami_dac;
void konami_sh_irqtrigger_w(int offset,int data);
int hyperspt_sh_timer_r(int offset);
void hyperspt_sound_w(int offset , int data);

/* these routines lurk in sndhrdw/trackfld.c */
extern struct VLM5030interface konami_vlm5030_interface;
extern struct SN76496interface konami_sn76496_interface;
extern struct DACinterface konami_dac_interface;
void konami_dac_w(int offset,int data);
void konami_SN76496_latch_w(int offset,int data);
void konami_SN76496_0_w(int offset,int data);

/* in machine/konami.c */
unsigned char KonamiDecode( unsigned char opcode, unsigned short address );

void hyperspt_init_machine(void)
{
	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_S | M6809_FAST_U;
}

/* handle fake button for speed cheat */
static int konami_IN1_r(int offset)
{
	int res;
	static int cheat = 0;
	static int bits[] = { 0xee, 0xff, 0xbb, 0xaa };

	res = readinputport(1);

	if ((res & 0x80) == 0)
	{
		res |= 0x55;
		res &= bits[cheat];
		cheat = (++cheat)%4;
	}
	return res;
}



static struct MemoryReadAddress hyperspt_readmem[] =
{
	{ 0x1000, 0x10ff, MRA_RAM },
	{ 0x1600, 0x1600, input_port_4_r }, /* DIP 2 */
	{ 0x1680, 0x1680, input_port_0_r }, /* IO Coin */
//	{ 0x1681, 0x1681, input_port_1_r }, /* P1 IO */
	{ 0x1681, 0x1681, konami_IN1_r }, /* P1 IO and handle fake button for cheating */
	{ 0x1682, 0x1682, input_port_2_r }, /* P2 IO */
	{ 0x1683, 0x1683, input_port_3_r }, /* DIP 1 */
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress roadf_readmem[] =
{
	{ 0x1000, 0x10ff, MRA_RAM },
	{ 0x1600, 0x1600, input_port_4_r }, /* DIP 2 */
	{ 0x1680, 0x1680, input_port_0_r }, /* IO Coin */
	{ 0x1681, 0x1681, input_port_1_r }, /* P1 IO */
	{ 0x1682, 0x1682, input_port_2_r }, /* P2 IO */
	{ 0x1683, 0x1683, input_port_3_r }, /* DIP 1 */
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x1000, 0x10bf, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x10C0, 0x10ff, MWA_RAM, &hyperspt_scroll },  /* Scroll amount */
	{ 0x1400, 0x1400, watchdog_reset_w },
	{ 0x1480, 0x1480, hyperspt_flipscreen_w },
	{ 0x1481, 0x1481, konami_sh_irqtrigger_w },  /* cause interrupt on audio CPU */
	{ 0x1483, 0x1484, coin_counter_w },
	{ 0x1487, 0x1487, interrupt_enable_w },  /* Interrupt enable */
	{ 0x1500, 0x1500, soundlatch_w },
	{ 0x2000, 0x27ff, videoram_w, &videoram, &videoram_size },
	{ 0x2800, 0x2fff, colorram_w, &colorram },
	{ 0x3000, 0x3fff, MWA_RAM },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4fff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0x8000, 0x8000, hyperspt_sh_timer_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x4fff, MWA_RAM },
	{ 0xa000, 0xa000, VLM5030_data_w }, /* speech data */
	{ 0xc000, 0xdfff, hyperspt_sound_w },     /* speech and output controll */
	{ 0xe000, 0xe000, konami_dac_w, &konami_dac },
	{ 0xe001, 0xe001, konami_SN76496_latch_w },  /* Loads the snd command into the snd latch */
	{ 0xe002, 0xe002, konami_SN76496_0_w },      /* This address triggers the SN chip to read the data port. */
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( hyperspt_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
//	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
	/* Fake button to press buttons 1 and 3 impossibly fast. Handle via konami_IN1_r */
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_CHEAT | IPF_PLAYER1, "Run Like Hell Cheat", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 /*| IPF_COCKTAIL*/ )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
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
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x50, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x10, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x70, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Disabled" )
/* 0x00 disables Coin 2. It still accepts coins and makes the sound, but
   it doesn't give you any credit */

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "After Last Event", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Game Over" )
	PORT_DIPSETTING(    0x00, "Game Continues" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )
	PORT_DIPNAME( 0x04, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "World Records", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Don't Erase" )
	PORT_DIPSETTING(    0x00, "Erase on Reset" )
	PORT_DIPNAME( 0xf0, 0xf0, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0xf0, "Easy 1" )
	PORT_DIPSETTING(    0xe0, "Easy 2" )
	PORT_DIPSETTING(    0xd0, "Easy 3" )
	PORT_DIPSETTING(    0xc0, "Easy 4" )
	PORT_DIPSETTING(    0xb0, "Normal 1" )
	PORT_DIPSETTING(    0xa0, "Normal 2" )
	PORT_DIPSETTING(    0x90, "Normal 3" )
	PORT_DIPSETTING(    0x80, "Normal 4" )
	PORT_DIPSETTING(    0x70, "Normal 5" )
	PORT_DIPSETTING(    0x60, "Normal 6" )
	PORT_DIPSETTING(    0x50, "Normal 7" )
	PORT_DIPSETTING(    0x40, "Normal 8" )
	PORT_DIPSETTING(    0x30, "Difficult 1" )
	PORT_DIPSETTING(    0x20, "Difficult 2" )
	PORT_DIPSETTING(    0x10, "Difficult 3" )
	PORT_DIPSETTING(    0x00, "Difficult 4" )
INPUT_PORTS_END

INPUT_PORTS_START( roadf_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* the game doesn't boot if this is 1 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
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
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x50, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x40, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x10, "4 Coins/3 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "3 Coins/4 Credits" )
	PORT_DIPSETTING(    0x70, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x90, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x00, "Disabled" )
/* 0x00 disables Coin 2. It still accepts coins and makes the sound, but
   it doesn't give you any credit */

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x06, 0x06, "Number of Opponents", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "Easy" )
	PORT_DIPSETTING(    0x04, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x08, 0x08, "Speed of Opponents", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x00, "Difficult" )
	PORT_DIPNAME( 0x30, 0x30, "Fuel Consumption", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "Easy" )
	PORT_DIPSETTING(    0x20, "Medium" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout hyperspt_charlayout =
{
	8,8,	/* 8*8 sprites */
	1024,	/* 1024 characters */
	4,	/* 4 bits per pixel */
	{ 0x4000*8+4, 0x4000*8+0, 4, 0  },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout hyperspt_spritelayout =
{
	16,16,	/* 16*16 sprites */
	512,	/* 512 sprites */
	4,	/* 4 bits per pixel */
	{ 0x8000*8+4, 0x8000*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 ,
		32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8, },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo hyperspt_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &hyperspt_charlayout,       0, 16 },
	{ 1, 0x8000, &hyperspt_spritelayout, 16*16, 16 },
	{ -1 } /* end of array */
};


static struct GfxLayout roadf_charlayout =
{
	8,8,	/* 8*8 sprites */
	1536,	/* 1536 characters */
	4,	/* 4 bits per pixel */
	{ 0x6000*8+4, 0x6000*8+0, 4, 0  },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxLayout roadf_spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel */
	{ 0x4000*8+4, 0x4000*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 ,
		32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8, },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo roadf_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &roadf_charlayout,       0, 16 },
	{ 1, 0xc000, &roadf_spritelayout, 16*16, 16 },
	{ -1 } /* end of array */
};



/* filename for hyper sports sample files */
static const char *hyperspt_sample_names[] =
{
	"00.sam","01.sam","02.sam","03.sam","04.sam","05.sam","06.sam","07.sam",
	"08.sam","09.sam","0a.sam","0b.sam","0c.sam","0d.sam","0e.sam","0f.sam",
	"10.sam","11.sam","12.sam","13.sam","14.sam","15.sam","16.sam","17.sam",
	"18.sam","19.sam","1a.sam","1b.sam","1c.sam","1d.sam","1e.sam","1f.sam",
	"20.sam","21.sam","22.sam","23.sam","24.sam","25.sam","26.sam","27.sam",
	"28.sam","29.sam","2a.sam","2b.sam","2c.sam","2d.sam","2e.sam","2f.sam",
	"30.sam","31.sam","32.sam","33.sam","34.sam","35.sam","36.sam","37.sam",
	"38.sam","39.sam","3a.sam","3b.sam","3c.sam","3d.sam","3e.sam","3f.sam",
	"40.sam","41.sam","42.sam","43.sam","44.sam","45.sam","46.sam","47.sam",
	"48.sam","49.sam",
	0
};

static struct MachineDriver hyperspt_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			2048000,        /* 1.400 Mhz ??? */
			0,
			hyperspt_readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318180/4,	/* Z80 Clock is derived from a 14.31818 Mhz crystal */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	hyperspt_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	hyperspt_gfxdecodeinfo,
	32,16*16+16*16,
	hyperspt_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	hyperspt_vh_start,
	hyperspt_vh_stop,
	hyperspt_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&konami_dac_interface
		},
		{
			SOUND_SN76496,
			&konami_sn76496_interface
		},
		{
			SOUND_VLM5030,
			&konami_vlm5030_interface
		}
	}
};

static struct MachineDriver roadf_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			2048000,        /* 1.400 Mhz ??? */
			0,
			roadf_readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318180/4,	/* Z80 Clock is derived from a 14.31818 Mhz crystal */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	hyperspt_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	roadf_gfxdecodeinfo,
	32,16*16+16*16,
	hyperspt_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	hyperspt_vh_start,
	hyperspt_vh_stop,
	roadf_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&konami_dac_interface
		},
		{
			SOUND_SN76496,
			&konami_sn76496_interface
		},
		{
			SOUND_VLM5030,
			&konami_vlm5030_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( hyperspt_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "c01",          0x4000, 0x2000, 0x0c720eeb )
	ROM_LOAD( "c02",          0x6000, 0x2000, 0x560258e0 )
	ROM_LOAD( "c03",          0x8000, 0x2000, 0x9b01c7e6 )
	ROM_LOAD( "c04",          0xA000, 0x2000, 0x10d7e9a2 )
	ROM_LOAD( "c05",          0xC000, 0x2000, 0xb105a8cd )
	ROM_LOAD( "c06",          0xE000, 0x2000, 0x1a34a849 )

	ROM_REGION_DISPOSE(0x18000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "c26",          0x00000, 0x2000, 0xa6897eac )
	ROM_LOAD( "c24",          0x02000, 0x2000, 0x5fb230c0 )
	ROM_LOAD( "c22",          0x04000, 0x2000, 0xed9271a0 )
	ROM_LOAD( "c20",          0x06000, 0x2000, 0x183f4324 )
	ROM_LOAD( "c14",          0x08000, 0x2000, 0xc72d63be )
	ROM_LOAD( "c13",          0x0a000, 0x2000, 0x76565608 )
	ROM_LOAD( "c12",          0x0c000, 0x2000, 0x74d2cc69 )
	ROM_LOAD( "c11",          0x0e000, 0x2000, 0x66cbcb4d )
	ROM_LOAD( "c18",          0x10000, 0x2000, 0xed25e669 )
	ROM_LOAD( "c17",          0x12000, 0x2000, 0xb145b39f )
	ROM_LOAD( "c16",          0x14000, 0x2000, 0xd7ff9f2b )
	ROM_LOAD( "c15",          0x16000, 0x2000, 0xf3d454e6 )

	ROM_REGION(0x220)	/* color/lookup proms */
	ROM_LOAD( "c03_c27.bin",  0x0000, 0x0020, 0xbc8a5956 )
	ROM_LOAD( "j12_c28.bin",  0x0020, 0x0100, 0x2c891d59 )
	ROM_LOAD( "a09_c29.bin",  0x0120, 0x0100, 0x811a3f3f )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "c10",          0x0000, 0x2000, 0x3dc1a6ff )
	ROM_LOAD( "c09",          0x2000, 0x2000, 0x9b525c3e )

	ROM_REGION(0x10000)	/*  64k for speech rom    */
	ROM_LOAD( "c08",          0x0000, 0x2000, 0xe8f8ea78 )
ROM_END

ROM_START( roadf_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "g05_g01.bin",  0x4000, 0x2000, 0xe2492a06 )
	ROM_LOAD( "g07_f02.bin",  0x6000, 0x2000, 0x0bf75165 )
	ROM_LOAD( "g09_g03.bin",  0x8000, 0x2000, 0xdde401f8 )
	ROM_LOAD( "g11_f04.bin",  0xA000, 0x2000, 0xb1283c77 )
	ROM_LOAD( "g13_f05.bin",  0xC000, 0x2000, 0x0ad4d796 )
	ROM_LOAD( "g15_f06.bin",  0xE000, 0x2000, 0xfa42e0ed )

	ROM_REGION_DISPOSE(0x14000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a14_e26.bin",  0x00000, 0x4000, 0xf5c738e2 )
	ROM_LOAD( "a12_d24.bin",  0x04000, 0x2000, 0x2d82c930 )
	ROM_LOAD( "c14_e22.bin",  0x06000, 0x4000, 0xfbcfbeb9 )
	ROM_LOAD( "c12_d20.bin",  0x0a000, 0x2000, 0x5e0cf994 )
	ROM_LOAD( "j19_e14.bin",  0x0c000, 0x4000, 0x16d2bcff )
	ROM_LOAD( "g19_e18.bin",  0x10000, 0x4000, 0x490685ff )

	ROM_REGION(0x220)	/* color/lookup proms */
	ROM_LOAD( "c03_c27.bin",  0x0000, 0x0020, 0x45d5e352 )
	ROM_LOAD( "j12_c28.bin",  0x0020, 0x0100, 0x2955e01f )
	ROM_LOAD( "a09_c29.bin",  0x0120, 0x0100, 0x5b3b5f2a )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "a17_d10.bin",  0x0000, 0x2000, 0xc33c927e )
ROM_END

ROM_START( roadf2_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "5g",           0x4000, 0x2000, 0xd8070d30 )
	ROM_LOAD( "6g",           0x6000, 0x2000, 0x8b661672 )
	ROM_LOAD( "8g",           0x8000, 0x2000, 0x714929e8 )
	ROM_LOAD( "11g",          0xA000, 0x2000, 0x0f2c6b94 )
	ROM_LOAD( "g13_f05.bin",  0xC000, 0x2000, 0x0ad4d796 )
	ROM_LOAD( "g15_f06.bin",  0xE000, 0x2000, 0xfa42e0ed )

	ROM_REGION_DISPOSE(0x14000)    /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "a14_e26.bin",  0x00000, 0x4000, 0xf5c738e2 )
	ROM_LOAD( "a12_d24.bin",  0x04000, 0x2000, 0x2d82c930 )
	ROM_LOAD( "c14_e22.bin",  0x06000, 0x4000, 0xfbcfbeb9 )
	ROM_LOAD( "c12_d20.bin",  0x0a000, 0x2000, 0x5e0cf994 )
	ROM_LOAD( "j19_e14.bin",  0x0c000, 0x4000, 0x16d2bcff )
	ROM_LOAD( "g19_e18.bin",  0x10000, 0x4000, 0x490685ff )

	ROM_REGION(0x220)	/* color/lookup proms */
	ROM_LOAD( "c03_c27.bin",  0x0000, 0x0020, 0x45d5e352 )
	ROM_LOAD( "j12_c28.bin",  0x0020, 0x0100, 0x2955e01f )
	ROM_LOAD( "a09_c29.bin",  0x0120, 0x0100, 0x5b3b5f2a )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "a17_d10.bin",  0x0000, 0x2000, 0xc33c927e )
ROM_END



static void hyperspt_decode(void)
{
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	for (A = 0x4000;A < 0x10000;A++)
	{
		ROM[A] = KonamiDecode(RAM[A],A);
	}
}



/*
 HyperSports has 2k of battery backed RAM which can be erased by setting a dipswitch
 All we need to do is load it in. If the Dipswitch is set it will be erased
*/

static int we_flipped_the_switch;

static int hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0x3800],0x4000-0x3800);
		osd_fclose(f);

		we_flipped_the_switch = 0;
	}
	else
	{
		struct InputPort *in;


		/* find the dip switch which resets the high score table, and set it on */
		in = Machine->input_ports;

		while (in->type != IPT_END)
		{
			if (in->name != NULL && in->name != IP_NAME_DEFAULT &&
					strcmp(in->name,"World Records") == 0)
			{
				if (in->default_value == in->mask)
				{
					in->default_value = 0;
					we_flipped_the_switch = 1;
				}
				break;
			}

			in++;
		}
	}

	return 1;
}

static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x3800],0x800);
		osd_fclose(f);
	}

	if (we_flipped_the_switch)
	{
		struct InputPort *in;


		/* find the dip switch which resets the high score table, and set it */
		/* back to off. */
		in = Machine->input_ports;

		while (in->type != IPT_END)
		{
			if (in->name != NULL && in->name != IP_NAME_DEFAULT &&
					strcmp(in->name,"World Records") == 0)
			{
				if (in->default_value == 0)
					in->default_value = in->mask;
				break;
			}

			in++;
		}

		we_flipped_the_switch = 0;
	}
}



struct GameDriver hyperspt_driver =
{
	__FILE__,
	0,
	"hyperspt",
	"HyperSports",
	"1984",
	"Konami (Centuri license)",
	"Chris Hardy (MAME driver)\nPaul Swan (color info)\nTatsuyuki Satoh(speech sound)",
	0,
	&hyperspt_machine_driver,
	0,

	hyperspt_rom,
	0, hyperspt_decode,
	hyperspt_sample_names,
	0,	/* sound_prom */

	hyperspt_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver roadf_driver =
{
	__FILE__,
	0,
	"roadf",
	"Road Fighter (set 1)",
	"1984",
	"Konami",
	"Chris Hardy (Hyper Sports driver)\nNicola Salmoria\nPaul Swan (color info)",
	0,
	&roadf_machine_driver,
	0,

	roadf_rom,
	0, hyperspt_decode,
	0,
	0,	/* sound_prom */

	roadf_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};

struct GameDriver roadf2_driver =
{
	__FILE__,
	&roadf_driver,
	"roadf2",
	"Road Fighter (set 2)",
	"1984",
	"Konami",
	"Chris Hardy (Hyper Sports driver)\nNicola Salmoria\nPaul Swan (color info)",
	0,
	&roadf_machine_driver,
	0,

	roadf2_rom,
	0, hyperspt_decode,
	0,
	0,	/* sound_prom */

	roadf_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};
