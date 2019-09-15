

/*
Taito Gladiator (1986)
Known ROM SETS: Golden Castle, Ohgon no Siro

Credits:
- Victor Trucco: original emulation and MAME driver
- Steve Ellenoff: YM2203 Sound, ADPCM Sound, dip switch fixes, high score save,
		  input port patches, panning fix, sprite banking,
		  Golden Castle Rom Set Support
- Phil Stroffolino: palette, sprites, misc video driver fixes
- Nicola Salmoria: Japaneese Rom Set Support(Ohgon no Siro)
- Tatsuyuki Satoh: YM2203 sound improvements, NEC 8741 emulation


special thanks to:
- Camilty for precious hardware information and screenshots
- Jason Richmond for hardware information and misc. notes
- Joe Rounceville for schematics
- and everyone else who'se offered support along the way!

Issues:
- YM2203 mixing problems (loss of bass notes)
- YM2203 some sound effects just don't sound correct
- Audio Filter Switch not hooked up (might solve YM2203 mixing issue)
- Ports 60,61,80,81 not fully understood yet...
- CPU speed may not be accurate
- some sprites linger on later stages (perhaps a sprite enable bit?)
- Flipscreen not implemented
- Scrolling issues in Test mode!
- Sample @ 5500 Hz being used because Mame core doesn't yet support multiple sample rates.
- The four 8741 ROMs are available but not used.

Preliminary Gladiator Memory Map

Main CPU (Z80)

$0000-$3FFF	QB0-5
$4000-$5FFF	QB0-4

$6000-$7FFF	QB0-1 Paged
$8000-$BFFF	QC0-3 Paged

    if port 02 = 00     QB0-1 offset (0000-1fff) at location 6000-7fff
                        QC0-3 offset (0000-3fff) at location 8000-bfff

    if port 02 = 01     QB0-1 offset (2000-3fff) at location 6000-7fff
                        QC0-3 offset (4000-7fff) at location 8000-bfff

$C000-$C3FF	sprite RAM
$C400-$C7FF	sprite attributes
$C800-$CBFF	more sprite attributes

$CC00-$D7FF	video registers

(scrolling, 2 screens wide)
$D800-DFFF	background layer VRAM (tiles)
$E000-E7FF	background layer VRAM (attributes)
$E800-EFFF	foreground text layer VRAM

$F000-$F3FF	Battery Backed RAM
$F400-$F7FF	Work RAM

Audio CPU (Z80)
$0000-$3FFF	QB0-17
$8000-$83FF	Work RAM 2.CPU


Preliminary Descriptions of I/O Ports.

Main z80
8 pins of LS259:
  00 - OBJACS ? (I can't read the name in schematics)
  01 - OBJCGBK (Sprite banking)
  02 - PR.BK (ROM banking)
  03 - NMIFG (connects to NMI of main Z80, but there's no code in 0066)
  04 - SRST (probably some type of reset)
  05 - CBK0 (unknown)
  06 - LOBJ (connected near graphic ROMs)
  07 - REVERS
  9E - Send data to NEC 8741-0 (comunicate with 2nd z80)
		(dip switch 1 is read in these ports too)
  9F - Send commands to NEC 8741-0

  C0-DF 8251 (Debug port ?)

2nd z80

00 - YM2203 Control Reg.
01 - YM2203 Data Read / Write Reg.
		Port B of the YM2203 is connected to dip switch 3
20 - Send data to NEC 8741-1 (comunicate with Main z80)
		(dip switch 2 is read in these ports too)
21 - Send commands to NEC 8741-1 (comunicate with Main z80)
40 - Clear Interrupt latch
60 - Send data to NEC 8741-2 (Read Joystick and Coin Slot (both players)
61 - Send commands to NEC 8741-2
80 - Send data to NEC 8741-3 (Read buttons (Fire 1, 2 and 3 (both players), service button) )
81 - Send commands to NEC 8741-3

A0-BF  - Audio mixer control ?
E0     - Comunication port to 6809
*/

#include "driver.h"
#include "vidhrdw/generic.h"

/*Video functions*/
extern unsigned char *gladiator_text;
extern void gladiatr_video_registers_w( int offset, int data );
extern int gladiatr_video_registers_r( int offset );
extern void gladiatr_paletteram_rg_w( int offset, int data );
extern void gladiatr_paletteram_b_w( int offset, int data );
extern int gladiatr_vh_start(void);
extern void gladiatr_vh_stop(void);
extern void gladiatr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern void gladiatr_spritebank_w( int offset, int data );

/*Sound and I/O Functions*/
void glad_adpcm_w( int channel, int data );
void glad_cpu_sound_command_w(int offset,int data);

/*Rom bankswitching*/
static int banka;
static int port9f;
void gladiatr_bankswitch_w(int offset,int data);
int gladiatr_bankswitch_r(int offset);


/* NEC 8741 program mode */
#define GLADIATOR_8741_MASTER 0
#define GLADIATOR_8741_SLAVE  1
#define GLADIATOR_8741_PORT   2

