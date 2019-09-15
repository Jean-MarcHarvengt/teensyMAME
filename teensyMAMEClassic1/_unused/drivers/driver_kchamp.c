/***************************************************************************

Karate Champ - (c) 1984 Data East

Currently supported sets:
Karate Champ - VS Version (kchampvs)
Karate Champ - VS Version Japanese (karatedo)
Karate Champ - 1 Player Version (kchamp)

VS Version Info:
---------------
Memory Map:
Main CPU
0000-bfff ROM (encrypted)
c000-cfff RAM
d000-d3ff char videoram
d400-d7ff color videoram
d800-d8ff sprites
e000-ffff ROM (encrypted)

Sound CPU
0000-5fff ROM
6000-6300 RAM

IO Ports:
Main CPU
INPUT  00 = Player 1 Controls - ( ACTIVE LOW )
INPUT  40 = Player 2 Controls - ( ACTIVE LOW )
INPUT  80 = Coins and Start Buttons - ( ACTIVE LOW )
INPUT  C0 = Dip Switches - ( ACTIVE LOW )
OUTPUT 00 = Screen Flip? (There isnt a cocktail switch?) UNINMPLEMENTED
OUTPUT 01 = CPU Control
                bit 0 = external nmi enable
OUTPUT 02 = Sound Reset
OUTPUT 40 = Sound latch write

Sound CPU
INPUT  01 = Sound latch read
OUTPUT 00 = AY8910 #1 data write
OUTPUT 01 = AY8910 #1 control write
OUTPUT 02 = AY8910 #2 data write
OUTPUT 03 = AY8910 #2 control write
OUTPUT 04 = MSM5205 write
OUTPUT 05 = CPU Control
                bit 0 = MSM5205 trigger
                bit 1 = external nmi enable

1P Version Info:
---------------
Same as VS version but with a DAC instead of a MSM5205. Also some minor
IO ports and memory map changes. Dip switches differ too.

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80/Z80.h"

/* from vidhrdw */
extern void kchamp_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern void kchamp_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
extern int kchampvs_vh_start(void);
extern int kchamp1p_vh_start(void);


static int nmi_enable = 0;
static int sound_nmi_enable = 0;

static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0xbfff, MRA_ROM },
    { 0xc000, 0xcfff, MRA_RAM },
    { 0xd000, 0xd3ff, videoram_r },
    { 0xd400, 0xd7ff, colorram_r },
    { 0xd800, 0xd8ff, spriteram_r },
    { 0xd900, 0xdfff, MRA_RAM },
    { 0xe000, 0xffff, MRA_ROM },
    { -1 }
};

static struct MemoryWriteAddress writemem[] =
{
    { 0x0000, 0xbfff, MWA_ROM },
    { 0xc000, 0xcfff, MWA_RAM },
    { 0xd000, 0xd3ff, videoram_w, &videoram, &videoram_size },
    { 0xd400, 0xd7ff, colorram_w, &colorram },
    { 0xd800, 0xd8ff, spriteram_w, &spriteram, &spriteram_size },
    { 0xd900, 0xdfff, MWA_RAM },
    { 0xe000, 0xffff, MWA_ROM },
    { -1 }
};

static struct MemoryReadAddress sound_readmem[] =
{
    { 0x0000, 0x5fff, MRA_ROM },
    { 0x6000, 0xffff, MRA_RAM },
    { -1 }
};

static struct MemoryWriteAddress sound_writemem[] =
{
    { 0x0000, 0x5fff, MWA_ROM },
    { 0x6000, 0xffff, MWA_RAM },
    { -1 }
};

static void control_w( int offset, int data ) {
    nmi_enable = data & 1;
}

static void sound_reset_w( int offset, int data ) {
    if ( !( data & 1 ) )
    	cpu_reset( 1 );
}

static void sound_control_w( int offset, int data ) {
	MSM5205_reset_w( 0, !( data & 1 ) );
	sound_nmi_enable = ( ( data >> 1 ) & 1 );
}

static void sound_command_w( int offset, int data ) {
	soundlatch_w( 0, data );
	cpu_cause_interrupt( 1, 0xff );
}

static int msm_data = 0;
static int msm_play_lo_nibble = 1;

