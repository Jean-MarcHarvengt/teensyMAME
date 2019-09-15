/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  Tapper machine started by Chris Kirmse

***************************************************************************/

#include <stdio.h>

#include "driver.h"
#include "z80/z80.h"
#include "machine/z80fmly.h"
#include "m6808/m6808.h"
#include "m6809/m6809.h"
#include "machine/6821pia.h"
#include "timer.h"


int pedal_sensitivity = 4;			/* Amount of change to read each time the pedal keys are pulsed */
int weighting_factor = 0;			/* Progressive weighting factor */

int mcr_loadnvram;
int spyhunt_lamp[8];

extern int spyhunt_scrollx,spyhunt_scrolly;
static int spyhunt_mux;

static int maxrpm_mux;
static int maxrpm_last_shift;
static int maxrpm_p1_shift;
static int maxrpm_p2_shift;

static unsigned char soundlatch[4];
static unsigned char soundstatus;

extern void dotron_change_light (int light);

/* z80 ctc */
static void ctc_interrupt (int state)
{
	cpu_cause_interrupt (0, Z80_VECTOR(0,state) );
}

static z80ctc_interface ctc_intf =
{
	1,                  /* 1 chip */
	{ 0 },              /* clock (filled in from the CPU 0 clock */
	{ 0 },              /* timer disables */
	{ ctc_interrupt },  /* interrupt handler */
	{ 0 },              /* ZC/TO0 callback */
	{ 0 },              /* ZC/TO1 callback */
	{ 0 }               /* ZC/TO2 callback */
};


/* used for the sound boards */
static int dacval;
static int suspended;


/* Chip Squeak Deluxe (CSD) interface */
static void csd_porta_w (int offset, int data);
static void csd_portb_w (int offset, int data);
static void csd_irq (void);

static pia6821_interface csd_pia_intf =
{
	1,                /* 1 chip */
	{ PIA_DDRA, PIA_NOP, PIA_DDRB, PIA_NOP, PIA_CTLA, PIA_NOP, PIA_CTLB, PIA_NOP }, /* offsets */
	{ 0 },            /* input port A  */
	{ 0 },            /* input bit CA1 */
	{ 0 },            /* input bit CA2 */
	{ 0 },            /* input port B  */
	{ 0 },            /* input bit CB1 */
	{ 0 },            /* input bit CB2 */
	{ csd_porta_w },  /* output port A */
	{ csd_portb_w },  /* output port B */
	{ 0 },            /* output CA2    */
	{ 0 },            /* output CB2    */
	{ csd_irq },      /* IRQ A         */
	{ csd_irq },      /* IRQ B         */
};


/* Sounds Good (SG) PIA interface */
static void sg_porta_w (int offset, int data);
static void sg_portb_w (int offset, int data);
static void sg_irq (void);

static pia6821_interface sg_pia_intf =
{
	1,                /* 1 chip */
	{ PIA_DDRA, PIA_NOP, PIA_DDRB, PIA_NOP, PIA_CTLA, PIA_NOP, PIA_CTLB, PIA_NOP }, /* offsets */
	{ 0 },            /* input port A  */
	{ 0 },            /* input bit CA1 */
	{ 0 },            /* input bit CA2 */
	{ 0 },            /* input port B  */
	{ 0 },            /* input bit CB1 */
	{ 0 },            /* input bit CB2 */
	{ sg_porta_w },   /* output port A */
	{ sg_portb_w },   /* output port B */
	{ 0 },            /* output CA2    */
	{ 0 },            /* output CB2    */
	{ sg_irq },       /* IRQ A         */
	{ sg_irq },       /* IRQ B         */
};


/* Turbo Chip Squeak (TCS) PIA interface */
static void tcs_irq (void);

static pia6821_interface tcs_pia_intf =
{
	1,                /* 1 chip */
	{ PIA_DDRA, PIA_DDRB, PIA_CTLA, PIA_CTLB }, /* offsets */
	{ 0 },            /* input port A  */
	{ 0 },            /* input bit CA1 */
	{ 0 },            /* input bit CA2 */
	{ 0 },            /* input port B  */
	{ 0 },            /* input bit CB1 */
	{ 0 },            /* input bit CB2 */
	{ DAC_data_w },   /* output port A */
	{ 0 },            /* output port B */
	{ 0 },            /* output CA2    */
	{ 0 },            /* output CB2    */
	{ tcs_irq },      /* IRQ A         */
	{ tcs_irq },      /* IRQ B         */
};


/* Squawk & Talk (SNT) PIA interface */
static void snt_porta1_w (int offset, int data);
static void snt_porta2_w (int offset, int data);
static void snt_portb2_w (int offset, int data);
static void snt_irq (void);