typedef struct gladiator_8741_status{
	unsigned char rdata;
	unsigned char status;
	unsigned char mode;
	unsigned char txd[8];
	unsigned char rxd[8];
	unsigned char parallelselect;
	unsigned char txpoint;
	unsigned char connect;
	unsigned char pending_4a;
	int (*portHandler)(unsigned char num);
}I8741;

static I8741 gladiator_8741[4];

/*Rom bankswitching*/
void gladiatr_bankswitch_w(int offset,int data){
	static int bank1[2] = { 0x10000, 0x12000 };
	static int bank2[2] = { 0x14000, 0x18000 };
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	banka = data;
	cpu_setbank(1,&RAM[bank1[(data & 0x03)]]);
	cpu_setbank(2,&RAM[bank2[(data & 0x03)]]);
}

int gladiatr_bankswitch_r(int offset){
	return banka;
}

static int gladiator_dsw1_r(unsigned char num)
{
	int orig = readinputport(0); /* DSW1 */
/*Reverse all bits for Input Port 0*/
/*ie..Bit order is: 0,1,2,3,4,5,6,7*/
return   ((orig&0x01)<<7) | ((orig&0x02)<<5)
       | ((orig&0x04)<<3) | ((orig&0x08)<<1)
       | ((orig&0x10)>>1) | ((orig&0x20)>>3)
       | ((orig&0x40)>>5) | ((orig&0x80)>>7);;
}

static int gladiator_dsw2_r(unsigned char num)
{
	int orig = readinputport(1); /* DSW2 */
/*Bits 2-7 are reversed for Input Port 1*/
/*ie..Bit order is: 2,3,4,5,6,7,1,0*/
return	  (orig&0x01) | (orig&0x02)
	| ((orig&0x04)<<5) | ((orig&0x08)<<3)
	| ((orig&0x10)<<1) | ((orig&0x20)>>1)
	| ((orig&0x40)>>3) | ((orig&0x80)>>5);
}

static int gladiator_controll_r(unsigned char num)
{
	int coins = 0;

	if( readinputport(4) & 0xc0 ) coins = 0x80;
	switch(num)
	{
	case 0x01: /* start button , coins */
		return readinputport(3) | coins;
	case 0x02: /* Player 1 Controller , coins */
		return readinputport(5) | coins;
	case 0x04: /* Player 2 Controller , coins */
		return readinputport(6) | coins;
	}
	/* unknown */
	return 0;
}

static int gladiator_button3_r(unsigned char num)
{
	switch(num)
	{
	case 0x01: /* button 3 */
		return readinputport(7);
	}
	/* unknown */
	return 0;
}

void gladiator_machine_init(void)
{
	int i;

	memset( gladiator_8741,0,sizeof(gladiator_8741));
	/* connect I8741 MASTER and SLAVE */
	gladiator_8741[0].connect = 1;
	gladiator_8741[1].connect = 0;
	for(i=0;i<4;i++)
	{
		gladiator_8741[i].status = 0x00;
	}
	gladiator_8741[0].portHandler = gladiator_dsw1_r; /* DSW1 */
	gladiator_8741[1].portHandler = gladiator_dsw2_r; /* DSW2 */
	/* button controller */
	gladiator_8741[2].portHandler = gladiator_button3_r; /* button3 */
	/* joystick controller */
	gladiator_8741[3].portHandler = gladiator_controll_r;
}

#if 1
/* !!!!! patch to IRQ timming for 2nd CPU !!!!! */
void gladiatr_irq_patch_w(int offset,int data)
{
	cpu_cause_interrupt(1,Z80_INT_REQ);
}
#endif

/* YM2203 port A handler (input) */
static int gladiator_dsw3_r(int offset)
{
	return input_port_2_r(offset)^0xff;
}
/* YM2203 port B handler (output) */
static void gladiator_int_controll_w(int offer , int data)
{
	/* bit 7   : SSRST = sound reset ? */
	/* bit 6-1 : N.C.                  */
	/* bit 0   : ??                    */
}
/* YM2203 IRQ */
static void gladiator_ym_irq(void)
{
	/* NMI IRQ is not used by gladiator sound program */
	cpu_cause_interrupt(1,Z80_NMI_INT);
}

/* gladiator I8741 emulation */

/* exchange data with serial port MASTER and SLAVE chip */
static void I8741_exchange_serial_data(int num)
{
	I8741 *mst = &gladiator_8741[num];
	I8741 *sst = &gladiator_8741[mst->connect];
	int i;
	int rp,wp;

	/* master -> slave transfer */
	rp = 0;
	sst->rxd[rp++] = mst->portHandler ? mst->portHandler(0) : 0;
	for( wp = 0 ; wp < 6 ; wp++)
		sst->rxd[rp++] = mst->txd[wp];

	/* slave -> master transfer */
	rp = 0;
	mst->rxd[rp++] = sst->portHandler ? sst->portHandler(0) : 0;
	for( wp = 0 ; wp < 6 ; wp++)
		mst->rxd[rp++] = sst->txd[wp];
}

/* read status port */
int I8741_status_r(int num)
{
	I8741 *st = &gladiator_8741[num];
#if 0
	if(errorlog) fprintf(errorlog,"8741-%d ST Read %02x PC=%04x\n",num,st->status,cpu_getpc());
#endif
	return st->status;
}

