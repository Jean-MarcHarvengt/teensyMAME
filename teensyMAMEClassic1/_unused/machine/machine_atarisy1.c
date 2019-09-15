/*************************************************************************

  Atari System 1 machine hardware

*************************************************************************/

#include "atarigen.h"
#include "sndhrdw/5220intf.h"
#include "vidhrdw/generic.h"
#include "M6502/m6502.h"
#include "m68000/m68000.h"


int slapstic_tweak (int offset);

void atarisys1_update_display_list (int scanline);



/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *marble_speedcheck;



/*************************************
 *
 *		Statics
 *
 *************************************/

static int joystick_value;
static int joystick_type;
static int trackball_type;
static void *joystick_timer;

static int pedal_value;

static int m6522_ddra, m6522_ddrb;
static int m6522_dra, m6522_drb;
static unsigned char m6522_regs[16];

static unsigned long speedcheck_time1, speedcheck_time2;

static int indytemp_setopbase (int pc);


/*************************************
 *
 *		Initialization of globals.
 *
 *************************************/

static void atarisys1_soundint (void)
{
	cpu_cause_interrupt (0, 6);
}


void atarisys1_init_machine (int slapstic)
{
	atarigen_init_machine (atarisys1_soundint, slapstic);

	joystick_value = joystick_type = trackball_type = 0;
	joystick_timer = NULL;

	m6522_ddra = m6522_ddrb = 0xff;
	m6522_dra = m6522_drb = 0xff;
	memset (m6522_regs, 0xff, sizeof (m6522_regs));

	speedcheck_time1 = speedcheck_time2 = 0;
}



/*************************************
 *
 *		Machine-specific initialization.
 *
 *************************************/

void marble_init_machine (void)
{
	atarisys1_init_machine (103);
	joystick_type = 0;
	trackball_type = 1;
}


void peterpak_init_machine (void)
{
	atarisys1_init_machine (107);
	joystick_type = 1;
	trackball_type = 0;
}


void indytemp_init_machine (void)
{
	atarisys1_init_machine (105);
	cpu_setOPbaseoverride (indytemp_setopbase);
	joystick_type = 1;
	trackball_type = 0;
}


void roadrunn_init_machine (void)
{
	atarisys1_init_machine (108);
	joystick_type = 2;
	trackball_type = 0;
}


void roadblst_init_machine (void)
{
	atarisys1_init_machine (110);
	joystick_type = 3;
	trackball_type = 2;
	pedal_value = 0;
}



/*************************************
 *
 *		LED handlers.
 *
 *************************************/

void atarisys1_led_w (int offset, int data)
{
	osd_led_w (offset, ~data & 1);
}



/*************************************
 *
 *		Opcode memory catcher.
 *
 *************************************/

static int indytemp_setopbase (int pc)
{
	int prevpc = cpu_getpreviouspc ();

	/*
	 *		This is a slightly ugly kludge for Indiana Jones & the Temple of Doom because it jumps
	 *		directly to code in the slapstic.  The general order of things is this:
	 *
	 *			jump to $3A, which turns off interrupts and jumps to $00 (the reset address)
	 *			look up the request in a table and jump there
	 *			(under some circumstances, tweak the special addresses)
	 *			return via an RTS at the real bankswitch address
	 *
	 *		To simulate this, we tweak the slapstic reset address on entry into slapstic code; then
	 *		we let the system tweak whatever other addresses it wishes.  On exit, we tweak the
	 *		address of the previous PC, which is the RTS instruction, thereby completing the
	 *		bankswitch sequence.
	 *
	 *		Fortunately for us, all 4 banks have exactly the same code at this point in their
	 *		ROM, so it doesn't matter which version we're actually executing.
	 */

	if (pc & 0x80000)
		slapstic_tweak (0);
	else if (prevpc & 0x80000)
		slapstic_tweak ((prevpc / 2) & 0x3fff);

	return pc;
}



