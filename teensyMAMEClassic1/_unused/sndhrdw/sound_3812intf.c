/*$DEADSERIOUSCLAN$*********************************************************************
* FILE
*	Yamaha 3812 emulator interface - MAME VERSION
*
* CREATED BY
*	Ernesto Corvi
*
* UPDATE LOG
*	CHS 1998-10-23	Mame streaming sound chip update
*	EC	1998		Created Interface
*
* NOTES
*	The ym3812 emulator supports several OPL (OPL1/OPL2/OPLL etc.) chips, update the
*	interface if you want to use this feature!!!
*
***************************************************************************************/
#include "driver.h"
#include "3812intf.h"
#include "fm.h"
#include "ym3812.h"

#define OPL3CONVERTFREQUENCY

/* Main emulated vs non-emulated switch */
/* default : Use emulated YM3812 */
int use_emulated_ym3812 = 1;

/* Emulated YM3812 variables and defines */
static ym3812* ym = 0;
static int ym_channel = 0;
static int sample_bits;
static FMSAMPLE *buffer;


/* Non-Emulated YM3812 variables and defines */
#define MASTER_CLOCK_BASE 3600000
static int pending_register;
static int register_0xbd;
static unsigned char status_register;
static unsigned char timer_register;
static unsigned int timer1_val;
static unsigned int timer2_val;
static double timer_scale;
static int aOPLFreqArray[16];		// Up to 9 channels..

/* These ones are used by both */
struct YM3812interface *intf;
static void *timer1;
static void *timer2;

/* Function procs to access the selected YM type */
static int ( *sh_start )( struct YM3812interface *interface );
static void ( *sh_stop )( void );
static void ( *sh_update )( void );
static int ( *status_port_0_r )( int offset );
static void ( *control_port_0_w )( int offset, int data );
static void ( *write_port_0_w )( int offset, int data );
static int ( *read_port_0_r )( int offset );

/**********************************************************************************************
	Begin of non-emulated YM3812 interface block
 **********************************************************************************************/

void timer1_callback (int param)
{
	if (!(timer_register & 0x40))
	{
		if (intf->handler) (*intf->handler)();

		/* set the IRQ and timer 1 signal bits */
		status_register |= 0x80;
		status_register |= 0x40;
	}

	/* next! */
	timer1 = timer_set (TIME_IN_USEC (timer1_val * 80) * timer_scale, 0, timer1_callback);
}

void timer2_callback (int param)
{
	if (!(timer_register & 0x20))
	{
		if (intf->handler) (*intf->handler)();

		/* set the IRQ and timer 2 signal bits */
		status_register |= 0x80;
		status_register |= 0x20;
	}

	/* next! */
	timer2 = timer_set (TIME_IN_USEC (timer2_val * 320) * timer_scale, 0, timer2_callback);
}

int nonemu_YM3812_sh_start(struct YM3812interface *interface)
{
	pending_register = -1;
	timer1 = timer2 = 0;
	status_register = 0x80;
	timer_register = 0;
	timer1_val = timer2_val = 256;
	intf = interface;

	timer_scale = (double)MASTER_CLOCK_BASE / (double)interface->baseclock;

	return 0;
}

void nonemu_YM3812_sh_stop(void)
{
}

void nonemu_YM3812_sh_update(void)
{
}


int nonemu_YM3812_status_port_0_r(int offset)
{
	/* mask out the timer 1 and 2 signal bits as requested by the timer register */
	return status_register & ~(timer_register & 0x60);
}

void nonemu_YM3812_control_port_0_w(int offset,int data)
{
	pending_register = data;

	/* pass through all non-timer registers */
#ifdef OPL3CONVERTFREQUENCY
	if ( ((data==0xbd)||((data&0xe0)!=0xa0)) && ((data<2)||(data>4)) )
#else
	if ( ((data<2)||(data>4)) )
#endif
		osd_ym3812_control(data);
}

