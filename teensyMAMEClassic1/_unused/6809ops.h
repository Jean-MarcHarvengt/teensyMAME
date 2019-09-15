
/*

HNZVC

? = undefined
* = affected
- = unaffected
0 = cleared
1 = set
# = ccr directly affected by instruction
@ = special - carry set if bit 7 is set

*/

/* $00 NEG direct ?**** */
#define neg_di() \
{ \
	word r,t; \
	DIRBYTE(t); r=-t; \
	CLR_NZVC; SET_FLAGS8(0,t,r); \
	M_WRMEM(eaddr,r); \
}

/* $01 ILLEGAL */

/* $02 ILLEGAL */

/* $03 COM direct -**01 */
#define com_di() \
{ \
	byte t; \
	DIRBYTE(t); t = ~t; \
	CLR_NZV; SET_NZ8(t); SEC; \
	M_WRMEM(eaddr,t); \
} 

/* $04 LSR direct -0*-* */
#define lsr_di() \
{ \
	byte t; \
	DIRBYTE(t); CLR_NZC; cc|=(t&0x01); \
	t>>=1; SET_Z8(t); \
	M_WRMEM(eaddr,t); \
}

/* $05 ILLEGAL */

/* $06 ROR direct -**-* */
#define ror_di() \
{ \
	byte t,r; \
	DIRBYTE(t); r=(cc&0x01)<<7; \
	CLR_NZC; cc|=(t&0x01); \
	r |= t>>1; SET_NZ8(r); \
	M_WRMEM(eaddr,r); \
}

/* $07 ASR direct ?**-* */
#define asr_di() \
{ \
	byte t; \
	DIRBYTE(t); CLR_NZC; cc|=(t&0x01); \
	t>>=1; t|=((t&0x40)<<1); \
	SET_NZ8(t); \
	M_WRMEM(eaddr,t); \
}

/* $08 ASL direct ?**** */
#define asl_di() \
{ \
	word t,r; \
	DIRBYTE(t); r=t<<1; \
	CLR_NZVC; SET_FLAGS8(t,t,r); \
	M_WRMEM(eaddr,r); \
}

/* $09 ROL direct -**** */
#define rol_di() \
{ \
	word t,r; \
	DIRBYTE(t); r = cc&0x01; r |= t<<1; \
	CLR_NZVC; SET_FLAGS8(t,t,r); \
	M_WRMEM(eaddr,r); \
}

/* $0A DEC direct -***- */
#define dec_di() \
{ \
	byte t; \
	DIRBYTE(t); --t; \
	CLR_NZV; SET_FLAGS8D(t); \
	M_WRMEM(eaddr,t); \
}

/* $0B ILLEGAL */

/* $OC INC direct -***- */
#define inc_di() \
{ \
	byte t; \
	DIRBYTE(t); ++t; \
	CLR_NZV; SET_FLAGS8I(t); \
	M_WRMEM(eaddr,t); \
}

/* $OD TST direct -**0- */
#define tst_di() \
{ \
	byte t; \
	DIRBYTE(t); CLR_NZV; SET_NZ8(t); \
}

/* $0E JMP direct ----- */
#define jmp_di() \
{ \
	DIRECT; pcreg=eaddr;change_pc(pcreg); \
}

/* $0E JMP direct ----- */
#define jmp_di_slap() \
{ \
	DIRECT; pcreg=eaddr;cpu_setOPbase16(pcreg);  \
}

/* $0F CLR direct -0100 */
#define clr_di() \
{ \
	DIRECT; M_WRMEM(eaddr,0); \
	CLR_NZVC; SEZ; \
}

/* $10 FLAG */

/* $11 FLAG */

/* $12 NOP inherent ----- */

/* $13 SYNC inherent ----- */
#define sync6809() \
{ \
	m6809_ICount = 0; \
	pending_interrupts |= M6809_SYNC;	 \
}

/* $14 ILLEGAL */

/* $15 ILLEGAL */

/* $16 LBRA relative ----- */
#define lbra() \
{ \
	IMMWORD(eaddr); \
 \
	pcreg+=eaddr;change_pc(pcreg); \
 \
	if ( eaddr == 0xfffd )  \
		m6809_ICount = 0; \
}

/* $17 LBSR relative ----- */
#define lbsr() \
{ \
	IMMWORD(eaddr); PUSHWORD(pcreg); pcreg+=eaddr;change_pc(pcreg); \
} \

/* $18 ILLEGAL */

/* $19 DAA inherent (areg) -**0* */
#define daa() \
{ \
	byte msn, lsn; \
	word t, cf = 0; \
	msn=areg & 0xf0; lsn=areg & 0x0f; \
	if( lsn>0x09 || cc&0x20 ) cf |= 0x06; \
	if( msn>0x80 && lsn>0x09 ) cf |= 0x60; \
	if( msn>0x90 || cc&0x01 ) cf |= 0x60; \
	t = cf + areg; \
	CLR_NZV;  \
	SET_NZ8((byte)t); SET_C8(t); \
	areg = t; \
}

/* $1A ORCC immediate ##### */
#define orcc() \
{ \
	byte t; \
	IMMBYTE(t); cc|=t; \
}

/* $1B ILLEGAL */

/* $1C ANDCC immediate ##### */
#define andcc() \
{ \
	byte t; \
	IMMBYTE(t); cc&=t; \
}

/* $1D SEX inherent -**0- */
#define sex() \
{ \
	word t; \
	t = SIGNED(breg); SETDREG(t); \
	CLR_NZV; SET_NZ16(t); \
}

/* $1E EXG inherent ----- */
#define exg() \
{ \
	word t1=0,t2=0; \
	byte tb; \
 \
	IMMBYTE(tb); \
	 \
        if( (tb^(tb>>4)) & 0x08 )       /* HJB 990225: mixed 8/16 bit case? */ \
        { \
                /* transfer $ff to both registers */ \
                t1 = t2 = 0xff; \
        } \
        else \
        { \
                switch(tb>>4) { \
                        case  0: t1 = GETDREG;  break; \
                        case  1: t1 = xreg;  break; \
                        case  2: t1 = yreg;  break; \
                        case  3: t1 = ureg;  break; \
                        case  4: t1 = sreg;  break; \
                        case  5: t1 = pcreg; break; \
                        case  8: t1 = areg;  break; \
                        case  9: t1 = breg;  break; \
                        case 10: t1 = cc; break; \
                        case 11: t1 = dpreg; break; \
                        default: t1 = 0xff; \
                } \
                switch(tb&15) { \
                        case  0: t2 = GETDREG;  break; \
                        case  1: t2 = xreg;  break; \
                        case  2: t2 = yreg;  break; \
                        case  3: t2 = ureg;  break; \
                        case  4: t2 = sreg;  break; \
                        case  5: t2 = pcreg; break; \
                        case  8: t2 = areg;  break; \
                        case  9: t2 = breg;  break; \
                        case 10: t2 = cc; break; \
                        case 11: t2 = dpreg; break; \
                        default: t2 = 0xff; \
		} \
        } \
        switch(tb>>4) { \
                case  0: SETDREG(t2);  break; \
                case  1: xreg = t2;  break; \
                case  2: yreg = t2;  break; \
                case  3: ureg = t2;  break; \
                case  4: sreg = t2;  break; \
                case  5: pcreg = t2; if (m6809_slapstic) cpu_setOPbase16(pcreg); else change_pc(pcreg); break; \
                case  8: areg = t2;  break; \
                case  9: breg = t2;  break; \
                case 10: cc = t2; break; \
                case 11: dpreg = t2; break; \
        } \
        switch(tb&15) { \
                case  0: SETDREG(t1);  break; \
                case  1: xreg = t1;  break; \
                case  2: yreg = t1;  break; \
                case  3: ureg = t1;  break; \
                case  4: sreg = t1;  break; \
                case  5: pcreg = t1; if (m6809_slapstic) cpu_setOPbase16(pcreg); else change_pc(pcreg); break; \
                case  8: areg = t1;  break; \
                case  9: breg = t1;  break; \
                case 10: cc = t1; break; \
                case 11: dpreg = t1; break; \
        } \
}

