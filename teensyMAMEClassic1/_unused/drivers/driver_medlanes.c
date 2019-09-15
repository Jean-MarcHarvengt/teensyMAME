/***************************************************************************
  Meadows Lanes memory map (preliminary)

  0000-0bff ROM

  0c00-0fff RAM

  1000-17ff ROM

  1c00-1c1f RAM 	used by the CPU (most probably latches only!?)
  1c03		D4		left  player marker
  1c04		D4		right player marker
  1c08		D0-D7	left  player y coord
  1c09      D0-D7   left  player x coord
  1c0a		D0-D7	right player y coord
  1c0b		D0-D7	right player x coord

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
					D0 DIP switch (1 / 2 Coins per game)
					D4 coin

  I/O ports:

  'data'    R   dip switch settings
                D0 - D1 Game time 60,90,120,180 seconds
                D2      unused
                D3      Coins per game 1, 2

***************************************************************************/

#include "driver.h"
#include "S2650/s2650.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/medlanes.h"

#define VERBOSE 1
#define VERBOSE_LATCHES 0
#define VERBOSE_MARKER	1

int medlanes_latches[32];

/*************************************************************
 *
 * externals
 *
 *************************************************************/
int medlanes_vh_start(void);
void medlanes_vh_stop(void);
void medlanes_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void medlanes_marker_dirty(int marker);

/*************************************************************
 *
 * Statics
 *
 *************************************************************/

static int medlanes_interrupt(void)
{
static int medlanes_sense_state = 0;
/************************************************************
 * fake something toggling the sense input line of the S2650
 * the rate should be at about 1,5 Hz
 ************************************************************/
	medlanes_sense_state ^= 1;
    S2650_set_sense(medlanes_sense_state);
	return S2650_INT_NONE;
}

/*************************************************************
 *
 * IO port read/write
 *
 *************************************************************/

/* triggered by WRTC,r opcode */
static void medlanes_ctrl_port_w(int offset, int data)
{
#if VERBOSE
    if (errorlog)
		fprintf(errorlog, "medlanes_ctrl_port_w %d <- $%02X\n", offset, data);
#endif
}

/* triggered by REDC,r opcode */
static int medlanes_ctrl_port_r(int offset)
{
int data = 0;

#if VERBOSE
    if (errorlog)
		fprintf(errorlog, "medlanes_ctrl_port_r %d -> $%02X\n", offset, data);
#endif
    return data;
}

/* triggered by WRTD,r opcode */
static void medlanes_data_port_w(int offset, int data)
{
#if VERBOSE
    if (errorlog)
		fprintf(errorlog, "medlanes_data_port_w %d <- $%02X\n", offset, data);
#endif
}

/* triggered by REDD,r opcode */
static int medlanes_data_port_r(int offset)
{
int data = 0;

	data = input_port_2_r(0) & 0x0f;
#if VERBOSE
    if (errorlog)
		fprintf(errorlog, "medlanes_data_port_r %d -> $%02X\n", offset, data);
#endif
    return data;
}

/*************************************************************
 *
 * Hardware read/write
 *
 *************************************************************/
/* write to something like latches in memory range 0x1c00 - 0x1c1f */
static void medlanes_latches_w(int offset, int data)
{
	switch (offset)
	{
		case MARKER0_ACTIVE:
#if VERBOSE_MARKER
			if (errorlog)
				fprintf(errorlog, "medlanes_latches_w MARKER0_ACTIVE %02x\n", data);
#endif
            medlanes_marker_dirty(0);
            break;
		case MARKER1_ACTIVE:
#if VERBOSE_MARKER
			if (errorlog)
				fprintf(errorlog, "medlanes_latches_w MARKER1_ACTIVE %02x\n", data);
#endif
            medlanes_marker_dirty(1);
            break;
		case MARKER0_VERT:
#if VERBOSE_MARKER
            if (errorlog)
				fprintf(errorlog, "medlanes_latches_w MARKER0_VERT %02x\n", data);
#endif
            medlanes_marker_dirty(0);
			break;
		case MARKER0_HORZ:
#if VERBOSE_MARKER
			if (errorlog)
				fprintf(errorlog, "medlanes_latches_w MARKER0_HORZ %02x\n", data);
#endif
            medlanes_marker_dirty(0);
            break;
		case MARKER1_VERT:
#if VERBOSE_MARKER
			if (errorlog)
				fprintf(errorlog, "medlanes_latches_w MARKER1_VERT %02x\n", data);
#endif
            medlanes_marker_dirty(1);
            break;
		case MARKER1_HORZ:
#if VERBOSE_MARKER
            if (errorlog)
				fprintf(errorlog, "medlanes_latches_w MARKER1_HORZ %02x\n", data);
#endif
            medlanes_marker_dirty(1);
            break;
    }
	medlanes_latches[offset] = data;
}

