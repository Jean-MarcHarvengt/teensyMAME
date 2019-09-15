/***************************************************************************

Arabian memory map (preliminary)

0000-1fff ROM 0
2000-3fff ROM 1
4000-5fff ROM 2
6000-7fff ROM 3
8000-bfff VIDEO RAM

d000-dfff RAM
e000-e100 CRT controller  ? blitter ?

memory mapped ports:
read:
d7f0 - IN 0
d7f1 - IN 1
d7f2 - IN 2
d7f3 - IN 3
d7f4 - IN 4
d7f5 - IN 5
d7f6 - clock HI  ?
d7f7 - clock set ?
d7f8 - clock LO  ?

c000 - DSW 0
c200 - DSW 1

DSW 0
bit 7 = ?
bit 6 = ?
bit 5 = ?
bit 4 = ?
bit 3 = ?
bit 2 = 1 - test mode
bit 1 = COIN 2
bit 0 = COIN 1

DSW 1
bit 7 - \
bit 6 - - coins per credit    (ALL=1 free play)
bit 5 - -
bit 4 - /
bit 3 - carry bowls / don't carry bowls to next level (0 DON'T CARRY)
bit 2 - PIC NORMAL or FLIPPED (0 NORMAL)
bit 1 - COCKTAIL or UPRIGHT   (0 COCKTAIL)
bit 0 - LIVES 0=3 lives 1=5 lives

write:
c800 - AY-3-8910 control
ca00 - AY-3-8910 write

ON AY PORTS there are two things :

port 0x0e (write only) to do ...

port 0x0f (write only) controls:
BIT 0 -
BIT 1 -
BIT 2 -
BIT 3 -
BIT 4 - 0-read RAM  1-read switches(ports)
BIT 5 -
BIT 6 -
BIT 7 -

interrupts:
standart IM 1 interrupt mode (rst #38 every vblank)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



int arabian_vh_start(void);
void arabian_vh_stop(void);
void arabian_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void arabian_spriteramw(int offset, int val);
void arabian_videoramw(int offset, int val);

int arabian_interrupt(void);
void arabian_portB_w(int offset,int data);
int arabian_input_port(int offset);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc200, 0xc200, input_port_1_r },
	{ 0xd000, 0xd7ef, MRA_RAM },
	{ 0xd7f0, 0xd7f8, arabian_input_port },
	{ 0xd7f9, 0xdfff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0xbfff, arabian_videoramw, &videoram },
	{ 0xd000, 0xd7ff, MWA_RAM },
	{ 0xe000, 0xe07f, arabian_spriteramw, &spriteram },
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }  /* end of table */
};



