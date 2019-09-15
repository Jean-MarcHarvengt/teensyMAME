#include "driver.h"

/*

	Red Baron sound notes:
	$fx - $5x = explosion volume
	$08 = disable all sounds? led?
	$04 = machine gun sound
	$02 = nosedive sound

*/

/* Constants for the sound names in the redbaron sample array */
#define kFire			0
#define kNosedive		1
#define kExplode		2

/* used in drivers/redbaron.c to select joystick pot */
extern int rb_input_select;


void redbaron_sounds_w (int offset,int data)
{
	static int lastValue = 0;

	rb_input_select = (data & 0x01);

	if (lastValue == data) return;
	lastValue = data;

	/* Enable explosion output */
	if (data & 0xf0)
		sample_start (0, kExplode, 0);

	/* Enable nosedive output */
	if (data & 0x02)
		sample_start (0, kNosedive, 0);

	/* Enable firing output */
	if (data & 0x04)
		sample_start (1, kFire, 0);

}
