/*****************************************************************************/
/*                                                                           */
/* Module:	POKEY Chip Emulator, V3.1										 */
/* Purpose: To emulate the sound generation hardware of the Atari POKEY chip.*/
/* Author:  Ron Fries                                                        */
/*                                                                           */
/* Revision History:                                                         */
/*                                                                           */
/* 09/22/96 - Ron Fries - Initial Release                                    */
/* 01/14/97 - Ron Fries - Corrected a minor problem to improve sound quality */
/*                        Also changed names from POKEY11.x to POKEY.x       */
/* 01/17/97 - Ron Fries - Added support for multiple POKEY chips.            */
/* 03/31/97 - Ron Fries - Made some minor mods for MAME (changed to signed   */
/*                        8-bit sample, increased gain range, removed        */
/*                        _disable() and _enable().)                         */
/* 04/06/97 - Brad Oliver - Some cross-platform modifications. Added         */
/*                          big/little endian #defines, removed <dos.h>,     */
/*                          conditional defines for TRUE/FALSE               */
/* 08/08/97 - Brad Oliver - Added code to read the random number register.   */
/*            & Eric Smith  This code relies on writes to SKCNTL as well.    */
/* 06/27/98 - pullmoll	  - implemented missing POKEY registers.			 */
/*						  Since this is a major change I incremeneted to	 */
/*						  V3.0; detailed list of changes below. 			 */
/* 11/19/98 - pullmoll	  - support for MAMEs streams interface.			 */
/*                                                                           */
/* V3.1 Detailed Changes													 */
/* ---------------------                                                     */
/*																			 */
/* Changed Pokey_process to calculate one chip at a time. It is now called   */
/* by streams.c after it's given as parameter callback to stream_init().     */
/* Removed COMP16 support since the new polynome, random code would not      */
/* work with 16 bit machines (128K RNG buffer). 							 */
/* Other minor cleanups in code: made all arguments int (should be more 	 */
/* efficient on 32bit machines). Remove already commented DOS dependencies	 */
/* _enable() and _disable() (interrupts).									 */
/*                                                                           */
/* V3.0 Detailed Changes                                                     */
/* ---------------------                                                     */
/*                                                                           */
/* Now emulates all aspects of a real POKEY chip, including the KBCODE, 	 */
/* SERIN, SEROUT registers and TIMERS 1,2 and 4. The code of pokyintf.c and  */
/* pokey.c is now combined to avoid a large number of otherwise unneeded	 */
/* externs and calls between the two pieces of code.						 */
/*                                                                           */
/* The intf structure now holds additional function pointers for reading	 */
/* serial input, writing serial output and for a interrupt call back.		 */
/* These function pointers are optional; current implementations using the	 */
/* POKEY can set them to zero. Because of the timers the code now depends	 */
/* on the presence of MAME's timer.c (or equivalent functions).              */
/*																			 */
/* POKEY supported 3 timers, called TIMER1, TIMER2 and TIMER4. They were	 */
/* fired by the corresponding AUDF1, AUDF2 and AUDF4 divide by N counters.	 */
/* Once a counter reaches zero, one or more of the bits 0, 1 and 2 in the	 */
/* IRQST register that are enabled in IRQEN are set and an IRQ is issued.	 */
/* To avoid time consuming checks inside the sound generation code I used	 */
/* the MAME/MESS specific timer code instead. Therefore the changes should	 */
/* not lead to a speed decrease if the timers are not used at all.			 */
/*																			 */
/* The code uses timer_pulse(duration, mask, callback) to set a function	 */
/* that is repeatedly called after Div_n_max[chan]/intf->baseclock time is	 */
/* gone. This function checks for enabled timer IRQs in IRQEN and sets the	 */
/* corresponding bits in the IRQST register; it then calls an application	 */
/* supllied callback to handle the IRQ which could call cpu_cause_interrupt  */
/* or take the appropriate action.											 */
/*																			 */
/* The timers are disabled by a call to timer_enable(handle, 0), if a write  */
/* to IRQEN disables the corresponding bits; if the bits are set, timers are */
/* started again with timer_enable(handle, !0). This avoids breaking the CPU */
/* emulation to make unneeded calls to the timer handler, which would do	 */
/* nothing at all (there is no timer status other than IRQST).				 */
/*                                                                           */
/* The sound generation code also emulate the filters now.					 */
/* The effect is pretty much identical to what I hear on my Atari 800XL :)	 */
/*                                                                           */
/* V2.0 Detailed Changes                                                     */
/* ---------------------                                                     */
/*                                                                           */
/* Now maintains both a POLY9 and POLY17 counter.  Though this slows the     */
/* emulator in general, it was required to support mutiple POKEYs since      */
/* each chip can individually select POLY9 or POLY17 operation.  Also,       */
/* eliminated the Poly17_size variable.                                      */
/*                                                                           */
/* Changed address of POKEY chip.  In the original, the chip was fixed at    */
/* location D200 for compatibility with the Atari 800 line of 8-bit          */
/* computers. The update function now only examines the lower four bits, so  */
/* the location for all emulated chips is effectively xxx0 - xxx8.           */
/*                                                                           */
/* The Update_pokey_sound function has two additional parameters which       */
/* selects the desired chip and selects the desired gain.                    */
/*                                                                           */
/* Added clipping to reduce distortion, configurable at compile-time.        */
/*                                                                           */
/* The Pokey_sound_init function has an additional parameter which selects   */
/* the number of pokey chips to emulate.                                     */
/*                                                                           */
/* The output will be amplified by gain/16.  If the output exceeds the       */
/* maximum value after the gain, it will be limited to reduce distortion.    */
/* The best value for the gain depends on the number of POKEYs emulated      */
/* and the maximum volume used.  The maximum possible output for each        */
/* channel is 15, making the maximum possible output for a single chip to    */
/* be 60.  Assuming all four channels on the chip are used at full volume,   */
/* a gain of 64 can be used without distortion.  If 4 POKEY chips are        */
/* emulated and all 16 channels are used at full volume, the gain must be    */
/* no more than 16 to prevent distortion.  Of course, if only a few of the   */
/* 16 channels are used or not all channels are used at full volume, a       */
/* larger gain can be used.                                                  */
/*                                                                           */
/* The Pokey_process routine automatically processes all channels.			 */
/* No additional calls or functions are required.							 */
/*                                                                           */
/* The unoptimized Pokey_process2() function has been removed.               */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*                 License Information and Copyright Notice                  */
/*                 ========================================                  */
/*                                                                           */
/* PokeySound is Copyright(c) 1996-1997 by Ron Fries                         */
/*                                                                           */
/* This library is free software; you can redistribute it and/or modify it   */
/* under the terms of version 2 of the GNU Library General Public License    */
/* as published by the Free Software Foundation.                             */
/*                                                                           */
/* This library is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library */
/* General Public License for more details.                                  */
/* To obtain a copy of the GNU Library General Public License, write to the  */
/* Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.   */
/*                                                                           */
/* Any permitted reproduction of these routines, in whole or in part, must   */
/* bear this legend.                                                         */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "driver.h"

#ifndef REGISTER
#define REGISTER register
#endif

#define VERBOSE         0
#define VERBOSE_SOUND	0
#define VERBOSE_TIMER	0
#define VERBOSE_POLY	0
#define VERBOSE_RANDOM	0

/* CONSTANT DEFINITIONS */

/* definitions for AUDCx (D201, D203, D205, D207) */
#define NOTPOLY5    0x80     /* selects POLY5 or direct CLOCK */
#define POLY4       0x40     /* selects POLY4 or POLY17 */
#define PURE        0x20     /* selects POLY4/17 or PURE tone */
#define VOL_ONLY    0x10     /* selects VOLUME OUTPUT ONLY */
#define VOLUME_MASK 0x0f     /* volume mask */

/* definitions for AUDCTL (D208) */
#define POLY9       0x80     /* selects POLY9 or POLY17 */
#define CH1_179     0x40     /* selects 1.78979 MHz for Ch 1 */
#define CH3_179     0x20     /* selects 1.78979 MHz for Ch 3 */
#define CH1_CH2     0x10     /* clocks channel 1 w/channel 2 */
#define CH3_CH4     0x08     /* clocks channel 3 w/channel 4 */
#define CH1_FILTER  0x04     /* selects channel 1 high pass filter */
#define CH2_FILTER  0x02     /* selects channel 2 high pass filter */
#define CLOCK_15    0x01     /* selects 15.6999kHz or 63.9210kHz */

