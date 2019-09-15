/***************************************************************************

Star Force memory map (preliminary)
MAIN BOARD:
0000-7fff ROM
8000-8fff RAM
9000-93ff Video RAM
9400-97ff Color RAM
9800-987f Sprites
9c00-9d7f Palette RAM
a000-a1ff Background Video RAM #1
a800-a9ff Background Video RAM #2
b000-b1ff Background Video RAM #3
b800-bbff RAM ? ( initialize only )

read:
d000      IN0
d001      IN1
d002      IN2
d003      ?
d004      DSW1
d005      DSW2

write:
9e20-9e21 background #1 x position
9e25      background #1 y position
9e28-9e29 background #? x position ??
9e30-9e31 background #2 & #3 x position
9e35      background #2 & #3 y position
d000      ?? (before access paletteram data 0x00 )
d002      watchdog reset?
          IN0/IN1 latch ? ( write before read IN0/IN1 )
d004      sound command ( pio-a )

SOUND BOARD
memory read/write
0000-3fff ROM
4000-43ff RAM

write
8000 sound chip channel 1 1st 9f,bf,df,ff
9000 sound chip channel 2 1st 9f,bf,df,ff
a000 sound chip channel 3 1st 9f,bf,df,ff
d000 bit 0-3 single sound volume ( freq = ctc2 )
e000 ? ( initialize only )
f000 ? ( initialize only )

I/O read/write
00   z80pio-A data     ( from sound command )
01   z80pio-A controll ( mode 1 input )
02   z80pio-B data     ( no use )
03   z80pio-B controll ( mode 3 bit i/o )
08   z80ctc-ch1        ( timer mode cysclk/16, bas clock 15.625KHz )
09   z80ctc-ch2        ( cascade from ctc-1  , tempo interrupt 88.778Hz )
0a   z80ctc-ch3        ( timer mode , single sound freq. )
0b   z80ctc-ch4        ( no use )

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/z80fmly.h"

extern unsigned char *starforc_scrollx2,*starforc_scrolly2;
extern unsigned char *starforc_scrollx3,*starforc_scrolly3;
extern unsigned char *starforc_tilebackground2;
extern unsigned char *starforc_tilebackground3;
extern unsigned char *starforc_tilebackground4;
extern int starforc_bgvideoram_size;
void starforc_tiles2_w(int offset,int data);
void starforc_tiles3_w(int offset,int data);
void starforc_tiles4_w(int offset,int data);

int starforc_vh_start(void);
void starforc_vh_stop(void);

void starforc_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

int starforc_sh_start(void);
void starforc_sh_stop(void);
void starforc_sh_update(void);

void starforc_sh_0_w( int offset , int data );
void starforc_sh_1_w( int offset , int data );
void starforc_sh_2_w( int offset , int data );

void starforc_pio_w( int offset , int data );
int  starforc_pio_r( int offset );

#if 1
void starforc_volume_w( int offset , int data );
#endif

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x97ff, MRA_RAM },
	{ 0xa000, 0xa1ff, MRA_RAM },
	{ 0xa800, 0xa9ff, MRA_RAM },
	{ 0xb000, 0xb1ff, MRA_RAM },
	{ 0xd000, 0xd000, input_port_0_r },	/* player 1 input */
	{ 0xd001, 0xd001, input_port_1_r },	/* player 2 input */
	{ 0xd002, 0xd002, input_port_2_r },	/* coin */
	{ 0xd004, 0xd004, input_port_3_r },	/* DSW1 */
	{ 0xd005, 0xd005, input_port_4_r },	/* DSW2 */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x8fff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9400, 0x97ff, colorram_w, &colorram },
	{ 0x9800, 0x987f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9c00, 0x9d7f, paletteram_IIBBGGRR_w, &paletteram },
	{ 0x9e20, 0x9e21, MWA_RAM, &starforc_scrollx2 },
	{ 0x9e25, 0x9e25, MWA_RAM, &starforc_scrolly2 },
//	{ 0x9e28, 0x9e29, MWA_RAM, &starforc_scrollx? },
	{ 0x9e30, 0x9e31, MWA_RAM, &starforc_scrollx3 },
	{ 0x9e35, 0x9e35, MWA_RAM, &starforc_scrolly3 },
	{ 0xa000, 0xa1ff, starforc_tiles2_w, &starforc_tilebackground2, &starforc_bgvideoram_size },
	{ 0xa800, 0xa9ff, starforc_tiles3_w, &starforc_tilebackground3 },
	{ 0xb000, 0xb1ff, starforc_tiles4_w, &starforc_tilebackground4 },
	{ 0xd004, 0xd004, z80pioA_0_p_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x8000, 0x8000, SN76496_0_w },
	{ 0x9000, 0x9000, SN76496_1_w },
	{ 0xa000, 0xa000, SN76496_2_w },
	{ 0xd000, 0xd000, starforc_volume_w },
#if 0
	{ 0xe000, 0xe000, unknown },
	{ 0xf000, 0xf000, unknown },