static void sound_msm_w( int offset, int data ) {
	msm_data = data;
	msm_play_lo_nibble = 1;
}

static struct IOReadPort readport[] =
{
    { 0x00, 0x00, input_port_0_r }, /* Player 1 controls - ACTIVE LOW */
    { 0x40, 0x40, input_port_1_r }, /* Player 2 controls - ACTIVE LOW */
    { 0x80, 0x80, input_port_2_r }, /* Coins & Start - ACTIVE LOW */
    { 0xC0, 0xC0, input_port_3_r }, /* Dipswitch */
    { -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
    { 0x00, 0x00, MWA_NOP },
    { 0x01, 0x01, control_w },
    { 0x02, 0x02, sound_reset_w },
    { 0x40, 0x40, sound_command_w },
    { -1 }  /* end of table */
};

static struct IOReadPort sound_readport[] =
{
    { 0x01, 0x01, soundlatch_r },
    { -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
    { 0x00, 0x00, AY8910_write_port_0_w },
    { 0x01, 0x01, AY8910_control_port_0_w },
    { 0x02, 0x02, AY8910_write_port_1_w },
    { 0x03, 0x03, AY8910_control_port_1_w },
    { 0x04, 0x04, sound_msm_w },
    { 0x05, 0x05, sound_control_w },
	{ -1 }	/* end of table */
};

/********************
* 1 Player Version  *
********************/

static struct MemoryReadAddress kc_readmem[] =
{
    { 0x0000, 0xbfff, MRA_ROM },
    { 0xc000, 0xdfff, MRA_RAM },
    { 0xe000, 0xe3ff, videoram_r },
    { 0xe400, 0xe7ff, colorram_r },
    { 0xea00, 0xeaff, spriteram_r },
    { 0xeb00, 0xffff, MRA_RAM },
    { -1 }
};

static struct MemoryWriteAddress kc_writemem[] =
{
    { 0x0000, 0xbfff, MWA_ROM },
    { 0xc000, 0xdfff, MWA_RAM },
    { 0xe000, 0xe3ff, videoram_w, &videoram, &videoram_size },
    { 0xe400, 0xe7ff, colorram_w, &colorram },
    { 0xea00, 0xeaff, spriteram_w, &spriteram, &spriteram_size },
	{ 0xeb00, 0xffff, MWA_RAM },
    { -1 }
};

static struct MemoryReadAddress kc_sound_readmem[] =
{
	{ 0x0000, 0xdfff, MRA_ROM },
	{ 0xe000, 0xe2ff, MRA_RAM },
	{ -1 }
};

static struct MemoryWriteAddress kc_sound_writemem[] =
{
	{ 0x0000, 0xdfff, MWA_ROM },
	{ 0xe000, 0xe2ff, MWA_RAM },
	{ -1 }
};

static int sound_reset_r( int offset ) {
    cpu_reset( 1 );
    return 0;
}

static void kc_sound_control_w( int offset, int data ) {
    if ( offset == 0 )
    	sound_nmi_enable = ( ( data >> 7 ) & 1 );
//    else
//		DAC_set_volume(0,( data == 1 ) ? 255 : 0,0);
}

static struct IOReadPort kc_readport[] =
{
    { 0x90, 0x90, input_port_0_r }, /* Player 1 controls - ACTIVE LOW */
    { 0x98, 0x98, input_port_1_r }, /* Player 2 controls - ACTIVE LOW */
    { 0xa0, 0xa0, input_port_2_r }, /* Coins & Start - ACTIVE LOW */
    { 0x80, 0x80, input_port_3_r }, /* Dipswitch */
    { 0xa8, 0xa8, sound_reset_r },
    { -1 }	/* end of table */
};

static struct IOWritePort kc_writeport[] =
{
    { 0x80, 0x80, MWA_NOP },
    { 0x81, 0x81, control_w },
    { 0xa8, 0xa8, sound_command_w },
    { -1 }  /* end of table */
};

static struct IOReadPort kc_sound_readport[] =
{
    { 0x06, 0x06, soundlatch_r },
    { -1 }	/* end of table */
};

static struct IOWritePort kc_sound_writeport[] =
{
    { 0x00, 0x00, AY8910_write_port_0_w },
    { 0x01, 0x01, AY8910_control_port_0_w },
    { 0x02, 0x02, AY8910_write_port_1_w },
    { 0x03, 0x03, AY8910_control_port_1_w },
    { 0x04, 0x04, DAC_data_w },
    { 0x05, 0x05, kc_sound_control_w },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( input_ports )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_4WAY )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_4WAY )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_4WAY )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_4WAY )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_4WAY )

        PORT_START      /* IN1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_PLAYER2 | IPF_4WAY )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_PLAYER2 | IPF_4WAY )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_PLAYER2 | IPF_4WAY )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_PLAYER2 | IPF_4WAY )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER2 | IPF_4WAY )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_PLAYER2 | IPF_4WAY )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_PLAYER2 | IPF_4WAY )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER2 | IPF_4WAY )

        PORT_START      /* IN2 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED  )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED  )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED  )

        PORT_START      /* DSW0 */
        PORT_DIPNAME( 0x03, 0x03, "Coin B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
        PORT_DIPNAME( 0x0c, 0x0c, "Coin A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
        PORT_DIPNAME( 0x30, 0x10, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x30, "Easy" )
        PORT_DIPSETTING(    0x20, "Medium" )
        PORT_DIPSETTING(    0x10, "Hard" )
        PORT_DIPSETTING(    0x00, "Hardest" )
        PORT_DIPNAME( 0x40, 0x00, "Demo Sounds", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPNAME( 0x80, 0x80, "Free Play", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "Off" )
        PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END


/********************
* 1 Player Version  *
********************/

INPUT_PORTS_START( kc_input_ports )
        PORT_START      /* IN0 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_4WAY )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_4WAY )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_4WAY )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_4WAY )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_4WAY )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_4WAY )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_4WAY )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_4WAY )

        PORT_START      /* IN1 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT | IPF_PLAYER2 | IPF_4WAY )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT | IPF_PLAYER2 | IPF_4WAY )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP | IPF_PLAYER2 | IPF_4WAY )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN | IPF_PLAYER2 | IPF_4WAY )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT | IPF_PLAYER2 | IPF_4WAY )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT | IPF_PLAYER2 | IPF_4WAY )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP | IPF_PLAYER2 | IPF_4WAY )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN | IPF_PLAYER2 | IPF_4WAY )

        PORT_START      /* IN2 */
        PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
        PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
        PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED  )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED  )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED  )

        PORT_START      /* DSW0 */
        PORT_DIPNAME( 0x03, 0x03, "Coin B", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
        PORT_DIPNAME( 0x0c, 0x0c, "Coin A", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
        PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
        PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
        PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
        PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x10, "Off" )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPNAME( 0x20, 0x20, "Free Play", IP_KEY_NONE )
        PORT_DIPSETTING(    0x20, "Off" )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPNAME( 0x40, 0x00, "Demo Sounds", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "Off" )
        PORT_DIPSETTING(    0x00, "On" )
        PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "Off" )
        PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

