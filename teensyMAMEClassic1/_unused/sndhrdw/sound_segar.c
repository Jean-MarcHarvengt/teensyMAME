/***************************************************************************

Sega G-80 Raster game sound code

Across these games, there's a mixture of discrete sound circuitry,
speech boards, ADPCM samples, and a TMS3617 music chip.

05-DEC-1998 - MAB:
 - completely rewrote sound code to use Samples interface.
   (It's based on the Zaxxon sndhrdw code.)

TODO:
 - Astro Blaster needs "Attack Rate" modifiers implemented
 - True ADPCM decoding for Monster Bash (NEC7715 chip)
 - Sample support for 005
 - Melody support for 005
 - Sound for Pig Newton
 - Speech for Astro Blaster

- Mike Balfour (mab22@po.cwru.edu)
***************************************************************************/

/* sound: segar.c */

#include "driver.h"

extern void TMS3617_voice_enable(int voice, int enable);
extern void TMS3617_pitch_w(int offset, int pitch);
extern void TMS3617_doupdate(void);

/* Special temporary code for Astro Blaster Speech handling */
#define MAX_SPEECH_QUEUE 10
#define NOT_PLAYING     -1              /* The queue holds the sample # so -1 will indicate no sample */
#define SPEECH_CHANNEL 11           /* Note that Astro Blaster sounds only use tracks 0-10 */

static int speechQueue[MAX_SPEECH_QUEUE];
static int speechQueuePtr = 0;
/* End of speech code */


#define TOTAL_SOUNDS 16

struct sa
{
        int channel;
        int num;
        int looped;
        int stoppable;
        int restartable;
};

const char *astrob_sample_names[] =
{
        "*astrob",
        "invadr1.sam","winvadr1.sam","invadr2.sam","winvadr2.sam",
        "invadr3.sam","winvadr3.sam","invadr4.sam","winvadr4.sam",
        "asteroid.sam","refuel.sam",

        "pbullet.sam","ebullet.sam","eexplode.sam","pexplode.sam",
        "deedle.sam","sonar.sam",

        "01.sam","02.sam","03.sam","04.sam","05.sam","06.sam","07.sam","08.sam",
        "09.sam","0a.sam","0b.sam","0c.sam","0d.sam","0e.sam","0f.sam","10.sam",
        "11.sam","12.sam","13.sam","14.sam","15.sam","16.sam","17.sam","18.sam",
        "19.sam","1a.sam","1b.sam","1c.sam","1d.sam","1e.sam","1f.sam","20.sam",
        "22.sam","23.sam",
        0
};

const char *s005_sample_names[] =
{
        0
};

const char *monsterb_sample_names[] =
{
        "zap.sam","jumpdown.sam","laughter.sam","wolfman.sam","warping.sam","tongue.sam",
        0
};

const char *spaceod_sample_names[] =
{
        "fire.sam", "bomb.sam", "eexplode.sam", "pexplode.sam",
        "warp.sam", "birth.sam", "scoreup.sam", "ssound.sam",
        "accel.sam", "damaged.sam", "erocket.sam",
        0
};

/* Astro Blaster */
enum
{
        invadr1 = 0,winvadr1,invadr2,winvadr2,
        invadr3,winvadr3,invadr4,winvadr4,
        asteroid,refuel,
        pbullet,ebullet,eexplode,pexplode,
        deedle,sonar,
        v01,v02,v03,v04,v05,v06,v07,v08,
        v09,v0a,v0b,v0c,v0d,v0e,v0f,v10,
        v11,v12,v13,v14,v15,v16,v17,v18,
        v19,v1a,v1b,v1c,v1d,v1e,v1f,v20,
        v22,v23
};

/* Monster Bash */
enum
{
        mbzap = 0,mbjumpdown,mblaughter,mbwolfman,mbwarping,mbtongue
};

/* Space Odyssey */
enum
{
        sofire = 0,sobomb,soeexplode,sopexplode,
        sowarp,sobirth,soscoreup,sossound,
        soaccel,sodamaged,soerocket
};


