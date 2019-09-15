/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6808/M6808.h"
#include "M6809/M6809.h"
#include "6821pia.h"
#include "machine/ticket.h"


/* defined in vidhrdw/williams.c */
extern unsigned char *williams_videoram;

/* various banking controls */
unsigned char *williams_bank_base;
unsigned char *williams_video_counter;
unsigned char *defender_bank_base;
unsigned char *blaster_bank2_base;

/* pointers to memory locations for speedup optimizations */
unsigned char *robotron_catch;
unsigned char *stargate_catch;
unsigned char *defender_catch;
unsigned char *splat_catch;
unsigned char *blaster_catch;
unsigned char *mayday_catch;

/* internal bank switching tracks */
int blaster_bank;
int vram_bank;

/* switches controlled by $c900 */
int sinistar_clip;
int williams_cocktail;

/* video counter offset */
static int port_select;
static unsigned char defender_video_counter;

/* various input port maps */
int williams_input_port_0_3 (int offset);
int williams_input_port_1_4 (int offset);
int stargate_input_port_0_r (int offset);
int blaster_input_port_0_r (int offset);
int sinistar_input_port_0_r (int offset);
int defender_input_port_0_r (int offset);
int lottofun_input_port_0_r (int offset);

/* Defender-specific code */
int defender_io_r (int offset);
void defender_io_w (int offset,int data);
void defender_bank_select_w (int offset, int data);

/* Colony 7-specific code */
void colony7_bank_select_w (int offset, int data);

/* PIA interface functions */
static void williams_irq (void);
static void williams_snd_irq (void);
static void williams_snd_cmd_w (int offset, int cmd);
static void williams_port_select_w (int offset, int data);
static void sinistar_snd_cmd_w (int offset, int cmd);

/* external code to update part of the screen */
void williams_vh_update (int counter);


/***************************************************************************

	PIA Interfaces for each game

***************************************************************************/

static pia6821_interface robotron_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ input_port_0_r, input_port_2_r, 0 },          /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ 0, 0, 0 },                                    /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface joust_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ williams_input_port_0_3, input_port_2_r, 0 }, /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ williams_port_select_w, 0, 0 },               /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface stargate_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ stargate_input_port_0_r, input_port_2_r, 0 }, /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ 0, 0, 0 },                                    /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface bubbles_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ input_port_0_r, input_port_2_r, 0 },          /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ 0, 0, 0 },                                    /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface splat_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ williams_input_port_0_3, input_port_2_r, 0 }, /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ williams_input_port_1_4, 0, 0 },              /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ williams_port_select_w, 0, 0 },               /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface sinistar_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ sinistar_input_port_0_r, input_port_2_r, 0 }, /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, sinistar_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ 0, 0, 0 },                                    /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface blaster_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ blaster_input_port_0_r, input_port_2_r, 0 },  /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ 0, 0, 0 },                                    /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface defender_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ defender_input_port_0_r, input_port_2_r, 0 }, /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ 0, 0, 0 },                                    /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface colony7_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ input_port_0_r, input_port_2_r, 0 },          /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ 0, williams_snd_cmd_w, 0 },                   /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ 0, 0, 0 },                                    /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};

static pia6821_interface lottofun_pia_intf =
{
	3,                                              /* 3 chips */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ lottofun_input_port_0_r, input_port_2_r, 0 }, /* input port A */
	{ 0, 0, 0 },                                    /* input bit CA1 */
	{ 0, 0, 0 },                                    /* input bit CA2 */
	{ input_port_1_r, 0, 0 },                       /* input port B */
	{ 0, 0, 0 },                                    /* input bit CB1 */
	{ 0, 0, 0 },                                    /* input bit CB2 */
	{ 0, 0, DAC_data_w },                           /* output port A */
	{ ticket_dispenser_w, williams_snd_cmd_w, 0 },  /* output port B */
	{ 0, 0, 0 },                                    /* output CA2 */
	{ 0, 0, 0 },                                    /* output CB2 */
	{ 0, williams_irq, williams_snd_irq },          /* IRQ A */
	{ 0, williams_irq, williams_snd_irq }           /* IRQ B */
};



/***************************************************************************

	Common Williams routines

***************************************************************************/

/*
 *  Initialize the machine
 */

void robotron_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&robotron_pia_intf);
}

void joust_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&joust_pia_intf);
}

void stargate_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&stargate_pia_intf);
}

void bubbles_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&bubbles_pia_intf);
}

void splat_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&splat_pia_intf);
}

void sinistar_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&sinistar_pia_intf);
}

void blaster_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&blaster_pia_intf);
}

void defender_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&defender_pia_intf);
	williams_video_counter = &defender_video_counter;

	/* initialize the banks */
	defender_bank_select_w (0, 0);
}

