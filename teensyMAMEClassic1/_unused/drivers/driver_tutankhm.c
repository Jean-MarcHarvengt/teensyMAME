/***************************************************************************

Tutankham :  memory map (preliminary)

I include here the document based on Rob Jarrett's research because it's
really exaustive.



Tutankham Emu Info
------------------

By Rob Jarrett
robj@astound.com (until June 20, 1997)
or robncait@inforamp.net

Special thanks go to Pete Custerson for the schematics!!


I've recently been working on an emulator for Tutankham. Unfortunately,
time and resources are not on my side so I'd like to provide anyone with
the technical information I've gathered so far, that way someone can
finish the project.

First of all, I'd like to say that I've had no prior experience in
writing an emulator, and my hardware knowledge is weak. I've managed to
find out a fair amount by looking at the schematics of the game and the
disassembled ROMs. Using the USim C++ 6809 core I have the game sort of
up and running, albeit in a pathetic state. It's not playable, and
crashes after a short amount of time. I don't feel the source code is
worth releasing because of the bad design; I was using it as a testing
bed and anticipated rewriting everything in the future.

Here's all the info I know about Tutankham:

Processor: 6809
Sound: Z80 slave w/2 AY3910 sound chips
Graphics: Bitmapped display, no sprites (!)
Memory Map:

Address		R/W	Bits		Function
------------------------------------------------------------------------------------------------------
$0000-$7fff				Video RAM
					- Screen is stored sideways, 256x256 pixels
					- 1 byte=2 pixels
		R/W	aaaaxxxx	- leftmost pixel palette index
		R/W	xxxxbbbb	- rightmost pixel palette index
					- **** not correct **** Looks like some of this memory is for I/O state, (I think < $0100)
					  so you might want to blit from $0100-$7fff

$8000-$800f	R/W     aaaaaaaa	Palette colors
					- Don't know how to decode them into RGB values

$8100		W			Not sure
					- Video chip function of some sort
					( split screen y pan position -- TT )

$8120		R			Not sure
					- Read from quite frequently
					- Some sort of video or interrupt thing?
					- Or a random number seed?
					( watchdog reset -- NS )

$8160					Dip Switch 2
					- Inverted bits (ie. 1=off)
		R	xxxxxxxa	DSWI1
		R
		R			.
		R			.
		R			.
		R
		R
		R	axxxxxxx	DSWI8

$8180					I/O: Coin slots, service, 1P/2P buttons
		R

$81a0					Player 1 I/O
		R

$81c0					Player 2 I/O
		R

$81e0					Dip Switch 1
					- Inverted bits
		R	xxxxxxxa	DSWI1
		R
		R			.
		R			.
		R			.
		R
		R
		R	axxxxxxx	DSWI8

$8200					IST on schematics
					- Enable/disable IRQ
		R/W	xxxxxxxa	- a=1 IRQ can be fired, a=0 IRQ can't be fired

$8202					OUT2 (Coin counter)
		W	xxxxxxxa	- Increment coin counter

$8203					OUT1 (Coin counter)
		W	xxxxxxxa	- Increment coin counter

$8204					Not sure - 401 on schematics
		W

$8205					MUT on schematics
		R/W	xxxxxxxa	- Sound amplification on/off?

$8206					HFF on schematics
		W			- Don't know what it does
					( horizontal screen flip -- NS )

$8207					Not sure - can't resolve on schematics
		W
					( vertical screen flip -- NS )

$8300					Graphics bank select
		W	xxxxxaaa	- Selects graphics ROM 0-11 that appears at $9000-9fff
					- But wait! There's only 9 ROMs not 12! I think the PCB allows 12
					  ROMs for patches/mods to the game. Just make 9-11 return 0's

$8600		W			SON on schematics
					( trigger interrupt on audio CPU -- NS )
$8608		R/W			SON on schematics
					- Sound on/off? i.e. Run/halt Z80 sound CPU?

$8700		W	aaaaaaaa	SDA on schematics
					- Sound data? Maybe Z80 polls here and plays the appropriate sound?
					- If so, easy to trigger samples here

$8800-$8fff				RAM
		R/W			- Memory for the program ROMs

$9000-$9fff				Graphics ROMs ra1_1i.cpu - ra1_9i.cpu
		R	aaaaaaaa	- See address $8300 for usage

$a000-$afff				ROM ra1_1h.cpu
		R	aaaaaaaa	- 6809 Code

$b000-$bfff				ROM ra1_2h.cpu
		R	aaaaaaaa	- 6809 Code

$c000-$cfff				ROM ra1_3h.cpu
		R	aaaaaaaa	- 6809 Code

$d000-$dfff				ROM ra1_4h.cpu
		R	aaaaaaaa	- 6809 Code

$e000-$efff				ROM ra1_5h.cpu
		R	aaaaaaaa	- 6809 Code

$f000-$ffff				ROM ra1_6h.cpu
		R	aaaaaaaa	- 6809 Code

Programming notes:

I found that generating an IRQ every 4096 instructions seemed to kinda work. Again, I know
little about emu writing and I think some fooling with this number might be needed.

Sorry I didn't supply the DSW and I/O bits, this info is available elsewhere on the net, I
think at tant or something. I just couldn't remember what they were at this writing!!

If there are any questions at all, please feel free to email me at robj@astound.com (until
June 20, 1997) or robncait@inforamp.net.


BTW, this information is completely free - do as you wish with it. I'm not even sure if it's
correct! (Most of it seems to be). Giving me some credit if credit is due would be nice,
and please let me know about your emulator if you release it.


Sound board: uses the same board as Pooyan.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809/m6809.h"



extern unsigned char *tutankhm_scrollx;

void tutankhm_videoram_w( int offset, int data );
void tutankhm_flipscreen_w( int offset, int data );
void tutankhm_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



void tutankhm_init_machine(void)
{
	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_S;
}

void tutankhm_bankselect_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + (data & 0x0f) * 0x1000;
	cpu_setbank(1,&RAM[bankaddress]);
}


/* I am not 100% sure that this timer is correct, but */
/* I'm using the Gyruss wired to the higher 4 bits    */
/* instead of the lower ones, so there is a good      */
/* chance it's the right one. */

