/***************************************************************************

  atarigen.c

  General functions for mid-to-late 80's Atari raster games.

***************************************************************************/


#include "driver.h"
#include "atarigen.h"
#include "M6502/m6502.h"


void slapstic_init (int chip);
int slapstic_bank (void);
int slapstic_tweak (int offset);



int atarigen_cpu_to_sound, atarigen_cpu_to_sound_ready;
int atarigen_sound_to_cpu, atarigen_sound_to_cpu_ready;

unsigned char *atarigen_playfieldram;
unsigned char *atarigen_spriteram;
unsigned char *atarigen_alpharam;
unsigned char *atarigen_vscroll;
unsigned char *atarigen_hscroll;
unsigned char *atarigen_eeprom;
unsigned char *atarigen_slapstic;

int atarigen_playfieldram_size;
int atarigen_spriteram_size;
int atarigen_alpharam_size;
int atarigen_eeprom_size;

static int unlocked;

static void *comm_timer;
static void *stop_comm_timer;

static struct atarigen_modesc *modesc;

static unsigned short *displaylist;
static unsigned short *displaylist_end;
static unsigned short *displaylist_last;

static void (*sound_int)(void);



/*************************************
 *
 *		Initialization of globals.
 *
 *************************************/

void atarigen_init_machine (void (*_sound_int)(void), int slapstic)
{
	unlocked = 0;
	atarigen_cpu_to_sound = atarigen_cpu_to_sound_ready = 0;
	atarigen_sound_to_cpu = atarigen_sound_to_cpu_ready = 0;

	comm_timer = stop_comm_timer = NULL;

	sound_int = _sound_int;

	if (slapstic) slapstic_init (slapstic);
}



/*************************************
 *
 *		EEPROM read/write/enable.
 *
 *************************************/

void atarigen_eeprom_enable_w (int offset, int data)
{
	unlocked = 1;
}


int atarigen_eeprom_r (int offset)
{
	return READ_WORD (&atarigen_eeprom[offset]) | 0xff00;
}


void atarigen_eeprom_w (int offset, int data)
{
	if (!unlocked)
		return;

	COMBINE_WORD_MEM (&atarigen_eeprom[offset], data);
	unlocked = 0;
}



/*************************************
 *
 *		High score save/load
 *
 *************************************/

int atarigen_hiload (void)
{
	void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 0);
	if (f)
	{
		osd_fread (f, atarigen_eeprom, atarigen_eeprom_size);
		osd_fclose (f);
	}
	else
		memset (atarigen_eeprom, 0xff, atarigen_eeprom_size);

	return 1;
}


void atarigen_hisave (void)
{
	void *f;

	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 1);
	if (f)
	{
		osd_fwrite (f, atarigen_eeprom, atarigen_eeprom_size);
		osd_fclose (f);
	}
}



/*************************************
 *
 *		Slapstic ROM read/write.
 *
 *************************************/

int atarigen_slapstic_r (int offset)
{
	int bank = slapstic_tweak (offset / 2) * 0x2000;
	return READ_WORD (&atarigen_slapstic[bank + (offset & 0x1fff)]);
}


void atarigen_slapstic_w (int offset, int data)
{
	slapstic_tweak (offset / 2);
}



/*************************************
 *
 *		Main CPU to sound CPU communications
 *
 *************************************/

static void atarigen_delayed_sound_reset (int param)
{
	cpu_reset (1);
	cpu_halt (1, 1);

	atarigen_cpu_to_sound_ready = atarigen_sound_to_cpu_ready = 0;
	atarigen_cpu_to_sound = atarigen_sound_to_cpu = 0;
}


void atarigen_sound_reset (void)
{
	timer_set (TIME_NOW, 0, atarigen_delayed_sound_reset);
}


static void atarigen_stop_comm_timer (int param)
{
	if (comm_timer)
		timer_remove (comm_timer);
	comm_timer = stop_comm_timer = NULL;
}


static void atarigen_delayed_sound_w (int param)
{
	if (atarigen_cpu_to_sound_ready)
		if (errorlog) fprintf (errorlog, "Missed command from 68010\n");

	atarigen_cpu_to_sound = param;
	atarigen_cpu_to_sound_ready = 1;
	cpu_cause_interrupt (1, M6502_INT_NMI);

	/* allocate a high frequency timer until a response is generated */
	/* the main CPU is *very* sensistive to the timing of the response */
	if (!comm_timer)
		comm_timer = timer_pulse (TIME_IN_USEC (50), 0, 0);
	if (stop_comm_timer)
		timer_remove (stop_comm_timer);
	stop_comm_timer = timer_set (TIME_IN_USEC (1000), 0, atarigen_stop_comm_timer);
}


void atarigen_sound_w (int offset, int data)
{
	/* use a timer to force a resynchronization */
	if (!(data & 0x00ff0000))
		timer_set (TIME_NOW, data & 0xff, atarigen_delayed_sound_w);
}


int atarigen_6502_sound_r (int offset)
{
	atarigen_cpu_to_sound_ready = 0;
	return atarigen_cpu_to_sound;
}



