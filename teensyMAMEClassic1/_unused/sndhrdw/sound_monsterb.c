#include "driver.h"

struct TMS3617_interface
{
	int samplerate;	/* sample rate */
	int gain;		/* 16 * gain adjustment */
	int volume;		/* playback volume */
};

/* note: we work with signed samples here, unlike other drivers */
#ifdef SIGNED_SAMPLES
	#define AUDIO_CONV(A) (A)
#else
	#define AUDIO_CONV(A) (A+0x80)
#endif

static int emulation_rate;
static struct TMS3617_interface *interface;
static int buffer_len;
static int sample_pos;
static signed char *output_buffer;

static int channel;

#define NUM_VOICES 6

static int voice_enable[NUM_VOICES];
static int counter[NUM_VOICES];
static int TMS3617_pitch;

static signed char  *mixer_table;
static signed char  *mixer_lookup;
static signed short *mixer_buffer;


/* waveforms for the audio hardware */
static const unsigned char TMS3617_waveform[] =
{
        0x00,0x00,0x01,0x01,0x02,0x02,0x03,0x03,0x04,0x04,0x05,0x05,0x06,0x06,0x07,0x07,
        0x08,0x08,0x09,0x09,0x0a,0x0a,0x0b,0x0b,0x0c,0x0c,0x0d,0x0d,0x0e,0x0e,0x0f,0x0f,
};


#define BASE_FREQ       246.9416506
#define PITCH_1         BASE_FREQ * 1.0			/* B  */
#define PITCH_2         BASE_FREQ * 1.059463094		/* C  */
#define PITCH_3         BASE_FREQ * 1.122462048		/* C# */
#define PITCH_4         BASE_FREQ * 1.189207115		/* D  */
#define PITCH_5         BASE_FREQ * 1.259921050		/* D# */
#define PITCH_6         BASE_FREQ * 1.334839854		/* E  */
#define PITCH_7         BASE_FREQ * 1.414213562		/* F  */
#define PITCH_8         BASE_FREQ * 1.498307077		/* F# */
#define PITCH_9         BASE_FREQ * 1.587401052		/* G  */
#define PITCH_10        BASE_FREQ * 1.681792830		/* G# */
#define PITCH_11        BASE_FREQ * 1.781797436		/* A = 440 */
#define PITCH_12        BASE_FREQ * 1.887748625		/* A# */
#define PITCH_13        BASE_FREQ * 2.0			/* B  */

#define SHIFT_1 8*1.0
#define SHIFT_2 8*2.0
#define SHIFT_3 8*2.0*1.334839854
#define SHIFT_4 8*4.0
#define SHIFT_5 8*4.0*1.334839854
#define SHIFT_6 8*8.0

static int TMS3617_freq[] =
{
  0, PITCH_1*SHIFT_1, PITCH_2*SHIFT_1, PITCH_3*SHIFT_1,
  PITCH_4*SHIFT_1, PITCH_5*SHIFT_1, PITCH_6*SHIFT_1, PITCH_7*SHIFT_1,
  PITCH_8*SHIFT_1, PITCH_9*SHIFT_1, PITCH_10*SHIFT_1, PITCH_11*SHIFT_1,
  PITCH_12*SHIFT_1, PITCH_13*SHIFT_1, 0, 0,

  0, PITCH_1*SHIFT_2, PITCH_2*SHIFT_2, PITCH_3*SHIFT_2,
  PITCH_4*SHIFT_2, PITCH_5*SHIFT_2, PITCH_6*SHIFT_2, PITCH_7*SHIFT_2,
  PITCH_8*SHIFT_2, PITCH_9*SHIFT_2, PITCH_10*SHIFT_2, PITCH_11*SHIFT_2,
  PITCH_12*SHIFT_2, PITCH_13*SHIFT_2, 0, 0,

  0, PITCH_1*SHIFT_3, PITCH_2*SHIFT_3, PITCH_3*SHIFT_3,
  PITCH_4*SHIFT_3, PITCH_5*SHIFT_3, PITCH_6*SHIFT_3, PITCH_7*SHIFT_3,
  PITCH_8*SHIFT_3, PITCH_9*SHIFT_3, PITCH_10*SHIFT_3, PITCH_11*SHIFT_3,
  PITCH_12*SHIFT_3, PITCH_13*SHIFT_3, 0, 0,

  0, PITCH_1*SHIFT_4, PITCH_2*SHIFT_4, PITCH_3*SHIFT_4,
  PITCH_4*SHIFT_4, PITCH_5*SHIFT_4, PITCH_6*SHIFT_4, PITCH_7*SHIFT_4,
  PITCH_8*SHIFT_4, PITCH_9*SHIFT_4, PITCH_10*SHIFT_4, PITCH_11*SHIFT_4,
  PITCH_12*SHIFT_4, PITCH_13*SHIFT_4, 0, 0,

  0, PITCH_1*SHIFT_5, PITCH_2*SHIFT_5, PITCH_3*SHIFT_5,
  PITCH_4*SHIFT_5, PITCH_5*SHIFT_5, PITCH_6*SHIFT_5, PITCH_7*SHIFT_5,
  PITCH_8*SHIFT_5, PITCH_9*SHIFT_5, PITCH_10*SHIFT_5, PITCH_11*SHIFT_5,
  PITCH_12*SHIFT_5, PITCH_13*SHIFT_5, 0, 0,

  0, PITCH_1*SHIFT_6, PITCH_2*SHIFT_6, PITCH_3*SHIFT_6,
  PITCH_4*SHIFT_6, PITCH_5*SHIFT_6, PITCH_6*SHIFT_6, PITCH_7*SHIFT_6,
  PITCH_8*SHIFT_6, PITCH_9*SHIFT_6, PITCH_10*SHIFT_6, PITCH_11*SHIFT_6,
  PITCH_12*SHIFT_6, PITCH_13*SHIFT_6, 0, 0

};