static struct sa astrob_sa[TOTAL_SOUNDS] =
{
        /* Port 0x3E: */
        {  0, invadr1,  1, 1, 1 },      /* Line  0 - Invader 1 */
        {  1, invadr2,  1, 1, 1 },      /* Line  1 - Invader 2 */
        {  2, invadr3,  1, 1, 1 },      /* Line  2 - Invader 3 */
        {  3, invadr4,  1, 1, 1 },      /* Line  3 - Invader 4 */
        {  4, asteroid, 1, 1, 1 },      /* Line  4 - Asteroids */
        { -1 },                                         /* Line  5 - <Mute> */
        {  5, refuel,   1, 1, 1 },      /* Line  6 - Refill */
        { -1 },                                         /* Line  7 - <Warp Modifier> */

        /* Port 0x3F: */
        {  6, pbullet,  0, 0, 1 },      /* Line  0 - Laser #1 */
        {  7, ebullet,  0, 0, 1 },      /* Line  1 - Laser #2 */
        {  8, eexplode, 0, 0, 1 },      /* Line  2 - Short Explosion */
        {  8, pexplode, 0, 0, 1 },      /* Line  3 - Long Explosion */
        { -1 },                                         /* Line  4 - <Attack Rate> */
        { -1 },                                         /* Line  5 - <Rate Reset> */
        {  9, deedle,   0, 0, 1 },      /* Line  6 - Bonus */
        { 10, sonar,    0, 0, 1 },      /* Line  7 - Sonar */
};

static struct sa spaceod_sa[TOTAL_SOUNDS] =
{
        /* Port 0x0E: */
//      {  0, sossound,   1, 1, 1 },    /* Line  0 - background noise */
        { -1 },                                                 /* Line  0 - background noise */
        { -1 },                                                 /* Line  1 - unused */
        {  1, soeexplode, 0, 0, 1 },    /* Line  2 - Short Explosion */
        { -1 },                                                 /* Line  3 - unused */
        {  2, soaccel,    0, 0, 1 },    /* Line  4 - Accelerate */
        {  3, soerocket,  0, 0, 1 },    /* Line  5 - Battle Star */
        {  4, sobomb,     0, 0, 1 },    /* Line  6 - D. Bomb */
        {  5, sopexplode, 0, 0, 1 },    /* Line  7 - Long Explosion */
        /* Port 0x0F: */
        {  6, sofire,     0, 0, 1 },    /* Line  0 - Shot */
        {  7, soscoreup,  0, 0, 1 },    /* Line  1 - Bonus Up */
        { -1 },                                                 /* Line  2 - unused */
        {  8, sowarp,     0, 0, 1 },    /* Line  3 - Warp */
        { -1 },                                                 /* Line  4 - unused */
        { -1 },                                                 /* Line  5 - unused */
        {  9, sobirth,    0, 0, 1 },    /* Line  6 - Appearance UFO */
        { 10, sodamaged,  0, 0, 1 },    /* Line  7 - Black Hole */
};


/* Special speech handling code.  Someday this will hopefully be
   replaced with true speech synthesis. */
int astrob_speech_sh_start(void)
{
   int i;

   for (i = 0; i < MAX_SPEECH_QUEUE; i++)
   {
      speechQueue[i] = NOT_PLAYING;
   }
   speechQueuePtr = 0;

   return 0;
}

void astrob_speech_sh_update (void)
{
        int sound;
        int track;

        if( Machine->samples == 0 )
                return;

        if (speechQueue[speechQueuePtr] != NOT_PLAYING)
        {
                sound = speechQueue[speechQueuePtr];

                if (!sample_playing(SPEECH_CHANNEL))
                {
                      if (Machine->samples->sample[sound])
                        sample_start(SPEECH_CHANNEL, sound, 0);
                      speechQueue[speechQueuePtr] = NOT_PLAYING;
                      speechQueuePtr = ((speechQueuePtr + 1) % MAX_SPEECH_QUEUE);
                }
   }
}


