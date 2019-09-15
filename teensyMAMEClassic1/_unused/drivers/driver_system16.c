/*

	working:
		Alien Syndrome
		Altered Beast
		Aurail: some graphics glitches
		Dynamite Dux: some graphics glitches
		Flash Point: dipswitches are wrong
		Golden Axe
		Passing Shot: wrong screen orientation
		SDI: I think it needs the analog controls mapped
		Shinobi
		Tetris
		Time Scanner: wrong screen orientation; transparency and sprite glitches
		Tough Turf: background glitches
		Wonderboy 3 - Monster Lair
		Wrestle War: some sprite glitches, wrong orientation

	not really working:
		Alex Kidd: no sprites, messed up graphics
		Alien Storm: black screen
		E-Swat: black screen
		Heavyweight Champ: messed up graphics
		Major League: missing sprites?
		Quartet 2: no sprites, messed up background


	other System16/18 games, not yet described in this file:
		Hang-on
		Space Harrier
		Moonwalker
		Outrun
		Super Hangon
		Shadow Dancer
*/

#define SYS16_CREDITS \
	"Thierry Lescot & Nao (Hardware Info)\n" \
	"Mirko Buffoni (MAME driver)\n" \
	"Phil Stroffolino"

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/2151intf.h"

//#include "sys16gcs.c"

/***************************************************************************/

extern int sys16_vh_start( void );
extern void sys16_vh_stop( void );
extern void sys16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* video driver constants (vary with game) */
extern int sys16_spritesystem;
extern int sys16_sprxoffset;
extern int *sys16_obj_bank;
void (* sys16_update_proc)( void );

/* video driver registers */
extern int sys16_refreshenable;
extern int sys16_tile_bank0;
extern int sys16_tile_bank1;
extern int sys16_bg_scrollx, sys16_bg_scrolly;
extern int sys16_bg_page[4];
extern int sys16_fg_scrollx, sys16_fg_scrolly;
extern int sys16_fg_page[4];

/* video driver has access to these memory regions */
unsigned char *sys16_tileram;
unsigned char *sys16_textram;
unsigned char *sys16_spriteram;

/* other memory regions */
static unsigned char *sys16_ROM;
static unsigned char *sys16_workingram;
static unsigned char *sys16_extraram;
static unsigned char *sys16_extraram2;
static unsigned char *sys16_extraram3;

/* shared memory */
static int mirror1_hi_addr;
static int mirror1_lo_addr;
static int mirror1_word;

static int mirror2_hi_addr;
static int mirror2_lo_addr;
static int mirror2_word;

static int mirror3_hi_addr;
static int mirror3_lo_addr;
static int mirror3_word;

/***************************************************************************/

#define MWA_PALETTERAM	sys16_paletteram_w, &paletteram
#define MRA_PALETTERAM	paletteram_word_r

#define MRA_WORKINGRAM	MRA_BANK1
#define MWA_WORKINGRAM	MWA_BANK1,&sys16_workingram

#define MRA_SPRITERAM	MRA_BANK2
#define MWA_SPRITERAM	MWA_BANK2,&sys16_spriteram

#define MRA_TILERAM		MRA_BANK3
#define MWA_TILERAM		MWA_BANK3,&sys16_tileram

#define MRA_TEXTRAM		MRA_BANK4
#define MWA_TEXTRAM		MWA_BANK4,&sys16_textram

#define MRA_EXTRAM		MRA_BANK5
#define MWA_EXTRAM		MWA_BANK5,&sys16_extraram

#define MRA_EXTRAM2		MRA_BANK6
#define MWA_EXTRAM2		MWA_BANK6,&sys16_extraram2

#define MRA_EXTRAM3		MRA_BANK7
#define MWA_EXTRAM3		MWA_BANK7,&sys16_extraram3

/****************************************************************************

Some System16 games need mirrored addresses.  In MAME this presents some
difficulties, because memory read/write handlers for 68000 games are
word-based.  The following private functions make supporting mirrored
bytes easier.

****************************************************************************/

static void mirror1_w( int offset, int data ){
	if( (data&0xff000000)==0 ){
		if( mirror1_hi_addr )
			cpu_writemem24( mirror1_hi_addr, (data>>8)&0xff );
		else
			mirror1_word = (mirror1_word&0x00ff) | (data&0xff00);
	}

	if( (data&0x00ff0000)==0 ){
		if( mirror1_lo_addr )
			cpu_writemem24( mirror1_lo_addr, data&0xff );
		else
			mirror1_word = (mirror1_word&0xff00) | (data&0xff);
	}
}

static int mirror1_r( int offset ){
	int data = mirror1_word;
	if( mirror1_hi_addr ) data |= cpu_readmem24( mirror1_hi_addr )<<8;
	if( mirror1_lo_addr ) data |= cpu_readmem24( mirror1_lo_addr );

	if( data!= (input_port_0_r(offset) << 8) + input_port_1_r(offset) ){
		if( errorlog ) fprintf( errorlog, "bad\n" );
	}

	return data;
}

static void define_mirror1( int hi_addr, int lo_addr ){
	mirror1_word = 0;
	mirror1_hi_addr = hi_addr;
	mirror1_lo_addr = lo_addr;
}

static void mirror2_w( int offset, int data ){
	if( (data&0xff000000)==0 ){
		if( mirror2_hi_addr )
			cpu_writemem24( mirror2_hi_addr, (data>>8)&0xff );
		else
			mirror2_word = (mirror2_word&0x00ff) | (data&0xff00);
	}

	if( (data&0x00ff0000)==0 ){
		if( mirror2_lo_addr )
			cpu_writemem24( mirror2_lo_addr, data&0xff );
		else
			mirror2_word = (mirror2_word&0xff00) | (data&0xff);
	}
}

static int mirror2_r( int offset ){
	int data = mirror2_word;
	if( mirror2_hi_addr ) data |= cpu_readmem24( mirror2_hi_addr )<<8;
	if( mirror2_lo_addr ) data |= cpu_readmem24( mirror2_lo_addr );

	if( data!= (input_port_0_r(offset) << 8) + input_port_1_r(offset) ){
		if( errorlog ) fprintf( errorlog, "bad\n" );
	}

	return data;
}

static void define_mirror2( int hi_addr, int lo_addr ){
	mirror2_word = 0;
	mirror2_hi_addr = hi_addr;
	mirror2_lo_addr = lo_addr;
}

static void mirror3_w( int offset, int data ){
	if( (data&0xff000000)==0 ){
		if( mirror3_hi_addr )
			cpu_writemem24( mirror3_hi_addr, (data>>8)&0xff );
		else
			mirror3_word = (mirror3_word&0x00ff) | (data&0xff00);
	}

	if( (data&0x00ff0000)==0 ){
		if( mirror3_lo_addr )
			cpu_writemem24( mirror3_lo_addr, data&0xff );
		else
			mirror3_word = (mirror3_word&0xff00) | (data&0xff);
	}
}

static int mirror3_r( int offset ){
	int data = mirror3_word;
	if( mirror3_hi_addr ) data |= cpu_readmem24( mirror3_hi_addr )<<8;
	if( mirror3_lo_addr ) data |= cpu_readmem24( mirror3_lo_addr );

	if( data!= (input_port_0_r(offset) << 8) + input_port_1_r(offset) ){
		if( errorlog ) fprintf( errorlog, "bad\n" );
	}

	return data;
}

static void define_mirror3( int hi_addr, int lo_addr ){
	mirror3_word = 0;
	mirror3_hi_addr = hi_addr;
	mirror3_lo_addr = lo_addr;
}

/***************************************************************************/

#define MACHINE_DRIVER( GAMENAME,READMEM,WRITEMEM,INITMACHINE,GFXSIZE ) \
static struct MachineDriver GAMENAME = \
{ \
	{ \
		{ \
			CPU_M68000, \
			10000000, \
			0, \
			READMEM,WRITEMEM,0,0, \
			sys16_interrupt,1 \
		}, \
		{ \
			CPU_Z80 | CPU_AUDIO_CPU, \
			4096000, \
			3, \
			sound_readmem,sound_writemem,sound_readport,sound_writeport, \
			ignore_interrupt,1 \
		}, \
	}, \
	60, DEFAULT_60HZ_VBLANK_DURATION, \
	1, \
	INITMACHINE, \
	40*8, 28*8, { 0*8, 40*8-1, 0*8, 28*8-1 }, \
	GFXSIZE, \
	2048,2048, \
	0, \
	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_SUPPORTS_DIRTY, \
	0, \
	sys16_vh_start, \
	sys16_vh_stop, \
	sys16_vh_screenrefresh, \
	SOUND_SUPPORTS_STEREO,0,0,0, \
	{ \
		{ \
			SOUND_YM2151, \
			&ym2151_interface \
		} \
	} \
};

/***************************************************************************/

int sys16_interrupt( void ){
	return 4; /* Interrupt vector 4, used by VBlank */
}

/***************************************************************************/

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xf800, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xf800, 0xffff, MWA_RAM },
	{ -1 }  /* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x01, 0x01, YM2151_status_port_0_r },
	{ 0xc0, 0xc0, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, YM2151_register_port_0_w },
	{ 0x01, 0x01, YM2151_data_port_0_w },
	{ 0x40, 0x40, IOWP_NOP },   /* adpcm? */
	{ 0x80, 0x80, IOWP_NOP },   /* ??? */
	{ -1 }
};

static void sound_command_w(int offset, int data){
	if( errorlog ) fprintf( errorlog, "SOUND COMMAND %04x <- %02x\n", offset, data&0xff );
	soundlatch_w( 0,data&0xff );
	cpu_cause_interrupt( 1, 0 );
}

static void irq_handler_mus(void){
}

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	4096000,	/* 3.58 MHZ ? */
	{ 60 },
	{ irq_handler_mus }
};

/***************************************************************************/

static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 chars */
	16384,	/* 16384 chars */
	3,	/* 3 bits per pixel */
	{ 0x20000*8, 0x10000*8, 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 chars */
	16384,	/* 16384 chars */
	3,	/* 3 bits per pixel */
	{ 0x40000*8, 0x20000*8, 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

static struct GfxLayout charlayout4 =
{
	8,8,	/* 8*8 chars */
	16384,	/* 16384 chars */
	3,	/* 3 bits per pixel */
	{ 0x80000*8, 0x40000*8, 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

static struct GfxLayout charlayout8 =
{
	8,8,	/* 8*8 chars */
	16384,	/* 16384 chars */
	3,	/* 3 bits per pixel */
	{ 0x10000*8, 0x08000*8, 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfx1[] =
{
	{ 1, 0x00000, &charlayout1,	0, 256 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfx2[] =
{
	{ 1, 0x00000, &charlayout2,	0, 256 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfx4[] =
{
	{ 1, 0x00000, &charlayout4,	0, 256 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfx8[] =
{
	{ 1, 0x00000, &charlayout8,	0, 256 },
	{ -1 } /* end of array */
};

/***************************************************************************/

void sys16_paletteram_w(int offset, int data){
	int oldword = READ_WORD (&paletteram[offset]);
	int newword = COMBINE_WORD (oldword, data);
	if( oldword!=newword ){
		/* we can do this, because we initialize palette RAM to all black in vh_start */

		/*	   byte 0    byte 1 */
		/*	GBGR BBBB GGGG RRRR */
		/*	5444 3210 3210 3210 */

		int r = (newword >> 0) & 0x0f;
		int g = (newword >> 4) & 0x0f;
		int b = (newword >> 8) & 0x0f;

		r = (r << 1);// | ((newword >> 12) & 1);
		g = (g << 1);// | ((newword >> 13) & 1);
		b = (b << 1);// | ((newword >> 14) & 1);

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);

		palette_change_color( offset/2, r,g,b );

		WRITE_WORD (&paletteram[offset], newword);
	}
}

/***************************************************************************/

static void set_refresh( int data ){
	sys16_refreshenable = data&0x20;
}

static void set_tile_bank( int data ){
	sys16_tile_bank1 = data&0xf;
	sys16_tile_bank0 = (data>>4)&0xf;
}

static void set_fg_page( int data ){
	sys16_fg_page[0] = data>>12;
	sys16_fg_page[1] = (data>>8)&0xf;
	sys16_fg_page[2] = (data>>4)&0xf;
	sys16_fg_page[3] = data&0xf;
}

static void set_bg_page( int data ){
	sys16_bg_page[0] = data>>12;
	sys16_bg_page[1] = (data>>8)&0xf;
	sys16_bg_page[2] = (data>>4)&0xf;
	sys16_bg_page[3] = data&0xf;
}

/***************************************************************************/
/*	Important: you must leave extra space when listing sprite ROMs
	in a ROM module definition.  This routine unpacks each sprite nibble
	into a byte, doubling the memory consumption. */

void sys16_sprite_decode( int num_banks, int bank_size ){
	unsigned char *base = Machine->memory_region[2];
	unsigned char *temp = malloc( bank_size );
	int i;

	if( !temp ) return;

	for( i = num_banks; i >0; i-- ){
		unsigned char *finish	= base + 2*bank_size*i;
		unsigned char *dest = finish - 2*bank_size;

		unsigned char *p1 = temp;
		unsigned char *p2 = temp+bank_size/2;

		unsigned char data;

		memcpy (temp, base+bank_size*(i-1), bank_size);

		do {
			data = *p2++;
			*dest++ = data >> 4;
			*dest++ = data & 0xF;

			data = *p1++;
			*dest++ = data >> 4;
			*dest++ = data & 0xF;
		} while( dest<finish );
	}

	free( temp );
}

/***************************************************************************/

static int io_player1_r( int offset ){ return input_port_0_r( offset ); }
static int io_player2_r( int offset ){ return input_port_1_r( offset ); }
static int io_player3_r( int offset ){ return input_port_5_r( offset ); }
static int io_service_r( int offset ){ return input_port_2_r( offset ); }

static int io_dip1_r( int offset ){ return input_port_3_r( offset ); }
static int io_dip2_r( int offset ){ return input_port_4_r( offset ); }

/***************************************************************************/

static void copy_rom64k( int dest, int source ){
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	memcpy( &RAM[dest*0x10000], &RAM[source*0x10000], 0x10000 );
}

static void patch_code( int offset, int data ){
	int aligned_offset = offset&0xfffffe;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	int old_word = READ_WORD( &RAM[aligned_offset] );

	if( offset&1 ){
		data = (old_word&0xff00)|data;
	}
	else {
		data = (old_word&0x00ff)|(data<<8);
	}

	WRITE_WORD (&RAM[aligned_offset], data);
}

/***************************************************************************/

#define SYS16_JOY1 PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

#define SYS16_JOY2 PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

#define SYS16_JOY3 PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )

#define SYS16_SERVICE PORT_START \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BITX(0x04, 0x04, 0, "Test Mode", OSD_KEY_F1, IP_JOY_NONE, 0 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define SYS16_COINAGE PORT_START \
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE ) \
	PORT_DIPSETTING(    0x07, "4 Coins/1 Credit") \
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit") \
	PORT_DIPSETTING(    0x09, "2 Coins/1 Credit") \
	PORT_DIPSETTING(    0x05, "2/1 5/3 6/4") \
	PORT_DIPSETTING(    0x04, "2/1 4/3") \
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit") \
	PORT_DIPSETTING(    0x01, "1/1 2/3") \
	PORT_DIPSETTING(    0x02, "1/1 4/5") \
	PORT_DIPSETTING(    0x03, "1/1 5/6") \
	PORT_DIPSETTING(    0x06, "2 Coins/3 Credits") \
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits") \
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits") \
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits") \
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits") \
	PORT_DIPSETTING(    0x0a, "1 Coin/6 Credits") \
	PORT_DIPSETTING(    0x00, "Free Play (if Coin B too) or 1/1") \
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE ) \
	PORT_DIPSETTING(    0x70, "4 Coins/1 Credit") \
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit") \
	PORT_DIPSETTING(    0x90, "2 Coins/1 Credit") \
	PORT_DIPSETTING(    0x50, "2/1 5/3 6/4") \
	PORT_DIPSETTING(    0x40, "2/1 4/3") \
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit") \
	PORT_DIPSETTING(    0x10, "1/1 2/3") \
	PORT_DIPSETTING(    0x20, "1/1 4/5") \
	PORT_DIPSETTING(    0x30, "1/1 5/6") \
	PORT_DIPSETTING(    0x60, "2 Coins/3 Credits") \
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits") \
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits") \
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits") \
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits") \
	PORT_DIPSETTING(    0xa0, "1 Coin/6 Credits") \
	PORT_DIPSETTING(    0x00, "Free Play (if Coin A too) or 1/1")

#define SYS16_OPTIONS PORT_START \
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE ) \
	PORT_DIPSETTING(    0x00, "Upright") \
	PORT_DIPSETTING(    0x01, "Cocktail") \
	PORT_DIPNAME( 0x02, 0x02, "Attract Mode Sound", IP_KEY_NONE ) \
	PORT_DIPSETTING(    0x02, "Off" ) \
	PORT_DIPSETTING(    0x00, "On" ) \
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE ) \
	PORT_DIPSETTING(    0x08, "2" ) \
	PORT_DIPSETTING(    0x0c, "3" ) \
	PORT_DIPSETTING(    0x04, "5" ) \
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "240", IP_KEY_NONE, IP_JOY_NONE, 0 ) \
	PORT_DIPNAME( 0x30, 0x30, "Difficulty", IP_KEY_NONE ) \
	PORT_DIPSETTING(    0x20, "Easy" ) \
	PORT_DIPSETTING(    0x30, "Normal" ) \
	PORT_DIPSETTING(    0x10, "Hard" ) \
	PORT_DIPSETTING(    0x00, "Hardest" ) \
	PORT_DIPNAME( 0x40, 0x40, "Enemy's Bullet Speed", IP_KEY_NONE ) \
	PORT_DIPSETTING(    0x40, "Slow" ) \
	PORT_DIPSETTING(    0x00, "Fast" ) \
	PORT_DIPNAME( 0x80, 0x80, "Language", IP_KEY_NONE ) \
	PORT_DIPSETTING(    0x80, "Japanese" ) \
	PORT_DIPSETTING(    0x00, "English" )

