/*** m6808: Portable 6808 emulator ******************************************

    m6808.c

	References:

		6809 Simulator V09, By L.C. Benschop, Eidnhoven The Netherlands.

		m6809: Portable 6809 emulator, DS (6809 code in MAME, derived from
			the 6809 Simulator V09)

		6809 Microcomputer Programming & Interfacing with Experiments"
			by Andrew C. Staugaard, Jr.; Howard W. Sams & Co., Inc.

	System dependencies:	word must be 16 bit unsigned int
							byte must be 8 bit unsigned int
							long must be more than 16 bits
							arrays up to 65536 bytes must be supported
							machine must be twos complement

*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "m6808.h"
#include "driver.h"

/* 6808 registers */
static byte cc,areg,breg,unused1;
static word xreg,sreg,pcreg;

static word eaddr; /* effective address */
static int pending_interrupts;

/* public globals */
int	m6808_ICount=50000;
int m6808_Flags;	/* flags for speed optimization */

/* handlers for speed optimization */
static int (*rd_s_handler)(int);
static int (*rd_s_handler_wd)(int);
static void (*wr_s_handler)(int,int);
static void (*wr_s_handler_wd)(int,int);

/* DS -- THESE ARE RE-DEFINED IN m6808.h TO RAM, ROM or FUNCTIONS IN cpuintrf.c */
#define M_RDMEM(A)      M6808_RDMEM(A)
#define M_WRMEM(A,V)    M6808_WRMEM(A,V)
#define M_RDOP(A)       M6808_RDOP(A)
#define M_RDOP_ARG(A)   M6808_RDOP_ARG(A)

/* macros to access memory */
#define IMMBYTE(b) {b=M_RDOP_ARG(pcreg++);}
#define IMMWORD(w) {w = (M_RDOP_ARG(pcreg)<<8) + M_RDOP_ARG(pcreg+1); pcreg+=2;}

#define PUSHBYTE(b) {(*wr_s_handler)(sreg,b);sreg--;}
#define PUSHWORD(w) {sreg--;(*wr_s_handler_wd)(sreg,w);sreg--;}
#define PULLBYTE(b) {sreg++;b=(*rd_s_handler)(sreg);}
#define PULLWORD(w) {sreg++;w=(*rd_s_handler_wd)(sreg);sreg++;}

/* CC masks						  HI NZVC
								7654 3210	*/
#define CLR_HNZVC	cc&=0xd0
#define CLR_NZV		cc&=0xf1
#define CLR_HNZC	cc&=0xd2
#define CLR_NZVC	cc&=0xf0
#define CLR_Z		cc&=0xfb
#define CLR_NZC		cc&=0xf2
#define CLR_ZC		cc&=0xfa

/* macros for CC -- CC bits affected should be reset before calling */
#define SET_Z(a)		if(!a)SEZ
#define SET_Z8(a)		SET_Z((byte)a)
#define SET_Z16(a)		SET_Z((word)a)
#define SET_N8(a)		cc|=((a&0x80)>>4)
#define SET_N16(a)		cc|=((a&0x8000)>>12)
#define SET_H(a,b,r)	cc|=(((a^b^r)&0x10)<<1)
#define SET_C8(a)		cc|=((a&0x100)>>8)
#define SET_C16(a)		cc|=((a&0x10000)>>16)
#define SET_V8(a,b,r)	cc|=(((a^b^r^(r>>1))&0x80)>>6)
#define SET_V16(a,b,r)	cc|=(((a^b^r^(r>>1))&0x8000)>>14)

static byte flags8i[256]=	/* increment */
{
0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x0a,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08
};
static byte flags8d[256]= /* decrement */
{
0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08
};
#define SET_FLAGS8I(a)		{cc|=flags8i[(a)&0xff];}
#define SET_FLAGS8D(a)		{cc|=flags8d[(a)&0xff];}

/* combos */
#define SET_NZ8(a)			{SET_N8(a);SET_Z(a);}
#define SET_NZ16(a)			{SET_N16(a);SET_Z(a);}
#define SET_FLAGS8(a,b,r)	{SET_N8(r);SET_Z8(r);SET_V8(a,b,r);SET_C8(r);}
#define SET_FLAGS16(a,b,r)	{SET_N16(r);SET_Z16(r);SET_V16(a,b,r);SET_C16(r);}

