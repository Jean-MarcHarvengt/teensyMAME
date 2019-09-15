/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

/* These are defined in vidhrdw/exidy.c */
extern unsigned char *exidy_sprite_enable;
extern unsigned char *exidy_collision;
extern int exidy_collision_counter;

/* These are defined in sndhrdw/targ.c */
extern unsigned char targ_spec_flag;


unsigned char exidy_collision_mask = 0x00;

void fax_bank_select_w(int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	cpu_setbank (1, &RAM[0x10000 + (0x2000 * (data & 0x1F))]);
	cpu_setbankhandler_r (1, MRA_BANK1);
	cpu_setbankhandler_w (1, MWA_ROM);

	if ((data & 0x1F)>0x17)
	{
		if (errorlog)
			fprintf(errorlog,"Banking to unpopulated ROM bank %02X!\n",data & 0x1F);
	}
}

int exidy_input_port_2_r(int offset)
{
	int value;

	/* Get 2 coin inputs and VBLANK */
	value = readinputport(2);

	/* Combine with collision bits */
	value = value | ((*exidy_collision) & (exidy_collision_mask));

	/* Reset collision bits */
	*exidy_collision &= 0xEB;

	return value;
}


void exidy_init_machine(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if (strcmp(Machine->gamedrv->name,"mtrap")==0)
	{
		exidy_collision_mask = 0x14;

		/* Disable ROM Check for quicker startup */
#if 0
		RAM[0xF439]=0xEA;
		RAM[0xF43A]=0xEA;
		RAM[0xF43B]=0xEA;
#endif
	}
	else if (strcmp(Machine->gamedrv->name,"pepper2")==0)
	{
		exidy_collision_mask = 0x00;

		/* Disable ROM Check for quicker startup */
#if 0
		RAM[0xF52D]=0xEA;
		RAM[0xF52E]=0xEA;
		RAM[0xF52F]=0xEA;
#endif
	}
	else if (strcmp(Machine->gamedrv->name,"venture")==0)
	{
		exidy_collision_mask = 0x80;

		/* Disable ROM Check for quicker startup */
#if 0
		RAM[0x8AF4]=0xEA;
		RAM[0x8AF5]=0xEA;
		RAM[0x8AF6]=0xEA;
#endif
	}
	else if (strcmp(Machine->gamedrv->name,"venture2")==0)
	{
		exidy_collision_mask = 0x80;

		/* Disable ROM Check for quicker startup */
#if 0
		RAM[0x8B04]=0xEA;
		RAM[0x8B05]=0xEA;
		RAM[0x8B06]=0xEA;
#endif
	}
	else if (strcmp(Machine->gamedrv->name,"targ")==0)
	{
		/* Targ does not have a sprite enable register so we have to fake it out */
		*exidy_sprite_enable = 0x10;
		exidy_collision_mask = 0x00;
		targ_spec_flag = 1;
	}
	else if (strcmp(Machine->gamedrv->name,"spectar")==0)
	{
		/* Spectar does not have a sprite enable register so we have to fake it out */
		*exidy_sprite_enable = 0x10;
		exidy_collision_mask = 0x00;
		targ_spec_flag = 0;
	}
	else if (strcmp(Machine->gamedrv->name,"fax")==0)
	{
		exidy_collision_mask = 0x00;

		/* Initialize our ROM question bank */
		fax_bank_select_w(0,0);

		/* Disable ROM Check for quicker startup */
#if 0
		RAM[0xFBFC]=0xEA;
		RAM[0xFBFD]=0xEA;
		RAM[0xFBFE]=0xEA;

		/* Disable Question ROM Check for quicker startup */
		RAM[0xFC00]=0xEA;
		RAM[0xFC01]=0xEA;
		RAM[0xFC02]=0xEA;
#endif
	}

}

int venture_interrupt(void)
{
	static int first_time = 1;
	static int interrupt_counter = 0;

	*exidy_collision = (*exidy_collision | 0x80) & exidy_collision_mask;

	if (first_time)
	{
		first_time=0;
		return nmi_interrupt();
	}

	interrupt_counter = (interrupt_counter + 1) % 32;

	if (interrupt_counter == 0)
	{
		*exidy_collision &= 0x7F;
		return interrupt();
	}

	if (exidy_collision_counter>0)
	{
		exidy_collision_counter--;
		return interrupt();
	}

	return 0;
}

int exidy_interrupt(void)
{
	static int first_time = 1;

	*exidy_collision = (*exidy_collision | 0x80) & exidy_collision_mask;

	if (first_time)
	{
		first_time=0;
		return nmi_interrupt();
	}

	return interrupt();
}