/***************************************************************************/

ROM_START( alexkidd_rom )
	ROM_REGION( 0x040000 ) /* 68000 code */
	ROM_LOAD_ODD ( "10445.26", 0x000000, 0x10000, 0x25ce5b6f )
	ROM_LOAD_EVEN( "10447.43", 0x000000, 0x10000, 0x29e87f71 )
	ROM_LOAD_ODD ( "10446.25", 0x020000, 0x10000, 0xcd61d23c )
	ROM_LOAD_EVEN( "10448.42", 0x020000, 0x10000, 0x05baedb5 )

	ROM_REGION( 0x18000 ) /* tiles */
	ROM_LOAD( "10431.95", 0x00000, 0x08000, 0xa7962c39 )
	ROM_LOAD( "10432.94", 0x08000, 0x08000, 0xdb8cd24e )
	ROM_LOAD( "10433.93", 0x10000, 0x08000, 0xe163c8c2 )

	ROM_REGION( 0x050000*2 ) /* sprites */
	ROM_LOAD( "10437.10", 0x000000, 0x008000, 0x522f7618 )
	ROM_LOAD( "10441.11", 0x008000, 0x008000, 0x74e3a35c )
	ROM_LOAD( "10438.17", 0x010000, 0x008000, 0x738a6362 )
	ROM_LOAD( "10442.18", 0x018000, 0x008000, 0x86cb9c14 )
	ROM_LOAD( "10439.23", 0x020000, 0x008000, 0xb391aca7 )
	ROM_LOAD( "10443.24", 0x028000, 0x008000, 0x95d32635 )
	ROM_LOAD( "10440.29", 0x030000, 0x008000, 0x23939508 )
	ROM_LOAD( "10444.30", 0x038000, 0x008000, 0x82115823 )
	ROM_LOAD( "10437.10", 0x040000, 0x008000, 0x522f7618 )
	ROM_LOAD( "10441.11", 0x048000, 0x008000, 0x74e3a35c )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "10434.12", 0x0000, 0x8000, 0x77141cce )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress alexkidd_readmem[] =
{
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },

	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x410000, 0x410fff, MRA_TEXTRAM },
	{ 0x440000, 0x440fff, MRA_SPRITERAM },
	{ 0x840000, 0x840fff, MRA_PALETTERAM },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x03ffff, MRA_ROM },
	{-1}
};

static struct MemoryWriteAddress alexkidd_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0x410000, 0x410fff, MWA_TEXTRAM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void alexkidd_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0ff8] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0ffa] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0f24] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0f26] );

	set_fg_page( READ_WORD( &sys16_textram[0x0e9e] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0e9c] ) );
//tile_bank:0x000000
//refresh:0x000000
}

void alexkidd_init_machine( void ){
	static int bank[16] = { 00,01,02,03,00,01,02,03,00,01,02,03,00,01,02,03};
	sys16_obj_bank = bank;

	//mirror( 0xc40001, 0xfe0007, 0xff );
	sys16_update_proc = alexkidd_update_proc;
}

void alexkidd_sprite_decode( void ){
	sys16_sprite_decode( 5,0x010000 );
}
/***************************************************************************/

INPUT_PORTS_START( alexkidd_input_ports )
//dipswitch:0x01 0 GAME_TYPE COCKTAIL_TABLE UPRIGHT _ _ _
//dipswitch:0x02 1 ADVERTISE_SOUND OFF ON _ _ _
//dipswitch:0x0C 2 NUMBER_OF_LIVES 3 2 5 FREE _
//dipswitch:0x30 4 DIFFICULTY NORMAL EASY HARD HARDEST _
//dipswitch:0x40 6 BULLET'S_SPEED SLOW FAST _ _ _
//dipswitch:0x80 7 LANGUAGE JAPANESE ENGLISH _ _ _
//labeljoy:_ JUMP SHOT _ DOWN UP RIGHT LEFT
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START _ _
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( alexkidd_machine_driver, \
	alexkidd_readmem,alexkidd_writemem,alexkidd_init_machine,gfx8 )

struct GameDriver alexkidd_driver =
{
	__FILE__,
	0,
	"alexkidd",
	"Alex Kidd (bootleg)",
	"1986",
	"bootleg",
	SYS16_CREDITS,
	GAME_NOT_WORKING,
	&alexkidd_machine_driver,
	0,
	alexkidd_rom,
	alexkidd_sprite_decode, 0,
	0,
	0,
	alexkidd_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( aliensyn_rom )
	ROM_REGION( 0x030000 ) /* 68000 code */
	ROM_LOAD_ODD ( "11080.a1", 0x00000, 0x8000, 0xfe7378d9 )
	ROM_LOAD_EVEN( "11083.a4", 0x00000, 0x8000, 0xcb2ad9b3 )
	ROM_LOAD_ODD ( "11081.a2", 0x10000, 0x8000, 0x1308ee63 )
	ROM_LOAD_EVEN( "11084.a5", 0x10000, 0x8000, 0x2e1ec7b1 )
	ROM_LOAD_ODD ( "11082.a3", 0x20000, 0x8000, 0x9cdc2a14 )
	ROM_LOAD_EVEN( "11085.a6", 0x20000, 0x8000, 0xcff78f39 )

	ROM_REGION( 0x30000 ) /* tiles */
	ROM_LOAD( "10702.b9",  0x00000, 0x10000, 0x393bc813 )
	ROM_LOAD( "10703.b10", 0x10000, 0x10000, 0x6b6dd9f5 )
	ROM_LOAD( "10704.b11", 0x20000, 0x10000, 0x911e7ebc )

	ROM_REGION( 0x080000*2 ) /* sprites */
	ROM_LOAD( "10709.b1", 0x00000, 0x10000, 0xaddf0a90 )
	ROM_LOAD( "10713.b5", 0x10000, 0x10000, 0xececde3a )
	ROM_LOAD( "10710.b2", 0x20000, 0x10000, 0x992369eb )
	ROM_LOAD( "10714.b6", 0x30000, 0x10000, 0x91bf42fb )
	ROM_LOAD( "10711.b3", 0x40000, 0x10000, 0x29166ef6 )
	ROM_LOAD( "10715.b7", 0x50000, 0x10000, 0xa7c57384 )
	ROM_LOAD( "10712.b4", 0x60000, 0x10000, 0x876ad019 )
	ROM_LOAD( "10716.b8", 0x70000, 0x10000, 0x40ba1d48 )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "10723.a7", 0x0000, 0x8000, 0x99953526 )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress aliensyn_readmem[] =
{
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },

	{ 0x410000, 0x410fff, MRA_TEXTRAM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x440000, 0x440fff, MRA_SPRITERAM },
	{ 0x840000, 0x840fff, MRA_PALETTERAM },
	{ 0xc40000, 0xc40fff, MRA_EXTRAM },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x02ffff, MRA_ROM },
	{-1}
};

static struct MemoryWriteAddress aliensyn_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x410000, 0x410fff, MWA_TEXTRAM },
	{ 0x440000, 0x440fff, MWA_SPRITERAM },
	{ 0x840000, 0x840fff, MWA_PALETTERAM },
	{ 0xc00006, 0xc00007, sound_command_w },
	{ 0xc40000, 0xc40fff, MWA_EXTRAM },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};

/***************************************************************************/

void aliensyn_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0e98] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0e9a] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0e90] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0e92] );

	set_fg_page( READ_WORD( &sys16_textram[0x0e80] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0e82] ) );

	set_refresh( READ_WORD( &sys16_extraram[0] ) ); // 0xc40001
}

void aliensyn_init_machine( void ){
	static int bank[16] = { 0,0,0,0,0,0,0,6,0,0,0,4,0,2,0,0 };
	sys16_obj_bank = bank;

	sys16_update_proc = aliensyn_update_proc;
}

void aliensyn_sprite_decode( void ){
	sys16_sprite_decode( 4,0x20000 );
}

/***************************************************************************/

INPUT_PORTS_START( aliensyn_input_ports )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START	/* DSW1 */
		PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
		PORT_DIPSETTING(    0x00, "On")
		PORT_DIPSETTING(    0x01, "Off")
		PORT_DIPNAME( 0x02, 0x02, "Demo Sound?", IP_KEY_NONE )
		PORT_DIPSETTING(    0x02, "Off" )
		PORT_DIPSETTING(    0x00, "On" )
		PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
		PORT_DIPSETTING(    0x08, "2" )
		PORT_DIPSETTING(    0x0c, "3" )
		PORT_DIPSETTING(    0x04, "4" )
		PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Free (127?)", IP_KEY_NONE, IP_JOY_NONE, 0 )
		PORT_DIPNAME( 0x30, 0x30, "Timer", IP_KEY_NONE )
		PORT_DIPSETTING(    0x30, "150" )
		PORT_DIPSETTING(    0x20, "140" )
		PORT_DIPSETTING(    0x10, "130" )
		PORT_DIPSETTING(    0x00, "120" )
		PORT_DIPNAME( 0xc0, 0xc0, "Difficulty", IP_KEY_NONE )
		PORT_DIPSETTING(    0x80, "Easy" )
		PORT_DIPSETTING(    0xc0, "Normal" )
		PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( aliensyn_machine_driver, \
	aliensyn_readmem,aliensyn_writemem,aliensyn_init_machine, gfx1 )

struct GameDriver aliensyn_driver =
{
	__FILE__,
	0,
	"aliensyn",
	"Alien Syndrome",
	"1987",
	"Sega",
	SYS16_CREDITS,
	0,
	&aliensyn_machine_driver,
	0,
	aliensyn_rom,
	aliensyn_sprite_decode, 0,
	0,
	0,
	aliensyn_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( altbeast_rom )
	ROM_REGION( 0x040000 ) /* 68000 code */
	ROM_LOAD_ODD ( "ab11739.bin", 0x000000, 0x20000, 0xe466eb65 )
	ROM_LOAD_EVEN( "ab11740.bin", 0x000000, 0x20000, 0xce227542 )

	ROM_REGION( 0x60000 ) /* tiles */
	ROM_LOAD( "ab11674.bin", 0x00000, 0x20000, 0xa57a66d5 )
	ROM_LOAD( "ab11675.bin", 0x20000, 0x20000, 0x2ef2f144 )
	ROM_LOAD( "ab11676.bin", 0x40000, 0x20000, 0x0c04acac )

	ROM_REGION( 0xe0000*2 ) /* sprites */
	ROM_LOAD( "ab11677.bin", 0x00000, 0x10000, 0xf8b3684e )
	ROM_LOAD( "ab11681.bin", 0x10000, 0x10000, 0xae3c2793 )
	ROM_LOAD( "ab11726.bin", 0x20000, 0x10000, 0x3cce5419 )
	ROM_LOAD( "ab11730.bin", 0x30000, 0x10000, 0x3af62b55 )
	ROM_LOAD( "ab11678.bin", 0x40000, 0x10000, 0xb0390078 )
	ROM_LOAD( "ab11682.bin", 0x50000, 0x10000, 0x2a87744a )
	ROM_LOAD( "ab11728.bin", 0x60000, 0x10000, 0xf3a43fd8 )
	ROM_LOAD( "ab11732.bin", 0x70000, 0x10000, 0x2fb3e355 )
	ROM_LOAD( "ab11679.bin", 0x80000, 0x10000, 0x676be0cb )
	ROM_LOAD( "ab11683.bin", 0x90000, 0x10000, 0x802cac94 )
	ROM_LOAD( "ab11718.bin", 0xa0000, 0x10000, 0x882864c2 )
	ROM_LOAD( "ab11734.bin", 0xb0000, 0x10000, 0x76c704d2 )
	ROM_LOAD( "ab11680.bin", 0xc0000, 0x10000, 0x339987f7 )
	ROM_LOAD( "ab11684.bin", 0xd0000, 0x10000, 0x4fe406aa )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "ab11671.bin", 0x0000, 0x8000, 0x2b71343b )
ROM_END

/***************************************************************************/

static int altbeast_skip(int offset)
{
	if (cpu_getpc()==0x3994) {cpu_spinuntil_int(); return 1<<8;}

	return READ_WORD(&sys16_workingram[0xf01c]);
}

static struct MemoryReadAddress altbeast_readmem[] =
{
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },

	{ 0x410000, 0x410fff, MRA_TEXTRAM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x440000, 0x440fff, MRA_SPRITERAM },
	{ 0x840000, 0x840fff, MRA_PALETTERAM },
	{ 0xc40000, 0xc40fff, MRA_EXTRAM },

  { 0xfff01c, 0xfff01d, altbeast_skip },

	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x03ffff, MRA_ROM },
	{-1}
};

static struct MemoryWriteAddress altbeast_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0x410000, 0x410fff, MWA_TEXTRAM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xc40000, 0xc40fff, MWA_EXTRAM },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};

/***************************************************************************/

void altbeast_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0e98] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0e9a] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0e90] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0e92] );

	set_fg_page( READ_WORD( &sys16_textram[0x0e80] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0e82] ) );

	set_tile_bank( READ_WORD( &sys16_workingram[0xf094] ) );
	set_refresh( READ_WORD( &sys16_extraram[0] ) );
}

void altbeast_init_machine( void ){
	static int bank[16] = {0x00,0x00,0x02,0x00,0x04,0x00,0x06,0x00,0x08,0x00,0x0A,0x00,0x0C,0x00,0x00,0x00};
	sys16_obj_bank = bank;
	sys16_update_proc = altbeast_update_proc;
}

void altbeast_sprite_decode( void ){
	sys16_sprite_decode( 7,0x20000 );
}
/***************************************************************************/

