/***************************************************************************
	M.A.M.E. Neo Geo driver presented to you by the Shin Emu Keikaku team.

	The following people have all spent probably far too much time on this:

    AVDB
    Bryan McPhail
    Fuzz
    Ernesto Corvi
    Andrew Prime

    Plus rom-loaders and testing from Santeri Saarimaa, Rodimus Prime and
    probably others :)


	TODO :

            - Properly emulate the uP4990.

            - What does 0x3c0006-7 *REALLY* do?

=============================================================================

Points to note, known and proven information deleted from this map:

	0x3000001		Dipswitches
				bit 0 : Selftest
				bit 1 : Unknown (Unused ?) \ something to do with
				bit 2 : Unknown (Unused ?) / auto repeating keys ?
				bit 3 : \
				bit 4 :  | communication setting ?
				bit 5 : /
				bit 6 : free play
				bit 7 : stop mode ?

	0x320001		bit 0 : COIN 1
				bit 1 : COIN 2
				bit 2 : SERVICE
				bit 3 : UNKNOWN
				bit 4 : UNKNOWN
				bit 5 : UNKNOWN
				bit 6 : 4990 test pulse bit.
				bit 7 : 4990 data bit.

	0x380051		4990 control write register
				bit 0: C0
				bit 1: C1
				bit 2: C2
				bit 3-7: unused.

				0x02 = shift.
				0x00 = register hold.
				0x04 = ????.
				0x03 = time read (reset register).

	0x3c000c		IRQ acknowledge

	0x380011		Backup bank select

	0x3a0001		Enable display.
	0x3a0011		Disable display

	0x3a0003		Swap in Bios (0x80 bytes vector table of BIOS)
	0x3a0013		Swap in Rom  (0x80 bytes vector table of ROM bank)

	0x3a000d		lock backup ram
	0x3a001d		unlock backup ram

	0x3a000b		set game vector table (?)  mirror ?
	0x3a001b		set bios vector table (?)  mirror ?

	0x3a000c		Unknown	(ghost pilots)
	0x31001c		Unknown (ghost pilots)

	IO word read

	0x3c0002		return vidram word (pointed to by 0x3c0000)
	0x3c0006		?????.
	0x3c0008		shadow adress for 0x3c0000 (not confirmed).
	0x3c000a		shadow adress for 0x3c0002 (confirmed, see
							   Puzzle de Pon).
	IO word write

	0x3c0006		Unknown, set vblank counter (?)

	0x3c0008		shadow address for 0x3c0000	(not confirmed)
	0x3c000a		shadow address for 0x3c0002	(not confirmed)

	The Neo Geo contains an NEC 4990 Serial I/O calendar & clock.
	accesed through 0x320001, 0x380050, 0x280050 (shadow adress).
	A schematic for this device can be found on the NEC webpages.

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/pd4990a.h"

extern unsigned char *vidram;
extern unsigned char *neogeo_ram;
extern unsigned char *neogeo_sram;

void neogeo_sram_lock_w(int offset,int data);
void neogeo_sram_unlock_w(int offset,int data);
int neogeo_sram_r(int offset);
void neogeo_sram_w(int offset,int data);
int neogeo_sram_load(void);
void neogeo_sram_save(void);


/* from vidhrdw/neogeo.c */
void neogeo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int  neogeo_mvs_vh_start(void);
int  neogeo_mgd2_vh_start(void);
void neogeo_vh_stop(void);
void neogeo_paletteram_w(int offset,int data);
int  neogeo_paletteram_r(int offset);
void neogeo_setpalbank0(int offset, int data);
void neogeo_setpalbank1(int offset, int data);

void neo_board_fix(int offset, int data);
void neo_game_fix(int offset, int data);

void vidram_modulo_w(int offset, int data);
void vidram_data_w(int offset, int data);
void vidram_offset_w(int offset, int data);

int vidram_data_r(int offset);
int vidram_modulo_r(int offset);

/* debug, used to 'see' the locations mapped in ROM space */
/* with the debugger */
int mish_vid_r(int offset);
void mish_vid_w(int offset, int data);
/* end debug */

void neo_unknown1(int offset, int data);
void neo_unknown2(int offset, int data);
void neo_unknown3(int offset, int data);
void neo_unknown4(int offset, int data);


/* from machine/neogeo.c */
void neogeo_init_machine(void);
void neogeo_onetime_init_machine(void);


/******************************************************************************/


unsigned int neogeo_frame_counter;
unsigned int neogeo_frame_counter_speed=4;

/******************************************************************************/

static int irq2_enable;

static int neogeo_interrupt(void)
{
	static int fc=0;


	/* Add a timer tick to the pd4990a */
	addretrace();

	/* Animation counter, 1 once per frame is too fast, every 4 seems good */
        if  (fc>=neogeo_frame_counter_speed) {
                fc=0;
                neogeo_frame_counter++;
        }
        fc++;

	if (irq2_enable || osd_key_pressed(OSD_KEY_F1)) cpu_cause_interrupt(0,2);

	/* return a standard vblank interrupt */
	return(1);      /* vertical blank */
}

/* Move to calendar file later */
static void neo_pd4990a_control_w(int offset, int data)
{
	int a=0;
    a++;
}


static int pending_command;
static int result_code;

/* Calendar, coins + Z80 communication */
static int timer_r (int offset)
{
	int res;


	/* This is just for testing purposes */
	/* not real correct code */
	int coinflip = read_4990_testbit();

   //	 if (errorlog) fprintf(errorlog,"CPU %04x - Read timer\n",cpu_getpc());

	res = readinputport(4) ^ (coinflip << 6) ^ (coinflip << 7);

	if (Machine->sample_rate)
	{
		res |= result_code << 8;
		if (pending_command) res &= 0x7fff;
	}
	else
		res |= 0x0100;

	return res;
}

static void neo_z80_w(int offset, int data)
{
	soundlatch_w(0,(data>>8)&0xff);
	pending_command = 1;
	cpu_cause_interrupt(1,Z80_NMI_INT);
}



static int controller1_r (int offset) { return ( (readinputport(0) << 8) + readinputport(3) ); }
static int controller2_r (int offset) { return (readinputport(1) << 8); }
static int controller3_r (int offset) {	return (readinputport(2) << 8); }
static int controller4_r (int offset) { return readinputport(6); }

static void neo_bankswitch_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[0];
	int bankaddress;


	if (Machine->memory_region_length[0] <= 0x100000)
	{
if (errorlog) fprintf(errorlog,"warning: bankswitch to %02x but no banks available\n",data);
		return;
	}

	data = data&0x7;
	bankaddress = (data+1)*0x100000;
	if (bankaddress >= Machine->memory_region_length[0])
	{
if (errorlog) fprintf(errorlog,"warning: bankswitch to empty bank %02x\n",data);
		bankaddress = 0x100000;
	}

	cpu_setbank(4,&RAM[bankaddress]);
}



extern int neogeo_game_fix;

/* Temporary, Todo: Figure out how this really works! :) */
static int neo_control_r(int offset)
{
	switch(neogeo_game_fix)
	{
		case 0:
			return (neogeo_frame_counter) & 0x0007;                 /* Blazing Stars */
		case 1:
			if (cpu_getpc() == 0x1b04) return 0x8000; /* Fix for Voltage Fighter */
		case 2:
			return 0x2000;          /* real bout 2 */
		case 3:
			return 0x80;            /* sam sho3 */
		case 4:
			return 0xb801;      /* overtop */
		case 5:
			return 0x7000; /* Fix for KOF97 */
		case 6:
			return 0x8000; /* Money Idol Exchanger */
		case 8:
			return 0xffff; /* Ninja Command */
	}
	return(0x8000);              /* anything 0x8000 seems better than 0*/
}

/* this does much more than this, but I'm not sure exactly what */
void neo_control_w(int offset, int data)
{
if (errorlog) fprintf(errorlog,"PC %06x: 3c0006 = %02x\n",cpu_getpc(),data);
	/* I'm waving my hands in a big way here... */
	/* Games which definitely need IRQ2:
	   sengoku2
	   spinmast
	   ridhero
	   turfmast
	*/
	if ((data & 0xff) == 0xd0)	/* certainly wrong, but fixes spinmast & sengoku2 */
		irq2_enable = 1;
	else
		irq2_enable = 0;

	if((data & 0xf0ff) == 0)
	{
		/* Auto-Anim Speed Control ? */
		int speed = (data >> 8) & 0xf;
		if(speed!=0) neogeo_frame_counter_speed=speed;
	}
}

static void neo_irq2pos_w(int offset,int data)
{
	static int value;

if (errorlog) fprintf(errorlog,"PC %06x: %06x = %02x\n",cpu_getpc(),0x3c0008+offset,data);

	if (offset == 0)
		value = (value & 0x0000ffff) | (data << 16);
	else
		value = (value & 0xffff0000) | data;

if (errorlog && offset == 2)
	fprintf(errorlog,"should trigger IRQ2 at raster line %d, horiz offset %d?\n",(value / 0x180) - 0x24,value % 0x180);
}


/******************************************************************************/

static struct MemoryReadAddress neogeo_readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM },   /* Rom bank 1 */
	{ 0x100000, 0x10ffff, MRA_BANK1 }, /* Ram bank 1 */
	{ 0x200000, 0x2fffff, MRA_BANK4 }, /* Rom bank 2 */

	{ 0x300000, 0x300001, controller1_r },
	{ 0x300080, 0x300081, controller4_r }, /* Test switch in here */
	{ 0x320000, 0x320001, timer_r }, /* Coins, Calendar, Z80 communication */
	{ 0x340000, 0x340001, controller2_r },
	{ 0x380000, 0x380001, controller3_r },
	{ 0x3c0000, 0x3c0001, vidram_data_r },	/* Baseball Stars */
	{ 0x3c0002, 0x3c0003, vidram_data_r },
	{ 0x3c0004, 0x3c0005, vidram_modulo_r },

	{ 0x3c0006, 0x3c0007, neo_control_r },
	{ 0x3c000a, 0x3c000b, vidram_data_r },	/* Puzzle de Pon */

	{ 0x400000, 0x401fff, neogeo_paletteram_r },
	{ 0x600000, 0x61ffff, mish_vid_r },
//	{ 0x800000, 0x800fff, MRA_ }, /* memory card */
	{ 0xc00000, 0xc1ffff, MRA_BANK3 }, /* system bios rom */
	{ 0xd00000, 0xd0ffff, neogeo_sram_r }, /* 64k battery backed SRAM */
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress neogeo_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM },    /* ghost pilots writes to ROM */
	{ 0x100000, 0x10ffff, MWA_BANK1 },
/*	{ 0x200000, 0x200fff, whp copies ROM data here. Why? Is there RAM in the banked ROM space? */
/* trally writes to 200000-200003 as well */
/* both games write to 0000fe before writing to 200000. The two things could be related. */
//	{ 0x280050, 0x280051, neo_pd4990a_control_w },
	{ 0x2ffff0, 0x2fffff, neo_bankswitch_w },      /* NOTE THIS CHANGE TO END AT FF !!! */
	{ 0x300000, 0x300001, watchdog_reset_w },
	{ 0x320000, 0x320001, neo_z80_w },	/* Sound CPU */
	{ 0x380000, 0x380001, MWA_NOP },	/* Used by bios, unknown */
	{ 0x380030, 0x380031, MWA_NOP },    /* Used by bios, unknown */
	{ 0x380040, 0x380041, MWA_NOP },	/* Output leds */
	{ 0x380050, 0x380051, neo_pd4990a_control_w },
	{ 0x380060, 0x380063, MWA_NOP },	/* Used by bios, unknown */
	{ 0x3800e0, 0x3800e3, MWA_NOP },	/* Used by bios, unknown */

	{ 0x3a0000, 0x3a0001, MWA_NOP },
	{ 0x3a0010, 0x3a0011, MWA_NOP },
	{ 0x3a0002, 0x3a0003, MWA_NOP },
	{ 0x3a0012, 0x3a0013, MWA_NOP },
	{ 0x3a000a, 0x3a000b, neo_board_fix }, /* Select board FIX char rom */
	{ 0x3a001a, 0x3a001b, neo_game_fix },  /* Select game FIX char rom */
	{ 0x3a000c, 0x3a000d, neogeo_sram_lock_w },
	{ 0x3a001c, 0x3a001d, neogeo_sram_unlock_w },
	{ 0x3a000e, 0x3a000f, neogeo_setpalbank1 },
	{ 0x3a001e, 0x3a001f, neogeo_setpalbank0 },    /* Palette banking */

	{ 0x3c0000, 0x3c0001, vidram_offset_w },
	{ 0x3c0002, 0x3c0003, vidram_data_w },
	{ 0x3c0004, 0x3c0005, vidram_modulo_w },

	{ 0x3c0006, 0x3c0007, neo_control_w },  /* See level 2 of spinmasters, rowscroll data? */
	{ 0x3c0008, 0x3c000b, neo_irq2pos_w },  /* IRQ2 x/y position? */

	{ 0x3c000c, 0x3c000d, MWA_NOP },	/* IRQ acknowledge */
										/* 4 = IRQ 1 */
										/* 2 = IRQ 2 */
										/* 1 = IRQ 3 (does any game use this?) */
//	{ 0x3c000e, 0x3c000f, irq_trigger }, /* IRQ 2 Trigger??  See spinmast */

	{ 0x400000, 0x401fff, neogeo_paletteram_w },
	{ 0x600000, 0x61ffff, mish_vid_w },	/* Debug only, not part of real NeoGeo */
//	{ 0x800000, 0x800fff, MWA_ },	/* mem card */
	{ 0xd00000, 0xd0ffff, neogeo_sram_w, &neogeo_sram }, /* 64k battery backed SRAM */
	{ -1 }  /* end of table */
};

/******************************************************************************/

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK5 },
	{ 0xc000, 0xdfff, MRA_BANK6 },
	{ 0xe000, 0xefff, MRA_BANK7 },
	{ 0xf000, 0xf7ff, MRA_BANK8 },
	{ 0xf800, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0xf7ff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};


static int z80_port_r(int offset)
{
	static int bank[4];


#if 0
{
	char buf[80];
	sprintf(buf,"%05x %05x %05x %05x",bank[0],bank[1],bank[2],bank[3]);
	usrintf_showmessage(buf);
}
#endif

	switch (offset & 0xff)
	{
		case 0x00:
			pending_command = 0;
			return soundlatch_r(0);
			break;

		case 0x04:
			return YM2610_status_port_0_A_r(0);
			break;

		case 0x05:
			return YM2610_read_port_0_r(0);
			break;

		case 0x06:
			return YM2610_status_port_0_B_r(0);
			break;

		case 0x08:
			{
				unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
				bank[3] = 0x0800 * ((offset >> 8) & 0x3f);
				cpu_setbank(8,&RAM[bank[3]]);
				return 0;
				break;
			}

		case 0x09:
			{
				unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
				bank[2] = 0x1000 * ((offset >> 8) & 0x1f);
				cpu_setbank(7,&RAM[bank[2]]);
				return 0;
				break;
			}

		case 0x0a:
			{
				unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
				bank[1] = 0x2000 * ((offset >> 8) & 0x0f);
				cpu_setbank(6,&RAM[bank[1]]);
				return 0;
				break;
			}

		case 0x0b:
			{
				unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
				bank[0] = 0x4000 * ((offset >> 8) & 0x07);
				cpu_setbank(5,&RAM[bank[0]]);
				return 0;
				break;
			}

		default:
if (errorlog) fprintf(errorlog,"CPU #1 PC %04x: read unmapped port %02x\n",cpu_getpc(),offset&0xff);
			return 0;
			break;
	}
}

static void z80_port_w(int offset,int data)
{
	switch (offset & 0xff)
	{
		case 0x04:
			YM2610_control_port_0_A_w(0,data);
			break;

		case 0x05:
			YM2610_data_port_0_A_w(0,data);
			break;

		case 0x06:
			YM2610_control_port_0_B_w(0,data);
			break;

		case 0x07:
			YM2610_data_port_0_B_w(0,data);
			break;

		case 0x08:
			/* NMI enable / acknowledge? (the data written doesn't matter) */
			break;

		case 0x0c:
			result_code = data;
			break;

		case 0x18:
			/* NMI disable? (the data written doesn't matter) */
			break;

		default:
if (errorlog) fprintf(errorlog,"CPU #1 PC %04x: write %02x to unmapped port %02x\n",cpu_getpc(),data,offset&0xff);
			break;
	}
}

static struct IOReadPort neo_readio[] =
{
	{ 0x0000, 0xffff, z80_port_r },
	{ -1 }
};

static struct IOWritePort neo_writeio[] =
{
	{ 0x0000, 0xffff, z80_port_w },
	{ -1 }
};

/******************************************************************************/

INPUT_PORTS_START( neogeo_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )   /* Player 1 Start */
	PORT_BITX( 0x02, IP_ACTIVE_LOW, 0, "SELECT 1",OSD_KEY_6, IP_JOY_NONE, 0) /* Player 1 Select */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START2 )   /* Player 2 Start */
	PORT_BITX( 0x08, IP_ACTIVE_LOW, 0, "SELECT 2",OSD_KEY_7, IP_JOY_NONE, 0) /* Player 2 Select */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Bits 10 & 20 have significance.. don't know what yet */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_DIPNAME( 0x01, 0x01, "Test Switch", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x04, 0x04, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x38, 0x38, "COMM Setting", IP_KEY_NONE )
	PORT_DIPSETTING(    0x38, "Off" )
	PORT_DIPSETTING(    0x30, "1" )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x40, 0x40, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 ) /* Service */

	/* Fake  IN 5 */
	PORT_START
	PORT_DIPNAME( 0x03, 0x02,"Territory", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00,"Japan" )
	PORT_DIPSETTING(    0x01,"USA" )
	PORT_DIPSETTING(    0x02,"Europe" )
	PORT_DIPNAME( 0x04, 0x04,"Machine mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00,"Home" )
	PORT_DIPSETTING(    0x04,"Arcade" )

	PORT_START      /* Test switch */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )  /* This bit is used.. */
	PORT_BITX( 0x80, IP_ACTIVE_LOW, 0, "Test Switch", OSD_KEY_F2, IP_JOY_NONE, 0)
INPUT_PORTS_END

/******************************************************************************/

/* character layout (same for all games) */
static struct GfxLayout charlayout =	/* All games */
{
	8,8,            /* 8 x 8 chars */
	4096,           /* 4096 in total */
	4,              /* 4 bits per pixel */
	{ 0, 1, 2, 3 },    /* planes are packed in a nibble */
	{ 33*4, 32*4, 49*4, 48*4, 1*4, 0*4, 17*4, 16*4 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	32*8    /* 32 bytes per char */
};

/* Placeholder and also reminder of how this graphic format is put together */
static struct GfxLayout dummy_mgd2_tilelayout =
{
	16,16,  /* 16*16 sprites */
	20,  /* sprites */
	4,      /* 4 bits per pixel */
	{ /*0x10000*32*8*3*/3, /*0x10000*32*8*2*/2, /*0x10000*32*8*/1, 0 },
	{ 16*8+7, 16*8+6, 16*8+5, 16*8+4, 16*8+3, 16*8+2, 16*8+1, 16*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

/* Placeholder and also reminder of how this graphic format is put together */
static struct GfxLayout dummy_mvs_tilelayout =
{
	16,16,   /* 16*16 sprites */
	20,
	4,
	{ /*0x400000*8 +*/ 8, /*0x400000*8 + */0, 8, 0 },     /* plane offset */

	{ 32*8+7, 32*8+6, 32*8+5, 32*8+4, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
	  7, 6, 5, 4, 3, 2, 1, 0 },

	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },

	64*8    /* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo neogeo_mvs_gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout, 0, 16 },
	{ 1, 0x020000, &charlayout, 0, 16 },
	{ 1, 0x000000, &dummy_mvs_tilelayout, 0, 256 },  /* Placeholder */
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo neogeo_mgd2_gfxdecodeinfo[] =
{
	{ 1, 0x000000, &charlayout, 0, 16 },
	{ 1, 0x020000, &charlayout, 0, 16 },
	{ 1, 0x000000, &dummy_mvs_tilelayout, 0, 256 },  /* Placeholder */
	{ -1 } /* end of array */
};

/******************************************************************************/

static void neogeo_sound_irq( void )
{
	cpu_cause_interrupt(1,0xff);
}

struct YM2610interface neogeo_ym2610_interface =
{
	1,
	8000000,
	{ YM2203_VOL(60,0x301e) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ neogeo_sound_irq },
	{ 7 },
	{ 6 }
};

/******************************************************************************/

static struct MachineDriver neogeo_mgd2_machine_driver =
{
	{
		{
			CPU_M68000,
			12000000,
			0,
			neogeo_readmem,neogeo_writemem,0,0,
			neogeo_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU | CPU_16BIT_PORT,
			4000000,
			5,
			sound_readmem,sound_writemem,neo_readio,neo_writeio,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	neogeo_init_machine,
  	40*8, 28*8, { 1*8, 39*8-1, 0*8, 28*8-1 },
  //	40*8, 60*8, { 1*8, 39*8-1, 0*8, 59*8-1 },

	neogeo_mgd2_gfxdecodeinfo,
	4096,4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	neogeo_mgd2_vh_start,
	neogeo_vh_stop,
	neogeo_vh_screenrefresh,

	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&neogeo_ym2610_interface,
		},
	}
};

static struct MachineDriver neogeo_mvs_machine_driver =
{
	{
		{
			CPU_M68000,
			12000000,
			0,
			neogeo_readmem,neogeo_writemem,0,0,
			neogeo_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU | CPU_16BIT_PORT,
			4000000,
			5,
			sound_readmem,sound_writemem,neo_readio,neo_writeio,
			ignore_interrupt,0
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
	neogeo_init_machine,
	40*8, 28*8, { 1*8, 39*8-1, 0*8, 28*8-1 },
//40*8, 56*8, { 1*8, 39*8-1, 0*8, 56*8-1 },
	neogeo_mvs_gfxdecodeinfo,
	4096,4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	neogeo_mvs_vh_start,
	neogeo_vh_stop,
	neogeo_vh_screenrefresh,

	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_YM2610,
			&neogeo_ym2610_interface,
		},
	}
};

/******************************************************************************/

#define NEO_BIOS_SOUND_128K(name,sum) \
	ROM_REGION(0x20000) \
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 ) \
	ROM_REGION(0x40000) \
	ROM_LOAD( name,         0x00000, 0x20000, sum ) \
	ROM_LOAD( "ng-sm1.rom", 0x20000, 0x20000, 0x97cf998b )

#define NEO_BIOS_SOUND_64K(name,sum) \
	ROM_REGION(0x20000) \
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 ) \
	ROM_REGION(0x40000) \
	ROM_LOAD( name,         0x00000, 0x10000, sum ) \
	ROM_LOAD( "ng-sm1.rom", 0x20000, 0x20000, 0x97cf998b )

#define NEO_SFIX_128K(name,sum) \
	ROM_REGION_DISPOSE(0x40000) \
    ROM_LOAD( name,           0x000000, 0x20000, sum ) \
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )

#define NEO_SFIX_64K(name,sum) \
	ROM_REGION_DISPOSE(0x40000) \
    ROM_LOAD( name,           0x000000, 0x10000, sum ) \
    ROM_LOAD( "ng-sfix.rom",  0x020000, 0x20000, 0x354029fc )