static struct GfxLayout tilelayout =
{
	8,8,	/* tile size */
	256*8,  /* number of tiles */
	2,	/* bits per pixel */
	{ 0x4000*8, 0 }, /* plane offsets */
	{ 0,1,2,3,4,5,6,7 }, /* x offsets */
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 }, /* y offsets */
	8*8      /* offset to next tile */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* tile size */
    512,  /* number of tiles */
	2,	/* bits per pixel */
    { 0xC000*8, 0 }, /* plane offsets */
	{ 0,1,2,3,4,5,6,7,
	  0x2000*8+0,0x2000*8+1,0x2000*8+2,0x2000*8+3,
	  0x2000*8+4,0x2000*8+5,0x2000*8+6,0x2000*8+7 }, /* x offsets */
    { 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,
      8*8,9*8,10*8,11*8,12*8,13*8,14*8, 15*8 }, /* y offsets */
        16*8     /* ofset to next tile */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        { 1, 0x18000, &tilelayout,      32*4, 32 },
        { 1, 0x08000, &spritelayout,    0, 16 },
        { 1, 0x04000, &spritelayout,    0, 16 },
        { 1, 0x00000, &spritelayout,	0, 16 },
        { -1 }
};



static int kc_interrupt( void ) {

        if ( nmi_enable )
                return Z80_NMI_INT;

        return Z80_IGNORE_INT;
}

