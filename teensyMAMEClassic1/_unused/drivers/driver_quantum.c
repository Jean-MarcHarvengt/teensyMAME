/*
  quantum

  Paul Forgey, 1997

  This code is donated to the MAME team, and inherits all copyrights
  and restrictions from MAME
*/


/*
	QUANTUM MEMORY MAP (per schem):

	000000-003FFF	ROM0
	004000-004FFF	ROM1
	008000-00BFFF	ROM2
	00C000-00FFFF	ROM3
	010000-013FFF	ROM4

	018000-01BFFF	RAM0
	01C000-01CFFF	RAM1

	940000			TRACKBALL
	948000			SWITCHES
	950000			COLORRAM
	958000			CONTROL (LED and coin control)
	960000-970000	RECALL (nvram read)
	968000			VGRST (vector reset)
	970000			VGGO (vector go)
	978000			WDCLR (watchdog)
	900000			NVRAM (nvram write)
	840000			I/OS (sound and dip switches)
	800000-801FFF	VMEM (vector display list)
	940000			I/O (shematic label really - covered above)
	900000			DTACK1

*/


#include "driver.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/avgdvg.h"



int quantum_interrupt(void);
int quantum_switches_r(int offset);
void quantum_led_write(int offset,int data);
void quantum_snd_write(int offset,int data);
int quantum_snd_read(int offset);
int quantum_trackball_r (int offset);
int quantum_input_1_r(int offset);
int quantum_input_2_r(int offset);

int foodf_nvram_r(int offset);
void foodf_nvram_w(int offset,int data);
int foodf_nvram_load(void);
void foodf_nvram_save(void);

static unsigned char *quantum_nvram;
static int quantum_nvram_size;



struct MemoryReadAddress quantum_read[] =
{
	{ 0x000000, 0x013fff, MRA_ROM },
	{ 0x018000, 0x01cfff, MRA_BANK1 },
	{ 0x800000, 0x801fff, MRA_BANK2 },
	{ 0x840000, 0x84003f, quantum_snd_read },
	{ 0x900000, 0x9001ff, foodf_nvram_r },
	{ 0x940000, 0x940001, quantum_trackball_r }, /* trackball */
	{ 0x948000, 0x948001, quantum_switches_r },
	{ 0x978000, 0x978001, MRA_NOP },	/* ??? */
	{ -1 }	/* end of table */
};

struct MemoryWriteAddress quantum_write[] =
{
	{ 0x000000, 0x013fff, MWA_ROM },
	{ 0x018000, 0x01cfff, MWA_BANK1 },
	{ 0x800000, 0x801fff, MWA_BANK2, &vectorram, &vectorram_size },
	{ 0x840000, 0x84003f, quantum_snd_write },
	{ 0x900000, 0x9001ff, foodf_nvram_w },
	{ 0x950000, 0x95001f, quantum_colorram_w },
	{ 0x958000, 0x958001, quantum_led_write },
	{ 0x960000, 0x960001, MWA_NOP },	/* enable NVRAM? */
	{ 0x968000, 0x968001, avgdvg_reset },
//	{ 0x970000, 0x970001, avgdvg_go },
//	{ 0x978000, 0x978001, watchdog_reset_w },
	/* the following is wrong, but it's the only way I found to fix the service mode */
	{ 0x978000, 0x978001, avgdvg_go },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( quantum_input_ports )
	PORT_START	/* IN0 */
	/* YHALT here MUST BE ALWAYS 0  */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH,IPT_UNKNOWN )	/* vg YHALT */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

/* first POKEY is SW2, second is SW1 -- more confusion! */
	PORT_START /* DSW0 */
	PORT_DIPNAME( 0xc0, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x00, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0xc0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x40, "Free Play" )
	PORT_DIPNAME( 0x30, 0x00, "Right Coin", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "*1" )
	PORT_DIPSETTING(    0x20, "*4" )
	PORT_DIPSETTING(    0x10, "*5" )
	PORT_DIPSETTING(    0x30, "*6" )
	PORT_DIPNAME( 0x08, 0x00, "Left Coin", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "*1" )
	PORT_DIPSETTING(    0x08, "*2" )
	PORT_DIPNAME( 0x07, 0x00, "Bonus Coins", IP_KEY_NONE)
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPSETTING(    0x01, "1 each 5" )
	PORT_DIPSETTING(    0x02, "1 each 4" )
	PORT_DIPSETTING(    0x05, "1 each 3" )
	PORT_DIPSETTING(    0x06, "2 each 4" )

	PORT_START /* DSW1 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_ANALOG( 0x0f, 0, IPT_TRACKBALL_Y | IPF_REVERSE, 20, 7, 0,0)

	PORT_START      /* IN3 */
	PORT_ANALOG( 0x0f, 0, IPT_TRACKBALL_X, 20, 7, 0, 0 )
INPUT_PORTS_END



