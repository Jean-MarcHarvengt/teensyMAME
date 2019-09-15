/*****************************************************************************/
/*                                                                           */
/* Module:	POKEY Chip Simulator Includes, V3.1 							 */
/* Purpose: To emulate the sound generation hardware of the Atari POKEY chip.*/
/* Author:  Ron Fries                                                        */
/* Date:    August 8, 1997                                                   */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*                 License Information and Copyright Notice                  */
/*                 ========================================                  */
/*                                                                           */
/* PokeySound is Copyright(c) 1997 by Ron Fries                              */
/*                                                                           */
/* This library is free software; you can redistribute it and/or modify it   */
/* under the terms of version 2 of the GNU Library General Public License    */
/* as published by the Free Software Foundation.                             */
/*                                                                           */
/* This library is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library */
/* General Public License for more details.                                  */
/* To obtain a copy of the GNU Library General Public License, write to the  */
/* Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.   */
/*                                                                           */
/* Any permitted reproduction of these routines, in whole or in part, must   */
/* bear this legend.                                                         */
/*                                                                           */
/*****************************************************************************/

#ifndef _POKEYSOUND_H
#define _POKEYSOUND_H

#ifndef _TYPEDEF_H
#define _TYPEDEF_H

/* switched to the global declarations of INT8...UINT32 */
#include "osd_cpu.h"

#ifdef __MWERKS__
#define BIG_ENDIAN
#endif

#endif	/* _TYPEDEF_H */

/* CONSTANT DEFINITIONS */

/* POKEY WRITE LOGICALS */
/* Note: only 0x00 - 0x09, part of 0x0f are emulated by POKEYSND */
#define AUDF1_C     0x00
#define AUDC1_C     0x01
#define AUDF2_C     0x02
#define AUDC2_C     0x03
#define AUDF3_C     0x04
#define AUDC3_C     0x05
#define AUDF4_C     0x06
#define AUDC4_C     0x07
#define AUDCTL_C    0x08
#define STIMER_C    0x09
#define SKREST_C    0x0A
#define POTGO_C     0x0B
#define SEROUT_C    0x0D
#define IRQEN_C     0x0E
#define SKCTL_C     0x0F

/* POKEY READ LOGICALS */
#define POT0_C      0x00
#define POT1_C      0x01
#define POT2_C      0x02
#define POT3_C      0x03
#define POT4_C      0x04
#define POT5_C      0x05
#define POT6_C      0x06
#define POT7_C      0x07
#define ALLPOT_C    0x08
#define KBCODE_C    0x09
#define RANDOM_C    0x0A
#define SERIN_C     0x0D
#define IRQST_C     0x0E
#define SKSTAT_C    0x0F


/* As an alternative to using the exact frequencies, selecting a playback
   frequency that is an exact division of the main clock provides a higher
   quality output due to less aliasing.  For best results, a value of
   1787520 MHz is used for the main clock.  With this value, both the
   64 kHz and 15 kHz clocks are evenly divisible.  Selecting a playback
   frequency that is also a division of the clock provides the best
   results.  The best options are FREQ_64 divided by either 2, 3, or 4.
   The best selection is based on a trade off between performance and
   sound quality.

   Of course, using a main clock frequency that is not exact will affect
   the pitch of the output.  With these numbers, the pitch will be low
   by 0.127%.  (More than likely, an actual unit will vary by this much!) */

#define FREQ_17_EXACT	1789790 /* exact 1.79 MHz clock freq */
#define FREQ_17_APPROX	1787520 /* approximate 1.79 MHz clock freq */

#define MAXPOKEYS	4	/* max number of emulated chips */

#define CLIP			/* required to force clipping */

#define USE_SAMP_N_MAX	0	/* limit max. frequency to playback rate */


#define NO_CLIP   0
#define USE_CLIP  1

#define POKEY_DEFAULT_GAIN 6

#ifdef __cplusplus
extern "C" {
#endif

struct POKEYinterface {
    int num;    /* total number of pokeys in the machine */
    int baseclock;
    int volume;
    int gain;
    int clip;               /* determines if pokey.c will clip the sample range */
    /*******************************************
     * Handlers for reading the pot values.
     * Some Atari games use ALLPOT to return
     * dipswitch settings and other things
     * New function pointers for serin/serout
     * and a interrupt callback.
     *******************************************/
    int (*pot0_r[MAXPOKEYS])(int offset);
    int (*pot1_r[MAXPOKEYS])(int offset);
    int (*pot2_r[MAXPOKEYS])(int offset);
    int (*pot3_r[MAXPOKEYS])(int offset);
    int (*pot4_r[MAXPOKEYS])(int offset);
    int (*pot5_r[MAXPOKEYS])(int offset);
    int (*pot6_r[MAXPOKEYS])(int offset);
    int (*pot7_r[MAXPOKEYS])(int offset);
    int (*allpot_r[MAXPOKEYS])(int offset);
    int (*serin_r[MAXPOKEYS])(int offset);
    void (*serout_w[MAXPOKEYS])(int offset, int data);
    void (*interrupt_cb[MAXPOKEYS])(int mask);
};

/* ASG 980126 - added a return parameter to indicate failure */
int Pokey_sound_init (int freq17, int playback_freq, int volume, int num_pokeys, int use_clip);
/* ASG 980126 - added this function for cleanup */
void Pokey_sound_exit (void);
void Update_pokey_sound (int addr, int val, int chip, int gain);
void Pokey_process (int chip, void *buffer, int n);
int Read_pokey_regs (int addr, int chip);

int pokey_sh_start (struct POKEYinterface *interface);
void pokey_sh_stop (void);

int pokey1_r (int offset);
int pokey2_r (int offset);
int pokey3_r (int offset);
int pokey4_r (int offset);
int quad_pokey_r (int offset);

void pokey1_w (int offset,int data);
void pokey2_w (int offset,int data);
void pokey3_w (int offset,int data);
void pokey4_w (int offset,int data);
void quad_pokey_w (int offset,int data);

void pokey1_serin_ready (int after);
void pokey2_serin_ready (int after);
void pokey3_serin_ready (int after);
void pokey4_serin_ready (int after);

void pokey1_break_w (int shift);
void pokey2_break_w (int shift);
void pokey3_break_w (int shift);
void pokey4_break_w (int shift);

void pokey1_kbcode_w (int kbcode, int make);
void pokey2_kbcode_w (int kbcode, int make);
void pokey3_kbcode_w (int kbcode, int make);
void pokey4_kbcode_w (int kbcode, int make);

void pokey_sh_update (void);

#ifdef __cplusplus
}
#endif

#endif	/* POKEYSOUND_H */
