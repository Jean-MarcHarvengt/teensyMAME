/* Great Swordsman (Taito) 1984

Credits:
- Steve Ellenoff: Original emulation and Mame driver
- Jarek Parchanski: Dip Switch Fixes, Color improvements, ADPCM Interface code

Special thanks to:
- Camilty for precious hardware information and screenshots

Issues:
- Colors are a mess
- communication to/from sound CPU(s) not yet hooked up
- CPU speed may not be accurate



There are 3 Z80s and two AY-3-8910s..

Prelim memory map (last updated 6/15/98)
*****************************************
GS1		z80 Main Code	(8K)	0000-1FFF
Gs2     z80 Game Data   (8K)    2000-3FFF
Gs3     z80 Game Data   (8K)    4000-5FFF
Gs4     z80 Game Data   (8K)    6000-7FFF
Gs5     z80 Game Data   (4K)    8000-8FFF
Gs6     Sprites         (8K)
Gs7     Sprites         (8K)
Gs8		Sprites			(8K)
Gs10	Tiles			(8K)
Gs11	Tiles			(8K)
Gs12    3rd z80 CPU &   (8K)
        ADPCM Samples?
Gs13    ADPCM Samples?  (8K)
Gs14    ADPCM Samples?  (8K)
Gs15    2nd z80 CPU     (8K)    0000-1FFF
Gs16    2nd z80 Data    (8K)    2000-3FFF
*****************************************

**********
*Main Z80*
**********

		9000 - 9fff	Work Ram
        982e - 982e Free play
        98e0 - 98e0 Coin Input
        98e1 - 98e1 Player 1 Controls
        98e2 - 98e2 Player 2 Controls
        9c00 - 9c30 (Hi score - Scores)
        9c78 - 9cd8 (Hi score - Names)
        9e00 - 9e7f Sprites in working ram!
        9e80 - 9eff Sprite X & Y in working ram!

		a000 - afff	Sprite RAM & Video Attributes
        a000 - a37F	???
        a380 - a77F	Sprite Tile #s
        a780 - a7FF	Sprite Y & X positions
        a980 - a980	Background Tile Bank Select
        ab00 - ab00	Background Tile Y-Scroll register
        ab80 - abff	Sprite Attributes(X & Y Flip)

		b000 - b7ff	Screen RAM
		b800 - ffff	not used?!

PORTS:
7e Read and Written - Communication to 2nd z80(or 8741?)
7f Read and Written - Communication to 2nd z80(or 8741?)
*****************************************
Unknown but written to....
9820,9821 = ?
980a = # of Credits?
980b = COIN SLOT 1?
980c = COIN SLOT 2?
980d = ?
981d = ?
980e, 980f ?
982b, 982c, 982d, 982a, 9824, 9825, 9826
98e3, 9e84 = ?

*************
*2nd Z80 CPU*
*************
0000 - 3FFF ROM CODE
4000 - 43FF WORK RAM

PORTS:
40 Read and Written - ?
41 Read and Written - ?
60 Read and Written - ?
61 Read and Written - ?
80 Written - ?
81 Written - ?
a0 Written - ?
e0 Read and Written - ?
******************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

void gsword_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int  gsword_vh_start(void);
void gsword_vh_stop(void);
void gsword_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void gs_video_attributes_w(int offset, int data);
void gs_flipscreen_w(int offset, int data);
void gs_videoram_w(int offset, int data);


extern int gs_videoram_size;
extern int gs_spritexy_size;


extern unsigned char *gs_videoram;
extern unsigned char *gs_scrolly_ram;
extern unsigned char *gs_spritexy_ram;
extern unsigned char *gs_spritetile_ram;
extern unsigned char *gs_spriteattrib_ram;

/*Cpu 1 Ports*/
static int port7e_c1=0;
static int port7f_c1=0,port_select=0,status=0,data_nr=0;

/*Cpu 2 Ports*/
static int port40_c2=0;
static int port41_c2=0;
static int port60_c2=0;
static int port61_c2=0;
static int port80_c2=0;
static int port81_c2=0;
static int porte0_c2=0;
static int porta0_c2=0;
static int port61_c2_buf[16];
static int port81_c2_buf[16];

