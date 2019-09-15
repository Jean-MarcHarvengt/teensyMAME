/*****************************************************************************
 *
 *	 m6502.c
 *	 Portable 6502/65c02/6510 emulator
 *
 *	 Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#include <stdio.h>
#include "memory.h"
#include "string.h"
#include "driver.h"
#include "m6502.h"
#include "m6502ops.h"

int     M6502_ICount = 0;
int 	M6502_Type = M6502_PLAIN;
static	void(**insn)(void);
static	M6502_Regs	m6502;

/***************************************************************
 * include the opcode macros, functions and tables
 ***************************************************************/
#include "tbl6502.h"
#include "tbl65c02.h"
#include "tbl6510.h"

/*****************************************************************************
 *
 *		6502 CPU interface functions
 *
 *****************************************************************************/

unsigned M6502_GetPC (void)
{
	return PCD;
}

void M6502_GetRegs (M6502_Regs *Regs)
{
	*Regs = m6502;
}

void M6502_SetRegs (M6502_Regs *Regs)
{
	m6502 = *Regs;
}

void M6502_Reset(void)
{
	switch (M6502_Type)
	{
#if SUPP65C02
        case M6502_65C02:
			insn = insn65c02;
            break;
#endif
#if SUPP6510
        case M6502_6510:
			insn = insn6510;
			break;
#endif
        default:
            insn = insn6502;
            break;
    }

    /* wipe out the m6502 structure */
	memset(&m6502, 0, sizeof(M6502_Regs));

	/* set T, I and Z flags */
	P = _fT | _fI | _fZ;

    /* stack starts at 0x01ff */
	m6502.sp.D = 0x1ff;

    /* read the reset vector into PC */
	PCL = RDMEM(M6502_RST_VEC);
	PCH = RDMEM(M6502_RST_VEC+1);
	change_pc16(PCD);

    /* clear pending interrupts */
    M6502_Clear_Pending_Interrupts();
}

int M6502_Execute(int cycles)
{
    M6502_ICount = cycles;

	change_pc16(PCD);

    do
    {
		{
			extern int previouspc;
			previouspc = PCW;
		}

		if (m6502.halt)
            break;

        insn[RDOP()]();

        /* check if the I flag was just reset (interrupts enabled) */
        if (m6502.after_cli)
            m6502.after_cli = 0;
        else
		if (m6502.pending_nmi || (m6502.pending_irq && !(P & _fI)))
        {
            if (m6502.pending_nmi)
            {
                m6502.pending_nmi = 0;
				EAD = M6502_NMI_VEC;
            }
            else
            {
                m6502.pending_irq = 0;
				EAD = M6502_IRQ_VEC;
            }
			M6502_ICount -= 7;
			m6502.halt = 0;
			PUSH(PCH);
			PUSH(PCL);
			COMPOSE_P;
			PUSH(P & ~_fB);
			P = (P & ~_fD) | _fI;		/* knock out D and set I flag */
			PCL = RDMEM(EAD);
			PCH = RDMEM(EAD+1);
			change_pc16(PCD);
        }

    } while (M6502_ICount > 0);

    return cycles - M6502_ICount;
}

void M6502_Cause_Interrupt(int type)
{
	if (type == M6502_INT_NMI)
		m6502.pending_nmi = 1;
	else
	if (type == M6502_INT_IRQ)
		m6502.pending_irq = 1;
}

void M6502_Clear_Pending_Interrupts(void)
{
	m6502.pending_nmi = M6502_INT_NONE;
	m6502.pending_irq = M6502_INT_NONE;
}

