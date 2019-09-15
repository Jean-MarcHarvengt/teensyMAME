
/*

HNZC

? = undefined
* = affected
- = unaffected
0 = cleared
1 = set
# = ccr directly affected by instruction
@ = special - carry set if bit 7 is set

*/

static void illegal( void )
{
}

#if macintosh
#pragma mark ____0x____
#endif

/* $00/$02/$04/$06/$08/$0A/$0C/$0E BRSET direct,relative ---- */
INLINE void brset (byte bit)
{
	byte t,r;
	DIRBYTE(r);
	IMMBYTE(t);
	if (r&bit)
		pcreg+=SIGNED(t);
	else
		/* speed up busy loops */
		if (t==0xfd) m6805_ICount = 0;
}

/* $01/$03/$05/$07/$09/$0B/$0D/$0F BRCLR direct,relative ---- */
INLINE void brclr (byte bit)
{
	byte t,r;
	DIRBYTE(r);
	IMMBYTE(t);
	if (!(r&bit))
		pcreg+=SIGNED(t);
	else
		/* speed up busy loops */
		if (t==0xfd) m6805_ICount = 0;
}


#if macintosh
#pragma mark ____1x____
#endif

/* $10/$12/$14/$16/$18/$1A/$1C/$1E BSET direct ---- */
INLINE void bset (byte bit)
{
	byte t,r;
	DIRBYTE(t); r=t|bit;
	M_WRMEM(eaddr,r);
}

/* $11/$13/$15/$17/$19/$1B/$1D/$1F BCLR direct ---- */
INLINE void bclr (byte bit)
{
	byte t,r;
	DIRBYTE(t); r=t&(~bit);
	M_WRMEM(eaddr,r);
}


#if macintosh
#pragma mark ____2x____
#endif

/* $20 BRA relative ---- */
INLINE void bra( void )
{
	byte t;
	IMMBYTE(t);pcreg+=SIGNED(t);
	/* speed up busy loops */
	if (t==0xfe) m6805_ICount = 0;
}

/* $21 BRN relative ---- */
INLINE void brn( void )
{
	byte t;
	IMMBYTE(t);
}

/* $22 BHI relative ---- */
INLINE void bhi( void )
{
	byte t;
	BRANCH(!(cc&(CFLAG|ZFLAG)));
}

/* $23 BLS relative ---- */
INLINE void bls( void )
{
	byte t;
	BRANCH(cc&(CFLAG|ZFLAG));
}

/* $24 BCC relative ---- */
INLINE void bcc( void )
{
	byte t;
	BRANCH(!(cc&CFLAG));
}

/* $25 BCS relative ---- */
INLINE void bcs( void )
{
	byte t;
	BRANCH(cc&CFLAG);
}

/* $26 BNE relative ---- */
INLINE void bne( void )
{
	byte t;
	BRANCH(!(cc&ZFLAG));
}

/* $27 BEQ relative ---- */
INLINE void beq( void )
{
	byte t;
	BRANCH(cc&ZFLAG);
}

/* $28 BHCC relative ---- */
INLINE void bhcc( void )
{
	byte t;
	BRANCH(!(cc&HFLAG));
}

/* $29 BHCS relative ---- */
INLINE void bhcs( void )
{
	byte t;
	BRANCH(cc&HFLAG);
}

/* $2a BPL relative ---- */
INLINE void bpl( void )
{
	byte t;
	BRANCH(!(cc&NFLAG));
}

/* $2b BMI relative ---- */
INLINE void bmi( void )
{
	byte t;
	BRANCH(cc&NFLAG);
}

/* $2c BMC relative ---- */
INLINE void bmc( void )
{
	byte t;
	BRANCH(!(cc&IFLAG));
}

/* $2d BMS relative ---- */
INLINE void bms( void )
{
	byte t;
	BRANCH(cc&IFLAG);
}

/* $2e BIL relative ---- */
INLINE void bil( void )
{
	byte t;
	BRANCH(pending_interrupts&M6805_INT_IRQ);
}

/* $2f BIH relative ---- */
INLINE void bih( void )
{
	byte t;
extern int taito_68705_ih(void);
	BRANCH(taito_68705_ih());
}


#if macintosh
#pragma mark ____3x____
#endif

/* $30 NEG direct -*** */
INLINE void neg_di( void )
{
	byte t;
	word r;
	DIRBYTE(t); r=-t;
	CLR_NZC; SET_FLAGS8(0,t,r);
	M_WRMEM(eaddr,r);
}

/* $31 ILLEGAL */

/* $32 ILLEGAL */