/* MGD2 roms */
ROM_START( joyjoy_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n022001a.038", 0x000000, 0x040000, 0xea512c9f )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n022001a.378", 0x6956d778 )

    ROM_REGION(0x80000)
    ROM_LOAD( "n022001a.538", 0x000000, 0x80000, 0xcb27be65 ) /* Plane 0,1 */

	ROM_REGION(0x80000)
    ROM_LOAD( "n022001a.638", 0x000000, 0x80000, 0x7e134979 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_64K( "n022001a.478", 0x058683ec )

	ROM_REGION_OPTIONAL(0x080000) /* sound samples */
	ROM_LOAD( "n022001a.178", 0x000000, 0x080000, 0x66c1e5c4 )

	ROM_REGION_OPTIONAL(0x080000) /* sound samples */
	ROM_LOAD( "n022001a.278", 0x000000, 0x080000, 0x8ed20a86 )
ROM_END

ROM_START( ridhero_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0xdabfac95 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_64K( "n046001a.378", 0x197d1a28 )

	ROM_REGION(0x100000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0x24096241 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x080000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0x7026a3a2 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )

	ROM_REGION(0x100000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0xdf6a5b00 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x080000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x15220d51 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )

	NEO_BIOS_SOUND_64K( "n046001a.478", 0xf7196558 )

	ROM_REGION_OPTIONAL(0x100000) /* sound samples */
	ROM_LOAD( "n046001a.178", 0x000000, 0x080000, 0xcdf74a42 )
	ROM_LOAD( "n046001a.17c", 0x080000, 0x080000, 0xe2fd2371 )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "n046001a.278", 0x000000, 0x080000, 0x94092bce )
	ROM_LOAD( "n046001a.27c", 0x080000, 0x080000, 0x4e2cd7c3 )
	ROM_LOAD( "n046001b.278", 0x100000, 0x080000, 0x069c71ed )
	ROM_LOAD( "n046001b.27c", 0x180000, 0x080000, 0x89fbb825 )
ROM_END

ROM_START( ttbb_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0xefb016a2 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n046001a.378", 0x7015b8fc )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0x746bf48a ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0x57bdcec0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0x0b054a38 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0x5c123d9c ) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x2f4bb615 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0xb2a86447 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	NEO_BIOS_SOUND_128K( "n046001a.4f8", 0xbf755068 )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "n046001a.1f8", 0x000000, 0x080000, 0x33e7886e )
	ROM_LOAD( "n046001a.1fc", 0x080000, 0x080000, 0xe7ca3882 )
	ROM_LOAD( "n046001b.1f8", 0x100000, 0x080000, 0x3cf9a433 )
	ROM_LOAD( "n046001b.1fc", 0x180000, 0x080000, 0x88b10192 )
ROM_END

ROM_START( lresort_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x5f0a5a4b )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n046001a.378", 0x5cef5cc6 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0x9f7995a9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0xe122b155 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0xe7138cb9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0x68c70bac ) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0xf18a9b02 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0x08178e27 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	NEO_BIOS_SOUND_128K( "n046001a.4f8", 0x3d40a1c6 )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "n046001a.1f8", 0x000000, 0x080000, 0x0722da38 )
	ROM_LOAD( "n046001a.1fc", 0x080000, 0x080000, 0x670ce3ec )
	ROM_LOAD( "n046001b.1f8", 0x100000, 0x080000, 0x2e39462b )
	ROM_LOAD( "n046001b.1fc", 0x180000, 0x080000, 0x7944754f )
ROM_END

ROM_START( ararmy_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x99c7b4fc )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n046001a.378", 0xac0daa1b )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0xe3afaf17 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0x17098f54 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0x13cbb7c5 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0x3a098b3b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x8e3b2b88 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0xb6f5fc62 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	NEO_BIOS_SOUND_128K( "n046001a.4f8", 0x98edc671 )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "n046001a.1f8", 0x000000, 0x080000, 0xdaff9896 )
	ROM_LOAD( "n046001a.1fc", 0x080000, 0x080000, 0x8781b1bc )
	ROM_LOAD( "n046001b.1f8", 0x100000, 0x080000, 0xb69c1da5 )
	ROM_LOAD( "n046001b.1fc", 0x180000, 0x080000, 0x2c929c17 )
ROM_END

ROM_START( fbfrenzy_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0xc9fc879c )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n046001a.378", 0x8472ed44 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0xcd377680 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0x2f6d09c2 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0x9abe41c8 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0x8b76358f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x77e45dd2 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0x336540a8 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	NEO_BIOS_SOUND_128K( "n046001a.4f8", 0x079a203c )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "n046001a.1f8", 0x000000, 0x080000, 0xd295da77 )
	ROM_LOAD( "n046001a.1fc", 0x080000, 0x080000, 0x249b7f52 )
	ROM_LOAD( "n046001b.1f8", 0x100000, 0x080000, 0xe438fb9d )
	ROM_LOAD( "n046001b.1fc", 0x180000, 0x080000, 0x4f9bc109 )
ROM_END

ROM_START( alpham2_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n082001a.038", 0x000000, 0x040000, 0x4400b34c )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n082001a.03c", 0x080000, 0x040000, 0xb0366875 )
	ROM_CONTINUE (                 0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n082001a.378", 0x85ec9acf )

	ROM_REGION(0x180000)
    ROM_LOAD( "n082001a.538", 0x000000, 0x40000, 0xc516b09e ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n082001a.53c", 0x040000, 0x40000, 0xd9a0ff6c ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n082001b.538", 0x080000, 0x40000, 0x3a7fe4fd ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n082001a.638", 0x000000, 0x40000, 0x6b674581 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n082001a.63c", 0x040000, 0x40000, 0x4ff21008 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n082001b.638", 0x080000, 0x40000, 0xd0e8eef3 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	NEO_BIOS_SOUND_64K( "n082001a.478", 0x0449acf5 )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "n082001a.178", 0x000000, 0x080000, 0x7ec0e76d )
	ROM_LOAD( "n082001a.17c", 0x080000, 0x080000, 0x7a796ead )
	ROM_LOAD( "n082001b.178", 0x100000, 0x080000, 0x70bc86a5 )
	ROM_LOAD( "n082001b.17c", 0x180000, 0x080000, 0x29963a92 )

	ROM_REGION_OPTIONAL(0x400000) /* sound samples */
	ROM_LOAD( "n082001a.278", 0x000000, 0x080000, 0x45f5e914 )
	ROM_LOAD( "n082001a.27c", 0x080000, 0x080000, 0x07524063 )
	ROM_LOAD( "n082001b.278", 0x100000, 0x080000, 0xc3178623 )
	ROM_LOAD( "n082001b.27c", 0x180000, 0x080000, 0x65bca6b7 )
	ROM_LOAD( "n082001c.278", 0x200000, 0x080000, 0x27cd2250 )
	ROM_LOAD( "n082001c.27c", 0x280000, 0x080000, 0x43025293 )
	ROM_LOAD( "n082001d.278", 0x300000, 0x080000, 0xae0a679a )
	ROM_LOAD( "n082001d.27c", 0x380000, 0x080000, 0x6a2e400d )
ROM_END

ROM_START( trally_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x400bed38 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n046001a.03c", 0x080000, 0x040000, 0x77196e9a )
	ROM_CONTINUE (                 0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n046001a.378", 0xfff62ae3 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0x4d002ecb ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0xb0be56db ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0x2f213750 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0x6b2f79de ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x091f38b4 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0x268be38b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )

	NEO_BIOS_SOUND_64K( "n046001a.4f8", 0x308c4a8d )

	ROM_REGION_OPTIONAL(0x180000) /* sound samples */
	ROM_LOAD( "n046001a.1f8", 0x000000, 0x080000, 0x1c93fb89 )
	ROM_LOAD( "n046001a.1fc", 0x080000, 0x080000, 0x39f18253 )
	ROM_LOAD( "n046001b.1f8", 0x100000, 0x080000, 0xddd8d1e6 )
ROM_END

ROM_START( eightman_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0xe23e2631 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n046001a.378", 0xa402202b )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0xc916c9bf ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0x4b057b13 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0x12d53af0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0x7114bce3 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x51da9a34 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0x43cf58f9 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	NEO_BIOS_SOUND_128K( "n046001a.4f8", 0x68b6e0ef )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "n046001a.1f8", 0x000000, 0x080000, 0x0a2299b4 )
	ROM_LOAD( "n046001a.1fc", 0x080000, 0x080000, 0xb695e254 )
	ROM_LOAD( "n046001b.1f8", 0x100000, 0x080000, 0x6c3c3fec )
	ROM_LOAD( "n046001b.1fc", 0x180000, 0x080000, 0x375764df )
ROM_END

ROM_START( ncombat_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x89660a31 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n046001a.378", 0xd49afee8 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0x0147a4b5 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0x4df6123a ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0x19441c78 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0xe3d367f8 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x1c1f6101 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0xf417f9ac ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	NEO_BIOS_SOUND_64K( "n046001a.478", 0x83821d6e )

	ROM_REGION_OPTIONAL(0x180000) /* sound samples */
	ROM_LOAD( "n046001a.178", 0x000000, 0x080000, 0xcf32a59c )
	ROM_LOAD( "n046001a.17c", 0x080000, 0x080000, 0x7b3588b7 )
	ROM_LOAD( "n046001b.178", 0x100000, 0x080000, 0x505a01b5 )

	ROM_REGION_OPTIONAL(0x080000) /* sound samples */
	ROM_LOAD( "n046001a.278", 0x000000, 0x080000, 0x365f9011 )
ROM_END

/* TODO: verify this dump. I think it's a combination of 2 reads */
ROM_START( bstars_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n042001a.038", 0x000000, 0x040000, 0x68ce5b06)
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n138001a.378", 0x1a7fd0c6 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n138001a.538", 0x000000, 0x40000, 0xc55a7229) /* Plane 0,1 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n138001a.53c", 0x040000, 0x40000, 0xa0074bd9) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n138001b.538", 0x080000, 0x40000, 0xd57767e6) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	ROM_REGION(0x180000)
    ROM_LOAD( "n138001a.638", 0x000000, 0x40000, 0xcd3eeb2d) /* Plane 2,3 */
	ROM_CONTINUE(             0x0c0000, 0x40000 )
    ROM_LOAD( "n138001a.63c", 0x040000, 0x40000, 0xd651fecf) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n138001b.638", 0x080000, 0x40000, 0x5d0a8692) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )

	NEO_BIOS_SOUND_64K( "n138001a.478", 0x79a8f4c2 )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "n138001a.178", 0x000000, 0x080000, 0xb7b925bd )
	ROM_LOAD( "n138001a.17c", 0x080000, 0x080000, 0x329f26fc )
	ROM_LOAD( "n138001b.178", 0x100000, 0x080000, 0x0c39f3c8 )
	ROM_LOAD( "n138001b.17c", 0x180000, 0x080000, 0xc7e11c38 )

	ROM_REGION_OPTIONAL(0x080000) /* sound samples */
	ROM_LOAD( "n042001a.278", 0x000000, 0x080000, 0x04a733d1 )
ROM_END

ROM_START( gpilots_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n058001a.038", 0x000000, 0x040000, 0xfc5837c0 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n058001a.03c", 0x080000, 0x040000, 0x47a641da )
	ROM_CONTINUE (                 0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n058001a.378", 0xa6d83d53 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.538", 0x000000, 0x40000, 0x92b8ee5f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.53c", 0x040000, 0x40000, 0x8c8e42e9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.538", 0x080000, 0x40000, 0x4f12268b ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.53c", 0x0c0000, 0x40000, 0x7c3c9c7e ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.638", 0x000000, 0x40000, 0x05733639 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.63c", 0x040000, 0x40000, 0x347fef2b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.638", 0x080000, 0x40000, 0x2c586176 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.63c", 0x0c0000, 0x40000, 0x9b2eee8b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	NEO_BIOS_SOUND_64K( "n058001a.478", 0xfc05fb8b )

	ROM_REGION_OPTIONAL(0x180000) /* sound samples */
	ROM_LOAD( "n058001a.178", 0x000000, 0x080000, 0x8cc44140 )
	ROM_LOAD( "n058001a.17c", 0x080000, 0x080000, 0x415c61cd )
	ROM_LOAD( "n058001b.178", 0x100000, 0x080000, 0x4a9e6f03 )

	ROM_REGION_OPTIONAL(0x080000) /* sound samples */
	ROM_LOAD( "n058001a.278", 0x000000, 0x080000, 0x7abf113d )
ROM_END

ROM_START( crsword_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n046001a.038", 0x000000, 0x040000, 0x42c78fe1 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n046001a.378", 0x74651f27 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n046001a.538", 0x000000, 0x40000, 0x4b373de7 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001a.53c", 0x040000, 0x40000, 0xcddf6d69 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n046001b.538", 0x080000, 0x40000, 0x61d25cb3 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n046001b.53c", 0x0c0000, 0x40000, 0x00bc3d84 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n046001a.638", 0x000000, 0x40000, 0xe05f5f33 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n046001a.63c", 0x040000, 0x40000, 0x91ab11a4 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n046001b.638", 0x080000, 0x40000, 0x01357559 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n046001b.63c", 0x0c0000, 0x40000, 0x28c6d19a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	NEO_BIOS_SOUND_64K( "n046001a.4f8", 0x66633e5e )

	ROM_REGION_OPTIONAL(0x100000) /* sound samples */
	ROM_LOAD( "n046001a.1f8", 0x000000, 0x080000, 0x525df5c8 )
	ROM_LOAD( "n046001a.1fc", 0x080000, 0x080000, 0xa11ecaf4 )
ROM_END

ROM_START( kingofm_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n058001a.038", 0x000000, 0x040000, 0xd239c184 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n058001a.03c", 0x080000, 0x040000, 0x7291a388 )
	ROM_CONTINUE (                 0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n058001a.378", 0x1a2eeeb3 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.538", 0x000000, 0x40000, 0x493db90e ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.53c", 0x040000, 0x40000, 0x0d211945 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.538", 0x080000, 0x40000, 0xcabb7b58 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.53c", 0x0c0000, 0x40000, 0xc7c20718 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.638", 0x000000, 0x40000, 0x8bc1c3a0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.63c", 0x040000, 0x40000, 0xcc793bbf ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.638", 0x080000, 0x40000, 0xfde45b59 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.63c", 0x0c0000, 0x40000, 0xb89b4201 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	NEO_BIOS_SOUND_128K( "n058001a.4f8", 0x40797389 )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "n058001a.1f8", 0x000000, 0x080000, 0xc3df83ba )
	ROM_LOAD( "n058001a.1fc", 0x080000, 0x080000, 0x22aa6096 )
	ROM_LOAD( "n058001b.1f8", 0x100000, 0x080000, 0xdf9a4854 )
	ROM_LOAD( "n058001b.1fc", 0x180000, 0x080000, 0x71f53a38 )
ROM_END

ROM_START( burningf_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n054001a.038", 0x000000, 0x040000, 0x188c5e11 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n054001a.378", 0x6799ea0d )

	ROM_REGION(0x200000)
    ROM_LOAD( "n054001a.538", 0x000000, 0x40000, 0x4ddc137b ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n054001a.53c", 0x040000, 0x40000, 0x896d8545 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n054001b.538", 0x080000, 0x40000, 0x2b2cb196 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n054001b.53c", 0x0c0000, 0x40000, 0x0f49caa9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n054001a.638", 0x000000, 0x40000, 0x7d7d87dc ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n054001a.63c", 0x040000, 0x40000, 0x39fe5307 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n054001b.638", 0x080000, 0x40000, 0x03aa8a36 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n054001b.63c", 0x0c0000, 0x40000, 0xf759626e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	NEO_BIOS_SOUND_128K( "n054001a.4f8", 0x58d10c2b )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "n054001a.1f8", 0x000000, 0x080000, 0xb55b9670 )
	ROM_LOAD( "n054001a.1fc", 0x080000, 0x080000, 0xa0bcf260 )
	ROM_LOAD( "n054001b.1f8", 0x100000, 0x080000, 0x270f4707 )
	ROM_LOAD( "n054001b.1fc", 0x180000, 0x080000, 0x924e3d69 )
ROM_END

ROM_START( mutnat_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n054001a.038", 0x000000, 0x040000, 0x30cbd46b )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n054001a.378", 0x99419733 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n054001a.538", 0x000000, 0x40000, 0x83d59ccf ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n054001a.53c", 0x040000, 0x40000, 0xb2f1409d ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n054001b.538", 0x080000, 0x40000, 0xeaa2801a ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n054001b.53c", 0x0c0000, 0x40000, 0xc718b731 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n054001a.638", 0x000000, 0x40000, 0x9e115a04 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n054001a.63c", 0x040000, 0x40000, 0x1bb648c1 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n054001b.638", 0x080000, 0x40000, 0x32bf4a2d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n054001b.63c", 0x0c0000, 0x40000, 0x7d120067 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	NEO_BIOS_SOUND_128K( "n054001a.4f8", 0x2db6862d )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "n054001a.1f8", 0x000000, 0x080000, 0x8db2effe )
	ROM_LOAD( "n054001a.1fc", 0x080000, 0x080000, 0xa49fe238 )
	ROM_LOAD( "n054001b.1f8", 0x100000, 0x080000, 0x2ba17cb7 )
	ROM_LOAD( "n054001b.1fc", 0x180000, 0x080000, 0x42419a29 )
ROM_END

ROM_START( lbowling_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n050001a.038", 0x000000, 0x040000, 0x380e358d )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n050001a.378", 0x5fcdc0ed )

	ROM_REGION(0x200000)
    ROM_LOAD( "n050001a.538", 0x000000, 0x40000, 0x17df7955 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n050001a.53c", 0x040000, 0x40000, 0x67bf2d89 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n050001b.538", 0x080000, 0x40000, 0x00d36f90 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n050001b.53c", 0x0c0000, 0x40000, 0x4e971be9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n050001a.638", 0x000000, 0x40000, 0x84fd2c90 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n050001a.63c", 0x040000, 0x40000, 0xcb4fbeb0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n050001b.638", 0x080000, 0x40000, 0xc2ddf431 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n050001b.63c", 0x0c0000, 0x40000, 0xe67f8c81 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	NEO_BIOS_SOUND_64K( "n050001a.478", 0x535ec016 )

	ROM_REGION_OPTIONAL(0x100000) /* sound samples */
	ROM_LOAD( "n050001a.178", 0x000000, 0x080000, 0x0fb74872 )
	ROM_LOAD( "n050001a.17c", 0x080000, 0x080000, 0x029faa57 )

	ROM_REGION_OPTIONAL(0x080000) /* sound samples */
	ROM_LOAD( "n050001a.278", 0x000000, 0x080000, 0x2efd5ada )
ROM_END

ROM_START( fatfury1_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n058001a.038", 0x000000, 0x040000, 0x47e51379 )
	ROM_CONTINUE (                0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n058001a.03c", 0x080000, 0x040000, 0x19d36805 )
	ROM_CONTINUE (                0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n058001a.378", 0x3c3bdf8c )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.538", 0x000000, 0x40000, 0x9aaa6d73 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.53c", 0x040000, 0x40000, 0xa986f4a9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.538", 0x080000, 0x40000, 0x7aefe57d ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.53c", 0x0c0000, 0x40000, 0xe3057c96 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.638", 0x000000, 0x40000, 0x9cae3703 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.63c", 0x040000, 0x40000, 0x308b619f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.638", 0x080000, 0x40000, 0xb39a0cde ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.63c", 0x0c0000, 0x40000, 0x737bc030 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	NEO_BIOS_SOUND_128K( "n058001a.4f8", 0xa8603979 )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "n058001a.1f8", 0x000000, 0x080000, 0x86fabf00 )
	ROM_LOAD( "n058001a.1fc", 0x080000, 0x080000, 0xead1467b )
	ROM_LOAD( "n058001b.1f8", 0x100000, 0x080000, 0xfc3bd6f7 )
	ROM_LOAD( "n058001b.1fc", 0x180000, 0x080000, 0xd312f6c0 )
ROM_END

ROM_START( ncommand_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n054001a.038", 0x000000, 0x040000, 0xfdaaca42)
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n054001a.03c", 0x080000, 0x040000, 0xb34e91fe)
	ROM_CONTINUE (                 0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n054001a.378", 0xdb8f9c8e)

	ROM_REGION(0x200000)
    ROM_LOAD( "n054001a.538", 0x000000, 0x40000, 0x73acaa79) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n054001a.53c", 0x040000, 0x40000, 0xad56623d) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n054001b.538", 0x080000, 0x40000, 0xc8d763cd) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n054001b.53c", 0x0c0000, 0x40000, 0x63829529) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n054001a.638", 0x000000, 0x40000, 0x7b24359f) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n054001a.63c", 0x040000, 0x40000, 0x0913a784) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n054001b.638", 0x080000, 0x40000, 0x574612ec) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n054001b.63c", 0x0c0000, 0x40000, 0x990d302a) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	NEO_BIOS_SOUND_64K( "n054001a.4f8", 0x26e93026 )

	ROM_REGION_OPTIONAL(0x180000) /* sound samples */
	ROM_LOAD( "n054001a.1f8", 0x000000, 0x080000, 0x222e71c8 )
	ROM_LOAD( "n054001a.1fc", 0x080000, 0x080000, 0x12acd064 )
	ROM_LOAD( "n054001b.1f8", 0x100000, 0x080000, 0x80b8a984 )
ROM_END

ROM_START( sengoku_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n058001a.038", 0x000000, 0x040000, 0x4483bae1 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n058001a.03c", 0x080000, 0x040000, 0xd0d55b2a )
	ROM_CONTINUE (                 0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n058001a.378", 0xb246204d )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.538", 0x000000, 0x40000, 0xe834b925 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.53c", 0x040000, 0x40000, 0x66be6d46 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.538", 0x080000, 0x40000, 0x443c552c ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.53c", 0x0c0000, 0x40000, 0xecb41adc ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n058001a.638", 0x000000, 0x40000, 0x96de5eb9 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n058001a.63c", 0x040000, 0x40000, 0x25f5fd7b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n058001b.638", 0x080000, 0x40000, 0xafbd5b0b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n058001b.63c", 0x0c0000, 0x40000, 0x78b25278 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	NEO_BIOS_SOUND_128K( "n058001a.4f8", 0xe7bc4a94 )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "n058001a.1f8", 0x000000, 0x080000, 0x205258a7 )
	ROM_LOAD( "n058001a.1fc", 0x080000, 0x080000, 0x6fbe52c8 )
	ROM_LOAD( "n058001b.1f8", 0x100000, 0x080000, 0x6421bdf3 )
	ROM_LOAD( "n058001b.1fc", 0x180000, 0x080000, 0x1f9578fb )
ROM_END

ROM_START( superspy_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n138001a.038", 0x000000, 0x040000, 0x2e949e32 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n138001a.03c", 0x080000, 0x040000, 0x54443d72 )
	ROM_CONTINUE (                 0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n138001a.378", 0xec5fdb96 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n138001a.538", 0x000000, 0x40000, 0x239f22c4 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n138001a.53c", 0x040000, 0x40000, 0xce80c326 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n138001b.538", 0x080000, 0x40000, 0x1edcf268 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n138001b.53c", 0x0c0000, 0x40000, 0xa41602a0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n138001a.638", 0x000000, 0x40000, 0x5f2e5184 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n138001a.63c", 0x040000, 0x40000, 0x79b3e0b1 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n138001b.638", 0x080000, 0x40000, 0xb2afe822 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n138001b.63c", 0x0c0000, 0x40000, 0xd425f967 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	NEO_BIOS_SOUND_64K( "n138001a.478", 0x64455806 )

    ROM_REGION_OPTIONAL(0x180000) /* sound samples */
	ROM_LOAD( "n138001a.178", 0x000000, 0x080000, 0xb993bc83 )
	ROM_LOAD( "n138001a.17c", 0x080000, 0x080000, 0xd7a059b1 )
	ROM_LOAD( "n138001b.178", 0x100000, 0x080000, 0x9f513d5a )
//	ROM_LOAD( "n138001b.17c", 0x180000, 0x080000, 0x9f513d5a ) // Check if this is needed, maybe not!

	ROM_REGION_OPTIONAL(0x080000) /* sound samples */
	ROM_LOAD( "n138001a.278", 0x000000, 0x080000, 0x426cd040 )
ROM_END

ROM_START( androdun_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n138001a.038", 0x000000, 0x040000, 0x4639b419 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n138001a.03c", 0x080000, 0x040000, 0x11beb098 )
	ROM_CONTINUE (                 0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n138001a.378", 0x6349de5d )

	ROM_REGION(0x200000)
    ROM_LOAD( "n138001a.538", 0x000000, 0x40000, 0xca08e432 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n138001a.53c", 0x040000, 0x40000, 0xfcbcb305 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n138001b.538", 0x080000, 0x40000, 0x806ab937 ) /* Plane 0,1 - needed? */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n138001b.53c", 0x0c0000, 0x40000, 0xe7e1a2be ) /* Plane 0,1 - needed? */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	ROM_REGION(0x200000)
    ROM_LOAD( "n138001a.638", 0x000000, 0x40000, 0x7a0deb9e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x100000, 0x40000 )
    ROM_LOAD( "n138001a.63c", 0x040000, 0x40000, 0xb1c640f5 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x140000, 0x40000 )
    ROM_LOAD( "n138001b.638", 0x080000, 0x40000, 0x33bee10f ) /* Plane 2,3 - needed? */
	ROM_CONTINUE(             0x180000, 0x40000 )
    ROM_LOAD( "n138001b.63c", 0x0c0000, 0x40000, 0x70f0d263 ) /* Plane 2,3 - needed? */
	ROM_CONTINUE(             0x1c0000, 0x40000 )

	NEO_BIOS_SOUND_128K( "n138001a.4f8", 0x1a009f8c )

	ROM_REGION_OPTIONAL(0x100000) /* sound samples */
	ROM_LOAD( "n138001a.1f8", 0x000000, 0x080000, 0x577c85b3 )
	ROM_LOAD( "n138001a.1fc", 0x080000, 0x080000, 0xe14551c4 )
ROM_END

ROM_START( countb_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n106001a.038", 0x000000, 0x040000, 0x10dbe66a)
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n106001a.03c", 0x080000, 0x040000, 0x6d5bfb61)
	ROM_CONTINUE (                 0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n106001a.378", 0xc362d484)

	ROM_REGION(0x400000)
    ROM_LOAD( "n106001a.538", 0x000000, 0x40000, 0xbe0d2fe0) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n106001a.53c", 0x040000, 0x40000, 0xfdb4df65) /* Plane 0,1 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n106001b.538", 0x080000, 0x40000, 0x714c2c01) /* Plane 0,1 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n106001b.53c", 0x0c0000, 0x40000, 0xc57ce8b0) /* Plane 0,1 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

    ROM_LOAD( "n106001c.538", 0x100000, 0x40000, 0x2e7e59df) /* Plane 0,1 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n106001c.53c", 0x140000, 0x40000, 0xa93185ce) /* Plane 0,1 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n106001d.538", 0x180000, 0x40000, 0x410938c5) /* Plane 0,1 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n106001d.53c", 0x1c0000, 0x40000, 0x50d66909) /* Plane 0,1 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	ROM_REGION(0x400000)
    ROM_LOAD( "n106001a.638", 0x000000, 0x40000, 0xf56dafa5) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n106001a.63c", 0x040000, 0x40000, 0xf2f68b2a) /* Plane 2,3 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n106001b.638", 0x080000, 0x40000, 0xb15a7f25) /* Plane 2,3 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n106001b.63c", 0x0c0000, 0x40000, 0x25f00cd3) /* Plane 2,3 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

    ROM_LOAD( "n106001c.638", 0x100000, 0x40000, 0x341438e4) /* Plane 2,3 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n106001c.63c", 0x140000, 0x40000, 0xfb8adce8) /* Plane 2,3 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n106001d.638", 0x180000, 0x40000, 0x74d995c5) /* Plane 2,3 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n106001d.63c", 0x1c0000, 0x40000, 0x521b6df1) /* Plane 2,3 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	NEO_BIOS_SOUND_128K( "n106001a.4f8", 0x3377cda3 )

	ROM_REGION_OPTIONAL(0x400000) /* sound samples */
	ROM_LOAD( "n106001a.1f8", 0x000000, 0x080000, 0x2abe6ab0 )
	ROM_LOAD( "n106001a.1fc", 0x080000, 0x080000, 0xd3fa2743 )
	ROM_LOAD( "n106001b.1f8", 0x100000, 0x080000, 0xf591b0a1 )
	ROM_LOAD( "n106001b.1fc", 0x180000, 0x080000, 0x01f3999e )
	ROM_LOAD( "n106001c.1f8", 0x200000, 0x080000, 0xf76aa00e )
	ROM_LOAD( "n106001c.1fc", 0x280000, 0x080000, 0x851ce851 )
	ROM_LOAD( "n106001d.1f8", 0x300000, 0x080000, 0x9e2c8366 )
	ROM_LOAD( "n106001d.1fc", 0x380000, 0x080000, 0xb1a0ccb0 )
ROM_END

ROM_START( artfight_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n102001a.038", 0x000000, 0x040000, 0x95102254 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n102001a.378", 0x89903f39 )

	ROM_REGION(0x400000)
    ROM_LOAD( "n102001a.538", 0x000000, 0x40000, 0xa2e4a168 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n102001a.53c", 0x040000, 0x40000, 0xda389ef7 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n102001b.538", 0x080000, 0x40000, 0x2a0c385b ) /* Plane 0,1 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n102001b.53c", 0x0c0000, 0x40000, 0x4a4317bf ) /* Plane 0,1 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

    ROM_LOAD( "n102001c.538", 0x100000, 0x40000, 0x471d9e57 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n102001c.53c", 0x140000, 0x40000, 0x23fe5675 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n102001d.538", 0x180000, 0x40000, 0x204e7b29 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n102001d.53c", 0x1c0000, 0x40000, 0x7f6d5144 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	ROM_REGION(0x400000)
    ROM_LOAD( "n102001a.638", 0x000000, 0x40000, 0xca12c80f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n102001a.63c", 0x040000, 0x40000, 0xd59746b0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n102001b.638", 0x080000, 0x40000, 0x8b73b3da ) /* Plane 2,3 */
	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n102001b.63c", 0x0c0000, 0x40000, 0x9fc3f8ea ) /* Plane 2,3 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

    ROM_LOAD( "n102001c.638", 0x100000, 0x40000, 0xcbf8a72e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n102001c.63c", 0x140000, 0x40000, 0x5ec93c96 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x340000, 0x40000 )
    ROM_LOAD( "n102001d.638", 0x180000, 0x40000, 0x47763b6d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x380000, 0x40000 )
    ROM_LOAD( "n102001d.63c", 0x1c0000, 0x40000, 0x4408f4eb ) /* Plane 2,3 */
	ROM_CONTINUE(             0x3c0000, 0x40000 )

	NEO_BIOS_SOUND_128K( "n102001a.4f8", 0x981345f8 )

	ROM_REGION_OPTIONAL(0x400000) /* sound samples */
	ROM_LOAD( "n102001a.1f8", 0x000000, 0x080000, 0xa4d8747f )
	ROM_LOAD( "n102001a.1fc", 0x080000, 0x080000, 0x55219d13 )
	ROM_LOAD( "n102001b.1f8", 0x100000, 0x080000, 0x6eae81fa )
	ROM_LOAD( "n102001b.1fc", 0x180000, 0x080000, 0xf91676e9 )
	ROM_LOAD( "n102001c.1f8", 0x200000, 0x080000, 0xd0c8bcd2 )
	ROM_LOAD( "n102001c.1fc", 0x280000, 0x080000, 0x167db9b2 )
	ROM_LOAD( "n102001d.1f8", 0x300000, 0x080000, 0xf03969fe )
	ROM_LOAD( "n102001d.1fc", 0x380000, 0x080000, 0xa823a19a )
ROM_END

ROM_START( wheroes_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_ODD ( "n138001a.038", 0x000000, 0x040000, 0xab39923d)
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
	ROM_LOAD_ODD ( "n138001a.03c", 0x080000, 0x040000, 0x5adc98ef)
	ROM_CONTINUE (                 0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n138001a.378", 0x8c2c2d6b)

	ROM_REGION(0x400000)
	ROM_LOAD( "n138001a.538", 0x000000, 0x40000, 0xad8fcc5d) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x40000 )
	ROM_LOAD( "n138001a.53c", 0x040000, 0x40000, 0x0dca726e) /* Plane 0,1 */
	ROM_CONTINUE(             0x240000, 0x40000 )
	ROM_LOAD( "n138001b.538", 0x080000, 0x40000, 0xbb807a43) /* Plane 0,1 */
	ROM_CONTINUE(             0x280000, 0x40000 )
	ROM_LOAD( "n138001b.53c", 0x0c0000, 0x40000, 0xe913f93c) /* Plane 0,1 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

	ROM_LOAD( "n138001c.538", 0x100000, 0x40000, 0x3c359a14) /* Plane 0,1 */
	ROM_CONTINUE(             0x300000, 0x40000 )
	ROM_LOAD( "n138001c.53c", 0x140000, 0x40000, 0xb1327d84) /* Plane 0,1 */
	ROM_CONTINUE(             0x340000, 0x40000 )
//	ROM_LOAD( "n138001d.538", 0x180000, 0x40000, 0xbb807a43) /* Plane 0,1 - remove? */
//	ROM_CONTINUE(             0x380000, 0x40000 )
//	ROM_LOAD( "n138001d.53c", 0x1c0000, 0x40000, 0xe913f93c) /* Plane 0,1 - remove? */
//	ROM_CONTINUE(             0x3c0000, 0x40000 )

	ROM_REGION(0x400000)
	ROM_LOAD( "n138001a.638", 0x000000, 0x40000, 0x3182b4db) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x40000 )
	ROM_LOAD( "n138001a.63c", 0x040000, 0x40000, 0x1cb0a840) /* Plane 2,3 */
	ROM_CONTINUE(             0x240000, 0x40000 )
	ROM_LOAD( "n138001b.638", 0x080000, 0x40000, 0xc9f439f8) /* Plane 2,3 */
	ROM_CONTINUE(             0x280000, 0x40000 )
	ROM_LOAD( "n138001b.63c", 0x0c0000, 0x40000, 0x80441c48) /* Plane 2,3 */
	ROM_CONTINUE(             0x2c0000, 0x40000 )

	ROM_LOAD( "n138001c.638", 0x100000, 0x40000, 0x7c4b85b4) /* Plane 2,3 */
	ROM_CONTINUE(             0x300000, 0x40000 )
	ROM_LOAD( "n138001c.63c", 0x140000, 0x40000, 0x959f29db) /* Plane 2,3 */
	ROM_CONTINUE(             0x340000, 0x40000 )
//	ROM_LOAD( "n138001d.638", 0x180000, 0x40000, 0xc9f439f8) /* Plane 2,3 - remove? */
//	ROM_CONTINUE(             0x380000, 0x40000 )
//	ROM_LOAD( "n138001d.63c", 0x1c0000, 0x40000, 0x80441c48) /* Plane 2,3 - remove? */
//	ROM_CONTINUE(             0x3c0000, 0x40000 )

	NEO_BIOS_SOUND_128K( "n138001a.4f8", 0x1bd9d04b )

	ROM_REGION_OPTIONAL(0x300000) /* sound samples */
	ROM_LOAD( "n138001a.1f8", 0x000000, 0x080000, 0x77994663 )
	ROM_LOAD( "n138001a.1fc", 0x080000, 0x080000, 0xd74ad0da )
	ROM_LOAD( "n138001b.1f8", 0x100000, 0x080000, 0x9d5fe808 )
	ROM_LOAD( "n138001b.1fc", 0x180000, 0x080000, 0x883fb383 )
	ROM_LOAD( "n138001c.1f8", 0x200000, 0x080000, 0xb4ff60d7 )
	ROM_LOAD( "n138001c.1fc", 0x280000, 0x080000, 0xb358e4f5 )
ROM_END

ROM_START( sengoku2_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_ODD ( "n138001a.038", 0x000000, 0x040000, 0xd1bf3fa5 )
	ROM_CONTINUE (                 0x000000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )
    ROM_LOAD_ODD ( "n138001a.03c", 0x080000, 0x040000, 0xee9d0bb4 )
	ROM_CONTINUE (                 0x080000 & ~1, 0x040000 | ROMFLAG_ALTERNATE )

	NEO_SFIX_128K( "n138001a.378", 0xcd9802a3 )

    ROM_REGION(0x400000)
    ROM_LOAD( "n138001a.538", 0x000000, 0x40000, 0xda18aaed ) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n138001a.53c", 0x040000, 0x40000, 0x19796c4f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x240000, 0x40000 )
    ROM_LOAD( "n138001b.538", 0x080000, 0x40000, 0x891b6386 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x280000, 0x40000 )
//	ROM_LOAD( "n138001b.53c", 0x0c0000, 0x40000, 0x891b6386 ) /* Plane 0,1 - remove? */
//	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n138001c.538", 0x100000, 0x40000, 0xc5eaabe8 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n138001c.53c", 0x140000, 0x40000, 0x22633905 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x340000, 0x40000 )
//	ROM_LOAD( "n138001d.538", 0x180000, 0x40000, 0x891b6386 ) /* Plane 0,1 - remove? */
//	ROM_CONTINUE(             0x380000, 0x40000 )
//	ROM_LOAD( "n138001d.53c", 0x1c0000, 0x40000, 0x891b6386 ) /* Plane 0,1 - remove? */
//	ROM_CONTINUE(             0x380000, 0x40000 )

    ROM_REGION(0x400000)
    ROM_LOAD( "n138001a.638", 0x000000, 0x40000, 0x5b27c829 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x40000 )
    ROM_LOAD( "n138001a.63c", 0x040000, 0x40000, 0xe8b46e26 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x240000, 0x40000 )
	ROM_LOAD( "n138001b.638", 0x080000, 0x40000, 0x93d25955 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x280000, 0x40000 )
//	ROM_LOAD( "n138001b.63c", 0x0c0000, 0x40000, 0x93d25955 ) /* Plane 2,3 - remove? */
//	ROM_CONTINUE(             0x280000, 0x40000 )
    ROM_LOAD( "n138001c.638", 0x100000, 0x40000, 0x432bd7d0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x300000, 0x40000 )
    ROM_LOAD( "n138001c.63c", 0x140000, 0x40000, 0xba3f54b2 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x340000, 0x40000 )
//	ROM_LOAD( "n138001d.638", 0x180000, 0x40000, 0x93d25955 ) /* Plane 2,3 - remove? */
//	ROM_CONTINUE(             0x380000, 0x40000 )
//	ROM_LOAD( "n138001d.63c", 0x1c0000, 0x40000, 0x93d25955 ) /* Plane 2,3 - remove? */
//	ROM_CONTINUE(             0x380000, 0x40000 )

	NEO_BIOS_SOUND_128K( "n138001a.4f8", 0x9902dfa2 )

	ROM_REGION_OPTIONAL(0x300000) /* sound samples */
	ROM_LOAD( "n138001a.1f8", 0x000000, 0x080000, 0xe6e9d82f )
	ROM_LOAD( "n138001a.1fc", 0x080000, 0x080000, 0x0504e71e )
	ROM_LOAD( "n138001b.1f8", 0x100000, 0x080000, 0xe6c57d21 )
	ROM_LOAD( "n138001b.1fc", 0x180000, 0x080000, 0x000d319d )
	ROM_LOAD( "n138001c.1f8", 0x200000, 0x080000, 0x6650bc9a )
	ROM_LOAD( "n138001c.1fc", 0x280000, 0x080000, 0xc6358d62 )
ROM_END

/************************** MVS FORMAT ROMS ***********************************/

ROM_START( puzzledp_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "pdpon_p1.rom", 0x000000, 0x080000, 0x2b61415b )

	NEO_SFIX_64K( "pdpon_s1.rom", 0x4a421612 )

	ROM_REGION(0x100000)
    ROM_LOAD( "pdpon_c1.rom", 0x000000, 0x100000, 0xcc0095ef ) /* Plane 0,1 */

	ROM_REGION(0x100000)
    ROM_LOAD( "pdpon_c2.rom", 0x000000, 0x100000, 0x42371307 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "pdpon_m1.rom", 0x9c0291ea )

	ROM_REGION_OPTIONAL(0x080000) /* sound samples */
	ROM_LOAD( "pdpon_v1.rom", 0x000000, 0x080000, 0xdebeb8fb )
ROM_END

ROM_START( mahretsu_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "maj_p1.rom", 0x000000, 0x080000, 0xfc6f53db )

	NEO_SFIX_64K( "maj_s1.rom", 0xb0d16529 )

	ROM_REGION(0x100000) /* Plane 0,1 */
    ROM_LOAD( "maj_c1.rom", 0x000000, 0x80000, 0xf1ae16bc )
    ROM_LOAD( "maj_c3.rom", 0x080000, 0x80000, 0x9c571a37 )

	ROM_REGION(0x100000) /* Plane 2,3 */
    ROM_LOAD( "maj_c2.rom", 0x000000, 0x80000, 0xbdc13520 )
    ROM_LOAD( "maj_c4.rom", 0x080000, 0x80000, 0x7e81cb29 )

	NEO_BIOS_SOUND_64K( "maj_m1.rom", 0x37965a73 )

    ROM_REGION_OPTIONAL(0x100000) /* sound samples */
	ROM_LOAD( "maj_v1.rom", 0x000000, 0x080000, 0xb2fb2153 )
    ROM_LOAD( "maj_v2.rom", 0x080000, 0x080000, 0x8503317b )

    ROM_REGION_OPTIONAL(0x180000) /* sound samples */
    ROM_LOAD( "maj_v3.rom", 0x000000, 0x080000, 0x4999fb27 )
    ROM_LOAD( "maj_v4.rom", 0x080000, 0x080000, 0x776fa2a2 )
    ROM_LOAD( "maj_v5.rom", 0x100000, 0x080000, 0xb3e7eeea )
ROM_END

ROM_START( bjourney_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "bj-p1.rom", 0x000000, 0x100000, 0x6a2f6d4a )

	NEO_SFIX_128K( "bj-s1.rom", 0x843c3624 )

	ROM_REGION(0x180000)
    ROM_LOAD( "bj-c1.rom", 0x000000, 0x100000, 0x4d47a48c )
    ROM_LOAD( "bj-c3.rom", 0x100000, 0x080000, 0x66e69753 )

	ROM_REGION(0x180000)
    ROM_LOAD( "bj-c2.rom", 0x000000, 0x100000, 0xe8c1491a )
    ROM_LOAD( "bj-c4.rom", 0x100000, 0x080000, 0x71bfd48a )

	NEO_BIOS_SOUND_64K( "bj-m1.rom",  0xa9e30496 )

	ROM_REGION_OPTIONAL(0x100000) /* sound samples */
	ROM_LOAD( "bj-v11.rom", 0x000000, 0x100000, 0x2cb4ad91 )

	ROM_REGION_OPTIONAL(0x100000) /* sound samples */
	ROM_LOAD( "bj-v22.rom", 0x000000, 0x100000, 0x65a54d13 )
ROM_END

ROM_START( nam_1975_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "nam_p1.rom", 0x000000, 0x080000, 0xcc9fc951 )

	NEO_SFIX_64K( "nam_s1.rom", 0x8ded55a5 )

	ROM_REGION(0x180000)
    ROM_LOAD( "nam_c1.rom", 0x000000, 0x80000, 0x32ea98e1 ) /* Plane 0,1 */
    ROM_LOAD( "nam_c3.rom", 0x080000, 0x80000, 0x0151054c ) /* Plane 0,1 */
    ROM_LOAD( "nam_c5.rom", 0x100000, 0x80000, 0x90b74cc2 ) /* Plane 0,1 */

	ROM_REGION(0x180000)
    ROM_LOAD( "nam_c2.rom", 0x000000, 0x80000, 0xcbc4064c ) /* Plane 2,3 */
    ROM_LOAD( "nam_c4.rom", 0x080000, 0x80000, 0x0a32570d ) /* Plane 2,3 */
    ROM_LOAD( "nam_c6.rom", 0x100000, 0x80000, 0xe62bed58 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_64K( "nam_m1.rom", 0xcd088502 )

	ROM_REGION_OPTIONAL(0x080000) /* sound samples */
	ROM_LOAD( "nam_v11.rom", 0x000000, 0x080000, 0xa7c3d5e5 )

	ROM_REGION_OPTIONAL(0x180000) /* sound samples */
	ROM_LOAD( "nam_v21.rom", 0x000000, 0x080000, 0x55e670b3 )
	ROM_LOAD( "nam_v22.rom", 0x080000, 0x080000, 0xab0d8368 )
	ROM_LOAD( "nam_v23.rom", 0x100000, 0x080000, 0xdf468e28 )
ROM_END

ROM_START( socbrawl_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "sbrl_p1.rom", 0x000000, 0x080000, 0xa2801c24 )

	NEO_SFIX_64K( "sbrl_s1.rom", 0x2db38c3b )

	ROM_REGION(0x180000)
    ROM_LOAD( "sbrl_c1.rom", 0x000000, 0x100000, 0xbd0a4eb8 ) /* Plane 0,1 */
    ROM_LOAD( "sbrl_c3.rom", 0x100000, 0x080000, 0x580f7f33 ) /* Plane 0,1 */

	ROM_REGION(0x180000)
    ROM_LOAD( "sbrl_c2.rom", 0x000000, 0x100000, 0xefde5382 ) /* Plane 2,3 */
    ROM_LOAD( "sbrl_c4.rom", 0x100000, 0x080000, 0xed297de8 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_64K( "sbrl_m1.rom", 0x2f38d5d3 )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "sbrl_v1.rom", 0x000000, 0x100000, 0xcc78497e )
	ROM_LOAD( "sbrl_v2.rom", 0x100000, 0x100000, 0xdda043c6 )
ROM_END

ROM_START( maglord_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "magl_p1.rom", 0x000000, 0x080000, 0xbd0a492d )

	NEO_SFIX_128K( "magl_s1.rom", 0x1c5369a2 )

	ROM_REGION(0x180000)
    ROM_LOAD( "magl_c1.rom", 0x000000, 0x80000, 0x806aee34 ) /* Plane 0,1 */
    ROM_LOAD( "magl_c3.rom", 0x080000, 0x80000, 0xc4c2b926 ) /* Plane 0,1 */
    ROM_LOAD( "magl_c5.rom", 0x100000, 0x80000, 0x69086dec ) /* Plane 0,1 */

	ROM_REGION(0x180000)
    ROM_LOAD( "magl_c2.rom", 0x000000, 0x80000, 0x34aa9a86 ) /* Plane 2,3 */
    ROM_LOAD( "magl_c4.rom", 0x080000, 0x80000, 0x9c46dcf4 ) /* Plane 2,3 */
    ROM_LOAD( "magl_c6.rom", 0x100000, 0x80000, 0xab7ac142 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_64K( "magl_m1.rom", 0x91ee1f73)

	ROM_REGION_OPTIONAL(0x080000) /* sound samples */
	ROM_LOAD( "magl_v11.rom", 0x000000, 0x080000, 0xcc0455fd )

	ROM_REGION_OPTIONAL(0x100000) /* sound samples */
	ROM_LOAD( "magl_v21.rom", 0x000000, 0x080000, 0xf94ab5b7 )
	ROM_LOAD( "magl_v22.rom", 0x080000, 0x080000, 0x232cfd04 )
ROM_END

ROM_START( cyberlip_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "cybl_p1.rom", 0x000000, 0x080000, 0x69a6b42d )

	NEO_SFIX_128K( "cybl_s1.rom", 0x79a35264 )

	ROM_REGION(0x180000)
    ROM_LOAD( "cybl_c1.rom", 0x000000, 0x80000, 0x8bba5113 ) /* Plane 0,1 */
    ROM_LOAD( "cybl_c3.rom", 0x080000, 0x80000, 0xe4f86efc ) /* Plane 0,1 */
    ROM_LOAD( "cybl_c5.rom", 0x100000, 0x80000, 0xe8076da0 ) /* Plane 0,1 */

	ROM_REGION(0x180000)
    ROM_LOAD( "cybl_c2.rom", 0x000000, 0x80000, 0xcbf66432 ) /* Plane 2,3 */
    ROM_LOAD( "cybl_c4.rom", 0x080000, 0x80000, 0xf7be4674 ) /* Plane 2,3 */
    ROM_LOAD( "cybl_c6.rom", 0x100000, 0x80000, 0xc495c567 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_64K( "cybl_m1.rom", 0x47980d3a )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "cybl_v11.rom", 0x000000, 0x080000, 0x90224d22 )
	ROM_LOAD( "cybl_v12.rom", 0x080000, 0x080000, 0xa0cf1834 )
	ROM_LOAD( "cybl_v13.rom", 0x100000, 0x080000, 0xae38bc84 )
	ROM_LOAD( "cybl_v14.rom", 0x180000, 0x080000, 0x70899bd2 )

	ROM_REGION_OPTIONAL(0x080000) /* sound samples */
	ROM_LOAD( "cybl_v21.rom", 0x000000, 0x080000, 0x586f4cb2 )
ROM_END

ROM_START( tpgolf_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "topg_p1.rom", 0x000000, 0x080000, 0xf75549ba )
    ROM_LOAD_WIDE_SWAP( "topg_p2.rom", 0x080000, 0x080000, 0xb7809a8f )

	NEO_SFIX_128K( "topg_s1.rom", 0x7b3eb9b1 )

	ROM_REGION(0x200000)
    ROM_LOAD( "topg_c1.rom", 0x000000, 0x80000, 0x0315fbaf )
    ROM_LOAD( "topg_c3.rom", 0x080000, 0x80000, 0xb09f1612 )
    ROM_LOAD( "topg_c5.rom", 0x100000, 0x80000, 0x9a7146da )
    ROM_LOAD( "topg_c7.rom", 0x180000, 0x80000, 0x2886710c )

	ROM_REGION(0x200000)
    ROM_LOAD( "topg_c2.rom", 0x000000, 0x80000, 0xb4c15d59 )
    ROM_LOAD( "topg_c4.rom", 0x080000, 0x80000, 0x150ea7a1 )
    ROM_LOAD( "topg_c6.rom", 0x100000, 0x80000, 0x1e63411a )
    ROM_LOAD( "topg_c8.rom", 0x180000, 0x80000, 0x422af22d )

	NEO_BIOS_SOUND_64K( "topg_m1.rom", 0x7851d0d9 )

	ROM_REGION_OPTIONAL(0x080000) /* sound samples */
	ROM_LOAD( "topg_v11.rom", 0x000000, 0x080000, 0xff97f1cb )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "topg_v21.rom", 0x000000, 0x080000, 0xd34960c6 )
	ROM_LOAD( "topg_v22.rom", 0x080000, 0x080000, 0x9a5f58d4 )
	ROM_LOAD( "topg_v23.rom", 0x100000, 0x080000, 0x30f53e54 )
	ROM_LOAD( "topg_v24.rom", 0x180000, 0x080000, 0x5ba0f501 )
ROM_END

ROM_START( legendos_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "joe_p1.rom", 0x000000, 0x080000, 0x9d563f19 )

	NEO_SFIX_128K( "joe_s1.rom",  0xbcd502f0 )

	ROM_REGION(0x200000)
    ROM_LOAD( "joe_c1.rom", 0x000000, 0x100000, 0x2f5ab875 )
    ROM_LOAD( "joe_c3.rom", 0x100000, 0x100000, 0x6bc52cb2 )

	ROM_REGION(0x200000)
    ROM_LOAD( "joe_c2.rom", 0x000000, 0x100000, 0x318b2711 )
    ROM_LOAD( "joe_c4.rom", 0x100000, 0x100000, 0x37ef298c )

	NEO_BIOS_SOUND_64K( "joe_m1.rom", 0x909d4ed9 )

	ROM_REGION_OPTIONAL(0x100000) /* sound samples */
	ROM_LOAD( "joe_v1.rom", 0x000000, 0x100000, 0x85065452 )
ROM_END

ROM_START( bstars2_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "star2_p1.rom", 0x000000, 0x080000, 0x523567fd )

	NEO_SFIX_128K( "star2_s1.rom", 0x015c5c94 )

	ROM_REGION(0x200000)
    ROM_LOAD( "star2_c1.rom", 0x000000, 0x100000, 0xb39a12e1 )
    ROM_LOAD( "star2_c3.rom", 0x100000, 0x100000, 0xfb31339d )

	ROM_REGION(0x200000)
    ROM_LOAD( "star2_c2.rom", 0x000000, 0x100000, 0x766cfc2f )
    ROM_LOAD( "star2_c4.rom", 0x100000, 0x100000, 0x70457a0c )

	NEO_BIOS_SOUND_64K( "star2_m1.rom", 0xb2611c03 )

	ROM_REGION_OPTIONAL(0x280000) /* sound samples */
	ROM_LOAD( "star2_v1.rom", 0x000000, 0x100000, 0xcb1da093 )
	ROM_LOAD( "star2_v2.rom", 0x100000, 0x100000, 0x1c954a9d )
	ROM_LOAD( "star2_v3.rom", 0x200000, 0x080000, 0xafaa0180 )
ROM_END

ROM_START( wjammers_rom )
    ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "windj_p1.rom", 0x000000, 0x080000, 0xe81e7a31 )

	NEO_SFIX_128K( "windj_s1.rom", 0x66cb96eb )

	ROM_REGION(0x200000)
    ROM_LOAD( "windj_c1.rom", 0x000000, 0x100000, 0xc7650204 )
    ROM_LOAD( "windj_c3.rom", 0x100000, 0x100000, 0x40986386 )

	ROM_REGION(0x200000)
    ROM_LOAD( "windj_c2.rom", 0x000000, 0x100000, 0xd9f3e71d )
    ROM_LOAD( "windj_c4.rom", 0x100000, 0x100000, 0x715e15ff )

	NEO_BIOS_SOUND_128K( "windj_m1.rom", 0x52c23cfc )

    ROM_REGION_OPTIONAL(0x380000) /* sound samples */
	ROM_LOAD( "windj_v1.rom", 0x000000, 0x100000, 0xce8b3698 )
	ROM_LOAD( "windj_v2.rom", 0x100000, 0x100000, 0x659f9b96 )
	ROM_LOAD( "windj_v3.rom", 0x200000, 0x100000, 0x39f73061 )
    ROM_LOAD( "windj_v4.rom", 0x300000, 0x080000, 0x3740edde )
ROM_END

ROM_START( janshin_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "jans-p1.rom", 0x000000, 0x100000, 0x7514cb7a )

	NEO_SFIX_128K( "jans-s1.rom", 0x8285b25a )

	ROM_REGION(0x200000)
    ROM_LOAD( "jans-c1.rom", 0x000000, 0x200000, 0x3fa890e9 )

	ROM_REGION(0x200000)
    ROM_LOAD( "jans-c2.rom", 0x000000, 0x200000, 0x59c48ad8 )

	NEO_BIOS_SOUND_64K( "jans-m1.rom", 0xe191f955 )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "jans-v1.rom", 0x000000, 0x200000, 0xf1947d2b )
ROM_END

ROM_START( neomrdo_rom )
    ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "neomd-p1.rom", 0x000000, 0x80000, 0x39efdb82 )

	NEO_SFIX_64K( "neomd-s1.rom", 0x6c4b09c4 )

	ROM_REGION(0x200000)
    ROM_LOAD( "neomd-c1.rom", 0x000000, 0x200000, 0xc7541b9d )

	ROM_REGION(0x200000)
    ROM_LOAD( "neomd-c2.rom", 0x000000, 0x200000, 0xf57166d2 )

	NEO_BIOS_SOUND_128K( "neomd-m1.rom", 0x81eade02 )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "neomd-v1.rom", 0x000000, 0x200000, 0x4143c052 )
ROM_END

ROM_START( pbobble_rom )
    ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "puzzb_p1.rom", 0x000000, 0x040000, 0x7c3c34e1 )

	NEO_SFIX_128K( "puzzb_s1.rom", 0x9caae538 )

	ROM_REGION(0x280000)
    ROM_LOAD( "puzzb_c1.rom", 0x000000, 0x100000, 0x7f250f76 ) /* Plane 0,1 == pspikes2 */
    ROM_LOAD( "puzzb_c3.rom", 0x100000, 0x100000, 0x4b641ba1 ) /* Plane 0,1 == pspikes2 */
    ROM_LOAD( "puzzb_c5.rom", 0x200000, 0x080000, 0xe89ad494 ) /* Plane 0,1 */

	ROM_REGION(0x280000)
    ROM_LOAD( "puzzb_c2.rom", 0x000000, 0x100000, 0x20912873 ) /* Plane 2,3 == pspikes2 */
    ROM_LOAD( "puzzb_c4.rom", 0x100000, 0x100000, 0x35072596 ) /* Plane 2,3 == pspikes2 */
    ROM_LOAD( "puzzb_c6.rom", 0x200000, 0x080000, 0x4b42d7eb ) /* Plane 2,3 */

	NEO_BIOS_SOUND_64K( "puzzb_m1.rom", 0x129e6054 )

	ROM_REGION_OPTIONAL(0x380000) /* sound samples */
	ROM_LOAD( "puzzb_v1.rom", 0x000000, 0x100000, 0x2ced86df ) /* == pspikes2 */
	ROM_LOAD( "puzzb_v2.rom", 0x100000, 0x100000, 0x970851ab ) /* == pspikes2 */
	ROM_LOAD( "puzzb_v3.rom", 0x200000, 0x100000, 0x0840cbc4 )
	ROM_LOAD( "puzzb_v4.rom", 0x300000, 0x080000, 0x0a548948 )
ROM_END

ROM_START( pspikes2_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "spike_p1.rom", 0x000000, 0x100000, 0x105a408f )

	NEO_SFIX_128K( "spike_s1.rom", 0x18082299 )

	ROM_REGION(0x300000)
    ROM_LOAD( "spike_c1.rom", 0x000000, 0x100000, 0x7f250f76 ) /* Plane 0,1 == pbobble */
    ROM_LOAD( "spike_c3.rom", 0x100000, 0x100000, 0x4b641ba1 ) /* Plane 0,1 == pbobble */
    ROM_LOAD( "spike_c5.rom", 0x200000, 0x100000, 0x151dd624 ) /* Plane 0,1 */

	ROM_REGION(0x300000)
    ROM_LOAD( "spike_c2.rom", 0x000000, 0x100000, 0x20912873 ) /* Plane 2,3 == pbobble */
    ROM_LOAD( "spike_c4.rom", 0x100000, 0x100000, 0x35072596 ) /* Plane 2,3 == pbobble */
    ROM_LOAD( "spike_c6.rom", 0x200000, 0x100000, 0xa6722604 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "spike_m1.rom", 0xb1c7911e )

	ROM_REGION_OPTIONAL(0x300000) /* sound samples */
	ROM_LOAD( "spike_v1.rom", 0x000000, 0x100000, 0x2ced86df )	/* == pbobble */
	ROM_LOAD( "spike_v2.rom", 0x100000, 0x100000, 0x970851ab )	/* == pbobble */
	ROM_LOAD( "spike_v3.rom", 0x200000, 0x100000, 0x81ff05aa )
ROM_END

ROM_START( sidkicks_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "sidek_p1.rom", 0x000000, 0x080000, 0x9cd97256 )

	NEO_SFIX_128K( "sidek_s1.rom", 0x97689804 )

	ROM_REGION(0x300000)
    ROM_LOAD( "sidek_c1.rom", 0x000000, 0x100000, 0x53e1c002 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x100000 )

	ROM_REGION(0x300000) /* CHECK THIS LATER */
    ROM_LOAD( "sidek_c2.rom", 0x000000, 0x100000, 0x776a2d1f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x100000 )

	NEO_BIOS_SOUND_128K( "sidek_m1.rom", 0x49f17d2d )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "sidek_v1.rom", 0x000000, 0x200000, 0x22c097a5 )
ROM_END

ROM_START( panicbom_rom )
    ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "panic_p1.rom", 0x000000, 0x040000, 0x0b21130d )

	NEO_SFIX_128K( "panic_s1.rom", 0xb876de7e )

    ROM_REGION(0x100000)
    ROM_LOAD( "panic_c1.rom", 0x000000, 0x100000, 0x8582e1b5 ) /* Plane 0,1 */

    ROM_REGION(0x100000)
    ROM_LOAD( "panic_c2.rom", 0x000000, 0x100000, 0xe15a093b ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "panic_m1.rom", 0x3cdf5d88 )

    ROM_REGION_OPTIONAL(0x300000) /* sound samples */
	ROM_LOAD( "panic_v1.rom", 0x000000, 0x200000, 0x7fc86d2f )
    ROM_LOAD( "panic_v2.rom", 0x200000, 0x100000, 0x082adfc7 )
ROM_END

ROM_START( viewpoin_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "viewp_p1.rom", 0x000000, 0x100000, 0x17aa899d )

	NEO_SFIX_64K( "viewp_s1.rom", 0x6d0f146a )

	ROM_REGION(0x300000)
    ROM_LOAD( "viewp_c1.rom", 0x000000, 0x100000, 0xd624c132 )
	ROM_CONTINUE(             0x200000, 0x100000 )

	ROM_REGION(0x300000)
    ROM_LOAD( "viewp_c2.rom", 0x000000, 0x100000, 0x40d69f1e )
	ROM_CONTINUE(             0x200000, 0x100000 )

	NEO_BIOS_SOUND_64K( "viewp_m1.rom", 0xd57bd7af )

	ROM_REGION_OPTIONAL(0x400000) /* sound samples */
	ROM_LOAD( "viewp_v1.rom", 0x000000, 0x200000, 0x019978b6 )
	ROM_LOAD( "viewp_v2.rom", 0x200000, 0x200000, 0x5758f38c )
ROM_END

ROM_START( zedblade_rom )
    ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "zedbl_p1.rom", 0x000000, 0x080000, 0xd7c1effd )

	NEO_SFIX_128K( "zedbl_s1.rom", 0xf4c25dd5 )

    ROM_REGION(0x400000)
    ROM_LOAD( "zedbl_c1.rom", 0x000000, 0x200000, 0x4d9cb038 ) /* Plane 0,1 */
    ROM_LOAD( "zedbl_c3.rom", 0x200000, 0x200000, 0xd06431e3 ) /* Plane 0,1 */

    ROM_REGION(0x400000)
    ROM_LOAD( "zedbl_c2.rom", 0x000000, 0x200000, 0x09233884 ) /* Plane 2,3 */
    ROM_LOAD( "zedbl_c4.rom", 0x200000, 0x200000, 0x4b1c089b ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "zedbl_m1.rom", 0x7b5f3d0a )

	ROM_REGION_OPTIONAL(0x500000) /* sound samples */
	ROM_LOAD( "zedbl_v1.rom", 0x000000, 0x200000, 0x1a21d90c )
	ROM_LOAD( "zedbl_v2.rom", 0x200000, 0x200000, 0xb61686c3 )
	ROM_LOAD( "zedbl_v3.rom", 0x400000, 0x100000, 0xb90658fa )
ROM_END

ROM_START( kotm2_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "kotm2_p1.rom", 0x000000, 0x080000, 0xb372d54c )
    ROM_LOAD_WIDE_SWAP( "kotm2_p2.rom", 0x080000, 0x080000, 0x28661afe )

	NEO_SFIX_128K( "kotm2_s1.rom", 0x63ee053a )

	ROM_REGION(0x400000)
    ROM_LOAD( "kotm2_c1.rom", 0x000000, 0x100000, 0x6d1c4aa9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x100000 )
    ROM_LOAD( "kotm2_c3.rom", 0x100000, 0x100000, 0x40156dca ) /* Plane 0,1 */
	ROM_CONTINUE(             0x300000, 0x100000 )

	ROM_REGION(0x400000)
    ROM_LOAD( "kotm2_c2.rom", 0x000000, 0x100000, 0xf7b75337 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x100000 )
    ROM_LOAD( "kotm2_c4.rom", 0x100000, 0x100000, 0xb0d44111 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x300000, 0x100000 )

	NEO_BIOS_SOUND_128K( "kotm2_m1.rom", 0x0c5b2ad5 )

	ROM_REGION_OPTIONAL(0x300000) /* sound samples */
	ROM_LOAD( "kotm2_v1.rom", 0x000000, 0x200000, 0x86d34b25 )
	ROM_LOAD( "kotm2_v2.rom", 0x200000, 0x100000, 0x8fa62a0b )
ROM_END

ROM_START( stakwin_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "stakw_p1.rom",  0x100000, 0x100000, 0xbd5814f6 )
	ROM_CONTINUE(                        0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP)

	NEO_SFIX_128K( "stakw_s1.rom", 0x073cb208 )

	ROM_REGION(0x400000)
    ROM_LOAD( "stakw_c1.rom", 0x000000, 0x200000, 0x6e733421 ) /* Plane 0,1 */
    ROM_LOAD( "stakw_c3.rom", 0x200000, 0x200000, 0x8fa5a9eb ) /* Plane 0,1 */

	ROM_REGION(0x400000)
    ROM_LOAD( "stakw_c2.rom", 0x000000, 0x200000, 0x4d865347 ) /* Plane 2,3 */
    ROM_LOAD( "stakw_c4.rom", 0x200000, 0x200000, 0x4604f0dc ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "stakw_m1.rom", 0x2fe1f499 )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "stakw_v1.rom", 0x000000, 0x200000, 0xb7785023 )
ROM_END

ROM_START( turfmast_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "turfm_p1.rom",  0x100000, 0x100000, 0x28c83048 )
	ROM_CONTINUE(                        0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP)

	NEO_SFIX_128K( "turfm_s1.rom", 0x9a5402b2 )

	ROM_REGION(0x400000)
	ROM_LOAD( "turfm_c1.rom", 0x200000, 0x200000, 0x8c6733f2 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )

	ROM_REGION(0x400000)
	ROM_LOAD( "turfm_c2.rom", 0x200000, 0x200000, 0x596cc256 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )

	NEO_BIOS_SOUND_128K( "turfm_m1.rom", 0x9994ac00 )

	ROM_REGION_OPTIONAL(0x800000) /* sound samples */
	ROM_LOAD( "turfm_v1.rom", 0x000000, 0x200000, 0x00fd48d2 )
	ROM_LOAD( "turfm_v2.rom", 0x200000, 0x200000, 0x082acb31 )
	ROM_LOAD( "turfm_v3.rom", 0x400000, 0x200000, 0x7abca053 )
	ROM_LOAD( "turfm_v4.rom", 0x600000, 0x200000, 0x6c7b4902 )
ROM_END

ROM_START( neodrift_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "drift_p1.rom",  0x100000, 0x100000, 0xe397d798 )
	ROM_CONTINUE(                        0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP)

	NEO_SFIX_128K( "drift_s1.rom", 0xb76b61bc )

	ROM_REGION(0x400000)
	ROM_LOAD( "drift_c1.rom", 0x200000, 0x200000, 0x62c5edc9 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )

	ROM_REGION(0x400000)
	ROM_LOAD( "drift_c2.rom", 0x200000, 0x200000, 0x9dc9c72a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )

	NEO_BIOS_SOUND_128K( "drift_m1.rom", 0x00000000 )

	ROM_REGION_OPTIONAL(0x400000) /* sound samples */
	ROM_LOAD( "drift_v1.rom", 0x000000, 0x200000, 0xa421c076 )
	ROM_LOAD( "drift_v2.rom", 0x200000, 0x200000, 0x233c7dd9 )
ROM_END

ROM_START( fatfury2_rom )
	ROM_REGION(0x100000)
	ROM_LOAD_WIDE_SWAP( "fury2_p1.rom", 0x000000, 0x080000, 0xbe40ea92 )
	ROM_LOAD_WIDE_SWAP( "fury2_p2.rom", 0x080000, 0x080000, 0x2a9beac5 )

	NEO_SFIX_128K( "fury2_s1.rom", 0xd7dbbf39 )

	ROM_REGION(0x400000)
	ROM_LOAD( "fury2_c1.rom", 0x000000, 0x100000, 0xf72a939e ) /* Plane 0,1 */
	ROM_CONTINUE(             0x200000, 0x100000 )
	ROM_LOAD( "fury2_c3.rom", 0x100000, 0x100000, 0x01e00738 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x300000, 0x100000 )

	ROM_REGION(0x400000)
	ROM_LOAD( "fury2_c2.rom", 0x000000, 0x100000, 0x05119a0d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x200000, 0x100000 )
	ROM_LOAD( "fury2_c4.rom", 0x100000, 0x100000, 0x9fe27432 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x300000, 0x100000 )

	NEO_BIOS_SOUND_128K( "fury2_m1.rom", 0x820b0ba7 )

	ROM_REGION_OPTIONAL(0x400000) /* sound samples */
	ROM_LOAD( "fury2_v1.rom", 0x000000, 0x200000, 0xd9d00784 )
	ROM_LOAD( "fury2_v2.rom", 0x200000, 0x200000, 0x2c9a4b33 )
ROM_END

ROM_START( sonicwi2_rom )
    ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "afig2_p1.rom", 0x100000, 0x100000, 0x92871738 )
    ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "afig2_s1.rom", 0x47cc6177 )

    ROM_REGION(0x400000)
    ROM_LOAD( "afig2_c1.rom", 0x000000, 0x200000, 0x3278e73e ) /* Plane 0,1 */
    ROM_LOAD( "afig2_c3.rom", 0x200000, 0x200000, 0xc1b438f1 ) /* Plane 0,1 */

    ROM_REGION(0x400000)
    ROM_LOAD( "afig2_c2.rom", 0x000000, 0x200000, 0xfe6355d6 ) /* Plane 2,3 */
    ROM_LOAD( "afig2_c4.rom", 0x200000, 0x200000, 0x1f777206 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "afig2_m1.rom", 0xbb828df1 )

    ROM_REGION_OPTIONAL(0x280000) /* sound samples */
	ROM_LOAD( "afig2_v1.rom", 0x000000, 0x200000, 0x7577e949 )
    ROM_LOAD( "afig2_v2.rom", 0x200000, 0x080000, 0x6d0a728e )
ROM_END

ROM_START( ssideki2_rom )
    ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "kick2_p1.rom", 0x000000, 0x100000, 0x5969e0dc )

	NEO_SFIX_128K( "kick2_s1.rom", 0x226d1b68 )

    ROM_REGION(0x400000)
    ROM_LOAD( "kick2_c1.rom", 0x000000, 0x200000, 0xa626474f ) /* Plane 0,1 */
    ROM_LOAD( "kick2_c3.rom", 0x200000, 0x200000, 0x2a7b98b9 ) /* Plane 0,1 */

    ROM_REGION(0x400000)
    ROM_LOAD( "kick2_c2.rom", 0x000000, 0x200000, 0xc3be42ae ) /* Plane 2,3 */
    ROM_LOAD( "kick2_c4.rom", 0x200000, 0x200000, 0xc0be9a1f ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "kick2_m1.rom", 0x156f6951 )

	ROM_REGION_OPTIONAL(0x400000) /* sound samples */
	ROM_LOAD( "kick2_v1.rom", 0x000000, 0x200000, 0xf081c8d3 )
	ROM_LOAD( "kick2_v2.rom", 0x200000, 0x200000, 0x7cd63302 )
ROM_END

ROM_START( tophuntr_rom )
    ROM_REGION(0x180000)
    ROM_LOAD_WIDE_SWAP( "thunt_p1.rom", 0x000000, 0x100000, 0x69fa9e29 )
    ROM_LOAD_WIDE_SWAP( "thunt_p2.rom", 0x100000, 0x080000, 0xdb71f269 )

	NEO_SFIX_128K( "thunt_s1.rom", 0x6a454dd1 )

	ROM_REGION(0x400000)
    ROM_LOAD( "thunt_c1.rom", 0x000000, 0x100000, 0xfa720a4a ) /* Plane 0,1 */
    ROM_LOAD( "thunt_c3.rom", 0x100000, 0x100000, 0x880e3c25 ) /* Plane 0,1 */
    ROM_LOAD( "thunt_c5.rom", 0x200000, 0x100000, 0x4b735e45 ) /* Plane 0,1 */
    ROM_LOAD( "thunt_c7.rom", 0x300000, 0x100000, 0x12829c4c ) /* Plane 0,1 */

	ROM_REGION(0x400000)
    ROM_LOAD( "thunt_c2.rom", 0x000000, 0x100000, 0xc900c205 ) /* Plane 2,3 */
    ROM_LOAD( "thunt_c4.rom", 0x100000, 0x100000, 0x7a2248aa ) /* Plane 2,3 */
    ROM_LOAD( "thunt_c6.rom", 0x200000, 0x100000, 0x273171df ) /* Plane 2,3 */
    ROM_LOAD( "thunt_c8.rom", 0x300000, 0x100000, 0xc944e03d ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "thunt_m1.rom", 0x3f84bb9f )

	ROM_REGION_OPTIONAL(0x400000) /* sound samples */
	ROM_LOAD( "thunt_v1.rom", 0x000000, 0x100000, 0xc1f9c2db )
	ROM_LOAD( "thunt_v2.rom", 0x100000, 0x100000, 0x56254a64 )
	ROM_LOAD( "thunt_v3.rom", 0x200000, 0x100000, 0x58113fb1 )
	ROM_LOAD( "thunt_v4.rom", 0x300000, 0x100000, 0x4f54c187 )
ROM_END

ROM_START( spinmast_rom )
    ROM_REGION(0x180000)
    ROM_LOAD_WIDE_SWAP( "spnm_p1.rom", 0x000000, 0x100000, 0x37aba1aa )
    ROM_LOAD_WIDE_SWAP( "spnm_p2.rom", 0x100000, 0x080000, 0x43763ad2 )

	NEO_SFIX_128K( "spnm_s1.rom", 0x289e2bbe )

	ROM_REGION(0x400000)
    ROM_LOAD( "spnm_c1.rom", 0x000000, 0x100000, 0xa9375aa2 ) /* Plane 0,1 */
    ROM_LOAD( "spnm_c3.rom", 0x100000, 0x100000, 0xdf51e465 ) /* Plane 0,1 */
    ROM_LOAD( "spnm_c5.rom", 0x200000, 0x100000, 0x7babd692 ) /* Plane 0,1 */
    ROM_LOAD( "spnm_c7.rom", 0x300000, 0x100000, 0xbb2fd7c0 ) /* Plane 0,1 */

	ROM_REGION(0x400000)
    ROM_LOAD( "spnm_c2.rom", 0x000000, 0x100000, 0x0e73b758 ) /* Plane 2,3 */
    ROM_LOAD( "spnm_c4.rom", 0x100000, 0x100000, 0x38517e90 ) /* Plane 2,3 */
    ROM_LOAD( "spnm_c6.rom", 0x200000, 0x100000, 0xcde5ade5 ) /* Plane 2,3 */
    ROM_LOAD( "spnm_c8.rom", 0x300000, 0x100000, 0x8d7be933 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "spnm_m1.rom", 0x76108b2f )

    ROM_REGION_OPTIONAL(0x100000) /* sound samples */
    ROM_LOAD( "spnm_v1.rom", 0x000000, 0x100000, 0xcc281aef )
ROM_END

ROM_START( strhoops_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "shoop_p1.rom", 0x000000, 0x100000, 0x5e78328e )

	NEO_SFIX_128K( "shoop_s1.rom", 0xa8205610 )

	ROM_REGION(0x400000)
    ROM_LOAD( "shoop_c1.rom", 0x000000, 0x200000, 0x0581c72a ) /* Plane 0,1 */
    ROM_LOAD( "shoop_c3.rom", 0x200000, 0x200000, 0xcd65bb62 ) /* Plane 0,1 */

	ROM_REGION(0x400000)
    ROM_LOAD( "shoop_c2.rom", 0x000000, 0x200000, 0x5b9b8fb6 ) /* Plane 2,3 */
    ROM_LOAD( "shoop_c4.rom", 0x200000, 0x200000, 0xa4c90213 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_64K( "shoop_m1.rom", 0x1a5f08db )

    ROM_REGION_OPTIONAL(0x280000) /* sound samples */
	ROM_LOAD( "shoop_v1.rom", 0x000000, 0x200000, 0x718a2400 )
    ROM_LOAD( "shoop_v2.rom", 0x200000, 0x080000, 0xb19884f8 )
ROM_END

ROM_START( samsho_rom )
	ROM_REGION(0x180000)
    ROM_LOAD_WIDE_SWAP( "samsh_p1.rom", 0x000000, 0x080000, 0x80aa6c97 )
    ROM_LOAD_WIDE_SWAP( "samsh_p2.rom", 0x080000, 0x080000, 0x71768728 )
    ROM_LOAD_WIDE_SWAP( "samsh_p3.rom", 0x100000, 0x080000, 0x38ee9ba9 )

	NEO_SFIX_128K( "samsh_s1.rom", 0x9142a4d3 )

	ROM_REGION(0x480000)
    ROM_LOAD( "samsh_c1.rom", 0x000000, 0x200000, 0x2e5873a4 ) /* Plane 0,1 */
    ROM_LOAD( "samsh_c3.rom", 0x200000, 0x200000, 0xf3dabd1e ) /* Plane 0,1 */
    ROM_LOAD( "samsh_c5.rom", 0x400000, 0x080000, 0xa2bb8284 ) /* Plane 0,1 */

	ROM_REGION(0x480000)
    ROM_LOAD( "samsh_c2.rom", 0x000000, 0x200000, 0x04febb10 ) /* Plane 2,3 */
    ROM_LOAD( "samsh_c4.rom", 0x200000, 0x200000, 0x935c62f0 ) /* Plane 2,3 */
    ROM_LOAD( "samsh_c6.rom", 0x400000, 0x080000, 0x4fa71252 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "samsh_m1.rom", 0x95170640 )

	ROM_REGION_OPTIONAL(0x400000) /* sound samples */
	ROM_LOAD( "samsh_v1.rom", 0x000000, 0x200000, 0x37f78a9b )
	ROM_LOAD( "samsh_v2.rom", 0x200000, 0x200000, 0x568b20cf )
ROM_END

ROM_START( neobombe_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "bombm_p1.rom", 0x000000, 0x100000, 0xa1a71d0d )

	NEO_SFIX_128K( "bombm_s1.rom", 0x4b3fa119 )

	ROM_REGION(0x480000) /* Plane 0,1 */
    ROM_LOAD( "bombm_c1.rom", 0x200000, 0x200000, 0xb90ebed4 )
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "bombm_c3.rom", 0x400000, 0x080000, 0xe37578c5 )

	ROM_REGION(0x480000) /* Plane 2,3 */
    ROM_LOAD( "bombm_c2.rom", 0x200000, 0x200000, 0x41e62b4f )
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "bombm_c4.rom", 0x400000, 0x080000, 0x59826783 )

	NEO_BIOS_SOUND_128K( "bombm_m1.rom", 0xe81e780b )

	ROM_REGION_OPTIONAL(0x600000) /* sound samples */
	ROM_LOAD( "bombm_v1.rom", 0x000000, 0x400000, 0x00000000 )
	ROM_LOAD( "bombm_v2.rom", 0x400000, 0x200000, 0xa92b8b3d )
ROM_END

ROM_START( tws96_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "tecmo_p1.rom", 0x000000, 0x100000, 0x03e20ab6 )

	NEO_SFIX_128K( "tecmo_s1.rom", 0x6f5e2b3a )

    ROM_REGION(0x500000) /* Plane 0,1 */
    ROM_LOAD( "tecmo_c1.rom", 0x200000, 0x200000, 0xd301a867 )
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "tecmo_c3.rom", 0x400000, 0x100000, 0x750ddc0c )

    ROM_REGION(0x500000) /* Plane 2,3 */
    ROM_LOAD( "tecmo_c2.rom", 0x200000, 0x200000, 0x305fc74f )
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "tecmo_c4.rom", 0x400000, 0x100000, 0x7a6e7d82 )

	NEO_BIOS_SOUND_64K( "tecmo_m1.rom", 0x860ba8c7 )

	ROM_REGION_OPTIONAL(0x400000) /* sound samples */
	ROM_LOAD( "tecmo_v1.rom", 0x000000, 0x200000, 0x97bf1986 )
	ROM_LOAD( "tecmo_v2.rom", 0x200000, 0x200000, 0xb7eb05df )
ROM_END

ROM_START( twinspri_rom )
	ROM_REGION(0x400000)
    ROM_LOAD_WIDE_SWAP( "sprit_p1.rom", 0x100000, 0x100000, 0x7697e445 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "sprit_s1.rom", 0xeeed5758 )

    ROM_REGION(0x500000) /* Plane 0,1 */
    ROM_LOAD( "sprit_c1.rom", 0x200000, 0x200000, 0x73b2a70b )
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "sprit_c3.rom", 0x400000, 0x100000, 0xc59e4129 )

    ROM_REGION(0x500000) /* Plane 2,3 */
    ROM_LOAD( "sprit_c2.rom", 0x200000, 0x200000, 0x3a3e506c )
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "sprit_c4.rom", 0x400000, 0x100000, 0xb5532e53 )

	NEO_BIOS_SOUND_128K( "sprit_m1.rom", 0x364d6f96 )

	ROM_REGION_OPTIONAL(0x600000) /* sound samples */
	ROM_LOAD( "sprit_v1.rom", 0x000000, 0x400000, 0x00000000 )	/* bad read: odd bytes all 0xff */
	ROM_LOAD( "sprit_v2.rom", 0x400000, 0x200000, 0x7ad26599 )
ROM_END

ROM_START( karnov_r_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "karev_p1.rom", 0x000000, 0x100000, 0x8c86fd22 )

	NEO_SFIX_128K( "karev_s1.rom", 0xbae5d5e5 )

	ROM_REGION(0x600000)
    ROM_LOAD( "karev_c1.rom", 0x000000, 0x200000, 0x09dfe061 ) /* Plane 0,1 */
    ROM_LOAD( "karev_c3.rom", 0x200000, 0x200000, 0xa673b4f7 ) /* Plane 0,1 */
    ROM_LOAD( "karev_c5.rom", 0x400000, 0x200000, 0x9a28785d ) /* Plane 0,1 */

	ROM_REGION(0x600000)
    ROM_LOAD( "karev_c2.rom", 0x000000, 0x200000, 0xe0f6682a ) /* Plane 2,3 */
    ROM_LOAD( "karev_c4.rom", 0x200000, 0x200000, 0xcb3dc5f4 ) /* Plane 2,3 */
    ROM_LOAD( "karev_c6.rom", 0x400000, 0x200000, 0xc15c01ed ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "karev_m1.rom", 0x030beae4 )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "karev_v1.rom", 0x000000, 0x200000, 0x0b7ea37a )
ROM_END

ROM_START( fatfursp_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "ffspe_p1.rom", 0x000000, 0x100000, 0x2f585ba2 )
    ROM_LOAD_WIDE_SWAP( "ffspe_p2.rom", 0x100000, 0x080000, 0xd7c71a6b )
    ROM_LOAD_WIDE_SWAP( "ffspe_p3.rom", 0x180000, 0x080000, 0x9f0c1e1a )

	NEO_SFIX_128K( "ffspe_s1.rom", 0x2df03197 )

	ROM_REGION(0x600000)
    ROM_LOAD( "ffspe_c1.rom", 0x000000, 0x200000, 0x044ab13c ) /* Plane 0,1 */
    ROM_LOAD( "ffspe_c3.rom", 0x200000, 0x200000, 0x6f7938d5 ) /* Plane 0,1 */
    ROM_LOAD( "ffspe_c5.rom", 0x400000, 0x200000, 0x49c5e0bf ) /* Plane 0,1 */

	ROM_REGION(0x600000)
    ROM_LOAD( "ffspe_c2.rom", 0x000000, 0x200000, 0x11e6bf96 ) /* Plane 2,3 */
    ROM_LOAD( "ffspe_c4.rom", 0x200000, 0x200000, 0x4ad066ff ) /* Plane 2,3 */
    ROM_LOAD( "ffspe_c6.rom", 0x400000, 0x200000, 0x8ff1f43d ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "ffspe_m1.rom", 0xccc5186e )

	ROM_REGION_OPTIONAL(0x500000) /* sound samples */
	ROM_LOAD( "ffspe_v1.rom", 0x000000, 0x200000, 0x55d7ce84 )
	ROM_LOAD( "ffspe_v2.rom", 0x200000, 0x200000, 0xee080b10 )
	ROM_LOAD( "ffspe_v3.rom", 0x400000, 0x100000, 0xf9eb3d4a )
ROM_END

ROM_START( sonicwi3_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "sonw3_p1.rom", 0x100000, 0x100000, 0x0547121d )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "sonw3_s1.rom", 0x8dd66743 )

	ROM_REGION(0x600000)
	ROM_LOAD( "sonw3_c1.rom", 0x200000, 0x200000, 0x3ca97864 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "sonw3_c3.rom", 0x400000, 0x200000, 0xc339fff5 ) /* Plane 0,1 */

	ROM_REGION(0x600000)
	ROM_LOAD( "sonw3_c2.rom", 0x200000, 0x200000, 0x1da4b3a9 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "sonw3_c4.rom", 0x400000, 0x200000, 0x84a40c6e ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "sonw3_m1.rom", 0xb20e4291 )

    ROM_REGION_OPTIONAL(0x500000) /* sound samples */
	ROM_LOAD( "sonw3_v1.rom", 0x000000, 0x400000, 0x00000000 )	/* bad! odd bytes all 0xff */
    ROM_LOAD( "sonw3_v2.rom", 0x000000, 0x100000, 0x32187ccd )
ROM_END

ROM_START( goalx3_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "goal!_p1.rom", 0x100000, 0x100000, 0x2a019a79 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "goal!_s1.rom", 0xc0eaad86 )

    ROM_REGION(0x500000)
    ROM_LOAD( "goal!_c1.rom", 0x200000, 0x200000, 0xd061f1f5 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "goal!_c3.rom", 0x400000, 0x100000, 0x5f91bace ) /* Plane 0,1 */

    ROM_REGION(0x500000)
    ROM_LOAD( "goal!_c2.rom", 0x200000, 0x200000, 0x3f63c1a2 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "goal!_c4.rom", 0x400000, 0x100000, 0x1e9f76f2 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_64K( "goal!_m1.rom", 0xdd945773 )

	ROM_REGION_OPTIONAL(0x200000) /* sound samples */
	ROM_LOAD( "goal!_v1.rom", 0x000000, 0x200000, 0xef214212 )
ROM_END

ROM_START( worlher2_rom )
    ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "hero2_p1.rom", 0x100000, 0x100000, 0x65a891d9 )
    ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "hero2_s1.rom", 0xfcaeb3a4 )

	ROM_REGION(0x600000)
	ROM_LOAD( "hero2_c1.rom", 0x000000, 0x200000, 0x21c6bb91 ) /* Plane 0,1 */
	ROM_LOAD( "hero2_c3.rom", 0x200000, 0x200000, 0xb725a219 ) /* Plane 0,1 */
	ROM_LOAD( "hero2_c5.rom", 0x400000, 0x200000, 0xb20354af ) /* Plane 0,1 */

	ROM_REGION(0x600000)
	ROM_LOAD( "hero2_c2.rom", 0x000000, 0x200000, 0xa3999925 ) /* Plane 2,3 */
	ROM_LOAD( "hero2_c4.rom", 0x200000, 0x200000, 0x8d96425e ) /* Plane 2,3 */
	ROM_LOAD( "hero2_c6.rom", 0x400000, 0x200000, 0xb13d1de3 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "hero2_m1.rom", 0x8fa3bc77 )

	ROM_REGION_OPTIONAL(0x400000) /* sound samples */
	ROM_LOAD( "hero2_v1.rom", 0x000000, 0x200000, 0x8877e301 )
	ROM_LOAD( "hero2_v2.rom", 0x200000, 0x200000, 0xc1317ff4 )
ROM_END

ROM_START( doubledr_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "ddrag_p1.rom", 0x100000, 0x100000, 0x34ab832a )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "ddrag_s1.rom", 0xbef995c5 )

    ROM_REGION(0x700000)
    ROM_LOAD( "ddrag_c1.rom", 0x000000, 0x200000, 0xb478c725 )
    ROM_LOAD( "ddrag_c3.rom", 0x200000, 0x200000, 0x8b0d378e )
    ROM_LOAD( "ddrag_c5.rom", 0x400000, 0x200000, 0xec87bff6 )
    ROM_LOAD( "ddrag_c7.rom", 0x600000, 0x100000, 0x727c4d02 )

    ROM_REGION(0x700000)
    ROM_LOAD( "ddrag_c2.rom", 0x000000, 0x200000, 0x2857da32 )
    ROM_LOAD( "ddrag_c4.rom", 0x200000, 0x200000, 0xc7d2f596 )
    ROM_LOAD( "ddrag_c6.rom", 0x400000, 0x200000, 0x844a8a11 )
    ROM_LOAD( "ddrag_c8.rom", 0x600000, 0x100000, 0x69a5fa37 )

	NEO_BIOS_SOUND_128K( "ddrag_m1.rom", 0x10b144de )

	ROM_REGION_OPTIONAL(0x400000) /* sound samples */
	ROM_LOAD( "ddrag_v1.rom", 0x000000, 0x200000, 0xcc1128e4 )
	ROM_LOAD( "ddrag_v2.rom", 0x200000, 0x200000, 0xc3ff5554 )
