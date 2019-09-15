/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

int lwings_bank_register=0xff;

void lwings_bankswitch_w(int offset,int data)
{
	int bankaddress = 0x10000 + (data & 0x06) * 0x1000 * 2;
	cpu_setbank(1,&ROM[bankaddress]);

	lwings_bank_register=data;

}

int lwings_interrupt(void)
{
	return 0x00d7;     /* RST 10h */
}

int avengers_interrupt(void)
{
	static int n;
	if (osd_key_pressed(OSD_KEY_S))
	{
		while (osd_key_pressed(OSD_KEY_S))
		{}
		n++;
		n&=0x0f;
		ADPCM_trigger(0, n);
	}
	if (!(lwings_bank_register & 0x20)) /* Interrupts enabled bit */
	{
		static int s;
		s=!s;
		if (s)
		{
			cpu_cause_interrupt(0, 0xd7);
		}
		else
		{
			return Z80_NMI_INT;
		}
	}

	return Z80_IGNORE_INT;
}