/*************************************
 *
 *		Interrupt handlers.
 *
 *************************************/

int atarisys1_interrupt (void)
{
	/* set a timer to reset the video parameters just before the end of VBLANK */
	timer_set (TIME_IN_USEC (Machine->drv->vblank_duration - 10), 0, atarisys1_update_display_list);

	/* update the gas pedal for RoadBlasters */
	if (joystick_type == 3)
	{
		if (input_port_1_r (0) & 0x80)
		{
			pedal_value += 64;
			if (pedal_value > 0xff) pedal_value = 0xff;
		}
		else
		{
			pedal_value -= 64;
			if (pedal_value < 0) pedal_value = 0;
		}
	}

	return 4;       /* Interrupt vector 4, used by VBlank */
}


void atarisys1_sound_interrupt (void)
{
	cpu_cause_interrupt (1, M6502_INT_IRQ);
}



/*************************************
 *
 *		Joystick read.
 *
 *************************************/

void atarisys1_delayed_joystick_int (int param)
{
	joystick_timer = NULL;
	joystick_value = param;
	cpu_cause_interrupt (0, 2);
}


int atarisys1_joystick_r (int offset)
{
	int newval = 0xff;

	/* digital joystick type */
	if (joystick_type == 1)
		newval = (input_port_0_r (offset) & (0x80 >> (offset / 2))) ? 0xf0 : 0x00;

	/* Hall-effect analog joystick */
	else if (joystick_type == 2)
		newval = (offset & 2) ? input_port_0_r (offset) : input_port_1_r (offset);

	/* Road Blasters gas pedal */
	else if (joystick_type == 3)
		newval = pedal_value;

	/* set a timer on the joystick interrupt */
	if (joystick_timer)
		timer_remove (joystick_timer);
	joystick_timer = NULL;
	if (offset < 0x10)
		joystick_timer = timer_set (TIME_IN_USEC (50), newval, atarisys1_delayed_joystick_int);

	return joystick_value;
}


void atarisys1_joystick_w (int offset, int data)
{
	atarisys1_joystick_r (offset);
}



/*************************************
 *
 *		Trackball read.
 *
 *************************************/

int atarisys1_trakball_r (int offset)
{
	int result = 0xff;

	/* Marble Madness trackball type -- rotated 45 degrees! */
	if (trackball_type == 1)
	{
		static int old[2][2], cur[2][2];
		int player = (offset >> 2) & 1;
		int which = (offset >> 1) & 1;
		int diff;

		/* when reading the even ports, do a real analog port update */
		if (which == 0)
		{
			int dx,dy;

			if(player == 0)
			{
				dx = (signed char)input_port_0_r (offset);
				dy = (signed char)input_port_1_r (offset);
			}
			else
			{
				dx = (signed char)input_port_2_r (offset);
				dy = (signed char)input_port_3_r (offset);
			}

			cur[player][0] += dx + dy;
			cur[player][1] += dx - dy;
		}

		/* clip the result to -0x3f to +0x3f to remove directional ambiguities */
		diff = cur[player][which] - old[player][which];
		if (diff < -0x3f) diff = -0x3f;
		if (diff >  0x3f) diff =  0x3f;
		result = old[player][which] += diff;
	}

	/* Road Blasters steering wheel */
	else if (trackball_type == 2)
	{
		result = input_port_0_r (offset);
	}

	return result;
}



/*************************************
 *
 *		I/O read dispatch.
 *
 *************************************/

int atarisys1_io_r (int offset)
{
	int temp = input_port_5_r (offset);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x80;
	return temp | 0xff00;
}


int atarisys1_6502_switch_r (int offset)
{
	int temp = input_port_4_r (offset);

	if (atarigen_cpu_to_sound_ready) temp ^= 0x08;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x10;
	if (!(input_port_5_r (offset) & 0x40)) temp ^= 0x80;

	return temp;
}



/*************************************
 *
 *		TMS5220 communications
 *
 *************************************/

