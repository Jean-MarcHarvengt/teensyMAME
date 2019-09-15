/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpuintrf.h"

extern void run_video(void);
extern void irobot_poly_clear(void);
extern int irvg_running;

unsigned char *irobot_nvRAM;
extern unsigned char *comRAM1,*comRAM2,*mbRAM,*mbROM;
static unsigned char irobot_control_num = 0;
static unsigned char irobot_statwr;
static unsigned char irobot_out0;
static unsigned char irobot_outx,irobot_mpage;

unsigned char irobot_comswap;
unsigned char irobot_bufsel;
unsigned char irobot_alphamap;

/* I-Robot Mathbox

    Based on 4 2901 chips slice processors connected to form a 16-bit ALU

    Microcode roms:
    6N: bits 0..3: Address of ALU A register
    5P: bits 0..3: Address of ALU B register
    6M: bits 0..3: ALU Function bits 5..8
    7N: bits 0..3: ALU Function bits 1..4
    8N: bits 0,1: Memory write timing
        bit 2: Hardware multiply mode
        bit 3: ALU Function bit 0
    6P: bits 0,1: Direct addressing bits 0,1
        bits 2,3: Jump address bits 0,1
    8M: bits 0..3: Jump address bits 6..9
    9N: bits 0..3: Jump address bits 2..5
    8P: bits 0..3: Memory address bits 2..5
    9M: bit 0: Shift control
        bits 1..3: Jump type
            0 = No Jump
            1 = On carry
            2 = On zero
            3 = On positive
            4 = On negative
            5 = Unconditional
            6 = Jump to Subroutine
            7 = Return from Subroutine
    7M: Bit 0: Mathbox memory enable
        Bit 1: Latch data to address bus
        Bit 2: Carry in select
        Bit 3: Carry in value
        (if 2,3 = 11 then mathbox is done)
    9P: Bit 0: Hardware divide enable
        Bits 1,2: Memory select
        Bit 3: Memory R/W
    7P: Bits 0,1: Direct addressing bits 6,7
        Bits 2,3: Unused
*/

/* Mathbox variable */

int irmb_running;
static int irmb_PC;
static int irmb_oldPC;

static unsigned int irmb_Q;
static unsigned int irmb_F;
static unsigned int irmb_Y;
static unsigned int irmb_regs[16];
static unsigned int irmb_stack[16];

static int irmb_sp;
static unsigned int irmb_pmadd;
static unsigned int irmb_latch;

static unsigned char irmb_nflag;
static unsigned char irmb_vflag;
static unsigned char irmb_cflag;
static unsigned char irmb_zflag;
static unsigned char irmb_CI;

#define FL_MULT	0x01
#define FL_shift 0x02
#define FL_MBMEMDEC 0x04
#define FL_ADDEN 0x08
#define FL_DPSEL 0x10
#define FL_carry 0x20
#define FL_DIV 0x40
#define FL_MBRW 0x80

/* flags:
	0: MULT
	1: Shift control
	2: MBMEMDEC
	3: ADDEN
	4: DPSEL
	5: Carry
	6: DIV
	7: MBR/W
*/

typedef struct {
    unsigned char areg;
    unsigned char breg;
	int func;
	unsigned int flags;
    unsigned char diradd;
	int nxtadd;
    unsigned char mab;
    unsigned char jtype;
    unsigned char ramsel;
} irmb_ops;

irmb_ops mbops[1024];

int irmb_exec(void);

unsigned int irmb_din(void) {
    unsigned int d=0;
	unsigned int ad;
	int diren;

    if (!(mbops[irmb_PC].flags & FL_MBMEMDEC) && (mbops[irmb_PC].flags & FL_MBRW)) {

        if (mbops[irmb_PC].ramsel == 0) {  /* DIREN = 1 */
			diren = 1;
            ad = mbops[irmb_PC].mab << 2;
            ad = ad | (mbops[irmb_PC].diradd & 0xC0);
            ad = ad | (irmb_latch & 0x1000);
		}
		else { 							 /* DIREN = 0 */
			diren = 0;
            ad = irmb_latch & 0x1FF8;
            ad = ad | (irmb_pmadd & 0x04);
		}

        if (mbops[irmb_PC].ramsel & 0x02)
            ad = ad | (irmb_pmadd & 0x03);
		else
            ad = ad | (mbops[irmb_PC].diradd & 0x03);

        if (diren == 1 || (irmb_latch & 0x6000) == 0) {   /* MB RAM read */
			ad = (ad & 0xFFF) << 1;
			d = (mbRAM[ad] << 8) | mbRAM[ad+1];
		}
		else {			/* MB ROM read */
			ad = (ad & 0x1FFF) << 1;
            if (irmb_latch & 0x4000) {  /* CEMATH = 1 */
                if (irmb_latch & 0x2000)
					d = (mbROM[ad+0x8000] << 8) | mbROM[ad+0x8001];
				else
					d = (mbROM[ad+0x4000] << 8) | mbROM[ad+0x4001];
			}
			else {  /* CEMATH = 0 */
					d = (mbROM[ad] << 8) | mbROM[ad+1];
			}
		}
	}
	return d;
}

