/*************************************************************************

  Toobin' machine hardware

*************************************************************************/

#include "machine/atarigen.h"
#include "sndhrdw/5220intf.h"
#include "vidhrdw/generic.h"
#include "M6502/m6502.h"
#include "m68000/m68000.h"


void toobin_update_display_list (int scanline);



/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *toobin_interrupt_scan;



/*************************************
 *
 *		Statics
 *
 *************************************/

static int interrupt_acked;

static void *interrupt_timer;



/*************************************
 *
 *		Initialization of globals.
 *
 *************************************/

static void toobin_soundint (void)
{
	cpu_cause_interrupt (0, 2);
}


void toobin_init_machine(void)
{
	atarigen_init_machine (toobin_soundint, 0);
	interrupt_acked = 1;
	interrupt_timer = 0;
}



/*************************************
 *
 *		Interrupt handlers.
 *
 *************************************/

int toobin_interrupt (void)
{
	/* set a timer to reset the video parameters just before the end of VBLANK */
	timer_set (TIME_IN_USEC (Machine->drv->vblank_duration - 10), 0, toobin_update_display_list);
	return ignore_interrupt ();
}


void toobin_interrupt_callback (int param)
{
	/* generate the interrupt */
	if (interrupt_acked)
		cpu_cause_interrupt (0, 1);
	interrupt_acked = 0;

	/* set a new timer to go off at the same scan line next frame */
	interrupt_timer = timer_set (TIME_IN_HZ (Machine->drv->frames_per_second), 0, toobin_interrupt_callback);
}


void toobin_interrupt_scan_w (int offset, int data)
{
	int oldword = READ_WORD (&toobin_interrupt_scan[offset]);
	int newword = COMBINE_WORD (oldword, data);

	/* if something changed, update the word in memory */
	if (oldword != newword)
	{
		WRITE_WORD (&toobin_interrupt_scan[offset], newword);

		/* if this is offset 0, modify the timer */
		if (!offset)
		{
			/* remove any previous timer and set a new one */
			if (interrupt_timer)
				timer_remove (interrupt_timer);
			interrupt_timer = timer_set (cpu_getscanlinetime (newword & 0x1ff), 0, toobin_interrupt_callback);
		}
	}
}


void toobin_interrupt_ack_w (int offset, int data)
{
	interrupt_acked = 1;
}


int toobin_sound_interrupt (void)
{
	return M6502_INT_IRQ;
}



/*************************************
 *
 *		I/O read dispatch.
 *
 *************************************/

int toobin_io_r (int offset)
{
	static int hblank = 0x8000;
	int result = input_port_3_r (offset) << 8;

	/* fake HBLANK by just toggling it every read */
	result ^= hblank ^= 0x8000;
	if (atarigen_cpu_to_sound_ready) result ^= 0x2000;

	return result | 0xff;
}


int toobin_6502_switch_r (int offset)
{
	int result = input_port_2_r (offset);

	if (!(input_port_3_r (offset) & 0x10)) result ^= 0x80;
	if (atarigen_cpu_to_sound_ready) result ^= 0x40;
	if (atarigen_sound_to_cpu_ready) result ^= 0x20;

	return result;
}



/*************************************
 *
 *		Controls read
 *
 *************************************/

int toobin_controls_r (int offset)
{
	return (input_port_0_r (offset) << 8) | input_port_1_r (offset);
}



/*************************************
 *
 *		Sound chip reset & bankswitching
 *
 *************************************/

void toobin_sound_reset_w (int offset, int data)
{
	atarigen_sound_reset ();
}


void toobin_6502_bank_w (int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];


	cpu_setbank (8, &RAM[0x10000 + 0x1000 * ((data >> 6) & 3)]);
}



/*************************************
 *
 *		Speed cheats
 *
 *************************************/



