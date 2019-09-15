/**************************************************************************

	Interrupt System Hardware for Bally/Midway games

 	Mike@Dissfulfils.co.uk

**************************************************************************/

#include "driver.h"
#include "Z80/Z80.h"



extern void CopyLine(int Line);
extern void Gorf_CopyLine(int Line);

/****************************************************************************
 * Scanline Interrupt System
 ****************************************************************************/

int NextScanInt=0;			/* Normal */
int CurrentScan=0;
int InterruptFlag=0;

int Controller1=32;			/* Seawolf II */
int Controller2=32;

int GorfDelay;				/* Gorf */
int Countdown=0;

void wow_interrupt_enable_w(int offset, int data)
{
    InterruptFlag = data;

    if (data & 0x01)					/* Disable Interrupts? */
  	    interrupt_enable_w(0,0);
    else
  		interrupt_enable_w(0,1);

    /* Gorf Special interrupt */

    if (data & 0x10)
 	{
  		GorfDelay =(CurrentScan + 10) & 0xFF;

        /* Gorf Special *MUST* occur before next scanline interrupt */

        if ((NextScanInt > CurrentScan) && (NextScanInt < GorfDelay))
        {
          	GorfDelay = NextScanInt - 1;
        }

#ifdef MAME_DEBUG
        if (errorlog) fprintf(errorlog,"Gorf Delay set to %02x\n",GorfDelay);
#endif

    }

#ifdef MAME_DEBUG
    if (errorlog) fprintf(errorlog,"Interrupt Flag set to %02x\n",InterruptFlag);
#endif
}

void wow_interrupt_w(int offset, int data)
{
	/* A write to 0F triggers an interrupt at that scanline */

#ifdef MAME_DEBUG
	if (errorlog) fprintf(errorlog,"Scanline interrupt set to %02x\n",data);
#endif

    NextScanInt = data;
}

int wow_interrupt(void)
{
	int res=ignore_interrupt();
    int Direction;

    CurrentScan++;

    if (CurrentScan == Machine->drv->cpu[0].vblank_interrupts_per_frame)
	{
		CurrentScan = 0;

    	/*
		 * Seawolf2 needs to emulate rotary ports
         *
         * Checked each flyback, takes 1 second to traverse screen
         */

        Direction = input_port_0_r(0);

        if ((Direction & 2) && (Controller1 > 0))
			Controller1--;

		if ((Direction & 1) && (Controller1 < 63))
			Controller1++;

        Direction = input_port_1_r(0);

        if ((Direction & 2) && (Controller2 > 0))
			Controller2--;

		if ((Direction & 1) && (Controller2 < 63))
			Controller2++;
    }

    if (CurrentScan < 204) CopyLine(CurrentScan);

    /* Scanline interrupt enabled ? */

    if ((InterruptFlag & 0x08) && (CurrentScan == NextScanInt))
		res = interrupt();

    return res;
}

/****************************************************************************
 * Gorf - Interrupt routine and Timer hack
 ****************************************************************************/

int gorf_interrupt(void)
{
	int res=ignore_interrupt();

    CurrentScan++;

    if (CurrentScan == 256)
	{
		CurrentScan=0;
    }

    if (CurrentScan < 204) Gorf_CopyLine(CurrentScan);

    /* Scanline interrupt enabled ? */

    if ((InterruptFlag & 0x08) && (CurrentScan == NextScanInt))
		res = interrupt();

    /* Gorf Special Bits */

    if (Countdown>0) Countdown--;

    if ((InterruptFlag & 0x10) && (CurrentScan==GorfDelay))
		res = interrupt() & 0xF0;

/*	cpu_clear_pending_interrupts(0); */

	Z80_Clear_Pending_Interrupts();					/* Temporary Fix */

    return res;
}

int gorf_timer_r(int offset)
{
	static int Skip=0;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((RAM[0x5A93]==160) || (RAM[0x5A93]==4)) 	/* INVADERS AND    */
	{												/* GALAXIAN SCREEN */
        if (cpu_getpc()==0x3086)
        {
    	    if(--Skip==-1)
            {
                Skip=2;
            }
        }

	   	return Skip;
    }
    else
    {
    	return RAM[0xD0A5];
    }

}

/****************************************************************************
 * Seawolf Controllers
 ****************************************************************************/

/*
 * Seawolf2 uses rotary controllers on input ports 10 + 11
 * each controller responds 0-63 for reading, with bit 7 as
 * fire button. Controller values are calculated in the
 * interrupt routine, and just formatted & returned here.
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

int seawolf2_controller1_r(int offset)
{
    return (input_port_0_r(0) & 0xC0) + ControllerTable[Controller1];
}

int seawolf2_controller2_r(int offset)
{
    return (input_port_1_r(0) & 0x80) + ControllerTable[Controller2];
}

