/*** Z80Em: Portable Z80 emulator *******************************************/
/***                                                                      ***/
/***                                Z80IO.h                               ***/
/***                                                                      ***/
/*** This file contains the prototypes for the functions accessing memory ***/
/*** and I/O                                                              ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#include "memory.h"

/****************************************************************************/
/* Input a byte from given I/O port                                         */
/****************************************************************************/
/*byte Z80_In (byte Port);*/
#define Z80_In(Port) ((byte)cpu_readport(Port))

/****************************************************************************/
/* Output a byte to given I/O port                                          */
/****************************************************************************/
/*void Z80_Out (byte Port,byte Value);*/
#define Z80_Out(Port,Value) (cpu_writeport(Port,Value))

/****************************************************************************/
/* Read a byte from given memory location                                   */
/****************************************************************************/
/*unsigned Z80_RDMEM(dword A);*/
/* ASG 971005 -- changed to cpu_readmem16/cpu_writemem16 */
#define Z80_RDMEM(A) ((unsigned)cpu_readmem16(A))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
/*void Z80_WRMEM(dword A,byte V);*/
#define Z80_WRMEM(A,V) (cpu_writemem16(A,V))

/****************************************************************************/
/* Just to show you can actually use macros as well                         */
/****************************************************************************/
/*
 extern byte *ReadPage[256];
 extern byte *WritePage[256];
 #define Z80_RDMEM(a) ReadPage[(a)>>8][(a)&0xFF]
 #define Z80_WRMEM(a,v) WritePage[(a)>>8][(a)&0xFF]=v
*/

/****************************************************************************/
/* Z80_RDOP() is identical to Z80_RDMEM() except it is used for reading     */
/* opcodes. In case of system with memory mapped I/O, this function can be  */
/* used to greatly speed up emulation                                       */
/****************************************************************************/
/*#define Z80_RDOP(A)		Z80_RDMEM(A)*/
#define Z80_RDOP(A) ((unsigned)cpu_readop(A))

/****************************************************************************/
/* Z80_RDOP_ARG() is identical to Z80_RDOP() except it is used for reading  */
/* opcode arguments. This difference can be used to support systems that    */
/* use different encoding mechanisms for opcodes and opcode arguments       */
/****************************************************************************/
/*#define Z80_RDOP_ARG(A)		Z80_RDOP(A)*/
#define Z80_RDOP_ARG(A) ((unsigned)cpu_readop_arg(A))

/****************************************************************************/
/* Z80_RDSTACK() is identical to Z80_RDMEM() except it is used for reading  */
/* stack variables. In case of system with memory mapped I/O, this function */
/* can be used to slightly speed up emulation                               */
/****************************************************************************/
/*#define Z80_RDSTACK(A)		Z80_RDMEM(A)*/
/* ASG 971005 -- changed to cpu_readmem16/cpu_writemem16 */
#define Z80_RDSTACK(A) ((unsigned)cpu_readmem16(A))

/****************************************************************************/
/* Z80_WRSTACK() is identical to Z80_WRMEM() except it is used for writing  */
/* stack variables. In case of system with memory mapped I/O, this function */
/* can be used to slightly speed up emulation                               */
/****************************************************************************/
/*#define Z80_WRSTACK(A,V)	Z80_WRMEM(A,V)*/
#define Z80_WRSTACK(A,V) (cpu_writemem16(A,V))