INPUT_PORTS_START( altbeast_input_ports )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START	/* DSW1 */
		PORT_DIPNAME( 0x01, 0x01, "Credits needed", IP_KEY_NONE )
		PORT_DIPSETTING(    0x01, "1 to start, 1 to continue")
		PORT_DIPSETTING(    0x00, "2 to start, 1 to continue")
		PORT_DIPNAME( 0x02, 0x02, "Demo Sounds", IP_KEY_NONE )
		PORT_DIPSETTING(    0x02, "Off" )
		PORT_DIPSETTING(    0x00, "On" )
		PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
		PORT_DIPSETTING(    0x08, "2" )
		PORT_DIPSETTING(    0x0c, "3" )
		PORT_DIPSETTING(    0x04, "4" )
		PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "240", IP_KEY_NONE, IP_JOY_NONE, 0 )
		PORT_DIPNAME( 0x30, 0x30, "Energy Meter", IP_KEY_NONE )
		PORT_DIPSETTING(    0x20, "2" )
		PORT_DIPSETTING(    0x30, "3" )
		PORT_DIPSETTING(    0x10, "4" )
		PORT_DIPSETTING(    0x00, "5" )
		PORT_DIPNAME( 0xc0, 0xc0, "Difficulty", IP_KEY_NONE )
		PORT_DIPSETTING(    0x80, "Easy" )
		PORT_DIPSETTING(    0xc0, "Normal" )
		PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( altbeast_machine_driver, \
	altbeast_readmem,altbeast_writemem,altbeast_init_machine, gfx2 )

struct GameDriver altbeast_driver =
{
	__FILE__,
	0,
	"altbeast",
	"Altered Beast",
	"1988",
	"Sega",
	SYS16_CREDITS,
	0,
	&altbeast_machine_driver,
	0,
	altbeast_rom,
	altbeast_sprite_decode, 0,
	0,
	0,
	altbeast_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( astormbl_rom )
	ROM_REGION( 0x080000 ) /* 68000 code */
	ROM_LOAD_ODD ( "astorm.a5", 0x000000, 0x40000, 0xefe9711e )
	ROM_LOAD_EVEN( "astorm.a6", 0x000000, 0x40000, 0x7682ed3e )

	ROM_REGION( 0xc0000 ) /* tiles */
	ROM_LOAD( "astorm.b1", 0x00000, 0x40000, 0xdf5d0a61 )
	ROM_LOAD( "astorm.b2", 0x40000, 0x40000, 0x787afab8 )
	ROM_LOAD( "astorm.b3", 0x80000, 0x40000, 0x4e01b477 )

	ROM_REGION( 0x200000*2 ) /* sprites */
	ROM_LOAD( "astorm.b11", 0x000000, 0x40000, 0xa782b704 )
	ROM_LOAD( "astorm.a11", 0x040000, 0x40000, 0x7829c4f3 )
	ROM_LOAD( "astorm.b10", 0x080000, 0x40000, 0xeb510228 )
	ROM_LOAD( "astorm.a10", 0x0c0000, 0x40000, 0x3b6b4c55 )
	ROM_LOAD( "astorm.b9",  0x100000, 0x40000, 0xe668eefb )
	ROM_LOAD( "astorm.a9",  0x140000, 0x40000, 0x2293427d )
	ROM_LOAD( "astorm.b8",  0x180000, 0x40000, 0xde9221ed )
	ROM_LOAD( "astorm.a8",  0x1c0000, 0x40000, 0x8c9a71c4 )

	ROM_REGION( 0x10000 ) /* sound CPU */
	//ROM_LOAD( "ab11671.bin", 0x0000, 0x8000, 0x0 )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress astormbl_readmem[] =
{
	{ 0x000000, 0x07ffff, MRA_ROM },

	{ 0x100000, 0x10ffff, MRA_TILERAM },
	{ 0x110000, 0x11ffff, MRA_TEXTRAM },
	{ 0x140000, 0x140fff, MRA_PALETTERAM },
	{ 0x200000, 0x200fff, MRA_SPRITERAM },
	{ 0xa00000, 0xa00001, io_dip1_r },
	{ 0xa00002, 0xa00003, io_dip2_r },
	{ 0xa01002, 0xa01003, io_player1_r },
	{ 0xa01004, 0xa01005, io_player2_r },
	{ 0xa01006, 0xa01007, io_player3_r },
	{ 0xa01000, 0xa01001, io_service_r },
	{ 0xa00000, 0xa00fff, MRA_EXTRAM2 },
	{ 0xc00000, 0xc0ffff, MRA_EXTRAM },
	{ 0xc40000, 0xc40fff, MRA_EXTRAM3 },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{-1}
};

static struct MemoryWriteAddress astormbl_writemem[] =
{
	{ 0x000000, 0x07ffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_TILERAM },
	{ 0x110000, 0x111fff, MWA_TEXTRAM },
	{ 0x140000, 0x14ffff, MWA_PALETTERAM },
	{ 0x200000, 0x20ffff, MWA_SPRITERAM },
	{ 0xa00000, 0xa00fff, MWA_EXTRAM2 },
	{ 0xc00000, 0xc0ffff, MWA_EXTRAM },
	{ 0xc40000, 0xc40fff, MWA_EXTRAM3 },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void astormbl_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0e98] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0e9a] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0e90] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0e92] );

	set_fg_page( READ_WORD( &sys16_textram[0x0e80] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0e82] ) );
	set_tile_bank( READ_WORD( &sys16_extraram2[0] ) ); // 0xa0000f
	set_refresh( READ_WORD( &sys16_extraram3[0] ) ); // 0xc40001
}

void astormbl_init_machine( void ){
	static int bank[16] = {0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,0x10,0x12,0x14,0x16,0x18,0x1A,0x1C,0x1E};
	sys16_obj_bank = bank;

	patch_code( 0x2D6E, 0x32 );
	patch_code( 0x2D6F, 0x3c );
	patch_code( 0x2D70, 0x80 );
	patch_code( 0x2D71, 0x00 );
	patch_code( 0x2D72, 0x33 );
	patch_code( 0x2D73, 0xc1 );
	patch_code( 0x2ea2, 0x30 );
	patch_code( 0x2ea3, 0x38 );
	patch_code( 0x2ea4, 0xec );
	patch_code( 0x2ea5, 0xf6 );
	patch_code( 0x2ea6, 0x30 );
	patch_code( 0x2ea7, 0x80 );
	patch_code( 0x2e5c, 0x30 );
	patch_code( 0x2e5d, 0x38 );
	patch_code( 0x2e5e, 0xec );
	patch_code( 0x2e5f, 0xe2 );
	patch_code( 0x2e60, 0xc0 );
	patch_code( 0x2e61, 0x7c );
	patch_code( 0x4cd8, 0x02 );
	patch_code( 0x4cec, 0x03 );
	patch_code( 0x2dc6c, 0xe9 );
	patch_code( 0x2dc64, 0x10 );
	patch_code( 0x2dc65, 0x10 );
	patch_code( 0x3a100, 0x10 );
	patch_code( 0x3a101, 0x13 );
	patch_code( 0x3a102, 0x90 );
	patch_code( 0x3a103, 0x2b );
	patch_code( 0x3a104, 0x00 );
	patch_code( 0x3a105, 0x01 );
	patch_code( 0x3a106, 0x0c );
	patch_code( 0x3a107, 0x00 );
	patch_code( 0x3a108, 0x00 );
	patch_code( 0x3a109, 0x01 );
	patch_code( 0x3a10a, 0x66 );
	patch_code( 0x3a10b, 0x06 );
	patch_code( 0x3a10c, 0x42 );
	patch_code( 0x3a10d, 0x40 );
	patch_code( 0x3a10e, 0x54 );
	patch_code( 0x3a10f, 0x8b );
	patch_code( 0x3a110, 0x60 );
	patch_code( 0x3a111, 0x02 );
	patch_code( 0x3a112, 0x30 );
	patch_code( 0x3a113, 0x1b );
	patch_code( 0x3a114, 0x34 );
	patch_code( 0x3a115, 0xc0 );
	patch_code( 0x3a116, 0x34 );
	patch_code( 0x3a117, 0xdb );
	patch_code( 0x3a118, 0x24 );
	patch_code( 0x3a119, 0xdb );
	patch_code( 0x3a11a, 0x24 );
	patch_code( 0x3a11b, 0xdb );
	patch_code( 0x3a11c, 0x4e );
	patch_code( 0x3a11d, 0x75 );
	patch_code( 0xaf8e, 0x66 );

	//mirror( 0xa00007, 0xfe0007, 0xff );
	sys16_update_proc = astormbl_update_proc;
}

void astormbl_sprite_decode( void ){
	sys16_sprite_decode( 4,0x080000 );
}
/***************************************************************************/

INPUT_PORTS_START( astormbl_input_ports )
//dipswitch:0x01 0 START_CREDIT ONE TWO _ _ _
//dipswitch:0x02 1 ADVERTISE_SOUND OFF ON _ _ _
//dipswitch:0x1C 2 GAME_DIFFICULTY NORMAL HARDEST HARDER HARD EASY
//labeljoy:ATTACK ROLL SPECIAL _SERVICE DOWN UP RIGHT LEFT
//labelgen:COIN_P3 COIN_P2 TEST ALL_SERVICE 1P_START 2P_START 3P_START COIN_P1
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
	SYS16_JOY3
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( astormbl_machine_driver, \
	astormbl_readmem,astormbl_writemem,astormbl_init_machine, gfx4 )

struct GameDriver astormbl_driver =
{
	__FILE__,
	0,
	"astormbl",
	"Alien Storm (bootleg)",
	"????",
	"bootleg",
	SYS16_CREDITS,
	GAME_NOT_WORKING,
	&astormbl_machine_driver,
	0,
	astormbl_rom,
	astormbl_sprite_decode, 0,
	0,
	0,
	astormbl_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************

   Aurail

***************************************************************************/

ROM_START( aurail_rom )
	ROM_REGION( 0x100000 ) /* 68000 code */
	ROM_LOAD_ODD ( "13576", 0x000000, 0x20000, 0x1e428d94 )
	ROM_LOAD_EVEN( "13577", 0x000000, 0x20000, 0x6701b686 )
	ROM_LOAD_ODD ( "13445", 0x040000, 0x20000, 0x28dfc3dd )
	ROM_LOAD_EVEN( "13447", 0x040000, 0x20000, 0x70a52167 )

	ROM_REGION( 0xc0000 ) /* tiles */
	ROM_LOAD( "aurail.a14", 0x00000, 0x20000, 0x0fc4a7a8 ) /* plane 1 */
	ROM_LOAD( "aurail.b14", 0x20000, 0x20000, 0xe08135e0 )
	ROM_LOAD( "aurail.a15", 0x40000, 0x20000, 0x1c49852f ) /* plane 2 */
	ROM_LOAD( "aurail.b15", 0x60000, 0x20000, 0xe14c6684 )
	ROM_LOAD( "aurail.a16", 0x80000, 0x20000, 0x047bde5e ) /* plane 3 */
	ROM_LOAD( "aurail.b16", 0xa0000, 0x20000, 0x6309fec4 )

	ROM_REGION( 0x200000*2 ) /* sprites */
	ROM_LOAD( "aurail.b1",  0x000000, 0x020000, 0x5fa0a9f8 )
	ROM_LOAD( "aurail.b5",  0x020000, 0x020000, 0x0d1b54da )
	ROM_LOAD( "aurail.b2",  0x040000, 0x020000, 0x5f6b33b1 )
	ROM_LOAD( "aurail.b6",  0x060000, 0x020000, 0xbad340c3 )
	ROM_LOAD( "aurail.b3",  0x080000, 0x020000, 0x4e80520b )
	ROM_LOAD( "aurail.b7",  0x0a0000, 0x020000, 0x7e9165ac )
	ROM_LOAD( "aurail.b4",  0x0c0000, 0x020000, 0x5733c428 )
	ROM_LOAD( "aurail.b8",  0x0e0000, 0x020000, 0x66b8f9b3 )
	ROM_LOAD( "aurail.a1",  0x100000, 0x020000, 0x4f370b2b )
	ROM_LOAD( "aurail.b10", 0x120000, 0x020000, 0xf76014bf )
	ROM_LOAD( "aurail.a2",  0x140000, 0x020000, 0x37cf9cb4 )
	ROM_LOAD( "aurail.b11", 0x160000, 0x020000, 0x1061e7da )
	ROM_LOAD( "aurail.a3",  0x180000, 0x020000, 0x049698ef )
	ROM_LOAD( "aurail.b12", 0x1a0000, 0x020000, 0x7dbcfbf1 )
	ROM_LOAD( "aurail.a4",  0x1c0000, 0x020000, 0x77a8989e )
	ROM_LOAD( "aurail.b13", 0x1e0000, 0x020000, 0x551df422 )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "13448",      0x0000, 0x8000, 0xb5183fb9 )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress aurail_readmem[] =
{
	{ 0x000000, 0x0fffff, MRA_ROM },
	{ 0x3f0000, 0x3fffff, MRA_EXTRAM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x410000, 0x410fff, MRA_TEXTRAM },
	{ 0x440000, 0x440fff, MRA_SPRITERAM },
	{ 0x840000, 0x840fff, MRA_PALETTERAM },
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },
	{ 0xfc0000, 0xfc0fff, MRA_EXTRAM3 },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{-1}
};

static struct MemoryWriteAddress aurail_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM },
	{ 0x3f0000, 0x3fffff, MWA_EXTRAM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x410000, 0x410fff, MWA_TEXTRAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xc40000, 0xc40fff, MWA_EXTRAM2 },
	{ 0xfc0000, 0xfc0fff, MWA_EXTRAM3 },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void aurail_update_proc (void)
{
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0e98] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0e9a] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0e90] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0e92] );

	set_fg_page( READ_WORD( &sys16_textram[0x0e80] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0e82] ) );

	set_tile_bank( READ_WORD( &sys16_extraram3[0x0002] ) );
	set_refresh( READ_WORD( &sys16_extraram2[0] ) );
}

void aurail_init_machine( void ){
	static int bank[16] = {0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,0x10,0x12,0x14,0x16,0x18,0x1A,0x1C,0x1E};
	sys16_obj_bank = bank;

	copy_rom64k( 0x8, 0x4 );
	copy_rom64k( 0x9, 0x5 );
	copy_rom64k( 0xA, 0x6 );
	copy_rom64k( 0xB, 0x7 );
	copy_rom64k( 0x4, 0x0 );
	copy_rom64k( 0x5, 0x1 );
	copy_rom64k( 0x6, 0x2 );
	copy_rom64k( 0x7, 0x3 );

	sys16_update_proc = aurail_update_proc;
}

void aurail_sprite_decode (void)
{
	sys16_sprite_decode (8,0x40000);
}

/***************************************************************************/

INPUT_PORTS_START( aurail_input_ports )
//dipswitch:0x01 0 CREDITS_TO_START ONE TWO _ _ _
//dipswitch:0x02 1 ADVERTISE_SOUND OFF ON _ _ _
//dipswitch:0x0C 2 NUMBER_OF_PLAYER 3 2 4 240 _
//dipswitch:0x30 4 PLAYER_METER 3 2 4 5 _
//dipswitch:0xC0 6 GAME_DIFFICULTY NORMAL EASY HARD HARDEST _
//labeljoy:JUMP PUNCH KICK _ DOWN UP RIGHT LEFT
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START _ _
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( aurail_machine_driver, \
	aurail_readmem,aurail_writemem,aurail_init_machine, gfx4 )

struct GameDriver aurail_driver =
{
	__FILE__,
	0,
	"aurail",
	"Aurail (bootleg)",
	"1990",
	"bootleg",
	SYS16_CREDITS,
	GAME_NOT_WORKING,
	&aurail_machine_driver,
	0,
	aurail_rom,
	aurail_sprite_decode, 0,
	0,
	0,
	aurail_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( dduxbl_rom )
	ROM_REGION( 0x0c0000 ) /* 68000 code */
	ROM_LOAD_ODD ( "dduxb05.bin", 0x000000, 0x20000, 0x459d1237 )
	ROM_LOAD_EVEN( "dduxb03.bin", 0x000000, 0x20000, 0xe7526012 )
	ROM_LOAD_ODD ( "dduxb05.bin", 0x040000, 0x20000, 0x459d1237 )
	ROM_LOAD_EVEN( "dduxb03.bin", 0x040000, 0x20000, 0xe7526012 )
	ROM_LOAD_ODD ( "dduxb04.bin", 0x080000, 0x20000, 0x30c6cb92 )
	ROM_LOAD_EVEN( "dduxb02.bin", 0x080000, 0x20000, 0xd8ed3132 )

