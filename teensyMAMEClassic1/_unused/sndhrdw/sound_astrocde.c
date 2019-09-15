/***********************************************************

     Astrocade custom 'IO' chip sound chip driver
	 Frank Palazzolo

	 Portions copied from the Pokey emulator by Ron Fries

	 First Release:
	 	09/20/98

	 Issues:
	 	Noise generators need work
		Can do lots of speed optimizations

***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "sndhrdw/astrocde.h"
#include "z80/z80.h"

#ifdef SIGNED_SAMPLES
  #define AUDIO_CONV(A) ((A))
#else
  #define AUDIO_CONV(A) ((A)+0x80)
#endif

static struct astrocade_interface *intf;

static int emulation_rate;
static int div_by_N_factor;
static int buffer_len;

static void *astrocade_buffer[MAX_ASTROCADE_CHIPS];

static int sample_pos[MAX_ASTROCADE_CHIPS];

static int current_count_A[MAX_ASTROCADE_CHIPS];
static int current_count_B[MAX_ASTROCADE_CHIPS];
static int current_count_C[MAX_ASTROCADE_CHIPS];
static int current_count_V[MAX_ASTROCADE_CHIPS];
static int current_count_N[MAX_ASTROCADE_CHIPS];

static int current_state_A[MAX_ASTROCADE_CHIPS];
static int current_state_B[MAX_ASTROCADE_CHIPS];
static int current_state_C[MAX_ASTROCADE_CHIPS];
static int current_state_V[MAX_ASTROCADE_CHIPS];

static int current_size_A[MAX_ASTROCADE_CHIPS];
static int current_size_B[MAX_ASTROCADE_CHIPS];
static int current_size_C[MAX_ASTROCADE_CHIPS];
static int current_size_V[MAX_ASTROCADE_CHIPS];
static int current_size_N[MAX_ASTROCADE_CHIPS];

static int volume[MAX_ASTROCADE_CHIPS];
static int channel;

/* Registers */

static int master_osc[MAX_ASTROCADE_CHIPS];
static int freq_A[MAX_ASTROCADE_CHIPS];
static int freq_B[MAX_ASTROCADE_CHIPS];
static int freq_C[MAX_ASTROCADE_CHIPS];
static int vol_A[MAX_ASTROCADE_CHIPS];
static int vol_B[MAX_ASTROCADE_CHIPS];
static int vol_C[MAX_ASTROCADE_CHIPS];
static int vibrato[MAX_ASTROCADE_CHIPS];
static int vibrato_speed[MAX_ASTROCADE_CHIPS];
static int mux[MAX_ASTROCADE_CHIPS];
static int noise_am[MAX_ASTROCADE_CHIPS];
static int vol_noise4[MAX_ASTROCADE_CHIPS];
static int vol_noise8[MAX_ASTROCADE_CHIPS];

static int randbyte = 0;
static int randbit = 1;

