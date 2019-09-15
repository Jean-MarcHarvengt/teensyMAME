/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

extern void cps1_dump_video(void);

int cps1_ram_size;
unsigned char *cps1_ram;
unsigned char * cps1_eeprom;
int cps1_eeprom_size;


#ifdef MAME_DEBUG
void cps1_dump_driver(void)
{
	FILE *fp=fopen("RAM.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_ram, cps1_ram_size, 1, fp);
		fclose(fp);
	}
	fp=fopen("ROM.DMP", "w+b");
	if (fp)
	{
		int i;
		unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
		unsigned char *p=RAM;
		for (i=0; i<0x80000; i++)
		{
			fwrite(p+1, 1, 1, fp);
			fwrite(p, 1, 1, fp);
			p+=2;
		}
		fclose(fp);
	}
	fp=fopen("EEPROM.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_eeprom, cps1_eeprom_size, 1, fp);
		fclose(fp);
	}
}

#define DEBUG_DUMP_DRV() \
{                         \
	if (osd_key_pressed(OSD_KEY_F))\
	{                              \
	     cps1_dump_video();        \
	     cps1_dump_driver();       \
	}                              \
}
#else
#define DEBUG_DUMP_DRV()
#endif

int cps1_input_r(int offset)
{
       int control=readinputport (offset/2);
       return (control<<8) | control;
}

int cps1_player_input_r(int offset)
{
       return (readinputport (offset + 4 )+
		(readinputport (offset+1 + 4 )<<8));
}

int cps1_interrupt(void)
{
	DEBUG_DUMP_DRV();
	return 6;
}


int cps1_interrupt2(void)
{
	if (cpu_getiloops() == 0)
	{
		DEBUG_DUMP_DRV();
		return 2;
	}
	else return 4;
}


int cps1_interrupt3(void)
{
       DEBUG_DUMP_DRV();
       return 2;
}


