/***************************************************************************

  Coors Light Bowling/Bowl-O-Rama memory map

  CPU Board:

  0000-3fff     3 Graphics ROMS mapped in using 0x4800 (Coors Light Bowling only)
  0000-001f		Turbo board area (Bowl-O-Rama only) See Below.
  4000          Display row selected
  4800          Graphics ROM select
  5000-57ff     Battery backed up RAM (Saves machine state after shut off)
                Enter setup menu by holding down the F2 key on the
                high score screen
  5800-5fff		TMS34061 area

                First 0x20 bytes of each row provide a 16 color palette for this
                row. 2 bytes per color: 0000RRRR GGGGBBBB.

                Remaining 0xe0 bytes contain 2 pixels each for a total of
                448 pixels, but only 360 seem to be displayed.
                (Each row appears vertically because the monitor is rotated)

  6000          Sound command
  6800			Trackball Reset. Double duties as a watchdog.
  7000          Input port 1    Bit 0-3 Trackball Vertical Position
							  	Bit 4   Player 2 Hook Left
								Bit 5   Player 2 Hook Right
								Bit 6   Upright/Cocktail DIP Switch
                                Bit 7   Coin 2
  7800          Input port 2    Bit 0-3 Trackball Horizontal Positon
                                Bit 4   Player 1 Hook Left
                                Bit 5   Player 1 Hook Right
                                Bit 6   Start
                                Bit 7   Coin 1
  8000-ffff		ROM


  Sound Board:

  0000-07ff		RAM
  1000-1001		YM2203
			  	Port A D7 Read  is ticket sensor
				Port B D7 Write is ticket dispenser enable
				Port B D6 looks like a diagnostics LED to indicate that
				          the PCB is operating. It's pulsated by the
						  sound CPU. It is kind of pointless to emulate it.
  2000			Not hooked up according to the schematics
  6000			DAC write
  7000			Sound command read (0x34 is used to dispense a ticket)
  8000-ffff		ROM


  Turbo Board Layout (Plugs in place of GR0):

  Bowl-O-Rama	Copyright 1991 P&P Marketing
				Marquee says "EXIT Entertainment"

				This portion: Mike Appolo with the help of Andrew Pines.
				Andrew was one of the game designers for Capcom Bowling,
				Coors Light Bowling, Strata Bowling, and Bowl-O-Rama.

				This game was an upgrade for Capcom Bowling and included a
				"Turbo PCB" that had a GAL address decoder / data mask

  Memory Map for turbo board (where GR0 is on Capcom Bowling PCBs:

  0000   		Read Mask
  0001-0003		Unused
  0004  		Read Data
  0005-0007		Unused
  0008  		GR Address High Byte (GR17-16)
  0009-0016		Unused
  0017			GR Address Middle Byte (GR15-0 written as a word to 0017-0018)
  0018  		GR address Low byte
  0019-001f		Unused

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/tms34061.h"
#include "machine/ticket.h"
#include "M6809/M6809.h"

void capbowl_init_machine(void);

void capbowl_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

int  capbowl_vh_start(void);
void capbowl_vh_stop(void);

extern unsigned char *capbowl_rowaddress;

void capbowl_rom_select_w(int offset,int data);

int  capbowl_pagedrom_r(int offset);

void bowlrama_turbo_w(int offset, int data);
int  bowlrama_turbo_r(int offset);



static void capbowl_sndcmd_w(int offset,int data)
{
	cpu_cause_interrupt(1, M6809_INT_IRQ);

	soundlatch_w(offset, data);
}


/* Handler called by the 2203 emulator when the internal timers cause an IRQ */
static void firqhandler(void)
{
	cpu_cause_interrupt(1, M6809_INT_FIRQ);
}


/***************************************************************************

  NMI is to trigger the self test. We use a fake input port to tie that
  event to a keypress.

***************************************************************************/
static int capbowl_interrupt(void)
{
	if (readinputport(2) & 1)	/* get status of the F2 key */
		return nmi_interrupt();	/* trigger self test */

	return ignore_interrupt();
}


