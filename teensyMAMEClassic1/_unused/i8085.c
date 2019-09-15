/*******************************************************
 *
 *      Portable (hopefully ;-) I8085A emulator
 *
 *      Written by Juergen Buchmueller for use with MAME
 *
 *      Partially based on Z80Em by Marcel De Kogel
 *
 *******************************************************/

#define VERBOSE

#ifdef  VERBOSE
#include <stdio.h>
#include "driver.h"
#endif

#include "memory.h"
#include "i8085.h"
#include "i8085cpu.h"
#include "i8085daa.h"

#ifndef INLINE
#define INLINE static __inline
#endif


int     I8085_ICount = 0;

static  I8085_Regs I;
static  byte    ZS[256];
static  byte    ZSP[256];

static  int     RDOP(void)          { return cpu_readop(I.PC.W.l++); }
static  int     RDOPARG(void)       { return cpu_readop_arg(I.PC.W.l++); }
static  int     RDOPARG_WORD(void)  { int w = cpu_readop_arg(I.PC.W.l++); w += cpu_readop_arg(I.PC.W.l++) << 8; return w; }
static  int     RDMEM(int a)        { return cpu_readmem16(a); }
static  void    WRMEM(int a, int v) { cpu_writemem16(a, v); }

INLINE void ExecOne(int opcode)
{
        switch (opcode)
        {
                case 0x00: I8085_ICount -= 4;   /* NOP  */
                        /* no op */
                        break;
                case 0x01: I8085_ICount -= 10;  /* LXI  B,nnnn */
                        I.BC.W.l = RDOPARG_WORD();
                        break;
                case 0x02: I8085_ICount -= 7;   /* STAX B */
                        WRMEM(I.BC.D, I.AF.B.h);
                        break;
                case 0x03: I8085_ICount -= 6;   /* INX  B */
                        I.BC.W.l++;
                        break;
                case 0x04: I8085_ICount -= 4;   /* INR  B */
                        M_INR(I.BC.B.h);
                        break;
                case 0x05: I8085_ICount -= 4;   /* DCR  B */
                        M_DCR(I.BC.B.h);
                        break;
                case 0x06: I8085_ICount -= 7;   /* MVI  B,nn */
                        M_MVI(I.BC.B.h);
                        break;
                case 0x07: I8085_ICount -= 4;   /* RLC  */
                        M_RLC;
                        break;

                case 0x08: I8085_ICount -= 4;   /* ???? */
                        break;
                case 0x09: I8085_ICount -= 11;  /* DAD  B */
                        M_DAD(BC);
                        break;
                case 0x0a: I8085_ICount -= 7;   /* LDAX B */
                        I.AF.B.h = RDMEM(I.BC.D);
                        break;
                case 0x0b: I8085_ICount -= 6;   /* DCX  B */
                        I.BC.W.l--;
                        break;
                case 0x0c: I8085_ICount -= 4;   /* INR  C */
                        M_INR(I.BC.B.l);
                        break;
                case 0x0d: I8085_ICount -= 4;   /* DCR  C */
                        M_DCR(I.BC.B.l);
                        break;
                case 0x0e: I8085_ICount -= 7;   /* MVI  C,nn */
                        M_MVI(I.BC.B.l);
                        break;
                case 0x0f: I8085_ICount -= 4;   /* RRC  */
                        M_RRC;
                        break;

                case 0x10: I8085_ICount -= 8;   /* ????  */
                        break;
                case 0x11: I8085_ICount -= 10;  /* LXI  D,nnnn */
                        I.DE.W.l = RDOPARG_WORD();
                        break;
                case 0x12: I8085_ICount -= 7;   /* STAX D */
                        WRMEM(I.DE.D, I.AF.B.h);
                        break;
                case 0x13: I8085_ICount -= 6;   /* INX  D */
                        I.DE.W.l++;
                        break;
                case 0x14: I8085_ICount -= 4;   /* INR  D */
                        M_INR(I.DE.B.h);
                        break;
                case 0x15: I8085_ICount -= 4;   /* DCR  D */
                        M_DCR(I.DE.B.h);
                        break;
                case 0x16: I8085_ICount -= 7;   /* MVI  D,nn */
                        M_MVI(I.DE.B.h);
                        break;
                case 0x17: I8085_ICount -= 4;   /* RAL  */
                        M_RAL;
                        break;

                case 0x18: I8085_ICount -= 7;   /* ????? */
                        break;
                case 0x19: I8085_ICount -= 11;  /* DAD  D */
                        M_DAD(DE);
                        break;
                case 0x1a: I8085_ICount -= 7;   /* LDAX D */
                        I.AF.B.h = RDMEM(I.DE.D);
                        break;
                case 0x1b: I8085_ICount -= 6;   /* DCX  D */
                        I.DE.W.l--;
                        break;
                case 0x1c: I8085_ICount -= 4;   /* INR  E */
                        M_INR(I.DE.B.l);
                        break;
                case 0x1d: I8085_ICount -= 4;   /* DCR  E */
                        M_DCR(I.DE.B.l);
                        break;
                case 0x1e: I8085_ICount -= 7;   /* MVI  E,nn */
                        M_MVI(I.DE.B.l);
                        break;
                case 0x1f: I8085_ICount -= 4;   /* RAR  */
                        M_RAR;
                        break;

                case 0x20: I8085_ICount -= 7;   /* RIM  */
                        I.AF.B.h = I.IM;
                        break;
                case 0x21: I8085_ICount -= 10;  /* LXI  H,nnnn */
                        I.HL.W.l = RDOPARG_WORD();
                        break;
                case 0x22: I8085_ICount -= 16;  /* SHLD nnnn */
                        I.XX.W.l = RDOPARG_WORD();
                        WRMEM(I.XX.D, I.HL.B.l);
                        I.XX.W.l++;
                        WRMEM(I.XX.D, I.HL.B.h);
                        break;
                case 0x23: I8085_ICount -= 6;   /* INX  H */
                        I.HL.W.l++;
                        break;
                case 0x24: I8085_ICount -= 4;   /* INR  H */
                        M_INR(I.HL.B.h);
                        break;
                case 0x25: I8085_ICount -= 4;   /* DCR  H */
                        M_DCR(I.HL.B.h);
                        break;
                case 0x26: I8085_ICount -= 7;   /* MVI  H,nn */
                        M_MVI(I.HL.B.h);
                        break;
                case 0x27: I8085_ICount -= 4;   /* DAA  */
                        I.XX.D = I.AF.B.h;
                        if (I.AF.B.l & CF) I.XX.D |= 0x100;
                        if (I.AF.B.l & HF) I.XX.D |= 0x200;
                        if (I.AF.B.l & NF) I.XX.D |= 0x400;
                        I.AF.W.l = DAA[I.XX.D];
                        break;
                case 0x28: I8085_ICount -= 7;   /* ???? */
                        break;
                case 0x29: I8085_ICount -= 11;  /* DAD  H */
                        M_DAD(HL);
                        break;
                case 0x2a: I8085_ICount -= 16;  /* LHLD nnnn */
                        I.XX.D = RDOPARG_WORD();
                        I.HL.B.l = RDMEM(I.XX.D);
                        I.XX.W.l++;
                        I.HL.B.h = RDMEM(I.XX.D);
                        break;
                case 0x2b: I8085_ICount -= 6;   /* DCX  H */
                        I.HL.W.l--;
                        break;
                case 0x2c: I8085_ICount -= 4;   /* INR  L */
                        M_INR(I.HL.B.l);
                        break;
                case 0x2d: I8085_ICount -= 4;   /* DCR  L */
                        M_DCR(I.HL.B.l);
                        break;
                case 0x2e: I8085_ICount -= 7;   /* MVI  L,nn */
                        M_MVI(I.HL.B.l);
                        break;
                case 0x2f: I8085_ICount -= 4;   /* CMA  */
                        I.AF.B.h ^= 0xff;
                        I.AF.B.l |= HF + NF;
                        break;

                case 0x30: I8085_ICount -= 7;   /* SIM  */
                        if ((I.IM ^ I.AF.B.h) & 0x80)
                        {
                                if (I.SOD_callback)
                                        (*I.SOD_callback)(I.AF.B.h >> 7);
                        }
                        I.IM &= (IM_SID + IM_IEN + IM_TRAP);
                        I.IM |= (I.AF.B.h & ~(IM_SID + IM_SOD + IM_IEN + IM_TRAP));
                        if (I.AF.B.h & 0x80)
                                I.IM |= IM_SOD;
                        break;
                case 0x31: I8085_ICount -= 10;  /* LXI SP,nnnn */
                        I.SP.W.l = RDOPARG_WORD();
                        break;
                case 0x32: I8085_ICount -= 13;  /* STAX nnnn */
                        I.XX.D = RDOPARG_WORD();
                        WRMEM(I.XX.D, I.AF.B.h);
                        break;
                case 0x33: I8085_ICount -= 6;   /* INX  SP */
                        I.SP.W.l++;
                        break;
                case 0x34: I8085_ICount -= 11;  /* INR  M */
                        I.XX.B.l = RDMEM(I.HL.D);
                        M_INR(I.XX.B.l);
                        WRMEM(I.HL.D, I.XX.B.l);
                        break;
                case 0x35: I8085_ICount -= 11;  /* DCR  M */
                        I.XX.B.l = RDMEM(I.HL.D);
                        M_DCR(I.XX.B.l);
                        WRMEM(I.HL.D, I.XX.B.l);
                        break;
                case 0x36: I8085_ICount -= 10;  /* MVI  M,nn */
                        I.XX.B.l = RDOPARG();
                        WRMEM(I.HL.D, I.XX.B.l);
                        break;
                case 0x37: I8085_ICount -= 4;   /* STC  */
                        I.AF.B.l = (I.AF.B.l & ~(HF + NF)) | CF;
                        break;

                case 0x38: I8085_ICount -= 7;   /* ???? */
                        break;
                case 0x39: I8085_ICount -= 11;  /* DAD SP */
                        M_DAD(SP);
                        break;
                case 0x3a: I8085_ICount -= 13;  /* LDAX nnnn */
                        I.XX.D = RDOPARG_WORD();
                        I.AF.B.h = RDMEM(I.XX.D);
                        break;
                case 0x3b: I8085_ICount -= 6;   /* DCX  SP */
                        I.SP.W.l--;
                        break;
                case 0x3c: I8085_ICount -= 4;   /* INR  A */
                        M_INR(I.AF.B.h);
                        break;
                case 0x3d: I8085_ICount -= 4;   /* DCR  A */
                        M_DCR(I.AF.B.h);
                        break;
                case 0x3e: I8085_ICount -= 7;   /* MVI  A,nn */
                        M_MVI(I.AF.B.h);
                        break;
                case 0x3f: I8085_ICount -= 4;   /* CMF  */
                        I.AF.B.l = ((I.AF.B.l & ~(HF + NF)) |
                                    ((I.AF.B.l & CF) << 4)) ^ CF;
                        break;

                case 0x40: I8085_ICount -= 4;   /* MOV  B,B */
                        /* no op */
                        break;
                case 0x41: I8085_ICount -= 4;   /* MOV  B,C */
                        I.BC.B.h = I.BC.B.l;
                        break;
                case 0x42: I8085_ICount -= 4;   /* MOV  B,D */
                        I.BC.B.h = I.DE.B.h;
                        break;
                case 0x43: I8085_ICount -= 4;   /* MOV  B,E */
                        I.BC.B.h = I.DE.B.l;
                        break;
                case 0x44: I8085_ICount -= 4;   /* MOV  B,H */
                        I.BC.B.h = I.HL.B.h;
                        break;
                case 0x45: I8085_ICount -= 4;   /* MOV  B,L */
                        I.BC.B.h = I.HL.B.l;
                        break;
                case 0x46: I8085_ICount -= 7;   /* MOV  B,M */
                        I.BC.B.h = RDMEM(I.HL.D);
                        break;
                case 0x47: I8085_ICount -= 4;   /* MOV  B,A */
                        I.BC.B.h = I.AF.B.h;
                        break;

                case 0x48: I8085_ICount -= 4;   /* MOV  C,B */
                        I.BC.B.l = I.BC.B.h;
                        break;
                case 0x49: I8085_ICount -= 4;   /* MOV  C,C */
                        /* no op */
                        break;
                case 0x4a: I8085_ICount -= 4;   /* MOV  C,D */
                        I.BC.B.l = I.DE.B.h;
                        break;
                case 0x4b: I8085_ICount -= 4;   /* MOV  C,E */
                        I.BC.B.l = I.DE.B.l;
                        break;
                case 0x4c: I8085_ICount -= 4;   /* MOV  C,H */
                        I.BC.B.l = I.HL.B.h;
                        break;
                case 0x4d: I8085_ICount -= 4;   /* MOV  C,L */
                        I.BC.B.l = I.HL.B.l;
                        break;
                case 0x4e: I8085_ICount -= 7;   /* MOV  C,M */
                        I.BC.B.l = RDMEM(I.HL.D);
                        break;
                case 0x4f: I8085_ICount -= 4;   /* MOV  C,A */
                        I.BC.B.l = I.AF.B.h;
                        break;

                case 0x50: I8085_ICount -= 4;   /* MOV  D,B */
                        I.DE.B.h = I.BC.B.h;
                        break;
                case 0x51: I8085_ICount -= 4;   /* MOV  D,C */
                        I.DE.B.h = I.BC.B.l;
                        break;
                case 0x52: I8085_ICount -= 4;   /* MOV  D,D */
                        /* no op */
                        break;
                case 0x53: I8085_ICount -= 4;   /* MOV  D,E */
                        I.DE.B.h = I.DE.B.l;
                        break;
                case 0x54: I8085_ICount -= 4;   /* MOV  D,H */
                        I.DE.B.h = I.HL.B.h;
                        break;
                case 0x55: I8085_ICount -= 4;   /* MOV  D,L */
                        I.DE.B.h = I.HL.B.l;
                        break;
                case 0x56: I8085_ICount -= 7;   /* MOV  D,M */
                        I.DE.B.h = RDMEM(I.HL.D);
                        break;
                case 0x57: I8085_ICount -= 4;   /* MOV  D,A */
                        I.DE.B.h = I.AF.B.h;
                        break;

                case 0x58: I8085_ICount -= 4;   /* MOV  E,B */
                        I.DE.B.l = I.BC.B.h;
                        break;
                case 0x59: I8085_ICount -= 4;   /* MOV  E,C */
                        I.DE.B.l = I.BC.B.l;
                        break;
                case 0x5a: I8085_ICount -= 4;   /* MOV  E,D */
                        I.DE.B.l = I.DE.B.h;
                        break;
                case 0x5b: I8085_ICount -= 4;   /* MOV  E,E */
                        /* no op */
                        break;
                case 0x5c: I8085_ICount -= 4;   /* MOV  E,H */
                        I.DE.B.l = I.HL.B.h;
                        break;
                case 0x5d: I8085_ICount -= 4;   /* MOV  E,L */
                        I.DE.B.l = I.HL.B.l;
                        break;
                case 0x5e: I8085_ICount -= 7;   /* MOV  E,M */
                        I.DE.B.l = RDMEM(I.HL.D);
                        break;
                case 0x5f: I8085_ICount -= 4;   /* MOV  E,A */
                        I.DE.B.l = I.AF.B.h;
                        break;

                case 0x60: I8085_ICount -= 4;   /* MOV  H,B */
                        I.HL.B.h = I.BC.B.h;
                        break;
                case 0x61: I8085_ICount -= 4;   /* MOV  H,C */
                        I.HL.B.h = I.BC.B.l;
                        break;
                case 0x62: I8085_ICount -= 4;   /* MOV  H,D */
                        I.HL.B.h = I.DE.B.h;
                        break;
                case 0x63: I8085_ICount -= 4;   /* MOV  H,E */
                        I.HL.B.h = I.DE.B.l;
                        break;
                case 0x64: I8085_ICount -= 4;   /* MOV  H,H */
                        /* no op */
                        break;
                case 0x65: I8085_ICount -= 4;   /* MOV  H,L */
                        I.HL.B.h = I.HL.B.l;
                        break;
                case 0x66: I8085_ICount -= 7;   /* MOV  H,M */
                        I.HL.B.h = RDMEM(I.HL.D);
                        break;
                case 0x67: I8085_ICount -= 4;   /* MOV  H,A */
                        I.HL.B.h = I.AF.B.h;
                        break;

                case 0x68: I8085_ICount -= 4;   /* MOV  L,B */
                        I.HL.B.l = I.BC.B.h;
                        break;
                case 0x69: I8085_ICount -= 4;   /* MOV  L,C */
                        I.HL.B.l = I.BC.B.l;
                        break;
                case 0x6a: I8085_ICount -= 4;   /* MOV  L,D */
                        I.HL.B.l = I.DE.B.h;
                        break;
                case 0x6b: I8085_ICount -= 4;   /* MOV  L,E */
                        I.HL.B.l = I.DE.B.l;
                        break;
                case 0x6c: I8085_ICount -= 4;   /* MOV  L,H */
                        I.HL.B.l = I.HL.B.h;
                        break;
                case 0x6d: I8085_ICount -= 4;   /* MOV  L,L */
                        /* no op */
                        break;
                case 0x6e: I8085_ICount -= 7;   /* MOV  L,M */
                        I.HL.B.l = RDMEM(I.HL.D);
                        break;
                case 0x6f: I8085_ICount -= 4;   /* MOV  L,A */
                        I.HL.B.l = I.AF.B.h;
                        break;

                case 0x70: I8085_ICount -= 7;   /* MOV  M,B */
                        WRMEM(I.HL.D, I.BC.B.h);
                        break;
                case 0x71: I8085_ICount -= 7;   /* MOV  M,C */
                        WRMEM(I.HL.D, I.BC.B.l);
                        break;
                case 0x72: I8085_ICount -= 7;   /* MOV  M,D */
                        WRMEM(I.HL.D, I.DE.B.h);
                        break;
                case 0x73: I8085_ICount -= 7;   /* MOV  M,E */
                        WRMEM(I.HL.D, I.DE.B.l);
                        break;
                case 0x74: I8085_ICount -= 7;   /* MOV  M,H */
                        WRMEM(I.HL.D, I.HL.B.h);
                        break;
                case 0x75: I8085_ICount -= 7;   /* MOV  M,L */
                        WRMEM(I.HL.D, I.HL.B.l);
                        break;
                case 0x76: I8085_ICount -= 4;   /* HALT */
                        I.PC.W.l--;
                        I.HALT = 1;
                        if (I8085_ICount > 0)
                                I8085_ICount = 0;
                        break;
                case 0x77: I8085_ICount -= 7;   /* MOV  M,A */
                        WRMEM(I.HL.D, I.AF.B.h);
                        break;

                case 0x78: I8085_ICount -= 4;   /* MOV  A,B */
                        I.AF.B.h = I.BC.B.h;
                        break;
                case 0x79: I8085_ICount -= 4;   /* MOV  A,C */
                        I.AF.B.h = I.BC.B.l;
                        break;
                case 0x7a: I8085_ICount -= 4;   /* MOV  A,D */
                        I.AF.B.h = I.DE.B.h;
                        break;
                case 0x7b: I8085_ICount -= 4;   /* MOV  A,E */
                        I.AF.B.h = I.DE.B.l;
                        break;
                case 0x7c: I8085_ICount -= 4;   /* MOV  A,H */
                        I.AF.B.h = I.HL.B.h;
                        break;
                case 0x7d: I8085_ICount -= 4;   /* MOV  A,L */
                        I.AF.B.h = I.HL.B.l;
                        break;
                case 0x7e: I8085_ICount -= 7;   /* MOV  A,M */
                        I.AF.B.h = RDMEM(I.HL.D);
                        break;
                case 0x7f: I8085_ICount -= 4;   /* MOV  A,A */
                        /* no op */
                        break;

                case 0x80: I8085_ICount -= 4;   /* ADD  B */
                        M_ADD(I.BC.B.h);
                        break;
                case 0x81: I8085_ICount -= 4;   /* ADD  C */
                        M_ADD(I.BC.B.l);
                        break;
                case 0x82: I8085_ICount -= 4;   /* ADD  D */
                        M_ADD(I.DE.B.h);
                        break;
                case 0x83: I8085_ICount -= 4;   /* ADD  E */
                        M_ADD(I.DE.B.l);
                        break;
                case 0x84: I8085_ICount -= 4;   /* ADD  H */
                        M_ADD(I.HL.B.h);
                        break;
                case 0x85: I8085_ICount -= 4;   /* ADD  L */
                        M_ADD(I.HL.B.l);
                        break;
                case 0x86: I8085_ICount -= 7;   /* ADD  M */
                        M_ADD(RDMEM(I.HL.D));
                        break;
                case 0x87: I8085_ICount -= 4;   /* ADD  A */
                        M_ADD(I.AF.B.h);
                        break;

                case 0x88: I8085_ICount -= 4;   /* ADC  B */
                        M_ADC(I.BC.B.h);
                        break;
                case 0x89: I8085_ICount -= 4;   /* ADC  C */
                        M_ADC(I.BC.B.l);
                        break;
                case 0x8a: I8085_ICount -= 4;   /* ADC  D */
                        M_ADC(I.DE.B.h);
                        break;
                case 0x8b: I8085_ICount -= 4;   /* ADC  E */
                        M_ADC(I.DE.B.l);
                        break;
                case 0x8c: I8085_ICount -= 4;   /* ADC  H */
                        M_ADC(I.HL.B.h);
                        break;
                case 0x8d: I8085_ICount -= 4;   /* ADC  L */
                        M_ADC(I.HL.B.l);
                        break;
                case 0x8e: I8085_ICount -= 7;   /* ADC  M */
                        M_ADC(RDMEM(I.HL.D));
                        break;
                case 0x8f: I8085_ICount -= 4;   /* ADC  A */
                        M_ADC(I.AF.B.h);
                        break;

                case 0x90: I8085_ICount -= 4;   /* SUB  B */
                        M_SUB(I.BC.B.h);
                        break;
                case 0x91: I8085_ICount -= 4;   /* SUB  C */
                        M_SUB(I.BC.B.l);
                        break;
                case 0x92: I8085_ICount -= 4;   /* SUB  D */
                        M_SUB(I.DE.B.h);
                        break;
                case 0x93: I8085_ICount -= 4;   /* SUB  E */
                        M_SUB(I.DE.B.l);
                        break;
                case 0x94: I8085_ICount -= 4;   /* SUB  H */
                        M_SUB(I.HL.B.h);
                        break;
                case 0x95: I8085_ICount -= 4;   /* SUB  L */
                        M_SUB(I.HL.B.l);
                        break;
                case 0x96: I8085_ICount -= 7;   /* SUB  M */
                        M_SUB(RDMEM(I.HL.D));
                        break;
                case 0x97: I8085_ICount -= 4;   /* SUB  A */
                        M_SUB(I.AF.B.h);
                        break;

                case 0x98: I8085_ICount -= 4;   /* SBB  B */
                        M_SBB(I.BC.B.h);
                        break;
                case 0x99: I8085_ICount -= 4;   /* SBB  C */
                        M_SBB(I.BC.B.l);
                        break;
                case 0x9a: I8085_ICount -= 4;   /* SBB  D */
                        M_SBB(I.DE.B.h);
                        break;
                case 0x9b: I8085_ICount -= 4;   /* SBB  E */
                        M_SBB(I.DE.B.l);
                        break;
                case 0x9c: I8085_ICount -= 4;   /* SBB  H */
                        M_SBB(I.HL.B.h);
                        break;
                case 0x9d: I8085_ICount -= 4;   /* SBB  L */
                        M_SBB(I.HL.B.l);
                        break;
                case 0x9e: I8085_ICount -= 7;   /* SBB  M */
                        M_SBB(RDMEM(I.HL.D));
                        break;
                case 0x9f: I8085_ICount -= 4;   /* SBB  A */
                        M_SBB(I.AF.B.h);
                        break;

                case 0xa0: I8085_ICount -= 4;   /* ANA  B */
                        M_ANA(I.BC.B.h);
                        break;
                case 0xa1: I8085_ICount -= 4;   /* ANA  C */
                        M_ANA(I.BC.B.l);
                        break;
                case 0xa2: I8085_ICount -= 4;   /* ANA  D */
                        M_ANA(I.DE.B.h);
                        break;
                case 0xa3: I8085_ICount -= 4;   /* ANA  E */
                        M_ANA(I.DE.B.l);
                        break;
                case 0xa4: I8085_ICount -= 4;   /* ANA  H */
                        M_ANA(I.HL.B.h);
                        break;
                case 0xa5: I8085_ICount -= 4;   /* ANA  L */
                        M_ANA(I.HL.B.l);
                        break;
                case 0xa6: I8085_ICount -= 7;   /* ANA  M */
                        M_ANA(RDMEM(I.HL.D));
                        break;
                case 0xa7: I8085_ICount -= 4;   /* ANA  A */
                        M_ANA(I.AF.B.h);
                        break;

                case 0xa8: I8085_ICount -= 4;   /* XRA  B */
                        M_XRA(I.BC.B.h);
                        break;
                case 0xa9: I8085_ICount -= 4;   /* XRA  C */
                        M_XRA(I.BC.B.l);
                        break;
                case 0xaa: I8085_ICount -= 4;   /* XRA  D */
                        M_XRA(I.DE.B.h);
                        break;
                case 0xab: I8085_ICount -= 4;   /* XRA  E */
                        M_XRA(I.DE.B.l);
                        break;
                case 0xac: I8085_ICount -= 4;   /* XRA  H */
                        M_XRA(I.HL.B.h);
                        break;
                case 0xad: I8085_ICount -= 4;   /* XRA  L */
                        M_XRA(I.HL.B.l);
                        break;
                case 0xae: I8085_ICount -= 7;   /* XRA  M */
                        M_XRA(RDMEM(I.HL.D));
                        break;
                case 0xaf: I8085_ICount -= 4;   /* XRA  A */
                        M_XRA(I.AF.B.h);
                        break;

                case 0xb0: I8085_ICount -= 4;   /* ORA  B */
                        M_ORA(I.BC.B.h);
                        break;
                case 0xb1: I8085_ICount -= 4;   /* ORA  C */
                        M_ORA(I.BC.B.l);
                        break;
                case 0xb2: I8085_ICount -= 4;   /* ORA  D */
                        M_ORA(I.DE.B.h);
                        break;
                case 0xb3: I8085_ICount -= 4;   /* ORA  E */
                        M_ORA(I.DE.B.l);
                        break;
                case 0xb4: I8085_ICount -= 4;   /* ORA  H */
                        M_ORA(I.HL.B.h);
                        break;
                case 0xb5: I8085_ICount -= 4;   /* ORA  L */
                        M_ORA(I.HL.B.l);
                        break;
                case 0xb6: I8085_ICount -= 7;   /* ORA  M */
                        M_ORA(RDMEM(I.HL.D));
                        break;
                case 0xb7: I8085_ICount -= 4;   /* ORA  A */
                        M_ORA(I.AF.B.h);
                        break;

                case 0xb8: I8085_ICount -= 4;   /* CMP  B */
                        M_CMP(I.BC.B.h);
                        break;
                case 0xb9: I8085_ICount -= 4;   /* CMP  C */
                        M_CMP(I.BC.B.l);
                        break;
                case 0xba: I8085_ICount -= 4;   /* CMP  D */
                        M_CMP(I.DE.B.h);
                        break;
                case 0xbb: I8085_ICount -= 4;   /* CMP  E */
                        M_CMP(I.DE.B.l);
                        break;
                case 0xbc: I8085_ICount -= 4;   /* CMP  H */
                        M_CMP(I.HL.B.h);
                        break;
                case 0xbd: I8085_ICount -= 4;   /* CMP  L */
                        M_CMP(I.HL.B.l);
                        break;
                case 0xbe: I8085_ICount -= 7;   /* CMP  M */
                        M_CMP(RDMEM(I.HL.D));
                        break;
                case 0xbf: I8085_ICount -= 4;   /* CMP  A */
                        M_CMP(I.AF.B.h);
                        break;

                case 0xc0: I8085_ICount -= 5;   /* RNZ  */
                        M_RET( !(I.AF.B.l & ZF) );
                        break;
                case 0xc1: I8085_ICount -= 10;  /* POP  B */
                        M_POP(BC);
                        break;
                case 0xc2: I8085_ICount -= 10;  /* JNZ  nnnn */
                        M_JMP( !(I.AF.B.l & ZF) );
                        break;
                case 0xc3: I8085_ICount -= 10;  /* JMP  nnnn */
                        M_JMP(1);
                        break;
                case 0xc4: I8085_ICount -= 10;  /* CNZ  nnnn */
                        M_CALL( !(I.AF.B.l & ZF) );
                        break;
                case 0xc5: I8085_ICount -= 11;  /* PUSH B */
                        M_PUSH(BC);
                        break;
                case 0xc6: I8085_ICount -= 7;   /* ADI  nn */
                        I.XX.B.l = RDOPARG();
                        M_ADD(I.XX.B.l);
                        break;
                case 0xc7: I8085_ICount -= 11;  /* RST  0 */
                        M_RST(0);
                        break;

                case 0xc8: I8085_ICount -= 5;   /* RZ   */
                        M_RET( I.AF.B.l & ZF );
                        break;
                case 0xc9: I8085_ICount -= 4;   /* RET  */
                        M_RET(1);
                        break;
                case 0xca: I8085_ICount -= 10;  /* JZ   nnnn */
                        M_JMP( I.AF.B.l & ZF );
                        break;
                case 0xcb: I8085_ICount -= 4;   /* ???? */
                        break;
                case 0xcc: I8085_ICount -= 10;  /* CZ   nnnn */
                        M_CALL( I.AF.B.l & ZF );
                        break;
                case 0xcd: I8085_ICount -= 10;  /* CALL nnnn */
                        M_CALL(1);
                        break;
                case 0xce: I8085_ICount -= 7;   /* ACI  nn */
                        I.XX.B.l = RDOPARG();
                        M_ADC(I.XX.B.l);
                        break;
                case 0xcf: I8085_ICount -= 11;  /* RST  1 */
                        M_RST(1);
                        break;

                case 0xd0: I8085_ICount -= 5;   /* RNC  */
                        M_RET( !(I.AF.B.l & CF) );
                        break;
                case 0xd1: I8085_ICount -= 10;  /* POP  D */
                        M_POP(DE);
                        break;
                case 0xd2: I8085_ICount -= 10;  /* JNC  nnnn */
                        M_JMP( !(I.AF.B.l & CF) );
                        break;
                case 0xd3: I8085_ICount -= 11;  /* OUT  nn */
                        M_OUT;
                        break;
                case 0xd4: I8085_ICount -= 10;  /* CNC  nnnn */
                        M_CALL( !(I.AF.B.l & CF) );
                        break;
                case 0xd5: I8085_ICount -= 11;  /* PUSH D */
                        M_PUSH(DE);
                        break;
                case 0xd6: I8085_ICount -= 7;   /* SUI  nn */
                        I.XX.B.l = RDOPARG();
                        M_SUB(I.XX.B.l);
                        break;
                case 0xd7: I8085_ICount -= 11;  /* RST  2 */
                        M_RST(2);
                        break;

                case 0xd8: I8085_ICount -= 5;   /* RC   */
                        M_RET( I.AF.B.l & CF );
                        break;
                case 0xd9: I8085_ICount -= 4;   /* ???? */
                        break;
                case 0xda: I8085_ICount -= 10;  /* JC   nnnn */
                        M_JMP( I.AF.B.l & CF );
                        break;
                case 0xdb: I8085_ICount -= 11;  /* IN   nn */
                        M_IN;
                        break;
                case 0xdc: I8085_ICount -= 10;  /* CC   nnnn */
                        M_CALL( I.AF.B.l & CF );
                        break;
                case 0xdd: I8085_ICount -= 4;   /* ???? */
                        break;
                case 0xde: I8085_ICount -= 7;   /* SBI  nn */
                        I.XX.B.l = RDOPARG();
                        M_SBB(I.XX.B.l);
                        break;
                case 0xdf: I8085_ICount -= 11;  /* RST  3 */
                        M_RST(3);
                        break;

                case 0xe0: I8085_ICount -= 5;   /* RPE    */
                        M_RET( !(I.AF.B.l & VF) );
                        break;
                case 0xe1: I8085_ICount -= 10;  /* POP  H */
                        M_POP(HL);
                        break;
                case 0xe2: I8085_ICount -= 10;  /* JPE  nnnn */
                        M_JMP( !(I.AF.B.l & VF) );
                        break;
                case 0xe3: I8085_ICount -= 19;  /* XTHL */
                        M_POP(XX);
                        M_PUSH(HL);
                        I.HL.D = I.XX.D;
                        break;
                case 0xe4: I8085_ICount -= 10;  /* CPE  nnnn */
                        M_CALL( !(I.AF.B.l & VF) );
                        break;
                case 0xe5: I8085_ICount -= 11;  /* PUSH H */
                        M_PUSH(HL);
                        break;
                case 0xe6: I8085_ICount -= 7;   /* ANI  nn */
                        I.XX.B.l = RDOPARG();
                        M_ANA(I.XX.B.l);
                        break;
                case 0xe7: I8085_ICount -= 11;  /* RST  4 */
                        M_RST(4);
                        break;

                case 0xe8: I8085_ICount -= 5;   /* RPO  */
                        M_RET( I.AF.B.l & VF );
                        break;
                case 0xe9: I8085_ICount -= 4;   /* PCHL */
                        I.PC.D = I.HL.W.l;
                        change_pc16(I.PC.D);
                        break;
                case 0xea: I8085_ICount -= 10;  /* JPO  nnnn */
                        M_JMP( I.AF.B.l & VF );
                        break;
                case 0xeb: I8085_ICount -= 4;   /* XCHG */
                        I.XX.D = I.DE.D;
                        I.DE.D = I.HL.D;
                        I.HL.D = I.XX.D;
                        break;
                case 0xec: I8085_ICount -= 10;  /* CPO  nnnn */
                        M_CALL( I.AF.B.l & VF );
                        break;
                case 0xed: I8085_ICount -= 4;   /* ???? */
                        break;
                case 0xee: I8085_ICount -= 7;   /* XRI  nn */
                        I.XX.B.l = RDOPARG();
                        M_XRA(I.XX.B.l);
                        break;
                case 0xef: I8085_ICount -= 11;  /* RST  5 */
                        M_RST(5);
                        break;

                case 0xf0: I8085_ICount -= 5;   /* RP   */
                        M_RET( !(I.AF.B.l&SF) );
                        break;
                case 0xf1: I8085_ICount -= 10;  /* POP  A */
                        M_POP(AF);
                        break;
                case 0xf2: I8085_ICount -= 10;  /* JP   nnnn */
                        M_JMP( !(I.AF.B.l & SF) );
                        break;
                case 0xf3: I8085_ICount -= 4;   /* DI   */
                        /* remove interrupt enable */
                        I.IM &= ~IM_IEN;
                        break;
                case 0xf4: I8085_ICount -= 10;  /* CP   nnnn */
                        M_CALL( !(I.AF.B.l & SF) );
                        break;
                case 0xf5: I8085_ICount -= 11;  /* PUSH A */
                        M_PUSH(AF);
                        break;
                case 0xf6: I8085_ICount -= 7;   /* ORI  nn */
                        I.XX.B.l = RDOPARG();
                        M_ORA(I.XX.B.l);
                        break;
                case 0xf7: I8085_ICount -= 11;  /* RST  6 */
                        M_RST(6);
                        break;

                case 0xf8: I8085_ICount -= 5;   /* RM   */
                        M_RET( I.AF.B.l & SF );
                        break;
                case 0xf9: I8085_ICount -= 6;   /* SPHL */
                        I.SP.D = I.HL.D;
                        break;
                case 0xfa: I8085_ICount -= 10;  /* JM   nnnn */
                        M_JMP( I.AF.B.l & SF );
                        break;
                case 0xfb: I8085_ICount -= 4;   /* EI */
                        /* set interrupt enable */
                        I.IM |= IM_IEN;
                        /* remove serviced IRQ flag */
                        I.IREQ &= ~I.ISRV;
                        /* reset serviced IRQ */
                        I.ISRV = 0;
                        /* find highest priority IREQ flag with
                          IM enabled and schedule for execution */
                        if (!(I.IM & IM_RST75) && (I.IREQ & IM_RST75))
                        {
                                I.ISRV = IM_RST75;
                                I.IRQ2 = ADDR_RST75;
                        }
                        else
                        if (!(I.IM & IM_RST65) && (I.IREQ & IM_RST65))
                        {
                                I.ISRV = IM_RST65;
                                I.IRQ2 = ADDR_RST65;
                        }
                        else
                        if (!(I.IM & IM_RST55) && (I.IREQ & IM_RST55))
                        {
                                I.ISRV = IM_RST55;
                                I.IRQ2 = ADDR_RST55;
                        }
                        else
                        if (!(I.IM & IM_INTR) && (I.IREQ & IM_INTR))
                        {
                                I.ISRV = IM_INTR;
                                I.IRQ2 = I.INTR;
                        }
                        break;
                case 0xfc: I8085_ICount -= 10;  /* CM   nnnn */
                        M_CALL( I.AF.B.l & SF );
                        break;
                case 0xfd: I8085_ICount -= 4;   /* ???? */
                        break;
                case 0xfe: I8085_ICount -= 7;   /* CPI  nn */
                        I.XX.B.l = RDOPARG();
                        M_CMP(I.XX.B.l);
                        break;
                case 0xff: I8085_ICount -= 11;  /* RST  7 */
                        M_RST(7);
                        break;
        }
}

