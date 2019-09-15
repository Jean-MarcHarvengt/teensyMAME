/***************************************************************************
  Lazer Command memory map (preliminary)

  0000-0bff ROM

  1c00-1c1f RAM 	used by the CPU (most probably latches only!?)
  1c03		D4		left  player marker
  1c04		D4		right player marker
  1c0a		D0-D7	right player y coord
  1c0b		D0-D7	left  player x coord
  1c0c		D0-D7	left  player y coord
  1c0d		D0-D7	right player x coord

  1c20-1eff RAM 	video buffer
			xxxx	D0 - D5 character select
					D6		horz line below character (row #9)
					D7		vert line right of character (bit #0)
  1f00-1f03 		hardware

			1f00 W	D4 speaker
			1f00 R	   player 1 joystick
					D0 up
					D1 down
					D2 right
					D3 left

			1f01 W	?? no idea ??
			1f01 R	   player 2 joystick
					D0 up
					D1 down
					D2 right
					D3 left

			1f02 W	?? no idea ??
			1f02 R	   player 1 + 2 buttons
					D0 button 1 player 2
					D1 button 1 player 1
					D2 button 2 player 2
					D3 button 2 player 1

            1f03 W  ?? no idea ??
			1f03 R	coinage and start button
					D0 coin 1/2 (selected by 1f02 ?)
					D1 start 'expert'
					D2 start 'novice'
					D3 vblank ?

  I/O ports:

  'data'    R   dip switch settings
                D0 - D1 Game time 60,90,120,180 seconds
                D2      unused
                D3      Coins per game 1, 2

***************************************************************************/

#include "driver.h"
#include "S2650/s2650.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/lazercmd.h"

int lazercmd_latches[32];

/*************************************************************

   externals

 *************************************************************/
int lazercmd_vh_start(void);
void lazercmd_vh_stop(void);
void lazercmd_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void lazercmd_marker_dirty(int marker);

/*************************************************************
 *
 * Statics
 *
 *************************************************************/
static int lazercmd_hw_1f02 = 0;
static int lazercmd_sense_state = 0;
static int lazercmd_vblank_state = 0;

/*************************************************************
 * Interrupt for the cpu
 *************************************************************/
static int lazercmd_vblank(void)
{
	lazercmd_vblank_state &= ~8;
	return S2650_INT_NONE;
}

static int lazercmd_interrupt(void)
{
/************************************************************
 * fake something toggling the sense input line of the S2650
 * the rate should be at about 1,5 Hz
 ************************************************************/
	lazercmd_sense_state ^= 1;
	S2650_set_sense(lazercmd_sense_state);
	return S2650_INT_NONE;
}

/*************************************************************
 *
 * IO port read/write
 *
 *************************************************************/

/* triggered by WRTC,r opcode */
static void lazercmd_ctrl_port_w(int offset, int data)
{
	if (errorlog)
		fprintf(errorlog, "lazercmd_ctrl_port_w %d <- $%02X\n", offset, data);
}

/* triggered by REDC,r opcode */
static int lazercmd_ctrl_port_r(int offset)
{
int data = 0;

	if (errorlog)
		fprintf(errorlog, "lazercmd_ctrl_port_r %d -> $%02X\n", offset, data);
	return data;
}

/* triggered by WRTD,r opcode */
static void lazercmd_data_port_w(int offset, int data)
{
	if (errorlog)
		fprintf(errorlog, "lazercmd_data_port_w %d <- $%02X\n", offset, data);
}

/* triggered by REDD,r opcode */
static int lazercmd_data_port_r(int offset)
{
int data = 0;

	data = input_port_2_r(0) & 0x0f;
	if (errorlog)
		fprintf(errorlog, "lazercmd_data_port_r %d -> $%02X\n", offset, data);
	return data;
}

/*************************************************************
 *
 * Hardware read/write
 *
 *************************************************************/