/* $33 COM direct -**1 */
INLINE void com_di( void )
{
	byte t;
	DIRBYTE(t); t = ~t;
	CLR_NZ; SET_NZ8(t); SEC;
	M_WRMEM(eaddr,t);
}

/* $34 LSR direct -0** */
INLINE void lsr_di( void )
{
	byte t;
	DIRBYTE(t);
	CLR_NZC; cc|=(t&0x01);
	t>>=1; SET_Z8(t);
	M_WRMEM(eaddr,t);
}

/* $35 ILLEGAL */

/* $36 ROR direct -*** */
INLINE void ror_di( void )
{
	byte t,r;
	DIRBYTE(t); r=(cc&0x01)<<7;
	CLR_NZC; cc|=(t&0x01);
	r |= t>>1; SET_NZ8(r);
	M_WRMEM(eaddr,r);
}

/* $37 ASR direct ?*** */
INLINE void asr_di( void )
{
	byte t;
	DIRBYTE(t);
	CLR_NZC; cc|=(t&0x01);
	t>>=1; t|=((t&0x40)<<1);
	SET_NZ8(t);
	M_WRMEM(eaddr,t);
}

/* $38 LSL direct ?*** */
INLINE void lsl_di( void )
{
	byte t;
	word r;
	DIRBYTE(t); r=t<<1;
	CLR_NZC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $39 ROL direct -*** */
INLINE void rol_di( void )
{
	word t,r;
	DIRBYTE(t); r = cc&0x01; r |= t<<1;
	CLR_NZC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $3a DEC direct -**- */
INLINE void dec_di( void )
{
	byte t;
	DIRBYTE(t);
	--t;
	CLR_NZ; SET_FLAGS8D(t);
	M_WRMEM(eaddr,t);
}

/* $3b ILLEGAL */

/* $3c INC direct -**- */
INLINE void inc_di( void )
{
	byte t;
	DIRBYTE(t);
	++t;
	CLR_NZ; SET_FLAGS8I(t);
	M_WRMEM(eaddr,t);
}

/* $3d TST direct -**- */
INLINE void tst_di( void )
{
	byte t;
	DIRBYTE(t);
	CLR_NZ; SET_NZ8(t);
}

/* $3e ILLEGAL */

/* $3f CLR direct -0100 */
INLINE void clr_di( void )
{
	DIRECT;
	CLR_NZC; SEZ;
	M_WRMEM(eaddr,0);
}


#if macintosh
#pragma mark ____4x____
#endif

/* $40 NEGA inherent ?*** */
INLINE void nega( void )
{
	word r;
	r=-areg;
	CLR_NZC; SET_FLAGS8(0,areg,r);
	areg=r;
}

/* $41 ILLEGAL */

/* $42 ILLEGAL */

/* $43 COMA inherent -**1 */
INLINE void coma( void )
{
	areg = ~areg;
	CLR_NZ; SET_NZ8(areg); SEC;
}

/* $44 LSRA inherent -0** */
INLINE void lsra( void )
{
	CLR_NZC; cc|=(areg&0x01);
	areg>>=1; SET_Z8(areg);
}

/* $45 ILLEGAL */

/* $46 RORA inherent -*** */
INLINE void rora( void )
{
	byte r;
	r=(cc&0x01)<<7;
	CLR_NZC; cc|=(areg&0x01);
	r |= areg>>1; SET_NZ8(r);
	areg=r;
}

/* $47 ASRA inherent ?*** */
INLINE void asra( void )
{
	CLR_NZC; cc|=(areg&0x01);
	areg>>=1; areg|=((areg&0x40)<<1);
	SET_NZ8(areg);
}

/* $48 LSLA inherent ?*** */
INLINE void lsla( void )
{
	word r;
	r=areg<<1;
	CLR_NZC; SET_FLAGS8(areg,areg,r);
	areg=r;
}

/* $49 ROLA inherent -*** */
INLINE void rola( void )
{
	word t,r;
	t = areg; r = cc&0x01; r |= t<<1;
	CLR_NZC; SET_FLAGS8(t,t,r);
	areg=r;
}

/* $4a DECA inherent -**- */
INLINE void deca( void )
{
	--areg;
	CLR_NZ; SET_FLAGS8D(areg);
}

/* $4b ILLEGAL */

/* $4c INCA inherent -**- */
INLINE void inca( void )
{
	++areg;
	CLR_NZ; SET_FLAGS8I(areg);
}

/* $4d TSTA inherent -**- */
INLINE void tsta( void )
{
	CLR_NZ; SET_NZ8(areg);
}

/* $4e ILLEGAL */

/* $4f CLRA inherent -010 */
INLINE void clra( void )
{
	areg=0;
	CLR_NZC; SEZ;
}


#if macintosh
#pragma mark ____5x____
#endif

/* $50 NEGX inherent ?*** */
INLINE void negx( void )
{
	word r;
	r=-xreg;
	CLR_NZC; SET_FLAGS8(0,xreg,r);
	xreg=r;
}

/* $51 ILLEGAL */

/* $52 ILLEGAL */

/* $53 COMX inherent -**1 */
INLINE void comx( void )
{
	xreg = ~xreg;
	CLR_NZ; SET_NZ8(xreg); SEC;
}

/* $54 LSRX inherent -0** */
INLINE void lsrx( void )
{
	CLR_NZC; cc|=(xreg&0x01);
	xreg>>=1; SET_Z8(xreg);
}

/* $55 ILLEGAL */

/* $56 RORX inherent -*** */
INLINE void rorx( void )
{
	byte r;
	r=(cc&0x01)<<7;
	CLR_NZC; cc|=(xreg&0x01);
	r |= xreg>>1; SET_NZ8(r);
	xreg=r;
}

/* $57 ASRX inherent ?*** */
INLINE void asrx( void )
{
	CLR_NZC; cc|=(xreg&0x01);
	xreg>>=1; xreg|=((xreg&0x40)<<1);
	SET_NZ8(xreg);
}

/* $58 ASLX inherent ?*** */
INLINE void aslx( void )
{
	word r;
	r=xreg<<1;
	CLR_NZC; SET_FLAGS8(xreg,xreg,r);
	xreg=r;
}

/* $59 ROLX inherent -*** */
INLINE void rolx( void )
{
	word t,r;
	t = xreg; r = cc&0x01; r |= t<<1;
	CLR_NZC; SET_FLAGS8(t,t,r);
	xreg=r;
}

/* $5a DECX inherent -**- */
INLINE void decx( void )
{
	--xreg;
	CLR_NZ; SET_FLAGS8D(xreg);
}

/* $5b ILLEGAL */

/* $5c INCX inherent -**- */
INLINE void incx( void )
{
	++xreg;
	CLR_NZ; SET_FLAGS8I(xreg);
}

/* $5d TSTX inherent -**- */
INLINE void tstx( void )
{
	CLR_NZ; SET_NZ8(xreg);
}

/* $5e ILLEGAL */

/* $5f CLRX inherent -010 */
INLINE void clrx( void )
{
	xreg=0;
	CLR_NZC; SEZ;
}


#if macintosh
#pragma mark ____6x____
#endif

/* $60 NEG indexed, 1 byte offset -*** */
INLINE void neg_ix1( void )
{
	byte t;
	word r;
	IDX1BYTE(t); r=-t;
	CLR_NZC; SET_FLAGS8(0,t,r);
	M_WRMEM(eaddr,r);
}

/* $61 ILLEGAL */

/* $62 ILLEGAL */

/* $63 COM indexed, 1 byte offset -**1 */
INLINE void com_ix1( void )
{
	byte t;
	IDX1BYTE(t); t = ~t;
	CLR_NZ; SET_NZ8(t); SEC;
	M_WRMEM(eaddr,t);
}

/* $64 LSR indexed, 1 byte offset -0** */
INLINE void lsr_ix1( void )
{
	byte t;
	IDX1BYTE(t);
	CLR_NZC; cc|=(t&0x01);
	t>>=1; SET_Z8(t);
	M_WRMEM(eaddr,t);
}

/* $65 ILLEGAL */

/* $66 ROR indexed, 1 byte offset -*** */
INLINE void ror_ix1( void )
{
	byte t,r;
	IDX1BYTE(t); r=(cc&0x01)<<7;
	CLR_NZC; cc|=(t&0x01);
	r |= t>>1; SET_NZ8(r);
	M_WRMEM(eaddr,r);
}

/* $67 ASR indexed, 1 byte offset ?*** */
INLINE void asr_ix1( void )
{
	byte t;
	IDX1BYTE(t);
	CLR_NZC; cc|=(t&0x01);
	t>>=1; t|=((t&0x40)<<1);
	SET_NZ8(t);
	M_WRMEM(eaddr,t);
}

/* $68 LSL indexed, 1 byte offset ?*** */
INLINE void lsl_ix1( void )
{
	byte t;
	word r;
	IDX1BYTE(t); r=t<<1;
	CLR_NZC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $69 ROL indexed, 1 byte offset -*** */
INLINE void rol_ix1( void )
{
	word t,r;
	IDX1BYTE(t); r = cc&0x01; r |= t<<1;
	CLR_NZC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $6a DEC indexed, 1 byte offset -**- */
INLINE void dec_ix1( void )
{
	byte t;
	IDX1BYTE(t);
	--t;
	CLR_NZ; SET_FLAGS8D(t);
	M_WRMEM(eaddr,t);
}

/* $6b ILLEGAL */

/* $6c INC indexed, 1 byte offset -**- */
INLINE void inc_ix1( void )
{
	byte t;
	IDX1BYTE(t);
	++t;
	CLR_NZ; SET_FLAGS8I(t);
	M_WRMEM(eaddr,t);
}

/* $6d TST indexed, 1 byte offset -**- */
INLINE void tst_ix1( void )
{
	byte t;
	IDX1BYTE(t);
	CLR_NZ; SET_NZ8(t);
}

/* $6e ILLEGAL */

/* $6f CLR indexed, 1 byte offset -0100 */
INLINE void clr_ix1( void )
{
	INDEXED1;
	CLR_NZC; SEZ;
	M_WRMEM(eaddr,0);
}


#if macintosh
#pragma mark ____7x____
#endif

/* $70 NEG indexed -*** */
INLINE void neg_ix( void )
{
	byte t;
	word r;
	IDXBYTE(t); r=-t;
	CLR_NZC; SET_FLAGS8(0,t,r);
	M_WRMEM(eaddr,r);
}

/* $71 ILLEGAL */

/* $72 ILLEGAL */

/* $73 COM indexed -**1 */
INLINE void com_ix( void )
{
	byte t;
	IDXBYTE(t); t = ~t;
	CLR_NZ; SET_NZ8(t); SEC;
	M_WRMEM(eaddr,t);
}

/* $74 LSR indexed -0** */
INLINE void lsr_ix( void )
{
	byte t;
	IDXBYTE(t);
	CLR_NZC; cc|=(t&0x01);
	t>>=1; SET_Z8(t);
	M_WRMEM(eaddr,t);
}

/* $75 ILLEGAL */

/* $76 ROR indexed -*** */
INLINE void ror_ix( void )
{
	byte t,r;
	IDXBYTE(t); r=(cc&0x01)<<7;
	CLR_NZC; cc|=(t&0x01);
	r |= t>>1; SET_NZ8(r);
	M_WRMEM(eaddr,r);
}

/* $77 ASR indexed ?*** */
INLINE void asr_ix( void )
{
	byte t;
	IDXBYTE(t);
	CLR_NZC; cc|=(t&0x01);
	t>>=1; t|=((t&0x40)<<1);
	SET_NZ8(t);
	M_WRMEM(eaddr,t);
}

/* $78 LSL indexed ?*** */
INLINE void lsl_ix( void )
{
	byte t;
	word r;
	IDXBYTE(t); r=t<<1;
	CLR_NZC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $79 ROL indexed -*** */
INLINE void rol_ix( void )
{
	word t,r;
	IDXBYTE(t); r = cc&0x01; r |= t<<1;
	CLR_NZC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $7a DEC indexed -**- */
INLINE void dec_ix( void )
{
	byte t;
	IDXBYTE(t);
	--t;
	CLR_NZ; SET_FLAGS8D(t);
	M_WRMEM(eaddr,t);
}

/* $7b ILLEGAL */

/* $7c INC indexed -**- */
INLINE void inc_ix( void )
{
	byte t;
	IDXBYTE(t);
	++t;
	CLR_NZ; SET_FLAGS8I(t);
	M_WRMEM(eaddr,t);
}

/* $7d TST indexed -**- */
INLINE void tst_ix( void )
{
	byte t;
	IDXBYTE(t);
	CLR_NZ; SET_NZ8(t);
}

/* $7e ILLEGAL */

/* $7f CLR indexed -0100 */
INLINE void clr_ix( void )
{
	INDEXED;
	CLR_NZC; SEZ;
	M_WRMEM(eaddr,0);
}


#if macintosh
#pragma mark ____8x____
#endif

/* $80 RTI inherent #### */
INLINE void rti( void )
{
	PULLBYTE(cc);
	PULLBYTE(areg);
	PULLBYTE(xreg);
	PULLWORD(pcreg);
}

/* $81 RTS inherent ---- */
INLINE void rts( void )
{
	PULLWORD(pcreg);
}

/* $82 ILLEGAL */

/* $83 SWI absolute indirect ---- */
INLINE void swi( void )
{
	PUSHWORD(pcreg);
	PUSHBYTE(xreg);
	PUSHBYTE(areg);
	PUSHBYTE(cc);
	SEI;
	pcreg = M_RDMEM_WORD(0x07fc);
}

/* $84 ILLEGAL */

/* $85 ILLEGAL */

/* $86 ILLEGAL */

/* $87 ILLEGAL */

/* $88 ILLEGAL */

/* $89 ILLEGAL */

/* $8A ILLEGAL */

/* $8B ILLEGAL */

/* $8C ILLEGAL */

/* $8D ILLEGAL */

/* $8E ILLEGAL */

/* $8F ILLEGAL */


#if macintosh
#pragma mark ____9x____
#endif

/* $90 ILLEGAL */

/* $91 ILLEGAL */

/* $92 ILLEGAL */

/* $93 ILLEGAL */

/* $94 ILLEGAL */

/* $95 ILLEGAL */

/* $96 ILLEGAL */

/* $97 TAX inherent ---- */
INLINE void tax (void)
{
	xreg=areg;
}

/* $98 CLC */

/* $99 SEC */

/* $9A CLI */

/* $9B SEI */

/* $9C RSP inherent ---- */
INLINE void rsp (void)
{
	sreg = 0x7f;
}

/* $9D NOP inherent ---- */
INLINE void nop (void)
{
}

/* $9E ILLEGAL */

/* $9F TXA inherent ---- */
INLINE void txa (void)
{
	areg=xreg;
}


#if macintosh
#pragma mark ____Ax____
#endif

/* $a0 SUBA immediate ?*** */
INLINE void suba_im( void )
{
	word	t,r;
	IMMBYTE(t); r = areg-t;
	CLR_NZC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $a1 CMPA immediate ?*** */
INLINE void cmpa_im( void )
{
	word	t,r;
	IMMBYTE(t); r = areg-t;
	CLR_NZC; SET_FLAGS8(areg,t,r);
}

/* $a2 SBCA immediate ?*** */
INLINE void sbca_im( void )
{
	word	t,r;
	IMMBYTE(t); r = areg-t-(cc&0x01);
	CLR_NZC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $a3 CPX immediate -*** */
INLINE void cpx_im( void )
{
	word	t,r;
	IMMBYTE(t); r = xreg-t;
	CLR_NZC; SET_FLAGS8(xreg,t,r);
}

/* $a4 ANDA immediate -**- */
INLINE void anda_im( void )
{
	byte t;
	IMMBYTE(t); areg &= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $a5 BITA immediate -**- */
INLINE void bita_im( void )
{
	byte t,r;
	IMMBYTE(t); r = areg&t;
	CLR_NZ; SET_NZ8(r);
}

/* $a6 LDA immediate -**- */
INLINE void lda_im( void )
{
	IMMBYTE(areg);
	CLR_NZ; SET_NZ8(areg);
}

/* $a7 ILLEGAL */

/* $a8 EORA immediate -**- */
INLINE void eora_im( void )
{
	byte t;
	IMMBYTE(t); areg ^= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $a9 ADCA immediate **** */
INLINE void adca_im( void )
{
	word t,r;
	IMMBYTE(t); r = areg+t+(cc&0x01);
	CLR_HNZC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $aa ORA immediate -**- */
INLINE void ora_im( void )
{
	byte t;
	IMMBYTE(t); areg |= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $ab ADDA immediate **** */
INLINE void adda_im( void )
{
	word t,r;
	IMMBYTE(t); r = areg+t;
	CLR_HNZC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $ac ILLEGAL */

/* $ad BSR ---- */
INLINE void bsr( void )
{
	byte t;
	IMMBYTE(t); PUSHWORD(pcreg); pcreg += SIGNED(t);
}

/* $ae LDX immediate -**- */
INLINE void ldx_im( void )
{
	IMMBYTE(xreg);
	CLR_NZ; SET_NZ8(xreg);
}

/* $af ILLEGAL */


#if macintosh
#pragma mark ____Bx____
#endif

/* $b0 SUBA direct ?*** */
INLINE void suba_di( void )
{
	word	t,r;
	DIRBYTE(t); r = areg-t;
	CLR_NZC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $b1 CMPA direct ?*** */
INLINE void cmpa_di( void )
{
	word	t,r;
	DIRBYTE(t); r = areg-t;
	CLR_NZC; SET_FLAGS8(areg,t,r);
}

/* $b2 SBCA direct ?*** */
INLINE void sbca_di( void )
{
	word	t,r;
	DIRBYTE(t); r = areg-t-(cc&0x01);
	CLR_NZC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $b3 CPX direct -*** */
INLINE void cpx_di( void )
{
	word	t,r;
	DIRBYTE(t); r = xreg-t;
	CLR_NZC; SET_FLAGS8(xreg,t,r);
}

/* $b4 ANDA direct -**- */
INLINE void anda_di( void )
{
	byte t;
	DIRBYTE(t); areg &= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $b5 BITA direct -**- */
INLINE void bita_di( void )
{
	byte t,r;
	DIRBYTE(t); r = areg&t;
	CLR_NZ; SET_NZ8(r);
}

/* $b6 LDA direct -**- */
INLINE void lda_di( void )
{
	DIRBYTE(areg);
	CLR_NZ; SET_NZ8(areg);
}

/* $b7 STA direct -**- */
INLINE void sta_di( void )
{
	CLR_NZ; SET_NZ8(areg);
	DIRECT; M_WRMEM(eaddr,areg);
}

/* $b8 EORA direct -**- */
INLINE void eora_di( void )
{
	byte t;
	DIRBYTE(t); areg ^= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $b9 ADCA direct **** */
INLINE void adca_di( void )
{
	word t,r;
	DIRBYTE(t); r = areg+t+(cc&0x01);
	CLR_HNZC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $ba ORA direct -**- */
INLINE void ora_di( void )
{
	byte t;
	DIRBYTE(t); areg |= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $bb ADDA direct **** */
INLINE void adda_di( void )
{
	word t,r;
	DIRBYTE(t); r = areg+t;
	CLR_HNZC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $bc JMP direct -*** */
INLINE void jmp_di( void )
{
	DIRECT;
	pcreg = eaddr;
}

/* $bd JSR direct ---- */
INLINE void jsr_di( void )
{
	DIRECT; PUSHWORD(pcreg);
	pcreg = eaddr;
}

/* $be LDX direct -**- */
INLINE void ldx_di( void )
{
	DIRBYTE(xreg);
	CLR_NZ; SET_NZ8(xreg);
}

/* $bf STX direct -**- */
INLINE void stx_di( void )
{
	CLR_NZ; SET_NZ8(xreg);
	DIRECT; M_WRMEM(eaddr,xreg);
}


#if macintosh
#pragma mark ____Cx____
#endif

/* $c0 SUBA extended ?*** */
INLINE void suba_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = areg-t;
	CLR_NZC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $c1 CMPA extended ?*** */
INLINE void cmpa_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = areg-t;
	CLR_NZC; SET_FLAGS8(areg,t,r);
}

/* $c2 SBCA extended ?*** */
INLINE void sbca_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = areg-t-(cc&0x01);
	CLR_NZC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $c3 CPX extended -*** */
INLINE void cpx_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = xreg-t;
	CLR_NZC; SET_FLAGS8(xreg,t,r);
}

/* $c4 ANDA extended -**- */
INLINE void anda_ex( void )
{
	byte t;
	EXTBYTE(t); areg &= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $c5 BITA extended -**- */
INLINE void bita_ex( void )
{
	byte t,r;
	EXTBYTE(t); r = areg&t;
	CLR_NZ; SET_NZ8(r);
}

/* $c6 LDA extended -**- */
INLINE void lda_ex( void )
{
	EXTBYTE(areg);
	CLR_NZ; SET_NZ8(areg);
}

/* $c7 STA extended -**- */
INLINE void sta_ex( void )
{
	CLR_NZ; SET_NZ8(areg);
	EXTENDED; M_WRMEM(eaddr,areg);
}

/* $c8 EORA extended -**- */
INLINE void eora_ex( void )
{
	byte t;
	EXTBYTE(t); areg ^= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $c9 ADCA extended **** */
INLINE void adca_ex( void )
{
	word t,r;
	EXTBYTE(t); r = areg+t+(cc&0x01);
	CLR_HNZC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $ca ORA extended -**- */
INLINE void ora_ex( void )
{
	byte t;
	EXTBYTE(t); areg |= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $cb ADDA extended **** */
INLINE void adda_ex( void )
{
	word t,r;
	EXTBYTE(t); r = areg+t;
	CLR_HNZC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $cc JMP extended -*** */
INLINE void jmp_ex( void )
{
	EXTENDED;
	pcreg = eaddr;
}

/* $cd JSR extended ---- */
INLINE void jsr_ex( void )
{
	EXTENDED; PUSHWORD(pcreg);
	pcreg = eaddr;
}

/* $ce LDX extended -**- */
INLINE void ldx_ex( void )
{
	EXTBYTE(xreg);
	CLR_NZ; SET_NZ8(xreg);
}

/* $cf STX extended -**- */
INLINE void stx_ex( void )
{
	CLR_NZ; SET_NZ8(xreg);
	EXTENDED; M_WRMEM(eaddr,xreg);
}


#if macintosh
#pragma mark ____Dx____
#endif

/* $d0 SUBA indexed, 2 byte offset ?*** */
INLINE void suba_ix2( void )
{
	word	t,r;
	IDX2BYTE(t); r = areg-t;
	CLR_NZC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $d1 CMPA indexed, 2 byte offset ?*** */
INLINE void cmpa_ix2( void )
{
	word	t,r;
	IDX2BYTE(t); r = areg-t;
	CLR_NZC; SET_FLAGS8(areg,t,r);
}

/* $d2 SBCA indexed, 2 byte offset ?*** */
INLINE void sbca_ix2( void )
{
	word	t,r;
	IDX2BYTE(t); r = areg-t-(cc&0x01);
	CLR_NZC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $d3 CPX indexed, 2 byte offset -*** */
INLINE void cpx_ix2( void )
{
	word	t,r;
	IDX2BYTE(t); r = xreg-t;
	CLR_NZC; SET_FLAGS8(xreg,t,r);
}

/* $d4 ANDA indexed, 2 byte offset -**- */
INLINE void anda_ix2( void )
{
	byte t;
	IDX2BYTE(t); areg &= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $d5 BITA indexed, 2 byte offset -**- */
INLINE void bita_ix2( void )
{
	byte t,r;
	IDX2BYTE(t); r = areg&t;
	CLR_NZ; SET_NZ8(r);
}

/* $d6 LDA indexed, 2 byte offset -**- */
INLINE void lda_ix2( void )
{
	IDX2BYTE(areg);
	CLR_NZ; SET_NZ8(areg);
}

/* $d7 STA indexed, 2 byte offset -**- */
INLINE void sta_ix2( void )
{
	CLR_NZ; SET_NZ8(areg);
	INDEXED2; M_WRMEM(eaddr,areg);
}

/* $d8 EORA indexed, 2 byte offset -**- */
INLINE void eora_ix2( void )
{
	byte t;
	IDX2BYTE(t); areg ^= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $d9 ADCA indexed, 2 byte offset **** */
INLINE void adca_ix2( void )
{
	word t,r;
	IDX2BYTE(t); r = areg+t+(cc&0x01);
	CLR_HNZC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $da ORA indexed, 2 byte offset -**- */
INLINE void ora_ix2( void )
{
	byte t;
	IDX2BYTE(t); areg |= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $db ADDA indexed, 2 byte offset **** */
INLINE void adda_ix2( void )
{
	word t,r;
	IDX2BYTE(t); r = areg+t;
	CLR_HNZC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $dc JMP indexed, 2 byte offset -*** */
INLINE void jmp_ix2( void )
{
	INDEXED2;
	pcreg = eaddr;
}

/* $dd JSR indexed, 2 byte offset ---- */
INLINE void jsr_ix2( void )
{
	INDEXED2; PUSHWORD(pcreg);
	pcreg = eaddr;
}

/* $de LDX indexed, 2 byte offset -**- */
INLINE void ldx_ix2( void )
{
	IDX2BYTE(xreg);
	CLR_NZ; SET_NZ8(xreg);
}

/* $df STX indexed, 2 byte offset -**- */
INLINE void stx_ix2( void )
{
	CLR_NZ; SET_NZ8(xreg);
	INDEXED2; M_WRMEM(eaddr,xreg);
}


#if macintosh
#pragma mark ____Ex____
#endif

/* $e0 SUBA indexed, 1 byte offset ?*** */
INLINE void suba_ix1( void )
{
	word	t,r;
	IDX1BYTE(t); r = areg-t;
	CLR_NZC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $e1 CMPA indexed, 1 byte offset ?*** */
INLINE void cmpa_ix1( void )
{
	word	t,r;
	IDX1BYTE(t); r = areg-t;
	CLR_NZC; SET_FLAGS8(areg,t,r);
}

/* $e2 SBCA indexed, 1 byte offset ?*** */
INLINE void sbca_ix1( void )
{
	word	t,r;
	IDX1BYTE(t); r = areg-t-(cc&0x01);
	CLR_NZC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $e3 CPX indexed, 1 byte offset -*** */
INLINE void cpx_ix1( void )
{
	word	t,r;
	IDX1BYTE(t); r = xreg-t;
	CLR_NZC; SET_FLAGS8(xreg,t,r);
}

/* $e4 ANDA indexed, 1 byte offset -**- */
INLINE void anda_ix1( void )
{
	byte t;
	IDX1BYTE(t); areg &= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $e5 BITA indexed, 1 byte offset -**- */
INLINE void bita_ix1( void )
{
	byte t,r;
	IDX1BYTE(t); r = areg&t;
	CLR_NZ; SET_NZ8(r);
}

/* $e6 LDA indexed, 1 byte offset -**- */
INLINE void lda_ix1( void )
{
	IDX1BYTE(areg);
	CLR_NZ; SET_NZ8(areg);
}

/* $e7 STA indexed, 1 byte offset -**- */
INLINE void sta_ix1( void )
{
	CLR_NZ; SET_NZ8(areg);
	INDEXED1; M_WRMEM(eaddr,areg);
}

/* $e8 EORA indexed, 1 byte offset -**- */
INLINE void eora_ix1( void )
{
	byte t;
	IDX1BYTE(t); areg ^= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $e9 ADCA indexed, 1 byte offset **** */
INLINE void adca_ix1( void )
{
	word t,r;
	IDX1BYTE(t); r = areg+t+(cc&0x01);
	CLR_HNZC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $ea ORA indexed, 1 byte offset -**- */
INLINE void ora_ix1( void )
{
	byte t;
	IDX1BYTE(t); areg |= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $eb ADDA indexed, 1 byte offset **** */
INLINE void adda_ix1( void )
{
	word t,r;
	IDX1BYTE(t); r = areg+t;
	CLR_HNZC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $ec JMP indexed, 1 byte offset -*** */
INLINE void jmp_ix1( void )
{
	INDEXED1;
	pcreg = eaddr;
}

/* $ed JSR indexed, 1 byte offset ---- */
INLINE void jsr_ix1( void )
{
	INDEXED1; PUSHWORD(pcreg);
	pcreg = eaddr;
}

/* $ee LDX indexed, 1 byte offset -**- */
INLINE void ldx_ix1( void )
{
	IDX1BYTE(xreg);
	CLR_NZ; SET_NZ8(xreg);
}

/* $ef STX indexed, 1 byte offset -**- */
INLINE void stx_ix1( void )
{
	CLR_NZ; SET_NZ8(xreg);
	INDEXED1; M_WRMEM(eaddr,xreg);
}


#if macintosh
#pragma mark ____Fx____
#endif

/* $f0 SUBA indexed ?*** */
INLINE void suba_ix( void )
{
	word	t,r;
	IDXBYTE(t); r = areg-t;
	CLR_NZC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $f1 CMPA indexed ?*** */
INLINE void cmpa_ix( void )
{
	word	t,r;
	IDXBYTE(t); r = areg-t;
	CLR_NZC; SET_FLAGS8(areg,t,r);
}

/* $f2 SBCA indexed ?*** */
INLINE void sbca_ix( void )
{
	word	t,r;
	IDXBYTE(t); r = areg-t-(cc&0x01);
	CLR_NZC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $f3 CPX indexed -*** */
INLINE void cpx_ix( void )
{
	word	t,r;
	IDXBYTE(t); r = xreg-t;
	CLR_NZC; SET_FLAGS8(xreg,t,r);
}

/* $f4 ANDA indexed -**- */
INLINE void anda_ix( void )
{
	byte t;
	IDXBYTE(t); areg &= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $f5 BITA indexed -**- */
INLINE void bita_ix( void )
{
	byte t,r;
	IDXBYTE(t); r = areg&t;
	CLR_NZ; SET_NZ8(r);
}

/* $f6 LDA indexed -**- */
INLINE void lda_ix( void )
{
	IDXBYTE(areg);
	CLR_NZ; SET_NZ8(areg);
}

/* $f7 STA indexed -**- */
INLINE void sta_ix( void )
{
	CLR_NZ; SET_NZ8(areg);
	INDEXED; M_WRMEM(eaddr,areg);
}

/* $f8 EORA indexed -**- */
INLINE void eora_ix( void )
{
	byte t;
	IDXBYTE(t); areg ^= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $f9 ADCA indexed **** */
INLINE void adca_ix( void )
{
	word t,r;
	IDXBYTE(t); r = areg+t+(cc&0x01);
	CLR_HNZC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $fa ORA indexed -**- */
INLINE void ora_ix( void )
{
	byte t;
	IDXBYTE(t); areg |= t;
	CLR_NZ; SET_NZ8(areg);
}

/* $fb ADDA indexed **** */
INLINE void adda_ix( void )
{
	word t,r;
	IDXBYTE(t); r = areg+t;
	CLR_HNZC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $fc JMP indexed -*** */
INLINE void jmp_ix( void )
{
	INDEXED;
	pcreg = eaddr;
}

/* $fd JSR indexed ---- */
INLINE void jsr_ix( void )
{
	INDEXED; PUSHWORD(pcreg);
	pcreg = eaddr;
}

/* $fe LDX indexed -**- */
INLINE void ldx_ix( void )
{
	IDXBYTE(xreg);
	CLR_NZ; SET_NZ8(xreg);
}

/* $ff STX indexed -**- */
INLINE void stx_ix( void )
{
	CLR_NZ; SET_NZ8(xreg);
	INDEXED; M_WRMEM(eaddr,xreg);
}
