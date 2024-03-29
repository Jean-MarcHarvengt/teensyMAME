/*****************************************************************************
 *
 *	 m6502.h
 *	 Portable 6502/65c02/6510 emulator interface
 *
 *	 Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#ifndef _M6502_H
#define _M6502_H

/****************************************************************************
 * sizeof(byte)=1, sizeof(word)=2, sizeof(dword)>=4
 ****************************************************************************/
#include "types.h"

#ifndef INLINE
#define INLINE static __inline
#endif

#define SUPP65C02	1		/* set to 1 to support the 65C02 opcodes */
#define SUPP6510	1		/* set to 1 to support the 6510 opcodes */

#define M6502_PLAIN 0		/* set M6502_Type to this for a plain 6502 emulation */

#ifdef  SUPP65C02
#define M6502_65C02 1		/* set M6502_Type to this for a 65C02 emulation */
#endif

#ifdef  SUPP6510
#define M6502_6510	2		/* set M6502_Type to this for a 6510 emulation */
#endif

#define FAST_MEMORY 1		/* set to 1 to test cur_mrhard/wmhard to avoid calls */

#define LAZY_FLAGS	1		/* set to 1 to use Bernd's idea for N and Z flags */

/****************************************************************************
 * Define a 6502 word. Upper bytes are always zero
 ****************************************************************************/
typedef 	union {
/*
#ifdef __128BIT__
 #ifdef LSB_FIRST
   struct { byte l,h,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11,h12,h13,h14,h15; } B;
   struct { word l,h,h2,h3,h4,h5,h6,h7; } W;
   dword D;
 #else
   struct { byte h15,h14,h13,h12,h11,h10,h9,h8,h7,h6,h5,h4,h3,h2,h,l; } B;
   struct { word h7,h6,h5,h4,h3,h2,h,l; } W;
   dword D;
 #endif
#elif __64BIT__
 #ifdef LSB_FIRST
   struct { byte l,h,h2,h3,h4,h5,h6,h7; } B;
   struct { word l,h,h2,h3; } W;
   dword D;
 #else
   struct { byte h7,h6,h5,h4,h3,h2,h,l; } B;
   struct { word h3,h2,h,l; } W;
   dword D;
 #endif
#else
*/
 #ifdef LSB_FIRST
   struct { byte l,h,h2,h3; } B;
   struct { word l,h; } W;
   dword D;
 #else
   struct { byte h3,h2,h,l; } B;
   struct { word h,l; } W;
   dword D;
 #endif
/*#endif*/
}	m6502_pair;

/****************************************************************************
 *** End of machine dependent definitions								  ***
 ****************************************************************************/

/****************************************************************************
 * The 6502 registers. HALT is set to 1 when the CPU is halted (6502c)
 ****************************************************************************/
typedef struct
{
	m6502_pair pc;					/* program counter */
	m6502_pair sp;					/* stack pointer (always 100 - 1FF) */
	m6502_pair zp;					/* zero page address */
	m6502_pair ea;					/* effective address */
	int a;							/* Accumulator */
	int x;							/* X index register */
	int y;							/* Y index register */
	int p;							/* Processor status */
	int halt;						/* nonzero if the CPU is halted */
	int pending_irq;				/* nonzero if an IRQ is pending */
	int pending_nmi;				/* nonzero if a NMI is pending */
	int after_cli;					/* pending IRQ and last insn cleared I */
#if LAZY_FLAGS
	int nz; 						/* last value (lazy N and Z flag) */
#endif
}   M6502_Regs;

#define M6502_INT_NONE	0
#define M6502_INT_IRQ	1
#define M6502_INT_NMI	2

#define M6502_NMI_VEC	0xfffa
#define M6502_RST_VEC	0xfffc
#define M6502_IRQ_VEC	0xfffe

extern int M6502_ICount;                /* cycle count */
extern int M6502_Type;					/* CPU subtype */

unsigned M6502_GetPC (void);			/* Get program counter */
void M6502_GetRegs (M6502_Regs *Regs);	/* Get registers */
void M6502_SetRegs (M6502_Regs *Regs);	/* Set registers */
void M6502_Reset (void);				/* Reset registers to the initial values */
int  M6502_Execute(int cycles); 		/* Execute cycles - returns number of cycles actually run */
void M6502_Cause_Interrupt(int type);
void M6502_Clear_Pending_Interrupts(void);

#endif /* _M6502_H */


