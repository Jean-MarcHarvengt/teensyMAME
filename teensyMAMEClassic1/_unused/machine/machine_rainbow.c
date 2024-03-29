/*************************************************************************

  Rastan machine hardware

*************************************************************************/

#include "driver.h"


/*************************************
 *
 *		Globals we own
 *
 *************************************/


/*************************************
 *
 *		Statics
 *
 *************************************/


/*************************************
 *
 *		Interrupt handler
 *
 *************************************/

int rainbow_interrupt(void)
{
	return 4;  /* Interrupt vector 4 */
}

/*************************************
 *
 * Rainbow C-Chip, Protection ?
 *
 *************************************/

int Rainbow_Level,FrameBank;
int Level_OK=0;

int Rainbow_Data[32][2] = {					/* Level Data, Level Height */
  	{0x030000,0x02dc},
    {0x030360,0x03b4},
    {0x030798,0x0444},
    {0x030ca8,0x02dc},
	{0x0311b8,0x02dc},						/* Default all to something */
	{0x0315f0,0x02dc},						/* that should draw a screen */
	{0x031b00,0x02dc},
	{0x0321c0,0x02dc},
	{0x0325b8,0x02dc},
	{0x0328f0,0x02dc},
	{0x032c00,0x02dc},
	{0x0330c0,0x02dc},
	{0x034000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc},
	{0x030000,0x02dc}
};

/* Lookup Table for C-Chip register results */

/* Rainbow Fadeout = 11-3f, bank 2 */
/* Rainbow Destroy = 51-7f, bank 2 */

unsigned char CChipData[128][8] = {
	{0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* 01 */	/* Timer ? */
    {0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00},				/* Screen Size */
    {0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* 05 */	/* Screen Size */
    {0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00},				/* IO Port */
    {0xff,0x00,0x00,0x00,0x33,0x03,0x00,0x00},	/* 09 */	/* IO Port */
    {0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00},				/* IO Port */
    {0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* 0D */	/* IO Port */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x10,0x00,0x33,0x00,0x00,0x00},  /* 11 */
    {0x00,0x00,0x11,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x12,0x00,0x01,0x00,0x00,0x00},	/* 15 */
    {0x00,0x00,0x13,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x14,0x00,0x01,0x00,0x00,0x00},	/* 19 */
    {0x00,0x00,0x15,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x16,0x00,0x01,0x00,0x00,0x00},	/* 1D */
    {0x00,0x00,0x17,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x18,0x00,0x01,0x00,0x00,0x00},  /* 21 */
    {0x00,0x00,0x19,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x1a,0x00,0x01,0x00,0x00,0x00},	/* 25 */
    {0x00,0x00,0x1b,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x1c,0x00,0x01,0x00,0x00,0x00},	/* 29 */
    {0x00,0x00,0x1d,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x1e,0x00,0x01,0x00,0x00,0x00},	/* 2D */
    {0x00,0x00,0x1f,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00},  /* 31 */
    {0x00,0x00,0x21,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x22,0x00,0x00,0x00,0x00,0x00},	/* 35 */
    {0x00,0x00,0x23,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x24,0x00,0x00,0x00,0x00,0x00},	/* 39 */
    {0x00,0x00,0x25,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x26,0x00,0x00,0x00,0x00,0x00},	/* 3D */
    {0x00,0x00,0x27,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},  /* 41 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* 45 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* 49 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* 4D */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x30,0x00,0x00,0x00,0x00,0x00},  /* 51 */
    {0x00,0x00,0x31,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x32,0x00,0x04,0x00,0x00,0x00},	/* 55 */
    {0x00,0x00,0x33,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x34,0x00,0x00,0x00,0x00,0x00},	/* 59 */
    {0x00,0x00,0x35,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x36,0x00,0x00,0x00,0x00,0x00},	/* 5D */
    {0x00,0x00,0x37,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x38,0x00,0x00,0x00,0x00,0x00},  /* 61 */
    {0x00,0x00,0x39,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x3a,0x00,0x00,0x00,0x00,0x00},	/* 65 */
    {0x00,0x00,0x3b,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x3c,0x00,0x00,0x00,0x00,0x00},	/* 69 */
    {0x00,0x00,0x3d,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x3e,0x00,0x00,0x00,0x00,0x00},	/* 6D */
    {0x00,0x00,0x3f,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00},  /* 71 */
    {0x00,0x00,0x41,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x42,0x00,0x00,0x00,0x00,0x00},	/* 75 */
    {0x00,0x00,0x43,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x44,0x00,0x00,0x00,0x00,0x00},	/* 79 */
    {0x00,0x00,0x45,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x46,0x00,0x00,0x00,0x00,0x00},	/* 7D */
    {0x00,0x00,0x47,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},  /* 81 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* 85 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* 89 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* 8D */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},  /* 91 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* 95 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* 99 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* 9D */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},  /* a1 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* a5 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* a9 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* aD */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},  /* b1 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* b5 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* b9 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* bD */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},  /* c1 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* c5 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* c9 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* cD */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},  /* d1 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* d5 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* d9 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* dD */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},  /* e1 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* e5 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* e9 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* eD */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},  /* f1 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* f5 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* f9 */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},	/* fD */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};

