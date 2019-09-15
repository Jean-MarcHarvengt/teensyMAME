/****************************************************************************

	Exterminator memory map


 Master CPU (TMS34010, all addresses are in bits)

 00000000-000fffff RW Video RAM (256x256x15)
 00c00000-00ffffff RW RAM
 01000000-010fffff  W Host Control Interface (HSTADRL)
 01100000-011fffff  W Host Control Interface (HSTADRH)
 01200000-012fffff RW Host Control Interface (HSTDATA)
 01300000-013fffff  W Host Control Interface (HSTCTLH)
 01400000-01400007 R  Input Port 0
 01400008-0140000f R  Input Port 1
 01440000-01440007 R  Input Port 2
 01440008-0144000f R  Input Port 3
 01480000-01480007 R  Input Port 4
 01500000-0150000f  W Output Port 0 (See machine/exterm.c)
 01580000-0158000f  W Sound Command
 015c0000-015c000f  W Watchdog
 01800000-01807fff RW Palette RAM
 02800000-02807fff RW EEPROM
 03000000-03ffffff R  ROM
 3f000000-3fffffff R  ROM Mirror
 c0000000-c00001ff RW TMS34010 I/O Registers
 ff000000-ffffffff R  ROM Mirror


 Slave CPU (TMS34010, all addresses are in bits)

 00000000-000fffff RW Video RAM (2 banks of 256x256x8)
 c0000000-c00001ff RW TMS34010 I/O Registers
 ff800000-ffffffff RW RAM


 DAC Controller CPU (6502)

 0000-07ff RW RAM
 4000      R  Sound Command
 8000-8001  W 2 Channels of DAC output
 8000-ffff R  ROM


 YM2151 Controller CPU (6502)

 0000-07ff RW RAM
 4000       W YM2151 Command/Data Register (Controlled by a bit A000)
 6000  		W NMI occurence rate (fed into a binary counter)
 6800      R  Sound Command
 7000      R  Causes NMI on DAC CPU
 8000-ffff R  ROM
 a000       W Control register (see sndhrdw/gottlieb.c)

****************************************************************************/

#include "driver.h"
#include "TMS34010/tms34010.h"

static unsigned char *eeprom;
static int eeprom_size;
static int code_rom_size;
unsigned char *exterm_code_rom;

extern unsigned char *exterm_master_speedup, *exterm_slave_speedup;
extern unsigned char *exterm_master_videoram, *exterm_slave_videoram;

/* Functions in vidhrdw/exterm.c */
void exterm_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int  exterm_vh_start(void);
void exterm_vh_stop (void);
int  exterm_master_videoram_r(int offset);
int  exterm_slave_videoram_r(int offset);
void exterm_paletteram_w(int offset, int data);
void exterm_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* Functions in sndhrdw/gottlieb.c */
void gottlieb_sh_w(int offset,int data);
int  gottlieb_cause_dac_nmi_r(int offset);
void gottlieb_nmi_rate_w(int offset, int data);
void exterm_sound_control_w(int offset, int data);
void exterm_ym2151_w(int offset, int data);

/* Functions in machine/exterm.c */
void exterm_init_machine(void);
int  exterm_coderom_r(int offset);
int  exterm_input_port_0_1_r(int offset);
int  exterm_input_port_2_3_r(int offset);
void exterm_output_port_0_w(int offset, int data);
int  exterm_master_speedup_r(int offset);
void exterm_slave_speedup_w(int offset, int data);
int  exterm_sound_dac_speedup_r(int offset);
int  exterm_sound_ym2151_speedup_r(int offset);


