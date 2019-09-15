/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

extern unsigned char *tnzs_workram;
int current_inputport;
int number_of_credits;

void tnzs_bankswitch_w (int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if (errorlog && (data < 0x10 || data > 0x17))
	{
		fprintf(errorlog, "WARNING: writing %02x to bankswitch\n", data);
		return;
	}

	cpu_setbank (1, &RAM[0x10000 + 0x4000 * (data & 0x07)]);
}

void tnzs_bankswitch1_w (int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	cpu_setbank (2, &RAM[0x10000 + 0x2000 * (data & 3)]);
}

void tnzs_init_machine (void)
{
    current_inputport = -3;
    number_of_credits = 0;

    /* Set up the banks */
    tnzs_bankswitch_w  (0, 0);
    tnzs_bankswitch1_w (0, 0);
}

int tnzs_workram_r (int offset)
{
	return tnzs_workram[offset];
}

/* Could there be a microcontroller here? */
int tnzs_inputport_r (int offset)
{
	int ret;

	if (offset == 0)
	{
        switch(current_inputport)
		{
			case -3: ret = 0x5a; break;
			case -2: ret = 0xa5; break;
			case -1: ret = 0x55; break;
            case 6: current_inputport = 0; /* fall through */
			case 0: ret = number_of_credits;
				break;
			default:
                ret = readinputport(current_inputport); break;
		}
#if 0
		if (errorlog) fprintf(errorlog, "$c000 is %02x (count %d)\n",
                              ret, current_inputport);
#endif
        current_inputport++;
		return ret;
	}
	else
		return (0x01);			/* 0xE* for TILT */
}

void tnzs_workram_w (int offset, int data)
{
	tnzs_workram[offset] = data;
}

void tnzs_inputport_w (int offset, int data)
{
	if (offset == 0)
	{
        current_inputport = (current_inputport + 5) % 6;
		if (errorlog) fprintf(errorlog,
							  "writing %02x to 0xc000: count set back to %d\n",
                              data, current_inputport);
	}
}