/* read from something like latches in memory range 0x1c00 - 0x1c1f */
static int medlanes_latches_r(int offset)
{
int data = medlanes_latches[offset];
#if VERBOSE_LATCHES
	if (errorlog)
		fprintf(errorlog, "medlanes_latches_r %d -> $%02X\n", offset, data);
#endif
    return data;
}

static void medlanes_hardware_w(int offset, int data)
{
static int DAC_data = 0;

	switch (offset)
	{
		case 0:				   /* speaker ? */
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "medlanes_hardware_w DAC $%02X\n", data);
#endif
            DAC_data = data & 0xf0;
			DAC_data_w(0, DAC_data);
			break;
		case 1:				   /* ???? it writes 0x05 here... */
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "medlanes_hardware_w hw1 $%02X\n", data);
#endif
            break;
		case 2:				   /* ???? it writes 0x3c / 0xcc here ... */
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "medlanes_hardware_w hw2 $%02X\n", data);
#endif
            break;
		case 3:				   /* ???? it writes 0x62 here... */
#if VERBOSE
			if (errorlog)
				fprintf(errorlog, "medlanes_hardware_w hw3 $%02X\n", data);
#endif
            break;
	}
}

static int medlanes_hardware_r(int offset)
{
static int last_offset = 0;
static int last_data = 0;
int data = 0;

	switch (offset)
	{
		case 0: 			   /* player 1 joystick */
			data = input_port_0_r(0);
			break;
		case 1: 			   /* player 2 joystick */
			data = input_port_1_r(0);
			break;
		case 2:				   /* player 1 + 2 buttons */
			data = input_port_4_r(0);
			break;
		case 3:				   /* coin slot + start buttons */
			data = input_port_3_r(0);
			break;
	}
#if VERBOSE
    if (errorlog)
	{
		if (offset != last_offset ||
			data != last_data)
		{
			last_offset = offset;
			last_data = data;
			fprintf(errorlog, "medlanes_hardware_r $1f0%d -> $%02x\n", last_offset, last_data);
		}
	}
#endif
    return data;
}

static struct MemoryWriteAddress medlanes_writemem[] =
{
	{ 0x0000, 0x0bff, MWA_ROM },
	{ 0x1000, 0x1800, MWA_ROM },
	{ 0x1c00, 0x1c1f, medlanes_latches_w },
	{ 0x1c20, 0x1eff, videoram_w, &videoram, &videoram_size },
	{ 0x1f00, 0x1f03, medlanes_hardware_w },
	{-1}					   /* end of table */
};

static struct MemoryReadAddress medlanes_readmem[] =
{
	{ 0x0000, 0x0bff, MRA_ROM },
	{ 0x1000, 0x1800, MRA_ROM },
	{ 0x1c00, 0x1c1f, medlanes_latches_r },
	{ 0x1c20, 0x1eff, MRA_RAM },
	{ 0x1f00, 0x1f03, medlanes_hardware_r },
	{-1}					   /* end of table */
};

static struct IOWritePort medlanes_writeport[] =
{
	{S2650_CTRL_PORT, S2650_CTRL_PORT, medlanes_ctrl_port_w},
	{S2650_DATA_PORT, S2650_DATA_PORT, medlanes_data_port_w},
	{-1}					   /* end of table */
};

static struct IOReadPort medlanes_readport[] =
{
	{S2650_CTRL_PORT, S2650_CTRL_PORT, medlanes_ctrl_port_r},
	{S2650_DATA_PORT, S2650_DATA_PORT, medlanes_data_port_r},
	{-1}					   /* end of table */
};

INPUT_PORTS_START(medlanes_input_ports)
	PORT_START					   /* IN0 player 1 controls */
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1		  | IPF_PLAYER1 )
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED						)
		PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1 )
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER1 )
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2		  | IPF_PLAYER1 )
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3		  | IPF_PLAYER1 )
		PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED						)
	PORT_START					   /* IN1 player 2 controls */
		PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1		  | IPF_PLAYER2 )
		PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED						)
		PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
		PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2		  | IPF_PLAYER2 )
		PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3		  | IPF_PLAYER2 )
        PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED                       )
    PORT_START                     /* IN2 coinage & start */
		PORT_BIT(0xff, IP_ACTIVE_HIGH, IPT_UNUSED)
	PORT_START					   /* IN3 dip switch */
		PORT_BITX(0x01, 0x00, IPT_DIPSWITCH_NAME, "Coins", IP_KEY_NONE, IP_JOY_NONE, 0)
            PORT_DIPSETTING(0x00, "1 Coin Game")
			PORT_DIPSETTING(0x01, "2 Coin Game")
		PORT_BIT( 0x06, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1)
		PORT_BIT( 0x70, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1)
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
		{1, 0x0000, &charlayout, 0, 1},
        {-1}                   /* end of array */
    };