/* note: gain is specified as gain*16 */
static int make_mixer_table (int gain)
{
	int count = NUM_VOICES * 128;
	int i;

	/* allocate memory */
	mixer_table = malloc (256 * NUM_VOICES);
	if (!mixer_table)
		return 1;

	/* find the middle of the table */
	mixer_lookup = mixer_table + (NUM_VOICES * 128);

	/* fill in the table */
	for (i = 0; i < count; i++)
	{
		int val = i * gain / (NUM_VOICES * 16);
		if (val > 127) val = 127;
		mixer_lookup[ i] = AUDIO_CONV(val);
		mixer_lookup[-i] = AUDIO_CONV(-val);
	}

	return 0;
}

static void TMS3617_update(signed char *buffer,int len)
{
	int i;
	int voice;

	signed short *mix;

	mix = mixer_buffer;
	for (i = 0; i < len; i++)
		*mix++ = 0;

	for (voice = 0; voice < NUM_VOICES; voice++)
	{
		int f;
		int v = 0x0F;

		f = TMS3617_freq[TMS3617_pitch+voice*16];

		if (v && f && voice_enable[voice])
		{
//			const unsigned char *w = wave[voice];
			const unsigned char *w = TMS3617_waveform;
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

/* This is just in case we ever need to turn the TMS3617 into a "standard" interface */
static struct TMS3617_interface monsterb_intf =
{
   44100,           /* sample rate */
   32,              /* gain adjustment */
   100              /* playback volume */
};

int TMS3617_sh_start(void)
{
	int voice;
	struct TMS3617_interface *intf = &monsterb_intf;

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

	if (make_mixer_table (intf->gain))
	{
		free (mixer_buffer);
		free (output_buffer);
		return 1;
	}

	sample_pos = 0;
	TMS3617_pitch = 0;
	interface = intf;

	for (voice = 0;voice < NUM_VOICES;voice++)
	{
		counter[voice] = 0;
		voice_enable[voice] = 1;
	}

	return 0;
}


void TMS3617_sh_stop(void)
{
	free (mixer_table);
	free (mixer_buffer);
	free (output_buffer);
}


void TMS3617_sh_update(void)
{
	if (Machine->sample_rate == 0) return;


	TMS3617_update(&output_buffer[sample_pos],buffer_len - sample_pos);
	sample_pos = 0;

	osd_play_streamed_sample(channel,output_buffer,buffer_len,emulation_rate,interface->volume,OSD_PAN_CENTER);
}

void TMS3617_voice_enable(int voice, int enable)
{
	if ((voice >=0) && (voice < NUM_VOICES))
		voice_enable[voice] = enable;
}

void TMS3617_pitch_w(int offset, int pitch)
{
	TMS3617_pitch = pitch;
}

/********************************************************************************/


void TMS3617_doupdate(void)
{
	int newpos;


	newpos = cpu_scalebyfcount(buffer_len);	/* get current position based on the timer */

	TMS3617_update(&output_buffer[sample_pos],newpos - sample_pos);
	sample_pos = newpos;
}

/********************************************************************************/