/* read data port */
int I8741_data_r(int num)
{
	I8741 *st = &gladiator_8741[num];
	int ret = st->rdata;
	st->status &= 0xfe;
#if 0
	if(errorlog) fprintf(errorlog,"8741-%d DATA Read %02x PC=%04x\n",num,ret,cpu_getpc());
#endif

	switch( st->mode )
	{
	case GLADIATOR_8741_PORT: /* parallel data */
		st->rdata = st->portHandler ? st->portHandler(st->parallelselect) : 0;
		st->status |= 0x01;
		break;
	}
	return ret;
}

/* Write data port */
void I8741_data_w(int num, int data)
{
	I8741 *st = &gladiator_8741[num];
#if 0
	if(errorlog) fprintf(errorlog,"8741-%d DATA Write %02x PC=%04x\n",num,data,cpu_getpc());
#endif
	switch( st->mode )
	{
	case GLADIATOR_8741_MASTER:
	case GLADIATOR_8741_SLAVE:
		/* buffering transmit data */
		if( st->txpoint < 8 )
		{
			st->txd[st->txpoint++] = data;
		}
		break;
	case GLADIATOR_8741_PORT:
		st->parallelselect = data;
		break;
	}
}

/* Write command port */
void I8741_command_w(int num, int data)
{
	I8741 *st = &gladiator_8741[num];
	I8741 *sst;

#if 0
	if(errorlog) fprintf(errorlog,"8741-%d CMD Write %02x PC=%04x\n",num,data,cpu_getpc());
#endif
	switch( data )
	{
	case 0x00: /* read parallel port */
		st->rdata = st->portHandler ? st->portHandler(0) : 0;
		st->status |= 0x01;
		break;
	case 0x01: /* read receive buffer 0 */
	case 0x02: /* read receive buffer 1 */
	case 0x03: /* read receive buffer 2 */
	case 0x04: /* read receive buffer 3 */
	case 0x05: /* read receive buffer 4 */
	case 0x06: /* read receive buffer 5 */
	case 0x07: /* read receive buffer 6 */
		st->rdata = st->rxd[data-1];
		st->status |= 0x01;
		break;
	case 0x08:	/* fix up transnit data */
		if( st->mode == GLADIATOR_8741_SLAVE )
		{
			I8741_exchange_serial_data(num);
			sst = &gladiator_8741[st->connect];
			/* release MASTER command 4a holding */
			if( sst->pending_4a )
			{
				sst->pending_4a = 0;
				sst->rdata   = 0x00; /* return code */
				sst->status |= 0x01;
			}
		}
		st->txpoint = 0;
		break;
	case 0x0a:	/* 8741-0 : set serial comminucation mode 'MASTER' */
		st->mode = GLADIATOR_8741_MASTER;
		break;
	case 0x0b:	/* 8741-1 : set serial comminucation mode 'SLAVE'  */
		st->mode = GLADIATOR_8741_SLAVE;
		break;
	case 0x1f:  /* 8741-2,3 : ?? set parallelport mode ?? */
	case 0x3f:  /* 8741-2,3 : ?? set parallelport mode ?? */
	case 0xe1:  /* 8741-2,3 : ?? set parallelport mode ?? */
		st->mode = GLADIATOR_8741_PORT;
		st->parallelselect = 1; /* preset read number */
		break;
	case 0x62:  /* 8741-3   : ? */
		break;
	case 0x4a:	/* ?? syncronus with other cpu and return 00H */
		st->pending_4a = 1;
		if( st->mode == GLADIATOR_8741_MASTER )
		{
			sst = &gladiator_8741[st->connect];
			/* release SLAVE command 4a holding */
			if( sst->pending_4a )
			{
				sst->pending_4a = 0;
				sst->rdata   = 0x00; /* return code */
				sst->status |= 0x01;
			}
		}
		break;
	case 0x80:	/* 8741-3 : return check code */
		st->rdata = 0x66;
		st->status |= 0x01;
		break;
	case 0x81:	/* 8741-2 : return check code */
		st->rdata = 0x48;
		st->status |= 0x01;
		break;
	}
}

/* Write data port */
void gladiator_8741_0_w(int offset, int data)
{
	if(offset&1) I8741_command_w(0,data);
	else         I8741_data_w(0,data);
}
void gladiator_8741_1_w(int offset, int data)
{
	if(offset&1) I8741_command_w(1,data);
	else         I8741_data_w(1,data);
}
void gladiator_8741_2_w(int offset, int data)
{
	if(offset&1) I8741_command_w(2,data);
	else         I8741_data_w(2,data);
}
void gladiator_8741_3_w(int offset, int data)
{
	if(offset&1) I8741_command_w(3,data);
	else         I8741_data_w(3,data);
}