/* for accuracy, the 64kHz and 15kHz clocks are exact divisions of
   the 1.79MHz clock */
#define DIV_64      28       /* divisor for 1.79MHz clock to 64 kHz */
#define DIV_15      114      /* divisor for 1.79MHz clock to 15 kHz */

/* SIZE     the size (in entries) of the polynomial tables */
/* SHL/SHR	the rotate factors for the poly counters */
/* ADD		constant value to be added to the poly counters */
#define POLY4_BITS  4
#define POLY5_BITS  5
#define POLY9_BITS  9
#define POLY17_BITS 17
#define POLY4_SIZE	((1<<POLY4_BITS)-1)
#define POLY5_SIZE	((1<<POLY5_BITS)-1)
#define POLY9_SIZE	((1<<POLY9_BITS)-1)
#define POLY17_SIZE ((1<<POLY17_BITS)-1)

#define POLY4_SHL   3
#define POLY4_SHR	1
#define POLY4_ADD	0x04

#define POLY5_SHL	3
#define POLY5_SHR	2
#define POLY5_ADD	0x08

#define POLY9_SHL	2
#define POLY9_SHR	7
#define POLY9_ADD	0x80

#define POLY17_SHL	7
#define POLY17_SHR	10
#define POLY17_ADD	0x18000

/* channel/chip definitions */
#define CHAN1       0
#define CHAN2       1
#define CHAN3       2
#define CHAN4       3
#define CHIP1       0
#define CHIP2       4
#define CHIP3       8
#define CHIP4      12
#define SAMPLE    127

/* interrupt enable/status definitions (D20E) */
#define IRQ_BREAK   0x80
#define IRQ_KEYBD   0x40
#define IRQ_SERIN   0x20
#define IRQ_SEROR   0x10
#define IRQ_SEROC	0x08
#define IRQ_TIMR4	0x04
#define IRQ_TIMR2	0x02
#define IRQ_TIMR1	0x01

/* SK status definitions (R/D20F) */
#define SK_FRAME    0x80
#define SK_OVERRUN	0x40
#define SK_KBERR	0x20
#define SK_SERIN	0x10
#define SK_SHIFT	0x08
#define SK_KEYBD	0x04
#define SK_SEROUT	0x02

/* SK control definitions (W/D20F) */
#define SK_BREAK    0x80
#define SK_BPS      0x70
#define SK_FM       0x08
#define SK_PADDLE   0x04
#define SK_RESET    0x03

/* timer definitions */
#define TIMER1      0
#define TIMER2		1
#define TIMER4		2

#define TIMER_MASK  (IRQ_TIMR4|IRQ_TIMR2|IRQ_TIMR1)
#define CHIP_SHIFT  3

#define MIN_TIMER	4


/* STATIC VARIABLE DEFINITIONS */

static int channel[MAXPOKEYS];

static struct POKEYinterface *intf;

/* number of pokey chips currently emulated */
static int Num_pokeys;

/* structures to hold the 9 pokey control bytes */
static UINT8 AUDF[4 * MAXPOKEYS];	/* AUDFx (D200, D202, D204, D206) */
static UINT8 AUDC[4 * MAXPOKEYS];	/* AUDCx (D201, D203, D205, D207) */
static UINT8 AUDCTL[MAXPOKEYS]; 	/* AUDCTL (D208) */

static void *TIMER[MAXPOKEYS][3];		   /* timers returned by timer.c */
static UINT8 KBCODE[MAXPOKEYS]; 		   /* KBCODE (R/D209) */
static UINT8 SERIN[MAXPOKEYS];			   /* SERIN (R/D20D) */
static UINT8 SEROUT[MAXPOKEYS]; 		   /* SEROUT (W/D20D) */
static UINT8 IRQST[MAXPOKEYS];			   /* IRQST (R/D20E) */
static UINT8 IRQEN[MAXPOKEYS];			   /* IRQEN (W/D20E) */
static UINT8 SKSTAT[MAXPOKEYS]; 		   /* SKSTAT (R/D20F) */
static UINT8 SKCTL[MAXPOKEYS];			   /* SKCTL (W/D20F) */

#ifdef CLIP
static UINT16 AUDV[4 * MAXPOKEYS];	/* Channel volume - derived */
#else
static UINT8 AUDV[4 * MAXPOKEYS];	/* Channel volume - derived */
#endif

static UINT8 Outbit[4 * MAXPOKEYS]; /* last output volume for each channel */


static UINT8 RANDOM[MAXPOKEYS]; 	/* The random number for each pokey */

/* Initialze the bit patterns for the polynomials. */

/* The 4bit and 5bit patterns are the identical ones used in the pokey chip. */
/* Though the patterns could be packed with 8 bits per byte, using only a */
/* single bit per byte keeps the math simple, which is important for */
/* efficient processing. */

static UINT8 poly4[POLY4_SIZE];
static UINT8 poly5[POLY5_SIZE];
static UINT8 poly9[POLY9_SIZE];
/* ASG 980126 - changed this to a dynamically allocated array */
static UINT8 *poly17;
static UINT8 *rand17;

static UINT32 Poly_adjust; /* the amount that the polynomial will need */
                           /* to be adjusted to process the next bit */

static UINT32 P4=0,   /* Global position pointer for the 4-bit	POLY array */
              P5=0,   /* Global position pointer for the 5-bit  POLY array */
              P9=0,   /* Global position pointer for the 9-bit  POLY array */
              P17=0;  /* Global position pointer for the 17-bit POLY array */

static UINT32 Div_n_cnt[4 * MAXPOKEYS],   /* Divide by n counter. one for each channel */
			  Div_n_max[4 * MAXPOKEYS],   /* Divide by n maximum, one for each channel */
			  Div_n_tmr[4 * MAXPOKEYS];   /* Divide by n real value (for timers) */

static UINT32 Samp_n_max,	  /* Sample max.  For accuracy, it is *256 */
              Samp_n_cnt[2];  /* Sample cnt. */

static UINT32 Base_mult[MAXPOKEYS]; /* selects either 64kHz or 15kHz clock mult */

static UINT8  clip; /* LBO 101297 */

/*****************************************************************************/
/* In my routines, I treat the sample output as another divide by N counter  */
/* For better accuracy, the Samp_n_cnt has a fixed binary decimal point      */
/* which has 8 binary digits to the right of the decimal point.  I use a two */
/* byte array to give me a minimum of 40 bits, and then use pointer math to  */
/* reference either the 24.8 whole/fraction combination or the 32-bit whole  */
/* only number.  This is mainly used to keep the math simple for             */
/* optimization. See below:                                                  */
/*                                                                           */
/* Representation on little-endian machines:                                 */
/* xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx | xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx */
/* fraction   whole    whole    whole      whole   unused   unused   unused  */
/*                                                                           */
/* Samp_n_cnt[0] gives me a 32-bit int 24 whole bits with 8 fractional bits, */
/* while (UINT32 *)((UINT8 *)(&Samp_n_cnt[0])+1) gives me the 32-bit whole	 */
/* number only.                                                              */
/*                                                                           */
/* Representation on big-endian machines:                                    */
/* xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx | xxxxxxxx xxxxxxxx xxxxxxxx.xxxxxxxx */
/*  unused   unused   unused    whole      whole    whole    whole  fraction */
/*                                                                           */
/* Samp_n_cnt[1] gives me a 32-bit int 24 whole bits with 8 fractional bits, */
/* while (UINT32 *)((UINT8 *)(&Samp_n_cnt[0])+3) gives me the 32-bit whole	 */
/* number only.                                                              */
/*																			 */
/* JAMC note:																 */
/* this strategy only works if ARCH allows use of unaligned integer pointers */
/* So , i've changed it to:                                                  */
/* Samp_n_cnt[0] stores whole part											 */
/* Samp_n_cnt[1] stores fractional part 									 */
/*****************************************************************************/