static void astrocade_update(int num, int newpos)
{
	void *buffer = astrocade_buffer[num];

	int pos = sample_pos[num];
	int i, data, data16, noise_plus_osc, vib_plus_osc;

	for(i=pos; i<newpos; i++)
	{
		if (current_count_N[i] == 0)
		{
			randbyte = rand() & 0xff;
		}

		current_size_V[num] = 32768*vibrato_speed[num]/div_by_N_factor;

		if (!mux[num])
		{
			if (current_state_V[num] == -1)
				vib_plus_osc = (master_osc[num]-vibrato[num])&0xff;
			else
				vib_plus_osc = master_osc[num];
			current_size_A[num] = vib_plus_osc*freq_A[num]/div_by_N_factor;
			current_size_B[num] = vib_plus_osc*freq_B[num]/div_by_N_factor;
			current_size_C[num] = vib_plus_osc*freq_C[num]/div_by_N_factor;
		}
		else
		{
			noise_plus_osc = ((master_osc[num]-(vol_noise8[num]&randbyte)))&0xff;
			current_size_A[num] = noise_plus_osc*freq_A[num]/div_by_N_factor;
			current_size_B[num] = noise_plus_osc*freq_B[num]/div_by_N_factor;
			current_size_C[num] = noise_plus_osc*freq_C[num]/div_by_N_factor;
			current_size_N[num] = 2*noise_plus_osc/div_by_N_factor;
		}

		data = (current_state_A[num]*vol_A[num] +
			    current_state_B[num]*vol_B[num] +
			    current_state_C[num]*vol_C[num]);

		if (noise_am[num])
		{
			randbit = rand() & 1;
			data = data + randbit*vol_noise4[num];
		}

		if( Machine->sample_bits == 16 )
		{	/* 16bit output */
			data16 = data<<8;
			((unsigned short *)buffer)[pos++] = data16;
		}
		else
		{	/* 8bit output */
			((unsigned char *)buffer)[pos++] = AUDIO_CONV(data);
		}

		if (current_count_A[num] >= current_size_A[num])
		{
			current_state_A[num] = -current_state_A[num];
			current_count_A[num] = 0;
		}
		else
			current_count_A[num]++;

		if (current_count_B[num] >= current_size_B[num])
		{
			current_state_B[num] = -current_state_B[num];
			current_count_B[num] = 0;
		}
		else
			current_count_B[num]++;

		if (current_count_C[num] >= current_size_C[num])
		{
			current_state_C[num] = -current_state_C[num];
			current_count_C[num] = 0;
		}
		else
			current_count_C[num]++;

		if (current_count_V[num] >= current_size_V[num])
		{
			current_state_V[num] = -current_state_V[num];
			current_count_V[num] = 0;
		}
		else
			current_count_V[num]++;

		if (current_count_N[num] >= current_size_N[num])
		{
			current_count_N[num] = 0;
		}
		else
			current_count_N[num]++;
	}
	sample_pos[num]    = pos;
}

int astrocade_sh_start(struct astrocade_interface *interface)
{
	int i;

	intf = interface;

	if (Machine->sample_rate == 0)
	{
		return 0;
	}

	buffer_len = Machine->sample_rate / Machine->drv->frames_per_second;

	emulation_rate = buffer_len * Machine->drv->frames_per_second;
	div_by_N_factor = intf->baseclock/emulation_rate;

	channel = get_play_channels(intf->num);
	/* reserve buffer */
	for (i = 0;i < intf->num;i++)
	{
		if ((astrocade_buffer[i] = malloc((Machine->sample_bits/8)*buffer_len)) == 0)
		{
			while (--i >= 0) free(astrocade_buffer[i]);
			return 1;
		}
		/* reset state */
		volume[i]  = intf->volume;
		sample_pos[i] = 0;
		current_count_A[i] = 0;
		current_count_B[i] = 0;
		current_count_C[i] = 0;
		current_count_V[i] = 0;
		current_count_N[i] = 0;
		current_state_A[i] = 1;
		current_state_B[i] = 1;
		current_state_C[i] = 1;
		current_state_V[i] = 1;
	}

	return 0;
}

void astrocade_sh_stop(void)
{
	int i;

	for (i = 0;i < intf->num;i++){
		free(astrocade_buffer[i]);
	}
}