/* write to something like latches in memory range 0x1c00 - 0x1c1f */
static void lazercmd_latches_w(int offset, int data)
{
//	if (errorlog) fprintf(errorlog, "lazercmd_latches_w %d <- $%02X\n", offset, data);
	switch (offset)
	{
		case 0x03: /* left player marker activate */
			lazercmd_marker_dirty(0);
            break;
		case 0x04: /* right player marker activate */
			lazercmd_marker_dirty(1);
            break;
		case 0x0a: /* right player marker y */
			lazercmd_marker_dirty(1);
			break;
		case 0x0b: /* left player marker x */
			lazercmd_marker_dirty(0);
            break;
		case 0x0c: /* left player marker y */
			lazercmd_marker_dirty(0);
            break;
		case 0x0d: /* right player marker x */
			lazercmd_marker_dirty(1);
            break;
    }
    lazercmd_latches[offset] = data;
}

/* read from something like latches in memory range 0x1c00 - 0x1c1f */
static int lazercmd_latches_r(int offset)
{
int data = lazercmd_latches[offset];

//	if (errorlog) fprintf(errorlog, "lazercmd_latches_r %d -> $%02X\n", offset, data);
	return data;
}

static void lazercmd_hardware_w(int offset, int data)
{
static int DAC_data = 0;

	switch (offset)
	{
		case 0:				   /* speaker ? */
            if (errorlog) fprintf(errorlog, "lazercmd_hardware_w DAC $%02X\n", data);
			DAC_data = data & 0xf0;
			DAC_data_w(0, DAC_data);
			break;
		case 1:				   /* ???? it writes 0x05 here... */
			if (errorlog) fprintf(errorlog, "lazercmd_hardware_w hw1 $%02X\n", data);
            break;
		case 2:				   /* ???? it writes 0x3c / 0xcc here ... */
			if (errorlog) fprintf(errorlog, "lazercmd_hardware_w hw2 $%02X\n", data);
            lazercmd_hw_1f02 = data;
			break;
		case 3:				   /* ???? it writes 0x62 here... */
			if (errorlog) fprintf(errorlog, "lazercmd_hardware_w hw3 $%02X\n", data);
            break;
	}
}

static int lazercmd_hardware_r(int offset)
{
static int last_offset = 0;
static int last_data = 0;
static int last_1f02 = 0;
int data = 0;

	switch (offset)
	{
		case 0:				   /* player 1 joysticks */
			data = input_port_0_r(0);
			break;
		case 1:				   /* player 2 joysticks */
			data = input_port_1_r(0);
			break;
		case 2:				   /* player 1 + 2 buttons */
			data = input_port_4_r(0);
			break;
		case 3:				   /* coin slot + start buttons */
			data = (input_port_3_r(0) & 7) | lazercmd_vblank_state;
			lazercmd_vblank_state |= 8;
			break;
	}
	if (errorlog)
	{
		if (lazercmd_hw_1f02 != last_1f02 ||
			offset != last_offset ||
			data != last_data)
		{
			last_offset = offset;
			last_data = data;
			last_1f02 = lazercmd_hw_1f02;
			fprintf(errorlog, "lazercmd_hardware_r (1f02:%02X) 1f0%d -> $%02x\n", last_1f02, last_offset, last_data);
		}
	}
	return data;
}

static struct MemoryWriteAddress lazercmd_writemem[] =
{
	{0x0000, 0x0bff, MWA_ROM},
	{0x1c00, 0x1c1f, lazercmd_latches_w },
	{0x1c20, 0x1eff, videoram_w, &videoram, &videoram_size },
	{0x1f00, 0x1f03, lazercmd_hardware_w },
	{-1}					   /* end of table */
};

static struct MemoryReadAddress lazercmd_readmem[] =
{
	{0x0000, 0x0bff, MRA_ROM},
	{0x1c00, 0x1c1f, lazercmd_latches_r },
	{0x1c20, 0x1eff, MRA_RAM},
	{0x1f00, 0x1f03, lazercmd_hardware_r},
	{-1}					   /* end of table */
};

static struct IOWritePort lazercmd_writeport[] =
{
	{S2650_CTRL_PORT, S2650_CTRL_PORT, lazercmd_ctrl_port_w},
	{S2650_DATA_PORT, S2650_DATA_PORT, lazercmd_data_port_w},
	{-1}					   /* end of table */
};