static pia6821_interface snt_pia_intf =
{
	2,                              /* 2 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB }, /* offsets */
	{ 0, 0 },                       /* input port A  */
	{ 0, 0 },                       /* input bit CA1 */
	{ 0, 0 },                       /* input bit CA2 */
	{ 0, 0 },                       /* input port B  */
	{ 0, 0 },                       /* input bit CB1 */
	{ 0, 0 },                       /* input bit CB2 */
	{ snt_porta1_w, snt_porta2_w }, /* output port A */
	{ 0, snt_portb2_w },            /* output port B */
	{ 0, 0 },                       /* output CA2    */
	{ 0, 0 },                       /* output CB2    */
	{ snt_irq, snt_irq },           /* IRQ A         */
	{ snt_irq, snt_irq },           /* IRQ B         */
};


/***************************************************************************

  Generic MCR handlers

***************************************************************************/

void mcr_init_machine(void)
{
   int i;

	/* reset the sound */
   for (i = 0; i < 4; i++)
      soundlatch[i] = 0;
   soundstatus = 0;
   suspended = 0;

   /* initialize the CTC */
   ctc_intf.baseclock[0] = Machine->drv->cpu[0].cpu_clock;
   z80ctc_init (&ctc_intf);

   /* daisy chain set */
	{
		static Z80_DaisyChain daisy_chain[] =
		{
			{ z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0 }, /* CTC number 0 */
			{ 0,0,0,-1}         /* end mark */
		};
		cpu_setdaisychain (0,daisy_chain );
	}

   /* can't load NVRAM right away */
   mcr_loadnvram = 0;
}

void spyhunt_init_machine (void)
{
   mcr_init_machine ();

   /* reset the PIAs */
   pia_startup (&csd_pia_intf);
}


void rampage_init_machine (void)
{
	mcr_init_machine ();
	mcr_loadnvram = 1;

   /* reset the PIAs */
   pia_startup (&sg_pia_intf);
}


void sarge_init_machine (void)
{
	mcr_init_machine ();
	mcr_loadnvram = 1;

   /* reset the PIAs */
   pia_startup (&tcs_pia_intf);
}


void dotron_init_machine (void)
{
   mcr_init_machine ();

   /* reset the PIAs */
   pia_startup (&snt_pia_intf);
}


int mcr_interrupt (void)
{
	/* once per frame, pulse the CTC line 3 */
	z80ctc_0_trg3_w (0, 1);
	z80ctc_0_trg3_w (0, 0);
	return ignore_interrupt ();
}

int dotron_interrupt (void)
{
	/* pulse the CTC line 2 to enable Platform Poles */
	z80ctc_0_trg2_w (0, 1);
	z80ctc_0_trg2_w (0, 0);
	/* once per frame, pulse the CTC line 3 */
	z80ctc_0_trg3_w (0, 1);
	z80ctc_0_trg3_w (0, 0);
	return ignore_interrupt ();
}


void mcr_delayed_write (int param)
{
	soundlatch[param >> 8] = param & 0xff;
}

void mcr_writeport(int port,int value)
{
	switch (port)
	{
		case 0:	/* OP0  Write latch OP0 (coin meters, 2 led's and cocktail 'flip') */
		   if (errorlog)
		      fprintf (errorlog, "mcr write to OP0 = %02i\n", value);
		   return;

		case 4:	/* Write latch OP4 */
		   if (errorlog)
		      fprintf (errorlog, "mcr write to OP4 = %02i\n", value);
			return;

		case 0x1c:	/* WIRAM0 - write audio latch 0 */
		case 0x1d:	/* WIRAM0 - write audio latch 1 */
		case 0x1e:	/* WIRAM0 - write audio latch 2 */
		case 0x1f:	/* WIRAM0 - write audio latch 3 */
			timer_set (TIME_NOW, ((port - 0x1c) << 8) | (value & 0xff), mcr_delayed_write);
/*			mcr_delayed_write (((port - 0x1c) << 8) | (value & 0xff));*/
/*			soundlatch[port - 0x1c] = value;*/
			return;

		case 0xe0:	/* clear watchdog timer */
			watchdog_reset_w (0, 0);
			return;

		case 0xe8:
			/* A sequence of values gets written here at startup; we don't know why;
			   However, it does give us the opportunity to tweak the IX register before
			   it's checked in Tapper and Timber, thus eliminating the need for patches
			   The value 5 is written last; we key on that, and only modify IX if it is
			   currently 0; hopefully this is 99.9999% safe :-) */
			if (value == 5)
			{
				Z80_Regs temp;
				Z80_GetRegs (&temp);
				if (temp.IX.D == 0)
					temp.IX.D += 1;
				Z80_SetRegs (&temp);
			}
			return;

		case 0xf0:	/* These are the ports of a Z80-CTC; it generates interrupts in mode 2 */
		case 0xf1:
		case 0xf2:
		case 0xf3:
		  z80ctc_0_w (port - 0xf0, value);
		  return;
	}

	/* log all writes that end up here */
   if (errorlog)
      fprintf (errorlog, "mcr unknown write port %02x %02i\n", port, value);
}