void rainbow_c_chip_w(int offset, int data)
{
  switch(offset+1)
  {
    case 0x283: Rainbow_Level = data & 0xff;
    			Level_OK = 0xff;
       			break;

    case 0xc01: FrameBank = data & 0x07;
    			break;
  }

#ifdef MAME_DEBUG
  if (errorlog) fprintf(errorlog,"PC %04x : C-Chip Write %06x = %08x\n",cpu_getpc(),offset+1,data);
#endif

}

/* This chip is used to control many aspects of the game, and seems  */
/* to be mainly for protection. register uses and return values have */
/* been found out by trial and error. 								 */

/* A bootleg rom set would probably help out no end!                 */

int  rainbow_c_chip_r(int offset)
{
  int ans;

  /* Get Table Entry as Default */

  if (offset < 256) ans=CChipData[offset >> 1][FrameBank];
  else ans=0;

  /* Override specific registers (Bank 0 Only) */


  switch(offset+1)
  {
				/* Input Ports */

  	case 0x007: if (FrameBank==0) ans=input_port_2_r(offset);
                break;

    case 0x009: if (FrameBank==0) ans=input_port_3_r(offset);
                break;

    case 0x00b: if (FrameBank==0) ans=input_port_4_r(offset);
                break;

    case 0x00d: if (FrameBank==0) ans=input_port_5_r(offset);
                break;


    			/* Display Related */

    case 0x001: ans=0xff;					/* Won't draw screen until */
                break;						/*   this is set to 0xff   */
                							/*    Countdown Timer ?    */

    case 0x295: ans=0;						/* G Below */
			    break;

    case 0x297: ans=0;						/* G Right */
			    break;

    case 0x299: ans=0x20;					/* O below */
			    break;

    case 0x29b: ans=0x10;					/* O Right */
			    break;

    case 0x29d: ans=0x40;					/* A Below */
			    break;

    case 0x29f: ans=0x20;					/* A Right */
			    break;

    case 0x2a1: ans=0x60;					/* L Below */
			    break;

    case 0x2a3: ans=0x38;					/* L Right */
			    break;

    case 0x2a5: ans=0x80;					/* I Below */
			    break;

    case 0x2a7: ans=0x58;					/* I Right */
			    break;

    case 0x2a9: ans=0xa0;					/* N Below */
			    break;

    case 0x2ab: ans=0x68;					/* N Right */
			    break;

                /* Level Data - Wanted written to 80283 */

  	case 0x201: ans=Level_OK;									/* Data OK */
                break;

    case 0x003: ans=Rainbow_Data[Rainbow_Level][1] & 0xFF;		/* Height */
    			break;

    case 0x005: ans=(Rainbow_Data[Rainbow_Level][1] >> 8) & 0xFF;
				break;

	case 0x285: ans=(Rainbow_Data[Rainbow_Level][0] >> 24) & 0xFF; /* Address */
                break;

	case 0x287: ans=(Rainbow_Data[Rainbow_Level][0] >> 16) & 0xFF;
                break;

	case 0x289: ans=(Rainbow_Data[Rainbow_Level][0] >> 8) & 0xFF;
                break;

	case 0x28B: ans=Rainbow_Data[Rainbow_Level][0] & 0xFF;
    			Level_OK = 0x0;
                break;


                /* Monsters */

    case 0x291: ans=0;									/* Battle Ship */
    			break;									/* Taking Too Long ? */


                /* If this isn't 1 then the program gives an error */

    case 0x803: ans=0x01;								/* C-Chip Check ? */
                break;
  }

#ifdef MAME_DEBUG
  if ((errorlog) && (ans==0)) fprintf(errorlog,"PC %06x : C-Chip Read %04x Bank %02x\n",cpu_getpc(),offset+1,FrameBank);
#endif

  return ans;
}