ROM_END

ROM_START( galaxyfg_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "galfi_p1.rom", 0x100000, 0x100000, 0x45906309 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "galfi_s1.rom", 0x72f8923e )

	ROM_REGION(0x700000)
    ROM_LOAD( "galfi_c1.rom", 0x000000, 0x200000, 0xc890c7c0 ) /* Plane 0,1 */
    ROM_LOAD( "galfi_c3.rom", 0x200000, 0x200000, 0x9d87e761 ) /* Plane 0,1 */
    ROM_LOAD( "galfi_c5.rom", 0x400000, 0x200000, 0xe6b77e6a ) /* Plane 0,1 */
    ROM_LOAD( "galfi_c7.rom", 0x600000, 0x100000, 0x4f27d580 ) /* Plane 0,1 */

	ROM_REGION(0x700000)
    ROM_LOAD( "galfi_c2.rom", 0x000000, 0x200000, 0xb6d25419 ) /* Plane 2,3 */
    ROM_LOAD( "galfi_c4.rom", 0x200000, 0x200000, 0x765d7cb8 ) /* Plane 2,3 */
    ROM_LOAD( "galfi_c6.rom", 0x400000, 0x200000, 0xd779a181 ) /* Plane 2,3 */
    ROM_LOAD( "galfi_c8.rom", 0x600000, 0x100000, 0x0a7cc0d8 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "galfi_m1.rom", 0x8e9e3b10 )

	ROM_REGION_OPTIONAL(0x500000) /* sound samples */
	ROM_LOAD( "galfi_v1.rom", 0x000000, 0x200000, 0xe3b735ac )
	ROM_LOAD( "galfi_v2.rom", 0x200000, 0x200000, 0x6a8e78c2 )
	ROM_LOAD( "galfi_v3.rom", 0x400000, 0x100000, 0x70bca656 )
