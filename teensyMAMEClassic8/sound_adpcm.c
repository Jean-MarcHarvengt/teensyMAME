/*
 *   streaming ADPCM driver
 *   by Aaron Giles
 *
 *   Library to transcode from an ADPCM source to raw PCM.
 *   Written by Buffoni Mirko in 08/06/97
 *   References: various sources and documents.
 *
 *	 HJB 08/31/98
 *	 modified to use an automatically selected oversampling factor
 *	 for the current Machine->sample_rate
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "sndhrdw/adpcm.h"

#define OVERSAMPLING	0	/* breaks 10 Yard Fight */

/* signed/unsigned 8-bit conversion macros */
#ifdef SIGNED_SAMPLES
	#define AUDIO_CONV(A) ((A))
#else
	#define AUDIO_CONV(A) ((A)+0x80)
#endif

/* struct describing a single playing ADPCM voice */
struct ADPCMVoice
{
	int playing;            /* 1 if we are actively playing */
	int channel;            /* which channel are we playing on? */
	unsigned char *base;    /* pointer to the base memory location */
	unsigned char *stream;  /* input stream data (if any) */
	void *buffer;           /* output buffer (could be 8 or 16 bits) */
	int bufpos;             /* current position in the buffer */
	int mask;               /* mask to keep us within the buffer */
	int sample;             /* current sample number */
	int count;              /* total samples to play */
	int signal;             /* current ADPCM signal */
	int step;               /* current ADPCM step */
	int volume;             /* output volume */
};

/* global pointer to the current interface */
static struct ADPCMinterface *adpcm_intf;

/* global pointer to the current array of samples */
static struct ADPCMsample *sample_list;

/* array of ADPCM voices */
static struct ADPCMVoice adpcm[MAX_ADPCM];

/* sound channel info */
static int channel;

/* global buffer length and emulation output rate */
static int buffer_len;
static int emulation_rate;
#if OVERSAMPLING
static int oversampling;
#endif

