/***************************************************************************

  machine.c

  Written by Kenneth Lin (kenneth_lin@ai.vancouver.bc.ca)

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809/M6809.h"

extern unsigned char jackal_interrupt_enable;

unsigned char *jackal_rambank = 0;
unsigned char *jackal_spritebank = 0;


void jackal_init_machine(void)
{
	/* Set optimization flags for M6809 */
//	m6809_Flags = M6809_FAST_S | M6809_FAST_U;

	cpu_setbank(1,&((Machine->memory_region[0])[0x4000]));
 	jackal_rambank = &((Machine->memory_region[0])[0]);
	jackal_spritebank = &((Machine->memory_region[0])[0]);
}



int jackal_zram_r(int offset)
{
	return jackal_rambank[0x0020+offset];
}


int jackal_commonram_r(int offset)
{
	return jackal_rambank[0x0060+offset];
}


int jackal_commonram1_r(int offset)
{
	return (Machine->memory_region[0])[0x0060+offset];
}


int jackal_voram_r(int offset)
{
	return jackal_rambank[0x2000+offset];
}


int jackal_spriteram_r(int offset)
{
	return jackal_spritebank[0x3000+offset];
}


void jackal_rambank_w(int offset,int data)
{
	jackal_rambank = &((Machine->memory_region[0])[((data & 0x10) << 12)]);
	jackal_spritebank = &((Machine->memory_region[0])[((data & 0x08) << 13)]);
	cpu_setbank(1,&((Machine->memory_region[0])[((data & 0x20) << 11) + 0x4000]));
}


void jackal_zram_w(int offset,int data)
{
	jackal_rambank[0x0020+offset] = data;
}


void jackal_commonram_w(int offset,int data)
{
	jackal_rambank[0x0060+offset] = data;
}


void jackal_commonram1_w(int offset,int data)
{
	(Machine->memory_region[0])[0x0060+offset] = data;
	(Machine->memory_region[2])[0x6060+offset] = data;
}


void jackal_voram_w(int offset,int data)
{
	if ((offset & 0xF800) == 0)
	{
		dirtybuffer[offset & 0x3FF] = 1;
	}
	jackal_rambank[0x2000+offset] = data;
}


void jackal_spriteram_w(int offset,int data)
{
	jackal_spritebank[0x3000+offset] = data;
}
