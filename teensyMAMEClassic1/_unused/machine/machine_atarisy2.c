/*************************************************************************

  atarisys2' machine hardware

*************************************************************************/

#include "machine/atarigen.h"
#include "sndhrdw/5220intf.h"
#include "vidhrdw/generic.h"
#include "M6502/m6502.h"
#include "t11/t11.h"

extern void atarisys2_update_display_list (int scanline);



/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *atarisys2_interrupt_enable;
unsigned char *atarisys2_bankselect;



/*************************************
 *
 *		Statics
 *
 *************************************/

static int tms5220_data;
static int tms5220_data_strobe;

static int last_sound_reset;

static int irq_hold0, irq_hold2, irq_hold3;

static int which_adc;

static int pedal_value[3], pedal_count;



/*************************************
 *
 *		Initialization of globals.
 *
 *************************************/

static void atarisys2_soundint (void)
{
	if (!irq_hold0 && (READ_WORD (&atarisys2_interrupt_enable[0]) & 1))
	{
		cpu_cause_interrupt (0, 0);
		irq_hold0 = 1;
	}
}


static void atarisys2_init_machine (int slapstic)
{
	int ch;


	atarigen_init_machine (atarisys2_soundint, slapstic);
	last_sound_reset = 0;
	irq_hold0 = irq_hold2 = irq_hold3 = 1;
	tms5220_data_strobe = 1;
	/* set panning for the two pokey chips */
	for (ch = 0;ch < MAX_STREAM_CHANNELS;ch++)
	{
		if (stream_get_name(ch) != 0 &&
				!strcmp(stream_get_name(ch),"Pokey #0"))
			stream_set_pan(ch,OSD_PAN_LEFT);
		if (stream_get_name(ch) != 0 &&
				!strcmp(stream_get_name(ch),"Pokey #1"))
			stream_set_pan(ch,OSD_PAN_RIGHT);
	}
	which_adc = 0;
	pedal_value[0] = pedal_value[1] = pedal_value[2] = 0;
	pedal_count = 0;
}


void paperboy_init_machine (void)
{
	atarisys2_init_machine (105);
}


void apb_init_machine (void)
{
	atarisys2_init_machine (110);
	pedal_count = 1;
}


void a720_init_machine (void)
{
	atarisys2_init_machine (107);
}


void ssprint_init_machine (void)
{
	atarisys2_init_machine (108);
	pedal_count = 3;
}


void csprint_init_machine (void)
{
	atarisys2_init_machine (109);
	pedal_count = 2;
}



/*************************************
 *
 *		Interrupt handlers.
 *
 *************************************/

void atarisys2_32v_interrupt (int param)
{
	/* generate the 32V interrupt (IRQ 2) */
	if (!irq_hold2 && (READ_WORD (&atarisys2_interrupt_enable[0]) & 4))
	{
		cpu_cause_interrupt (0, 2);
		irq_hold2 = 1;
	}

	/* set the timer for the next one */
	param += 64;
	if (param < 384)
		timer_set (64.0 * cpu_getscanlineperiod (), param, atarisys2_32v_interrupt);
}


void atarisys2_video_update (int param)
{
	atarisys2_update_display_list (param);
	param += 8;
	if (param < 384)
		timer_set (8.0 * cpu_getscanlineperiod (), param, atarisys2_video_update);
}


int atarisys2_interrupt (void)
{
    int i;

	/* set the 32V timer */
	timer_set (TIME_IN_USEC (Machine->drv->vblank_duration), 0, atarisys2_32v_interrupt);
	timer_set (TIME_IN_USEC (Machine->drv->vblank_duration), 0, atarisys2_video_update);

	/* update the pedals once per frame */
    for (i = 0; i < pedal_count; i++)
	{
		if (readinputport (3 + i) & 0x80)
		{
			pedal_value[i] += 64;
			if (pedal_value[i] > 0xff) pedal_value[i] = 0xff;
		}
		else
		{
			pedal_value[i] -= 64;
			if (pedal_value[i] < 0) pedal_value[i] = 0;
		}
	}

	/* VBLANK is 3 */
	if (!irq_hold3 && (READ_WORD (&atarisys2_interrupt_enable[0]) & 8))
	{
		irq_hold3 = 1;
		return 3;
	}
	else
		return ignore_interrupt ();
}