/*****************************************************************************/
/* Module:	Poly_init() 													 */
/* Purpose: Hopefully exact emulation of the poly counters					 */
/*			Based on a description from Perry McFarlane posted on			 */
/*			comp.sys.atari.8bit 1996/12/06: 								 */
/*	I have been working on writing a simple program to play pokey chip		 */
/*	sounds on the pc.  While detailed technical information is available	 */
/*	in the 400/800 hardware manual including a schematic diagram of the 	 */
/*	operation of the pokey chip it lacks a description of the precise		 */
/*	operation of the polynomial counters which generate the pseudorandom	 */
/*	bit patterns used to make the distortion.  I have experimented and i	 */
/*	believe i have the exact formula which is used.  let x=0 then			 */
/*	x=((x<<a)+(x>>b)+c)%m gives the next value of x, and x%2 is the bit 	 */
/*	used. if n is the # of bits in the poly counter, then a+b=n and m=2^n.	 */
/*	the sequence of bits generated has a period of 2^n-1.  acutally what	 */
/*	this is is just a circular shift, plus an addition.  I empirically		 */
/*	determined the values of a,b, and c which generate bit patterns 		 */
/*	matching those used by the pokey.										 */
/*	4 bits a=3 b=1 c=4														 */
/*	5 bits a=3 b=2 c=8														 */
/*	9 bits a=2 b=7 c=128													 */
/*	17bits a=7 b=10 c=98304 												 */
/*	i suspect a similar formula is used for the atari vcs chips , perhaps	 */
/*	for other sound generation hardware 									 */
/*                                                                           */
/* Author:	Juergen Buchmueller 											 */
/* Date:	June 29, 1998													 */
/*                                                                           */
/* Inputs:	p - pointer to the polynome buffer (one byte per bit)			 */
/*			size - length of polynome (2^bits-1)							 */
/*			a - left shift factor											 */
/*			b - right shift factor											 */
/*			c - value for addition											 */
/*                                                                           */
/* Outputs: fill the buffer at p with poly bit 0							 */
/*                                                                           */
/*****************************************************************************/
static void Poly_init(UINT8 *p, int size, int a, int b, int c)
{
UINT32 i, x = 0;
#if VERBOSE_POLY
	if (errorlog) fprintf(errorlog, "Poly %05x %d %d %05x\n", size, a, b, c);
#endif
    for (i = 0; i < size; i++)
	{
	UINT8 bit = x & 1;
        /* store new value */
		*p++ = bit;
#if VERBOSE_POLY
        if (errorlog) fprintf(errorlog, "%d%s", bit, ((i&63)==63)?"\n":"");
#endif
        /* calculate next bit */
		x = ((x << a) + (x >> b) + c) & size;
	}
#if VERBOSE_POLY
    if (errorlog) fprintf(errorlog, "\n");
#endif
}

static void Rand_init(UINT8 *p, int size, int a, int b, int c)
{
UINT32 i, x = 0;
    for (i = 0; i < size; i++)
	{
	UINT8 rnd = x >> 3;
#if VERBOSE_RANDOM
        if (errorlog) fprintf(errorlog, "%02x%s", rnd, ((i&31)==31)?"\n":" ");
#endif
        /* store the 8 bits of the new value */
		*p++ = rnd;
		/* calculate next bit */
		x = ((x << a) + (x >> b) + c) & size;
	}
}

/*****************************************************************************/
/* Module:  Pokey_sound_init()                                               */
/* Purpose: to handle the power-up initialization functions                  */
/*          these functions should only be executed on a cold-restart        */
/*                                                                           */
/* Author:  Ron Fries                                                        */
/* Date:    January 1, 1997                                                  */
/*                                                                           */
/* Inputs:  freq17 - the value for the '1.79MHz' Pokey audio clock           */
/*          playback_freq - the playback frequency in samples per second     */
/*          num_pokeys - specifies the number of pokey chips to be emulated  */
/*                                                                           */
/* Outputs: Adjusts local globals - no return value                          */
/*                                                                           */
/*****************************************************************************/

/* ASG 980126 - added a return parameter to indicate failure */
int Pokey_sound_init (int freq17, int playback_freq, int volume, int num_pokeys, int use_clip)
{
	int chip, chan;

	/* ASG 980126 - dynamically allocate this array */
	poly17 = malloc (POLY17_SIZE+1);
	if (!poly17) return 1;

    /* HJB 980706 - random numbers are the upper 8 bits of poly 17 */
	rand17 = malloc (POLY17_SIZE+1);
	if (!rand17) return 1;

	/* Initialize the poly counters */
	Poly_init(poly4,  POLY4_SIZE,  POLY4_SHL,  POLY4_SHR,  POLY4_ADD);
	Poly_init(poly5,  POLY5_SIZE,  POLY5_SHL,  POLY5_SHR,  POLY5_ADD);
	Poly_init(poly9,  POLY9_SIZE,  POLY9_SHL,  POLY9_SHR,  POLY9_ADD);
	Poly_init(poly17, POLY17_SIZE, POLY17_SHL, POLY17_SHR, POLY17_ADD);
	Rand_init(rand17, POLY17_SIZE, POLY17_SHL, POLY17_SHR, POLY17_ADD);

	/* start all of the polynomial counters at zero */
	Poly_adjust = 0;
	P4 = 0;
	P5 = 0;
	P9 = 0;
	P17 = 0;

	/* calculate the sample 'divide by N' value based on the playback freq. */
	if (playback_freq)
		Samp_n_max = ((UINT32)freq17 << 8) / playback_freq;
    else
		Samp_n_max = 1;

	Samp_n_cnt[0] = 0;	/* initialize all bits of the sample */
	Samp_n_cnt[1] = 0;	/* 'divide by N' counter */

	for (chip = 0; chip < MAXPOKEYS; chip++) {
        AUDCTL[chip] = 0;
        Base_mult[chip] = DIV_64;
        TIMER[chip][TIMER1] = 0;
        TIMER[chip][TIMER2] = 0;
        TIMER[chip][TIMER4] = 0;
        KBCODE[chip] = 0x09;                /* Atari 800 'no key' */
        SEROUT[chip] = 0;
        SERIN[chip] = 0;
        IRQST[chip] = 0;
        IRQEN[chip] = 0;
        SKSTAT[chip] = 0;
        SKCTL[chip] = SK_RESET;             /* let the RNG run after reset */
    }

	for (chan = 0; chan < (MAXPOKEYS * 4); chan++) {
        Outbit[chan] = 0;
		Div_n_cnt[chan] = 0;
		Div_n_max[chan] = 0x7fffffffL;
		Div_n_tmr[chan] = 0;
		AUDC[chan] = 0;
		AUDF[chan] = 0;
		AUDV[chan] = 0;
    }

	for (chip = 0; chip < num_pokeys; chip++) {
		char name[40];

        sprintf(name, "Pokey #%d", chip);
        channel[chip] = stream_init(name, playback_freq, 8, chip, Pokey_process);
		stream_set_volume(channel[chip],volume);

        if (channel[chip] == -1)
            return 1;
    }

    /* set the number of pokey chips currently emulated */
	Num_pokeys = num_pokeys;
	clip = use_clip; /* LBO 101297 */

	/* ASG 980126 - return success */
	return 0;
}

/*****************************************************************************/
/* Module:  Pokey_Timer(int param)                                           */
/* Purpose: Is called when a pokey timer expires.                            */
/*                                                                           */
/* Author:  Juergen Buchmueller                                              */
/* Date:    June, 27 1998                                                    */
/*                                                                           */
/* Inputs:  param - chip * 8 + bitmask for timers (1,2,4)                    */
/*                                                                           */
/* Outputs: modifies the IRQ status and calls a user supplied function       */
/*                                                                           */
/*****************************************************************************/

void Pokey_Timer(int param)
{
int chip;
    /* split param into chip number and timer mask */
    chip = param >> CHIP_SHIFT;
	param &= TIMER_MASK;
#if VERBOSE_TIMER
	if (errorlog) fprintf(errorlog, "POKEY #%d timer%d with IRQEN $%02x\n", chip, param, IRQEN[chip]);
#endif
    /* check if some of the requested timer interrupts are enabled */
	param &= IRQEN[chip];
    if (param)
    {
        /* set the enabled timer irq status bits */
        IRQST[chip] |= param;
        /* call back an application supplied function to handle the interrupt */
        if (intf->interrupt_cb[chip])
            (*intf->interrupt_cb[chip])(param);
    }
}

#if VERBOSE_SOUND
static char *audc2str(int val)
{
	static char buff[80];
	if (val & NOTPOLY5) {
		if (val & PURE) {
			strcpy(buff,"pure");
		} else if (val & POLY4) {
			strcpy(buff,"poly4");
        } else {
			strcpy(buff,"poly9/17");
		}
	} else {
		if (val & PURE) {
			strcpy(buff,"poly5");
		} else if (val & POLY4) {
			strcpy(buff,"poly4+poly5");
        } else {
			strcpy(buff,"poly9/17+poly5");
        }
    }
	return buff;
}