	ROM_REGION( 0x30000 ) /* tiles */
	ROM_LOAD( "dduxb14.bin", 0x00000, 0x10000, 0x664bd135 )
	ROM_LOAD( "dduxb15.bin", 0x10000, 0x10000, 0xce0d2b30 )
	ROM_LOAD( "dduxb16.bin", 0x20000, 0x10000, 0x6de95434 )

	ROM_REGION( 0x080000*2 ) /* sprites */
	ROM_LOAD( "dduxb10.bin", 0x000000, 0x010000, 0x0be3aee5 )
	ROM_LOAD( "dduxb06.bin", 0x010000, 0x010000, 0xb0079e99 )
	ROM_LOAD( "dduxb11.bin", 0x020000, 0x010000, 0xcfb2af18 )
	ROM_LOAD( "dduxb07.bin", 0x030000, 0x010000, 0x0217369c )
	ROM_LOAD( "dduxb12.bin", 0x040000, 0x010000, 0x28ce9b15 )
	ROM_LOAD( "dduxb08.bin", 0x050000, 0x010000, 0x8844f336 )
	ROM_LOAD( "dduxb13.bin", 0x060000, 0x010000, 0xefe57759 )
	ROM_LOAD( "dduxb09.bin", 0x070000, 0x010000, 0x6b64f665 )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "dduxb01.bin", 0x0000, 0x8000, 0x0dbef0d7 )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress dduxbl_readmem[] =
{
	{ 0x000000, 0x0bffff, MRA_ROM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x410000, 0x41ffff, MRA_TEXTRAM },
	{ 0x440000, 0x44ffff, MRA_SPRITERAM },
	{ 0x840000, 0x84ffff, MRA_PALETTERAM },
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{-1}
};

static struct MemoryWriteAddress dduxbl_writemem[] =
{
	{ 0x000000, 0x0bffff, MWA_ROM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x410000, 0x41ffff, MWA_TEXTRAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xc40000, 0xc4ffff, MWA_EXTRAM2 },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void dduxbl_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_extraram2[0x6018] );
	sys16_bg_scrollx = READ_WORD( &sys16_extraram2[0x6008] );
	sys16_fg_scrolly = READ_WORD( &sys16_extraram2[0x6010] );
	sys16_bg_scrolly = READ_WORD( &sys16_extraram2[0x6000] );

	set_fg_page( READ_WORD( &sys16_extraram2[0x6126] ) );
	set_bg_page( READ_WORD( &sys16_extraram2[0x6122] ) );
	set_refresh( READ_WORD( &sys16_extraram2[0] ) );
}

void dduxbl_init_machine( void ){
	static int bank[16] = {00,00,00,00,00,00,00,0x06,00,00,00,0x04,00,0x02,00,00};
	sys16_obj_bank = bank;

	patch_code( 0x3932e, 0x4e );
	patch_code( 0x3932f, 0x71 );
	patch_code( 0x3935a, 0x4e );
	patch_code( 0x3935b, 0x71 );
	patch_code( 0x3934e, 0x00 );
	patch_code( 0x1eb2e, 0x01 );
	patch_code( 0x1eb2f, 0x01 );
	patch_code( 0x1eb3c, 0x00 );
	patch_code( 0x1eb3d, 0x00 );
	patch_code( 0x23132, 0x01 );
	patch_code( 0x23133, 0x01 );
	patch_code( 0x23140, 0x00 );
	patch_code( 0x23141, 0x00 );
	patch_code( 0x24a9a, 0x01 );
	patch_code( 0x24a9b, 0x01 );
	patch_code( 0x24aa8, 0x00 );
	patch_code( 0x24aa9, 0x00 );
	//mirror( 0xc40007, 0xfe0007, 0xff );
	sys16_update_proc = dduxbl_update_proc;
//	sys16_sprxoffset = -112;
}

void dduxbl_sprite_decode (void)
{
	int i;

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x30000; i++)
		Machine->memory_region[1][i] ^= 0xff;

	sys16_sprite_decode( 4,0x020000 );
}
/***************************************************************************/

INPUT_PORTS_START( dduxbl_input_ports )
//dipswitch:0x01 0 ADVERTISE_SOUND OFF ON _ _ _
//dipswitch:0x06 1 GANE_DIFFICULTY NORMAL EASY HARD HARDEST _
//dipswitch:0x18 3 PLAYER_NUMBERS 3 2 4 5 _
//dipswitch:0x60 5 EXTEND_PLAYER 200000 150000 300000 400000 _
//labeljoy:_ FIRE JUMP _ DOWN UP RIGHT LEFT
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START _ _
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( dduxbl_machine_driver, \
	dduxbl_readmem,dduxbl_writemem,dduxbl_init_machine, gfx1 )

struct GameDriver dduxbl_driver =
{
	__FILE__,
	0,
	"dduxbl",
	"Dynamite Dux (bootleg)",
	"1989",
	"bootleg",
	SYS16_CREDITS,
	0,
	&dduxbl_machine_driver,
	0,
	dduxbl_rom,
	dduxbl_sprite_decode, 0,
	0,
	0,
	dduxbl_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( eswatbl_rom )
	ROM_REGION( 0x050000 ) /* 68000 code */
	ROM_LOAD_ODD ( "eswat_f.rom", 0x000000, 0x10000, 0xf7b2d388 )
	ROM_LOAD_EVEN( "eswat_c.rom", 0x000000, 0x10000, 0x1028cc81 )
	ROM_LOAD_ODD ( "eswat_e.rom", 0x020000, 0x10000, 0x937ddf9a )
	ROM_LOAD_EVEN( "eswat_b.rom", 0x020000, 0x10000, 0x87c6b1b5 )
	ROM_LOAD_ODD ( "eswat_d.rom", 0x040000, 0x08000, 0xb4751e19 )
	ROM_LOAD_EVEN( "eswat_a.rom", 0x040000, 0x08000, 0x2af4fc62 )

	ROM_REGION( 0xc0000 ) /* tiles */
	ROM_LOAD( "ic19.bin", 0x00000, 0x40000, 0x375a5ec4 )
	ROM_LOAD( "ic20.bin", 0x40000, 0x40000, 0x3b8c757e )
	ROM_LOAD( "ic21.bin", 0x80000, 0x40000, 0x3efca25c )

	ROM_REGION( 0x180000*2 ) /* sprites */
	ROM_LOAD( "ic9.bin",  0x000000, 0x040000, 0x0d1530bf )
	ROM_LOAD( "ic12.bin", 0x040000, 0x040000, 0x18ff0799 )
	ROM_LOAD( "ic10.bin", 0x080000, 0x040000, 0x32069246 )
	ROM_LOAD( "ic13.bin", 0x0c0000, 0x040000, 0xa3dfe436 )
	ROM_LOAD( "ic11.bin", 0x100000, 0x040000, 0xf6b096e0 )
	ROM_LOAD( "ic14.bin", 0x140000, 0x040000, 0x6773fef6 )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "ic8.bin", 0x0000, 0x8000, 0x7efecf23 )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress eswatbl_readmem[] =
{
	{ 0x000000, 0x04ffff, MRA_ROM },
	{ 0x3e0000, 0xe3ffff, MRA_EXTRAM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x410000, 0x41ffff, MRA_TEXTRAM },
	{ 0x440000, 0x44ffff, MRA_SPRITERAM },
	{ 0x840000, 0x84ffff, MRA_PALETTERAM },
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },
	{ 0xc80000, 0xc8ffff, MRA_EXTRAM3 },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{-1}
};

static struct MemoryWriteAddress eswatbl_writemem[] =
{
	{ 0x000000, 0x04ffff, MWA_ROM },
	{ 0x3e0000, 0xe3ffff, MWA_EXTRAM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x410000, 0x41ffff, MWA_TEXTRAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xc40000, 0xc4ffff, MWA_EXTRAM2 },
	{ 0xc80000, 0xc8ffff, MWA_EXTRAM3 },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void eswatbl_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x8008] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x8018] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x8000] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x8010] );

	set_fg_page( READ_WORD( &sys16_textram[0x8020] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x8028] ) );
	set_tile_bank( READ_WORD( &sys16_textram[0x8030] ) );
	set_refresh( READ_WORD( &sys16_extraram2[0] ) );
}

void eswatbl_init_machine( void ){
	static int bank[16] = { 0,2,8,10,16,18,24,26,4,6,12,14,20,22,28,30};
	sys16_obj_bank = bank;

//	patch_code( 0x65c, 0x4e );
//	patch_code( 0x65d, 0x71 );
	patch_code( 0x9ec, 0x4e );
	patch_code( 0x9ed, 0x71 );
	patch_code( 0x9f8, 0x4e );
	patch_code( 0x9f9, 0x71 );
	patch_code( 0x3897, 0x11 );
	//mirror( 0xc42007, 0xfe0007, 0xff );

	sys16_update_proc = eswatbl_update_proc;
}

void eswatbl_sprite_decode( void ){
	sys16_sprite_decode( 3,0x040000 );
}
/***************************************************************************/

INPUT_PORTS_START( eswatbl_input_ports )
//dipswitch:0x01 0 START_CREDIT 1 2 _ _ _
//dipswitch:0x02 1 ADVERTISE_SOUND OFF ON _ _ _
//dipswitch:0x04 2 DISPLAY_FLIP OFF ON _ _ _
//dipswitch:0x08 3 TIME NORMAL HARD _ _ _
//dipswitch:0x30 4 GAME_DIFFICULTY NORMAL EASY HARD HARDEST _
//dipswitch:0xC0 6 NUMBER_OF_PLAYERS 3 4 2 1 _
//labeljoy:SPECIAL ATTACK JUMP _ DOWN UP RIGHT LEFT
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START _ _
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( eswatbl_machine_driver, \
	eswatbl_readmem,eswatbl_writemem,eswatbl_init_machine, gfx4 )

struct GameDriver eswatbl_driver =
{
	__FILE__,
	0,
	"eswatbl",
	"E-Swat (bootleg)",
	"????",
	"bootleg",
	SYS16_CREDITS,
	GAME_NOT_WORKING,
	&eswatbl_machine_driver,
	0,
	eswatbl_rom,
	eswatbl_sprite_decode, 0,
	0,
	0,
	eswatbl_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( fantzone_rom )
	ROM_REGION( 0x030000 ) /* 68000 code */
	ROM_LOAD_ODD ( "7382.26", 0x000000, 0x8000, 0x3fda7416 )
	ROM_LOAD_EVEN( "7385.43", 0x000000, 0x8000, 0x5cb64450 )
	ROM_LOAD_ODD ( "7383.25", 0x010000, 0x8000, 0xa001e10a )
	ROM_LOAD_EVEN( "7386.42", 0x010000, 0x8000, 0x15810ace )
	ROM_LOAD_ODD ( "7384.24", 0x020000, 0x8000, 0xfd909341 )
	ROM_LOAD_EVEN( "7387.41", 0x020000, 0x8000, 0x0acd335d )

	ROM_REGION( 0x18000 ) /* tiles */
	ROM_LOAD( "7388.95", 0x00000, 0x08000, 0x8eb02f6b )
	ROM_LOAD( "7389.94", 0x08000, 0x08000, 0x2f4f71b8 )
	ROM_LOAD( "7390.93", 0x10000, 0x08000, 0xd90609c6 )

	ROM_REGION( 0x030000*2 ) /* sprites */
	ROM_LOAD( "7392.10", 0x000000, 0x008000, 0x5bb7c8b6 )
	ROM_LOAD( "7396.11", 0x008000, 0x008000, 0x74ae4b57 )
	ROM_LOAD( "7393.17", 0x010000, 0x008000, 0x14fc7e82 )
	ROM_LOAD( "7397.18", 0x018000, 0x008000, 0xe05a1e25 )
	ROM_LOAD( "7394.23", 0x020000, 0x008000, 0x531ca13f )
	ROM_LOAD( "7398.24", 0x028000, 0x008000, 0x68807b49 )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "epr7535.fz", 0x0000, 0x8000, 0xbc1374fa )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress fantzone_readmem[] =
{
	{ 0x000000, 0x02ffff, MRA_ROM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x410000, 0x41ffff, MRA_TEXTRAM },
	{ 0x440000, 0x44ffff, MRA_SPRITERAM },
	{ 0x840000, 0x84ffff, MRA_PALETTERAM },
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42000, 0xc42001, io_dip1_r },
	{ 0xc42002, 0xc42003, io_dip2_r },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{-1}
};

static struct MemoryWriteAddress fantzone_writemem[] =
{
	{ 0x000000, 0x02ffff, MWA_ROM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x410000, 0x41ffff, MWA_TEXTRAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void fantzone_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0ff8] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0ffa] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0f24] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0f26] );

	set_fg_page( READ_WORD( &sys16_textram[0x0e9e] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0e9c] ) );
//tile_bank:0x418031
//refresh:0xc40001
}

void fantzone_init_machine( void ){
	static int bank[16] = { 00,01,02,03,00,01,02,03,00,01,02,03,00,01,02,03};
	sys16_obj_bank = bank;

	patch_code( 0x20e7, 0x16 );
	patch_code( 0x30ef, 0x16 );

	//mirror( 0xc40001, 0xfe0007, 0xff );
	sys16_update_proc = fantzone_update_proc;
}

void fantzone_sprite_decode( void ){
	sys16_sprite_decode( 3,0x010000 );
}
/***************************************************************************/

INPUT_PORTS_START( fantzone_input_ports )
//dipswitch:0x01 0 TYPE COCKTAIL_TABLE UPRIGHT _ _ _
//dipswitch:0x02 1 ADVERTISE_SOUND OFF ON _ _ _
//dipswitch:0x0C 2 PLAYERS 3 4 5 6 _
//dipswitch:0xC0 6 DIFFICULTY NORMAL EASY HARD HARDEST _
//labeljoy:_ SHOT BOMB _ DOWN UP RIGHT LEFT
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START _ _
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( fantzone_machine_driver, \
	fantzone_readmem,fantzone_writemem,fantzone_init_machine, gfx8 )

struct GameDriver fantzone_driver =
{
	__FILE__,
	0,
	"fantzone",
	"Fantasy Zone",
	"????",
	"Sega",
	SYS16_CREDITS,
	GAME_NOT_WORKING,
	&fantzone_machine_driver,
	0,
	fantzone_rom,
	fantzone_sprite_decode, 0,
	0,
	0,
	fantzone_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( fpointbl_rom )
	ROM_REGION( 0x020000 ) /* 68000 code */
	ROM_LOAD_ODD ( "flpoint.002", 0x000000, 0x10000, 0x4dff2ee8 )
	ROM_LOAD_EVEN( "flpoint.003", 0x000000, 0x10000, 0x4d6df514 )

	ROM_REGION( 0x30000 ) /* tiles */
	ROM_LOAD( "flpoint.006", 0x00000, 0x10000, 0xc539727d )
	ROM_LOAD( "flpoint.005", 0x10000, 0x10000, 0x82c0b8b0 )
	ROM_LOAD( "flpoint.004", 0x20000, 0x10000, 0x522426ae )

	ROM_REGION( 0x020000*2 ) /* sprites */
	ROM_LOAD( "flpoint.007", 0x000000, 0x010000, 0x4a4041f3 )
	ROM_LOAD( "flpoint.008", 0x010000, 0x010000, 0x6961e676 )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "EPR12592.BIN", 0x0000, 0x8000, 0x9a8c11bb )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress fpointbl_readmem[] =
{
	{ 0x601002, 0x601003, io_player1_r },
	{ 0x601004, 0x601005, io_player2_r },
	{ 0x601000, 0x601001, io_service_r },
	{ 0x602002, 0x602003, io_dip1_r },
	{ 0x602000, 0x602001, io_dip2_r },

	{ 0x410000, 0x410fff, MRA_TEXTRAM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x440000, 0x440fff, MRA_SPRITERAM },
	{ 0x840000, 0x840fff, MRA_PALETTERAM },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x01ffff, MRA_ROM },
	{-1}
};

