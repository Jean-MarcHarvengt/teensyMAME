/*



On MC6840, from Xenophobe schematics:
	Counter 1 appears to be vertical blank.
	Counter 3 appears to be horizontal blank.
	Counter 2 & gate inputs appear unused.


*/

#include "driver.h"
#include "machine/6821pia.h"

static int m6840_cr_select=3;
static int m6840_timerLSB[3];
static int m6840_timerMSB[3];

static unsigned char soundlatch[4];
static unsigned char soundstatus;
static int dacval;
static int suspended;

/* Sounds Good (SG) PIA interface */
static void sg_porta_w (int offset, int data);
static void sg_portb_w (int offset, int data);
static void sg_irq (void);

static pia6821_interface sg_pia_intf =
{
	1,                /* 1 chip */
	{ PIA_DDRA, PIA_NOP, PIA_DDRB, PIA_NOP, PIA_CTLA, PIA_NOP, PIA_CTLB, PIA_NOP }, /* offsets */
	{ 0 },            /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ 0 },         	/* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ sg_porta_w },   /* output port A */
	{ sg_portb_w },   /* output port B */
	{ 0 },            /* output CA2 */
	{ 0 },            /* output CB2 */
	{ sg_irq },       /* IRQ A */
	{ sg_irq },       /* IRQ B */
};

int mcr68_init_machine(void)
{

	int i;
	for (i=0; i<3; i++) {
		m6840_timerLSB[i]=255;
		m6840_timerMSB[i]=255;
        /* More to come.. */
	}


	/* reset the sound */
	for (i = 0; i < 4; i++)
      soundlatch[i] = 0;
	soundstatus = 0;
	suspended = 0;

	/* reset the PIAs */
	pia_startup (&sg_pia_intf);

	return 0;
}

int mcr68_interrupt(void)
{
	static int on=0;
	static int i=0;

//	int a=input_port_0_r(0);

//	if (!(a&0x01)) cpu_cause_interrupt(0,1);
//	if (!(a&0x02)) cpu_cause_interrupt(0,2);

//	if (!(a&0x01)) on=1;

	/* Timer stuff - timer 3 is VBL? */
	m6840_timerLSB[2]--;
	if (m6840_timerLSB[2]<0) {m6840_timerLSB[2]=255;m6840_timerMSB[2]--;}
	if (m6840_timerMSB[2]<0) {
    	on=1;
        if (errorlog) fprintf(errorlog,"***TIMER3INT***\n");
        cpu_cause_interrupt(0,2);
    }

    /* Other 2 timers disabled for now.. */
 /*
	m6840_timerLSB[1]--;
  if (m6840_timerLSB[1]<0) {m6840_timerLSB[1]=255;m6840_timerMSB[1]--;}
	if (m6840_timerMSB[1]<0) {if (errorlog) fprintf(errorlog,"***TIME2INT***\n");cpu_cause_interrupt(0,2);}

	m6840_timerLSB[0]--;
  if (m6840_timerLSB[0]<0) {m6840_timerLSB[0]=255;m6840_timerMSB[0]--;}
	if (m6840_timerMSB[0]<0) {if (errorlog) fprintf(errorlog,"***TIMER1INT***\n");cpu_cause_interrupt(0,2);}

   */

	/* HBL ? */
	if (on) i++;
	if (i>4) {i=0; return 1;}

	return 0;
}



int mcr68_control_0(int offset)
{
//	int a=readinputport( 0 );
//	if ((a&0x7)!=0x7) cpu_cause_interrupt(0,1);

	return (readinputport( 1 ) << 8 ) + readinputport( 0 );
}

int mcr68_control_1(int offset)
{
	return (readinputport( 2 ) << 8 ) + readinputport( 3 );
}

int mcr68_control_2(int offset)
{
	return readinputport( 4 );
}

int mcr68_6840_r(int offset)
{
	/* From datasheet:
		0 - nothing
2		1 - read status register
4		2 - read timer 1 counter
6		3 - read lsb buffer
8		4 - timer 2 counter
a		5 - read lsb buffer
c		6 - timer 3 counter
e		7 - lsb buffer


	*/

	switch (offset) {
		case 0:
			if (errorlog) fprintf(errorlog,"Read from unimplemented port...\n");
			break;
		case 2:
			if (errorlog) fprintf(errorlog,"Read status register\n");

			return 0x80 + 7;

			break;

		case 4:
			if (errorlog) fprintf(errorlog,"Read MSB of timer 1 (%d)\n",m6840_timerMSB[0]);
			return m6840_timerMSB[0];

		case 6:
			if (errorlog) fprintf(errorlog,"Read LSB of timer 1\n");
			return m6840_timerLSB[0];

		case 8:
			if (errorlog) fprintf(errorlog,"Read MSB of timer 2\n");
			return m6840_timerMSB[1];

		case 10:
			if (errorlog) fprintf(errorlog,"Read LSB of timer 2\n");
			return m6840_timerLSB[1];

		case 12:
			if (errorlog) fprintf(errorlog,"Read MSB of timer 3 (%d)\n",m6840_timerMSB[2]);
			return m6840_timerMSB[2];

		case 14:
			if (errorlog) fprintf(errorlog,"Read LSB of timer 3\n");
			return m6840_timerLSB[2];
	}

	/* Reads are from 2 4 c */

//	if (errorlog) fprintf(errorlog,"Read from 6840 %02x\n",offset);

	return 0;
}


