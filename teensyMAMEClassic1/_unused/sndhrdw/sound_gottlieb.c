#include "driver.h"
#include "M6502/m6502.h"




void gottlieb_sh_w(int offset,int data)
{
	data &= 0xff;

	if (data != 0xff)
	{
		if (Machine->gamedrv->samplenames)
		{
			/* if we have loaded samples, we must be Q*Bert */
			switch(data ^ 0xff)
			{
				case 0x12:
					/* play a sample here (until Votrax speech is emulated) */
					sample_start (0, 0, 0);
					break;

				case 0x14:
					/* play a sample here (until Votrax speech is emulated) */
					sample_start (0, 1, 0);
					break;

				case 0x16:
					/* play a sample here (until Votrax speech is emulated) */
					sample_start (0, 2, 0);
					break;

				case 0x11:
					/* play a sample here (until Votrax speech is emulated) */
					sample_start (0, 3, 0);
					break;
			}
		}

		soundlatch_w(offset,data);

		switch (cpu_gettotalcpu())
		{
		case 2:
			/* Revision 1 sound board */
			cpu_cause_interrupt(1,M6502_INT_IRQ);
			break;
		case 3:
		case 4:
			/* Revision 2 & 3 sound board */
			cpu_cause_interrupt(cpu_gettotalcpu()-1,M6502_INT_IRQ);
			cpu_cause_interrupt(cpu_gettotalcpu()-2,M6502_INT_IRQ);
			break;
		}
	}
}



/* callback for the timer */
void gottlieb_nmi_generate(int param)
{
	cpu_cause_interrupt(1,M6502_INT_NMI);
}

static const char *PhonemeTable[65] =
{
 "EH3","EH2","EH1","PA0","DT" ,"A1" ,"A2" ,"ZH",
 "AH2","I3" ,"I2" ,"I1" ,"M"  ,"N"  ,"B"  ,"V",
 "CH" ,"SH" ,"Z"  ,"AW1","NG" ,"AH1","OO1","OO",
 "L"  ,"K"  ,"J"  ,"H"  ,"G"  ,"F"  ,"D"  ,"S",
 "A"  ,"AY" ,"Y1" ,"UH3","AH" ,"P"  ,"O"  ,"I",
 "U"  ,"Y"  ,"T"  ,"R"  ,"E"  ,"W"  ,"AE" ,"AE1",
 "AW2","UH2","UH1","UH" ,"O2" ,"O1" ,"IU" ,"U1",
 "THV","TH" ,"ER" ,"EH" ,"E1" ,"AW" ,"PA1","STOP",
 0
};

void gottlieb_speech_w(int offset, int data)
{
	data ^= 255;

if (errorlog) fprintf(errorlog,"Votrax: intonation %d, phoneme %02x %s\n",data >> 6,data & 0x3f,PhonemeTable[data & 0x3f]);

	/* generate a NMI after a while to make the CPU continue to send data */
	timer_set(TIME_IN_USEC(50),0,gottlieb_nmi_generate);
}

void gottlieb_speech_clock_DAC_w(int offset, int data)
{}



    /* partial decoding takes place to minimize chip count in a 6502+6532
       system, so both page 0 (direct page) and 1 (stack) access the same
       128-bytes ram,
       either with the first 128 bytes of the page or the last 128 bytes */

int riot_ram_r(int offset)
{
	extern unsigned char *RAM;


    return RAM[offset&0x7f];
}

void riot_ram_w(int offset, int data)
{
	extern unsigned char *RAM;


	/* pb is that M6502.c does some memory reads directly, so we
	  repeat the writes */
    RAM[offset&0x7F]=data;
    RAM[0x80+(offset&0x7F)]=data;
    RAM[0x100+(offset&0x7F)]=data;
    RAM[0x180+(offset&0x7F)]=data;
}

static unsigned char riot_regs[32];
    /* lazy handling of the 6532's I/O, and no handling of timers at all */

