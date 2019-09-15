#include "driver.h"
#include "machine/Z80fmly.h"
#include <math.h>

/* mixing level */
#define SINGLE_VOLUME 32
#define SSG_VOLUME 255

/* z80 pio */
static void pio_interrupt(int state)
{
	cpu_cause_interrupt (1, Z80_VECTOR(0,state) );
}

static z80pio_interface pio_intf =
{
	1,
	{pio_interrupt},
	{0},
	{0}
};

/* z80 ctc */
static void ctc_interrupt (int state)
{
	cpu_cause_interrupt (1, Z80_VECTOR(1,state) );
}

static z80ctc_interface ctc_intf =
{
	1,                   /* 1 chip */
	{ 0 },               /* clock (filled in from the CPU 0 clock */
	{ NOTIMER_2 },       /* timer disables */
	{ ctc_interrupt },   /* interrupt handler */
	{ z80ctc_0_trg1_w }, /* ZC/TO0 callback */
	{ 0 },               /* ZC/TO1 callback */
	{ 0 }                /* ZC/TO2 callback */
};


/* single tone generator */
#define SINGLE_LENGTH 10000
#define SINGLE_DIVIDER 8

static signed char *_single;
static int single_rate = 1000;
static int single_volume = 0;

void starforc_volume_w( int offset , int data )
{
	single_volume = ((data & 0x0f)<<4)|(data & 0x0f);
}

static struct SN76496interface interface =
{
	3,	/* 3 chips */
	2000000,	/* 2 Mhz? */
	{ 255, 255, 255 }
};

int starforc_sh_start(void)
{
	int i;


	if (SN76496_sh_start(&interface) != 0)
		return 1;

	/* z80 ctc init */
	ctc_intf.baseclock[0] = Machine->drv->cpu[1].cpu_clock;
	z80ctc_init (&ctc_intf);

	/* z80 pio init */
	z80pio_init (&pio_intf);

	/* setup daisy chain connection */
	{
		static Z80_DaisyChain daisy_chain[] =
		{
			{ z80pio_reset , z80pio_interrupt, z80pio_reti , 0 }, /* device 0 = PIO_0 , low  priority */
			{ z80ctc_reset , z80ctc_interrupt, z80ctc_reti , 0 }, /* device 1 = CTC_0 , high priority */
			{ 0,0,0,-1}        /* end mark */
		};
		cpu_setdaisychain (1,daisy_chain );
/*
	daisy_chain is connect link for Z80 daisy-chain .
	paramater is
	{ pointer of reset , pointer of interrupt entry,pointer of RETI handler , device paramater }

	reset           : This function is called when z80 cpu reset
	interrupt entry : This function is called when z80 interrupt entry for this device
	                  It shoud be change interrupt status and
	                  set new status with cpu_cause_interrupt function
	                  return value is interrupt vector

	RETI handler    : This function is called when z80 reti operation for this device
	                  It shoud be change interrupt status and
	                  set new status with cpu_cause_interrupt function

	handler shoud be allocate static.
	Because this pointers is used when reset cpu.

	The daisy chain link is build by this pointers.

	daisy chain priority:
		As for Priority , the top side is lower , bottom side is higher

*/
	}

	if ((_single = (signed char *)malloc(SINGLE_LENGTH)) == 0)
	{
		SN76496_sh_stop();
		free(_single);
		return 1;
	}
	for (i = 0;i < SINGLE_LENGTH;i++)		/* freq = ctc2 zco / 8 */
		_single[i] = ((i/SINGLE_DIVIDER)&0x01)*(SINGLE_VOLUME/2);

	/* CTC2 single tone generator */
	osd_play_sample(4,_single,SINGLE_LENGTH,single_rate,single_volume,1);

	return 0;
}



void starforc_sh_stop(void)
{
	SN76496_sh_stop();
	free(_single);
}



void starforc_sh_update(void)
{
	double period;

	if (Machine->sample_rate == 0) return;

	SN76496_sh_update();

	/* ctc2 timer single tone generator frequency */
	period = z80ctc_getperiod (0, 2);
	if( period != 0 ) single_rate = (int)(1.0 / period );
	else single_rate = 0;

	osd_adjust_sample(4,single_rate,single_volume );
}
