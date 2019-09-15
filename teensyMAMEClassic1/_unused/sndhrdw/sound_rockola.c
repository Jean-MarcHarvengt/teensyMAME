/* from Andrew Scott (ascott@utkux.utcc.utk.edu) */
#include "driver.h"

void rockola_flipscreen_w(int offset,int data);


static int NoSound0=1;
static int Sound0Offset;
static int Sound0Base;
static int Sound0Mask;
static int Sound0StopOnRollover;
static int NoSound1=1;
static int Sound1Offset;
static int Sound1Base;
static int Sound1Mask;
static int NoSound2=1;
static int Sound2Offset;
static int Sound2Base;
static int Sound2Mask;
static unsigned char LastPort1;

#define SOUND_MEMORY_REGION	3

static unsigned char waveform[32] =
{
	0x88, 0x88, 0x88, 0x88, 0xaa, 0xaa, 0xaa, 0xaa,
	0xcc, 0xcc, 0xcc, 0xcc, 0xee, 0xee, 0xee, 0xee,
	0x11, 0x11, 0x11, 0x11, 0x22, 0x22, 0x22, 0x22,
	0x44, 0x44, 0x44, 0x44, 0x66, 0x66, 0x66, 0x66
};



int rockola_sh_start(void)
{
	NoSound0=1;
	Sound0Offset=0;
	Sound0Base=0x0000;
	NoSound1=1;
	Sound1Offset=0;
	Sound1Base=0x0800;
	NoSound2=1;
	Sound2Offset=0;
	Sound2Base=0x1000;

	osd_play_sample(0, (signed char*)waveform, 32, 1000, 0, 1);
	osd_play_sample(1, (signed char*)waveform, 32, 1000, 0, 1);
	osd_play_sample(2, (signed char*)waveform, 32, 1000, 0, 1);

	return 0;
}

void rockola_sh_update(void)
{
	static int count;


	/* only update every second call (30 Hz update) */
	count++;
	if (count & 1) return;


	/* play musical tones according to tunes stored in ROM */

	if (!NoSound0)
	{
 		if (Machine->memory_region[SOUND_MEMORY_REGION][Sound0Base+Sound0Offset]!=0xff)
 			osd_adjust_sample(0, (32000 / (256-Machine->memory_region[SOUND_MEMORY_REGION][Sound0Base+Sound0Offset])) * 16, 128);
		else
			osd_adjust_sample(0, 1000, 0);
		Sound0Offset = (Sound0Offset + 1) & Sound0Mask;
		if (Sound0Offset == 0 && Sound0StopOnRollover)
			NoSound0 = 1;
	}
	else
		osd_adjust_sample(0, 1000, 0);

	if (!NoSound1)
	{
		if (Machine->memory_region[SOUND_MEMORY_REGION][Sound1Base+Sound1Offset]!=0xff)
			osd_adjust_sample(1, (32000 / (256-Machine->memory_region[SOUND_MEMORY_REGION][Sound1Base+Sound1Offset])) * 16, 128);
		else
			osd_adjust_sample(1, 1000, 0);
		Sound1Offset = (Sound1Offset + 1) & Sound1Mask;
	}
	else
		osd_adjust_sample(1, 1000, 0);

	if (!NoSound2)
	{
		if (Machine->memory_region[SOUND_MEMORY_REGION][Sound2Base+Sound2Offset]!=0xff)
			osd_adjust_sample(2, (32000 / (256-Machine->memory_region[SOUND_MEMORY_REGION][Sound2Base+Sound2Offset])) * 16, 128);
		else
			osd_adjust_sample(2, 1000, 0);
		Sound2Offset = (Sound2Offset + 1) & Sound2Mask;
	}
	else
		osd_adjust_sample(2, 1000, 0);
}



void satansat_sound0_w(int offset,int data)
{
	/* bit 0 = analog sound trigger */

	/* bit 1 = to 76477 */

	/* bit 2 = analog sound trigger */
	if (Machine->samples!=0 && Machine->samples->sample[0]!=0)
	{
		if (data & 0x04 && !(LastPort1 & 0x04))
		{
			osd_play_sample(3,Machine->samples->sample[0]->data,
			                  Machine->samples->sample[0]->length,
			                  Machine->samples->sample[0]->smpfreq,
			                  Machine->samples->sample[0]->volume,0);
		}
	}

	if (data & 0x08)
	{
		NoSound0=1;
		Sound0Offset = 0;
	}

	/* bit 4-6 sound0 volume control (TODO) */

	/* bit 7 sound1 volume control (TODO) */

	LastPort1 = data;
}

