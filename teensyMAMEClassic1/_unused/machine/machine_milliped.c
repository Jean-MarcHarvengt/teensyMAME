/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

/*
 * This wrapper routine is necessary because Millipede requires a direction bit
 * to be set or cleared. The direction bit is held until the mouse is moved
 * again. We still don't understand why the difference between
 * two consecutive reads must not exceed 7. After all, the input is 4 bits
 * wide, and we have a fifth bit for the sign...
 *
 * The other reason it is necessary is that Millipede uses the same address to
 * read the dipswitches.
 * JB 971220, BW 980121
 */

int milliped_IN0_r (int offset)
{
	static int counter, sign;
	int delta;

	/* Hack: return dipswitch when 3000 cycles remain before interrupt. */
	if (cpu_geticount () < 3000)
		return (readinputport (0) | sign);

	delta=readinputport(6);
	if (delta !=0)
	{
		counter=(counter+delta) & 0x0f;
		sign = delta & 0x80;
	}

	return ((readinputport(0) & 0x70) | counter | sign );
}

int milliped_IN1_r (int offset)
{
	static int counter, sign;
	int delta;

	/* Hack: return dipswitch when 3000 cycles remain before interrupt. */
	if (cpu_geticount () < 3000)
		return (readinputport (1) | sign);

	delta=readinputport(7);
	if (delta !=0)
	{
		counter=(counter+delta) & 0x0f;
		sign = delta & 0x80;
	}

	return ((readinputport(1) & 0x70) | counter | sign );
}
