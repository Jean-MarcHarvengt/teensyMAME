#include "driver.h"
#include "I8039.h"



void mario_sh_w(int offset,int data)
{
	if (data)
		cpu_cause_interrupt(1,I8039_EXT_INT);
}


/* Mario running sample */
void mario_sh1_w(int offset,int data)
{
	static int last;

	if (last!= data)
	{
		last = data;
		if (data) sample_start (0, 4, 0);
	}
}

/* Luigi running sample */
void mario_sh2_w(int offset,int data)
{
	static int last;

	if (last!= data)
	{
		last = data;
		if (data) sample_start (1, 5, 0);
	}
}

/* Misc samples */
void mario_sh3_w (int offset,int data)
{
	static int state[8];

	/* Don't trigger the sample if it's still playing */
	if (state[offset] == data) return;

	state[offset] = data;
	if (data)
	{
		switch (offset)
		{
			case 0: /* death */
				sample_start (2, 0, 0);
				break;
			case 2: /* ice */
				sample_start (2, 1, 0);
				break;
			case 6: /* coin */
				sample_start (2, 2, 0);
				break;
			case 7: /* skid */
				sample_start (2, 3, 0);
				break;
		}
	}
}
