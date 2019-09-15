#include "driver.h"
#include "I8039.h"


/*#define USE_SAMPLES*/


/* The timer clock which feeds the lower 4 bits of    */
/* AY-3-8910 port A is based on the same clock        */
/* feeding the sound CPU Z80.  It is a divide by      */
/* 10240, formed by a standard divide by 1024,        */
/* followed by a divide by 10 using a 4 bit           */
/* bi-quinary count sequence. (See LS90 data sheet    */
/* for an example).                                   */
/* Bits 1-3 come directly from the upper three bits   */
/* of the bi-quinary counter. Bit 0 comes from the    */
/* output of the divide by 1024.                      */

static int gyruss_timer[20] = {
0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x02, 0x03, 0x04, 0x05,
0x08, 0x09, 0x08, 0x09, 0x0a, 0x0b, 0x0a, 0x0b, 0x0c, 0x0d
};

int gyruss_portA_r(int offset)
{
	/* need to protect from totalcycles overflow */
	static int last_totalcycles = 0;

	/* number of Z80 clock cycles to count */
	static int clock;

	int current_totalcycles;

	current_totalcycles = cpu_gettotalcycles();
	clock = (clock + (current_totalcycles-last_totalcycles)) % 10240;

	last_totalcycles = current_totalcycles;

	return gyruss_timer[clock/512];
}



int gyruss_sh_start(void)
{
	int i;


	for (i = 0;i < 3;i++) stream_set_pan(3*0+i,OSD_PAN_RIGHT);	/* AY8910 #0 */
	for (i = 0;i < 3;i++) stream_set_pan(3*1+i,OSD_PAN_LEFT);	/* AY8910 #1 */
	for (i = 0;i < 3;i++) stream_set_pan(3*2+i,OSD_PAN_RIGHT);	/* AY8910 #2 */
	for (i = 0;i < 3;i++) stream_set_pan(3*3+i,OSD_PAN_RIGHT);	/* AY8910 #3 */
	for (i = 0;i < 3;i++) stream_set_pan(3*4+i,OSD_PAN_LEFT);	/* AY8910 #4 */
	stream_set_pan(15,OSD_PAN_LEFT);							/* DAC */

	return 0;
}

static void filter_w(int chip,int data)
{
	int i;


	for (i = 0;i < 3;i++)
	{
		int C;


		C = 0;
		if (data & 1) C += 47000;	/* 47000pF = 0.047uF */
		if (data & 2) C += 220000;	/* 220000pF = 0.22uF */
		data >>= 2;
		set_RC_filter(3*chip + i,1000,2200,200,C);
	}
}

void gyruss_filter0_w(int offset,int data)
{
	filter_w(0,data);
}

void gyruss_filter1_w(int offset,int data)
{
	filter_w(1,data);
}



void gyruss_sh_irqtrigger_w(int offset,int data)
{
	/* writing to this register triggers IRQ on the sound CPU */
	cpu_cause_interrupt(1,0xff);
}

void gyruss_i8039_irq_w(int offset,int data)
{
	cpu_cause_interrupt(2,I8039_EXT_INT);
}