void nonemu_WriteConvertedFrequency( int nFrq, int nCh )
{
	int		nRealOctave;
	double	vRealFrq;

	vRealFrq = (((nFrq&0x3ff)<<((nFrq&0x7000)>>12))) * (double)intf->baseclock / (double)ym3812_StdClock;
	nRealOctave = 0;

	while( (vRealFrq>1023.0)&&(nRealOctave<7) )
	{
		vRealFrq /= 2.0;
		nRealOctave++;
	}
	osd_ym3812_control(0xa0|nCh);
	osd_ym3812_write(((int)vRealFrq)&0xff);
	osd_ym3812_control(0xb0|nCh);
	osd_ym3812_write( ((((int)vRealFrq)>>8)&3)|(nRealOctave<<2)|((nFrq&0x8000)>>10) );
}

void nonemu_YM3812_write_port_0_w(int offset,int data)
{
//	if (pending_register < 2 || pending_register > 4)
	int nCh = pending_register&0x0f;

	if( pending_register == 0xbd ) register_0xbd = data;
#ifdef OPL3CONVERTFREQUENCY
	if( (nCh<9) )// if( (((~register_0xbd)&0x20)||(nCh<6)) )
	{
		if( (pending_register&0xf0) == 0xa0 )
		{
			aOPLFreqArray[nCh] = (aOPLFreqArray[nCh] & 0xf300)|(data&0xff);
			nonemu_WriteConvertedFrequency( aOPLFreqArray[nCh], nCh );
			return;
		}
		else if( (pending_register&0xf0)==0xb0 )
		{
			aOPLFreqArray[pending_register&0xf] = (aOPLFreqArray[nCh] & 0x00ff)|((data&0x3)<<8)|((data&0x1c)<<10)|((data&0x20)<<10);
			nonemu_WriteConvertedFrequency( aOPLFreqArray[nCh], nCh );
			return;
		}
	}
#endif
	if ( (pending_register<2)||(pending_register>4) ) osd_ym3812_write(data);
	else
	{
		switch (pending_register)
		{
			case 2:
				timer1_val = 256 - data;
				break;

			case 3:
				timer2_val = 256 - data;
				break;

			case 4:
				/* bit 7 means reset the IRQ signal and status bits, and ignore all the other bits */
				if (data & 0x80)
				{
					status_register = 0;
				}
				else
				{
					/* set the new timer register */
					timer_register = data;

					/*  bit 0 starts/stops timer 1 */
					if (data & 0x01)
					{
						if (!timer1)
							timer1 = timer_set (TIME_IN_USEC (timer1_val * 80) * timer_scale, 0, timer1_callback);
					}
					else if (timer1)
					{
						timer_remove (timer1);
						timer1 = 0;
					}

					/*  bit 1 starts/stops timer 2 */
					if (data & 0x02)
					{
						if (!timer2)
							timer2 = timer_set (TIME_IN_USEC (timer2_val * 320) * timer_scale, 0, timer2_callback);
					}
					else if (timer2)
					{
						timer_remove (timer2);
						timer2 = 0;
					}

					/* bits 5 & 6 clear and mask the appropriate bit in the status register */
					status_register &= ~(data & 0x60);
				}
				break;
		}
	}
	pending_register = -1;
}

int nonemu_YM3812_read_port_0_r( int offset ) {
	return pending_register;
}

/**********************************************************************************************
	End of non-emulated YM3812 interface block
 **********************************************************************************************/


/**********************************************************************************************
	Begin of emulated YM3812 interface block
 **********************************************************************************************/

void timer_callback( int timer ) {

	if ( ym3812_TimerEvent( ym, timer ) ) { /* update sr */
			if (intf->handler)
				(*intf->handler)();
	}
}

void timer_handler( int timer, double period, ym3812 *pOPL, int Tremove ) {
	switch( timer ) {
		case 1:
            if ( Tremove ) {
				if ( timer1 ) {
					timer_remove( timer1 );
					timer1 = 0;
				}
			} else
				timer1 = timer_pulse( period, timer, timer_callback );
		break;

		case 2:
            if ( Tremove ) {
				if ( timer2 ) {
					timer_remove( timer2 );
					timer2 = 0;
				}
			} else
				timer2 = timer_pulse( period, timer, timer_callback );
		break;
	}
}

