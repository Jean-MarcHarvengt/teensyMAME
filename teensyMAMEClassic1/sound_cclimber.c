#include "driver.h"


#ifdef SIGNED_SAMPLES
	#define AUDIO_CONV(a) ((a)-0x80)
#else
	#define AUDIO_CONV(a) (a)
#endif

#define SND_CLOCK 3072000	/* 3.072 Mhz */


static signed char *samples;	/* 16k for samples */
static int sample_freq,sample_volume;
static int porta;
static int channel;


void cclimber_portA_w(int offset,int data)
{
	porta = data;
}



int cclimber_sh_start(void)
{
	int i;
	unsigned char bits;


	channel = get_play_channels(1);

	samples = malloc (0x4000);
	if (!samples)
		return 1;

	/* decode the rom samples */
	for (i = 0;i < 0x2000;i++)
	{
		bits = Machine->memory_region[3][i] & 0xf0;
		samples[2 * i] = AUDIO_CONV((bits | (bits >> 4)));

		bits = Machine->memory_region[3][i] & 0x0f;
		samples[2 * i + 1] = AUDIO_CONV(((bits << 4) | bits));
	}

	return 0;
}


void cclimber_sh_stop(void)
{
	if (samples)
		free(samples);
	samples = NULL;
}


void cclimber_sample_rate_w(int offset,int data)
{
	/* calculate the sampling frequency */
	sample_freq = SND_CLOCK / 4 / (256 - data);
}



void cclimber_sample_volume_w(int offset,int data)
{
	sample_volume = data & 0x1f;	/* range 0-31 */
}



void cclimber_sample_trigger_w(int offset,int data)
{
	int start,end;


	if (data == 0 || Machine->sample_rate == 0)
		return;

	start = 64 * porta;
	end = start;

	/* find end of sample */
	while (end < 0x4000 && (samples[end] != AUDIO_CONV(0x77) || samples[end+1] != AUDIO_CONV(0x00)))
		end += 2;

	osd_play_sample(channel,samples + start,end - start,sample_freq,sample_volume*8,0);
}
