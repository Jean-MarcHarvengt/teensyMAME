/***************************************************************************

  Speed Rumbler

  Driver provided by Paul Leaman

  M6809 for game, Z80 and YM-2203 for sound.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809/m6809.h"

extern unsigned char *srumbler_backgroundram;
extern int srumbler_backgroundram_size;
extern unsigned char *srumbler_scrollx;
extern unsigned char *srumbler_scrolly;

void srumbler_bankswitch_w(int offset,int data);
void srumbler_background_w(int offset,int data);
void srumbler_4009_w(int offset,int data);

int srumbler_interrupt(void);

int  srumbler_vh_start(void);
void srumbler_vh_stop(void);
void srumbler_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



void srumbler_bankswitch_w(int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	cpu_setbank (1, &RAM[0x10000+(data&0x0f)*0x9000]);
}

int srumbler_interrupt(void)
{
	if (cpu_getiloops()==0)
	{
		return interrupt();
	}
	else
	{
		return M6809_INT_FIRQ;
	}
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_RAM },   /* RAM (of 1 sort or another) */
	{ 0x4008, 0x4008, input_port_0_r },
	{ 0x4009, 0x4009, input_port_1_r },
	{ 0x400a, 0x400a, input_port_2_r },
	{ 0x400b, 0x400b, input_port_3_r },
	{ 0x400c, 0x400c, input_port_4_r },
	{ 0x5000, 0xdfff, MRA_BANK1},  /* Banked ROM */
	{ 0xe000, 0xffff, MRA_ROM },   /* ROM */
	{ -1 }  /* end of table */
};

