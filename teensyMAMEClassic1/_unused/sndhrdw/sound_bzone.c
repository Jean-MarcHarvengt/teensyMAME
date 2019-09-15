#include "driver.h"

/*

Battlezone sound info, courtesy of Al Kossow:

D7	motor enable			this enables the engine sound
D6	start LED
D5	sound enable			this enables ALL sound outputs
							including the POKEY output
D4	engine rev en			this controls the engine speed
							the engine sound is an integrated square
							wave (saw tooth) that is frequency modulated
							by engine rev.
D3	shell loud, soft/		explosion volume
D2	shell enable
D1	explosion loud, soft/	explosion volume
D0	explosion enable		gates a noise generator

*/

/* Constants for the sound names in the bzone sample array */
#define kFire1			0
#define kFire2			1
#define kEngine1		2
#define kEngine2		3
#define kExplode1		4
#define kExplode2		5

static int	soundEnable = 0;


void bzone_sounds_w (int offset,int data)
{
	static int lastValue = 0;

	/* Enable/disable all sound output */
	if (data & 0x20)
		soundEnable = 1;
	else soundEnable = 0;

	/* If sound is off, don't bother playing samples */
	if (!soundEnable)
	{
		sample_stop (0); /* Turn off explosion */
		sample_stop (1); /* Turn off engine noise */
		return;
	}

	if (lastValue == data) return;
	lastValue = data;

	/* Enable explosion output */
	if (data & 0x01)
	{
		if (data & 0x02)
			sample_start(0,kExplode1,0);
		else
			sample_start(0,kExplode2,0);
	}

	/* Enable shell output */
	if (data & 0x04)
	{
		if (data & 0x08)
		{
			/* loud shell */
			sample_start(0,kFire1,0);
		}
		else
		{
			/* soft shell */
			sample_start(0,kFire2,0);
		}
	}

	/* Enable engine output */
	if (data & 0x80)
	{
		if (data & 0x10)
		{
			/* Fast rumble */
			sample_start(1,kEngine2,1);
		}
		else
		{
			/* Slow rumble */
			sample_start(1,kEngine1,1);
		}
	}
}

void bzone_pokey_w (int offset,int data)
{
	if (soundEnable)
		pokey1_w (offset, data);
}
