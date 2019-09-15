/*****************************************************************************/
/*                                                                           */
/*                    (C) Copyright 1998  Peter J.C.Clare                    */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*      Module name:    vicdual.c                                            */
/*                                                                           */
/*      Creation date:  15/03/98                Revision date:  xx/xx/9x     */
/*                                                                           */
/*      Produced by:    Peter J.C.Clare                                      */
/*                                                                           */
/*                                                                           */
/*      Abstract:                                                            */
/*                                                                           */
/*              MAME sound & music driver for Sega/Gremlin Carnival.         */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*      Acknowledgements:                                                    */
/*                                                                           */
/*      Mike Coates, for the original Carnival MAME sound driver.            */
/*      Virtu-Al, for the sound samples & hardware information.              */
/*      The MAME Team, for the emulator framework.                           */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*      Revision history                                                     */
/*      ================                                                     */
/*                                                                           */
/*      Date    Vsn.    Initials        Description                          */
/*      ~~~~    ~~~~    ~~~~~~~~        ~~~~~~~~~~~                          */
/*                                                                           */
/*****************************************************************************/




/*****************************************************************************/
/*                                                                           */
/*      I n c l u d e   f i l e s                                            */
/*                                                                           */
/*****************************************************************************/


#include                        "driver.h"




/*****************************************************************************/
/*                                                                           */
/*      L o c a l   d e f i n i t i o n s                                    */
/*                                                                           */
/*****************************************************************************/


#define CPU_MUSIC_ID            1       /* music CPU id number */


/* output port 0x01 definitions - sound effect drive outputs */
#define OUT_PORT_1_RIFLE_SHOT   0x01
#define OUT_PORT_1_CLANG        0x02
#define OUT_PORT_1_DUCK_1       0x04
#define OUT_PORT_1_DUCK_2       0x08
#define OUT_PORT_1_DUCK_3       0x10
#define OUT_PORT_1_PIPE_HIT     0x20
#define OUT_PORT_1_BONUS_1      0x40
#define OUT_PORT_1_BONUS_2      0x80


/* output port 0x02 definitions - sound effect drive outputs */
#define OUT_PORT_2_BEAR         0x04
#define OUT_PORT_2_MUSIC_T1     0x08
#define OUT_PORT_2_MUSIC_RESET  0x10
#define OUT_PORT_2_RANKING      0x20


/* music CPU port definitions */
#define MUSIC_PORT2_PSG_BDIR    0x40    /* bit 6 on P2 */
#define MUSIC_PORT2_PSG_BC1     0x80    /* bit 7 on P2 */


#define PSG_BC_INACTIVE         0
#define PSG_BC_READ             MUSIC_PORT2_PSG_BC1
#define PSG_BC_WRITE            MUSIC_PORT2_PSG_BDIR
#define PSG_BC_LATCH_ADDRESS    ( MUSIC_PORT2_PSG_BDIR | MUSIC_PORT2_PSG_BC1 )


#define Play(id,loop)           sample_start( id, id, loop )
#define Stop(id)                sample_stop( id )


/* sample sound IDs - must match sample file table in driver file */
enum
{
        SND_BEAR = 0,
        SND_BONUS_1,
        SND_BONUS_2,
        SND_CLANG,
        SND_DUCK_1,
        SND_DUCK_2,
        SND_DUCK_3,
        SND_PIPE_HIT,
        SND_RANKING,
        SND_RIFLE_SHOT,
        NUMBER_OF_SOUND_EFFECTS
};




/*****************************************************************************/
/*                                                                           */
/*      P u b l i c   r o u t i n e s                                        */
/*                                                                           */
/*****************************************************************************/


void                            carnival_sh_port1_w( int offset, int data );
void                            carnival_sh_port2_w( int offset, int data );

int                             carnival_music_port_t1_r( int offset );

void                            carnival_music_port_1_w( int offset, int data );
void                            carnival_music_port_2_w( int offset, int data );

int                             carnival_sh_start( void );
void                            carnival_sh_stop( void );
void                            carnival_sh_update( void );




/*****************************************************************************/
/*                                                                           */
/*      L o c a l   v a r i a b l e s                                        */
/*                                                                           */
/*****************************************************************************/


static int                      port2State = 0;

static int                      psgData = 0;




/*****************************************************************************/
/*                                                                           */
/*      M o d u l e   c o d e                                                */
/*                                                                           */
/*****************************************************************************/


