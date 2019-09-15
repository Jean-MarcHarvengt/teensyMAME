/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"



static int sound_ctrl;


void starfire_soundctrl_w(int offset, int data) {
    sound_ctrl = data;
}

int starfire_io1_r(int offset) {
    int in,out;

    in = readinputport(1);
    out = (in & 0x07) | 0xE0;

    if (sound_ctrl & 0x04)
        out = out | 0x08;
    else
        out = out & 0xF7;

    if (sound_ctrl & 0x08)
        out = out | 0x10;
    else
        out = out & 0xEF;

    return out;
}

int starfire_interrupt (void)
{

    return nmi_interrupt();
}