//void gsword_adpcm_data_w(int offset, int data)
//{
//	if (data==0x30)
//	{
//	  MSM5205_reset_w(1,0);
//	  return;
//	}
//	else if (data==0x20)
//	{
//	  MSM5205_reset_w(0,0);
//	  return;
//	}
//	if (data & 0x10)
//     	 MSM5205_data_w (1, data & 0xf);
//	else
//	 MSM5205_data_w (0, data & 0xf);
//}

//void adpcm_soundcommand_w(int offset,int data)
//{
//	soundlatch2_w(0,data);
//	cpu_cause_interrupt(2, Z80_NMI_INT);
//}

//void sound_command_w(int offset,int data)
//{
//	soundlatch_w(0,data);
//	cpu_cause_interrupt(1, Z80_NMI_INT);
//}

int gsword_port_read(int offset)
{
 	switch(offset)
	{
		case 0:
			return(input_port_4_r(0));
		case 1:
			return(input_port_3_r(0));
		case 2:
			return(input_port_1_r(0));
		case 3:
			return(input_port_0_r(0));
		case 5:
			port_select=5;
 		        data_nr=0;
			return(0);	// ???
		case 8:
			port_select=8;
			return(0);	// ???
		case 10:
			port_select=10;
			return(0);	// ???
		default:
			return 0;
	}
}

int gsword_port_c1r(int offset)
{
	int result=0;

	if (errorlog) fprintf(errorlog,"%x  READ form port: %x\n",cpu_getpc(),offset);

	switch(offset)
        {
        	case 0x7e:
			if (status & 1)
			{
				result = gsword_port_read(port7f_c1);
			}
			else
			{
				result = status;
			}
			status &= 0xfe;
	                break;
        	case 0x7f:
			if (status & 2)
	                      result = port7e_c1;
			else
			      result = 1;
			status &= 0xfd;
                break;
        }
	return result;
}

void gsword_port_c1w(int offset, int data)
{

	if (errorlog) fprintf(errorlog,"%x  write to port: %x -> %x      %d     S:%d\n",cpu_getpc(),offset,data,port_select,data_nr);

	switch(offset)
        {
        	case 0x7e:
                	if (!(status & 2))
			{
           	     port7e_c1 = data;
			     if (port_select==5)
			     {

//					if(data > 0xdf)
//						adpcm_soundcommand_w(0,data-0xdf);

//					if(data < 0xdf)
//						sound_command_w(0,data);

					switch(data_nr)
			        {
			        	case 0x0:
				     		if (data & 0x80)// SOUND COMMANDS
						{
							//soundlatch_w(0,data);
						}
						break;
			        	case 0x1:
						break;
			        	case 0x2:
						break;
			        	case 0x3:
						break;
			        	case 0x4:
						break;
				 }
				 data_nr++;
			     }
			}
			status |= 2;  // busy for write
	                break;
        	case 0x7f:
                	if (!(status & 1)) port7f_c1 = data;
			status |= 1;  // busy for write
	                break;
        }
}



int gsword_port_c2r(int offset)
{
int result=0;

if (errorlog)
      fprintf(errorlog,"%x  read from cpu2 port: %x\n",cpu_getpc(),offset);

	switch(offset)
        {
        case 0x40:
                result = port40_c2;
                break;
        case 0x41:
                result = port41_c2;
				port41_c2 = 1;
                break;
        case 0x60:
                result = port61_c2_buf[port60_c2 & 0xF];
                break;
        case 0x61:
                result = port61_c2;
                break;
        case 0x80:
//				result = AY8910_read_port_0_r(offset);
                result = port81_c2_buf[port80_c2 & 0xF];
                break;
        case 0x81:
//				result = AY8910_read_port_1_r(offset);
                result = port81_c2;
                break;
        case 0xa0:
                result = porta0_c2;
                break;
        case 0xe0:
                result = porte0_c2;
				porte0_c2 = 1;
                break;
        }

	return result;
}