/*
The "scroll test" routine on the test screen appears to overflow and write
over the control registers (0x4000-0x4080) when it clears the screen.

This doesn't affect anything since it happens to write the correct value
to the page register.

Ignore the warnings about writing to unmapped memory.
*/

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x1dff, MWA_RAM },
	{ 0x1e00, 0x1fff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x2000, 0x3fff, srumbler_background_w, &srumbler_backgroundram, &srumbler_backgroundram_size },
	{ 0x4008, 0x4008, srumbler_bankswitch_w},
	{ 0x4009, 0x4009, srumbler_4009_w},
	{ 0x400a, 0x400b, MWA_RAM, &srumbler_scrollx},
	{ 0x400c, 0x400d, MWA_RAM, &srumbler_scrolly},
	{ 0x400e, 0x400e, soundlatch_w},
	{ 0x5000, 0x5fff, videoram_w, &videoram, &videoram_size },
	{ 0x6000, 0x6fff, MWA_RAM }, /* Video RAM 2 ??? (not used) */
	{ 0x7100, 0x73ff, paletteram_RRRRGGGGBBBBxxxx_swap_w, &paletteram },
	{ 0x7400, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0xe000, 0xe000, soundlatch_r },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0x0000, 0x7fff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0x8000, 0x8000, YM2203_control_port_0_w },
	{ 0x8001, 0x8001, YM2203_write_port_0_w },
	{ 0xa000, 0xa000, YM2203_control_port_1_w },
	{ 0xa001, 0xa001, YM2203_write_port_1_w },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x07, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x07, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x06, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/6 Credits" )
	PORT_DIPNAME( 0x38, 0x38, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x10, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x38, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x30, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x28, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x20, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x18, "1 Coin/6 Credits" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x04, "Cocktail" )
	PORT_DIPNAME( 0x18, 0x18, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "20000, 70000 every 70000" )
	PORT_DIPSETTING(    0x10, "30000, 80000 every 80000" )
	PORT_DIPSETTING(    0x08, "20000, 80000" )
	PORT_DIPSETTING(    0x00, "30000, 80000" )
	PORT_DIPNAME( 0x60, 0x60, "Level", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "0" )
	PORT_DIPSETTING(    0x60, "10" )
	PORT_DIPSETTING(    0x20, "20" )
	PORT_DIPSETTING(    0x00, "30" )
	PORT_DIPNAME( 0x80, 0x80, "Music", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	2,      /* 2 bits per pixel */
	{ 4, 0 },
	{ 0, 1, 2, 3, 8+0,8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout tilelayout =
{
	16,16,  /* 16*16 tiles */
	2048,   /* 2048  tiles */
	4,      /* 4 bits per pixel */
	{ 0x20000*8+4, 0x20000*8+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	2048,   /* 2048 sprites */
	4,      /* 4 bits per pixel */
	{ 0x30000*8, 0x20000*8, 0x10000*8, 0   },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			2*64+0, 2*64+1, 2*64+2, 2*64+3, 2*64+4, 2*64+5, 2*64+6, 2*64+7, 2*64+8 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32*8 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/*   start    pointer       colour start   number of colours */
	{ 1, 0x00000, &charlayout,   320, 16 }, /* colors 320 - 383 */
	{ 1, 0x10000, &tilelayout,     0,  8 }, /* colors   0 - 127 */
	{ 1, 0x50000, &spritelayout, 128,  8 }, /* colors 128 - 255 */
	{ -1 } /* end of array */
};

/*

    SPEED-RUMBLER Paging system.
    ===========================

    This is quite complex.

    The area from 0xe000-0xffff is resident all the time. The area from
    0x5000-0xdfff is paged in and out.

    The following map is derived from the ROM test routine. The routine
    tests the ROM chips in the following order.

    1=RC4
    2=RC3
    3=RC2
    4=RC1
    5=RC9
    6=RC8
    7=RC7
    8=RC6

    The numbers on the bars on the paging map refer to the ROM test
    order. This is also the order that the ROM chips are loaded into MAME.

    For example, page 0 consists of rom-test numbers 2 and 8.
    This means it uses elements from RC3 and RC6.

    All locations under the arrows correspond to the ROM test and should be
    correct (since the ROMs pass their tests). It may be necessary to
    swap two blocks of the same size and number.

    Any locations not under the arrows are not covered by the ROM test. I
    believe that these come from either page 0 or 5. Some of these, I know
    are correct, some may not be.
*/

static int page_table[16][9]=
{
//  Arcade machine ROM location
//  0x5000  0x6000  0x7000  0x8000  0x9000  0xa000  0xb000  0xc000  0xd000

// <======= 2=====> <============ 8 ==============> <==2==> <=======2======>
   {0x08000,0x09000,0x3c000,0x3d000,0x3e000,0x3f000,0x0b000,0x0e000,0x0f000}, // 00
//                  <============ 7 ==============>
   {0x08000,0x09000,0x30000,0x31000,0x32000,0x33000,0x0b000,0x0e000,0x0f000}, // 01
//                  <============ 7 ==============>
   {0x08000,0x09000,0x34000,0x35000,0x36000,0x37000,0x0b000,0x0e000,0x0f000}, // 02
//                  <============ 8 ==============>
   {0x08000,0x09000,0x38000,0x39000,0x3a000,0x3b000,0x0b000,0x0e000,0x0f000}, // 03
//                                  <==2==> <==3==>
   {0x08000,0x09000,0x3c000,0x3d000,0x0a000,0x16000,0x0b000,0x0e000,0x0f000}, // 04
// <=============== 1 ============> <============ 1 ==============> <==3==>
   {0x00000,0x01000,0x02000,0x03000,0x04000,0x05000,0x06000,0x07000,0x17000}, // 05
//                                  <===== 3 =====>
   {0x00000,0x01000,0x02000,0x03000,0x10000,0x11000,0x06000,0x07000,0x17000}, // 06
//                                  <============== 3 ============>
   {0x00000,0x01000,0x02000,0x03000,0x12000,0x13000,0x14000,0x15000,0x17000}, // 07
//  <================================ 4 ==========================>
   {0x18000,0x19000,0x1a000,0x1b000,0x1c000,0x1d000,0x1e000,0x1f000,0x17000}, // 08
//                                  <============== 5 ============>
   {0x00000,0x01000,0x02000,0x03000,0x20000,0x21000,0x22000,0x23000,0x17000}, // 09
//                                  <============== 5 ============>
   {0x00000,0x01000,0x02000,0x03000,0x24000,0x25000,0x26000,0x27000,0x17000}, // 0a
//                                  <============== 6 ============>
   {0x00000,0x01000,0x02000,0x03000,0x28000,0x29000,0x2a000,0x2b000,0x17000}, // 0b
//                                  <============== 6 ============>
   {0x00000,0x01000,0x02000,0x03000,0x2c000,0x2d000,0x2e000,0x2f000,0x17000}, // 0c

   /* Empty pages, kept to simplify the paging formula in the machine driver */
   {0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, 0xfffff, 0xfffff, 0xfffff, 0xfffff}, // 0d
   {0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, 0xfffff, 0xfffff, 0xfffff, 0xfffff}, // 0e
   {0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, 0xfffff, 0xfffff, 0xfffff, 0xfffff}  // 0f
};



static void srumbler_init_machine(void)
{
     /*
     Use the paging map to copy the ROM blocks into a more usable format.
     The machine driver uses blocks of 0x9000 bytes long.
     */

    int j, i;
    unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
    unsigned char *pROM = Machine->memory_region[3];

    /* Set optimization flags for M6809 */
    m6809_Flags = M6809_FAST_NONE;

    /* Resident ROM area e000-ffff */
    memcpy(&RAM[0xe000], pROM+0x0c000, 0x2000);

    /* Region 5000-dfff contains paged ROMS */
    for (j=0; j<16; j++)        /* 16 Pages */
    {
	for (i=0; i<9; i++)     /* 9 * 0x1000 blocks */
	{
	    int nADDR=page_table[j][i];
	    unsigned char *p=&RAM[0x10000+0x09000*j+i*0x1000];

	    if (nADDR == 0xfffff)
	    {
		/* Fill unassigned regions with an illegal M6809 opcode (1) */
		memset(p, 1, 0x1000);
	    }
	    else
	    {
		memcpy(p, pROM+nADDR, 0x01000);
	    }
	}
    }

    /* Patch out startup test */
//    RAM[0xe039]=12;
//    RAM[0xe03a]=12;
//    RAM[0xe03b]=12;
}



static struct YM2203interface ym2203_interface =
{
	2,                      /* 2 chips */
	2000000,        /* 2.0 MHz (? hand tuned to match the real board) */
	{ YM2203_VOL(255,255), YM2203_VOL(255,255) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			1500000,        /* 1.5 Mhz (?) */
			0,
			readmem,writemem,0,0,
			srumbler_interrupt,2
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,        /* 3 Mhz ??? */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			interrupt,4
		}
	},
	60, 2500,       /* frames per second, vblank duration */
				/* hand tuned to get rid of sprite lag */
	1,      /* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	srumbler_init_machine,

	/* video hardware */
	64*8, 32*8, { 10*8, (64-10)*8-1, 1*8, 31*8-1 },

	gfxdecodeinfo,
	384, 384,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	srumbler_vh_start,
	srumbler_vh_stop,
	srumbler_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( srumbler_rom )
	ROM_REGION(0x10000+0x9000*16)  /* 64k for code + banked ROM images */
	/* empty, will be filled later */

	ROM_REGION_DISPOSE(0x90000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "6g_sr10.bin",  0x00000, 0x4000, 0xadabe271 ) /* characters */
	ROM_LOAD( "11a_sr11.bin", 0x10000, 0x8000, 0x5fa042ba ) /* tiles */
	ROM_LOAD( "13a_sr12.bin", 0x18000, 0x8000, 0xa2db64af )
	ROM_LOAD( "14a_sr13.bin", 0x20000, 0x8000, 0xf1df5499 )
	ROM_LOAD( "15a_sr14.bin", 0x28000, 0x8000, 0xb22b31b3 )
	ROM_LOAD( "11c_sr15.bin", 0x30000, 0x8000, 0xca3a3af3 )
	ROM_LOAD( "13c_sr16.bin", 0x38000, 0x8000, 0xc49a4a11 )
	ROM_LOAD( "14c_sr17.bin", 0x40000, 0x8000, 0xaa80aaab )
	ROM_LOAD( "15c_sr18.bin", 0x48000, 0x8000, 0xce67868e )
	ROM_LOAD( "15e_sr20.bin", 0x50000, 0x8000, 0x3924c861 ) /* sprites */
	ROM_LOAD( "14e_sr19.bin", 0x58000, 0x8000, 0xff8f9129 )
	ROM_LOAD( "15f_sr22.bin", 0x60000, 0x8000, 0xab64161c )
	ROM_LOAD( "14f_sr21.bin", 0x68000, 0x8000, 0xfd64bcd1 )
	ROM_LOAD( "15h_sr24.bin", 0x70000, 0x8000, 0xc972af3e )
	ROM_LOAD( "14h_sr23.bin", 0x78000, 0x8000, 0x8c9abf57 )
	ROM_LOAD( "15j_sr26.bin", 0x80000, 0x8000, 0xd4f1732f )
	ROM_LOAD( "14j_sr25.bin", 0x88000, 0x8000, 0xd2a4ea4f )

	ROM_REGION(0x10000) /* 64k for the audio CPU */
	ROM_LOAD( "2f_sr05.bin",  0x0000, 0x8000, 0x0177cebe )

	ROM_REGION(0x40000) /* Paged ROMs */
	ROM_LOAD( "14e_sr04.bin", 0x00000, 0x08000, 0xa68ce89c )  /* RC4 */
	ROM_LOAD( "13e_sr03.bin", 0x08000, 0x08000, 0x87bda812 )  /* RC3 */
	ROM_LOAD( "12e_sr02.bin", 0x10000, 0x08000, 0xd8609cca )  /* RC2 */
	ROM_LOAD( "11e_sr01.bin", 0x18000, 0x08000, 0x27ec4776 )  /* RC1 */
	ROM_LOAD( "14f_sr09.bin", 0x20000, 0x08000, 0x2146101d )  /* RC9 */
	ROM_LOAD( "13f_sr08.bin", 0x28000, 0x08000, 0x838369a6 )  /* RC8 */
	ROM_LOAD( "12f_sr07.bin", 0x30000, 0x08000, 0xde785076 )  /* RC7 */
	ROM_LOAD( "11f_sr06.bin", 0x38000, 0x08000, 0xa70f4fd4 )  /* RC6 */

	ROM_REGION(0x0300) /* Proms (not used for now.. Transparency???) */
	ROM_LOAD( "63s141.12a",   0x00000, 0x00100, 0x8421786f )
	ROM_LOAD( "63s141.13a",   0x00100, 0x00100, 0x6048583f )
	ROM_LOAD( "63s141.8j",    0x00200, 0x00100, 0x1a89a7ff )
ROM_END


ROM_START( srumblr2_rom )
	ROM_REGION(0x10000+0x9000*16)  /* 64k for code + banked ROM images */
	/* empty, will be filled later */

	ROM_REGION_DISPOSE(0x90000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "6g_sr10.bin",  0x00000, 0x4000, 0xadabe271 ) /* characters */
	ROM_LOAD( "11a_sr11.bin", 0x10000, 0x8000, 0x5fa042ba ) /* tiles */
	ROM_LOAD( "13a_sr12.bin", 0x18000, 0x8000, 0xa2db64af )
	ROM_LOAD( "14a_sr13.bin", 0x20000, 0x8000, 0xf1df5499 )
	ROM_LOAD( "15a_sr14.bin", 0x28000, 0x8000, 0xb22b31b3 )
	ROM_LOAD( "11c_sr15.bin", 0x30000, 0x8000, 0xca3a3af3 )
	ROM_LOAD( "13c_sr16.bin", 0x38000, 0x8000, 0xc49a4a11 )
	ROM_LOAD( "14c_sr17.bin", 0x40000, 0x8000, 0xaa80aaab )
	ROM_LOAD( "15c_sr18.bin", 0x48000, 0x8000, 0xce67868e )
	ROM_LOAD( "15e_sr20.bin", 0x50000, 0x8000, 0x3924c861 ) /* sprites */
	ROM_LOAD( "14e_sr19.bin", 0x58000, 0x8000, 0xff8f9129 )
	ROM_LOAD( "15f_sr22.bin", 0x60000, 0x8000, 0xab64161c )
	ROM_LOAD( "14f_sr21.bin", 0x68000, 0x8000, 0xfd64bcd1 )
	ROM_LOAD( "15h_sr24.bin", 0x70000, 0x8000, 0xc972af3e )
	ROM_LOAD( "14h_sr23.bin", 0x78000, 0x8000, 0x8c9abf57 )
	ROM_LOAD( "15j_sr26.bin", 0x80000, 0x8000, 0xd4f1732f )
	ROM_LOAD( "14j_sr25.bin", 0x88000, 0x8000, 0xd2a4ea4f )

	ROM_REGION(0x10000) /* 64k for the audio CPU */
	ROM_LOAD( "rc05.2f",      0x0000, 0x8000, 0xea04fa07 )  /* AUDIO (different) */

	ROM_REGION(0x40000) /* Paged ROMs */
	ROM_LOAD( "14e_sr04.bin", 0x00000, 0x08000, 0xa68ce89c )  /* RC4 */
	ROM_LOAD( "rc03.13e",     0x08000, 0x08000, 0xe82f78d4 )  /* RC3 (different) */
	ROM_LOAD( "rc02.12e",     0x10000, 0x08000, 0x009a62d8 )  /* RC2 (different) */
	ROM_LOAD( "rc01.11e",     0x18000, 0x08000, 0x2ac48d1d )  /* RC1 (different) */
	ROM_LOAD( "rc09.14f",     0x20000, 0x08000, 0x64f23e72 )  /* RC9 (different) */
	ROM_LOAD( "rc08.13f",     0x28000, 0x08000, 0x74c71007 )  /* RC8 (different) */
	ROM_LOAD( "12f_sr07.bin", 0x30000, 0x08000, 0xde785076 )  /* RC7 */
	ROM_LOAD( "11f_sr06.bin", 0x38000, 0x08000, 0xa70f4fd4 )  /* RC6 */

	ROM_REGION(0x0300) /* Proms (not used for now.. Transparency???) */
	ROM_LOAD( "63s141.12a",   0x00000, 0x00100, 0x8421786f )
	ROM_LOAD( "63s141.13a",   0x00100, 0x00100, 0x6048583f )
	ROM_LOAD( "63s141.8j",    0x00200, 0x00100, 0x1a89a7ff )
ROM_END

struct GameDriver srumbler_driver =
{
	__FILE__,
	0,
	"srumbler",
	"Speed Rumbler (set 1)",
	"1986",
	"Capcom",
	"Paul Leaman",
	0,
	&machine_driver,
	0,

	srumbler_rom,
	0,
	0,0,
	0,      /* sound_prom */

	input_ports,

	NULL, 0, 0,

	ORIENTATION_ROTATE_270,
	NULL, NULL
};

struct GameDriver srumblr2_driver =
{
	__FILE__,
	&srumbler_driver,
	"srumblr2",
	"Speed Rumbler (set 2)",
	"1986",
	"Capcom",
	"Paul Leaman",
	0,
	&machine_driver,
	0,

	srumblr2_rom,
	0,
	0,0,
	0,      /* sound_prom */

	input_ports,

	NULL, 0, 0,

	ORIENTATION_ROTATE_270,
	NULL, NULL
};