/* some colors for the frontend */
static unsigned char palette[] = {
	0x00,0x00,0x00,
	0xe0,0x00,0x00,
	0x00,0xe0,0x00,
	0xe0,0xe0,0x00,
	0x00,0x00,0xe0,
	0xe0,0x00,0xe0,
	0x00,0xe0,0xe0,
	0xe0,0xe0,0xe0,
};

static unsigned short colortable[] = {
	 7, 0,	  /* white on black */
};


static struct DACinterface medlanes_DAC_interface =
{
	1,			/* number of DACs */
	{ 100, }	/* volume */
};

static struct MachineDriver medlanes_machine_driver =
{
/* basic machine hardware */
	{
		{
			CPU_S2650,
			312500, 		/* 312.5 kHz? */
			0,
			medlanes_readmem, medlanes_writemem,
			medlanes_readport, medlanes_writeport,
			0, 0,
			medlanes_interrupt, 1	/* one interrupt per second */
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
	medlanes_vh_screenrefresh,

/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_DAC,
			&medlanes_DAC_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

	ROM_START(medlanes_rom)
	ROM_REGION(0x8000)			   /* 32K cpu, 4K for ROM/RAM */
	ROM_LOAD("medlanes.2a", 0x0000, 0x0400, 0x9c77566a)
	ROM_LOAD("medlanes.2b", 0x0400, 0x0400, 0x7841b1a9)
	ROM_LOAD("medlanes.2c", 0x0800, 0x0400, 0xa359b5b8)
	ROM_LOAD("medlanes.1a", 0x1000, 0x0400, 0x0d57c596)
	ROM_LOAD("medlanes.1b", 0x1400, 0x0400, 0x1d451630)
	ROM_LOAD("medlanes.3a", 0x4000, 0x0400, 0x22bc56a6)
	ROM_LOAD("medlanes.3b", 0x4400, 0x0400, 0x6616dbef)
	ROM_LOAD("medlanes.3c", 0x4800, 0x0400, 0xb3db0f3d)
	ROM_LOAD("medlanes.4a", 0x5000, 0x0400, 0x30d495e9)
	ROM_LOAD("medlanes.4b", 0x5400, 0x0400, 0xa4abb5db)
	ROM_REGION_DISPOSE(0x0c00)	   /* 4 times 64, 8x8 character generator */
	ROM_LOAD("medlanes.8b", 0x0a00, 0x0200, 0x44e5de8f)
	ROM_END

void medlanes_rom_decode(void)
{
int i, y;

/*
 * The ROMs are 1K x 4 bit, so we have to mix them into bytes.
 * ...and they are inverted
 */
	for (i = 0; i < 0x4000; i++)
	{
		Machine->memory_region[0][i + 0x0000] =
			~((Machine->memory_region[0][i + 0x0000] << 4) |
			  (Machine->memory_region[0][i + 0x4000] & 15));
	}
/******************************************************************
 * To show the maze bit #6 and #7 of the video ram are used.
 * Bit #7: add a vertical line to the right of the character
 * Bit #6: add a horizontal line below the character
 * The video logic generates 10 lines per character row, but the
 * character generator only contains 8 rows, so we expand the
 * font to 8x10.
 ******************************************************************/
	for (i = 0; i < 0x40; i++)
	{
unsigned char *d = &Machine->memory_region[1][0 * 64 * 10 + i * VERT_CHR];
unsigned char *s = &Machine->memory_region[1][4 * 64 * 10 + i * VERT_FNT];

		for (y = 0; y < VERT_CHR; y++)
		{
			d[0*64*10] = (y < VERT_FNT) ? *s++ : 0xff;
			d[1*64*10] = (y == VERT_CHR - 1) ? 0 : *d;
			d[2*64*10] = *d & 0xfe;
			d[3*64*10] = (y == VERT_CHR - 1) ? 0 : *d & 0xfe;
			d++;
		}
    }
}

struct GameDriver medlanes_driver =
{
	__FILE__,
	0,
	"medlanes",
	"Meadows Lanes",
	"1977",
	"Meadows Games, Inc.",
	"Juergen Buchmueller",
	0,
	&medlanes_machine_driver,
	0,

	medlanes_rom,
	medlanes_rom_decode,
	0,
	0,
	0,      /* sound_prom */

	medlanes_input_ports,

	0,		/* color_prom */
	palette,
	colortable,
	ORIENTATION_DEFAULT,

	0,0
};