/* $1F TFR inherent ----- */
#define tfr() \
{ \
	byte tb; \
	word t=0; \
 \
	IMMBYTE(tb); \
        if( (tb^(tb>>4)) & 0x08 )       /* HJB 990225: mixed 8/16 bit case? */ \
        { \
                /* transfer $ff to register */ \
                t = 0xff; \
	}    \
        else   \
        {        \
                switch(tb>>4) { \
                        case  0: t = GETDREG;  break; \
                        case  1: t = xreg;  break; \
                        case  2: t = yreg;  break; \
                        case  3: t = ureg;  break; \
                        case  4: t = sreg;  break; \
                        case  5: t = pcreg; break; \
                        case  8: t = areg;  break; \
                        case  9: t = breg;  break; \
                        case 10: t = cc; break; \
                        case 11: t = dpreg; break; \
                        default: t = 0xff; \
		}        \
        } \
        switch(tb&15) { \
                case  0: SETDREG(t);  break; \
                case  1: xreg = t;  break; \
                case  2: yreg = t;  break; \
                case  3: ureg = t;  break; \
                case  4: sreg = t;  break; \
                case  5: pcreg = t; if (m6809_slapstic) cpu_setOPbase16(pcreg); else change_pc(pcreg); break; \
                case  8: areg = t;  break; \
                case  9: breg = t;  break; \
                case 10: cc = t; break; \
                case 11: dpreg = t; break; \
    } \
}

/* $20 BRA relative ----- */
#define bra() \
{ \
	byte t; \
	IMMBYTE(t);pcreg+=SIGNED(t);change_pc(pcreg);	 \
	if (t==0xfe) m6809_ICount = 0; \
}

/* $21 BRN relative ----- */
#define brn() \
{ \
	byte t; \
	IMMBYTE(t); \
}

/* $1021 LBRN relative ----- */
#define lbrn() \
{ \
	word t; \
	IMMWORD(t); \
}

/* $22 BHI relative ----- */
#define bhi() \
{ \
	byte t; \
	BRANCH(!(cc&0x05)); \
}

/* $1022 LBHI relative ----- */
#define lbhi() \
{ \
	word t; \
	LBRANCH(!(cc&0x05)); \
}

/* $23 BLS relative ----- */
#define bls() \
{ \
	byte t; \
	BRANCH(cc&0x05); \
}

/* $1023 LBLS relative ----- */
#define lbls() \
{ \
	word t; \
	LBRANCH(cc&0x05); \
}

/* $24 BCC relative ----- */
#define bcc() \
{ \
	byte t; \
	BRANCH(!(cc&0x01)); \
}

/* $1024 LBCC relative ----- */
#define lbcc() \
{ \
	word t; \
	LBRANCH(!(cc&0x01)); \
}

/* $25 BCS relative ----- */
#define bcs() \
{ \
	byte t; \
	BRANCH(cc&0x01); \
}

/* $1025 LBCS relative ----- */
#define lbcs() \
{ \
	word t; \
	LBRANCH(cc&0x01); \
}

/* $26 BNE relative ----- */
#define bne() \
{ \
	byte t; \
	BRANCH(!(cc&0x04)); \
}

/* $1026 LBNE relative ----- */
#define lbne() \
{ \
	word t; \
	LBRANCH(!(cc&0x04)); \
}

/* $27 BEQ relative ----- */
#define beq() \
{ \
	byte t; \
	BRANCH(cc&0x04); \
}

/* $1027 LBEQ relative ----- */
#define lbeq() \
{ \
	word t; \
	LBRANCH(cc&0x04); \
}

/* $28 BVC relative ----- */
#define bvc() \
{ \
	byte t; \
	BRANCH(!(cc&0x02)); \
}

/* $1028 LBVC relative ----- */
#define lbvc() \
{ \
	word t; \
	LBRANCH(!(cc&0x02)); \
}

/* $29 BVS relative ----- */
#define bvs() \
{ \
	byte t; \
	BRANCH(cc&0x02); \
}

/* $1029 LBVS relative ----- */
#define lbvs() \
{ \
	word t; \
	LBRANCH(cc&0x02); \
}

/* $2A BPL relative ----- */
#define bpl() \
{ \
	byte t; \
	BRANCH(!(cc&0x08)); \
}

/* $102A LBPL relative ----- */
#define lbpl() \
{ \
	word t; \
	LBRANCH(!(cc&0x08)); \
}

/* $2B BMI relative ----- */
#define bmi() \
{ \
	byte t; \
	BRANCH(cc&0x08); \
}

/* $102B LBMI relative ----- */
#define lbmi() \
{ \
	word t; \
	LBRANCH(cc&0x08); \
}

/* $2C BGE relative ----- */
#define bge() \
{ \
	byte t; \
	BRANCH(!NXORV); \
}

/* $102C LBGE relative ----- */
#define lbge() \
{ \
	word t; \
	LBRANCH(!NXORV); \
}

/* $2D BLT relative ----- */
#define blt() \
{ \
	byte t; \
	BRANCH(NXORV); \
}

/* $102D LBLT relative ----- */
#define lblt() \
{ \
	word t; \
	LBRANCH(NXORV); \
}

/* $2E BGT relative ----- */
#define bgt() \
{ \
	byte t; \
	BRANCH(!(NXORV||cc&0x04)); \
}

/* $102E LBGT relative ----- */
#define lbgt() \
{ \
	word t; \
	LBRANCH(!(NXORV||cc&0x04)); \
}

/* $2F BLE relative ----- */
#define ble() \
{ \
	byte t; \
	BRANCH(NXORV||cc&0x04); \
}

/* $102F LBLE relative ----- */
#define lble() \
{ \
	word t; \
	LBRANCH(NXORV||cc&0x04); \
}

/* $30 LEAX indexed --*-- */
#define leax() \
{ \
	xreg=eaddr; CLR_Z; SET_Z(xreg); \
}

/* $31 LEAY indexed --*-- */
#define leay() \
{ \
	yreg=eaddr; CLR_Z; SET_Z(yreg); \
}

/* $32 LEAS indexed ----- */
#define leas() \
{ \
	sreg=eaddr; \
}

/* $33 LEAU indexed ----- */
#define leau() \
{ \
	ureg=eaddr; \
}

/* $34 PSHS inherent ----- */
#define pshs() \
{ \
	byte t; \
	IMMBYTE(t); \
	if(t&0x80) PUSHWORD(pcreg); \
	if(t&0x40) PUSHWORD(ureg); \
	if(t&0x20) PUSHWORD(yreg); \
	if(t&0x10) PUSHWORD(xreg); \
	if(t&0x08) PUSHBYTE(dpreg); \
	if(t&0x04) PUSHBYTE(breg); \
	if(t&0x02) PUSHBYTE(areg); \
	if(t&0x01) PUSHBYTE(cc); \
}

/* 35 PULS inherent ----- */
#define puls() \
{ \
	byte t; \
	IMMBYTE(t); \
	if(t&0x01) PULLBYTE(cc); \
	if(t&0x02) PULLBYTE(areg); \
	if(t&0x04) PULLBYTE(breg); \
	if(t&0x08) PULLBYTE(dpreg); \
	if(t&0x10) PULLWORD(xreg); \
	if(t&0x20) PULLWORD(yreg); \
	if(t&0x40) PULLWORD(ureg); \
	if(t&0x80){ PULLWORD(pcreg);change_pc(pcreg); }	 \
}

/* $36 PSHU inherent ----- */
#define pshu() \
{ \
	byte t; \
	IMMBYTE(t); \
	if(t&0x80) PSHUWORD(pcreg); \
	if(t&0x40) PSHUWORD(sreg); \
	if(t&0x20) PSHUWORD(yreg); \
	if(t&0x10) PSHUWORD(xreg); \
	if(t&0x08) PSHUBYTE(dpreg); \
	if(t&0x04) PSHUBYTE(breg); \
	if(t&0x02) PSHUBYTE(areg); \
	if(t&0x01) PSHUBYTE(cc); \
}