int mcr_readport(int port)
{
	/* ports 0-4 are read directly via the input ports structure */
	port += 5;

   /* only a few ports here */
   switch (port)
   {
		case 0x07:	/* Read audio status register */

			/* once the system starts checking the sound, memory tests are done; load the NVRAM */
		   mcr_loadnvram = 1;
			return soundstatus;

		case 0x10:	/* Tron reads this as an alias to port 0 -- does this apply to all ports 10-14? */
			return cpu_readport (port & 0x0f);
   }

	/* log all reads that end up here */
   if (errorlog)
      fprintf (errorlog, "reading port %i (PC=%04X)\n", port, cpu_getpc ());
	return 0;
}


void mcr_soundstatus_w (int offset,int data)
{
   soundstatus = data;
}


int mcr_soundlatch_r (int offset)
{
   return soundlatch[offset];
}


/***************************************************************************

  Game-specific port handlers

***************************************************************************/

/* Translation table for one-joystick emulation */
static int one_joy_trans[32]={
        0x00,0x05,0x0A,0x00,0x06,0x04,0x08,0x00,
        0x09,0x01,0x02,0x00,0x00,0x00,0x00,0x00 };

int sarge_IN1_r(int offset)
{
	int res,res1;

	res=readinputport(1);
	res1=readinputport(6);

	res&=~one_joy_trans[res1&0xf];

	return (res);
}

int sarge_IN2_r(int offset)
{
	int res,res1;

	res=readinputport(2);
	res1=readinputport(6)>>4;

	res&=~one_joy_trans[res1&0xf];

	return (res);
}



void spyhunt_delayed_write (int param)
{
	pia_1_portb_w (0, param & 0x0f);
	pia_1_ca1_w (0, param & 0x10);
}

void spyhunt_writeport(int port,int value)
{
	static int lastport4;

	switch (port)
	{
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
			/* mux select is in bit 7 */
		   spyhunt_mux = value & 0x80;

	   	/* lamp driver command triggered by bit 5, data is in low four bits */
		   if (((lastport4 ^ value) & 0x20) && !(value & 0x20))
		   {
		   	if (value & 8)
		   		spyhunt_lamp[value & 7] = 1;
		   	else
		   		spyhunt_lamp[value & 7] = 0;
		   }

	   	/* CSD command triggered by bit 4, data is in low four bits */
	   	timer_set (TIME_NOW, value, spyhunt_delayed_write);

		   /* remember the last value */
		   lastport4 = value;
		   break;

		case 0x84:
			spyhunt_scrollx = (spyhunt_scrollx & ~0xff) | value;
			break;

		case 0x85:
			spyhunt_scrollx = (spyhunt_scrollx & 0xff) | ((value & 0x07) << 8);
			spyhunt_scrolly = (spyhunt_scrolly & 0xff) | ((value & 0x80) << 1);
			break;

		case 0x86:
			spyhunt_scrolly = (spyhunt_scrolly & ~0xff) | value;
			break;

		default:
			mcr_writeport(port,value);
			break;
	}
}



void rampage_delayed_write (int param)
{
	pia_1_portb_w (0, (param >> 1) & 0x0f);
	pia_1_ca1_w (0, ~param & 0x01);
}

void rampage_writeport(int port,int value)
{
	switch (port)
	{
		case 0x04:
		case 0x05:
			break;

		case 0x06:
			timer_set (TIME_NOW, value, rampage_delayed_write);
	   	break;

		case 0x07:
		   break;

		default:
			mcr_writeport(port,value);
			break;
	}
}



void maxrpm_writeport(int port,int value)
{
	switch (port)
	{
		case 0x04:
			break;

		case 0x05:
			maxrpm_mux = (value >> 1) & 3;
			break;

		case 0x06:
			timer_set (TIME_NOW, value, rampage_delayed_write);
			break;

		case 0x07:
		   break;

		default:
			mcr_writeport(port,value);
			break;
	}
}



void sarge_delayed_write (int param)
{
	pia_1_portb_w (0, (param >> 1) & 0x0f);
	pia_1_ca1_w (0, ~param & 0x01);
}

