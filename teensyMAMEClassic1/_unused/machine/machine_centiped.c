/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

/*
 * This wrapper routine is necessary because Centipede requires a direction bit
 * to be set or cleared. The direction bit is held until the mouse is moved
 * again.
 *
 * There is a 4-bit counter, and two inputs from the trackball: DIR and CLOCK.
 * CLOCK makes the counter move in the direction of DIR. Since DIR is latched
 * only when a CLOCK arrives, the DIR bit in the input port doesn't change
 * until the trackball actually moves.
 *
 * There is also a CLR input to the counter which could be used by the game to
 * clear the counter, but Centipede doesn't use it (though it would be a good
 * idea to support it anyway).
 *
 * The counter is read 240 times per second. There is no provision whatsoever
 * to prevent the counter from wrapping around between reads.
 */
int centiped_IN0_r(int offset)
{
	static int counter, sign;
	int delta;

	delta = readinputport(6);
	if (delta != 0)
	{
		counter = (counter + delta) & 0x0f;
		sign = delta & 0x80;
	}

	return (readinputport(0) | counter | sign );
}

int centiped_IN2_r(int offset)
{
	static int counter, sign;
	int delta;

	delta = readinputport(2);
	if (delta != 0)
	{
		counter = (counter + delta) & 0x0f;
		sign = delta & 0x80;
	}

	return (counter | sign );
}
