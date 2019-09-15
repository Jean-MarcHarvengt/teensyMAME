/***************************************************************************

  machine\qix.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "timer.h"
#include "M6809/M6809.h"
#include "M6805/M6805.h"
#include "6821pia.h"

static int sdungeon_coin_r (int offset);
static void sdungeon_coinctrl_w (int offset, int data);
static void sdungeon_coin_w (int offset, int data);

static int qix_sound_r (int offset);
static void qix_dac_w (int offset, int data);
static void qix_pia_sint (void);
static void qix_pia_dint (void);
static int suspended;

static int sdungeon_coin_io;
static int sdungeon_main_io;
static int sdungeon_ioctrl;


/***************************************************************************

	Qix has 6 PIAs on board:

	From the ROM I/O schematic:

	PIA 1 = U11:
		port A = external input (input_port_0)
		port B = external input (input_port_1)
	PIA 2 = U20:
		port A = external input (???)
		port B = external input (???)
	PIA 3 = U30:
		port A = external input (???)
		port B = external input (???)


	From the data/sound processor schematic:

	PIA 4 = U20:
		port A = data CPU to sound CPU communication
		port B = some kind of sound control
		CA1 = interrupt signal from sound CPU
		CA2 = interrupt signal to sound CPU
	PIA 5 = U8:
		port A = sound CPU to data CPU communication
		port B = DAC value (port B)
		CA1 = interrupt signal from data CPU
		CA2 = interrupt signal to data CPU
	PIA 6 = U7: (never actually used)
		port A = unused
		port B = sound CPU to TMS5220 communication
		CA1 = interrupt signal from TMS5220
		CA2 = write signal to TMS5220
		CB1 = ready signal from TMS5220
		CB2 = read signal to TMS5220

***************************************************************************/

static pia6821_interface pia_intf_withmcu =
{
	6,                                             	/* 6 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },    	/* offsets */
	{ input_port_0_r, 0, 0, 0, qix_sound_r, 0 },   	/* input port A  */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CA1 */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CA2 */
        { sdungeon_coin_r, input_port_2_r, 0, 0, 0, 0 }, /* input port B  */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CB1 */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CB2 */
	{ 0, 0, 0, pia_5_porta_w, pia_4_porta_w, 0 },   /* output port A */
        { sdungeon_coin_w, 0, sdungeon_coinctrl_w, 0, qix_dac_w, 0 },  /* output port B */
	{ 0, 0, 0, pia_5_ca1_w, pia_4_ca1_w, 0 },       /* output CA2 */
	{ 0, 0, 0, 0, 0, 0 },                          	/* output CB2 */
	{ 0, 0, 0, 0/*qix_pia_dint*/, qix_pia_sint, 0 },/* IRQ A */
	{ 0, 0, 0, 0/*qix_pia_dint*/, qix_pia_sint, 0 },/* IRQ B */
};

static pia6821_interface pia_intf =
{
	6,                                             	/* 6 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },    	/* offsets */
	{ input_port_0_r, 0, 0, 0, qix_sound_r, 0 },   	/* input port A  */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CA1 */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CA2 */
	{ input_port_1_r, input_port_2_r, 0, 0, 0, 0 }, /* input port B  */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CB1 */
	{ 0, 0, 0, 0, 0, 0 },                           /* input bit CB2 */
	{ 0, 0, 0, pia_5_porta_w, pia_4_porta_w, 0 },   /* output port A */
	{ 0, 0, 0, 0, qix_dac_w, 0 },                   /* output port B */
	{ 0, 0, 0, pia_5_ca1_w, pia_4_ca1_w, 0 },       /* output CA2 */
	{ 0, 0, 0, 0, 0, 0 },                          	/* output CB2 */
	{ 0, 0, 0, 0/*qix_pia_dint*/, qix_pia_sint, 0 },/* IRQ A */
	{ 0, 0, 0, 0/*qix_pia_dint*/, qix_pia_sint, 0 },/* IRQ B */
};



unsigned char *qix_sharedram;


int qix_sharedram_r(int offset)
{
	return qix_sharedram[offset];
}


void qix_sharedram_w(int offset,int data)
{
	qix_sharedram[offset] = data;
}