void irmb_dout(unsigned int d) {
	unsigned int ad;
	int diren;

	/* Write to video com ram */
    if (mbops[irmb_PC].ramsel == 3) {
        ad = irmb_latch & 0x1FF8;
        ad = ad | (irmb_pmadd & 0x07);
		ad = (ad & 0x7FF) << 1;
        if (irobot_comswap) {
			comRAM2[ad] = (d & 0xFF00) >> 8;
			comRAM2[ad+1] = d & 0x00FF;
		}
		else {
			comRAM1[ad] = (d & 0xFF00) >> 8;
			comRAM1[ad+1] = d & 0x00FF;
		}
	}

	/* Write to mathox ram */
    if (!(mbops[irmb_PC].flags & FL_MBMEMDEC)) {
        if (mbops[irmb_PC].ramsel == 0) {  /* DIREN = 1 */
			diren = 1;
            ad = mbops[irmb_PC].mab << 2;
            ad = ad | (mbops[irmb_PC].diradd & 0xC0);
            ad = ad | (irmb_latch & 0x1000);
		}
		else { 							 /* DIREN = 0 */
			diren = 0;
            ad = irmb_latch & 0x1FF8;
            ad = ad | (irmb_pmadd & 0x04);
		}

        if (mbops[irmb_PC].ramsel & 0x02)
            ad = ad | (irmb_pmadd & 0x03);
		else
            ad = ad | (mbops[irmb_PC].diradd & 0x03);

		ad = (ad & 0xFFF) << 1;
        if (diren == 1 || (irmb_latch & 0x6000) == 0) {   /* MB RAM write */
			mbRAM[ad] = (d & 0xFF00) >> 8;
			mbRAM[ad+1] = (d & 0x00FF);
		}
	}
}

/* Convert microcode roms to a more usable form */

void load_oproms(void) {
	int i;
    unsigned char *MB = Machine->memory_region[2];

	for (i=0; i<1024; i++) {
        mbops[i].areg = MB[0xC000 + i] & 0x0F;
        mbops[i].breg = MB[0xC400 + i] & 0x0F;
        mbops[i].func = (MB[0xC800 + i] & 0x0F) << 5;
        mbops[i].func |= ((MB[0xCC00 +i] & 0x0F) << 1);
        mbops[i].func |= (MB[0xD000 + i] & 0x08) >> 3;
        mbops[i].flags = (MB[0xD000 + i] & 0x04) >> 2;
        mbops[i].nxtadd = (MB[0xD400 + i] & 0x0C) >> 2;
        mbops[i].diradd = MB[0xD400 +i] & 0x03;
        mbops[i].nxtadd |= ((MB[0xD800 + i] & 0x0F) << 6);
        mbops[i].nxtadd |= ((MB[0xDC00 + i] & 0x0F) << 2);
        mbops[i].mab = (MB[0xE000 + i] & 0x0F);
        mbops[i].jtype = (MB[0xE400 + i] & 0x0E) >> 1;
        mbops[i].flags |= (MB[0xE400 + i] & 0x01) << 1;
        mbops[i].flags |= (MB[0xE800 + i] & 0x0F) << 2;
        mbops[i].flags |= (MB[0xEC00 + i] & 0x01) << 6;
        mbops[i].flags |= (MB[0xEC00 + i] & 0x08) << 4;
        mbops[i].ramsel = (MB[0xEC00 + i] & 0x06) >> 1;
        mbops[i].diradd |= (MB[0xF000 + i] & 0x03) << 6;
	}
}

/* Init mathbox (only called once) */