static void msmint( int data ) {

	static int counter = 0;

	if ( msm_play_lo_nibble )
		MSM5205_data_w( 0, msm_data & 0x0f );
	else
		MSM5205_data_w( 0, ( msm_data >> 4 ) & 0x0f );

	msm_play_lo_nibble ^= 1;

	if ( !( counter ^= 1 ) ) {
		if ( sound_nmi_enable ) {
	                cpu_cause_interrupt( 1, Z80_NMI_INT );
	        }
	}
}

static struct AY8910interface ay8910_interface =
{
	2, /* 2 chips */
	1500000,     /* 12 Mhz / 8 = 1.5 Mhz */
	{ 50, 50 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct MSM5205interface msm_interface =
{
	1,              /* 1 chips */
	3906,           /* 12Mhz / 16 / 2 / 96 = 3906.25Hz playback */
	&msmint,        /* irq */
	{ 100 }
};

/********************
* 1 Player Version  *
********************/

static int sound_int( void ) {

	if ( sound_nmi_enable )
		return Z80_NMI_INT;

	return Z80_IGNORE_INT;
}

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};


static struct MachineDriver kchampvs_machine_driver =
{
        /* basic machine hardware */
        {
                {
                        CPU_Z80,
                        3000000,        /* 12Mhz / 4 = 3.0 Mhz */
                        0,
                        readmem,writemem,readport,writeport,
                        kc_interrupt,1
                },
                {
                        CPU_Z80 | CPU_AUDIO_CPU,
                        3000000,        /* 12Mhz / 4 = 3.0 Mhz */
                        3,
                        sound_readmem,sound_writemem,sound_readport,sound_writeport,
                        ignore_interrupt, 0
                        /* irq's triggered from main cpu */
                        /* nmi's from msm5205 */
                }
        },
        60,DEFAULT_60HZ_VBLANK_DURATION,
        1,     /* Interleaving forced by interrupts */
        0, /* init machine */

        /* video hardware */
        32*8, 32*8, { 0, 32*8-1, 2*8, 30*8-1 },
        gfxdecodeinfo,
        256, /* number of colors */
        256, /* color table length */
        kchamp_vh_convert_color_prom,

        VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
        0,
        kchampvs_vh_start,
        generic_vh_stop,
        kchamp_vh_screenrefresh,

        /* sound hardware */
        0,0,0,0,
        {
                {
                        SOUND_AY8910,
                        &ay8910_interface
                },
                {
                        SOUND_MSM5205,
                        &msm_interface
                }
        }
};

/********************
* 1 Player Version  *
********************/

static struct MachineDriver kchamp_machine_driver =
{
        /* basic machine hardware */
        {
                {
                        CPU_Z80,
                        3000000,        /* 12Mhz / 4 = 3.0 Mhz */
                        0,
                        kc_readmem, kc_writemem, kc_readport, kc_writeport,
                        kc_interrupt,1
                },
                {
                        CPU_Z80 | CPU_AUDIO_CPU,
                        3000000,        /* 12Mhz / 4 = 3.0 Mhz */
                        3,
                        kc_sound_readmem,kc_sound_writemem,kc_sound_readport,kc_sound_writeport,
                        ignore_interrupt, 0,
                        sound_int, 125 /* Hz */
                        /* irq's triggered from main cpu */
                        /* nmi's from 125 Hz clock */
                }
        },
        60,DEFAULT_60HZ_VBLANK_DURATION,
        1,     /* Interleaving forced by interrupts */
        0, /* init machine */

        /* video hardware */
        32*8, 32*8, { 0, 32*8-1, 2*8, 30*8-1 },
        gfxdecodeinfo,
        256, /* number of colors */
        256, /* color table length */
        kchamp_vh_convert_color_prom,

        VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
        0,
        kchamp1p_vh_start,
        generic_vh_stop,
        kchamp_vh_screenrefresh,

