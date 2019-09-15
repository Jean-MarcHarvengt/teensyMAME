/*************************************************************************

  Atari System 1 machine hardware

*************************************************************************/

#include "atarigen.h"
#include "sndhrdw/5220intf.h"
#include "vidhrdw/generic.h"
#include "M6502/m6502.h"
#include "m68000/m68000.h"


void blstroid_update_display_list (int param);



/*************************************
 *
 *		Globals we own
 *
 *************************************/



/*************************************
 *
 *		Statics
 *
 *************************************/

static int speech_val;
static int last_ctl;



/*************************************
 *
 *		Initialization of globals.
 *
 *************************************/

static void blstroid_soundint (void)
{
	cpu_cause_interrupt (0, 4);
}


void blstroid_init_machine (void)
{
	atarigen_init_machine (blstroid_soundint, 0);
	speech_val = 0x80;
	last_ctl = 0x02;
}



/*************************************
 *
 *		Interrupt handlers.
 *
 *************************************/

int blstroid_interrupt (void)
{
	/* set a timer to reset the video parameters just before the end of VBLANK */
	timer_set (TIME_IN_USEC (Machine->drv->vblank_duration - 10), 0, blstroid_update_display_list);
	return 2;       /* Interrupt vector 2, used by VBlank */
}


int blstroid_sound_interrupt (void)
{
	return interrupt ();
}



/*************************************
 *
 *		I/O read dispatch.
 *
 *************************************/

int blstroid_io_r (int offset)
{
	if (offset == 0)
	{
		static int hblank = 0x10;
		int temp = input_port_2_r (offset);

		if (atarigen_cpu_to_sound_ready) temp ^= 0x40;
		temp ^= hblank ^= 0x10;

		return temp | 0xff00;
	}
	else
		return input_port_3_r (offset) | 0xff00;
}


int blstroid_6502_switch_r (int offset)
{
	int temp = input_port_4_r (offset);

	if (!(input_port_2_r (offset) & 0x80)) temp ^= 0x80;
	if (atarigen_cpu_to_sound_ready) temp ^= 0x40;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x20;
	if (tms5220_ready_r ()) temp ^= 0x10;

	return temp;
}



/*************************************
 *
 *		I/O write dispatch.
 *
 *************************************/

void blstroid_tms5220_w (int offset, int data)
{
	speech_val = data;
}


void blstroid_6502_ctl_w (int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];


	if (((data ^ last_ctl) & 0x02) && (data & 0x02))
		tms5220_data_w (0, speech_val);
	last_ctl = data;
	cpu_setbank (8, &RAM[0x10000 + 0x1000 * ((data >> 6) & 3)]);
}


void blstroid_sound_reset_w (int offset, int data)
{
	atarigen_sound_reset ();
}



/*************************************
 *
 *		Speed cheats
 *
 *************************************/