void mcr68_6840_w(int offset, int data)
{
	int byt;

	/* From datasheet:


0		0 - write control reg 1/3 (reg 1 if lsb of Reg 2 is 1, else 3)
2		1 - write control reg 2
4		2 - write msb buffer reg
6		3 - write timer 1 latch
8		4 - msb buffer register
a		5 - write timer 2 latch
c		6 - msb buffer reg
e		7 - write timer 3 latch


So:

Write ff0100 to 6840 02  = }
Write ff0000 to 6840 00  = } Write 0 to control reg 1

Write ff0000 to 6840 02

Write ff0000 to 6840 0c
Write ff3500 to 6840 0e  = } Write 0035 to timer 3 latch?

Write ff0a00 to 6840 00  = } Write 0a to control reg 3

	*/

	byt=(data>>8)&0xff;


	switch (offset) {
		case 0: /* CR1/3 */
			if (m6840_cr_select==1) {

				if (byt&0x1) {
					int i;
				if (errorlog) fprintf(errorlog,"MC6840: Internal reset\n");
				for (i=0; i<3; i++) {
					m6840_timerLSB[i]=255;
					m6840_timerMSB[i]=255;

				}


					}

			}

			else if (m6840_cr_select==3) {
				if (byt&0x1) {
					if (errorlog) fprintf(errorlog,"MC6840: Divide by 8 prescaler selected\n");
				}
			}

			/* Following bits apply to both registers */
			if (byt&0x2) {
				if (errorlog) fprintf(errorlog,"MC6840: Internal clock selected on CR %d\n",m6840_cr_select);
			else
				if (errorlog) fprintf(errorlog,"MC6840: External clock selected on CR %d\n",m6840_cr_select);
			}

			if (byt&0x4) {
				if (errorlog) fprintf(errorlog,"MC6840: 16 bit count mode selected on CR %d\n",m6840_cr_select);
			else
				if (errorlog) fprintf(errorlog,"MC6840: Dual 8 bit count mode selected on CR %d\n",m6840_cr_select);
			}

			if (errorlog) fprintf(errorlog," Write %02x to control register 1/3\n",(data&0xff00)>>8);
			break;
		case 2:
			if (byt&0x1) {
				m6840_cr_select=1;
//				if (errorlog) fprintf(errorlog,"MC6840: Control register 1 selected\n");
			}
			else {
				m6840_cr_select=3;
//				if (errorlog) fprintf(errorlog,"MC6840: Control register 3 selected\n");
			}

			if (byt&0x80)
				if (errorlog) fprintf(errorlog,"MC6840: Cr2 Timer output enabled\n");

			if (byt&0x40)
				if (errorlog) fprintf(errorlog,"MC6840: Cr2 interrupt output enabled\n");


//			if (byt==0xe2) cpu_cause_interrupt(0,1);

			if (errorlog) fprintf(errorlog," Write %02x to control register 2\n",(data&0xff00)>>8);
			break;
		case 4:
			m6840_timerMSB[0]=byt;
			if (errorlog) fprintf(errorlog," Write %02x to MSB of Timer 1\n",(data&0xff00)>>8);
			break;
		case 6:
			m6840_timerLSB[0]=byt;
			if (errorlog) fprintf(errorlog," Write %02x to LSB of Timer 1\n",(data&0xff00)>>8);
			break;
		case 8:
			m6840_timerMSB[1]=byt;
			if (errorlog) fprintf(errorlog," Write %02x to MSB of Timer 2\n",(data&0xff00)>>8);
			break;
		case 10:
			m6840_timerLSB[1]=byt;
			if (errorlog) fprintf(errorlog," Write %02x to LSB of Timer 2\n",(data&0xff00)>>8);
			break;
		case 12:
			m6840_timerMSB[2]=byt;
			if (errorlog) fprintf(errorlog," Write %02x to MSB of Timer 3\n",(data&0xff00)>>8);
			break;
		case 14:
			m6840_timerLSB[2]=byt;
			if (errorlog) fprintf(errorlog," Write %02x to LSB of Timer 3\n",(data&0xff00)>>8);
			break;
	}

//	if (errorlog) fprintf(errorlog,"Write %04x to 6840 %02x\n",data,offset);

}

/******************************************************************************/

void xenophobe_delayed_write (int param)
{
	pia_1_portb_w (0, (param >> 1) & 0x0f);
	pia_1_ca1_w (0, ~param & 0x01);
}


void mcr68_sg_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog,"Sound CPU: %04x %04x\n",offset,data);

	timer_set (TIME_NOW, data&0xff, xenophobe_delayed_write);
}


 /*
 *		Sounds Good (Rampage) board
 *
 *		MC68000, 1 PIA, 10-bit DAC
 */
static void sg_porta_w (int offset, int data)
{
	int temp;
	dacval = (dacval & ~0x3fc) | (data << 2);
	temp = dacval/2;
	if (temp > 0xff) temp = 0xff;
	DAC_data_w (0, temp);
}

static void sg_portb_w (int offset, int data)
{
	dacval = (dacval & ~0x003) | (data >> 6);
	/* only update when the MSB's are changed */
}

static void sg_irq (void)
{
	/* generate a sound interrupt */
	cpu_cause_interrupt (1, 3); /* From 4 */
}