/* The timer clock which feeds the lower 4 bits of    */
/* AY-3-8910 port A is based on the same clock        */
/* feeding the sound CPU Z80.  It is a divide by      */
/* 10240, formed by a standard divide by 1024,        */
/* followed by a divide by 10 using a 4 bit           */
/* bi-quinary count sequence. (See LS90 data sheet    */
/* for an example).                                   */
/* Bits 1-3 come directly from the upper three bits   */
/* of the bi-quinary counter. Bit 0 comes from the    */
/* output of the divide by 1024.                      */

static int tutankhm_timer[20] = {
0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03, 0x04, 0x05,
0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b, 0x0c, 0x0d
};

static int tutankhm_portB_r(int offset)
{
	/* need to protect from totalcycles overflow */
	static int last_totalcycles = 0;

	/* number of Z80 clock cycles to count */
	static int clock;

	int current_totalcycles;

	current_totalcycles = cpu_gettotalcycles();
	clock = (clock + (current_totalcycles-last_totalcycles)) % 10240;

	last_totalcycles = current_totalcycles;

	return tutankhm_timer[clock/512] << 4;
}

void tutankhm_sh_irqtrigger_w(int offset,int data)
{
	static int last;


	if (last == 0 && data == 1)
	{
		/* setting bit 0 low then high triggers IRQ on the sound CPU */
		cpu_cause_interrupt(1,0xff);
	}

	last = data;
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_RAM },
	{ 0x8120, 0x8120, watchdog_reset_r },
	{ 0x8160, 0x8160, input_port_0_r },	/* DSW2 (inverted bits) */
	{ 0x8180, 0x8180, input_port_1_r },	/* IN0 I/O: Coin slots, service, 1P/2P buttons */
	{ 0x81a0, 0x81a0, input_port_2_r },	/* IN1: Player 1 I/O */
	{ 0x81c0, 0x81c0, input_port_3_r },	/* IN2: Player 2 I/O */
	{ 0x81e0, 0x81e0, input_port_4_r },	/* DSW1 (inverted bits) */
	{ 0x8800, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9fff, MRA_BANK1 },
	{ 0xa000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, tutankhm_videoram_w, &videoram, &videoram_size },
	{ 0x8000, 0x800f, paletteram_BBGGGRRR_w, &paletteram },
	{ 0x8100, 0x8100, MWA_RAM, &tutankhm_scrollx },
	{ 0x8200, 0x8200, interrupt_enable_w },
	{ 0x8202, 0x8203, MWA_RAM },	/* coin counters */
	{ 0x8205, 0x8205, MWA_NOP },	/* ??? */
	{ 0x8206, 0x8207, tutankhm_flipscreen_w },
	{ 0x8300, 0x8300, tutankhm_bankselect_w },
	{ 0x8600, 0x8600, tutankhm_sh_irqtrigger_w },
	{ 0x8700, 0x8700, soundlatch_w },
	{ 0x8800, 0x8fff, MWA_RAM },
	{ 0xa000, 0xffff, MWA_ROM },
	{ -1 } /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x3000, 0x33ff, MRA_RAM },
	{ 0x4000, 0x4000, AY8910_read_port_0_r },
	{ 0x6000, 0x6000, AY8910_read_port_1_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x3000, 0x33ff, MWA_RAM },
	{ 0x4000, 0x4000, AY8910_write_port_0_w },
	{ 0x5000, 0x5000, AY8910_control_port_0_w },
	{ 0x6000, 0x6000, AY8910_write_port_1_w },
	{ 0x7000, 0x7000, AY8910_control_port_1_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "256", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x08, 0x08, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x30, 0x30, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "Easy" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, "Flash Bomb", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "1 per Life" )
	PORT_DIPSETTING(    0x00, "1 per Game" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
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
/* 0x00 not remmed out since the game makes the usual sound if you insert the coin */
INPUT_PORTS_END



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1789750,	/* 1.78975 MHz ? (same as other Konami games) */
	{ 0x20ff, 0x20ff },
	{ soundlatch_r },
	{ tutankhm_portB_r },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1500000,			/* 1.5 Mhz ??? */
			0,				/* memory region # 0 */
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318180/4,	/* ???? same as other Konami games */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are triggered by the main CPU */
		}
	},
	30, DEFAULT_30HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	tutankhm_init_machine,				/* init machine routine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },	/* not sure about the visible area */
	0,					/* GfxDecodeInfo * */
	16, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE,
	0,						/* vh_init routine */
	generic_vh_start,					/* vh_start routine */
	generic_vh_stop,					/* vh_stop routine */
	tutankhm_vh_screenrefresh,				/* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};


ROM_START( tutankhm_rom )
	ROM_REGION( 0x20000 )      /* 64k for M6809 CPU code + 64k for ROM banks */
	ROM_LOAD( "h1.bin",       0x0a000, 0x1000, 0xda18679f ) /* program ROMs */
	ROM_LOAD( "h2.bin",       0x0b000, 0x1000, 0xa0f02c85 )
	ROM_LOAD( "h3.bin",       0x0c000, 0x1000, 0xea03a1ab )
	ROM_LOAD( "h4.bin",       0x0d000, 0x1000, 0xbd06fad0 )
	ROM_LOAD( "h5.bin",       0x0e000, 0x1000, 0xbf9fd9b0 )
	ROM_LOAD( "h6.bin",       0x0f000, 0x1000, 0xfe079c5b )
	ROM_LOAD( "j1.bin",       0x10000, 0x1000, 0x7eb59b21 ) /* graphic ROMs (banked) -- only 9 of 12 are filled */
	ROM_LOAD( "j2.bin",       0x11000, 0x1000, 0x6615eff3 )
	ROM_LOAD( "j3.bin",       0x12000, 0x1000, 0xa10d4444 )
	ROM_LOAD( "j4.bin",       0x13000, 0x1000, 0x58cd143c )
	ROM_LOAD( "j5.bin",       0x14000, 0x1000, 0xd7e7ae95 )
	ROM_LOAD( "j6.bin",       0x15000, 0x1000, 0x91f62b82 )
	ROM_LOAD( "j7.bin",       0x16000, 0x1000, 0xafd0a81f )
	ROM_LOAD( "j8.bin",       0x17000, 0x1000, 0xdabb609b )
	ROM_LOAD( "j9.bin",       0x18000, 0x1000, 0x8ea9c6a6 )
	/* the other banks (1900-1fff) are empty */

	ROM_REGION_DISPOSE( 0x1000 ) /* ROM Region 1 -- discarded */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION( 0x10000 ) /* 64k for Z80 sound CPU code */
	ROM_LOAD( "11-7a.bin",    0x0000, 0x1000, 0xb52d01fa )
	ROM_LOAD( "10-8a.bin",    0x1000, 0x1000, 0x9db5c0ce )