void irmb_init(void) {
	int i;

	for(i=0; i<16; i++) {
        irmb_regs[i]=0x00;
        irmb_stack[i]=0x00;
	}
    irmb_PC=irmb_Q=irmb_F=irmb_Y=irmb_sp=0;
    irmb_nflag=irmb_cflag=irmb_zflag=0;
    irmb_pmadd=0;
    irmb_running=0;
    irmb_oldPC=0;
	load_oproms();
}

/* Run mathbox */
void irmb_run(void) {
	int i;

    if (errorlog) fprintf(errorlog,"Math Start\n");
    irmb_oldPC=irmb_PC=irmb_sp=i=0;
	while (!i) {
        i = irmb_exec();
	}
}

/* Execute instruction */

void do_jump(void) {
    irmb_PC = mbops[irmb_PC].nxtadd;
}

int irmb_exec(void) {
	int fu;
    unsigned int R,S,F;
	unsigned int MDB;
    unsigned long result;

    result = 0;
    R=S=F=0;

    /* Get function code */
    fu = mbops[irmb_PC].func;

    /* Check for done */
    if ((mbops[irmb_PC].flags & FL_DPSEL) && (mbops[irmb_PC].flags & FL_carry)) {
		return 1;
	}

//    if (errorlog) fprintf(errorlog,"MBPC: %x\n",irmb_PC);

	/* Modify function for MULT */
    if (!(mbops[irmb_oldPC].flags & FL_MULT)) {
    	fu = fu ^ 0x02;
    }
    else {
        if (!(irmb_Q & 0x01)) {
			fu = fu | 0x02;
		}
		else {
			fu = fu ^ 0x02;
		}
    }

	/* Modify function for DIV */
    if ((mbops[irmb_oldPC].flags & FL_DIV) || irmb_nflag) {
    	fu = fu ^ 0x08;
    }
    if (!(mbops[irmb_oldPC].flags & FL_DIV) && !irmb_nflag) {
    	fu = fu | 0x08;
    }

	/* Get registers to work on */
	switch (fu & 0x07) {
		case 0:
            R = irmb_regs[mbops[irmb_PC].areg];
            S = irmb_Q;
			break;
		case 1:
            R = irmb_regs[mbops[irmb_PC].areg];
            S = irmb_regs[mbops[irmb_PC].breg];
			break;
		case 2:
			R = 0;
            S = irmb_Q;
			break;
		case 3:
			R = 0;
            S = irmb_regs[mbops[irmb_PC].breg];
			break;
		case 4:
			R = 0;
            S = irmb_regs[mbops[irmb_PC].areg];
			break;
		case 5:
            R = irmb_din();
            S = irmb_regs[mbops[irmb_PC].areg];
			break;
		case 6:
            R = irmb_din();
            S = irmb_Q;
			break;
		case 7:
            R = irmb_din();
			S = 0;
			break;
	}

	/* determine carry in */
    irmb_CI=0;
    if (mbops[irmb_PC].flags & FL_DPSEL) {
        irmb_CI = irmb_cflag;
	}
	else {
        if (mbops[irmb_PC].flags & FL_carry) irmb_CI = 1;
        if (!(mbops[irmb_oldPC].flags & FL_DIV) && !irmb_nflag) irmb_CI = 1;
	}


	/* Do the function */
    irmb_cflag=0;
	switch ((fu & 0x38) >> 3) {
		case 0:
            result = R + S + irmb_CI;
			break;
		case 1:
            result = (R ^ 0xFFFF) + S + irmb_CI;
			break;
		case 2:
            result = R + (S ^ 0xFFFF) + irmb_CI;
			break;
		case 3:
            result = R | S;
			break;
		case 4:
            result = R & S;
			break;
		case 5:
            result = (R ^ 0xFFFF) & S;
			break;
		case 6:
            result = R ^ S;
			break;
		case 7:
            result = (R ^ S) ^ 0xFFFF;
			break;
	}

    F = result & 0xFFFF;

    /* Evaluate flags */
    if (F == 0)
        irmb_zflag = 1;
	else
        irmb_zflag = 0;

	if (F & 0x8000)
        irmb_nflag = 1;
	else
        irmb_nflag = 0;

    if (result > 0xFFFF)
        irmb_cflag = 1;
    else
        irmb_cflag = 0;

    R = R & 0x8000;
    S = S & 0x8000;
    result = result & 0x8000;
    irmb_vflag = !((R ^ S) & (R ^ result));

    /* Do destination */
	switch ((fu & 0x1C0) >> 6) {
		case 0:
            irmb_Q = F;
            irmb_Y = F;
			break;
		case 1:
            irmb_Y = F;
			break;
		case 2:
            irmb_regs[mbops[irmb_PC].breg] = F;
            irmb_Y = irmb_regs[mbops[irmb_PC].areg];
			break;
		case 3:
            irmb_regs[mbops[irmb_PC].breg] = F;
            irmb_Y = F;
			break;
		case 4:
            irmb_regs[mbops[irmb_PC].breg] = F >> 1;
            irmb_Q = irmb_Q >> 1;
            if (mbops[irmb_PC].flags & FL_shift) {
                irmb_Q |= ((F & 0x01) << 15);
                irmb_regs[mbops[irmb_PC].breg] |= ((irmb_nflag ^ irmb_vflag) << 15);
			}
			else {
                irmb_Q |= ((mbops[irmb_PC].flags & 0x20) << 10);
                irmb_regs[mbops[irmb_PC].breg] |= ((mbops[irmb_PC].flags & 0x20) << 10);
			}

            irmb_Y = F;
			break;
		case 5:
            irmb_regs[mbops[irmb_PC].breg] = F >> 1;
            if (mbops[irmb_PC].flags & FL_shift) {
                irmb_regs[mbops[irmb_PC].breg] |= ((irmb_nflag ^ irmb_vflag) << 15);
			}
			else {
                irmb_regs[mbops[irmb_PC].breg] |= ((mbops[irmb_PC].flags & 0x20) << 10);
			}
            irmb_Y = F;
			break;
		case 6:
            irmb_regs[mbops[irmb_PC].breg] = F << 1;
            if (mbops[irmb_PC].flags & FL_shift) {
                irmb_regs[mbops[irmb_PC].breg] |= ((irmb_Q & 0x8000) >> 15);
			}
            irmb_Q = irmb_Q << 1;
            if (mbops[irmb_PC].flags & FL_shift) {
                irmb_Q |= (!irmb_nflag);
			}
            irmb_Y = F;
			break;
		case 7:
            irmb_regs[mbops[irmb_PC].breg] = F << 1;
            if (mbops[irmb_PC].flags & FL_shift) {
                irmb_regs[mbops[irmb_PC].breg] |= ((irmb_Q & 0x8000) >> 15);
			}
            irmb_Y = F;
			break;
	}

	/* Do write */
    if (!(mbops[irmb_PC].flags & FL_MBRW)) {
        irmb_dout(irmb_Y);
	}

	/* ADDEN */
    if (!(mbops[irmb_PC].flags & FL_ADDEN)) {
        if (mbops[irmb_PC].flags & FL_MBRW)
            MDB = irmb_din();
		else
            MDB = irmb_Y;

        irmb_pmadd = MDB & 0x07;
        irmb_latch = MDB & 0xFFF8;
	}

    irmb_oldPC=irmb_PC;
	/* handle jump */
    switch (mbops[irmb_PC].jtype) {
		case 1:
            if (irmb_cflag)
				do_jump();
			else
                irmb_PC=irmb_PC+1;
			break;
		case 2:
            if (irmb_zflag)
				do_jump();
			else
                irmb_PC=irmb_PC+1;
			break;
		case 3:
            if (!irmb_nflag)
				do_jump();
			else
                irmb_PC=irmb_PC+1;
			break;
		case 4:
            if (irmb_nflag)
				do_jump();
			else
                irmb_PC=irmb_PC+1;
			break;
		case 5:
			do_jump();
			break;
		case 6:
            irmb_stack[irmb_sp] = irmb_PC + 1;
            irmb_sp++;
            if (irmb_sp > 15) irmb_sp=0;
			do_jump();
			break;
		case 7:
            if (irmb_sp == 0)
                irmb_sp = 15;
			else
                irmb_sp--;
            irmb_PC = irmb_stack[irmb_sp];
			break;
		default:
            irmb_PC=irmb_PC+1;
	}

	return 0;
}



