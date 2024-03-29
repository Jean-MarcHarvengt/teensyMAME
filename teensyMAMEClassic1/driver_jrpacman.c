/***************************************************************************

Jr. Pac Man memory map (preliminary)

0000-3fff ROM
4000-47ff Video RAM (also color RAM)
4800-4fff RAM
8000-dfff ROM

memory mapped ports:

read:
5000      IN0
5040      IN1
5080      DSW1

*
 * IN0 (all bits are inverted)
 * bit 7 : CREDIT
 * bit 6 : COIN 2
 * bit 5 : COIN 1
 * bit 4 : RACK TEST
 * bit 3 : DOWN player 1
 * bit 2 : RIGHT player 1
 * bit 1 : LEFT player 1
 * bit 0 : UP player 1
 *
*
 * IN1 (all bits are inverted)
 * bit 7 : TABLE or UPRIGHT cabinet select (1 = UPRIGHT)
 * bit 6 : START 2
 * bit 5 : START 1
 * bit 4 : TEST SWITCH
 * bit 3 : DOWN player 2 (TABLE only)
 * bit 2 : RIGHT player 2 (TABLE only)
 * bit 1 : LEFT player 2 (TABLE only)
 * bit 0 : UP player 2 (TABLE only)
 *
*
 * DSW1 (all bits are inverted)
 * bit 7 :  ?
 * bit 6 :  difficulty level
 *                       1 = Normal  0 = Harder
 * bit 5 :\ bonus pac at xx000 pts
 * bit 4 :/ 00 = 10000  01 = 15000  10 = 20000  11 = 30000
 * bit 3 :\ nr of lives
 * bit 2 :/ 00 = 1  01 = 2  10 = 3  11 = 5
 * bit 1 :\ play mode
 * bit 0 :/ 00 = free play   01 = 1 coin 1 credit
 *          10 = 1 coin 2 credits   11 = 2 coins 1 credit
 *

write:
4ff2-4ffd 6 pairs of two bytes:
          the first byte contains the sprite image number (bits 2-7), Y flip (bit 0),
		  X flip (bit 1); the second byte the color
5000      interrupt enable
5001      sound enable
5002      unused
5003      flip screen
5004      unused
5005      unused
5006      unused
5007      coin counter
5040-5044 sound voice 1 accumulator (nibbles) (used by the sound hardware only)
5045      sound voice 1 waveform (nibble)
5046-5049 sound voice 2 accumulator (nibbles) (used by the sound hardware only)
504a      sound voice 2 waveform (nibble)
504b-504e sound voice 3 accumulator (nibbles) (used by the sound hardware only)
504f      sound voice 3 waveform (nibble)
5050-5054 sound voice 1 frequency (nibbles)
5055      sound voice 1 volume (nibble)
5056-5059 sound voice 2 frequency (nibbles)
505a      sound voice 2 volume (nibble)
505b-505e sound voice 3 frequency (nibbles)
505f      sound voice 3 volume (nibble)
5062-506d Sprite coordinates, x/y pairs for 6 sprites
5070      palette bank
5071      colortable bank
5073      background priority over sprites
5074      char gfx bank
5075      sprite gfx bank
5080      scroll
50c0      Watchdog reset

I/O ports:
OUT on port $0 sets the interrupt vector


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


void jrpacman_init_machine(void);
int jrpacman_interrupt(void);

extern unsigned char *jrpacman_scroll,*jrpacman_bgpriority;
extern unsigned char *jrpacman_charbank,*jrpacman_spritebank;
extern unsigned char *jrpacman_palettebank,*jrpacman_colortablebank;
void jrpacman_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int jrpacman_vh_start(void);
void jrpacman_vh_stop(void);
void jrpacman_videoram_w(int offset,int data);
void jrpacman_palettebank_w(int offset,int data);
void jrpacman_colortablebank_w(int offset,int data);
void jrpacman_charbank_w(int offset,int data);
void jrpacman_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern unsigned char *pengo_soundregs;
void pengo_sound_enable_w(int offset,int data);
void pengo_sound_w(int offset,int data);



static struct MemoryReadAddress readmem[] =
{
	{ 0x4000, 0x4fff, MRA_RAM },	/* including video and color RAM */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0xdfff, MRA_ROM },
	{ 0x5000, 0x503f, input_port_0_r },	/* IN0 */
	{ 0x5040, 0x507f, input_port_1_r },	/* IN1 */
	{ 0x5080, 0x50bf, input_port_2_r },	/* DSW1 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x4800, 0x4fef, MWA_RAM },
	{ 0x4000, 0x47ff, jrpacman_videoram_w, &videoram, &videoram_size },
	{ 0x5040, 0x505f, pengo_sound_w, &pengo_soundregs },
	{ 0x4ff0, 0x4fff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x5060, 0x506f, MWA_RAM, &spriteram_2 },
	{ 0x5080, 0x5080, MWA_RAM, &jrpacman_scroll },
	{ 0x5070, 0x5070, jrpacman_palettebank_w, &jrpacman_palettebank },
	{ 0x5071, 0x5071, jrpacman_colortablebank_w, &jrpacman_colortablebank },
	{ 0x5073, 0x5073, MWA_RAM, &jrpacman_bgpriority },
	{ 0x5074, 0x5074, jrpacman_charbank_w, &jrpacman_charbank },
	{ 0x5075, 0x5075, MWA_RAM, &jrpacman_spritebank },
	{ 0x5000, 0x5000, interrupt_enable_w },
	{ 0x50c0, 0x50c0, MWA_NOP },
	{ 0x5001, 0x5001, pengo_sound_enable_w },
	{ 0x5002, 0x5007, MWA_NOP },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0xdfff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct IOWritePort writeport[] =
{
	{ 0, 0, interrupt_vector_w },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN3 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BITX(    0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x01, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x0c, 0x08, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x30, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x10, "15000" )
	PORT_DIPSETTING(    0x20, "20000" )
	PORT_DIPSETTING(    0x30, "30000" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the fire button */
	/* and activate the speedup cheat if it is. */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Speedup Cheat", OSD_KEY_LCONTROL, OSD_JOY_FIRE1, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,
	512,	/* 512 characters */
	2,		/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },	/* bits are packed in groups of four */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,		/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,      0, 128 },
	{ 1, 0x2000, &spritelayout,    0, 128 },
	{ -1 } /* end of array */
};



