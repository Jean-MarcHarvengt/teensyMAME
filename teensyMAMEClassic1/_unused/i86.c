/****************************************************************************
*             real mode i286 emulator v1.4 by Fabrice Frances               *
*               (initial work based on David Hedley's pcemu)                *
****************************************************************************/

#include <stdio.h>
#include "host.h"
#include "i86.h"
#include "i86intrf.h"
#include "../src/memory.h"

/***************************************************************************/
/* cpu state                                                               */
/***************************************************************************/

static i86basicregs regs;       /* ASG 971222 */

int i86_ICount;
#define cycle_count i86_ICount

static unsigned ip;     /* instruction pointer register */
static unsigned sregs[4];       /* four segment registers */
static unsigned base[4];        /* and their base addresses */
static unsigned prefix_base;    /* base address of the latest prefix segment */
static char seg_prefix;         /* prefix segment indicator */

static BYTE TF, IF, DF; /* 0 or 1 valued flags */
static int AuxVal, OverVal, SignVal, ZeroVal, CarryVal, ParityVal; /* 0 or non-0 valued flags */

int int86_pending;   /* The interrupt number of a pending external interrupt pending */
	/* NMI is number 2. For INTR interrupts, the level is caught on the bus during an INTA cycle */

/* ASG 971222 static unsigned char *Memory;   * fetching goes directly to memory instead of going through MAME's cpu_readmem() */

/***************************************************************************/
/* ASG 971222 static int cycle_count, cycles_per_run; */

#include "instr.h"
#include "ea.h"
#include "modrm.h"

static UINT8 parity_table[256];
/***************************************************************************/


/* ASG 971222 void I86_Reset(unsigned char *mem,int cycles)*/
void i86_Reset (void)
{
    unsigned int i,j,c;
    BREGS reg_name[8]={ AL, CL, DL, BL, AH, CH, DH, BH };

    /* ASG 971222 cycles_per_run=cycles;*/
    int86_pending=0;
    for (i = 0; i < 4; i++) sregs[i] = 0;
    sregs[CS]=0xFFFF;

    base[CS] = SegBase(CS);
    base[DS] = SegBase(DS);
    base[ES] = SegBase(ES);
    base[SS] = SegBase(SS);

    for (i=0; i < 8; i++) regs.w[i] = 0;
    ip = 0;

    /* ASG 971222 Memory=mem;*/

    for (i = 0;i < 256; i++)
    {
	for (j = i, c = 0; j > 0; j >>= 1)
	    if (j & 1) c++;

	parity_table[i] = !(c & 1);
    }

    TF = IF = DF = 0;
    SignVal=CarryVal=AuxVal=OverVal=0;
    ZeroVal=ParityVal=1;

    for (i = 0; i < 256; i++)
    {
	Mod_RM.reg.b[i] = reg_name[(i & 0x38) >> 3];
	Mod_RM.reg.w[i] = (WREGS) ( (i & 0x38) >> 3) ;
    }

    for (i = 0xc0; i < 0x100; i++)
    {
	Mod_RM.RM.w[i] = (WREGS)( i & 7 );
	Mod_RM.RM.b[i] = (BREGS)reg_name[i & 7];
    }
}


static void I86_interrupt(unsigned int_num)
{
    unsigned dest_seg, dest_off;

    i_pushf();
    dest_off = ReadWord(int_num*4);
    dest_seg = ReadWord(int_num*4+2);

    PUSH(sregs[CS]);
    PUSH(ip);
    ip = (WORD)dest_off;
    sregs[CS] = (WORD)dest_seg;
    base[CS] = SegBase(CS);

    TF = IF = 0;

}

void trap(void)
{
    instruction[FETCH]();
    I86_interrupt(1);
}

static void external_int(void)
{
    I86_interrupt(int86_pending);
    int86_pending = 0;
}

/****************************************************************************/

static void i_add_br8(void)    /* Opcode 0x00 */
{
    DEF_br8(dst,src);
    cycle_count-=3;
    ADDB(dst,src);
    PutbackRMByte(ModRM,dst);
}

static void i_add_wr16(void)    /* Opcode 0x01 */
{
    DEF_wr16(dst,src);
    cycle_count-=3;
    ADDW(dst,src);
    PutbackRMWord(ModRM,dst);
}

static void i_add_r8b(void)    /* Opcode 0x02 */
{
    DEF_r8b(dst,src);
    cycle_count-=3;
    ADDB(dst,src);
    RegByte(ModRM)=dst;
}

static void i_add_r16w(void)    /* Opcode 0x03 */
{
    DEF_r16w(dst,src);
    cycle_count-=3;
    ADDW(dst,src);
    RegWord(ModRM)=dst;
}


static void i_add_ald8(void)    /* Opcode 0x04 */
{
    DEF_ald8(dst,src);
    cycle_count-=4;
    ADDB(dst,src);
    regs.b[AL]=dst;
}


static void i_add_axd16(void)    /* Opcode 0x05 */
{
    DEF_axd16(dst,src);
    cycle_count-=4;
    ADDW(dst,src);
    regs.w[AX]=dst;
}


static void i_push_es(void)    /* Opcode 0x06 */
{
    cycle_count-=3;
    PUSH(sregs[ES]);
}


static void i_pop_es(void)    /* Opcode 0x07 */
{
    POP(sregs[ES]);
    base[ES] = SegBase(ES);
    cycle_count-=2;
}

static void i_or_br8(void)    /* Opcode 0x08 */
{
    DEF_br8(dst,src);
    cycle_count-=3;
    ORB(dst,src);
    PutbackRMByte(ModRM,dst);
}

static void i_or_wr16(void)    /* Opcode 0x09 */
{
    DEF_wr16(dst,src);
    cycle_count-=3;
    ORW(dst,src);
    PutbackRMWord(ModRM,dst);
}

static void i_or_r8b(void)    /* Opcode 0x0a */
{
    DEF_r8b(dst,src);
    cycle_count-=3;
    ORB(dst,src);
    RegByte(ModRM)=dst;
}

static void i_or_r16w(void)    /* Opcode 0x0b */
{
    DEF_r16w(dst,src);
    cycle_count-=3;
    ORW(dst,src);
    RegWord(ModRM)=dst;
}

static void i_or_ald8(void)    /* Opcode 0x0c */
{
    DEF_ald8(dst,src);
    cycle_count-=4;
    ORB(dst,src);
    regs.b[AL]=dst;
}

static void i_or_axd16(void)    /* Opcode 0x0d */
{
    DEF_axd16(dst,src);
    cycle_count-=4;
    ORW(dst,src);
    regs.w[AX]=dst;
}

static void i_push_cs(void)    /* Opcode 0x0e */
{
    cycle_count-=3;
    PUSH(sregs[CS]);
}

/* Opcode 0x0f invalid */

static void i_adc_br8(void)    /* Opcode 0x10 */
{
    DEF_br8(dst,src);
    cycle_count-=3;
    src+=CF;
    ADDB(dst,src);
    PutbackRMByte(ModRM,dst);
}

static void i_adc_wr16(void)    /* Opcode 0x11 */
{
    DEF_wr16(dst,src);
    cycle_count-=3;
    src+=CF;
    ADDW(dst,src);
    PutbackRMWord(ModRM,dst);
}

static void i_adc_r8b(void)    /* Opcode 0x12 */
{
    DEF_r8b(dst,src);
    cycle_count-=3;
    src+=CF;
    ADDB(dst,src);
    RegByte(ModRM)=dst;
}

static void i_adc_r16w(void)    /* Opcode 0x13 */
{
    DEF_r16w(dst,src);
    cycle_count-=3;
    src+=CF;
    ADDW(dst,src);
    RegWord(ModRM)=dst;
}

static void i_adc_ald8(void)    /* Opcode 0x14 */
{
    DEF_ald8(dst,src);
    cycle_count-=4;
    src+=CF;
    ADDB(dst,src);
    regs.b[AL] = dst;
}

static void i_adc_axd16(void)    /* Opcode 0x15 */
{
    DEF_axd16(dst,src);
    cycle_count-=4;
    src+=CF;
    ADDW(dst,src);
    regs.w[AX]=dst;
}

static void i_push_ss(void)    /* Opcode 0x16 */
{
    cycle_count-=3;
    PUSH(sregs[SS]);
}

static void i_pop_ss(void)    /* Opcode 0x17 */
{
    cycle_count-=2;
    POP(sregs[SS]);
    base[SS] = SegBase(SS);
    instruction[FETCH](); /* no interrupt before next instruction */
}

static void i_sbb_br8(void)    /* Opcode 0x18 */
{
    DEF_br8(dst,src);
    cycle_count-=3;
    src+=CF;
    SUBB(dst,src);
    PutbackRMByte(ModRM,dst);
}

static void i_sbb_wr16(void)    /* Opcode 0x19 */
{
    DEF_wr16(dst,src);
    cycle_count-=3;
    src+=CF;
    SUBW(dst,src);
    PutbackRMWord(ModRM,dst);
}

static void i_sbb_r8b(void)    /* Opcode 0x1a */
{
    DEF_r8b(dst,src);
    cycle_count-=3;
    src+=CF;
    SUBB(dst,src);
    RegByte(ModRM)=dst;
}

static void i_sbb_r16w(void)    /* Opcode 0x1b */
{
    DEF_r16w(dst,src);
    cycle_count-=3;
    src+=CF;
    SUBW(dst,src);
    RegWord(ModRM)= dst;
}

static void i_sbb_ald8(void)    /* Opcode 0x1c */
{
    DEF_ald8(dst,src);
    cycle_count-=4;
    src+=CF;
    SUBB(dst,src);
    regs.b[AL] = dst;
}

static void i_sbb_axd16(void)    /* Opcode 0x1d */
{
    DEF_axd16(dst,src);
    cycle_count-=4;
    src+=CF;
    SUBW(dst,src);
    regs.w[AX]=dst;
}

static void i_push_ds(void)    /* Opcode 0x1e */
{
    cycle_count-=3;
    PUSH(sregs[DS]);
}

static void i_pop_ds(void)    /* Opcode 0x1f */
{
    POP(sregs[DS]);
    base[DS] = SegBase(DS);
    cycle_count-=2;
}

static void i_and_br8(void)    /* Opcode 0x20 */
{
    DEF_br8(dst,src);
    cycle_count-=3;
    ANDB(dst,src);
    PutbackRMByte(ModRM,dst);
}

static void i_and_wr16(void)    /* Opcode 0x21 */
{
    DEF_wr16(dst,src);
    cycle_count-=3;
    ANDW(dst,src);
    PutbackRMWord(ModRM,dst);
}

static void i_and_r8b(void)    /* Opcode 0x22 */
{
    DEF_r8b(dst,src);
    cycle_count-=3;
    ANDB(dst,src);
    RegByte(ModRM)=dst;
}

static void i_and_r16w(void)    /* Opcode 0x23 */
{
    DEF_r16w(dst,src);
    cycle_count-=3;
    ANDW(dst,src);
    RegWord(ModRM)=dst;
}

static void i_and_ald8(void)    /* Opcode 0x24 */
{
    DEF_ald8(dst,src);
    cycle_count-=4;
    ANDB(dst,src);
    regs.b[AL] = dst;
}

static void i_and_axd16(void)    /* Opcode 0x25 */
{
    DEF_axd16(dst,src);
    cycle_count-=4;
    ANDW(dst,src);
    regs.w[AX]=dst;
}

