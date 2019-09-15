/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


static int interrupt_enable;


void espial_init_machine(void)
{
	/* we must start with NMI interrupts disabled */
	interrupt_enable = 0;
}



void espial_interrupt_enable_w(int offset,int data)
{
	interrupt_enable = !(data & 1);
}

int espial_interrupt(void)
{
	if (cpu_getiloops() != 0)
	{
		return interrupt();
	}
	if (interrupt_enable) return nmi_interrupt();
	else return interrupt();
}

int espial_sh_interrupt(void)
{
	if (interrupt_enable) return nmi_interrupt();
	else return ignore_interrupt();
}
