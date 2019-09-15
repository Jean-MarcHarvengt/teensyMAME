/****************************************************************************/
/*            real mode i286 emulator by Fabrice Frances                    */
/*           (initial work based on David Hedley's pcemu)                   */
/*                                                                          */
/****************************************************************************/

typedef enum { ES, CS, SS, DS } SREGS;
typedef enum { AX, CX, DX, BX, SP, BP, SI, DI } WREGS;

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

#ifdef LSB_FIRST
typedef enum { AL,AH,CL,CH,DL,DH,BL,BH,SPL,SPH,BPL,BPH,SIL,SIH,DIL,DIH } BREGS;
#else
typedef enum { AH,AL,CH,CL,DH,DL,BH,BL,SPH,SPL,BPH,BPL,SIH,SIL,DIH,DIL } BREGS;
#endif

/* parameter x = result, y = source 1, z = source 2 */

#define SetTF(x)        (TF = (x))
#define SetIF(x)        (IF = (x))
#define SetDF(x)        (DF = (x))

#define SetOFW_Add(x,y,z)   (OverVal = ((x) ^ (y)) & ((x) ^ (z)) & 0x8000)
#define SetOFB_Add(x,y,z)   (OverVal = ((x) ^ (y)) & ((x) ^ (z)) & 0x80)
#define SetOFW_Sub(x,y,z)   (OverVal = ((z) ^ (y)) & ((z) ^ (x)) & 0x8000)
#define SetOFB_Sub(x,y,z)   (OverVal = ((z) ^ (y)) & ((z) ^ (x)) & 0x80)

#define SetCFB(x)       (CarryVal = (x) & 0x100)
#define SetCFW(x)       (CarryVal = (x) & 0x10000)
#define SetAF(x,y,z)    (AuxVal = ((x) ^ ((y) ^ (z))) & 0x10)
#define SetSF(x)        (SignVal = (x))
#define SetZF(x)        (ZeroVal = (x))
#define SetPF(x)        (ParityVal = (x))

#define SetSZPF_Byte(x) (SignVal=ZeroVal=ParityVal=(INT8)(x))
#define SetSZPF_Word(x) (SignVal=ZeroVal=ParityVal=(INT16)(x))

#define ADDB(dst,src) { unsigned res=dst+src; SetCFB(res); SetOFB_Add(res,src,dst); SetAF(res,src,dst); SetSZPF_Byte(res); dst=(BYTE)res; }
#define ADDW(dst,src) { unsigned res=dst+src; SetCFW(res); SetOFW_Add(res,src,dst); SetAF(res,src,dst); SetSZPF_Word(res); dst=(WORD)res; }

#define SUBB(dst,src) { unsigned res=dst-src; SetCFB(res); SetOFB_Sub(res,src,dst); SetAF(res,src,dst); SetSZPF_Byte(res); dst=(BYTE)res; }
#define SUBW(dst,src) { unsigned res=dst-src; SetCFW(res); SetOFW_Sub(res,src,dst); SetAF(res,src,dst); SetSZPF_Word(res); dst=(WORD)res; }

#define ORB(dst,src) dst|=src; CarryVal=OverVal=AuxVal=0; SetSZPF_Byte(dst)
#define ORW(dst,src) dst|=src; CarryVal=OverVal=AuxVal=0; SetSZPF_Word(dst)

#define ANDB(dst,src) dst&=src; CarryVal=OverVal=AuxVal=0; SetSZPF_Byte(dst)
#define ANDW(dst,src) dst&=src; CarryVal=OverVal=AuxVal=0; SetSZPF_Word(dst)

#define XORB(dst,src) dst^=src; CarryVal=OverVal=AuxVal=0; SetSZPF_Byte(dst)
#define XORW(dst,src) dst^=src; CarryVal=OverVal=AuxVal=0; SetSZPF_Word(dst)

#define CF      (CarryVal!=0)
#define SF      (SignVal<0)
#define ZF      (ZeroVal==0)
#define PF      parity_table[(BYTE)ParityVal]
#define AF      (AuxVal!=0)
#define OF      (OverVal!=0)
/************************************************************************/

/* drop lines A16-A19 for a 64KB memory (yes, I know this should be done after adding the offset 8-) */
#define SegBase(Seg) ((sregs[Seg] << 4) & 0xFFFF)

#define DefaultBase(Seg) ((seg_prefix && (Seg==DS || Seg==SS)) ? prefix_base : base[Seg])

/* ASG 971005 -- changed to cpu_readmem20/cpu_writemem20 */
#define GetMemB(Seg,Off) (cycle_count-=6,(BYTE)cpu_readmem20(DefaultBase(Seg)+(Off)))
#define GetMemW(Seg,Off) (cycle_count-=10,(WORD)GetMemB(Seg,Off)+(WORD)(GetMemB(Seg,(Off)+1)<<8))
#define PutMemB(Seg,Off,x) { cycle_count-=7; cpu_writemem20(DefaultBase(Seg)+(Off),(x)); }
#define PutMemW(Seg,Off,x) { cycle_count-=11; PutMemB(Seg,Off,(BYTE)(x)); PutMemB(Seg,(Off)+1,(BYTE)((x)>>8)); }

#define ReadByte(ea) (cycle_count-=6,(BYTE)cpu_readmem20(ea))
#define ReadWord(ea) (cycle_count-=10,cpu_readmem20(ea)+(cpu_readmem20((ea)+1)<<8))
#define WriteByte(ea,val) { cycle_count-=7; cpu_writemem20(ea,val); }
#define WriteWord(ea,val) { cycle_count-=11; cpu_writemem20(ea,(BYTE)(val)); cpu_writemem20(ea+1,(val)>>8); }

#define read_port(port) cpu_readport(port)
#define write_port(port,val) cpu_writeport(port,val)

/* no need to go through cpu_readmem for these ones... */
/* ASG 971222 -- PUSH/POP now use the standard mechanisms; opcode reading is the same */
#define FETCH ((BYTE)cpu_readop(base[CS]+ip++))
#define FETCHWORD(var) { var=cpu_readop(base[CS]+ip)+(cpu_readop(base[CS]+ip+1)<<8); ip+=2; }
#define PUSH(val) { regs.w[SP]-=2; WriteWord(base[SS]+regs.w[SP],val); }
#define POP(var) { var = ReadWord(base[SS]+regs.w[SP]); regs.w[SP]+=2; }
/************************************************************************/
#define CompressFlags() (WORD)(CF | (PF << 2) | (AF << 4) | (ZF << 6) \
			    | (SF << 7) | (TF << 8) | (IF << 9) \
			    | (DF << 10) | (OF << 11))

#define ExpandFlags(f) \
{ \
      CarryVal = (f) & 1; \
      ParityVal = !((f) & 4); \
      AuxVal = (f) & 16; \
      ZeroVal = !((f) & 64); \
      SignVal = (f) & 128 ? -1 : 0; \
      TF = ((f) & 256) == 256; \
      IF = ((f) & 512) == 512; \
      DF = ((f) & 1024) == 1024; \
      OverVal = (f) & 2048; \
}
