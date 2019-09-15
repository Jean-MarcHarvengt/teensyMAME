/************************************************/
#ifndef __YM2610_H__
#define __YM2610_H__

#include "fm.h"


#define   MAX_2610    (2)

int OPNBInit(int num, int inclock, int rate, int bitsize , int bufsiz , FMSAMPLE **buffer, int *pcmroma, int *pcmromb );
void OPNBShutdown(void);
void OPNBUpdate(void);
void OPNBWriteReg(int n, int port, int r, int v);
unsigned char OPNBReadStatus(int n);
unsigned char OPNBReadADPCMStatus(int n);
void OPNBUpdateOne(int num, int endp);

FMSAMPLE *OPNBBuffer(int n);
int OPNBSetBuffer(int n, FMSAMPLE **buf );
void OPNBSetIrqHandler(int n, void (*handler)(void) );

/************************************************************

  port 0 $10       ???
  port 0 $11       ???
  port 0 $12/$13   start address
  port 0 $14/$15   end address
  port 0 $16       ???
  port 0 $17       ???
  port 0 $18       ???
  port 0 $19/$1a   deltaT
  port 0 $1b       ???

  port 1 $00       ??? (channel select ?)
  7 6 5 4 3 2 1 0
  | | | | | | | +--- ch 0
              +----- ch 1
            +------- ch 2
          +--------- ch 3
        +----------- ch 4
      +------------- ch 5
  +----------------- deltaT ADPCM
  port 1 $08       ch 0 volume(pan) 7/6 pan? 5-0 volume?
  port 1 $09       ch 1 volume(pan)?
  port 1 $0a       ch 2 volume(pan)?
  port 1 $0b       ch 3 volume(pan)?
  port 1 $0c       ch 4 volume(pan)?
  port 1 $0d       ch 5 volume(pan)?

  port 1 $10/18    ch 0 start address
  port 1 $11/19    ch 1 start address
  port 1 $12/1a    ch 2 start address
  port 1 $13/1b    ch 3 start address
  port 1 $14/1c    ch 4 start address
  port 1 $15/1d    ch 5 start address

  port 1 $20/28    ch 0 end address
  port 1 $21/29    ch 1 end address
  port 1 $22/2a    ch 2 end address
  port 1 $23/2b    ch 3 end address
  port 1 $24/2c    ch 4 end address
  port 1 $25/2d    ch 5 end address

  ***********************************************************/

#endif
/**************** end of file ****************/