static struct MemoryWriteAddress fpointbl_writemem[] =
{
	{ 0x000000, 0x01ffff, MWA_ROM },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0x410000, 0x410fff, MWA_TEXTRAM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void fpointbl_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0e98] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0e9a] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0e90] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0e92] );

	set_fg_page( READ_WORD( &sys16_textram[0x0e80] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0e82] ) );
//tile_bank:0x418031
//refresh:0xc40001
}

void fpointbl_init_machine( void ){
	static int bank[16] = {00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00};
	sys16_obj_bank = bank;

	patch_code( 0x454, 0x33 );
	patch_code( 0x455, 0xf8 );
	patch_code( 0x456, 0xe0 );
	patch_code( 0x457, 0xe2 );
	patch_code( 0x8ce8, 0x16 );
	patch_code( 0x8ce9, 0x66 );
	patch_code( 0x17687, 0x00 );
	patch_code( 0x7bed, 0x04 );
	patch_code( 0x7ea8, 0x61 );
	patch_code( 0x7ea9, 0x00 );
	patch_code( 0x7eaa, 0x84 );
	patch_code( 0x7eab, 0x16 );
	patch_code( 0x2c0, 0xe7 );
	patch_code( 0x2c1, 0x48 );
	patch_code( 0x2c2, 0xe7 );
	patch_code( 0x2c3, 0x49 );
	patch_code( 0x2c4, 0x04 );
	patch_code( 0x2c5, 0x40 );
	patch_code( 0x2c6, 0x00 );
	patch_code( 0x2c7, 0x10 );
	patch_code( 0x2c8, 0x4e );
	patch_code( 0x2c9, 0x75 );

	//mirror( 0x601001, 0x44302b, -1 );
	//mirror( 0x601001, 0x443035, -1 );
	//mirror( 0x601001, 0x44303d, -1 );
	//mirror( 0x601001, 0x443045, -1 );
	//mirror( 0x601001, 0x44304d, -1 );
	//mirror( 0x601001, 0x02002e, -1 );
	//mirror( 0x601001, 0x020041, -1 );
	//mirror( 0x601001, 0x020049, -1 );
	//mirror( 0x601001, 0xfe003f, -1 );
	//mirror( 0x600007, 0xfd0007, 0xff );
	sys16_update_proc = fpointbl_update_proc;
}

void fpointbl_sprite_decode (void)
{
	int i;

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x30000; i++)
		Machine->memory_region[1][i] ^= 0xff;

	sys16_sprite_decode( 1,0x020000 );
}
/***************************************************************************/

INPUT_PORTS_START( fpointbl_input_ports )
//dipswitch:0x02 1 ADVERTISE_SOUND OFF ON _ _ _
//dipswitch:0x30 4 GAME_DIFFICULTY NORMAL EASY HARD HARDEST _
//labeljoy:TURN_PIECE_1 TURN_PIECE_2 TURN_PIECE_3 _ DOWN _ RIGHT LEFT
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START _ _
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( fpointbl_machine_driver, \
	fpointbl_readmem,fpointbl_writemem,fpointbl_init_machine, gfx1 )

struct GameDriver fpointbl_driver =
{
	__FILE__,
	0,
	"fpointbl",
	"Flash Point (bootleg)",
	"1989",
	"bootleg",
	SYS16_CREDITS,
	GAME_NOT_WORKING,
	&fpointbl_machine_driver,
	0,
	fpointbl_rom,
	fpointbl_sprite_decode, 0,
	0,
	0,
	fpointbl_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( goldnaxe_rom )
	ROM_REGION( 0x0c0000 ) /* 68000 code */
	ROM_LOAD_ODD ( "ga12522.bin", 0x00000, 0x20000, 0xb6c35160 )
	ROM_LOAD_EVEN( "ga12523.bin", 0x00000, 0x20000, 0x8e6128d7 )
	ROM_LOAD_ODD ( "ga12544.bin", 0x40000, 0x40000, 0x5e38f668 )
	ROM_LOAD_EVEN( "ga12545.bin", 0x40000, 0x40000, 0xa97c4e4d )

	ROM_REGION( 0x60000 ) /* tiles */
	ROM_LOAD( "ga12385.bin", 0x00000, 0x20000, 0xb8a4e7e0 )
	ROM_LOAD( "ga12386.bin", 0x20000, 0x20000, 0x25d7d779 )
	ROM_LOAD( "ga12387.bin", 0x40000, 0x20000, 0xc7fcadf3 )

	ROM_REGION( 0x180000*2 ) /* sprites */
	ROM_LOAD( "ga12378.bin", 0x000000, 0x40000, 0x119e5a82 )
	ROM_LOAD( "ga12379.bin", 0x040000, 0x40000, 0x1a0e8c57 )
	ROM_LOAD( "ga12380.bin", 0x080000, 0x40000, 0xbb2c0853 )
	ROM_LOAD( "ga12381.bin", 0x0c0000, 0x40000, 0x81ba6ecc )
	ROM_LOAD( "ga12382.bin", 0x100000, 0x40000, 0x81601c6f )
	ROM_LOAD( "ga12383.bin", 0x140000, 0x40000, 0x5dbacf7a )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "ga12390.bin", 0x0000, 0x8000, 0x399fc5f5 )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress goldnaxe_readmem[] =
{
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },

	{ 0x110000, 0x110fff, MRA_TEXTRAM },
	{ 0x100000, 0x10ffff, MRA_TILERAM },
	{ 0x200000, 0x200fff, MRA_SPRITERAM },
	{ 0x140000, 0x140fff, MRA_PALETTERAM },
	{ 0x1f0000, 0x1fffff, MRA_EXTRAM },
	{ 0xc40000, 0xc40fff, MRA_EXTRAM2 },
	{ 0xffecd0, 0xffecd1, mirror1_r },
	{ 0xffec96, 0xffec97, mirror2_r },
	{ 0xffecfc, 0xffecfd, mirror3_r },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x0bffff, MRA_ROM },
	{-1}
};

static struct MemoryWriteAddress goldnaxe_writemem[] =
{
	{ 0x000000, 0x0bffff, MWA_ROM },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0x110000, 0x110fff, MWA_TEXTRAM },
	{ 0x100000, 0x10ffff, MWA_TILERAM },
	{ 0x200000, 0x20ffff, MWA_SPRITERAM },
	{ 0x140000, 0x14ffff, MWA_PALETTERAM },
	{ 0x1f0000, 0x1fffff, MWA_EXTRAM },
	{ 0xc40000, 0xc40fff, MWA_EXTRAM2 },
	{ 0xffecd0, 0xffecd1, mirror1_w },
	{ 0xffec96, 0xffec97, mirror2_w },
	{ 0xffecfc, 0xffecfd, mirror3_w },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void goldnaxe_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0e98] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0e9a] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0e90] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0e92] );

	set_fg_page( READ_WORD( &sys16_textram[0x0e80] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0e82] ) );
	set_tile_bank( READ_WORD( &sys16_workingram[0xec94] ) );
	set_refresh( READ_WORD( &sys16_extraram2[0] ) );
}

void goldnaxe_init_machine( void ){
	static int bank[16] = { 0,2,8,10,16,18,0,0,4,6,12,14,20,22,0,0 };
	sys16_obj_bank = bank;

	copy_rom64k( 0x6, 0xA );
	copy_rom64k( 0x7, 0xB );

	patch_code( 0x3CB2, 0x4E );
	patch_code( 0x3CB3, 0x75 );

	define_mirror1( 0xc41003, 0xc41007 );
	define_mirror2( 0xc41001, 0 );
	define_mirror3( 0xfe0007,0 );

	sys16_sprxoffset = -0xb8;
	sys16_update_proc = goldnaxe_update_proc;
}

void goldnaxe_sprite_decode( void ){
	sys16_sprite_decode( 3,0x80000 );
}
/***************************************************************************/

INPUT_PORTS_START( goldnaxe_input_ports )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START	/* DSW1 */
		PORT_DIPNAME( 0x01, 0x01, "Credits needed", IP_KEY_NONE )
		PORT_DIPSETTING(    0x01, "1 to start, 1 to continue")
		PORT_DIPSETTING(    0x00, "2 to start, 1 to continue")
		PORT_DIPNAME( 0x02, 0x02, "Attract Mode Sound", IP_KEY_NONE )
		PORT_DIPSETTING(    0x02, "Off" )
		PORT_DIPSETTING(    0x00, "On" )
		PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
		PORT_DIPSETTING(    0x08, "1" )
		PORT_DIPSETTING(    0x0c, "2" )
		PORT_DIPSETTING(    0x04, "3" )
		PORT_DIPSETTING(    0x00, "4" )
		PORT_DIPNAME( 0x30, 0x30, "Energy Meter", IP_KEY_NONE )
		PORT_DIPSETTING(    0x20, "2" )
		PORT_DIPSETTING(    0x30, "3" )
		PORT_DIPSETTING(    0x10, "4" )
		PORT_DIPSETTING(    0x00, "5" )
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( goldnaxe_machine_driver, \
	goldnaxe_readmem,goldnaxe_writemem,goldnaxe_init_machine, gfx2 )

struct GameDriver goldnaxe_driver =
{
	__FILE__,
	0,
	"goldnaxe",
	"Golden Axe",
	"1989",
	"Sega",
	SYS16_CREDITS,
	0,
	&goldnaxe_machine_driver,
	0,
	goldnaxe_rom,
	goldnaxe_sprite_decode, 0,
	0,
	0,
	goldnaxe_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( hwchamp_rom )
	ROM_REGION( 0x040000 ) /* 68000 code */
	ROM_LOAD_ODD ( "rom0-o.bin", 0x000000, 0x20000, 0x25180124 )
	ROM_LOAD_EVEN( "rom0-e.bin", 0x000000, 0x20000, 0xe5abfed7 )

	ROM_REGION( 0x60000 ) /* tiles */
	ROM_LOAD( "scr11.bin", 0x00000, 0x20000, 0xaeaaa9d8 )
	ROM_LOAD( "scr12.bin", 0x20000, 0x20000, 0x63a82afa )
	ROM_LOAD( "scr13.bin", 0x40000, 0x20000, 0x5b8494a8 )

	ROM_REGION( 0x100000*2 ) /* sprites */
	ROM_LOAD( "obj0-o.bin", 0x000000, 0x020000, 0xfc098a13 )
	ROM_LOAD( "obj0-e.bin", 0x020000, 0x020000, 0x5db934a8 )
	ROM_LOAD( "obj1-o.bin", 0x040000, 0x020000, 0x1f27ee74 )
	ROM_LOAD( "obj1-e.bin", 0x060000, 0x020000, 0x8a6a5cf1 )
	ROM_LOAD( "obj2-o.bin", 0x080000, 0x020000, 0xc0b2ba82 )
	ROM_LOAD( "obj2-e.bin", 0x0a0000, 0x020000, 0xd6c7917b )
	ROM_LOAD( "obj3-o.bin", 0x0c0000, 0x020000, 0x52fa3a49 )
	ROM_LOAD( "obj3-e.bin", 0x0e0000, 0x020000, 0x57e8f9d2 )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "s-prog.bin", 0x0000, 0x8000, 0x96a12d9d )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress hwchamp_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x3f0000, 0x3fffff, MRA_EXTRAM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x410000, 0x41ffff, MRA_TEXTRAM },
	{ 0x440000, 0x44ffff, MRA_SPRITERAM },
	{ 0x840000, 0x84ffff, MRA_PALETTERAM },
	{ 0xc43ffd, 0xc43ffe, io_player1_r },
	{ 0xc43ffd, 0xc43ffe, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{-1}
};

static struct MemoryWriteAddress hwchamp_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x3f0000, 0x3fffff, MWA_EXTRAM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x410000, 0x41ffff, MWA_TEXTRAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void hwchamp_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0e98] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0e9a] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0e90] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0e92] );

	set_fg_page( READ_WORD( &sys16_textram[0x0e80] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0e82] ) );
	set_tile_bank( READ_WORD( &sys16_extraram[0xec94] ) );
//tile_bank:0x3f0003
//refresh:0xc40001
}

void hwchamp_init_machine( void ){
	static int bank[16] = {0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30};
	sys16_obj_bank = bank;

	sys16_update_proc = hwchamp_update_proc;
}

void hwchamp_sprite_decode( void ){
	sys16_sprite_decode( 4,0x040000 );
}
/***************************************************************************/

INPUT_PORTS_START( hwchamp_input_ports )
//dipswitch:0x01 0 GAME_TYPE COCKTAIL_TABLE UPRIGHT _ _ _
//dipswitch:0x02 1 ADVERTISE_SOUND OFF ON _ _ _
//dipswitch:0x0C 2 NUMBER_OF_LIVES 3 2 5 FREE _
//dipswitch:0x30 4 DIFFICULTY NORMAL EASY HARD HARDEST _
//dipswitch:0x40 6 BULLET'S_SPEED SLOW FAST _ _ _
//dipswitch:0x80 7 LANGUAGE JAPANESE ENGLISH _ _ _
//labeljoy:_ _ _ _ _ _ _ _
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START _ _
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( hwchamp_machine_driver, \
	hwchamp_readmem,hwchamp_writemem,hwchamp_init_machine, gfx2 )

struct GameDriver hwchamp_driver =
{
	__FILE__,
	0,
	"hwchamp",
	"Heavyweight Champ",
	"????",
	"?????",
	SYS16_CREDITS,
	GAME_NOT_WORKING,
	&hwchamp_machine_driver,
	0,
	hwchamp_rom,
	hwchamp_sprite_decode, 0,
	0,
	0,
	hwchamp_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( mjleague_rom )
	ROM_REGION( 0x030000 ) /* 68000 code */
	ROM_LOAD_ODD ( "epr-7401.06b", 0x000000, 0x8000, 0x2befa5e0 )
	ROM_LOAD_EVEN( "epr-7404.09b", 0x000000, 0x8000, 0xec1655b5 )
	ROM_LOAD_ODD ( "epr-7402.07b", 0x010000, 0x8000, 0xb7bef762 )
	ROM_LOAD_EVEN( "epr-7405.10b", 0x010000, 0x8000, 0x7a4f4e38 )
	ROM_LOAD_ODD ( "epra7403.08b", 0x020000, 0x8000, 0xd86250cf )
	ROM_LOAD_EVEN( "epra7406.11b", 0x020000, 0x8000, 0xbb743639 )

	ROM_REGION( 0x18000 ) /* tiles */
	ROM_LOAD( "epr-7051.09a", 0x00000, 0x08000, 0x10ca255a )
	ROM_LOAD( "epr-7052.10a", 0x08000, 0x08000, 0x2550db0e )
	ROM_LOAD( "epr-7053.11a", 0x10000, 0x08000, 0x5bfea038 )

	ROM_REGION( 0x050000*2 ) /* sprites */
	ROM_LOAD( "epr-7055.05a", 0x000000, 0x008000, 0x1fb860bd )
	ROM_LOAD( "epr-7059.02b", 0x008000, 0x008000, 0x3d14091d )
	ROM_LOAD( "epr-7056.06a", 0x010000, 0x008000, 0xb35dd968 )
	ROM_LOAD( "epr-7060.03b", 0x018000, 0x008000, 0x61bb3757 )
	ROM_LOAD( "epr-7057.07a", 0x020000, 0x008000, 0x3e5a2b6f )
	ROM_LOAD( "epr-7061.04b", 0x028000, 0x008000, 0xc808dad5 )
	ROM_LOAD( "epr-7058.08a", 0x030000, 0x008000, 0xb543675f )
	ROM_LOAD( "epr-7062.05b", 0x038000, 0x008000, 0x9168eb47 )
	ROM_LOAD( "epr-7055.05a", 0x040000, 0x008000, 0x1fb860bd )
	ROM_LOAD( "epr-7059.02b", 0x048000, 0x008000, 0x3d14091d )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "eprc7054.01b", 0x0000, 0x8000, 0x4443b744 )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress mjleague_readmem[] =
{
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },

	{ 0x410000, 0x410fff, MRA_TEXTRAM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x440000, 0x440fff, MRA_SPRITERAM },
	{ 0x840000, 0x840fff, MRA_PALETTERAM },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x02ffff, MRA_ROM },
	{-1}
};

