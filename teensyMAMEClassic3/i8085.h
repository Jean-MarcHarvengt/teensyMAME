#ifndef I8085_H
#define I8085_H
/*******************************************************
 *
 *      Portable (hopefully ;-) I8085A emulator
 *
 *      Written by J. Buchmueller for use with MAME
 *
 *      Partially based on Z80Em by Marcel De Kogel
 *
 *******************************************************/

#include "types.h"

typedef union {
#ifdef  LSB_FIRST
        struct { byte l,h,h2,h3; } B;
        struct { word l,h; } W;
        dword D;
#else
        struct { byte h3,h2,h,l; } B;
        struct { word h,l; } W;
        dword D;
#endif
}       I8085_Pair;

typedef struct {
        I8085_Pair      PC;
        I8085_Pair      SP;
        I8085_Pair      BC;
        I8085_Pair      DE;
        I8085_Pair      HL;
        I8085_Pair      AF;
        I8085_Pair      XX;     /* temporary storage for EA, immediates */
        int             HALT;
        int             IM;     /* interrupt mask */
        int             IREQ;   /* requested interrupts */
        int             ISRV;   /* serviced interrupt */
        int             INTR;   /* vector for INTR */
        int             IRQ2;   /* scheduled interrupt address */
        int             IRQ1;   /* executed interrupt address */
        void (*SOD_callback)(int state);
}       I8085_Regs;

#define I8085_INTR      0xff
#define I8085_SID       0x10
#define I8085_RST75     0x08
#define I8085_RST65     0x04
#define I8085_RST55     0x02
#define I8085_TRAP      0x01
#define I8085_NONE      0

extern  int     I8085_ICount;

void    I8085_SetRegs(I8085_Regs * regs);
void    I8085_GetRegs(I8085_Regs * regs);
int     I8085_GetPC(void);
void    I8085_SetSID(int state);
void    I8085_SetSOD_callback(void (*callback)(int state));
void    I8085_Reset(void);
int     I8085_Execute(int cycles);
void    I8085_Cause_Interrupt(int type);
void    I8085_Clear_Pending_Interrupts(void);

#endif