ROM_END

ROM_START( worlhe2j_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "wh2j_p1.rom", 0x100000, 0x100000, 0x385a2e86 )
	ROM_CONTINUE(                      0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "wh2j_s1.rom", 0x2a03998a )

	ROM_REGION(0x800000)
	ROM_LOAD( "wh2j_c1.rom", 0x000000, 0x200000, 0x2ec87cea ) /* Plane 0,1 */
	ROM_LOAD( "wh2j_c3.rom", 0x200000, 0x200000, 0x436d1b31 ) /* Plane 0,1 */
	ROM_LOAD( "wh2j_c5.rom", 0x400000, 0x200000, 0x8e34a9f4 ) /* Plane 0,1 */
	ROM_LOAD( "wh2j_c7.rom", 0x600000, 0x200000, 0x59d97215 ) /* Plane 0,1 */

	ROM_REGION(0x800000)
	ROM_LOAD( "wh2j_c2.rom", 0x000000, 0x200000, 0x526b81ab ) /* Plane 2,3 */
	ROM_LOAD( "wh2j_c4.rom", 0x200000, 0x200000, 0xf9c8dd26 ) /* Plane 2,3 */
	ROM_LOAD( "wh2j_c6.rom", 0x400000, 0x200000, 0xa43e4766 ) /* Plane 2,3 */
	ROM_LOAD( "wh2j_c8.rom", 0x600000, 0x200000, 0xfc092367 ) /* Plane 0,1 */

	NEO_BIOS_SOUND_128K( "wh2j_m1.rom", 0xd2eec9d3 )

	ROM_REGION_OPTIONAL(0x400000) /* sound samples */
	ROM_LOAD( "wh2j_v1.rom", 0x000000, 0x200000, 0xaa277109 )
	ROM_LOAD( "wh2j_v2.rom", 0x200000, 0x200000, 0xb6527edd )
