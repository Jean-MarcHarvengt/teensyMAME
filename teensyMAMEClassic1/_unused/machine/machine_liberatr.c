/***************************************************************************

  liberator.c - 'machine.c'

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/
#include <stdio.h>
#include "driver.h"
#include "M6502/m6502.h"
#include "cpuintrf.h"
#include "machine/atari_vg.h"

extern FILE *dbfile ;

static int ctrld = 1 ;

extern int  liberator_bitmap_r(int address);
extern void liberator_bitmap_w(int address, int data);
extern void liberator_colorram_w(int address, int data);
extern void liberator_basram_w(int address, int data);
extern void liberator_planetbit_w(int address, int data) ;
extern void liberator_startlg_w(int address,int data) ;


/********************************************************************************************/
void liberator_init_machine(void)
{
	/* this function intentionally left blank */
	return ;
}

/********************************************************************************************/
void liberator_w(int address, int data)
{
	/* TODO: get rid of this */
	extern unsigned char *RAM;

	RAM[address] = data;

	if( address == 0x0002 || (address == 0x0341) )
		liberator_bitmap_w( address, data ) ;
}

/********************************************************************************************/
void liberator_port_w(int offset, int data)
{
	offset &= 0x0007 ;
	switch( offset )
	{
		case 0 :
			/* START LED 1 */
			osd_led_w(0,(data & 0x10) ? 1 : 0) ;
			break;
		case 1 :
			/* START LED 2 */
			osd_led_w(1,(data & 0x10) ? 1 : 0) ;
			break;
		case 2 :
			/* TBSWP* */
			break;
		case 3 :
			/* SPARE */
			break;
		case 4 :
			/* CTRLD* */
			ctrld = data ;
			break;
		case 5 :
			/* COINCNTRR */
			break;
		case 6 :
			/* COINCNTRL */
			break;
		case 7 :
			/* PLANET */
			liberator_planetbit_w( offset, data ) ;
			break;
	}
}


/********************************************************************************************/
int liberator_r(int address)
{
	if( address == 0x0002 )
	{
		return( liberator_bitmap_r( address ) ) ;
	}
	else
	{
		/* TODO: get rid of this */
		extern unsigned char *RAM;

		return( RAM[address] );
	}
}

/********************************************************************************************/
int liberator_port_r(int offset)
{
	int	res ;
	int xdelta, ydelta;
	/* TODO: get rid of this */
	extern unsigned char *RAM;


	if( offset == 0x00 )
	{
		/*
		HEX        R/W   D7 D6 D5 D4 D3 D2 D2 D0  function
		---------+-----+------------------------+------------------------
	    5000         R                        D   coin AUX   (CTRLD* set low)
	    5000         R                     D      coin LEFT  (CTRLD* set low)
	    5000         R                  D         coin RIGHT (CTRLD* set low)
	    5000         R               D            SLAM       (CTRLD* set low)
	    5000         R            D               SPARE      (CTRLD* set low)
	    5000         R         D                  SPARE      (CTRLD* set low)
	    5000         R      D                     COCKTAIL   (CTRLD* set low)
	    5000         R   D                        SELF-TEST  (CTRLD* set low)

	    5000         R               D  D  D  D   HDIR   (CTRLD* set high)
	    5000         R   D  D  D  D               VDIR   (CTRLD* set high)
		*/

		if(ctrld)
		{
			/* 	mouse support */
			xdelta = readinputport(4) ;
			ydelta = readinputport(5) ;
			res = ( ((ydelta << 4) & 0xF0)  |  (xdelta & 0x0F) ) ;
		}
		else
		{
			res = readinputport(0) ;
		}
	}
	else if( offset == 0x01 )
	{
		/*
		HEX        R/W   D7 D6 D5 D4 D3 D2 D2 D0  function
		---------+-----+------------------------+------------------------
	    5001         R                        D   SHIELD 2
	    5001         R                     D      SHIELD 1
	    5001         R                  D         FIRE 2
	    5001         R               D            FIRE 1
	    5001         R            D               SPARE      (CTRLD* set low)
	    5001         R         D                  START 2
	    5001         R      D                     START 1
	    5001         R   D                        VBLANK
		*/

		res = readinputport(1) ;

	}
	else
	{
		res = RAM[0x5000+offset] ;
	}

	return( res ) ;
}

/********************************************************************************************/
int liberator_interrupt(void)
{
	return interrupt();
}

/* -- eof -- */