void gsword_port_c2w(int offset, int data)
{

	if (errorlog)
	   fprintf(errorlog,"%x  Write to cpu2 port: %x -> %x\n",cpu_getpc(),offset,data);

	switch(offset)
        {
        case 0x40:
                port40_c2 = data;
                break;
        case 0x41:
                port41_c2 = data;
                break;
        case 0x60:

//				AY8910_control_port_0_w(offset,data);
                port60_c2 = data;
                break;
        case 0x61:

//				AY8910_write_port_0_w(offset,data);

				port61_c2 = data;
				port61_c2_buf[port60_c2 & 0xF] = data;
                break;
        case 0x80:

//				AY8910_control_port_1_w(offset,data);

                port80_c2 = data;
                break;
        case 0x81:

//				AY8910_write_port_1_w(offset,data);

                port81_c2 = data;
				port81_c2_buf[port80_c2 & 0xF] = data;
                break;
        case 0xa0:
                porta0_c2 = data;
                break;
        case 0xe0:
                porte0_c2 = data;
                break;
        }
}

static struct MemoryReadAddress gsword_readmem[] =
{
	{ 0x0000, 0x8fff, MRA_ROM },
	{ 0x9000, 0x9fff, MRA_RAM },
	{ 0xa000, 0xa37f, MRA_RAM },
	{ 0xa380, 0xa3ff, MRA_RAM, &gs_spritetile_ram },
	{ 0xa400, 0xa77f, MRA_RAM },
	{ 0xa780, 0xa7ff, MRA_RAM, &gs_spritexy_ram, &gs_spritexy_size},
	{ 0xa800, 0xaaff, MRA_RAM },
	{ 0xab00, 0xab00, MRA_RAM, &gs_scrolly_ram },
	{ 0xab01, 0xab7f, MRA_RAM },
	{ 0xab80, 0xabff, MRA_RAM, &gs_spriteattrib_ram },
	{ 0xac00, 0xafff, MRA_RAM },
	{ 0xb000, 0xb7ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress gsword_writemem[] =
{
	{ 0x0000, 0x8fff, MWA_ROM },
	{ 0x9000, 0x9834, MWA_RAM },
	{ 0x9835, 0x9835, gs_flipscreen_w },
	{ 0x9836, 0xa97f, MWA_RAM },
	{ 0xa980, 0xa980, gs_video_attributes_w },
	{ 0xa981, 0xafff, MWA_RAM },
	{ 0xb000, 0xb7ff, gs_videoram_w, &gs_videoram, &gs_videoram_size },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_cpu2[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ -1 }
};

static struct MemoryWriteAddress writemem_cpu2[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ -1 }
};

static struct MemoryReadAddress readmem_cpu3[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x9fff, MRA_NOP },
//	{ 0xa000, 0xa000, soundlatch2_r },
	{ 0xa001, 0xffff, MRA_NOP },
	{ -1 }
};

static struct MemoryWriteAddress writemem_cpu3[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x6000, 0x7fff, MWA_NOP },
//	{ 0x8000, 0x8000, gsword_adpcm_data_w },
	{ 0x8001, 0xffff, MWA_NOP },
	{ -1 }
};

static struct IOReadPort readport[] =
{
    { 0x00, 0xff, gsword_port_c1r },
	{ -1 }
};

static struct IOWritePort writeport[] =
{
    { 0x00, 0xff, gsword_port_c1w },
	{ -1 }
};


static struct IOReadPort readport_cpu2[] =
{
	{ 0x00, 0xff, gsword_port_c2r },
	{ -1 }
};