static void i_es(void)    /* Opcode 0x26 */
{
    seg_prefix=TRUE;
    prefix_base=base[ES];
    cycle_count-=2;
    instruction[FETCH]();
}

static void i_daa(void)    /* Opcode 0x27 */
{
	if (AF || ((regs.b[AL] & 0xf) > 9))
	{
		int tmp;
		regs.b[AL] = tmp = regs.b[AL] + 6;
		AuxVal = 1;
		CarryVal |= tmp & 0x100;
	}

	if (CF || (regs.b[AL] > 0x9f))
	{
		regs.b[AL] += 0x60;
		CarryVal = 1;
	}

	SetSZPF_Byte(regs.b[AL]);
	cycle_count-=4;
}

static void i_sub_br8(void)    /* Opcode 0x28 */
{
    DEF_br8(dst,src);
    cycle_count-=3;
    SUBB(dst,src);
    PutbackRMByte(ModRM,dst);
}

static void i_sub_wr16(void)    /* Opcode 0x29 */
{
    DEF_wr16(dst,src);
    cycle_count-=3;
    SUBW(dst,src);
    PutbackRMWord(ModRM,dst);
}

static void i_sub_r8b(void)    /* Opcode 0x2a */
{
    DEF_r8b(dst,src);
    cycle_count-=3;
    SUBB(dst,src);
    RegByte(ModRM)=dst;
}

static void i_sub_r16w(void)    /* Opcode 0x2b */
{
    DEF_r16w(dst,src);
    cycle_count-=3;
    SUBW(dst,src);
    RegWord(ModRM)=dst;
}

static void i_sub_ald8(void)    /* Opcode 0x2c */
{
    DEF_ald8(dst,src);
    cycle_count-=4;
    SUBB(dst,src);
    regs.b[AL] = dst;
}

static void i_sub_axd16(void)    /* Opcode 0x2d */
{
    DEF_axd16(dst,src);
    cycle_count-=4;
    SUBW(dst,src);
    regs.w[AX]=dst;
}

static void i_cs(void)    /* Opcode 0x2e */
{
    seg_prefix=TRUE;
    prefix_base=base[CS];
    cycle_count-=2;
    instruction[FETCH]();
}

static void i_das(void)    /* Opcode 0x2f */
{
	if (AF || ((regs.b[AL] & 0xf) > 9))
	{
		int tmp;
		regs.b[AL] = tmp = regs.b[AL] - 6;
		AuxVal = 1;
		CarryVal |= tmp & 0x100;
	}

	if (CF || (regs.b[AL] > 0x9f))
	{
		regs.b[AL] -= 0x60;
		CarryVal = 1;
	}

	SetSZPF_Byte(regs.b[AL]);
	cycle_count-=4;
}

static void i_xor_br8(void)    /* Opcode 0x30 */
{
    DEF_br8(dst,src);
    cycle_count-=3;
    XORB(dst,src);
    PutbackRMByte(ModRM,dst);
}

static void i_xor_wr16(void)    /* Opcode 0x31 */
{
    DEF_wr16(dst,src);
    cycle_count-=3;
    XORW(dst,src);
    PutbackRMWord(ModRM,dst);
}

static void i_xor_r8b(void)    /* Opcode 0x32 */
{
    DEF_r8b(dst,src);
    cycle_count-=3;
    XORB(dst,src);
    RegByte(ModRM)=dst;
}

static void i_xor_r16w(void)    /* Opcode 0x33 */
{
    DEF_r16w(dst,src);
    cycle_count-=3;
    XORW(dst,src);
    RegWord(ModRM)=dst;
}

static void i_xor_ald8(void)    /* Opcode 0x34 */
{
    DEF_ald8(dst,src);
    cycle_count-=4;
    XORB(dst,src);
    regs.b[AL] = dst;
}

static void i_xor_axd16(void)    /* Opcode 0x35 */
{
    DEF_axd16(dst,src);
    cycle_count-=4;
    XORW(dst,src);
    regs.w[AX]=dst;
}

static void i_ss(void)    /* Opcode 0x36 */
{
    seg_prefix=TRUE;
    prefix_base=base[SS];
    cycle_count-=2;
    instruction[FETCH]();
}

static void i_aaa(void)    /* Opcode 0x37 */
{
    if (AF || ((regs.b[AL] & 0xf) > 9))
    {
	regs.b[AL] += 6;
	regs.b[AH] += 1;
	AuxVal = 1;
	CarryVal = 1;
    }
    else {
	AuxVal = 0;
	CarryVal = 0;
    }
    regs.b[AL] &= 0x0F;
    cycle_count-=8;
}

static void i_cmp_br8(void)    /* Opcode 0x38 */
{
    DEF_br8(dst,src);
    cycle_count-=3;
    SUBB(dst,src);
}

static void i_cmp_wr16(void)    /* Opcode 0x39 */
{
    DEF_wr16(dst,src);
    cycle_count-=3;
    SUBW(dst,src);
}

static void i_cmp_r8b(void)    /* Opcode 0x3a */
{
    DEF_r8b(dst,src);
    cycle_count-=3;
    SUBB(dst,src);
}

static void i_cmp_r16w(void)    /* Opcode 0x3b */
{
    DEF_r16w(dst,src);
    cycle_count-=3;
    SUBW(dst,src);
}

static void i_cmp_ald8(void)    /* Opcode 0x3c */
{
    DEF_ald8(dst,src);
    cycle_count-=4;
    SUBB(dst,src);
}

static void i_cmp_axd16(void)    /* Opcode 0x3d */
{
    DEF_axd16(dst,src);
    cycle_count-=4;
    SUBW(dst,src);
}

static void i_ds(void)    /* Opcode 0x3e */
{
    seg_prefix=TRUE;
    prefix_base=base[DS];
    cycle_count-=2;
    instruction[FETCH]();
}

static void i_aas(void)    /* Opcode 0x3f */
{
    if (AF || ((regs.b[AL] & 0xf) > 9))
    {
	regs.b[AL] -= 6;
	regs.b[AH] -= 1;
	AuxVal = 1;
	CarryVal = 1;
    }
    else {
	AuxVal = 0;
	CarryVal = 0;
    }
    regs.b[AL] &= 0x0F;
    cycle_count-=8;
}

#define IncWordReg(Reg) \
{ \
    unsigned tmp = (unsigned)regs.w[Reg]; \
    unsigned tmp1 = tmp+1; \
    SetOFW_Add(tmp1,tmp,1); \
    SetAF(tmp1,tmp,1); \
    SetSZPF_Word(tmp1); \
    regs.w[Reg]=tmp1; \
    cycle_count-=3; \
}

static void i_inc_ax(void)    /* Opcode 0x40 */
{
    IncWordReg(AX);
}

static void i_inc_cx(void)    /* Opcode 0x41 */
{
    IncWordReg(CX);
}

static void i_inc_dx(void)    /* Opcode 0x42 */
{
    IncWordReg(DX);
}

static void i_inc_bx(void)    /* Opcode 0x43 */
{
    IncWordReg(BX);
}

static void i_inc_sp(void)    /* Opcode 0x44 */
{
    IncWordReg(SP);
}

static void i_inc_bp(void)    /* Opcode 0x45 */
{
    IncWordReg(BP);
}

static void i_inc_si(void)    /* Opcode 0x46 */
{
    IncWordReg(SI);
}

static void i_inc_di(void)    /* Opcode 0x47 */
{
    IncWordReg(DI);
}

#define DecWordReg(Reg) \
{ \
    unsigned tmp = (unsigned)regs.w[Reg]; \
    unsigned tmp1 = tmp-1; \
    SetOFW_Sub(tmp1,1,tmp); \
    SetAF(tmp1,tmp,1); \
    SetSZPF_Word(tmp1); \
    regs.w[Reg]=tmp1; \
    cycle_count-=3; \
}

static void i_dec_ax(void)    /* Opcode 0x48 */
{
    DecWordReg(AX);
}

static void i_dec_cx(void)    /* Opcode 0x49 */
{
    DecWordReg(CX);
}

static void i_dec_dx(void)    /* Opcode 0x4a */
{
    DecWordReg(DX);
}

static void i_dec_bx(void)    /* Opcode 0x4b */
{
    DecWordReg(BX);
}

static void i_dec_sp(void)    /* Opcode 0x4c */
{
    DecWordReg(SP);
}

static void i_dec_bp(void)    /* Opcode 0x4d */
{
    DecWordReg(BP);
}

static void i_dec_si(void)    /* Opcode 0x4e */
{
    DecWordReg(SI);
}

static void i_dec_di(void)    /* Opcode 0x4f */
{
    DecWordReg(DI);
}

static void i_push_ax(void)    /* Opcode 0x50 */
{
    cycle_count-=4;
    PUSH(regs.w[AX]);
}

static void i_push_cx(void)    /* Opcode 0x51 */
{
    cycle_count-=4;
    PUSH(regs.w[CX]);
}

static void i_push_dx(void)    /* Opcode 0x52 */
{
    cycle_count-=4;
    PUSH(regs.w[DX]);
}

static void i_push_bx(void)    /* Opcode 0x53 */
{
    cycle_count-=4;
    PUSH(regs.w[BX]);
}

static void i_push_sp(void)    /* Opcode 0x54 */
{
    cycle_count-=4;
    PUSH(regs.w[SP]);
}

static void i_push_bp(void)    /* Opcode 0x55 */
{
    cycle_count-=4;
    PUSH(regs.w[BP]);
}


static void i_push_si(void)    /* Opcode 0x56 */
{
    cycle_count-=4;
    PUSH(regs.w[SI]);
}

static void i_push_di(void)    /* Opcode 0x57 */
{
    cycle_count-=4;
    PUSH(regs.w[DI]);
}

static void i_pop_ax(void)    /* Opcode 0x58 */
{
    cycle_count-=2;
    POP(regs.w[AX]);
}

static void i_pop_cx(void)    /* Opcode 0x59 */
{
    cycle_count-=2;
    POP(regs.w[CX]);
}

static void i_pop_dx(void)    /* Opcode 0x5a */
{
    cycle_count-=2;
    POP(regs.w[DX]);
}

static void i_pop_bx(void)    /* Opcode 0x5b */
{
    cycle_count-=2;
    POP(regs.w[BX]);
}

static void i_pop_sp(void)    /* Opcode 0x5c */
{
    cycle_count-=2;
    POP(regs.w[SP]);
}

static void i_pop_bp(void)    /* Opcode 0x5d */
{
    cycle_count-=2;
    POP(regs.w[BP]);
}

static void i_pop_si(void)    /* Opcode 0x5e */
{
    cycle_count-=2;
    POP(regs.w[SI]);
}

static void i_pop_di(void)    /* Opcode 0x5f */
{
    cycle_count-=2;
    POP(regs.w[DI]);
}

static void i_pusha(void)    /* Opcode 0x60 */
{
    unsigned tmp=regs.w[SP];
    cycle_count-=17;
    PUSH(regs.w[AX]);
    PUSH(regs.w[CX]);
    PUSH(regs.w[DX]);
    PUSH(regs.w[BX]);
    PUSH(tmp);
    PUSH(regs.w[BP]);
    PUSH(regs.w[SI]);
    PUSH(regs.w[DI]);
}

static void i_popa(void)    /* Opcode 0x61 */
{
    unsigned tmp;
    cycle_count-=19;
    POP(regs.w[DI]);
    POP(regs.w[SI]);
    POP(regs.w[BP]);
    POP(tmp);
    POP(regs.w[BX]);
    POP(regs.w[DX]);
    POP(regs.w[CX]);
    POP(regs.w[AX]);
}