static struct MemoryReadAddress master_readmem[] =
{
	{ TOBYTE(0x00000000), TOBYTE(0x000fffff), exterm_master_videoram_r, &exterm_master_videoram },
	{ TOBYTE(0x00c800e0), TOBYTE(0x00c800ef), exterm_master_speedup_r, &exterm_master_speedup },
	{ TOBYTE(0x00c00000), TOBYTE(0x00ffffff), MRA_BANK1 },
	{ TOBYTE(0x01000000), TOBYTE(0x0100000f), MRA_NOP }, /* Off by one bug in RAM test, prevent log entry */
	{ TOBYTE(0x01200000), TOBYTE(0x012fffff), TMS34010_HSTDATA_r },
	{ TOBYTE(0x01400000), TOBYTE(0x0140000f), exterm_input_port_0_1_r },
	{ TOBYTE(0x01440000), TOBYTE(0x0144000f), exterm_input_port_2_3_r },
	{ TOBYTE(0x01480000), TOBYTE(0x0148000f), input_port_4_r },
	{ TOBYTE(0x01800000), TOBYTE(0x01807fff), paletteram_word_r },
	{ TOBYTE(0x01808000), TOBYTE(0x0180800f), MRA_NOP }, /* Off by one bug in RAM test, prevent log entry */
	{ TOBYTE(0x02800000), TOBYTE(0x02807fff), MRA_BANK2, &eeprom, &eeprom_size },
	{ TOBYTE(0x03000000), TOBYTE(0x03ffffff), exterm_coderom_r },
	{ TOBYTE(0x3f000000), TOBYTE(0x3fffffff), exterm_coderom_r },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), TMS34010_io_register_r },
	{ TOBYTE(0xff000000), TOBYTE(0xffffffff), MRA_BANK3, &exterm_code_rom, &code_rom_size },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress master_writemem[] =
{
  /*{ TOBYTE(0x00000000), TOBYTE(0x000fffff), exterm_master_videoram_16_w },	 OR		*/
  /*{ TOBYTE(0x00000000), TOBYTE(0x000fffff), exterm_master_videoram_8_w },				*/
	{ TOBYTE(0x00c00000), TOBYTE(0x00ffffff), MWA_BANK1 },
	{ TOBYTE(0x01000000), TOBYTE(0x010fffff), TMS34010_HSTADRL_w },
	{ TOBYTE(0x01100000), TOBYTE(0x011fffff), TMS34010_HSTADRH_w },
	{ TOBYTE(0x01200000), TOBYTE(0x012fffff), TMS34010_HSTDATA_w },
	{ TOBYTE(0x01300000), TOBYTE(0x013fffff), TMS34010_HSTCTLH_w },
	{ TOBYTE(0x01500000), TOBYTE(0x0150000f), exterm_output_port_0_w },
	{ TOBYTE(0x01580000), TOBYTE(0x0158000f), gottlieb_sh_w },
	{ TOBYTE(0x015c0000), TOBYTE(0x015c000f), watchdog_reset_w },
	{ TOBYTE(0x01800000), TOBYTE(0x01807fff), exterm_paletteram_w, &paletteram },
	{ TOBYTE(0x02800000), TOBYTE(0x02807fff), MWA_BANK2 }, /* EEPROM */
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), TMS34010_io_register_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress slave_readmem[] =
{
	{ TOBYTE(0x00000000), TOBYTE(0x000fffff), exterm_slave_videoram_r, &exterm_slave_videoram },
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), TMS34010_io_register_r },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MRA_BANK4 },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress slave_writemem[] =
{
  /*{ TOBYTE(0x00000000), TOBYTE(0x000fffff), exterm_slave_videoram_16_w },      OR		*/
  /*{ TOBYTE(0x00000000), TOBYTE(0x000fffff), exterm_slave_videoram_8_w },       OR		*/
	{ TOBYTE(0xc0000000), TOBYTE(0xc00001ff), TMS34010_io_register_w },
	{ TOBYTE(0xfffffb90), TOBYTE(0xfffffb90), exterm_slave_speedup_w, &exterm_slave_speedup },
	{ TOBYTE(0xff800000), TOBYTE(0xffffffff), MWA_BANK4 },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_dac_readmem[] =
{
	{ 0x0007, 0x0007, exterm_sound_dac_speedup_r },
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x4000, 0x4000, soundlatch_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_dac_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x8000, 0x8001, DAC_data_w },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sound_ym2151_readmem[] =
{
	{ 0x02b6, 0x02b6, exterm_sound_ym2151_speedup_r },
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x6800, 0x6800, soundlatch_r },
	{ 0x7000, 0x7000, gottlieb_cause_dac_nmi_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_ym2151_writemem[] =
{
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x4000, 0x4000, exterm_ym2151_w },
	{ 0x6000, 0x6000, gottlieb_nmi_rate_w },
	{ 0xa000, 0xa000, exterm_sound_control_w },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( exterm_input_ports )

	PORT_START      /* IN0 LO */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )

	PORT_START      /* IN0 HI */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1, "Aim Left",  OSD_KEY_Z, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1, "Aim Right", OSD_KEY_X, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xec, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* IN1 LO */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )

	PORT_START      /* IN1 HI */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2, "2 Aim Left",  OSD_KEY_H, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2, "2 Aim Right", OSD_KEY_J, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* DSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )	/* According to the test screen */
	/* Note that the coin settings don't match the setting shown on the test screen,
	   but instead what the game appears to used. This is either a bug in the game,
	   or I don't know what else. */
	PORT_DIPNAME( 0x06, 0x06, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "1 Credit/Coin" )
	PORT_DIPSETTING(    0x02, "2 Credits/Coin" )
	PORT_DIPSETTING(    0x04, "3 Credits/Coin" )
	PORT_DIPSETTING(    0x00, "4 Credits/Coin" )
	PORT_DIPNAME( 0x38, 0x38, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x38, "1 Credit/Coin" )
	PORT_DIPSETTING(    0x18, "2 Credits/Coin" )
	PORT_DIPSETTING(    0x28, "3 Credits/Coin" )
	PORT_DIPSETTING(    0x08, "4 Credits/Coin" )
	PORT_DIPSETTING(    0x30, "5 Credits/Coin" )
	PORT_DIPSETTING(    0x10, "6 Credits/Coin" )
	PORT_DIPSETTING(    0x20, "7 Credits/Coin" )
	PORT_DIPSETTING(    0x00, "8 Credits/Coin" )
	PORT_DIPNAME( 0x40, 0x40, "Memory Test", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Single" )
	PORT_DIPSETTING(    0x00, "Continous" )
	PORT_DIPNAME( 0x80, 0x80, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END


static struct DACinterface dac_interface =
{
	2, 			/* 2 channels on 1 chip */
	{ 50, 50 },
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz */
	{ 50 },
	{ 0 }
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS34010,
			40000000,	/* 40 Mhz */
			0,
            master_readmem,master_writemem,0,0,
            ignore_interrupt,0  /* Display Interrupts caused internally */
		},
		{
			CPU_TMS34010,
			40000000,	/* 40 Mhz */
			2,
            slave_readmem,slave_writemem,0,0,
            ignore_interrupt,0  /* Display Interrupts caused internally */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			3,
			sound_dac_readmem,sound_dac_writemem,0,0,
			ignore_interrupt,0	/* IRQ caused when sound command is written */
								/* NMIs are triggered by the YM2151 CPU */
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			2000000,	/* 2 Mhz */
			4,
            sound_ym2151_readmem,sound_ym2151_writemem,0,0,
            ignore_interrupt,0	/* IRQ caused when sound command is written */
								/* NMIs are triggered by a programmable timer */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,

	/* video hardware, the reason for 263 is that the VCOUNT register is
	   supposed to go from 0 to the value in VEND-1, which is 263 */
    256, 263, { 0, 255, 0, 238 },

	0,
	4096+32768,0,
    exterm_vh_convert_color_prom,

    VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_SUPPORTS_16BIT,
	0,
	exterm_vh_start,
	exterm_vh_stop,
	exterm_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		},
		{
			SOUND_YM2151,
			&ym2151_interface
		}
	}
};