ROM_END


ROM_START( tutankst_rom )
	ROM_REGION( 0x20000 )      /* 64k for M6809 CPU code + 64k for ROM banks */
	ROM_LOAD( "h1.bin",       0x0a000, 0x1000, 0xda18679f ) /* program ROMs */
	ROM_LOAD( "h2.bin",       0x0b000, 0x1000, 0xa0f02c85 )
	ROM_LOAD( "ra1_3h.cpu",   0x0c000, 0x1000, 0x2d62d7b1 )
	ROM_LOAD( "h4.bin",       0x0d000, 0x1000, 0xbd06fad0 )
	ROM_LOAD( "h5.bin",       0x0e000, 0x1000, 0xbf9fd9b0 )
	ROM_LOAD( "ra1_6h.cpu",   0x0f000, 0x1000, 0xc43b3865 )
	ROM_LOAD( "j1.bin",       0x10000, 0x1000, 0x7eb59b21 ) /* graphic ROMs (banked) -- only 9 of 12 are filled */
	ROM_LOAD( "j2.bin",       0x11000, 0x1000, 0x6615eff3 )
	ROM_LOAD( "j3.bin",       0x12000, 0x1000, 0xa10d4444 )
	ROM_LOAD( "j4.bin",       0x13000, 0x1000, 0x58cd143c )
	ROM_LOAD( "j5.bin",       0x14000, 0x1000, 0xd7e7ae95 )
	ROM_LOAD( "j6.bin",       0x15000, 0x1000, 0x91f62b82 )
	ROM_LOAD( "j7.bin",       0x16000, 0x1000, 0xafd0a81f )
	ROM_LOAD( "j8.bin",       0x17000, 0x1000, 0xdabb609b )
	ROM_LOAD( "j9.bin",       0x18000, 0x1000, 0x8ea9c6a6 )
	/* the other banks (1900-1fff) are empty */

	ROM_REGION_DISPOSE( 0x1000 ) /* ROM Region 1 -- discarded */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION( 0x10000 ) /* 64k for Z80 sound CPU code */
	ROM_LOAD( "11-7a.bin",    0x0000, 0x1000, 0xb52d01fa )
	ROM_LOAD( "10-8a.bin",    0x1000, 0x1000, 0x9db5c0ce )