static char *audctl2str(int val)
{
	static char buff[80];
	if (val & POLY9) {
		strcpy(buff,"poly9");
	} else {
		strcpy(buff,"poly17");
    }
	if (val & CH1_179) {
		strcpy(buff,"+ch1hi");
    }
	if (val & CH3_179) {
		strcpy(buff,"+ch3hi");
    }
	if (val & CH1_CH2) {
		strcpy(buff,"+ch1/2");
    }
	if (val & CH3_CH4) {
		strcpy(buff,"+ch3/4");
    }
	if (val & CH1_FILTER) {
		strcpy(buff,"+ch1filter");
    }
	if (val & CH2_FILTER) {
		strcpy(buff,"+ch2filter");
    }
	if (val & CLOCK_15) {
		strcpy(buff,"+clk15");
    }
    return buff;
}
#endif

/*****************************************************************************/
/* Module:	Pokey_SerinReady(int param) 									 */
/* Purpose: Is called when another data byte is ready at the SERIN register. */
/*                                                                           */
/* Author:  Juergen Buchmueller                                              */
/* Date:	June, 28 1998													 */
/*                                                                           */
/* Inputs:	chip - chip number												 */
/*                                                                           */
/* Outputs: modifies the IRQ status and calls a user supplied function       */
/*                                                                           */
/*****************************************************************************/

void Pokey_SerinReady(int chip)
{
	if (IRQEN[chip] & IRQ_SERIN)
	{
		/* set the enabled timer irq status bits */
		IRQST[chip] |= IRQ_SERIN;
		/* call back an application supplied function to handle the interrupt */
		if (intf->interrupt_cb[chip])
			(*intf->interrupt_cb[chip])(IRQ_SERIN);
	}
}


/*****************************************************************************/
/* Module:  Update_pokey_sound()                                             */
/* Purpose: To process the latest control values stored in the AUDF, AUDC,   */
/*          and AUDCTL registers.  It pre-calculates as much information as  */
/*          possible for better performance.  This routine has not been      */
/*          optimized.                                                       */
/*                                                                           */
/* Author:  Ron Fries                                                        */
/* Date:    January 1, 1997                                                  */
/*                                                                           */
/* Inputs:  addr - the address of the parameter to be changed                */
/*          val - the new value to be placed in the specified address        */
/*          gain - specified as an 8-bit fixed point number - use 1 for no   */
/*                 amplification (output is multiplied by gain)              */
/*                                                                           */
/* Outputs: Adjusts local globals - no return value                          */
/*                                                                           */
/*****************************************************************************/