INLINE  void    Interrupt(void)
{
        /* if the CPU was halted */
        if (I.HALT)
        {
                I.PC.W.l++;     /* skip HALT instr */
                I.HALT = 0;
        }
        I.IM &= ~IM_IEN;        /* remove general interrupt enable bit */
        switch (I.IRQ1 & 0xff0000)
        {
                case 0xcd0000:  /* CALL nnnn */
                        I8085_ICount -= 7;
                        M_PUSH(PC);
                case 0xc30000:  /* JMP  nnnn */
                        I8085_ICount -= 10;
                        I.PC.D = I.IRQ1 & 0xffff;
                        change_pc16(I.PC.D);
                        break;
                default:
                        switch (I.IRQ1)
                        {
                                case I8085_TRAP:
                                case I8085_RST75:
                                case I8085_RST65:
                                case I8085_RST55:
                                        M_PUSH(PC);
                                        I.PC.D = I.IRQ1;
                                        change_pc16(I.PC.D);
                                        break;
                                default:
                                        ExecOne(I.IRQ1 & 0xff);
                        }

        }
}

int     I8085_Execute(int cycles)
{

        I8085_ICount = cycles;
        do
        {
                /* interrupts enabled or TRAP pending ? */
                if ( (I.IM & IM_IEN) || (I.IREQ & IM_TRAP) )
                {
                        /* copy scheduled to executed interrupt request */
                        I.IRQ1 = I.IRQ2;
                        /* reset scheduled interrupt request */
                        I.IRQ2 = 0;
                        /* interrupt now ? */
                        if (I.IRQ1) Interrupt();
                }

                /* here we go... */
                ExecOne(RDOP());

        } while (I8085_ICount > 0);

        return cycles - I8085_ICount;
}

