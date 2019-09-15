/***************************************************************************

  liberator.c - 'driver.c'

***************************************************************************/
/***************************************************************************
	Liberator Memory Map
	 (from the schematics/manual)

	HEX        R/W   D7 D6 D5 D4 D3 D2 D2 D0  function
	---------+-----+------------------------+------------------------
    0000             D  D  D  D  D  D  D  D   XCOORD
    0001             D  D  D  D  D  D  D  D   YCOORD
    0002             D  D  D                  BIT MODE DATA
	---------+-----+------------------------+------------------------
    0003-033F        D  D  D  D  D  D  D  D   Working RAM
    0340-303F        D  D  D  D  D  D  D  D   Screen RAM  \
    3D40-3FFF        D  D  D  D  D  D  D  D   Working RAM -> misprint in manual?
	---------+-----+------------------------+------------------------
    4000-403F    R   D  D  D  D  D  D  D  D   EARD*  read from non-volatile memory
	---------+-----+------------------------+------------------------
    5000         R                        D   coin AUX   (CTRLD* set low)
    5000         R                     D      coin LEFT  (CTRLD* set low)
    5000         R                  D         coin RIGHT (CTRLD* set low)
    5000         R               D            SLAM       (CTRLD* set low)
    5000         R            D               SPARE      (CTRLD* set low)
    5000         R         D                  SPARE      (CTRLD* set low)
    5000         R      D                     COCKTAIL   (CTRLD* set low)
    5000         R   D                        SELF-TEST  (CTRLD* set low)
    5000         R               D  D  D  D   HDIR   (CTRLD* set high)
    5000         R   D  D  D  D               VDIR   (CTRLD* set high)
	---------+-----+------------------------+------------------------
    5001         R                        D   SHIELD 2
    5001         R                     D      SHIELD 1
    5001         R                  D         FIRE 2
    5001         R               D            FIRE 1
    5001         R            D               SPARE      (CTRLD* set low)
    5001         R         D                  START 2
    5001         R      D                     START 1
    5001         R   D                        VBLANK
	---------+-----+------------------------+------------------------
    6000-600F    W               D  D  D  D   BASRAM*
    6200-621F    W   D  D  D  D  D  D  D  D   COLORAM*
    6400         W                            INTACK*
    6600         W               D  D  D  D   EARCON
    6800         W   D  D  D  D  D  D  D  D   STARTLG
    6A00         W                            WDOG*
	---------+-----+------------------------+------------------------
    6C00         W            D               START LED 1
    6C01         W            D               START LED 2
    6C02         W            D               TBSWP*
    6C03         W            D               SPARE
    6C04         W            D               CTRLD*
    6C05         W            D               COINCNTRR
    6C06         W            D               COINCNTRL
    6C07         W            D               PLANET
	---------+-----+------------------------+------------------------
    6E00-6E3F    W   D  D  D  D  D  D  D  D   EARWR*
    7000-701F        D  D  D  D  D  D  D  D   IOS2* (Pokey 2)
    7800-781F        D  D  D  D  D  D  D  D   IOS1* (Pokey 1)
    8000-EFFF    R   D  D  D  D  D  D  D  D   ROM
	-----------------------------------------------------------------


 Dip switches at D4 on the PCB for play options: (IN2)

LSB  D1   D2   D3   D4   D5   D6   MSB
SW8  SW7  SW6  SW5  SW4  SW3  SW2  SW1    Option
-------------------------------------------------------------------------------------
Off  Off                                 4 ships per game   <-
On   Off                                 5 ships per game
Off  On                                  6 ships per game
On   On                                  8 ships per game
-------------------------------------------------------------------------------------
          Off  Off                       Bonus ship every 15000 points
          On   Off                       Bonus ship every 20000 points   <-
          Off  On                        Bonus ship every 25000 points
          On   On                        Bonus ship every 30000 points
-------------------------------------------------------------------------------------
                    On   Off             Easy game play
                    Off  Off             Normal game play   <-
                    Off  On              Hard game play
-------------------------------------------------------------------------------------
                                X    X   Not used
-------------------------------------------------------------------------------------


 Dip switches at A4 on the PCB for price options: (IN3)

LSB  D1   D2   D3   D4   D5   D6   MSB
SW8  SW7  SW6  SW5  SW4  SW3  SW2  SW1    Option
-------------------------------------------------------------------------------------
Off  Off                                 Free play
On   Off                                 1 coin for 2 credits
Off  On                                  1 coin for 1 credit   <-
On   On                                  2 coins for 1 credit
-------------------------------------------------------------------------------------
          Off  Off                       Right coin mech X 1   <-
          On   Off                       Right coin mech X 4
          Off  On                        Right coin mech X 5
          On   On                        Right coin mech X 6
-------------------------------------------------------------------------------------
                    Off                  Left coin mech X 1    <-
                    On                   Left coin mech X 2
-------------------------------------------------------------------------------------
                         Off  Off  Off   No bonus coins        <-
                         Off  On   Off   For every 4 coins inserted, game logic
                                          adds 1 more coin

                         On   On   Off   For every 4 coins inserted, game logic
                                          adds 2 more coin
                         Off  Off  On    For every 5 coins inserted, game logic
                                          adds 1 more coin
                         On   Off  On    For every 3 coins inserted, game logic
                                          adds 1 more coin
                          X   On   On    No bonus coins
-------------------------------------------------------------------------------------
<-  = Manufacturer's suggested settings

******************************************************************************************/