        /* sound hardware */
        0,0,0,0,
        {
                {
                        SOUND_AY8910,
                        &ay8910_interface
                },
                {
                        SOUND_DAC,
                        &dac_interface
                }
        }
};


/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( kchampvs_rom )
        ROM_REGION(0x10000)     /* 64k for code */
        ROM_LOAD( "bs24", 0x0000, 0x2000, 0x829da69b )
        ROM_LOAD( "bs23", 0x2000, 0x2000, 0x091f810e )
        ROM_LOAD( "bs22", 0x4000, 0x2000, 0xd4df2a52 )
        ROM_LOAD( "bs21", 0x6000, 0x2000, 0x3d4ef0da )
        ROM_LOAD( "bs20", 0x8000, 0x2000, 0x623a467b )
        ROM_LOAD( "bs19", 0xa000, 0x4000, 0x43e196c4 )

        ROM_REGION_DISPOSE(0x20000)
        ROM_LOAD( "bs00", 0x00000, 0x2000, 0x51eda56c )  /* top, plane0 */ /* sprites */
        ROM_LOAD( "bs06", 0x02000, 0x2000, 0x593264cf )  /* bot, plane0 */ /* sprites */
        ROM_LOAD( "bs01", 0x04000, 0x2000, 0xb4842ea9 )  /* top, plane0 */ /* sprites */
        ROM_LOAD( "bs07", 0x06000, 0x2000, 0x8cd166a5 )  /* bot, plane0 */ /* sprites */
        ROM_LOAD( "bs02", 0x08000, 0x2000, 0x4cbd3aa3 )  /* top, plane0 */ /* sprites */
        ROM_LOAD( "bs08", 0x0A000, 0x2000, 0x6be342a6 )  /* bot, plane0 */ /* sprites */
        ROM_LOAD( "bs03", 0x0C000, 0x2000, 0x8dcd271a )  /* top, plane1 */ /* sprites */
        ROM_LOAD( "bs09", 0x0E000, 0x2000, 0x4ee1dba7 )  /* bot, plane1 */ /* sprites */
        ROM_LOAD( "bs04", 0x10000, 0x2000, 0x7346db8a )  /* top, plane1 */ /* sprites */
        ROM_LOAD( "bs10", 0x12000, 0x2000, 0xb78714fc )  /* bot, plane1 */ /* sprites */
        ROM_LOAD( "bs05", 0x14000, 0x2000, 0xb2557102 )  /* top, plane1 */ /* sprites */
        ROM_LOAD( "bs11", 0x16000, 0x2000, 0xc85aba0e )  /* bot, plane1 */ /* sprites */
        ROM_LOAD( "bs12", 0x18000, 0x2000, 0x4c574ecd )  /* plane0 */ /* tiles */
        ROM_LOAD( "bs13", 0x1A000, 0x2000, 0x750b66af )  /* plane0 */ /* tiles */
        ROM_LOAD( "bs14", 0x1C000, 0x2000, 0x9ad6227c )  /* plane1 */ /* tiles */
        ROM_LOAD( "bs15", 0x1E000, 0x2000, 0x3b6d5de5 )  /* plane1 */ /* tiles */

        ROM_REGION(0x0300) /* color proms */
        ROM_LOAD( "br27", 0x0000, 0x0100, 0xf683c54a ) /* red */
        ROM_LOAD( "br26", 0x0100, 0x0100, 0x3ddbb6c4 ) /* green */
        ROM_LOAD( "br25", 0x0200, 0x0100, 0xba4a5651 ) /* blue */

        ROM_REGION(0x10000) /* Sound CPU */ /* 64k for code */
        ROM_LOAD( "bs18", 0x0000, 0x2000, 0xeaa646eb )
        ROM_LOAD( "bs17", 0x2000, 0x2000, 0xd71031ad ) /* adpcm */
        ROM_LOAD( "bs16", 0x4000, 0x2000, 0x6f811c43 ) /* adpcm */
ROM_END

ROM_START( karatedo_rom )
        ROM_REGION(0x10000)     /* 64k for code */
        ROM_LOAD( "br24", 0x0000, 0x2000, 0xea9cda49 )
        ROM_LOAD( "br23", 0x2000, 0x2000, 0x46074489 )
        ROM_LOAD( "br22", 0x4000, 0x2000, 0x294f67ba )
        ROM_LOAD( "br21", 0x6000, 0x2000, 0x934ea874 )
        ROM_LOAD( "br20", 0x8000, 0x2000, 0x97d7816a )
        ROM_LOAD( "br19", 0xa000, 0x4000, 0xdd2239d2 )