static void i_bound(void)    /* Opcode 0x62 */
{
    unsigned ModRM = FETCH;
    int low = (INT16)GetRMWord(ModRM);
    int high= (INT16)GetnextRMWord;
    int tmp= (INT16)RegWord(ModRM);
    if (tmp<low || tmp>high) {
        ip-=2;
        I86_interrupt(5);
    }
}

static void i_push_d16(void)    /* Opcode 0x68 */
{
    unsigned tmp = FETCH;
    cycle_count-=3;
    tmp += FETCH << 8;
    PUSH(tmp);
}

static void i_imul_d16(void)    /* Opcode 0x69 */
{
    DEF_r16w(dst,src);
    unsigned src2=FETCH;
    src+=(FETCH<<8);

    cycle_count-=150;
    dst = (INT32)((INT16)src)*(INT32)((INT16)src2);
    CarryVal = OverVal = (((INT32)dst) >> 15 != 0) && (((INT32)dst) >> 15 != -1);
    RegWord(ModRM)=(WORD)dst;
}


static void i_push_d8(void)    /* Opcode 0x6a */
{
    unsigned tmp = (WORD)((INT16)((INT8)FETCH));
    cycle_count-=3;
    PUSH(tmp);
}

static void i_imul_d8(void)    /* Opcode 0x6b */
{
    DEF_r16w(dst,src);
    unsigned src2= (WORD)((INT16)((INT8)FETCH));

    cycle_count-=150;
    dst = (INT32)((INT16)src)*(INT32)((INT16)src2);
    CarryVal = OverVal = (((INT32)dst) >> 15 != 0) && (((INT32)dst) >> 15 != -1);
    RegWord(ModRM)=(WORD)dst;
}

static void i_insb(void)    /* Opcode 0x6c */
{
    cycle_count-=5;
    PutMemB(ES,regs.w[DI],read_port(regs.w[DX]));
    regs.w[DI]+= -2*DF+1;
}

static void i_insw(void)    /* Opcode 0x6d */
{
    cycle_count-=5;
    PutMemB(ES,regs.w[DI],read_port(regs.w[DX]));
    PutMemB(ES,regs.w[DI]+1,read_port(regs.w[DX]+1));
    regs.w[DI]+= -4*DF+2;
}

static void i_outsb(void)    /* Opcode 0x6e */
{
    cycle_count-=5;
    write_port(regs.w[DX],GetMemB(DS,regs.w[SI]));
    regs.w[DI]+= -2*DF+1;
}

static void i_outsw(void)    /* Opcode 0x6f */
{
    cycle_count-=5;
    write_port(regs.w[DX],GetMemB(DS,regs.w[SI]));
    write_port(regs.w[DX]+1,GetMemB(DS,regs.w[SI]+1));
    regs.w[DI]+= -4*DF+2;
}

