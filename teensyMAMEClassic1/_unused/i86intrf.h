/* ASG 971222 -- rewrote this interface */
#ifndef __I86_H_
#define __I86_H_

#include "memory.h"
#include "types.h"

/* I86 registers */
typedef union
{                  /* eight general registers */
	word w[8];      /* viewed as 16 bits registers */
	byte b[16];     /* or as 8 bit registers */
} i86basicregs;

typedef struct
{
	i86basicregs regs;
	int ip;
	word flags;
	word sregs[4];
	int pending_interrupts;
} i86_Regs;



#define I86_INT_NONE 0
#define I86_NMI_INT 2


/* Public functions */
extern void i86_SetRegs(i86_Regs *Regs);
extern void i86_GetRegs(i86_Regs *Regs);
extern unsigned i86_GetPC(void);
extern void i86_Reset(void);
extern int i86_Execute(int cycles);
extern void i86_Cause_Interrupt(int type);
extern void i86_Clear_Pending_Interrupts(void);


extern int i86_ICount;

#endif