void colony7_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&colony7_pia_intf);
	williams_video_counter = &defender_video_counter;

	/* initialize the banks */
	colony7_bank_select_w (0, 0);
}

void lottofun_init_machine (void)
{
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&lottofun_pia_intf);

	/* Initialize the ticket dispenser to 70 milliseconds */
	/* (I'm not sure what the correct value really is) */
	ticket_dispenser_init(70, TICKET_MOTOR_ACTIVE_LOW, TICKET_STATUS_ACTIVE_HIGH);
}


/*
 *  Generic interrupt routine; interrupts are generated via the PIA, so we merely pulse the
 *  external inputs here; the PIA emulator will generate the interrupt if the proper conditions
 *  are met
 */

int williams_interrupt (void)
{
	/* the video counter value is taken from the six bits of VA8-VA13 */
	*williams_video_counter = ((64 - cpu_getiloops ()) << 2);

	/* the IRQ signal comes into CB1, and is set to VA11 */
	pia_2_cb1_w (0, *williams_video_counter & 0x20);

	/* the COUNT240 signal comes into CA1, and is set to the logical AND of VA10-VA13 */
	pia_2_ca1_w (0, (*williams_video_counter & 0xf0) == 0xf0);

	/* update the screen partially */
	williams_vh_update (*williams_video_counter);

	/* PIA generates interrupts, not us */
	return ignore_interrupt ();
}


/*
 *  Read either port 0 or 3, depending on the value in CB2
 */

int williams_input_port_0_3 (int offset)
{
	if (port_select)
		return input_port_3_r (0);
	else
		return input_port_0_r (0);
}


/*
 *  Read either port 2 or 3, depending on the value in CB2
 */

int williams_input_port_1_4 (int offset)
{
	if (port_select)
	   return input_port_4_r (0);
	else
	   return input_port_1_r (0);
}


/*
 *  Switch between VRAM and ROM
 */

void williams_vram_select_w (int offset, int data)
{
	vram_bank = data & 0x01;
	williams_cocktail = data & 0x02;
	sinistar_clip = data & 0x04;

	if (vram_bank)
	{
		cpu_setbank (1, williams_bank_base);
	}
	else
	{
		cpu_setbank (1, williams_videoram);
	}
}


/*
 *  PIA callback to generate the interrupt to the main CPU
 */

static void williams_irq (void)
{
	cpu_cause_interrupt (0, M6809_INT_IRQ);
}


/*
 *  PIA callback to generate the interrupt to the sound CPU
 */

static void williams_snd_irq (void)
{
	cpu_cause_interrupt (1, M6808_INT_IRQ);
}


/*
 *  Handle a write to the PIA which communicates to the sound CPU
 */

static void williams_snd_cmd_real_w (int param)
{
	pia_3_portb_w (0, param);
	pia_3_cb1_w (0, (param == 0xff) ? 0 : 1);
}

static void williams_snd_cmd_w (int offset, int cmd)
{
	/* the high two bits are set externally, and should be 1 */
	timer_set (TIME_NOW, cmd | 0xc0, williams_snd_cmd_real_w);
}


/*
 *  Detect a switch between input port sets
 */

static void williams_port_select_w (int offset, int data)
{
	port_select = data;
}


/***************************************************************************

	Robotron-specific routines

***************************************************************************/

/* JB 970823 - speed up very busy loop in Robotron */
/*    D19B: LDA   $10    ; dp=98
      D19D: CMPA  #$02
      D19F: BCS   $D19B  ; (BLO)   */
int robotron_catch_loop_r (int offset)
{
	unsigned char t = *robotron_catch;
	if (t < 2 && cpu_getpc () == 0xd19d)
		cpu_seticount (0);
	return t;
}



/***************************************************************************

	Stargate-specific routines

***************************************************************************/

int stargate_input_port_0_r(int offset)
{
	int keys, altkeys;

	keys = input_port_0_r (0);
	altkeys = input_port_3_r (0);

	if (altkeys)
	{
		keys |= altkeys;
		if (Machine->memory_region[0][0x9c92] == 0xfd)
		{
			if (keys & 0x02)
				keys = (keys & 0xfd) | 0x40;
			else if (keys & 0x40)
				keys = (keys & 0xbf) | 0x02;
		}
	}

	return keys;
}


/* JB 970823 - speed up very busy loop in Stargate */
/*    0011: LDA   $39    ; dp=9c
      0013: BEQ   $0011   */
int stargate_catch_loop_r (int offset)
{
	unsigned char t = *stargate_catch;
	if (t == 0 && cpu_getpc () == 0x0013)
		cpu_seticount (0);
	return t;
}



/***************************************************************************

	Defender-specific routines

***************************************************************************/