/* step size index shift table */
static int index_shift[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

/* lookup table for the precomputed difference */
static int diff_lookup[49*16];



/*
 *   Compute the difference table
 */

static void ComputeTables (void)
{
	/* nibble to bit map */
	static int nbl2bit[16][4] =
	{
		{ 1, 0, 0, 0}, { 1, 0, 0, 1}, { 1, 0, 1, 0}, { 1, 0, 1, 1},
		{ 1, 1, 0, 0}, { 1, 1, 0, 1}, { 1, 1, 1, 0}, { 1, 1, 1, 1},
		{-1, 0, 0, 0}, {-1, 0, 0, 1}, {-1, 0, 1, 0}, {-1, 0, 1, 1},
		{-1, 1, 0, 0}, {-1, 1, 0, 1}, {-1, 1, 1, 0}, {-1, 1, 1, 1}
	};

	int step, nib;

	/* loop over all possible steps */
	for (step = 0; step <= 48; step++)
	{
		/* compute the step value */
		int stepval = floor (16.0 * pow (11.0 / 10.0, (double)step));

		/* loop over all nibbles and compute the difference */
		for (nib = 0; nib < 16; nib++)
		{
			diff_lookup[step*16 + nib] = nbl2bit[nib][0] *
				(stepval   * nbl2bit[nib][1] +
				 stepval/2 * nbl2bit[nib][2] +
				 stepval/4 * nbl2bit[nib][3] +
				 stepval/8);
		}
	}
}



/*
 *   Start emulation of several ADPCM output streams
 */

int ADPCM_sh_start (struct ADPCMinterface *intf)
{
	int i;

	/* compute the difference tables */
	ComputeTables ();

	/* copy the interface pointer to a global */
	adpcm_intf = intf;

	/* set the default sample table */
	sample_list = (struct ADPCMsample *)Machine->gamedrv->sound_prom;

	/* if there's an init function, call it now to generate the table */
	if (intf->init)
	{
		sample_list = malloc (257 * sizeof (struct ADPCMsample));
		if (!sample_list)
			return 1;
		memset (sample_list, 0, 257 * sizeof (struct ADPCMsample));
		(*intf->init) (intf, sample_list, 256);
	}

	/* reserve sound channels */
	channel = get_play_channels (intf->num);

	/* compute the emulation rate and buffer size */

#if OVERSAMPLING
	oversampling = (intf->frequency) ? Machine->sample_rate / intf->frequency + 1 : 1;
	if (errorlog) fprintf(errorlog, "adpcm: using %d times oversampling\n", oversampling);
    buffer_len = intf->frequency * oversampling / Machine->drv->frames_per_second;
#else
	buffer_len = intf->frequency / Machine->drv->frames_per_second;
#endif
    emulation_rate = buffer_len * Machine->drv->frames_per_second;

	/* initialize the voices */
	memset (adpcm, 0, sizeof (adpcm));
	for (i = 0; i < intf->num; i++)
	{
		adpcm[i].channel = channel + i;
		adpcm[i].mask = 0xffffffff;
		adpcm[i].signal = -2;
		adpcm[i].volume = intf->volume[i];

		/* allocate an output buffer */
		adpcm[i].buffer = malloc (buffer_len * Machine->sample_bits / 8);
		if (!adpcm[i].buffer)
			return 1;
	}

	/* success */
	return 0;
}



/*
 *   Stop emulation of several ADPCM output streams
 */

void ADPCM_sh_stop (void)
{
	int i;

	/* free the temporary table if we created it */
	if (sample_list && sample_list != (struct ADPCMsample *)Machine->gamedrv->sound_prom)
		free (sample_list);
	sample_list = 0;

	/* free any output and streaming buffers */
	for (i = 0; i < adpcm_intf->num; i++)
	{
		if (adpcm[i].stream)
			free (adpcm[i].stream);
		if (adpcm[i].buffer)
			free (adpcm[i].buffer);
	}
}



/*
 *   Update emulation of an ADPCM output stream
 */

static void ADPCM_update (struct ADPCMVoice *voice, int finalpos)
{
	int left = finalpos - voice->bufpos;

	/* see if there's actually any need to generate samples */
	if (left > 0)
	{
		/* if this voice is active */
		if (voice->playing)
		{
			unsigned char *base = voice->base;
			int sample = voice->sample;
			int signal = voice->signal;
			int count = voice->count;
			int step = voice->step;
			int mask = voice->mask;
			int val;
#if OVERSAMPLING
            int oldsignal = signal;
			int delta, i;
#endif

			/* 16-bit case */
			if (Machine->sample_bits == 16)
			{
				short *buffer = (short *)voice->buffer + voice->bufpos;

				while (left)
				{
					/* compute the new amplitude and update the current step */
					val = base[(sample / 2) & mask] >> (((sample & 1) << 2) ^ 4);
					signal += diff_lookup[step * 16 + (val & 15)];
					if (signal > 2047) signal = 2047;
					else if (signal < -2048) signal = -2048;
					step += index_shift[val & 7];
					if (step > 48) step = 48;
					else if (step < 0) step = 0;

#if OVERSAMPLING
                    /* antialiasing samples */
                    delta = signal - oldsignal;
					for (i = 1; left && i <= oversampling; left--, i++)
						*buffer++ = (oldsignal + delta * i / oversampling) * 16;
					oldsignal = signal;
#else
					*buffer++ = signal * 16;
					left--;
#endif

                    /* next! */
					if (++sample > count)
					{
						/* if we're not streaming, fill with silence and stop */
						if (voice->base != voice->stream)
						{
							while (left--)
								*buffer++ = 0;
							voice->playing = 0;
						}

						/* if we are streaming, pad with the last sample */
						else
						{
							short last = buffer[-1];
							while (left--)
								*buffer++ = last;
						}
						break;
					}
				}
			}

			/* 8-bit case */
			else
			{
				unsigned char *buffer = (unsigned char *)voice->buffer + voice->bufpos;

				while (left)
				{
					/* compute the new amplitude and update the current step */
					val = base[(sample / 2) & mask] >> (((sample & 1) << 2) ^ 4);
					signal += diff_lookup[step * 16 + (val & 15)];
					if (signal > 2047) signal = 2047;
					else if (signal < -2048) signal = -2048;
					step += index_shift[val & 7];
					if (step > 48) step = 48;
					else if (step < 0) step = 0;

#if OVERSAMPLING
                    delta = signal - oldsignal;
					for (i = 1; left && i <= oversampling; left--, i++)
						*buffer++ = AUDIO_CONV((oldsignal + delta * i / oversampling) / 16);
                    oldsignal = signal;
#else
					*buffer++ = AUDIO_CONV(signal / 16);
					left--;
#endif
                    /* next! */
					if (++sample > count)
					{
						/* if we're not streaming, fill with silence and stop */
						if (voice->base != voice->stream)
						{
							while (left--)
								*buffer++ = AUDIO_CONV (0);
							voice->playing = 0;
						}

						/* if we are streaming, pad with the last sample */
						else
						{
							unsigned char last = buffer[-1];
							while (left--)
								*buffer++ = last;
						}
						break;
					}
				}
			}

			/* update the parameters */
			voice->sample = sample;
			voice->signal = signal;
			voice->step = step;
		}

		/* voice is not playing */
		else
		{
			if (Machine->sample_bits == 16)
				memset ((short *)voice->buffer + voice->bufpos, 0, left * 2);
			else
				memset ((unsigned char *)voice->buffer + voice->bufpos, AUDIO_CONV (0), left);
		}

		/* update the final buffer position */
		voice->bufpos = finalpos;
	}
}



/*
 *   Update emulation of several ADPCM output streams
 */

void ADPCM_sh_update (void)
{
	int i;

	/* bail if we're not emulating sound */
	if (Machine->sample_rate == 0)
		return;

	/* loop over voices */
	for (i = 0; i < adpcm_intf->num; i++)
	{
		struct ADPCMVoice *voice = adpcm + i;

		/* update to the end of buffer */
		ADPCM_update (voice, buffer_len);

		/* play the result */
		if (Machine->sample_bits == 16)
			osd_play_streamed_sample_16 (voice->channel, voice->buffer, 2*buffer_len, emulation_rate, voice->volume,OSD_PAN_CENTER);
		else
			osd_play_streamed_sample (voice->channel, voice->buffer, buffer_len, emulation_rate, voice->volume,OSD_PAN_CENTER);

		/* reset the buffer position */
		voice->bufpos = 0;
	}
}



/*
 *   Handle a write to the ADPCM data stream
 */

void ADPCM_trigger (int num, int which)
{
	struct ADPCMVoice *voice = adpcm + num;
	struct ADPCMsample *sample;

	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return;

	/* range check the numbers */
	if (num >= adpcm_intf->num)
	{
		if (errorlog) fprintf(errorlog,"error: ADPCM_trigger() called with channel = %d, but only %d channels allocated\n", num, adpcm_intf->num);
		return;
	}

	/* find a match */
	for (sample = sample_list; sample->length > 0; sample++)
	{
		if (sample->num == which)
		{
			/* update the ADPCM voice */
			ADPCM_update (voice, cpu_scalebyfcount (buffer_len));

			/* set up the voice to play this sample */
			voice->playing = 1;
			voice->base = &Machine->memory_region[adpcm_intf->region][sample->offset];
			voice->sample = 0;
			voice->count = sample->length;

			/* also reset the ADPCM parameters */
			voice->signal = -2;
			voice->step = 0;

			return;
		}
	}

if (errorlog) fprintf(errorlog,"warning: ADPCM_trigger() called with unknown trigger = %08x\n",which);
}



void ADPCM_play (int num, int offset, int length)
{
	struct ADPCMVoice *voice = adpcm + num;


	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return;

	/* range check the numbers */
	if (num >= adpcm_intf->num)
	{
		if (errorlog) fprintf(errorlog,"error: ADPCM_trigger() called with channel = %d, but only %d channels allocated\n", num, adpcm_intf->num);
		return;
	}

	/* update the ADPCM voice */
	ADPCM_update (voice, cpu_scalebyfcount (buffer_len));

	/* set up the voice to play this sample */
	voice->playing = 1;
	voice->base = &Machine->memory_region[adpcm_intf->region][offset];
	voice->sample = 0;
	voice->count = length;

	/* also reset the ADPCM parameters */
	voice->signal = -2;
	voice->step = 0;
}



/*
 *   Stop playback on an ADPCM data channel
 */

void ADPCM_stop (int num)
{
	struct ADPCMVoice *voice = adpcm + num;

	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return;

	/* range check the numbers */
	if (num >= adpcm_intf->num)
	{
		if (errorlog) fprintf(errorlog,"error: ADPCM_stop() called with channel = %d, but only %d channels allocated\n", num, adpcm_intf->num);
		return;
	}

	/* update the ADPCM voice */
	ADPCM_update (voice, cpu_scalebyfcount (buffer_len));

	/* stop playback */
	voice->playing = 0;
}

/*
 *   Change volume on an ADPCM data channel
 */

void ADPCM_setvol (int num, int vol)
{
	struct ADPCMVoice *voice = adpcm + num;

	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return;

	/* range check the numbers */
	if (num >= adpcm_intf->num)
	{
		if (errorlog) fprintf(errorlog,"error: ADPCM_setvol() called with channel = %d, but only %d channels allocated\n", num, adpcm_intf->num);
		return;
	}

	voice->volume = vol;

	/* update the ADPCM voice */
	ADPCM_update(voice, cpu_scalebyfcount (buffer_len));
}


/*
 *   Stop playback on an ADPCM data channel
 */

int ADPCM_playing (int num)
{
	struct ADPCMVoice *voice = adpcm + num;

	/* bail if we're not playing anything */
	if (Machine->sample_rate == 0)
		return 0;

	/* range check the numbers */
	if (num >= adpcm_intf->num)
	{
		if (errorlog) fprintf(errorlog,"error: ADPCM_playing() called with channel = %d, but only %d channels allocated\n", num, adpcm_intf->num);
		return 0;
	}

	/* update the ADPCM voice */
	ADPCM_update (voice, cpu_scalebyfcount (buffer_len));

	/* return true if we're playing */
	return voice->playing;
}



/*
 *
 *	OKIM 6295 ADPCM chip:
 *
 *	Command bytes are sent:
 *
 *		1xxx xxxx = start of 2-byte command sequence, xxxxxxx is the sample number to trigger
 *		abcd ???? = second half of command; one of the abcd bits is set to indicate which voice
 *		            the ???? bits may be a volume or an extension of the xxxxxxx bits above
 *
 *		0abc d000 = stop playing; one or more of the abcd bits is set to indicate which voice(s)
 *
 *	Status is read:
 *
 *		???? abcd = one bit per voice, set to 0 if nothing is playing, or 1 if it is active
 *
 */


static struct OKIM6295interface *okim6295_interface;
static int okim6295_command[MAX_OKIM6295];



/*
 *    Start emulation of an OKIM6295-compatible chip
 */

int OKIM6295_sh_start (struct OKIM6295interface *intf)
{
	static struct ADPCMinterface generic_interface;
	int i;

	/* save a global pointer to our interface */
	okim6295_interface = intf;

	/* create an interface for the generic system here */
	generic_interface.num = 4 * intf->num;
	generic_interface.frequency = intf->frequency;
	generic_interface.region = intf->region;
	generic_interface.init = 0;
	for (i = 0; i < intf->num; i++)
		generic_interface.volume[i*4+0] =
		generic_interface.volume[i*4+1] =
		generic_interface.volume[i*4+2] =
		generic_interface.volume[i*4+3] = intf->volume[i];

	/* reset the parameters */
	for (i = 0; i < intf->num; i++)
		okim6295_command[i] = -1;

	/* initialize it in the standard fashion */
	return ADPCM_sh_start (&generic_interface);
}



/*
 *    Stop emulation of an OKIM6295-compatible chip
 */

void OKIM6295_sh_stop (void)
{
	/* standard stop */
	ADPCM_sh_stop ();
}



/*
 *    Update emulation of an OKIM6295-compatible chip
 */

void OKIM6295_sh_update (void)
{
	/* standard update */
	ADPCM_sh_update ();
}



/*
 *    Read the status port of an OKIM6295-compatible chip
 */

int OKIM6295_status_r (int num)
{
	int i, result, buffer_end;

	/* range check the numbers */
	if (num >= okim6295_interface->num)
	{
		if (errorlog) fprintf(errorlog,"error: OKIM6295_status_r() called with chip = %d, but only %d chips allocated\n", num, okim6295_interface->num);
		return 0x0f;
	}

	/* precompute the end of buffer */
	buffer_end = cpu_scalebyfcount (buffer_len);

	/* set the bit to 1 if something is playing on a given channel */
	result = 0;
	for (i = 0; i < 4; i++)
	{
		struct ADPCMVoice *voice = adpcm + num * 4 + i;

		/* update the ADPCM voice */
		ADPCM_update (voice, buffer_end);

		/* set the bit if it's playing */
		if (voice->playing)
			result |= 1 << i;
	}

	return result;
}



/*
 *    Write to the data port of an OKIM6295-compatible chip
 */

void OKIM6295_data_w (int num, int data)
{
	/* range check the numbers */
	if (num >= okim6295_interface->num)
	{
		if (errorlog) fprintf(errorlog,"error: OKIM6295_data_w() called with chip = %d, but only %d chips allocated\n", num, okim6295_interface->num);
		return;
	}

	/* if a command is pending, process the second half */
	if (okim6295_command[num] != -1)
	{
		int temp = data >> 4, i, start, stop, buffer_end;
		unsigned char *base;

		/* determine the start/stop positions */
		base = &Machine->memory_region[okim6295_interface->region][okim6295_command[num] * 8];
		start = (base[0] << 16) + (base[1] << 8) + base[2];
		stop = (base[3] << 16) + (base[4] << 8) + base[5];

		/* precompute the end of buffer */
		buffer_end = cpu_scalebyfcount (buffer_len);

		/* determine which voice(s) (voice is set by a 1 bit in the upper 4 bits of the second byte */
		for (i = 0; i < 4; i++)
		{
			if (temp & 1)
			{
				struct ADPCMVoice *voice = adpcm + num * 4 + i;

				/* update the ADPCM voice */
				ADPCM_update (voice, buffer_end);

				/* set up the voice to play this sample */
				voice->playing = 1;
				voice->base = &Machine->memory_region[okim6295_interface->region][start];
				voice->sample = 0;
				voice->count = 2 * (stop - start + 1);

				/* also reset the ADPCM parameters */
				voice->signal = -2;
				voice->step = 0;
			}
			temp >>= 1;
		}

		/* reset the command (there may be additional information in the low 4 bits (volume?) */
		okim6295_command[num] = -1;
	}

	/* if this is the start of a command, remember the sample number for next time */
	else if (data & 0x80)
	{
		okim6295_command[num] = data & 0x7f;
	}

	/* otherwise, see if this is a silence command */
	else
	{
		int temp = data >> 3, i, buffer_end;

		/* precompute the end of buffer */
		buffer_end = cpu_scalebyfcount (buffer_len);

		/* determine which voice(s) (voice is set by a 1 bit in bits 3-6 of the command */
		for (i = 0; i < 4; i++)
		{
			if (temp & 1)
			{
				struct ADPCMVoice *voice = adpcm + num * 4 + i;

				/* update the ADPCM voice */
				ADPCM_update (voice, buffer_end);

				/* stop this guy from playing */
				voice->playing = 0;
			}
			temp >>= 1;
		}
	}
}




/*
 *
 *	MSM 5205 ADPCM chip:
 *
 *	Data is streamed from a CPU by means of a clock generated on the chip.
 *
 *	A reset signal is set high or low to determine whether playback (and interrupts) are occuring
 *
 */


static struct MSM5205interface *msm5205_interface;



/*
 *    Start emulation of an MSM5205-compatible chip
 */

int MSM5205_sh_start (struct MSM5205interface *intf)
{
	static struct ADPCMinterface generic_interface;
	int i, stream_size, result;

	/* save a global pointer to our interface */
	msm5205_interface = intf;

	/* if there's an interrupt function to be called, set it up */
	if (msm5205_interface->interrupt)
		timer_pulse (TIME_IN_HZ (msm5205_interface->frequency), 0, msm5205_interface->interrupt);

	/* create an interface for the generic system here */
	generic_interface.num = intf->num;
	generic_interface.frequency = intf->frequency;
	generic_interface.region = 0;
	generic_interface.init = 0;
	for (i = 0; i < intf->num; i++)
		generic_interface.volume[i] = intf->volume[i];

	/* initialize it in the standard fashion */
	result = ADPCM_sh_start (&generic_interface);

	/* if we succeeded, create streams for everyone */
	if (!result)
	{
		/* stream size is the number of samples per second rounded to the next power of 2 */
		stream_size = 1;
		while (stream_size < msm5205_interface->frequency)
			stream_size <<= 1;

		/* allocate streams for everyone */
		for (i = 0; i < intf->num; i++)
		{
			struct ADPCMVoice *voice = adpcm + i;

			/* create the stream memory */
			voice->stream = malloc (stream_size);
			if (!voice->stream)
				return 1;

			/* set the mask value */
			voice->mask = stream_size - 1;
		}
	}

	/* return the result */
	return result;
}



/*
 *    Stop emulation of an MSM5205-compatible chip
 */

void MSM5205_sh_stop (void)
{
	/* standard stop */
	ADPCM_sh_stop ();
}



/*
 *    Update emulation of an MSM5205-compatible chip
 */

void MSM5205_sh_update (void)
{
	/* standard update */
	ADPCM_sh_update ();
}



/*
 *    Handle an update of the reset status of a chip (1 is reset ON, 0 is reset OFF)
 */

void MSM5205_reset_w (int num, int reset)
{
	struct ADPCMVoice *voice = adpcm + num;

	/* range check the numbers */
	if (num >= msm5205_interface->num)
	{
		if (errorlog) fprintf(errorlog,"error: MSM5205_reset_w() called with chip = %d, but only %d chips allocated\n", num, msm5205_interface->num);
		return;
	}

	/* see if this is turning on playback */
	if (!voice->playing && !reset)
	{
		/* update the ADPCM voice */
		ADPCM_update (voice, cpu_scalebyfcount (buffer_len));

		/* mark this voice as playing */
		voice->playing = 1;
		voice->base = voice->stream;
		voice->sample = 0;
		voice->count = 0;

		/* also reset the ADPCM parameters */
		voice->signal = -2;
		voice->step = 0;
	}

	/* see if this is turning off playback */
	else if (voice->playing && reset)
	{
		/* update the ADPCM voice */
		ADPCM_update (voice, cpu_scalebyfcount (buffer_len));

		/* mark this voice as not playing */
		voice->playing = 0;
	}
}



/*
 *    Handle an update of the data to the chip
 */

void MSM5205_data_w (int num, int data)
{
	struct ADPCMVoice *voice = adpcm + num;

	/* only do this if we're playing */
	if (voice->playing)
	{
		int count = voice->count;

		/* set the upper or lower half */
		if (!(count & 1))
			voice->stream[(count / 2) & voice->mask] = data << 4;
		else
			voice->stream[(count / 2) & voice->mask] |= data & 0x0f;

		/* update the count */
		voice->count = count + 1;
	}
}