void zoo_bankswitch_w(int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];


	if (data & 0x04) cpu_setbank (1, &RAM[0x10000])
	else cpu_setbank (1, &RAM[0xa000]);
}


void qix_video_firq_w(int offset, int data)
{
	/* generate firq for video cpu */
	cpu_cause_interrupt(1,M6809_INT_FIRQ);
}



void qix_data_firq_w(int offset, int data)
{
	/* generate firq for data cpu */
	cpu_cause_interrupt(0,M6809_INT_FIRQ);
}



/* Return the current video scan line */
int qix_scanline_r(int offset)
{
	return cpu_scalebyfcount(256);
}

void withmcu_init_machine(void)
{
	suspended = 0;

	/* Set OPTIMIZATION FLAGS FOR M6809 */
        m6809_Flags = M6809_FAST_S;/* | M6809_FAST_U;*/

        pia_startup (&pia_intf_withmcu);
}

void qix_init_machine(void)
{
	suspended = 0;

	/* Set OPTIMIZATION FLAGS FOR M6809 */
	m6809_Flags = M6809_FAST_S;/* | M6809_FAST_U;*/

        pia_startup (&pia_intf);
}

void zoo_init_machine(void)
{
	suspended = 0;

	/* Set OPTIMIZATION FLAGS FOR M6809 */
	m6809_Flags = M6809_FAST_NONE;

        pia_startup (&pia_intf);
}


/***************************************************************************

	6821 PIA handlers

***************************************************************************/

static void qix_dac_w (int offset, int data)
{
	DAC_data_w (0, data);
}

int qix_sound_r (int offset)
{
	/* if we've suspended the main CPU for this, trigger it and give up some of our timeslice */
	if (suspended)
	{
		timer_trigger (500);
		cpu_yielduntil_time (TIME_IN_USEC (100));
		suspended = 0;
	}
	return pia_5_porta_r (offset);
}

static void qix_pia_dint (void)
{
	/* not used by Qix, but others might use it; depends on a jumper on the PCB */
}

static void qix_pia_sint (void)
{
	/* generate a sound interrupt */
	cpu_cause_interrupt (2, M6809_INT_IRQ);

	/* wait for the sound CPU to read the command */
	cpu_yielduntil_trigger (500);
	suspended = 1;

	/* but add a watchdog so that we're not hosed if interrupts are disabled */
	cpu_triggertime (TIME_IN_USEC (100), 500);
}

/***************************************************************************

        68705 Communication

***************************************************************************/

void sdungeon_68705_mcu_w(int offest, int value) {
//  if (errorlog) fprintf(errorlog,"PC: %x SD COINTOMAIN W: %x\n",cpu_getpc(),value);
    sdungeon_coin_io = value;
}

int sdungeon_68705_mcu_r(int offset) {
    return sdungeon_main_io;
}

int sdungeon_68705_portc_r(int offset) {
    int i;

    i = (input_port_1_r(0) & 0x70) >> 4;
    i |= ((sdungeon_ioctrl & 0x08) ^ 0x08);
//    if (errorlog) fprintf(errorlog,"PC: %x MCU PORTC R = %x\n",cpu_getpc(),i);
    return i;
}

int sdungeon_68705_portb_r(int offset) {
    int i;

    i = input_port_1_r(0) & 0x0F;
    i = i | ((input_port_1_r(0) & 0x80) >> 3);
    i = i | 0xE0;
    return i;
}

int sdungeon_coin_r (int offset) {
//    if (errorlog) fprintf(errorlog,"PC: %x SD MAIN READ FROM COIN %x\n",cpu_getpc(),sdungeon_coin_io);
    return sdungeon_coin_io;
}

static void sdungeon_coin_w (int offset, int data) {
    sdungeon_main_io = data;
}

static void sdungeon_coinctrl_w (int offset, int data) {
// if (errorlog) fprintf(errorlog,"SD COINCTRL W: %x  %x\n",data,sdungeon_ioctrl);
   if ((data & 0x04) && !(sdungeon_ioctrl & 0x04)) {
        cpu_cause_interrupt(3,M6805_INT_IRQ);
   }
   sdungeon_ioctrl = data;
}