void Update_pokey_sound (int addr, int val, int chip, int gain)
{
	UINT32 new_val = 0;
	UINT8 chan;
	UINT8 chan_mask = 0;
	UINT8 chip_offs;

    /* calculate the chip_offs for the channel arrays */
    chip_offs = chip << 2;

    /* determine which address was changed */
    switch (addr & 0x0f)
    {
		case AUDF1_C:
			if (val == AUDF[CHAN1 + chip_offs])
				return;
#if VERBOSE_SOUND
            if (errorlog) fprintf(errorlog, "POKEY #%d AUDF1  $%02x\n", chip, val);
#endif
            AUDF[CHAN1 + chip_offs] = val;
			chan_mask = 1 << CHAN1;

			if (AUDCTL[chip] & CH1_CH2)    /* if ch 1&2 tied together */
				chan_mask |= 1 << CHAN2;   /* then also change on ch2 */
			break;

		case AUDC1_C:
			if (val == AUDC[CHAN1 + chip_offs])
				return;
#if VERBOSE_SOUND
			if (errorlog) fprintf(errorlog, "POKEY #%d AUDC1  $%02x (%s)\n", chip, val, audc2str(val));
#endif
            AUDC[CHAN1 + chip_offs] = val;
			AUDV[CHAN1 + chip_offs] = (val & VOLUME_MASK) * gain;
			chan_mask = 1 << CHAN1;
			break;

		case AUDF2_C:
			if (val == AUDF[CHAN2 + chip_offs])
				return;
#if VERBOSE_SOUND
            if (errorlog) fprintf(errorlog, "POKEY #%d AUDF2  $%02x\n", chip, val);
#endif
            AUDF[CHAN2 + chip_offs] = val;
			chan_mask = 1 << CHAN2;
			break;

		case AUDC2_C:
			if (val == AUDC[CHAN2 + chip_offs])
				return;
#if VERBOSE_SOUND
			if (errorlog) fprintf(errorlog, "POKEY #%d AUDC2  $%02x (%s)\n", chip, val, audc2str(val));
#endif
            AUDC[CHAN2 + chip_offs] = val;
			AUDV[CHAN2 + chip_offs] = (val & VOLUME_MASK) * gain;
			chan_mask = 1 << CHAN2;
            break;

		case AUDF3_C:
			if (val == AUDF[CHAN3 + chip_offs])
				return;
#if VERBOSE_SOUND
            if (errorlog) fprintf(errorlog, "POKEY #%d AUDF3  $%02x\n", chip, val);
#endif
            AUDF[CHAN3 + chip_offs] = val;
			chan_mask = 1 << CHAN3;

			if (AUDCTL[chip] & CH3_CH4)   /* if ch 3&4 tied together */
				chan_mask |= 1 << CHAN4;  /* then also change on ch4 */
			break;

		case AUDC3_C:
			if (val == AUDC[CHAN3 + chip_offs])
				return;
#if VERBOSE_SOUND
			if (errorlog) fprintf(errorlog, "POKEY #%d AUDC3  $%02x (%s)\n", chip, val, audc2str(val));
#endif
            AUDC[CHAN3 + chip_offs] = val;
			AUDV[CHAN3 + chip_offs] = (val & VOLUME_MASK) * gain;
			chan_mask = 1 << CHAN3;
			break;

		case AUDF4_C:
			if (val == AUDF[CHAN4 + chip_offs])
				return;
#if VERBOSE_SOUND
            if (errorlog) fprintf(errorlog, "POKEY #%d AUDF4  $%02x\n", chip, val);
#endif
            AUDF[CHAN4 + chip_offs] = val;
			chan_mask = 1 << CHAN4;
			break;

		case AUDC4_C:
			if (val == AUDC[CHAN4 + chip_offs])
				return;
#if VERBOSE_SOUND
			if (errorlog) fprintf(errorlog, "POKEY #%d AUDC4  $%02x (%s)\n", chip, val, audc2str(val));
#endif
            AUDC[CHAN4 + chip_offs] = val;
			AUDV[CHAN4 + chip_offs] = (val & VOLUME_MASK) * gain;
			chan_mask = 1 << CHAN4;
            break;

		case AUDCTL_C:
			if (val == AUDCTL[chip])
				return;
#if VERBOSE_SOUND
			if (errorlog) fprintf(errorlog, "POKEY #%d AUDCTL $%02x (%s)\n", chip, val, audctl2str(val));
#endif
            AUDCTL[chip] = val;
			chan_mask = 15; 	  /* all channels */
			/* determine the base multiplier for the 'div by n' calculations */
			Base_mult[chip] = (AUDCTL[chip] & CLOCK_15) ? DIV_15 : DIV_64;
			break;

		case STIMER_C:
			/* first remove any existing timers */
#if VERBOSE_TIMER
            if (errorlog) fprintf(errorlog, "POKEY #%d STIMER $%02x\n", chip, val);
#endif
            if (TIMER[chip][TIMER1])
				timer_remove(TIMER[chip][TIMER1]);
			if (TIMER[chip][TIMER2])
				timer_remove(TIMER[chip][TIMER2]);
			if (TIMER[chip][TIMER4])
				timer_remove(TIMER[chip][TIMER4]);
            TIMER[chip][TIMER1] = 0;
			TIMER[chip][TIMER2] = 0;
			TIMER[chip][TIMER4] = 0;

			/* reset all counters to zero (side effect) */
			Div_n_cnt[CHAN1 + chip_offs] = 0;
			Div_n_cnt[CHAN2 + chip_offs] = 0;
			Div_n_cnt[CHAN3 + chip_offs] = 0;
			Div_n_cnt[CHAN4 + chip_offs] = 0;

            /* joined chan#1 and chan#2 ? */
			if (AUDCTL[chip] & CH1_CH2)
			{
				if (Div_n_tmr[CHAN2 + chip_offs] > MIN_TIMER)
                {
#if VERBOSE_TIMER
					if (errorlog) fprintf(errorlog, "POKEY #%d timer1+2 after %d clocks\n", chip, Div_n_tmr[CHAN2 + chip_offs]);
#endif
                    /* set timer #1 _and_ #2 event after Div_n_tmr clocks of joined CHAN1+CHAN2 */
					TIMER[chip][TIMER2] =
						timer_pulse(1.0 * Div_n_tmr[CHAN2 + chip_offs] / intf->baseclock,
							(chip << CHIP_SHIFT) | IRQ_TIMR2 | IRQ_TIMR1, Pokey_Timer);
				}
            }
			else
			{
				if (Div_n_tmr[CHAN1 + chip_offs] > MIN_TIMER)
                {
#if VERBOSE_TIMER
					if (errorlog) fprintf(errorlog, "POKEY #%d timer1 after %d clocks\n", chip, Div_n_tmr[CHAN1 + chip_offs]);
#endif
                    /* set timer #1 event after Div_n_tmr clocks of CHAN1 */
					TIMER[chip][TIMER1] =
						timer_pulse(1.0 * Div_n_tmr[CHAN1 + chip_offs] / intf->baseclock,
							(chip << CHIP_SHIFT) | IRQ_TIMR1, Pokey_Timer);
				}
				if (Div_n_tmr[CHAN1 + chip_offs] > MIN_TIMER)
                {
#if VERBOSE_TIMER
					if (errorlog) fprintf(errorlog, "POKEY #%d timer2 after %d clocks\n", chip, Div_n_tmr[CHAN2 + chip_offs]);
#endif
                    /* set timer #2 event after Div_n_tmr clocks of CHAN2 */
					TIMER[chip][TIMER2] =
						timer_pulse(1.0 * Div_n_tmr[CHAN2 + chip_offs] / intf->baseclock,
							(chip << CHIP_SHIFT) | IRQ_TIMR2, Pokey_Timer);
				}
            }

            /* NB: POKEY has no timer #3 :) */

            if (AUDCTL[chip] & CH3_CH4)
			{
				/* not sure about this: if audc4 == 0000xxxx don't start timer 4 ? */
				if (AUDC[CHAN4 + chip_offs] & 0xf0)
				{
					if (Div_n_tmr[CHAN4 + chip_offs] > MIN_TIMER)
					{
#if VERBOSE_TIMER
						if (errorlog) fprintf(errorlog, "POKEY #%d timer4 after %d clocks\n", chip, Div_n_tmr[CHAN4 + chip_offs]);
#endif
                        /* set timer #4 event after Div_n_tmr clocks of CHAN4 */
						TIMER[chip][TIMER4] =
							timer_pulse(1.0 * Div_n_tmr[CHAN4 + chip_offs] / intf->baseclock,
								(chip << CHIP_SHIFT) | IRQ_TIMR4, Pokey_Timer);
					}
				}
            }
			else
			{
				if (Div_n_tmr[CHAN4 + chip_offs] > MIN_TIMER)
				{
#if VERBOSE_TIMER
					if (errorlog) fprintf(errorlog, "POKEY #%d timer4 after %d clocks\n", chip, Div_n_tmr[CHAN4 + chip_offs]);
#endif
                    /* set timer #4 event after Div_n_tmr clocks of CHAN4 */
					TIMER[chip][TIMER4] =
						timer_pulse(1.0 * Div_n_tmr[CHAN4 + chip_offs] / intf->baseclock,
							(chip << CHIP_SHIFT) | IRQ_TIMR4, Pokey_Timer);
				}
            }
			if (TIMER[chip][TIMER1])
				timer_enable(TIMER[chip][TIMER1], IRQEN[chip] & IRQ_TIMR1);
			if (TIMER[chip][TIMER2])
				timer_enable(TIMER[chip][TIMER2], IRQEN[chip] & IRQ_TIMR2);
			if (TIMER[chip][TIMER4])
				timer_enable(TIMER[chip][TIMER4], IRQEN[chip] & IRQ_TIMR4);
			break;

        case SKREST_C:
			/* reset SKSTAT */
#if VERBOSE
            if (errorlog) fprintf(errorlog, "POKEY #%d SKREST $%02x\n", chip, val);
#endif
            SKSTAT[chip] &= ~(SK_FRAME|SK_OVERRUN|SK_KBERR);
			break;

		case POTGO_C:
#if VERBOSE
            if (errorlog) fprintf(errorlog, "POKEY #%d POTGO  $%02x\n", chip, val);
#endif
            break;

		case SEROUT_C:
#if VERBOSE
            if (errorlog) fprintf(errorlog, "POKEY #%d SEROUT $%02x\n", chip, val);
#endif
            if (intf->serout_w[chip])
				(*intf->serout_w[chip])(addr, val);
			SKSTAT[chip] |= SK_SEROUT;
			if (IRQEN[chip] & IRQ_SEROR)
			{
				IRQST[chip] |= IRQ_SEROR;
				if (intf->interrupt_cb[chip])
					(*intf->interrupt_cb[chip])(IRQ_SEROR);
			}
			break;

        case IRQEN_C:
#if VERBOSE
            if (errorlog) fprintf(errorlog, "POKEY #%d IRQEN  $%02x\n", chip, val);
#endif
            /* check if serout ready is clear */
			if (!(IRQST[chip] & IRQ_SEROR))
			{
				/* write enables serout completed ? */
				if (val & IRQ_SEROC)
				{
					/* SK status serout bit was set ? */
					if (SKSTAT[chip] & SK_SEROUT)
					{
						/* remove SK status serout bit */
						SKSTAT[chip] &= ~SK_SEROUT;
						/* and set the serout complete bit */
						IRQST[chip] |= IRQ_SEROC;
						if (intf->interrupt_cb[chip])
							(*intf->interrupt_cb[chip])(IRQ_SEROC);
					}
				}
            }
			/* acknowledge one or more IRQST bits ? */
            if (IRQST[chip] & ~val)
			{
				/* reset IRQST bits that are masked now */
				IRQST[chip] &= val;
			}
			else
			{
				/* enable/disable timers now to avoid unneeded
				   breaking of the CPU cores for masked timers */
				if (TIMER[chip][TIMER1] && ((IRQEN[chip]^val) & IRQ_TIMR1))
					timer_enable(TIMER[chip][TIMER1], val & IRQ_TIMR1);
				if (TIMER[chip][TIMER2] && ((IRQEN[chip]^val) & IRQ_TIMR2))
					timer_enable(TIMER[chip][TIMER2], val & IRQ_TIMR2);
				if (TIMER[chip][TIMER4] && ((IRQEN[chip]^val) & IRQ_TIMR4))
					timer_enable(TIMER[chip][TIMER4], val & IRQ_TIMR4);
			}
			/* store irq enable */
            IRQEN[chip] = val;
            break;

		case SKCTL_C:
			if (val == SKCTL[chip])
				return;
#if VERBOSE
            if (errorlog) fprintf(errorlog, "POKEY #%d SKCTL  $%02x\n", chip, val);
#endif
            SKCTL[chip] = val;
			if (!(val & SK_RESET))
			{
				Update_pokey_sound(IRQEN_C,  0, chip, gain);
                Update_pokey_sound(SKREST_C, 0, chip, gain);
#if 0	/* reset does not seem to change the AUDxx registers!? */
                Update_pokey_sound(AUDCTL_C, 0, chip, gain);
                Update_pokey_sound(AUDC4_C,  0, chip, gain);
                Update_pokey_sound(AUDF4_C,  0, chip, gain);
                Update_pokey_sound(AUDC3_C,  0, chip, gain);
                Update_pokey_sound(AUDF3_C,  0, chip, gain);
                Update_pokey_sound(AUDC2_C,  0, chip, gain);
                Update_pokey_sound(AUDF2_C,  0, chip, gain);
                Update_pokey_sound(AUDC1_C,  0, chip, gain);
                Update_pokey_sound(AUDF1_C,  0, chip, gain);
#endif
            }
			break;
    }

    /************************************************************/
    /* As defined in the manual, the exact Div_n_cnt values are */
    /* different depending on the frequency and resolution:     */
    /*    64 kHz or 15 kHz - AUDF + 1                           */
    /*    1 MHz, 8-bit -     AUDF + 4                           */
    /*    1 MHz, 16-bit -    AUDF[CHAN1]+256*AUDF[CHAN2] + 7    */
    /************************************************************/

    /* only reset the channels that have changed */

    if (chan_mask & (1 << CHAN1))
    {
		/* process channel 1 frequency */
		if (AUDCTL[chip] & CH1_179)
			new_val = AUDF[CHAN1 + chip_offs] + 4;
		else
			new_val = (AUDF[CHAN1 + chip_offs] + 1) * Base_mult[chip];

#if VERBOSE_SOUND
		if (errorlog) fprintf(errorlog, "POKEY #%d chan1 %d\n", chip, new_val);
#endif
        /* timer 'div by n' changed ? */
		if (intf->interrupt_cb[chip] && new_val != Div_n_tmr[CHAN1 + chip_offs])
		{
			Div_n_tmr[CHAN1 + chip_offs] = new_val;
			if (TIMER[chip][TIMER1] && new_val > MIN_TIMER)
				timer_reset(TIMER[chip][TIMER1], 1.0 * new_val / intf->baseclock);
		}

        /* sound 'div by n' changed ? */
        if (new_val != Div_n_max[CHAN1 + chip_offs])
		{
			Div_n_max[CHAN1 + chip_offs] = new_val;
			if (Div_n_cnt[CHAN1 + chip_offs] > new_val)
				Div_n_cnt[CHAN1 + chip_offs] = new_val;
		}
    }

    if (chan_mask & (1 << CHAN2))
    {
		/* process channel 2 frequency */
		if (AUDCTL[chip] & CH1_CH2)
		{
			if (AUDCTL[chip] & CH1_179)
				new_val = AUDF[CHAN2 + chip_offs] * 256 +
						  AUDF[CHAN1 + chip_offs] + 7;
			else
				new_val = (AUDF[CHAN2 + chip_offs] * 256 +
						   AUDF[CHAN1 + chip_offs] + 1) * Base_mult[chip];
		}
		else
			new_val = (AUDF[CHAN2 + chip_offs] + 1) * Base_mult[chip];

#if VERBOSE_SOUND
		if (errorlog) fprintf(errorlog, "POKEY #%d chan2 %d\n", chip, new_val);
#endif
        /* timer 'div by n' changed ? */
        if (intf->interrupt_cb[chip] && new_val != Div_n_tmr[CHAN2 + chip_offs])
		{
			Div_n_tmr[CHAN2 + chip_offs] = new_val;
			if (TIMER[chip][TIMER2] && new_val > MIN_TIMER)
				timer_reset(TIMER[chip][TIMER2], 1.0 * new_val / intf->baseclock);
        }

        /* sound 'div by n' changed ? */
        if (new_val != Div_n_max[CHAN2 + chip_offs])
		{
			Div_n_max[CHAN2 + chip_offs] = new_val;
			if (Div_n_cnt[CHAN2 + chip_offs] > new_val)
				Div_n_cnt[CHAN2 + chip_offs] = new_val;
        }
    }

    if (chan_mask & (1 << CHAN3))
    {
		/* process channel 3 frequency */
		if (AUDCTL[chip] & CH3_179)
			new_val = AUDF[CHAN3 + chip_offs] + 4;
		else
			new_val= (AUDF[CHAN3 + chip_offs] + 1) * Base_mult[chip];

#if VERBOSE_SOUND
		if (errorlog) fprintf(errorlog, "POKEY #%d chan3 %d\n", chip, new_val);
#endif
        /* channel 3 has no timer associated; we don't need Div_n_tmr[CHAN3] */
		if (new_val != Div_n_max[CHAN3 + chip_offs])
		{
			Div_n_max[CHAN3 + chip_offs] = new_val;
			if (Div_n_cnt[CHAN3 + chip_offs] > new_val)
				Div_n_cnt[CHAN3 + chip_offs] = new_val;
        }
    }

    if (chan_mask & (1 << CHAN4))
    {
		/* process channel 4 frequency */
		if (AUDCTL[chip] & CH3_CH4)
		{
			if (AUDCTL[chip] & CH3_179)
				new_val = AUDF[CHAN4 + chip_offs] * 256 +
						  AUDF[CHAN3 + chip_offs] + 7;
			else
				new_val = (AUDF[CHAN4 + chip_offs] * 256 +
						   AUDF[CHAN3 + chip_offs] + 1) * Base_mult[chip];
		}
		else
			new_val = (AUDF[CHAN4 + chip_offs] + 1) * Base_mult[chip];

#if VERBOSE_SOUND
		if (errorlog) fprintf(errorlog, "POKEY #%d chan4 %d\n", chip, new_val);
#endif
        /* timer 'div by n' changed ? */
        if (intf->interrupt_cb[chip] && new_val != Div_n_tmr[CHAN4 + chip_offs])
		{
			Div_n_tmr[CHAN4 + chip_offs] = new_val;
			if (TIMER[chip][TIMER4] && new_val > MIN_TIMER)
				timer_reset(TIMER[chip][TIMER4], 1.0 * new_val / intf->baseclock);
        }

        /* sound 'div by n' changed ? */
        if (new_val != Div_n_max[CHAN4 + chip_offs])
		{
			Div_n_max[CHAN4 + chip_offs] = new_val;
			if (Div_n_cnt[CHAN4 + chip_offs] > new_val)
				Div_n_cnt[CHAN4 + chip_offs] = new_val;
        }
    }

	/* if channel is volume only, set current output */
    for (chan = CHAN1; chan <= CHAN4; chan++)
    {
		if (chan_mask & (1 << chan))
		{
			/* I've disabled any frequencies that exceed the sampling
			   frequency.  There isn't much point in processing frequencies
			   that the hardware can't reproduce.  I've also disabled
			   processing if the volume is zero. */

            /* if the channel is volume only */
			/* or the channel is off (volume == 0) */
			/* or the channel freq is greater than the playback freq */
			if ((AUDC[chan + chip_offs] & VOL_ONLY) ||
			   !(AUDC[chan + chip_offs] & VOLUME_MASK)
#if USE_SAMP_N_MAX
				|| (Div_n_max[chan + chip_offs] < (Samp_n_max >> 8)) */
#endif
               )
			{
                /* indicate the channel is 'on' */
				Outbit[chan + chip_offs] = 1;
				/* can only ignore channel if filtering off */
				if ((chan == CHAN3 && !(AUDCTL[chip] & CH1_FILTER)) ||
					(chan == CHAN4 && !(AUDCTL[chip] & CH2_FILTER)) ||
					(chan == CHAN1) ||
					(chan == CHAN2)
#if USE_SAMP_N_MAX
					|| (Div_n_max[chan + chip_offs] < (Samp_n_max >> 8))
#endif
                   )
				{
					/* and set channel freq to max to reduce processing */
					Div_n_max[chan + chip_offs] = 0x7fffffffL;
					Div_n_cnt[chan + chip_offs] = 0x7fffffffL;
				}
            }
        }
    }
}


