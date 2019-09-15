/*************************************************************************
 Preliminary driver for Williams/Midway games using the TMS34010 processor
 This is very much a work in progress.

 sound hardware

**************************************************************************/
#include "driver.h"
#include "TMS34010/tms34010.h"
#include "M6809/M6809.h"
#include "vidhrdw/generic.h"
#include "machine/6821pia.h"

/* CVSD stuff */
static unsigned char ucStepSize[64] = {
	1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31, /* 16 */
	33,34,36,37,39,40,41,42,43,44,45,46,47,48,49,50, /* 16 */
	51,52,53,53,54,54,55,55,55,56,56,56,56,56,57,57, /* 16 */
	57,57,57,57,58,58,58,58,58,59,59,59,59,59,59,59}; /* 16 */

static int iStepIndex = 0; /* Index into the step values table */
static char cData, cShift; /* Shift register values for determining step size */
static int iDAC = 0;
static int cDAC;
static int iOldTicks = 0;

void CVSD_clock_w(int offset, int databit)
{
	int iDelta;
	/* speech clock */
	databit = /* (databit&0x01) ? */ 1 /*: 0*/;
//	if (errorlog) fprintf(errorlog, "CVSD clock %x\n", databit);
//	iDelta = iOldTicks - cpu_gettotalcycles();
	iDelta = cpu_gettotalcycles() - iOldTicks;
	/* Number of E states since last change */
	if (iDelta > 50000 ) /* If has been idle for a while, reset step value*/
	{
		iStepIndex = 0;
		iDAC = 0;
	}
	if (databit||1)/* Speech clock changing (active on rising edge) */
	{
		cShift=((cShift<<1)+cData)&0x07;
		if (cData)
		{
			iDAC += ucStepSize[iStepIndex];
			if (iDAC > 1023)
				iDAC = 1023;
		}
		else
		{
			iDAC -= ucStepSize[iStepIndex];
			if (iDAC < -1024)
				iDAC = -1024;
		}
		/* Check 3-bit shift register for all 0's or all 1's to change step size */
		if (cShift == 7 || cShift == 0) /* Increase step size */
		{
			iStepIndex++;
			if (iStepIndex > 63)
				iStepIndex = 63;
		}
		else /* Decrease step size */
		{
			iStepIndex--;
			if (iStepIndex == 11)
				iStepIndex = 12;
			if (iStepIndex < 0)
				iStepIndex = 0;
		}
		cDAC = (iDAC / 8) + 128;
		DAC_data_w(1,cDAC);
		iOldTicks = cpu_gettotalcycles(); /* In preparation for next time */
	}
}
void CVSD_digit_w(int offset, int databit)
{
	/* speech data */
	databit = (databit&0x01) ? 1 : 0;
//	if (errorlog) fprintf(errorlog, "CVSD digit %x\n", databit);
	cData = databit; /* Save the current speech data bit */
	CVSD_clock_w(0,1);
}


void smashtv_ym2151_int (void)
{
//	if (errorlog) fprintf(errorlog, "ym2151_int\n");
	pia_1_ca1_w (0, (0));
	pia_1_ca1_w (0, (1));
	
}
void narc_ym2151_int (void)
{
	cpu_cause_interrupt(1,M6809_INT_FIRQ);
//	if (errorlog) fprintf(errorlog, "ym2151_int: ");
//	if (errorlog) fprintf(errorlog, "sound FIRQ\n");
}
void smashtv_snd_cmd_real_w (int param)
{
	pia_1_portb_w (0, param&0xff);
	pia_1_cb1_w (0, (param&0x200?1:0));
	if (!(param&0x100))
	{
		cpu_reset(1);
		if (errorlog) fprintf(errorlog, "sound reset\n");
	}
}
void narc_snd_cmd_real_w (int param)
{
	soundlatch_w (0, param&0xff);
	if (!(param&0x200))
	{
		if (errorlog) fprintf(errorlog, "sound IRQ\n");
		cpu_cause_interrupt(1, M6809_INT_IRQ);
	}
	if (!(param&0x100))
	{
		if (errorlog) fprintf(errorlog, "sound NMI\n");
		cpu_cause_interrupt(1, M6809_INT_NMI);
	}
}
void mk_snd_cmd_real_w (int param)
{
	soundlatch_w (0, param&0xff);
	if (!(param&0x200))
	{
		if (errorlog) fprintf(errorlog, "sound IRQ\n");
		cpu_cause_interrupt(1, M6809_INT_IRQ);
	}
}
void smashtv_sound_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: ", cpu_getpc());
	if (errorlog) fprintf(errorlog, "sound write %x\n", data);
	timer_set (TIME_NOW, data, smashtv_snd_cmd_real_w);
}
void trog_sound_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: ", cpu_getpc());
	if (errorlog) fprintf(errorlog, "sound write %x\n", data|0x100);
	timer_set (TIME_NOW, data|0x0100, smashtv_snd_cmd_real_w);
}
void narc_sound_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: ", cpu_getpc());
	if (errorlog) fprintf(errorlog, "sound write %x\n", data);
	timer_set (TIME_NOW, data, narc_snd_cmd_real_w);
}
void mk_sound_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: ", cpu_getpc());
	if (errorlog) fprintf(errorlog, "sound write %x\n", data);
	timer_set (TIME_NOW, data, mk_snd_cmd_real_w);
}
void mk_sound_talkback_w(int offset, int data)
{
//	cpu_cause_interrupt(0,TMS34010_INT2);
}
int narc_DAC_r(int offset)
{
	return 0;
}
void narc_slave_cmd_real_w (int param)
{
	soundlatch2_w (0, param&0xff);
	cpu_cause_interrupt(2, M6809_INT_FIRQ);
}
void narc_slave_cmd_w(int offset, int data)
{
	timer_set (TIME_NOW, data, narc_slave_cmd_real_w);
}
void narc_slave_DAC_w(int offset, int data)
{
	DAC_data_w(2,data);
}