static void i_jo(void)    /* Opcode 0x70 */
{
    int tmp = (int)((INT8)FETCH);
    if (OF) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_jno(void)    /* Opcode 0x71 */
{
    int tmp = (int)((INT8)FETCH);
    if (!OF) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_jb(void)    /* Opcode 0x72 */
{
    int tmp = (int)((INT8)FETCH);
    if (CF) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_jnb(void)    /* Opcode 0x73 */
{
    int tmp = (int)((INT8)FETCH);
    if (!CF) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_jz(void)    /* Opcode 0x74 */
{
    int tmp = (int)((INT8)FETCH);
    if (ZF) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_jnz(void)    /* Opcode 0x75 */
{
    int tmp = (int)((INT8)FETCH);
    if (!ZF) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_jbe(void)    /* Opcode 0x76 */
{
    int tmp = (int)((INT8)FETCH);
    if (CF || ZF) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_jnbe(void)    /* Opcode 0x77 */
{
    int tmp = (int)((INT8)FETCH);
    if (!(CF || ZF)) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_js(void)    /* Opcode 0x78 */
{
    int tmp = (int)((INT8)FETCH);
    if (SF) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_jns(void)    /* Opcode 0x79 */
{
    int tmp = (int)((INT8)FETCH);
    if (!SF) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_jp(void)    /* Opcode 0x7a */
{
    int tmp = (int)((INT8)FETCH);
    if (PF) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_jnp(void)    /* Opcode 0x7b */
{
    int tmp = (int)((INT8)FETCH);
    if (!PF) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_jl(void)    /* Opcode 0x7c */
{
    int tmp = (int)((INT8)FETCH);
    if ((SF!=OF)&&!ZF) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_jnl(void)    /* Opcode 0x7d */
{
    int tmp = (int)((INT8)FETCH);
    if (ZF||(SF==OF)) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_jle(void)    /* Opcode 0x7e */
{
    int tmp = (int)((INT8)FETCH);
    if (ZF||(SF!=OF)) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_jnle(void)    /* Opcode 0x7f */
{
    int tmp = (int)((INT8)FETCH);
    if ((SF==OF)&&!ZF) {
	ip = (WORD)(ip+tmp);
	cycle_count-=16;
    } else cycle_count-=4;
}

static void i_80pre(void)    /* Opcode 0x80 */
{
    unsigned ModRM = FETCH;
    unsigned dst = GetRMByte(ModRM);
    unsigned src = FETCH;
    cycle_count-=4;

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ADD eb,d8 */
        ADDB(dst,src);
        PutbackRMByte(ModRM,dst);
	break;
    case 0x08:  /* OR eb,d8 */
        ORB(dst,src);
        PutbackRMByte(ModRM,dst);
	break;
    case 0x10:  /* ADC eb,d8 */
        src+=CF;
        ADDB(dst,src);
        PutbackRMByte(ModRM,dst);
	break;
    case 0x18:  /* SBB eb,b8 */
        src+=CF;
        SUBB(dst,src);
        PutbackRMByte(ModRM,dst);
	break;
    case 0x20:  /* AND eb,d8 */
        ANDB(dst,src);
        PutbackRMByte(ModRM,dst);
	break;
    case 0x28:  /* SUB eb,d8 */
        SUBB(dst,src);
        PutbackRMByte(ModRM,dst);
	break;
    case 0x30:  /* XOR eb,d8 */
        XORB(dst,src);
        PutbackRMByte(ModRM,dst);
	break;
    case 0x38:  /* CMP eb,d8 */
        SUBB(dst,src);
	break;
    }
}


static void i_81pre(void)    /* Opcode 0x81 */
{
    unsigned ModRM = FETCH;
    unsigned dst = GetRMWord(ModRM);
    unsigned src = FETCH;
    src+= (FETCH << 8);
    cycle_count-=2;

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ADD ew,d16 */
        ADDW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x08:  /* OR ew,d16 */
        ORW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x10:  /* ADC ew,d16 */
        src+=CF;
        ADDW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x18:  /* SBB ew,d16 */
        src+=CF;
        SUBW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x20:  /* AND ew,d16 */
        ANDW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x28:  /* SUB ew,d16 */
        SUBW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x30:  /* XOR ew,d16 */
        XORW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x38:  /* CMP ew,d16 */
        SUBW(dst,src);
	break;
    }
}

static void i_83pre(void)    /* Opcode 0x83 */
{
    unsigned ModRM = FETCH;
    unsigned dst = GetRMWord(ModRM);
    unsigned src = (WORD)((INT16)((INT8)FETCH));
    cycle_count-=2;

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ADD ew,d8 */
        ADDW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x08:  /* OR ew,d8 */
        ORW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x10:  /* ADC ew,d8 */
        src+=CF;
        ADDW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x18:  /* SBB ew,d8 */
        src+=CF;
        SUBW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x20:  /* AND ew,d8 */
        ANDW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x28:  /* SUB ew,d8 */
        SUBW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x30:  /* XOR ew,d8 */
        XORW(dst,src);
        PutbackRMWord(ModRM,dst);
	break;
    case 0x38:  /* CMP ew,d8 */
        SUBW(dst,src);
	break;
    }
}

static void i_test_br8(void)    /* Opcode 0x84 */
{
    DEF_br8(dst,src);
    cycle_count-=3;
    ANDB(dst,src);
}

static void i_test_wr16(void)    /* Opcode 0x85 */
{
    DEF_wr16(dst,src);
    cycle_count-=3;
    ANDW(dst,src);
}

static void i_xchg_br8(void)    /* Opcode 0x86 */
{
    DEF_br8(dst,src);
    cycle_count-=4;
    RegByte(ModRM)=dst;
    PutbackRMByte(ModRM,src);
}

static void i_xchg_wr16(void)    /* Opcode 0x87 */
{
    DEF_wr16(dst,src);
    cycle_count-=4;
    RegWord(ModRM)=dst;
    PutbackRMWord(ModRM,src);
}

static void i_mov_br8(void)    /* Opcode 0x88 */
{
    unsigned ModRM = FETCH;
    BYTE src = RegByte(ModRM);
    cycle_count-=2;
    PutRMByte(ModRM,src);
}

static void i_mov_wr16(void)    /* Opcode 0x89 */
{
    unsigned ModRM = FETCH;
    WORD src = RegWord(ModRM);
    cycle_count-=2;
    PutRMWord(ModRM,src);
}

static void i_mov_r8b(void)    /* Opcode 0x8a */
{
    unsigned ModRM = FETCH;
    BYTE src = GetRMByte(ModRM);
    cycle_count-=2;
    RegByte(ModRM)=src;
}

static void i_mov_r16w(void)    /* Opcode 0x8b */
{
    unsigned ModRM = FETCH;
    WORD src = GetRMWord(ModRM);
    cycle_count-=2;
    RegWord(ModRM)=src;
}

static void i_mov_wsreg(void)    /* Opcode 0x8c */
{
    unsigned ModRM = FETCH;
    cycle_count-=2;
    PutRMWord(ModRM,sregs[(ModRM & 0x38) >> 3]);
}

static void i_lea(void)    /* Opcode 0x8d */
{
    unsigned ModRM = FETCH;
    cycle_count-=2;
    RegWord(ModRM)=(*GetEA[ModRM])();
}

static void i_mov_sregw(void)    /* Opcode 0x8e */
{
    unsigned ModRM = FETCH;
    WORD src = GetRMWord(ModRM);

    cycle_count-=2;
    switch (ModRM & 0x38)
    {
    case 0x00:  /* mov es,ew */
	sregs[ES] = src;
	base[ES] = SegBase(ES);
	break;
    case 0x18:  /* mov ds,ew */
	sregs[DS] = src;
	base[DS] = SegBase(DS);
	break;
    case 0x10:  /* mov ss,ew */
	sregs[SS] = src;
	base[SS] = SegBase(SS); /* no interrupt allowed before next instr */
	instruction[FETCH]();
	break;
    case 0x08:  /* mov cs,ew */
	break;  /* doesn't do a jump far */
    }
}

static void i_popw(void)    /* Opcode 0x8f */
{
    unsigned ModRM = FETCH;
    WORD tmp;
    POP(tmp);
    cycle_count-=4;
    PutRMWord(ModRM,tmp);
}


#define XchgAXReg(Reg) \
{ \
    WORD tmp; \
    tmp = regs.w[Reg]; \
    regs.w[Reg] = regs.w[AX]; \
    regs.w[AX] = tmp; \
    cycle_count-=3; \
}


static void i_nop(void)    /* Opcode 0x90 */
{
    /* this is XchgAXReg(AX); */
    cycle_count-=3;
}

static void i_xchg_axcx(void)    /* Opcode 0x91 */
{
    XchgAXReg(CX);
}

static void i_xchg_axdx(void)    /* Opcode 0x92 */
{
    XchgAXReg(DX);
}

static void i_xchg_axbx(void)    /* Opcode 0x93 */
{
    XchgAXReg(BX);
}

static void i_xchg_axsp(void)    /* Opcode 0x94 */
{
    XchgAXReg(SP);
}

static void i_xchg_axbp(void)    /* Opcode 0x95 */
{
    XchgAXReg(BP);
}

static void i_xchg_axsi(void)    /* Opcode 0x96 */
{
    XchgAXReg(SI);
}

static void i_xchg_axdi(void)    /* Opcode 0x97 */
{
    XchgAXReg(DI);
}

static void i_cbw(void)    /* Opcode 0x98 */
{
    cycle_count-=2;
    regs.b[AH] = (regs.b[AL] & 0x80) ? 0xff : 0;
}

static void i_cwd(void)    /* Opcode 0x99 */
{
    cycle_count-=5;
    regs.w[DX] = (regs.b[AH] & 0x80) ? 0xffff : 0;
}

static void i_call_far(void)
{
    unsigned tmp, tmp2;

    tmp = FETCH;
    tmp += FETCH << 8;

    tmp2 = FETCH;
    tmp2 += FETCH << 8;

    PUSH(sregs[CS]);
    PUSH(ip);

    ip = (WORD)tmp;
    sregs[CS] = (WORD)tmp2;
    base[CS] = SegBase(CS);
    cycle_count-=14;
}

static void i_wait(void)    /* Opcode 0x9b */
{
    cycle_count-=4;
}

static void i_pushf(void)    /* Opcode 0x9c */
{
    cycle_count-=3;
    PUSH( CompressFlags() | 0xf000 );
}

static void i_popf(void)    /* Opcode 0x9d */
{
    unsigned tmp;
    POP(tmp);
    cycle_count-=2;
    ExpandFlags(tmp);

    if (TF) trap();
}

static void i_sahf(void)    /* Opcode 0x9e */
{
    unsigned tmp = (CompressFlags() & 0xff00) | (regs.b[AH] & 0xd5);

    ExpandFlags(tmp);
}

static void i_lahf(void)    /* Opcode 0x9f */
{
    regs.b[AH] = CompressFlags() & 0xff;
    cycle_count-=4;
}

static void i_mov_aldisp(void)    /* Opcode 0xa0 */
{
    unsigned addr;

    addr = FETCH;
    addr += FETCH << 8;

    cycle_count-=4;
    regs.b[AL] = GetMemB(DS, addr);
}

static void i_mov_axdisp(void)    /* Opcode 0xa1 */
{
    unsigned addr;

    addr = FETCH;
    addr += FETCH << 8;

    cycle_count-=4;
    regs.b[AL] = GetMemB(DS, addr);
    regs.b[AH] = GetMemB(DS, addr+1);
}

static void i_mov_dispal(void)    /* Opcode 0xa2 */
{
    unsigned addr;

    addr = FETCH;
    addr += FETCH << 8;

    cycle_count-=3;
    PutMemB(DS, addr, regs.b[AL]);
}

static void i_mov_dispax(void)    /* Opcode 0xa3 */
{
    unsigned addr;

    addr = FETCH;
    addr += FETCH << 8;

    cycle_count-=3;
    PutMemB(DS, addr, regs.b[AL]);
    PutMemB(DS, addr+1, regs.b[AH]);
}

static void i_movsb(void)    /* Opcode 0xa4 */
{
    BYTE tmp = GetMemB(DS,regs.w[SI]);
    PutMemB(ES,regs.w[DI], tmp);
    regs.w[DI] += -2*DF +1;
    regs.w[SI] += -2*DF +1;
    cycle_count-=5;
}

static void i_movsw(void)    /* Opcode 0xa5 */
{
    WORD tmp = GetMemW(DS,regs.w[SI]);
    PutMemW(ES,regs.w[DI], tmp);
    regs.w[DI] += -4*DF +2;
    regs.w[SI] += -4*DF +2;
    cycle_count-=5;
}

static void i_cmpsb(void)    /* Opcode 0xa6 */
{
    unsigned dst = GetMemB(ES, regs.w[DI]);
    unsigned src = GetMemB(DS, regs.w[SI]);
    SUBB(src,dst); /* opposite of the usual convention */
    regs.w[DI] += -2*DF +1;
    regs.w[SI] += -2*DF +1;
    cycle_count-=10;
}

static void i_cmpsw(void)    /* Opcode 0xa7 */
{
    unsigned dst = GetMemW(ES, regs.w[DI]);
    unsigned src = GetMemW(DS, regs.w[SI]);
    SUBW(src,dst); /* opposite of the usual convention */
    regs.w[DI] += -4*DF +2;
    regs.w[SI] += -4*DF +2;
    cycle_count-=10;
}

static void i_test_ald8(void)    /* Opcode 0xa8 */
{
    DEF_ald8(dst,src);
    cycle_count-=4;
    ANDB(dst,src);
}

static void i_test_axd16(void)    /* Opcode 0xa9 */
{
    DEF_axd16(dst,src);
    cycle_count-=4;
    ANDW(dst,src);
}

static void i_stosb(void)    /* Opcode 0xaa */
{
    PutMemB(ES,regs.w[DI],regs.b[AL]);
    regs.w[DI] += -2*DF +1;
    cycle_count-=4;
}

static void i_stosw(void)    /* Opcode 0xab */
{
    PutMemB(ES,regs.w[DI],regs.b[AL]);
    PutMemB(ES,regs.w[DI]+1,regs.b[AH]);
    regs.w[DI] += -4*DF +2;
    cycle_count-=4;
}

static void i_lodsb(void)    /* Opcode 0xac */
{
    regs.b[AL] = GetMemB(DS,regs.w[SI]);
    regs.w[SI] += -2*DF +1;
    cycle_count-=6;
}

static void i_lodsw(void)    /* Opcode 0xad */
{
    regs.w[AX] = GetMemW(DS,regs.w[SI]);
    regs.w[SI] +=  -4*DF+2;
    cycle_count-=6;
}

static void i_scasb(void)    /* Opcode 0xae */
{
    unsigned src = GetMemB(ES, regs.w[DI]);
    unsigned dst = regs.b[AL];
    SUBB(dst,src);
    regs.w[DI] += -2*DF +1;
    cycle_count-=9;
}

static void i_scasw(void)    /* Opcode 0xaf */
{
    unsigned src = GetMemW(ES, regs.w[DI]);
    unsigned dst = regs.w[AX];
    SUBW(dst,src);
    regs.w[DI] += -4*DF +2;
    cycle_count-=9;
}

static void i_mov_ald8(void)    /* Opcode 0xb0 */
{
    regs.b[AL] = FETCH;
    cycle_count-=4;
}

static void i_mov_cld8(void)    /* Opcode 0xb1 */
{
    regs.b[CL] = FETCH;
    cycle_count-=4;
}

static void i_mov_dld8(void)    /* Opcode 0xb2 */
{
    regs.b[DL] = FETCH;
    cycle_count-=4;
}

static void i_mov_bld8(void)    /* Opcode 0xb3 */
{
    regs.b[BL] = FETCH;
    cycle_count-=4;
}

static void i_mov_ahd8(void)    /* Opcode 0xb4 */
{
    regs.b[AH] = FETCH;
    cycle_count-=4;
}

static void i_mov_chd8(void)    /* Opcode 0xb5 */
{
    regs.b[CH] = FETCH;
    cycle_count-=4;
}

static void i_mov_dhd8(void)    /* Opcode 0xb6 */
{
    regs.b[DH] = FETCH;
    cycle_count-=4;
}

static void i_mov_bhd8(void)    /* Opcode 0xb7 */
{
    regs.b[BH] = FETCH;
    cycle_count-=4;
}

static void i_mov_axd16(void)    /* Opcode 0xb8 */
{
    regs.b[AL] = FETCH;
    regs.b[AH] = FETCH;
    cycle_count-=4;
}

static void i_mov_cxd16(void)    /* Opcode 0xb9 */
{
    regs.b[CL] = FETCH;
    regs.b[CH] = FETCH;
    cycle_count-=4;
}

static void i_mov_dxd16(void)    /* Opcode 0xba */
{
    regs.b[DL] = FETCH;
    regs.b[DH] = FETCH;
    cycle_count-=4;
}

static void i_mov_bxd16(void)    /* Opcode 0xbb */
{
    regs.b[BL] = FETCH;
    regs.b[BH] = FETCH;
    cycle_count-=4;
}

static void i_mov_spd16(void)    /* Opcode 0xbc */
{
    regs.b[SPL] = FETCH;
    regs.b[SPH] = FETCH;
    cycle_count-=4;
}

static void i_mov_bpd16(void)    /* Opcode 0xbd */
{
    regs.b[BPL] = FETCH;
    regs.b[BPH] = FETCH;
    cycle_count-=4;
}

static void i_mov_sid16(void)    /* Opcode 0xbe */
{
    regs.b[SIL] = FETCH;
    regs.b[SIH] = FETCH;
    cycle_count-=4;
}

static void i_mov_did16(void)    /* Opcode 0xbf */
{
    regs.b[DIL] = FETCH;
    regs.b[DIH] = FETCH;
    cycle_count-=4;
}

void rotate_shift_Byte(unsigned ModRM, unsigned count)
{
  unsigned src = (unsigned)GetRMByte(ModRM);
  unsigned dst=src;

  if (count==0)
  {
    cycle_count-=8; /* or 7 if dest is in memory */
  }
  else if (count==1)
  {
    cycle_count-=2;
    switch (ModRM & 0x38)
    {
      case 0x00:  /* ROL eb,1 */
        CarryVal = src & 0x80;
        dst=(src<<1)+CF;
        PutbackRMByte(ModRM,dst);
        OverVal = (src^dst)&0x80;
	break;
      case 0x08:  /* ROR eb,1 */
        CarryVal = src & 0x01;
        dst = ((CF<<8)+src) >> 1;
        PutbackRMByte(ModRM,dst);
        OverVal = (src^dst)&0x80;
	break;
      case 0x10:  /* RCL eb,1 */
        dst=(src<<1)+CF;
        PutbackRMByte(ModRM,dst);
        SetCFB(dst);
        OverVal = (src^dst)&0x80;
	break;
      case 0x18:  /* RCR eb,1 */
        dst = ((CF<<8)+src) >> 1;
        PutbackRMByte(ModRM,dst);
        CarryVal = src & 0x01;
        OverVal = (src^dst)&0x80;
	break;
      case 0x20:  /* SHL eb,1 */
      case 0x30:
        dst = src << 1;
        PutbackRMByte(ModRM,dst);
        SetCFB(dst);
        OverVal = (src^dst)&0x80;
	AuxVal = 1;
        SetSZPF_Byte(dst);
	break;
      case 0x28:  /* SHR eb,1 */
        dst = src >> 1;
        PutbackRMByte(ModRM,dst);
        CarryVal = src & 0x01;
        OverVal = src & 0x80;
	AuxVal = 1;
        SetSZPF_Byte(dst);
	break;
      case 0x38:  /* SAR eb,1 */
        dst = ((INT8)src) >> 1;
        PutbackRMByte(ModRM,dst);
        CarryVal = src & 0x01;
	OverVal = 0;
	AuxVal = 1;
        SetSZPF_Byte(dst);
	break;
    }
  }
  else
  {
    cycle_count-=8+4*count; /* or 7+4*count if dest is in memory */
    switch (ModRM & 0x38)
    {
      case 0x00:  /* ROL eb,count */
	for (; count > 0; count--)
	{
            CarryVal = dst & 0x80;
            dst = (dst << 1) + CF;
	}
        PutbackRMByte(ModRM,(BYTE)dst);
	break;
     case 0x08:  /* ROR eb,count */
	for (; count > 0; count--)
	{
            CarryVal = dst & 0x01;
            dst = (dst >> 1) + (CF << 7);
	}
        PutbackRMByte(ModRM,(BYTE)dst);
	break;
      case 0x10:  /* RCL eb,count */
	for (; count > 0; count--)
	{
            dst = (dst << 1) + CF;
            SetCFB(dst);
	}
        PutbackRMByte(ModRM,(BYTE)dst);
	break;
      case 0x18:  /* RCR eb,count */
	for (; count > 0; count--)
	{
            dst = (CF<<8)+dst;
            CarryVal = dst & 0x01;
            dst >>= 1;
	}
        PutbackRMByte(ModRM,(BYTE)dst);
	break;
      case 0x20:
      case 0x30:  /* SHL eb,count */
        dst <<= count;
        SetCFB(dst);
	AuxVal = 1;
        SetSZPF_Byte(dst);
        PutbackRMByte(ModRM,(BYTE)dst);
	break;
      case 0x28:  /* SHR eb,count */
        dst >>= count-1;
        CarryVal = dst & 0x1;
        dst >>= 1;
        SetSZPF_Byte(dst);
	AuxVal = 1;
        PutbackRMByte(ModRM,(BYTE)dst);
	break;
      case 0x38:  /* SAR eb,count */
        dst = ((INT8)dst) >> (count-1);
        CarryVal = dst & 0x1;
        dst = ((INT8)((BYTE)dst)) >> 1;
        SetSZPF_Byte(dst);
	AuxVal = 1;
        PutbackRMByte(ModRM,(BYTE)dst);
	break;
    }
  }
}

void rotate_shift_Word(unsigned ModRM, unsigned count)
{
  unsigned src = GetRMWord(ModRM);
  unsigned dst=src;

  if (count==0)
  {
    cycle_count-=8; /* or 7 if dest is in memory */
  }
  else if (count==1)
  {
    cycle_count-=2;
    switch (ModRM & 0x38)
    {
#if 0
      case 0x00:  /* ROL ew,1 */
        tmp2 = (tmp << 1) + CF;
	SetCFW(tmp2);
	OverVal = !(!(tmp & 0x4000)) != CF;
	PutbackRMWord(ModRM,tmp2);
	break;
      case 0x08:  /* ROR ew,1 */
	CarryVal = tmp & 0x01;
	tmp2 = (tmp >> 1) + ((unsigned)CF << 15);
	OverVal = !(!(tmp & 0x8000)) != CF;
	PutbackRMWord(ModRM,tmp2);
	break;
      case 0x10:  /* RCL ew,1 */
	tmp2 = (tmp << 1) + CF;
	SetCFW(tmp2);
	OverVal = (tmp ^ (tmp << 1)) & 0x8000;
	PutbackRMWord(ModRM,tmp2);
	break;
      case 0x18:  /* RCR ew,1 */
	tmp2 = (tmp >> 1) + ((unsigned)CF << 15);
	OverVal = !(!(tmp & 0x8000)) != CF;
	CarryVal = tmp & 0x01;
	PutbackRMWord(ModRM,tmp2);
	break;
      case 0x20:  /* SHL ew,1 */
      case 0x30:
	tmp <<= 1;

	SetCFW(tmp);
	SetOFW_Add(tmp,tmp2,tmp2);
	AuxVal = 1;
	SetSZPF_Word(tmp);

	PutbackRMWord(ModRM,tmp);
	break;
      case 0x28:  /* SHR ew,1 */
	CarryVal = tmp & 0x01;
	OverVal = tmp & 0x8000;

	tmp2 = tmp >> 1;

	SetSZPF_Word(tmp2);
	AuxVal = 1;
	PutbackRMWord(ModRM,tmp2);
	break;
      case 0x38:  /* SAR ew,1 */
	CarryVal = tmp & 0x01;
	OverVal = 0;

	tmp2 = (tmp >> 1) | (tmp & 0x8000);

	SetSZPF_Word(tmp2);
	AuxVal = 1;
	PutbackRMWord(ModRM,tmp2);
	break;
#else
      case 0x00:  /* ROL ew,1 */
        CarryVal = src & 0x8000;
        dst=(src<<1)+CF;
        PutbackRMWord(ModRM,dst);
        OverVal = (src^dst)&0x8000;
	break;
      case 0x08:  /* ROR ew,1 */
        CarryVal = src & 0x01;
        dst = ((CF<<16)+src) >> 1;
        PutbackRMWord(ModRM,dst);
        OverVal = (src^dst)&0x8000;
	break;
      case 0x10:  /* RCL ew,1 */
        dst=(src<<1)+CF;
        PutbackRMWord(ModRM,dst);
        SetCFW(dst);
        OverVal = (src^dst)&0x8000;
	break;
      case 0x18:  /* RCR ew,1 */
        dst = ((CF<<16)+src) >> 1;
        PutbackRMWord(ModRM,dst);
        CarryVal = src & 0x01;
        OverVal = (src^dst)&0x8000;
	break;
      case 0x20:  /* SHL ew,1 */
      case 0x30:
        dst = src << 1;
        PutbackRMWord(ModRM,dst);
        SetCFW(dst);
        OverVal = (src^dst)&0x8000;
	AuxVal = 1;
        SetSZPF_Word(dst);
	break;
      case 0x28:  /* SHR ew,1 */
        dst = src >> 1;
        PutbackRMWord(ModRM,dst);
        CarryVal = src & 0x01;
        OverVal = src & 0x8000;
	AuxVal = 1;
        SetSZPF_Word(dst);
	break;
      case 0x38:  /* SAR ew,1 */
        dst = ((INT16)src) >> 1;
        PutbackRMWord(ModRM,dst);
        CarryVal = src & 0x01;
	OverVal = 0;
	AuxVal = 1;
        SetSZPF_Word(dst);
	break;
#endif
    }
  }
  else
  {
    cycle_count-=8+4*count; /* or 7+4*count if dest is in memory */

    switch (ModRM & 0x38)
    {
      case 0x00:  /* ROL ew,count */
	for (; count > 0; count--)
	{
            CarryVal = dst & 0x8000;
            dst = (dst << 1) + CF;
	}
        PutbackRMWord(ModRM,dst);
	break;
      case 0x08:  /* ROR ew,count */
	for (; count > 0; count--)
	{
            CarryVal = dst & 0x01;
            dst = (dst >> 1) + (CF << 15);
	}
        PutbackRMWord(ModRM,dst);
	break;
      case 0x10:  /* RCL ew,count */
	for (; count > 0; count--)
	{
            dst = (dst << 1) + CF;
            SetCFW(dst);
	}
        PutbackRMWord(ModRM,dst);
	break;
      case 0x18:  /* RCR ew,count */
	for (; count > 0; count--)
	{
            dst = dst + (CF << 16);
            CarryVal = dst & 0x01;
            dst >>= 1;
	}
        PutbackRMWord(ModRM,dst);
	break;
      case 0x20:
      case 0x30:  /* SHL ew,count */
        dst <<= count;
        SetCFW(dst);
	AuxVal = 1;
        SetSZPF_Word(dst);
        PutbackRMWord(ModRM,dst);
	break;
      case 0x28:  /* SHR ew,count */
        dst >>= count-1;
        CarryVal = dst & 0x1;
        dst >>= 1;
        SetSZPF_Word(dst);
	AuxVal = 1;
        PutbackRMWord(ModRM,dst);
	break;
      case 0x38:  /* SAR ew,count */
        dst = ((INT16)dst) >> (count-1);
        CarryVal = dst & 0x01;
        dst = ((INT16)((WORD)dst)) >> 1;
        SetSZPF_Word(dst);
	AuxVal = 1;
        PutbackRMWord(ModRM,dst);
	break;
    }
  }
}


static void i_rotshft_bd8(void)    /* Opcode 0xc0 */
{
    unsigned ModRM=FETCH;
    unsigned count=FETCH;

    rotate_shift_Byte(ModRM,count);
}

static void i_rotshft_wd8(void)    /* Opcode 0xc1 */
{
    unsigned ModRM=FETCH;
    unsigned count=FETCH;

    rotate_shift_Word(ModRM,count);
}


static void i_ret_d16(void)    /* Opcode 0xc2 */
{
    unsigned count = FETCH;
    count += FETCH << 8;
    POP(ip);
    regs.w[SP]+=count;
    cycle_count-=14;
}

static void i_ret(void)    /* Opcode 0xc3 */
{
    POP(ip);
    cycle_count-=10;
}

static void i_les_dw(void)    /* Opcode 0xc4 */
{
    unsigned ModRM = FETCH;
    WORD tmp = GetRMWord(ModRM);

    RegWord(ModRM)= tmp;
    sregs[ES] = GetnextRMWord;
    base[ES] = SegBase(ES);
    cycle_count-=4;
}

static void i_lds_dw(void)    /* Opcode 0xc5 */
{
    unsigned ModRM = FETCH;
    WORD tmp = GetRMWord(ModRM);

    RegWord(ModRM)=tmp;
    sregs[DS] = GetnextRMWord;
    base[DS] = SegBase(DS);
    cycle_count-=4;
}

static void i_mov_bd8(void)    /* Opcode 0xc6 */
{
    unsigned ModRM = FETCH;
    cycle_count-=4;
    PutImmRMByte(ModRM);
}

static void i_mov_wd16(void)    /* Opcode 0xc7 */
{
    unsigned ModRM = FETCH;
    cycle_count-=4;
    PutImmRMWord(ModRM);
}

static void i_enter(void)    /* Opcode 0xc8 */
{
    unsigned nb = FETCH;
    unsigned i,level;

    cycle_count-=11;
    nb += FETCH << 8;
    level=FETCH;
    PUSH(regs.w[BP]);
    regs.w[BP]=regs.w[SP];
    regs.w[SP] -= nb;
    for (i=1;i<level;i++) {
        PUSH(GetMemW(SS,regs.w[BP]-i*2));
        cycle_count-=4;
    }
    if (level) PUSH(regs.w[BP]);
}

static void i_leave(void)    /* Opcode 0xc9 */
{
    cycle_count-=5;
    regs.w[SP]=regs.w[BP];
    POP(regs.w[BP]);
}

static void i_retf_d16(void)    /* Opcode 0xca */
{
    unsigned count = FETCH;
    count += FETCH << 8;
    POP(ip);
    POP(sregs[CS]);
    base[CS] = SegBase(CS);
    regs.w[SP]+=count;
    cycle_count-=13;
}

static void i_retf(void)    /* Opcode 0xcb */
{
    POP(ip);
    POP(sregs[CS]);
    base[CS] = SegBase(CS);
    cycle_count-=14;
}

static void i_int3(void)    /* Opcode 0xcc */
{
    cycle_count-=16;
    I86_interrupt(3);
}

static void i_int(void)    /* Opcode 0xcd */
{
    unsigned int_num = FETCH;
    cycle_count-=15;
    I86_interrupt(int_num);
}

static void i_into(void)    /* Opcode 0xce */
{
    if (OF) {
	cycle_count-=17;
	I86_interrupt(4);
    } else cycle_count-=4;
}

static void i_iret(void)    /* Opcode 0xcf */
{
    cycle_count-=12;
    POP(ip);
    POP(sregs[CS]);
    base[CS] = SegBase(CS);
    i_popf();
}

static void i_rotshft_b(void)    /* Opcode 0xd0 */
{
    rotate_shift_Byte(FETCH,1);
}


static void i_rotshft_w(void)    /* Opcode 0xd1 */
{
    rotate_shift_Word(FETCH,1);
}


static void i_rotshft_bcl(void)    /* Opcode 0xd2 */
{
    rotate_shift_Byte(FETCH,regs.b[CL]);
}

static void i_rotshft_wcl(void)    /* Opcode 0xd3 */
{
    rotate_shift_Word(FETCH,regs.b[CL]);
}

static void i_aam(void)    /* Opcode 0xd4 */
{
    unsigned mult = FETCH;

    cycle_count-=83;
    if (mult == 0)
	I86_interrupt(0);
    else
    {
	regs.b[AH] = regs.b[AL] / mult;
	regs.b[AL] %= mult;

	SetSZPF_Word(regs.w[AX]);
    }
}


static void i_aad(void)    /* Opcode 0xd5 */
{
    unsigned mult = FETCH;

    cycle_count-=60;
    regs.b[AL] = regs.b[AH] * mult + regs.b[AL];
    regs.b[AH] = 0;

    SetZF(regs.b[AL]);
    SetPF(regs.b[AL]);
    SignVal = 0;
}

static void i_xlat(void)    /* Opcode 0xd7 */
{
    unsigned dest = regs.w[BX]+regs.b[AL];

    cycle_count-=5;
    regs.b[AL] = GetMemB(DS, dest);
}

static void i_escape(void)    /* Opcodes 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde and 0xdf */
{
    unsigned ModRM = FETCH;
    cycle_count-=2;
    GetRMByte(ModRM);
}

static void i_loopne(void)    /* Opcode 0xe0 */
{
    int disp = (int)((INT8)FETCH);
    unsigned tmp = regs.w[CX]-1;

    regs.w[CX]=tmp;

    if (!ZF && tmp) {
	cycle_count-=19;
	ip = (WORD)(ip+disp);
    } else cycle_count-=5;
}

static void i_loope(void)    /* Opcode 0xe1 */
{
    int disp = (int)((INT8)FETCH);
    unsigned tmp = regs.w[CX]-1;

    regs.w[CX]=tmp;

    if (ZF && tmp) {
	cycle_count-=18;
	ip = (WORD)(ip+disp);
    } else cycle_count-=6;
}

static void i_loop(void)    /* Opcode 0xe2 */
{
    int disp = (int)((INT8)FETCH);
    unsigned tmp = regs.w[CX]-1;

    regs.w[CX]=tmp;

    if (tmp) {
	cycle_count-=17;
	ip = (WORD)(ip+disp);
    } else cycle_count-=5;
}

static void i_jcxz(void)    /* Opcode 0xe3 */
{
    int disp = (int)((INT8)FETCH);

    if (regs.w[CX] == 0) {
	cycle_count-=18;
	ip = (WORD)(ip+disp);
    } else cycle_count-=6;
}

static void i_inal(void)    /* Opcode 0xe4 */
{
    unsigned port = FETCH;

    cycle_count-=10;
    regs.b[AL] = read_port(port);
}

static void i_inax(void)    /* Opcode 0xe5 */
{
    unsigned port = FETCH;

    cycle_count-=14;
    regs.b[AL] = read_port(port);
    regs.b[AH] = read_port(port+1);
}

static void i_outal(void)    /* Opcode 0xe6 */
{
    unsigned port = FETCH;

    cycle_count-=10;
    write_port(port, regs.b[AL]);
}

static void i_outax(void)    /* Opcode 0xe7 */
{
    unsigned port = FETCH;

    cycle_count-=14;
    write_port(port, regs.b[AL]);
    write_port(port+1, regs.b[AH]);
}

static void i_call_d16(void)    /* Opcode 0xe8 */
{
    unsigned tmp = FETCH;
    tmp += FETCH << 8;

    PUSH(ip);
    ip = (WORD)(ip+(INT16)tmp);
    cycle_count-=12;
}


static void i_jmp_d16(void)    /* Opcode 0xe9 */
{
    int tmp = FETCH;
    tmp += FETCH << 8;

    ip = (WORD)(ip+(INT16)tmp);
    cycle_count-=15;
}

static void i_jmp_far(void)    /* Opcode 0xea */
{
    unsigned tmp,tmp1;

    tmp = FETCH;
    tmp += FETCH << 8;

    tmp1 = FETCH;
    tmp1 += FETCH << 8;

    sregs[CS] = (WORD)tmp1;
    base[CS] = SegBase(CS);
    ip = (WORD)tmp;
    cycle_count-=15;
}

static void i_jmp_d8(void)    /* Opcode 0xeb */
{
    int tmp = (int)((INT8)FETCH);
    ip = (WORD)(ip+tmp);
    cycle_count-=15;
}

static void i_inaldx(void)    /* Opcode 0xec */
{
    cycle_count-=8;
    regs.b[AL] = read_port(regs.w[DX]);
}

static void i_inaxdx(void)    /* Opcode 0xed */
{
    unsigned port = regs.w[DX];

    cycle_count-=12;
    regs.b[AL] = read_port(port);
    regs.b[AH] = read_port(port+1);
}

static void i_outdxal(void)    /* Opcode 0xee */
{
    cycle_count-=8;
    write_port(regs.w[DX], regs.b[AL]);
}

static void i_outdxax(void)    /* Opcode 0xef */
{
    unsigned port = regs.w[DX];

    cycle_count-=12;
    write_port(port, regs.b[AL]);
    write_port(port+1, regs.b[AH]);
}

static void i_lock(void)    /* Opcode 0xf0 */
{
    cycle_count-=2;
    instruction[FETCH]();  /* un-interruptible */
}

static void rep(int flagval)
{
    /* Handles rep- and repnz- prefixes. flagval is the value of ZF for the
       loop  to continue for CMPS and SCAS instructions. */

    unsigned next = FETCH;
    unsigned count = regs.w[CX];

    switch(next)
    {
    case 0x26:  /* ES: */
        seg_prefix=TRUE;
        prefix_base=base[ES];
	cycle_count-=2;
	rep(flagval);
	break;
    case 0x2e:  /* CS: */
        seg_prefix=TRUE;
        prefix_base=base[CS];
	cycle_count-=2;
	rep(flagval);
	break;
    case 0x36:  /* SS: */
        seg_prefix=TRUE;
        prefix_base=base[SS];
	cycle_count-=2;
	rep(flagval);
	break;
    case 0x3e:  /* DS: */
        seg_prefix=TRUE;
        prefix_base=base[DS];
	cycle_count-=2;
	rep(flagval);
	break;
    case 0x6c:  /* REP INSB */
	cycle_count-=9-count;
	for (; count > 0; count--)
            i_insb();
	regs.w[CX]=count;
	break;
    case 0x6d:  /* REP INSW */
	cycle_count-=9-count;
	for (; count > 0; count--)
            i_insw();
	regs.w[CX]=count;
	break;
    case 0x6e:  /* REP OUTSB */
	cycle_count-=9-count;
	for (; count > 0; count--)
            i_outsb();
	regs.w[CX]=count;
	break;
    case 0x6f:  /* REP OUTSW */
	cycle_count-=9-count;
	for (; count > 0; count--)
            i_outsw();
	regs.w[CX]=count;
	break;
    case 0xa4:  /* REP MOVSB */
	cycle_count-=9-count;
	for (; count > 0; count--)
	    i_movsb();
	regs.w[CX]=count;
	break;
    case 0xa5:  /* REP MOVSW */
	cycle_count-=9-count;
	for (; count > 0; count--)
	    i_movsw();
	regs.w[CX]=count;
	break;
    case 0xa6:  /* REP(N)E CMPSB */
	cycle_count-=9;
	for (ZeroVal = !flagval; (ZF == flagval) && (count > 0); count--)
	    i_cmpsb();
	regs.w[CX]=count;
	break;
    case 0xa7:  /* REP(N)E CMPSW */
	cycle_count-=9;
	for (ZeroVal = !flagval; (ZF == flagval) && (count > 0); count--)
	    i_cmpsw();
	regs.w[CX]=count;
	break;
    case 0xaa:  /* REP STOSB */
	cycle_count-=9-count;
	for (; count > 0; count--)
	    i_stosb();
	regs.w[CX]=count;
	break;
    case 0xab:  /* REP STOSW */
	cycle_count-=9-count;
	for (; count > 0; count--)
	    i_stosw();
	regs.w[CX]=count;
	break;
    case 0xac:  /* REP LODSB */
	cycle_count-=9;
	for (; count > 0; count--)
	    i_lodsb();
	regs.w[CX]=count;
	break;
    case 0xad:  /* REP LODSW */
	cycle_count-=9;
	for (; count > 0; count--)
	    i_lodsw();
	regs.w[CX]=count;
	break;
    case 0xae:  /* REP(N)E SCASB */
	cycle_count-=9;
	for (ZeroVal = !flagval; (ZF == flagval) && (count > 0); count--)
	    i_scasb();
	regs.w[CX]=count;
	break;
    case 0xaf:  /* REP(N)E SCASW */
	cycle_count-=9;
	for (ZeroVal = !flagval; (ZF == flagval) && (count > 0); count--)
	    i_scasw();
	regs.w[CX]=count;
	break;
    default:
	instruction[next]();
    }
}

static void i_repne(void)    /* Opcode 0xf2 */
{
    rep(0);
}

static void i_repe(void)    /* Opcode 0xf3 */
{
    rep(1);
}

static void i_hlt(void)    /* Opcode 0xf4 */
{
    cycle_count=0;
}

static void i_cmc(void)    /* Opcode 0xf5 */
{
    cycle_count-=2;
    CarryVal = !CF;
}

static void i_f6pre(void)
{
	/* Opcode 0xf6 */
    unsigned ModRM = FETCH;
    unsigned tmp = (unsigned)GetRMByte(ModRM);
    unsigned tmp2;


    switch (ModRM & 0x38)
    {
    case 0x00:  /* TEST Eb, data8 */
    case 0x08:  /* ??? */
	cycle_count-=5;
	tmp &= FETCH;

	CarryVal = OverVal = AuxVal = 0;
	SetSZPF_Byte(tmp);
	break;

    case 0x10:  /* NOT Eb */
	cycle_count-=3;
	PutbackRMByte(ModRM,~tmp);
	break;

    case 0x18:  /* NEG Eb */
	cycle_count-=3;
        tmp2=0;
        SUBB(tmp2,tmp);
        PutbackRMByte(ModRM,tmp2);
	break;
    case 0x20:  /* MUL AL, Eb */
	cycle_count-=77;
	{
	    UINT16 result;
	    tmp2 = regs.b[AL];

	    SetSF((INT8)tmp2);
	    SetPF(tmp2);

	    result = (UINT16)tmp2*tmp;
	    regs.w[AX]=(WORD)result;

	    SetZF(regs.w[AX]);
	    CarryVal = OverVal = (regs.b[AH] != 0);
	}
	break;
    case 0x28:  /* IMUL AL, Eb */
	cycle_count-=80;
	{
	    INT16 result;

	    tmp2 = (unsigned)regs.b[AL];

	    SetSF((INT8)tmp2);
	    SetPF(tmp2);

	    result = (INT16)((INT8)tmp2)*(INT16)((INT8)tmp);
	    regs.w[AX]=(WORD)result;

	    SetZF(regs.w[AX]);

	    CarryVal = OverVal = (result >> 7 != 0) && (result >> 7 != -1);
	}
	break;
    case 0x30:  /* DIV AL, Ew */
	cycle_count-=90;
	{
	    UINT16 result;

	    result = regs.w[AX];

	    if (tmp)
	    {
                if ((result / tmp) > 0xff)
                {
                        I86_interrupt(0);
                        break;
                }
                else
                {
                        regs.b[AH] = result % tmp;
                        regs.b[AL] = result / tmp;
                }

	    }
	    else
	    {
                I86_interrupt(0);
                break;
	    }
	}
	break;
    case 0x38:  /* IDIV AL, Ew */
	cycle_count-=106;
	{

	    INT16 result;

	    result = regs.w[AX];

	    if (tmp)
	    {
                tmp2 = result % (INT16)((INT8)tmp);

                if ((result /= (INT16)((INT8)tmp)) > 0xff)
                {
                        I86_interrupt(0);
                        break;
                }
                else
                {
                        regs.b[AL] = result;
                        regs.b[AH] = tmp2;
                }
	    }
	    else
	    {
                I86_interrupt(0);
                break;
	    }
	}
	break;
    }
}


static void i_f7pre(void)
{
	/* Opcode 0xf7 */
    unsigned ModRM = FETCH;
    unsigned tmp = GetRMWord(ModRM);
    unsigned tmp2;


    switch (ModRM & 0x38)
    {
    case 0x00:  /* TEST Ew, data16 */
    case 0x08:  /* ??? */
	cycle_count-=3;
	tmp2 = FETCH;
	tmp2 += FETCH << 8;

	tmp &= tmp2;

	CarryVal = OverVal = AuxVal = 0;
	SetSZPF_Word(tmp);
	break;

    case 0x10:  /* NOT Ew */
	cycle_count-=3;
	tmp = ~tmp;
	PutbackRMWord(ModRM,tmp);
	break;

    case 0x18:  /* NEG Ew */
	cycle_count-=3;
        tmp2 = 0;
        SUBW(tmp2,tmp);
        PutbackRMWord(ModRM,tmp2);
	break;
    case 0x20:  /* MUL AX, Ew */
	cycle_count-=129;
	{
	    UINT32 result;
	    tmp2 = regs.w[AX];

	    SetSF((INT16)tmp2);
	    SetPF(tmp2);

	    result = (UINT32)tmp2*tmp;
	    regs.w[AX]=(WORD)result;
            result >>= 16;
	    regs.w[DX]=result;

	    SetZF(regs.w[AX] | regs.w[DX]);
	    CarryVal = OverVal = (regs.w[DX] != 0);
	}
	break;

    case 0x28:  /* IMUL AX, Ew */
	cycle_count-=150;
	{
	    INT32 result;

	    tmp2 = regs.w[AX];

	    SetSF((INT16)tmp2);
	    SetPF(tmp2);

	    result = (INT32)((INT16)tmp2)*(INT32)((INT16)tmp);
	    CarryVal = OverVal = (result >> 15 != 0) && (result >> 15 != -1);

	    regs.w[AX]=(WORD)result;
	    result = (WORD)(result >> 16);
	    regs.w[DX]=result;

	    SetZF(regs.w[AX] | regs.w[DX]);
	}
	break;
    case 0x30:  /* DIV AX, Ew */
	cycle_count-=158;
	{
	    UINT32 result;

	    result = (regs.w[DX] << 16) + regs.w[AX];

	    if (tmp)
	    {
                tmp2 = result % tmp;
                if ((result / tmp) > 0xffff)
                {
                        I86_interrupt(0);
                        break;
                }
                else
                {
                        regs.w[DX]=tmp2;
                        result /= tmp;
                        regs.w[AX]=result;
                }
	    }
	    else
	    {
                I86_interrupt(0);
                break;
	    }
	}
	break;
    case 0x38:  /* IDIV AX, Ew */
	cycle_count-=180;
	{
	    INT32 result;

	    result = (regs.w[DX] << 16) + regs.w[AX];

	    if (tmp)
	    {
                tmp2 = result % (INT32)((INT16)tmp);

                if ((result /= (INT32)((INT16)tmp)) > 0xffff)
                {
                        I86_interrupt(0);
                        break;
                }
                else
                {
                        regs.w[AX]=result;
                        regs.w[DX]=tmp2;
                }
	    }
	    else
	    {
                I86_interrupt(0);
                break;
	    }
	}
	break;
    }
}


static void i_clc(void)    /* Opcode 0xf8 */
{
    cycle_count-=2;
    CarryVal = 0;
}

static void i_stc(void)    /* Opcode 0xf9 */
{
    cycle_count-=2;
    CarryVal = 1;
}

static void i_cli(void)    /* Opcode 0xfa */
{
    cycle_count-=2;
    IF = 0;
}

static void i_sti(void)    /* Opcode 0xfb */
{
    cycle_count-=2;
    IF = 1;
}

static void i_cld(void)    /* Opcode 0xfc */
{
    cycle_count-=2;
    DF = 0;
}

static void i_std(void)    /* Opcode 0xfd */
{
    cycle_count-=2;
    DF = 1;
}

static void i_fepre(void)    /* Opcode 0xfe */
{
    unsigned ModRM = FETCH;
    unsigned tmp = GetRMByte(ModRM);
    unsigned tmp1;

    cycle_count-=3; /* 2 if dest is in memory */
    if ((ModRM & 0x38) == 0)  /* INC eb */
    {
	tmp1 = tmp+1;
	SetOFB_Add(tmp1,tmp,1);
    }
    else  /* DEC eb */
    {
	tmp1 = tmp-1;
	SetOFB_Sub(tmp1,1,tmp);
    }

    SetAF(tmp1,tmp,1);
    SetSZPF_Byte(tmp1);

    PutbackRMByte(ModRM,(BYTE)tmp1);
}


static void i_ffpre(void)    /* Opcode 0xff */
{
    unsigned ModRM = FETCH;
    unsigned tmp;
    unsigned tmp1;

    switch(ModRM & 0x38)
    {
    case 0x00:  /* INC ew */
	cycle_count-=3; /* 2 if dest is in memory */
	tmp = GetRMWord(ModRM);
	tmp1 = tmp+1;

	SetOFW_Add(tmp1,tmp,1);
	SetAF(tmp1,tmp,1);
	SetSZPF_Word(tmp1);

	PutbackRMWord(ModRM,(WORD)tmp1);
	break;

    case 0x08:  /* DEC ew */
	cycle_count-=3; /* 2 if dest is in memory */
	tmp = GetRMWord(ModRM);
	tmp1 = tmp-1;

	SetOFW_Sub(tmp1,1,tmp);
	SetAF(tmp1,tmp,1);
	SetSZPF_Word(tmp1);

	PutbackRMWord(ModRM,(WORD)tmp1);
	break;

    case 0x10:  /* CALL ew */
	cycle_count-=9; /* 8 if dest is in memory */
	tmp = GetRMWord(ModRM);
	PUSH(ip);
	ip = (WORD)tmp;
	break;

	case 0x18:  /* CALL FAR ea */
	cycle_count-=11;
	PUSH(sregs[CS]);
	PUSH(ip);
	ip = GetRMWord(ModRM);
	sregs[CS] = GetnextRMWord;
	base[CS] = SegBase(CS);
	break;

    case 0x20:  /* JMP ea */
	cycle_count-=11; /* 8 if address in memory */
	ip = GetRMWord(ModRM);
	break;

    case 0x28:  /* JMP FAR ea */
	cycle_count-=4;
	ip = GetRMWord(ModRM);
	sregs[CS] = GetnextRMWord;
	base[CS] = SegBase(CS);
	break;

    case 0x30:  /* PUSH ea */
	cycle_count-=3;
	tmp = GetRMWord(ModRM);
	PUSH(tmp);
	break;
    }
}


static void i_invalid(void)
{
    /* makes the cpu loops forever until user resets it */
    ip--;
    cycle_count-=10;
}


/* ASG 971222 -- added these interface functions */
void i86_SetRegs(i86_Regs *Regs)
{
	regs = Regs->regs;
	ip = Regs->ip;
	ExpandFlags(Regs->flags);
	sregs[CS] = Regs->sregs[CS];
	sregs[DS] = Regs->sregs[DS];
	sregs[ES] = Regs->sregs[ES];
	sregs[SS] = Regs->sregs[SS];
	int86_pending = Regs->pending_interrupts;

	base[CS] = SegBase(CS);
	base[DS] = SegBase(DS);
	base[ES] = SegBase(ES);
	base[SS] = SegBase(SS);
}


void i86_GetRegs(i86_Regs *Regs)
{
	Regs->regs = regs;
	Regs->ip = ip;
	Regs->flags = CompressFlags();
	Regs->sregs[CS] = sregs[CS];
	Regs->sregs[DS] = sregs[DS];
	Regs->sregs[ES] = sregs[ES];
	Regs->sregs[SS] = sregs[SS];
	Regs->pending_interrupts = int86_pending;
}


unsigned i86_GetPC(void)
{
	return ip;
}


void i86_Cause_Interrupt(int type)
{
	int86_pending = type;
}


void i86_Clear_Pending_Interrupts(void)
{
	int86_pending = 0;
}



int i86_Execute(int cycles)
{
/* ASG 971222    if (int86_pending) external_int();*/

    cycle_count=cycles;/* ASG 971222 cycles_per_run;*/
    while(cycle_count>0)
    {

	if (int86_pending) external_int();      /* ASG 971222 */

	seg_prefix=FALSE;
#if defined(BIGCASE) && !defined(RS6000)
  /* Some compilers cannot handle large case statements */
	switch(FETCH)
	{
	case 0x00:    i_add_br8(); break;
	case 0x01:    i_add_wr16(); break;
	case 0x02:    i_add_r8b(); break;
	case 0x03:    i_add_r16w(); break;
	case 0x04:    i_add_ald8(); break;
	case 0x05:    i_add_axd16(); break;
	case 0x06:    i_push_es(); break;
	case 0x07:    i_pop_es(); break;
	case 0x08:    i_or_br8(); break;
	case 0x09:    i_or_wr16(); break;
	case 0x0a:    i_or_r8b(); break;
	case 0x0b:    i_or_r16w(); break;
	case 0x0c:    i_or_ald8(); break;
	case 0x0d:    i_or_axd16(); break;
	case 0x0e:    i_push_cs(); break;
	case 0x0f:    i_invalid(); break;
	case 0x10:    i_adc_br8(); break;
	case 0x11:    i_adc_wr16(); break;
	case 0x12:    i_adc_r8b(); break;
	case 0x13:    i_adc_r16w(); break;
	case 0x14:    i_adc_ald8(); break;
	case 0x15:    i_adc_axd16(); break;
	case 0x16:    i_push_ss(); break;
	case 0x17:    i_pop_ss(); break;
	case 0x18:    i_sbb_br8(); break;
	case 0x19:    i_sbb_wr16(); break;
	case 0x1a:    i_sbb_r8b(); break;
	case 0x1b:    i_sbb_r16w(); break;
	case 0x1c:    i_sbb_ald8(); break;
	case 0x1d:    i_sbb_axd16(); break;
	case 0x1e:    i_push_ds(); break;
	case 0x1f:    i_pop_ds(); break;
	case 0x20:    i_and_br8(); break;
	case 0x21:    i_and_wr16(); break;
	case 0x22:    i_and_r8b(); break;
	case 0x23:    i_and_r16w(); break;
	case 0x24:    i_and_ald8(); break;
	case 0x25:    i_and_axd16(); break;
	case 0x26:    i_es(); break;
	case 0x27:    i_daa(); break;
	case 0x28:    i_sub_br8(); break;
	case 0x29:    i_sub_wr16(); break;
	case 0x2a:    i_sub_r8b(); break;
	case 0x2b:    i_sub_r16w(); break;
	case 0x2c:    i_sub_ald8(); break;
	case 0x2d:    i_sub_axd16(); break;
	case 0x2e:    i_cs(); break;
	case 0x2f:    i_das(); break;
	case 0x30:    i_xor_br8(); break;
	case 0x31:    i_xor_wr16(); break;
	case 0x32:    i_xor_r8b(); break;
	case 0x33:    i_xor_r16w(); break;
	case 0x34:    i_xor_ald8(); break;
	case 0x35:    i_xor_axd16(); break;
	case 0x36:    i_ss(); break;
	case 0x37:    i_aaa(); break;
	case 0x38:    i_cmp_br8(); break;
	case 0x39:    i_cmp_wr16(); break;
	case 0x3a:    i_cmp_r8b(); break;
	case 0x3b:    i_cmp_r16w(); break;
	case 0x3c:    i_cmp_ald8(); break;
	case 0x3d:    i_cmp_axd16(); break;
	case 0x3e:    i_ds(); break;
	case 0x3f:    i_aas(); break;
	case 0x40:    i_inc_ax(); break;
	case 0x41:    i_inc_cx(); break;
	case 0x42:    i_inc_dx(); break;
	case 0x43:    i_inc_bx(); break;
	case 0x44:    i_inc_sp(); break;
	case 0x45:    i_inc_bp(); break;
	case 0x46:    i_inc_si(); break;
	case 0x47:    i_inc_di(); break;
	case 0x48:    i_dec_ax(); break;
	case 0x49:    i_dec_cx(); break;
	case 0x4a:    i_dec_dx(); break;
	case 0x4b:    i_dec_bx(); break;
	case 0x4c:    i_dec_sp(); break;
	case 0x4d:    i_dec_bp(); break;
	case 0x4e:    i_dec_si(); break;
	case 0x4f:    i_dec_di(); break;
	case 0x50:    i_push_ax(); break;
	case 0x51:    i_push_cx(); break;
	case 0x52:    i_push_dx(); break;
	case 0x53:    i_push_bx(); break;
	case 0x54:    i_push_sp(); break;
	case 0x55:    i_push_bp(); break;
	case 0x56:    i_push_si(); break;
	case 0x57:    i_push_di(); break;
	case 0x58:    i_pop_ax(); break;
	case 0x59:    i_pop_cx(); break;
	case 0x5a:    i_pop_dx(); break;
	case 0x5b:    i_pop_bx(); break;
	case 0x5c:    i_pop_sp(); break;
	case 0x5d:    i_pop_bp(); break;
	case 0x5e:    i_pop_si(); break;
	case 0x5f:    i_pop_di(); break;
        case 0x60:    i_pusha(); break;
        case 0x61:    i_popa(); break;
        case 0x62:    i_bound(); break;
	case 0x63:    i_invalid(); break;
	case 0x64:    i_invalid(); break;
	case 0x65:    i_invalid(); break;
	case 0x66:    i_invalid(); break;
	case 0x67:    i_invalid(); break;
        case 0x68:    i_push_d16(); break;
        case 0x69:    i_imul_d16(); break;
        case 0x6a:    i_push_d8(); break;
        case 0x6b:    i_imul_d8(); break;
        case 0x6c:    i_insb(); break;
        case 0x6d:    i_insw(); break;
        case 0x6e:    i_outsb(); break;
        case 0x6f:    i_outsw(); break;
	case 0x70:    i_jo(); break;
	case 0x71:    i_jno(); break;
	case 0x72:    i_jb(); break;
	case 0x73:    i_jnb(); break;
	case 0x74:    i_jz(); break;
	case 0x75:    i_jnz(); break;
	case 0x76:    i_jbe(); break;
	case 0x77:    i_jnbe(); break;
	case 0x78:    i_js(); break;
	case 0x79:    i_jns(); break;
	case 0x7a:    i_jp(); break;
	case 0x7b:    i_jnp(); break;
	case 0x7c:    i_jl(); break;
	case 0x7d:    i_jnl(); break;
	case 0x7e:    i_jle(); break;
	case 0x7f:    i_jnle(); break;
	case 0x80:    i_80pre(); break;
	case 0x81:    i_81pre(); break;
	case 0x82:    i_invalid(); break;
	case 0x83:    i_83pre(); break;
	case 0x84:    i_test_br8(); break;
	case 0x85:    i_test_wr16(); break;
	case 0x86:    i_xchg_br8(); break;
	case 0x87:    i_xchg_wr16(); break;
	case 0x88:    i_mov_br8(); break;
	case 0x89:    i_mov_wr16(); break;
	case 0x8a:    i_mov_r8b(); break;
	case 0x8b:    i_mov_r16w(); break;
	case 0x8c:    i_mov_wsreg(); break;
	case 0x8d:    i_lea(); break;
	case 0x8e:    i_mov_sregw(); break;
	case 0x8f:    i_popw(); break;
	case 0x90:    i_nop(); break;
	case 0x91:    i_xchg_axcx(); break;
	case 0x92:    i_xchg_axdx(); break;
	case 0x93:    i_xchg_axbx(); break;
	case 0x94:    i_xchg_axsp(); break;
	case 0x95:    i_xchg_axbp(); break;
	case 0x96:    i_xchg_axsi(); break;
	case 0x97:    i_xchg_axdi(); break;
	case 0x98:    i_cbw(); break;
	case 0x99:    i_cwd(); break;
	case 0x9a:    i_call_far(); break;
	case 0x9b:    i_wait(); break;
	case 0x9c:    i_pushf(); break;
	case 0x9d:    i_popf(); break;
	case 0x9e:    i_sahf(); break;
	case 0x9f:    i_lahf(); break;
	case 0xa0:    i_mov_aldisp(); break;
	case 0xa1:    i_mov_axdisp(); break;
	case 0xa2:    i_mov_dispal(); break;
	case 0xa3:    i_mov_dispax(); break;
	case 0xa4:    i_movsb(); break;
	case 0xa5:    i_movsw(); break;
	case 0xa6:    i_cmpsb(); break;
	case 0xa7:    i_cmpsw(); break;
	case 0xa8:    i_test_ald8(); break;
	case 0xa9:    i_test_axd16(); break;
	case 0xaa:    i_stosb(); break;
	case 0xab:    i_stosw(); break;
	case 0xac:    i_lodsb(); break;
	case 0xad:    i_lodsw(); break;
	case 0xae:    i_scasb(); break;
	case 0xaf:    i_scasw(); break;
	case 0xb0:    i_mov_ald8(); break;
	case 0xb1:    i_mov_cld8(); break;
	case 0xb2:    i_mov_dld8(); break;
	case 0xb3:    i_mov_bld8(); break;
	case 0xb4:    i_mov_ahd8(); break;
	case 0xb5:    i_mov_chd8(); break;
	case 0xb6:    i_mov_dhd8(); break;
	case 0xb7:    i_mov_bhd8(); break;
	case 0xb8:    i_mov_axd16(); break;
	case 0xb9:    i_mov_cxd16(); break;
	case 0xba:    i_mov_dxd16(); break;
	case 0xbb:    i_mov_bxd16(); break;
	case 0xbc:    i_mov_spd16(); break;
	case 0xbd:    i_mov_bpd16(); break;
	case 0xbe:    i_mov_sid16(); break;
	case 0xbf:    i_mov_did16(); break;
        case 0xc0:    i_rotshft_bd8(); break;
        case 0xc1:    i_rotshft_wd8(); break;
	case 0xc2:    i_ret_d16(); break;
	case 0xc3:    i_ret(); break;
	case 0xc4:    i_les_dw(); break;
	case 0xc5:    i_lds_dw(); break;
	case 0xc6:    i_mov_bd8(); break;
	case 0xc7:    i_mov_wd16(); break;
        case 0xc8:    i_enter(); break;
        case 0xc9:    i_leave(); break;
	case 0xca:    i_retf_d16(); break;
	case 0xcb:    i_retf(); break;
	case 0xcc:    i_int3(); break;
	case 0xcd:    i_int(); break;
	case 0xce:    i_into(); break;
	case 0xcf:    i_iret(); break;
        case 0xd0:    i_rotshft_b(); break;
        case 0xd1:    i_rotshft_w(); break;
        case 0xd2:    i_rotshft_bcl(); break;
        case 0xd3:    i_rotshft_wcl(); break;
	case 0xd4:    i_aam(); break;
	case 0xd5:    i_aad(); break;
	case 0xd6:    i_invalid(); break;
	case 0xd7:    i_xlat(); break;
	case 0xd8:    i_escape(); break;
	case 0xd9:    i_escape(); break;
	case 0xda:    i_escape(); break;
	case 0xdb:    i_escape(); break;
	case 0xdc:    i_escape(); break;
	case 0xdd:    i_escape(); break;
	case 0xde:    i_escape(); break;
	case 0xdf:    i_escape(); break;
	case 0xe0:    i_loopne(); break;
	case 0xe1:    i_loope(); break;
	case 0xe2:    i_loop(); break;
	case 0xe3:    i_jcxz(); break;
	case 0xe4:    i_inal(); break;
	case 0xe5:    i_inax(); break;
	case 0xe6:    i_outal(); break;
	case 0xe7:    i_outax(); break;
	case 0xe8:    i_call_d16(); break;
	case 0xe9:    i_jmp_d16(); break;
	case 0xea:    i_jmp_far(); break;
	case 0xeb:    i_jmp_d8(); break;
	case 0xec:    i_inaldx(); break;
	case 0xed:    i_inaxdx(); break;
	case 0xee:    i_outdxal(); break;
	case 0xef:    i_outdxax(); break;
	case 0xf0:    i_lock(); break;
	case 0xf1:    i_invalid(); break;
	case 0xf2:    i_repne(); break;
	case 0xf3:    i_repe(); break;
	case 0xf4:    i_hlt(); break;
	case 0xf5:    i_cmc(); break;
	case 0xf6:    i_f6pre(); break;
	case 0xf7:    i_f7pre(); break;
	case 0xf8:    i_clc(); break;
	case 0xf9:    i_stc(); break;
	case 0xfa:    i_cli(); break;
	case 0xfb:    i_sti(); break;
	case 0xfc:    i_cld(); break;
	case 0xfd:    i_std(); break;
	case 0xfe:    i_fepre(); break;
	case 0xff:    i_ffpre(); break;
	};
#else
	instruction[FETCH]();
#endif
    }
/* ASG 971222    int86_pending=I86_NMI_INT;*/
	return cycles - cycle_count;
}