        ROM_REGION_DISPOSE(0x20000)
        ROM_LOAD( "br00", 0x00000, 0x2000, 0xc46a8b88 )  /* top, plane0 */ /* sprites */
        ROM_LOAD( "br06", 0x02000, 0x2000, 0xcf8982ff )  /* bot, plane0 */ /* sprites */
        ROM_LOAD( "bs01", 0x04000, 0x2000, 0xb4842ea9 )  /* top, plane0 */ /* sprites */
        ROM_LOAD( "bs07", 0x06000, 0x2000, 0x8cd166a5 )  /* bot, plane0 */ /* sprites */
        ROM_LOAD( "bs02", 0x08000, 0x2000, 0x4cbd3aa3 )  /* top, plane0 */ /* sprites */
        ROM_LOAD( "bs08", 0x0A000, 0x2000, 0x6be342a6 )  /* bot, plane0 */ /* sprites */
        ROM_LOAD( "br03", 0x0C000, 0x2000, 0xbde8a52b )  /* top, plane1 */ /* sprites */
        ROM_LOAD( "br09", 0x0E000, 0x2000, 0xe9a5f945 )  /* bot, plane1 */ /* sprites */
        ROM_LOAD( "bs04", 0x10000, 0x2000, 0x7346db8a )  /* top, plane1 */ /* sprites */
        ROM_LOAD( "bs10", 0x12000, 0x2000, 0xb78714fc )  /* bot, plane1 */ /* sprites */
        ROM_LOAD( "bs05", 0x14000, 0x2000, 0xb2557102 )  /* top, plane1 */ /* sprites */
        ROM_LOAD( "bs11", 0x16000, 0x2000, 0xc85aba0e )  /* bot, plane1 */ /* sprites */
        ROM_LOAD( "br12", 0x18000, 0x2000, 0x9ed6f00d )  /* plane0 */ /* tiles */
        ROM_LOAD( "bs13", 0x1A000, 0x2000, 0x750b66af )  /* plane0 */ /* tiles */
        ROM_LOAD( "br14", 0x1C000, 0x2000, 0xfc399229 )  /* plane1 */ /* tiles */
        ROM_LOAD( "bs15", 0x1E000, 0x2000, 0x3b6d5de5 )  /* plane1 */ /* tiles */

        ROM_REGION(0x0300) /* color proms */
        ROM_LOAD( "br27", 0x0000, 0x0100, 0xf683c54a ) /* red */
        ROM_LOAD( "br26", 0x0100, 0x0100, 0x3ddbb6c4 ) /* green */
        ROM_LOAD( "br25", 0x0200, 0x0100, 0xba4a5651 ) /* blue */

        ROM_REGION(0x10000) /* Sound CPU */ /* 64k for code */
        ROM_LOAD( "br18", 0x0000, 0x2000, 0x00ccb8ea )
        ROM_LOAD( "bs17", 0x2000, 0x2000, 0xd71031ad ) /* adpcm */
        ROM_LOAD( "br16", 0x4000, 0x2000, 0x2512d961 ) /* adpcm */
ROM_END

/********************
* 1 Player Version  *
********************/

ROM_START( kchamp_rom )
        ROM_REGION(0x10000)     /* 64k for code */
        ROM_LOAD( "B014.BIN", 0x0000, 0x2000, 0x0000d1a0 )
        ROM_LOAD( "B015.BIN", 0x2000, 0x2000, 0x03fae67e )
        ROM_LOAD( "B016.BIN", 0x4000, 0x2000, 0x3b6e1d08 )
        ROM_LOAD( "B017.BIN", 0x6000, 0x2000, 0xc1848d1a )
        ROM_LOAD( "B018.BIN", 0x8000, 0x2000, 0xb824abc7 )
        ROM_LOAD( "B019.BIN", 0xa000, 0x2000, 0x3b487a46 )