void sarge_writeport(int port,int value)
{
	switch (port)
	{
		case 0x04:
		case 0x05:
			break;

		case 0x06:
			timer_set (TIME_NOW, value, sarge_delayed_write);
			break;

		case 0x07:
		   break;

		default:
			mcr_writeport(port,value);
			break;
	}
}



void dotron_delayed_write (int param)
{
	pia_1_porta_w (0, ~param & 0x0f);
	pia_1_cb1_w (0, ~param & 0x10);
	dotron_change_light (param >> 6);
}

void dotron_writeport(int port,int value)
{
	switch (port)
	{
		case 0x04:
			timer_set (TIME_NOW, value, dotron_delayed_write);
			break;

		case 0x05:
		case 0x06:
		case 0x07:
			if (errorlog) fprintf(errorlog,"Write to port %02X = %02X\n",port,value);
		   break;

		default:
			mcr_writeport(port,value);
			break;
	}
}


void crater_writeport(int port,int value)
{
	switch (port)
	{
		case 0x84:
			spyhunt_scrollx = (spyhunt_scrollx & ~0xff) | value;
			break;

		case 0x85:
			spyhunt_scrollx = (spyhunt_scrollx & 0xff) | ((value & 0x07) << 8);
			spyhunt_scrolly = (spyhunt_scrolly & 0xff) | ((value & 0x80) << 1);
			break;

		case 0x86:
			spyhunt_scrolly = (spyhunt_scrolly & ~0xff) | value;
			break;

		default:
			mcr_writeport(port,value);
			break;
	}
}


/***************************************************************************

  Game-specific input handlers

***************************************************************************/

/* Max RPM -- multiplexed steering wheel/gas pedals */
int maxrpm_IN1_r(int offset)
{
	return readinputport (6 + maxrpm_mux);
}

int maxrpm_IN2_r(int offset)
{
	static int shift_bits[5] = { 0x00, 0x05, 0x06, 0x01, 0x02 };
	int start = readinputport (0);
	int shift = readinputport (10);

	/* reset on a start */
	if (!(start & 0x08))
		maxrpm_p1_shift = 0;
	if (!(start & 0x04))
		maxrpm_p2_shift = 0;

	/* increment, decrement on falling edge */
	if (!(shift & 0x01) && (maxrpm_last_shift & 0x01))
	{
		maxrpm_p1_shift++;
		if (maxrpm_p1_shift > 4)
			maxrpm_p1_shift = 4;
	}
	if (!(shift & 0x02) && (maxrpm_last_shift & 0x02))
	{
		maxrpm_p1_shift--;
		if (maxrpm_p1_shift < 0)
			maxrpm_p1_shift = 0;
	}
	if (!(shift & 0x04) && (maxrpm_last_shift & 0x04))
	{
		maxrpm_p2_shift++;
		if (maxrpm_p2_shift > 4)
			maxrpm_p2_shift = 4;
	}
	if (!(shift & 0x08) && (maxrpm_last_shift & 0x08))
	{
		maxrpm_p2_shift--;
		if (maxrpm_p2_shift < 0)
			maxrpm_p2_shift = 0;
	}

	maxrpm_last_shift = shift;

	return ~((shift_bits[maxrpm_p1_shift] << 4) + shift_bits[maxrpm_p2_shift]);
}


/* Spy Hunter -- normal port plus CSD status bits */
int spyhunt_port_1_r(int offset)
{
	return input_port_1_r(offset) | ((pia_1_portb_r (0) & 0x30) << 1);
}


/* Spy Hunter -- multiplexed steering wheel/gas pedal */
int spyhunt_port_2_r(int offset)
{
	int accel = readinputport (6);
	int steer = readinputport (7);

	/* mux high bit on means return steering wheel */
	if (spyhunt_mux & 0x80)
	{
		if (errorlog) fprintf(errorlog, "return steer = %d", steer);
	   return steer;
	}

	/* mux high bit off means return gas pedal */
	else
	{
		if (errorlog) fprintf(errorlog, "return accel = %d", accel);
	   return accel;
	}
}


/* Destruction Derby -- 6 bits of steering plus 2 bits of normal inputs */
int destderb_port_r(int offset)
{
	return readinputport (1 + offset);
}


/* Kozmik Krooz'r -- dial reader */
int kroozr_dial_r(int offset)
{
	int dial = readinputport (7);
	int val = readinputport (1);

	val |= (dial & 0x80) >> 1;
	val |= (dial & 0x70) >> 4;

	return val;
}