/***********************************************************************/

void irobot_nvram_w(int offset,int data)
{
    irobot_nvRAM[offset] = data & 0x0F;
}


int irobot_sharedmem_r(int offset) {

    if (irobot_outx == 3) {
        return mbRAM[offset];
    }

    if (irobot_outx == 2) {
        if (irobot_comswap)
            return comRAM2[offset & 0xFFF];
        else
            return comRAM1[offset & 0xFFF];
    }

    if (irobot_outx == 0) {
        if (!(irobot_mpage & 0x01))
            return mbROM[offset];
        else
            return mbROM[0x2000 + offset];
    }
    if (irobot_outx == 1) {
        switch (irobot_mpage) {
            case 0x00:
                return mbROM[0x4000 + offset];
            case 0x01:
                return mbROM[0x6000 + offset];
            case 0x02:
                return mbROM[0x8000 + offset];
            case 0x03:
                return mbROM[0xA000 + offset];
        }
    }
    return 0xFF;
}

/* Comment out the mbRAM =, comRAM2 = or comRAM1 = and it will start working */
void irobot_sharedmem_w(int offset,int data) {

    if (irobot_outx == 3) {
          mbRAM[offset] = (unsigned char)data;
    }

    if (irobot_outx == 2) {
          if (irobot_comswap) {
             comRAM2[offset & 0xFFF] = (unsigned char)data;
          }
          else {
             comRAM1[offset & 0xFFF] = (unsigned char)data;
          }
    }
}

