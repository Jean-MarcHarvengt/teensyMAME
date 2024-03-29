/***************************************************************************
Atari Super Breakout Driver

Note:  I'm cheating a little bit with the paddle control.  The original
game handles the paddle control as following.  The paddle is a potentiometer.
Every VBlank signal triggers the start of a voltage ramp.  Whenever the
ramp has the same value as the potentiometer, an NMI is generated.	In the
NMI code, the current scanline value is used to calculate the value to
put into location $1F in memory.  I cheat in this driver by just putting
the paddle value directly into $1F, which has the same net result.

If you have any questions about how this driver works, don't hesitate to
ask.  - Mike Balfour (mab22@po.cwru.edu)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* vidhrdw/sbrkout.c */
extern void sbrkout_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern int sbrkout_vh_start(void);
extern void sbrkout_vh_stop(void);
extern unsigned char *sbrkout_horiz_ram;
extern unsigned char *sbrkout_vert_ram;

/* machine/sbrkout.c */
extern void sbrkout_serve_led(int offset,int value);
extern void sbrkout_start_1_led(int offset,int value);
extern void sbrkout_start_2_led(int offset,int value);
extern int sbrkout_read_DIPs(int offset);
extern int sbrkout_interrupt(void);
extern int sbrkout_select1(int offset);
extern int sbrkout_select2(int offset);


/* sound hardware - temporary */

static void sbrkout_dac_w(int offset,int data);
static void sbrkout_tones_4V(int foo);
static int init_timer=1;

#define TIME_4V 4.075/4

unsigned char *sbrkout_sound;

static void sbrkout_dac_w(int offset,int data)
{
	sbrkout_sound[offset]=data;

	if (init_timer)
	{
		timer_set (TIME_IN_MSEC(TIME_4V), 0, sbrkout_tones_4V);
		init_timer=0;
	}
}

static void sbrkout_tones_4V(int foo)
{
	static int vlines=0;

	if ((*sbrkout_sound) & vlines)
		DAC_data_w(0,255);
	else
		DAC_data_w(0,0);

	vlines = (vlines+1) % 16;

	timer_set (TIME_IN_MSEC(TIME_4V), 0, sbrkout_tones_4V);
}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0010, 0x0014, MRA_RAM, &sbrkout_horiz_ram }, /* Horizontal Ball Position */
	{ 0x0018, 0x001d, MRA_RAM, &sbrkout_vert_ram }, /* Vertical Ball Position / ball picture */
	{ 0x001f, 0x001f, input_port_6_r }, /* paddle value */
	{ 0x0000, 0x00ff, MRA_RAM }, /* Zero Page RAM */
	{ 0x0100, 0x01ff, MRA_RAM }, /* ??? */
	{ 0x0400, 0x077f, MRA_RAM }, /* Video Display RAM */
	{ 0x0828, 0x0828, sbrkout_select1 }, /* Select 1 */
	{ 0x082f, 0x082f, sbrkout_select2 }, /* Select 2 */
	{ 0x082e, 0x082e, input_port_5_r }, /* Serve Switch */
	{ 0x0830, 0x0833, sbrkout_read_DIPs }, /* DIP Switches */
	{ 0x0840, 0x0840, input_port_1_r }, /* Coin Switches */
	{ 0x0880, 0x0880, input_port_2_r }, /* Start Switches */
	{ 0x08c0, 0x08c0, input_port_3_r }, /* Self Test Switch */
	{ 0x0c00, 0x0c00, input_port_4_r }, /* Vertical Sync Counter */
	{ 0x2c00, 0x3fff, MRA_ROM }, /* PROGRAM */
	{ 0xfff0, 0xffff, MRA_ROM }, /* PROM8 for 6502 vectors */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0011, 0x0011, sbrkout_dac_w, &sbrkout_sound }, /* Noise Generation Bits */
	{ 0x0000, 0x00ff, MWA_RAM }, /* WRAM */
	{ 0x0100, 0x01ff, MWA_RAM }, /* ??? */
	{ 0x0400, 0x07ff, videoram_w, &videoram, &videoram_size }, /* DISPLAY */