/* read data port */
int gladiator_8741_0_r(int offset)
{
	int ret;
	if(offset&1) ret = I8741_status_r(0);
	else         ret = I8741_data_r(0);
	return ret;
}
int gladiator_8741_1_r(int offset)
{
	int ret;
	if(offset&1) ret = I8741_status_r(1);
	else         ret = I8741_data_r(1);
	return ret;
}
int gladiator_8741_2_r(int offset)
{
	int ret;
	if(offset&1) ret = I8741_status_r(2);
	else         ret = I8741_data_r(2);
	return ret;
}
int gladiator_8741_3_r(int offset)
{
	int ret;
	if(offset&1) ret = I8741_status_r(3);
	else         ret = I8741_data_r(3);
	return ret;
}

/*Sound Functions*/
void glad_adpcm_w( int channel, int data ) {
int command = data-0x03;
	/*If sample # is 4,7,8 use the samples which playback at 5500Hz*/
	if( command == 0x04 || command == 0x07 || command == 0x08)
		sample_start(0,command==0x04?0:command==0x07?1:2,0);
	else
		ADPCM_trigger(0,command);
}

void glad_cpu_sound_command_w(int offset,int data)
{
	soundlatch_w(0,data);
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x7fff, MRA_BANK1},
	{ 0x8000, 0xbfff, MRA_BANK2},
	{ 0xc000, 0xcbff, MRA_RAM },
	{ 0xcc00, 0xcfff, gladiatr_video_registers_r },
	{ 0xd000, 0xffff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcbff, MWA_RAM, &spriteram },
	{ 0xcc00, 0xcfff, gladiatr_video_registers_w },
	{ 0xd000, 0xd1ff, gladiatr_paletteram_rg_w, &paletteram },
	{ 0xd200, 0xd3ff, MWA_RAM },
	{ 0xd400, 0xd5ff, gladiatr_paletteram_b_w, &paletteram_2 },
	{ 0xd600, 0xd7ff, MWA_RAM },
	{ 0xd800, 0xdfff, videoram_w, &videoram },
	{ 0xe000, 0xe7ff, colorram_w, &colorram },
	{ 0xe800, 0xefff, MWA_RAM, &gladiator_text },
	{ 0xf000, 0xf3ff, MWA_RAM }, /* battery backed RAM */
	{ 0xf400, 0xffff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem_cpu2[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem_cpu2[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x8000, 0x83ff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ -1 }  /* end of table */
};


static struct IOReadPort readport[] =
{
	{ 0x02, 0x02, gladiatr_bankswitch_r },
	{ 0x9e, 0x9f, gladiator_8741_0_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x01, 0x01, gladiatr_spritebank_w},
	{ 0x02, 0x02, gladiatr_bankswitch_w},
	{ 0x04, 0x04, gladiatr_irq_patch_w}, /* !!! patch to 2nd CPU IRQ !!! */
	{ 0x9e, 0x9f, gladiator_8741_0_w },
	{ 0xbf, 0xbf, IORP_NOP },
	{ -1 }	/* end of table */
};

static struct IOReadPort readport_cpu2[] =
{
	{ 0x00, 0x00, YM2203_status_port_0_r },
	{ 0x01, 0x01, YM2203_read_port_0_r },
	{ 0x20, 0x21, gladiator_8741_1_r },
	{ 0x40, 0x40, IOWP_NOP },
	{ 0x60, 0x61, gladiator_8741_2_r },
	{ 0x80, 0x81, gladiator_8741_3_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport_cpu2[] =
{
	{ 0x00, 0x00, YM2203_control_port_0_w },
	{ 0x01, 0x01, YM2203_write_port_0_w },
	{ 0x20, 0x21, gladiator_8741_1_w },
	{ 0x60, 0x61, gladiator_8741_2_w },
	{ 0x80, 0x81, gladiator_8741_3_w },
/*	{ 0x40, 0x40, glad_sh_irq_clr }, */
	{ 0xe0, 0xe0, glad_adpcm_w },
	{ -1 }	/* end of table */
};

INPUT_PORTS_START( input_ports )
	PORT_START		/* DSW1 (8741-0 parallel port)*/
	PORT_DIPNAME( 0x03, 0x01, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x01, "Medium" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x03, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "After 4 Stages", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Continues" )
	PORT_DIPSETTING(    0x00, "Ends" )
        PORT_DIPNAME( 0x08, 0x00, "Bonus Life", IP_KEY_NONE )   /*NOTE: Actual manual has these settings reversed(typo?)! */
        PORT_DIPSETTING(    0x00, "Only at 100000" )
        PORT_DIPSETTING(    0x08, "Every 100000" )
	PORT_DIPNAME( 0x30, 0x20, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x20, "3" )
	PORT_DIPSETTING(    0x30, "4" )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* DSW2  (8741-1 parallel port) - Dips 6 Unused */
	PORT_DIPNAME( 0x03, 0x00, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x02, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/5 Credits" )
	PORT_DIPNAME( 0x0c, 0x00, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPNAME( 0x10, 0x00, "Free Play", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x00, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START      /* DSW3 (YM2203 port B) - Dips 5,6,7 Unused */
	PORT_BITX(    0x01, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Memory Backup", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x02, "Clear" )
	PORT_DIPNAME( 0x0c, 0x00, "Starting Stage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START	/* IN0 (8741-3 parallel port 1) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* COINS */

	PORT_START	/* COINS (8741-3 parallel port bit7) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE, "Coin A", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1)
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_COIN2 | IPF_IMPULSE, "Coin B", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1)

	PORT_START	/* IN1 (8741-3 parallel port 2) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* COINS */

	PORT_START	/* IN2 (8741-3 parallel port 4) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* COINS */

	PORT_START	/* IN3 (8741-2 parallel port 1) */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

/*******************************************************************/

static struct GfxLayout gladiator_text_layout  =   /* gfxset 0 */
{
	8,8,	/* 8*8 tiles */
	1024,	/* number of tiles */
	1,		/* bits per pixel */
	{ 0 },	/* plane offsets */
	{ 0,1,2,3,4,5,6,7 }, /* x offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 }, /* y offsets */
	64 /* offset to next tile */
};

/*******************************************************************/

#define DEFINE_LAYOUT( NAME,P0,P1,P2) static struct GfxLayout NAME = { \
	8,8,512,3, \
	{ P0, P1, P2}, \
	{ 0,1,2,3,64+0,64+1,64+2,64+3 }, \
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 }, \
	128 \
};

DEFINE_LAYOUT( gladiator_tile0, 4,			0x08000*8, 0x08000*8+4 )
DEFINE_LAYOUT( gladiator_tile1, 0,			0x0A000*8, 0x0A000*8+4 )
DEFINE_LAYOUT( gladiator_tile2, 4+0x2000*8, 0x10000*8, 0x10000*8+4 )
DEFINE_LAYOUT( gladiator_tile3, 0+0x2000*8, 0x12000*8, 0x12000*8+4 )
DEFINE_LAYOUT( gladiator_tile4, 4+0x4000*8, 0x0C000*8, 0x0C000*8+4 )
DEFINE_LAYOUT( gladiator_tile5, 0+0x4000*8, 0x0E000*8, 0x0E000*8+4 )
DEFINE_LAYOUT( gladiator_tile6, 4+0x6000*8, 0x14000*8, 0x14000*8+4 )
DEFINE_LAYOUT( gladiator_tile7, 0+0x6000*8, 0x16000*8, 0x16000*8+4 )
DEFINE_LAYOUT( gladiator_tileA, 4+0x2000*8, 0x0A000*8, 0x0A000*8+4 )
DEFINE_LAYOUT( gladiator_tileB, 0,			0x10000*8, 0x10000*8+4 )
DEFINE_LAYOUT( gladiator_tileC, 4+0x6000*8, 0x0E000*8, 0x0E000*8+4 )
DEFINE_LAYOUT( gladiator_tileD, 0+0x4000*8, 0x14000*8, 0x14000*8+4 )

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* monochrome text layer */
	{ 1, 0x34000, &gladiator_text_layout, 512, 1 },

	/* background tiles */
	{ 1, 0x18000, &gladiator_tile0, 0, 64 },
	{ 1, 0x18000, &gladiator_tile1, 0, 64 },
	{ 1, 0x18000, &gladiator_tile2, 0, 64 },
	{ 1, 0x18000, &gladiator_tile3, 0, 64 },
	{ 1, 0x18000, &gladiator_tile4, 0, 64 },
	{ 1, 0x18000, &gladiator_tile5, 0, 64 },
	{ 1, 0x18000, &gladiator_tile6, 0, 64 },
	{ 1, 0x18000, &gladiator_tile7, 0, 64 },

	/* sprites */
	{ 1, 0x30000, &gladiator_tile0, 0, 64 },
	{ 1, 0x30000, &gladiator_tileB, 0, 64 },

	{ 1, 0x30000, &gladiator_tileA, 0, 64 },
	{ 1, 0x30000, &gladiator_tile3, 0, 64 }, /* "GLAD..." */

	{ 1, 0x00000, &gladiator_tile0, 0, 64 },
	{ 1, 0x00000, &gladiator_tileB, 0, 64 },

	{ 1, 0x00000, &gladiator_tileA, 0, 64 },
	{ 1, 0x00000, &gladiator_tile3, 0, 64 }, /* ...DIATOR */

	{ 1, 0x00000, &gladiator_tile4, 0, 64 },
	{ 1, 0x00000, &gladiator_tileD, 0, 64 },

	{ 1, 0x00000, &gladiator_tileC, 0, 64 },
	{ 1, 0x00000, &gladiator_tile7, 0, 64 },

	{ -1 } /* end of array */
};

#undef DEFINE_LAYOUT



static struct YM2203interface ym2203_interface =
{
	1,		/* 1 chip */
	1500000,	/* 1.5 MHz? */
	{ YM2203_VOL(255,0xff) },
	{ 0 },
	{ gladiator_dsw3_r },         /* port B read */
	{ gladiator_int_controll_w }, /* port A write */
	{ 0 },
	{ gladiator_ym_irq }          /* NMI request for 2nd cpu */
};

static struct ADPCMinterface adpcm_interface =
{
	1,			/* 1 channel */
    8000,		       /* 8000Hz playback */
	4,		       /* memory region 4 */
	0,			/* init function */
	{ 255, 255, 255 }
};

static struct Samplesinterface	samples_interface =
{
        1       /* 1 channel */
};


static struct MachineDriver machine_driver =
{
	{
		{
			CPU_Z80,
			6000000, /* 6 MHz? */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000, /* 3 MHz? */
			2,	/* memory region #2 */
			readmem_cpu2,writemem_cpu2,readport_cpu2,writeport_cpu2,
			ignore_interrupt,1

		},
		{
			CPU_M6809 | CPU_AUDIO_CPU,
			1,	/* Using 1 hz to speed up emulation since 6809 code isn't being used*/
//			3000000, /* 3 MHz? */
			3,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION, /* fps, vblank duration */
	10,	/* interleaving */
	gladiator_machine_init,

	/* video hardware */
	32*8, 32*8, { 0, 255, 0+16, 255-16 },

	gfxdecodeinfo,
	512+2, 512+2,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	gladiatr_vh_start,
	gladiatr_vh_stop,
	gladiatr_vh_screenrefresh,

	/* sound hardware */
	0, 0,	0, 0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
                {
			SOUND_ADPCM,
			&adpcm_interface
                },
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gladiatr_rom )
	ROM_REGION(0x1c000)
	ROM_LOAD( "qb0-5",        0x00000, 	0x4000, 0x25b19efb )
	ROM_LOAD( "qb0-4",        0x04000, 	0x2000, 0x347ec794 )
	ROM_LOAD( "qb0-1",        0x10000, 	0x4000, 0x040c9839 )
	ROM_LOAD( "qc0-3",        0x14000, 	0x8000, 0x8d182326 )

	ROM_REGION_DISPOSE(0x44000)	/* temporary space for graphics (disposed after conversion) */
	/* sprites */
	ROM_LOAD( "qc2-7",        	0x00000, 0x8000, 0xc992c4f7 ) /* plane 3 */
	ROM_LOAD( "qc1-10",       	0x08000, 0x8000, 0x364cdb58 ) /* planes 1,2 */
	ROM_LOAD( "qc2-11",       	0x10000, 0x8000, 0xc9fecfff ) /* planes 1,2 */

	ROM_LOAD( "qc1-6",        	0x30000, 0x4000, 0x651e6e44 ) /* plane 3 */
	ROM_LOAD( "qc0-8",        	0x38000, 0x4000, 0x1c7ffdad ) /* planes 1,2 */
	ROM_LOAD( "qc1-9",        	0x40000, 0x4000, 0x01043e03 ) /* planes 1,2 */

	/* tiles */
	ROM_LOAD( "qb0-12",       	0x18000, 0x8000, 0x0585d9ac ) /* plane 3 */
	ROM_LOAD( "qb0-13",       	0x20000, 0x8000, 0xa6bb797b ) /* planes 1,2 */
	ROM_LOAD( "qb0-14",       	0x28000, 0x8000, 0x85b71211 ) /* planes 1,2 */

	ROM_LOAD( "qc0-15",       	0x34000, 0x2000, 0xa7efa340 ) /* (monochrome) */

	ROM_REGION( 0x10000 ) /* Code for the 2nd CPU */
	ROM_LOAD( "qb0-17",       	0x0000, 0x4000, 0xe78be010 )

	ROM_REGION( 0x20000 )  /* QB0-18 contains 6809 Code & some ADPCM samples */
	ROM_LOAD( "qb0-18",       0x08000, 0x8000, 0xe9591260 )

	ROM_REGION( 0x24600 )	/* Load all ADPCM samples into seperate region */
	ROM_LOAD( "qb0-18",       0x00000, 0x8000, 0xe9591260 )
	ROM_LOAD( "qb0-19",       0x08000, 0x8000, 0x79caa7ed )
	ROM_LOAD( "qb0-20",       0x10000, 0x8000, 0x15916eda )
ROM_END

ROM_START( ogonsiro_rom )
	ROM_REGION(0x1c000)
	ROM_LOAD( "qb0-5",        0x00000, 	0x4000, 0x25b19efb )
	ROM_LOAD( "qb0-4",        0x04000, 	0x2000, 0x347ec794 )
	ROM_LOAD( "qb0-1",        0x10000, 	0x4000, 0x040c9839 )
	ROM_LOAD( "qb0_3",        0x14000, 	0x8000, 0xd6a342e7 )

	ROM_REGION_DISPOSE(0x44000)	/* temporary space for graphics (disposed after conversion) */
	/* sprites */
	ROM_LOAD( "qb0_7",        	0x00000, 0x8000, 0x4b677bd9 ) /* plane 3 */
	ROM_LOAD( "qb0_10",       	0x08000, 0x8000, 0x87ab6cc4 ) /* planes 1,2 */
	ROM_LOAD( "qb0_11",       	0x10000, 0x8000, 0x25eaa4ff ) /* planes 1,2 */

	ROM_LOAD( "qb0_6",        	0x30000, 0x4000, 0x1a2bc769 ) /* plane 3 */
	ROM_LOAD( "qc0-8",        	0x38000, 0x4000, 0x1c7ffdad ) /* planes 1,2 */
	ROM_LOAD( "qb0_9",        	0x40000, 0x4000, 0x38f5152d ) /* planes 1,2 */

	/* tiles */
	ROM_LOAD( "qb0-12",       	0x18000, 0x8000, 0x0585d9ac ) /* plane 3 */
	ROM_LOAD( "qb0-13",       	0x20000, 0x8000, 0xa6bb797b ) /* planes 1,2 */
	ROM_LOAD( "qb0-14",       	0x28000, 0x8000, 0x85b71211 ) /* planes 1,2 */

	ROM_LOAD( "qb0_15",       	0x34000, 0x2000, 0x5e1332b8 ) /* (monochrome) */

	ROM_REGION( 0x10000 ) /* Code for the 2nd CPU */
	ROM_LOAD( "qb0-17",       	0x0000, 0x4000, 0xe78be010 )

	ROM_REGION( 0x20000 )  /* QB0-18 contains 6809 Code & some ADPCM samples */
	ROM_LOAD( "qb0-18",       0x08000, 0x8000, 0xe9591260 )

	ROM_REGION( 0x24600 )	/* Load all ADPCM samples into seperate region */
	ROM_LOAD( "qb0-18",       0x00000, 0x8000, 0xe9591260 )
	ROM_LOAD( "qb0-19",       0x08000, 0x8000, 0x79caa7ed )
	ROM_LOAD( "qb0-20",       0x10000, 0x8000, 0x15916eda )
ROM_END

ROM_START( gcastle_rom )
	ROM_REGION(0x1c000)
	ROM_LOAD( "qb0-5",        0x00000, 	0x4000, 0x25b19efb )
	ROM_LOAD( "qb0-4",        0x04000, 	0x2000, 0x347ec794 )
	ROM_LOAD( "qb0-1",        0x10000, 	0x4000, 0x040c9839 )
	ROM_LOAD( "qb0_3",        0x14000, 	0x8000, 0xd6a342e7 )

	ROM_REGION_DISPOSE(0x44000)	/* temporary space for graphics (disposed after conversion) */
	/* sprites */
	ROM_LOAD( "gc2-7",        	0x00000, 0x8000, 0xbb2cb454 ) /* plane 3 */
	ROM_LOAD( "qb0_10",       	0x08000, 0x8000, 0x87ab6cc4 ) /* planes 1,2 */
	ROM_LOAD( "gc2-11",       	0x10000, 0x8000, 0x5c512365 ) /* planes 1,2 */

	ROM_LOAD( "gc1-6",        	0x30000, 0x4000, 0x94f49be2 ) /* plane 3 */
	ROM_LOAD( "qc0-8",        	0x38000, 0x4000, 0x1c7ffdad ) /* planes 1,2 */
	ROM_LOAD( "gc1-9",        	0x40000, 0x4000, 0x69b977fd ) /* planes 1,2 */

	/* tiles */
	ROM_LOAD( "qb0-12",       	0x18000, 0x8000, 0x0585d9ac ) /* plane 3 */
	ROM_LOAD( "qb0-13",       	0x20000, 0x8000, 0xa6bb797b ) /* planes 1,2 */
	ROM_LOAD( "qb0-14",       	0x28000, 0x8000, 0x85b71211 ) /* planes 1,2 */

	ROM_LOAD( "qb0_15",       	0x34000, 0x2000, 0x5e1332b8 ) /* (monochrome) */

	ROM_REGION( 0x10000 ) /* Code for the 2nd CPU */
	ROM_LOAD( "qb0-17",       	0x0000, 0x4000, 0xe78be010 )

	ROM_REGION( 0x20000 )  /* QB0-18 contains 6809 Code & some ADPCM samples */
	ROM_LOAD( "qb0-18",       0x08000, 0x8000, 0xe9591260 )

	ROM_REGION( 0x24600 )	/* Load all ADPCM samples into seperate region */
	ROM_LOAD( "qb0-18",       0x00000, 0x8000, 0xe9591260 )
	ROM_LOAD( "qb0-19",       0x08000, 0x8000, 0x79caa7ed )
	ROM_LOAD( "qb0-20",       0x10000, 0x8000, 0x15916eda )
ROM_END

/*Manually combine samples: lgzaid - p1 with lgzaid p2!*/
static void gladiatr_decode(void)
{
	unsigned char *RAM = Machine->memory_region[4];

	memcpy (&RAM[0x20000], &RAM[0x11d00], 0x2300);	/* copy Part 1 */
	memcpy (&RAM[0x22300], &RAM[0x8000], 0x2300);	/* copy Part 2 */
}

/*************************/
/* Gladiator Samples	 */
/*************************/

ADPCM_SAMPLES_START( glad_samples )
	/* samples on ROM 1 */
	/* dtirene, paw, death, atirene, gong, atirene2  */
	/* samples on ROM 2 */
	/* lgzaid-p2, lgirene, dtsolon, irene, solon, zaid, fight  */
	/* samples on ROM 3 */
	/* dtzaid, atzaid, lgzaid-p1, lgsolon, atsolon, agadon  */
	/*NOTES: lgzaid - part 1: 1d00- 4000 - Length = 0x2300 */
	/*NOTES: lgzaid - part 2: 8000- A300 - Lenght = 0x2300 */

	ADPCM_SAMPLE( 0x000, 0x10000+0x0000, (0x1000-0x0000)*2 )	//DTZAID
	ADPCM_SAMPLE( 0x001, 0x10000+0x1000, (0x1d00-0x1000)*2 )	//ATZAID
	ADPCM_SAMPLE( 0x002, 0x20000+0x0000, (0x4600-0x0000)*2 )	//Full LGZAID!
	ADPCM_SAMPLE( 0x003, 0x8000+0x2200, (0x3e00-0x2300)*2 )		//LGIRENE
	ADPCM_SAMPLE( 0x004, 0x0000, (0x1300-0x0000)*2 )		//DTIRENE
	ADPCM_SAMPLE( 0x005, 0x1300, (0x1f00-0x1300)*2 )		//PAW
	ADPCM_SAMPLE( 0x006, 0x1f00, (0x2C00-0x1f00)*2 )		//DEATH
	ADPCM_SAMPLE( 0x007, 0x2C00, (0x3000-0x2C00)*2 )		//ATIRENE
	ADPCM_SAMPLE( 0x008, 0x7000, (0x7c00-0x7000)*2 )		//ATIRENE2
	ADPCM_SAMPLE( 0x009, 0x10000+0x5e00, (0x6e00-0x5e00)*2 )	//ATSOLON
	ADPCM_SAMPLE( 0x00a, 0x8000+0x3d00, (0x5400-0x3e00)*2 )		//DTSOLON
	ADPCM_SAMPLE( 0x00b, 0x10000+0x4000, (0x5e00-0x4000)*2 )	//LGSOLON
	ADPCM_SAMPLE( 0x00c, 0x3FA0, (0x7000-0x3FA0)*2 )		//GONG
	ADPCM_SAMPLE( 0x00d, 0x8000+0x5500, (0x6000-0x5500)*2 )		//IRENE
	ADPCM_SAMPLE( 0x00e, 0x8000+0x6000, (0x6a00-0x6000)*2 )		//SOLON
	ADPCM_SAMPLE( 0x00f, 0x8000+0x6a00, (0x7100-0x6a00)*2 )		//ZAID
	ADPCM_SAMPLE( 0x010, 0x10000+0x7000,(0x7B00-0x7000)*2 )		//AGADON
	ADPCM_SAMPLE( 0x011, 0x8000+0x7100, (0x7A00-0x7100)*2 )		//FIGHT
//	ADPCM_SAMPLE( 0x012, 0x10000+0x1d00, (0x4000-0x1d00)*2 )	//LGZAID - p1
//	ADPCM_SAMPLE( 0x013, 0x8000+0x0000, (0x2300-0x0000)*2 )		//LGZAID - p2

ADPCM_SAMPLES_END

/*These samples are played back at 5500Hz since the ADPCM interface can't support
  multiple sample rates at this time*/
static const char *gladiatr_samplenames[]=
{
	"*gladiatr",
        "dtirene.sam",  /*data used to request sample = 0xe7*/
        "atirene.sam",  /*data used to request sample = 0xea*/
        "atirene2.sam", /*data used to request sample = 0xeb*/
	0
};

static int gladiatr_nvram_load(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0xf000],0x400);
		osd_fclose(f);
	}
	return 1;
}

static void gladiatr_nvram_save(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xf000],0x400);
		osd_fclose(f);
	}
}



struct GameDriver gladiatr_driver =
{
	__FILE__,
	0,
	"gladiatr",
	"Gladiator",
	"1986",
	"Taito America",
	"Victor Trucco\nSteve Ellenoff\nPhil Stroffolino\nNicola Salmoria\nTatsuyuki Satoh\n",
	0,
	&machine_driver,
	0,

	gladiatr_rom,
	gladiatr_decode,
	0,
//	0,
//	0,
	gladiatr_samplenames,
	(void *)glad_samples,
	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	gladiatr_nvram_load, gladiatr_nvram_save
};

struct GameDriver ogonsiro_driver =
{
	__FILE__,
	&gladiatr_driver,
	"ogonsiro",
	"Ohgon no Siro",
	"1986",
	"Taito",
	"Victor Trucco\nSteve Ellenoff\nPhil Stroffolino\nNicola Salmoria\nTatsuyuki Satoh\n",
	0,
	&machine_driver,
	0,

	ogonsiro_rom,
	gladiatr_decode,
	0,
//	0,
//	0,
	gladiatr_samplenames,
	(void *)glad_samples,
	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	gladiatr_nvram_load, gladiatr_nvram_save
};

struct GameDriver gcastle_driver =
{
	__FILE__,
	&gladiatr_driver,
	"gcastle",
	"Golden Castle",
	"1986",
	"Taito",
	"Victor Trucco\nSteve Ellenoff\nPhil Stroffolino\nNicola Salmoria\nTatsuyuki Satoh\n",
	0,
	&machine_driver,
	0,

	gcastle_rom,
	gladiatr_decode,
	0,
//	0,
//	0,
	gladiatr_samplenames,
	(void *)glad_samples,
	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	gladiatr_nvram_load, gladiatr_nvram_save
};