/* for treating an unsigned byte as a signed word */
#define SIGNED(b) ((word)(b&0x80?b|0xff00:b))

/* macros to access dreg */
#define GETDREG ((areg<<8)|breg)
#define SETDREG(n) {areg=(n)>>8;breg=(n);}

/* Macros for addressing modes */
#define DIRECT IMMBYTE(eaddr)
#define IMM8 eaddr=pcreg++
#define IMM16 {eaddr=pcreg;pcreg+=2;}
#define EXTENDED IMMWORD(eaddr)
#define INDEXED {eaddr=xreg+(byte)M_RDOP(pcreg++);}

/* macros to set status flags */
#define SEC cc|=0x01
#define CLC cc&=0xfe
#define SEZ cc|=0x04
#define CLZ cc&=0xfb
#define SEN cc|=0x08
#define CLN cc&=0xf7
#define SEV cc|=0x02
#define CLV cc&=0xfd
#define SEH cc|=0x20
#define CLH cc&=0xdf
#define SEI cc|=0x10
#define CLI cc&=~0x10

/* macros for convenience */
#define DIRBYTE(b) {DIRECT;b=M_RDMEM(eaddr);}
#define DIRWORD(w) {DIRECT;w=M_RDMEM_WORD(eaddr);}
#define EXTBYTE(b) {EXTENDED;b=M_RDMEM(eaddr);}
#define EXTWORD(w) {EXTENDED;w=M_RDMEM_WORD(eaddr);}

#define IDXBYTE(b) {INDEXED;b=M_RDMEM(eaddr);}
#define IDXWORD(w) {INDEXED;w=M_RDMEM_WORD(eaddr);}

/* Macros for branch instructions */
#define BRANCH(f) {IMMBYTE(t);if(f){pcreg+=SIGNED(t);change_pc(pcreg);}}
#define LBRANCH(f) {IMMWORD(t);if(f){pcreg+=t;change_pc(pcreg);}}
#define NXORV  ((cc&0x08)^((cc&0x02)<<2))

/* what they say it is ... */
static unsigned char cycles1[] =
{
		/*	0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
  /*0*/		0, 2, 0, 0, 0, 0, 2, 2, 4, 4, 2, 2, 2, 2, 2, 2,
  /*1*/		2, 2, 0, 0, 0, 0, 2, 2, 0, 2, 0, 2, 0, 0, 0, 0,
  /*2*/		4, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  /*3*/		4, 4, 4, 4, 4, 4, 4, 4, 0, 5, 0,19, 0, 0, 9,12,
  /*4*/		2, 0, 0, 2, 2, 0, 2, 2, 2, 2, 2, 0, 2, 2, 0, 2,
  /*5*/		2, 0, 0, 2, 2, 0, 2, 2, 2, 2, 2, 0, 2, 2, 0, 2,
  /*6*/		7, 0, 0, 7, 7, 0, 7, 7, 7, 7, 7, 0, 7, 7, 4, 7,
  /*7*/		6, 0, 0, 6, 6, 0, 6, 6, 6, 6, 6, 0, 6, 6, 3, 6,
  /*8*/		2, 2, 2, 0, 2, 2, 2, 0, 2, 2, 2, 2, 3, 8, 3, 0,
  /*9*/		3, 3, 3, 0, 3, 3, 3, 4, 3, 3, 3, 3, 4, 0, 4, 5,
  /*A*/		5, 5, 5, 0, 5, 5, 5, 6, 5, 5, 5, 5, 6, 8, 6, 7,
  /*B*/		4, 4, 4, 0, 4, 4, 4, 5, 4, 4, 4, 4, 5, 9, 5, 6,
  /*C*/		2, 2, 2, 0, 2, 2, 2, 0, 2, 2, 2, 2, 0, 0, 3, 0,
  /*D*/		3, 3, 3, 0, 3, 3, 3, 4, 3, 3, 3, 3, 0, 0, 4, 5,
  /*E*/		5, 5, 5, 0, 5, 5, 5, 6, 5, 5, 5, 5, 4, 0, 6, 7,
  /*F*/		4, 4, 4, 0, 4, 4, 4, 5, 4, 4, 4, 4, 0, 0, 5, 6,
};


static int rd_slow( int addr )
{
    return M_RDMEM(addr);
}

static int rd_slow_wd( int addr )
{
    return( (M_RDMEM(addr)<<8) | (M_RDMEM((addr+1)&0xffff)) );
}