/* 37 PULU inherent ----- */
#define pulu() \
{ \
	byte t; \
	IMMBYTE(t); \
	if(t&0x01) PULUBYTE(cc); \
	if(t&0x02) PULUBYTE(areg); \
	if(t&0x04) PULUBYTE(breg); \
	if(t&0x08) PULUBYTE(dpreg); \
	if(t&0x10) PULUWORD(xreg); \
	if(t&0x20) PULUWORD(yreg); \
	if(t&0x40) PULUWORD(sreg); \
	if(t&0x80) { PULUWORD(pcreg);change_pc(pcreg); }	 \
}

/* $38 ILLEGAL */

/* $39 RTS inherent ----- */
#define rts() \
{ \
	PULLWORD(pcreg);change_pc(pcreg);	 \
}

/* $3A ABX inherent ----- */
#define abx() \
{ \
	xreg += breg; \
}

/* $3B RTI inherent ##### */
#define rti() \
{ \
	byte t; \
	PULLBYTE(cc); \
	t=cc&0x80;	 \
	if(t) \
	{ \
		m6809_ICount -= 9; \
		PULLBYTE(areg); \
		PULLBYTE(breg); \
		PULLBYTE(dpreg); \
		PULLWORD(xreg); \
		PULLWORD(yreg); \
		PULLWORD(ureg); \
	} \
	PULLWORD(pcreg);change_pc(pcreg);	 \
}

/* $3C CWAI inherent ----1 */
#define cwai() \
{ \
	byte t; \
	IMMBYTE(t); cc&=t; \
	if (!(((pending_interrupts & M6809_INT_IRQ) != 0 && (cc & 0x10) == 0) || ((pending_interrupts & M6809_INT_FIRQ) != 0 && (cc & 0x40) == 0))) \
	{ \
		m6809_ICount = 0; \
		pending_interrupts |= M6809_CWAI;	 \
	} \
}

/* $3D MUL inherent --*-@ */
#define mul() \
{ \
	word t; \
	t=areg*breg; \
	CLR_ZC; SET_Z16(t); if(t&0x80) SEC; \
	SETDREG(t); \
}

/* $3E ILLEGAL */

/* $3F SWI (SWI2 SWI3) absolute indirect ----- */
#define swi() \
{ \
	cc|=0x80; \
	PUSHWORD(pcreg); \
	PUSHWORD(ureg); \
	PUSHWORD(yreg); \
	PUSHWORD(xreg); \
	PUSHBYTE(dpreg); \
	PUSHBYTE(breg); \
	PUSHBYTE(areg); \
	PUSHBYTE(cc); \
	cc|=0x50; \
	pcreg = M_RDMEM_WORD(0xfffa);change_pc(pcreg);	 \
}

/* $103F SWI2 absolute indirect ----- */
#define swi2() \
{ \
	cc|=0x80; \
	PUSHWORD(pcreg); \
	PUSHWORD(ureg); \
	PUSHWORD(yreg); \
	PUSHWORD(xreg); \
	PUSHBYTE(dpreg); \
	PUSHBYTE(breg); \
	PUSHBYTE(areg); \
	PUSHBYTE(cc); \
	pcreg = M_RDMEM_WORD(0xfff4);change_pc(pcreg);	 \
}

/* $113F SWI3 absolute indirect ----- */
#define swi3() \
{ \
	cc|=0x80; \
	PUSHWORD(pcreg); \
	PUSHWORD(ureg); \
	PUSHWORD(yreg); \
	PUSHWORD(xreg); \
	PUSHBYTE(dpreg); \
	PUSHBYTE(breg); \
	PUSHBYTE(areg); \
	PUSHBYTE(cc); \
	pcreg = M_RDMEM_WORD(0xfff2);change_pc(pcreg);	 \
}

/* $40 NEGA inherent ?**** */
#define nega() \
{ \
	word r; \
	r=-areg; \
	CLR_NZVC; SET_FLAGS8(0,areg,r); \
	areg=r; \
}

/* $41 ILLEGAL */

/* $42 ILLEGAL */

/* $43 COMA inherent -**01 */
#define coma() \
{ \
	areg = ~areg; \
	CLR_NZV; SET_NZ8(areg); SEC; \
}

/* $44 LSRA inherent -0*-* */
#define lsra() \
{ \
	CLR_NZC; cc|=(areg&0x01); \
	areg>>=1; SET_Z8(areg); \
}

/* $45 ILLEGAL */

/* $46 RORA inherent -**-* */
#define rora() \
{ \
	byte r; \
	r=(cc&0x01)<<7; \
	CLR_NZC; cc|=(areg&0x01); \
	r |= areg>>1; SET_NZ8(r); \
	areg=r; \
}

/* $47 ASRA inherent ?**-* */
#define asra() \
{ \
	CLR_NZC; cc|=(areg&0x01); \
	areg>>=1; areg|=((areg&0x40)<<1); \
	SET_NZ8(areg); \
}

/* $48 ASLA inherent ?**** */
#define asla() \
{ \
	word r; \
	r=areg<<1; \
	CLR_NZVC; SET_FLAGS8(areg,areg,r); \
	areg=r; \
}

/* $49 ROLA inherent -**** */
#define rola() \
{ \
	word t,r; \
	t = areg; r = cc&0x01; r |= t<<1; \
	CLR_NZVC; SET_FLAGS8(t,t,r); \
	areg=r; \
}

/* $4A DECA inherent -***- */
#define deca() \
{ \
	--areg; \
	CLR_NZV; SET_FLAGS8D(areg); \
}

/* $4B ILLEGAL */

/* $4C INCA inherent -***- */
#define inca() \
{ \
	++areg; \
	CLR_NZV; SET_FLAGS8I(areg); \
}

/* $4D TSTA inherent -**0- */
#define tsta() \
{ \
	CLR_NZV; SET_NZ8(areg); \
}

/* $4E ILLEGAL */

/* $4F CLRA inherent -0100 */
#define clra() \
{ \
	areg=0; \
	CLR_NZVC; SEZ; \
}

/* $50 NEGB inherent ?**** */
#define negb() \
{ \
	word r; \
	r=-breg; \
	CLR_NZVC; SET_FLAGS8(0,breg,r); \
	breg=r; \
}

/* $51 ILLEGAL */

/* $52 ILLEGAL */

/* $53 COMB inherent -**01 */
#define comb() \
{ \
	breg = ~breg; \
	CLR_NZV; SET_NZ8(breg); SEC; \
}

/* $54 LSRB inherent -0*-* */
#define lsrb() \
{ \
	CLR_NZC; cc|=(breg&0x01); \
	breg>>=1; SET_Z8(breg); \
}

/* $55 ILLEGAL */

/* $56 RORB inherent -**-* */
#define rorb() \
{ \
	byte r; \
	r=(cc&0x01)<<7; \
	CLR_NZC; cc|=(breg&0x01); \
	r |= breg>>1; SET_NZ8(r); \
	breg=r; \
}

/* $57 ASRB inherent ?**-* */
#define asrb() \
{ \
	CLR_NZC; cc|=(breg&0x01); \
	breg>>=1; breg|=((breg&0x40)<<1); \
	SET_NZ8(breg); \
}

/* $58 ASLB inherent ?**** */
#define aslb() \
{ \
	word r; \
	r=breg<<1; \
	CLR_NZVC; SET_FLAGS8(breg,breg,r); \
	breg=r; \
}

/* $59 ROLB inherent -**** */
#define rolb() \
{ \
	word t,r; \
	t = breg; r = cc&0x01; r |= t<<1; \
	CLR_NZVC; SET_FLAGS8(t,t,r); \
	breg=r; \
}

/* $5A DECB inherent -***- */
#define decb() \
{ \
	--breg; \
	CLR_NZV; SET_FLAGS8D(breg); \
}

/* $5B ILLEGAL */

/* $5C INCB inherent -***- */
#define incb() \
{ \
	++breg; \
	CLR_NZV; SET_FLAGS8I(breg); \
}

/* $5D TSTB inherent -**0- */
#define tstb() \
{ \
	CLR_NZV; SET_NZ8(breg); \
}

/* $5E ILLEGAL */