/*
 *		All communication to the 5220 goes through an SY6522A, which is an overpowered chip
 *		for the job.  Here is a listing of the I/O addresses:
 *
 *			$00	DRB	Data register B
 *			$01	DRA	Data register A
 *			$02	DDRB	Data direction register B (0=input, 1=output)
 *			$03	DDRA	Data direction register A (0=input, 1=output)
 *			$04	T1CL	T1 low counter
 *			$05	T1CH	T1 high counter
 *			$06	T1LL	T1 low latches
 *			$07	T1LH	T1 high latches
 *			$08	T2CL	T2 low counter
 *			$09	T2CH	T2 high counter
 *			$0A	SR		Shift register
 *			$0B	ACR	Auxiliary control register
 *			$0C	PCR	Peripheral control register
 *			$0D	IFR	Interrupt flag register
 *			$0E	IER	Interrupt enable register
 *			$0F	NHDRA	No handshake DRA
 *
 *		Fortunately, only addresses $00,$01,$0B,$0C, and $0F are accessed in the code, and
 *		$0B and $0C are merely set up once.
 *
 *		The ports are hooked in like follows:
 *
 *		Port A, D0-D7 = TMS5220 data lines (i/o)
 *
 *		Port B, D0 = Write strobe (out)
 *		        D1 = Read strobe (out)
 *            D2 = Ready signal (in)
 *		        D3 = Interrupt signal (in)
 *		        D4 = LED (out)
 *		        D5 = ??? (out)
 */

int atarisys1_6522_r (int offset)
{
	switch (offset)
	{
		case 0x00:	/* DRB */
			return (m6522_drb & m6522_ddrb) | (!tms5220_ready_r () << 2) | (!tms5220_int_r () << 3);

		case 0x01:	/* DRA */
		case 0x0f:	/* NHDRA */
			return (m6522_dra & m6522_ddra);

		case 0x02:	/* DDRB */
			return m6522_ddrb;

		case 0x03:	/* DDRA */
			return m6522_ddra;

		default:
			return m6522_regs[offset & 15];
	}
}


void atarisys1_6522_w (int offset, int data)
{
	int old;

	switch (offset)
	{
		case 0x00:	/* DRB */
			old = m6522_drb;
			m6522_drb = (m6522_drb & ~m6522_ddrb) | (data & m6522_ddrb);
			if (!(old & 1) && (m6522_drb & 1))
				tms5220_data_w (0, m6522_dra);
			if (!(old & 2) && (m6522_drb & 2))
				m6522_dra = (m6522_dra & m6522_ddra) | (tms5220_status_r (0) & ~m6522_ddra);
			break;

		case 0x01:	/* DRA */
		case 0x0f:	/* NHDRA */
			m6522_dra = (m6522_dra & ~m6522_ddra) | (data & m6522_ddra);
			break;

		case 0x02:	/* DDRB */
			m6522_ddrb = data;
			break;

		case 0x03:	/* DDRA */
			m6522_ddra = data;
			break;

		default:
			m6522_regs[offset & 15] = data;
			break;
	}
}



/*************************************
 *
 *		Speed cheats
 *
 *************************************/

int marble_speedcheck_r (int offset)
{
	int result = READ_WORD (&marble_speedcheck[offset]);

	if (offset == 2 && result == 0)
	{
		int time = cpu_gettotalcycles ();
		if (time - speedcheck_time1 < 100 && speedcheck_time1 - speedcheck_time2 < 100)
			cpu_spinuntil_int ();

		speedcheck_time2 = speedcheck_time1;
		speedcheck_time1 = time;
	}

	return result;
}

void marble_speedcheck_w (int offset, int data)
{
	COMBINE_WORD_MEM (&marble_speedcheck[offset], data);
	speedcheck_time1 = cpu_gettotalcycles () - 1000;
	speedcheck_time2 = speedcheck_time1 - 1000;
}