int gottlieb_riot_r(int offset)
{
    switch (offset&0x1f) {
	case 0: /* port A */
		return soundlatch_r(offset) ^ 0xff;	/* invert command */
	case 2: /* port B */
		return 0x40;    /* say that PB6 is 1 (test SW1 not pressed) */
	case 5: /* interrupt register */
		return 0x40;    /* say that edge detected on PA7 */
	default:
		return riot_regs[offset&0x1f];
    }
}

void gottlieb_riot_w(int offset, int data)
{
    riot_regs[offset&0x1f]=data;
}




static int psg_latch;
static void *nmi_timer;
static int nmi_rate, nmi_enabled;
static int ym2151_port;

int gottlieb_sh_start(void)
{
	nmi_timer = NULL;
	return 0;
}

int stooges_sound_input_r(int offset)
{
	/* bits 0-3 are probably unused (future expansion) */

	/* bits 4 & 5 are two dip switches. Unused? */

	/* bit 6 is the test switch. When 0, the CPU plays a pulsing tone. */

	/* bit 7 comes from the speech chip DATA REQUEST pin */

	return 0xc0;
}

void stooges_8910_latch_w(int offset,int data)
{
	psg_latch = data;
}

/* callback for the timer */
static void nmi_callback(int param)
{
	cpu_cause_interrupt(cpu_gettotalcpu()-1, M6502_INT_NMI);
}

static void common_sound_control_w(int offset, int data)
{
	/* Bit 0 enables and starts NMI timer */

	if (nmi_timer)
	{
		timer_remove(nmi_timer);
		nmi_timer = 0;
	}

	if (data & 0x01)
	{
		/* base clock is 250kHz divided by 256 */
		double interval = TIME_IN_HZ(250000.0/256/(256-nmi_rate));
		nmi_timer = timer_pulse(interval, 0, nmi_callback);
	}

	/* Bit 1 controls a LED on the sound board. I'm not emulating it */
}

void stooges_sound_control_w(int offset,int data)
{
	static int last;

	common_sound_control_w(offset, data);

	/* bit 2 goes to 8913 BDIR pin  */
	if ((last & 0x04) == 0x04 && (data & 0x04) == 0x00)
	{
		/* bit 3 selects which of the two 8913 to enable */
		if (data & 0x08)
		{
			/* bit 4 goes to the 8913 BC1 pin */
			if (data & 0x10)
				AY8910_control_port_0_w(0,psg_latch);
			else
				AY8910_write_port_0_w(0,psg_latch);
		}
		else
		{
			/* bit 4 goes to the 8913 BC1 pin */
			if (data & 0x10)
				AY8910_control_port_1_w(0,psg_latch);
			else
				AY8910_write_port_1_w(0,psg_latch);
		}
	}

	/* bit 5 goes to the speech chip DIRECT DATA TEST pin */

	/* bit 6 = speech chip DATA PRESENT pin; high then low to make the chip read data */
	if ((last & 0x40) == 0x40 && (data & 0x40) == 0x00)
	{
	}

	/* bit 7 goes to the speech chip RESET pin */

	last = data & 0x44;
}

void exterm_sound_control_w(int offset, int data)
{
	common_sound_control_w(offset, data);

	/* Bit 7 selects YM2151 register or data port */
	ym2151_port = data & 0x80;
}

void gottlieb_nmi_rate_w(int offset, int data)
{
	nmi_rate = data;
}

static void cause_dac_nmi_callback(int param)
{
	cpu_cause_interrupt(cpu_gettotalcpu()-2, M6502_INT_NMI);
}

void gottlieb_cause_dac_nmi_w(int offset, int data)
{
	/* make all the CPUs synchronize, and only AFTER that cause the NMI */
	timer_set(TIME_NOW,0,cause_dac_nmi_callback);
}

int gottlieb_cause_dac_nmi_r(int offset)
{
    gottlieb_cause_dac_nmi_w(offset, 0);
	return 0;
}

void exterm_ym2151_w(int offset, int data)
{
	if (ym2151_port)
	{
		YM2151_data_port_0_w(offset, data);
	}
	else
	{
		YM2151_register_port_0_w(offset, data);
	}
}
