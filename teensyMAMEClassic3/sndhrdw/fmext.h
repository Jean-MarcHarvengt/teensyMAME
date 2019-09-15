/*
          FM.C External interface port for MAME
*/

/* For YM2203 / YM2608 */
/* ----- SSG(YM2149/AY-3-8910) emurator interface port  ----- */
/* void SSGClk(int n,int clk,int rate) */
/* void SSGWrite(int n,int a,int v)    */
/* unsigned char SSGRead(int n,int a)  */
/* void SSGReset(int n)                */

/* SSGClk   : Set Clock          */
/* int n    = chip number        */
/* int clk  = MasterClock(Hz)    */
/* int rate = sample rate(Hz) */
#define SSGClk(chip,clock) AY8910_set_clock(chip,clock)

/* SSGWrite : Write SSG port     */
/* int n    = chip number        */
/* int a    = address            */
/* int v    = data               */
#define SSGWrite(n,a,v) AY8910Write(n,a,v)

/* SSGRead  : Read SSG port */
/* int n    = chip number   */
/* return   = Read data     */
#define SSGRead(n) AY8910Read(n)

/* SSGReset : Reset SSG chip */
/* int n    = chip number   */
#define SSGReset(chip) AY8910_reset(chip)

/* -------------------- Timer Interface ---------------------*/
#ifndef INTERNAL_TIMER

/* update request callback */

#ifdef BUILD_YM2203
INLINE void YM2203UpdateReq(int chip)
{
	YM2203UpdateRequest(chip); /* in psgintf.c */
}
#endif
#ifdef BUILD_YM2608
INLINE void YM2608UpdateReq(int chip)
{
#if 0
	YM2608UpdateRequest(chip); /* in 2608intf.c */
#endif
}
#endif
#ifdef BUILD_YM2610
INLINE void YM2610UpdateReq(int chip)
{
	YM2610UpdateRequest(chip); /* in 2610intf.c */
}
#endif


#ifdef BUILD_YM2612
INLINE void YM2612UpdateReq(int chip)
{
#if 0
	YM2612UpdateRequest(chip); /* in 2612intf.c */
#endif
}
#endif

#ifdef BUILD_YM2151
INLINE void YM2151UpdateReq(int chip)
{
	YM2151UpdateRequest(chip); /* in 2151intf.c */
}
#endif /* BUILD_YM2151 */



#endif

#ifdef BUILD_YM2608
/*******************************************************************************/
/*		YM2608 local section                                                   */
/*******************************************************************************/
/* --------------- External Rhythm(ADPCM) emurator interface port  ---------------*/
/* int n = chip number */
/* int c = rhythm number */
void RhythmStop(int n,int r)
{
	return;
}
void RhythmStart(int n,int r)
{
	return;
}
#endif
