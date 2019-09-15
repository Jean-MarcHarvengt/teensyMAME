/* sound driver now does correct sounds for both small and large
   phoenixes.  force field sound still needs work.
   also all sounds need to be fine-tuned.
   music not implemented yet either.
   Andrew Scott (ascott@utkux.utcc.utk.edu) */

#include "driver.h"
#include <math.h>

/* PI isn't in the ANSI standard, so we'll define an instance here */
#define PHX_PI (3.14159265358979323846L)

#define MAX_VOLUME 20

/* A nice macro which saves us a lot of typing */
#define M_OSD_PLAY_SAMPLE(channel, soundnum, loop) { \
	if (Machine->samples->sample[soundnum] != 0) \
		osd_play_sample(channel, \
			Machine->samples->sample[soundnum]->data, \
			Machine->samples->sample[soundnum]->length, \
			Machine->samples->sample[soundnum]->smpfreq, \
			Machine->samples->sample[soundnum]->volume,loop); }


#define SAFREQ  1400
#define SBFREQ  1400
#define MAXFREQ_A 44100*7
#define MAXFREQ_B 44100*4

/* for voice A effects */
#define SW_INTERVAL 4
#define MOD_RATE 0.14
#define MOD_DEPTH 0.1

/* for voice B effect */
#define SWEEP_RATE 0.14
#define SWEEP_DEPTH 0.24

/* values needed by phoenix_sh_update */
static int sound_a_play = 0;
static int sound_a_vol = 0;
static int sound_a_freq = SAFREQ;
static int sound_a_sw = 0;
static int sound_a_adjust=1;
static int hifreq = 0;
static double t=0;
static double x;

static int sound_b_play = 0;
static int sound_b_vol = 0;
static int sound_b_freq = SBFREQ;
static int sound_b_adjust=1;

static int noise_vol = 0;
static int noise_freq = 1000;
static int pitch_a = 0;
static int pitch_b = 0;

static int song_playing;

int noisemulate = 0;



/* waveforms for the audio hardware */
static unsigned char waveform1[32] =
{
	/* sine-wave */
	0x0F, 0x0F, 0x0F, 0x06, 0x06, 0x09, 0x09, 0x06, 0x06, 0x09, 0x06, 0x0D, 0x0F, 0x0F, 0x0D, 0x00,
	0xE6, 0xDE, 0xE1, 0xE6, 0xEC, 0xE6, 0xE7, 0xE7, 0xE7, 0xEC, 0xEC, 0xEC, 0xE7, 0xE1, 0xE1, 0xE7,
};
static unsigned char waveform2[] =
{
	/* white-noise ? */
	0x79, 0x75, 0x71, 0x72, 0x72, 0x6F, 0x70, 0x71, 0x71, 0x73, 0x75, 0x76, 0x74, 0x74, 0x78, 0x7A,
	0x79, 0x7A, 0x7B, 0x7C, 0x7C, 0x7C, 0x7C, 0x7C, 0x7C, 0x7C, 0x7D, 0x80, 0x85, 0x88, 0x88, 0x87,
	0x8B, 0x8B, 0x8A, 0x8A, 0x89, 0x87, 0x85, 0x87, 0x89, 0x86, 0x83, 0x84, 0x84, 0x85, 0x84, 0x84,
	0x85, 0x86, 0x87, 0x87, 0x88, 0x88, 0x86, 0x81, 0x7E, 0x7D, 0x7F, 0x7D, 0x7C, 0x7D, 0x7D, 0x7C,
	0x7E, 0x81, 0x7F, 0x7C, 0x7E, 0x82, 0x82, 0x82, 0x82, 0x83, 0x83, 0x84, 0x83, 0x82, 0x82, 0x83,
	0x82, 0x84, 0x88, 0x8C, 0x8E, 0x8B, 0x8B, 0x8C, 0x8A, 0x8A, 0x8A, 0x89, 0x85, 0x86, 0x89, 0x89,
	0x86, 0x85, 0x85, 0x85, 0x84, 0x83, 0x82, 0x83, 0x83, 0x83, 0x82, 0x83, 0x83
};




