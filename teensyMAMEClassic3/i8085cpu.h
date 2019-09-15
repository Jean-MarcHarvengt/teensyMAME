/*******************************************************
 *
 *      Portable (hopefully ;-) 8085A emulator
 *
 *      Written by J. Buchmueller for use with MAME
 *
 *      Partially based on Z80Em from M. De Kogel
 *
 *      CPU related macros
 *
 *******************************************************/

#define SF              0x80
#define ZF              0x40
#define YF              0x20
#define HF              0x10
#define XF              0x08
#define VF              0x04
#define NF              0x02
#define CF              0x01

#define IM_SID          0x80
#define IM_SOD          0x40
#define IM_IEN          0x20
#define IM_TRAP         0x10
#define IM_INTR         0x08
#define IM_RST75        0x04
#define IM_RST65        0x02
#define IM_RST55        0x01

#define ADDR_TRAP       0x0024
#define ADDR_RST55      0x002c
#define ADDR_RST65      0x0034
#define ADDR_RST75      0x003c
#define ADDR_INTR       0x0038

#define M_INR(R) ++R; I.AF.B.l=(I.AF.B.l&CF)|ZS[R]|((R==0x80)?VF:0)|((R&0x0F)?0:HF)
#define M_DCR(R) I.AF.B.l=(I.AF.B.l&CF)|NF|((R==0x80)?VF:0)|((R&0x0F)?0:HF); I.AF.B.l|=ZS[--R]
#define M_MVI(R) R=RDOPARG()

#define M_ANA(R) I.AF.B.h&=R; I.AF.B.l=ZSP[I.AF.B.h]|HF
#define M_ORA(R) I.AF.B.h|=R; I.AF.B.l=ZSP[I.AF.B.h]
#define M_XRA(R) I.AF.B.h^=R; I.AF.B.l=ZSP[I.AF.B.h]

#define M_RLC { \
        I.AF.B.h = (I.AF.B.h << 1) | (I.AF.B.h >> 7); \
        I.AF.B.l = (I.AF.B.l & ~(HF+NF+CF)) | (I.AF.B.h & CF); \
}
#define M_RRC { \
        I.AF.B.l = (I.AF.B.l & ~(HF+NF+CF)) | (I.AF.B.h & CF); \
        I.AF.B.h = (I.AF.B.h >> 1) | (I.AF.B.h << 7); \
}
#define M_RAL { \
int c = I.AF.B.l&CF; \
        I.AF.B.l = (I.AF.B.l & ~(HF+NF+CF)) | (I.AF.B.h >> 7); \
        I.AF.B.h = (I.AF.B.h << 1) | c; \
}
#define M_RAR { \
int c = (I.AF.B.l&CF) << 7; \
        I.AF.B.l = (I.AF.B.l & ~(HF+NF+CF)) | (I.AF.B.h & CF); \
        I.AF.B.h = (I.AF.B.h >> 1) | c; \
}

#define M_ADD(R) { \
int q = I.AF.B.h+R; \
        I.AF.B.l=ZS[q&255]|((q>>8)&CF)| \
          ((I.AF.B.h^q^R)&HF)| \
          (((R^I.AF.B.h^SF)&(R^q)&SF)>>5);  \
        I.AF.B.h=q; \
}
#define M_ADC(R) { \
int q = I.AF.B.h+R+(I.AF.B.l&CF); \
        I.AF.B.l=ZS[q&255]|((q>>8)&CF)| \
          ((I.AF.B.h^q^R)&HF)| \
          (((R^I.AF.B.h^SF)&(R^q)&SF)>>5);  \
        I.AF.B.h=q; \
}
#define M_SUB(R) { \
int q = I.AF.B.h-R; \
        I.AF.B.l=ZS[q&255]|((q>>8)&CF)|NF| \
          ((I.AF.B.h^q^R)&HF)| \
          (((R^I.AF.B.h)&(I.AF.B.h^q)&SF)>>5); \
        I.AF.B.h=q; \
}
#define M_SBB(R) { \
int q = I.AF.B.h-R-(I.AF.B.l&CF); \
        I.AF.B.l=ZS[q&255]|((q>>8)&CF)|NF| \
          ((I.AF.B.h^q^R)&HF)| \
          (((R^I.AF.B.h)&(I.AF.B.h^q)&SF)>>5); \
        I.AF.B.h=q; \
}
#define M_CMP(R) { \
int q = I.AF.B.h-R; \
        I.AF.B.l=ZS[q&255]|((q>>8)&CF)|NF| \
          ((I.AF.B.h^q^R)&HF)| \
          (((R^I.AF.B.h)&(I.AF.B.h^q)&SF)>>5); \
}

#define M_IN    I.XX.D=RDOPARG(); I.AF.B.h=cpu_readport(I.XX.D); I.AF.B.l=(I.AF.B.l&CF)|ZSP[I.AF.B.h]
#define M_OUT   I.XX.D=RDOPARG(); cpu_writeport(I.XX.D,I.AF.B.h)

#define M_DAD(R) { \
int q = I.HL.D + I.R.D; \
        I.AF.B.l = ( I.AF.B.l & ~(HF+CF) ) | \
          ( ((I.HL.D^q^I.R.D) >> 8) & HF ) | \
          ( (q>>16) & CF ); \
        I.HL.W.l = q; \
}

#define M_PUSH(R) { \
        WRMEM(--I.SP.W.l, I.R.B.h); \
        WRMEM(--I.SP.W.l, I.R.B.l); \
}

#define M_POP(R) { \
        I.R.B.l = RDMEM(I.SP.W.l++); \
        I.R.B.h = RDMEM(I.SP.W.l++); \
}

#define M_RET(cc) { \
        if (cc) { \
                I8085_ICount -= 6; \
                M_POP(PC); \
                change_pc16(I.PC.D); \
        } \
}

#define M_JMP(cc) { \
        if (cc) { \
                I.PC.W.l = RDOPARG_WORD(); \
                change_pc16(I.PC.D); \
        } else I.PC.W.l += 2; \
}

#define M_CALL(cc) { \
        if (cc) { \
                word a = RDOPARG_WORD(); \
                I8085_ICount -= 7; \
                M_PUSH(PC); \
                I.PC.D = a; \
                change_pc16(I.PC.D); \
        } else I.PC.W.l += 2; \
}

#define M_RST(nn) { \
        M_PUSH(PC); \
        I.PC.D = 8 * nn; \
        change_pc16(I.PC.D); \
}


