#include "driver.h"
#include "m6502/m6502.h"
#include "machine/6821pia.h"

static void *timer;
#define BASE_TIME 1/.894
#define BASE_FREQ 1789773*8
int exidy_sample_channels[3];
unsigned int exidy_sh8253_count[3]; 	/* 8253 Counter */
int exidy_sh8253_clstate[3];			/* which byte to load */
int flag;

static signed char exidy_waveform1[16] =
{
	/* square-wave */
	0x00, 0x7F, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x7F,
	0x00, 0x7F, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x7F
};

int exidy_shdata_latch = 0xFF;
int exidy_mhdata_latch = 0xFF;

static int irq_flag = 0;   /* 6532 interrupt flag register */
static int irq_enable = 0;
static int PA7_irq = 0;  /* IRQ-on-write flag (sound CPU) */


/***************************************************************************
	PIA Interface
***************************************************************************/

static void exidy_irq (void);

static pia6821_interface pia_intf =
{
	2,                                              /* 2 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ pia_2_portb_r, pia_1_portb_r },               /* input port A */
	{ pia_2_cb2_r,   pia_1_cb2_r },                 /* input bit CA1 */
	{ pia_2_cb1_r,   pia_1_cb1_r },                 /* input bit CA2 */
	{ pia_2_porta_r, pia_1_porta_r },               /* input port B */
	{ pia_2_ca2_r,   pia_1_ca2_r },                 /* input bit CB1 */
	{ pia_2_ca1_r,   pia_1_ca1_r },                 /* input bit CB2 */
	{ pia_2_portb_w, pia_1_portb_w },               /* output port A */
	{ pia_2_porta_w, pia_1_porta_w },               /* output port B */
	{ pia_2_cb1_w,   pia_1_cb1_w },                 /* output CA2 */
	{ pia_2_ca1_w,   pia_1_ca1_w },                 /* output CB2 */
	{ 0, exidy_irq },                               /* IRQ A */
	{ 0, 0 }                                        /* IRQ B */
};

int exidy_sh_start(void)
{
	flag=0;
	/* Init 8253 */
	exidy_sh8253_clstate[0]=0;
	exidy_sh8253_clstate[1]=0;
	exidy_sh8253_clstate[2]=0;
	exidy_sh8253_count[0]=0;
	exidy_sh8253_count[1]=0;
	exidy_sh8253_count[2]=0;


	exidy_sample_channels[0] = get_play_channels(1);
	exidy_sample_channels[0] = get_play_channels(2);
	exidy_sample_channels[0] = get_play_channels(3);
	osd_play_sample(exidy_sample_channels[0],(signed char*)exidy_waveform1,16,1000,0xFF,1);
	osd_play_sample(exidy_sample_channels[1],(signed char*)exidy_waveform1,16,1000,0xFF,1);
	osd_play_sample(exidy_sample_channels[2],(signed char*)exidy_waveform1,16,1000,0xFF,1);

	pia_startup (&pia_intf);

	return 0;
}

void exidy_sh_stop(void)
{
	osd_stop_sample(exidy_sample_channels[0]);
	osd_stop_sample(exidy_sample_channels[1]);
	osd_stop_sample(exidy_sample_channels[2]);

}

static void riot_interrupt(int parm)
{
	irq_flag |= 0x80; /* set timer interrupt flag */
	cpu_cause_interrupt (1, M6502_INT_IRQ);
}

/*
 *  PIA callback to generate the interrupt to the main CPU
 */

static void exidy_irq (void)
{
    cpu_cause_interrupt (1, M6502_INT_IRQ);
}


void exidy_shriot_w(int offset,int data)
{
   long i=0;

   if (errorlog) fprintf(errorlog,"RIOT: %x=%x\n",offset,data);
   offset &= 0x7F;
   switch (offset)
   {
		case 7: /* 0x87 - Enable Interrupt on PA7 Transitions */
			PA7_irq = data;
			return;
		case 0x1e:
			 irq_enable=1;
			 timer_set (TIME_IN_USEC((64*BASE_TIME)*data), 0, riot_interrupt);
			 return;
		case 0x1f:
			 irq_enable=1;
			 timer_set (TIME_IN_USEC((1024*BASE_TIME)*data), 0, riot_interrupt);
			 return;
		default:
			 return;
	}
	return; /* will never execute this */
}


int exidy_shriot_r(int offset)
{
	static int temp;

    if (errorlog) fprintf(errorlog,"RIOT(r): %x\n",offset);
	offset &= 0x7f;
	switch (offset)
	{
		case 5: /* 0x85 - Read Interrupt Flag Register */
		   temp = irq_flag;
		   irq_flag = 0;   /* Clear int flags */
		   return temp;
		default:
		   return 0;
	 }
	return 0;
}

void exidy_sh8253_w(int offset,int data)
{
	int i,c;
	long f;


    if (errorlog) fprintf(errorlog,"8253: %x=%x\n",offset,data);

	i = offset & 0x03;
	if (i == 0x03) {
		c = (data & 0xc0) >> 6;
		if (exidy_sh8253_count[c])
			f = BASE_FREQ / exidy_sh8253_count[c];
		else
			f=0;
		if ((data & 0x0E) == 0)
			osd_adjust_sample(exidy_sample_channels[c],f,0x00);
		else
			osd_adjust_sample(exidy_sample_channels[c],f,0xFF);
	}

	if (i < 0x03)
	{
		if (!exidy_sh8253_clstate[i])
		{
			exidy_sh8253_clstate[i]=1;
			exidy_sh8253_count[i] &= 0xFF00;
			exidy_sh8253_count[i] |= (data & 0xFF);

		}
		else
		{
			exidy_sh8253_clstate[i]=0;
			exidy_sh8253_count[i] &= 0x00FF;
			exidy_sh8253_count[i] |= ((data & 0xFF) << 8);
			if (errorlog) fprintf(errorlog,"HIGH BYTE=%x\n",exidy_sh8253_count[i]);

			f = BASE_FREQ / exidy_sh8253_count[i];
			osd_adjust_sample(exidy_sample_channels[i],f,0xFF);
			if (errorlog) fprintf(errorlog,"Timer %d = %u\n",i,exidy_sh8253_count[i]);
		}
	}


}

int exidy_sh8253_r(int offset)
{
//	  if (errorlog) fprintf(errorlog,"8253(R): %x\n",offset);
	return 0;
}