#endif
	{ -1 }  /* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x00, 0x03, z80pio_0_r },
	{ 0x08, 0x0b, z80ctc_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x03, z80pio_0_w },
	{ 0x08, 0x0b, z80ctc_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
/* coin input for both must be active between 2 and 9 frames to be consistently recognized */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE, "Coin A", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_COIN2 | IPF_IMPULSE, "Coin B", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_START	/* DSW0 */

	PORT_DIPNAME( 0x03, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x30, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "2" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x07, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "50000 200000 500000" )
	PORT_DIPSETTING(    0x01, "100000 300000 800000" )
	PORT_DIPSETTING(    0x02, "50000 200000" )
	PORT_DIPSETTING(    0x03, "100000 300000" )
	PORT_DIPSETTING(    0x04, "50000" )
	PORT_DIPSETTING(    0x05, "100000" )
	PORT_DIPSETTING(    0x06, "200000" )
	PORT_DIPSETTING(    0x07, "None" )
	PORT_DIPNAME( 0x38, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easiest" )
	PORT_DIPSETTING(    0x08, "Easy" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x18, "Difficult" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x28, "Hardest" )
	/* 0x30 and x038 are unused */
	PORT_DIPNAME( 0x40, 0x00, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	3,	/* 3 bits per pixel */
	{ 0, 512*8*8, 2*512*8*8 },	/* the bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout charlayout2 =
{
	16,16,	/* 16*16 characters */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel */
	{ 2*256*16*16, 256*16*16, 0 },	/* the bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every character takes 32 consecutive bytes */
};
static struct GfxLayout charlayout3 =
{
	16,16,	/* 16*16 characters */
	128,	/* 128 characters */
	3,	/* 3 bits per pixel */
	{ 2*128*16*16, 128*16*16, 0 },	/* the bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every character takes 32 consecutive bytes */
};
static struct GfxLayout spritelayout1 =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 2*512*16*16, 512*16*16, 0 },	/* the bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
static struct GfxLayout spritelayout2 =
{
	32,32,	/* 32*32 sprites */
	64,	/* 64 sprites */
	3,	/* 3 bits per pixel */
	{ 2*128*32*32, 128*32*32, 0 },	/* the bitplanes are separated */
	{ 87*8, 86*8, 85*8, 84*8, 83*8, 82*8, 81*8, 80*8,
			71*8, 70*8, 69*8, 68*8, 67*8, 66*8, 65*8, 64*8,
			23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* pretty straightforward layout */
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
			40*8+0, 40*8+1, 40*8+2, 40*8+3, 40*8+4, 40*8+5, 40*8+6, 40*8+7 },
	128*8	/* every sprite takes 128 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout1,     0, 8 },	/*   0- 63 characters */
	{ 1, 0x03000, &charlayout2,   128, 8 },	/* 128-191 background #1 */
	{ 1, 0x09000, &charlayout2,    64, 8 },	/*  64-127 background #2 */
	{ 1, 0x0f000, &charlayout3,   192, 8 },	/* 192-255 star background */
	{ 1, 0x12000, &spritelayout1, 320, 8 },	/* 320-383 normal sprites */
	{ 1, 0x14000, &spritelayout2, 320, 8 },	/* 320-383 large sprites */
	{ -1 } /* end of array */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,        /* 2 Mhz */
			2,
			sound_readmem,sound_writemem,
			sound_readport,sound_writeport,
			0,0 /* interrupts are made by z80 daisy chain system */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0, 32*8-1 },
	gfxdecodeinfo,
	384, 384,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	starforc_vh_start,
	starforc_vh_stop,
	starforc_vh_screenrefresh,

	/* sound hardware */
	0,
	starforc_sh_start,
	starforc_sh_stop,
	starforc_sh_update,
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( starforc_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "starforc.3",   0x0000, 0x4000, 0x8ba27691 )
	ROM_LOAD( "starforc.2",   0x4000, 0x4000, 0x0fc4d2d6 )

	ROM_REGION_DISPOSE(0x1e000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "starforc.7",   0x00000, 0x1000, 0xf4803339 )
	ROM_LOAD( "starforc.8",   0x01000, 0x1000, 0x96979684 )
	ROM_LOAD( "starforc.9",   0x02000, 0x1000, 0xeead1d5c )
	ROM_LOAD( "starforc.10",  0x03000, 0x2000, 0xc62a19c1 )
	ROM_LOAD( "starforc.11",  0x05000, 0x2000, 0x668aea14 )
	ROM_LOAD( "starforc.12",  0x07000, 0x2000, 0xfdd9e38b )
	ROM_LOAD( "starforc.13",  0x09000, 0x2000, 0x84603285 )
	ROM_LOAD( "starforc.14",  0x0b000, 0x2000, 0x9e9384fe )
	ROM_LOAD( "starforc.15",  0x0d000, 0x2000, 0xc3bda12f )
	ROM_LOAD( "starforc.16",  0x0f000, 0x1000, 0xce20b469 )
	ROM_LOAD( "starforc.17",  0x10000, 0x1000, 0x68c60d0f )
	ROM_LOAD( "starforc.18",  0x11000, 0x1000, 0x6455c3ad )
	ROM_LOAD( "starforc.4",   0x12000, 0x4000, 0xdd9d68a4 )
	ROM_LOAD( "starforc.5",   0x16000, 0x4000, 0xf71717f8 )
	ROM_LOAD( "starforc.6",   0x1a000, 0x4000, 0x5468a21d )

	ROM_REGION(0x10000)     /* 64k for sound board */
	ROM_LOAD( "starforc.1",   0x0000, 0x2000, 0x2735bb22 )
