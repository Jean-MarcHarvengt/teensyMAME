/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80/Z80.h"


int panic_interrupt(void)
{
	static int count=0;
    int ans=0;

	count++;

    /* Apparently, only the first 2 ever get called on the real thing! */

	switch(count)
	{
		case 1: ans = 0x00cf;		/* RST 08h */
        	    break;

        case 2: ans = 0x00d7;		/* RST 10h */
                count=0;
                break;

#if 0
        case 3: ans = 0x00df;		/* RST 18h */
                break;

        case 4: ans = 0x00e7;		/* RST 20h */
                break;
#endif

    };

    return ans;

}