static struct namco_interface namco_interface =
{
	3072000/32,	/* sample rate */
	3,			/* number of voices */
	32,			/* gain adjustment */
	255,		/* playback volume */
	3			/* memory region */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			18432000/6,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,writeport,
			jrpacman_interrupt,1
		}
	},
	60, 2500,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	jrpacman_init_machine,

	/* video hardware */
	36*8, 28*8, { 0*8, 36*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	32,128*4,
	jrpacman_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	jrpacman_vh_start,
	jrpacman_vh_stop,
	jrpacman_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_NAMCO,
			&namco_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( jrpacman_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "jrp8d.bin",    0x0000, 0x2000, 0xe3fa972e )
	ROM_LOAD( "jrp8e.bin",    0x2000, 0x2000, 0xec889e94 )
	ROM_LOAD( "jrp8h.bin",    0x8000, 0x2000, 0x35f1fc6e )
	ROM_LOAD( "jrp8j.bin",    0xa000, 0x2000, 0x9737099e )
	ROM_LOAD( "jrp8k.bin",    0xc000, 0x2000, 0x5252dd97 )

	ROM_REGION_DISPOSE(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "jrp2c.bin",    0x0000, 0x2000, 0x0527ff9b )
	ROM_LOAD( "jrp2e.bin",    0x2000, 0x2000, 0x73477193 )

	ROM_REGION(0x0140)	/* color proms */
	ROM_LOAD( "jrpacman.9e",  0x0000, 0x0020, 0x90012b3f ) /* palette low bits */
	ROM_LOAD( "jrpacman.9f",  0x0020, 0x0020, 0x8300178e ) /* palette high bits */
	ROM_LOAD( "jrpacman.9p",  0x0040, 0x0100, 0x9f6ea9d8 ) /* color lookup table */

	ROM_REGION(0x0100)	/* sound prom */
	/* I don't know if this is correct. I'm using the Pac Man one. */
	ROM_LOAD( "pacman.spr",   0x0000, 0x0100, 0xa9cc86bf )
ROM_END



static void jrpacman_decode(void)
{
	/* The encryption PALs garble bits 0, 2 and 7 of the ROMs. The encryption */
	/* scheme is complex (basically it's a state machine) and can only be */
	/* faithfully emulated at run time. To avoid the performance hit that would */
	/* cause, here we have a table of the values which must be XORed with */
	/* each memory region to obtain the decrypted bytes. */
	/* Decryption table provided by David Caldwell (david@indigita.com) */
	/* For an accurate reproduction of the encryption, see jrcrypt.c */
	struct {
	    int count;
	    int value;
	} table[] =
	{
		{ 0x00C1, 0x00 },{ 0x0002, 0x80 },{ 0x0004, 0x00 },{ 0x0006, 0x80 },
		{ 0x0003, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0004, 0x80 },
		{ 0x9968, 0x00 },{ 0x0001, 0x80 },{ 0x0002, 0x00 },{ 0x0001, 0x80 },
		{ 0x0009, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0001, 0x80 },
		{ 0x00AF, 0x00 },{ 0x000E, 0x04 },{ 0x0002, 0x00 },{ 0x0004, 0x04 },
		{ 0x001E, 0x00 },{ 0x0001, 0x80 },{ 0x0002, 0x00 },{ 0x0001, 0x80 },
		{ 0x0002, 0x00 },{ 0x0002, 0x80 },{ 0x0009, 0x00 },{ 0x0002, 0x80 },
		{ 0x0009, 0x00 },{ 0x0002, 0x80 },{ 0x0083, 0x00 },{ 0x0001, 0x04 },
		{ 0x0001, 0x01 },{ 0x0001, 0x00 },{ 0x0002, 0x05 },{ 0x0001, 0x00 },
		{ 0x0003, 0x04 },{ 0x0003, 0x01 },{ 0x0002, 0x00 },{ 0x0001, 0x04 },
		{ 0x0003, 0x01 },{ 0x0003, 0x00 },{ 0x0003, 0x04 },{ 0x0001, 0x01 },
		{ 0x002E, 0x00 },{ 0x0078, 0x01 },{ 0x0001, 0x04 },{ 0x0001, 0x05 },
		{ 0x0001, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },
		{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },{ 0x0001, 0x01 },
		{ 0x0001, 0x04 },{ 0x0002, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },
		{ 0x0001, 0x05 },{ 0x0001, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },
		{ 0x0002, 0x00 },{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0002, 0x00 },
		{ 0x0001, 0x01 },{ 0x0001, 0x04 },{ 0x0001, 0x05 },{ 0x0001, 0x00 },
		{ 0x01B0, 0x01 },{ 0x0001, 0x00 },{ 0x0002, 0x01 },{ 0x00AD, 0x00 },
		{ 0x0031, 0x01 },{ 0x005C, 0x00 },{ 0x0005, 0x01 },{ 0x604E, 0x00 },
	    { 0,0 }
	};
	int i,j,A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	A = 0;
	i = 0;
	while (table[i].count)
	{
		for (j = 0;j < table[i].count;j++)
		{
			RAM[A] ^= table[i].value;
			A++;
		}
		i++;
	}
}


static int hiload(void)
{
	static int resetcount;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* during a reset, leave time to the game to clear the screen */
	if (++resetcount < 60) return 0;

	/* wait for "HIGH SCORE" to be on screen */
	if ((memcmp(&RAM[0x476d],"\x40\x40\x40\x40",4) == 0) &&
            (memcmp(&RAM[0x4751],"\x48\x47\x49\x48",4) == 0))
	{
		void *f;


		resetcount = 0;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			char buf[10];
			int hi;


			osd_fread(f,&RAM[0x4e88],4);
			/* also copy the high score to the screen, otherwise it won't be */
			/* updated */
			hi = (RAM[0x4e88] & 0x0f) +
			     (RAM[0x4e88] >> 4) * 10 +
			     (RAM[0x4e89] & 0x0f) * 100 +
			     (RAM[0x4e89] >> 4) * 1000 +
			     (RAM[0x4e8a] & 0x0f) * 10000 +
			     (RAM[0x4e8a] >> 4) * 100000 +
			     (RAM[0x4e8b] & 0x0f) * 1000000 +
			     (RAM[0x4e8b] >> 4) * 10000000;
			if (hi)
			{
				sprintf(buf,"%8d",hi);
				if (buf[2] != ' ') jrpacman_videoram_w(0x0772,buf[2]-'0');
				if (buf[3] != ' ') jrpacman_videoram_w(0x0771,buf[3]-'0');
				if (buf[4] != ' ') jrpacman_videoram_w(0x0770,buf[4]-'0');
				if (buf[5] != ' ') jrpacman_videoram_w(0x076f,buf[5]-'0');
				if (buf[6] != ' ') jrpacman_videoram_w(0x076e,buf[6]-'0');
				jrpacman_videoram_w(0x076d,buf[7]-'0');
			}
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x4e88],4);
		osd_fclose(f);
	}
}



struct GameDriver jrpacman_driver =
{
	__FILE__,
	0,
	"jrpacman",
	"Jr. Pac Man",
	"1983",
	"Bally Midway",
	"David Caldwell\nNicola Salmoria\nMarco Cassili",
	0,
	&machine_driver,
	0,

	jrpacman_rom,
	jrpacman_decode, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	hiload, hisave
};
