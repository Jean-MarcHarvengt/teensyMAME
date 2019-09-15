/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

extern unsigned char spacefb_vref;

void spacefb_port_0_w(int offset,int data)
{
	spacefb_vref = (data & (1<<5)) ? 128 : 0;
}

void spacefb_port_2_w(int offset,int data)
{
}

void spacefb_port_3_w(int offset,int data)
{
}

int spacefb_interrupt(void)
{
	static int count;

	count++;

	if (count & 1)
		return (0x00cf);		/* RST 08h */
	return (0x00d7);		/* RST 10h */

}