ROM_END

ROM_START( magdrop3_rom )
	ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "drop3_p1.rom", 0x000000, 0x100000, 0x931e17fa )

	NEO_SFIX_128K( "drop3_s1.rom", 0x7399e68a )

	ROM_REGION(0x800000)
    ROM_LOAD( "drop3_c1.rom", 0x200000, 0x200000, 0x734db3d6 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "drop3_c3.rom", 0x600000, 0x200000, 0xec65f472 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )

	ROM_REGION(0x800000)
    ROM_LOAD( "drop3_c2.rom", 0x200000, 0x200000, 0xd78f50e5 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "drop3_c4.rom", 0x600000, 0x200000, 0xf55dddf3 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )

	NEO_BIOS_SOUND_128K( "drop3_m1.rom", 0x5beaf34e )

	ROM_REGION_OPTIONAL(0x480000) /* sound samples */
	ROM_LOAD( "drop3_v1.rom", 0x000000, 0x400000, 0x00000000 )	/* bad! odd bytes all 0xff */
	ROM_LOAD( "drop3_v2.rom", 0x400000, 0x080000, 0xd5e30df4 )
ROM_END

ROM_START( samsho2_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "sams2_p1.rom", 0x100000, 0x100000, 0x22368892 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "sams2_s1.rom", 0x64a5cd66 )

	ROM_REGION(0x800000)
    ROM_LOAD( "sams2_c1.rom", 0x000000, 0x200000, 0x86cd307c ) /* Plane 0,1 */
    ROM_LOAD( "sams2_c3.rom", 0x200000, 0x200000, 0x7a63ccc7 ) /* Plane 0,1 */
    ROM_LOAD( "sams2_c5.rom", 0x400000, 0x200000, 0x20d3a475 ) /* Plane 0,1 */
    ROM_LOAD( "sams2_c7.rom", 0x600000, 0x200000, 0x2df3cbcf ) /* Plane 0,1 */

	ROM_REGION(0x800000)
    ROM_LOAD( "sams2_c2.rom", 0x000000, 0x200000, 0xcdfcc4ca ) /* Plane 2,3 */
    ROM_LOAD( "sams2_c4.rom", 0x200000, 0x200000, 0x751025ce ) /* Plane 2,3 */
    ROM_LOAD( "sams2_c6.rom", 0x400000, 0x200000, 0xae4c0a88 ) /* Plane 2,3 */
    ROM_LOAD( "sams2_c8.rom", 0x600000, 0x200000, 0x1ffc6dfa ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "sams2_m1.rom", 0x56675098 )

	ROM_REGION_OPTIONAL(0x700000) /* sound samples */
	ROM_LOAD( "sams2_v1.rom", 0x000000, 0x200000, 0x37703f91 )
	ROM_LOAD( "sams2_v2.rom", 0x200000, 0x200000, 0x0142bde8 )
	ROM_LOAD( "sams2_v3.rom", 0x400000, 0x200000, 0xd07fa5ca )
	ROM_LOAD( "sams2_v4.rom", 0x600000, 0x100000, 0x24aab4bb )