static struct IOReadPort lazercmd_readport[] =
{
	{S2650_CTRL_PORT, S2650_CTRL_PORT, lazercmd_ctrl_port_r},
	{S2650_DATA_PORT, S2650_DATA_PORT, lazercmd_data_port_r},
	{-1}					   /* end of table */
};

INPUT_PORTS_START(lazercmd_input_ports)
	PORT_START					   /* IN0 player 1 controls */
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1)
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1)
		PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1)
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1)
		PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START					   /* IN1 player 2 controls */
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2)
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2)
		PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2)
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2)
		PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START					   /* IN2 dip switch */
		PORT_BITX(0x03, 0x03, IPT_DIPSWITCH_NAME, "Game Time", IP_KEY_NONE, IP_JOY_NONE, 0)
			PORT_DIPSETTING(0x00, "60 seconds")
			PORT_DIPSETTING(0x01, "90 seconds")
			PORT_DIPSETTING(0x02, "120 seconds")
			PORT_DIPSETTING(0x03, "180 seconds")
		PORT_BITX(0x08, 0x00, IPT_DIPSWITCH_NAME, "Coins", IP_KEY_NONE, IP_JOY_NONE, 0)
			PORT_DIPSETTING(0x00, "1 Coin Game")
			PORT_DIPSETTING(0x08, "2 Coin Game")
		PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BITX(0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Color overlay", OSD_KEY_F2, IP_JOY_NONE, 0)
			PORT_DIPSETTING(0x80, "On")
			PORT_DIPSETTING(0x00, "Off")
	PORT_START					   /* IN3 coinage & start */
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_COIN1)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_START2)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_START1)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_COIN2)
		PORT_BIT(0xf0, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START					   /* IN4 player 1 + 2 buttons ? */
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1)
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1)
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2)
		PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2)
		PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

    static struct GfxLayout charlayout =
	{
		8, 10,					/* 8*10 characters */
		4*64,					/* 4 * 64 characters */
		1,						/* 1 bit per pixel */
		{0},					/* no bitplanes */
		{0, 1, 2, 3, 4, 5, 6, 7},
		{0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8},
		10 * 8					/* every char takes 10 bytes */
	};

    static struct GfxDecodeInfo gfxdecodeinfo[] =
    {
        {1, 0x0000, &charlayout, 0, 6},
        {-1}                   /* end of array */
    };

/* some colors for the frontend */
static unsigned char palette[12 * 3] = {
	0x00,0x00,0x00,
	0xe0,0x00,0x00,
	0x00,0xe0,0x00,
	0xe0,0xe0,0x00,
	0x00,0x00,0xe0,
	0xe0,0x00,0xe0,
	0x00,0xe0,0xe0,
	0xe0,0xe0,0xe0,
	0x0e,0x0a,0x02,
	0xe0,0xa0,0x20,
	0x02,0x0e,0x02,
	0x20,0xe0,0x20,
};

static unsigned short colortable[6 * 2] = {
	 8,  9, 	/* mustard yellow bright on very dark */
	10, 11, 	/* jade green bright on very dark */
	 7,  0, 	/* white on black */
	 9,  8, 	/* above inverted */
	11, 10,
	 0,  7,
};


static struct DACinterface lazercmd_DAC_interface =
{
	1,
	{ 255 }
};

static struct MachineDriver lazercmd_machine_driver =
{
/* basic machine hardware */
	{
		{
			CPU_S2650,
			625000, 		/* 625 kHz? */
			0,
			lazercmd_readmem, lazercmd_writemem,
			lazercmd_readport, lazercmd_writeport,
			lazercmd_vblank, 5,
			lazercmd_interrupt, 1	/* one interrupt per second */
		}
	},
	60, 2000,	/* frames per second, vblank duration (arbitrary values!) */
	1,			/* single CPU, no need for interleaving */
	0,

/* video hardware */
	HORZ_RES * HORZ_CHR, VERT_RES * VERT_CHR,
	{0 * HORZ_CHR, HORZ_RES * HORZ_CHR - 1,
	 0 * VERT_CHR, VERT_RES * VERT_CHR - 1},