void carnival_sh_port1_w( int offset, int data )
{
        static int port1State = 0;
        int sndActiveMask;
        int changeMask;

        /* U64 74LS374 8 bit latch */

        /* bit 0: connector pin 36 - rifle shot */
        /* bit 1: connector pin 35 - clang */
        /* bit 2: connector pin 33 - duck #1 */
        /* bit 3: connector pin 34 - duck #2 */
        /* bit 4: connector pin 32 - duck #3 */
        /* bit 5: connector pin 31 - pipe hit */
        /* bit 6: connector pin 30 - bonus #1 */
        /* bit 7: connector pin 29 - bonus #2 */

        changeMask = port1State ^ data;
        sndActiveMask = changeMask & ~data;
        port1State = data;

        if ( sndActiveMask & OUT_PORT_1_RIFLE_SHOT )
        {
                Play( SND_RIFLE_SHOT, 0 );
        }

        if ( sndActiveMask & OUT_PORT_1_CLANG )
        {
                Play( SND_CLANG, 0 );
        }

        if ( sndActiveMask & OUT_PORT_1_DUCK_1 )
        {
                Play( SND_DUCK_1, 1 );
        }
        else if ( changeMask & OUT_PORT_1_DUCK_1 )
        {
                Stop( SND_DUCK_1 );
        }

        if ( sndActiveMask & OUT_PORT_1_DUCK_2 )
        {
                Play( SND_DUCK_2, 1 );
        }
        else if ( changeMask & OUT_PORT_1_DUCK_2 )
        {
                Stop( SND_DUCK_2 );
        }

        if ( sndActiveMask & OUT_PORT_1_DUCK_3 )
        {
                Play( SND_DUCK_3, 1 );
        }
        else if ( changeMask & OUT_PORT_1_DUCK_3 )
        {
                Stop( SND_DUCK_3 );
        }

        if ( sndActiveMask & OUT_PORT_1_PIPE_HIT )
        {
                Play( SND_PIPE_HIT, 0 );
        }

        if ( sndActiveMask & OUT_PORT_1_BONUS_1 )
        {
                Play( SND_BONUS_1, 0 );
        }

        if ( sndActiveMask & OUT_PORT_1_BONUS_2 )
        {
                Play( SND_BONUS_2, 0 );
        }
}




void carnival_sh_port2_w( int offset, int data )
{
        int sndActiveMask;
        int changeMask;

        /* U63 74LS374 8 bit latch */

        /* bit 0: connector pin 48 */
        /* bit 1: connector pin 47 */
        /* bit 2: connector pin 45 - bear */
        /* bit 3: connector pin 46 - Music !T1 input */
        /* bit 4: connector pin 44 - Music reset */
        /* bit 5: connector pin 43 - ranking */
        /* bit 6: connector pin 42 */
        /* bit 7: connector pin 41 */

        changeMask = port2State ^ data;

        sndActiveMask = changeMask & ~data;
        port2State = data;

        if ( sndActiveMask & OUT_PORT_2_BEAR )
        {
                Play( SND_BEAR, 0 );
        }

        if ( sndActiveMask & OUT_PORT_2_RANKING )
        {
                Play( SND_RANKING, 0 );
        }

        if ( sndActiveMask & OUT_PORT_2_MUSIC_RESET )
        {
                /* reset output is low - we'll reset 8039 when it goes high */
        }
        else if ( changeMask & OUT_PORT_2_MUSIC_RESET )
        {
                /* reset output is no longer asserted active low */
                cpu_reset( CPU_MUSIC_ID );
        }
}




int carnival_music_port_t1_r( int offset )
{
        /* note: 8039 T1 signal is inverted on music board */
        return ( port2State & OUT_PORT_2_MUSIC_T1 ) ? 0 : 1;
}




void carnival_music_port_1_w( int offset, int data )
{
        psgData = data;
}




void carnival_music_port_2_w( int offset, int data )
{
        static int psgSelect = 0;
        int newSelect;

        newSelect = data & ( MUSIC_PORT2_PSG_BDIR | MUSIC_PORT2_PSG_BC1 );
        if ( psgSelect != newSelect )
        {
                psgSelect = newSelect;

                switch ( psgSelect )
                {
                case PSG_BC_INACTIVE:
                        /* do nowt */
                        break;


                case PSG_BC_READ:
                        /* not very sensible for a write */
                        break;


                case PSG_BC_WRITE:
                        AY8910_write_port_0_w( 0, psgData );
                        break;


                case PSG_BC_LATCH_ADDRESS:
                        AY8910_control_port_0_w( 0, psgData );
                        break;
                }
        }
}
