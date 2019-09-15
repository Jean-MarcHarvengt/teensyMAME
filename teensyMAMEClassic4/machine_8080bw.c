/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


static int shift_data1,shift_data2,shift_amount;


int invaders_shift_data_r(int offset)
{
	return ((((shift_data1 << 8) | shift_data2) << shift_amount) >> 8) & 0xff;
}



void invaders_shift_amount_w(int offset,int data)
{
	shift_amount = data;
}



void invaders_shift_data_w(int offset,int data)
{
	shift_data2 = shift_data1;
	shift_data1 = data;
}



int invaders_interrupt(void)
{
	static int count;


	count++;

        if (count & 1)
                return 0x00cf;  /* RST 08h */
	else
                return 0x00d7;  /* RST 10h */
}

int seawolf_shift_data_r(int offset)
{
	int reverse_data;

	reverse_data  = ((((shift_data1 << 8) | shift_data2) << shift_amount) >> 8) & 0xff;
	reverse_data  = ((reverse_data & 0x01) << 7)
	              | ((reverse_data & 0x02) << 5)
	              | ((reverse_data & 0x04) << 3)
	              | ((reverse_data & 0x08) << 1)
	              | ((reverse_data & 0x10) >> 1)
	              | ((reverse_data & 0x20) >> 3)
	              | ((reverse_data & 0x40) >> 5)
	              | ((reverse_data & 0x80) >> 7);
	return reverse_data;
}


void zzzap_snd2_w(int offset, int data)
{
}

void zzzap_snd5_w(int offset, int data)
{
}

void zzzap_wdog(void)
{
}

/****************************************************************************
	Extra / Different functions for Boot Hill                (MJC 300198)
****************************************************************************/

int boothill_shift_data_r(int offset)
{
	if (shift_amount < 0x10)
		return invaders_shift_data_r(0);
    else
    {
    	int reverse_data1,reverse_data2;

    	/* Reverse the bytes */

        reverse_data1 = ((shift_data1 & 0x01) << 7)
                      | ((shift_data1 & 0x02) << 5)
                      | ((shift_data1 & 0x04) << 3)
                      | ((shift_data1 & 0x08) << 1)
                      | ((shift_data1 & 0x10) >> 1)
                      | ((shift_data1 & 0x20) >> 3)
                      | ((shift_data1 & 0x40) >> 5)
                      | ((shift_data1 & 0x80) >> 7);

        reverse_data2 = ((shift_data2 & 0x01) << 7)
                      | ((shift_data2 & 0x02) << 5)
                      | ((shift_data2 & 0x04) << 3)
                      | ((shift_data2 & 0x08) << 1)
                      | ((shift_data2 & 0x10) >> 1)
                      | ((shift_data2 & 0x20) >> 3)
                      | ((shift_data2 & 0x40) >> 5)
                      | ((shift_data2 & 0x80) >> 7);

		return ((((reverse_data2 << 8) | reverse_data1) << (0xff-shift_amount)) >> 8) & 0xff;
    }
}

/* Grays Binary again! */

static const int BootHillTable[8] = {
	0x00, 0x40, 0x60, 0x70, 0x30, 0x10, 0x50, 0x50
};

int boothill_port_0_r(int offset)
{
    return (input_port_0_r(0) & 0x8F) | BootHillTable[input_port_3_r(0) >> 5];
}

int boothill_port_1_r(int offset)
{
    return (input_port_1_r(0) & 0x8F) | BootHillTable[input_port_4_r(0) >> 5];
}

/*
 * Space Encounters uses rotary controllers on input ports 0 & 1
 * each controller responds 0-63 for reading, with bit 7 as
 * fire button.
 *
 * The controllers look like they returns Grays binary,
 * so I use a table to translate my simple counter into it!
 */

static const int ControllerTable[64] = {
    0  , 1  , 3  , 2  , 6  , 7  , 5  , 4  ,
    12 , 13 , 15 , 14 , 10 , 11 , 9  , 8  ,
    24 , 25 , 27 , 26 , 30 , 31 , 29 , 28 ,
    20 , 21 , 23 , 22 , 18 , 19 , 17 , 16 ,
    48 , 49 , 51 , 50 , 54 , 55 , 53 , 52 ,
    60 , 61 , 63 , 62 , 58 , 59 , 57 , 56 ,
    40 , 41 , 43 , 42 , 46 , 47 , 45 , 44 ,
    36 , 37 , 39 , 38 , 34 , 35 , 33 , 32
};

/* TODO: I think the bits may need inverting for Sea Wolf and Space Encounters, a la Boot Hill */
int gray6bit_controller0_r(int offset)
{
    return (input_port_0_r(0) & 0xc0) + ControllerTable[input_port_0_r(0) & 0x3f];
}

int gray6bit_controller1_r(int offset)
{
    return (input_port_1_r(0) & 0xc0) + ControllerTable[input_port_1_r(0) & 0x3f];
}

int seawolf_port_0_r (int offset)
{
	return (input_port_0_r(0) & 0xe0) + ControllerTable[input_port_0_r(0) & 0x1f];
}


int midbowl_shift_data_r(int offset)
{
return ((~(((shift_data1 << 8) | shift_data2) << shift_amount) >> 8)) & 0xff;
}

int midbowl_shift_data_rev_r(int offset)
{
int reverse_data, return_data;

reverse_data  = ((~(((shift_data1 << 8) | shift_data2) << shift_amount) >> 8)) & 0xff;
return_data  =  ((reverse_data & 0x01) << 7)
              | ((reverse_data & 0x02) << 5)
              | ((reverse_data & 0x04) << 3)
              | ((reverse_data & 0x08) << 1)
              | ((reverse_data & 0x10) >> 1)
              | ((reverse_data & 0x20) >> 3)
              | ((reverse_data & 0x40) >> 5)
              | ((reverse_data & 0x80) >> 7);
return return_data;
}

/*
 * note: shift_amount is always 0
 */

int blueshrk_shift_data_r(int offset)
{
return (((((shift_data1 << 8) | shift_data2) << (0)) >> 8)) & 0xff;
}

int blueshrk_shift_data_rev_r(int offset)
{
int reverse_data, return_data;

reverse_data  = (((((shift_data1 << 8) | shift_data2) << (0)) >> 8)) & 0xff;
return_data  =  ((reverse_data & 0x01) << 7)
              | ((reverse_data & 0x02) << 5)
              | ((reverse_data & 0x04) << 3)
              | ((reverse_data & 0x08) << 1)
              | ((reverse_data & 0x10) >> 1)
              | ((reverse_data & 0x20) >> 3)
              | ((reverse_data & 0x40) >> 5)
              | ((reverse_data & 0x80) >> 7);
return return_data;
}