/****************************************************************************/
/* Initialise the various lookup tables used by the emulation code          */
/****************************************************************************/
static void InitTables (void)
{
byte    zs;
int     i, p;
        for (i = 0; i < 256; i++)
        {
                zs = 0;
                if (i==0) zs |= ZF;
                if (i&128) zs |= SF;
                p = 0;
                if (i&1) ++p;
                if (i&2) ++p;
                if (i&4) ++p;
                if (i&8) ++p;
                if (i&16) ++p;
                if (i&32) ++p;
                if (i&64) ++p;
                if (i&128) ++p;
                ZS[i] = zs;
                ZSP[i] = zs | ((p&1) ? 0 : VF);
        }
}

/****************************************************************************/
/* Reset the 8085 emulation                                                 */
/****************************************************************************/
void    I8085_Reset(void)
{
        InitTables();
        memset(&I, 0, sizeof(I8085_Regs));
        change_pc16(I.PC.D);
}

/****************************************************************************/
/* Get the current 8085 context                                             */
/****************************************************************************/
void    I8085_GetRegs(I8085_Regs * regs)
{
        *regs = I;
}

/****************************************************************************/
/* Set the current 8085 context                                             */
/****************************************************************************/
void    I8085_SetRegs(I8085_Regs * regs)
{
        I = *regs;
}

