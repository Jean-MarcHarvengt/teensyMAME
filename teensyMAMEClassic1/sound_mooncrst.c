#include "driver.h"
#include <math.h>


#ifdef SIGNED_SAMPLES
	#define AUDIO_CONV(A) (A-0x80)
#else
	#define AUDIO_CONV(A) (A)
#endif

#define SOUND_CLOCK (18432000/6/2) /* 1.536 Mhz */

#define NOISE_LENGTH 8000
#define NOISE_RATE 1000
#define TOOTHSAW_LENGTH 16
#define TOOTHSAW_VOLUME 36
#define LFO_VOLUME 8
#define NOISE_VOLUME 50
#define WAVE_AMPLITUDE 70
#define MAXFREQ 200
#define MINFREQ 80

#define STEP 1

static signed char *noise;

static int shootsampleloaded = 0;
static int deathsampleloaded = 0;
static int t=0;
static int LastPort1=0;
static int LastPort2=0;
static int lfo_rate=0;
static int lfo_active=0;
static int freq=MAXFREQ;
static int lforate0=0;
static int lforate1=0;
static int lforate2=0;
static int lforate3=0;

static signed char waveform1[4][TOOTHSAW_LENGTH];
static int pitch,vol;

static signed char waveform2[32] =
{
   0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
   0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
  -0x40,-0x40,-0x40,-0x40,-0x40,-0x40,-0x40,-0x40,
  -0x40,-0x40,-0x40,-0x40,-0x40,-0x40,-0x40,-0x40,
};

static int channel;



void mooncrst_pitch_w(int offset,int data)
{
	if (data != 0xff) pitch = SOUND_CLOCK*TOOTHSAW_LENGTH/(256-data)/16;
	else pitch = 0;

	if (pitch) osd_adjust_sample(channel+0,pitch,TOOTHSAW_VOLUME);
	else osd_adjust_sample(channel+0,1000,0);
}

void mooncrst_vol_w(int offset,int data)
{
	int newvol;


	/* offset 0 = bit 0, offset 1 = bit 1 */
	newvol = (vol & ~(1 << offset)) | ((data & 1) << offset);

	if (newvol != vol)
	{
		vol = newvol;
		if (pitch) osd_play_sample(channel+0,waveform1[vol],TOOTHSAW_LENGTH,pitch,TOOTHSAW_VOLUME,1);
		else osd_play_sample(channel+0,waveform1[vol],TOOTHSAW_LENGTH,1000,0,1);
	}
}



void mooncrst_noise_w(int offset,int data)
{
        if (deathsampleloaded)
        {
           if (data & 1 && !(LastPort1 & 1))
              osd_play_sample(channel+1,Machine->samples->sample[1]->data,
                           Machine->samples->sample[1]->length,
                           Machine->samples->sample[1]->smpfreq,
                           Machine->samples->sample[1]->volume,0);
           LastPort1=data;
        }
        else
        {
  	  if (data & 1) osd_adjust_sample(channel+1,NOISE_RATE,NOISE_VOLUME);
	  else osd_adjust_sample(channel+1,NOISE_RATE,0);
        }
}

void mooncrst_background_w(int offset,int data)
{
       if (data & 1)
       lfo_active=1;
       else
       lfo_active=0;
}

void mooncrst_shoot_w(int offset,int data)
{

      if (data & 1 && !(LastPort2 & 1) && shootsampleloaded)
         osd_play_sample(channel+2,Machine->samples->sample[0]->data,
                           Machine->samples->sample[0]->length,
                           Machine->samples->sample[0]->smpfreq,
                           Machine->samples->sample[0]->volume,0);
      LastPort2=data;
}


int mooncrst_sh_start(void)
{
	int i;


	channel = get_play_channels(5);

	if (Machine->samples != 0 && Machine->samples->sample[0] != 0)    /* We should check also that Samplename[0] = 0 */
	  shootsampleloaded = 1;
	else
	  shootsampleloaded = 0;

	if (Machine->samples != 0 && Machine->samples->sample[1] != 0)    /* We should check also that Samplename[0] = 0 */
	  deathsampleloaded = 1;
	else
	  deathsampleloaded = 0;

	if ((noise = malloc(NOISE_LENGTH)) == 0)
	{
		return 1;
	}

	for (i = 0;i < NOISE_LENGTH;i++)
		noise[i] = (rand() % (2*WAVE_AMPLITUDE)) - WAVE_AMPLITUDE;
	for (i = 0;i < TOOTHSAW_LENGTH;i++)
	{
		int bit0,bit2,bit3;

		bit0 = (i >> 0) & 1;
		bit2 = (i >> 2) & 1;
		bit3 = (i >> 3) & 1;

		/* relative weigths derived from the schematics. There are 4 possible */
		/* "volume" settings which also affect the pitch. */
		waveform1[0][i] = AUDIO_CONV(0x20 * bit0 + 0x30 * bit2);
		waveform1[1][i] = AUDIO_CONV(0x20 * bit0 + 0x99 * bit2);
		waveform1[2][i] = AUDIO_CONV(0x20 * bit0 + 0x30 * bit2 + 0x46 * bit3);
		waveform1[3][i] = AUDIO_CONV(0x20 * bit0 + 0x99 * bit2 + 0x46 * bit3);
	}

	pitch = 0;
	vol = 0;

	osd_play_sample(channel+0,waveform1[vol],TOOTHSAW_LENGTH,1000,0,1);

	if (!deathsampleloaded)
		osd_play_sample(channel+1,noise,NOISE_LENGTH,NOISE_RATE,0,1);

	osd_play_sample(channel+3,waveform2,32,1000,0,1);
	osd_play_sample(channel+4,waveform2,32,1000,0,1);

	return 0;
}



void mooncrst_sh_stop(void)
{
	free(noise);
	osd_stop_sample(channel+0);
	osd_stop_sample(channel+1);
	osd_stop_sample(channel+2);
	osd_stop_sample(channel+3);
	osd_stop_sample(channel+4);
}

void mooncrst_lfo_freq_w(int offset,int data)
{
        if (offset==3) lforate3=(data & 1);
        if (offset==2) lforate2=(data & 1);
        if (offset==1) lforate1=(data & 1);
        if (offset==0) lforate0=(data & 1);
        lfo_rate=lforate3*8+lforate2*4+lforate1*2+lforate0;
        lfo_rate=16-lfo_rate;
}

void mooncrst_sh_update(void)
{
    if (lfo_active)
    {
      osd_adjust_sample(channel+3,freq*32,LFO_VOLUME);
      osd_adjust_sample(channel+4,(freq+60)*32,LFO_VOLUME);
      if (t==0)
         freq-=lfo_rate;
      if (freq<=MINFREQ)
         freq=MAXFREQ;
    }
    else
    {
      osd_adjust_sample(channel+3,1000,0);
      osd_adjust_sample(channel+4,1000,0);
    }
    t++;
    if (t==3) t=0;
}