static struct IOWritePort writeport_cpu2[] =
{
    { 0x00, 0xFF, gsword_port_c2w },
	{ -1 }
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN)

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE, "Coin A", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1)

	PORT_START	/* DSW0 */
	/* NOTE: Switches 0 & 1, 6,7,8 not used 	 */
	/*	 Coins configurations were handled 	 */
	/*	 via external hardware & not via program */
	PORT_DIPNAME( 0x1c, 0x00, "Coin#1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x04, "1 Coin/2 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/4 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/5 Credit" )
	PORT_DIPSETTING(    0x14, "2 Coin/1 Credit" )
	PORT_DIPSETTING(    0x18, "4 Coin/1 Credit" )
	PORT_DIPSETTING(    0x1c, "5 Coin/1 Credit" )

	PORT_START      /* DSW1 */
	/* NOTE: Switches 0 & 1 not used */
	PORT_DIPNAME( 0x0c, 0x04, "Stage 1 Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x04, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x0c, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Stage 2 Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPNAME( 0x20, 0x20, "Stage 3 Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPNAME( 0x40, 0x40, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Free Game Round", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* DSW2 */
	/* NOTE: Switches 0 & 1 not used */
	PORT_DIPNAME( 0x04, 0x00, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x04, "Yes" )
	PORT_DIPNAME( 0x88, 0x80, "Cabinet Type", IP_KEY_NONE )
	PORT_DIPSETTING(    0x88, "Cocktail" )
	PORT_DIPSETTING(    0x80, "Upright" )
	PORT_DIPNAME( 0x30, 0x00, "Stage Begins", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Fencing" )
	PORT_DIPSETTING(    0x10, "Kendo" )
	PORT_DIPSETTING(    0x20, "Rome" )
	PORT_DIPSETTING(    0x30, "Kendo" )
	PORT_DIPNAME( 0x40, 0x00, "Video Flip", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x40, "Yes" )
INPUT_PORTS_END



static struct GfxLayout gsword_text =
{
	8,8,    /* 8x8 characters */
	1024,	/* 1024 characters */
	2,      /* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8    /* every char takes 16 bytes */
};

static struct GfxLayout gsword_sprites1 =
{
	16,16,   /* 16x16 sprites */
	64*2,    /* 128 sprites */
	2,       /* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8     /* every sprite takes 64 bytes */
};

static struct GfxLayout gsword_sprites2 =
{
	32,32,    /* 32x32 sprites */
	64,       /* 64 sprites */
	2,       /* 2 bits per pixel */
	{ 0, 4 }, /* the two bitplanes for 4 pixels are packed into one byte */
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3,
			64*8+0, 64*8+1, 64*8+2, 64*8+3, 72*8+0, 72*8+1, 72*8+2, 72*8+3,
			80*8+0, 80*8+1, 80*8+2, 80*8+3, 88*8+0, 88*8+1, 88*8+2, 88*8+3},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8,
			128*8, 129*8, 130*8, 131*8, 132*8, 133*8, 134*8, 135*8,
			160*8, 161*8, 162*8, 163*8, 164*8, 165*8, 166*8, 167*8 },
	64*8*4    /* every sprite takes (64*8=16x6)*4) bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* character set */
	{ 1, 0x00000, &gsword_text,         0, 64 },
	{ 1, 0x04000, &gsword_sprites1,  64*4, 64 },
	{ 1, 0x06000, &gsword_sprites2,  64*4, 64 },
	{ -1 } /* end of array */
};



static struct AY8910interface ay8910_interface =
{
	2,			/* 2 chips */
	1500000,	/* 1.5 MHZ ?????? */
	{ 255, 255 },
	{ 0,0 },
	{ 0,0 },
	{ 0,0 },
	{ 0,0 }
};

static struct MSM5205interface msm5205_interface =
{
	2,		/* 2 chips */
	4000,	/* 4000Hz playback ? */
	0,		/* interrupt function */
	{ 255, 255 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,        /* 3.072 Mhz ? */
			0,
			gsword_readmem,gsword_writemem,
			readport,writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3140000,        /* 3.140 Mhz ? */
			3,
			readmem_cpu2,writemem_cpu2,
			readport_cpu2,writeport_cpu2,
			ignore_interrupt,0
//			interrupt,1

		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3140000,        /* 3.140 Mhz ? */
			4,
			readmem_cpu3,writemem_cpu3,
			0,0,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
    10,                                 /* Allow time for 2nd cpu to interleave*/
	0,

	/* video hardware */
    32*8, 32*8,{ 0*8, 32*8-1, 2*8, 30*8-1 },

	gfxdecodeinfo,
	256, 64*4+64*4,
	gsword_vh_convert_color_prom,
	VIDEO_TYPE_RASTER,
	0,
	gsword_vh_start,
	gsword_vh_stop,
	gsword_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_MSM5205,
			&msm5205_interface
		}
	}

};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gsword_rom )
	ROM_REGION(0x10000)	/* 64K for main CPU */
	ROM_LOAD( "gs1",          0x0000, 0x2000, 0x565c4d9e )
	ROM_LOAD( "gs2",          0x2000, 0x2000, 0xd772accf )
	ROM_LOAD( "gs3",          0x4000, 0x2000, 0x2cee1871 )
	ROM_LOAD( "gs4",          0x6000, 0x2000, 0xca9d206d )
	ROM_LOAD( "gs5",          0x8000, 0x1000, 0x2a892326 )

	ROM_REGION_DISPOSE(0xa000)	/* graphics (disposed after conversion) */
	ROM_LOAD( "gs10",         0x0000, 0x2000, 0x517c571b )	/* tiles */
	ROM_LOAD( "gs11",         0x2000, 0x2000, 0x7a1d8a3a )
	ROM_LOAD( "gs6",          0x4000, 0x2000, 0x1b0a3cb7 )	/* sprites */
	ROM_LOAD( "gs7",          0x6000, 0x2000, 0xef5f28c6 )
	ROM_LOAD( "gs8",          0x8000, 0x2000, 0x46824b30 )

	ROM_REGION(0x0360)	/* color PROMs */
	ROM_LOAD( "ac0-1.bpr",    0x0000, 0x0100, 0x5c4b2adc )	/* palette high bits */
	ROM_LOAD( "ac0-2.bpr",    0x0100, 0x0100, 0x966bda66 )	/* palette low bits */
	ROM_LOAD( "ac0-3.bpr",    0x0200, 0x0100, 0xdae13f77 )	/* sprite lookup table? */
	ROM_LOAD( "003",          0x0300, 0x0020, 0x43a548b8 )	/* address decoder? not used */
	ROM_LOAD( "004",          0x0320, 0x0020, 0x43a548b8 )	/* address decoder? not used */
	ROM_LOAD( "005",          0x0340, 0x0020, 0xe8d6dec0 )	/* address decoder? not used */

	ROM_REGION(0x10000)	/* 64K for 2nd CPU */
	ROM_LOAD( "gs15",         0x0000, 0x2000, 0x1aa4690e )
	ROM_LOAD( "gs16",         0x2000, 0x2000, 0x10accc10 )

	ROM_REGION(0x10000)	/* 64K for 3nd z80 */
	ROM_LOAD( "gs12",         0x0000, 0x2000, 0xa6589068 )
	ROM_LOAD( "gs13",         0x2000, 0x2000, 0x4ee79796 )
	ROM_LOAD( "gs14",         0x4000, 0x2000, 0x455364b6 )