/****************************************************************************/
/* Get the current 8085 PC                                                  */
/****************************************************************************/
int     I8085_GetPC(void)
{
        return I.PC.D;
}

/****************************************************************************/
/* Set the 8085 SID input signal state                                      */
/****************************************************************************/
void    I8085_SetSID(int state)
{
        if (state)
                I.IM |= IM_SID;
        else
                I.IM &= ~IM_SID;
}

/****************************************************************************/
/* Set a callback to be called at SOD output change                         */
/****************************************************************************/
void    I8085_Set_SOD_callback(void (*callback)(int state))
{
        I.SOD_callback = callback;
}

/****************************************************************************/
/* Set TRAP signal state                                                    */
/****************************************************************************/
void    I8085_SetTRAP(int state)
{
        if (state)
        {
                I.IREQ |= IM_TRAP;
                /* already servicing TRAP ? */
                if (I.ISRV & IM_TRAP)
                        return;
                /* schedule TRAP */
                I.ISRV = IM_TRAP;
                I.IRQ2 = ADDR_TRAP;
        }
        else
        {
                /* remove request for TRAP */
                I.IREQ &= ~IM_TRAP;
        }
}

/****************************************************************************/
/* Set RST7.5 signal state                                                  */
/****************************************************************************/
void    I8085_SetRST75(int state)
{
        if (state)
        {
                /* request RST7.5 */
                I.IREQ |= IM_RST75;
                /* if masked, ignore it for now */
                if (I.IM & IM_RST75)
                        return;
                /* if no higher priority IREQ is serviced */
                if (!I.ISRV)
                {
                        /* schedule RST7.5 */
                        I.ISRV = IM_RST75;
                        I.IRQ2 = ADDR_RST75;
                }
        }
        /* RST7.5 is reset only by SIM or end of service routine ! */
}

