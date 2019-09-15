#include "driver.h"
#include "m6808.h"



static int port1;
static int port2;
static int ddr1;
static int ddr2;



void irem_sound_cmd_w(int offset,int data)
{
	if ((data & 0x80) == 0)
		soundlatch_w(0,data & 0x7f);
	else
		cpu_cause_interrupt(1,M6808_INT_IRQ);
}


void irem_io_w(int offset,int data)
{
	switch (offset)
	{
		/* port 1 DDR */
		case 0:
			ddr1 = data;
			break;

		/* port 2 DDR */
		case 1:
			ddr2 = data;
			break;

		/* port 1 */
		case 2:
			data = (port1 & ~ddr1) | (data & ddr1);
			port1 = data;
			break;

		/* port 2 */
		case 3:
			data = (port2 & ~ddr2) | (data & ddr2);

			/* write latch */
			if ((port2 & 0x01) && !(data & 0x01))
			{
				/* control or data port? */
				if (port2 & 0x04)
				{
					/* PSG 0 or 1? */
					if (port2 & 0x10)
						AY8910_control_port_1_w(0,port1);
					else if (port2 & 0x08)
						AY8910_control_port_0_w(0,port1);
				}
				else
				{
					/* PSG 0 or 1? */
					if (port2 & 0x10)
						AY8910_write_port_1_w(0,port1);
					else if (port2 & 0x08)
						AY8910_write_port_0_w(0,port1);
				}
			}
			port2 = data;
			break;
	}
}


int irem_io_r(int offset)
{
	switch (offset)
	{
		/* port 1 DDR */
		case 0:
			return ddr1;
			break;

		/* port 2 DDR */
		case 1:
			return ddr2;
			break;

		/* port 1 */
		case 2:

			/* input 0 or 1? */
			if (port2 & 0x10)
				return (AY8910_read_port_1_r(0) & ~ddr1) | (port1 & ddr1);
			else if (port2 & 0x08)
				return (AY8910_read_port_0_r(0) & ~ddr1) | (port1 & ddr1);
//			return (port1 & ddr1);
			break;

		/* port 2 */
		case 3:
//			return (port2 & ddr2);
			break;
	}

	return 0;
}


static void irem_adpcm_reset_w(int offset,int data)
{
	MSM5205_reset_w(0,data & 1);
	MSM5205_reset_w(1,data & 2);
}


static void irem_adpcm_int (int data)
{
	cpu_cause_interrupt(1,M6808_INT_NMI);
}


struct AY8910interface irem_ay8910_interface =
{
	2,	/* 2 chips */
	910000,	/* .91 MHZ ?? */
	{ 25, 25 },
	{ soundlatch_r, 0 },
	{ 0 },
	{ 0 },
	{ irem_adpcm_reset_w, 0 }
};

struct MSM5205interface irem_msm5205_interface =
{
	2,			/* 2 chips */
	4000,       /* 4000Hz playback */
	irem_adpcm_int,	/* interrupt function */
	{ 80, 80 }
};