void astrocade_sound_w(int num, int offset, int data)
{
	Z80_Regs regs;
	int i, bvalue, temp_vib;

	/* update */
	astrocade_update(num,cpu_scalebyfcount(buffer_len));

	switch(offset)
	{
		case 0:  /* Master Oscillator */
#ifdef MAME_DEBUG
			if (errorlog) fprintf(errorlog,"Master Osc Write: %02x\n",data);
#endif
			master_osc[num] = data+1;
		break;

		case 1:  /* Tone A Frequency */
#ifdef MAME_DEBUG
			if (errorlog) fprintf(errorlog,"Tone A Write:        %02x\n",data);
#endif
			freq_A[num] = data+1;
		break;

		case 2:  /* Tone B Frequency */
#ifdef MAME_DEBUG
			if (errorlog) fprintf(errorlog,"Tone B Write:           %02x\n",data);
#endif
			freq_B[num] = data+1;
		break;

		case 3:  /* Tone C Frequency */
#ifdef MAME_DEBUG
			if (errorlog) fprintf(errorlog,"Tone C Write:              %02x\n",data);
#endif
			freq_C[num] = data+1;
		break;

		case 4:  /* Vibrato Register */
#ifdef MAME_DEBUG
			if (errorlog)
			{
				fprintf(errorlog,"Vibrato Depth:                %02x\n",data&0x3f);
				fprintf(errorlog,"Vibrato Speed:                %02x\n",data>>6);
			}
#endif
			vibrato[num] = data & 0x3f;

			temp_vib = (data>>6) & 0x03;
			vibrato_speed[num] = 1;
			for(i=0;i<temp_vib;i++)
				vibrato_speed[num] <<= 1;

		break;

		case 5:  /* Tone C Volume, Noise Modulation Control */
			vol_C[num] = data & 0x0f;
			mux[num] = (data>>4) & 0x01;
			noise_am[num] = (data>>5) & 0x01;
#ifdef MAME_DEBUG
			if (errorlog)
			{
				fprintf(errorlog,"Tone C Vol:                      %02x\n",vol_C[num]);
				fprintf(errorlog,"Mux Source:                      %02x\n",mux[num]);
				fprintf(errorlog,"Noise Am:                        %02x\n",noise_am[num]);
			}
#endif
		break;

		case 6:  /* Tone A & B Volume */
			vol_B[num] = (data>>4) & 0x0f;
			vol_A[num] = data & 0x0f;
#ifdef MAME_DEBUG
			if (errorlog)
			{
				fprintf(errorlog,"Tone A Vol:                         %02x\n",vol_A[num]);
				fprintf(errorlog,"Tone B Vol:                         %02x\n",vol_B[num]);
			}
#endif
		break;

		case 7:  /* Noise Volume Register */
			vol_noise8[num] = data;
			vol_noise4[num] = (data>>4) & 0x0f;
#ifdef MAME_DEBUG
			if (errorlog)
			{
				fprintf(errorlog,"Noise Vol:                             %02x\n",vol_noise8[num]);
				fprintf(errorlog,"Noise Vol (4):                         %02x\n",vol_noise4[num]);
			}
#endif
		break;

		case 8:  /* Sound Block Transfer */

			Z80_GetRegs(&regs);
			bvalue = regs.BC.B.h & 0x0F;

			astrocade_sound_w(num, bvalue-1, data);

		break;
	}
}

void astrocade_sound1_w(int offset, int data)
{
	astrocade_sound_w(0, offset, data);
}

void astrocade_sound2_w(int offset, int data)
{
	astrocade_sound_w(1, offset, data);
}

/* #define DUMPFILE 1 */

#if DUMPFILE
FILE *ofp = 0;
#endif

void astrocade_sh_update(void)
{
	int num;
	int i;

	if (Machine->sample_rate == 0 ) return;

#if DUMPFILE
	if (!ofp)
		ofp = fopen("astrocde.pcm","wb");
#endif

	for (num = 0;num < intf->num;num++)
	{
		astrocade_update(num, buffer_len);
		/* reset position , step , count */
		sample_pos[num] = 0;
		/* play sound */
		if( Machine->sample_bits == 16 )
			osd_play_streamed_sample_16(channel+num,astrocade_buffer[num],2*buffer_len,emulation_rate,volume[num],OSD_PAN_CENTER);
		else
		{
			osd_play_streamed_sample(channel+num,astrocade_buffer[num],buffer_len,emulation_rate,volume[num],OSD_PAN_CENTER);
		}
	}

#if DUMPFILE
	/* Note: this assumes 8-bit sound */
	for(i=0;i<buffer_len;i++)
		fprintf(ofp,"%c",(((unsigned char *)astrocade_buffer[0])[i]) +
					     (((unsigned char *)astrocade_buffer[1])[i]) );
#endif
}