//		  { 0x0c10, 0x0c11, sbrkout_serve_led }, /* Serve LED */
	{ 0x0c30, 0x0c31, sbrkout_start_1_led }, /* 1 Player Start Light */
	{ 0x0c40, 0x0c41, sbrkout_start_2_led }, /* 2 Player Start Light */
	{ 0x0c50, 0x0c51, MWA_RAM }, /* NMI Pot Reading Enable */
	{ 0x0c70, 0x0c71, MWA_RAM }, /* Coin Counter */
	{ 0x0c80, 0x0c80, MWA_NOP }, /* Watchdog */
	{ 0x0e00, 0x0e00, MWA_NOP }, /* IRQ Enable? */
	{ 0x1000, 0x1000, MWA_RAM }, /* LSB of Pot Reading */
	{ 0x2c00, 0x3fff, MWA_ROM }, /* PROM1-PROM8 */
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( sbrkout_input_ports )
	PORT_START		/* DSW - fake port, gets mapped to Super Breakout ports */
	PORT_DIPNAME( 0x03, 0x00, "Language", IP_KEY_NONE )
	PORT_DIPSETTING(	0x00, "English" )
	PORT_DIPSETTING(	0x01, "German" )
	PORT_DIPSETTING(	0x02, "French" )
	PORT_DIPSETTING(	0x03, "Spanish" )
	PORT_DIPNAME( 0x0C, 0x08, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(	0x0C, "2 Coins/1 Credit" )
	PORT_DIPSETTING(	0x08, "1 Coin/1 Credit" )
	PORT_DIPSETTING(	0x04, "1 Coin/2 Credits" )
	PORT_DIPSETTING(	0x00, "Free Play" )
	PORT_DIPNAME( 0x70, 0x00, "Extended Play", IP_KEY_NONE ) /* P=Progressive, C=Cavity, D=Double */
	PORT_DIPSETTING(	0x10, "200P/200C/200D" )
	PORT_DIPSETTING(	0x20, "400P/300C/400D" )
	PORT_DIPSETTING(	0x30, "600P/400C/600D" )
	PORT_DIPSETTING(	0x40, "900P/700C/800D" )
	PORT_DIPSETTING(	0x50, "1200P/900C/1000D" )
	PORT_DIPSETTING(	0x60, "1600P/1100C/1200D" )
	PORT_DIPSETTING(	0x70, "2000P/1400C/1500D" )
	PORT_DIPSETTING(	0x00, "None" )
	PORT_DIPNAME( 0x80, 0x80, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0x00, "5" )

	PORT_START		/* IN0 */
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START		/* IN1 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START		/* IN2 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BITX(	  0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(	0x80, "Off" )
	PORT_DIPSETTING(	0x00, "On" )

	PORT_START		/* IN3 */
	PORT_BIT ( 0xFF, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START		/* IN4 */
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 )

	PORT_START		/* IN5 */
	PORT_ANALOG ( 0xff, 0x00, IPT_PADDLE | IPF_REVERSE, 100, 7, 0, 255 )

	PORT_START		/* IN6 - fake port, used to set the game select dial */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN, "Progressive", OSD_KEY_E, IP_JOY_NONE, 0 )
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN, "Double", OSD_KEY_D, IP_JOY_NONE, 0 )
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN, "Cavity", OSD_KEY_C, IP_JOY_NONE, 0 )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	64, 	/* 64 characters */
	1,		/* 1 bit per pixel */
	{ 0 },		  /* no separation in 1 bpp */
	{ 4, 5, 6, 7, 0x200*8 + 4, 0x200*8 + 5, 0x200*8 + 6, 0x200*8 + 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8 /* every char takes 8 consecutive bytes */
};

/* I think the actual ball layout is 3x3, but we split it into 3 1x3 sprites
   so that we can handle the color overlay on a line by line basis. */
static struct GfxLayout balllayout =
{
	1,3,	/* 1*3 character? */
	6,	   /* 6 characters */
	1,		/* 1 bit per pixel */
	{ 0 },		  /* no separation in 1 bpp */
	{ 0 },
	{ 0, 1, 2 },
	1*8 /* every char takes 1 consecutive byte */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout, 0x00, 0x06 }, /* offset into colors, # of colors */
	{ 1, 0x0400, &balllayout, 0x00, 0x06 }, /* offset into colors, # of colors */
	{ -1 } /* end of array */
};

static unsigned char palette[] =
{
	0x00,0x00,0x00, /* BLACK  */
	0xff,0xff,0xff, /* WHITE  */
	0x00,0x00,0xff, /* BLUE */
	0xff,0x80,0x00, /* ORANGE */
	0x00,0xff,0x00, /* GREEN */
	0xff,0xff,0x00, /* YELLOW */
};

static unsigned short colortable[] =
{
	0x00, 0x00,  /* Don't draw */
	0x00, 0x01,  /* Draw */
	0x00, 0x02,  /* Draw */
	0x00, 0x03,  /* Draw */
	0x00, 0x04,  /* Draw */
	0x00, 0x05,  /* Draw */

};



static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			375000, 	   /* 375 KHz? Should be 750KHz? */
			0,
			readmem,writemem,0,0,
			sbrkout_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 28*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER,
	0,
	sbrkout_vh_start,
	sbrkout_vh_stop,
	sbrkout_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};





/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( sbrkout_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "033453.c1",    0x2800, 0x0800, 0xa35d00e3 )
	ROM_LOAD( "033454.d1",    0x3000, 0x0800, 0xd42ea79a )
	ROM_LOAD( "033455.e1",    0x3800, 0x0800, 0xe0a6871c )
	ROM_RELOAD( 			0xF800, 0x0800 )

	ROM_REGION_DISPOSE(0x420)	  /* 2k for graphics */
	ROM_LOAD( "033280.p4",    0x0000, 0x0200, 0x5a69ce85 )
	ROM_LOAD( "033281.r4",    0x0200, 0x0200, 0x066bd624 )
	ROM_LOAD( "033282.k6",    0x0400, 0x0020, 0x6228736b )
ROM_END



/***************************************************************************
  Hi Score Routines
***************************************************************************/

static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0012],"\x3C\xAA\x3C\xAA",4) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x0022],0x3);
			osd_fread(f,&RAM[0x0027],0x3);
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
		osd_fwrite(f,&RAM[0x0022],0x3);
		osd_fwrite(f,&RAM[0x0027],0x3);
		osd_fclose(f);
	}

}



struct GameDriver sbrkout_driver =
{
	__FILE__,
	0,
	"sbrkout",
	"Super Breakout",
	"1978",
	"Atari",
	"Mike Balfour",
	0,
	&machine_driver,
	0,

	sbrkout_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	sbrkout_input_ports,

	0, palette, colortable,
	ORIENTATION_ROTATE_270,
	hiload,hisave
};