void phoenix_sound_control_a_w(int offset,int data)
{
	static int lastnoise;

	/* voice a */
	int freq = data & 0x0f;
	int vol = (data & 0x30) >> 4;

	/* noise */
	int noise = (data & 0xc0) >> 6;

	if (freq != sound_a_freq) sound_a_adjust = 1;
	else sound_a_adjust=0;

	sound_a_freq = freq;
	sound_a_vol = vol;

	if (freq != 0x0f)
	{
		osd_adjust_sample(0,MAXFREQ_A/(16-sound_a_freq),MAX_VOLUME*(3-vol));
		sound_a_play = 1;
	}
	else
	{
		osd_adjust_sample(0,SAFREQ,0);
		sound_a_play = 0;
	}

	if (noisemulate)
	{
		if (noise_freq != 1750*(4-noise))
		{
			noise_freq = 1750*(4-noise);
			noise_vol = 85*noise;
		}

		if (noise) osd_adjust_sample(2,noise_freq,noise_vol/4);
		else
		{
			osd_adjust_sample(2,1000,0);
			noise_vol = 0;
		}
	}
	else
	{
		switch (noise)
		{
			case 1 :
				if (lastnoise != noise)
					M_OSD_PLAY_SAMPLE (2, 0, 0);
				break;
			case 2 :
				if (lastnoise != noise)
					M_OSD_PLAY_SAMPLE (2, 1, 0);
				break;
		}
		lastnoise = noise;
	}
}



void phoenix_sound_control_b_w(int offset,int data)
{
	/* voice b */
	int freq = data & 0x0f;
	int vol = (data & 0x30) >> 4;
	int tune = (data & 0xc0) >> 6;

	if (freq != sound_b_freq) sound_b_adjust = 1;
	else sound_b_adjust=0;

	sound_b_freq = freq;
	sound_b_vol = vol;

	if (freq != 0x0f)
	{
		osd_adjust_sample(1,MAXFREQ_B/(16-sound_b_freq),MAX_VOLUME*(3-vol));
		sound_b_play = 1;
	}
	else
	{
		osd_adjust_sample(1,SBFREQ,0);
		sound_b_play = 0;
	}

	/* melody */
	/* 0 - no tune, 1 - alarm beep?, 2 - even level tune, 3 - odd level tune */

	switch (tune)
	{
		case 0:
			/* This case seems to prevent the melody from repeating rather than stopping it outright */
/*			osd_stop_sample (3); */
			if (song_playing != 0)
				M_OSD_PLAY_SAMPLE (3, song_playing, 0);
			song_playing = 0;
			break;
		case 2:
			if (song_playing != 2)
			{
				song_playing = 2;
				M_OSD_PLAY_SAMPLE (3, 2, 1);
			}
			break;
		case 3:
			if (song_playing != 3)
			{
				song_playing = 3;
				M_OSD_PLAY_SAMPLE (3, 3, 1);
			}
			break;
	}
}



int phoenix_sh_start(void)
{
	x = PHX_PI/2;

	if (Machine->samples != 0 && Machine->samples->sample[0] != 0)    /* We should check also that Samplename[0] = 0 */
		noisemulate = 0;
	else
		noisemulate = 1;

	osd_play_sample(0,(signed char*)waveform1,32,1000,0,1);
	osd_play_sample(1,(signed char*)waveform1,32,1000,0,1);
	osd_play_sample(2,(signed char*)waveform2,128,1000,0,1);
	song_playing = 0;

	return 0;
}



void phoenix_sh_update(void)
{
	pitch_a=MAXFREQ_A/(16-sound_a_freq);
	pitch_b=MAXFREQ_B/(16-sound_b_freq);


	/* do special effects of voice A */
	if (hifreq)
		pitch_a=pitch_a*5/4;

	pitch_a+=((double)pitch_a*MOD_DEPTH*sin(t));

/*	if (sound_a_adjust)
		sound_a_sw=0;        */

	sound_a_sw++;

	if (sound_a_sw==SW_INTERVAL)
	{
		hifreq=!hifreq;
		sound_a_sw=0;
	}

/*	if (sound_a_adjust)
		t=0;                 */

	t+=MOD_RATE;

	if (t>2*PHX_PI)
		t=0;

	/* do special effects of voice B */
	pitch_b+=((double)pitch_b*SWEEP_DEPTH*sin(x));

	if (sound_b_adjust)
		x=0;

	x+=SWEEP_RATE;

	if (x>3*PHX_PI/2)
		x=3*PHX_PI/2;



	if (sound_a_play)
		osd_adjust_sample(0,pitch_a,MAX_VOLUME*(3-sound_a_vol));
	if (sound_b_play)
		osd_adjust_sample(1,pitch_b,MAX_VOLUME*(3-sound_b_vol));

	if ((noise_vol) && (noisemulate))
	{
		osd_adjust_sample(2,noise_freq,noise_vol/4);
		noise_vol-=3;
	}

}