/****************************************************************************/
/* Set RST6.5 signal state                                                  */
/****************************************************************************/
void    I8085_SetRST65(int state)
{
        if (state)
        {
                /* request RST6.5 */
                I.IREQ |= IM_RST65;
                /* if masked, ignore it for now */
                if (I.IM & IM_RST65)
                        return;
                /* if no higher priority IREQ is serviced */
                if (!I.ISRV)
                {
                        /* schedule RST6.5 */
                        I.ISRV = IM_RST65;
                        I.IRQ2 = ADDR_RST65;
                }
        }
        else
        {
                /* remove request for RST6.5 */
                I.IREQ &= ~IM_RST65;
        }
}

/****************************************************************************/
/* Set RST5.5 signal state                                                  */
/****************************************************************************/
void    I8085_SetRST55(int state)
{
        if (state)
        {
                /* request RST5.5 */
                I.IREQ |= IM_RST55;
                /* if masked, ignore it for now */
                if (I.IM & IM_RST55)
                        return;
                /* if no higher priority IREQ is serviced */
                if (!I.ISRV)
                {
                        /* schedule RST5.5 */
                        I.ISRV = IM_RST55;
                        I.IRQ2 = ADDR_RST55;
                }
        }
        else
        {
                /* remove request for RST5.5 */
                I.IREQ &= ~IM_RST55;
        }
}