/* $5F CLRB inherent -0100 */
#define clrb() \
{ \
	breg=0; \
	CLR_NZVC; SEZ; \
}

/* $60 NEG indexed ?**** */
#define neg_ix() \
{ \
	word r,t; \
	t=M_RDMEM(eaddr); r=-t; \
	CLR_NZVC; SET_FLAGS8(0,t,r); \
	M_WRMEM(eaddr,r); \
}

/* $61 ILLEGAL */

/* $62 ILLEGAL */

/* $63 COM indexed -**01 */
#define com_ix() \
{ \
	byte t; \
	t = ~M_RDMEM(eaddr); \
	CLR_NZV; SET_NZ8(t); SEC; \
	M_WRMEM(eaddr,t); \
}

/* $64 LSR indexed -0*-* */
#define lsr_ix() \
{ \
	byte t; \
	t=M_RDMEM(eaddr); CLR_NZC; cc|=(t&0x01); \
	t>>=1; SET_Z8(t); \
	M_WRMEM(eaddr,t); \
}

/* $65 ILLEGAL */

/* $66 ROR indexed -**-* */
#define ror_ix() \
{ \
	byte t,r; \
	t=M_RDMEM(eaddr); r=(cc&0x01)<<7; \
	CLR_NZC; cc|=(t&0x01); \
	r |= t>>1; SET_NZ8(r); \
	M_WRMEM(eaddr,r); \
}

/* $67 ASR indexed ?**-* */
#define asr_ix() \
{ \
	byte t; \
	t=M_RDMEM(eaddr); CLR_NZC; cc|=(t&0x01); \
	t>>=1; t|=((t&0x40)<<1); \
	SET_NZ8(t); \
	M_WRMEM(eaddr,t); \
}

/* $68 ASL indexed ?**** */
#define asl_ix() \
{ \
	word t,r; \
	t=M_RDMEM(eaddr); r=t<<1; \
	CLR_NZVC; SET_FLAGS8(t,t,r); \
	M_WRMEM(eaddr,r); \
}

/* $69 ROL indexed -**** */
#define rol_ix() \
{ \
	word t,r; \
	t=M_RDMEM(eaddr); r = cc&0x01; r |= t<<1; \
	CLR_NZVC; SET_FLAGS8(t,t,r); \
	M_WRMEM(eaddr,r); \
}

/* $6A DEC indexed -***- */
#define dec_ix() \
{ \
	byte t; \
	t=M_RDMEM(eaddr)-1; \
	CLR_NZV; SET_FLAGS8D(t); \
	M_WRMEM(eaddr,t); \
}

/* $6B ILLEGAL */

/* $6C INC indexed -***- */
#define inc_ix() \
{ \
	byte t; \
	t=M_RDMEM(eaddr)+1; \
	CLR_NZV; SET_FLAGS8I(t); \
	M_WRMEM(eaddr,t); \
}

/* $6D TST indexed -**0- */
#define tst_ix() \
{ \
	byte t; \
	t=M_RDMEM(eaddr); CLR_NZV; SET_NZ8(t); \
}

/* $6E JMP indexed ----- */
#define jmp_ix() \
{ \
	pcreg=eaddr;change_pc(pcreg);	 \
}

/* $6E JMP indexed ----- */
#define jmp_ix_slap() \
{ \
	pcreg=eaddr;cpu_setOPbase16(pcreg);	 \
}

/* $6F CLR indexed -0100 */
#define clr_ix() \
{ \
	M_WRMEM(eaddr,0); \
	CLR_NZVC; SEZ; \
}

/* $70 NEG extended ?**** */
#define neg_ex() \
{ \
	word r,t; \
	EXTBYTE(t); r=-t; \
	CLR_NZVC; SET_FLAGS8(0,t,r); \
	M_WRMEM(eaddr,r); \
}

/* $71 ILLEGAL */

/* $72 ILLEGAL */

/* $73 COM extended -**01 */
#define com_ex() \
{ \
	byte t; \
	EXTBYTE(t); t = ~t; \
	CLR_NZV; SET_NZ8(t); SEC; \
	M_WRMEM(eaddr,t); \
}

/* $74 LSR extended -0*-* */
#define lsr_ex() \
{ \
	byte t; \
	EXTBYTE(t); CLR_NZC; cc|=(t&0x01); \
	t>>=1; SET_Z8(t); \
	M_WRMEM(eaddr,t); \
}

/* $75 ILLEGAL */

/* $76 ROR extended -**-* */
#define ror_ex() \
{ \
	byte t,r; \
	EXTBYTE(t); r=(cc&0x01)<<7; \
	CLR_NZC; cc|=(t&0x01); \
	r |= t>>1; SET_NZ8(r); \
	M_WRMEM(eaddr,r); \
}

/* $77 ASR extended ?**-* */
#define asr_ex() \
{ \
	byte t; \
	EXTBYTE(t); CLR_NZC; cc|=(t&0x01); \
	t>>=1; t|=((t&0x40)<<1); \
	SET_NZ8(t); \
	M_WRMEM(eaddr,t); \
}

/* $78 ASL extended ?**** */
#define asl_ex() \
{ \
	word t,r; \
	EXTBYTE(t); r=t<<1; \
	CLR_NZVC; SET_FLAGS8(t,t,r); \
	M_WRMEM(eaddr,r); \
}

/* $79 ROL extended -**** */
#define rol_ex() \
{ \
	word t,r; \
	EXTBYTE(t); r = cc&0x01; r |= t<<1; \
	CLR_NZVC; SET_FLAGS8(t,t,r); \
	M_WRMEM(eaddr,r); \
}

/* $7A DEC extended -***- */
#define dec_ex() \
{ \
	byte t; \
	EXTBYTE(t); --t; \
	CLR_NZV; SET_FLAGS8D(t); \
	M_WRMEM(eaddr,t); \
}

/* $7B ILLEGAL */

/* $7C INC extended -***- */
#define inc_ex() \
{ \
	byte t; \
	EXTBYTE(t); ++t; \
	CLR_NZV; SET_FLAGS8I(t); \
	M_WRMEM(eaddr,t); \
}

/* $7D TST extended -**0- */
#define tst_ex() \
{ \
	byte t; \
	EXTBYTE(t); CLR_NZV; SET_NZ8(t); \
}

/* $7E JMP extended ----- */
#define jmp_ex() \
{ \
	EXTENDED; pcreg=eaddr;change_pc(pcreg);	 \
}

/* $7E JMP extended ----- */
#define jmp_ex_slap() \
{ \
	EXTENDED; pcreg=eaddr;cpu_setOPbase16(pcreg);	 \
}

/* $7F CLR extended -0100 */
#define clr_ex() \
{ \
	EXTENDED; M_WRMEM(eaddr,0); \
	CLR_NZVC; SEZ; \
}


/* $80 SUBA immediate ?**** */
#define suba_im() \
{ \
	word	t,r; \
	IMMBYTE(t); r = areg-t; \
	CLR_NZVC; SET_FLAGS8(areg,t,r); \
	areg = r; \
}

/* $81 CMPA immediate ?**** */
#define cmpa_im() \
{ \
	word	t,r; \
	IMMBYTE(t); r = areg-t; \
	CLR_NZVC; SET_FLAGS8(areg,t,r); \
}

/* $82 SBCA immediate ?**** */
#define sbca_im() \
{ \
	word	t,r; \
	IMMBYTE(t); r = areg-t-(cc&0x01); \
	CLR_NZVC; SET_FLAGS8(areg,t,r); \
	areg = r; \
}

/* $83 SUBD (CMPD CMPU) immediate -**** */
#define subd_im() \
{ \
	dword r,d,b; \
	IMMWORD(b); d = GETDREG; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
	SETDREG(r); \
}