/*
 *  Defender Select a bank
 *  There is just data in bank 0
 */

void defender_bank_select_w (int offset,int data)
{
	static int bank[8] = { 0x0c000, 0x10000, 0x11000, 0x12000, 0x0c000, 0x0c000, 0x0c000, 0x13000 };
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* set bank address */
	cpu_setbank (1, &RAM[bank[data & 7]]);

	if (bank[data] < 0x10000)
	{
		/* i/o area map */
		cpu_setbankhandler_r (1, defender_io_r);
		cpu_setbankhandler_w (1, defender_io_w);
	}
	else
	{
		/* bank rom map */
		cpu_setbankhandler_r (1, MRA_BANK1);
		cpu_setbankhandler_w (1, MWA_ROM);
	}
}


/*
 *   Defender Read at C000-CFFF
 */

int defender_input_port_0_r(int offset)
{
	int keys, altkeys;

	keys = readinputport(0);
	altkeys = readinputport(3);

	if (altkeys)
	{
		keys |= altkeys;
		if (Machine->memory_region[0][0xa0bb] == 0xfd)
		{
			if (keys & 0x02)
				keys = (keys & 0xfd) | 0x40;
			else if (keys & 0x40)
				keys = (keys & 0xbf) | 0x02;
		}
	}

	return keys;
}

int defender_io_r (int offset)
{
	/* PIAs */
	if (offset >= 0x0c00 && offset < 0x0c04)
		return pia_2_r (offset - 0x0c00);
	else if (offset >= 0x0c04 && offset < 0x0c08)
		return pia_1_r (offset - 0x0c04);

	/* video counter */
	else if (offset == 0x800)
		return *williams_video_counter;

	/* If not bank 0 then return banked RAM */
	return defender_bank_base[offset];
}


/*
 *  Defender Write at C000-CFFF
 */

void defender_io_w (int offset,int data)
{
	defender_bank_base[offset] = data;

	/* WatchDog */
	if (offset == 0x03fc)
		watchdog_reset_w (offset, data);

	/* Palette */
	else if (offset < 0x10)
		paletteram_BBGGGRRR_w(offset,data);

	/* PIAs */
	else if (offset >= 0x0c00 && offset < 0x0c04)
		pia_2_w (offset - 0x0c00, data);
	else if (offset >= 0x0c04 && offset < 0x0c08)
		pia_1_w (offset - 0x0c04, data);
}


/* JB 970823 - speed up very busy loop in Defender */
/*    E7C3: LDA   $5D    ; dp=a0
      E7C5: BEQ   $E7C3   */
int defender_catch_loop_r(int offset)
{
	unsigned char t = *defender_catch;
	if (t == 0 && cpu_getpc () == 0xe7c5)
		cpu_seticount (0);
	return t;
}


/* DW 980925 - speed up very busy loop in Mayday */
/* There are two possible loops:

   E7EA: LDA   $3A   ; 96 3A       D665: LDA   $5D   ; 96 5D
   E7EC: BEQ   $E7EA ; 27 FC       D667: BEQ   $D66B ; 27 02

   Consider the E7EA loop for the moment   */

int mayday_catch_loop_r(int offset)
{
    unsigned char t = *mayday_catch;
    if (t == 0 && cpu_getpc () == 0xe7ec)
//    if (t == 0 && cpu_getpc () == 0xd667)
		cpu_seticount (0);
	return t;
}

/***************************************************************************

	Colony 7-specific routines

***************************************************************************/

/*
 *  Colony7 Select a bank
 *  There is just data in bank 0
 */
void colony7_bank_select_w (int offset,int data)
{
	static int bank[8] = { 0x0c000, 0x10000, 0x11000, 0x12000, 0x0c000, 0x0c000, 0x0c000, 0xc000 };
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* set bank address */
	cpu_setbank (1, &RAM[bank[data & 7]]);
	if (bank[data] < 0x10000)
	{
		/* i/o area map */
		cpu_setbankhandler_r (1, defender_io_r);
		cpu_setbankhandler_w (1, defender_io_w);
	}
	else
	{
		/* bank rom map */
		cpu_setbankhandler_r (1, MRA_BANK1);
		cpu_setbankhandler_w (1, MWA_ROM);
	}
}


/***************************************************************************

	Defense Command-specific routines

***************************************************************************/

/*
 *  Defense Command Select a bank
 *  There is just data in bank 0
 */
