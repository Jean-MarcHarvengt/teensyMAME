// CHECK ym3812.c FOR LEGAL AND USAGE INFORMATION

#ifndef __YM3812_H_
#define __YM3812_H_

#ifdef __cplusplus
extern "C" {
#endif

// Signed or unsigned samples? (^80, ^8000 are unsigned)

#ifdef SIGNED_SAMPLES
#define ym3812_Sign8(n) (n)
#define ym3812_Sign16(n) (n)
#else
#define ym3812_Sign8(n) (n^0x80)
#define ym3812_Sign16(n) (n^0x8000)
#endif

// This frequency is from Yamaha 2413 documentation

#define ym3812_StdClock 3579545

// This volume is something I made up that sounds ok

#define ym3812_StdVolume 128

// Some bit defines (Status and Timer Control bit masks)

#define ym3812_STFLAG2	0x20
#define ym3812_STFLAG1	0x40
#define ym3812_STIRQ	0x80

#define ym3812_TCST1	0x01
#define ym3812_TCST2	0x02
#define ym3812_TCMASK2	0x20
#define ym3812_TCMASK1	0x40
#define ym3812_TCIRQRES	0x80

// These defines are probably private to this file.

enum EnvStat { ADSR_Silent, ADSR_Attack, ADSR_Decay, ADSR_Sustain, ADSR_Release };

typedef struct ym3812_s {

	// TIMER Related stuff

	unsigned char	nReg;				// Current register for write/read
	int				nStatus;			// The ym3812 STATUS register
	double			vTimer1;			// Timer 1 (80 microseconds)
	double			vTimer2;			// Timer 2 (320 microseconds)
	int				nTimerCtrl;			// Timer control...
	int				nYM3812Clk;			// The input clock rate for the chip
	int				nYM3812DivClk;		// The clock frequency divided by 72...

	// ym3812 Registers

	int				fWave;				// If true=>Rythm sounds are used
	int				nDepthRhythm;		// 0xbd - control of depth and rhythm
	int				nCSM;				// Computer Speech Mode - NOT EMULATED!!!
	int				fEGTyp[18];			// true=>continuing sound (use keyoff); false=>diminishing sound (no keyoff).
	int				fVibrato[18];		// true=>vibrato (use 0xbd reg for vibrato depth)
	int				fAM[18];			// true=>Amplitude modulation (use 0xbd reg for amplitude modulation depth)
	int				fKSR[18];			// true=>Key scale rate on
	int				nTotalLevel[18];	// Total volume level for each slot
	int				nKSL[18];			// Key Scale Level (somewhat similar to Key Scale Rate)
	int				nMulti[18];			// Multiple for carrier frequency
	int				nWave[18];			// Wave select for each slot
	int				nAttack[18];		// Attack value for each slot/channel
	int				nDecay[18];			// Decay value for each slot/channel
	int				nSustain[18];		// Sustain value for each slot/channel
	int				nRelease[18];		// Release value for each slot/channel
	int				nEnvState[18];		// Envelop state
	float			vEnvTime[18];		// Envelop time for current A, D, S or R.
	int				nFNumber[9];		// Frequency info (F-NUMBER) for each channel
	int				nOctave[9];			// Octave (BLOCK) for each channel
	int				nFeedback[9];		// Modulation factor for feedback FM modulation of the first slot
	int				fConnection[9];		// true=>parallell, false=>serial connection.
	int				fKeyDown[9];		// Check that key is downed before starting new sound!

	// Work registers

	unsigned int	nCurrPos[18];		// Current position in sin table
	int				nVibratoOffs;		// Vibrato sinus offset
	int				nAMDepthOffs;		// AM offset
	int				nSinValue[18];		// Keep the sinus value for each channel for the feedback stuff

	// Read back stuff table..

	char			aRegArray[256];		// Read back written stuff

	// Initialized by emulator
	int				nSubDivide;			// Number of times to subdivide the sample buffer generation
	int				f16Bit;				// True if 16 bit samples.
	int				aVolumes[256];		// Volume conversion table
	signed char		*pDrum[5];			// Pointer to samples (11025 Hz)
	int				nDrumSize[5];		// Size of samples
	int				nDrumOffs[5];		// Current (playing) offset of samples
	int				nDrumRate[5];		// Rate of each individual drum
	int				nReplayFrq;			// The replay frequency (for calculating number of samples to play)
	void			(*SetTimer)(int, double, struct ym3812_s *, int);	// Routine that starts/removes an IRQ timer.
} ym3812;

#ifndef cFALSE
#define cFALSE 0
#endif
#ifndef cTRUE
#define cTRUE 1
#endif

extern ym3812* ym3812_Init( int nReplayFrq, int nClock, int f16Bit );
extern ym3812* ym3812_DeInit( ym3812 *pOPL );
extern void ym3812_Update_stream( ym3812* pOPL, void *pBuffer_in, int nLength );
extern int ym3812_ReadStatus( ym3812 *pOPL );
extern int	ym3812_ReadReg( ym3812 *pOPL );
extern void ym3812_SetReg( ym3812 *pOPL, unsigned char nReg );
extern void ym3812_WriteReg( ym3812 *pOPL, unsigned char nData);
extern int ym3812_TimerEvent( ym3812 *pOPL, int nTimer );

#ifdef __cplusplus
};
#endif
#endif