/* Kozmik Krooz'r -- joystick readers */
int kroozr_trakball_x_r(int data)
{
	int val = readinputport (6);

	if (val & 0x02)		/* left */
		return 0x64 - 0x34;
	if (val & 0x01)		/* right */
		return 0x64 + 0x34;
	return 0x64;
}

int kroozr_trakball_y_r(int data)
{
	int val = readinputport (6);

	if (val & 0x08)		/* up */
		return 0x64 - 0x34;
	if (val & 0x04)		/* down */
		return 0x64 + 0x34;
	return 0x64;
}

/* Discs of Tron -- remap up and down on the mouse to aim up and down */
int dotron_IN2_r(int offset)
{
	int data;
	static int delta = 0;
	char fake;
	static char lastfake = 0;
	static int mask = 0x00FF;
	static int count = 0;

	data = input_port_2_r(offset);
	fake = input_port_6_r(offset);

	delta += (fake - lastfake);
	lastfake = fake;

	/* Map to "aim up" */
	if (delta > 5)
	{
		mask = 0x00EF;
		count = 5;
		delta = 0;
	}
	/* Map to "aim down" */
	else if (delta < -5)
	{
		mask = 0x00DF;
		count = 5;
		delta = 0;
	}

	if ((count--) <= 0)
	{
		count = 0;
		mask = 0x00FF;
	}

	data &= mask;

	return data;
}


/***************************************************************************

  Sound board-specific PIA handlers

***************************************************************************/

void mcr_pia_1_w (int offset, int data)
{
	if (!(data & 0xff000000))
		pia_1_w (offset, (data >> 8) & 0xff);
}

int mcr_pia_1_r (int offset)
{
	return pia_1_r (offset) << 8;
}


/*
 *		Chip Squeak Deluxe (Spy Hunter) board
 *
 *		MC68000, 1 PIA, 10-bit DAC
 */
static void csd_porta_w (int offset, int data)
{
	int temp;
	dacval = (dacval & ~0x3fc) | (data << 2);
	temp = dacval/2;
	if (temp > 0xff) temp = 0xff;
	DAC_data_w (0, temp);
}

static void csd_portb_w (int offset, int data)
{
	dacval = (dacval & ~0x003) | (data >> 6);
	/* only update when the MSB's are changed */
}

static void csd_irq (void)
{
	/* generate a sound interrupt */
  	cpu_cause_interrupt (2, 4);
}


/*
 *		Sounds Good (Rampage) board
 *
 *		MC68000, 1 PIA, 10-bit DAC
 */
static void sg_porta_w (int offset, int data)
{
	int temp;
	dacval = (dacval & ~0x3fc) | (data << 2);
	temp = dacval/2;
	if (temp > 0xff) temp = 0xff;
	DAC_data_w (0, temp);
}

static void sg_portb_w (int offset, int data)
{
	dacval = (dacval & ~0x003) | (data >> 6);
	/* only update when the MSB's are changed */
}

static void sg_irq (void)
{
	/* generate a sound interrupt */
  	cpu_cause_interrupt (1, 4);
}


/*
 *		Turbo Chip Squeak (Sarge) board
 *
 *		MC6809, 1 PIA, 8-bit DAC
 */
static void tcs_irq (void)
{
	/* generate a sound interrupt */
	cpu_cause_interrupt (1, M6809_INT_IRQ);
}


/*
 *		Squawk & Talk (Discs of Tron) board
 *
 *		MC6802, 2 PIAs, TMS5220, AY8912 (not used), 8-bit DAC (not used)
 */
static int tms_command;
static int tms_strobes;

static void snt_porta1_w (int offset, int data)
{
	/*printf ("Write to AY-8912\n");*/
}

static void snt_porta2_w (int offset, int data)
{
	tms_command = data;
}

static void snt_portb2_w (int offset, int data)
{
	/* bits 0-1 select read/write strobes on the TMS5220 */
	data &= 0x03;
	if (((data ^ tms_strobes) & 0x02) && !(data & 0x02))
	{
		tms5220_data_w (offset, tms_command);

		/* DoT expects the ready line to transition on a command/write here, so we oblige */
		pia_2_ca2_w (0,1);
		pia_2_ca2_w (0,0);
	}
	else if (((data ^ tms_strobes) & 0x01) && !(data & 0x01))
	{
		pia_2_porta_w (0,tms5220_status_r(offset));

		/* DoT expects the ready line to transition on a command/write here, so we oblige */
		pia_2_ca2_w (0,1);
		pia_2_ca2_w (0,0);
	}
	tms_strobes = data;
}

static void snt_irq (void)
{
	cpu_cause_interrupt (2, M6808_INT_IRQ);
}