/*****************************************************************************/
/* Module:  Pokey_process()                                                  */
/* Purpose: To fill the output buffer with the sound output based on the     */
/*			pokey chip parameters.											 */
/*                                                                           */
/* Author:  Ron Fries                                                        */
/* Date:    January 1, 1997                                                  */
/*                                                                           */
/* Inputs:	chip - chip number to process									 */
/*			*buffer - pointer to the buffer where the audio output will 	 */
/*                    be placed                                              */
/*          n - size of the playback buffer                                  */
/*                                                                           */
/* Outputs: the buffer will be filled with n bytes of audio - no return val  */
/*                                                                           */
/*****************************************************************************/

void Pokey_process (int chip, void *buffer, int n)
{
	REGISTER UINT32 *div_n_ptr;
	REGISTER UINT32 event_min;
	REGISTER UINT8 next_event;
	REGISTER UINT16 chip_offs;
	REGISTER UINT8 *buf_ptr;
#ifdef CLIP 					/* if clipping is selected */
    REGISTER INT16 cur_val;     /* then we have to count as 16-bit */
#else
    REGISTER INT8 cur_val;      /* otherwise we'll simplify as 8-bit */
#endif

/* GSL 980313 B'zarre defines to handle optimised non-dword-aligned load/stores on ARM etc processors */
/* HDG 980501 Removed and replaced by cleaner solution without ifdef's,
   as in use with unix port for several versions now */

	if (n < 0 || chip >= Num_pokeys) {
		if (errorlog) fprintf (errorlog, "Pokey_process called with %d,0x%08x,%d\n", chip, (int)buffer, n);
		return;
	}

	chip_offs = chip << 2;

    /* The current output is pre-determined and then adjusted based on each */
    /* output change for increased performance (less over-all math). */
    /* add the output values of all 4 channels */

	cur_val = (Outbit[chip_offs+CHAN1]?AUDV[chip_offs+CHAN1]:-AUDV[chip_offs+CHAN1]) / 2 +
			  (Outbit[chip_offs+CHAN2]?AUDV[chip_offs+CHAN2]:-AUDV[chip_offs+CHAN2]) / 2 +
			  (Outbit[chip_offs+CHAN3]?AUDV[chip_offs+CHAN3]:-AUDV[chip_offs+CHAN3]) / 2 +
			  (Outbit[chip_offs+CHAN4]?AUDV[chip_offs+CHAN4]:-AUDV[chip_offs+CHAN4]) / 2;

	buf_ptr = buffer;

    /* loop until the buffer is filled */
	while (n > 0) {
		/* Normally the routine would simply decrement the 'div by N' */
		/* counters and react when they reach zero.  Since we normally */
		/* won't be processing except once every 80 or so counts, */
		/* I've optimized by finding the smallest count and then */
		/* 'accelerated' time by adjusting all pointers by that amount. */

		/* find next smallest event (either sample or chan 1-4) */
        next_event = SAMPLE;
        event_min = Samp_n_cnt[0];

		if (Div_n_cnt[chip_offs+CHAN1] <= event_min) {
			event_min = Div_n_cnt[chip_offs+CHAN1];
			next_event = chip_offs + CHAN1;
		}
		if (Div_n_cnt[chip_offs+CHAN2] <= event_min) {
			event_min = Div_n_cnt[chip_offs+CHAN2];
			next_event = chip_offs + CHAN2;
        }
		if (Div_n_cnt[chip_offs+CHAN3] <= event_min) {
			event_min = Div_n_cnt[chip_offs+CHAN3];
			next_event = chip_offs + CHAN3;
        }
		if (Div_n_cnt[chip_offs+CHAN4] <= event_min) {
			event_min = Div_n_cnt[chip_offs+CHAN4];
			next_event = chip_offs + CHAN4;
        }

        /* decrement all counters by the smallest count found */

		Div_n_cnt[chip_offs+CHAN1] -= event_min;
		Div_n_cnt[chip_offs+CHAN2] -= event_min;
		Div_n_cnt[chip_offs+CHAN3] -= event_min;
		Div_n_cnt[chip_offs+CHAN4] -= event_min;

        Samp_n_cnt[0] -= event_min;

		/* since the polynomials require a mod (%) function which is
		   division, I don't adjust the polynomials on the SAMPLE events,
		   only the CHAN events.  I have to keep track of the change,
		   though. */
		Poly_adjust += event_min;

		/* if the next event is a channel change */
		if (next_event != SAMPLE)
		{
			REGISTER UINT8 audc;
			REGISTER UINT8 audctl;
			REGISTER UINT8 toggle;

            /* shift the polynomial counters */
			P4	= (P4  + Poly_adjust) % POLY4_SIZE;
			P5	= (P5  + Poly_adjust) % POLY5_SIZE;
			P9	= (P9  + Poly_adjust) % POLY9_SIZE;
			P17 = (P17 + Poly_adjust) % POLY17_SIZE;

			/* reset the polynomial adjust counter to zero */
			Poly_adjust = 0;

			/* adjust channel counter */
			Div_n_cnt[next_event] += Div_n_max[next_event];

			/* get the current AUDC into a register (for optimization) */
			audc = AUDC[next_event];

			/* get the current chips AUDCTL */
			audctl = AUDCTL[next_event>>2];

			/* assume no changes to the output */
			toggle = 0;

			/* From here, a good understanding of the hardware is required */
			/* to understand what is happening.  I won't be able to provide */
			/* much description to explain it here. */

            /* if the output is pure or the output is poly5 */
			/* and the poly5 bit is set */
			if ((audc & NOTPOLY5) || poly5[P5]) {
				/* if the PURE bit is set */
				if (audc & PURE) {
					/* simply toggle the output */
					toggle = 1;
				} else if (audc & POLY4) {
					/* otherwise if POLY4 is selected */
					/* then compare to the poly4 bit */
					toggle = poly4[P4] == !Outbit[next_event];
				} else {
					/* if 9-bit poly is selected on this chip */
					if (audctl & POLY9) {
						/* compare to the poly9 bit */
						toggle = poly9[P9] == !Outbit[next_event];
					} else {
						/* otherwise compare to the poly17 bit 0 */
						toggle = poly17[P17] == !Outbit[next_event];
					}
				}
			}

            /* check channel 1 filter (clocked by channel 3) */
			if (audctl & CH1_FILTER) {
				/* if we're processing channel 3 */
				if ((next_event & 3) == CHAN3) {
					/* check output of channel 1 on same chip */
					if (Outbit[next_event-2]) {
						/* if on, turn it off */
						Outbit[next_event-2] = 0;
						cur_val -= AUDV[next_event & ~2];
					}
				}
			}

			/* check channel 2 filter (clocked by channel 4) */
			if (audctl & CH2_FILTER) {
				/* if we're processing channel 4 */
				if ((next_event & 3) == CHAN4) {
					/* check output of channel 2 on same chip */
					if (Outbit[next_event-2]) {
						/* if on, turn it off */
						Outbit[next_event-2] = 0;
						cur_val -= AUDV[next_event & ~2];
					}
				}
			}

            /* if the current output bit has changed */
			if (toggle) {
				if (Outbit[next_event]) {
					/* remove this channel from the signal */
					cur_val -= AUDV[next_event];

					/* and turn the output off */
					Outbit[next_event] = 0;
				} else {
					/* turn the output on */
					Outbit[next_event] = 1;

					/* and add it to the output signal */
					cur_val += AUDV[next_event];
				}
			}
		} else {
			/* otherwise we're processing a sample.
			   adjust the sample counter - note we're using the
			   24.8 integer includes an 8 bit fraction for accuracy */
			Samp_n_cnt[1] += Samp_n_max;
			if ( Samp_n_cnt[1] & 0xffffff00 )
			{
				Samp_n_cnt[0] += (Samp_n_cnt[1]>>8);
				Samp_n_cnt[1] &= 0x000000ff;
			}

#ifdef CLIP                         /* if clipping is selected */
            if (clip)
            {
				if (cur_val > 127) *buf_ptr++ = 127;
				else if (cur_val < -128) *buf_ptr++ = -128;
				else *buf_ptr++ = (INT8)cur_val;
			}
			else *buf_ptr++ = (INT8)cur_val;
#else
			*buf_ptr++ = (INT8)cur_val;  /* clipping not selected, use value */
#endif
			/* and indicate one less byte in the buffer */
			n--;
		}
    }
}