        ROM_REGION_DISPOSE(0x20000)
        ROM_LOAD( "B013.BIN", 0x00000, 0x2000, 0xeaad4168 )  /* top, plane0 */ /* sprites */
        ROM_LOAD( "B004.BIN", 0x02000, 0x2000, 0x10a47e2d )  /* bot, plane0 */ /* sprites */
        ROM_LOAD( "B012.BIN", 0x04000, 0x2000, 0xb4842ea9 )  /* top, plane0 */ /* sprites */
        ROM_LOAD( "B003.BIN", 0x06000, 0x2000, 0x8cd166a5 )  /* bot, plane0 */ /* sprites */
        ROM_LOAD( "B011.BIN", 0x08000, 0x2000, 0x4cbd3aa3 )  /* top, plane0 */ /* sprites */
        ROM_LOAD( "B002.BIN", 0x0A000, 0x2000, 0x6be342a6 )  /* bot, plane0 */ /* sprites */
        ROM_LOAD( "B007.BIN", 0x0C000, 0x2000, 0xcb91d16b )  /* top, plane1 */ /* sprites */
        ROM_LOAD( "B010.BIN", 0x0E000, 0x2000, 0x489c9c04 )  /* bot, plane1 */ /* sprites */
        ROM_LOAD( "B006.BIN", 0x10000, 0x2000, 0x7346db8a )  /* top, plane1 */ /* sprites */
        ROM_LOAD( "B009.BIN", 0x12000, 0x2000, 0xb78714fc )  /* bot, plane1 */ /* sprites */
        ROM_LOAD( "B005.BIN", 0x14000, 0x2000, 0xb2557102 )  /* top, plane1 */ /* sprites */
        ROM_LOAD( "B008.BIN", 0x16000, 0x2000, 0xc85aba0e )  /* bot, plane1 */ /* sprites */
        ROM_LOAD( "B000.BIN", 0x18000, 0x2000, 0xa4fa98a1 )  /* plane0 */ /* tiles */
        ROM_LOAD( "B001.BIN", 0x1C000, 0x2000, 0xfea09f7c )  /* plane1 */ /* tiles */

        ROM_REGION(0x0300) /* color proms */
        ROM_LOAD( "BR27", 0x0000, 0x0100, 0xf683c54a ) /* red */
        ROM_LOAD( "BR26", 0x0100, 0x0100, 0x3ddbb6c4 ) /* green */
        ROM_LOAD( "BR25", 0x0200, 0x0100, 0xba4a5651 ) /* blue */

        ROM_REGION(0x10000) /* Sound CPU */ /* 64k for code */
        ROM_LOAD( "B026.BIN", 0x0000, 0x2000, 0x999ed2c7 )
		ROM_LOAD( "B025.BIN", 0x2000, 0x2000, 0x33171e07 ) /* adpcm */
        ROM_LOAD( "B024.BIN", 0x4000, 0x2000, 0x910b48b9 ) /* adpcm */
		ROM_LOAD( "B023.BIN", 0x6000, 0x2000, 0x47f66aac )
		ROM_LOAD( "B022.BIN", 0x8000, 0x2000, 0x5928e749 )
		ROM_LOAD( "B021.BIN", 0xa000, 0x2000, 0xca17e3ba )
		ROM_LOAD( "B020.BIN", 0xc000, 0x2000, 0xada4f2cd )
ROM_END

static void kchamp_decode( void ) {
        /*
                Encryption consists of data lines scrambling
                bits have to be converted from 67852341 to 87654321

                Notice that the very first 2 opcodes that the program
                executes aint encrypted for some obscure reason.
        */

        static int encrypt_table_hi[] = {
                0x00, 0x10, 0x80, 0x90, 0x40, 0x50, 0xC0, 0xD0,
                0x20, 0x30, 0xA0, 0xB0, 0x60, 0x70, 0xE0, 0xF0
        };

        static int encrypt_table_lo[] = {
                0x00, 0x01, 0x08, 0x09, 0x04, 0x05, 0x0C, 0x0D,
                0x02, 0x03, 0x0A, 0x0B, 0x06, 0x07, 0x0E, 0x0F
        };

	unsigned char *RAM = Machine->memory_region[0];
        int A;

        for (A = 1;A < 0xE000;A++)
                ROM[A] = ( encrypt_table_hi[RAM[A] >> 4] ) | encrypt_table_lo[RAM[A] & 0x0f];

        /* Move the upper part of bs19 into $e000 */
        for (A = 0xC000;A < 0xE000;A++) {
                ROM[A+0x2000] = ( encrypt_table_hi[RAM[A] >> 4] ) | encrypt_table_lo[RAM[A] & 0x0f];
                RAM[A+0x2000] = RAM[A];
        }
}

static void kchampvs_decode( void ) {

        unsigned char *RAM = Machine ->memory_region[0];

        ROM[0] = RAM[0];
        RAM[1] = 0x6e;
        RAM[2] = 0xb4;
        kchamp_decode();
}

static void karatedo_decode( void ){

        unsigned char *RAM = Machine ->memory_region[0];

        ROM[0] = RAM[0];
        RAM[1] = 0x5f;
        RAM[2] = 0xb4;
        kchamp_decode();
}

static int kchampvs_hiload(void)
{
        unsigned char *RAM = Machine->memory_region[0];
        void *f;

        /* Wait for hiscore table initialization to be done. */
        if ( RAM[0xc0a8] != 0x2f )
                return 0;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0) {
                /* Load and set hiscore table. */
                osd_fread(f,&RAM[0xc040],0x6f);
		RAM[0xc0c0] = RAM[0xc040];
		RAM[0xc0c1] = RAM[0xc041];
		RAM[0xc0c2] = RAM[0xc042];
                osd_fclose(f);
        }
        return 1;
}