/* $1083 CMPD immediate -**** */
#define cmpd_im() \
{ \
	dword r,d,b; \
	IMMWORD(b); d = GETDREG; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $1183 CMPU immediate -**** */
#define cmpu_im() \
{ \
	dword r,b; \
	IMMWORD(b); r = ureg-b; \
	CLR_NZVC; SET_FLAGS16(ureg,b,r); \
}

/* $84 ANDA immediate -**0- */
#define anda_im() \
{ \
	byte t; \
	IMMBYTE(t); areg &= t; \
	CLR_NZV; SET_NZ8(areg); \
}

/* $85 BITA immediate -**0- */
#define bita_im() \
{ \
	byte t,r; \
	IMMBYTE(t); r = areg&t; \
	CLR_NZV; SET_NZ8(r); \
}

/* $86 LDA immediate -**0- */
#define lda_im() \
{ \
	IMMBYTE(areg); \
	CLR_NZV; SET_NZ8(areg); \
}

/* is this a legal instruction? */
/* $87 STA immediate -**0- */
#define sta_im() \
{ \
	CLR_NZV; SET_NZ8(areg); \
	IMM8; M_WRMEM(eaddr,areg); \
}

/* $88 EORA immediate -**0- */
#define eora_im() \
{ \
	byte t; \
	IMMBYTE(t); areg ^= t; \
	CLR_NZV; SET_NZ8(areg); \
}

/* $89 ADCA immediate ***** */
#define adca_im() \
{ \
	word t,r; \
	IMMBYTE(t); r = areg+t+(cc&0x01); \
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r); \
	areg = r; \
}

/* $8A ORA immediate -**0- */
#define ora_im() \
{ \
	byte t; \
	IMMBYTE(t); areg |= t; \
	CLR_NZV; SET_NZ8(areg); \
}

/* $8B ADDA immediate ***** */
#define adda_im() \
{ \
	word t,r; \
	IMMBYTE(t); r = areg+t; \
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r); \
	areg = r; \
}