static void astrob_queue_speech(int sound)
{
    int newPtr;

    /* Queue the new sound */
    newPtr = speechQueuePtr;
    while (speechQueue[newPtr] != NOT_PLAYING)
    {
        newPtr = ((newPtr + 1) % MAX_SPEECH_QUEUE);
        if (newPtr == speechQueuePtr)
        {
             /* The queue has overflowed. Oops. */
            if (errorlog)
                fprintf (errorlog, "*** Speech queue overflow!\n");
            return;
        }
     }
    speechQueue[newPtr] = sound;
}

void astrob_speech_port_w( int offset, int val )
{
        if( Machine->samples == 0 )
                return;

        switch (val)
        {
                case 0x01:      astrob_queue_speech(v01); break;
                case 0x02:      astrob_queue_speech(v02); break;
                case 0x03:      astrob_queue_speech(v03); break;
                case 0x04:      astrob_queue_speech(v04); break;
                case 0x05:      astrob_queue_speech(v05); break;
                case 0x06:      astrob_queue_speech(v06); break;
                case 0x07:      astrob_queue_speech(v07); break;
                case 0x08:      astrob_queue_speech(v08); break;
                case 0x09:      astrob_queue_speech(v09); break;
                case 0x0a:      astrob_queue_speech(v0a); break;
                case 0x0b:      astrob_queue_speech(v0b); break;
                case 0x0c:      astrob_queue_speech(v0c); break;
                case 0x0d:      astrob_queue_speech(v0d); break;
                case 0x0e:      astrob_queue_speech(v0e); break;
                case 0x0f:      astrob_queue_speech(v0f); break;
                case 0x10:      astrob_queue_speech(v10); break;
                case 0x11:      astrob_queue_speech(v11); break;
                case 0x12:      astrob_queue_speech(v12); break;
                case 0x13:      astrob_queue_speech(v13); break;
                case 0x14:      astrob_queue_speech(v14); break;
                case 0x15:      astrob_queue_speech(v15); break;
                case 0x16:      astrob_queue_speech(v16); break;
                case 0x17:      astrob_queue_speech(v17); break;
                case 0x18:      astrob_queue_speech(v18); break;
                case 0x19:      astrob_queue_speech(v19); break;
                case 0x1a:      astrob_queue_speech(v1a); break;
                case 0x1b:      astrob_queue_speech(v1b); break;
                case 0x1c:      astrob_queue_speech(v1c); break;
                case 0x1d:      astrob_queue_speech(v1d); break;
                case 0x1e:      astrob_queue_speech(v1e); break;
                case 0x1f:      astrob_queue_speech(v1f); break;
                case 0x20:      astrob_queue_speech(v20); break;
                case 0x22:      astrob_queue_speech(v22); break;
                case 0x23:      astrob_queue_speech(v23); break;

        }

}

void astrob_audio_ports_w( int offset, int data )
{
        int line;
        int noise;
        int warp = 0;

        /* First, handle special control lines: */

        /* MUTE */
        if ((offset == 0) && (data & 0x20))
        {
                /* Note that this also stops our speech from playing. */
                /* (If our speech ever gets synthesized, this will probably
                   need to call some type of speech_mute function) */
                for (noise = 0; noise < TOTAL_SOUNDS; noise++)
                        sample_stop(astrob_sa[noise].channel);
                return;
        }

        /* WARP */
        if ((offset == 0) && (!(data & 0x80)))
        {
                warp = 1;
        }

        /* ATTACK RATE */
        if ((offset == 1) && (!(data & 0x10)))
        {
                /* TODO: this seems to modify the speed of the invader sounds */
        }

        /* RATE RESET */
        if ((offset == 1) && (!(data & 0x20)))
        {
                /* TODO: this seems to modify the speed of the invader sounds */
        }

        /* Now, play our discrete sounds */

        for (line = 0;line < 8;line++)
        {
                noise = 8 * offset + line;

                if (astrob_sa[noise].channel != -1)
                {
                        /* trigger sound */
                        if ((data & (1 << line)) == 0)
                        {
                                /* Special case: If we're on Invaders sounds, modify with warp */
                                if ((astrob_sa[noise].num >= invadr1) &&
                                        (astrob_sa[noise].num <= invadr4))
                                {
                                        if (astrob_sa[noise].restartable || !sample_playing(astrob_sa[noise].channel))
                                                sample_start(astrob_sa[noise].channel, astrob_sa[noise].num + warp, astrob_sa[noise].looped);
                                }
                                /* All other sounds are normal */
                                else
                                {
                                        if (astrob_sa[noise].restartable || !sample_playing(astrob_sa[noise].channel))
                                                sample_start(astrob_sa[noise].channel, astrob_sa[noise].num, astrob_sa[noise].looped);
                                }
                        }
                        else
                        {
                                if (sample_playing(astrob_sa[noise].channel) && astrob_sa[noise].stoppable)
                                        sample_stop(astrob_sa[noise].channel);
                        }
                }
        }
}

