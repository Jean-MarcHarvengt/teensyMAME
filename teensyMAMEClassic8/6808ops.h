
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

static void illegal( void )
{
}

#if macintosh
#pragma mark ____0x____
#endif

/* $00 ILLEGAL */

/* $01 NOP */
INLINE void nop( void )
{
}

/* $02 ILLEGAL */

/* $03 ILLEGAL */

/* $04 LSRD inherent -0*-* */
INLINE void lsrd (void)
{
	word t;
	CLR_NZC; t = GETDREG; cc|=(t&0x0001);
	t>>=1; SET_Z16(t); SETDREG(t);
}

/* $05 ASLD inherent ?**** */
INLINE void asld (void)
{
	int r;
	word t;
	t = GETDREG; r=t<<1;
	CLR_NZVC; SET_FLAGS16(t,t,r);
	SETDREG(r);
}

/* $06 TAP inherent ##### */
INLINE void tap (void)
{
	cc=areg;
}

/* $07 TPA inherent ----- */
INLINE void tpa (void)
{
	areg=cc;
}

/* $08 INX inherent --*-- */
INLINE void inx (void)
{
	++xreg;
	CLR_Z; SET_Z16(xreg);
}

/* $09 DEX inherent --*-- */
INLINE void dex (void)
{
	--xreg;
	CLR_Z; SET_Z16(xreg);
}

/* $0a CLV */

/* $0b SEV */

/* $0c CLC */

/* $0d SEC */

/* $0e CLI */

/* $0f SEI */


#if macintosh
#pragma mark ____1x____
#endif

/* $10 SBA inherent -**** */
INLINE void sba (void)
{
	word t;
	t=areg-breg;
	CLR_NZVC; SET_FLAGS8(areg,breg,t);
	areg=t;
}

/* $11 CBA inherent -**** */
INLINE void cba (void)
{
	word t;
	t=areg-breg;
	CLR_NZVC; SET_FLAGS8(areg,breg,t);
}

/* $12 ILLEGAL */

/* $13 ILLEGAL */

/* $14 ILLEGAL */

/* $15 ILLEGAL */

/* $16 TAB inherent -**0- */
INLINE void tab (void)
{
	breg=areg;
	CLR_NZV; SET_NZ8(breg);
}

/* $17 TBA inherent -**0- */
INLINE void tba (void)
{
	areg=breg;
	CLR_NZV; SET_NZ8(areg);
}

/* $18 XGDX inherent ----- */ /* HD63701YO only */
INLINE void xgdx( void )
{
	word t = xreg;
	xreg = GETDREG;
	SETDREG(t);
}

/* $19 DAA inherent (areg) -**0* */
INLINE void daa( void )
{
	byte msn, lsn;
	word t, cf = 0;
	msn=areg & 0xf0; lsn=areg & 0x0f;
	if( lsn>0x09 || cc&0x20 ) cf |= 0x06;
	if( msn>0x80 && lsn>0x09 ) cf |= 0x60;
	if( msn>0x90 || cc&0x01 ) cf |= 0x60;
	t = cf + areg;
	CLR_NZV; /* keep carry from previous operation */
	SET_NZ8((byte)t); SET_C8(t);
	areg = t;
}

/* $1a ILLEGAL */ /* SLEEP on the HD63701YO - no info available on the opcode! */

/* $1b ABA inherent ***** */
INLINE void aba (void)
{
	word t;
	t=areg+breg;
	CLR_HNZVC; SET_FLAGS8(areg,breg,t); SET_H(areg,breg,t);
	areg=t;
}

/* $1c ILLEGAL */

/* $1d ILLEGAL */

/* $1e ILLEGAL */

/* $1f ILLEGAL */

#if macintosh
#pragma mark ____2x____
#endif

/* $20 BRA relative ----- */
INLINE void bra( void )
{
	byte t;
	IMMBYTE(t);pcreg+=SIGNED(t);change_pc(pcreg);
	/* speed up busy loops */
	if (t==0xfe) m6808_ICount = 0;
}

/* $21 BRN relative ----- */
INLINE void brn( void )
{
	byte t;
	IMMBYTE(t);
}

/* $22 BHI relative ----- */
INLINE void bhi( void )
{
	byte t;
	BRANCH(!(cc&0x05));
}

/* $23 BLS relative ----- */
INLINE void bls( void )
{
	byte t;
	BRANCH(cc&0x05);
}

/* $24 BCC relative ----- */
INLINE void bcc( void )
{
	byte t;
	BRANCH(!(cc&0x01));
}

/* $25 BCS relative ----- */
INLINE void bcs( void )
{
	byte t;
	BRANCH(cc&0x01);
}

/* $26 BNE relative ----- */
INLINE void bne( void )
{
	byte t;
	BRANCH(!(cc&0x04));
}

/* $27 BEQ relative ----- */
INLINE void beq( void )
{
	byte t;
	BRANCH(cc&0x04);
}

/* $28 BVC relative ----- */
INLINE void bvc( void )
{
	byte t;
	BRANCH(!(cc&0x02));
}

/* $29 BVS relative ----- */
INLINE void bvs( void )
{
	byte t;
	BRANCH(cc&0x02);
}

/* $2a BPL relative ----- */
INLINE void bpl( void )
{
	byte t;
	BRANCH(!(cc&0x08));
}

/* $2b BMI relative ----- */
INLINE void bmi( void )
{
	byte t;
	BRANCH(cc&0x08);
}