#include "driver.h"
#include "machine/atari_vg.h"

/* Important:
//   These next two defines must match the ones in the
//     driver file, or very bad things will happen.		*/
#define LIB_ASPECTRATIO_512x384		0
#define LIB_ASPECTRATIO_342x256		1

/* from machine */
void liberator_init_machine(void);
void liberator_w(int offset, int data);
void liberator_port_w(int offset, int data);
int  liberator_r(int offset);
int  liberator_port_r(int offset) ;
int  liberator_interrupt(void) ;

/* from vidhrdw */
int  liberator_vh_start(void);
void liberator_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void liberator_vh_stop(void);
void liberator_startlg_w(int offset,int data) ;
void liberator_basram_w(int address, int data) ;
void liberator_colorram_w(int address, int data) ;


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3FFF, liberator_r },
	{ 0x4000, 0x403F, atari_vg_earom_r },
	{ 0x5000, 0x5001, liberator_port_r },
	{ 0x7000, 0x701F, pokey2_r },
	{ 0x7800, 0x781F, pokey1_r },
	{ 0x8000, 0xFFFF, MRA_ROM },
	{ -1 }	/* end of table */
};


static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3FFF, liberator_w },
	{ 0x6000, 0x600F, liberator_basram_w },
	{ 0x6200, 0x621F, liberator_colorram_w },
	{ 0x6400, 0x6400, MWA_NOP },
	{ 0x6600, 0x6600, atari_vg_earom_ctrl },
	{ 0x6800, 0x6800, liberator_startlg_w },
	{ 0x6A00, 0x6A00, MWA_NOP },
	{ 0x6C00, 0x6C07, liberator_port_w },
	{ 0x6E00, 0x6E3F, atari_vg_earom_w },
	{ 0x7000, 0x701F, pokey2_w },
	{ 0x7800, 0x781F, pokey1_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START			/* IN0 - $5000 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x40, 0x40, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START			/* IN1 - $5001 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH,IPT_VBLANK )

	PORT_START			/* IN2  -  Game Option switches DSW @ D4 on PCB */
	PORT_DIPNAME( 0x03, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x02, "6" )
	PORT_DIPSETTING(    0x03, "8" )
	PORT_DIPNAME( 0x0C, 0x04, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "15000" )
	PORT_DIPSETTING(    0x04, "20000" )
	PORT_DIPSETTING(    0x08, "25000" )
	PORT_DIPSETTING(    0x0C, "30000" )
	PORT_DIPNAME( 0x30, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Normal" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x30, "???" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START			/* IN3  -  Pricing Option switches DSW @ A4 on PCB */
	PORT_DIPNAME( 0x03, 0x02, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	PORT_DIPNAME( 0x0c, 0x00, "Right Coin", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "*1" )
	PORT_DIPSETTING (   0x04, "*4" )
	PORT_DIPSETTING (   0x08, "*5" )
	PORT_DIPSETTING (   0x0c, "*6" )
	PORT_DIPNAME( 0x10, 0x00, "Left Coin", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "*1" )
	PORT_DIPSETTING (   0x10, "*2" )
	/* TODO: verify the following settings */
	PORT_DIPNAME( 0xe0, 0x00, "Bonus Coins", IP_KEY_NONE )
	PORT_DIPSETTING (   0x00, "None" )
	PORT_DIPSETTING (   0x80, "1 each 5" )
	PORT_DIPSETTING (   0x40, "1 each 4 (+Demo)" )
	PORT_DIPSETTING (   0xa0, "1 each 3" )
	PORT_DIPSETTING (   0x60, "2 each 4 (+Demo)" )
	PORT_DIPSETTING (   0x20, "1 each 2" )
	PORT_DIPSETTING (   0xc0, "Freeze Mode" )
	PORT_DIPSETTING (   0xe0, "Freeze Mode" )

	PORT_START	/* IN4 - FAKE - overlaps IN0 in the HW */
	PORT_ANALOG ( 0x0f, 0x0, IPT_TRACKBALL_X , 100, 7, 0, 0)

	PORT_START	/* IN5 - FAKE - overlaps IN0 in the HW */
	PORT_ANALOG ( 0x0f, 0x0, IPT_TRACKBALL_Y , 100, 7, 0, 0)
INPUT_PORTS_END



static struct POKEYinterface pokey_interface =
{
	2,				/* 2 chips */
	FREQ_17_APPROX,	/* 1.7 Mhz */
	100,
	POKEY_DEFAULT_GAIN,
	NO_CLIP,
	/* The 8 pot handlers */
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	/* The allpot handler */
	{ input_port_3_r, input_port_2_r }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,		/* cpu_type					*/
			1250000,		/* cpu_clock (Hz)			*/
			0,				/* number of CPU memory region (allocated by loadroms())	*/
			readmem,writemem,
			0,0,			/* port_read, port_write	*/
			liberator_interrupt, 4	/* interrupt(), ints / frame 	*/
		}
	},						/* MachineCPU[]				*/
	60,						/* frames_per_second		*/
	DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,						/* cpu_slices_per_frame		*/
	liberator_init_machine,	/* init_machine fcn			*/

	/* video hardware */
#if LIB_ASPECTRATIO_342x256
	342, 256, 				/* scrn wd,ht				*/
	{ 0, 342-1, 0, 256-1 },	/* visible area {minx, maxx, miny, maxy}	*/
#else
	512, 384, 				/* scrn wd,ht				*/
	{ 0, 512-1, 0, 384-1 },	/* visible area {minx, maxx, miny, maxy}	*/
#endif
	0, 						/* GfxDecodeInfo			*/
	32, 					/* total_colors				*/
	0,						/* length in bytes of the color lookup table (3*total_colors bytes)	*/
	0,	/* vh_convert_color_prom()	*/

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,	/* video_attributes	*/
	0,						/* vh_init()	*/
	liberator_vh_start,		/* vh_start()	*/
	liberator_vh_stop,		/* vh_stop()	*/
	liberator_vh_screenrefresh,	/* vh_update()	*/

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_POKEY,
			&pokey_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( liberator_rom )
	ROM_REGION(0x10000)	/* 64k for code and data  */
	/* note: the Planet Picture ROMs are not in the address space of the
	         processor on the real board, but it is convenient to allow
	         the built-in routines to load the files into memory.         */
	ROM_LOAD( "136012.110",   0x0000, 0x1000, 0x6eb11221 )	/* Planet Picture ROM */
	ROM_LOAD( "136012.107",   0x1000, 0x1000, 0x8a616a63 )	/* Planet Picture ROM */
	ROM_LOAD( "136012.108",   0x2000, 0x1000, 0x3f8e4cf6 )	/* Planet Picture ROM */
	ROM_LOAD( "136012.109",   0x3000, 0x1000, 0xdda0c0ef )	/* Planet Picture ROM */
	ROM_LOAD( "136012.206",   0x8000, 0x1000, 0x1a0cb4a0 )	/* code ROM */
	ROM_LOAD( "136012.205",   0x9000, 0x1000, 0x2f071920 )	/* code ROM */
	ROM_LOAD( "136012.204",   0xa000, 0x1000, 0xbcc91827 )	/* code ROM */
	ROM_LOAD( "136012.203",   0xb000, 0x1000, 0xb558c3d4 )	/* code ROM */
	ROM_LOAD( "136012.202",   0xc000, 0x1000, 0x569ba7ea )	/* code ROM */
	ROM_LOAD( "136012.201",   0xd000, 0x1000, 0xd12cd6d0 )	/* code ROM */
	ROM_LOAD( "136012.200",   0xe000, 0x1000, 0x1e98d21a )	/* code ROM */
	ROM_RELOAD(             0xf000, 0x1000 )		/* for interrupt vectors  */
ROM_END



struct GameDriver liberatr_driver =
{
	__FILE__,
	0,
	"liberatr",			/* name					*/
	"Liberator",		/* description			*/
	"1982",
	"Atari",
	"Paul Winkler",		/* credits				*/
	0,
	&machine_driver,	/* MachineDriver		*/
	0,

	liberator_rom,		/* RomModule			*/
	0, 0,				/* ROM decoding fcn's	*/
	0,					/* sound sample names	*/
	0,					/* sound_prom			*/

	input_ports, /* NewInputPort *		*/

	0, 0, 0,			/* color_prom, palette,colortable		*/
	ORIENTATION_DEFAULT,/* monitor orientation	*/

	atari_vg_earom_load,atari_vg_earom_save
};
