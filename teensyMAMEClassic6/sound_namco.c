#include "driver.h"


/* note: we work with signed samples here, unlike other drivers */
#ifdef SIGNED_SAMPLES
	#define AUDIO_CONV(A) (A)
#else
	#define AUDIO_CONV(A) (A+0x80)
#endif

static int emulation_rate;
static struct namco_interface *interface;
static int buffer_len;
static int sample_pos;
static signed char *output_buffer;

static int channel;

unsigned char *pengo_soundregs;
unsigned char *mappy_soundregs;
static int sound_enable;

#define MAX_VOICES 8

static int freq[MAX_VOICES],volume[MAX_VOICES];
static const unsigned char *wave[MAX_VOICES];
static const unsigned char *sound_prom;
static int counter[MAX_VOICES];


static signed char  *mixer_table;
static signed char  *mixer_lookup;
static signed short *mixer_buffer;


/* note: gain is specified as gain*16 */
static int make_mixer_table (int voices, int gain)
{
	int count = voices * 128;
	int i;

	/* allocate memory */
	mixer_table = malloc (256 * voices);
	if (!mixer_table)
		return 1;

	/* find the middle of the table */
	mixer_lookup = mixer_table + (voices * 128);

	/* fill in the table */
	for (i = 0; i < count; i++)
	{
		int val = i * gain / (voices * 16);
		if (val > 127) val = 127;
		mixer_lookup[ i] = AUDIO_CONV(val);
		mixer_lookup[-i] = AUDIO_CONV(-val);
	}

	return 0;
}


static void namco_update(signed char *buffer,int len)
{
	int i;
	int voice;

	signed short *mix;

	/* if no sound, fill with silence */
	if (sound_enable == 0)
	{
		for (i = 0; i < len; i++)
			*buffer++ = AUDIO_CONV (0x00);
		return;
	}


	mix = mixer_buffer;
	for (i = 0; i < len; i++)
		*mix++ = 0;

	for (voice = 0; voice < interface->voices; voice++)
	{
		int f = freq[voice];
		int v = volume[voice];

		if (v && f)
		{
			const unsigned char *w = wave[voice];
			int c = counter[voice];

			mix = mixer_buffer;
			for (i = 0; i < len; i++)
			{
				c += f;
				*mix++ += ((w[(c >> 15) & 0x1f] & 0x0f) - 8) * v;
			}

			counter[voice] = c;
		}
	}

	mix = mixer_buffer;
	for (i = 0; i < len; i++)
		*buffer++ = mixer_lookup[*mix++];
}


int namco_sh_start(struct namco_interface *intf)
{
	int voice;


	buffer_len = intf->samplerate / Machine->drv->frames_per_second;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;

	channel = get_play_channels(1);

	if ((output_buffer = malloc(buffer_len)) == 0)
		return 1;

	if ((mixer_buffer = malloc(sizeof(short) * buffer_len)) == 0)
	{
		free (output_buffer);
		return 1;
	}

	if (make_mixer_table (intf->voices,intf->gain))
	{
		free (mixer_buffer);
		free (output_buffer);
		return 1;
	}

	sample_pos = 0;
	interface = intf;
	sound_enable = 1;	/* start with sound enabled, many games don't have a sound enable register */

	sound_prom = Machine->memory_region[intf->region];

	for (voice = 0;voice < intf->voices;voice++)
	{
		freq[voice] = 0;
		volume[voice] = 0;
		wave[voice] = &sound_prom[0];
		counter[voice] = 0;
	}

	return 0;
}


void namco_sh_stop(void)
{
	free (mixer_table);
	free (mixer_buffer);
	free (output_buffer);
}


void namco_sh_update(void)
{
	if (Machine->sample_rate == 0) return;


	namco_update(&output_buffer[sample_pos],buffer_len - sample_pos);
	sample_pos = 0;

	osd_play_streamed_sample(channel,output_buffer,buffer_len,emulation_rate,interface->volume,OSD_PAN_CENTER);
}


/********************************************************************************/


static void doupdate(void)
{
	int newpos;


	newpos = cpu_scalebyfcount(buffer_len);	/* get current position based on the timer */

	namco_update(&output_buffer[sample_pos],newpos - sample_pos);
	sample_pos = newpos;
}


/********************************************************************************/


void pengo_sound_enable_w(int offset,int data)
{
	sound_enable = data;
}

void pengo_sound_w(int offset,int data)
{
	int voice;


	doupdate();	/* update output buffer before changing the registers */

	pengo_soundregs[offset] = data & 0x0f;


	for (voice = 0;voice < interface->voices;voice++)
	{
		freq[voice] = pengo_soundregs[0x14 + 5 * voice];	/* always 0 */
		freq[voice] = freq[voice] * 16 + pengo_soundregs[0x13 + 5 * voice];
		freq[voice] = freq[voice] * 16 + pengo_soundregs[0x12 + 5 * voice];
		freq[voice] = freq[voice] * 16 + pengo_soundregs[0x11 + 5 * voice];
		if (voice == 0)
			freq[voice] = freq[voice] * 16 + pengo_soundregs[0x10 + 5 * voice];
		else freq[voice] = freq[voice] * 16;

		volume[voice] = pengo_soundregs[0x15 + 5 * voice];

		wave[voice] = &sound_prom[32 * (pengo_soundregs[0x05 + 5 * voice] & 7)];
	}
}


/********************************************************************************/

void mappy_sound_enable_w(int offset,int data)
{
	sound_enable = offset;
}

void mappy_sound_w(int offset,int data)
{
	int voice;


	doupdate();	/* update output buffer before changing the registers */

	mappy_soundregs[offset] = data;


	for (voice = 0;voice < interface->voices;voice++)
	{
		freq[voice] = mappy_soundregs[0x06 + 8 * voice] & 15;	/* high bits are from here */
		freq[voice] = freq[voice] * 256 + mappy_soundregs[0x05 + 8 * voice];
		freq[voice] = freq[voice] * 256 + mappy_soundregs[0x04 + 8 * voice];

		volume[voice] = mappy_soundregs[0x03 + 8 * voice];

		wave[voice] = &sound_prom[32 * ((mappy_soundregs[0x06 + 8 * voice] >> 4) & 7)];
	}
}