int pokey_sh_start (struct POKEYinterface *interface)
{
	int res;

    intf = interface;

	res = Pokey_sound_init (intf->baseclock, Machine->sample_rate, intf->volume, intf->num, intf->clip);

	return res;
}


void pokey_sh_stop (void)
{
	if (rand17)	free (rand17);
	rand17 = NULL;
	if (poly17)	free (poly17);
    poly17 = NULL;
}

static void update_pokeys(void)
{
	int chip;

	for (chip = 0; chip < Num_pokeys; chip++)
		stream_update(channel[chip], 0);
}

/*****************************************************************************/
/* Module:  Read_pokey_regs()                                                */
/* Purpose: To return the values of the Pokey registers. Currently, only the */
/*          random number generator register is returned.                    */
/*                                                                           */
/* Author:  Keith Gerdes, Brad Oliver & Eric Smith                           */
/* Date:    August 8, 1997                                                   */
/*                                                                           */
/* Inputs:  addr - the address of the parameter to be changed                */
/*          chip - the pokey chip to read                                    */
/*                                                                           */
/* Outputs: Adjusts local globals, returns the register in question          */
/*                                                                           */
/*****************************************************************************/

int Read_pokey_regs (int addr, int chip)
{
	int data = 0;	/* note: not returning 0 for unsupported ports breaks */
					/* Quantum and Food Fight */


    switch (addr & 0x0f)
    {
        case POT0_C:
			if (intf->pot0_r[chip]) data = (*intf->pot0_r[chip])(addr);
            break;
        case POT1_C:
			if (intf->pot1_r[chip]) data = (*intf->pot1_r[chip])(addr);
            break;
        case POT2_C:
			if (intf->pot2_r[chip]) data = (*intf->pot2_r[chip])(addr);
            break;
        case POT3_C:
			if (intf->pot3_r[chip]) data = (*intf->pot3_r[chip])(addr);
            break;
        case POT4_C:
			if (intf->pot4_r[chip]) data = (*intf->pot4_r[chip])(addr);
            break;
        case POT5_C:
			if (intf->pot5_r[chip]) data = (*intf->pot5_r[chip])(addr);
            break;
        case POT6_C:
			if (intf->pot6_r[chip]) data = (*intf->pot6_r[chip])(addr);
            break;
        case POT7_C:
			if (intf->pot7_r[chip]) data = (*intf->pot7_r[chip])(addr);
            break;

        case ALLPOT_C:
			if (intf->allpot_r[chip]) data = (*intf->allpot_r[chip])(addr);
            break;

        case KBCODE_C:
			/* clear keyboard status bit */
			data = KBCODE[chip];
            break;

        case RANDOM_C:
			{
				/****************************************************************
				 * If the 2 least significant bits of SKCTL are 0, the random
				 * number generator is disabled (SKRESET). Thanks to Eric Smith
				 * for pointing out this critical bit of info! If the random
				 * number generator is enabled, get a new random number. Take
				 * the time gone since the last read into account and read the
				 * new value from an appropriate offset in the rand17 table.
				 ****************************************************************/
				/* save the absolute time of this read */
                if (SKCTL[chip] & SK_RESET)
				{
					UINT32 rngoffs = (UINT32)(timer_get_time() * intf->baseclock) % POLY17_SIZE;
					/* and get the random value */
					RANDOM[chip] = rand17[rngoffs];
#if VERBOSE_RANDOM
					if (errorlog) fprintf (errorlog, "POKEY #%d rand17[$%05x]: $%02x\n", chip, rngoffs, RANDOM[chip]);
#endif
				}
				else
				{
#if VERBOSE_RANDOM
					if (errorlog) fprintf (errorlog, "POKEY #%d rand17 freezed (SKCTL): $%02x\n", chip, RANDOM[chip]);
#endif
				}
			}
			data = RANDOM[chip];
            break;

        case SERIN_C:
			if (intf->serin_r[chip]) SERIN[chip] = (*intf->serin_r[chip])(addr);
			data = SERIN[chip];
#if VERBOSE
            if (errorlog) fprintf (errorlog, "POKEY #%d SERIN  $%02x\n", chip, data);
#endif
            break;

        case IRQST_C:
			/* IRQST is an active low input port; we keep it active high */
			/* internally to ease the (un-)masking of bits */
			data = IRQST[chip] ^ 0xff;
#if VERBOSE
            if (errorlog) fprintf (errorlog, "POKEY #%d IRQST  $%02x\n", chip, data);
#endif
            break;

        case SKSTAT_C:
			/* SKSTAT is an active low input port also */
			data = SKSTAT[chip] ^ 0xff;
#if VERBOSE
            if (errorlog) fprintf (errorlog, "POKEY #%d SKSTAT $%02x\n", chip, data);
#endif
            break;

        default:
#if VERBOSE
            if (errorlog) fprintf (errorlog, "POKEY #%d register $%02x\n", chip, addr);
#endif
            break;
    }

	return data;
}