/***************************************************************************

  High score save/load

***************************************************************************/

static int hiload (void)
{
	void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
	if (f)
	{
		osd_fread (f, eeprom, eeprom_size);
		osd_fclose (f);
	}

	return 1;
}


static void hisave (void)
{
	void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 1);
	if (f)
	{
		osd_fwrite (f, eeprom, eeprom_size);
		osd_fclose (f);
	}
}

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( exterm_rom )
	ROM_REGION(0x200000)     /* 2MB for 34010 code */
	ROM_LOAD_ODD(  "v101bg0",  0x000000, 0x10000, 0x8c8e72cf )
	ROM_LOAD_EVEN( "v101bg1",  0x000000, 0x10000, 0xcc2da0d8 )
	ROM_LOAD_ODD(  "v101bg2",  0x020000, 0x10000, 0x2dcb3653 )
	ROM_LOAD_EVEN( "v101bg3",  0x020000, 0x10000, 0x4aedbba0 )
	ROM_LOAD_ODD(  "v101bg4",  0x040000, 0x10000, 0x576922d4 )
	ROM_LOAD_EVEN( "v101bg5",  0x040000, 0x10000, 0xa54a4bc2 )
	ROM_LOAD_ODD(  "v101bg6",  0x060000, 0x10000, 0x7584a676 )
	ROM_LOAD_EVEN( "v101bg7",  0x060000, 0x10000, 0xa4f24ff6 )
	ROM_LOAD_ODD(  "v101bg8",  0x080000, 0x10000, 0xfda165d6 )
	ROM_LOAD_EVEN( "v101bg9",  0x080000, 0x10000, 0xe112a4c4 )
	ROM_LOAD_ODD(  "v101bg10", 0x0a0000, 0x10000, 0xf1a5cf54 )
	ROM_LOAD_EVEN( "v101bg11", 0x0a0000, 0x10000, 0x8677e754 )
	ROM_LOAD_ODD(  "v101fg0",  0x180000, 0x10000, 0x38230d7d )
	ROM_LOAD_EVEN( "v101fg1",  0x180000, 0x10000, 0x22a2bd61 )
	ROM_LOAD_ODD(  "v101fg2",  0x1a0000, 0x10000, 0x9420e718 )
	ROM_LOAD_EVEN( "v101fg3",  0x1a0000, 0x10000, 0x84992aa2 )
	ROM_LOAD_ODD(  "v101fg4",  0x1c0000, 0x10000, 0x38da606b )
	ROM_LOAD_EVEN( "v101fg5",  0x1c0000, 0x10000, 0x842de63a )
	ROM_LOAD_ODD(  "v101p0",   0x1e0000, 0x10000, 0x6c8ee79a )
	ROM_LOAD_EVEN( "v101p1",   0x1e0000, 0x10000, 0x557bfc84 )

	ROM_REGION_DISPOSE(0x1000)   /* temporary space for graphics (disposed after conversion) */

	ROM_REGION(0x1000)	 /* Slave CPU memory space. There are no ROMs mapped here */

	ROM_REGION(0x10000)	 /* 64k for DAC code */
	ROM_LOAD( "v101d1", 0x08000, 0x08000, 0x83268b7d )

	ROM_REGION(0x10000)	 /* 64k for YM2151 code */
	ROM_LOAD( "v101y1", 0x08000, 0x08000, 0xcbeaa837 )
ROM_END


void driver_init(void)
{
	memcpy (exterm_code_rom,Machine->memory_region[Machine->drv->cpu[0].memory_region],code_rom_size);

	TMS34010_set_stack_base(0, cpu_bankbase[1], TOBYTE(0x00c00000));
	TMS34010_set_stack_base(1, cpu_bankbase[4], TOBYTE(0xff800000));
}


struct GameDriver exterm_driver =
{
	__FILE__,
	0,
	"exterm",
	"Exterminator",
	"1989",
	"Gottlieb / Premier Technology",
	"Zsolt Vasvari\nAlex Pasadyn",
	0,
	&machine_driver,
	driver_init,

	exterm_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	exterm_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,

	hiload, hisave
};