/* $2c BGE relative ----- */
INLINE void bge( void )
{
	byte t;
	BRANCH(!NXORV);
}

/* $2d BLT relative ----- */
INLINE void blt( void )
{
	byte t;
	BRANCH(NXORV);
}

/* $2e BGT relative ----- */
INLINE void bgt( void )
{
	byte t;
	BRANCH(!(NXORV||cc&0x04));
}

/* $2f BLE relative ----- */
INLINE void ble( void )
{
	byte t;
	BRANCH(NXORV||cc&0x04);
}


#if macintosh
#pragma mark ____3x____
#endif

/* $30 TSX inherent ----- */
INLINE void tsx (void)
{
	xreg=( sreg + 1 );
}

/* $31 INS inherent ----- */
INLINE void ins (void)
{
	++sreg;
}

/* $32 PULA inherent ----- */
INLINE void pula (void)
{
	PULLBYTE(areg);
}

/* $33 PULB inherent ----- */
INLINE void pulb (void)
{
	PULLBYTE(breg);
}

/* $34 DES inherent ----- */
INLINE void des (void)
{
	--sreg;
}

/* $35 TXS inherent ----- */
INLINE void txs (void)
{
	sreg=( xreg - 1 );
}

/* $36 PSHA inherent ----- */
INLINE void psha (void)
{
	PUSHBYTE(areg);
}

/* $37 PSHB inherent ----- */
INLINE void pshb (void)
{
	PUSHBYTE(breg);
}

/* $38 PULX inherent ----- */
INLINE void pulx (void)
{
	PULLWORD(xreg);
}

/* $39 RTS inherent ----- */
INLINE void rts( void )
{
	PULLWORD(pcreg); change_pc(pcreg);
}

/* $3a ABX inherent ----- */
INLINE void abx( void )
{
	xreg += breg;
}

/* $3b RTI inherent ##### */
INLINE void rti( void )
{
	PULLBYTE(cc);
	PULLBYTE(areg);
	PULLBYTE(breg);
	PULLWORD(xreg);
	PULLWORD(pcreg);change_pc(pcreg);
}

/* $3c PSHX inherent ----- */
INLINE void pshx (void)
{
	PUSHWORD(xreg);
}

/* $3d MUL inherent --*-@ */
INLINE void mul( void )
{
	word t;
	t=areg*breg;
	CLR_ZC; SET_Z16(t); if(t&0x80) SEC;
	SETDREG(t);
}

/* $3e WAI inherent ----- */
INLINE void wai( void )
{
	/* WAI should stack the entire machine state on the hardware stack,
		then wait for an interrupt. We just wait for an IRQ. */
	m6808_ICount = 0;
	pending_interrupts |= M6808_WAI;
}

/* $3f SWI absolute indirect ----- */
INLINE void swi( void )
{
	PUSHWORD(pcreg);
	PUSHWORD(xreg);
	PUSHBYTE(breg);
	PUSHBYTE(areg);
	PUSHBYTE(cc);
	SEI;
	pcreg = M_RDMEM_WORD(0xfffa);change_pc(pcreg);
}

#if macintosh
#pragma mark ____4x____
#endif

/* $40 NEGA inherent ?**** */
INLINE void nega( void )
{
	word r;
	r=-areg;
	CLR_NZVC; SET_FLAGS8(0,areg,r);
	areg=r;
}

/* $41 ILLEGAL */

/* $42 ILLEGAL */

/* $43 COMA inherent -**01 */
INLINE void coma( void )
{
	areg = ~areg;
	CLR_NZV; SET_NZ8(areg); SEC;
}

/* $44 LSRA inherent -0*-* */
INLINE void lsra( void )
{
	CLR_NZC; cc|=(areg&0x01);
	areg>>=1; SET_Z8(areg);
}

/* $45 ILLEGAL */

/* $46 RORA inherent -**-* */
INLINE void rora( void )
{
	byte r;
	r=(cc&0x01)<<7;
	CLR_NZC; cc|=(areg&0x01);
	r |= areg>>1; SET_NZ8(r);
	areg=r;
}

/* $47 ASRA inherent ?**-* */
INLINE void asra( void )
{
	CLR_NZC; cc|=(areg&0x01);
	areg>>=1; areg|=((areg&0x40)<<1);
	SET_NZ8(areg);
}

/* $48 ASLA inherent ?**** */
INLINE void asla( void )
{
	word r;
	r=areg<<1;
	CLR_NZVC; SET_FLAGS8(areg,areg,r);
	areg=r;
}

/* $49 ROLA inherent -**** */
INLINE void rola( void )
{
	word t,r;
	t = areg; r = cc&0x01; r |= t<<1;
	CLR_NZVC; SET_FLAGS8(t,t,r);
	areg=r;
}

/* $4a DECA inherent -***- */
INLINE void deca( void )
{
	--areg;
	CLR_NZV; SET_FLAGS8D(areg);
}

/* $4b ILLEGAL */

/* $4c INCA inherent -***- */
INLINE void inca( void )
{
	++areg;
	CLR_NZV; SET_FLAGS8I(areg);
}

/* $4d TSTA inherent -**0- */
INLINE void tsta( void )
{
	CLR_NZV; SET_NZ8(areg);
}

/* $4e ILLEGAL */

/* $4f CLRA inherent -0100 */
INLINE void clra( void )
{
	areg=0;
	CLR_NZVC; SEZ;
}