static struct GfxLayout fakelayout =
{
        1,1,
        0,
        1,
        { 0 },
        { 0 },
        { 0 },
        0
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        { 0, 0,      &fakelayout,     0, 256 },
        { -1 } /* end of array */
};

static unsigned char color_prom[] = { VEC_PAL_COLOR };



static struct POKEYinterface pokey_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz? */
	50,
	POKEY_DEFAULT_GAIN,
	NO_CLIP,
	/* The 8 pot handlers */
	{ quantum_input_1_r, quantum_input_2_r },
	{ quantum_input_1_r, quantum_input_2_r },
	{ quantum_input_1_r, quantum_input_2_r },
	{ quantum_input_1_r, quantum_input_2_r },
	{ quantum_input_1_r, quantum_input_2_r },
	{ quantum_input_1_r, quantum_input_2_r },
	{ quantum_input_1_r, quantum_input_2_r },
	{ quantum_input_1_r, quantum_input_2_r },
	/* The allpot handler */
	{ 0, 0 },
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,		/* really should be 6MHz, but we use 8 because the 68k isn't properly timed */
			0,
			quantum_read,quantum_write,0,0,
			quantum_interrupt,3	/* IRQ rate = 750kHz/4096 */
		}
	},
	60, 0,	/* frames per second, vblank duration (vector game, so no vblank) */
	1,
	0,

	/* video hardware */
	350, 400, { 0, 600, 0, 900 },
	gfxdecodeinfo,
	256, 256,
	avg_init_colors,

	VIDEO_TYPE_VECTOR,
	0,
	avg_start_quantum,
	avg_stop,
	avg_screenrefresh,

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

ROM_START( quantum_rom )
	ROM_REGION(0x014000)
    ROM_LOAD_EVEN( "136016.201",   0x000000, 0x002000, 0x7e7be63a )
    ROM_LOAD_ODD ( "136016.206",   0x000000, 0x002000, 0x2d8f5759 )
    ROM_LOAD_EVEN( "136016.102",   0x004000, 0x002000, 0x408d34f4 )
    ROM_LOAD_ODD ( "136016.107",   0x004000, 0x002000, 0x63154484 )
    ROM_LOAD_EVEN( "136016.203",   0x008000, 0x002000, 0xbdc52fad )
    ROM_LOAD_ODD ( "136016.208",   0x008000, 0x002000, 0xdab4066b )
    ROM_LOAD_EVEN( "136016.104",   0x00C000, 0x002000, 0xbf271e5c )
    ROM_LOAD_ODD ( "136016.109",   0x00C000, 0x002000, 0xd2894424 )
    ROM_LOAD_EVEN( "136016.105",   0x010000, 0x002000, 0x13ec512c )
    ROM_LOAD_ODD ( "136016.110",   0x010000, 0x002000, 0xacb50363 )
ROM_END

ROM_START( quantum1_rom )
	ROM_REGION(0x014000)
    ROM_LOAD_EVEN( "136016.101",   0x000000, 0x002000, 0x5af0bd5b )
    ROM_LOAD_ODD ( "136016.106",   0x000000, 0x002000, 0xf9724666 )
    ROM_LOAD_EVEN( "136016.102",   0x004000, 0x002000, 0x408d34f4 )
    ROM_LOAD_ODD ( "136016.107",   0x004000, 0x002000, 0x63154484 )
    ROM_LOAD_EVEN( "136016.103",   0x008000, 0x002000, 0x948f228b )
    ROM_LOAD_ODD ( "136016.108",   0x008000, 0x002000, 0xe4c48e4e )
    ROM_LOAD_EVEN( "136016.104",   0x00C000, 0x002000, 0xbf271e5c )
    ROM_LOAD_ODD ( "136016.109",   0x00C000, 0x002000, 0xd2894424 )
    ROM_LOAD_EVEN( "136016.105",   0x010000, 0x002000, 0x13ec512c )
    ROM_LOAD_ODD ( "136016.110",   0x010000, 0x002000, 0xacb50363 )
ROM_END



struct GameDriver quantum_driver =
{
	__FILE__,
	0,
	"quantum",
	"Quantum (rev 2)",
	"1982",
	"Atari",
	"Paul Forgey (MAME driver)\nAaron Giles (MAME driver)\n"VECTOR_TEAM,
	0,
	&machine_driver,
	0,

	quantum_rom,
	NULL, NULL,

	NULL,
	NULL,

	quantum_input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	foodf_nvram_load, foodf_nvram_save
};

struct GameDriver quantum1_driver =
{
	__FILE__,
	&quantum_driver,
	"quantum1",
	"Quantum (rev 1)",
	"1982",
	"Atari",
	"Paul Forgey (MAME driver)\nAaron Giles (MAME driver)\n"VECTOR_TEAM,
	0,
	&machine_driver,
	0,

	quantum1_rom,
	NULL, NULL,

	NULL,
	NULL,

	quantum_input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	foodf_nvram_load, foodf_nvram_save
};