static struct IOWritePort writeport[] =
{
	{ 0xc800, 0xc800, AY8910_control_port_0_w },
	{ 0xca00, 0xca00, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START      /* IN6 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BITX(    0x04, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPNAME( 0x02, 0x02, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Upright" )
	PORT_DIPSETTING(    0x00, "Cocktail" )
	PORT_DIPNAME( 0x04, 0x04, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x00, "Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Carry Bowls to Next Life" )
	PORT_DIPSETTING(    0x08, "Do Not Carry Bowls" )
	PORT_DIPNAME( 0xf0, 0x00, "Coinage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "A 2/1 B 2/1" )
	PORT_DIPSETTING(    0x20, "A 2/1 B 1/3" )
	PORT_DIPSETTING(    0x00, "A 1/1 B 1/1" )
	PORT_DIPSETTING(    0x30, "A 1/1 B 1/2" )
	PORT_DIPSETTING(    0x40, "A 1/1 B 1/3" )
	PORT_DIPSETTING(    0x50, "A 1/1 B 1/4" )
	PORT_DIPSETTING(    0x60, "A 1/1 B 1/5" )
	PORT_DIPSETTING(    0x70, "A 1/1 B 1/6" )
	PORT_DIPSETTING(    0x80, "A 1/2 B 1/2" )
	PORT_DIPSETTING(    0x90, "A 1/2 B 1/4" )
	PORT_DIPSETTING(    0xa0, "A 1/2 B 1/6" )
	PORT_DIPSETTING(    0xb0, "A 1/2 B 1/10" )
	PORT_DIPSETTING(    0xc0, "A 1/2 B 1/11" )
	PORT_DIPSETTING(    0xd0, "A 1/2 B 1/12" )
	PORT_DIPSETTING(    0xf0, "Free Play" )
	/* 0xe0 gives A 1/2 B 1/6 */

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN5 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No Bonus" )
	PORT_DIPSETTING(    0x04, "20000" )
	PORT_DIPSETTING(    0x08, "30000" )
	PORT_DIPSETTING(    0x0c, "Forget Bonus (??)" )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END





static unsigned char palette[] =
{

/*colors for plane 1*/
	0   , 0   , 0,
	0   , 37*4, 53*4,
	0   , 40*4, 63*4,
	0   , 44*4, 63*4,
	48*4, 22*4, 0,
	63*4, 39*4, 51*4,
	63*4, 56*4, 0,
	60*4, 60*4, 60*4,
	0   , 0   , 0,
	32*4, 12*4, 0,
	39*4, 18*4, 0,
	0   , 24*4, 51*4,
	45*4, 20*4, 1*4,
	63*4, 36*4, 51*4,
	57*4, 41*4, 10*4,
	63*4, 39*4, 51*4,
/*colors for plane 2*/
	0   , 0   , 0,
	0   , 28*4, 0,
	0   , 11*4, 0,
	0   , 40*4, 0,
	48*4, 22*4, 0,
	58*4, 48*4, 0,
	44*4, 18*4, 0,
	60*4, 60*4, 60*4,
	25*4, 6*4 , 0,
	28*4, 21*4, 0,
	26*4, 18*4, 0,
	0   , 24*4, 0,
	45*4, 20*4, 1*4,
	51*4, 30*4, 5*4,
	57*4, 41*4, 10*4,
	63*4, 53*4, 16*4,
};


enum {BLACK,BLUE1,BLUE2,BLUE3,YELLOW };

static unsigned short colortable[] =
{
	/* characters and sprites */
	BLACK,BLUE1,BLACK,YELLOW,
};



static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1500000,	/* 1.5 MHz?????? */
	{ 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ arabian_portB_w }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80 | CPU_16BIT_PORT,
			4000000,	/* 4 Mhz */
			0,
			readmem,writemem,0,writeport,
			arabian_interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0x0b, 0xf2, 0, 32*8-1 },
        0,
	sizeof(palette)/3,sizeof(colortable)/sizeof(unsigned short),
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	arabian_vh_start,
	arabian_vh_stop,
	arabian_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( arabian_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic1.87",       0x0000, 0x2000, 0x51e9a6b1 )
	ROM_LOAD( "ic2.88",       0x2000, 0x2000, 0x1cdcc1ab )
	ROM_LOAD( "ic3.89",       0x4000, 0x2000, 0xb7b7faa0 )
	ROM_LOAD( "ic4.90",       0x6000, 0x2000, 0xdbded961 )

	ROM_REGION_DISPOSE(0x2000)	/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000) /* space for graphics roms */
	ROM_LOAD( "ic84.91",      0x0000, 0x2000, 0xc4637822 )	/* because of very rare way */
	ROM_LOAD( "ic85.92",      0x2000, 0x2000, 0xf7c6866d )  /* CRT controller uses these roms */
	ROM_LOAD( "ic86.93",      0x4000, 0x2000, 0x71acd48d )  /* there's no way, but to decode */
	ROM_LOAD( "ic87.94",      0x6000, 0x2000, 0x82160b9a )	/* it at runtime - which is SLOW */

ROM_END



static int arabian_hiload(void)
{
  unsigned char *RAM = Machine->memory_region[0];
  void *f;

  /* Wait for hiscore table initialization to be done. */
  if (memcmp(&RAM[0xd384], "\x00\x00\x00\x01\x00\x00", 6) != 0)
    return 0;

  if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
    {
      /* Load and set hiscore table. */
      osd_fread(f,&RAM[0xd384],6*10);
      osd_fclose(f);
    }

  return 1;
}



static void arabian_hisave(void)
{
  unsigned char *RAM = Machine->memory_region[0];
  void *f;

  if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
    {
      /* Write hiscore table. */
      osd_fwrite(f,&RAM[0xd384],6*10);
      osd_fclose(f);
    }
}



struct GameDriver arabian_driver =
{
	__FILE__,
	0,
	"arabian",
	"Arabian",
	"1983",
	"Atari",
	"Jarek Burczynski (MAME driver)\nMarco Cassili",
	0,
	&machine_driver,
	0,

	arabian_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	0, palette, colortable,
	ORIENTATION_DEFAULT,

	arabian_hiload, arabian_hisave
};