ROM_END


static int hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x88a9],"\x03\x58\x40",3) == 0 &&
			memcmp(&RAM[0x88cd],"\x01\x20\x60",3) == 0 &&
			memcmp(&RAM[0x88a6],"\x03\x58\x40",3) == 0)	/* high score */
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x88a9],6*7+7);
			RAM[0x88a6] = RAM[0x88a9];
			RAM[0x88a7] = RAM[0x88aa];
			RAM[0x88a8] = RAM[0x88ab];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0; /* we can't load the hi scores yet */
}

static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x88a9],6*7+7);
		osd_fclose(f);
	}
}


struct GameDriver tutankhm_driver =
{
	__FILE__,
	0,
	"tutankhm",
	"Tutankham (Konami)",
	"1982",
	"Konami",
	"Mirko Buffoni (MAME driver)\nDavid Dahl (hardware info)\nAaron Giles\nMarco Cassili",
	0,
	&machine_driver,
	0,

	tutankhm_rom,
	0, 0,   /* ROM decode and opcode decode functions */
	0,      /* Sample names */
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_ROTATE_90,

	hiload, hisave		        /* High score load and save */
};

struct GameDriver tutankst_driver =
{
	__FILE__,
	&tutankhm_driver,
	"tutankst",
	"Tutankham (Stern)",
	"1982",
	"Stern",
	"Mirko Buffoni (MAME driver)\nDavid Dahl (hardware info)\nAaron Giles\nMarco Cassili",
	0,
	&machine_driver,
	0,

	tutankst_rom,
	0, 0,   /* ROM decode and opcode decode functions */
	0,      /* Sample names */
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_ROTATE_90,

	hiload, hisave		        /* High score load and save */
};
