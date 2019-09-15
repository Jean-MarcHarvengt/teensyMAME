/* Sound hardware for Pleiades, Naughty Boy and Pop Flamer. Note that it's very */
/* similar to Phoenix. */

/* some of the missing sounds are now audible,
   but there is no modulation of any kind yet.
   Andrew Scott (ascott@utkux.utcc.utk.edu) */

#include "driver.h"

#define SAFREQ  1400
#define SBFREQ  1400
#define MAXFREQ_A 44100*7
#define MAXFREQ_B1 44100*4
#define MAXFREQ_B2 44100*4

#define MAX_VOLUME 20

/* for voice A effects */
#define SW_INTERVAL 4
#define MOD_RATE 0.14
#define MOD_DEPTH 0.1

/* for voice B effect */
#define SWEEP_RATE 0.14
#define SWEEP_DEPTH 0.24

static int sound_play[3];
static int sound_vol[3];
static int sound_freq[3];

static int noise_vol;
static int noise_freq;
static int pitch_a;
static int pitch_b1;
static int pitch_b2;

static int noisemulate;
static int portBstatus;

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



void pleiads_sound_control_a_w (int offset,int data)
{
	static int lastnoise;

	/* voice a */
	int freq = data & 0x0f;
	int vol = (data & 0x30) >> 4;

	/* noise */
	int noise = (data & 0xc0) >> 6;

	sound_freq[0] = freq;
	sound_vol[0] = vol;

	if (freq != 0x0f)
	{
		osd_adjust_sample (0, MAXFREQ_A/(16-sound_freq[0]), MAX_VOLUME*(3-vol));
		sound_play[0] = 1;
	}
	else
	{
		osd_adjust_sample (0,SAFREQ,0);
		sound_play[0] = 0;
	}

	if (noisemulate)
	{
		if (noise_freq != 1750*(4-noise))
		{
			noise_freq = 1750*(4-noise);
			noise_vol = 85*noise;
		}

		if (noise) osd_adjust_sample (3,noise_freq,noise_vol/4);
		else
		{
			osd_adjust_sample (3,1000,0);
			noise_vol = 0;
		}
	}
	else
	{
		switch (noise)
		{
			case 1 :
				if (lastnoise != noise)
					osd_play_sample(3,Machine->samples->sample[0]->data,
						Machine->samples->sample[0]->length,
						Machine->samples->sample[0]->smpfreq,
						Machine->samples->sample[0]->volume,0);
				break;
			case 2 :
		 		if (lastnoise != noise)
					osd_play_sample(3,Machine->samples->sample[1]->data,
						Machine->samples->sample[1]->length,
						Machine->samples->sample[1]->smpfreq,
						Machine->samples->sample[1]->volume,0);
				break;
		}
		lastnoise = noise;
	}
/*	if (errorlog) fprintf(errorlog,"A:%X \n",data);*/
}



void pleiads_sound_control_b_w (int offset,int data)
{
	/* voice b1 & b2 */
	int freq = data & 0x0f;
	int vol = (data & 0x30) >> 4;

	/* melody - osd_play_midi anyone? */
	/* 0 - no tune, 1 - alarm beep?, 2 - even level tune, 3 - odd level tune */
	/*int tune = (data & 0xc0) >> 6;*/
	/* LBO - not sure if the non-phoenix games play a tune. For example, */
	/* Pop Flamer & Pleiades set bit 0x40 occassionally. */

	sound_freq[portBstatus + 1] = freq;
	sound_vol[portBstatus + 1] = vol;

	if (freq < 0x0e) /* LBO clip both 0xe and 0xf to get rid of whine in Pop Flamer */
	{
		osd_adjust_sample (portBstatus + 1, MAXFREQ_B1/(16-sound_freq[portBstatus + 1]), MAX_VOLUME*(3-vol));
		sound_play[portBstatus + 1] = 1;
	}
	else
	{
		osd_adjust_sample (portBstatus + 1, SBFREQ, 0);
		sound_play[portBstatus + 1] = 0;
	}
	portBstatus ^= 0x01;
	if (errorlog) fprintf(errorlog,"B:%X freq: %02x vol: %02x\n",data, data & 0x0f, (data & 0x30) >> 4);
}



int pleiads_sh_start (void)
{
	if (Machine->samples != 0 && Machine->samples->sample[0] != 0)
		noisemulate = 0;
	else
		noisemulate = 1;

	/* Clear all the variables */
	{
		int i;
		for (i = 0; i < 3; i ++)
		{
			sound_play[i] = 0;
			sound_vol[i] = 0;
			sound_freq[i] = SAFREQ; /* The B voices use the same constant for now */
		}
		noise_vol = 0;
		noise_freq = 1000;
		pitch_a = pitch_b1 = pitch_b2 = 0;
		portBstatus = 0;
	}

	osd_play_sample (0,(signed char *)waveform1,32,1000,0,1);
	osd_play_sample (1,(signed char *)waveform1,32,1000,0,1);
	osd_play_sample (2,(signed char *)waveform1,32,1000,0,1);
	osd_play_sample (3,(signed char *)waveform2,128,1000,0,1);

	return 0;
}



void pleiads_sh_update (void)
{
	pitch_a  = MAXFREQ_A/(16-sound_freq[0]);
	pitch_b1 = MAXFREQ_B1/(16-sound_freq[1]);
	pitch_b2 = MAXFREQ_B2/(16-sound_freq[2]);
/*
	if (hifreq)
		pitch_a=pitch_a*5/4;

	pitch_a+=((double)pitch_a*MOD_DEPTH*sin(t));

	sound_a_sw++;

	if (sound_a_sw==SW_INTERVAL)
	{
		hifreq=!hifreq;
		sound_a_sw=0;
	}

	t+=MOD_RATE;

	if (t>2*PI)
		t=0;

	pitch_b+=((double)pitch_b*SWEEP_DEPTH*sin(x));

if (sound_b_vol==3 || (last_b_freq==15&&sound_b_freq==12) || (last_b_freq!=14&&sound_b_freq==6))
		x=0;

	x+=SWEEP_RATE;

	if (x>3*PI/2)
		x=3*PI/2;

  */

	if (sound_play[0])
		osd_adjust_sample (0, pitch_a, MAX_VOLUME*(3-sound_vol[0]));
	if (sound_play[1])
		osd_adjust_sample (1, pitch_b1, MAX_VOLUME*(3-sound_vol[1]));
	if (sound_play[2])
		osd_adjust_sample (2, pitch_b2, MAX_VOLUME*(3-sound_vol[2]));

	if ((noise_vol) && (noisemulate))
	{
		osd_adjust_sample (3,noise_freq,noise_vol/4);
		noise_vol-=3;
	}
}

