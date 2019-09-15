/*** Z80Em: Portable Z80 emulator *******************************************/
/***                                                                      ***/
/***                                 Z80.h                                ***/
/***                                                                      ***/
/*** This file contains the function prototypes and variable declarations ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#ifndef _Z80_H
#define _Z80_H

#define Z80_DAISYCHAIN		/* DAISY CHAIN SUPPORT SECTION */

/****************************************************************************/
/*** Machine dependent definitions                                        ***/
/****************************************************************************/
/* #define DEBUG      */              /* Compile debugging version          */
/* #define X86_ASM    */              /* Compile optimised GCC/x86 version  */
/* #define LSB_FIRST  */              /* Compile for low-endian CPU         */
/* #define __64BIT__  */              /* Compile for 64 bit machines        */
/* #define __128BIT__ */              /* Compile for 128 bit machines       */

/****************************************************************************/
/* If your compiler doesn't know about inlined functions, uncomment this    */
/****************************************************************************/
/* #define INLINE static */

#ifndef EMU_TYPES
#define EMU_TYPES

/****************************************************************************/
/* sizeof(byte)=1, sizeof(word)=2, sizeof(dword)>=4                         */
/****************************************************************************/
#include "types.h"	/* -NS- */

/****************************************************************************/
/* Define a Z80 word. Upper bytes are always zero                           */
/****************************************************************************/
typedef union
{
#ifdef __128BIT__
 #ifdef LSB_FIRST
   struct { byte l,h,h2,h3,h4,h5,h6,h7,
                 h8,h9,h10,h11,h12,h13,h14,h15; } B;
   struct { word l,h,h2,h3,h4,h5,h6,h7; } W;
   dword D;
 #else
   struct { byte h15,h14,h13,h12,h11,h10,h9,h8,
                 h7,h6,h5,h4,h3,h2,h,l; } B;
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
 #ifdef LSB_FIRST
   struct { byte l,h,h2,h3; } B;
   struct { word l,h; } W;
   dword D;
 #else
   struct { byte h3,h2,h,l; } B;
   struct { word h,l; } W;
   dword D;
 #endif
#endif
} z80_pair;	/* -NS- */

#endif /* EMU_TYPES */

/****************************************************************************/
/*** End of machine dependent definitions                                 ***/
/****************************************************************************/

#ifndef INLINE
#define INLINE static inline
#endif

#ifdef Z80_DAISYCHAIN

#define Z80_MAXDAISY 4          /* maximum of daisy chan device */

#define Z80_INT_REQ 0x01        /* interrupt request mask       */
#define Z80_INT_IEO 0x02        /* interrupt disable mask(IEO)  */

#define Z80_VECTOR(device,state) (((device)<<8)|(state))

/* daisy-chain link */
typedef struct
{
   void (*reset)(int);            /* reset callback     */
   int  (*interrupt_entry)(int);  /* entry callback     */
   void (*interrupt_reti)(int);   /* reti callback      */
   int irq_param;                 /* callback paramater */
}Z80_DaisyChain;

#endif

/****************************************************************************/
/* The Z80 registers. HALT is set to 1 when the CPU is halted, the refresh  */
/* register is calculated as follows: refresh=(Regs.R&127)|(Regs.R2&128)    */
/****************************************************************************/
typedef struct
{
  z80_pair AF,BC,DE,HL,IX,IY,PC,SP;	/* -NS- */
  z80_pair AF2,BC2,DE2,HL2;	/* -NS- */
  unsigned IFF1,IFF2,HALT,IM,I,R,R2;

#ifdef Z80_DAISYCHAIN
  int vector;              /* interrupt vector for SINGLE INT mode   */
  int pending_irq;
  int irq_max;             /* number of daisy chain device           */
  int request_irq;         /* daisy chain next request device        */
  int service_irq;         /* daisy chain next reti handling devicve */
  int int_state[Z80_MAXDAISY];
  Z80_DaisyChain irq[Z80_MAXDAISY];
#else
  int pending_irq,pending_nmi;	/* NS 970904 */
#endif
} Z80_Regs;

/****************************************************************************/
/* Set Z80_Trace to 1 when PC==Z80_Trap. When trace is on, Z80_Debug() is   */
/* called after every instruction                                           */
/****************************************************************************/
#ifdef DEBUG
extern int Z80_Trace;
extern int Z80_Trap;
void Z80_Debug(Z80_Regs *R);
#endif

extern int Z80_Running;      /* When 0, emulation terminates                */
/*extern int Z80_IPeriod; */ /* NS 970904 */     /* Number of T-states per interrupt            */
extern int Z80_ICount;       /* T-state count                               */
/* extern int Z80_IRQ;*/  /* NS 970904 */          /* Current IRQ status. Checked after EI occurs */

#define Z80_IGNORE_INT  -1   /* Ignore interrupt                            */
#define Z80_NMI_INT     -2   /* Execute NMI                                 */

unsigned Z80_GetPC (void);         /* Get program counter                   */
void Z80_GetRegs (Z80_Regs *Regs); /* Get registers                         */
void Z80_SetRegs (Z80_Regs *Regs); /* Set registers                         */
#ifdef Z80_DAISYCHAIN
void Z80_Reset (Z80_DaisyChain *daisy_chain );
#else
void Z80_Reset (void);             /* Reset registers to the initial values */
#endif
/*int  Z80_Execute (void);*/ /* NS 970904 */           /* Execute IPeriod T-States              */
int  Z80_Execute(int cycles);           /* Execute cycles T-States - returns number of cycles actually run */
/*word Z80 (void);*/ /* NS 970904 */                   /* Execute until Z80_Running==0          */
void Z80_RegisterDump (void);      /* Prints a dump to stdout               */
void Z80_Patch (Z80_Regs *Regs);   /* Called when ED FE occurs. Can be used */
                                   /* to emulate disk access etc.           */
/*int Z80_Interrupt(void);*/           /* This is called after IPeriod T-States */
                                   /* have been executed. It should return  */
                                   /* Z80_IGNORE_INT, Z80_NMI_INT or a byte */
                                   /* identifying the device (most often    */
                                   /* 0xFF)                                 */
/*int cpu_interrupt(void);
#define Z80_Interrupt() (cpu_interrupt()) */  /* NS 970904 */

void Z80_Cause_Interrupt(int type);	/* NS 970904 */
void Z80_Clear_Pending_Interrupts(void);	/* NS 970904 */

void Z80_Reti (void);              /* Called when RETI occurs               */
void Z80_Retn (void);              /* Called when RETN occurs               */

/****************************************************************************/
/* Definitions of functions to read/write memory and I/O ports              */
/* You can replace these with your own, inlined if necessary                */
/****************************************************************************/
#include "Z80IO.h"

#endif /* _Z80_H */