/****************************************************************************/
/* Set INTR signal                                                          */
/****************************************************************************/
void    I8085_SetINTR(int type)
{
        if (type)
        {
                /* request INTR */
                I.IREQ |= IM_INTR;
                I.INTR = type;
                /* if masked, ignore it for now */
                if (I.IM & IM_INTR)
                        return;
                /* if no higher priority IREQ is serviced */
                if (!I.ISRV)
                {
                        /* schedule INTR */
                        I.ISRV = IM_INTR;
                        I.IRQ2 = I.INTR;
                }
        }
        else
        {
                /* remove request for INTR */
                I.IREQ &= ~IM_INTR;
        }
}

/****************************************************************************/
/* Cause an interrupt                                                       */
/****************************************************************************/
void    I8085_Cause_Interrupt(int type)
{
        switch (type)
        {
                case I8085_SID:
                        I8085_SetSID(1);
                        break;
                case I8085_TRAP:
                        I8085_SetTRAP(1);
                        break;
                case I8085_RST75:
                        I8085_SetRST75(1);
                        break;
                case I8085_RST65:
                        I8085_SetRST65(1);
                        break;
                case I8085_RST55:
                        I8085_SetRST55(1);
                        break;
                default:
                        I8085_SetINTR(type);
                        break;
        }
}

void    I8085_Clear_Pending_Interrupts(void)
{
        if (I.IREQ & IM_TRAP)
                I8085_SetTRAP(I8085_NONE);
        if (I.IREQ & IM_RST75)
                I8085_SetRST75(I8085_NONE);
        if (I.IREQ & IM_RST65)
                I8085_SetRST65(I8085_NONE);
        if (I.IREQ & IM_RST55)
                I8085_SetRST55(I8085_NONE);
        if (I.IREQ & IM_INTR)
                I8085_SetINTR(I8085_NONE);
}


