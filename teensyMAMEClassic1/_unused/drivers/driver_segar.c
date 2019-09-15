/***************************************************************************

Sega G-80 Raster drivers

The G-80 Raster games are Astro Blaster, Monster Bash, 005, Space Odyssey,
and Pig Newton.

See also sega.c for the Sega G-80 Vector games.

Many thanks go to Dave Fish for the fine detective work he did into the
G-80 security chips (315-0064, 315-0070, 315-0076, 315-0082) which provided
me with enough information to emulate those chips at runtime along with
the 315-0062 Astro Blaster chip and the 315-0063 Space Odyssey chip.

Thanks also go to Paul Tonizzo, Clay Cowgill, John Bowes, and Kevin Klopp
for all the helpful information, samples, and schematics!

TODO:
- redo TMS3617 to use streams (and fix)
- fix Pig Newton colors
- fix Pig Newton hi load/save
- locate Pig Newton cocktail mode?
- verify Pig Newton DIPs
- attempt Pig Newton sound
- add Astro Blaster 8035 CPU

b4 - 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f
b5 - 28 20 22 44 1f 06 1c 1d 00 07 00 00 00 00 00 00

00 101 000
00 100 000
00 100 010
01 000 100
00 011 111
00 000 110
00 011 100
00 011 101

b9 - 80, 81, 82, 83
bb - seems to be a counter of how many times b9 loops back to 80

b8 - always 00
ba - usually 00, had some 01 when b9=82
bc - 00 at the start, gets 05 right before b9=83

(probably sound?)
be - 00 and 05 alternating until the end,
        there's a       04, 02, 06, 03, 01, 06, 01, 03, 05, 02, 07, 03,
                        07, 03, 07, 03, 07, 02, 05, 01, 04, 06, 01, 04,
                        06, 02, 04, 06, 01, 02, 03, 04, 05

- Mike Balfour (mab22@po.cwru.edu)
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* sndhrdw/segar.c */

extern void astrob_speech_port_w(int offset, int data);
extern void astrob_audio_ports_w(int offset, int data);
extern void spaceod_audio_ports_w(int offset, int data);
extern void monsterb_audio_8255_w(int offset, int data);
extern  int monsterb_audio_8255_r(int offset);

/* temporary speech handling through samples */
extern int astrob_speech_sh_start(void);
extern void astrob_speech_sh_update(void);

/* sndhrdw/monsterb.c */
extern int TMS3617_sh_start(void);
extern void TMS3617_sh_stop(void);
extern void TMS3617_sh_update(void);


extern const char *astrob_sample_names[];
extern const char *s005_sample_names[];
extern const char *monsterb_sample_names[];
extern const char *spaceod_sample_names[];

/* machine/segar.c */

extern void sega_security(int chip);
extern void segar_wr(int offset, int data);

extern unsigned char *segar_mem;

/* vidhrdw/segar.c */

extern unsigned char *segar_characterram;
extern unsigned char *segar_characterram2;
extern unsigned char *segar_mem_colortable;
extern unsigned char *segar_mem_bcolortable;
extern unsigned char *segar_mem_blookup;