void emu_ym3812_fixed_pointer_problem_update( int nNoll, void *pBuffer, int nLength )
{
	// The ym3812 supports several ym chips, update the interface if you want to use this feature!!!
	ym3812_Update_stream( ym, pBuffer, nLength );
}

int emu_YM3812_sh_start( struct YM3812interface *interface ) {
	int rate = Machine->sample_rate;

	if ( ym )		/* duplicate entry */
		return 1;

	intf = interface;

	sample_bits = Machine->sample_bits;

    ym = ym3812_Init( rate, intf->baseclock, ( sample_bits == 16 ) );

	if ( ym == NULL )
		return -1;

	timer1 = timer2 = 0;
	ym->SetTimer = timer_handler;

   ym_channel = stream_init(
			"OPL 1/2/L",rate,sample_bits,
			0,emu_ym3812_fixed_pointer_problem_update);
	return 0;
}

void emu_YM3812_sh_stop( void ) {

	if ( ym ) {
		ym = ym3812_DeInit( ym );
		ym = 0;
	}

	if ( timer1 ) {
		timer_remove( timer1 );
		timer1 = 0;
	}

	if ( timer2 ) {
		timer_remove( timer2 );
		timer2 = 0;
	}

	if ( buffer )
		free( buffer );
}

void emu_YM3812_sh_update( void ) {
}

int emu_YM3812_status_port_0_r( int offset ) {
	return ym3812_ReadStatus( ym );
}

void emu_YM3812_control_port_0_w( int offset, int data ) {
	ym3812_SetReg( ym, data );
}

void emu_YM3812_write_port_0_w( int offset, int data ) {
//	stream_update( ym_channel, 0 );	// Update the buffer before writing new regs
	ym3812_WriteReg( ym, data );
}

int emu_YM3812_read_port_0_r( int offset ) {
	return ym3812_ReadReg( ym );
}

/**********************************************************************************************
	End of emulated YM3812 interface block
 **********************************************************************************************/


/**********************************************************************************************
	Begin of YM3812 interface stubs block
 **********************************************************************************************/

int YM3812_sh_start( struct YM3812interface *interface ) {

	if ( use_emulated_ym3812 ) {
		sh_start = emu_YM3812_sh_start;
		sh_stop = emu_YM3812_sh_stop;
		sh_update = emu_YM3812_sh_update;
		status_port_0_r = emu_YM3812_status_port_0_r;
		control_port_0_w = emu_YM3812_control_port_0_w;
		write_port_0_w = emu_YM3812_write_port_0_w;
		read_port_0_r = emu_YM3812_read_port_0_r;
	} else {
		sh_start = nonemu_YM3812_sh_start;
		sh_stop = nonemu_YM3812_sh_stop;
		sh_update = nonemu_YM3812_sh_update;
		status_port_0_r = nonemu_YM3812_status_port_0_r;
		control_port_0_w = nonemu_YM3812_control_port_0_w;
		write_port_0_w = nonemu_YM3812_write_port_0_w;
		read_port_0_r = nonemu_YM3812_read_port_0_r;
	}

   /* now call the proper handler */
	return (*sh_start)(interface);
}

void YM3812_sh_stop( void ) {
	(*sh_stop)();
}

void YM3812_sh_update( void ) {
	(*sh_update)();
}

int YM3812_status_port_0_r( int offset ) {
	return (*status_port_0_r)( offset );
}

void YM3812_control_port_0_w( int offset, int data ) {
	(*control_port_0_w)( offset, data );
}

void YM3812_write_port_0_w( int offset, int data ) {
	(*write_port_0_w)( offset, data );
}

int YM3812_read_port_0_r( int offset ) {
	return (*read_port_0_r)( offset );
}

/**********************************************************************************************
	End of YM3812 interface stubs block
 **********************************************************************************************/
