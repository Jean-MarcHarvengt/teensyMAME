/*** m6805: Portable 6805 emulator ******************************************/

#ifndef _M6805_H
#define _M6805_H

#include "memory.h"


/****************************************************************************/
/* sizeof(byte)=1, sizeof(word)=2, sizeof(dword)>=4                         */
/****************************************************************************/
#include "types.h"	/* -NS- */

/* 6805 Registers */
typedef struct
{
	word		pc;	/* Program counter */
	byte		s;		/* Stack pointer */
	byte		x;		/* Index register */
	byte		a;		/* Accumulator */
	byte		cc;

	int pending_interrupts;	/* MB */
} m6805_Regs;

#ifndef INLINE
#define INLINE static __inline
#endif


#define M6805_INT_NONE  0			/* No interrupt required */
#define M6805_INT_IRQ	1 			/* Standard IRQ interrupt */


/* PUBLIC FUNCTIONS */
extern void m6805_SetRegs(m6805_Regs *Regs);
extern void m6805_GetRegs(m6805_Regs *Regs);
extern unsigned m6805_GetPC(void);
extern void m6805_reset(void);
extern int  m6805_execute(int cycles);             /* MB */
extern void m6805_Cause_Interrupt(int type);       /* MB */
extern void m6805_Clear_Pending_Interrupts(void);  /* MB */

/* PUBLIC GLOBALS */
extern int	m6805_ICount;

/****************************************************************************/
/* Read a byte from given memory location                                   */
/****************************************************************************/
/* ASG 971005 -- changed to cpu_readmem16/cpu_writemem16 */
#define M6805_RDMEM(A) ((unsigned)cpu_readmem16(A))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
#define M6805_WRMEM(A,V) (cpu_writemem16(A,V))

/****************************************************************************/
/* M6805_RDOP() is identical to M6805_RDMEM() except it is used for reading */
/* opcodes. In case of system with memory mapped I/O, this function can be  */
/* used to greatly speed up emulation                                       */
/****************************************************************************/
#define M6805_RDOP(A) ((unsigned)cpu_readop(A))

/****************************************************************************/
/* M6805_RDOP_ARG() is identical to M6805_RDOP() but it's used for reading  */
/* opcode arguments. This difference can be used to support systems that    */
/* use different encoding mechanisms for opcodes and opcode arguments       */
/****************************************************************************/
#define M6805_RDOP_ARG(A) ((unsigned)cpu_readop_arg(A))

/****************************************************************************/
/* Flags for optimizing memory access. Game drivers should set m6805_Flags  */
/* to a combination of these flags depending on what can be safely          */
/* optimized. For example, if M6809_FAST_S is set, bytes are pulled         */
/* directly from the RAM array, and cpu_readmem() is not called.            */
/* The flags affect reads and writes.                                       */
/****************************************************************************/
extern int m6805_Flags;
#define M6805_FAST_NONE	0x00	/* no memory optimizations */
#define M6805_FAST_S	0x02	/* stack */

#ifndef FALSE
#    define FALSE 0
#endif
#ifndef TRUE
#    define TRUE (!FALSE)
#endif

#endif /* _M6805_H */
