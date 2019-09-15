/*
**
** File: ym2151.h -- header file for software implementation of YM-2151
**						FM Operator Type-M(OPM).
**
** (C) 1997,1998 Jarek Burczynski (s0246@goblin.pjwstk.waw.pl)
** Many of the optimizing ideas by Tatsuyuki Satoh
**
** Version 2.0  November, 29th 1998
**
**
** CAUTION !!! CAUTION !!!           H E L P   W A N T E D !!!
**
** If you have some very technical information about YM2151 and you would
** like to help:
**                  - PLEASE CONTACT ME ASAP!!! -
**
**
** Big THANK YOU to Beauty Planets for making a lot of real YM2151 samples and
** also for giving additional informations about the chip. Also for the time
** spent making the samples and the speed of replying to my endless requests.
**
** I would like to thank to Shigeharu Isoda without whom this project wouldn't
** be possible to finish (or even start). He has been so kind and helped me
** by scanning his YM2151 Japanese Manual first, and then by answering MANY
** of my questions.
**
** I would like to thank to nao also - for giving me some informations about
** YM2151 and pointing me to Shigeharu.
**
** Big thanks to Aaron Giles and Chris Hardy - they made some samples of one
** of my favourite arcade games so I could compare it to my emulator. That's
** how I know there's still something I need to know about this chip.
**
** Greetings to Ishmair (thanks for the datasheet !) who inspired me to write
** my own 2151 emu by telling me "... maybe with some somes and a lot of
** luck... in theory it's difficult... " - wow that really was a challenge :)
**
*/

#ifndef _H_YM2151_
#define _H_YM2151_


typedef void SAMP;  /* 16-bit or 8-bit sample modes supported */

/*
** Initialize YM2151 emulator(s).
**
** 'num' is the number of virtual YM2151's to allocate
** 'clock' is the chip clock in Hz
** 'rate' is sampling rate
** 'sample_bits' is 8-bit/16-bit sample selector
*/
int YM2151Init(int num, int clock, int rate, int sample_bits);


/* shutdown the YM2151 emulators */
void YM2151Shutdown(void);


/* reset all chip registers for YM2151 number 'num'*/
void YM2151ResetChip(int num);

/*
** Generate samples for one of the YM2151's
**
** 'num' is the number of virtual YM2151
** '**buffers' is table of pointers to the buffers: left and right
** 'length' is the number of samples should be generated
*/
void YM2151UpdateOne(int num, SAMP **buffers, int length);


/* write 'v' to register 'r' on YM2151 chip number 'n'*/
void YM2151WriteReg(int n, int r, int v);


/* read status register on YM2151 chip number 'n'*/
int YM2151ReadStatus(int n);


/* set interrupt handler on YM2151 chip number 'n'*/
void YM2151SetIrqHandler(int n, void (*handler)(void));


/* set port write handler on YM2151 chip number 'n' */
void YM2151SetPortWriteHandler(int n, void (*handler)(int offset, int data));


#endif /* _H_YM2151_ */