extern void segar_init_colors(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern void segar_video_port_w(int offset,int data);
extern void segar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern void monsterb_back_port_w(int offset,int data);
extern void monsterb_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern int spaceod_vh_start(void);
extern void spaceod_vh_stop(void);
extern void spaceod_back_port_w(int offset,int data);
extern void spaceod_backshift_w(int offset,int data);
extern void spaceod_backshift_clear_w(int offset,int data);
extern void spaceod_backfill_w(int offset,int data);
extern void spaceod_nobackfill_w(int offset,int data);
extern void spaceod_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern void pignewt_back_color_w(int offset,int data);
extern void pignewt_back_ports_w(int offset,int data);
extern void pignewt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* drivers/segar.c */

static int segar_interrupt(void);
static int segar_read_ports(int offset);


static struct MemoryReadAddress readmem[] =
{
    { 0x0000, 0xC7FF, MRA_ROM },
        { 0xC800, 0xCFFF, MRA_RAM },    /* Misc RAM */
        { 0xE000, 0xE3FF, MRA_RAM, &videoram, &videoram_size },
        { 0xE400, 0xE7FF, MRA_RAM },  /* Used by at least Monster Bash? */
        { 0xE800, 0xEFFF, MRA_RAM, &segar_characterram },
        { 0xF000, 0xF03F, MRA_RAM, &segar_mem_colortable },     /* Dynamic color table */
        { 0xF040, 0xF07F, MRA_RAM, &segar_mem_bcolortable },    /* Dynamic color table for background (Monster Bash)*/
        { 0xF080, 0xF7FF, MRA_RAM },
        { 0xF800, 0xFFFF, MRA_RAM, &segar_characterram2 },
    {0x10000,0x13FFF, MRA_ROM, &segar_mem_blookup }, /* Background pattern ROMs (not all games use this) */
        { -1 }  /* end of table */
};


static struct MemoryWriteAddress writemem[] =
{
        { 0x0000, 0xFFFF, segar_wr, &segar_mem },
        { -1 }
};

static struct IOReadPort readport[] =
{
{0x3f, 0x3f, MRA_NOP }, /* Pig Newton - read from 1D87 */
        { 0x0e, 0x0e, monsterb_audio_8255_r },
        { 0xf8, 0xfc, segar_read_ports },
        { -1 }  /* end of table */
};

static struct IOWritePort astrob_writeport[] =
{
        { 0x38, 0x38, astrob_speech_port_w },
        { 0x3e, 0x3f, astrob_audio_ports_w },
        { 0xbf, 0xbf, segar_video_port_w }, /* bit0=cocktail flip, bit1=write to color RAM, bit2=always on?, bit6=write to background color RAM */
        { -1 }  /* end of table */
};

static struct IOWritePort spaceod_writeport[] =
{
        { 0x08, 0x08, spaceod_back_port_w },
        { 0x09, 0x09, spaceod_backshift_clear_w },
        { 0x0a, 0x0a, spaceod_backshift_w },
        { 0x0b, 0x0c, spaceod_nobackfill_w }, /* I'm not sure what these ports really do */
        { 0x0d, 0x0d, spaceod_backfill_w },
        { 0x0e, 0x0f, spaceod_audio_ports_w },
        { 0xbf, 0xbf, segar_video_port_w }, /* bit0=cocktail flip, bit1=write to color RAM, bit2=always on?, bit6=write to background color RAM */
        { -1 }  /* end of table */
};

static struct IOWritePort s005_writeport[] =
{
        { 0xbf, 0xbf, segar_video_port_w }, /* bit0=cocktail flip, bit1=write to color RAM, bit2=always on?, bit6=write to background color RAM */
        { -1 }  /* end of table */
};

static struct IOWritePort monsterb_writeport[] =
{
        { 0x0c, 0x0f, monsterb_audio_8255_w },
        { 0xbc, 0xbc, monsterb_back_port_w },
        { 0xbf, 0xbf, segar_video_port_w }, /* bit0=cocktail flip, bit1=write to color RAM, bit2=always on?, bit6=write to background color RAM */
        { -1 }  /* end of table */
};

static struct IOWritePort pignewt_writeport[] =
{
        { 0xb4, 0xb5, pignewt_back_color_w },   /* Just guessing */
        { 0xb8, 0xbc, pignewt_back_ports_w },   /* Just guessing */
        { 0xbe, 0xbe, MWA_NOP },                        /* probably some type of music register */
        { 0xbf, 0xbf, segar_video_port_w }, /* bit0=cocktail flip, bit1=write to color RAM, bit2=always on?, bit6=write to background color RAM */
        { -1 }  /* end of table */
};

static struct MemoryReadAddress speech_readmem[] =
{
        { 0x0000, 0x07ff, MRA_ROM },
        { -1 }  /* end of table */
};

static struct MemoryWriteAddress speech_writemem[] =
{
        { 0x0000, 0x07ff, MWA_ROM },
        { -1 }  /* end of table */
};

static struct IOReadPort speech_readport[] =
{
//      { 0x00,     0xff,     dkong_sh_gettune },
//      { I8039_p1, I8039_p1, dkong_sh_getp1 },
//      { I8039_p2, I8039_p2, dkong_sh_getp2 },
//      { I8039_t0, I8039_t0, dkong_sh_gett0 },
//      { I8039_t1, I8039_t1, dkong_sh_gett1 },
        { -1 }  /* end of table */
};

static struct IOWritePort speech_writeport[] =
{
//      { I8039_p1, I8039_p1, dkong_sh_putp1 },
//      { I8039_p2, I8039_p2, dkong_sh_putp2 },
        { -1 }  /* end of table */
};


/***************************************************************************

  The Sega games use NMI to trigger the self test. We use a fake input port to
  tie that event to a keypress.

***************************************************************************/
static int segar_interrupt(void)
{
        if (readinputport(5) & 1)       /* get status of the F2 key */
                return nmi_interrupt(); /* trigger self test */
        else return interrupt();
}

/***************************************************************************

  The Sega games store the DIP switches in a very mangled format that's
  not directly useable by MAME.  This function mangles the DIP switches
  into a format that can be used.

  Original format:
  Port 0 - 2-4, 2-8, 1-4, 1-8
  Port 1 - 2-3, 2-7, 1-3, 1-7
  Port 2 - 2-2, 2-6, 1-2, 1-6
  Port 3 - 2-1, 2-5, 1-1, 1-5
  MAME format:
  Port 6 - 1-1, 1-2, 1-3, 1-4, 1-5, 1-6, 1-7, 1-8
  Port 7 - 2-1, 2-2, 2-3, 2-4, 2-5, 2-6, 2-7, 2-8
***************************************************************************/
static int segar_read_ports(int offset)
{
        int dip1, dip2;

        dip1 = input_port_6_r(offset);
        dip2 = input_port_7_r(offset);

        switch(offset)
        {
                case 0:
                   return ((input_port_0_r(0) & 0xF0) | ((dip2 & 0x08)>>3) | ((dip2 & 0x80)>>6) |
                                                                                                ((dip1 & 0x08)>>1) | ((dip1 & 0x80)>>4));
                case 1:
                   return ((input_port_1_r(0) & 0xF0) | ((dip2 & 0x04)>>2) | ((dip2 & 0x40)>>5) |
                                                                                                ((dip1 & 0x04)>>0) | ((dip1 & 0x40)>>3));
                case 2:
                   return ((input_port_2_r(0) & 0xF0) | ((dip2 & 0x02)>>1) | ((dip2 & 0x20)>>4) |
                                                                                                ((dip1 & 0x02)<<1) | ((dip1 & 0x20)>>2));
                case 3:
                   return ((input_port_3_r(0) & 0xF0) | ((dip2 & 0x01)>>0) | ((dip2 & 0x10)>>3) |
                                                                                                ((dip1 & 0x01)<<2) | ((dip1 & 0x10)>>1));
                case 4:
                   return input_port_4_r(0);
        }

        return 0;
}
/***************************************************************************
Input Ports
***************************************************************************/

INPUT_PORTS_START( astrob_input_ports )
        PORT_START      /* IN0 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE,
                        IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
        PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
                        IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )

        PORT_START      /* IN1 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN2 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN3 */
        PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_BUTTON2, "Warp", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
        PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1, "Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN4 */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
        PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
        PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
        PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL, "Warp", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
        PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL, "Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
        PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL)
        PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_COCKTAIL)

        PORT_START      /* FAKE */
        /* This fake input port is used to get the status of the F2 key, */
        /* and activate the test mode, which is triggered by a NMI */
        PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

        PORT_START      /* FAKE */
        /* This fake input port is used for DIP Switch 1 */
        PORT_DIPNAME( 0x20, 0x20, "Orientation", IP_KEY_NONE )
        PORT_DIPSETTING(    0x20, "Upright" )
        PORT_DIPSETTING(    0x00, "Cocktail" )
        PORT_DIPNAME( 0xC0, 0xC0, "Number of Ships", IP_KEY_NONE )
        PORT_DIPSETTING(    0xC0, "5 Ships" )
        PORT_DIPSETTING(    0x40, "4 Ships" )
        PORT_DIPSETTING(    0x80, "3 Ships" )
        PORT_DIPSETTING(    0x00, "2 Ships" )

        PORT_START      /* FAKE */
        /* This fake input port is used for DIP Switch 2 */
        PORT_DIPNAME( 0x0F, 0x0C, "Coins/Credits (R)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x08, "3 / 1" )
        PORT_DIPSETTING(    0x04, "2 / 1" )
        PORT_DIPSETTING(    0x0C, "1 / 1" )
        PORT_DIPSETTING(    0x02, "1 / 2" )
        PORT_DIPSETTING(    0x0A, "1 / 3" )
        PORT_DIPSETTING(    0x06, "1 / 4" )
        PORT_DIPSETTING(    0x0E, "1 / 5" )
        PORT_DIPSETTING(    0x01, "1 / 6" )
        PORT_DIPSETTING(    0x09, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x05, "2/4 / 1/3" )
        PORT_DIPSETTING(    0x0D, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x03, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0x0B, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x07, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0x0F, "1/2/3/4 / 2/4/6/9" )

        PORT_DIPNAME( 0xF0, 0xC0, "Coins/Credits (L)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x80, "3 / 1" )
        PORT_DIPSETTING(    0x40, "2 / 1" )
        PORT_DIPSETTING(    0xC0, "1 / 1" )
        PORT_DIPSETTING(    0x20, "1 / 2" )
        PORT_DIPSETTING(    0xA0, "1 / 3" )
        PORT_DIPSETTING(    0x60, "1 / 4" )
        PORT_DIPSETTING(    0xE0, "1 / 5" )
        PORT_DIPSETTING(    0x10, "1 / 6" )
        PORT_DIPSETTING(    0x90, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x50, "2/4 / 1/3" )
        PORT_DIPSETTING(    0xD0, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x30, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0xB0, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x70, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0xF0, "1/2/3/4 / 2/4/6/9" )
INPUT_PORTS_END

INPUT_PORTS_START( s005_input_ports )
        PORT_START      /* IN0 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE,
        IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
        PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
        IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )

        PORT_START      /* IN1 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN2 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN3 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN4 */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
        PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
        PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_COCKTAIL )
        PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_COCKTAIL)
        PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL)
        PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL)
        PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_COCKTAIL)

        PORT_START      /* FAKE */
        /* This fake input port is used to get the status of the F2 key, */
        /* and activate the test mode, which is triggered by a NMI */
        PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

        PORT_START      /* FAKE */
        /* This fake input port is used for DIP Switch 1 */
        PORT_DIPNAME( 0x20, 0x20, "Orientation", IP_KEY_NONE )
        PORT_DIPSETTING(    0x20, "Upright" )
        PORT_DIPSETTING(    0x00, "Cocktail" )
        PORT_DIPNAME( 0xC0, 0x40, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(    0xC0, "6 Men" )
        PORT_DIPSETTING(    0x80, "5 Men" )
        PORT_DIPSETTING(    0x40, "4 Men" )
        PORT_DIPSETTING(    0x00, "3 Men" )

        PORT_START      /* FAKE */
        /* This fake input port is used for DIP Switch 2 */
        PORT_DIPNAME( 0x0F, 0x0C, "Coins/Credits (R)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x08, "3 / 1" )
        PORT_DIPSETTING(    0x04, "2 / 1" )
        PORT_DIPSETTING(    0x0C, "1 / 1" )
        PORT_DIPSETTING(    0x02, "1 / 2" )
        PORT_DIPSETTING(    0x0A, "1 / 3" )
        PORT_DIPSETTING(    0x06, "1 / 4" )
        PORT_DIPSETTING(    0x0E, "1 / 5" )
        PORT_DIPSETTING(    0x01, "1 / 6" )
        PORT_DIPSETTING(    0x09, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x05, "2/4 / 1/3" )
        PORT_DIPSETTING(    0x0D, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x03, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0x0B, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x07, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0x0F, "1/2/3/4 / 2/4/6/9" )

        PORT_DIPNAME( 0xF0, 0xC0, "Coins/Credits (L)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x80, "3 / 1" )
        PORT_DIPSETTING(    0x40, "2 / 1" )
        PORT_DIPSETTING(    0xC0, "1 / 1" )
        PORT_DIPSETTING(    0x20, "1 / 2" )
        PORT_DIPSETTING(    0xA0, "1 / 3" )
        PORT_DIPSETTING(    0x60, "1 / 4" )
        PORT_DIPSETTING(    0xE0, "1 / 5" )
        PORT_DIPSETTING(    0x10, "1 / 6" )
        PORT_DIPSETTING(    0x90, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x50, "2/4 / 1/3" )
        PORT_DIPSETTING(    0xD0, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x30, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0xB0, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x70, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0xF0, "1/2/3/4 / 2/4/6/9" )
INPUT_PORTS_END

INPUT_PORTS_START( monsterb_input_ports )
        PORT_START      /* IN0 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE,
        IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
        PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
        IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )

        PORT_START      /* IN1 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN2 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN3 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
        PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON1, "Zap", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN4 */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
        PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
        PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_COCKTAIL )
        PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_COCKTAIL)
        PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL, "Zap", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
        PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL)
        PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_COCKTAIL)

        PORT_START      /* FAKE */
        /* This fake input port is used to get the status of the F2 key, */
        /* and activate the test mode, which is triggered by a NMI */
        PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

        PORT_START      /* FAKE */
        /* This fake input port is used for DIP Switch 1 */
        PORT_DIPNAME( 0x01, 0x01, "Game Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x01, "Normal" )
        PORT_DIPSETTING(    0x00, "Endless" )
        PORT_DIPNAME( 0x06, 0x02, "Bonus Life", IP_KEY_NONE )
        PORT_DIPSETTING(    0x04, "10000" )
        PORT_DIPSETTING(    0x02, "20000" )
        PORT_DIPSETTING(    0x06, "40000" )
        PORT_DIPSETTING(    0x00, "None" )
        PORT_DIPNAME( 0x18, 0x08, "Difficulty", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "Easy" )
        PORT_DIPSETTING(    0x08, "Medium" )
        PORT_DIPSETTING(    0x10, "Hard" )
        PORT_DIPSETTING(    0x18, "Hardest" )
        PORT_DIPNAME( 0x20, 0x20, "Cabinet", IP_KEY_NONE )
        PORT_DIPSETTING(    0x20, "Upright" )
        PORT_DIPSETTING(    0x00, "Cocktail" )
        PORT_DIPNAME( 0xC0, 0x40, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "3" )
        PORT_DIPSETTING(    0x40, "4" )
        PORT_DIPSETTING(    0x80, "5" )
        PORT_DIPSETTING(    0xC0, "6" )

        PORT_START      /* FAKE */
        /* This fake input port is used for DIP Switch 2 */
        PORT_DIPNAME( 0x0F, 0x0C, "Coins/Credits (R)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x08, "3 / 1" )
        PORT_DIPSETTING(    0x04, "2 / 1" )
        PORT_DIPSETTING(    0x0C, "1 / 1" )
        PORT_DIPSETTING(    0x02, "1 / 2" )
        PORT_DIPSETTING(    0x0A, "1 / 3" )
        PORT_DIPSETTING(    0x06, "1 / 4" )
        PORT_DIPSETTING(    0x0E, "1 / 5" )
        PORT_DIPSETTING(    0x01, "1 / 6" )
        PORT_DIPSETTING(    0x09, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x05, "2/4 / 1/3" )
        PORT_DIPSETTING(    0x0D, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x03, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0x0B, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x07, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0x0F, "1/2/3/4 / 2/4/6/9" )

        PORT_DIPNAME( 0xF0, 0xC0, "Coins/Credits (L)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x80, "3 / 1" )
        PORT_DIPSETTING(    0x40, "2 / 1" )
        PORT_DIPSETTING(    0xC0, "1 / 1" )
        PORT_DIPSETTING(    0x20, "1 / 2" )
        PORT_DIPSETTING(    0xA0, "1 / 3" )
        PORT_DIPSETTING(    0x60, "1 / 4" )
        PORT_DIPSETTING(    0xE0, "1 / 5" )
        PORT_DIPSETTING(    0x10, "1 / 6" )
        PORT_DIPSETTING(    0x90, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x50, "2/4 / 1/3" )
        PORT_DIPSETTING(    0xD0, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x30, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0xB0, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x70, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0xF0, "1/2/3/4 / 2/4/6/9" )
INPUT_PORTS_END

INPUT_PORTS_START( spaceod_input_ports )
        PORT_START      /* IN0 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE,
        IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
        PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
        IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )

        PORT_START      /* IN1 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN2 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_BUTTON2, "Bomb", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN3 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN4 */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
        PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
        PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
        PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
        PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
        PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL)
        PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_COCKTAIL)

        PORT_START      /* FAKE */
        /* This fake input port is used to get the status of the F2 key, */
        /* and activate the test mode, which is triggered by a NMI */
        PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

        PORT_START      /* FAKE */
        /* This fake input port is used for DIP Switch 1 */
        PORT_DIPNAME( 0x01, 0x01, "Game Mode", IP_KEY_NONE )
        PORT_DIPSETTING(    0x01, "Normal" )
        PORT_DIPSETTING(    0x00, "Endless" )
        PORT_DIPNAME( 0x18, 0x08, "Extra Ship @", IP_KEY_NONE )
        PORT_DIPSETTING(    0x18, "80,000 Points" )
        PORT_DIPSETTING(    0x10, "60,000 Points" )
        PORT_DIPSETTING(    0x08, "40,000 Points" )
        PORT_DIPSETTING(    0x00, "20,000 Points" )
        PORT_DIPNAME( 0x20, 0x20, "Orientation", IP_KEY_NONE )
        PORT_DIPSETTING(    0x20, "Upright" )
        PORT_DIPSETTING(    0x00, "Cocktail" )
        PORT_DIPNAME( 0xC0, 0x40, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(    0xC0, "6 Ships" )
        PORT_DIPSETTING(    0x80, "5 Ships" )
        PORT_DIPSETTING(    0x40, "4 Ships" )
        PORT_DIPSETTING(    0x00, "3 Ships" )

        PORT_START      /* FAKE */
        /* This fake input port is used for DIP Switch 2 */
        PORT_DIPNAME( 0x0F, 0x0C, "Coins/Credits (R)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x08, "3 / 1" )
        PORT_DIPSETTING(    0x04, "2 / 1" )
        PORT_DIPSETTING(    0x0C, "1 / 1" )
        PORT_DIPSETTING(    0x02, "1 / 2" )
        PORT_DIPSETTING(    0x0A, "1 / 3" )
        PORT_DIPSETTING(    0x06, "1 / 4" )
        PORT_DIPSETTING(    0x0E, "1 / 5" )
        PORT_DIPSETTING(    0x01, "1 / 6" )
        PORT_DIPSETTING(    0x09, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x05, "2/4 / 1/3" )
        PORT_DIPSETTING(    0x0D, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x03, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0x0B, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x07, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0x0F, "1/2/3/4 / 2/4/6/9" )

        PORT_DIPNAME( 0xF0, 0xC0, "Coins/Credits (L)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x80, "3 / 1" )
        PORT_DIPSETTING(    0x40, "2 / 1" )
        PORT_DIPSETTING(    0xC0, "1 / 1" )
        PORT_DIPSETTING(    0x20, "1 / 2" )
        PORT_DIPSETTING(    0xA0, "1 / 3" )
        PORT_DIPSETTING(    0x60, "1 / 4" )
        PORT_DIPSETTING(    0xE0, "1 / 5" )
        PORT_DIPSETTING(    0x10, "1 / 6" )
        PORT_DIPSETTING(    0x90, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x50, "2/4 / 1/3" )
        PORT_DIPSETTING(    0xD0, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x30, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0xB0, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x70, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0xF0, "1/2/3/4 / 2/4/6/9" )
INPUT_PORTS_END

INPUT_PORTS_START( pignewt_input_ports )
        PORT_START      /* IN0 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE,
        IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
        PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
        IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )

        PORT_START      /* IN1 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN2 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN3 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN4 */
        PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
        PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
        PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
        PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_COCKTAIL )
        PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_COCKTAIL)
        PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL)
        PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL)
        PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_COCKTAIL)

        PORT_START      /* FAKE */
        /* This fake input port is used to get the status of the F2 key, */
        /* and activate the test mode, which is triggered by a NMI */
        PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

        PORT_START      /* FAKE */
        /* This fake input port is used for DIP Switch 1 */
        PORT_DIPNAME( 0x01, 0x01, "Unknown 1", IP_KEY_NONE )
        PORT_DIPSETTING(    0x01, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPNAME( 0x02, 0x02, "Unknown 2", IP_KEY_NONE )
        PORT_DIPSETTING(    0x02, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPNAME( 0x04, 0x04, "Unknown 3", IP_KEY_NONE )
        PORT_DIPSETTING(    0x04, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPNAME( 0x08, 0x08, "Unknown 4", IP_KEY_NONE )
        PORT_DIPSETTING(    0x08, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPNAME( 0x30, 0x20, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(    0x30, "6 Pigs" )
        PORT_DIPSETTING(    0x20, "5 Pigs" )
        PORT_DIPSETTING(    0x10, "4 Pigs" )
        PORT_DIPSETTING(    0x00, "3 Pigs" )
        PORT_DIPNAME( 0x40, 0x40, "Unknown 5", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPNAME( 0x80, 0x80, "Unknown 6", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "On" )
        PORT_DIPSETTING(    0x00, "Off" )

        PORT_START      /* FAKE */
        /* This fake input port is used for DIP Switch 2 */
        PORT_DIPNAME( 0x0F, 0x0C, "Coins/Credits (R)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x08, "3 / 1" )
        PORT_DIPSETTING(    0x04, "2 / 1" )
        PORT_DIPSETTING(    0x0C, "1 / 1" )
        PORT_DIPSETTING(    0x02, "1 / 2" )
        PORT_DIPSETTING(    0x0A, "1 / 3" )
        PORT_DIPSETTING(    0x06, "1 / 4" )
        PORT_DIPSETTING(    0x0E, "1 / 5" )
        PORT_DIPSETTING(    0x01, "1 / 6" )
        PORT_DIPSETTING(    0x09, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x05, "2/4 / 1/3" )
        PORT_DIPSETTING(    0x0D, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x03, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0x0B, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x07, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0x0F, "1/2/3/4 / 2/4/6/9" )

        PORT_DIPNAME( 0xF0, 0xC0, "Coins/Credits (L)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x80, "3 / 1" )
        PORT_DIPSETTING(    0x40, "2 / 1" )
        PORT_DIPSETTING(    0xC0, "1 / 1" )
        PORT_DIPSETTING(    0x20, "1 / 2" )
        PORT_DIPSETTING(    0xA0, "1 / 3" )
        PORT_DIPSETTING(    0x60, "1 / 4" )
        PORT_DIPSETTING(    0xE0, "1 / 5" )
        PORT_DIPSETTING(    0x10, "1 / 6" )
        PORT_DIPSETTING(    0x90, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x50, "2/4 / 1/3" )
        PORT_DIPSETTING(    0xD0, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x30, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0xB0, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x70, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0xF0, "1/2/3/4 / 2/4/6/9" )
INPUT_PORTS_END

INPUT_PORTS_START( pignewta_input_ports )
        PORT_START      /* IN0 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 )
        PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE,
        IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )
        PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE,
        IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT, 3 )

        PORT_START      /* IN1 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1 )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN2 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN3 */
        PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

        PORT_START      /* IN4 */
        PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )
        PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
        PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
        PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
        PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )

        PORT_START      /* FAKE */
        /* This fake input port is used to get the status of the F2 key, */
        /* and activate the test mode, which is triggered by a NMI */
        PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )

        PORT_START      /* FAKE */
        /* This fake input port is used for DIP Switch 1 */
        PORT_DIPNAME( 0x01, 0x01, "Unknown 1", IP_KEY_NONE )
        PORT_DIPSETTING(    0x01, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPNAME( 0x02, 0x02, "Unknown 2", IP_KEY_NONE )
        PORT_DIPSETTING(    0x02, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPNAME( 0x04, 0x04, "Unknown 3", IP_KEY_NONE )
        PORT_DIPSETTING(    0x04, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPNAME( 0x08, 0x08, "Unknown 4", IP_KEY_NONE )
        PORT_DIPSETTING(    0x08, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPNAME( 0x30, 0x20, "Lives", IP_KEY_NONE )
        PORT_DIPSETTING(    0x30, "6 Pigs" )
        PORT_DIPSETTING(    0x20, "5 Pigs" )
        PORT_DIPSETTING(    0x10, "4 Pigs" )
        PORT_DIPSETTING(    0x00, "3 Pigs" )
        PORT_DIPNAME( 0x40, 0x40, "Unknown 5", IP_KEY_NONE )
        PORT_DIPSETTING(    0x40, "On" )
        PORT_DIPSETTING(    0x00, "Off" )
        PORT_DIPNAME( 0x80, 0x80, "Unknown 6", IP_KEY_NONE )
        PORT_DIPSETTING(    0x80, "On" )
        PORT_DIPSETTING(    0x00, "Off" )

        PORT_START      /* FAKE */
        /* This fake input port is used for DIP Switch 2 */
        PORT_DIPNAME( 0x0F, 0x0C, "Coins/Credits (R)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x08, "3 / 1" )
        PORT_DIPSETTING(    0x04, "2 / 1" )
        PORT_DIPSETTING(    0x0C, "1 / 1" )
        PORT_DIPSETTING(    0x02, "1 / 2" )
        PORT_DIPSETTING(    0x0A, "1 / 3" )
        PORT_DIPSETTING(    0x06, "1 / 4" )
        PORT_DIPSETTING(    0x0E, "1 / 5" )
        PORT_DIPSETTING(    0x01, "1 / 6" )
        PORT_DIPSETTING(    0x09, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x05, "2/4 / 1/3" )
        PORT_DIPSETTING(    0x0D, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x03, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0x0B, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x07, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0x0F, "1/2/3/4 / 2/4/6/9" )

        PORT_DIPNAME( 0xF0, 0xC0, "Coins/Credits (L)", IP_KEY_NONE )
        PORT_DIPSETTING(    0x00, "4 / 1" )
        PORT_DIPSETTING(    0x80, "3 / 1" )
        PORT_DIPSETTING(    0x40, "2 / 1" )
        PORT_DIPSETTING(    0xC0, "1 / 1" )
        PORT_DIPSETTING(    0x20, "1 / 2" )
        PORT_DIPSETTING(    0xA0, "1 / 3" )
        PORT_DIPSETTING(    0x60, "1 / 4" )
        PORT_DIPSETTING(    0xE0, "1 / 5" )
        PORT_DIPSETTING(    0x10, "1 / 6" )
        PORT_DIPSETTING(    0x90, "2/4/5 / 1/2/3" )
        PORT_DIPSETTING(    0x50, "2/4 / 1/3" )
        PORT_DIPSETTING(    0xD0, "1/2/3/4/5 / 1/2/3/4/6" )
        PORT_DIPSETTING(    0x30, "1/2/3/4 / 1/2/3/5" )
        PORT_DIPSETTING(    0xB0, "1/2 / 1/3" )
        PORT_DIPSETTING(    0x70, "1/2/3/4/5 / 2/4/6/8/11" )
        PORT_DIPSETTING(    0xF0, "1/2/3/4 / 2/4/6/9" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
        8,8,    /* 8*8 characters */
        256,    /* 256 characters? */
        2,      /* 2 bits per pixel */
        { 0, 0x1000*8 },        /* separated by 0x1000 bytes */
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
        { 7, 6, 5, 4, 3, 2, 1, 0 },     /* pretty straightforward layout */
        8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout backlayout =
{
        8,8,    /* 8*8 characters */
        256*4,  /* 256 characters per scene, 4 scenes */
        2,      /* 2 bits per pixel */
        { 0, 0x2000*8 },        /* separated by 0x2000 bytes */
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
        { 7, 6, 5, 4, 3, 2, 1, 0 },     /* pretty straightforward layout */
        8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout spacelayout =
{
        8,8,   /* 16*8 characters */
        256*2,    /* 256 characters */
        6,      /* 6 bits per pixel */
        { 0, 0x1000*8, 0x2000*8, 0x3000*8, 0x4000*8, 0x5000*8 },        /* separated by 0x1000 bytes (1 EPROM per bit) */
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
        { 7, 6, 5, 4, 3, 2, 1, 0 },     /* pretty straightforward layout */
        8*8     /* every char takes 16 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
        { 0, 0xE800, &charlayout, 0x01, 0x10 }, /* offset into colors, # of colors */
        { -1 } /* end of array */
};

static struct GfxDecodeInfo monsterb_gfxdecodeinfo[] =
{
        { 0, 0xE800, &charlayout, 0x01, 0x10 }, /* offset into colors, # of colors */
        { 1, 0x0000, &backlayout, 0x41, 0x10 }, /* offset into colors, # of colors */
        { -1 } /* end of array */
};

static struct GfxDecodeInfo spaceod_gfxdecodeinfo[] =
{
        { 0, 0xE800, &charlayout,   0x01, 0x10 }, /* offset into colors, # of colors */
        { 1, 0x0000, &spacelayout,  0x41, 1 }, /* offset into colors, # of colors */
        { -1 } /* end of array */
};



/***************************************************************************
  Game ROMs
***************************************************************************/

ROM_START( astrob_rom )
        ROM_REGION(0x14000)     /* 64k for code + space for background (unused) */
        ROM_LOAD( "829b",         0x0000, 0x0800, 0x14ae953c ) /* U25 */
        ROM_LOAD( "888",          0x0800, 0x0800, 0x42601744 ) /* U1 */
        ROM_LOAD( "889",          0x1000, 0x0800, 0xdd9ab173 ) /* U2 */
        ROM_LOAD( "890",          0x1800, 0x0800, 0x26f5b4cf ) /* U3 */
        ROM_LOAD( "891",          0x2000, 0x0800, 0x6437c95f ) /* U4 */
        ROM_LOAD( "892",          0x2800, 0x0800, 0x2d3c949b ) /* U5 */
        ROM_LOAD( "893",          0x3000, 0x0800, 0xccdb1a76 ) /* U6 */
        ROM_LOAD( "894",          0x3800, 0x0800, 0x66ae5ced ) /* U7 */
        ROM_LOAD( "895",          0x4000, 0x0800, 0x202cf3a3 ) /* U8 */
        ROM_LOAD( "896",          0x4800, 0x0800, 0xb603fe23 ) /* U9 */
        ROM_LOAD( "897",          0x5000, 0x0800, 0x989198c6 ) /* U10 */
        ROM_LOAD( "898",          0x5800, 0x0800, 0xef2bab04 ) /* U11 */
        ROM_LOAD( "899",          0x6000, 0x0800, 0xe0d189ee ) /* U12 */
        ROM_LOAD( "900",          0x6800, 0x0800, 0x682d4604 ) /* U13 */
        ROM_LOAD( "901",          0x7000, 0x0800, 0x9ed11c61 ) /* U14 */
        ROM_LOAD( "902",          0x7800, 0x0800, 0xb4d6c330 ) /* U15 */
        ROM_LOAD( "903",          0x8000, 0x0800, 0x84acc38c ) /* U16 */
        ROM_LOAD( "904",          0x8800, 0x0800, 0x5eba3097 ) /* U16 */
        ROM_LOAD( "905",          0x9000, 0x0800, 0x4f08f9f4 ) /* U16 */
        ROM_LOAD( "906",          0x9800, 0x0800, 0x58149df1 ) /* U16 */

        ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
        /* empty memory region - not used by the game, but needed because the main */
        /* core currently always frees region #1 after initialization. */

        ROM_REGION(0x10000)     /* 64k for speech code */
        ROM_LOAD( "808b",         0x0000, 0x0800, 0x5988c767 ) /* U7 */
        ROM_LOAD( "809a",         0x0800, 0x0800, 0x893f228d ) /* U6 */
        ROM_LOAD( "810",          0x1000, 0x0800, 0xff0163c5 ) /* U5 */
        ROM_LOAD( "811",          0x1800, 0x0800, 0x219f3978 ) /* U4 */
        ROM_LOAD( "812a",         0x2000, 0x0800, 0x410ad0d2 ) /* U3 */
ROM_END

ROM_START( astrob1_rom )
        ROM_REGION(0x14000)     /* 64k for code + space for background (unused) */
        ROM_LOAD( "829",          0x0000, 0x0800, 0x5f66046e ) /* U25 */
        ROM_LOAD( "837",          0x0800, 0x0800, 0xce9c3763 ) /* U1 */
        ROM_LOAD( "838",          0x1000, 0x0800, 0x3557289e ) /* U2 */
        ROM_LOAD( "839",          0x1800, 0x0800, 0xc88bda24 ) /* U3 */
        ROM_LOAD( "840",          0x2000, 0x0800, 0x24c9fe23 ) /* U4 */
        ROM_LOAD( "841",          0x2800, 0x0800, 0xf153c683 ) /* U5 */
        ROM_LOAD( "842",          0x3000, 0x0800, 0x4c5452b2 ) /* U6 */
        ROM_LOAD( "843",          0x3800, 0x0800, 0x673161a6 ) /* U7 */
        ROM_LOAD( "844",          0x4000, 0x0800, 0x6bfc59fd ) /* U8 */
        ROM_LOAD( "845",          0x4800, 0x0800, 0x018623f3 ) /* U9 */
        ROM_LOAD( "846",          0x5000, 0x0800, 0x4d7c5fb3 ) /* U10 */
        ROM_LOAD( "847",          0x5800, 0x0800, 0x24d1d50a ) /* U11 */
        ROM_LOAD( "848",          0x6000, 0x0800, 0x1c145541 ) /* U12 */
        ROM_LOAD( "849",          0x6800, 0x0800, 0xd378c169 ) /* U13 */
        ROM_LOAD( "850",          0x7000, 0x0800, 0x9da673ae ) /* U14 */
        ROM_LOAD( "851",          0x7800, 0x0800, 0x3d4cf9f0 ) /* U15 */
        ROM_LOAD( "852",          0x8000, 0x0800, 0xaf88a97e ) /* U16 */

        ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
        /* empty memory region - not used by the game, but needed because the main */
        /* core currently always frees region #1 after initialization. */

        ROM_REGION(0x10000)     /* 64k for speech code */
        ROM_LOAD( "808b",         0x0000, 0x0800, 0x5988c767 ) /* U7 */
        ROM_LOAD( "809a",         0x0800, 0x0800, 0x893f228d ) /* U6 */
        ROM_LOAD( "810",          0x1000, 0x0800, 0xff0163c5 ) /* U5 */
        ROM_LOAD( "811",          0x1800, 0x0800, 0x219f3978 ) /* U4 */
        ROM_LOAD( "812a",         0x2000, 0x0800, 0x410ad0d2 ) /* U3 */
ROM_END

ROM_START( s005_rom )
        ROM_REGION(0x14000)     /* 64k for code + space for background (unused) */
        ROM_LOAD( "1346b.u25",    0x0000, 0x0800, 0x8e68533e ) /* U25 */
        ROM_LOAD( "5092.u1",      0x0800, 0x0800, 0x29e10a81 ) /* U1 */
        ROM_LOAD( "5093.u2",      0x1000, 0x0800, 0xe1edc3df ) /* U2 */
        ROM_LOAD( "5094.u3",      0x1800, 0x0800, 0x995773bb ) /* U3 */
        ROM_LOAD( "5095.u4",      0x2000, 0x0800, 0xf887f575 ) /* U4 */
        ROM_LOAD( "5096.u5",      0x2800, 0x0800, 0x5545241e ) /* U5 */
        ROM_LOAD( "5097.u6",      0x3000, 0x0800, 0x428edb54 ) /* U6 */
        ROM_LOAD( "5098.u7",      0x3800, 0x0800, 0x5bcb9d63 ) /* U7 */
        ROM_LOAD( "5099.u8",      0x4000, 0x0800, 0x0ea24ba3 ) /* U8 */
        ROM_LOAD( "5100.u9",      0x4800, 0x0800, 0xa79af131 ) /* U9 */
        ROM_LOAD( "5101.u10",     0x5000, 0x0800, 0x8a1cdae0 ) /* U10 */
        ROM_LOAD( "5102.u11",     0x5800, 0x0800, 0x70826a15 ) /* U11 */
        ROM_LOAD( "5103.u12",     0x6000, 0x0800, 0x7f80c5b0 ) /* U12 */
        ROM_LOAD( "5104.u13",     0x6800, 0x0800, 0x0140930e ) /* U13 */
        ROM_LOAD( "5105.u14",     0x7000, 0x0800, 0x17807a05 ) /* U14 */
        ROM_LOAD( "5106.u15",     0x7800, 0x0800, 0xc7cdfa9d ) /* U15 */
        ROM_LOAD( "5107.u16",     0x8000, 0x0800, 0x95f8a2e6 ) /* U16 */
        ROM_LOAD( "5108.u17",     0x8800, 0x0800, 0xd371cacd ) /* U17 */
        ROM_LOAD( "5109.u18",     0x9000, 0x0800, 0x48a20617 ) /* U18 */
        ROM_LOAD( "5110.u19",     0x9800, 0x0800, 0x7d26111a ) /* U19 */
        ROM_LOAD( "5111.u20",     0xA000, 0x0800, 0xa888e175 ) /* U20 */

        ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
        /* empty memory region - not used by the game, but needed because the main */
        /* core currently always frees region #1 after initialization. */

        ROM_REGION(0x800)       /* 2k for sound */
        ROM_LOAD( "epr-1286.16",  0x0000, 0x0800, 0xfbe0d501 )
ROM_END

ROM_START( monsterb_rom )
        ROM_REGION(0x14000)     /* 64k for code + space for background */
        ROM_LOAD( "1778cpu.bin",  0x0000, 0x0800, 0x19761be3 ) /* U25 */
        ROM_LOAD( "1779.bin",     0x0800, 0x0800, 0x5b67dc4c ) /* U1 */
        ROM_LOAD( "1780.bin",     0x1000, 0x0800, 0xfac5aac6 ) /* U2 */
        ROM_LOAD( "1781.bin",     0x1800, 0x0800, 0x3b104103 ) /* U3 */
        ROM_LOAD( "1782.bin",     0x2000, 0x0800, 0xc1523553 ) /* U4 */
        ROM_LOAD( "1783.bin",     0x2800, 0x0800, 0xe0ea08c5 ) /* U5 */
        ROM_LOAD( "1784.bin",     0x3000, 0x0800, 0x48976d11 ) /* U6 */
        ROM_LOAD( "1785.bin",     0x3800, 0x0800, 0x297d33ae ) /* U7 */
        ROM_LOAD( "1786.bin",     0x4000, 0x0800, 0xef94c8f4 ) /* U8 */
        ROM_LOAD( "1787.bin",     0x4800, 0x0800, 0x1b62994e ) /* U9 */
        ROM_LOAD( "1788.bin",     0x5000, 0x0800, 0xa2e32d91 ) /* U10 */
        ROM_LOAD( "1789.bin",     0x5800, 0x0800, 0x08a172dc ) /* U11 */
        ROM_LOAD( "1790.bin",     0x6000, 0x0800, 0x4e320f9d ) /* U12 */
        ROM_LOAD( "1791.bin",     0x6800, 0x0800, 0x3b4cba31 ) /* U13 */
        ROM_LOAD( "1792.bin",     0x7000, 0x0800, 0x7707b9f8 ) /* U14 */
        ROM_LOAD( "1793.bin",     0x7800, 0x0800, 0xa5d05155 ) /* U15 */
        ROM_LOAD( "1794.bin",     0x8000, 0x0800, 0xe4813da9 ) /* U16 */
        ROM_LOAD( "1795.bin",     0x8800, 0x0800, 0x4cd6ed88 ) /* U17 */
        ROM_LOAD( "1796.bin",     0x9000, 0x0800, 0x9f141a42 ) /* U18 */
        ROM_LOAD( "1797.bin",     0x9800, 0x0800, 0xec14ad16 ) /* U19 */
        ROM_LOAD( "1798.bin",     0xA000, 0x0800, 0x86743a4f ) /* U20 */
        ROM_LOAD( "1799.bin",     0xA800, 0x0800, 0x41198a83 ) /* U21 */
        ROM_LOAD( "1800.bin",     0xB000, 0x0800, 0x6a062a04 ) /* U22 */
        ROM_LOAD( "1801.bin",     0xB800, 0x0800, 0xf38488fe ) /* U23 */
        /* Background pattern ROM */
        ROM_LOAD( "1518a.bin",   0x10000, 0x2000, 0x2d5932fe ) /* ??? */

        ROM_REGION_DISPOSE(0x4000)      /* 16k for background graphics */
        ROM_LOAD( "1516.bin",     0x0000, 0x2000, 0xe93a2281 ) /* ??? */
        ROM_LOAD( "1517.bin",     0x2000, 0x2000, 0x1e589101 ) /* ??? */

        ROM_REGION(0x2000)      /* 8k for sound */
        ROM_LOAD( "1543snd.bin",  0x0000, 0x1000, 0xb525ce8f ) /* ??? */
        ROM_LOAD( "1544snd.bin",  0x1000, 0x1000, 0x56c79fb0 ) /* ??? */
ROM_END

ROM_START( spaceod_rom )
        ROM_REGION(0x14000)     /* 64k for code + space for background */
        ROM_LOAD( "so-959.dat",   0x0000, 0x0800, 0xbbae3cd1 ) /* U25 */
        ROM_LOAD( "so-941.dat",   0x0800, 0x0800, 0x8b63585a ) /* U1 */
        ROM_LOAD( "so-942.dat",   0x1000, 0x0800, 0x93e7d900 ) /* U2 */
        ROM_LOAD( "so-943.dat",   0x1800, 0x0800, 0xe2f5dc10 ) /* U3 */
        ROM_LOAD( "so-944.dat",   0x2000, 0x0800, 0xb5ab01e9 ) /* U4 */
        ROM_LOAD( "so-945.dat",   0x2800, 0x0800, 0x6c5fa1b1 ) /* U5 */
        ROM_LOAD( "so-946.dat",   0x3000, 0x0800, 0x4cef25d6 ) /* U6 */
        ROM_LOAD( "so-947.dat",   0x3800, 0x0800, 0x7220fc42 ) /* U7 */
        ROM_LOAD( "so-948.dat",   0x4000, 0x0800, 0x94bcd726 ) /* U8 */
        ROM_LOAD( "so-949.dat",   0x4800, 0x0800, 0xe11e7034 ) /* U9 */
        ROM_LOAD( "so-950.dat",   0x5000, 0x0800, 0x70a7a3b4 ) /* U10 */
        ROM_LOAD( "so-951.dat",   0x5800, 0x0800, 0xf5f0d3f9 ) /* U11 */
        ROM_LOAD( "so-952.dat",   0x6000, 0x0800, 0x5bf19a12 ) /* U12 */
        ROM_LOAD( "so-953.dat",   0x6800, 0x0800, 0x8066ac83 ) /* U13 */
        ROM_LOAD( "so-954.dat",   0x7000, 0x0800, 0x44ed6a0d ) /* U14 */
        ROM_LOAD( "so-955.dat",   0x7800, 0x0800, 0xb5e2748d ) /* U15 */
        ROM_LOAD( "so-956.dat",   0x8000, 0x0800, 0x97de45a9 ) /* U16 */
        ROM_LOAD( "so-957.dat",   0x8800, 0x0800, 0xc14b98c4 ) /* U17 */
        ROM_LOAD( "so-958.dat",   0x9000, 0x0800, 0x4c0a7242 ) /* U18 */
        /* Background Pattern ROMs */
        ROM_LOAD( "epr-09.dat",  0x10000, 0x1000, 0xa87bfc0a )
        ROM_LOAD( "epr-10.dat",  0x11000, 0x1000, 0x8ce88100 )
        ROM_LOAD( "epr-11.dat",  0x12000, 0x1000, 0x1bdbdab5 )
        ROM_LOAD( "epr-12.dat",  0x13000, 0x1000, 0x629a4a1f )


        ROM_REGION_DISPOSE(0x6000)      /* for backgrounds */
        ROM_LOAD( "epr-13.dat",   0x0000, 0x1000, 0x74bd7f9a )
        ROM_LOAD( "epr-14.dat",   0x1000, 0x1000, 0xd2ebd915 )
        ROM_LOAD( "epr-15.dat",   0x2000, 0x1000, 0xae0e5d71 )
        ROM_LOAD( "epr-16.dat",   0x3000, 0x1000, 0xacdf203e )
        ROM_LOAD( "epr-17.dat",   0x4000, 0x1000, 0x6c7490c0 )
        ROM_LOAD( "epr-18.dat",   0x5000, 0x1000, 0x24a81c04 )
ROM_END

ROM_START( pignewt_rom )
        ROM_REGION(0x14000)     /* 64k for code + space for background */
        ROM_LOAD( "cpu.u25",    0x0000, 0x0800, 0x00000000 ) /* U25 */
        ROM_LOAD( "1888c",      0x0800, 0x0800, 0xfd18ed09 ) /* U1 */
        ROM_LOAD( "1889c",      0x1000, 0x0800, 0xf633f5ff ) /* U2 */
        ROM_LOAD( "1890c",      0x1800, 0x0800, 0x22009d7f ) /* U3 */
        ROM_LOAD( "1891c",      0x2000, 0x0800, 0x1540a7d6 ) /* U4 */
        ROM_LOAD( "1892c",      0x2800, 0x0800, 0x960385d0 ) /* U5 */
        ROM_LOAD( "1893c",      0x3000, 0x0800, 0x58c5c461 ) /* U6 */
        ROM_LOAD( "1894c",      0x3800, 0x0800, 0x5817a59d ) /* U7 */
        ROM_LOAD( "1895c",      0x4000, 0x0800, 0x812f67d7 ) /* U8 */
        ROM_LOAD( "1896c",      0x4800, 0x0800, 0xcc0ecdd0 ) /* U9 */
        ROM_LOAD( "1897c",      0x5000, 0x0800, 0x7820e93b ) /* U10 */
        ROM_LOAD( "1898c",      0x5800, 0x0800, 0xe9a10ded ) /* U11 */
        ROM_LOAD( "1899c",      0x6000, 0x0800, 0xd7ddf02b ) /* U12 */
        ROM_LOAD( "1900c",      0x6800, 0x0800, 0x8deff4e5 ) /* U13 */
        ROM_LOAD( "1901c",      0x7000, 0x0800, 0x46051305 ) /* U14 */
        ROM_LOAD( "1902c",      0x7800, 0x0800, 0xcb937e19 ) /* U15 */
        ROM_LOAD( "1903c",      0x8000, 0x0800, 0x53239f12 ) /* U16 */
        ROM_LOAD( "1913c",      0x8800, 0x0800, 0x4652cb0c ) /* U17 */
        ROM_LOAD( "1914c",      0x9000, 0x0800, 0xcb758697 ) /* U18 */
        ROM_LOAD( "1915c",      0x9800, 0x0800, 0x9f3bad66 ) /* U19 */
        ROM_LOAD( "1916c",      0xA000, 0x0800, 0x5bb6f61e ) /* U20 */
        ROM_LOAD( "1917c",      0xA800, 0x0800, 0x725e2c87 ) /* U21 */
        /* Background Pattern ROMs? */
        ROM_LOAD( "1906c.bg",  0x10000, 0x1000, 0xc79d33ce ) /* ??? */
        ROM_LOAD( "1907c.bg",  0x11000, 0x1000, 0xbc839d3c ) /* ??? */
        ROM_LOAD( "1908c.bg",  0x12000, 0x1000, 0x92cb14da ) /* ??? */

        ROM_REGION_DISPOSE(0x4000)      /* for background graphics */
        ROM_LOAD( "1904c.bg",   0x0000, 0x2000, 0xe9de2c8b ) /* ??? */
        ROM_LOAD( "1905c.bg",   0x2000, 0x2000, 0xaf7cfe0b ) /* ??? */

        /* SOUND ROMS ARE PROBABLY MISSING! */
ROM_END

ROM_START( pignewta_rom )
        ROM_REGION(0x14000)     /* 64k for code + space for background */
        ROM_LOAD( "cpu.u25",    0x0000, 0x0800, 0x00000000 ) /* U25 */
        ROM_LOAD( "1888a",      0x0800, 0x0800, 0x491c0835 ) /* U1 */
        ROM_LOAD( "1889a",      0x1000, 0x0800, 0x0dcf0af2 ) /* U2 */
        ROM_LOAD( "1890a",      0x1800, 0x0800, 0x640b8b2e ) /* U3 */
        ROM_LOAD( "1891a",      0x2000, 0x0800, 0x7b8aa07f ) /* U4 */
        ROM_LOAD( "1892a",      0x2800, 0x0800, 0xafc545cb ) /* U5 */
        ROM_LOAD( "1893a",      0x3000, 0x0800, 0x82448619 ) /* U6 */
        ROM_LOAD( "1894a",      0x3800, 0x0800, 0x4302dbfb ) /* U7 */
        ROM_LOAD( "1895a",      0x4000, 0x0800, 0x137ebaaf ) /* U8 */
        ROM_LOAD( "1896",       0x4800, 0x0800, 0x1604c811 ) /* U9 */
        ROM_LOAD( "1897",       0x5000, 0x0800, 0x3abee406 ) /* U10 */
        ROM_LOAD( "1898",       0x5800, 0x0800, 0xa96410dc ) /* U11 */
        ROM_LOAD( "1899",       0x6000, 0x0800, 0x612568a5 ) /* U12 */
        ROM_LOAD( "1900",       0x6800, 0x0800, 0x5b231cea ) /* U13 */
        ROM_LOAD( "1901",       0x7000, 0x0800, 0x3fd74b05 ) /* U14 */
        ROM_LOAD( "1902",       0x7800, 0x0800, 0xd568fc22 ) /* U15 */
        ROM_LOAD( "1903",       0x8000, 0x0800, 0x7d16633b ) /* U16 */
        ROM_LOAD( "1913",       0x8800, 0x0800, 0xfa4be04f ) /* U17 */
        ROM_LOAD( "1914",       0x9000, 0x0800, 0x08253c50 ) /* U18 */
        ROM_LOAD( "1915",       0x9800, 0x0800, 0xde786c3b ) /* U19 */
        /* Background Pattern ROMs? */
        /* NOTE: No background ROMs for set A have been dumped, so the
           ROMs from set C have been copied and renamed. This is to
           provide a reminder that these ROMs still need to be dumped. */
        ROM_LOAD( "1906a.bg",  0x10000, 0x1000, 0x00000000 ) /* ??? */
        ROM_LOAD( "1907a.bg",  0x11000, 0x1000, 0x00000000 ) /* ??? */
        ROM_LOAD( "1908a.bg",  0x12000, 0x1000, 0x00000000 ) /* ??? */

        ROM_REGION_DISPOSE(0x4000)      /* for background graphics */
        ROM_LOAD( "1904a.bg",   0x0000, 0x2000, 0x00000000 ) /* ??? */
        ROM_LOAD( "1905a.bg",   0x2000, 0x2000, 0x00000000 ) /* ??? */

        /* SOUND ROMS ARE PROBABLY MISSING! */
ROM_END

/***************************************************************************
  Security Decode "chips"
***************************************************************************/

void astrob_decode(void)
{
    /* This game uses the 315-0062 security chip */
    sega_security(62);
}

void s005_decode(void)
{
    /* This game uses the 315-0070 security chip */
    sega_security(70);
}

void monsterb_decode(void)
{
    /* This game uses the 315-0082 security chip */
    sega_security(82);
}

void spaceod_decode(void)
{
    /* This game uses the 315-0063 security chip */
    sega_security(63);
}

void pignewt_decode(void)
{
    /* This game uses the 315-0063? security chip */
    sega_security(63);
}

/***************************************************************************
  Hi Score Routines
***************************************************************************/

static int astrob_hiload(void)
{
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* check if the hi score table has already been initialized */
        if ((memcmp(&RAM[0xCC0E],"KUV",3) == 0) &&
                (memcmp(&RAM[0xCC16],"PS\\",3) == 0))
        {
                void *f;


                if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
                {
            osd_fread(f,&RAM[0xCB3F],0xDA);
                        osd_fclose(f);
                }

                return 1;
        }
        else return 0;  /* we can't load the hi scores yet */
}



static void astrob_hisave(void)
{
        void *f;
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* Hi score memory gets corrupted by the self test */
        if (memcmp(&RAM[0xCB3F],"\xFF\xFF\xFF\xFF",4)==0)
                return;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
        {
                osd_fwrite(f,&RAM[0xCB3F],0xDA);
                osd_fclose(f);
        }

}

static int monsterb_hiload(void)
{
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* check if memory has already been initialized */
        if (memcmp(&RAM[0xC8E8],"\x22\x0e\xd5\x0a",4) == 0)
        {
                void *f;

                if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
                {
                        osd_fread(f,&RAM[0xC913],7);
                        osd_fclose(f);
                }

                return 1;
        }
        else return 0;  /* we can't load the hi scores yet */
}



static void monsterb_hisave(void)
{
        void *f;
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* Hi score memory gets corrupted by the self test */
        if (memcmp(&RAM[0xC913],"\xFF\xFF\xFF\xFF",4)==0)
                return;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
        {
                osd_fwrite(f,&RAM[0xC913],7);
                osd_fclose(f);
        }

}

static int s005_hiload(void)
{
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* check if memory has already been initialized */
        if (memcmp(&RAM[0xC8ED],"\x10\x1B\x17",3) == 0)
        {
                void *f;


                if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
                {
                        osd_fread(f,&RAM[0xC911],8);
                        osd_fclose(f);
                }

                return 1;
        }
        else return 0;  /* we can't load the hi scores yet */
}



static void s005_hisave(void)
{
        void *f;
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* Hi score memory gets corrupted by the self test */
        if (memcmp(&RAM[0xC911],"\xFF\xFF\xFF\xFF",4)==0)
                return;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
        {
                osd_fwrite(f,&RAM[0xC911],8);
                osd_fclose(f);
        }

}

static int spaceod_hiload(void)
{
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* check if memory has already been initialized */
        if (memcmp(&RAM[0xC8F1],"\xE2\x00\x04\x03",4) == 0)
        {
                void *f;

                if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
                {
                        osd_fread(f,&RAM[0xC906],4);
                        osd_fclose(f);
                }

                return 1;
        }
        else return 0;  /* we can't load the hi scores yet */
}



static void spaceod_hisave(void)
{
        void *f;
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* Hi score memory gets corrupted by the self test */
        if (memcmp(&RAM[0xC906],"\xFF\xFF\xFF\xFF",4)==0)
                return;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
        {
                osd_fwrite(f,&RAM[0xC906],4);
                osd_fclose(f);
        }

}

/* TODO: fix this */
static int pignewt_hiload(void)
{
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

    /* check if memory has already been initialized */
    if (memcmp(&RAM[0xCFE7],"PIGNEWTON",9) == 0)
        {
                void *f;

                if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
                {
                        osd_fread(f,&RAM[0xCE0C],3*30); /* Top 30 hi scores? */
                        osd_fread(f,&RAM[0xCFD2],3*10); /* Top 10 initials */
                        osd_fclose(f);
                }

                return 1;
        }
        else return 0;  /* we can't load the hi scores yet */
}



static void pignewt_hisave(void)
{
        void *f;
        unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


        /* Hi score memory gets corrupted by the self test */
        if (memcmp(&RAM[0xCE0C],"\xFF\xFF\xFF\xFF",4)==0)
                return;

        if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
        {
                osd_fwrite(f,&RAM[0xCE0C],3*30);        /* Top 30 hi scores? */
                osd_fwrite(f,&RAM[0xCFD2],3*10);        /* Top 10 initials */
                osd_fclose(f);
        }

}

/***************************************************************************
  Game drivers
***************************************************************************/

static struct Samplesinterface astrob_samples_interface =
{
        12      /* 12 channels */
};

/* TODO: someday this will become a speech synthesis interface */
static struct CustomSound_interface astrob_custom_interface =
{
        astrob_speech_sh_start,
        0,
        astrob_speech_sh_update
};

static struct MachineDriver astrob_machine_driver =
{
        /* basic machine hardware */
        {
                {
                        CPU_Z80,
                        3867120,        /* 3.86712 Mhz ??? */
                        0,
                        readmem,writemem,readport,astrob_writeport,
                        segar_interrupt,1
                },
                {
                        CPU_I8035 | CPU_AUDIO_CPU,
                        3120000,        /* 3.12Mhz crystal ??? */
                        2,
                        speech_readmem,speech_writemem,speech_readport,speech_writeport,
                        ignore_interrupt,1
                }
        },
        60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
        1,
        0,

        /* video hardware */
        32*8, 32*8, { 0*8, 28*8-1, 0*8, 32*8-1 },
        gfxdecodeinfo,
        16*4+1,16*4+1,
        segar_init_colors,

        VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
        0,
        generic_vh_start,
        generic_vh_stop,
        segar_vh_screenrefresh,

        /* sound hardware */
        0,0,0,0,
        {
                {
                        SOUND_CUSTOM,
                        &astrob_custom_interface
                },
                {
                        SOUND_SAMPLES,
                        &astrob_samples_interface
                }
        }
};

static struct Samplesinterface spaceod_samples_interface =
{
        12      /* 12 channels */
};

static struct MachineDriver spaceod_machine_driver =
{
        /* basic machine hardware */
        {
                {
                        CPU_Z80,
                        3867120,        /* 3.86712 Mhz ??? */
                        0,
                        readmem,writemem,readport,spaceod_writeport,
                        segar_interrupt,1
                }
        },
        60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
        1,
        0,

        /* video hardware */
        32*8, 32*8, { 0*8, 28*8-1, 0*8, 32*8-1 },
        spaceod_gfxdecodeinfo,
        16*4*2+1,16*4*2+1,
        segar_init_colors,

        VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
        0,
        spaceod_vh_start,
        spaceod_vh_stop,
        spaceod_vh_screenrefresh,

        /* sound hardware */
        0,0,0,0,
        {
                {
                        SOUND_SAMPLES,
                        &spaceod_samples_interface
                }
        }
};

static struct Samplesinterface s005_samples_interface =
{
        12      /* 12 channels */
};

static struct MachineDriver s005_machine_driver =
{
        /* basic machine hardware */
        {
                {
                        CPU_Z80,
                        3867120,        /* 3.86712 Mhz ??? */
                        0,
                        readmem,writemem,readport,s005_writeport,
                        segar_interrupt,1
                }
        },
        60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
        1,
        0,

        /* video hardware */
        32*8, 32*8, { 0*8, 28*8-1, 0*8, 32*8-1 },
        gfxdecodeinfo,
        16*4+1,16*4+1,
        segar_init_colors,

        VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
        0,
        generic_vh_start,
        generic_vh_stop,
        segar_vh_screenrefresh,

        /* sound hardware */
        0,0,0,0,
        {
                {
                        SOUND_SAMPLES,
                        &s005_samples_interface
                }
        }
};

static struct Samplesinterface monsterb_samples_interface =
{
        4       /* 4 channels */
};

static struct CustomSound_interface monsterb_custom_interface =
{
        TMS3617_sh_start,
        TMS3617_sh_stop,
        TMS3617_sh_update
};

static struct MachineDriver monsterb_machine_driver =
{
        /* basic machine hardware */
        {
                {
                        CPU_Z80,
                        3867120,        /* 3.86712 Mhz ??? */
                        0,
                        readmem,writemem,readport,monsterb_writeport,
                        segar_interrupt,1
                }
        },
        60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
        1,
        0,

        /* video hardware */
        32*8, 32*8, { 0*8, 28*8-1, 0*8, 32*8-1 },
        monsterb_gfxdecodeinfo,
        16*4*2+1,16*4*2+1,
        segar_init_colors,

        VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
        0,
        generic_vh_start,
        generic_vh_stop,
        monsterb_vh_screenrefresh,

        /* sound hardware */
        /* sound hardware */
        0,0,0,0,
        {
                {
                        SOUND_CUSTOM,
                        &monsterb_custom_interface
                },
                {
                        SOUND_SAMPLES,
                        &monsterb_samples_interface
                }
        }
};

static struct MachineDriver pignewt_machine_driver =
{
        /* basic machine hardware */
        {
                {
                        CPU_Z80,
                        3867120,        /* 3.86712 Mhz ??? */
                        0,
                        readmem,writemem,readport,pignewt_writeport,
                        segar_interrupt,1
                }
        },
        60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
        1,
        0,

        /* video hardware */
        32*8, 32*8, { 0*8, 28*8-1, 0*8, 32*8-1 },
        monsterb_gfxdecodeinfo,
        16*4*2+1,16*4*2+1,
        segar_init_colors,

        VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
        0,
        generic_vh_start,
        generic_vh_stop,
        pignewt_vh_screenrefresh,

        /* sound hardware */
        0,0,0,0
};




struct GameDriver astrob_driver =
{
        __FILE__,
        0,
        "astrob",
        "Astro Blaster (version 2)",
        "1981",
        "Sega",
        "Dave Fish (security consultant)\nMike Balfour (game driver)",
        0,
        &astrob_machine_driver,
        0,

        astrob_rom,
        astrob_decode, 0,
        astrob_sample_names,
        0,      /* sound_prom */

        astrob_input_ports,

        0, 0, 0,
        ORIENTATION_DEFAULT,

        astrob_hiload, astrob_hisave
};

struct GameDriver astrob1_driver =
{
        __FILE__,
        &astrob_driver,
        "astrob1",
        "Astro Blaster (version 1)",
        "1981",
        "Sega",
        "Dave Fish (security consultant)\nMike Balfour (game driver)",
        0,
        &astrob_machine_driver,
        0,

        astrob1_rom,
        astrob_decode, 0,
        astrob_sample_names,
        0,      /* sound_prom */

        astrob_input_ports,

        0, 0, 0,
        ORIENTATION_DEFAULT,

        astrob_hiload, astrob_hisave
};

struct GameDriver s005_driver =
{
        __FILE__,
        0,
        "005",
        "005",
        "1981",
        "Sega",
        "Dave Fish (security consultant)\nMike Balfour (game driver)",
        0,
        &s005_machine_driver,
        0,

        s005_rom,
        s005_decode, 0,
        s005_sample_names,
        0,      /* sound_prom */

        s005_input_ports,

        0, 0, 0,
        ORIENTATION_DEFAULT,

        s005_hiload, s005_hisave
};

struct GameDriver monsterb_driver =
{
        __FILE__,
        0,
        "monsterb",
        "Monster Bash",
        "1982",
        "Sega",
        "Dave Fish (security consultant)\nMike Balfour (game driver)",
        0,
        &monsterb_machine_driver,
        0,

        monsterb_rom,
        monsterb_decode, 0,
        monsterb_sample_names,
        0,      /* sound_prom */

        monsterb_input_ports,

        0, 0, 0,
        ORIENTATION_DEFAULT,

        monsterb_hiload, monsterb_hisave
};

struct GameDriver spaceod_driver =
{
        __FILE__,
        0,
        "spaceod",
        "Space Odyssey",
        "1981",
        "Sega",
        "Dave Fish (security consultant)\nMike Balfour (game driver)",
        0,
        &spaceod_machine_driver,
        0,

        spaceod_rom,
        spaceod_decode, 0,
        spaceod_sample_names,
        0,      /* sound_prom */

        spaceod_input_ports,

        0, 0, 0,
        ORIENTATION_DEFAULT,

        spaceod_hiload, spaceod_hisave
};

struct GameDriver pignewt_driver =
{
        __FILE__,
        0,
        "pignewt",
        "Pig Newton (Revision C)",
        "1983",
        "Sega",
        "Dave Fish (security consultant)\nMike Balfour (game driver)",
        0,
        &pignewt_machine_driver,
        0,

        pignewt_rom,
        pignewt_decode, 0,
        0,
        0,      /* sound_prom */

        pignewt_input_ports,

        0, 0, 0,
        ORIENTATION_DEFAULT,

        pignewt_hiload,pignewt_hisave
};

struct GameDriver pignewta_driver =
{
        __FILE__,
        &pignewt_driver,
        "pignewta",
        "Pig Newton (Revision A)",
        "1983",
        "Sega",
        "Dave Fish (security consultant)\nMike Balfour (game driver)",
        0,
        &pignewt_machine_driver,
        0,

        pignewta_rom,
        pignewt_decode, 0,
        0,
        0,      /* sound_prom */

        pignewta_input_ports,

        0, 0, 0,
        ORIENTATION_DEFAULT,

        pignewt_hiload,pignewt_hisave
};