static struct MemoryWriteAddress mjleague_writemem[] =
{
	{ 0x000000, 0x02ffff, MWA_ROM },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0x410000, 0x410fff, MWA_TEXTRAM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void mjleague_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0ff8] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0ffa] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0f24] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0f26] );

	set_fg_page( READ_WORD( &sys16_textram[0x0e8e] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0e8c] ) );
//tile_bank:0x3f0003
//refresh:0xc40001
}

void mjleague_init_machine( void ){
	static int bank[16] = { 00,01,02,03,00,01,02,03,00,01,02,03,00,01,02,03};
	sys16_obj_bank = bank;

	patch_code( 0xBD42, 0x66 );

	//mirror( 0xc40001, 0xfe0007, 0xff );
	sys16_update_proc = mjleague_update_proc;
}

void mjleague_sprite_decode( void ){
	sys16_sprite_decode( 5,0x010000 );
}
/***************************************************************************/

INPUT_PORTS_START( mjleague_input_ports )
//dipswitch:0x01 0 TYPE COCKTAIL_TABLE UPRIGHT _ _ _
//dipswitch:0x02 1 ADVERTISE_SOUND OFF ON _ _ _
//dipswitch:0x04 2 SW2 OFF ON _ _ _
//dipswitch:0x08 3 SW3 OFF ON _ _ _
//dipswitch:0x10 4 SW4 OFF ON _ _ _
//dipswitch:0x20 5 SW5 OFF ON _ _ _
//dipswitch:0x40 6 SW6 OFF ON _ _ _
//dipswitch:0x80 7 SW7 OFF ON _ _ _
//labeljoy:B1 B2 B3 _ DOWN UP RIGHT LEFT
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START _ _
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( mjleague_machine_driver, \
	mjleague_readmem,mjleague_writemem,mjleague_init_machine, gfx8 )

struct GameDriver mjleague_driver =
{
	__FILE__,
	0,
	"mjleague",
	"Major League",
	"1985",
	"Sega",
	SYS16_CREDITS,
	GAME_NOT_WORKING,
	&mjleague_machine_driver,
	0,
	mjleague_rom,
	mjleague_sprite_decode, 0,
	0,
	0,
	mjleague_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( passshtb_rom )
	ROM_REGION( 0x020000 ) /* 68000 code */
	ROM_LOAD_ODD ( "pass4_2p.bin", 0x000000, 0x10000, 0x06ac6d5d )
	ROM_LOAD_EVEN( "pass3_2p.bin", 0x000000, 0x10000, 0x26bb9299 )

	ROM_REGION( 0x30000 ) /* tiles */
	ROM_LOAD( "passshot.b9",  0x00000, 0x10000, 0xd31c0b6c )
	ROM_LOAD( "passshot.b10", 0x10000, 0x10000, 0xb78762b4 )
	ROM_LOAD( "passshot.b11", 0x20000, 0x10000, 0xea49f666 )

	ROM_REGION( 0x60000*2 ) /* sprites */
	ROM_LOAD( "passshot.b1", 0x00000, 0x010000, 0xb6e94727 )
	ROM_LOAD( "passshot.b5", 0x10000, 0x010000, 0x17e8d5d5 )
	ROM_LOAD( "passshot.b2", 0x20000, 0x010000, 0x3e670098 )
	ROM_LOAD( "passshot.b6", 0x30000, 0x010000, 0x50eb71cc )
	ROM_LOAD( "passshot.b3", 0x40000, 0x010000, 0x05733ca8 )
	ROM_LOAD( "passshot.b7", 0x50000, 0x010000, 0x81e49697 )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "passshot.a7", 0x0000, 0x8000, 0x789edc06 )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress passshtb_readmem[] =
{
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },

	{ 0x410000, 0x410fff, MRA_TEXTRAM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x440000, 0x440fff, MRA_SPRITERAM },
	{ 0x840000, 0x840fff, MRA_PALETTERAM },
	{ 0xc40000, 0xc40fff, MRA_EXTRAM },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x01ffff, MRA_ROM },
	{-1}
};

static struct MemoryWriteAddress passshtb_writemem[] =
{
	{ 0x000000, 0x01ffff, MWA_ROM },
	{ 0xc42006, 0xc42007, sound_command_w },
	{ 0x410000, 0x410fff, MWA_TEXTRAM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xc40000, 0xc40fff, MWA_EXTRAM },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void passshtb_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_workingram[0xf4be] );
	sys16_bg_scrollx = READ_WORD( &sys16_workingram[0xf4c2] );
	sys16_fg_scrolly = READ_WORD( &sys16_workingram[0xf4bc] );
	sys16_bg_scrolly = READ_WORD( &sys16_workingram[0xf4c0] );

	set_fg_page( READ_WORD( &sys16_textram[0x0ff6] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0ff4] ) );
	set_refresh( READ_WORD( &sys16_extraram[0] ) );
}

void passshtb_init_machine( void ){
	static int bank[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,3 };
	sys16_obj_bank = bank;

	sys16_sprxoffset = -0x48;
	sys16_spritesystem = 0;
	sys16_update_proc = passshtb_update_proc;
}

void passshtb_sprite_decode( void ){
	sys16_sprite_decode( 3,0x20000 );
}
/***************************************************************************/

INPUT_PORTS_START( passshtb_input_ports )
//labeljoy:LOB FLAT SPIN _ DOWN UP RIGHT LEFT
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START _ _
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE

	PORT_START	/* DSW1 */
		PORT_DIPNAME( 0x01, 0x01, "Attract Mode Sound", IP_KEY_NONE )
		PORT_DIPSETTING(    0x01, "Off" )
		PORT_DIPSETTING(    0x00, "On" )
		PORT_DIPNAME( 0x0e, 0x0e, "Initial Point", IP_KEY_NONE )
		PORT_DIPSETTING(    0x06, "2000" )
		PORT_DIPSETTING(    0x0a, "3000" )
		PORT_DIPSETTING(    0x0c, "4000" )
		PORT_DIPSETTING(    0x0e, "5000" )
		PORT_DIPSETTING(    0x08, "6000" )
		PORT_DIPSETTING(    0x04, "7000" )
		PORT_DIPSETTING(    0x02, "8000" )
		PORT_DIPSETTING(    0x00, "9000" )
		PORT_DIPNAME( 0x30, 0x30, "Point Table", IP_KEY_NONE )
		PORT_DIPSETTING(    0x20, "Easy" )
		PORT_DIPSETTING(    0x30, "Normal" )
		PORT_DIPSETTING(    0x10, "Hard" )
		PORT_DIPSETTING(    0x00, "Hardest" )
		PORT_DIPNAME( 0xc0, 0xc0, "Game Difficulty", IP_KEY_NONE )
		PORT_DIPSETTING(    0x80, "Easy" )
		PORT_DIPSETTING(    0xc0, "Normal" )
		PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( passshtb_machine_driver, \
	passshtb_readmem,passshtb_writemem,passshtb_init_machine, gfx1 )

struct GameDriver passshtb_driver =
{
	__FILE__,
	0,
	"passshtb",
	"Passing Shot (bootleg)",
	"????",
	"bootleg",
	SYS16_CREDITS,
	0,
	&passshtb_machine_driver,
	0,
	passshtb_rom,
	passshtb_sprite_decode, 0,
	0,
	0,
	passshtb_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( quartet2_rom )
	ROM_REGION( 0x030000 ) /* 68000 code */
	ROM_LOAD_ODD ( "quartet2.b6",  0x000000, 0x8000, 0x50f50b08 )
	ROM_LOAD_EVEN( "quartet2.b9",  0x000000, 0x8000, 0x67177cd8 )
	ROM_LOAD_ODD ( "quartet2.b7",  0x010000, 0x8000, 0x0aa337bb )
	ROM_LOAD_EVEN( "quartet2.b10", 0x010000, 0x8000, 0x4273c3b7 )
	ROM_LOAD_ODD ( "quartet2.b8",  0x020000, 0x8000, 0xd87b2ca2 )
	ROM_LOAD_EVEN( "quartet2.b11", 0x020000, 0x8000, 0x3a6a375d )

	ROM_REGION( 0x18000 ) /* tiles */
	ROM_LOAD( "quartet2.c9",  0x00000, 0x08000, 0x547a6058 )
	ROM_LOAD( "quartet2.c10", 0x08000, 0x08000, 0x77ec901d )
	ROM_LOAD( "quartet2.c11", 0x10000, 0x08000, 0x7e348cce )

	ROM_REGION( 0x050000*2 ) /* sprites */
	ROM_LOAD( "quartet2.c5", 0x000000, 0x008000, 0x8a1ab7d7 )
	ROM_LOAD( "quartet2.b2", 0x008000, 0x008000, 0xcb65ae4f )
	ROM_LOAD( "quartet2.c6", 0x010000, 0x008000, 0xb2d3f4f3 )
	ROM_LOAD( "quartet2.b3", 0x018000, 0x008000, 0x16fc67b1 )
	ROM_LOAD( "quartet2.c7", 0x020000, 0x008000, 0x0af68de2 )
	ROM_LOAD( "quartet2.b4", 0x028000, 0x008000, 0x13fad5ac )
	ROM_LOAD( "quartet2.c8", 0x030000, 0x008000, 0xddfd40c0 )
	ROM_LOAD( "quartet2.b5", 0x038000, 0x008000, 0x8e2762ec )
	ROM_LOAD( "quartet2.c5", 0x040000, 0x008000, 0x8a1ab7d7 )
	ROM_LOAD( "quartet2.b2", 0x048000, 0x008000, 0xcb65ae4f )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "quartet2.b1", 0x0000, 0x8000, 0x9f291306 )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress quartet2_readmem[] =
{
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42000, 0xc42001, io_dip1_r },
	{ 0xc42002, 0xc42003, io_dip2_r },

	{ 0x410000, 0x41ffff, MRA_TEXTRAM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x440000, 0x44ffff, MRA_SPRITERAM },
	{ 0x840000, 0x84ffff, MRA_PALETTERAM },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x02ffff, MRA_ROM },
	{-1}
};

static struct MemoryWriteAddress quartet2_writemem[] =
{
	{ 0x000000, 0x02ffff, MWA_ROM },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0x410000, 0x41ffff, MWA_TEXTRAM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void quartet2_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0xcd14] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0xcd18] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0f24] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0f26] );

	set_fg_page( READ_WORD( &sys16_textram[0xcd1c] ) );
	set_bg_page( READ_WORD( &sys16_textram[0xcd1e] ) );
//tile_bank:0x3f0003
//refresh:0xc40001
}

void quartet2_init_machine( void ){
	static int bank[16] = { 00,01,02,03,00,01,02,03,00,01,02,03,00,01,02,03};
	sys16_obj_bank = bank;

	//mirror( 0xc40001, 0xfe0007, 0xff );
	sys16_update_proc = quartet2_update_proc;
}

void quartet2_sprite_decode( void ){
	sys16_sprite_decode( 5,0x010000 );
}
/***************************************************************************/

INPUT_PORTS_START( quartet2_input_ports )
//dipswitch:0x01 0 ATTRACT_MODE_SOUND OFF ON _ _ _
//dipswitch:0x06 1 1_CREDIT POWER_1000 POWER_500 POWER_2000 POWER_9000 _
//dipswitch:0x18 3 DIFFICULTY MEDIUM EASY HARD HARDEST _
//dipswitch:0x20 5 1_CREDIT_DURING_GAME POWER CREDIT _ _ _
//dipswitch:0x40 6 FREE_PLAY OFF ON _ _ _
//dipswitch:0x80 7 GAME_MODE NORMAL TEST _ _ _
//labeljoy:_ JUMP SHOOT _ DOWN UP RIGHT LEFT
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START _ _
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( quartet2_machine_driver, \
	quartet2_readmem,quartet2_writemem,quartet2_init_machine, gfx8 )

struct GameDriver quartet2_driver =
{
	__FILE__,
	0,
	"quartet2",
	"Quartet II",
	"1986",
	"Sega",
	SYS16_CREDITS,
	GAME_NOT_WORKING,
	&quartet2_machine_driver,
	0,
	quartet2_rom,
	quartet2_sprite_decode, 0,
	0,
	0,
	quartet2_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( sdi_rom )
	ROM_REGION( 0x030000 ) /* 68000 code */
	ROM_LOAD_ODD ( "a1.rom", 0x000000, 0x8000, 0xa9f816ef )
	ROM_LOAD_EVEN( "a4.rom", 0x000000, 0x8000, 0xf2c41dd6 )
	ROM_LOAD_ODD ( "a2.rom", 0x010000, 0x8000, 0x369af326 )
	ROM_LOAD_EVEN( "a5.rom", 0x010000, 0x8000, 0x7952e27e )
	ROM_LOAD_ODD ( "a3.rom", 0x020000, 0x8000, 0x193e4231 )
	ROM_LOAD_EVEN( "a6.rom", 0x020000, 0x8000, 0x8ee2c287 )

	ROM_REGION( 0x30000 ) /* tiles */
	ROM_LOAD( "b9.rom",  0x00000, 0x10000, 0x182b6301 )
	ROM_LOAD( "b10.rom", 0x10000, 0x10000, 0x8f7129a2 )
	ROM_LOAD( "b11.rom", 0x20000, 0x10000, 0x4409411f )

	ROM_REGION( 0x060000*2 ) /* sprites */
	ROM_LOAD( "b1.rom", 0x000000, 0x010000, 0x30e2c50a )
	ROM_LOAD( "b5.rom", 0x010000, 0x010000, 0x794e3e8b )
	ROM_LOAD( "b2.rom", 0x020000, 0x010000, 0x6a8b3fd0 )
	ROM_LOAD( "b6.rom", 0x030000, 0x010000, 0x602da5d5 )
	ROM_LOAD( "b3.rom", 0x040000, 0x010000, 0xb9de3aeb )
	ROM_LOAD( "b7.rom", 0x050000, 0x010000, 0x0a73a057 )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "a7.rom", 0x0000, 0x8000, 0x793f9f7f )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress sdi_readmem[] =
{
	{ 0xc41004, 0xc41005, io_player1_r },
	{ 0xc41002, 0xc41003, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },

	{ 0x410000, 0x410fff, MRA_TEXTRAM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x440000, 0x440fff, MRA_SPRITERAM },
	{ 0x840000, 0x840fff, MRA_PALETTERAM },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x02ffff, MRA_ROM },
	{-1}
};

static struct MemoryWriteAddress sdi_writemem[] =
{
	{ 0x000000, 0x02ffff, MWA_ROM },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0x410000, 0x410fff, MWA_TEXTRAM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void sdi_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0e98] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0e9a] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0e90] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0e92] );

	set_fg_page( READ_WORD( &sys16_textram[0x0e80] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0e82] ) );
//tile_bank:0x3f0003
//refresh:0xc40001
}

void sdi_init_machine( void ){
	static int bank[16] = {00,00,00,00,00,00,00,0x06,00,00,00,0x04,00,0x02,00,00};
	sys16_obj_bank = bank;

	patch_code( 0x110c, 0x80 );
	patch_code( 0x102f2, 0x00 );
	patch_code( 0x102f3, 0x02 );

	//mirror( 0x123407, 0xfe0007, 0xff );
	sys16_update_proc = sdi_update_proc;
}

void sdi_sprite_decode( void ){
	sys16_sprite_decode( 3,0x020000 );
}
/***************************************************************************/

INPUT_PORTS_START( sdi_input_ports )
//labeljoy:DOWN_P1 UP_P1 RIGHT_P1 LEFT_P1 DOWN_P2 UP_P2 RIGHT_P2 LEFT_P2
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START 6 7
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( sdi_machine_driver, \
	sdi_readmem,sdi_writemem,sdi_init_machine, gfx1 )