#if macintosh
#pragma mark ____5x____
#endif


/* $50 NEGB inherent ?**** */
INLINE void negb( void )
{
	word r;
	r=-breg;
	CLR_NZVC; SET_FLAGS8(0,breg,r);
	breg=r;
}

/* $51 ILLEGAL */

/* $52 ILLEGAL */

/* $53 COMB inherent -**01 */
INLINE void comb( void )
{
	breg = ~breg;
	CLR_NZV; SET_NZ8(breg); SEC;
}

/* $54 LSRB inherent -0*-* */
INLINE void lsrb( void )
{
	CLR_NZC; cc|=(breg&0x01);
	breg>>=1; SET_Z8(breg);
}

/* $55 ILLEGAL */

/* $56 RORB inherent -**-* */
INLINE void rorb( void )
{
	byte r;
	r=(cc&0x01)<<7;
	CLR_NZC; cc|=(breg&0x01);
	r |= breg>>1; SET_NZ8(r);
	breg=r;
}

/* $57 ASRB inherent ?**-* */
INLINE void asrb( void )
{
	CLR_NZC; cc|=(breg&0x01);
	breg>>=1; breg|=((breg&0x40)<<1);
	SET_NZ8(breg);
}

/* $58 ASLB inherent ?**** */
INLINE void aslb( void )
{
	word r;
	r=breg<<1;
	CLR_NZVC; SET_FLAGS8(breg,breg,r);
	breg=r;
}

/* $59 ROLB inherent -**** */
INLINE void rolb( void )
{
	word t,r;
	t = breg; r = cc&0x01; r |= t<<1;
	CLR_NZVC; SET_FLAGS8(t,t,r);
	breg=r;
}

/* $5a DECB inherent -***- */
INLINE void decb( void )
{
	--breg;
	CLR_NZV; SET_FLAGS8D(breg);
}

/* $5b ILLEGAL */

/* $5c INCB inherent -***- */
INLINE void incb( void )
{
	++breg;
	CLR_NZV; SET_FLAGS8I(breg);
}

/* $5d TSTB inherent -**0- */
INLINE void tstb( void )
{
	CLR_NZV; SET_NZ8(breg);
}

/* $5e ILLEGAL */

/* $5f CLRB inherent -0100 */
INLINE void clrb( void )
{
	breg=0;
	CLR_NZVC; SEZ;
}

#if macintosh
#pragma mark ____6x____
#endif

/* $60 NEG indexed ?**** */
INLINE void neg_ix( void )
{
	word r,t;
	IDXBYTE(t); r=-t;
	CLR_NZVC; SET_FLAGS8(0,t,r);
	M_WRMEM(eaddr,r);
}

/* $61 AIM --**0- */ /* HD63701YO only */
INLINE void aim_ix( void )
{
	byte t, r;
	IMMBYTE(t);
	IDXBYTE(r);
	r &= t;
	CLR_NZV; SET_NZ8(r);
	M_WRMEM(eaddr,r);
}

/* $62 OIM --**0- */ /* HD63701YO only */
INLINE void oim_ix( void )
{
	byte t, r;
	IMMBYTE(t);
	IDXBYTE(r);
	r |= t;
	CLR_NZV; SET_NZ8(r);
	M_WRMEM(eaddr,r);
}

/* $63 COM indexed -**01 */
INLINE void com_ix( void )
{
	byte t;
	IDXBYTE(t); t = ~t;
	CLR_NZV; SET_NZ8(t); SEC;
	M_WRMEM(eaddr,t);
}

/* $64 LSR indexed -0*-* */
INLINE void lsr_ix( void )
{
	byte t;
	IDXBYTE(t); CLR_NZC; cc|=(t&0x01);
	t>>=1; SET_Z8(t);
	M_WRMEM(eaddr,t);
}

/* $65 EIM --**0- */ /* HD63701YO only */
INLINE void eim_ix( void )
{
	byte t, r;
	IMMBYTE(t);
	IDXBYTE(r);
	r ^= t;
	CLR_NZV; SET_NZ8(r);
	M_WRMEM(eaddr,r);
}

/* $66 ROR indexed -**-* */
INLINE void ror_ix( void )
{
	byte t,r;
	IDXBYTE(t); r=(cc&0x01)<<7;
	CLR_NZC; cc|=(t&0x01);
	r |= t>>1; SET_NZ8(r);
	M_WRMEM(eaddr,r);
}

/* $67 ASR indexed ?**-* */
INLINE void asr_ix( void )
{
	byte t;
	IDXBYTE(t); CLR_NZC; cc|=(t&0x01);
	t>>=1; t|=((t&0x40)<<1);
	SET_NZ8(t);
	M_WRMEM(eaddr,t);
}