static int rd_fast( int addr )
{
	extern unsigned char *RAM;

	return RAM[addr];
}

static int rd_fast_wd( int addr )
{
	extern unsigned char *RAM;

	return( (RAM[addr]<<8) | (RAM[(addr+1)&0xffff]) );
}

static void wr_slow( int addr, int v )
{
	M_WRMEM(addr,v);
}

static void wr_slow_wd( int addr, int v )
{
	M_WRMEM(addr,v>>8);
	M_WRMEM(((addr)+1)&0xFFFF,v&255);
}

static void wr_fast( int addr, int v )
{
	extern unsigned char *RAM;

	RAM[addr] = v;
}

static void wr_fast_wd( int addr, int v )
{
	extern unsigned char *RAM;

	RAM[addr] = v>>8;
	RAM[(addr+1)&0xffff] = v&255;
}

INLINE unsigned M_RDMEM_WORD (dword A)
{
	int i;

    i = M_RDMEM(A)<<8;
    i |= M_RDMEM(((A)+1)&0xFFFF);
	return i;
}

INLINE void M_WRMEM_WORD (dword A,word V)
{
	M_WRMEM (A,V>>8);
	M_WRMEM (((A)+1)&0xFFFF,V&255);
}


/****************************************************************************/
/* Set all registers to given values                                        */
/****************************************************************************/
void m6808_SetRegs(m6808_Regs *Regs)
{
	pcreg = Regs->pc;change_pc(pcreg);
	sreg = Regs->s;
	xreg = Regs->x;
	areg = Regs->a;
	breg = Regs->b;
	cc = Regs->cc;

	pending_interrupts = Regs->pending_interrupts;
}


/****************************************************************************/
/* Get all registers in given buffer                                        */
/****************************************************************************/
void m6808_GetRegs(m6808_Regs *Regs)
{
	Regs->pc = pcreg;
	Regs->s = sreg;
	Regs->x = xreg;
	Regs->a = areg;
	Regs->b = breg;
	Regs->cc = cc;

	Regs->pending_interrupts = pending_interrupts;
}


/****************************************************************************/
/* Return program counter                                                   */
/****************************************************************************/
unsigned m6808_GetPC(void)
{
	return pcreg;
}


/* Generate interrupts */
INLINE void Interrupt(void)
{
	if ((pending_interrupts & M6808_INT_NMI) != 0)
	{
		pending_interrupts &= ~M6808_INT_NMI;

		/* NMI */
		PUSHWORD(pcreg);
		PUSHWORD(xreg);
		PUSHBYTE(breg);
		PUSHBYTE(areg);
		PUSHBYTE(cc);
		SEI;
		pcreg=M_RDMEM_WORD(0xfffc);change_pc(pcreg);
		m6808_ICount -= 19;
	} else
		if  ( ( cc & 0x10 ) == 0 ) {
			if ((pending_interrupts & M6808_INT_IRQ) != 0) {
				pending_interrupts &= ~M6808_INT_IRQ;

				/* standard IRQ */
				PUSHWORD(pcreg)
				PUSHWORD(xreg)
				PUSHBYTE(breg)
				PUSHBYTE(areg)
				PUSHBYTE(cc)
				SEI;
				pcreg=M_RDMEM_WORD(0xfff8);change_pc(pcreg);
				m6808_ICount -= 19;
			}
			else
			if ((pending_interrupts & M6808_INT_OCI) != 0) {
				pending_interrupts &= ~M6808_INT_OCI;

				/* standard IRQ */
				PUSHWORD(pcreg)
				PUSHWORD(xreg)
				PUSHBYTE(breg)
				PUSHBYTE(areg)
				PUSHBYTE(cc)
				SEI;
				pcreg=M_RDMEM_WORD(0xfff4);change_pc(pcreg);
				m6808_ICount -= 19;
			}
		}
}


void m6808_reset(void)
{
	pcreg = M_RDMEM_WORD(0xfffe);change_pc(pcreg);

	cc = 0x00;		/* Clear all flags */
	SEI;			/* IRQ disabled */
	areg = 0x00;	/* clear accumulator a */
	breg = 0x00;	/* clear accumulator b */
	m6808_Clear_Pending_Interrupts();

	/* default to unoptimized memory access */
	rd_s_handler = rd_slow;
	rd_s_handler_wd = rd_slow_wd;
	wr_s_handler = wr_slow;
	wr_s_handler_wd = wr_slow_wd;

	/* optimize memory access according to flags */
	if( m6808_Flags & M6808_FAST_S )
	{
		rd_s_handler=rd_fast; rd_s_handler_wd=rd_fast_wd;
		wr_s_handler=wr_fast; wr_s_handler_wd=wr_fast_wd;
	}
}