/*************************************
 *
 *		Sound CPU to main CPU communications
 *
 *************************************/

static void atarigen_delayed_6502_sound_w (int param)
{
	if (atarigen_sound_to_cpu_ready)
		if (errorlog) fprintf (errorlog, "Missed result from 6502\n");

	atarigen_sound_to_cpu = param;
	atarigen_sound_to_cpu_ready = 1;
	if (sound_int)
		(*sound_int) ();

	/* remove the high frequency timer if there is one */
	if (comm_timer)
		timer_remove (comm_timer);
	comm_timer = NULL;
}


void atarigen_6502_sound_w (int offset, int data)
{
	/* use a timer to force a resynchronization */
	timer_set (TIME_NOW, data, atarigen_delayed_6502_sound_w);
}


int atarigen_sound_r (int offset)
{
	atarigen_sound_to_cpu_ready = 0;
	return atarigen_sound_to_cpu | 0xff00;
}



/*************************************
 *
 *		MO display list management
 *
 *************************************/

int atarigen_init_display_list (struct atarigen_modesc *_modesc)
{
	modesc = _modesc;

	displaylist = malloc (modesc->maxmo * 10 * (Machine->drv->screen_height / 8));
	if (!displaylist)
		return 1;

	displaylist_end = displaylist;
	displaylist_last = NULL;

	return 0;
}


void atarigen_update_display_list (unsigned char *base, int start, int scanline)
{
	int link = start, match = 0, moskip = modesc->moskip, wordskip = modesc->mowordskip;
	int ignoreword = modesc->ignoreword;
	unsigned short *d, *startd, *lastd;
	unsigned char spritevisit[1024];

	/* scanline 0 means first update */
	if (scanline <= 0)
	{
		displaylist_end = displaylist;
		displaylist_last = NULL;
	}

	/* set up local pointers */
	startd = d = displaylist_end;
	lastd = displaylist_last;

	/* if the last list entries were on the same scanline, overwrite them */
	if (lastd)
	{
		if (*lastd == scanline)
			d = startd = lastd;
		else
			match = 1;
	}

	/* visit all the sprites and copy their data into the display list */
	memset (spritevisit, 0, sizeof (spritevisit));
	while (!spritevisit[link])
	{
		unsigned char *modata = &base[link * moskip];
		unsigned short data[4];

		/* bounds checking */
		if (d - displaylist >= modesc->maxmo * 5 * (Machine->drv->screen_height / 8))
		{
			if (errorlog) fprintf (errorlog, "Motion object list exceeded maximum\n");
			break;
		}

		/* start with the scanline */
		*d++ = scanline;

		/* add the data words */
		data[0] = *d++ = READ_WORD (&modata[0]);
		data[1] = *d++ = READ_WORD (&modata[wordskip]);
		data[2] = *d++ = READ_WORD (&modata[2 * wordskip]);
		data[3] = *d++ = READ_WORD (&modata[3 * wordskip]);

		/* is this one to ignore? */
		if (data[ignoreword] == 0xffff)
			d -= 5;

		/* update our match status */
		else if (match)
		{
			lastd++;
			if (*lastd++ != data[0] || *lastd++ != data[1] || *lastd++ != data[2] || *lastd++ != data[3])
				match = 0;
		}

		/* link to the next object */
		spritevisit[link] = 1;
		if (modesc->linkword >= 0)
			link = (data[modesc->linkword] >> modesc->linkshift) & modesc->linkmask;
		else
			link = (link + 1) & modesc->linkmask;
	}

	/* if we didn't match the last set of entries, update the counters */
	if (!match)
	{
		displaylist_end = d;
		displaylist_last = startd;
	}
}


void atarigen_render_display_list (struct osd_bitmap *bitmap, atarigen_morender morender, void *param)
{
	unsigned short *base = displaylist;
	int last_start_scan = -1;
	struct rectangle clip;

	/* create a clipping rectangle so that only partial sections are updated at a time */
	clip.min_x = 0;
	clip.max_x = Machine->drv->screen_width - 1;

	/* loop over the list until the end */
	while (base < displaylist_end)
	{
		unsigned short *d, *first, *last;
		int start_scan = base[0], step;

		last_start_scan = start_scan;
		clip.min_y = start_scan;

		/* look for an entry whose scanline start is different from ours; that's our bottom */
		for (d = base; d < displaylist_end; d += 5)
			if (*d != start_scan)
			{
				clip.max_y = *d;
				break;
			}

		/* if we didn't find any additional regions, go until the bottom of the screen */
		if (d == displaylist_end)
			clip.max_y = Machine->drv->screen_height - 1;

		/* set the start and end points */
		if (modesc->reverse)
		{
			first = d - 5;
			last = base - 5;
			step = -5;
		}
		else
		{
			first = base;
			last = d;
			step = 5;
		}

		/* update the base */
		base = d;

		/* render the mos */
		for (d = first; d != last; d += step)
			(*morender)(bitmap, &clip, &d[1], param);
	}
}
