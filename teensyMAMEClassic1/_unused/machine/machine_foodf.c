/*************************************************************************

  Food Fight machine hardware

*************************************************************************/

#include <stdlib.h>
#include "driver.h"




/*
 *		Statics
 */

static int whichport = 0;


/*
 *		Interrupt handlers.
 */

void foodf_delayed_interrupt (int param)
{
	cpu_cause_interrupt (0, 2);
}

int foodf_interrupt (void)
{
	/* INT 2 once per frame in addition to... */
	if (cpu_getiloops () == 0)
		timer_set (TIME_IN_USEC (100), 0, foodf_delayed_interrupt);

	/* INT 1 on the 32V signal */
	return 1;
}


/*
 *		NVRAM read/write.
 *      also used by Quantum
 */

static unsigned char nvram[128];

int foodf_nvram_r (int offset)
{
	return ((nvram[(offset / 4) ^ 0x03] >> 2*(offset % 4))) & 0x0f;
}


void foodf_nvram_w (int offset, int data)
{
	nvram[(offset / 4) ^ 0x03] &= ~(0x0f << 2*(offset % 4));
	nvram[(offset / 4) ^ 0x03] |= (data & 0x0f) << 2*(offset % 4);
}

int foodf_nvram_load(void)
{
	void *f;


	/* Try loading static RAM */
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,nvram,128);
		osd_fclose(f);
	}
	/* Invalidate the static RAM to force reset to factory settings */
	else memset(nvram,0xff,128);

	return 1;
}

void foodf_nvram_save(void)
{
	void *f;


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,nvram,128);
		osd_fclose(f);
	}
}


/*
 *		Analog controller read dispatch.
 */

int foodf_analog_r (int offset)
{
	switch (offset)
	{
		case 0:
		case 2:
		case 4:
		case 6:
			return readinputport (whichport);
	}
	return 0;
}


/*
 *		Digital controller read dispatch.
 */

int foodf_digital_r (int offset)
{
	switch (offset)
	{
		case 0:
			return input_port_4_r (offset);
	}
	return 0;
}


/*
 *		Analog write dispatch.
 */

void foodf_analog_w (int offset, int data)
{
	whichport = 3 - ((offset/2) & 3);
}


/*
 *		Digital write dispatch.
 */

void foodf_digital_w (int offset, int data)
{
}
