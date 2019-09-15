/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "machine/6821pia.h"
#include "m6809/m6809.h"



/* Declare inividual irq handlers, may be able to combine later */
/* Main CPU */
void spiders_irq1a(void) { cpu_cause_interrupt(0,M6809_INT_IRQ); }
void spiders_irq1b(void) { cpu_cause_interrupt(0,M6809_INT_IRQ); }
void spiders_irq2a(void) { cpu_cause_interrupt(0,M6809_INT_IRQ); }
void spiders_irq2b(void) { cpu_cause_interrupt(0,M6809_INT_IRQ); }
void spiders_irq3a(void) { cpu_cause_interrupt(0,M6809_INT_IRQ); }
void spiders_irq3b(void) { cpu_cause_interrupt(0,M6809_INT_IRQ); }

/* Sound CPU */
void spiders_irq4a(void) { }
void spiders_irq4b(void) { }

/* Function prototypes */

void spiders_flip_w(int address,int data);
void spiders_vrif_w(int address,int data);
int spiders_vrom_r(int address);


/* Declare PIA structure */

static pia6821_interface pia_intf =
{
	4,								/* 4 chips (3-Main CPU + 1-Sound CPU) */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },			/* offsets */

	{ input_port_0_r , spiders_vrom_r , 0              , 0              },	/* input port A  */
	{ 0              , input_port_5_r , 0              , 0              },	/* input bit CA1 */
	{ 0              , 0              , 0              , 0              },	/* input bit CA2 */
	{ input_port_1_r , 0              , 0              , 0              },	/* input port B  */
	{ 0              , 0              , 0              , 0              },	/* input bit CB1 */
	{ 0              , 0              , 0              , 0              },	/* input bit CB2 */
	{ 0              , 0              , 0              , 0              },	/* output port A */
	{ 0              , spiders_vrif_w , 0              , 0              },	/* output port B */
	{ 0              , 0              , 0              , 0              },	/* output CA2 */
	{ 0              , spiders_flip_w , 0              , 0              },	/* output CB2 */
	{ spiders_irq1a  , spiders_irq2a  , spiders_irq3a  , spiders_irq4a  },	/* IRQ A */
	{ spiders_irq1b  , spiders_irq2b  , spiders_irq3b  , spiders_irq4b  },	/* IRQ B */
};

/***************************************************************************

    Spiders Machine initialisation

***************************************************************************/

void spiders_init_machine(void)
{
	/* Set OPTIMIZATION FLAGS FOR M6809 */
//	m6809_Flags = M6809_FAST_NONE;

	pia_startup (&pia_intf);
}



/***************************************************************************

    Timed interrupt handler - Updated PIA input ports

***************************************************************************/

int spiders_timed_irq(void)
{
	/* Update CA1 on PIA1 - copy of PA0 (COIN1?) */
	pia_1_ca1_w(0 , input_port_0_r(0)&0x01);

	/* Update CA2 on PIA1 - copy of PA0 (PS2) */
	pia_1_ca2_w(0 , input_port_0_r(0)&0x02);

	/* Update CA1 on PIA1 - copy of PA0 (COIN1?) */
	pia_1_cb1_w(0 , input_port_6_r(0));

	/* Update CB2 on PIA1 - NOT CONNECTED */

	return ignore_interrupt();
}


/***************************************************************************

    6821 PIA handlers

***************************************************************************/

/* The middle PIA is wired up differently than the other two !!! */

static int pia_remap[4]={0,2,1,3};

void spiders_pia_2_w(int address, int data)
{
	pia_2_w(pia_remap[address],data);
}

int spiders_pia_2_r(int address)
{
	return pia_2_r(pia_remap[address]);
}


/***************************************************************************

    Video access port definition (On PIA 2)

    Bit 7 6 5 4 3 2 1 0
        X                Mode Setup/Read 1/0
            X X          Latch select (see below)
                X X X X  Data nibble

    When in setup mode data is clocked into the latch by a read from port a.
    When in read mode the read from port a auto increments the address.

    Latch 0 - Low byte low nibble
          1 - Low byte high nibble
          2 - High order low nibble
          3 - High order high nibble

***************************************************************************/

static int vrom_address;
static int vrom_ctrl_mode;
static int vrom_ctrl_latch;
static int vrom_ctrl_data;

int spiders_video_flip=0;

void spiders_flip_w(int address,int data)
{
	spiders_video_flip=data;
}

void spiders_vrif_w(int address,int data)
{
	vrom_ctrl_mode=(data&0x80)>>7;
	vrom_ctrl_latch=(data&0x30)>>4;
	vrom_ctrl_data=15-(data&0x0f);
}

int spiders_vrom_r(int address)
{
	int retval;
	unsigned char *RAM = Machine->memory_region[3];

	if(vrom_ctrl_mode)
	{
		retval=RAM[vrom_address];
//	        if (errorlog) fprintf(errorlog,"VIDEO : Read data %02x from Port address %04x\n",retval,vrom_address);
		vrom_address++;
	}
	else
	{
		switch(vrom_ctrl_latch)
		{
			case 0:
				vrom_address&=0xfff0;
				vrom_address|=vrom_ctrl_data;
				break;
			case 1:
				vrom_address&=0xff0f;
				vrom_address|=vrom_ctrl_data<<4;
				break;
			case 2:
				vrom_address&=0xf0ff;
				vrom_address|=vrom_ctrl_data<<8;
				break;
			case 3:
				vrom_address&=0x0fff;
				vrom_address|=vrom_ctrl_data<<12;
				break;
			default:
				break;
		}
		retval=0;
//	        if (errorlog) fprintf(errorlog,"VIDEO : Port address set to %04x\n",vrom_address);
	}
	return retval;
}

