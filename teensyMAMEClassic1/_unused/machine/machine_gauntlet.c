/*************************************************************************

  Gauntlet machine hardware

*************************************************************************/

#include "machine/atarigen.h"
#include "sndhrdw/5220intf.h"
#include "vidhrdw/generic.h"
#include "M6502/m6502.h"

int gauntlet_update_display_list (int scanline);



/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *gauntlet_speed_check;



/*************************************
 *
 *		Statics
 *
 *************************************/

static int last_speed_check;

static int speech_val;
static int last_speech_write;



/*************************************
 *
 *		Initialization of globals.
 *
 *************************************/

static void gauntlet_soundint (void)
{
	cpu_cause_interrupt (0, 6);
}


static void generic_init_machine (int slapstic)
{
	last_speed_check = 0;
	last_speech_write = 0x80;
	atarigen_init_machine (gauntlet_soundint, slapstic);
}


void gauntlet_init_machine (void)
{
	generic_init_machine (104);
}


void gaunt2p_init_machine (void)
{
	generic_init_machine (107);
}


void gauntlet2_init_machine (void)
{
	generic_init_machine (106);
}



/*************************************
 *
 *		Interrupt handlers.
 *
 *************************************/

void gauntlet_update (int param)
{
	int yscroll;

	/* update the display list */
	yscroll = gauntlet_update_display_list (param);

	/* reset the timer */
	if (!param)
	{
		int next = 8 - (yscroll & 7);
		timer_set (cpu_getscanlineperiod () * (double)next, next, gauntlet_update);
	}
	else if (param < 240)
		timer_set (cpu_getscanlineperiod () * 8.0, param + 8, gauntlet_update);
}


int gauntlet_interrupt(void)
{
	timer_set (TIME_IN_USEC (Machine->drv->vblank_duration), 0, gauntlet_update);

	return 4;       /* Interrupt vector 4, used by VBlank */
}


int gauntlet_sound_interrupt(void)
{
	return M6502_INT_IRQ;
}



/*************************************
 *
 *		Controller read dispatch.
 *
 *************************************/

int gauntlet_control_r (int offset)
{
	int p1 = input_port_6_r (offset);
	switch (offset)
	{
		case 0:
			return readinputport (p1);
		case 2:
			return readinputport ((p1 != 1) ? 1 : 0);
		case 4:
			return readinputport ((p1 != 2) ? 2 : 0);
		case 6:
			return readinputport ((p1 != 3) ? 3 : 0);
	}
	return 0xffff;
}



/*************************************
 *
 *		I/O read dispatch.
 *
 *************************************/

int gauntlet_io_r (int offset)
{
	int temp;

	switch (offset)
	{
		case 0:
			temp = input_port_5_r (offset);
			if (atarigen_cpu_to_sound_ready) temp ^= 0x20;
			if (atarigen_sound_to_cpu_ready) temp ^= 0x10;
			return temp | 0xff00;

		case 6:
			return atarigen_sound_r (0);
	}
	return 0xffff;
}


int gauntlet_6502_switch_r (int offset)
{
	int temp = 0x30;

	if (atarigen_cpu_to_sound_ready) temp ^= 0x80;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x40;
	if (tms5220_ready_r ()) temp ^= 0x20;
	if (!(input_port_5_r (offset) & 0x08)) temp ^= 0x10;

	return temp;
}



/*************************************
 *
 *		Controller write dispatch.
 *
 *************************************/

void gauntlet_io_w (int offset, int data)
{
	switch (offset)
	{
		case 0x0e:		/* sound CPU reset */
			if (data & 1)
				cpu_halt (1, 1);
			else
			{
				cpu_halt (1, 0);
				cpu_reset (1);
			}
			break;
	}
}



/*************************************
 *
 *		Sound TMS5220 write.
 *
 *************************************/

void gauntlet_tms_w (int offset, int data)
{
	speech_val = data;
}



/*************************************
 *
 *		Sound control write.
 *
 *************************************/

void gauntlet_sound_ctl_w (int offset, int data)
{
	switch (offset & 7)
	{
		case 0:	/* music reset, bit D7, low reset */
			break;

		case 1:	/* speech write, bit D7, active low */
			if (((data ^ last_speech_write) & 0x80) && (data & 0x80))
				tms5220_data_w (0, speech_val);
			last_speech_write = data;
			break;

		case 2:	/* speech reset, bit D7, active low */
			break;

		case 3:	/* speech squeak, bit D7, low = 650kHz clock */
			break;
	}
}


/*************************************
 *
 *		Speed cheats
 *
 *************************************/

int gauntlet_68010_speedup_r (int offset)
{
	int result = READ_WORD (&gauntlet_speed_check[offset]);

	if (offset == 2)
	{
		int time = cpu_gettotalcycles ();
		int delta = time - last_speed_check;

		last_speed_check = time;
		if (delta <= 100 && result == 0 && delta >= 0)
			cpu_spin ();
	}

	return result;
}


void gauntlet_68010_speedup_w (int offset, int data)
{
	if (offset == 2)
		last_speed_check -= 1000;
	COMBINE_WORD_MEM (&gauntlet_speed_check[offset], data);
}


int gauntlet_6502_speedup_r (int offset)
{
	extern unsigned char *RAM;
	int result = RAM[0x0211];
	if (cpu_getpreviouspc() == 0x412a && RAM[0x0211] == RAM[0x0210] && RAM[0x0225] == RAM[0x0224])
		cpu_spin ();
	return result;
}