static void kchampvs_hisave(void)
{
        unsigned char *RAM = Machine->memory_region[0];
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0) {
                /* Write hiscore table. */
                osd_fwrite(f,&RAM[0xc040],0x6f);
                osd_fclose(f);
        }
}

/********************
* 1 Player Version  *
********************/
static int kchamp_hiload(void)
{
        unsigned char *RAM = Machine->memory_region[0];
        void *f;

        /* Wait for hiscore table initialization to be done. */
        if ( RAM[0xc0ab] != 0x01 )
                return 0;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0) {
                /* Load and set hiscore table. */
                osd_fread(f,&RAM[0xc040],0x6c);
		RAM[0xc0c0] = RAM[0xc040];
		RAM[0xc0c1] = RAM[0xc041];
		RAM[0xc0c2] = RAM[0xc042];
                osd_fclose(f);
        }
        return 1;
}

static void kchamp_hisave(void)
{
        unsigned char *RAM = Machine->memory_region[0];
        void *f;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0) {
                /* Write hiscore table. */
                osd_fwrite(f,&RAM[0xc040],0x6c);
                osd_fclose(f);
        }
}



struct GameDriver kchamp_driver =
{
	__FILE__,
	0,
	"kchamp",
	"Karate Champ",
	"1984",
	"Data East USA",
	"Ernesto Corvi\nGareth Hall\nCarlos Lozano\nHowie Cohen\nFrank Palazzolo",
	0,
	&kchamp_machine_driver,
	0,

	kchamp_rom,
	0, 0,
	0,
	0,

	kc_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	kchamp_hiload, kchamp_hisave
};

struct GameDriver kchampvs_driver =
{
	__FILE__,
	0,
	"kchampvs",
	"Karate Champ (VS version)",
	"1984",
	"Data East USA",
	"Ernesto Corvi\nGareth Hall\nCarlos Lozano\nHowie Cohen\nFrank Palazzolo",
	0,
	&kchampvs_machine_driver,
	0,

	kchampvs_rom,
	0, kchampvs_decode,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	kchampvs_hiload, kchampvs_hisave
};

struct GameDriver karatedo_driver =
{
	__FILE__,
	&kchampvs_driver,
	"karatedo",
	"Taisen Karate Dou (VS version)",
	"1984",
	"Data East Corporation",
	"Ernesto Corvi\nGareth Hall\nCarlos Lozano\nHowie Cohen\nFrank Palazzolo",
	0,
	&kchampvs_machine_driver,
	0,

	karatedo_rom,
	0, karatedo_decode,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_90,

	kchampvs_hiload, kchampvs_hisave
};
