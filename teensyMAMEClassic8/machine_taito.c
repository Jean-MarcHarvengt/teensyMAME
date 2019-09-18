/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "m6805.h"
#include "vidhrdw/generic.h"


static unsigned char fromz80,toz80;
static int zaccept,zready;

unsigned char *alpine1_protection;
unsigned char *alpine2_protection;


void taito_init_machine(void)
{
	int i,j;
	int weight[8],totweight;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* set the default ROM bank (many games only have one bank and */
	/* never write to the bank selector register) */
	cpu_setbank(1,&RAM[0x6000]);


	zaccept = 1;
	zready = 0;
}



void alpine1_protection_w(int offset, int data)
{
	switch (data)
	{
		case 0x05:
			*alpine1_protection = 0x18;
			break;
		case 0x07:
		case 0x0c:
		case 0x0f:
			*alpine1_protection = 0x00;		/* not used as far as I can tell */
			break;
		case 0x16:
			*alpine1_protection = 0x08;
			break;
		case 0x1d:
			*alpine1_protection = 0x18;
			break;
		default:
			*alpine1_protection = data;		/* not used as far as I can tell */
			break;
	}
}


void taito_bankswitch_w(int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	extern struct GameDriver alpinea_driver;


	if (data & 0x80) { cpu_setbank(1,&RAM[0x10000]); }
	else cpu_setbank(1,&RAM[0x6000]);

	if (Machine->gamedrv == &alpinea_driver)
		*alpine2_protection = data >> 2;
}



int taito_port_2_r(int offset)
{
	extern struct GameDriver alpine_driver;


	if (Machine->gamedrv == &alpine_driver)
		return input_port_2_r(offset) | *alpine1_protection;
	else
		return input_port_2_r(offset) | *alpine2_protection;
}



/***************************************************************************

                           PROTECTION HANDLING

 Some of the games running on this hardware are protected with a 68705 mcu.
 It can either be on a daughter board containing Z80+68705+one ROM, which
 replaces the Z80 on an unprotected main board; or it can be built-in on the
 main board. The two are fucntionally equivalent.

 The 68705 can read commands from the Z80, send back result codes, and has
 direct access to the Z80 memory space. It can also trigger IRQs on the Z80.

***************************************************************************/
int taito_fake_data_r(int offset)
{
if (errorlog) fprintf(errorlog,"%04x: protection read\n",cpu_getpc());
	return 0;
}

void taito_fake_data_w(int offset,int data)
{
if (errorlog) fprintf(errorlog,"%04x: protection write %02x\n",cpu_getpc(),data);
}

int taito_fake_status_r(int offset)
{
if (errorlog) fprintf(errorlog,"%04x: protection status read\n",cpu_getpc());
	return 0xff;
}


int taito_mcu_data_r(int offset)
{
if (errorlog) fprintf(errorlog,"%04x: protection read %02x\n",cpu_getpc(),toz80);

	zaccept = 1;
	return toz80;
}

void taito_mcu_data_w(int offset,int data)
{
if (errorlog) fprintf(errorlog,"%04x: protection write %02x\n",cpu_getpc(),data);

	if (zready == 0) cpu_cause_interrupt(2,M6805_INT_IRQ);
	zready = 1;
	fromz80 = data;
}

int taito_mcu_status_r(int offset)
{
	/* bit 0 = the 68705 has read data from the Z80 */
	/* bit 1 = the 68705 has written data for the Z80 */
	return ~((zready << 0) | (zaccept << 1));
}


static unsigned char portA_in,portA_out;

int taito_68705_portA_r(int offset)
{
//if (errorlog) fprintf(errorlog,"%04x: 68705 port A read %02x\n",cpu_getpc(),portA_in);
	return portA_in;
}

void taito_68705_portA_w(int offset,int data)
{
//if (errorlog) fprintf(errorlog,"%04x: 68705 port A write %02x\n",cpu_getpc(),data);
	portA_out = data;
}



/*
 *  Port B connections:
 *
 *  all bits are logical 1 when read (+5V pullup)
 *
 *  0   W  !68INTRQ
 *  1   W  !68LRD (enables latch which holds command from the Z80)
 *  2   W  !68LWR (loads the latch which holds data for the Z80, and sets a
 *                 status bit so the Z80 knows there's data waiting)
 *  3   W  to Z80 !BUSRQ (aka !WAIT) pin
 *  4   W  !68WRITE (triggers write to main Z80 memory area and increases low
 *                   8 bits of the latched address)
 *  5   W  !68READ (triggers read from main Z80 memory area and increases low
 *                   8 bits of the latched address)
 *  6   W  !LAL (loads the latch which holds the low 8 bits of the address of
 *               the main Z80 memory location to access)
 *  7   W  !UAL (loads the latch which holds the high 8 bits of the address of
 *               the main Z80 memory location to access)
 */

int taito_68705_portB_r(int offset)
{
	return 0xff;
}

static int address;

void taito_68705_portB_w(int offset,int data)
{
//if (errorlog) fprintf(errorlog,"%04x: 68705 port B write %02x\n",cpu_getpc(),data);

	if (~data & 0x01)
	{
if (errorlog) fprintf(errorlog,"%04x: 68705  68INTRQ **NOT SUPPORTED**!\n",cpu_getpc());
	}
	if (~data & 0x02)
	{
		zready = 0;	/* 68705 is going to read data from the Z80 */
		portA_in = fromz80;
if (errorlog) fprintf(errorlog,"%04x: 68705 <- Z80 %02x\n",cpu_getpc(),portA_in);
	}
	if (~data & 0x04)
	{
if (errorlog) fprintf(errorlog,"%04x: 68705 -> Z80 %02x\n",cpu_getpc(),portA_out);
		zaccept = 0;	/* 68705 is writing data for the Z80 */
		toz80 = portA_out;
	}
	if (~data & 0x10)
	{
if (errorlog) fprintf(errorlog,"%04x: 68705 write %02x to address %04x\n",cpu_getpc(),portA_out,address);
		if (address >= 0xc400 && address <= 0xc7ff)
			/* trap writes to video memory */
			videoram_w(address - 0xc400,portA_out);
		else
			Machine->memory_region[0][address] = portA_out;

		/* increase low 8 bits of latched address for burst writes */
		address = (address & 0xff00) | ((address + 1) & 0xff);
	}
	if (~data & 0x20)
	{
		if (address == 0xd408)
			portA_in = input_port_0_r(0);
		else if (address == 0xd409)
			portA_in = input_port_1_r(0);
		else if (address == 0xd40c)
			portA_in = input_port_3_r(0);
		else if (address == 0xd40d)
			portA_in = input_port_4_r(0);
		else
			portA_in = Machine->memory_region[0][address];
if (errorlog) fprintf(errorlog,"%04x: 68705 read %02x from address %04x\n",cpu_getpc(),portA_in,address);
	}
	if (~data & 0x40)
	{
if (errorlog) fprintf(errorlog,"%04x: 68705 address low %02x\n",cpu_getpc(),portA_out);
		address = (address & 0xff00) | portA_out;
	}
	if (~data & 0x80)
	{
if (errorlog) fprintf(errorlog,"%04x: 68705 address high %02x\n",cpu_getpc(),portA_out);
		address = (address & 0x00ff) | (portA_out << 8);
	}
}

/*
 *  Port C connections:
 *
 *  0   R  ZREADY (1 when the Z80 has written a command in the latch)
 *  1   R  ZACCEPT (1 when the Z80 has read data from the latch)
 *  2   R  from Z80 !BUSAK pin
 *  3   R  68INTAK (goes 0 when the interrupt request done with 68INTRQ
 *                  passes through)
 */

int taito_68705_portC_r(int offset)
{
	int res;

	res = (zready << 0) | (zaccept << 1);
if (errorlog) fprintf(errorlog,"%04x: 68705 port C read %02x\n",cpu_getpc(),res);
	return res;
}

int taito_68705_ih(void)
{
	return !zready;
}
