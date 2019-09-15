/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6809/M6809.h" /* JB 970829 */

extern unsigned char *tp84_sharedram;	/* JB 970829 */

/* JB 970829 */
void tp84_init_machine(void)
{
	m6809_Flags = M6809_FAST_S | M6809_FAST_U;
}

/* JB 970829 - just give it what it wants
	F104: LDX   $6400
	F107: LDU   $6402
	F10A: LDA   $640B
	F10D: BEQ   $F13B
	F13B: LDX   $6404
	F13E: LDU   $6406
	F141: LDA   $640C
	F144: BEQ   $F171
	F171: LDA   $2000	; read beam
	F174: ADDA  #$20
	F176: BCC   $F104
*/
int tp84_beam_r(int offset)
{
/*	return 255 - cpu_getiloops();	* return beam position */
	return 255; /* always return beam position 255 */ /* JB 970829 */
}

int tp84_interrupt(void)
{
	return interrupt(); /* JB 970829 */
/*	if (cpu_getiloops() == 0) return interrupt();
	else return ignore_interrupt();*/
}

/* JB 970829 - catch a busy loop for CPU 1
	E0ED: LDA   #$01
	E0EF: STA   $4000
	E0F2: BRA   $E0ED
*/
void tp84_catchloop_w(int offset,int data)
{
	extern unsigned char *RAM;

	if( cpu_getpc()==0xe0f2 ) cpu_seticount (0);
	RAM[0x4000] = data;
}

/* JB 970829 - catch a busy loop for CPU 0
	83AF: CLRA
	83B0: LDX   $523A
	83B3: LDD   ,X
	83B5: ASLA
	83B6: BCC   $8398
	83B8: BRA   $83AF
*/
int tp84_catchloop_r(int offset)
{
	int data, dataw;

	data = tp84_sharedram[0x23a];

	if( cpu_getpc()==0x83b3 )
	{
		dataw = data<<8 | tp84_sharedram[0x23b];
		if (tp84_sharedram[dataw-0x5000] & 0x80) cpu_seticount (0);
	}
	return data;
}