void defcomnd_bank_select_w (int offset,int data)
{
	static int bank[8] = { 0x0c000, 0x10000, 0x11000, 0x12000, 0x13000, 0x0c000, 0x0c000, 0x14000 };
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	static int bankhit[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	if ((errorlog) && (bankhit[data & 7] == 0))
		fprintf(errorlog,"bank = %02X\n",data);

	bankhit[data & 7] = 1;

	/* set bank address */
	cpu_setbank (1, &RAM[bank[data & 7]]);
	if (bank[data] < 0x10000)
	{
		/* i/o area map */
		cpu_setbankhandler_r (1, defender_io_r);
		cpu_setbankhandler_w (1, defender_io_w);
	}
	else
	{
		/* bank rom map */
		cpu_setbankhandler_r (1, MRA_BANK1);
		cpu_setbankhandler_w (1, MWA_ROM);
	}
}



/***************************************************************************

	Splat!-specific routines

***************************************************************************/

/* JB 970823 - speed up very busy loop in Splat */
/*    D04F: LDA   $4B    ; dp=98
      D051: BEQ   $D04F   */
int splat_catch_loop_r(int offset)
{
	unsigned char t = *splat_catch;
	if (t == 0 && cpu_getpc () == 0xd051)
		cpu_seticount (0);
	return t;
}


/***************************************************************************

	Sinistar-specific routines

***************************************************************************/

/*
 *  Sinistar Joystick
 */
int sinistar_input_port_0_r (int offset)
{
	int i;
	int keys;


/*~~~~~ make it incremental */
	keys = input_port_0_r (0);

	if (keys & 0x04)
		i = 0x40;
	else if (keys & 0x08)
		i = 0xC0;
	else
		i = 0x20;

	if (keys&0x02)
		i += 0x04;
	else if (keys&0x01)
		i += 0x0C;
	else
		i += 0x02;

	return i;
}


static void sinistar_snd_cmd_w (int offset, int cmd)
{
/*	if (errorlog) fprintf (errorlog, "snd command: %02x %d\n", cmd, cmd); */

	switch (cmd)
	{
		case 0x31: /* beware i live */
			sample_start (0,0,0);
			break;
		case 0x30: /* coin sound "I Hunger"*/
			sample_start (0,1,0);
			break;
		case 0x3d: /* roar */
			sample_start (0,2,0);
			break;
		case 0x32: /* i am sinistar */
			sample_start (0,4,0);
			break;
		case 0x34: /* Run Run Run (may be wrong) */
			sample_start (0,3,0);
			break;
		case 0x2c: /* Run coward (may be wrong) */
			sample_start (0,5,0);
			break;
		case 0x39: /* Run coward (may be wrong) */
			sample_start (0,7,0);
			break;
		case 0x22: /* I hunger coward (may be wrong) */
			sample_start (0,6,0);
			break;
	}

	timer_set (TIME_NOW, cmd | 0xc0, williams_snd_cmd_real_w);
}


/***************************************************************************

	Blaster-specific routines

***************************************************************************/

/*
 *  Blaster Joystick
 */
int blaster_input_port_0_r (int offset)
{
	int i;
	int keys;

	keys = input_port_0_r (0);

	if (keys & 0x04)
		i = 0x00;
	else if (keys & 0x08)
		i = 0x80;
	else
		i = 0x40;

	if (keys&0x02)
		i += 0x00;
	else if (keys&0x01)
		i += 0x08;
	else
		i += 0x04;

	return i;
}


/*
 *  Blaster bank select
 */

static int bank[16] = { 0x00000, 0x10000, 0x14000, 0x18000, 0x1c000, 0x20000, 0x24000, 0x28000,
                        0x2c000, 0x30000, 0x34000, 0x38000, 0x2c000, 0x30000, 0x34000, 0x38000 };

void blaster_vram_select_w (int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	vram_bank = data;
	if (vram_bank)
	{
		cpu_setbank (1, &RAM[bank[blaster_bank]]);
		cpu_setbank (2, blaster_bank2_base);
	}
	else
	{
		cpu_setbank (1, williams_videoram);
		cpu_setbank (2, williams_videoram + 0x4000);
	}
}


void blaster_bank_select_w (int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	blaster_bank = data & 15;
	if (vram_bank)
	{
		cpu_setbank (1, &RAM[bank[blaster_bank]]);
	}
	else
	{
		cpu_setbank (1, williams_videoram);
	}
}


#if 0 /* the fix for Blaster is more expensive than the loop */
/* JB 970823 - speed up very busy loop in Blaster */
/*    D184: LDA   $00    ; dp=97
      D186: CMPA  #$02
      D188: BCS   $D184  ; (BLO)   */
int blaster_catch_loop_r(int offset)
{
	unsigned char t = *blaster_catch;
	if (t < 2 && cpu_getpc () == 0xd186)
		cpu_seticount (0);
	return t;
}
#endif


/***************************************************************************

	Lotto Fun-specific routines

***************************************************************************/

int lottofun_input_port_0_r(int offset)
{
	return input_port_0_r(offset) | ticket_dispenser_r(offset);
}