int pokey1_r (int offset)
{
    return Read_pokey_regs (offset,0);
}

int pokey2_r (int offset)
{
    return Read_pokey_regs (offset,1);
}

int pokey3_r (int offset)
{
    return Read_pokey_regs (offset,2);
}

int pokey4_r (int offset)
{
    return Read_pokey_regs (offset,3);
}

int quad_pokey_r (int offset)
{
    int pokey_num = (offset >> 3) & ~0x04;
    int control = (offset & 0x20) >> 2;
    int pokey_reg = (offset % 8) | control;

#if VERBOSE
	if (errorlog) fprintf (errorlog, "POKEY offset: %04x pokey_num: %02x pokey_reg: %02x\n", offset, pokey_num, pokey_reg);
#endif
    return Read_pokey_regs (pokey_reg,pokey_num);
}


void pokey1_w (int offset,int data)
{
    update_pokeys ();
    Update_pokey_sound (offset,data,0,intf->gain);
}

void pokey2_w (int offset,int data)
{
    update_pokeys ();
    Update_pokey_sound (offset,data,1,intf->gain);
}

void pokey3_w (int offset,int data)
{
    update_pokeys ();
    Update_pokey_sound (offset,data,2,intf->gain);
}

void pokey4_w (int offset,int data)
{
    update_pokeys ();
    Update_pokey_sound (offset,data,3,intf->gain);
}

void quad_pokey_w (int offset,int data)
{
    int pokey_num = (offset >> 3) & ~0x04;
    int control = (offset & 0x20) >> 2;
    int pokey_reg = (offset % 8) | control;

    switch (pokey_num) {
        case 0:
            pokey1_w (pokey_reg, data);
            break;
        case 1:
            pokey2_w (pokey_reg, data);
            break;
        case 2:
            pokey3_w (pokey_reg, data);
            break;
        case 3:
            pokey4_w (pokey_reg, data);
            break;
	}
}

void pokey1_serin_ready(int after)
{
	timer_set(1.0 * after / intf->baseclock, 0, Pokey_SerinReady);
}

void pokey2_serin_ready(int after)
{
	timer_set(1.0 * after / intf->baseclock, 1, Pokey_SerinReady);
}

void pokey3_serin_ready(int after)
{
	timer_set(1.0 * after / intf->baseclock, 2, Pokey_SerinReady);
}

void pokey4_serin_ready(int after)
{
	timer_set(1.0 * after / intf->baseclock, 3, Pokey_SerinReady);
}

void pokey_break_w(int chip, int shift)
{
	if (shift)						/* shift code ? */
		SKSTAT[chip] |= SK_SHIFT;
	else
		SKSTAT[chip] &= ~SK_SHIFT;
	/* check if the break IRQ is enabled */
	if (IRQEN[chip] & IRQ_BREAK) {
		/* set break IRQ status and call back the interrupt handler */
        IRQST[chip] |= IRQ_BREAK;
		if (intf->interrupt_cb[chip])
			(*intf->interrupt_cb[chip])(IRQ_BREAK);
    }
}

void pokey1_break_w(int shift)
{
	pokey_break_w(0, shift);
}

void pokey2_break_w(int shift)
{
	pokey_break_w(1, shift);
}

void pokey3_break_w(int shift)
{
	pokey_break_w(2, shift);
}

void pokey4_break_w(int shift)
{
	pokey_break_w(3, shift);
}

void pokey_kbcode_w(int chip, int kbcode, int make)
{
	/* make code ? */
	if (make) {
        KBCODE[chip] = kbcode;
        SKSTAT[chip] |= SK_KEYBD;
		if (kbcode & 0x40)			/* shift code ? */
			SKSTAT[chip] |= SK_SHIFT;
		else
			SKSTAT[chip] &= ~SK_SHIFT;
		if (IRQEN[chip] & IRQ_KEYBD) {
			/* last interrupt not acknowledged ? */
			if (IRQST[chip] & IRQ_KEYBD)
				SKSTAT[chip] |= SK_KBERR;
			IRQST[chip] |= IRQ_KEYBD;
			if (intf->interrupt_cb[chip])
				(*intf->interrupt_cb[chip])(IRQ_KEYBD);
		}
	} else {
		KBCODE[chip] = kbcode;
		SKSTAT[chip] &= ~SK_KEYBD;
    }
}

void pokey1_kbcode_w(int kbcode, int make)
{
	pokey_kbcode_w(0, kbcode, make);
}

void pokey2_kbcode_w(int kbcode, int make)
{
	pokey_kbcode_w(1, kbcode, make);
}

void pokey3_kbcode_w(int kbcode, int make)
{
	pokey_kbcode_w(2, kbcode, make);
}

void pokey4_kbcode_w(int kbcode, int make)
{
	pokey_kbcode_w(3, kbcode, make);
}

void pokey_sh_update (void)
{
}

