/***************************************************************************
Xain'd Sleena (TECHNOS), Solar Warrior (TAITO).
By Carlos A. Lozano & Rob Rosenbrock & Phil Stroffolino

	- MC68B09EP (2)
        - 6809EP (1)
        - 68705 (only in Solar Warrior)
        - ym2203 (2)

Remaining Issues:

       - Fix the random loops. (yet???)

       - Get better music.

       - Don't understood the timers.

       - Implement the sprite-plane2 priorities.

       - 68705 in Solar Warrior. (partial missing sprites)

       - Optimize. (For example, pallete updates)

       - CPU speed may be wrong

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "m6809/m6809.h"

unsigned char *xain_sharedram;
static int xain_timer = 0xff;

unsigned char waitIRQA;
unsigned char waitIRQB;
unsigned char waitIRQsound;

void xain_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int xain_vh_start(void);
void xain_vh_stop(void);
void xain_scrollxP2_w(int offset,int data);
void xain_scrollyP2_w(int offset,int data);
void xain_scrollxP3_w(int offset,int data);
void xain_scrollyP3_w(int offset,int data);
void xain_videoram2_w(int offset,int data);

extern unsigned char *xain_videoram;
extern int xain_videoram_size;
extern unsigned char *xain_videoram2;
extern int xain_videoram2_size;


void xain_init_machine(void)
{
   //unsigned char *RAM = Machine->memory_region[2];
   //RAM[0x4b90] = 0x00;
}

int xain_sharedram_r(int offset)
{
	return xain_sharedram[offset];
}

void xain_sharedram_w(int offset, int data)
{
	xain_sharedram[offset] = data;
}

void xainCPUA_bankswitch_w(int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (data&0x08) {cpu_setbank(1,&RAM[0x10000]);}
	else {cpu_setbank(1,&RAM[0x4000]);}
}

void xainCPUB_bankswitch_w(int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];


	if (data&0x1) {cpu_setbank(2,&RAM[0x10000]);}
	else {cpu_setbank(2,&RAM[0x4000]);}
}

int xain_timer_r(int offset)
{
      return (xain_timer);
}

int solarwar_slapstic_r(int offset)
{
      return (0x4d);
}

void xainB_forcedIRQ_w(int offset,int data)
{
    waitIRQB = 1;
    cpu_spin();  // Forced change of CPU
}

void xainA_forcedIRQ_w(int offset,int data)
{
    waitIRQA = 1;
    cpu_spin(); // Forced change of CPU
}

void xainA_writesoundcommand_w(int offset, int data)
{
    waitIRQsound = 1;
    soundlatch_w(offset,data);
}

static struct MemoryReadAddress readmem[] =
{
        { 0x0000, 0x1fff, MRA_RAM, &xain_sharedram},
	{ 0x2000, 0x39ff, MRA_RAM },
        { 0x3a00, 0x3a00, input_port_0_r },
        { 0x3a01, 0x3a01, input_port_1_r },
        { 0x3a02, 0x3a02, input_port_2_r },
        { 0x3a03, 0x3a03, input_port_3_r },
        { 0x3a04, 0x3a04, solarwar_slapstic_r},
        { 0x3a05, 0x3a05, xain_timer_r},       /* how?? */
        { 0x3a06, 0x3fff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x1fff, MWA_RAM, &xain_sharedram},
	{ 0x2000, 0x27ff, MWA_RAM, &xain_videoram, &xain_videoram_size },
	{ 0x2800, 0x2fff, xain_videoram2_w, &xain_videoram2, &xain_videoram2_size },
	{ 0x3000, 0x37ff, videoram_w, &videoram, &videoram_size },
	{ 0x3800, 0x397f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3980, 0x39ff, MWA_RAM },
	{ 0x3a00, 0x3a01, xain_scrollxP2_w},
	{ 0x3a02, 0x3a03, xain_scrollyP2_w},
	{ 0x3a04, 0x3a05, xain_scrollxP3_w},
	{ 0x3a06, 0x3a07, xain_scrollyP3_w},
	{ 0x3a08, 0x3a08, xainA_writesoundcommand_w},
	{ 0x3a09, 0x3a0b, MWA_RAM },
	{ 0x3a0c, 0x3a0c, xainB_forcedIRQ_w},
	{ 0x3a0d, 0x3a0e, MWA_RAM },
	{ 0x3a0f, 0x3a0f, xainCPUA_bankswitch_w},
	{ 0x3a10, 0x3bff, MWA_RAM },
	{ 0x3c00, 0x3dff, paletteram_xxxxBBBBGGGGRRRR_split1_w, &paletteram },
	{ 0x3e00, 0x3fff, paletteram_xxxxBBBBGGGGRRRR_split2_w, &paletteram_2 },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmemB[] =
{
	{ 0x0000, 0x1fff, xain_sharedram_r },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writememB[] =
{
	{ 0x0000, 0x1fff, xain_sharedram_w },
        { 0x2000, 0x27ff, xainA_forcedIRQ_w},
	{ 0x2800, 0x2fff, MWA_RAM },
        { 0x3000, 0x37ff, xainCPUB_bankswitch_w},
        { 0x3800, 0x3fff, MWA_RAM },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_sound[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x1000, soundlatch_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_sound[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
        { 0x2800, 0x2800, YM2203_control_port_0_w },
	{ 0x2801, 0x2801, YM2203_write_port_0_w },
	{ 0x3000, 0x3000, YM2203_control_port_1_w },
	{ 0x3001, 0x3001, YM2203_write_port_1_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};

static int xainA_interrupt(void)
{
     xain_timer ^= 0x38;
     if (waitIRQA)
     { waitIRQA = 0;
       return (M6809_INT_IRQ);}
     return (M6809_INT_FIRQ | M6809_INT_NMI);
}

static int xainB_interrupt(void)
{
     if (waitIRQB)
     { waitIRQB = 0;
       return (M6809_INT_IRQ);}
     return ignore_interrupt();
}

static int xain_sound_interrupt(void)
{
     if (waitIRQsound)
     { waitIRQsound = 0;
       return (M6809_INT_IRQ);}
     return (M6809_INT_FIRQ);
}

INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x10, 0x10, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x20, "Yes" )
	PORT_DIPNAME( 0x40, 0x40, "Screen Type", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPNAME( 0x80, 0x80, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Game Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Slow" )
	PORT_DIPSETTING(    0x08, "Normal" )
	PORT_DIPSETTING(    0x04, "Fast" )
	PORT_DIPSETTING(    0x00, "Very Fast" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "20000 70000 70000" )
	PORT_DIPSETTING(    0x20, "30000 80000 80000" )
	PORT_DIPSETTING(    0x10, "20000 80000" )
	PORT_DIPSETTING(    0x00, "30000 80000" )
	PORT_DIPNAME( 0xC0, 0xC0, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0xC0, "2")
	PORT_DIPSETTING(    0x80, "3")
	PORT_DIPSETTING(    0x40, "5")
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,	// 8*8 chars
	1024,	// 1024 characters
	4,	// 4 bits per pixel
	{ 0, 2, 4, 6 },	// plane offset
	{ 1, 0, 65, 64, 129, 128, 193, 192 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8	// every char takes 32 consecutive bytes
};

static struct GfxLayout tileslayout =
{
	16,16,	/* 8*8 chars */
	512,	/* 512 characters */
	4,	/* 4 bits per pixel */
        { 0x8000*8+0, 0x8000*8+4, 0, 4 },	/* plane offset */
	{ 3, 2, 1, 0, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
          32*8+3,32*8+2 ,32*8+1 ,32*8+0 ,48*8+3 ,48*8+2 ,48*8+1 ,48*8+0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
          8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	64*8	/* every char takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* 8x8 text */
	{ 1, 0x00000, &charlayout,    0, 8 },

	/* 16x16 Background */
	{ 1, 0x08000, &tileslayout, 256, 8 },
	{ 1, 0x18000, &tileslayout, 256, 8 },
	{ 1, 0x28000, &tileslayout, 256, 8 },
	{ 1, 0x38000, &tileslayout, 256, 8 },

	/* 16x16 Background */
	{ 1, 0x48000, &tileslayout, 384, 8 },
	{ 1, 0x58000, &tileslayout, 384, 8 },
	{ 1, 0x68000, &tileslayout, 384, 8 },
	{ 1, 0x78000, &tileslayout, 384, 8 },

	/* Sprites */
	{ 1, 0x88000, &tileslayout, 128, 8 },
	{ 1, 0x98000, &tileslayout, 128, 8 },
	{ 1, 0xa8000, &tileslayout, 128, 8 },
	{ 1, 0xb8000, &tileslayout, 128, 8 },
	{ -1 } // end of array
};

static struct YM2203interface ym2203_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz (?) */      /* Real unknown */
	{ YM2203_VOL(30,30), YM2203_VOL(30,30) },
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
			2000000,	/* 2 Mhz (?) */
			0,
			readmem,writemem,0,0,
			xainA_interrupt,4      /* Number Unknown */
		},
		{
 			CPU_M6809,
			2000000,	/* 2 Mhz (?) */
			2,
			readmemB,writememB,0,0,
			xainB_interrupt,4      /* Number Unknown */
		},
		{
 			CPU_M6809 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz (?) */
			3,
			readmem_sound,writemem_sound,0,0,
			xain_sound_interrupt,16 /* Number Unknown */
		},

	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	4*60, 	/* 240 CPU slices per frame */
	xain_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 30*8-1 },
	gfxdecodeinfo,
	512, 512,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	xain_vh_start,
	xain_vh_stop,
	xain_vh_screenrefresh,

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
ROM_START( xain_rom )
	ROM_REGION(0x14000)	/* 64k for code */
	ROM_LOAD( "1.rom",        0x08000, 0x8000, 0x79f515a7 )
	ROM_LOAD( "2.rom",        0x04000, 0x4000, 0xd22bf859 )
	ROM_CONTINUE(      0x10000, 0x4000 )

	ROM_REGION_DISPOSE(0xc8000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "12.rom",       0x00000, 0x8000, 0x83c00dd8 ) /* Characters */
	ROM_LOAD( "21.rom",       0x08000, 0x8000, 0x11eb4247 ) /* Characters */
	ROM_LOAD( "13.rom",       0x10000, 0x8000, 0x8f0aa1a7 ) /* Characters */
	ROM_LOAD( "22.rom",       0x18000, 0x8000, 0x422b536e ) /* Characters */
	ROM_LOAD( "14.rom",       0x20000, 0x8000, 0x45681910 ) /* Characters */
	ROM_LOAD( "23.rom",       0x28000, 0x8000, 0x828c1b0c ) /* Characters */
	ROM_LOAD( "15.rom",       0x30000, 0x8000, 0xa8eeabc8 ) /* Characters */
	ROM_LOAD( "24.rom",       0x38000, 0x8000, 0xf10f7dd9 ) /* Characters */
	ROM_LOAD( "16.rom",       0x40000, 0x8000, 0xe59a2f27 ) /* Characters */

	ROM_LOAD( "6.rom",        0x48000, 0x8000, 0x5c6c453c ) /* Characters */
	ROM_LOAD( "7.rom",        0x50000, 0x8000, 0x8d637639 ) /* Characters */
	ROM_LOAD( "5.rom",        0x58000, 0x8000, 0x59d87a9a ) /* Characters */
	ROM_LOAD( "8.rom",        0x60000, 0x8000, 0x71eec4e6 ) /* Characters */
	ROM_LOAD( "4.rom",        0x68000, 0x8000, 0x84884a2e ) /* Characters */
	ROM_LOAD( "9.rom",        0x70000, 0x8000, 0x7fc9704f ) /* Characters */

	ROM_LOAD( "25.rom",       0x88000, 0x8000, 0x252976ae ) /* Sprites */
	ROM_LOAD( "17.rom",       0x90000, 0x8000, 0x4d977f33 ) /* Sprites */
	ROM_LOAD( "26.rom",       0x98000, 0x8000, 0xe6f1e8d5 ) /* Sprites */
	ROM_LOAD( "18.rom",       0xa0000, 0x8000, 0x3f3b62a0 ) /* Sprites */
	ROM_LOAD( "27.rom",       0xa8000, 0x8000, 0x785381ed ) /* Sprites */
	ROM_LOAD( "19.rom",       0xb0000, 0x8000, 0x76641ee3 ) /* Sprites */
	ROM_LOAD( "28.rom",       0xb8000, 0x8000, 0x59754e3d ) /* Sprites */
	ROM_LOAD( "20.rom",       0xc0000, 0x8000, 0x37671f36 ) /* Sprites */

	ROM_REGION(0x14000)	/* 64k for code */
	ROM_LOAD( "10.rom",       0x08000, 0x8000, 0xa1a860e2 )
	ROM_LOAD( "11.rom",       0x04000, 0x4000, 0x948b9757 )
	ROM_CONTINUE(       0x10000, 0x4000 )

	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "3.rom",        0x8000, 0x8000, 0xa5318cb8 )
ROM_END

ROM_START( solarwar_rom )
	ROM_REGION(0x14000)	/* 64k for code */
	ROM_LOAD( "p9-0.bin",     0x08000, 0x8000, 0x8ff372a8 )
	ROM_LOAD( "pa-0.bin",     0x04000, 0x4000, 0x154f946f )
	ROM_CONTINUE(         0x10000, 0x4000 )

	ROM_REGION_DISPOSE(0xc8000)     /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "12.rom",       0x00000, 0x8000, 0x83c00dd8 ) /* Characters */
	ROM_LOAD( "21.rom",       0x08000, 0x8000, 0x11eb4247 ) /* Characters */
	ROM_LOAD( "13.rom",       0x10000, 0x8000, 0x8f0aa1a7 ) /* Characters */
	ROM_LOAD( "22.rom",       0x18000, 0x8000, 0x422b536e ) /* Characters */
	ROM_LOAD( "14.rom",       0x20000, 0x8000, 0x45681910 ) /* Characters */
	ROM_LOAD( "23.rom",       0x28000, 0x8000, 0x828c1b0c ) /* Characters */
	ROM_LOAD( "15.rom",       0x30000, 0x8000, 0xa8eeabc8 ) /* Characters */
	ROM_LOAD( "pn-0.bin",     0x38000, 0x8000, 0xd2ed6f94 ) /* Characters */
	ROM_LOAD( "pf-0.bin",     0x40000, 0x8000, 0x6e627a77 ) /* Characters */

	ROM_LOAD( "6.rom",        0x48000, 0x8000, 0x5c6c453c ) /* Characters */
	ROM_LOAD( "7.rom",        0x50000, 0x8000, 0x8d637639 ) /* Characters */
	ROM_LOAD( "5.rom",        0x58000, 0x8000, 0x59d87a9a ) /* Characters */
	ROM_LOAD( "8.rom",        0x60000, 0x8000, 0x71eec4e6 ) /* Characters */
	ROM_LOAD( "4.rom",        0x68000, 0x8000, 0x84884a2e ) /* Characters */
	ROM_LOAD( "9.rom",        0x70000, 0x8000, 0x7fc9704f ) /* Characters */

	ROM_LOAD( "25.rom",       0x88000, 0x8000, 0x252976ae ) /* Sprites */
	ROM_LOAD( "17.rom",       0x90000, 0x8000, 0x4d977f33 ) /* Sprites */
	ROM_LOAD( "26.rom",       0x98000, 0x8000, 0xe6f1e8d5 ) /* Sprites */
	ROM_LOAD( "18.rom",       0xa0000, 0x8000, 0x3f3b62a0 ) /* Sprites */
	ROM_LOAD( "27.rom",       0xa8000, 0x8000, 0x785381ed ) /* Sprites */
	ROM_LOAD( "19.rom",       0xb0000, 0x8000, 0x76641ee3 ) /* Sprites */
	ROM_LOAD( "28.rom",       0xb8000, 0x8000, 0x59754e3d ) /* Sprites */
	ROM_LOAD( "20.rom",       0xc0000, 0x8000, 0x37671f36 ) /* Sprites */

	ROM_REGION(0x14000)	/* 64k for code */
	ROM_LOAD( "p1-0.bin",     0x08000, 0x8000, 0xf5f235a3 )
	ROM_LOAD( "p0-0.bin",     0x04000, 0x4000, 0x51ae95ae )
	ROM_CONTINUE(         0x10000, 0x4000 )

	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "3.rom",        0x8000, 0x8000, 0xa5318cb8 )
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x1bca],"\x27\x2c\x26",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x1bc7],6*10);
			RAM[0x0033] = RAM[0x1bc7];
			RAM[0x0034] = RAM[0x1bc8];
			RAM[0x0035] = RAM[0x1bc9];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x1bc7],6*10);
		osd_fclose(f);
	}
}



struct GameDriver xsleena_driver =
{
	__FILE__,
	0,
	"xsleena",
	"Xain'd Sleena",
	"1986",
	"Technos",
	"Carlos A. Lozano\nRob Rosenbrock\nPhil Stroffolino\n",
	0,
	&machine_driver,
	0,

	xain_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

struct GameDriver solarwar_driver =
{
	__FILE__,
	&xsleena_driver,
	"solarwar",
	"Solar Warrior",
	"1986",
	"Technos (Memetron license)",
	"Carlos A. Lozano\nRob Rosenbrock\nPhil Stroffolino\n",
	0,
	&machine_driver,
	0,

	solarwar_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