void atarisys2_interrupt_ack_w (int offset, int data)
{
	/* reset sound IRQ */
	if (offset == 0x00)
		irq_hold0 = 0;

	/* reset sound CPU */
	else if (offset == 0x20)
	{
		if (last_sound_reset == 0 && (data & 1))
			atarigen_sound_reset ();
		last_sound_reset = data & 1;
	}

	/* reset 32V IRQ */
	else if (offset == 0x40)
		irq_hold2 = 0;

	/* reset VBLANK IRQ */
	else if (offset == 0x60)
		irq_hold3 = 0;
}


int atarisys2_sound_interrupt (void)
{
	return interrupt ();
}


void atarisys2_watchdog_w (int offset, int data)
{
	watchdog_reset_w (offset, data);
}



/*************************************
 *
 *		Bank selection.
 *
 *************************************/

void atarisys2_bankselect_w (int offset, int data)
{
	static int bankoffset[64] =
	{
		0x28000, 0x20000, 0x18000, 0x10000,
		0x2a000, 0x22000, 0x1a000, 0x12000,
		0x2c000, 0x24000, 0x1c000, 0x14000,
		0x2e000, 0x26000, 0x1e000, 0x16000,
		0x48000, 0x40000, 0x38000, 0x30000,
		0x4a000, 0x42000, 0x3a000, 0x32000,
		0x4c000, 0x44000, 0x3c000, 0x34000,
		0x4e000, 0x46000, 0x3e000, 0x36000,
		0x68000, 0x60000, 0x58000, 0x50000,
		0x6a000, 0x62000, 0x5a000, 0x52000,
		0x6c000, 0x64000, 0x5c000, 0x54000,
		0x6e000, 0x66000, 0x5e000, 0x56000,
		0x88000, 0x80000, 0x78000, 0x70000,
		0x8a000, 0x82000, 0x7a000, 0x72000,
		0x8c000, 0x84000, 0x7c000, 0x74000,
		0x8e000, 0x86000, 0x7e000, 0x76000
	};

	int oldword = READ_WORD (&atarisys2_bankselect[offset]);
	int newword = COMBINE_WORD (oldword, data);
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	unsigned char *base = &RAM[bankoffset[(newword >> 10) & 0x3f]];

	WRITE_WORD (&atarisys2_bankselect[offset], newword);
	if (offset == 0)
	{
		cpu_setbank (1, base);
		t11_SetBank (0x4000, base);
	}
	else if (offset == 2)
	{
		cpu_setbank (2, base);
		t11_SetBank (0x6000, base);
	}
}



/*************************************
 *
 *		I/O read dispatch.
 *
 *************************************/

int atarisys2_switch_r (int offset)
{
	int result = input_port_1_r (offset) | (input_port_2_r (offset) << 8);

	if (atarigen_cpu_to_sound_ready) result ^= 0x20;
	if (atarigen_sound_to_cpu_ready) result ^= 0x10;

	return result;
}


int atarisys2_6502_switch_r (int offset)
{
	int result = input_port_0_r (offset);

	if (atarigen_cpu_to_sound_ready) result ^= 0x01;
	if (atarigen_sound_to_cpu_ready) result ^= 0x02;
	if (tms5220_ready_r ()) result ^= 0x04;
	if (!(input_port_2_r (offset) & 0x80)) result ^= 0x10;

	return result;
}


void atarisys2_6502_switch_w (int offset, int data)
{
}



/*************************************
 *
 *		Controls read
 *
 *************************************/

void atarisys2_adc_strobe_w (int offset, int data)
{
	which_adc = (offset / 2) & 3;
}


int atarisys2_adc_r (int offset)
{
    if (which_adc == 1 && pedal_count == 1)   /* APB */
        return ~pedal_value[0];

	if (which_adc < pedal_count)
		return (~pedal_value[which_adc]);
	return readinputport (3 + which_adc) | 0xff00;
}


int atarisys2_leta_r (int offset)
{
	return readinputport (7 + (offset & 3));
}



/*************************************
 *
 *		Global sound control
 *
 *************************************/

void atarisys2_mixer_w (int offset, int data)
{
}


void atarisys2_sound_enable_w (int offset, int data)
{
}



/*************************************
 *
 *		Speech chip
 *
 *************************************/

void atarisys2_tms5220_w (int offset, int data)
{
	tms5220_data = data;
}


void atarisys2_tms5220_strobe_w (int offset, int data)
{
	if (!(offset & 1) && tms5220_data_strobe)
		tms5220_data_w (0, tms5220_data);
	tms5220_data_strobe = offset & 1;
}



/*************************************
 *
 *		Speed cheats
 *
 *************************************/