	gfxdecodeinfo,
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(colortable) / sizeof(colortable[0]),
	0,					   /* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	lazercmd_vh_screenrefresh,

/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_DAC,
			&lazercmd_DAC_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

	ROM_START(lazercmd_rom)
	ROM_REGION(0x8000)			   /* 32K cpu, 4K for ROM/RAM */
	ROM_LOAD( "lc.e5",        0x0000, 0x0400, 0x56dc7a40 )
	ROM_LOAD( "lc.e6",        0x0400, 0x0400, 0xb1ef0aa2 )
	ROM_LOAD( "lc.e7",        0x0800, 0x0400, 0x8e6ffc97 )
	ROM_LOAD( "lc.f5",        0x1000, 0x0400, 0xfc5b38a4 )
	ROM_LOAD( "lc.f6",        0x1400, 0x0400, 0x26eaee21 )
	ROM_LOAD( "lc.f7",        0x1800, 0x0400, 0x9ec3534d )
	ROM_REGION_DISPOSE(0x0c00)	   /* 4 times 32, 8x8 character generator */
	ROM_LOAD( "lc.b8",        0x0a00, 0x0200, 0x6d708edd )
	ROM_END

void lazercmd_rom_decode(void)
{
int i, y;

/* The ROMs are 1K x 4 bit, so we have to mix */
/* them into 8 bit bytes. They are inverted too */
	for (i = 0; i < 0x0c00; i++)
	{
		Machine->memory_region[0][i + 0x0000] =
			((Machine->memory_region[0][i + 0x0000] << 4) |
			 (Machine->memory_region[0][i + 0x1000] & 15)) ^ 0xff;
	}
/******************************************************************
 * To show the maze bit #6 and #7 of the video ram are used.
 * Bit #7: add a vertical line to the right of the character
 * Bit #6: add a horizontal line below the character
 * The video logic generates 10 lines per character row, but the
 * character generator only contains 8 rows, so we expand the
 * font to 8x10. Special care is taken of some characters
 * that are used together on to adjacent character cells.
 ******************************************************************/
	for (i = 0; i < 0x40; i++)
	{
unsigned char *d = &Machine->memory_region[1][0x0000 + i * VERT_CHR];
unsigned char *s = &Machine->memory_region[1][0x0a00 + i * VERT_FNT];

		for (y = 0; y < VERT_CHR; y++)
		{
			*d = (y < VERT_FNT) ? *s++ : 0xff;
			switch (i)
			{

				case 0x22:	   /* front of white tank going left */
				case 0x2b:	   /* rear of white tank going right */
					if (y == 2 || y == 6)
						*d &= 0xfe;
					break;
				case 0x32:	   /* front of black tank going left */
				case 0x3b:	   /* rear of black tank going right */
					if (y >= 2 && y <= 6)
						*d &= 0xfe;
					break;
				case 0x26:	   /* front of white tank going up */
				case 0x2f:	   /* rear of white tank going down */
					if (y >= 8)
						*d &= 0xbb;
					break;
				case 0x36:	   /* front of black tank going up */
				case 0x3f:	   /* read of black tank going down */
					if (y >= 8)
						*d &= 0x83;
					break;
			}
			d[0x280] = (y == VERT_CHR - 1) ? 0x00 : d[0x000];
			d[0x500] = d[0x000] & 0xfe;
			d[0x780] = (y == VERT_CHR - 1) ? 0x00 : d[0x000] & 0xfe;
			d++;
		}
	}
}

struct GameDriver lazercmd_driver =
{
	__FILE__,
	0,
	"lazercmd",
	"Lazer Command",
	"1976",
	"Meadows Games, Inc.",
	"Juergen Buchmueller\nMalcolm (Markers tech. info)\n",
	0,
	&lazercmd_machine_driver,
	0,

	lazercmd_rom,
	lazercmd_rom_decode,
	0,
	0,
	0,      /* sound_prom */

	lazercmd_input_ports,

	0,		/* color_prom */
	palette,
	colortable,
	ORIENTATION_DEFAULT,

	0,0
};