/* $8C CMPX (CMPY CMPS) immediate -**** */
#define cmpx_im() \
{ \
	dword r,d,b; \
	IMMWORD(b); d = xreg; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $108C CMPY immediate -**** */
#define cmpy_im() \
{ \
	dword r,d,b; \
	IMMWORD(b); d = yreg; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $118C CMPS immediate -**** */
#define cmps_im() \
{ \
	dword r,d,b; \
	IMMWORD(b); d = sreg; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $8D BSR ----- */
#define bsr() \
{ \
	byte t; \
	IMMBYTE(t); PUSHWORD(pcreg); pcreg += SIGNED(t);change_pc(pcreg);	 \
}

/* $8E LDX (LDY) immediate -**0- */
#define ldx_im() \
{ \
	IMMWORD(xreg); \
	CLR_NZV; SET_NZ16(xreg); \
}

/* $108E LDY immediate -**0- */
#define ldy_im() \
{ \
	IMMWORD(yreg); \
	CLR_NZV; SET_NZ16(yreg); \
}

/* is this a legal instruction? */
/* $8F STX (STY) immediate -**0- */
#define stx_im() \
{ \
	CLR_NZV; SET_NZ16(xreg); \
	IMM16; M_WRMEM_WORD(eaddr,xreg); \
}

/* is this a legal instruction? */
/* $108F STY immediate -**0- */
#define sty_im() \
{ \
	CLR_NZV; SET_NZ16(yreg); \
	IMM16; M_WRMEM_WORD(eaddr,yreg); \
}

/* $90 SUBA direct ?**** */
#define suba_di() \
{ \
	word	t,r; \
	DIRBYTE(t); r = areg-t; \
	CLR_NZVC; SET_FLAGS8(areg,t,r); \
	areg = r; \
}

/* $91 CMPA direct ?**** */
#define cmpa_di() \
{ \
	word	t,r; \
	DIRBYTE(t); r = areg-t; \
	CLR_NZVC; SET_FLAGS8(areg,t,r); \
}

/* $92 SBCA direct ?**** */
#define sbca_di() \
{ \
	word	t,r; \
	DIRBYTE(t); r = areg-t-(cc&0x01); \
	CLR_NZVC; SET_FLAGS8(areg,t,r); \
	areg = r; \
}

/* $93 SUBD (CMPD CMPU) direct -**** */
#define subd_di() \
{ \
	dword r,d,b; \
	DIRWORD(b); d = GETDREG; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
	SETDREG(r); \
}

/* $1093 CMPD direct -**** */
#define cmpd_di() \
{ \
	dword r,d,b; \
	DIRWORD(b); d = GETDREG; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $1193 CMPU direct -**** */
#define cmpu_di() \
{ \
	dword r,b; \
	DIRWORD(b); r = ureg-b; \
	CLR_NZVC; SET_FLAGS16(ureg,b,r); \
}

/* $94 ANDA direct -**0- */
#define anda_di() \
{ \
	byte t; \
	DIRBYTE(t); areg &= t; \
	CLR_NZV; SET_NZ8(areg); \
}

/* $95 BITA direct -**0- */
#define bita_di() \
{ \
	byte t,r; \
	DIRBYTE(t); r = areg&t; \
	CLR_NZV; SET_NZ8(r); \
}

/* $96 LDA direct -**0- */
#define lda_di() \
{ \
	DIRBYTE(areg); \
	CLR_NZV; SET_NZ8(areg); \
}

/* $97 STA direct -**0- */
#define sta_di() \
{ \
	CLR_NZV; SET_NZ8(areg); \
	DIRECT; M_WRMEM(eaddr,areg); \
}

/* $98 EORA direct -**0- */
#define eora_di() \
{ \
	byte t; \
	DIRBYTE(t); areg ^= t; \
	CLR_NZV; SET_NZ8(areg); \
}

/* $99 ADCA direct ***** */
#define adca_di() \
{ \
	word t,r; \
	DIRBYTE(t); r = areg+t+(cc&0x01); \
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r); \
	areg = r; \
}

/* $9A ORA direct -**0- */
#define ora_di() \
{ \
	byte t; \
	DIRBYTE(t); areg |= t; \
	CLR_NZV; SET_NZ8(areg); \
}

/* $9B ADDA direct ***** */
#define adda_di() \
{ \
	word t,r; \
	DIRBYTE(t); r = areg+t; \
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r); \
	areg = r; \
}

/* $9C CMPX (CMPY CMPS) direct -**** */
#define cmpx_di() \
{ \
	dword r,d,b; \
	DIRWORD(b); d = xreg; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $109C CMPY direct -**** */
#define cmpy_di() \
{ \
	dword r,d,b; \
	DIRWORD(b); d = yreg; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $119C CMPS direct -**** */
#define cmps_di() \
{ \
	dword r,d,b; \
	DIRWORD(b); d = sreg; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $9D JSR direct ----- */
#define jsr_di() \
{ \
	DIRECT; PUSHWORD(pcreg); \
	pcreg = eaddr;change_pc(pcreg);	 \
}

/* $9D JSR direct ----- */
#define jsr_di_slap() \
{ \
	DIRECT; PUSHWORD(pcreg); \
	pcreg = eaddr;cpu_setOPbase16(pcreg);	 \
}

/* $9E LDX (LDY) direct -**0- */
#define ldx_di() \
{ \
	DIRWORD(xreg); \
	CLR_NZV; SET_NZ16(xreg); \
}

/* $109E LDY direct -**0- */
#define ldy_di() \
{ \
	DIRWORD(yreg); \
	CLR_NZV; SET_NZ16(yreg); \
}

/* $9F STX (STY) direct -**0- */
#define stx_di() \
{ \
	CLR_NZV; SET_NZ16(xreg); \
	DIRECT; M_WRMEM_WORD(eaddr,xreg); \
}

/* $109F STY direct -**0- */
#define sty_di() \
{ \
	CLR_NZV; SET_NZ16(yreg); \
	DIRECT; M_WRMEM_WORD(eaddr,yreg); \
}

/* $a0 SUBA indexed ?**** */
#define suba_ix() \
{ \
	word	t,r; \
	t = M_RDMEM(eaddr); r = areg-t; \
	CLR_NZVC; SET_FLAGS8(areg,t,r); \
	areg = r; \
}

/* $a1 CMPA indexed ?**** */
#define cmpa_ix() \
{ \
	word	t,r; \
	t = M_RDMEM(eaddr); r = areg-t; \
	CLR_NZVC; SET_FLAGS8(areg,t,r); \
}

/* $a2 SBCA indexed ?**** */
#define sbca_ix() \
{ \
	word	t,r; \
	t = M_RDMEM(eaddr); r = areg-t-(cc&0x01); \
	CLR_NZVC; SET_FLAGS8(areg,t,r); \
	areg = r; \
}

/* $a3 SUBD (CMPD CMPU) indexed -**** */
#define subd_ix() \
{ \
	dword r,d,b; \
	b = M_RDMEM_WORD(eaddr); d = GETDREG; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
	SETDREG(r); \
}

/* $10a3 CMPD indexed -**** */
#define cmpd_ix() \
{ \
	dword r,d,b; \
	b = M_RDMEM_WORD(eaddr); d = GETDREG; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $11a3 CMPU indexed -**** */
#define cmpu_ix() \
{ \
	dword r,b; \
	b = M_RDMEM_WORD(eaddr); r = ureg-b; \
	CLR_NZVC; SET_FLAGS16(ureg,b,r); \
}

/* $a4 ANDA indexed -**0- */
#define anda_ix() \
{ \
	areg &= M_RDMEM(eaddr); \
	CLR_NZV; SET_NZ8(areg); \
}

/* $a5 BITA indexed -**0- */
#define bita_ix() \
{ \
	byte r; \
	r = areg&M_RDMEM(eaddr); \
	CLR_NZV; SET_NZ8(r); \
}

/* $a6 LDA indexed -**0- */
#define lda_ix() \
{ \
	areg = M_RDMEM(eaddr); \
	CLR_NZV; SET_NZ8(areg); \
}

/* $a7 STA indexed -**0- */
#define sta_ix() \
{ \
	CLR_NZV; SET_NZ8(areg); \
	M_WRMEM(eaddr,areg); \
}

/* $a8 EORA indexed -**0- */
#define eora_ix() \
{ \
	areg ^= M_RDMEM(eaddr); \
	CLR_NZV; SET_NZ8(areg); \
}

/* $a9 ADCA indexed ***** */
#define adca_ix() \
{ \
	word t,r; \
	t = M_RDMEM(eaddr); r = areg+t+(cc&0x01); \
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r); \
	areg = r; \
}

/* $aA ORA indexed -**0- */
#define ora_ix() \
{ \
	areg |= M_RDMEM(eaddr); \
	CLR_NZV; SET_NZ8(areg); \
}

/* $aB ADDA indexed ***** */
#define adda_ix() \
{ \
	word t,r; \
	t = M_RDMEM(eaddr); r = areg+t; \
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r); \
	areg = r; \
}

/* $aC CMPX (CMPY CMPS) indexed -**** */
#define cmpx_ix() \
{ \
	dword r,d,b; \
	b = M_RDMEM_WORD(eaddr); d = xreg; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $10aC CMPY indexed -**** */
#define cmpy_ix() \
{ \
	dword r,d,b; \
	b = M_RDMEM_WORD(eaddr); d = yreg; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $11aC CMPS indexed -**** */
#define cmps_ix() \
{ \
	dword r,d,b; \
	b = M_RDMEM_WORD(eaddr); d = sreg; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $aD JSR indexed ----- */
#define jsr_ix() \
{ \
	PUSHWORD(pcreg); \
	pcreg = eaddr;change_pc(pcreg);	 \
}

/* $aD JSR indexed ----- */
#define jsr_ix_slap() \
{ \
	PUSHWORD(pcreg); \
	pcreg = eaddr;cpu_setOPbase16(pcreg);	 \
}

/* $aE LDX (LDY) indexed -**0- */
#define ldx_ix() \
{ \
	xreg = M_RDMEM_WORD(eaddr); \
	CLR_NZV; SET_NZ16(xreg); \
}

/* $10aE LDY indexed -**0- */
#define ldy_ix() \
{ \
	yreg = M_RDMEM_WORD(eaddr); \
	CLR_NZV; SET_NZ16(yreg); \
}

/* $aF STX (STY) indexed -**0- */
#define stx_ix() \
{ \
	CLR_NZV; SET_NZ16(xreg); \
	M_WRMEM_WORD(eaddr,xreg); \
}

/* $10aF STY indexed -**0- */
#define sty_ix() \
{ \
	CLR_NZV; SET_NZ16(yreg); \
	M_WRMEM_WORD(eaddr,yreg); \
}

/* $b0 SUBA extended ?**** */
#define suba_ex() \
{ \
	word	t,r; \
	EXTBYTE(t); r = areg-t; \
	CLR_NZVC; SET_FLAGS8(areg,t,r); \
	areg = r; \
}

/* $b1 CMPA extended ?**** */
#define cmpa_ex() \
{ \
	word	t,r; \
	EXTBYTE(t); r = areg-t; \
	CLR_NZVC; SET_FLAGS8(areg,t,r); \
}

/* $b2 SBCA extended ?**** */
#define sbca_ex() \
{ \
	word	t,r; \
	EXTBYTE(t); r = areg-t-(cc&0x01); \
	CLR_NZVC; SET_FLAGS8(areg,t,r); \
	areg = r; \
}

/* $b3 SUBD (CMPD CMPU) extended -**** */
#define subd_ex() \
{ \
	dword r,d,b; \
	EXTWORD(b); d = GETDREG; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
	SETDREG(r); \
}

/* $10b3 CMPD extended -**** */
#define cmpd_ex() \
{ \
	dword r,d,b; \
	EXTWORD(b); d = GETDREG; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $11b3 CMPU extended -**** */
#define cmpu_ex() \
{ \
	dword r,b; \
	EXTWORD(b); r = ureg-b; \
	CLR_NZVC; SET_FLAGS16(ureg,b,r); \
}

/* $b4 ANDA extended -**0- */
#define anda_ex() \
{ \
	byte t; \
	EXTBYTE(t); areg &= t; \
	CLR_NZV; SET_NZ8(areg); \
}

/* $b5 BITA extended -**0- */
#define bita_ex() \
{ \
	byte t,r; \
	EXTBYTE(t); r = areg&t; \
	CLR_NZV; SET_NZ8(r); \
}

/* $b6 LDA extended -**0- */
#define lda_ex() \
{ \
	EXTBYTE(areg); \
	CLR_NZV; SET_NZ8(areg); \
} 

/* $b7 STA extended -**0- */
#define sta_ex() \
{ \
	CLR_NZV; SET_NZ8(areg); \
	EXTENDED; M_WRMEM(eaddr,areg); \
}

/* $b8 EORA extended -**0- */
#define eora_ex() \
{ \
	byte t; \
	EXTBYTE(t); areg ^= t; \
	CLR_NZV; SET_NZ8(areg); \
}

/* $b9 ADCA extended ***** */
#define adca_ex() \
{ \
	word t,r; \
	EXTBYTE(t); r = areg+t+(cc&0x01); \
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r); \
	areg = r; \
}

/* $bA ORA extended -**0- */
#define ora_ex() \
{ \
	byte t; \
	EXTBYTE(t); areg |= t; \
	CLR_NZV; SET_NZ8(areg); \
}

/* $bB ADDA extended ***** */
#define adda_ex() \
{ \
	word t,r; \
	EXTBYTE(t); r = areg+t; \
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r); \
	areg = r; \
}

/* $bC CMPX (CMPY CMPS) extended -**** */
#define cmpx_ex() \
{ \
	dword r,d,b; \
	EXTWORD(b); d = xreg; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $10bC CMPY extended -**** */
#define cmpy_ex() \
{ \
	dword r,d,b; \
	EXTWORD(b); d = yreg; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $11bC CMPS extended -**** */
#define cmps_ex() \
{ \
	dword r,d,b; \
	EXTWORD(b); d = sreg; r = d-b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
}

/* $bD JSR extended ----- */
#define jsr_ex() \
{ \
	EXTENDED; PUSHWORD(pcreg); \
	pcreg = eaddr;change_pc(pcreg);	 \
}

/* $bD JSR extended - Slapstic ----- */
#define jsr_ex_slap() \
{ \
	EXTENDED; PUSHWORD(pcreg); \
	pcreg = eaddr;cpu_setOPbase16(pcreg);	 \
}

/* $bE LDX (LDY) extended -**0- */
#define ldx_ex() \
{ \
	EXTWORD(xreg); \
	CLR_NZV; SET_NZ16(xreg); \
}

/* $10bE LDY extended -**0- */
#define ldy_ex() \
{ \
	EXTWORD(yreg); \
	CLR_NZV; SET_NZ16(yreg); \
}

/* $bF STX (STY) extended -**0- */
#define stx_ex() \
{ \
	CLR_NZV; SET_NZ16(xreg); \
	EXTENDED; M_WRMEM_WORD(eaddr,xreg); \
}

/* $10bF STY extended -**0- */
#define sty_ex() \
{ \
	CLR_NZV; SET_NZ16(yreg); \
	EXTENDED; M_WRMEM_WORD(eaddr,yreg); \
}


/* $c0 SUBB immediate ?**** */
#define subb_im() \
{ \
	word	t,r; \
	IMMBYTE(t); r = breg-t; \
	CLR_NZVC; SET_FLAGS8(breg,t,r); \
	breg = r; \
}

/* $c1 CMPB immediate ?**** */
#define cmpb_im() \
{ \
	word	t,r; \
	IMMBYTE(t); r = breg-t; \
	CLR_NZVC; SET_FLAGS8(breg,t,r); \
}

/* $c2 SBCB immediate ?**** */
#define sbcb_im() \
{ \
	word	t,r; \
	IMMBYTE(t); r = breg-t-(cc&0x01); \
	CLR_NZVC; SET_FLAGS8(breg,t,r); \
	breg = r; \
}

/* $c3 ADDD immediate -**** */
#define addd_im() \
{ \
	dword r,d,b; \
	IMMWORD(b); d = GETDREG; r = d+b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
	SETDREG(r); \
}

/* $c4 ANDB immediate -**0- */
#define andb_im() \
{ \
	byte t; \
	IMMBYTE(t); breg &= t; \
	CLR_NZV; SET_NZ8(breg); \
}

/* $c5 BITB immediate -**0- */
#define bitb_im() \
{ \
	byte t,r; \
	IMMBYTE(t); r = breg&t; \
	CLR_NZV; SET_NZ8(r); \
}

/* $c6 LDB immediate -**0- */
#define ldb_im() \
{ \
	IMMBYTE(breg); \
	CLR_NZV; SET_NZ8(breg); \
}

/* is this a legal instruction? */
/* $c7 STB immediate -**0- */
#define stb_im() \
{ \
	CLR_NZV; SET_NZ8(breg); \
	IMM8; M_WRMEM(eaddr,breg); \
}

/* $c8 EORB immediate -**0- */
#define eorb_im() \
{ \
	byte t; \
	IMMBYTE(t); breg ^= t; \
	CLR_NZV; SET_NZ8(breg); \
}

/* $c9 ADCB immediate ***** */
#define adcb_im() \
{ \
	word t,r; \
	IMMBYTE(t); r = breg+t+(cc&0x01); \
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r); \
	breg = r; \
}

/* $cA ORB immediate -**0- */
#define orb_im() \
{ \
	byte t; \
	IMMBYTE(t); breg |= t; \
	CLR_NZV; SET_NZ8(breg); \
}

/* $cB ADDB immediate ***** */
#define addb_im() \
{ \
	word t,r; \
	IMMBYTE(t); r = breg+t; \
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r); \
	breg = r; \
}

/* $cC LDD immediate -**0- */
#define ldd_im() \
{ \
	word t; \
	IMMWORD(t); SETDREG(t); \
	CLR_NZV; SET_NZ16(t); \
}

/* is this a legal instruction? */
/* $cD STD immediate -**0- */
#define std_im() \
{ \
	word t; \
	IMM16; t=GETDREG; \
	CLR_NZV; SET_NZ16(t); \
	M_WRMEM_WORD(eaddr,t); \
}

/* $cE LDU (LDS) immediate -**0- */
#define ldu_im() \
{ \
	IMMWORD(ureg); \
	CLR_NZV; SET_NZ16(ureg); \
}

/* $10cE LDS immediate -**0- */
#define lds_im() \
{ \
	IMMWORD(sreg); \
	CLR_NZV; SET_NZ16(sreg); \
}

/* is this a legal instruction? */
/* $cF STU (STS) immediate -**0- */
#define stu_im() \
{ \
	CLR_NZV; SET_NZ16(ureg); \
	IMM16; M_WRMEM_WORD(eaddr,ureg); \
}

/* is this a legal instruction? */
/* $10cF STS immediate -**0- */
#define sts_im() \
{ \
	CLR_NZV; SET_NZ16(sreg); \
	IMM16; M_WRMEM_WORD(eaddr,sreg); \
}


/* $d0 SUBB direct ?**** */
#define subb_di() \
{ \
	word	t,r; \
	DIRBYTE(t); r = breg-t; \
	CLR_NZVC; SET_FLAGS8(breg,t,r); \
	breg = r; \
}

/* $d1 CMPB direct ?**** */
#define cmpb_di() \
{ \
	word	t,r; \
	DIRBYTE(t); r = breg-t; \
	CLR_NZVC; SET_FLAGS8(breg,t,r); \
}

/* $d2 SBCB direct ?**** */
#define sbcb_di() \
{ \
	word	t,r; \
	DIRBYTE(t); r = breg-t-(cc&0x01); \
	CLR_NZVC; SET_FLAGS8(breg,t,r); \
	breg = r; \
}

/* $d3 ADDD direct -**** */
#define addd_di() \
{ \
	dword r,d,b; \
	DIRWORD(b); d = GETDREG; r = d+b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
	SETDREG(r); \
}

/* $d4 ANDB direct -**0- */
#define andb_di() \
{ \
	byte t; \
	DIRBYTE(t); breg &= t; \
	CLR_NZV; SET_NZ8(breg); \
}

/* $d5 BITB direct -**0- */
#define bitb_di() \
{ \
	byte t,r; \
	DIRBYTE(t); r = breg&t; \
	CLR_NZV; SET_NZ8(r); \
}

/* $d6 LDB direct -**0- */
#define ldb_di() \
{ \
	DIRBYTE(breg); \
	CLR_NZV; SET_NZ8(breg); \
}

/* $d7 STB direct -**0- */
#define stb_di() \
{ \
	CLR_NZV; SET_NZ8(breg); \
	DIRECT; M_WRMEM(eaddr,breg); \
}

/* $d8 EORB direct -**0- */
#define eorb_di() \
{ \
	byte t; \
	DIRBYTE(t); breg ^= t; \
	CLR_NZV; SET_NZ8(breg); \
}

/* $d9 ADCB direct ***** */
#define adcb_di() \
{ \
	word t,r; \
	DIRBYTE(t); r = breg+t+(cc&0x01); \
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r); \
	breg = r; \
}

/* $dA ORB direct -**0- */
#define orb_di() \
{ \
	byte t; \
	DIRBYTE(t); breg |= t; \
	CLR_NZV; SET_NZ8(breg); \
}

/* $dB ADDB direct ***** */
#define addb_di() \
{ \
	word t,r; \
	DIRBYTE(t); r = breg+t; \
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r); \
	breg = r; \
}

/* $dC LDD direct -**0- */
#define ldd_di() \
{ \
	word t; \
	DIRWORD(t); SETDREG(t); \
	CLR_NZV; SET_NZ16(t); \
}

/* $dD STD direct -**0- */
#define std_di() \
{ \
	word t; \
	DIRECT; t=GETDREG; \
	CLR_NZV; SET_NZ16(t); \
	M_WRMEM_WORD(eaddr,t); \
}

/* $dE LDU (LDS) direct -**0- */
#define ldu_di() \
{ \
	DIRWORD(ureg); \
	CLR_NZV; SET_NZ16(ureg); \
}

/* $10dE LDS direct -**0- */
#define lds_di() \
{ \
	DIRWORD(sreg); \
	CLR_NZV; SET_NZ16(sreg); \
}

/* $dF STU (STS) direct -**0- */
#define stu_di() \
{ \
	CLR_NZV; SET_NZ16(ureg); \
	DIRECT; M_WRMEM_WORD(eaddr,ureg); \
}

/* $10dF STS direct -**0- */
#define sts_di() \
{ \
	CLR_NZV; SET_NZ16(sreg); \
	DIRECT; M_WRMEM_WORD(eaddr,sreg); \
}

/* $e0 SUBB indexed ?**** */
#define subb_ix() \
{ \
	word	t,r; \
	t = M_RDMEM(eaddr); r = breg-t; \
	CLR_NZVC; SET_FLAGS8(breg,t,r); \
	breg = r; \
}

/* $e1 CMPB indexed ?**** */
#define cmpb_ix() \
{ \
	word	t,r; \
	t = M_RDMEM(eaddr); r = breg-t; \
	CLR_NZVC; SET_FLAGS8(breg,t,r); \
}

/* $e2 SBCB indexed ?**** */
#define sbcb_ix() \
{ \
	word	t,r; \
	t = M_RDMEM(eaddr); r = breg-t-(cc&0x01); \
	CLR_NZVC; SET_FLAGS8(breg,t,r); \
	breg = r; \
}

/* $e3 ADDD indexed -**** */
#define addd_ix() \
{ \
	dword r,d,b; \
	b = M_RDMEM_WORD(eaddr); d = GETDREG; r = d+b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
	SETDREG(r); \
}

/* $e4 ANDB indexed -**0- */
#define andb_ix() \
{ \
	breg &= M_RDMEM(eaddr); \
	CLR_NZV; SET_NZ8(breg); \
}

/* $e5 BITB indexed -**0- */
#define bitb_ix() \
{ \
	byte r; \
	r = breg&M_RDMEM(eaddr); \
	CLR_NZV; SET_NZ8(r); \
}

/* $e6 LDB indexed -**0- */
#define ldb_ix() \
{ \
	breg = M_RDMEM(eaddr); \
	CLR_NZV; SET_NZ8(breg); \
}

/* $e7 STB indexed -**0- */
#define stb_ix() \
{ \
	CLR_NZV; SET_NZ8(breg); \
	M_WRMEM(eaddr,breg); \
}

/* $e8 EORB indexed -**0- */
#define eorb_ix() \
{ \
	breg ^= M_RDMEM(eaddr); \
	CLR_NZV; SET_NZ8(breg); \
}

/* $e9 ADCB indexed ***** */
#define adcb_ix() \
{ \
	word t,r; \
	t = M_RDMEM(eaddr); r = breg+t+(cc&0x01); \
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r); \
	breg = r; \
}

/* $eA ORB indexed -**0- */
#define orb_ix() \
{ \
	breg |= M_RDMEM(eaddr); \
	CLR_NZV; SET_NZ8(breg); \
}

/* $eB ADDB indexed ***** */
#define addb_ix() \
{ \
	word t,r; \
	t = M_RDMEM(eaddr); r = breg+t; \
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r); \
	breg = r; \
}

/* $eC LDD indexed -**0- */
#define ldd_ix() \
{ \
	word t; \
	t = M_RDMEM_WORD(eaddr); SETDREG(t); \
	CLR_NZV; SET_NZ16(t); \
}

/* $eD STD indexed -**0- */
#define std_ix() \
{ \
	word t; \
	t=GETDREG; \
	CLR_NZV; SET_NZ16(t); \
	M_WRMEM_WORD(eaddr,t); \
}

/* $eE LDU (LDS) indexed -**0- */
#define ldu_ix() \
{ \
	ureg = M_RDMEM_WORD(eaddr); \
	CLR_NZV; SET_NZ16(ureg); \
}

/* $10eE LDS indexed -**0- */
#define lds_ix() \
{ \
	sreg = M_RDMEM_WORD(eaddr); \
	CLR_NZV; SET_NZ16(sreg); \
}

/* $eF STU (STS) indexed -**0- */
#define stu_ix() \
{ \
	CLR_NZV; SET_NZ16(ureg); \
	M_WRMEM_WORD(eaddr,ureg); \
}

/* $10eF STS indexed -**0- */
#define sts_ix() \
{ \
	CLR_NZV; SET_NZ16(sreg); \
	M_WRMEM_WORD(eaddr,sreg); \
}

/* $f0 SUBB extended ?**** */
#define subb_ex() \
{ \
	word	t,r; \
	EXTBYTE(t); r = breg-t; \
	CLR_NZVC; SET_FLAGS8(breg,t,r); \
	breg = r; \
}

/* $f1 CMPB extended ?**** */
#define cmpb_ex() \
{ \
	word	t,r; \
	EXTBYTE(t); r = breg-t; \
	CLR_NZVC; SET_FLAGS8(breg,t,r); \
}

/* $f2 SBCB extended ?**** */
#define sbcb_ex() \
{ \
	word	t,r; \
	EXTBYTE(t); r = breg-t-(cc&0x01); \
	CLR_NZVC; SET_FLAGS8(breg,t,r); \
	breg = r; \
}

/* $f3 ADDD extended -**** */
#define addd_ex() \
{ \
	dword r,d,b; \
	EXTWORD(b); d = GETDREG; r = d+b; \
	CLR_NZVC; SET_FLAGS16(d,b,r); \
	SETDREG(r); \
}

/* $f4 ANDB extended -**0- */
#define andb_ex() \
{ \
	byte t; \
	EXTBYTE(t); breg &= t; \
	CLR_NZV; SET_NZ8(breg); \
}

/* $f5 BITB extended -**0- */
#define bitb_ex() \
{ \
	byte t,r; \
	EXTBYTE(t); r = breg&t; \
	CLR_NZV; SET_NZ8(r); \
}

/* $f6 LDB extended -**0- */
#define ldb_ex() \
{ \
	EXTBYTE(breg); \
	CLR_NZV; SET_NZ8(breg); \
}

/* $f7 STB extended -**0- */
#define stb_ex() \
{ \
	CLR_NZV; SET_NZ8(breg); \
	EXTENDED; M_WRMEM(eaddr,breg); \
}

/* $f8 EORB extended -**0- */
#define eorb_ex() \
{ \
	byte t; \
	EXTBYTE(t); breg ^= t; \
	CLR_NZV; SET_NZ8(breg); \
}

/* $f9 ADCB extended ***** */
#define adcb_ex() \
{ \
	word t,r; \
	EXTBYTE(t); r = breg+t+(cc&0x01); \
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r); \
	breg = r; \
}

/* $fA ORB extended -**0- */
#define orb_ex() \
{ \
	byte t; \
	EXTBYTE(t); breg |= t; \
	CLR_NZV; SET_NZ8(breg); \
}

/* $fB ADDB extended ***** */
#define addb_ex() \
{ \
	word t,r; \
	EXTBYTE(t); r = breg+t; \
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r); \
	breg = r; \
}

/* $fC LDD extended -**0- */
#define ldd_ex() \
{ \
	word t; \
	EXTWORD(t); SETDREG(t); \
	CLR_NZV; SET_NZ16(t); \
}

/* $fD STD extended -**0- */
#define std_ex() \
{ \
	word t; \
	EXTENDED; t=GETDREG; \
	CLR_NZV; SET_NZ16(t); \
	M_WRMEM_WORD(eaddr,t); \
}

/* $fE LDU (LDS) extended -**0- */
#define ldu_ex() \
{ \
	EXTWORD(ureg); \
	CLR_NZV; SET_NZ16(ureg); \
}

/* $10fE LDS extended -**0- */
#define lds_ex() \
{ \
	EXTWORD(sreg); \
	CLR_NZV; SET_NZ16(sreg); \
}

/* $fF STU (STS) extended -**0- */
#define stu_ex() \
{ \
	CLR_NZV; SET_NZ16(ureg); \
	EXTENDED; M_WRMEM_WORD(eaddr,ureg); \
}

/* $10fF STS extended -**0- */
#define sts_ex() \
{ \
	CLR_NZV; SET_NZ16(sreg); \
	EXTENDED; M_WRMEM_WORD(eaddr,sreg); \
}