ROM_END



static int gsword_hiload(void)
{
	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */
	unsigned char *RAM = Machine->memory_region[0];

        /* Work RAM - 9c00 (3*10 for scores), 9c78(6*10 for names)*/
        /* check if the hi score table has already been initialized */

        if( memcmp(&RAM[0x9c00],"\x00\x00\x01",3) == 0)
	{
                void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
		if (f)
		{
                        osd_fread(f,&RAM[0x9c00],3*10);
                        osd_fread(f,&RAM[0x9c78],6*10);
			osd_fclose(f);
                }
		return 1;
	}
	return 0;  /* we can't load the hi scores yet */
}

static void gsword_hisave(void)
{
    	/* get RAM pointer (this game is multiCPU, we can't assume the global */
	/* RAM pointer is pointing to the right place) */

	unsigned char *RAM = Machine->memory_region[0];
	void *f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1);

	if (f)
	{
                osd_fwrite(f,&RAM[0x9c00],3*10);
                osd_fwrite(f,&RAM[0x9c78],6*10);
		osd_fclose(f);
	}
}


struct GameDriver gsword_driver =
{
	__FILE__,
	0,
	"gsword",
	"Great Swordsman",
	"1984",
	"Taito",
    "Steve Ellenoff\nJarek Parchanski\n",
	GAME_WRONG_COLORS,
	&machine_driver,
	0,

    gsword_rom,
	0, 0,
    0,
	0,	/* sound_prom */

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
    ORIENTATION_DEFAULT,

    gsword_hiload, gsword_hisave
};

