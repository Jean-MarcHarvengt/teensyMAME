#ifndef __2610INTF_H__
#define __2610INTF_H__

#include "fm.h"
#ifdef BUILD_YM2610
  void YM2610UpdateRequest(int chip);
#else
  #include "ym2610.h"
#endif

#define   MAX_2610    (2)

struct YM2610interface{
	int num;	/* total number of 8910 in the machine */
	int baseclock;
	int volume[MAX_2610];
	int ( *portAread[MAX_2610] )( int offset );
	int ( *portBread[MAX_2610] )( int offset );
	void ( *portAwrite[MAX_2610] )( int offset, int data );
	void ( *portBwrite[MAX_2610] )( int offset, int data );
	void ( *handler[MAX_2610] )( void );	/* IRQ handler for the YM2610 */
	int pcmroma[MAX_2610];		/* ADPCM rom top buffer */
	int pcmromb[MAX_2610];		/* ADPCM rom top buffer */
};

/************************************************/
/* Sound Hardware Start							*/
/************************************************/
int YM2610_sh_start(struct YM2610interface *interface );

/************************************************/
/* Sound Hardware Stop							*/
/************************************************/
void YM2610_sh_stop(void);

/************************************************/
/* Sound Hardware Update						*/
/************************************************/
void YM2610_sh_update(void);

/************************************************/
/* Chip 0 functions								*/
/************************************************/
int YM2610_status_port_0_A_r( int offset );
int YM2610_status_port_0_B_r( int offset );
int YM2610_read_port_0_r(int offset);
void YM2610_control_port_0_A_w(int offset,int data);
void YM2610_control_port_0_B_w(int offset,int data);
void YM2610_data_port_0_A_w(int offset,int data);
void YM2610_data_port_0_B_w(int offset,int data);

/************************************************/
/* Chip 1 functions								*/
/************************************************/
int YM2610_status_port_1_A_r( int offset );
int YM2610_status_port_1_B_r( int offset );
int YM2610_read_port_1_r(int offset);
void YM2610_control_port_1_A_w(int offset,int data);
void YM2610_control_port_1_B_w(int offset,int data);
void YM2610_data_port_1_A_w(int offset,int data);
void YM2610_data_port_1_B_w(int offset,int data);

/**************************************************/
/*   YM2610 left/right position change (TAITO)    */
/**************************************************/
void YM2610_pan( int lr, int v );

#endif
/**************** end of file ****************/