static struct MemoryReadAddress capbowl_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_BANK1 },
	{ 0x5000, 0x57ff, MRA_RAM },
	{ 0x5800, 0x5fff, TMS34061_r },
	{ 0x7000, 0x7000, input_port_0_r },
	{ 0x7800, 0x7800, input_port_1_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress bowlrama_readmem[] =
{
	{ 0x0000, 0x001f, bowlrama_turbo_r },
	{ 0x5000, 0x57ff, MRA_RAM },
	{ 0x5800, 0x5fff, TMS34061_r },
	{ 0x7000, 0x7000, input_port_0_r },
	{ 0x7800, 0x7800, input_port_1_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x001f, bowlrama_turbo_w },	/* Bowl-O-Rama only */
	{ 0x4000, 0x4000, MWA_RAM, &capbowl_rowaddress },
	{ 0x4800, 0x4800, capbowl_rom_select_w },
	{ 0x5000, 0x57ff, MWA_RAM },
	{ 0x5800, 0x5fff, TMS34061_w },
	{ 0x6000, 0x6000, capbowl_sndcmd_w },
	{ 0x6800, 0x6800, watchdog_reset_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x1000, 0x1000, YM2203_status_port_0_r },
	{ 0x1001, 0x1001, YM2203_read_port_0_r },
	{ 0x7000, 0x7000, soundlatch_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM},
	{ 0x1000, 0x1000, YM2203_control_port_0_w },
	{ 0x1001, 0x1001, YM2203_write_port_0_w },
	{ 0x2000, 0x2000, MWA_NOP },  /* Not hooked up according to the schematics */
	{ 0x6000, 0x6000, DAC_data_w },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_ANALOG ( 0x0f, 0x00, IPT_TRACKBALL_Y | IPF_REVERSE | IPF_CENTER, 100, 7, 0, 0 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE ) /* This version of Bowl-O-Rama */
	PORT_DIPSETTING(    0x40, "Upright" )			   /* is Upright only */
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START	/* IN1 */
	PORT_ANALOG ( 0x0f, 0x00, IPT_TRACKBALL_X | IPF_CENTER, 100, 7, 0, 0 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
INPUT_PORTS_END


static struct YM2203interface ym2203_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3 MHz ??? */
	{ YM2203_VOL(40,40) },
	{ ticket_dispenser_r },
	{ 0 },
	{ 0 },
	{ ticket_dispenser_w },  /* Also a status LED. See memory map above */
	{ firqhandler }
};

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};


#define MACHINEDRIVER(NAME, VISIBLE_Y)						\
															\
static struct MachineDriver NAME##_machine_driver =			\
{															\
	/* basic machine hardware */   							\
	{														\
		{													\
			CPU_M6809,										\
			2000000,        /* 2 Mhz */						\
			0,												\
			NAME##_readmem,writemem,0,0,					\
			capbowl_interrupt, 1,       /* To check Service mode status */ \
		},													\
		{													\
			CPU_M6809 | CPU_AUDIO_CPU,						\
			2000000,        /* 2 Mhz */						\
			2,												\
			sound_readmem,sound_writemem,0,0,				\
			ignore_interrupt,1	/* interrupts are generated by the sound hardware */ \
		}													\
	},														\
	60, 5000,	/* frames per second, vblank duration (guess) */ \
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */ \
	capbowl_init_machine,									\
															\
	/* video hardware */									\
	360, 256, { 0, 359, 0, VISIBLE_Y },						\
	0,														\
	16*256,16*256,											\
	0,														\
															\
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_SUPPORTS_16BIT,	\
	0,														\
	capbowl_vh_start,										\
	capbowl_vh_stop,										\
	capbowl_vh_screenrefresh,								\
															\
	/* sound hardware */									\
	0,0,0,0,												\
	{														\
		{													\
			SOUND_YM2203,									\
			&ym2203_interface								\
		},													\
		{													\
			SOUND_DAC,										\
			&dac_interface									\
		}													\
	}														\
};


MACHINEDRIVER(capbowl,  244)

MACHINEDRIVER(bowlrama, 239)


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( capbowl_rom )
	ROM_REGION(0x28000)   /* 160k for code and graphics */
	ROM_LOAD( "u6",           0x08000, 0x8000, 0x14924c96 )
	ROM_LOAD( "gr0",          0x10000, 0x8000, 0xef53ca7a )
	ROM_LOAD( "gr1",          0x18000, 0x8000, 0x27ede6ce )
	ROM_LOAD( "gr2",          0x20000, 0x8000, 0xe49238f4 )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)   /* 64k for sound */
	ROM_LOAD( "sound",        0x8000, 0x8000, 0x8c9c3b8a )
ROM_END

ROM_START( clbowl_rom )
	ROM_REGION(0x28000)   /* 160k for code and graphics */
	ROM_LOAD( "u6.cl",        0x08000, 0x8000, 0x91e06bc4 )
	ROM_LOAD( "gr0.cl",       0x10000, 0x8000, 0x899c8f15 )
	ROM_LOAD( "gr1.cl",       0x18000, 0x8000, 0x0ac0dc4c )
	ROM_LOAD( "gr2.cl",       0x20000, 0x8000, 0x251f5da5 )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)   /* 64k for sound */
	ROM_LOAD( "sound.cl",     0x8000, 0x8000, 0x1eba501e )
ROM_END

ROM_START( bowlrama_rom )
	ROM_REGION(0x10000)      /* 64k for code */
	ROM_LOAD( "u6",           0x08000, 0x08000, 0x7103ad55 )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for sound */
	ROM_LOAD( "u30",          0x8000, 0x8000, 0xf3168834 )

	ROM_REGION(0x40000)     /* 256K for Graphics */
	ROM_LOAD( "ux7",          0x00000, 0x40000, 0x8727432a )
ROM_END


static int hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* Try loading static RAM */
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0x5000],0x800);
		osd_fclose(f);
	}
	else
	{
		/* Invalidate the static RAM to force reset to factory settings */
		memset(&RAM[0x5000],0x01,0x800);
	}

	RAM[0x5725] = 0x01; // Otherwise Bowl-O-Rama won't start
						// This location doesn't seem to be used by
						// Capcom Bowling, so this should be Ok.
	return 1;
}

static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x5000],0x0800);
		osd_fclose(f);
	}
}



struct GameDriver capbowl_driver =
{
	__FILE__,
	0,
	"capbowl",
	"Capcom Bowling",
	"1988",
	"Incredible Technologies",
	"Zsolt Vasvari\nMirko Buffoni\nNicola Salmoria\nMichael Appolo",
	0,
	&capbowl_machine_driver,
	0,

	capbowl_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver clbowl_driver =
{
	__FILE__,
	&capbowl_driver,
	"clbowl",
	"Coors Light Bowling",
	"1989",
	"Incredible Technologies",
	"Zsolt Vasvari\nMirko Buffoni\nNicola Salmoria\nMichael Appolo",
	0,
	&capbowl_machine_driver,
	0,

	clbowl_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};

struct GameDriver bowlrama_driver =
{
	__FILE__,
	0,
	"bowlrama",
	"Bowl-O-Rama",
	"1991",
	"P & P Marketing",
	"Michael Appolo\nZsolt Vasvari\nMirko Buffoni\nNicola Salmoria",
	0,
	&bowlrama_machine_driver,
	0,

	bowlrama_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	hiload, hisave
};