void satansat_sound1_w(int offset,int data)
{
	/* select tune in ROM based on sound command byte */
	Sound0Base = 0x0000 + ((data & 0x0e) << 7);
	Sound0Mask = 0xff;
	Sound0StopOnRollover = 1;
	Sound1Base = 0x0800 + ((data & 0x60) << 4);
	Sound1Mask = 0x1ff;

	if (data & 0x01)
		NoSound0=0;

	if (data & 0x10)
		NoSound1=0;
	else
	{
		NoSound1=1;
		Sound1Offset = 0;
	}

	/* bit 7 = ? */
}


void vanguard_sound0_w(int offset,int data)
{
	/* select musical tune in ROM based on sound command byte */

	Sound0Base = ((data & 0x07) << 8);
	Sound0Mask = 0xff;
	Sound0StopOnRollover = 0;

	/* play noise samples requested by sound command byte */
	if (Machine->samples!=0 && Machine->samples->sample[0]!=0)
	{
		if (data & 0x20 && !(LastPort1 & 0x20))
			osd_play_sample(5,Machine->samples->sample[0]->data,
			                  Machine->samples->sample[0]->length,
			                  Machine->samples->sample[0]->smpfreq,
			                  Machine->samples->sample[0]->volume,0);
		else if (!(data & 0x20) && LastPort1 & 0x20)
			osd_stop_sample(5);

		if (data & 0x40 && !(LastPort1 & 0x40))
			osd_play_sample(3,Machine->samples->sample[0]->data,
			                  Machine->samples->sample[0]->length,
			                  Machine->samples->sample[0]->smpfreq,
			                  Machine->samples->sample[0]->volume,0);
		else if (!(data & 0x20) && LastPort1 & 0x20)
			osd_stop_sample(3);

		if (data & 0x80 && !(LastPort1 & 0x80))
		{
			osd_play_sample(4,Machine->samples->sample[1]->data,
			                  Machine->samples->sample[1]->length,
			                  Machine->samples->sample[1]->smpfreq,
			                  Machine->samples->sample[1]->volume,0);
		}
	}

	if (data & 0x10)
	{
		NoSound0=0;
	}

	if (data & 0x08)
	{
		NoSound0=1;
		Sound0Offset = 0;
	}

	LastPort1 = data;
}

void vanguard_sound1_w(int offset,int data)
{
	/* select tune in ROM based on sound command byte */
	Sound1Base = 0x0800 + ((data & 0x07) << 8);
	Sound1Mask = 0xff;

	if (data & 0x08)
		NoSound1=0;
	else
	{
		NoSound1=1;
		Sound1Offset = 0;
	}
}



void fantasy_sound0_w(int offset,int data)
{
	/* select musical tune in ROM based on sound command byte */
	Sound0Base = 0x0000 + ((data & 0x07) << 8);
	Sound0Mask = 0xff;
	Sound0StopOnRollover = 0;

	/* play noise samples requested by sound command byte */
	if (Machine->samples!=0 && Machine->samples->sample[0]!=0)
	{
		if (data & 0x80 && !(LastPort1 & 0x80))
		{
			osd_play_sample(3,Machine->samples->sample[0]->data,
			                  Machine->samples->sample[0]->length,
			                  Machine->samples->sample[0]->smpfreq,
			                  Machine->samples->sample[0]->volume,0);
		}
	}

	if (data & 0x08)
		NoSound0=0;
	else
	{
		Sound0Offset = Sound0Base;
		NoSound0=1;
	}

	if (data & 0x10)
		NoSound2=0;
	else
	{
		Sound2Offset = 0;
		NoSound2=1;
	}

	LastPort1 = data;
}

void fantasy_sound1_w(int offset,int data)
{
	/* select tune in ROM based on sound command byte */
	Sound1Base = 0x0800 + ((data & 0x07) << 8);
	Sound1Mask = 0xff;

	if (data & 0x08)
		NoSound1=0;
	else
	{
		NoSound1=1;
		Sound1Offset = 0;
	}
}

void fantasy_sound2_w(int offset,int data)
{
	rockola_flipscreen_w(offset,data);

	/* select tune in ROM based on sound command byte */
	Sound2Base = 0x1000 + ((data & 0x70) << 4);
//	Sound2Base = 0x1000 + ((data & 0x10) << 5) + ((data & 0x20) << 5) + ((data & 0x40) << 2);
	Sound2Mask = 0xff;
}