ROM_END

ROM_START( sskick3_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "side3_p1.rom", 0x100000, 0x100000, 0x6bc27a3d )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "side3_s1.rom", 0x7626da34 )

	ROM_REGION(0x600000)
    ROM_LOAD( "side3_c1.rom", 0x000000, 0x200000, 0x1fb68ebe ) /* Plane 0,1 */
    ROM_LOAD( "side3_c3.rom", 0x200000, 0x200000, 0x3b2572e8 ) /* Plane 0,1 */
    ROM_LOAD( "side3_c5.rom", 0x400000, 0x200000, 0x17d42f0d ) /* Plane 0,1 */

	ROM_REGION(0x600000)
    ROM_LOAD( "side3_c2.rom", 0x000000, 0x200000, 0xb28d928f ) /* Plane 2,3 */
    ROM_LOAD( "side3_c4.rom", 0x200000, 0x200000, 0x47d26a7c ) /* Plane 2,3 */
    ROM_LOAD( "side3_c6.rom", 0x400000, 0x200000, 0x6b53fb75 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "side3_m1.rom", 0x82fcd863 )

	ROM_REGION_OPTIONAL(0x600000) /* sound samples */
	ROM_LOAD( "side3_v1.rom", 0x000000, 0x200000, 0x201fa1e1 )
	ROM_LOAD( "side3_v2.rom", 0x200000, 0x200000, 0xacf29d96 )
	ROM_LOAD( "side3_v3.rom", 0x400000, 0x200000, 0xe524e415 )
ROM_END

ROM_START( aof2_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "aof2_p1.rom", 0x000000, 0x100000, 0xa3b1d021 )

	NEO_SFIX_128K( "aof2_s1.rom", 0x8b02638e )

	ROM_REGION(0x800000)
    ROM_LOAD( "aof2_c1.rom", 0x000000, 0x200000, 0x17b9cbd2 ) /* Plane 0,1 */
    ROM_LOAD( "aof2_c3.rom", 0x200000, 0x200000, 0xd2c88768 ) /* Plane 0,1 */
    ROM_LOAD( "aof2_c5.rom", 0x400000, 0x200000, 0xc3074137 ) /* Plane 0,1 */
    ROM_LOAD( "aof2_c7.rom", 0x600000, 0x200000, 0x3f36df57 ) /* Plane 0,1 */

	ROM_REGION(0x800000)
    ROM_LOAD( "aof2_c2.rom", 0x000000, 0x200000, 0x5fd76b67 ) /* Plane 2,3 */
    ROM_LOAD( "aof2_c4.rom", 0x200000, 0x200000, 0xdb39b883 ) /* Plane 2,3 */
    ROM_LOAD( "aof2_c6.rom", 0x400000, 0x200000, 0x31de68d3 ) /* Plane 2,3 */
    ROM_LOAD( "aof2_c8.rom", 0x600000, 0x200000, 0xe546d7a8 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "aof2_m1.rom", 0xf27e9d52 )

	ROM_REGION_OPTIONAL(0x500000) /* sound samples */
	ROM_LOAD( "aof2_v1.rom", 0x000000, 0x200000, 0x4628fde0 )
	ROM_LOAD( "aof2_v2.rom", 0x200000, 0x200000, 0xb710e2f2 )
	ROM_LOAD( "aof2_v3.rom", 0x400000, 0x100000, 0xd168c301 )
ROM_END

ROM_START( aodk_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "aodk_p1.rom", 0x100000, 0x100000, 0x62369553 )
	ROM_CONTINUE(                      0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "aodk_s1.rom", 0x96148d2b )

	ROM_REGION(0x800000)
    ROM_LOAD( "aodk_c1.rom", 0x000000, 0x200000, 0xa0b39344 ) /* Plane 0,1 */
    ROM_LOAD( "aodk_c3.rom", 0x200000, 0x200000, 0x7fff4d41 ) /* Plane 0,1 */
    ROM_LOAD( "aodk_c5.rom", 0x400000, 0x200000, 0xc74c5e51 ) /* Plane 0,1 */
    ROM_LOAD( "aodk_c7.rom", 0x600000, 0x200000, 0xac7daa01 ) /* Plane 0,1 */

	ROM_REGION(0x800000)
    ROM_LOAD( "aodk_c2.rom", 0x000000, 0x200000, 0x203f6074 ) /* Plane 2,3 */
    ROM_LOAD( "aodk_c4.rom", 0x200000, 0x200000, 0x48db3e0a ) /* Plane 2,3 */
    ROM_LOAD( "aodk_c6.rom", 0x400000, 0x200000, 0x73e8e7e0 ) /* Plane 2,3 */
    ROM_LOAD( "aodk_c8.rom", 0x600000, 0x200000, 0x14e7ad71 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "aodk_m1.rom", 0x5a52a9d1 )

	ROM_REGION_OPTIONAL(0x400000) /* sound samples */
	ROM_LOAD( "aodk_v1.rom", 0x000000, 0x200000, 0x7675b8fa )
	ROM_LOAD( "aodk_v2.rom", 0x200000, 0x200000, 0xa9da86e9 )
ROM_END

ROM_START( kof94_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "kof94_p1.rom", 0x100000, 0x100000, 0xf10a2042 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "kof94_s1.rom", 0x825976c1 )

	ROM_REGION(0x800000)
    ROM_LOAD( "kof94_c1.rom", 0x000000, 0x200000, 0xb96ef460 ) /* Plane 0,1 */
    ROM_LOAD( "kof94_c3.rom", 0x200000, 0x200000, 0x54f66254 ) /* Plane 0,1 */
    ROM_LOAD( "kof94_c5.rom", 0x400000, 0x200000, 0xee759363 ) /* Plane 0,1 */
    ROM_LOAD( "kof94_c7.rom", 0x600000, 0x200000, 0x62f66888 ) /* Plane 0,1 */

	ROM_REGION(0x800000)
    ROM_LOAD( "kof94_c2.rom", 0x000000, 0x200000, 0x15e096a7 ) /* Plane 2,3 */
    ROM_LOAD( "kof94_c4.rom", 0x200000, 0x200000, 0x0b01765f ) /* Plane 2,3 */
    ROM_LOAD( "kof94_c6.rom", 0x400000, 0x200000, 0x498da52c ) /* Plane 2,3 */
    ROM_LOAD( "kof94_c8.rom", 0x600000, 0x200000, 0xfe0a235d ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "kof94_m1.rom", 0xf6e77cf5 )

    ROM_REGION_OPTIONAL(0x600000) /* sound samples */
    ROM_LOAD( "kof94_v1.rom", 0x000000, 0x200000, 0x8889596d )
    ROM_LOAD( "kof94_v2.rom", 0x200000, 0x200000, 0x25022b27 )
    ROM_LOAD( "kof94_v3.rom", 0x400000, 0x200000, 0x83cf32c0 )
ROM_END

ROM_START( savagere_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "savag_p1.rom", 0x100000, 0x100000, 0x01d4e9c0 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "savag_s1.rom", 0xe08978ca )

	ROM_REGION(0x800000)
    ROM_LOAD( "savag_c1.rom", 0x000000, 0x200000, 0x763ba611 ) /* Plane 0,1 */
    ROM_LOAD( "savag_c3.rom", 0x200000, 0x200000, 0x3e4eba4b ) /* Plane 0,1 */
    ROM_LOAD( "savag_c5.rom", 0x400000, 0x200000, 0x59013f9e ) /* Plane 0,1 */
    ROM_LOAD( "savag_c7.rom", 0x600000, 0x200000, 0xc88f7035 ) /* Plane 0,1 */

	ROM_REGION(0x800000)
    ROM_LOAD( "savag_c2.rom", 0x000000, 0x200000, 0xe05e8ca6 ) /* Plane 2,3 */
    ROM_LOAD( "savag_c4.rom", 0x200000, 0x200000, 0x3c2a3808 ) /* Plane 2,3 */
    ROM_LOAD( "savag_c6.rom", 0x400000, 0x200000, 0x1c8d5def ) /* Plane 2,3 */
    ROM_LOAD( "savag_c8.rom", 0x600000, 0x200000, 0x484ce3ba ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "savag_m1.rom", 0x29992eba )

	ROM_REGION_OPTIONAL(0x600000) /* sound samples */
	ROM_LOAD( "savag_v1.rom", 0x000000, 0x200000, 0x530c50fd )
	ROM_LOAD( "savag_v2.rom", 0x200000, 0x200000, 0xe79a9bd0 )
	ROM_LOAD( "savag_v3.rom", 0x400000, 0x200000, 0x7038c2f9 )
ROM_END

ROM_START( gowcaizr_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "vfgow_p1.rom", 0x100000, 0x100000, 0x33019545 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "vfgow_s1.rom", 0x2f8748a2 )

	ROM_REGION(0x800000)
    ROM_LOAD( "vfgow_c1.rom", 0x000000, 0x200000, 0x042f6af5 ) /* Plane 0,1 */
    ROM_LOAD( "vfgow_c3.rom", 0x200000, 0x200000, 0x58bfbaa1 ) /* Plane 0,1 */
    ROM_LOAD( "vfgow_c5.rom", 0x400000, 0x200000, 0xff9cf48c ) /* Plane 0,1 */
    ROM_LOAD( "vfgow_c7.rom", 0x600000, 0x200000, 0x2091ec04 ) /* Plane 0,1 */

	ROM_REGION(0x800000)
    ROM_LOAD( "vfgow_c2.rom", 0x000000, 0x200000, 0x0fbcd046 ) /* Plane 2,3 */
    ROM_LOAD( "vfgow_c4.rom", 0x200000, 0x200000, 0x9451ee73 ) /* Plane 2,3 */
    ROM_LOAD( "vfgow_c6.rom", 0x400000, 0x200000, 0x31bbd918 ) /* Plane 2,3 */
    ROM_LOAD( "vfgow_c8.rom", 0x600000, 0x200000, 0x0d31dee6 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "vfgow_m1.rom", 0x78c851cb )

	ROM_REGION_OPTIONAL(0x500000) /* sound samples */
	ROM_LOAD( "vfgow_v1.rom", 0x000000, 0x200000, 0x6c31223c )
	ROM_LOAD( "vfgow_v2.rom", 0x200000, 0x200000, 0x8edb776c )
	ROM_LOAD( "vfgow_v3.rom", 0x400000, 0x100000, 0xc63b9285 )
ROM_END

ROM_START( mslug_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "mslug_p1.rom", 0x100000, 0x100000, 0x08d8daa5 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "mslug_s1.rom", 0x2f55958d )

	ROM_REGION(0x800000)
    ROM_LOAD( "mslug_c1.rom", 0x200000, 0x200000, 0xd00bd152 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "mslug_c3.rom", 0x600000, 0x200000, 0xd3d5f9e5 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )

	ROM_REGION(0x800000)
    ROM_LOAD( "mslug_c2.rom", 0x200000, 0x200000, 0xddff1dea ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "mslug_c4.rom", 0x600000, 0x200000, 0x5ac1d497 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )

	NEO_BIOS_SOUND_128K( "mslug_m1.rom", 0xc28b3253 )

	ROM_REGION_OPTIONAL(0x800000) /* sound samples */
	ROM_LOAD( "mslug_v1.rom", 0x000000, 0x400000, 0x23d22ed1 )
	ROM_LOAD( "mslug_v2.rom", 0x400000, 0x400000, 0x472cf9db )
ROM_END

ROM_START( kabukikl_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "klash_p1.rom", 0x100000, 0x100000, 0x28ec9b77 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "klash_s1.rom", 0xa3d68ee2 )

	ROM_REGION(0x800000)
    ROM_LOAD( "klash_c1.rom", 0x200000, 0x200000, 0x4d896a58 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "klash_c3.rom", 0x600000, 0x200000, 0x58c454e7 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )

	ROM_REGION(0x800000)
    ROM_LOAD( "klash_c2.rom", 0x200000, 0x200000, 0x3cf78a18 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "klash_c4.rom", 0x600000, 0x200000, 0xe1a8aa6a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )

	NEO_BIOS_SOUND_128K( "klash_m1.rom", 0x91957ef6 )

	ROM_REGION_OPTIONAL(0x700000) /* sound samples */
	ROM_LOAD( "klash_v1.rom", 0x000000, 0x200000, 0x69e90596 )
	ROM_LOAD( "klash_v2.rom", 0x200000, 0x200000, 0x7abdb75d )
	ROM_LOAD( "klash_v3.rom", 0x400000, 0x200000, 0xeccc98d3 )
	ROM_LOAD( "klash_v4.rom", 0x600000, 0x100000, 0xa7c9c949 )
ROM_END

ROM_START( overtop_rom )
	ROM_REGION(0x200000)
    ROM_LOAD_WIDE_SWAP( "ovr_p1.rom", 0x100000, 0x100000, 0x16c063a9 )
	ROM_CONTINUE(                     0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "ovr_s1.rom",  0x481d3ddc )

	ROM_REGION(0xa00000)
    ROM_LOAD( "ovr_c1.rom", 0x000000, 0x400000, 0x50f43087 ) /* Plane 0,1 */
    ROM_LOAD( "ovr_c3.rom", 0x400000, 0x400000, 0x9252ea02 ) /* Plane 0,1 */
    ROM_LOAD( "ovr_c5.rom", 0x800000, 0x200000, 0xfc858bef ) /* Plane 0,1 */

	ROM_REGION(0xa00000)
    ROM_LOAD( "ovr_c2.rom", 0x000000, 0x400000, 0xa5b39807 ) /* Plane 2,3 */
    ROM_LOAD( "ovr_c4.rom", 0x400000, 0x400000, 0x5f41a699 ) /* Plane 2,3 */
    ROM_LOAD( "ovr_c6.rom", 0x800000, 0x200000, 0x0589c15e ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "ovr_m1.rom", 0xfcab6191 )

    ROM_REGION_OPTIONAL(0x400000) /* sound samples */
    ROM_LOAD( "ovr_v1.rom", 0x000000, 0x400000, 0x013d4ef9 )
ROM_END