struct GameDriver sdi_driver =
{
	__FILE__,
	0,
	"sdi",
	"SDI",
	"1987",
	"Sega",
	SYS16_CREDITS,
	GAME_NOT_WORKING,
	&sdi_machine_driver,
	0,
	sdi_rom,
	sdi_sprite_decode, 0,
	0,
	0,
	sdi_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( shinobi_rom )
	ROM_REGION( 0x040000 ) /* 68000 code */
	ROM_LOAD_ODD ( "shinobi.a1", 0x000000, 0x10000, 0x343f4c46 )
	ROM_LOAD_EVEN( "shinobi.a4", 0x000000, 0x10000, 0xb930399d )
	ROM_LOAD_ODD ( "shinobi.a2", 0x020000, 0x10000, 0x7961d07e )
	ROM_LOAD_EVEN( "shinobi.a5", 0x020000, 0x10000, 0x9d46e707 )

	ROM_REGION( 0x30000 ) /* tiles */
	ROM_LOAD( "shinobi.b9",  0x00000, 0x10000, 0x5f62e163 )
	ROM_LOAD( "shinobi.b10", 0x10000, 0x10000, 0x75f8fbc9 )
	ROM_LOAD( "shinobi.b11", 0x20000, 0x10000, 0x06508bb9 )

	ROM_REGION( 0x080000*2 ) /* sprites */
	ROM_LOAD( "shinobi.b1", 0x00000, 0x10000, 0x611f413a )
	ROM_LOAD( "shinobi.b5", 0x10000, 0x10000, 0x5eb00fc1 )
	ROM_LOAD( "shinobi.b2", 0x20000, 0x10000, 0x3c0797c0 )
	ROM_LOAD( "shinobi.b6", 0x30000, 0x10000, 0x25307ef8 )
	ROM_LOAD( "shinobi.b3", 0x40000, 0x10000, 0xc29ac34e )
	ROM_LOAD( "shinobi.b7", 0x50000, 0x10000, 0x04a437f8 )
	ROM_LOAD( "shinobi.b4", 0x60000, 0x10000, 0x41f41063 )
	ROM_LOAD( "shinobi.b8", 0x70000, 0x10000, 0xb6e1fd72 )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "shinobi.a7", 0x0000, 0x8000, 0x2457a7cf )
ROM_END

/***************************************************************************/

static int shinobi_skip(int offset)
{
	if (cpu_getpc()==0x32e0) {cpu_spinuntil_int(); return 1<<8;}

	return READ_WORD(&sys16_workingram[0xf01c]);
}

static struct MemoryReadAddress shinobi_readmem[] =
{
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },

	{ 0x410000, 0x410fff, MRA_TEXTRAM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x440000, 0x440fff, MRA_SPRITERAM },
	{ 0x840000, 0x840fff, MRA_PALETTERAM },

	{ 0xfff01c, 0xfff01d, shinobi_skip },

	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x03ffff, MRA_ROM },
	{-1}
};

static struct MemoryWriteAddress shinobi_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0x410000, 0x410fff, MWA_TEXTRAM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};

/***************************************************************************/

void shinobi_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0e98] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0e9a] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0e90] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0e92] );

	set_fg_page( READ_WORD( &sys16_textram[0x0e80] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0e82] ) );

	set_refresh( READ_WORD( &sys16_workingram[0xf018] )>>8 );
}

void shinobi_init_machine( void ){
	static int bank[16] = { 0,0,0,0,0,0,0,6,0,0,0,4,0,2,0,0 };
	sys16_obj_bank = bank;
	sys16_update_proc = shinobi_update_proc;

//	parse_gcs();
}

void shinobi_sprite_decode( void ){
	sys16_sprite_decode( 4,0x20000 );
}

/***************************************************************************/


//dipswitch:0x01 0 GAME_TYPE COCKTAIL_TABLE UPRIGHT _ _ _
//dipswitch:0x02 1 ADVERTISE_SOUND OFF ON _ _ _
//dipswitch:0x0C 2 NUMBER_OF_LIVES 3 2 5 FREE _
//dipswitch:0x30 4 DIFFICULTY NORMAL EASY HARD HARDEST _
//dipswitch:0x40 6 BULLET'S_SPEED SLOW FAST _ _ _
//dipswitch:0x80 7 LANGUAGE JAPANESE ENGLISH _ _ _
//labeljoy:NINJA ATTACK JUMP _ DOWN UP RIGHT LEFT
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START _ _

INPUT_PORTS_START( shinobi_input_ports )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( shinobi_machine_driver, \
	shinobi_readmem,shinobi_writemem,shinobi_init_machine, gfx1 )

struct GameDriver shinobi_driver =
{
	__FILE__,
	0,
	"shinobi",
	"Shinobi",
	"1987",
	"Sega",
	SYS16_CREDITS,
	0,
	&shinobi_machine_driver,
	0,
	shinobi_rom,
	shinobi_sprite_decode, 0,
	0,
	0,
	shinobi_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( tetrisbl_rom )
	ROM_REGION( 0x020000 ) /* 68000 code */
	ROM_LOAD_ODD ( "rom1.bin", 0x000000, 0x10000, 0x1e912131 )
	ROM_LOAD_EVEN( "rom2.bin", 0x000000, 0x10000, 0x4d165c38 )

	ROM_REGION( 0x30000 ) /* tiles */
	ROM_LOAD( "scr01.rom", 0x00000, 0x10000, 0x62640221 )
	ROM_LOAD( "scr02.rom", 0x10000, 0x10000, 0x9abd183b )
	ROM_LOAD( "scr03.rom", 0x20000, 0x10000, 0x2495fd4e )

	ROM_REGION( 0x020000*2 ) /* sprites */
	ROM_LOAD( "obj0-o.rom", 0x00000, 0x10000, 0x2fb38880 )
	ROM_LOAD( "obj0-e.rom", 0x10000, 0x10000, 0xd6a02cba )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "S-PROG.ROM", 0x0000, 0x8000, 0xbd9ba01b )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress tetrisbl_readmem[] =
{
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },

	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x410000, 0x41ffff, MRA_TEXTRAM },
	{ 0x440000, 0x440fff, MRA_SPRITERAM },
	{ 0x840000, 0x840fff, MRA_PALETTERAM },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x01ffff, MRA_ROM },
	{-1}
};

static struct MemoryWriteAddress tetrisbl_writemem[] =
{
	{ 0x000000, 0x01ffff, MWA_ROM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x410000, 0x41ffff, MWA_TEXTRAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xc42006, 0xc42007, sound_command_w },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void tetrisbl_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0e98] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0e9a] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0e90] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0e92] );

	set_fg_page( READ_WORD( &sys16_textram[0x8038] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x8028] ) );

//tile_bank:0x3f0003
//refresh:0x00f019
}

void tetrisbl_init_machine( void ){
	static int bank[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	sys16_obj_bank = bank;

	patch_code( 0xba6, 0x4e );
	patch_code( 0xba7, 0x71 );

	sys16_sprxoffset = -0x40;
	sys16_update_proc = tetrisbl_update_proc;
}

void tetrisbl_sprite_decode( void ){
	sys16_sprite_decode( 1,0x20000 );
}
/***************************************************************************/

INPUT_PORTS_START( tetrisbl_input_ports )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE /* unconfirmed */

	PORT_START	/* DSW1 */
		PORT_DIPNAME( 0x01, 0x01, "Cabinet?", IP_KEY_NONE )
		PORT_DIPSETTING(    0x01, "Upright")
		PORT_DIPSETTING(    0x00, "Cocktail")
		PORT_DIPNAME( 0x02, 0x02, "Demo Sounds", IP_KEY_NONE )
		PORT_DIPSETTING(    0x02, "Off" )
		PORT_DIPSETTING(    0x00, "On" )
		PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
		PORT_DIPSETTING(    0x0c, "3" )
		PORT_DIPSETTING(    0x08, "2" )
		PORT_DIPSETTING(    0x04, "5" )
		PORT_DIPSETTING(    0x00, "FREE" )
		PORT_DIPNAME( 0x30, 0x30, "Difficulty", IP_KEY_NONE )
		PORT_DIPSETTING(    0x30, "Normal" )
		PORT_DIPSETTING(    0x20, "Easy" )
		PORT_DIPSETTING(    0x10, "Hard" )
		PORT_DIPSETTING(    0x00, "Hardest" )
		PORT_DIPNAME( 0x40, 0x40, "Bullets Speed", IP_KEY_NONE )
		PORT_DIPSETTING(    0x40, "Slow" )
		PORT_DIPSETTING(    0x00, "Fast" )
		PORT_DIPNAME( 0x80, 0x80, "Language", IP_KEY_NONE )
		PORT_DIPSETTING(    0x80, "Japanese" )
	PORT_DIPSETTING(    0x00, "English" )
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( tetrisbl_machine_driver, \
	tetrisbl_readmem,tetrisbl_writemem,tetrisbl_init_machine, gfx1 )

struct GameDriver tetrisbl_driver =
{
	__FILE__,
	0,
	"tetrisbl",
	"Tetris (Sega bootleg)",
	"1987",
	"bootleg",
	SYS16_CREDITS,
	0,
	&tetrisbl_machine_driver,
	0,
	tetrisbl_rom,
	tetrisbl_sprite_decode, 0,
	0,
	0,
	tetrisbl_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( timscanr_rom )
	ROM_REGION( 0x030000 ) /* 68000 code */
	ROM_LOAD_ODD ( "ts10850.bin", 0x00000, 0x8000, 0xf1575732 )
	ROM_LOAD_EVEN( "ts10853.bin", 0x00000, 0x8000, 0x24d7c5fb )
	ROM_LOAD_ODD ( "ts10851.bin", 0x10000, 0x8000, 0xf5ce271b )
	ROM_LOAD_EVEN( "ts10854.bin", 0x10000, 0x8000, 0x82d0b237 )
	ROM_LOAD_ODD ( "ts10852.bin", 0x20000, 0x8000, 0x7cd1382b )
	ROM_LOAD_EVEN( "ts10855.bin", 0x20000, 0x8000, 0x63e95a53 )

	ROM_REGION( 0x18000 ) /* tiles */
	ROM_LOAD( "timscanr.b9",  0x00000, 0x8000, 0x07dccc37 )
	ROM_LOAD( "timscanr.b10", 0x08000, 0x8000, 0x84fb9a3a )
	ROM_LOAD( "timscanr.b11", 0x10000, 0x8000, 0xc8694bc0 )

	ROM_REGION( 0x40000*2 ) /* sprites */
	ROM_LOAD( "ts10548.bin", 0x00000, 0x8000, 0xaa150735 )
	ROM_LOAD( "ts10552.bin", 0x08000, 0x8000, 0x6fcbb9f7 )
	ROM_LOAD( "ts10549.bin", 0x10000, 0x8000, 0x2f59f067 )
	ROM_LOAD( "ts10553.bin", 0x18000, 0x8000, 0x8a220a9f )
	ROM_LOAD( "ts10550.bin", 0x20000, 0x8000, 0xf05069ff )
	ROM_LOAD( "ts10554.bin", 0x28000, 0x8000, 0xdc64f809 )
	ROM_LOAD( "ts10551.bin", 0x30000, 0x8000, 0x435d811f )
	ROM_LOAD( "ts10555.bin", 0x38000, 0x8000, 0x2143c471 )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "ts10562.bin", 0x0000, 0x8000, 0x3f5028bf )
/*	ROM_LOAD( "ts10563.bin", 0x8000, 0x8000, 0x9db7eddf ) banked sound ROM? */
ROM_END

/***************************************************************************/

static struct MemoryReadAddress timscanr_readmem[] =
{
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },

	{ 0x410000, 0x41ffff, MRA_TEXTRAM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x440000, 0x44ffff, MRA_SPRITERAM },
	{ 0x840000, 0x84ffff, MRA_PALETTERAM },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x02ffff, MRA_ROM },
	{-1}
};

static struct MemoryWriteAddress timscanr_writemem[] =
{
	{ 0x000000, 0x02ffff, MWA_ROM },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0x410000, 0x41ffff, MWA_TEXTRAM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void timscanr_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0e98] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0e9a] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0e90] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0e92] );

	set_fg_page( READ_WORD( &sys16_textram[0x0e80] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0e82] ) );

	//tile_bank:0x3f0003
	//refresh:0x00c011
}

void timscanr_init_machine( void ){
	static int bank[16] = {00,00,00,00,00,00,00,0x03,00,00,00,0x02,00,0x01,00,00};
	sys16_obj_bank = bank;
	sys16_update_proc = timscanr_update_proc;
}

void timscanr_sprite_decode( void ){
	sys16_sprite_decode( 4,0x10000 );
}
/***************************************************************************/

//labeljoy:_ LEFT_FLIPPER RIGHT_FLIPPER _ _ PUSH_DOWN PUSH_RIGHT PUSH_LEFT
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START _ _

INPUT_PORTS_START( timscanr_input_ports )
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( timscanr_machine_driver, \
	timscanr_readmem,timscanr_writemem,timscanr_init_machine, gfx1 )

struct GameDriver timscanr_driver =
{
	__FILE__,
	0,
	"timscanr",
	"Time Scanner",
	"1987",
	"Sega",
	SYS16_CREDITS,
	0,
	&timscanr_machine_driver,
	0,
	timscanr_rom,
	timscanr_sprite_decode, 0,
	0,
	0,
	timscanr_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( tturfbl_rom )
	ROM_REGION( 0x40000 ) /* 68000 code */
	ROM_LOAD_ODD ( "TT06C794.ROM", 0x00000, 0x10000, 0x90e6a95a )
	ROM_LOAD_EVEN( "TT042197.ROM", 0x00000, 0x10000, 0xdeee5af1 )
	ROM_LOAD_ODD ( "TT05EF8A.ROM", 0x20000, 0x10000, 0xf787a948 )
	ROM_LOAD_EVEN( "TT030BE3.ROM", 0x20000, 0x10000, 0x100264a2 )

	ROM_REGION( 0x30000 ) /* tiles */
	ROM_LOAD( "TT1574B3.ROM", 0x00000, 0x10000, 0xe9e630da )
	ROM_LOAD( "TT16CF44.ROM", 0x10000, 0x10000, 0x4c467735 )
	ROM_LOAD( "TT17D59E.ROM", 0x20000, 0x10000, 0x60c0f2fe )

	ROM_REGION( 0x80000*2 ) /* sprites */
	ROM_LOAD( "TT11081E.ROM", 0x00000, 0x10000, 0x7a169fb1 )
	ROM_LOAD( "TT07C5AA.ROM", 0x10000, 0x10000, 0xae0fa085 )
	ROM_LOAD( "TT128958.ROM", 0x20000, 0x10000, 0x961d06b7 )
	ROM_LOAD( "TT083ACC.ROM", 0x30000, 0x10000, 0xe8671ee1 )
	ROM_LOAD( "TT13E508.ROM", 0x40000, 0x10000, 0xf16b6ba2 )
	ROM_LOAD( "TT09AAE6.ROM", 0x50000, 0x10000, 0x1ef1077f )
	ROM_LOAD( "TT14489C.ROM", 0x60000, 0x10000, 0x838bd71f )
	ROM_LOAD( "TT107319.ROM", 0x70000, 0x10000, 0x639a57cb )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "TT014D68.ROM", 0x0000, 0x10000, 0xd4aab1d9 )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress tturfbl_readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x100000, 0x2fffff, MRA_EXTRAM },
	{ 0x300000, 0x30ffff, MRA_SPRITERAM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x410000, 0x41ffff, MRA_TEXTRAM },
	{ 0x500000, 0x50ffff, MRA_PALETTERAM },
	{ 0x601002, 0x601003, io_player1_r },
	{ 0x601006, 0x601007, io_player2_r },
	{ 0x601000, 0x601001, io_service_r },
	{ 0x602002, 0x602003, io_dip1_r },
	{ 0x602000, 0x602001, io_dip2_r },
	{ 0xc40000, 0xc4ffff, MRA_EXTRAM3 },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{-1}
};