/* $68 ASL indexed ?**** */
INLINE void asl_ix( void )
{
	word t,r;
	IDXBYTE(t); r=t<<1;
	CLR_NZVC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $69 ROL indexed -**** */
INLINE void rol_ix( void )
{
	word t,r;
	IDXBYTE(t); r = cc&0x01; r |= t<<1;
	CLR_NZVC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $6a DEC indexed -***- */
INLINE void dec_ix( void )
{
	byte t;
	IDXBYTE(t); --t;
	CLR_NZV; SET_FLAGS8D(t);
	M_WRMEM(eaddr,t);
}

/* $6b TIM --**0- */ /* HD63701YO only */
INLINE void tim_ix( void )
{
	byte t, r;
	IMMBYTE(t);
	IDXBYTE(r);
	r &= t;
	CLR_NZV; SET_NZ8(r);
}

/* $6c INC indexed -***- */
INLINE void inc_ix( void )
{
	byte t;
	IDXBYTE(t); ++t;
	CLR_NZV; SET_FLAGS8I(t);
	M_WRMEM(eaddr,t);
}

/* $6d TST indexed -**0- */
INLINE void tst_ix( void )
{
	byte t;
	IDXBYTE(t); CLR_NZV; SET_NZ8(t);
}

/* $6e JMP indexed ----- */
INLINE void jmp_ix( void )
{
	INDEXED; pcreg=eaddr; change_pc(pcreg);
}

/* $6f CLR indexed -0100 */
INLINE void clr_ix( void )
{
	INDEXED; M_WRMEM(eaddr,0);
	CLR_NZVC; SEZ;
}

#if macintosh
#pragma mark ____7x____
#endif

/* $70 NEG extended ?**** */
INLINE void neg_ex( void )
{
	word r,t;
	EXTBYTE(t); r=-t;
	CLR_NZVC; SET_FLAGS8(0,t,r);
	M_WRMEM(eaddr,r);
}

/* $71 AIM --**0- */ /* HD63701YO only */
INLINE void aim_di( void )
{
	byte t, r;
	IMMBYTE(t);
	DIRBYTE(r);
	r &= t;
	CLR_NZV; SET_NZ8(r);
	M_WRMEM(eaddr,r);
}

/* $72 OIM --**0- */ /* HD63701YO only */
INLINE void oim_di( void )
{
	byte t, r;
	IMMBYTE(t);
	DIRBYTE(r);
	r |= t;
	CLR_NZV; SET_NZ8(r);
	M_WRMEM(eaddr,r);
}

/* $73 COM extended -**01 */
INLINE void com_ex( void )
{
	byte t;
	EXTBYTE(t); t = ~t;
	CLR_NZV; SET_NZ8(t); SEC;
	M_WRMEM(eaddr,t);
}

/* $74 LSR extended -0*-* */
INLINE void lsr_ex( void )
{
	byte t;
	EXTBYTE(t); CLR_NZC; cc|=(t&0x01);
	t>>=1; SET_Z8(t);
	M_WRMEM(eaddr,t);
}

/* $75 EIM --**0- */ /* HD63701YO only */
INLINE void eim_di( void )
{
	byte t, r;
	IMMBYTE(t);
	DIRBYTE(r);
	r ^= t;
	CLR_NZV; SET_NZ8(r);
	M_WRMEM(eaddr,r);
}

/* $76 ROR extended -**-* */
INLINE void ror_ex( void )
{
	byte t,r;
	EXTBYTE(t); r=(cc&0x01)<<7;
	CLR_NZC; cc|=(t&0x01);
	r |= t>>1; SET_NZ8(r);
	M_WRMEM(eaddr,r);
}

/* $77 ASR extended ?**-* */
INLINE void asr_ex( void )
{
	byte t;
	EXTBYTE(t); CLR_NZC; cc|=(t&0x01);
	t>>=1; t|=((t&0x40)<<1);
	SET_NZ8(t);
	M_WRMEM(eaddr,t);
}

/* $78 ASL extended ?**** */
INLINE void asl_ex( void )
{
	word t,r;
	EXTBYTE(t); r=t<<1;
	CLR_NZVC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $79 ROL extended -**** */
INLINE void rol_ex( void )
{
	word t,r;
	EXTBYTE(t); r = cc&0x01; r |= t<<1;
	CLR_NZVC; SET_FLAGS8(t,t,r);
	M_WRMEM(eaddr,r);
}

/* $7a DEC extended -***- */
INLINE void dec_ex( void )
{
	byte t;
	EXTBYTE(t); --t;
	CLR_NZV; SET_FLAGS8D(t);
	M_WRMEM(eaddr,t);
}

/* $7b TIM --**0- */ /* HD63701YO only */
INLINE void tim_di( void )
{
	byte t, r;
	IMMBYTE(t);
	DIRBYTE(r);
	r &= t;
	CLR_NZV; SET_NZ8(r);
}

/* $7c INC extended -***- */
INLINE void inc_ex( void )
{
	byte t;
	EXTBYTE(t); ++t;
	CLR_NZV; SET_FLAGS8I(t);
	M_WRMEM(eaddr,t);
}

/* $7d TST extended -**0- */
INLINE void tst_ex( void )
{
	byte t;
	EXTBYTE(t); CLR_NZV; SET_NZ8(t);
}

/* $7e JMP extended ----- */
INLINE void jmp_ex( void )
{
	EXTENDED; pcreg=eaddr;change_pc(pcreg);	/* TS 971002 */
}

/* $7f CLR extended -0100 */
INLINE void clr_ex( void )
{
	EXTENDED; M_WRMEM(eaddr,0);
	CLR_NZVC; SEZ;
}


#if macintosh
#pragma mark ____8x____
#endif

/* $80 SUBA immediate ?**** */
INLINE void suba_im( void )
{
	word	t,r;
	IMMBYTE(t); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $81 CMPA immediate ?**** */
INLINE void cmpa_im( void )
{
	word	t,r;
	IMMBYTE(t); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
}

/* $82 SBCA immediate ?**** */
INLINE void sbca_im( void )
{
	word	t,r;
	IMMBYTE(t); r = areg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $83 SUBD immediate -**** */
INLINE void subd_im( void )
{
	dword r,d,b;
	IMMWORD(b); d = GETDREG; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $84 ANDA immediate -**0- */
INLINE void anda_im( void )
{
	byte t;
	IMMBYTE(t); areg &= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $85 BITA immediate -**0- */
INLINE void bita_im( void )
{
	byte t,r;
	IMMBYTE(t); r = areg&t;
	CLR_NZV; SET_NZ8(r);
}

/* $86 LDA immediate -**0- */
INLINE void lda_im( void )
{
	IMMBYTE(areg);
	CLR_NZV; SET_NZ8(areg);
}

/* is this a legal instruction? */
/* $87 STA immediate -**0- */
INLINE void sta_im( void )
{
	CLR_NZV; SET_NZ8(areg);
	IMM8; M_WRMEM(eaddr,areg);
}

/* $88 EORA immediate -**0- */
INLINE void eora_im( void )
{
	byte t;
	IMMBYTE(t); areg ^= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $89 ADCA immediate ***** */
INLINE void adca_im( void )
{
	word t,r;
	IMMBYTE(t); r = areg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $8a ORA immediate -**0- */
INLINE void ora_im( void )
{
	byte t;
	IMMBYTE(t); areg |= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $8b ADDA immediate ***** */
INLINE void adda_im( void )
{
	word t,r;
	IMMBYTE(t); r = areg+t;
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $8c CMPX immediate -**** */
INLINE void cmpx_im( void )
{
	dword r,d,b;
	IMMWORD(b); d = xreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $8d BSR ----- */
INLINE void bsr( void )
{
	byte t;
	IMMBYTE(t); PUSHWORD(pcreg); pcreg += SIGNED(t);change_pc(pcreg);	/* TS 971002 */
}

/* $8e LDS immediate -**0- */
INLINE void lds_im( void )
{
	IMMWORD(sreg);
	CLR_NZV; SET_NZ16(sreg);
}

/* $8f STS immediate -**0- */
INLINE void sts_im( void )
{
	CLR_NZV; SET_NZ16(sreg);
	IMM16; M_WRMEM_WORD(eaddr,sreg);
}

#if macintosh
#pragma mark ____9x____
#endif

/* $90 SUBA direct ?**** */
INLINE void suba_di( void )
{
	word	t,r;
	DIRBYTE(t); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $91 CMPA direct ?**** */
INLINE void cmpa_di( void )
{
	word	t,r;
	DIRBYTE(t); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
}

/* $92 SBCA direct ?**** */
INLINE void sbca_di( void )
{
	word	t,r;
	DIRBYTE(t); r = areg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $93 SUBD direct -**** */
INLINE void subd_di( void )
{
	dword r,d,b;
	DIRWORD(b); d = GETDREG; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $94 ANDA direct -**0- */
INLINE void anda_di( void )
{
	byte t;
	DIRBYTE(t); areg &= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $95 BITA direct -**0- */
INLINE void bita_di( void )
{
	byte t,r;
	DIRBYTE(t); r = areg&t;
	CLR_NZV; SET_NZ8(r);
}

/* $96 LDA direct -**0- */
INLINE void lda_di( void )
{
	DIRBYTE(areg);
	CLR_NZV; SET_NZ8(areg);
}

/* $97 STA direct -**0- */
INLINE void sta_di( void )
{
	CLR_NZV; SET_NZ8(areg);
	DIRECT; M_WRMEM(eaddr,areg);
}

/* $98 EORA direct -**0- */
INLINE void eora_di( void )
{
	byte t;
	DIRBYTE(t); areg ^= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $99 ADCA direct ***** */
INLINE void adca_di( void )
{
	word t,r;
	DIRBYTE(t); r = areg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $9a ORA direct -**0- */
INLINE void ora_di( void )
{
	byte t;
	DIRBYTE(t); areg |= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $9b ADDA direct ***** */
INLINE void adda_di( void )
{
	word t,r;
	DIRBYTE(t); r = areg+t;
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $9c CMPX direct -**** */
INLINE void cmpx_di( void )
{
	dword r,d,b;
	DIRWORD(b); d = xreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $9d JSR direct ----- */
INLINE void jsr_di( void )
{
	DIRECT; PUSHWORD(pcreg);
	pcreg = eaddr; change_pc(pcreg);
}

/* $9e LDS direct -**0- */
INLINE void lds_di( void )
{
	DIRWORD(sreg);
	CLR_NZV; SET_NZ16(sreg);
}

/* $9f STS direct -**0- */
INLINE void sts_di( void )
{
	CLR_NZV; SET_NZ16(sreg);
	DIRECT; M_WRMEM_WORD(eaddr,sreg);
}

#if macintosh
#pragma mark ____Ax____
#endif


/* $a0 SUBA indexed ?**** */
INLINE void suba_ix( void )
{
	word	t,r;
	IDXBYTE(t); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $a1 CMPA indexed ?**** */
INLINE void cmpa_ix( void )
{
	word	t,r;
	IDXBYTE(t); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
}

/* $a2 SBCA indexed ?**** */
INLINE void sbca_ix( void )
{
	word	t,r;
	IDXBYTE(t); r = areg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $a3 SUBD indexed -**** */
INLINE void subd_ix( void )
{
	dword r,d,b;
	IDXWORD(b); d = GETDREG; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $a4 ANDA indexed -**0- */
INLINE void anda_ix( void )
{
	byte t;
	IDXBYTE(t); areg &= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $a5 BITA indexed -**0- */
INLINE void bita_ix( void )
{
	byte t,r;
	IDXBYTE(t); r = areg&t;
	CLR_NZV; SET_NZ8(r);
}

/* $a6 LDA indexed -**0- */
INLINE void lda_ix( void )
{
	IDXBYTE(areg);
	CLR_NZV; SET_NZ8(areg);
}

/* $a7 STA indexed -**0- */
INLINE void sta_ix( void )
{
	CLR_NZV; SET_NZ8(areg);
	INDEXED; M_WRMEM(eaddr,areg);
}

/* $a8 EORA indexed -**0- */
INLINE void eora_ix( void )
{
	byte t;
	IDXBYTE(t); areg ^= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $a9 ADCA indexed ***** */
INLINE void adca_ix( void )
{
	word t,r;
	IDXBYTE(t); r = areg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $aa ORA indexed -**0- */
INLINE void ora_ix( void )
{
	byte t;
	IDXBYTE(t); areg |= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $ab ADDA indexed ***** */
INLINE void adda_ix( void )
{
	word t,r;
	IDXBYTE(t); r = areg+t;
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $ac CMPX indexed -**** */
INLINE void cmpx_ix( void )
{
	dword r,d,b;
	IDXWORD(b); d = xreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $ad JSR indexed ----- */
INLINE void jsr_ix( void )
{
	INDEXED; PUSHWORD(pcreg);
	pcreg = eaddr; change_pc(pcreg);
}

/* $ae LDS indexed -**0- */
INLINE void lds_ix( void )
{
	IDXWORD(sreg);
	CLR_NZV; SET_NZ16(sreg);
}

/* $af STS indexed -**0- */
INLINE void sts_ix( void )
{
	CLR_NZV; SET_NZ16(sreg);
	INDEXED; M_WRMEM_WORD(eaddr,sreg);
}

#if macintosh
#pragma mark ____Bx____
#endif

/* $b0 SUBA extended ?**** */
INLINE void suba_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $b1 CMPA extended ?**** */
INLINE void cmpa_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = areg-t;
	CLR_NZVC; SET_FLAGS8(areg,t,r);
}

/* $b2 SBCA extended ?**** */
INLINE void sbca_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = areg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(areg,t,r);
	areg = r;
}

/* $b3 SUBD extended -**** */
INLINE void subd_ex( void )
{
	dword r,d,b;
	EXTWORD(b); d = GETDREG; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $b4 ANDA extended -**0- */
INLINE void anda_ex( void )
{
	byte t;
	EXTBYTE(t); areg &= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $b5 BITA extended -**0- */
INLINE void bita_ex( void )
{
	byte t,r;
	EXTBYTE(t); r = areg&t;
	CLR_NZV; SET_NZ8(r);
}

/* $b6 LDA extended -**0- */
INLINE void lda_ex( void )
{
	EXTBYTE(areg);
	CLR_NZV; SET_NZ8(areg);
}

/* $b7 STA extended -**0- */
INLINE void sta_ex( void )
{
	CLR_NZV; SET_NZ8(areg);
	EXTENDED; M_WRMEM(eaddr,areg);
}

/* $b8 EORA extended -**0- */
INLINE void eora_ex( void )
{
	byte t;
	EXTBYTE(t); areg ^= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $b9 ADCA extended ***** */
INLINE void adca_ex( void )
{
	word t,r;
	EXTBYTE(t); r = areg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $ba ORA extended -**0- */
INLINE void ora_ex( void )
{
	byte t;
	EXTBYTE(t); areg |= t;
	CLR_NZV; SET_NZ8(areg);
}

/* $bb ADDA extended ***** */
INLINE void adda_ex( void )
{
	word t,r;
	EXTBYTE(t); r = areg+t;
	CLR_HNZVC; SET_FLAGS8(areg,t,r); SET_H(areg,t,r);
	areg = r;
}

/* $bc CMPX extended -**** */
INLINE void cmpx_ex( void )
{
	dword r,d,b;
	EXTWORD(b); d = xreg; r = d-b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
}

/* $bd JSR extended ----- */
INLINE void jsr_ex( void )
{
	EXTENDED; PUSHWORD(pcreg);
	pcreg = eaddr; change_pc(pcreg);
}

/* $be LDS extended -**0- */
INLINE void lds_ex( void )
{
	EXTWORD(sreg);
	CLR_NZV; SET_NZ16(sreg);
}

/* $bf STS extended -**0- */
INLINE void sts_ex( void )
{
	CLR_NZV; SET_NZ16(sreg);
	EXTENDED; M_WRMEM_WORD(eaddr,sreg);
}


#if macintosh
#pragma mark ____Cx____
#endif

/* $c0 SUBB immediate ?**** */
INLINE void subb_im( void )
{
	word	t,r;
	IMMBYTE(t); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $c1 CMPB immediate ?**** */
INLINE void cmpb_im( void )
{
	word	t,r;
	IMMBYTE(t); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
}

/* $c2 SBCB immediate ?**** */
INLINE void sbcb_im( void )
{
	word	t,r;
	IMMBYTE(t); r = breg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $c3 ADDD immediate -**** */
INLINE void addd_im( void )
{
	dword r,d,b;
	IMMWORD(b); d = GETDREG; r = d+b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $c4 ANDB immediate -**0- */
INLINE void andb_im( void )
{
	byte t;
	IMMBYTE(t); breg &= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $c5 BITB immediate -**0- */
INLINE void bitb_im( void )
{
	byte t,r;
	IMMBYTE(t); r = breg&t;
	CLR_NZV; SET_NZ8(r);
}

/* $c6 LDB immediate -**0- */
INLINE void ldb_im( void )
{
	IMMBYTE(breg);
	CLR_NZV; SET_NZ8(breg);
}

/* is this a legal instruction? */
/* $c7 STB immediate -**0- */
INLINE void stb_im( void )
{
	CLR_NZV; SET_NZ8(breg);
	IMM8; M_WRMEM(eaddr,breg);
}

/* $c8 EORB immediate -**0- */
INLINE void eorb_im( void )
{
	byte t;
	IMMBYTE(t); breg ^= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $c9 ADCB immediate ***** */
INLINE void adcb_im( void )
{
	word t,r;
	IMMBYTE(t); r = breg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $ca ORB immediate -**0- */
INLINE void orb_im( void )
{
	byte t;
	IMMBYTE(t); breg |= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $cb ADDB immediate ***** */
INLINE void addb_im( void )
{
	word t,r;
	IMMBYTE(t); r = breg+t;
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $cc LDD immediate -**0- */
INLINE void ldd_im( void )
{
	word t;
	IMMWORD(t); SETDREG(t);
	CLR_NZV; SET_NZ16(t);
}

/* is this a legal instruction? */
/* $cd STD immediate -**0- */
INLINE void std_im( void )
{
	word t;
	IMM16; t=GETDREG;
	CLR_NZV; SET_NZ16(t);
	M_WRMEM_WORD(eaddr,t);
}

/* $ce LDX immediate -**0- */
INLINE void ldx_im( void )
{
	IMMWORD(xreg);
	CLR_NZV; SET_NZ16(xreg);
}

/* $cf STX immediate -**0- */
INLINE void stx_im( void )
{
	CLR_NZV; SET_NZ16(xreg);
	IMM16; M_WRMEM_WORD(eaddr,xreg);
}


#if macintosh
#pragma mark ____Dx____
#endif

/* $d0 SUBB direct ?**** */
INLINE void subb_di( void )
{
	word	t,r;
	DIRBYTE(t); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $d1 CMPB direct ?**** */
INLINE void cmpb_di( void )
{
	word	t,r;
	DIRBYTE(t); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
}

/* $d2 SBCB direct ?**** */
INLINE void sbcb_di( void )
{
	word	t,r;
	DIRBYTE(t); r = breg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $d3 ADDD direct -**** */
INLINE void addd_di( void )
{
	dword r,d,b;
	DIRWORD(b); d = GETDREG; r = d+b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $d4 ANDB direct -**0- */
INLINE void andb_di( void )
{
	byte t;
	DIRBYTE(t); breg &= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $d5 BITB direct -**0- */
INLINE void bitb_di( void )
{
	byte t,r;
	DIRBYTE(t); r = breg&t;
	CLR_NZV; SET_NZ8(r);
}

/* $d6 LDB direct -**0- */
INLINE void ldb_di( void )
{
	DIRBYTE(breg);
	CLR_NZV; SET_NZ8(breg);
}

/* $d7 STB direct -**0- */
INLINE void stb_di( void )
{
	CLR_NZV; SET_NZ8(breg);
	DIRECT; M_WRMEM(eaddr,breg);
}

/* $d8 EORB direct -**0- */
INLINE void eorb_di( void )
{
	byte t;
	DIRBYTE(t); breg ^= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $d9 ADCB direct ***** */
INLINE void adcb_di( void )
{
	word t,r;
	DIRBYTE(t); r = breg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $da ORB direct -**0- */
INLINE void orb_di( void )
{
	byte t;
	DIRBYTE(t); breg |= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $db ADDB direct ***** */
INLINE void addb_di( void )
{
	word t,r;
	DIRBYTE(t); r = breg+t;
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $dc LDD direct -**0- */
INLINE void ldd_di( void )
{
	word t;
	DIRWORD(t); SETDREG(t);
	CLR_NZV; SET_NZ16(t);
}

/* $dd STD direct -**0- */
INLINE void std_di( void )
{
	word t;
	DIRECT; t=GETDREG;
	CLR_NZV; SET_NZ16(t);
	M_WRMEM_WORD(eaddr,t);
}

/* $de LDX direct -**0- */
INLINE void ldx_di( void )
{
	DIRWORD(xreg);
	CLR_NZV; SET_NZ16(xreg);
}

/* $dF STX direct -**0- */
INLINE void stx_di( void )
{
	CLR_NZV; SET_NZ16(xreg);
	DIRECT; M_WRMEM_WORD(eaddr,xreg);
}

#if macintosh
#pragma mark ____Ex____
#endif


/* $e0 SUBB indexed ?**** */
INLINE void subb_ix( void )
{
	word	t,r;
	IDXBYTE(t); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $e1 CMPB indexed ?**** */
INLINE void cmpb_ix( void )
{
	word	t,r;
	IDXBYTE(t); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
}

/* $e2 SBCB indexed ?**** */
INLINE void sbcb_ix( void )
{
	word	t,r;
	IDXBYTE(t); r = breg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $e3 ADDD indexed -**** */
INLINE void addd_ix( void )
{
	dword r,d,b;
	IDXWORD(b); d = GETDREG; r = d+b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $e4 ANDB indexed -**0- */
INLINE void andb_ix( void )
{
	byte t;
	IDXBYTE(t); breg &= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $e5 BITB indexed -**0- */
INLINE void bitb_ix( void )
{
	byte t,r;
	IDXBYTE(t); r = breg&t;
	CLR_NZV; SET_NZ8(r);
}

/* $e6 LDB indexed -**0- */
INLINE void ldb_ix( void )
{
	IDXBYTE(breg);
	CLR_NZV; SET_NZ8(breg);
}

/* $e7 STB indexed -**0- */
INLINE void stb_ix( void )
{
	CLR_NZV; SET_NZ8(breg);
	INDEXED; M_WRMEM(eaddr,breg);
}

/* $e8 EORB indexed -**0- */
INLINE void eorb_ix( void )
{
	byte t;
	IDXBYTE(t); breg ^= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $e9 ADCB indexed ***** */
INLINE void adcb_ix( void )
{
	word t,r;
	IDXBYTE(t); r = breg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $ea ORB indexed -**0- */
INLINE void orb_ix( void )
{
	byte t;
	IDXBYTE(t); breg |= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $eb ADDB indexed ***** */
INLINE void addb_ix( void )
{
	word t,r;
	IDXBYTE(t); r = breg+t;
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $ec LDD indexed -**0- */
INLINE void ldd_ix( void )
{
	word t;
	IDXWORD(t); SETDREG(t);
	CLR_NZV; SET_NZ16(t);
}

/* $ed STD indexed -**0- */
INLINE void std_ix( void )
{
	word t;
	INDEXED; t=GETDREG;
	CLR_NZV; SET_NZ16(t);
	M_WRMEM_WORD(eaddr,t);
}

/* $ee LDX indexed -**0- */
INLINE void ldx_ix( void )
{
	IDXWORD(xreg);
	CLR_NZV; SET_NZ16(xreg);
}

/* $ef STX indexed -**0- */
INLINE void stx_ix( void )
{
	CLR_NZV; SET_NZ16(xreg);
	INDEXED; M_WRMEM_WORD(eaddr,xreg);
}

#if macintosh
#pragma mark ____Fx____
#endif

/* $f0 SUBB extended ?**** */
INLINE void subb_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $f1 CMPB extended ?**** */
INLINE void cmpb_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = breg-t;
	CLR_NZVC; SET_FLAGS8(breg,t,r);
}

/* $f2 SBCB extended ?**** */
INLINE void sbcb_ex( void )
{
	word	t,r;
	EXTBYTE(t); r = breg-t-(cc&0x01);
	CLR_NZVC; SET_FLAGS8(breg,t,r);
	breg = r;
}

/* $f3 ADDD extended -**** */
INLINE void addd_ex( void )
{
	dword r,d,b;
	EXTWORD(b); d = GETDREG; r = d+b;
	CLR_NZVC; SET_FLAGS16(d,b,r);
	SETDREG(r);
}

/* $f4 ANDB extended -**0- */
INLINE void andb_ex( void )
{
	byte t;
	EXTBYTE(t); breg &= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $f5 BITB extended -**0- */
INLINE void bitb_ex( void )
{
	byte t,r;
	EXTBYTE(t); r = breg&t;
	CLR_NZV; SET_NZ8(r);
}

/* $f6 LDB extended -**0- */
INLINE void ldb_ex( void )
{
	EXTBYTE(breg);
	CLR_NZV; SET_NZ8(breg);
}

/* $f7 STB extended -**0- */
INLINE void stb_ex( void )
{
	CLR_NZV; SET_NZ8(breg);
	EXTENDED; M_WRMEM(eaddr,breg);
}

/* $f8 EORB extended -**0- */
INLINE void eorb_ex( void )
{
	byte t;
	EXTBYTE(t); breg ^= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $f9 ADCB extended ***** */
INLINE void adcb_ex( void )
{
	word t,r;
	EXTBYTE(t); r = breg+t+(cc&0x01);
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $fa ORB extended -**0- */
INLINE void orb_ex( void )
{
	byte t;
	EXTBYTE(t); breg |= t;
	CLR_NZV; SET_NZ8(breg);
}

/* $fb ADDB extended ***** */
INLINE void addb_ex( void )
{
	word t,r;
	EXTBYTE(t); r = breg+t;
	CLR_HNZVC; SET_FLAGS8(breg,t,r); SET_H(breg,t,r);
	breg = r;
}

/* $fc LDD extended -**0- */
INLINE void ldd_ex( void )
{
	word t;
	EXTWORD(t); SETDREG(t);
	CLR_NZV; SET_NZ16(t);
}

/* $fd STD extended -**0- */
INLINE void std_ex( void )
{
	word t;
	EXTENDED; t=GETDREG;
	CLR_NZV; SET_NZ16(t);
	M_WRMEM_WORD(eaddr,t);
}

/* $fe LDX extended -**0- */
INLINE void ldx_ex( void )
{
	EXTWORD(xreg);
	CLR_NZV; SET_NZ16(xreg);
}

/* $ff STX extended -**0- */
INLINE void stx_ex( void )
{
	CLR_NZV; SET_NZ16(xreg);
	EXTENDED; M_WRMEM_WORD(eaddr,xreg);
}