ROM_START( fatfury3_rom )
	ROM_REGION(0x300000)
    ROM_LOAD_WIDE_SWAP( "fury3_p1.rom", 0x000000, 0x100000, 0xc7b743eb )
    ROM_LOAD_WIDE_SWAP( "fury3_p2.rom", 0x100000, 0x200000, 0xdbe963ed )

	NEO_SFIX_128K( "fury3_s1.rom", 0x0b33a800 )

	ROM_REGION(0xa00000)
    ROM_LOAD( "fury3_c1.rom", 0x200000, 0x200000, 0xc73e86e4 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "fury3_c3.rom", 0x600000, 0x200000, 0xf6738c87 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
    ROM_LOAD( "fury3_c5.rom", 0x800000, 0x200000, 0xb3ec6fa6 ) /* Plane 0,1 */

	ROM_REGION(0xa00000)
    ROM_LOAD( "fury3_c2.rom", 0x200000, 0x200000, 0xbfaf3258 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
    ROM_LOAD( "fury3_c4.rom", 0x600000, 0x200000, 0x9c31e334 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
    ROM_LOAD( "fury3_c6.rom", 0x800000, 0x200000, 0x69210441 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "fury3_m1.rom", 0xfce72926 )

	ROM_REGION_OPTIONAL(0xa00000) /* sound samples */
	ROM_LOAD( "fury3_v1.rom", 0x000000, 0x400000, 0x2bdbd4db )
	ROM_LOAD( "fury3_v2.rom", 0x400000, 0x400000, 0xa698a487 )
	ROM_LOAD( "fury3_v3.rom", 0x800000, 0x200000, 0x581c5304 )
ROM_END

ROM_START( wakuwak7_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "waku7_p1.rom", 0x000000, 0x100000, 0xb14da766 )
	ROM_LOAD_WIDE_SWAP( "waku7_p2.rom", 0x100000, 0x200000, 0xfe190665 )

	NEO_SFIX_128K( "waku7_s1.rom", 0x71c4b4b5 )

	ROM_REGION(0x0c00000)
	ROM_LOAD( "waku7_c1.rom", 0x200000, 0x200000, 0xd91d386f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "waku7_c3.rom", 0x600000, 0x200000, 0x02fcff2f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "waku7_c5.rom", 0xa00000, 0x200000, 0x03d32f25 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )

	ROM_REGION(0x0c00000)
	ROM_LOAD( "waku7_c2.rom", 0x200000, 0x200000, 0x36b5cf41 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "waku7_c4.rom", 0x600000, 0x200000, 0xcd7f1241 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "waku7_c6.rom", 0xa00000, 0x200000, 0xd996a90a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )

	NEO_BIOS_SOUND_128K( "waku7_m1.rom", 0x0634bba6 )

	ROM_REGION_OPTIONAL(0x800000) /* sound samples */
	ROM_LOAD( "waku7_v1.rom", 0x000000, 0x400000, 0x00000000 )	/* bad read: odd bytes all 0xff */
	ROM_LOAD( "waku7_v2.rom", 0x400000, 0x400000, 0x00000000 )	/* bad read: odd bytes all 0xff */
ROM_END

ROM_START( samsho3_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "sams3_p1.rom", 0x000000, 0x100000, 0x282a336e )
	ROM_LOAD_WIDE_SWAP( "sams3_p2.rom", 0x100000, 0x200000, 0x9bbe27e0 )

	NEO_SFIX_128K( "sams3_s1.rom", 0x74ec7d9f )

    ROM_REGION(0x0d00000)	/* lowering this to 0xc80000 corrupts the graphics */
	ROM_LOAD( "sams3_c1.rom", 0x200000, 0x200000, 0xe079f767 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "sams3_c3.rom", 0x600000, 0x200000, 0xc61218d7 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "sams3_c5.rom", 0xa00000, 0x200000, 0x05feee47 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
    ROM_LOAD( "sams3_c7.rom", 0xc00000, 0x080000, 0x7a01f666 ) /* Plane 0,1 */

    ROM_REGION(0x0d00000)
	ROM_LOAD( "sams3_c2.rom", 0x200000, 0x200000, 0xfc045909 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "sams3_c4.rom", 0x600000, 0x200000, 0x054ec754 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "sams3_c6.rom", 0xa00000, 0x200000, 0xef7d9e29 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
    ROM_LOAD( "sams3_c8.rom", 0xc00000, 0x080000, 0xffd009c2 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "sams3_m1.rom", 0x8e6440eb )

	ROM_REGION_OPTIONAL(0x600000) /* sound samples */
	ROM_LOAD( "sams3_v1.rom", 0x000000, 0x400000, 0x00000000 )	/* bad read: odd bytes all 0xff */
	ROM_LOAD( "sams3_v2.rom", 0x400000, 0x200000, 0xac0f261a )
ROM_END

ROM_START( kof95_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "kof95_p1.rom", 0x100000, 0x100000, 0x5e54cf95 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "kof95_s1.rom", 0xde716f8a )

	ROM_REGION(0x0d00000)
	ROM_LOAD( "kof95_c1.rom", 0x200000, 0x200000, 0x33bf8657 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "kof95_c3.rom", 0x600000, 0x200000, 0x0cee1ddb ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "kof95_c5.rom", 0x800000, 0x200000, 0x8a2c1edc ) /* Plane 0,1 */
	/* a00000-bfffff empty */
	ROM_LOAD( "kof95_c7.rom", 0xc00000, 0x100000, 0x9904025f ) /* Plane 0,1 */

	ROM_REGION(0x0d00000)
	ROM_LOAD( "kof95_c2.rom", 0x200000, 0x200000, 0xf21908a4 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "kof95_c4.rom", 0x600000, 0x200000, 0x729db15d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "kof95_c6.rom", 0x800000, 0x200000, 0xf593ac35 ) /* Plane 2,3 */
	/* a00000-bfffff empty */
	ROM_LOAD( "kof95_c8.rom", 0xc00000, 0x100000, 0x78eb0f9b ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "kof95_m1.rom", 0x6f2d7429 )

	ROM_REGION_OPTIONAL(0x900000) /* sound samples */
	ROM_LOAD( "kof95_v1.rom", 0x000000, 0x400000, 0x21469561 )
	ROM_LOAD( "kof95_v2.rom", 0x400000, 0x200000, 0xb38a2803 )
	/* 600000-7fffff empty */
	ROM_LOAD( "kof95_v3.rom", 0x800000, 0x100000, 0xd683a338 )
ROM_END

ROM_START( kizuna_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "ke_p1.rom", 0x100000, 0x100000, 0x75d2b3de )
	ROM_CONTINUE(                    0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "ke_s1.rom",   0xefdc72d7 )

    ROM_REGION(0xe00000)
    ROM_LOAD( "ke_c1.rom", 0x000000, 0x200000, 0x763ba611 ) /* Plane 0,1 */
	/* 200000-3fffff empty */
    ROM_LOAD( "ke_c3.rom", 0x400000, 0x400000, 0x665c9f16 ) /* Plane 0,1 */
    ROM_LOAD( "ke_c5.rom", 0x800000, 0x200000, 0x59013f9e ) /* Plane 0,1 */
	/* a00000-bfffff empty */
    ROM_LOAD( "ke_c7.rom", 0xc00000, 0x200000, 0xc88f7035 ) /* Plane 0,1 */

    ROM_REGION(0xe00000)
    ROM_LOAD( "ke_c2.rom", 0x000000, 0x200000, 0xe05e8ca6 ) /* Plane 2,3 */
	/* 200000-3fffff empty */
    ROM_LOAD( "ke_c4.rom", 0x400000, 0x400000, 0x7f5d03db ) /* Plane 2,3 */
    ROM_LOAD( "ke_c6.rom", 0x800000, 0x200000, 0x1c8d5def ) /* Plane 2,3 */
	/* a00000-bfffff empty */
    ROM_LOAD( "ke_c8.rom", 0xc00000, 0x200000, 0x484ce3ba ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "ke_m1.rom", 0x1b096820 )

    ROM_REGION_OPTIONAL(0x800000) /* sound samples */
    ROM_LOAD( "ke_v1.rom", 0x000000, 0x200000, 0x530c50fd )
    ROM_LOAD( "ke_v2.rom", 0x200000, 0x200000, 0x03667a8d )
    ROM_LOAD( "ke_v3.rom", 0x400000, 0x200000, 0x7038c2f9 )
    ROM_LOAD( "ke_v4.rom", 0x600000, 0x200000, 0x31b99bd6 )
ROM_END

ROM_START( aof3_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "aof3_p1.rom", 0x000000, 0x100000, 0x9edb420d )
	ROM_LOAD_WIDE_SWAP( "aof3_p2.rom", 0x100000, 0x200000, 0x4d5a2602 )

	NEO_SFIX_128K( "aof3_s1.rom", 0xcc7fd344 )

	ROM_REGION(0x0e00000)
	ROM_LOAD( "aof3_c1.rom", 0x200000, 0x200000, 0xf6c74731 ) /* Plane 0,1 */
	ROM_CONTINUE(            0x000000, 0x200000 )
	ROM_LOAD( "aof3_c3.rom", 0x600000, 0x200000, 0x7749f5e6 ) /* Plane 0,1 */
	ROM_CONTINUE(            0x400000, 0x200000 )
	ROM_LOAD( "aof3_c5.rom", 0xa00000, 0x200000, 0x1718bdcd ) /* Plane 0,1 */
	ROM_CONTINUE(            0x800000, 0x200000 )
	ROM_LOAD( "aof3_c7.rom", 0xc00000, 0x200000, 0x51bd8ab2 ) /* Plane 0,1 */

	ROM_REGION(0x0e00000)
	ROM_LOAD( "aof3_c2.rom", 0x200000, 0x200000, 0xf587f149 ) /* Plane 2,3 */
	ROM_CONTINUE(            0x000000, 0x200000 )
	ROM_LOAD( "aof3_c4.rom", 0x600000, 0x200000, 0xcbd58369 ) /* Plane 2,3 */
	ROM_CONTINUE(            0x400000, 0x200000 )
	ROM_LOAD( "aof3_c6.rom", 0xa00000, 0x200000, 0x4fca967f ) /* Plane 2,3 */
	ROM_CONTINUE(            0x800000, 0x200000 )
	ROM_LOAD( "aof3_c8.rom", 0xc00000, 0x200000, 0x9a34f99c ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "aof3_m1.rom", 0xcb07b659 )

	ROM_REGION_OPTIONAL(0x600000) /* sound samples */
	ROM_LOAD( "aof3_v1.rom", 0x000000, 0x200000, 0xe2c32074 )
	ROM_LOAD( "aof3_v2.rom", 0x200000, 0x200000, 0xa290eee7 )
	ROM_LOAD( "aof3_v3.rom", 0x400000, 0x200000, 0x199d12ea )
ROM_END

ROM_START( pulstar_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "pstar_p1.rom", 0x000000, 0x100000, 0x5e5847a2 )
	ROM_LOAD_WIDE_SWAP( "pstar_p2.rom", 0x100000, 0x200000, 0x028b774c )

	NEO_SFIX_128K( "pstar_s1.rom", 0xc79fc2c8 )

	ROM_REGION(0xe00000)
	ROM_LOAD( "pstar_c1.rom", 0x200000, 0x200000, 0x63020fc6 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "pstar_c3.rom", 0x600000, 0x200000, 0x21ef41d7 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "pstar_c5.rom", 0xa00000, 0x200000, 0x6fe9259c ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "pstar_c7.rom", 0xc00000, 0x200000, 0x6a5618ca ) /* Plane 0,1 */

	ROM_REGION(0xe00000)
	ROM_LOAD( "pstar_c2.rom", 0x200000, 0x200000, 0x260e9d4d ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "pstar_c4.rom", 0x600000, 0x200000, 0x3b9e288f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "pstar_c6.rom", 0xa00000, 0x200000, 0xdc32f2b4 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "pstar_c8.rom", 0xc00000, 0x200000, 0xa223572d ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "pstar_m1.rom", 0xff3df7c7 )

	ROM_REGION_OPTIONAL(0x800000) /* sound samples */
	ROM_LOAD( "pstar_v1.rom", 0x000000, 0x400000, 0x00000000 )	/* bad! odd bytes all 0xff */
	ROM_LOAD( "pstar_v2.rom", 0x400000, 0x400000, 0x00000000 )	/* bad! odd bytes all 0xff */
ROM_END

ROM_START( rbff1_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "rbff1_p1.rom", 0x000000, 0x100000, 0x63b4d8ae )
	ROM_LOAD_WIDE_SWAP( "rbff1_p2.rom", 0x100000, 0x200000, 0xcc15826e )

	NEO_SFIX_128K( "rbff1_s1.rom", 0xb6bf5e08 )

	ROM_REGION(0xe00000)
	ROM_LOAD( "rbff1_c1.rom", 0x200000, 0x200000, 0xc73e86e4 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "rbff1_c3.rom", 0x600000, 0x200000, 0xf6738c87 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "rbff1_c5.rom", 0xa00000, 0x200000, 0x248ff860 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "rbff1_c7.rom", 0xc00000, 0x200000, 0xca605e12 ) /* Plane 0,1 */

	ROM_REGION(0xe00000)
	ROM_LOAD( "rbff1_c2.rom", 0x200000, 0x200000, 0xbfaf3258 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "rbff1_c4.rom", 0x600000, 0x200000, 0x9c31e334 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "rbff1_c6.rom", 0xa00000, 0x200000, 0x0bfb2d1f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "rbff1_c8.rom", 0xc00000, 0x200000, 0x4e6beb6c ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "rbff1_m1.rom", 0x653492a7 )

	ROM_REGION_OPTIONAL(0xc00000) /* sound samples */
	ROM_LOAD( "rbff1_v1.rom", 0x000000, 0x400000, 0x00000000 )	/* bad! odd bytes all 0xff */
	ROM_LOAD( "rbff1_v2.rom", 0x400000, 0x400000, 0x00000000 )	/* bad! odd bytes all 0xff */
	ROM_LOAD( "rbff1_v3.rom", 0x800000, 0x400000, 0x00000000 )	/* bad! odd bytes all 0xff */
ROM_END

ROM_START( whp_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "whp_p1.rom", 0x100000, 0x100000, 0xafaa4702 )
	ROM_CONTINUE(                     0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "whp_s1.rom",  0x174a880f )

    ROM_REGION(0xe00000)
    ROM_LOAD( "whp_c1.rom", 0x200000, 0x200000, 0xaecd5bb1 )
	ROM_CONTINUE(           0x000000, 0x200000 )
    ROM_LOAD( "whp_c3.rom", 0x400000, 0x200000, 0x436d1b31 )
    ROM_RELOAD(             0x600000, 0x200000 )
    ROM_LOAD( "whp_c5.rom", 0x800000, 0x200000, 0x8e34a9f4 )
    ROM_RELOAD(             0xa00000, 0x200000 )
    ROM_LOAD( "whp_c7.rom", 0xc00000, 0x200000, 0x59d97215 )

    ROM_REGION(0xe00000)
    ROM_LOAD( "whp_c2.rom", 0x200000, 0x200000, 0x7566ffc0 )
	ROM_CONTINUE(           0x000000, 0x200000 )
    ROM_LOAD( "whp_c4.rom", 0x400000, 0x200000, 0xf9c8dd26 )
    ROM_RELOAD(             0x600000, 0x200000 )
    ROM_LOAD( "whp_c6.rom", 0x800000, 0x200000, 0xa43e4766 )
    ROM_RELOAD(             0xa00000, 0x200000 )
    ROM_LOAD( "whp_c8.rom", 0xc00000, 0x200000, 0xfc092367 )

    NEO_BIOS_SOUND_128K( "whp_m1.rom", 0x28065668 )

	ROM_REGION_OPTIONAL(0x600000) /* sound samples */
	ROM_LOAD( "whp_v1.rom", 0x000000, 0x200000, 0x30cf2709 )
	ROM_LOAD( "whp_v2.rom", 0x200000, 0x200000, 0xb6527edd )
	ROM_LOAD( "whp_v3.rom", 0x400000, 0x200000, 0x1908a7ce )
ROM_END

ROM_START( mslug2_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "ms2_p1.rom", 0x000000, 0x100000, 0x2a53c5da )
	ROM_LOAD_WIDE_SWAP( "ms2_p2.rom", 0x100000, 0x200000, 0x38883f44 )

	NEO_SFIX_128K( "ms2_s1.rom",  0xf3d32f0f )

	ROM_REGION(0x1000000)
	ROM_LOAD( "ms2_c1.rom", 0x0000000, 0x800000, 0x394b5e0d ) /* Plane 0,1 */
	ROM_LOAD( "ms2_c3.rom", 0x0800000, 0x800000, 0x9f6bfa6f ) /* Plane 0,1 */

	ROM_REGION(0x1000000)
	ROM_LOAD( "ms2_c2.rom", 0x0000000, 0x800000, 0xe5806221 ) /* Plane 2,3 */
	ROM_LOAD( "ms2_c4.rom", 0x0800000, 0x800000, 0x7d3e306f ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "ms2_m1.rom", 0x94520ebd )

	ROM_REGION_OPTIONAL(0x800000) /* sound samples */
	ROM_LOAD( "ms2_v1.rom", 0x000000, 0x400000, 0x99ec20e8 )
	ROM_LOAD( "ms2_v2.rom", 0x400000, 0x400000, 0xecb16799 )
ROM_END

ROM_START( ragnagrd_rom )
	ROM_REGION(0x200000)
	ROM_LOAD_WIDE_SWAP( "rgard_p1.rom", 0x100000, 0x100000, 0xca372303 )
	ROM_CONTINUE(                       0x000000, 0x100000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "rgard_s1.rom", 0x7d402f9a )

	ROM_REGION(0x1000000)
	ROM_LOAD( "rgard_c1.rom", 0x200000, 0x200000, 0x18f61d79 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "rgard_c3.rom", 0x600000, 0x200000, 0x108d5589 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "rgard_c5.rom", 0xa00000, 0x200000, 0x4b74021a ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "rgard_c7.rom", 0xe00000, 0x200000, 0x32189762 ) /* Plane 0,1 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x1000000)
	ROM_LOAD( "rgard_c2.rom", 0x200000, 0x200000, 0xdbf4ff4b ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "rgard_c4.rom", 0x600000, 0x200000, 0x7962d5ac ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "rgard_c6.rom", 0xa00000, 0x200000, 0xf5cf90bc ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "rgard_c8.rom", 0xe00000, 0x200000, 0xd5915828 ) /* Plane 2,3 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	NEO_BIOS_SOUND_128K( "rgard_m1.rom", 0x17028bcf )

	ROM_REGION_OPTIONAL(0x800000) /* sound samples */
	ROM_LOAD( "rgard_v1.rom", 0x000000, 0x400000, 0x00000000 )	/* bad read: odd bytes all 0xff */
	ROM_LOAD( "rgard_v2.rom", 0x400000, 0x400000, 0x00000000 )	/* bad read: odd bytes all 0xff */
ROM_END

ROM_START( blazstar_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "bstar_p1.rom", 0x000000, 0x100000, 0x183682f8 )
	ROM_LOAD_WIDE_SWAP( "bstar_p2.rom", 0x100000, 0x200000, 0x9a9f4154 )

	NEO_SFIX_128K( "bstar_s1.rom", 0xd56cb498 )

	ROM_REGION(0x1000000)
	ROM_LOAD( "bstar_c1.rom", 0x200000, 0x200000, 0x754744e0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "bstar_c3.rom", 0x600000, 0x200000, 0x7b39b590 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "bstar_c5.rom", 0xa00000, 0x200000, 0x9ceb113b ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "bstar_c7.rom", 0xe00000, 0x200000, 0x50d28eca ) /* Plane 0,1 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x1000000)
	ROM_LOAD( "bstar_c2.rom", 0x200000, 0x200000, 0xaf98c037 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "bstar_c4.rom", 0x600000, 0x200000, 0x6e731b30 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "bstar_c6.rom", 0xa00000, 0x200000, 0x6a78e810 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "bstar_c8.rom", 0xe00000, 0x200000, 0xcdbbb7d7 ) /* Plane 2,3 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	NEO_BIOS_SOUND_128K( "bstar_m1.rom", 0xd31a3aea )

	ROM_REGION_OPTIONAL(0x800000) /* sound samples */
	ROM_LOAD( "bstar_v1.rom", 0x000000, 0x400000, 0x1b8d5bf7 )
	ROM_LOAD( "bstar_v2.rom", 0x400000, 0x400000, 0x74cf0a70 )
ROM_END

ROM_START( ninjamas_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "ninjm_p1.rom", 0x000000, 0x100000, 0x3e97ed69 )
	ROM_LOAD_WIDE_SWAP( "ninjm_p2.rom", 0x100000, 0x200000, 0x191fca88 )

	NEO_SFIX_128K( "ninjm_s1.rom", 0x8ff782f0 )

	ROM_REGION(0x1000000)
	ROM_LOAD( "ninjm_c1.rom", 0x200000, 0x200000, 0x58f91ae0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "ninjm_c3.rom", 0x600000, 0x200000, 0x36c29ce3 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "ninjm_c5.rom", 0xa00000, 0x200000, 0x4683ffc0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "ninjm_c7.rom", 0xe00000, 0x200000, 0x3e1885c0 ) /* Plane 0,1 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x1000000)
	ROM_LOAD( "ninjm_c2.rom", 0x200000, 0x200000, 0x4258147f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "ninjm_c4.rom", 0x600000, 0x200000, 0x17e97a6e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "ninjm_c6.rom", 0xa00000, 0x200000, 0xde004f4a ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "ninjm_c8.rom", 0xe00000, 0x200000, 0x5a5df034 ) /* Plane 2,3 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	NEO_BIOS_SOUND_128K( "ninjm_m1.rom", 0xd00fb2af )

    ROM_REGION_OPTIONAL(0x600000) /* sound samples */
	ROM_LOAD( "ninjm_v1.rom", 0x000000, 0x400000, 0x00000000 )
    ROM_LOAD( "ninjm_v2.rom", 0x400000, 0x200000, 0x22f1c681 )
ROM_END

ROM_START( kof96_rom )
	ROM_REGION(0x300000)
	ROM_LOAD_WIDE_SWAP( "kof96_p1.rom", 0x000000, 0x100000, 0x52755d74 )
	ROM_LOAD_WIDE_SWAP( "kof96_p2.rom", 0x100000, 0x200000, 0x002ccb73 )

	NEO_SFIX_128K( "kof96_s1.rom", 0x1254cbdb )

    ROM_REGION(0x1000000)
    ROM_LOAD( "kof96_c1.rom", 0x000000, 0x400000, 0x7ecf4aa2 ) /* Plane 0,1 */
    ROM_LOAD( "kof96_c3.rom", 0x400000, 0x400000, 0x64989a65 ) /* Plane 0,1 */
    ROM_LOAD( "kof96_c5.rom", 0x800000, 0x400000, 0x2a3bbd26 ) /* Plane 0,1 */
    ROM_LOAD( "kof96_c7.rom", 0xc00000, 0x400000, 0x3687331b ) /* Plane 0,1 */

	ROM_REGION(0x1000000)
    ROM_LOAD( "kof96_c2.rom", 0x000000, 0x400000, 0x05b54f37 ) /* Plane 2,3 */
    ROM_LOAD( "kof96_c4.rom", 0x400000, 0x400000, 0xafbea515 ) /* Plane 2,3 */
    ROM_LOAD( "kof96_c6.rom", 0x800000, 0x400000, 0x44d30dc7 ) /* Plane 2,3 */
    ROM_LOAD( "kof96_c8.rom", 0xc00000, 0x400000, 0xfa1461ad ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "kof96_m1.rom", 0xdabc427c )

	ROM_REGION_OPTIONAL(0xa00000) /* sound samples */
	ROM_LOAD( "kof96_v1.rom", 0x000000, 0x400000, 0x63f7b045 )
	ROM_LOAD( "kof96_v2.rom", 0x400000, 0x400000, 0x25929059 )
	ROM_LOAD( "kof96_v3.rom", 0x800000, 0x200000, 0x92a2257d )
ROM_END

ROM_START( rbffspec_rom )
	ROM_REGION(0x500000)
	ROM_LOAD_WIDE_SWAP( "rbffs_p1.rom", 0x000000, 0x100000, 0xf84a2d1d )
	ROM_LOAD_WIDE_SWAP( "rbffs_p2.rom", 0x300000, 0x200000, 0x27e3e54b )
	ROM_CONTINUE(                       0x100000, 0x200000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "rbffs_s1.rom", 0x7ecd6e8c )

	ROM_REGION(0x1000000)
	ROM_LOAD( "rbffs_c1.rom", 0x200000, 0x200000, 0x436edad4 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "rbffs_c3.rom", 0x600000, 0x200000, 0x375954ea ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "rbffs_c5.rom", 0xa00000, 0x200000, 0x12c5418e ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "rbffs_c7.rom", 0xe00000, 0x200000, 0x5c33d1d8 ) /* Plane 0,1 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x1000000)
	ROM_LOAD( "rbffs_c2.rom", 0x200000, 0x200000, 0xcc7dc384 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "rbffs_c4.rom", 0x600000, 0x200000, 0xc1a98dd7 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "rbffs_c6.rom", 0xa00000, 0x200000, 0xc8ad71d5 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "rbffs_c8.rom", 0xe00000, 0x200000, 0xefdeb140 ) /* Plane 2,3 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	NEO_BIOS_SOUND_128K( "rbffs_m1.rom", 0x3fee46bf )

	ROM_REGION_OPTIONAL(0xc00000) /* sound samples */
	ROM_LOAD( "rbffs_v1.rom", 0x000000, 0x400000, 0x76673869 )
	ROM_LOAD( "rbffs_v2.rom", 0x400000, 0x400000, 0x7a275acd )
	ROM_LOAD( "rbffs_v3.rom", 0x800000, 0x400000, 0x5a797fd2 )
ROM_END

ROM_START( samsho4_rom )
	ROM_REGION(0x500000)
	ROM_LOAD_WIDE_SWAP( "sams4_p1.rom", 0x000000, 0x100000, 0x1a5cb56d )
	ROM_LOAD_WIDE_SWAP( "sams4_p2.rom", 0x300000, 0x200000, 0x7587f09b )
	ROM_CONTINUE(                       0x100000, 0x200000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "sams4_s1.rom", 0x8d3d3bf9 )

	ROM_REGION(0x1000000)
	ROM_LOAD( "sams4_c1.rom", 0x200000, 0x200000, 0x289100fa ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "sams4_c3.rom", 0x600000, 0x200000, 0x6659734f ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "sams4_c5.rom", 0xa00000, 0x200000, 0xe22254ed ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "sams4_c7.rom", 0xe00000, 0x200000, 0xe3e3b0cd ) /* Plane 0,1 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x1000000)
	ROM_LOAD( "sams4_c2.rom", 0x200000, 0x200000, 0xc2716ea0 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "sams4_c4.rom", 0x600000, 0x200000, 0x91ebea00 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "sams4_c6.rom", 0xa00000, 0x200000, 0x00947b2e ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "sams4_c8.rom", 0xe00000, 0x200000, 0xf33967f1 ) /* Plane 2,3 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	NEO_BIOS_SOUND_128K( "sams4_m1.rom", 0x7615bc1b )

	ROM_REGION_OPTIONAL(0xa00000) /* sound samples */
	ROM_LOAD( "sams4_v1.rom", 0x000000, 0x400000, 0x7d6ba95f )
	ROM_LOAD( "sams4_v2.rom", 0x400000, 0x400000, 0x6c33bb5d )
	ROM_LOAD( "sams4_v3.rom", 0x800000, 0x200000, 0x831ea8c0 )
ROM_END

ROM_START( shocktro_rom )
	ROM_REGION(0x500000)
	ROM_LOAD_WIDE_SWAP( "shock_p1.rom", 0x000000, 0x100000, 0x5677456f )
	ROM_LOAD_WIDE_SWAP( "shock_p2.rom", 0x300000, 0x200000, 0x646f6c76 )
	ROM_CONTINUE(                       0x100000, 0x200000 | ROMFLAG_WIDE | ROMFLAG_SWAP )

	NEO_SFIX_128K( "shock_s1.rom", 0x1f95cedb )

	ROM_REGION(0x1000000)
	ROM_LOAD( "shock_c1.rom", 0x200000, 0x200000, 0xaad087fc ) /* Plane 0,1 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "shock_c3.rom", 0x600000, 0x200000, 0x6682a458 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "shock_c5.rom", 0xa00000, 0x200000, 0xe17762b1 ) /* Plane 0,1 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "shock_c7.rom", 0xe00000, 0x200000, 0xa47e62d2 ) /* Plane 0,1 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	ROM_REGION(0x1000000)
	ROM_LOAD( "shock_c2.rom", 0x200000, 0x200000, 0x7e39df1f ) /* Plane 2,3 */
	ROM_CONTINUE(             0x000000, 0x200000 )
	ROM_LOAD( "shock_c4.rom", 0x600000, 0x200000, 0xcbef1f17 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x400000, 0x200000 )
	ROM_LOAD( "shock_c6.rom", 0xa00000, 0x200000, 0x28beab71 ) /* Plane 2,3 */
	ROM_CONTINUE(             0x800000, 0x200000 )
	ROM_LOAD( "shock_c8.rom", 0xe00000, 0x200000, 0xe8e890fb ) /* Plane 2,3 */
	ROM_CONTINUE(             0xc00000, 0x200000 )

	NEO_BIOS_SOUND_128K( "shock_m1.rom", 0x075b9518 )

	ROM_REGION_OPTIONAL(0x600000) /* sound samples */
	ROM_LOAD( "shock_v1.rom", 0x000000, 0x400000, 0x00000000 )	/* bad read: odd bytes all 0xff */
	ROM_LOAD( "shock_v2.rom", 0x400000, 0x200000, 0x4ad7d59e )
ROM_END

ROM_START( kof97_rom )
	ROM_REGION(0x500000)
	ROM_LOAD_WIDE_SWAP( "kof97_p1.rom", 0x000000, 0x100000, 0x7db81ad9 )
	ROM_LOAD_WIDE_SWAP( "kof97_p2.rom", 0x100000, 0x400000, 0x158b23f6 )

	NEO_SFIX_128K( "kof97_s1.rom", 0x8514ecf5 )

	ROM_REGION(0x1400000)
	ROM_LOAD( "kof97_c1.rom", 0x0000000, 0x800000, 0x5f8bf0a1 ) /* Plane 0,1 */
	ROM_LOAD( "kof97_c3.rom", 0x0800000, 0x800000, 0x581d6618 ) /* Plane 0,1 */
	ROM_LOAD( "kof97_c5.rom", 0x1000000, 0x400000, 0x34fc4e51 ) /* Plane 0,1 */

	ROM_REGION(0x1400000)
	ROM_LOAD( "kof97_c2.rom", 0x0000000, 0x800000, 0xe4d45c81 ) /* Plane 2,3 */
	ROM_LOAD( "kof97_c4.rom", 0x0800000, 0x800000, 0x49bb1e68 ) /* Plane 2,3 */
	ROM_LOAD( "kof97_c6.rom", 0x1000000, 0x400000, 0x4ff4d47b ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "kof97_m1.rom", 0x45348747 )

	ROM_REGION_OPTIONAL(0xc00000) /* sound samples */
	ROM_LOAD( "kof97_v1.rom", 0x000000, 0x400000, 0x22a2b5b5 )
	ROM_LOAD( "kof97_v2.rom", 0x400000, 0x400000, 0x2304e744 )
	ROM_LOAD( "kof97_v3.rom", 0x800000, 0x400000, 0x759eb954 )
ROM_END

ROM_START( realbou2_rom )
	ROM_REGION(0x500000)
	ROM_LOAD_WIDE_SWAP( "rb2_p1.rom", 0x000000, 0x100000, 0xb6969780 )
	ROM_LOAD_WIDE_SWAP( "rb2_p2.rom", 0x100000, 0x400000, 0x960aa88d )

	NEO_SFIX_128K( "rb2_s1.rom",  0xda3b40de )

	ROM_REGION(0x1800000)
	ROM_LOAD( "rb2_c1.rom", 0x0000000, 0x800000, 0xeffac504 ) /* Plane 0,1 */
	ROM_LOAD( "rb2_c3.rom", 0x0800000, 0x800000, 0x22e0330a ) /* Plane 0,1 */
	ROM_LOAD( "rb2_c5.rom", 0x1000000, 0x800000, 0x244dff5a ) /* Plane 0,1 */

	ROM_REGION(0x1800000)
	ROM_LOAD( "rb2_c2.rom", 0x0000000, 0x800000, 0xed182d44 ) /* Plane 2,3 */
	ROM_LOAD( "rb2_c4.rom", 0x0800000, 0x800000, 0xc19a07eb ) /* Plane 2,3 */
	ROM_LOAD( "rb2_c6.rom", 0x1000000, 0x800000, 0x4609e507 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "rb2_m1.rom", 0x00000000 )	/* second half is missing */

    ROM_REGION_OPTIONAL(0x1000000) /* sound samples */
	ROM_LOAD( "rb2_v1.rom", 0x000000, 0x400000, 0x00000000 )	/* bad read: odd bytes all 0xff */
	ROM_LOAD( "rb2_v2.rom", 0x400000, 0x400000, 0x00000000 )	/* bad read: odd bytes all 0xff */
	ROM_LOAD( "rb2_v3.rom", 0x800000, 0x400000, 0x00000000 )	/* bad read: odd bytes all 0xff */
	ROM_LOAD( "rb2_v4.rom", 0xc00000, 0x400000, 0x00000000 )	/* bad read: in 1st half, odd bytes all 0xff */
ROM_END

ROM_START( gururin_rom )
    ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "gurin_p1.rom", 0x000000, 0x40000, 0x102787c0 )

	NEO_SFIX_128K( "gurin_s1.rom", 0x4f0cbd58 )

    ROM_REGION(0x200000)
    ROM_LOAD( "gurin_c1.rom", 0x0000000, 0x200000, 0x35866126 ) /* Plane 0,1 */

    ROM_REGION(0x200000)
    ROM_LOAD( "gurin_c2.rom", 0x0000000, 0x200000, 0x9db64084 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_64K( "gurin_m1.rom", 0x833cdf1b )

    ROM_REGION_OPTIONAL(0x80000) /* sound samples */
    ROM_LOAD( "gurin_v1.rom", 0x000000, 0x80000, 0xcf23afd0 )
ROM_END

ROM_START( magdrop2_rom )
    ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "drop2_p1.rom", 0x000000, 0x80000, 0x7be82353 )

	NEO_SFIX_128K( "drop2_s1.rom", 0x2a4063a3 )

    ROM_REGION(0x400000)
    ROM_LOAD( "drop2_c1.rom", 0x0000000, 0x400000, 0x1f862a14 ) /* Plane 0,1 */

    ROM_REGION(0x400000)
    ROM_LOAD( "drop2_c2.rom", 0x0000000, 0x400000, 0x14b90536 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "drop2_m1.rom", 0xbddae628 )

    ROM_REGION_OPTIONAL(0x200000) /* sound samples */
    ROM_LOAD( "drop2_v1.rom", 0x000000, 0x200000, 0x7e5e53e4 )
ROM_END

ROM_START( miexchng_rom )
    ROM_REGION(0x100000)
    ROM_LOAD_WIDE_SWAP( "miex-p1.rom", 0x000000, 0x80000, 0x61be1810 )

	NEO_SFIX_128K( "miex-s1.rom", 0xfe0c0c53 )

    ROM_REGION(0x280000)
    ROM_LOAD( "miex-c1.rom", 0x0000000, 0x200000, 0x6c403ba3 ) /* Plane 0,1 */
    ROM_LOAD( "miex-c3.rom", 0x0200000, 0x080000, 0x14524eb5 ) /* Plane 0,1 */

    ROM_REGION(0x280000)
    ROM_LOAD( "miex-c2.rom", 0x0000000, 0x200000, 0x554bcd9b ) /* Plane 2,3 */
    ROM_LOAD( "miex-c4.rom", 0x0200000, 0x080000, 0x1694f171 ) /* Plane 2,3 */

    NEO_BIOS_SOUND_64K( "miex-m1.rom", 0xf5316be1 )

    ROM_REGION_OPTIONAL(0x400000) /* sound samples */
    ROM_LOAD( "miex-v1.rom", 0x000000, 0x400000, 0x00000000 )
ROM_END

ROM_START( lastblad_rom )
    ROM_REGION(0x500000)
    ROM_LOAD_WIDE_SWAP( "lb_p1.rom", 0x000000, 0x100000, 0xcd01c06d )
    ROM_LOAD_WIDE_SWAP( "lb_p2.rom", 0x100000, 0x400000, 0x0fdc289e )

	NEO_SFIX_128K( "lb_s1.rom", 0x95561412 )

    ROM_REGION(0x1200000)
    ROM_LOAD( "lb_c1.rom", 0x0000000, 0x800000, 0x9f7e2bd3 ) /* Plane 0,1 */
    ROM_LOAD( "lb_c3.rom", 0x0800000, 0x800000, 0x91ab1a30 ) /* Plane 0,1 */
    ROM_LOAD( "lb_c5.rom", 0x1000000, 0x200000, 0x17bbd7ca ) /* Plane 0,1 */

    ROM_REGION(0x1200000)
    ROM_LOAD( "lb_c2.rom", 0x0000000, 0x800000, 0x80623d3c ) /* Plane 2,3 */
    ROM_LOAD( "lb_c4.rom", 0x0800000, 0x800000, 0x3d60b037 ) /* Plane 2,3 */
    ROM_LOAD( "lb_c6.rom", 0x1000000, 0x200000, 0x5c35d541 ) /* Plane 2,3 */

	NEO_BIOS_SOUND_128K( "lb_m1.rom", 0x087628ea )

    ROM_REGION_OPTIONAL(0xe00000) /* sound samples */
    ROM_LOAD( "lb_v1.rom", 0x000000, 0x400000, 0xed66b76f )
    ROM_LOAD( "lb_v2.rom", 0x400000, 0x400000, 0xa0e7f6e2 )
    ROM_LOAD( "lb_v3.rom", 0x800000, 0x400000, 0xa506e1e2 )
    ROM_LOAD( "lb_v4.rom", 0xc00000, 0x200000, 0x13583c4b )
ROM_END

/******************************************************************************/

/* dummy entry for the dummy bios driver */
ROM_START( bios_rom )
	ROM_REGION(0x020000)
	ROM_LOAD_WIDE_SWAP( "neo-geo.rom", 0x00000, 0x020000, 0x9036d879 )

	ROM_REGION(0x020000)
	ROM_LOAD( "ng-sfix.rom",  0x00000, 0x20000, 0x354029fc )
 	ROM_LOAD( "ng-sm1.rom",   0x20000, 0x20000, 0x97cf998b )
ROM_END

/******************************************************************************/

#define NGCRED "The Shin Emu Keikaku team"

/* Inspired by the CPS1 driver, compressed version of GameDrivers */
#define NEODRIVER(NEO_NAME,NEO_REALNAME,NEO_YEAR,NEO_MANU,NEO_CREDIT,NEO_MACHINE) \
struct GameDriver NEO_NAME##_driver  = \
{                               \
	__FILE__,                   \
	&neogeo_bios,               \
	#NEO_NAME,                  \
	NEO_REALNAME,               \
	NEO_YEAR,                   \
	NEO_MANU,                   \
	NGCRED,          \
	0,                          \
	NEO_MACHINE,                \
    neogeo_onetime_init_machine,		\
	NEO_NAME##_rom,             \
	0, 0,           \
	0,                          \
	0, 	    	                \
	neogeo_ports,               \
	0, 0, 0,                    \
	ORIENTATION_DEFAULT,        \
	neogeo_sram_load,neogeo_sram_save  \
};

/* Use this macro when the romset name starts with a number */
#define NEODRIVERX(NEO_NAME,NEO_ROMNAME,NEO_REALNAME,NEO_YEAR,NEO_MANU,NEO_CREDIT,NEO_MACHINE) \
struct GameDriver NEO_NAME##_driver  = \
{                               \
	__FILE__,                   \
	&neogeo_bios,               \
	NEO_ROMNAME,                \
	NEO_REALNAME,               \
	NEO_YEAR,                   \
	NEO_MANU,                   \
	NGCRED,          \
	0,                          \
	NEO_MACHINE,                \
	neogeo_onetime_init_machine,		\
	NEO_NAME##_rom,             \
	0, 0,           \
	0,                          \
	0, 	    	                \
	neogeo_ports,               \
	0, 0, 0,                    \
	ORIENTATION_DEFAULT,        \
	neogeo_sram_load,neogeo_sram_save  \
};

/* A dummy driver, so that the bios can be debugged, and to serve as */
/* parent for the neo-geo.rom file, so that we do not have to include */
/* it in every zip file */
struct GameDriver neogeo_bios =
{
	__FILE__,
	0,
	"neogeo",
	"NeoGeo BIOS - NOT A REAL DRIVER",
	"19??",
	"SNK",
	"Do NOT link this from driver.c",
	0,
	&neogeo_mvs_machine_driver, /* Dummy */
    neogeo_onetime_init_machine,

	bios_rom,
	0, 0,
	0,
	0,      /* sound_prom */
	neogeo_ports,
	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	0,0
};

/******************************************************************************/

/* MGD2 format dumps */

/* SNK */
NEODRIVER(joyjoy,  "Puzzled / Joy Joy Kid","1990","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(ridhero, "Riding Hero","1990","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(bstars,  "Baseball Stars Professional","1990","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(lbowling,"League Bowling","1990","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(superspy,"Super Spy","1990","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVERX(ttbb,   "2020bb","2020 Super Baseball","1991","SNK / Pallas","",&neogeo_mgd2_machine_driver)
NEODRIVER(alpham2, "Alpha Mission 2 / ASD Last Guardian 2","1991","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(eightman,"Eight Man","1991","SNK / Pallas","",&neogeo_mgd2_machine_driver)
NEODRIVER(ararmy,  "Robo Army","1991","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(fatfury1,"Fatal Fury - King of Fighters / Garou Densetsu","1991","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(burningf,"Burning Fight","1991","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(kingofm, "King of the Monsters","1991","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(gpilots, "Ghost Pilots","1991","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(sengoku, "Sengoku","1991","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(lresort, "Last Resort","1992","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(fbfrenzy,"Football Frenzy","1992","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(mutnat,  "Mutation Nation","1992","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(artfight,"Art of Fighting / Ryuu Ko No Ken","1992","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVERX(countb, "3countb","3 Count Bout / Fire Suplex","1993","SNK","",&neogeo_mgd2_machine_driver)
NEODRIVER(sengoku2,"Sengoku 2","1993","SNK","",&neogeo_mgd2_machine_driver)

/* Alpha Denshi Co */
NEODRIVER(ncombat, "Ninja Combat","1990","Alpha Denshi Co","",&neogeo_mgd2_machine_driver)
NEODRIVER(crsword, "Crossed Swords","1991","Alpha Denshi Co","",&neogeo_mgd2_machine_driver)
NEODRIVER(trally,  "Thrash Rally","1991","Alpha Denshi Co","",&neogeo_mgd2_machine_driver)
NEODRIVER(ncommand,"Ninja Commando","1992","Alpha Denshi Co","",&neogeo_mgd2_machine_driver)
NEODRIVER(wheroes, "World Heroes","1992","Alpha Denshi Co","",&neogeo_mgd2_machine_driver)

/* Visco */
NEODRIVER(androdun,"Andro Dunos","1992","Visco","",&neogeo_mgd2_machine_driver)

/******************************************************************************/

/* MVS format dumps, I started to rearrange these to be in order - Manufacturer/Date, it makes
them out of sync with the order above, but this file is so large you need to use the search
function of the editor to find stuff anyway :) */

/* Alpha Denshi Co */
NEODRIVER(bjourney,"Blues Journey / Raguy","1990","Alpha Denshi Co","TJ Grant",&neogeo_mvs_machine_driver)
NEODRIVER(maglord, "Magician Lord","1990","Alpha Denshi Co","",&neogeo_mvs_machine_driver)

/* Aicom */
NEODRIVER(janshin, "Janshin Densetsu - Quest of Jongmaster","1994","Aicom","",&neogeo_mvs_machine_driver)
NEODRIVER(pulstar, "Pulstar","1995","Aicom","",&neogeo_mvs_machine_driver)

/* Yumekobo */
NEODRIVER(blazstar,"Blazing Star","1998","Yumekobo","",&neogeo_mvs_machine_driver)

/* Taito */
NEODRIVER(pbobble, "Puzzle Bobble / Bust-A-Move","1994","Taito","",&neogeo_mvs_machine_driver)
NEODRIVER(puzzledp,"Puzzle de Pon","1995","Taito (Visco license)","",&neogeo_mvs_machine_driver)

/* Visco */
NEODRIVER(pspikes2,"Power Spikes 2","1994","Video System Co.","",&neogeo_mvs_machine_driver)
NEODRIVER(sonicwi2,"Aero Fighters 2 / Sonic Wings 2","1994","Video System Co.","Santeri Saarimaa\nAtila Mert",&neogeo_mvs_machine_driver)
NEODRIVER(sonicwi3,"Aero Fighters 3 / Sonic Wings 3","1995","Video System Co.","",&neogeo_mvs_machine_driver)
NEODRIVER(goalx3,  "Goal! Goal! Goal!","1995","Visco","",&neogeo_mvs_machine_driver)
NEODRIVER(neodrift,"Neo Drift Out - New Technology","1996","Visco","Marko Pusljar (SI)",&neogeo_mvs_machine_driver)
NEODRIVER(neomrdo, "Neo Mr Do!","1996","Visco","",&neogeo_mvs_machine_driver)

/* Data East Corporation */
NEODRIVER(spinmast,"Spinmaster / Miracle Adventure","1993","Data East Corporation","",&neogeo_mvs_machine_driver)
NEODRIVER(karnov_r,"Karnov's Revenge / Fighter's History Dynamite","1994","Data East Corporation","",&neogeo_mvs_machine_driver)
NEODRIVER(wjammers,"Wind Jammers / Flying Power Disc","1994","Data East Corporation","TJ Grant",&neogeo_mvs_machine_driver)
NEODRIVER(strhoops,"Street Hoop / Street Slam / Dunk Dream","1994","Data East Corporation","",&neogeo_mvs_machine_driver)
NEODRIVER(magdrop2,"Magical Drop 2 / Toretate Zoukangou 2","1996","Data East Corporation","Santeri Saarimaa",&neogeo_mvs_machine_driver)
NEODRIVER(magdrop3,"Magical Drop 3 / Toretate Zoukangou III","1997","Data East Corporation","",&neogeo_mvs_machine_driver)

/* Nazca */
NEODRIVER(mslug,   "Metal Slug","1996","Nazca","",&neogeo_mvs_machine_driver)
NEODRIVER(turfmast,"Neo Turf Masters / Big Tournament Golf","1996","Nazca","",&neogeo_mvs_machine_driver)

/* Hudson Soft */
NEODRIVER(panicbom,"Panic Bomber","1994","Eighting / Hudson","Rodimus Prime",&neogeo_mvs_machine_driver)
NEODRIVER(kabukikl,"Kabuki Klash / Far East of Eden","1995","Hudson","Santeri Saarimaa",&neogeo_mvs_machine_driver)
NEODRIVER(neobombe,"Neo Bomberman","1997","Hudson","",&neogeo_mvs_machine_driver)

/* ADK */
NEODRIVER(worlher2,"World Heroes 2","1993","ADK","",&neogeo_mvs_machine_driver)
NEODRIVER(worlhe2j,"World Heroes 2 Jet","1994","ADK/SNK","Santeri Saarimaa",&neogeo_mvs_machine_driver)
NEODRIVER(aodk,    "Aggressors of Dark Kombat / Tougai Gan San","1994","ADK/SNK","Santeri Saarimaa\nRodimus Prime",&neogeo_mvs_machine_driver)
NEODRIVER(whp,     "World Heroes Perfect","1995","ADK/SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(ninjamas,"Ninja Master's","1996","ADK/SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(overtop, "Overtop","1996","ADK","Santeri Saarimaa",&neogeo_mvs_machine_driver)
NEODRIVER(twinspri,"Twinkle Star Sprites","1996","ADK","Santeri Saarimaa",&neogeo_mvs_machine_driver)

/* Saurus */
NEODRIVER(stakwin, "Stakes Winner","1995","Saurus","Santeri Saarimaa",&neogeo_mvs_machine_driver)
NEODRIVER(ragnagrd,"Operation Ragnagard / Shin Oh Ken","1996","Saurus","",&neogeo_mvs_machine_driver)
NEODRIVER(shocktro,"Shock Troopers","1997","Saurus","",&neogeo_mvs_machine_driver)

/* Tecmo */
NEODRIVER(tws96,   "Tecmo World Soccer 96","1996","Tecmo","Santeri Saarimaa",&neogeo_mvs_machine_driver)

/* NMK */
NEODRIVER(zedblade,"Zed Blade / Operation Ragnarok","1994","NMK","Santeri Saarimaa",&neogeo_mvs_machine_driver)

/* Technos */
NEODRIVER(doubledr,"Double Dragon (Neo Geo)","1995","Technos","Santeri Saarimaa\nAtila Mert",&neogeo_mvs_machine_driver)
NEODRIVER(gowcaizr,"Voltage Fighter Gowcaizer","1995","Technos","",&neogeo_mvs_machine_driver)

/* Sunsoft */
NEODRIVER(galaxyfg,"Galaxy Fight - Universal Warriors","1995","Sunsoft","Santeri Saarimaa",&neogeo_mvs_machine_driver)
NEODRIVER(wakuwak7,"Waku Waku 7","1996","Sunsoft","",&neogeo_mvs_machine_driver)

/* Sammy */
NEODRIVER(viewpoin,"Viewpoint","1992","Sammy","",&neogeo_mvs_machine_driver)

/* Face */
NEODRIVER(gururin, "Gururin","1994","Face","Santeri Saarimaa",&neogeo_mvs_machine_driver)
NEODRIVER(miexchng,"Money Puzzle Exchanger","1997","Face","Santeri Saarimaa",&neogeo_mvs_machine_driver)

/* SNK */
NEODRIVER(nam_1975,"NAM 1975","1990","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(mahretsu,"Mahjong Kyo Retsuden","1990","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(cyberlip,"Cyber-Lip","1990","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(tpgolf,  "Top Player's Golf","1990","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(legendos,"Legend of Success Joe / Ashita No Joe Densetsu","1991","SNK","Santeri Saarimaa",&neogeo_mvs_machine_driver)
NEODRIVER(socbrawl,"Soccer Brawl","1991","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(fatfury2,"Fatal Fury 2","1992","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(bstars2, "Baseball Stars 2","1992","SNK","Santeri Saarimaa",&neogeo_mvs_machine_driver)
NEODRIVER(sidkicks,"Super Sidekicks","1992","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(kotm2,   "King of the Monsters 2","1992","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(samsho,  "Samurai Shodown / Samurai Spirits","1993","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(fatfursp,"Fatal Fury Special","1993","SNK","Santeri Saarimaa",&neogeo_mvs_machine_driver)
NEODRIVER(tophuntr,"Top Hunter","1994","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(kof94,   "The King of Fighters '94","1994","SNK","Santeri Saarimaa",&neogeo_mvs_machine_driver)
NEODRIVER(aof2,    "Art of Fighting 2 / Ryuu Ko No Ken 2","1994","SNK","Santeri Saarimaa",&neogeo_mvs_machine_driver)
NEODRIVER(ssideki2,"Super Sidekicks 2","1994","SNK","Santeri Saarimaa",&neogeo_mvs_machine_driver)
NEODRIVER(samsho2, "Samurai Shodown 2 / Samurai Spirits 2","1994","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(sskick3, "Super Sidekicks 3 - The Next Glory","1995","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(savagere,"Savage Reign","1995","SNK","Rodimus Prime",&neogeo_mvs_machine_driver)
NEODRIVER(samsho3, "Samurai Shodown 3 / Samurai Spirits 3","1995","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(fatfury3,"Fatal Fury 3","1995","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(kof95,   "The King of Fighters '95","1995","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(rbff1,   "Real Bout Fatal Fury","1995","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(aof3,    "Art of Fighting 3 / Ryuu Ko No Ken 3","1996","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(kof96,   "The King of Fighters '96","1996","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(samsho4, "Samurai Shodown 4 / Samurai Spirits 4","1996","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(rbffspec,"Real Bout Fatal Fury Special","1996","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(kizuna,  "Kizuna Encounter Super Tag Battle","1996","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(kof97,   "The King of Fighters '97","1997","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(lastblad,"Last Blade / Gekka No Kenshi","1997","SNK","Santeri Saarimaa",&neogeo_mvs_machine_driver)
NEODRIVER(mslug2,  "Metal Slug 2","1998","SNK","",&neogeo_mvs_machine_driver)
NEODRIVER(realbou2,"Real Bout Fatal Fury 2 - The Newcomers","1998","SNK","",&neogeo_mvs_machine_driver)

/* Note:  The information above is pretty accurate now, but they may still be
   some mistakes, check it if you have time :)

   General notes to future rom submitters :

   * MAKE SURE you get the date and manufacturer right :>

   * If the game has different names in Japan/America or Home/Arcade then put
     both in eg, "Samurai Shodown 2 / Samurai Spirits 2"
*/