static struct MemoryWriteAddress tturfbl_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x100000, 0x2fffff, MWA_EXTRAM },
	{ 0x300000, 0x30ffff, MWA_SPRITERAM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x410000, 0x41ffff, MWA_TEXTRAM },
	{ 0x500000, 0x50ffff, MWA_PALETTERAM },
	{ 0x600000, 0x600005, MWA_EXTRAM2 },
	{ 0x600006, 0x600007, sound_command_w },
	{ 0xc40000, 0xc4ffff, MWA_EXTRAM3 },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void tturfbl_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0e98] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0e9a] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0e90] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0e92] );

	set_fg_page( READ_WORD( &sys16_extraram3[0x6122] ) );
	set_bg_page( READ_WORD( &sys16_extraram3[0x6126] ) );
	set_refresh( READ_WORD( &sys16_extraram2[0] ) );
}

void tturfbl_init_machine( void ){
	static int bank[16] = {00,00,00,00,00,00,00,0x06,00,00,00,0x04,00,0x02,00,00};
	sys16_obj_bank = bank;

	//mirror( 0x410e82, 0xc46021, -1 );
	//mirror( 0x410e80, 0xc46023, -1 );
	//mirror( 0x410e83, 0xc46025, -1 );
	//mirror( 0x410e81, 0xc46027, -1 );
	//mirror( 0x600007, 0xfe0007, 0xff );
	sys16_update_proc = tturfbl_update_proc;
}

void tturfbl_sprite_decode (void)
{
	int i;

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x30000; i++)
		Machine->memory_region[1][i] ^= 0xff;

	sys16_sprite_decode( 4,0x20000 );
}
/***************************************************************************/

INPUT_PORTS_START( tturfbl_input_ports )
//dipswitch:0x01 0 ADVERTISE_SOUND OFF ON _ _ _
//dipswitch:0x06 1 GANE_DIFFICULTY NORMAL EASY HARD HARDEST _
//dipswitch:0x18 3 PLAYER_NUMBERS 3 2 4 5 _
//dipswitch:0x60 5 EXTEND_PLAYER 200000 150000 300000 400000 _
//labeljoy:JUMP PUNCH KICK _ DOWN UP RIGHT LEFT
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START _ _
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( tturfbl_machine_driver, \
	tturfbl_readmem,tturfbl_writemem,tturfbl_init_machine, gfx1 )

struct GameDriver tturfbl_driver =
{
	__FILE__,
	0,
	"tturfbl",
	"Tough Turf (bootleg)",
	"1989",
	"bootleg",
	SYS16_CREDITS,
	GAME_NOT_WORKING,
	&tturfbl_machine_driver,
	0,
	tturfbl_rom,
	tturfbl_sprite_decode, 0,
	0,
	0,
	tturfbl_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( wb3bl_rom )
	ROM_REGION( 0x040000 ) /* 68000 code */
	ROM_LOAD_ODD ( "wb3_05", 0x000000, 0x10000, 0x196e17ee )
	ROM_LOAD_EVEN( "wb3_03", 0x000000, 0x10000, 0x0019ab3b )
	ROM_LOAD_ODD ( "wb3_04", 0x020000, 0x10000, 0x565d5035 )
	ROM_LOAD_EVEN( "wb3_02", 0x020000, 0x10000, 0xc87350cb )

	ROM_REGION( 0x30000 ) /* tiles */
	ROM_LOAD( "wb3_14", 0x00000, 0x10000, 0xd3f20bca )
	ROM_LOAD( "wb3_15", 0x10000, 0x10000, 0x96ff9d52 )
	ROM_LOAD( "wb3_16", 0x20000, 0x10000, 0xafaf0d31 )

	ROM_REGION( 0x080000*2 ) /* sprites */
	ROM_LOAD( "wb3_12", 0x000000, 0x010000, 0x4891e7bb )
	ROM_LOAD( "wb3_06", 0x010000, 0x010000, 0xe645902c )
	ROM_LOAD( "wb3_13", 0x020000, 0x010000, 0x8409a243 )
	ROM_LOAD( "wb3_07", 0x030000, 0x010000, 0xe774ec2c )
	ROM_LOAD( "wb3_10", 0x040000, 0x010000, 0xaeeecfca )
	ROM_LOAD( "wb3_08", 0x050000, 0x010000, 0x615e4927 )
	ROM_LOAD( "wb3_11", 0x060000, 0x010000, 0x5c2f0d90 )
	ROM_LOAD( "wb3_09", 0x070000, 0x010000, 0x0cd59d6e )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "a07.bin", 0x0000, 0x8000, 0x0bb901bb )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress wb3bl_readmem[] =
{
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },

	{ 0x3f0000, 0x3fffff, MRA_EXTRAM },
	{ 0x400000, 0x40ffff, MRA_TILERAM },
	{ 0x410000, 0x41ffff, MRA_TEXTRAM },
	{ 0x440000, 0x44ffff, MRA_SPRITERAM },
	{ 0x840000, 0x84ffff, MRA_PALETTERAM },
	{ 0xdf0000, 0xdfffff, MRA_EXTRAM3 },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{ 0x000000, 0x03ffff, MRA_ROM },
	{-1}
};

static struct MemoryWriteAddress wb3bl_writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x3f0000, 0x3fffff, MWA_EXTRAM },
	{ 0x400000, 0x40ffff, MWA_TILERAM },
	{ 0x410000, 0x41ffff, MWA_TEXTRAM },
	{ 0x440000, 0x44ffff, MWA_SPRITERAM },
	{ 0x840000, 0x84ffff, MWA_PALETTERAM },
	{ 0xc40000, 0xc4ffff, MWA_EXTRAM2 },
	{ 0xdf0000, 0xdfffff, MWA_EXTRAM3 },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void wb3bl_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_workingram[0xc030] );
	sys16_bg_scrollx = READ_WORD( &sys16_workingram[0xc038] );
	sys16_fg_scrolly = READ_WORD( &sys16_workingram[0xc032] );
	sys16_bg_scrolly = READ_WORD( &sys16_workingram[0xc03c] );

	set_fg_page( READ_WORD( &sys16_textram[0x0ff6] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0ff4] ) );
	set_refresh( READ_WORD( &sys16_extraram2[0] ) );
}

void wb3bl_init_machine( void ){
	static int bank[16] = {4,0,2,0,6,0,0,0x06,0,0,0,0x04,0,0x02,0,0};
	sys16_obj_bank = bank;

	patch_code( 0x17058, 0x4e );
	patch_code( 0x17059, 0xb9 );
	patch_code( 0x1705a, 0x00 );
	patch_code( 0x1705b, 0x00 );
	patch_code( 0x1705c, 0x09 );
	patch_code( 0x1705d, 0xdc );
	patch_code( 0x1705e, 0x4e );
	patch_code( 0x1705f, 0xf9 );
	patch_code( 0x17060, 0x00 );
	patch_code( 0x17061, 0x01 );
	patch_code( 0x17062, 0x70 );
	patch_code( 0x17063, 0xe0 );
	patch_code( 0x1a3a, 0x31 );
	patch_code( 0x1a3b, 0x7c );
	patch_code( 0x1a3c, 0x80 );
	patch_code( 0x1a3d, 0x00 );
	patch_code( 0x23df8, 0x14 );
	patch_code( 0x23df9, 0x41 );
	patch_code( 0x23dfa, 0x10 );
	patch_code( 0x23dfd, 0x14 );
	patch_code( 0x23dff, 0x1c );

	sys16_update_proc = wb3bl_update_proc;
}

void wb3bl_sprite_decode (void)
{
	int i;

	/* invert the graphics bits on the tiles */
	for (i = 0; i < 0x30000; i++)
		Machine->memory_region[1][i] ^= 0xff;

	sys16_sprite_decode( 4,0x20000 );
}

/***************************************************************************/

INPUT_PORTS_START( wb3bl_input_ports )
//labeljoy:_ SHOT JUMP _ DOWN UP RIGHT LEFT
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START _ _
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( wb3bl_machine_driver, \
	wb3bl_readmem,wb3bl_writemem,wb3bl_init_machine, gfx1 )

struct GameDriver wb3bl_driver =
{
	__FILE__,
	0,
	"wb3bl",
	"Wonder Boy Monster Lair (bootleg)",
	"1988",
	"bootleg",
	SYS16_CREDITS,
	0,
	&wb3bl_machine_driver,
	0,
	wb3bl_rom,
	wb3bl_sprite_decode, 0,
	0,
	0,
	wb3bl_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

/***************************************************************************/

ROM_START( wrestwar_rom )
	ROM_REGION( 0x100000 ) /* 68000 code */
	ROM_LOAD_ODD ( "ww.a5", 0x00000, 0x20000, 0x6714600a )
	ROM_LOAD_EVEN( "ww.a7", 0x00000, 0x20000, 0xeeaba126 )
	ROM_LOAD_ODD ( "ww.a6", 0x40000, 0x20000, 0xddf075cb )
	ROM_LOAD_EVEN( "ww.a8", 0x40000, 0x20000, 0xb77ba665 )

	ROM_REGION( 0x60000 ) /* tiles */
	ROM_LOAD( "ww.a14", 0x00000, 0x20000, 0x6a821ab9 )
	ROM_LOAD( "ww.a15", 0x20000, 0x20000, 0x2b1a0751 )
	ROM_LOAD( "ww.a16", 0x40000, 0x20000, 0xf6e190fe )

	ROM_REGION( 0x180000*2 ) /* sprites */
	ROM_LOAD( "ww.b1",  0x000000, 0x20000, 0xffa7d368 )
	ROM_LOAD( "ww.b5",  0x020000, 0x20000, 0x8d7794c1 )
	ROM_LOAD( "ww.b2",  0x040000, 0x20000, 0x0ed343f2 )
	ROM_LOAD( "ww.b6",  0x060000, 0x20000, 0x99458d58 )
	ROM_LOAD( "ww.b3",  0x080000, 0x20000, 0x3087104d )
	ROM_LOAD( "ww.b7",  0x0a0000, 0x20000, 0xabcf9bed )
	ROM_LOAD( "ww.b4",  0x0c0000, 0x20000, 0x41b6068b )
	ROM_LOAD( "ww.b8",  0x0e0000, 0x20000, 0x97eac164 )
	ROM_LOAD( "ww.a1",  0x100000, 0x20000, 0x260311c5 )
	ROM_LOAD( "ww.b10", 0x120000, 0x20000, 0x35a4b1b1 )
	ROM_LOAD( "ww.a2",  0x140000, 0x10000, 0x12e38a5c )
	ROM_LOAD( "ww.b11", 0x150000, 0x10000, 0xfa06fd24 )

	ROM_REGION( 0x10000 ) /* sound CPU */
	ROM_LOAD( "ww.a10", 0x0000, 0x08000, 0xc3609607 )
ROM_END

/***************************************************************************/

static struct MemoryReadAddress wrestwar_readmem[] =
{

	{ 0x000000, 0x0fffff, MRA_ROM },
	{ 0x100000, 0x10ffff, MRA_TILERAM },
	{ 0x110000, 0x11ffff, MRA_TEXTRAM },
	{ 0x200000, 0x20ffff, MRA_SPRITERAM },
	{ 0x300000, 0x30ffff, MRA_PALETTERAM },
	{ 0x400000, 0x40ffff, MRA_EXTRAM },
	{ 0xc40000, 0xc40fff, MRA_EXTRAM2 },
	{ 0xc41002, 0xc41003, io_player1_r },
	{ 0xc41006, 0xc41007, io_player2_r },
	{ 0xc41000, 0xc41001, io_service_r },
	{ 0xc42002, 0xc42003, io_dip1_r },
	{ 0xc42000, 0xc42001, io_dip2_r },
	{ 0xffe082, 0xffe083, mirror1_r },
	{ 0xffe08e, 0xffe08f, mirror2_r },
	{ 0xff0000, 0xffffff, MRA_WORKINGRAM },
	{-1}
};

static struct MemoryWriteAddress wrestwar_writemem[] =
{
	{ 0x000000, 0x0fffff, MWA_ROM },
	{ 0x100000, 0x10ffff, MWA_TILERAM },
	{ 0x110000, 0x11ffff, MWA_TEXTRAM },
	{ 0x200000, 0x20ffff, MWA_SPRITERAM },
	{ 0x300000, 0x30ffff, MWA_PALETTERAM },
	{ 0x400000, 0x40ffff, MWA_EXTRAM },
	{ 0xc40000, 0xc40fff, MWA_EXTRAM2 },
	{ 0xfe0006, 0xfe0007, sound_command_w },
	{ 0xffe082, 0xffe083, mirror1_w },
	{ 0xffe08e, 0xffe08f, mirror2_w },
	{ 0xff0000, 0xffffff, MWA_WORKINGRAM },
	{-1}
};
/***************************************************************************/

void wrestwar_update_proc( void ){
	sys16_fg_scrollx = READ_WORD( &sys16_textram[0x0e98] );
	sys16_bg_scrollx = READ_WORD( &sys16_textram[0x0e9a] );
	sys16_fg_scrolly = READ_WORD( &sys16_textram[0x0e90] );
	sys16_bg_scrolly = READ_WORD( &sys16_textram[0x0e92] );

	set_fg_page( READ_WORD( &sys16_textram[0x0e80] ) );
	set_bg_page( READ_WORD( &sys16_textram[0x0e82] ) );
	set_tile_bank( READ_WORD( &sys16_extraram[2] ) );
	set_refresh( READ_WORD( &sys16_extraram2[0] ) );
}

void wrestwar_init_machine( void ){
	static int bank[16] = {0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,0x10,0x12,0x14,0x16,0x18,0x1A,0x1C,0x1E};
	sys16_obj_bank = bank;

	copy_rom64k( 0x8, 0x4 );
	copy_rom64k( 0x9, 0x5 );
	copy_rom64k( 0xA, 0x6 );
	copy_rom64k( 0xB, 0x7 );
	copy_rom64k( 0x4, 0x0 );
	copy_rom64k( 0x5, 0x1 );
	copy_rom64k( 0x6, 0x2 );
	copy_rom64k( 0x7, 0x3 );

	define_mirror1( 0,0xc41001 );
	define_mirror2( 0xfe0007, 0 );

	//mirror( 0xc41001, 0xffe083, 0 );
	//mirror( 0xffe08f, 0xfe0007, 0xff );

	sys16_update_proc = wrestwar_update_proc;
}

void wrestwar_sprite_decode( void ){
	sys16_sprite_decode( 6,0x40000 );
}
/***************************************************************************/

INPUT_PORTS_START( wrestwar_input_ports )
//dipswitch:0x02 1 ADVERTISE_SOUND OFF ON _ _ _
//dipswitch:0x0C 2 ROUND_TIME 110 120 130 100 _
//dipswitch:0x20 5 CONTINUTATION CONTINUE NO_CONTINUE _ _ _
//dipswitch:0xC0 6 DIFFICULTY NORMAL EASY HARD HARDEST _
//labeljoy:_ PUNCH KICK _ DOWN UP RIGHT LEFT
//labelgen:COIN_1 COIN_2 TEST SERVICE 1P_START 2P_START _ _
	SYS16_JOY1
	SYS16_JOY2
	SYS16_SERVICE
	SYS16_COINAGE
	SYS16_OPTIONS
INPUT_PORTS_END

/***************************************************************************/

MACHINE_DRIVER( wrestwar_machine_driver, \
	wrestwar_readmem,wrestwar_writemem,wrestwar_init_machine, gfx2 )

struct GameDriver wrestwar_driver =
{
	__FILE__,
	0,
	"wrestwar",
	"Wrestle War",
	"1989",
	"Sega",
	SYS16_CREDITS,
	0,
	&wrestwar_machine_driver,
	0,
	wrestwar_rom,
	wrestwar_sprite_decode, 0,
	0,
	0,
	wrestwar_input_ports,
	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};