ROM_END

ROM_START( megaforc_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "mf3.bin",      0x0000, 0x4000, 0xd3ea82ec )
	ROM_LOAD( "mf2.bin",      0x4000, 0x4000, 0xaa320718 )

	ROM_REGION_DISPOSE(0x1e000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "mf7.bin",      0x00000, 0x1000, 0x43ef8d20 )
	ROM_LOAD( "mf8.bin",      0x01000, 0x1000, 0xc36fb746 )
	ROM_LOAD( "mf9.bin",      0x02000, 0x1000, 0x62e7c9ec )
	ROM_LOAD( "starforc.10",  0x03000, 0x2000, 0xc62a19c1 )
	ROM_LOAD( "starforc.11",  0x05000, 0x2000, 0x668aea14 )
	ROM_LOAD( "starforc.12",  0x07000, 0x2000, 0xfdd9e38b )
	ROM_LOAD( "starforc.13",  0x09000, 0x2000, 0x84603285 )
	ROM_LOAD( "starforc.14",  0x0b000, 0x2000, 0x9e9384fe )
	ROM_LOAD( "starforc.15",  0x0d000, 0x2000, 0xc3bda12f )
	ROM_LOAD( "starforc.16",  0x0f000, 0x1000, 0xce20b469 )
	ROM_LOAD( "starforc.17",  0x10000, 0x1000, 0x68c60d0f )
	ROM_LOAD( "starforc.18",  0x11000, 0x1000, 0x6455c3ad )
	ROM_LOAD( "starforc.4",   0x12000, 0x4000, 0xdd9d68a4 )
	ROM_LOAD( "starforc.5",   0x16000, 0x4000, 0xf71717f8 )
	ROM_LOAD( "starforc.6",   0x1a000, 0x4000, 0x5468a21d )

	ROM_REGION(0x10000)     /* 64k for sound board */
	ROM_LOAD( "starforc.1",   0x0000, 0x2000, 0x2735bb22 )
ROM_END



static int hiload(void)
{
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        if (memcmp(&RAM[0x8348],"\x00\x08\x05\x00",4) == 0 &&
        memcmp(&RAM[0x9181],"\x18",1) == 0 &&
        memcmp(&RAM[0x91a1],"\x18",1) == 0 &&
        memcmp(&RAM[0x91c1],"\x21",1) == 0 &&
        memcmp(&RAM[0x91e1],"\x18",1) == 0 &&
        memcmp(&RAM[0x9201],"\x1d",1) == 0)

        {
                void *f;

                if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
                {
                        int p,temp;

                        osd_fread(f,&RAM[0x8038],112);
                        RAM[0x8348] = RAM[0x803d];
                        RAM[0x8349] = RAM[0x803c];
                        RAM[0x834a] = RAM[0x803b];
                        RAM[0x834b] = RAM[0x803a];

                        for (p=0;p<4;p++)
                           {
                            temp=0x18+(RAM[0x8348+p]%16);
                            if (temp>=0x20) temp++;
                            RAM[0x9181+(p*0x40)]=temp;
                            temp=0x18+(RAM[0x8348+p]/16);
                            if (temp>=0x20) temp++;
                            RAM[0x91A1+(p*0x40)]=temp;
                            }

                        for (p=0;p<8;p++ )
                        {
                                if (RAM[0x9261-(p*0x20)]==0x18) RAM[0x9261-(p*0x20)]=0x23;
                                else break;
                        }

                        osd_fclose(f);
                }

                return 1;
        }
        else return 0;  /* we can't load the hi scores yet */

}



static void hisave(void)
{
        void *f;
        int i;
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* Bug to solve the problem about resetting in the hi-score screen */
        for (i = 0x8039; i < 0x80a0; i+=0x0b)
                        if (RAM[i] == 0x02) RAM[i] = 0x01;

        if ((RAM[0x8038] != 0) &&
            ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0))


        {
                osd_fwrite(f,&RAM[0x8038],112);

                osd_fclose(f);
        }
}



struct GameDriver starforc_driver =
{
	__FILE__,
	0,
	"starforc",
	"Star Force",
	"1984",
	"Tehkan",
	"Mirko Buffoni\nNicola Salmoria\nTatsuyuki Satoh\nJuan Carlos Lorente\nMarco Cassili",
	0,
	&machine_driver,
	0,

	starforc_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver megaforc_driver =
{
	__FILE__,
	&starforc_driver,
	"megaforc",
	"Mega Force",
	"1985",
	"Tehkan (Video Ware license)",
	"Mirko Buffoni\nNicola Salmoria\nTatsuyuki Satoh\nJuan Carlos Lorente\nMarco Cassili",
	0,
	&machine_driver,
	0,

	megaforc_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