void irobot_statwr_w(int offset, int data) {
    irobot_comswap = data & 0x80;
    irobot_bufsel = data & 0x02;
    if ((data & 0x04) && !(irobot_statwr & 0x04)) {
        if (errorlog) fprintf(errorlog,"VG START\n");
        run_video();
        irvg_running=1;
    }
    if (!(data & 0x01)) irobot_poly_clear();
    if ((data & 0x10) && !(irobot_statwr & 0x10)) {

/**** Comment out the next line to see the gameplay mode ****/
        irmb_run();
        irmb_running=1;
    }
    irobot_statwr = data;

}

void irobot_out0_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


    irobot_out0 = data;
    switch (data & 0x60) {
        case 0:
            cpu_setbank(2, &RAM[0x1C000]);
            break;
        case 0x20:
            cpu_setbank(2, &RAM[0x1C800]);
            break;
        case 0x40:
            cpu_setbank(2, &RAM[0x1D000]);
            break;
    }
    irobot_outx = (data & 0x18) >> 3;
    irobot_mpage = (data & 0x06) >> 1;
    irobot_alphamap = (data & 0x80);
}

void irobot_rom_banksel( int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

    switch ((data & 0x0E) >> 1) {
        case 0:
            cpu_setbank(1, &RAM[0x10000]);
            break;
        case 1:
            cpu_setbank(1, &RAM[0x12000]);
            break;
        case 2:
            cpu_setbank(1, &RAM[0x14000]);
            break;
        case 3:
            cpu_setbank(1, &RAM[0x16000]);
            break;
        case 4:
            cpu_setbank(1, &RAM[0x18000]);
            break;
        case 5:
            cpu_setbank(1, &RAM[0x1A000]);
            break;
    }
    osd_led_w(0,data & 0x10);
    osd_led_w(1,data & 0x20);
}

void irobot_init_machine (void) {
    int i,p;
    unsigned char *MB = Machine->memory_region[2];

    irobot_rom_banksel(0,0);
    irobot_out0_w(0,0);
    irobot_comswap = 0;
    irobot_outx=0;

    /* Convert Mathbox Proms */
    p=0;
    for (i=0; i<16384; i+=2) mbROM[i+1] = MB[p++];
    for (i=0; i<16384; i+=2) mbROM[i] = MB[p++];
    for (i=0; i<32768; i+=2) mbROM[i + 0x4001] = MB[p++];
    for (i=0; i<32768; i+=2) mbROM[i + 0x4000] = MB[p++];
    irmb_init();

}

void irobot_control_w (int offset, int data) {

    irobot_control_num = offset & 0x03;
}

int irobot_control_r (int offset) {

    if (irobot_control_num == 0)
        return readinputport (5);
    else if (irobot_control_num == 1)
        return readinputport (6);
    return 0;

}

/* This still needs work */
int irobot_status_r(int offset)
{
        int d;

        d = (irmb_running * 0x20) | (irvg_running * 0x40);

        irmb_running=irvg_running=0;
        return input_port_2_r(offset) | d;
}