/* Monster Bash uses an 8255 to control the sounds, much like Zaxxon */
void monsterb_audio_8255_w( int offset, int data )
{
        static unsigned char mb_rom[] =
        { 0xC0, 0x5E, 0xE0, 0x82, 0xB2, 0x21, 0x05, 0x06,
        0xD6, 0x04, 0x6A, 0x21, 0xC1, 0xFE, 0xA0, 0x00 };

        /* Port A controls the special TMS3617 music chip */
        if (offset == 0)
        {
                int enable_val,i;

                TMS3617_doupdate();

                /* Lower four data lines get decoded into 13 control lines */
                TMS3617_pitch_w(0, ((data & 0x0F)+1) % 16);

                /* Top four data lines address an 82S123 ROM that enables/disables voices */
//              enable_val = Machine->memory_region[3][(data & 0xF0) >> 4];
                enable_val = mb_rom[(data & 0xF0) >> 4];

                for (i=2; i<8; i++)
                {
                        if (enable_val & (1<<i))
                                TMS3617_voice_enable(i-2,1);
                        else
                                TMS3617_voice_enable(i-2,0);
                }
        }
        /* Port B controls the two discrete sound circuits */
        else if (offset == 1)
        {
                if (!(data & 0x01))     sample_start(1, mbzap, 0);

                if (!(data & 0x02))     sample_start(2, mbjumpdown, 0);

                /* TODO: D7 on Port B might affect TMS3617 output (mute?) */
        }
        /* Port C controls a NEC7751, which is probably an ADPCM decoder */
        /* (TODO: switch from samples to a true ADPCM decoder) */
        else if (offset == 2)
        {
                switch (data & 0x0F)
                {
                        case 0x0F:      sample_start(3, mblaughter, 0); break;
                        case 0x0E:      sample_start(3, mbwolfman, 0); break;
                        case 0x0C:      sample_start(3, mbwarping, 0); break;
                        case 0x0D:      sample_start(3, mbtongue, 0); break;
                        default:                sample_stop(3);
                }
        }
        /* Write to 8255 control port, this should be 0x80 for "simple mode" */
        else
        {
                if ((errorlog) && (data != 0x80))
                        fprintf(errorlog,"8255 Control Port Write = %02X\n",data);
        }
}

int monsterb_audio_8255_r( int offset )
{
        /* TODO: this should be returning some type of response from the NEC chip */
        if (sample_playing(3))
                return 0x00;
        else
                return 0xFF;
}


void spaceod_audio_ports_w(int offset,int data)
{
        int line;
        int noise;

        for (line = 0;line < 8;line++)
        {
                noise = 8 * offset + line;

                if (spaceod_sa[noise].channel != -1)
                {
                        /* trigger sound */
                        if ((data & (1 << line)) == 0)
                        {
                                if (spaceod_sa[noise].restartable || !sample_playing(spaceod_sa[noise].channel))
                                        sample_start(spaceod_sa[noise].channel, spaceod_sa[noise].num, spaceod_sa[noise].looped);
                        }
                        else
                        {
                                if (sample_playing(spaceod_sa[noise].channel) && spaceod_sa[noise].stoppable)
                                        sample_stop(spaceod_sa[noise].channel);
                        }
                }
        }
}


