#include "driver.h"

static int sound_enabled = 0;

void dday_sound_enable(int enabled)
{
	/* If sound is being turned off, need to do an update right away,
	   otherwise the Morse sound would become one continous tone */
	if (sound_enabled && !enabled)
	{
		AY8910_reset(0);
		AY8910_reset(1);
	}

	sound_enabled = enabled;
}


static void common_AY8910_w(int offset, int data,
				            void (*write_port)(int, int),
				            void (*control_port)(int, int))
{
	/* Get out if sound is off */
	if (!sound_enabled) return;

	if (offset & 1)
	{
		write_port(0, data);
	}
	else
	{
		control_port(0, data);
	}
}

void dday_AY8910_0_w(int offset, int data)
{
	common_AY8910_w(offset, data, AY8910_write_port_0_w, AY8910_control_port_0_w);
}

void dday_AY8910_1_w(int offset, int data)
{
	common_AY8910_w(offset, data, AY8910_write_port_1_w, AY8910_control_port_1_w);
}