void m6808_Cause_Interrupt(int type)
{
	pending_interrupts |= type;
	if (pending_interrupts & M6808_WAI)
	{
		if ((pending_interrupts & M6808_INT_NMI) != 0)
			pending_interrupts &= ~M6808_WAI;
		else if ( (cc & 0x10) == 0 )
				pending_interrupts &= ~M6808_WAI;
	}
}


void m6808_Clear_Pending_Interrupts(void)
{
	pending_interrupts &= ~( M6808_INT_IRQ | M6808_INT_NMI | M6808_INT_OCI );
}


#include "6808ops.h"


/* execute instructions on this CPU until icount expires */
int m6808_execute(int cycles)
{
	byte ireg;
	m6808_ICount = cycles;

	if ((pending_interrupts & M6808_WAI) != 0)
	{
		m6808_ICount = 0;
		goto getout;
	}

	do
	{
		if (pending_interrupts != 0)
			Interrupt();

		ireg=M_RDOP(pcreg++);

		switch( ireg )
		{
			case 0x00: illegal(); break;
			case 0x01: nop(); break;
			case 0x02: illegal(); break;
			case 0x03: illegal(); break;
			case 0x04: lsrd(); /* 6803 only */; break;
			case 0x05: asld(); /* 6803 only */; break;
			case 0x06: tap(); break;
			case 0x07: tpa(); break;
			case 0x08: inx(); break;
			case 0x09: dex(); break;
			case 0x0A: CLV; break;
			case 0x0B: SEV; break;
			case 0x0C: CLC; break;
			case 0x0D: SEC; break;
			case 0x0E: CLI; break;
			case 0x0F: SEI; break;
			case 0x10: sba(); break;
			case 0x11: cba(); break;
			case 0x12: illegal(); break;
			case 0x13: illegal(); break;
			case 0x14: illegal(); break;
			case 0x15: illegal(); break;
			case 0x16: tab(); break;
			case 0x17: tba(); break;
			case 0x18: xgdx(); /* HD63701YO only */; break;
			case 0x19: daa(); break;
			case 0x1a: illegal(); break;
			case 0x1b: aba(); break;
			case 0x1c: illegal(); break;
			case 0x1d: illegal(); break;
			case 0x1e: illegal(); break;
			case 0x1f: illegal(); break;
			case 0x20: bra(); break;
			case 0x21: brn(); break;
			case 0x22: bhi(); break;
			case 0x23: bls(); break;
			case 0x24: bcc(); break;
			case 0x25: bcs(); break;
			case 0x26: bne(); break;
			case 0x27: beq(); break;
			case 0x28: bvc(); break;
			case 0x29: bvs(); break;
			case 0x2a: bpl(); break;
			case 0x2b: bmi(); break;
			case 0x2c: bge(); break;
			case 0x2d: blt(); break;
			case 0x2e: bgt(); break;
			case 0x2f: ble(); break;
			case 0x30: tsx(); break;
			case 0x31: ins(); break;
			case 0x32: pula(); break;
			case 0x33: pulb(); break;
			case 0x34: des(); break;
			case 0x35: txs(); break;
			case 0x36: psha(); break;
			case 0x37: pshb(); break;
			case 0x38: pulx(); /* 6803 only */ break;
			case 0x39: rts(); break;
			case 0x3a: abx(); /* 6803 only */ break;
			case 0x3b: rti(); break;
			case 0x3c: pshx(); /* 6803 only */ break;
			case 0x3d: mul(); /* 6803 only */ break;
			case 0x3e: wai(); break;
			case 0x3f: swi(); break;
			case 0x40: nega(); break;
			case 0x41: illegal(); break;
			case 0x42: illegal(); break;
			case 0x43: coma(); break;
			case 0x44: lsra(); break;
			case 0x45: illegal(); break;
			case 0x46: rora(); break;
			case 0x47: asra(); break;
			case 0x48: asla(); break;
			case 0x49: rola(); break;
			case 0x4a: deca(); break;
			case 0x4b: illegal(); break;
			case 0x4c: inca(); break;
			case 0x4d: tsta(); break;
			case 0x4e: illegal(); break;
			case 0x4f: clra(); break;
			case 0x50: negb(); break;
			case 0x51: illegal(); break;
			case 0x52: illegal(); break;
			case 0x53: comb(); break;
			case 0x54: lsrb(); break;
			case 0x55: illegal(); break;
			case 0x56: rorb(); break;
			case 0x57: asrb(); break;
			case 0x58: aslb(); break;
			case 0x59: rolb(); break;
			case 0x5a: decb(); break;
			case 0x5b: illegal(); break;
			case 0x5c: incb(); break;
			case 0x5d: tstb(); break;
			case 0x5e: illegal(); break;
			case 0x5f: clrb(); break;
			case 0x60: neg_ix(); break;
			case 0x61: aim_ix(); /* HD63701YO only */; break;
			case 0x62: oim_ix(); /* HD63701YO only */; break;
			case 0x63: com_ix(); break;
			case 0x64: lsr_ix(); break;
			case 0x65: eim_ix(); /* HD63701YO only */; break;
			case 0x66: ror_ix(); break;
			case 0x67: asr_ix(); break;
			case 0x68: asl_ix(); break;
			case 0x69: rol_ix(); break;
			case 0x6a: dec_ix(); break;
			case 0x6b: tim_ix(); /* HD63701YO only */; break;
			case 0x6c: inc_ix(); break;
			case 0x6d: tst_ix(); break;
			case 0x6e: jmp_ix(); break;
			case 0x6f: clr_ix(); break;
			case 0x70: neg_ex(); break;
			case 0x71: aim_di(); /* HD63701YO only */; break;
			case 0x72: oim_di(); /* HD63701YO only */; break;
			case 0x73: com_ex(); break;
			case 0x74: lsr_ex(); break;
			case 0x75: eim_di(); /* HD63701YO only */; break;
			case 0x76: ror_ex(); break;
			case 0x77: asr_ex(); break;
			case 0x78: asl_ex(); break;
			case 0x79: rol_ex(); break;
			case 0x7a: dec_ex(); break;
			case 0x7b: tim_di(); /* HD63701YO only */; break;
			case 0x7c: inc_ex(); break;
			case 0x7d: tst_ex(); break;
			case 0x7e: jmp_ex(); break;
			case 0x7f: clr_ex(); break;
			case 0x80: suba_im(); break;
			case 0x81: cmpa_im(); break;
			case 0x82: sbca_im(); break;
			case 0x83: subd_im(); /* 6803 only */ break;
			case 0x84: anda_im(); break;
			case 0x85: bita_im(); break;
			case 0x86: lda_im(); break;
			case 0x87: sta_im(); break;
			case 0x88: eora_im(); break;
			case 0x89: adca_im(); break;
			case 0x8a: ora_im(); break;
			case 0x8b: adda_im(); break;
			case 0x8c: cmpx_im(); break;
			case 0x8d: bsr(); break;
			case 0x8e: lds_im(); break;
			case 0x8f: sts_im(); /* orthogonality */ break;
			case 0x90: suba_di(); break;
			case 0x91: cmpa_di(); break;
			case 0x92: sbca_di(); break;
			case 0x93: subd_di(); /* 6803 only */ break;
			case 0x94: anda_di(); break;
			case 0x95: bita_di(); break;
			case 0x96: lda_di(); break;
			case 0x97: sta_di(); break;
			case 0x98: eora_di(); break;
			case 0x99: adca_di(); break;
			case 0x9a: ora_di(); break;
			case 0x9b: adda_di(); break;
			case 0x9c: cmpx_di(); break;
			case 0x9d: jsr_di(); break;
			case 0x9e: lds_di(); break;
			case 0x9f: sts_di(); break;
			case 0xa0: suba_ix(); break;
			case 0xa1: cmpa_ix(); break;
			case 0xa2: sbca_ix(); break;
			case 0xa3: subd_ix(); /* 6803 only */ break;
			case 0xa4: anda_ix(); break;
			case 0xa5: bita_ix(); break;
			case 0xa6: lda_ix(); break;
			case 0xa7: sta_ix(); break;
			case 0xa8: eora_ix(); break;
			case 0xa9: adca_ix(); break;
			case 0xaa: ora_ix(); break;
			case 0xab: adda_ix(); break;
			case 0xac: cmpx_ix(); break;
			case 0xad: jsr_ix(); break;
			case 0xae: lds_ix(); break;
			case 0xaf: sts_ix(); break;
			case 0xb0: suba_ex(); break;
			case 0xb1: cmpa_ex(); break;
			case 0xb2: sbca_ex(); break;
			case 0xb3: subd_ex(); /* 6803 only */ break;
			case 0xb4: anda_ex(); break;
			case 0xb5: bita_ex(); break;
			case 0xb6: lda_ex(); break;
			case 0xb7: sta_ex(); break;
			case 0xb8: eora_ex(); break;
			case 0xb9: adca_ex(); break;
			case 0xba: ora_ex(); break;
			case 0xbb: adda_ex(); break;
			case 0xbc: cmpx_ex(); break;
			case 0xbd: jsr_ex(); break;
			case 0xbe: lds_ex(); break;
			case 0xbf: sts_ex(); break;
			case 0xc0: subb_im(); break;
			case 0xc1: cmpb_im(); break;
			case 0xc2: sbcb_im(); break;
			case 0xc3: addd_im(); /* 6803 only */ break;
			case 0xc4: andb_im(); break;
			case 0xc5: bitb_im(); break;
			case 0xc6: ldb_im(); break;
			case 0xc7: stb_im(); break;
			case 0xc8: eorb_im(); break;
			case 0xc9: adcb_im(); break;
			case 0xca: orb_im(); break;
			case 0xcb: addb_im(); break;
			case 0xcc: ldd_im(); /* 6803 only */ break;
			case 0xcd: std_im(); /* 6803 only -- orthogonality */ break;
			case 0xce: ldx_im(); break;
			case 0xcf: stx_im(); break;
			case 0xd0: subb_di(); break;
			case 0xd1: cmpb_di(); break;
			case 0xd2: sbcb_di(); break;
			case 0xd3: addd_di(); /* 6803 only */ break;
			case 0xd4: andb_di(); break;
			case 0xd5: bitb_di(); break;
			case 0xd6: ldb_di(); break;
			case 0xd7: stb_di(); break;
			case 0xd8: eorb_di(); break;
			case 0xd9: adcb_di(); break;
			case 0xda: orb_di(); break;
			case 0xdb: addb_di(); break;
			case 0xdc: ldd_di(); /* 6803 only */ break;
			case 0xdd: std_di(); /* 6803 only */ break;
			case 0xde: ldx_di(); break;
			case 0xdf: stx_di(); break;
			case 0xe0: subb_ix(); break;
			case 0xe1: cmpb_ix(); break;
			case 0xe2: sbcb_ix(); break;
			case 0xe3: addd_ix(); /* 6803 only */ break;
			case 0xe4: andb_ix(); break;
			case 0xe5: bitb_ix(); break;
			case 0xe6: ldb_ix(); break;
			case 0xe7: stb_ix(); break;
			case 0xe8: eorb_ix(); break;
			case 0xe9: adcb_ix(); break;
			case 0xea: orb_ix(); break;
			case 0xeb: addb_ix(); break;
			case 0xec: ldd_ix(); /* 6803 only */ break;
			case 0xed: std_ix(); /* 6803 only */ break;
			case 0xee: ldx_ix(); break;
			case 0xef: stx_ix(); break;
			case 0xf0: subb_ex(); break;
			case 0xf1: cmpb_ex(); break;
			case 0xf2: sbcb_ex(); break;
			case 0xf3: addd_ex(); /* 6803 only */ break;
			case 0xf4: andb_ex(); break;
			case 0xf5: bitb_ex(); break;
			case 0xf6: ldb_ex(); break;
			case 0xf7: stb_ex(); break;
			case 0xf8: eorb_ex(); break;
			case 0xf9: adcb_ex(); break;
			case 0xfa: orb_ex(); break;
			case 0xfb: addb_ex(); break;
			case 0xfc: ldd_ex(); /* 6803 only */ break;
			case 0xfd: std_ex(); /* 6803 only */ break;
			case 0xfe: ldx_ex(); break;
			case 0xff: stx_ex(); break;
		}
		m6808_ICount -= cycles1[ireg];
	} while( m6808_ICount>0 );

getout:
	return cycles - m6808_ICount;
}
