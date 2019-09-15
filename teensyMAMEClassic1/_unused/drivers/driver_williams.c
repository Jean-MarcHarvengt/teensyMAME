/***************************************************************************

Changes:
	Mar 12 98 LBO
	* Added Sinistar speech samples provided by Howie Cohen
	* Fixed clipping circuit with help from Sean Riddle and Pat Lawrence

	Mar 22 98 ASG
	* Fixed Sinistar head drawing, did another major cleanup
	* Rewrote the blitter routines
	* Fixed interrupt timing finally!!!

Do to:  Mystic Marathon
Not sure of the board: Turkey shoot, Joust 2

Note: the visible area (according to the service mode) seems to be
	{ 6, 297-1, 7, 246-1 },
however I use
	{ 6, 298-1, 7, 247-1 },
because the DOS port doesn't support well screen widths which are not multiple of 4.

------- Blaster Bubbles Joust Robotron Sinistar Splat Stargate -------------

0000-8FFF ROM   (for Blaster, 0000-3FFF is a bank of 12 ROMs)
0000-97FF Video  RAM Bank switched with ROM (96FF for Blaster)
9800-BFFF RAM
	0xBB00 Blaster only, Color 0 for each line (256 entry)
	0xBC00 Blaster only, Color 0 flags, latch color only if bit 0 = 1 (256 entry)
	    Do something else with the bit 1, I do not know what
C000-CFFF I/O
D000-FFFF ROM

c000-C00F color_registers  (16 bytes of BBGGGRRR)

c804 widget_pia_dataa (widget = I/O board)
c805 widget_pia_ctrla
c806 widget_pia_datab
c807 widget_pia_ctrlb (CB2 select between player 1 and player 2
		       controls if Table or Joust)
      bits 5-3 = 110 = player 2
      bits 5-3 = 111 = player 1

c80c rom_pia_dataa
c80d rom_pia_ctrla
c80e rom_pia_datab
      bit 0 \
      bit 1 |
      bit 2 |-6 bits to sound board
      bit 3 |
      bit 4 |
      bit 5 /
      bit 6 \
      bit 7 /Plus CA2 and CB2 = 4 bits to drive the LED 7 segment
c80f rom_pia_ctrlb

C900 rom_enable_scr_ctrl  Switch between video ram and rom at 0000-97FF

C940 Blaster only: Select bank in the color Prom for color remap
C980 Blaster only: Select which ROM is at 0000-3FFF
C9C0 Blaster only: bit 0 = enable the color 0 changing each lines
		   bit 1 = erase back each frame


***** Blitter ****** (Stargate and Defender do not have blitter)
CA00 start_blitter    Each bits has a function
      1000 0000 Do not process half the byte 4-7
      0100 0000 Do not process half the byte 0-3
      0010 0000 Shift the shape one pixel right (to display a shape on an odd pixel)
      0001 0000 Remap, if shape != 0 then pixel = mask
      0000 1000 Source  1 = take source 0 = take Mask only
      0000 0100 ?
      0000 0010 Transparent
      0000 0001
CA01 blitter_mask     Not really a mask, more a remap color, see Blitter
CA02 blitter_source   hi
CA03 blitter_source   lo
CA04 blitter_dest     hi
CA05 blitter_dest     lo
CA06 blitter_w_h      H  Do a XOR with 4 to have the real value (Except Splat)
CA07 blitter_w_h      W  Do a XOR with 4 to have the real value (Except Splat)

CB00 6 bits of the video counters bits 2-7

CBFF watchdog

CC00-CFFF 1K X 4 CMOS ram battery backed up (8 bits on Sinistar)




------------------ Robotron -------------------
c804 widget_pia_dataa (widget = I/O board)
  bit 0  Move Up
  bit 1  Move Down
  bit 2  Move Left
  bit 3  Move Right
  bit 4  1 Player
  bit 5  2 Players
  bit 6  Fire Up
  bit 7  Fire Down

c806 widget_pia_datab
  bit 0  Fire Left
  bit 1  Fire Right
  bit 2
  bit 3
  bit 4
  bit 5
  bit 6
  bit 7

c80c rom_pia_dataa
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin
  bit 3  High Score Reset
  bit 4  Left Coin
  bit 5  Center Coin
  bit 6  Slam Door Tilt
  bit 7  Hand Shake from sound board


------------------ Joust -------------------
c804 widget_pia_dataa (widget = I/O board)
  bit 0  Move Left   player 1/2
  bit 1  Move Right  player 1/2
  bit 2  Flap        player 1/2
  bit 3
  bit 4  2 Player
  bit 5  1 Players
  bit 6
  bit 7

c806 widget_pia_datab
  bit 0
  bit 1
  bit 2
  bit 3
  bit 4
  bit 5
  bit 6
  bit 7

c80c rom_pia_dataa
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin
  bit 3  High Score Reset
  bit 4  Left Coin
  bit 5  Center Coin
  bit 6  Slam Door Tilt
  bit 7  Hand Shake from sound board


------------------ Stargate -------------------
c804 widget_pia_dataa (widget = I/O board)
  bit 0  Fire
  bit 1  Thrust
  bit 2  Smart Bomb
  bit 3  HyperSpace
  bit 4  2 Players
  bit 5  1 Player
  bit 6  Reverse
  bit 7  Down

c806 widget_pia_datab
  bit 0  Up
  bit 1  Inviso
  bit 2
  bit 3
  bit 4
  bit 5
  bit 6
  bit 7  0 = Upright  1 = Table

c80c rom_pia_dataa
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin        (High Score Reset in schematics)
  bit 3  High Score Reset  (Left Coin in schematics)
  bit 4  Left Coin         (Center Coin in schematics)
  bit 5  Center Coin       (Right Coin in schematics)
  bit 6  Slam Door Tilt
  bit 7  Hand Shake from sound board


------------------ Splat -------------------
c804 widget_pia_dataa (widget = I/O board)
  bit 0  Walk Up
  bit 1  Walk Down
  bit 2  Walk Left
  bit 3  Walk Right
  bit 4  1 Player
  bit 5  2 Players
  bit 6  Throw Up
  bit 7  Throw Down

c806 widget_pia_datab
  bit 0  Throw Left
  bit 1  Throw Right
  bit 2
  bit 3
  bit 4
  bit 5
  bit 6
  bit 7

c80c rom_pia_dataa
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin
  bit 3  High Score Reset
  bit 4  Left Coin
  bit 5  Center Coin
  bit 6  Slam Door Tilt
  bit 7  Hand Shake from sound board


------------------ Blaster -------------------
c804 widget_pia_dataa (widget = I/O board)
  bit 0  up/down switch a
  bit 1  up/down switch b
  bit 2  up/down switch c
  bit 3  up/down direction
  bit 4  left/right switch a
  bit 5  left/right switch b
  bit 6  left/right switch c
  bit 7  left/right direction

c806 widget_pia_datab
  bit 0  Thrust (Panel)
  bit 1  Blast
  bit 2  Thrust (Joystick)
  bit 3
  bit 4  1 Player
  bit 5  2 Player
  bit 6
  bit 7

c80c rom_pia_dataa
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin
  bit 3  High Score Reset
  bit 4  Left Coin
  bit 5  Center Coin
  bit 6  Slam Door Tilt
  bit 7  Hand Shake from sound board

------------------------- Sinistar -------------------------------------
c804 widget_pia_dataa (widget = I/O board)
  bit 0  up/down switch a
  bit 1  up/down switch b
  bit 2  up/down switch c
  bit 3  up/down direction
  bit 4  left/right switch a
  bit 5  left/right switch b
  bit 6  left/right switch c
  bit 7  left/right direction

c806 widget_pia_datab
  bit 0  Fire
  bit 1  Bomb
  bit 2
  bit 3
  bit 4  1 Player
  bit 5  2 Player
  bit 6
  bit 7

c80c rom_pia_dataa
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin
  bit 3  High Score Reset
  bit 4  Left Coin
  bit 5  Center Coin
  bit 6  Slam Door Tilt
  bit 7  Hand Shake from sound board


------------------ Bubbles -------------------
c804 widget_pia_dataa (widget = I/O board)
  bit 0  Up
  bit 1  Down
  bit 2  Left
  bit 3  Right
  bit 4  2 Players
  bit 5  1 Player
  bit 6
  bit 7

c806 widget_pia_datab
  bit 0
  bit 1
  bit 2
  bit 3
  bit 4
  bit 5
  bit 6
  bit 7

c80c rom_pia_dataa
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin        (High Score Reset in schematics)
  bit 3  High Score Reset  (Left Coin in schematics)
  bit 4  Left Coin         (Center Coin in schematics)
  bit 5  Center Coin       (Right Coin in schematics)
  bit 6  Slam Door Tilt
  bit 7  Hand Shake from sound board

------------------------- Defender -------------------------------------
0000-9800 Video RAM
C000-CFFF ROM (4 banks) + I/O
d000-ffff ROM

c000-c00f color_registers  (16 bytes of BBGGGRRR)

C3FC      WatchDog

C400-C4FF CMOS ram battery backed up

C800      6 bits of the video counters bits 2-7

cc00 pia1_dataa (widget = I/O board)
  bit 0  Auto Up
  bit 1  Advance
  bit 2  Right Coin
  bit 3  High Score Reset
  bit 4  Left Coin
  bit 5  Center Coin
  bit 6
  bit 7
cc01 pia1_ctrla

cc02 pia1_datab
  bit 0 \
  bit 1 |
  bit 2 |-6 bits to sound board
  bit 3 |
  bit 4 |
  bit 5 /
  bit 6 \
  bit 7 /Plus CA2 and CB2 = 4 bits to drive the LED 7 segment
cc03 pia1_ctrlb (CB2 select between player 1 and player 2 controls if Table)

cc04 pia2_dataa
  bit 0  Fire
  bit 1  Thrust
  bit 2  Smart Bomb
  bit 3  HyperSpace
  bit 4  2 Players
  bit 5  1 Player
  bit 6  Reverse
  bit 7  Down
cc05 pia2_ctrla

cc06 pia2_datab
  bit 0  Up
  bit 1
  bit 2
  bit 3
  bit 4
  bit 5
  bit 6
  bit 7
cc07 pia2_ctrlb
  Control the IRQ

d000 Select bank (c000-cfff)
  0 = I/O
  1 = BANK 1
  2 = BANK 2
  3 = BANK 3
  7 = BANK 4


***************************************************************************/

#include "driver.h"
#include "machine/6821pia.h"
#include "vidhrdw/generic.h"


/**** from machine/williams.h ****/
extern unsigned char *williams_bank_base;
extern unsigned char *williams_video_counter;

void robotron_init_machine(void);
void joust_init_machine(void);
void stargate_init_machine(void);
void bubbles_init_machine(void);
void sinistar_init_machine(void);
void defender_init_machine(void);
void splat_init_machine(void);
void blaster_init_machine(void);
void colony7_init_machine(void);
void lottofun_init_machine(void);

int williams_interrupt(void);
void williams_vram_select_w(int offset,int data);

extern unsigned char *robotron_catch;
int robotron_catch_loop_r(int offset);

extern unsigned char *stargate_catch;
int stargate_catch_loop_r(int offset);

extern unsigned char *mayday_catch;
int mayday_catch_loop_r(int offset);

extern unsigned char *defender_catch;
extern unsigned char *defender_bank_base;
int defender_catch_loop_r(int offset);
void defender_bank_select_w(int offset,int data);

void colony7_bank_select_w(int offset,int data);

extern void defcomnd_bank_select_w(int offset,int data);

extern unsigned char *blaster_catch;
extern unsigned char *blaster_bank2_base;
int blaster_catch_loop_r(int offset);
void blaster_bank_select_w(int offset,int data);
void blaster_vram_select_w(int offset,int data);

extern unsigned char *splat_catch;
int splat_catch_loop_r(int offset);


/**** from vidhrdw/williams.h ****/
extern unsigned char *williams_blitterram;
extern unsigned char *williams_remap_select;

void williams_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int williams_vh_start(void);
int williams_vh_start_sc2(void);
int blaster_vh_start(void);
void williams_vh_stop(void);
void williams_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void williams_videoram_w(int offset,int data);
void williams_blitter_w(int offset,int data);
void williams_remap_select_w(int offset, int data);

extern unsigned char *blaster_color_zero_table;
extern unsigned char *blaster_color_zero_flags;
extern unsigned char *blaster_video_bits;
void blaster_vh_convert_color_prom (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void blaster_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void blaster_video_bits_w(int offset, int data);

int sinistar_vh_start(void);

void defender_videoram_w(int offset,int data);


static void defndjeu_decode(void);



/***************************************************************************

  Memory handlers

***************************************************************************/

/*
 *   Read/Write mem for Robotron Joust Stargate Bubbles
 */

static struct MemoryReadAddress robotron_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x9810, 0x9810, robotron_catch_loop_r, &robotron_catch },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc807, pia_1_r },
	{ 0xc80c, 0xc80f, pia_2_r },
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },            /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress joust_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc807, pia_1_r },
	{ 0xc80c, 0xc80f, pia_2_r },
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },                 /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress stargate_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x9c39, 0x9c39, stargate_catch_loop_r, &stargate_catch },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc807, pia_1_r },
	{ 0xc80c, 0xc80f, pia_2_r },
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },            /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress bubbles_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc807, pia_1_r },
	{ 0xc80c, 0xc80f, pia_2_r },
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },            /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress williams_writemem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, paletteram_BBGGGRRR_w, &paletteram },
	{ 0xc804, 0xc807, pia_1_w },
	{ 0xc80c, 0xc80f, pia_2_w },
	{ 0xc900, 0xc900, williams_vram_select_w },           /* bank  */
	{ 0xca00, 0xca07, williams_blitter_w, &williams_blitterram }, /* blitter */
	{ 0xcbff, 0xcbff, watchdog_reset_w },                         /* WatchDog (have to be $39) */
	{ 0xcc00, 0xcfff, MWA_RAM },                                  /* CMOS                      */
	{ 0xd000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*
 *   Read/Write mem for Splat
 */

static struct MemoryReadAddress splat_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x984b, 0x984b, splat_catch_loop_r, &splat_catch },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc807, pia_1_r },
	{ 0xc80c, 0xc80f, pia_2_r },
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },                           /* CMOS */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress splat_writemem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, paletteram_BBGGGRRR_w, &paletteram },
	{ 0xc804, 0xc807, pia_1_w },
	{ 0xc80c, 0xc80f, pia_2_w },
	{ 0xc900, 0xc900, williams_vram_select_w },         /* bank  */
	{ 0xca00, 0xca07, williams_blitter_w, &williams_blitterram },  /* blitter */
	{ 0xcbff, 0xcbff, watchdog_reset_w },                         /* WatchDog (have to be $39) */
	{ 0xcc00, 0xcfff, MWA_RAM },                                /* CMOS         */
	{ 0xd000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*
 *   Read/Write mem for Sinistar
 */

static struct MemoryReadAddress sinistar_readmem[] =
{
	{ 0x0000, 0x97ff, MRA_BANK1, &williams_bank_base },
	{ 0x9800, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc807, pia_1_r },
	{ 0xc80c, 0xc80f, pia_2_r },
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },                 /* CMOS */
	{ 0xd000, 0xdfff, MRA_RAM },                 /* for Sinistar */
	{ 0xe000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sinistar_writemem[] =
{
	{ 0x0000, 0x97ff, williams_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, paletteram_BBGGGRRR_w, &paletteram },
	{ 0xc804, 0xc807, pia_1_w },
	{ 0xc80c, 0xc80f, pia_2_w },
	{ 0xc900, 0xc900, williams_vram_select_w },             /* bank  */
	{ 0xca00, 0xca07, williams_blitter_w, &williams_blitterram },   /* blitter */
	{ 0xcbff, 0xcbff, watchdog_reset_w },                         /* WatchDog (have to be $39) */
	{ 0xcc00, 0xcfff, MWA_RAM },                                    /* CMOS         */
	{ 0xd000, 0xdfff, MWA_RAM },                                    /* for Sinistar */
	{ 0xe000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*
 *   Read/Write mem for Blaster
 */

static struct MemoryReadAddress blaster_readmem[] =
{
	{ 0x0000, 0x3fff, MRA_BANK1, &williams_bank_base },
	{ 0x4000, 0x96ff, MRA_BANK2, &blaster_bank2_base },
/*      { 0x9700, 0x9700, blaster_catch_loop_r, &blaster_catch },*/
	{ 0x9700, 0xbfff, MRA_RAM },
	{ 0xc804, 0xc807, pia_1_r },
	{ 0xc80c, 0xc80f, pia_2_r },
	{ 0xcb00, 0xcb00, MRA_RAM, &williams_video_counter },
	{ 0xcc00, 0xcfff, MRA_RAM },                 /* CMOS      */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress blaster_writemem[] =
{
	{ 0x0000, 0x96ff, williams_videoram_w, &videoram, &videoram_size },
	{ 0x9700, 0xbaff, MWA_RAM },
	{ 0xbb00, 0xbbff, MWA_RAM, &blaster_color_zero_table },     /* Color 0 for each line */
	{ 0xbc00, 0xbcff, MWA_RAM, &blaster_color_zero_flags },     /* Color 0 flags, latch color only if bit 0 = 1 */
	{ 0xbd00, 0xbfff, MWA_RAM },
	{ 0xc000, 0xc00f, paletteram_BBGGGRRR_w, &paletteram },
	{ 0xc804, 0xc807, pia_1_w },
	{ 0xc80c, 0xc80f, pia_2_w },
	{ 0xc900, 0xc900, blaster_vram_select_w },          			 /* VRAM bank  */
	{ 0xc940, 0xc940, williams_remap_select_w, &williams_remap_select }, /* select remap colors in Remap prom */
	{ 0xc980, 0xc980, blaster_bank_select_w },                   /* Bank Select */
	{ 0xc9C0, 0xc9c0, blaster_video_bits_w, &blaster_video_bits },/* Video related bits */
	{ 0xca00, 0xca07, williams_blitter_w, &williams_blitterram }, /* blitter */
	{ 0xcbff, 0xcbff, watchdog_reset_w },                         /* WatchDog (have to be $39) */
	{ 0xcc00, 0xcfff, MWA_RAM },                                 /* CMOS         */
	{ 0xd000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/*
 *   Read/Write mem for Defender
 */

static struct MemoryReadAddress defender_readmem[] =
{
	{ 0xa05d, 0xa05d, defender_catch_loop_r, &defender_catch },
	{ 0x0000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xcfff, MRA_BANK1 },          /* i/o / rom */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress defender_writemem[] =
{
	{ 0x0000, 0x97ff, defender_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xcfff, MWA_BANK1, &defender_bank_base }, /* non map */
	{ 0xc000, 0xc00f, MWA_RAM, &paletteram },   /* here only to initialize the pointers */
	{ 0xd000, 0xd000, defender_bank_select_w },       /* Bank Select */
	{ 0xd001, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Read/Write mem for Defense Command
 */

static struct MemoryReadAddress defcomnd_readmem[] =
{
//	{ 0xa05d, 0xa05d, defender_catch_loop_r, &defender_catch },
	{ 0x0000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xcfff, MRA_BANK1 },          /* i/o / rom */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress defcomnd_writemem[] =
{
	{ 0x0000, 0x97ff, defender_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xcfff, MWA_BANK1, &defender_bank_base }, /* non map */
	{ 0xc000, 0xc00f, MWA_RAM, &paletteram },   /* here only to initialize the pointers */
    { 0xd000, 0xdfff, defcomnd_bank_select_w },
	{ 0xe000, 0xffff, MWA_ROM },

    { -1 }  /* end of table */
};

/*
 *   Read/Write mem for Mayday
 *
 * Don't know where busy loop is: E7EA or D665...
 */

static struct MemoryReadAddress mayday_readmem[] =
{
//    { 0xa03a, 0xa03a, mayday_catch_loop_r, &mayday_catch },
//    { 0xa05d, 0xa05d, mayday_catch_loop_r, &mayday_catch },
	{ 0x0000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xcfff, MRA_BANK1 },          /* i/o / rom */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress mayday_writemem[] =
{
	{ 0x0000, 0x97ff, defender_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xcfff, MWA_BANK1, &defender_bank_base }, /* non map */
	{ 0xc000, 0xc00f, MWA_RAM, &paletteram },   /* here only to initialize the pointers */
	{ 0xd000, 0xd000, defender_bank_select_w },       /* Bank Select */
	{ 0xd001, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   Read/Write mem for Colony7
 */

static struct MemoryReadAddress colony7_readmem[] =
{
	{ 0x0000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xcfff, MRA_BANK1 },		/* i/o / rom */
	{ 0xd000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress colony7_writemem[] =
{
	{ 0x0000, 0x97ff, defender_videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xcfff, MWA_BANK1, &defender_bank_base }, /* non map */
	{ 0xc000, 0xc00f, MWA_RAM, &paletteram },	/* here only to initialize the pointers */
	{ 0xd000, 0xd000, colony7_bank_select_w },       /* Bank Select */
	{ 0xd001, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   memory handlers for the audio CPU
 */

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x0400, 0x0403, pia_3_r },
	{ 0xf000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0x0400, 0x0403, pia_3_w },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


/*
 *   memory handlers for the Colony 7 audio CPU
 */

static struct MemoryReadAddress colony7_sound_readmem[] =
{
	{ 0x0000, 0x007f, MRA_RAM },
	{ 0x8400, 0x8403, pia_3_r },
	{ 0xf000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress colony7_sound_writemem[] =
{
	{ 0x0000, 0x007f, MWA_RAM },
	{ 0x8400, 0x8403, pia_3_w },
	{ 0xf000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};



/***************************************************************************

  I/O port definitions

***************************************************************************/

/*
 *   Robotron buttons
 */

INPUT_PORTS_START( robotron_input_ports )
	PORT_START      /* IN0 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP, "Move Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN, "Move Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT, "Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT, "Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP, "Fire Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN, "Fire Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT, "Fire Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT, "Fire Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )
INPUT_PORTS_END


/*
 *   Joust buttons
 */

INPUT_PORTS_START( joust_input_ports )
	PORT_START      /* IN0 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER2, "Player 2 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER2, "Player 2 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2, "Player 2 Flap", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN1 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )

	PORT_START      /* IN3 (muxed with IN0) */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_2WAY | IPF_PLAYER1, "Player 1 Move Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_2WAY | IPF_PLAYER1, "Player 1 Move Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1, "Player 1 Flap", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )
INPUT_PORTS_END


/*
 *   Stargate buttons
 */

INPUT_PORTS_START( stargate_input_ports )
	PORT_START      /* IN0 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "Fire", OSD_KEY_COLON, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "Thrust", OSD_KEY_L, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON3, "Smart Bomb", OSD_KEY_COMMA, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON6, "Hyperspace", OSD_KEY_N, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_BUTTON4, "Reverse", OSD_KEY_ALT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY, "Move Down", OSD_KEY_S, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY, "Move Up", OSD_KEY_Q, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON5, "Inviso", OSD_KEY_SPACE, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )

	PORT_START      /* IN3 - fake port for better joystick control */
	/* This fake port is handled via stargate_input_port_0 */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT )
INPUT_PORTS_END


/*
 *   Defender buttons
 */

INPUT_PORTS_START( defender_input_ports )
	PORT_START      /* IN0 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "Fire", OSD_KEY_COLON, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "Thrust", OSD_KEY_L, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON3, "Smart Bomb", OSD_KEY_COMMA, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_BUTTON4, "Hyperspace", OSD_KEY_N, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_BUTTON6, "Reverse", OSD_KEY_ALT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_8WAY, "Move Down", OSD_KEY_S, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_8WAY, "Move Up", OSD_KEY_Q, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )

	PORT_START      /* IN3 - fake port for better joystick control */
	/* This fake port is handled via defender_input_port_1 */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_CHEAT )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_CHEAT )
INPUT_PORTS_END


/*
 *   Sinistar buttons
 */

INPUT_PORTS_START( sinistar_input_ports )
	PORT_START      /* IN0 */
/* Not really the hardware bits, see sinistar_input_port_0() */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1, "Fire", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON2, "Bomb", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )
INPUT_PORTS_END


/*
 *   Bubbles buttons
 */

INPUT_PORTS_START( bubbles_input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )
INPUT_PORTS_END


/*
 *   Splat buttons
 */

INPUT_PORTS_START( splat_input_ports )
	PORT_START      /* IN0 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP | IPF_8WAY | IPF_PLAYER2, "Walk Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY | IPF_PLAYER2, "Walk Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY | IPF_PLAYER2, "Walk Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_PLAYER2, "Walk Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP | IPF_8WAY | IPF_PLAYER2, "Throw Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY | IPF_PLAYER2, "Throw Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY | IPF_PLAYER2, "Throw Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_PLAYER2, "Throw Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN4 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )

	PORT_START      /* IN3 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_UP | IPF_8WAY | IPF_PLAYER1, "Walk Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_DOWN | IPF_8WAY | IPF_PLAYER1, "Walk Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_LEFT | IPF_8WAY | IPF_PLAYER1, "Walk Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_JOYSTICKLEFT_RIGHT | IPF_8WAY | IPF_PLAYER1, "Walk Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_UP | IPF_8WAY | IPF_PLAYER1, "Throw Up", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_DOWN | IPF_8WAY | IPF_PLAYER1, "Throw Down", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)

	PORT_START      /* IN4 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_LEFT | IPF_8WAY | IPF_PLAYER1, "Throw Left", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_JOYSTICKRIGHT_RIGHT | IPF_8WAY | IPF_PLAYER1, "Throw Right", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0xfc, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


/*
 *   Blaster buttons
 */

INPUT_PORTS_START( blaster_input_ports )
	PORT_START      /* IN0 */
/* Not really the hardware bits */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_BUTTON2, "Thrust panel", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_BUTTON1, "Blast", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_BUTTON3, "Thrust joystick", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 0)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, 0, "Auto Up", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )
INPUT_PORTS_END


/*
 *   Colony7 buttons
 */

INPUT_PORTS_START( colony7_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0xfe, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END


/*
 *   Lotto Fun buttons
 */

INPUT_PORTS_START( lottofun_input_ports )
	PORT_START		/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Used by ticket dispenser */

	PORT_START		/* IN1 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* IN2 */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPF_TOGGLE, "Memory Protect", OSD_KEY_F1, IP_JOY_NONE, 0)
	PORT_BITX(0x02, IP_ACTIVE_HIGH, 0, "Advance", OSD_KEY_F2, IP_JOY_NONE, 0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BITX(0x08, IP_ACTIVE_HIGH, 0, "High Score Reset", OSD_KEY_7, IP_JOY_NONE, 0)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // COIN1.5? :)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // Sound board handshake
INPUT_PORTS_END


/***************************************************************************

  Machine drivers

***************************************************************************/

/*
 *   Sound interface
 */

static struct DACinterface dac_interface =
{
	1,
	{ 100 }
};


/*
 *   Robotron driver
 */

static struct MachineDriver robotron_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */
			0,                      /* memory region */
			robotron_readmem,       /* MemoryReadAddress */
			williams_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	robotron_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/*
 *   Joust driver
 */

static struct MachineDriver joust_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */
			0,                      /* memory region */
			joust_readmem,          /* MemoryReadAddress */
			williams_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	joust_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};



/*
 *   Stargate driver
 */

static struct MachineDriver stargate_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */
			0,                      /* memory region */
			stargate_readmem,       /* MemoryReadAddress */
			williams_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	stargate_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/*
 *   Bubbles driver
 */

static struct MachineDriver bubbles_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */
			0,                      /* memory region */
			bubbles_readmem,        /* MemoryReadAddress */
			williams_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	bubbles_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/*
 *   Sinistar driver
 */

static struct Samplesinterface sinistar_samples_interface =
{
	1	/* 1 channel */
};

static struct MachineDriver sinistar_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */ /*Sinistar do not like 1 mhz*/
			0,                      /* memory region */
			sinistar_readmem,       /* MemoryReadAddress */
			sinistar_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	sinistar_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	sinistar_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		},
		{
			SOUND_SAMPLES,
			&sinistar_samples_interface
		}
	}
};


/*
 *   Defender driver
 */

static struct MachineDriver defender_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1200000,                /* ? Mhz */ /*Defender do not like 1 mhz. Collect at least 9 humans, when you depose them, the game stuck */
			0,                      /* memory region */
			defender_readmem,       /* MemoryReadAddress */
			defender_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	defender_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/*
 *   Splat driver
 */

static struct MachineDriver splat_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */
			0,                      /* memory region */
			splat_readmem,          /* MemoryReadAddress */
			splat_writemem,         /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	splat_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start_sc2,                  /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/*
 *   Blaster driver
 */

static struct MachineDriver blaster_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* ? Mhz */
			0,                      /* memory region */
			blaster_readmem,        /* MemoryReadAddress */
			blaster_writemem,       /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	blaster_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16+256,	/* 16 colors for the real palette, 256 colors for the background */
	0,                                      /* color table length */
	blaster_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	blaster_vh_start,                  /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	blaster_vh_screenrefresh,               /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/*
 *   Colony 7 driver
 */

static struct MachineDriver colony7_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,                /* 1 Mhz */
			0,                      /* memory region */
			colony7_readmem,       /* MemoryReadAddress */
			colony7_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750,   /* 0.89475 Mhz (3.579 / 4) */
			2,        /* memory region #2 */
			colony7_sound_readmem,colony7_sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	colony7_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/*
 *	 Lotto Fun driver
 */

static struct MachineDriver lottofun_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,				/* ? Mhz */
			0,						/* memory region */
			bubbles_readmem,		/* MemoryReadAddress */
			williams_writemem,		/* MemoryWriteAddress */
			0,						/* IOReadPort */
			0,						/* IOWritePort */
			williams_interrupt, 	/* interrupt routine */
			64						/* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,		/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1		/* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	lottofun_init_machine,			/* init machine routine */

	/* video hardware */
	304, 256,								/* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,							/* GfxDecodeInfo * */
	16, 								 /* total colors */
	0,										/* color table length */
	williams_vh_convert_color_prom, 		/* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,										/* vh_init routine */
	williams_vh_start,						/* vh_start routine */
	williams_vh_stop,						/* vh_stop routine */
	williams_vh_screenrefresh,				/* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};


/*
 *   Defense Command driver
 */

static struct MachineDriver defcomnd_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1200000,                /* ? Mhz */ /*Defender do not like 1 mhz. Collect at least 9 humans, when you depose them, the game stuck */
			0,                      /* memory region */
			defcomnd_readmem,       /* MemoryReadAddress */
			defcomnd_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	defender_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

/*
 *   Mayday Driver
 */

static struct MachineDriver mayday_machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1200000,                /* ? Mhz */ /*Defender do not like 1 mhz. Collect at least 9 humans, when you depose them, the game stuck */
			0,                      /* memory region */
            mayday_readmem,       /* MemoryReadAddress */
            mayday_writemem,      /* MemoryWriteAddress */
			0,                      /* IOReadPort */
			0,                      /* IOWritePort */
			williams_interrupt,     /* interrupt routine */
			64                      /* interrupts per frame (64 times/frame for video counter) */
		},
		{
			CPU_M6808 | CPU_AUDIO_CPU,
			894750, /* 0.89475 Mhz (3.579 / 4) */
			2,      /* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1      /* interrupts are triggered by the main CPU */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	defender_init_machine,          /* init machine routine */

	/* video hardware */
	304, 256,                               /* screen_width, screen_height */
	{ 6, 298-1, 7, 247-1 },                 /* struct rectangle visible_area */
	0,                          /* GfxDecodeInfo * */
	16,                                  /* total colors */
	0,                                      /* color table length */
	williams_vh_convert_color_prom,         /* convert color prom routine */

	VIDEO_TYPE_RASTER|VIDEO_MODIFIES_PALETTE|VIDEO_SUPPORTS_DIRTY,
	0,                                      /* vh_init routine */
	williams_vh_start,                      /* vh_start routine */
	williams_vh_stop,                       /* vh_stop routine */
	williams_vh_screenrefresh,              /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_DAC,
			&dac_interface
		}
	}
};

/***************************************************************************

  High score/CMOS save/load

***************************************************************************/

static int cmos_load(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0xcc00],0x400);
		osd_fclose (f);
	}

	return 1;
}

static void cmos_save(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xcc00],0x400);
		osd_fclose (f);
	}
}


static int defender_cmos_load(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
	{
		osd_fread(f,&RAM[0xc400],0x100);
		osd_fclose (f);
	}

	return 1;
}

static void defender_cmos_save(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xc400],0x100);
		osd_fclose (f);
	}
}



/***************************************************************************

  Color PROM data

***************************************************************************/

static unsigned char blaster_remap_prom[] =
{
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x04,0x01,0x03,0x09,0x0A,0x01,0x0C,0x0D,0x0F,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x0C,0x0D,0x0A,0x09,0x0A,0x0C,0x0C,0x0D,0x0B,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x0C,0x0F,0x01,0x01,0x09,0x0A,0x07,0x0C,0x0D,0x06,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x01,0x09,0x0E,0x09,0x09,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x04,0x0C,0x0D,0x05,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x0C,0x09,0x0E,0x09,0x09,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x09,0x09,0x0E,0x01,0x0A,0x09,0x0C,0x0D,0x09,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x0C,0x0A,0x09,0x0C,0x0D,0x0B,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x05,0x0B,0x0E,0x0C,0x0B,0x06,0x0E,0x0D,0x07,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x05,0x06,0x0E,0x0C,0x06,0x01,0x0E,0x0D,0x03,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x05,0x01,0x0E,0x0C,0x01,0x0B,0x0E,0x0D,0x0D,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x05,0x0B,0x0E,0x0C,0x0B,0x0C,0x0E,0x0D,0x0D,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x04,0x0A,0x05,0x0C,0x0D,0x01,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x05,0x0C,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x05,0x01,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x06,0x03,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x0C,0x0B,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x0E,0x01,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x0E,0x0B,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x01,0x0B,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x0E,0x01,0x04,0x03,0x09,0x0B,0x0C,0x0C,0x0D,0x0B,0x0F,
     0x00,0x0F,0x02,0x0D,0x0C,0x0B,0x0A,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,
     0x00,0x03,0x02,0x07,0x09,0x0B,0x0D,0x0F,0x05,0x04,0x06,0x08,0x0A,0x0C,0x0E,0x01,
     0x00,0x03,0x02,0x0E,0x07,0x01,0x0F,0x09,0x0D,0x06,0x08,0x0A,0x0C,0x05,0x04,0x0B,
     0x00,0x0B,0x02,0x05,0x0C,0x0A,0x04,0x06,0x0D,0x09,0x0F,0x01,0x07,0x0E,0x08,0x03,
     0x00,0x06,0x02,0x0A,0x0C,0x05,0x04,0x0B,0x03,0x08,0x0E,0x07,0x01,0x0F,0x09,0x0D,
     0x00,0x07,0x02,0x0D,0x0F,0x0C,0x04,0x0B,0x05,0x0A,0x06,0x09,0x01,0x03,0x08,0x0E,
     0x00,0x09,0x02,0x05,0x03,0x07,0x01,0x0D,0x0A,0x04,0x08,0x0F,0x06,0x0C,0x0B,0x0E,
     0x00,0x01,0x02,0x01,0x01,0x01,0x01,0x01,0x05,0x09,0x0C,0x01,0x0C,0x01,0x01,0x05,
     0x00,0x05,0x02,0x01,0x01,0x01,0x01,0x01,0x01,0x09,0x0C,0x01,0x0C,0x01,0x01,0x05,
     0x00,0x05,0x02,0x05,0x01,0x01,0x01,0x01,0x01,0x09,0x0C,0x01,0x0C,0x01,0x01,0x01,
     0x00,0x01,0x02,0x05,0x05,0x01,0x01,0x01,0x01,0x09,0x0C,0x01,0x0C,0x01,0x01,0x01,
     0x00,0x01,0x02,0x01,0x05,0x05,0x01,0x01,0x01,0x09,0x0C,0x01,0x0C,0x01,0x01,0x01,
     0x00,0x01,0x05,0x01,0x01,0x05,0x05,0x01,0x01,0x09,0x05,0x05,0x05,0x05,0x05,0x01,
     0x00,0x01,0x05,0x01,0x01,0x01,0x05,0x05,0x01,0x09,0x05,0x05,0x05,0x05,0x05,0x01,
     0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x05,0x05,0x09,0x05,0x05,0x05,0x05,0x05,0x01,
     0x00,0x00,0x02,0x03,0x0D,0x05,0x00,0x07,0x08,0x0D,0x00,0x0B,0x0C,0x0D,0x0D,0x0F,
     0x00,0x00,0x02,0x03,0x0D,0x05,0x09,0x07,0x08,0x09,0x00,0x0B,0x0C,0x0D,0x0D,0x0F,
     0x00,0x00,0x02,0x03,0x0D,0x05,0x00,0x07,0x08,0x0D,0x09,0x0B,0x0C,0x0D,0x09,0x0F,
     0x00,0x09,0x02,0x03,0x09,0x05,0x00,0x07,0x08,0x0D,0x00,0x0B,0x0C,0x0D,0x0D,0x0F,
     0x00,0x00,0x02,0x03,0x0D,0x05,0x00,0x07,0x08,0x0D,0x00,0x09,0x0C,0x0D,0x0D,0x0F,
     0x00,0x00,0x02,0x03,0x0D,0x05,0x09,0x07,0x08,0x09,0x00,0x09,0x0C,0x0D,0x0D,0x0F,
     0x00,0x00,0x02,0x03,0x0D,0x05,0x00,0x07,0x08,0x0D,0x09,0x09,0x0C,0x0D,0x09,0x0F,
     0x00,0x09,0x02,0x03,0x09,0x05,0x00,0x07,0x08,0x0D,0x00,0x09,0x0C,0x0D,0x0D,0x0F,
     0x00,0x0D,0x02,0x03,0x01,0x04,0x05,0x04,0x01,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x03,
     0x00,0x03,0x02,0x0D,0x03,0x01,0x04,0x05,0x04,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x01,
     0x00,0x01,0x02,0x03,0x0D,0x03,0x01,0x04,0x05,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x04,
     0x00,0x04,0x02,0x01,0x03,0x0D,0x03,0x01,0x04,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x05,
     0x00,0x05,0x02,0x04,0x01,0x03,0x0D,0x03,0x01,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x04,
     0x00,0x04,0x02,0x05,0x04,0x01,0x03,0x0D,0x03,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x01,
     0x00,0x01,0x02,0x04,0x05,0x04,0x01,0x03,0x0D,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x03,
     0x00,0x03,0x02,0x01,0x04,0x05,0x04,0x01,0x03,0x09,0x0E,0x0B,0x0C,0x0A,0x09,0x0D,
     0x00,0x0B,0x02,0x03,0x04,0x01,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x09,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x0C,0x02,0x03,0x04,0x01,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x05,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x05,0x02,0x0A,0x05,0x05,0x08,0x08,0x08,0x09,0x0A,0x0B,0x06,0x07,0x0E,0x06,
     0x00,0x09,0x02,0x0B,0x09,0x05,0x02,0x02,0x02,0x09,0x0A,0x0B,0x09,0x0A,0x0E,0x0E,
     0x00,0x04,0x02,0x03,0x01,0x05,0x05,0x05,0x05,0x09,0x0A,0x0B,0x04,0x03,0x0E,0x0F,
     0x00,0x01,0x02,0x0D,0x01,0x05,0x0B,0x0B,0x0B,0x09,0x0A,0x0B,0x05,0x01,0x0E,0x06,
     0x00,0x01,0x02,0x03,0x01,0x05,0x09,0x09,0x09,0x09,0x0A,0x0B,0x0F,0x01,0x0E,0x09,
     0x00,0x01,0x02,0x0D,0x01,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x05,0x04,0x0E,0x0F,
     0x00,0x02,0x02,0x03,0x04,0x05,0x0B,0x02,0x0B,0x09,0x0A,0x0B,0x0C,0x0D,0x02,0x0B,
     0x00,0x02,0x02,0x03,0x04,0x05,0x0B,0x02,0x09,0x09,0x0A,0x0B,0x0C,0x0D,0x09,0x0B,
     0x00,0x02,0x02,0x03,0x04,0x05,0x0B,0x09,0x0B,0x09,0x0A,0x0B,0x0C,0x0D,0x02,0x09,
     0x00,0x09,0x02,0x03,0x04,0x05,0x09,0x02,0x0B,0x09,0x0A,0x0B,0x0C,0x0D,0x02,0x0B,
     0x00,0x0E,0x02,0x0A,0x0B,0x09,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x0B,0x02,0x0A,0x01,0x09,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0C,0x0E,0x0F,
     0x00,0x07,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x04,0x0E,0x0F,
     0x00,0x01,0x02,0x0B,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0C,
     0x00,0x05,0x02,0x07,0x03,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x08,0x0E,0x09,
     0x00,0x0F,0x02,0x01,0x0C,0x0B,0x06,0x07,0x08,0x0D,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x08,0x02,0x06,0x07,0x09,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0A,0x0E,0x05,
     0x00,0x0C,0x02,0x08,0x03,0x04,0x0E,0x0E,0x0E,0x06,0x08,0x0B,0x0C,0x08,0x07,0x0F,
     0x00,0x0B,0x02,0x0A,0x0D,0x0C,0x0E,0x0E,0x0A,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0C,
     0x00,0x06,0x02,0x0D,0x02,0x0A,0x06,0x07,0x08,0x0C,0x01,0x0B,0x0C,0x0D,0x0F,0x0F,
     0x00,0x05,0x02,0x03,0x01,0x04,0x05,0x04,0x01,0x0F,0x03,0x0B,0x0C,0x0D,0x0A,0x0F,
     0x00,0x01,0x02,0x02,0x08,0x06,0x05,0x04,0x01,0x05,0x07,0x0B,0x0C,0x0D,0x0A,0x06,
     0x00,0x01,0x02,0x03,0x04,0x05,0x05,0x01,0x0E,0x09,0x01,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x05,0x06,0x0E,0x09,0x06,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x01,0x0E,0x09,0x01,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x0B,0x0E,0x09,0x0B,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x0E,0x0F,0x0C,0x0D,0x05,0x05,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x09,0x05,0x04,0x03,0x0C,0x0C,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x0F,0x04,0x01,0x03,0x06,0x06,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x09,0x0E,0x0A,0x0D,0x01,0x01,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x0A,0x02,0x0D,0x0C,0x0E,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x07,0x02,0x08,0x06,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x0C,0x02,0x03,0x04,0x06,0x0E,0x07,0x08,0x05,0x0A,0x0B,0x01,0x03,0x06,0x04,
     0x00,0x0C,0x02,0x03,0x04,0x06,0x02,0x07,0x08,0x05,0x0A,0x0B,0x01,0x03,0x06,0x04,
     0x00,0x05,0x02,0x0A,0x05,0x05,0x01,0x01,0x01,0x09,0x0A,0x0B,0x06,0x07,0x0E,0x06,
     0x00,0x05,0x02,0x0A,0x05,0x05,0x05,0x05,0x05,0x09,0x0A,0x0B,0x06,0x07,0x0E,0x06,
     0x00,0x01,0x02,0x03,0x04,0x05,0x02,0x02,0x02,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x09,0x02,0x0B,0x09,0x05,0x01,0x01,0x01,0x09,0x0A,0x0B,0x09,0x0A,0x0E,0x0E,
     0x00,0x04,0x02,0x03,0x01,0x05,0x01,0x01,0x01,0x09,0x0A,0x0B,0x04,0x03,0x0E,0x0F,
     0x00,0x02,0x02,0x03,0x04,0x05,0x01,0x02,0x01,0x02,0x0E,0x01,0x0C,0x01,0x02,0x01,
     0x00,0x02,0x02,0x03,0x04,0x05,0x01,0x02,0x05,0x02,0x0E,0x01,0x0C,0x01,0x05,0x01,
     0x00,0x02,0x02,0x03,0x04,0x05,0x01,0x05,0x01,0x02,0x0E,0x01,0x0C,0x01,0x02,0x05,
     0x00,0x05,0x02,0x03,0x04,0x05,0x05,0x02,0x01,0x02,0x0E,0x01,0x0C,0x01,0x02,0x01,
     0x00,0x02,0x02,0x0A,0x09,0x0E,0x0C,0x02,0x0C,0x02,0x0A,0x0C,0x0C,0x0A,0x02,0x0C,
     0x00,0x02,0x02,0x0A,0x09,0x0E,0x0C,0x02,0x0F,0x02,0x0A,0x0C,0x0C,0x0A,0x0F,0x0C,
     0x00,0x02,0x02,0x0A,0x09,0x0E,0x0C,0x09,0x0C,0x02,0x0A,0x0C,0x0C,0x0A,0x02,0x0F,
     0x00,0x09,0x02,0x0A,0x09,0x0E,0x0F,0x02,0x0C,0x02,0x0A,0x0C,0x0C,0x0A,0x02,0x0C,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x0B,0x0A,0x09,0x0C,0x0D,0x0B,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x03,0x04,0x05,0x0C,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x0B,0x09,0x0E,0x09,0x09,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x0C,0x09,0x0E,0x09,0x09,0x0B,0x0E,0x0D,0x0E,0x0F,
     0x00,0x01,0x02,0x03,0x04,0x05,0x01,0x04,0x03,0x09,0x0A,0x0B,0x04,0x03,0x0E,0x05,
     0x00,0x01,0x02,0x03,0x04,0x0E,0x06,0x07,0x08,0x09,0x0A,0x0B,0x04,0x03,0x0E,0x05,
     0x00,0x01,0x02,0x03,0x04,0x0E,0x05,0x04,0x03,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0A,
     0x00,0x01,0x02,0x03,0x04,0x06,0x05,0x04,0x03,0x0E,0x0A,0x0B,0x01,0x0D,0x0E,0x04,
     0x00,0x01,0x02,0x03,0x04,0x0C,0x01,0x01,0x03,0x09,0x0E,0x0B,0x07,0x08,0x0E,0x06,
     0x00,0x01,0x02,0x03,0x04,0x05,0x09,0x09,0x0A,0x09,0x0A,0x0B,0x01,0x0D,0x0E,0x0C,
     0x00,0x01,0x02,0x03,0x04,0x09,0x05,0x05,0x03,0x09,0x0A,0x0B,0x04,0x04,0x0E,0x03,
     0x00,0x01,0x02,0x03,0x04,0x01,0x0C,0x0C,0x0D,0x01,0x0D,0x0B,0x0A,0x0D,0x0E,0x09,
     0x00,0x01,0x02,0x03,0x04,0x0E,0x06,0x07,0x08,0x0E,0x0A,0x0B,0x04,0x04,0x0E,0x03,
     0x00,0x01,0x02,0x03,0x04,0x0F,0x04,0x01,0x03,0x0F,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

/*
 *   Robotron
 */

ROM_START( robotron_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "robotron.sb1", 0x0000, 0x1000, 0x66c7d3ef )
	ROM_LOAD( "robotron.sb2", 0x1000, 0x1000, 0x5bc6c614 )
	ROM_LOAD( "robotron.sb3", 0x2000, 0x1000, 0xe99a82be )
	ROM_LOAD( "robotron.sb4", 0x3000, 0x1000, 0xafb1c561 )
	ROM_LOAD( "robotron.sb5", 0x4000, 0x1000, 0x62691e77 )
	ROM_LOAD( "robotron.sb6", 0x5000, 0x1000, 0xbd2c853d )
	ROM_LOAD( "robotron.sb7", 0x6000, 0x1000, 0x49ac400c )
	ROM_LOAD( "robotron.sb8", 0x7000, 0x1000, 0x3a96e88c )
	ROM_LOAD( "robotron.sb9", 0x8000, 0x1000, 0xb124367b )
	ROM_LOAD( "robotron.sba", 0xd000, 0x1000, 0x13797024 )
	ROM_LOAD( "robotron.sbb", 0xe000, 0x1000, 0x7e3c1b87 )
	ROM_LOAD( "robotron.sbc", 0xf000, 0x1000, 0x645d543e )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "robotron.snd", 0xf000, 0x1000, 0xc56c1d28 )
ROM_END

ROM_START( robotryo_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "robotron.yo1", 0x0000, 0x1000, 0x66c7d3ef )
	ROM_LOAD( "robotron.yo2", 0x1000, 0x1000, 0x5bc6c614 )
	ROM_LOAD( "robotron.yo3", 0x2000, 0x1000, 0x67a369bc )
	ROM_LOAD( "robotron.yo4", 0x3000, 0x1000, 0xb0de677a )
	ROM_LOAD( "robotron.yo5", 0x4000, 0x1000, 0x24726007 )
	ROM_LOAD( "robotron.yo6", 0x5000, 0x1000, 0x028181a6 )
	ROM_LOAD( "robotron.yo7", 0x6000, 0x1000, 0x4dfcceae )
	ROM_LOAD( "robotron.yo8", 0x7000, 0x1000, 0x3a96e88c )
	ROM_LOAD( "robotron.yo9", 0x8000, 0x1000, 0xb124367b )
	ROM_LOAD( "robotron.yoa", 0xd000, 0x1000, 0x4a9d5f52 )
	ROM_LOAD( "robotron.yob", 0xe000, 0x1000, 0x2afc5e7f )
	ROM_LOAD( "robotron.yoc", 0xf000, 0x1000, 0x45da9202 )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "robotron.snd", 0xf000, 0x1000, 0xc56c1d28 )
ROM_END


struct GameDriver robotron_driver =
{
	__FILE__,
	0,
	"robotron",
	"Robotron (Solid Blue label)",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&robotron_machine_driver,       /* MachineDriver * */
	0,

	robotron_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	robotron_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};


struct GameDriver robotryo_driver =
{
	__FILE__,
	&robotron_driver,
	"robotryo",
	"Robotron (Yellow/Orange label)",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nValerio Verrando",
	0,
	&robotron_machine_driver,       /* MachineDriver * */
	0,

	robotryo_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	robotron_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};



/*
 *   Joust
 */

ROM_START( joust_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "joust.wg1",    0x0000, 0x1000, 0xfe41b2af )
	ROM_LOAD( "joust.wg2",    0x1000, 0x1000, 0x501c143c )
	ROM_LOAD( "joust.wg3",    0x2000, 0x1000, 0x43f7161d )
	ROM_LOAD( "joust.wg4",    0x3000, 0x1000, 0xdb5571b6 )
	ROM_LOAD( "joust.wg5",    0x4000, 0x1000, 0xc686bb6b )
	ROM_LOAD( "joust.wg6",    0x5000, 0x1000, 0xfac5f2cf )
	ROM_LOAD( "joust.wg7",    0x6000, 0x1000, 0x81418240 )
	ROM_LOAD( "joust.wg8",    0x7000, 0x1000, 0xba5359ba )
	ROM_LOAD( "joust.wg9",    0x8000, 0x1000, 0x39643147 )
	ROM_LOAD( "joust.wga",    0xd000, 0x1000, 0x3f1c4f89 )
	ROM_LOAD( "joust.wgb",    0xe000, 0x1000, 0xea48b359 )
	ROM_LOAD( "joust.wgc",    0xf000, 0x1000, 0xc710717b )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "joust.snd",    0xf000, 0x1000, 0xf1835bdd )
ROM_END

ROM_START( joustr_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "joust.sr1",    0x0000, 0x1000, 0xfe41b2af )
	ROM_LOAD( "joust.sr2",    0x1000, 0x1000, 0x501c143c )
	ROM_LOAD( "joust.sr3",    0x2000, 0x1000, 0x43f7161d )
	ROM_LOAD( "joust.sr4",    0x3000, 0x1000, 0xab347170 )
	ROM_LOAD( "joust.sr5",    0x4000, 0x1000, 0xc686bb6b )
	ROM_LOAD( "joust.sr6",    0x5000, 0x1000, 0x3d9a6fac )
	ROM_LOAD( "joust.sr7",    0x6000, 0x1000, 0x0a70b3d1 )
	ROM_LOAD( "joust.sr8",    0x7000, 0x1000, 0xa7f01504 )
	ROM_LOAD( "joust.sr9",    0x8000, 0x1000, 0x978687ad )
	ROM_LOAD( "joust.sra",    0xd000, 0x1000, 0xc0c6e52a )
	ROM_LOAD( "joust.srb",    0xe000, 0x1000, 0xab11bcf9 )
	ROM_LOAD( "joust.src",    0xf000, 0x1000, 0xea14574b )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "joust.snd",    0xf000, 0x1000, 0xf1835bdd )
ROM_END

ROM_START( joustwr_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "joust.wr1",    0x0000, 0x1000, 0xfe41b2af )
	ROM_LOAD( "joust.wr2",    0x1000, 0x1000, 0x501c143c )
	ROM_LOAD( "joust.wr3",    0x2000, 0x1000, 0x43f7161d )
	ROM_LOAD( "joust.wr4",    0x3000, 0x1000, 0xdb5571b6 )
	ROM_LOAD( "joust.wr5",    0x4000, 0x1000, 0xc686bb6b )
	ROM_LOAD( "joust.wr6",    0x5000, 0x1000, 0xfac5f2cf )
	ROM_LOAD( "joust.wr7",    0x6000, 0x1000, 0xe6f439c4 )
	ROM_LOAD( "joust.wr8",    0x7000, 0x1000, 0xba5359ba )
	ROM_LOAD( "joust.wr9",    0x8000, 0x1000, 0x39643147 )
	ROM_LOAD( "joust.wra",    0xd000, 0x1000, 0x2039014a )
	ROM_LOAD( "joust.wrb",    0xe000, 0x1000, 0xea48b359 )
	ROM_LOAD( "joust.wrc",    0xf000, 0x1000, 0xc710717b )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "joust.snd",    0xf000, 0x1000, 0xf1835bdd )
ROM_END


struct GameDriver joust_driver =
{
	__FILE__,
	0,
	"joust",
	"Joust (White/Green label)",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nValerio Verrando",
	0,
	&joust_machine_driver,          /* MachineDriver * */
	0,

	joust_rom,                      /* White/Green version, latest */
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	joust_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};

struct GameDriver joustr_driver =
{
	__FILE__,
	&joust_driver,
	"joustr",
	"Joust (Solid Red label)",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&joust_machine_driver,          /* MachineDriver * */
	0,

	joustr_rom,                     /* Solid Red version, has pterodactyl bug */
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	joust_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};

struct GameDriver joustwr_driver =
{
	__FILE__,
	&joust_driver,
	"joustwr",
	"Joust (White/Red label)",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&joust_machine_driver,          /* MachineDriver * */
	0,

	joustwr_rom,
	0, 0,                           /* ROM decrypt routines */
	0,          /* samplenames */
	0,      /* sound_prom */

	joust_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};




/*
 *   Sinistar
 */

ROM_START( sinistar_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "sinistar.01",  0x0000, 0x1000, 0xf6f3a22c )
	ROM_LOAD( "sinistar.02",  0x1000, 0x1000, 0xcab3185c )
	ROM_LOAD( "sinistar.03",  0x2000, 0x1000, 0x1ce1b3cc )
	ROM_LOAD( "sinistar.04",  0x3000, 0x1000, 0x6da632ba )
	ROM_LOAD( "sinistar.05",  0x4000, 0x1000, 0xb662e8fc )
	ROM_LOAD( "sinistar.06",  0x5000, 0x1000, 0x2306183d )
	ROM_LOAD( "sinistar.07",  0x6000, 0x1000, 0xe5dd918e )
	ROM_LOAD( "sinistar.08",  0x7000, 0x1000, 0x4785a787 )
	ROM_LOAD( "sinistar.09",  0x8000, 0x1000, 0x50cb63ad )
	ROM_LOAD( "sinistar.10",  0xe000, 0x1000, 0x3d670417 )
	ROM_LOAD( "sinistar.11",  0xf000, 0x1000, 0x3162bc50 )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "sinistar.snd", 0xf000, 0x1000, 0xb82f4ddb )
ROM_END

ROM_START( sinista1_rom )
	ROM_REGION(0x10000) /* 64k for code */
	ROM_LOAD( "sinrev1.01",   0x0000, 0x1000, 0x3810d7b8 )
	ROM_LOAD( "sinistar.02",  0x1000, 0x1000, 0xcab3185c )
	ROM_LOAD( "sinrev1.03",   0x2000, 0x1000, 0x7c984ca9 )
	ROM_LOAD( "sinrev1.04",   0x3000, 0x1000, 0xcc6c4f24 )
	ROM_LOAD( "sinrev1.05",   0x4000, 0x1000, 0x12285bfe )
	ROM_LOAD( "sinrev1.06",   0x5000, 0x1000, 0x7a675f35 )
	ROM_LOAD( "sinrev1.07",   0x6000, 0x1000, 0xb0463243 )
	ROM_LOAD( "sinrev1.08",   0x7000, 0x1000, 0x909040d4 )
	ROM_LOAD( "sinrev1.09",   0x8000, 0x1000, 0xcc949810 )
	ROM_LOAD( "sinrev1.10",   0xe000, 0x1000, 0xea87a53f )
	ROM_LOAD( "sinrev1.11",   0xf000, 0x1000, 0x88d36e80 )

	ROM_REGION_DISPOSE(0x1000) /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000) /* 64k for the sound CPU */
	ROM_LOAD( "sinistar.snd", 0xf000, 0x1000, 0xb82f4ddb )
ROM_END

ROM_START( sinista2_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "sinistar.01",  0x0000, 0x1000, 0xf6f3a22c )
	ROM_LOAD( "sinistar.02",  0x1000, 0x1000, 0xcab3185c )
	ROM_LOAD( "sinistar.03",  0x2000, 0x1000, 0x1ce1b3cc )
	ROM_LOAD( "sinistar.04",  0x3000, 0x1000, 0x6da632ba )
	ROM_LOAD( "sinistar.05",  0x4000, 0x1000, 0xb662e8fc )
	ROM_LOAD( "sinistar.06",  0x5000, 0x1000, 0x2306183d )
	ROM_LOAD( "sinistar.07",  0x6000, 0x1000, 0xe5dd918e )
	ROM_LOAD( "sinrev2.08",   0x7000, 0x1000, 0xd7ecee45 )
	ROM_LOAD( "sinistar.09",  0x8000, 0x1000, 0x50cb63ad )
	ROM_LOAD( "sinistar.10",  0xe000, 0x1000, 0x3d670417 )
	ROM_LOAD( "sinrev2.11",   0xf000, 0x1000, 0x792c8b00 )

	ROM_REGION_DISPOSE(0x1000) /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000) /* 64k for the sound CPU */
	ROM_LOAD( "sinistar.snd", 0xf000, 0x1000, 0xb82f4ddb )
ROM_END


/* Sinistar speech samples */
const char *sinistar_sample_names[]=
{
	"*sinistar",
	"ilive.sam",
	"ihunger.sam",
	"roar.sam",
	"runrun.sam",
	"iamsin.sam",
	"bewarcow.sam",
	"ihungerc.sam",
	"runcow.sam",
	0 /*array end*/
};

struct GameDriver sinistar_driver =
{
	__FILE__,
	0,
	"sinistar",
	"Sinistar (revision 3)",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nHowie Cohen\nSean Riddle\nPat Lawrence",
	0,
	&sinistar_machine_driver,       /* MachineDriver * */
	0,

	sinistar_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	sinistar_sample_names,          /* samplenames */
	0,      /* sound_prom */

	sinistar_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	cmos_load, cmos_save
};

struct GameDriver sinista1_driver =
{
	__FILE__,
	&sinistar_driver,
	"sinista1",
	"Sinistar (prototype version)",
	"1982",
	"Williams",
	"\nSinistar team:\nMarc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nHowie Cohen\nSean Riddle\nPat Lawrence\nBrian Deuel (prototype driver)\n\nSpecial thanks to Peter Freeman",
	0,
	&sinistar_machine_driver, /* MachineDriver * */
	0,

	sinista1_rom, /* RomModule * */
	0, 0, /* ROM decrypt routines; not sure if these are needed here */
	sinistar_sample_names, /* samplenames */
	0, /* sound_prom */

	sinistar_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	cmos_load, cmos_save
};

struct GameDriver sinista2_driver =
{
	__FILE__,
	&sinistar_driver,
	"sinista2",
	"Sinistar (revision 2)",
	"1982",
	"Williams",
	"\nSinistar team:\nMarc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nHowie Cohen\nSean Riddle\nPat Lawrence\nBrian Deuel (prototype driver)\n\nSpecial thanks to Peter Freeman",
	0,
	&sinistar_machine_driver, /* MachineDriver * */
	0,

	sinista2_rom, /* RomModule * */
	0, 0, /* ROM decrypt routines; not sure if these are needed here */
	sinistar_sample_names, /* samplenames */
	0, /* sound_prom */

	sinistar_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	cmos_load, cmos_save
};



/*
 *   Bubbles
 */

ROM_START( bubbles_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "bubbles.1b",   0x0000, 0x1000, 0x8234f55c )
	ROM_LOAD( "bubbles.2b",   0x1000, 0x1000, 0x4a188d6a )
	ROM_LOAD( "bubbles.3b",   0x2000, 0x1000, 0x7728f07f )
	ROM_LOAD( "bubbles.4b",   0x3000, 0x1000, 0x040be7f9 )
	ROM_LOAD( "bubbles.5b",   0x4000, 0x1000, 0x0b5f29e0 )
	ROM_LOAD( "bubbles.6b",   0x5000, 0x1000, 0x4dd0450d )
	ROM_LOAD( "bubbles.7b",   0x6000, 0x1000, 0xe0a26ec0 )
	ROM_LOAD( "bubbles.8b",   0x7000, 0x1000, 0x4fd23d8d )
	ROM_LOAD( "bubbles.9b",   0x8000, 0x1000, 0xb48559fb )
	ROM_LOAD( "bubbles.10b",  0xd000, 0x1000, 0x26e7869b )
	ROM_LOAD( "bubbles.11b",  0xe000, 0x1000, 0x5a5b572f )
	ROM_LOAD( "bubbles.12b",  0xf000, 0x1000, 0xce22d2e2 )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "bubbles.snd",  0xf000, 0x1000, 0x689ce2aa )
ROM_END

ROM_START( bubblesr_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "bubblesr.1b",  0x0000, 0x1000, 0xdda4e782 )
	ROM_LOAD( "bubblesr.2b",  0x1000, 0x1000, 0x3c8fa7f5 )
	ROM_LOAD( "bubblesr.3b",  0x2000, 0x1000, 0xf869bb9c )
	ROM_LOAD( "bubblesr.4b",  0x3000, 0x1000, 0x0c65eaab )
	ROM_LOAD( "bubblesr.5b",  0x4000, 0x1000, 0x7ece4e13 )
	ROM_LOAD( "bubblesr.6b",  0x5000, 0x1000, 0x4dd0450d )
	ROM_LOAD( "bubblesr.7b",  0x6000, 0x1000, 0xe0a26ec0 )
	ROM_LOAD( "bubblesr.8b",  0x7000, 0x1000, 0x598b9bd6 )
	ROM_LOAD( "bubblesr.9b",  0x8000, 0x1000, 0xb48559fb )
	ROM_LOAD( "bubblesr.10b", 0xd000, 0x1000, 0x8b396db0 )
	ROM_LOAD( "bubblesr.11b", 0xe000, 0x1000, 0x096af43e )
	ROM_LOAD( "bubblesr.12b", 0xf000, 0x1000, 0x5c1244ef )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "bubbles.snd",  0xf000, 0x1000, 0x689ce2aa )
ROM_END


struct GameDriver bubbles_driver =
{
	__FILE__,
	0,
	"bubbles",
	"Bubbles",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&bubbles_machine_driver,        /* MachineDriver * */
	0,

	bubbles_rom,                    /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	bubbles_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};


struct GameDriver bubblesr_driver =
{
	__FILE__,
	&bubbles_driver,
	"bubblesr",
	"Bubbles (Solid Red label)",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nValerio Verrando",
	0,
	&bubbles_machine_driver,        /* MachineDriver * */
	0,

	bubblesr_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	bubbles_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};




/*
 *   Stargate
 */

ROM_START( stargate_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "01",           0x0000, 0x1000, 0x88824d18 )
	ROM_LOAD( "02",           0x1000, 0x1000, 0xafc614c5 )
	ROM_LOAD( "03",           0x2000, 0x1000, 0x15077a9d )
	ROM_LOAD( "04",           0x3000, 0x1000, 0xa8b4bf0f )
	ROM_LOAD( "05",           0x4000, 0x1000, 0x2d306074 )
	ROM_LOAD( "06",           0x5000, 0x1000, 0x53598dde )
	ROM_LOAD( "07",           0x6000, 0x1000, 0x23606060 )
	ROM_LOAD( "08",           0x7000, 0x1000, 0x4ec490c7 )
	ROM_LOAD( "09",           0x8000, 0x1000, 0x88187b64 )
	ROM_LOAD( "10",           0xd000, 0x1000, 0x60b07ff7 )
	ROM_LOAD( "11",           0xe000, 0x1000, 0x7d2c5daf )
	ROM_LOAD( "12",           0xf000, 0x1000, 0xa0396670 )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "sg.snd",       0xf800, 0x0800, 0x2fcf6c4d )
ROM_END

struct GameDriver stargate_driver =
{
	__FILE__,
	0,
	"stargate",
	"Stargate",
	"1981",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&stargate_machine_driver,       /* MachineDriver * */
	0,

	stargate_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	stargate_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};




/*
 *   Defender
 */

ROM_START( defender_rom )
	ROM_REGION(0x14000)
	ROM_LOAD( "defend.1",     0x0d000, 0x0800, 0xc3e52d7e )
	ROM_LOAD( "defend.4",     0x0d800, 0x0800, 0x9a72348b )
	ROM_LOAD( "defend.2",     0x0e000, 0x1000, 0x89b75984 )
	ROM_LOAD( "defend.3",     0x0f000, 0x1000, 0x94f51e9b )
	/* bank 0 is the place for CMOS ram */
	ROM_LOAD( "defend.9",     0x10000, 0x0800, 0x6870e8a5 )
	ROM_LOAD( "defend.12",    0x10800, 0x0800, 0xf1f88938 )
	ROM_LOAD( "defend.8",     0x11000, 0x0800, 0xb649e306 )
	ROM_LOAD( "defend.11",    0x11800, 0x0800, 0x9deaf6d9 )
	ROM_LOAD( "defend.7",     0x12000, 0x0800, 0x339e092e )
	ROM_LOAD( "defend.10",    0x12800, 0x0800, 0xa543b167 )
	ROM_RELOAD(            0x13800, 0x0800 )
	ROM_LOAD( "defend.6",     0x13000, 0x0800, 0x65f4efd1 )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "defend.snd",   0xf800, 0x0800, 0xfefd5b48 )
ROM_END

struct GameDriver defender_driver =
{
	__FILE__,
	0,
	"defender",
	"Defender",
	"1980",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&defender_machine_driver,       /* MachineDriver * */
	0,

	defender_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	defender_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	defender_cmos_load, defender_cmos_save
};




/*
 *   Splat!
 */

ROM_START( splat_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "splat.01",     0x0000, 0x1000, 0x1cf26e48 )
	ROM_LOAD( "splat.02",     0x1000, 0x1000, 0xac0d4276 )
	ROM_LOAD( "splat.03",     0x2000, 0x1000, 0x74873e59 )
	ROM_LOAD( "splat.04",     0x3000, 0x1000, 0x70a7064e )
	ROM_LOAD( "splat.05",     0x4000, 0x1000, 0xc6895221 )
	ROM_LOAD( "splat.06",     0x5000, 0x1000, 0xea4ab7fd )
	ROM_LOAD( "splat.07",     0x6000, 0x1000, 0x82fd8713 )
	ROM_LOAD( "splat.08",     0x7000, 0x1000, 0x7dded1b4 )
	ROM_LOAD( "splat.09",     0x8000, 0x1000, 0x71cbfe5a )
	ROM_LOAD( "splat.10",     0xd000, 0x1000, 0xd1a1f632 )
	ROM_LOAD( "splat.11",     0xe000, 0x1000, 0xca8cde95 )
	ROM_LOAD( "splat.12",     0xf000, 0x1000, 0x5bee3e60 )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "splat.snd",    0xf000, 0x1000, 0xa878d5f3 )
ROM_END


struct GameDriver splat_driver =
{
	__FILE__,
	0,
	"splat",
	"Splat",
	"1982",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&splat_machine_driver,          /* MachineDriver * */
	0,

	splat_rom,                      /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	splat_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

  cmos_load, cmos_save
};




/*
 *   Blaster
 */

ROM_START( blaster_rom )
	ROM_REGION(0x3c000)
	ROM_LOAD( "blaster.11",   0x04000, 0x2000, 0x6371e62f )
	ROM_LOAD( "blaster.12",   0x06000, 0x2000, 0x9804faac )
	ROM_LOAD( "blaster.17",   0x08000, 0x1000, 0xbf96182f )
	ROM_LOAD( "blaster.16",   0x0d000, 0x1000, 0x54a40b21 )
	ROM_LOAD( "blaster.13",   0x0e000, 0x2000, 0xf4dae4c8 )

	ROM_LOAD( "blaster.15",   0x00000, 0x4000, 0x1ad146a4 )
	ROM_LOAD( "blaster.8",    0x10000, 0x4000, 0xf110bbb0 )
	ROM_LOAD( "blaster.9",    0x14000, 0x4000, 0x5c5b0f8a )
	ROM_LOAD( "blaster.10",   0x18000, 0x4000, 0xd47eb67f )
	ROM_LOAD( "blaster.6",    0x1c000, 0x4000, 0x47fc007e )
	ROM_LOAD( "blaster.5",    0x20000, 0x4000, 0x15c1b94d )
	ROM_LOAD( "blaster.14",   0x24000, 0x4000, 0xaea6b846 )
	ROM_LOAD( "blaster.7",    0x28000, 0x4000, 0x7a101181 )
	ROM_LOAD( "blaster.1",    0x2c000, 0x4000, 0x8d0ea9e7 )
	ROM_LOAD( "blaster.2",    0x30000, 0x4000, 0x03c4012c )
	ROM_LOAD( "blaster.4",    0x34000, 0x4000, 0xfc9d39fb )
	ROM_LOAD( "blaster.3",    0x38000, 0x4000, 0x253690fb )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
	ROM_LOAD( "blaster.18",   0xf000, 0x1000, 0xc33a3145 )
ROM_END


struct GameDriver blaster_driver =
{
	__FILE__,
	0,
	"blaster",
	"Blaster",
	"1983",
	"Williams",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&blaster_machine_driver,        /* MachineDriver * */
	0,

	blaster_rom,                    /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	blaster_input_ports,

	blaster_remap_prom, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};




/*
 *   Colony 7
 */

ROM_START( colony7_rom )
	ROM_REGION(0x14000)
	ROM_LOAD( "cs03.bin",     0x0d000, 0x1000, 0x7ee75ae5 )
	ROM_LOAD( "cs02.bin",     0x0e000, 0x1000, 0xc60b08cb )
	ROM_LOAD( "cs01.bin",     0x0f000, 0x1000, 0x1bc97436 )
	/* bank 0 is the place for CMOS ram */
	ROM_LOAD( "cs06.bin",     0x10000, 0x0800, 0x318b95af )
	ROM_LOAD( "cs04.bin",     0x10800, 0x0800, 0xd740faee )
	ROM_LOAD( "cs07.bin",     0x11000, 0x0800, 0x0b23638b )
	ROM_LOAD( "cs05.bin",     0x11800, 0x0800, 0x59e406a8 )
	ROM_LOAD( "cs08.bin",     0x12000, 0x0800, 0x3bfde87a )
	ROM_RELOAD(           0x12800, 0x0800 )

	ROM_REGION_DISPOSE(0x1000)		/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)
	ROM_LOAD( "cs11.bin",     0xf800, 0x0800, 0x6032293c ) /* Sound ROM */
ROM_END

ROM_START( colony7a_rom )
	ROM_REGION(0x14000)
	ROM_LOAD( "cs03a.bin",    0x0d000, 0x1000, 0xe0b0d23b )
	ROM_LOAD( "cs02a.bin",    0x0e000, 0x1000, 0x370c6f41 )
	ROM_LOAD( "cs01a.bin",    0x0f000, 0x1000, 0xba299946 )
	/* bank 0 is the place for CMOS ram */
	ROM_LOAD( "cs06.bin",     0x10000, 0x0800, 0x318b95af )
	ROM_LOAD( "cs04.bin",     0x10800, 0x0800, 0xd740faee )
	ROM_LOAD( "cs07.bin",     0x11000, 0x0800, 0x0b23638b )
	ROM_LOAD( "cs05.bin",     0x11800, 0x0800, 0x59e406a8 )
	ROM_LOAD( "cs08.bin",     0x12000, 0x0800, 0x3bfde87a )
	ROM_RELOAD(            0x12800, 0x0800 )

	ROM_REGION_DISPOSE(0x1000)		/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)
	ROM_LOAD( "cs11.bin",     0xf800, 0x0800, 0x6032293c ) /* Sound ROM */
ROM_END

struct GameDriver colony7_driver =
{
	__FILE__,
	0,
	"colony7",
	"Colony 7 (set 1)",
	"1981",
	"Taito",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nMike Balfour",
	0,
	&colony7_machine_driver,       /* MachineDriver * */
	0,

	colony7_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,	/* sound_prom */

	colony7_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	defender_cmos_load, defender_cmos_save
};

struct GameDriver colony7a_driver =
{
	__FILE__,
	&colony7_driver,
	"colony7a",
	"Colony 7 (set 2)",
	"1981",
	"Taito",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nMike Balfour",
	0,
	&colony7_machine_driver,       /* MachineDriver * */
	0,

	colony7a_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,	/* sound_prom */

	colony7_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	defender_cmos_load, defender_cmos_save
};




/*
 *   Lotto Fun
 */

ROM_START( lottofun_rom )
	ROM_REGION(0x10000) 	/* 64k for code */
	ROM_LOAD( "vl4e.dat",     0x0000, 0x1000, 0x5e9af236 )
	ROM_LOAD( "vl4c.dat",     0x1000, 0x1000, 0x4b134ae2 )
	ROM_LOAD( "vl4a.dat",     0x2000, 0x1000, 0xb2f1f95a )
	ROM_LOAD( "vl5e.dat",     0x3000, 0x1000, 0xc8681c55 )
	ROM_LOAD( "vl5c.dat",     0x4000, 0x1000, 0xeb9351e0 )
	ROM_LOAD( "vl5a.dat",     0x5000, 0x1000, 0x534f2fa1 )
	ROM_LOAD( "vl6e.dat",     0x6000, 0x1000, 0xbefac592 )
	ROM_LOAD( "vl6c.dat",     0x7000, 0x1000, 0xa73d7f13 )
	ROM_LOAD( "vl6a.dat",     0x8000, 0x1000, 0x5730a43d )
	ROM_LOAD( "vl7a.dat",     0xd000, 0x1000, 0xfb2aec2c )
	ROM_LOAD( "vl7c.dat",     0xe000, 0x1000, 0x9a496519 )
	ROM_LOAD( "vl7e.dat",     0xf000, 0x1000, 0x032cab4b )

	ROM_REGION_DISPOSE(0x1000)		/* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000) 	/* 64k for the sound CPU */
	ROM_LOAD( "vl2532.snd",   0xf000, 0x1000, 0x214b8a04 )
ROM_END



struct GameDriver lottofun_driver =
{
	__FILE__,
	0,
	"lottofun",
	"Lotto Fun",
	"1987",
	"H.A.R. Management",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nMike Balfour (Ticket Dispenser)",
	0,
	&lottofun_machine_driver,		/* MachineDriver * */
	0,

	lottofun_rom,					 /* RomModule * */
	0, 0,							/* ROM decrypt routines */
	0,								/* samplenames */
	0,		/* sound_prom */

	lottofun_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cmos_load, cmos_save
};


/*
 *   Defense Command
 */

ROM_START( defcomnd_rom )
	ROM_REGION(0x15000)
        ROM_LOAD( "dfndr-c.rom", 0x0d000, 0x1000, 0x2a256b93 )
        ROM_LOAD( "dfndr-b.rom", 0x0e000, 0x1000, 0xe34e87fc )
        ROM_LOAD( "dfndr-a.rom", 0x0f000, 0x1000, 0xf78d62fa )
	/* bank 0 is the place for CMOS ram */
        ROM_LOAD( "dfndr-d.rom", 0x10000, 0x1000, 0xef2179fe )
        ROM_LOAD( "dfndr-e.rom", 0x11000, 0x1000, 0x4fa3d99c )
        ROM_LOAD( "dfndr-f.rom", 0x12000, 0x1000, 0x03721aa7 )
        ROM_LOAD( "dfndr-i.rom", 0x13000, 0x1000, 0x5998a4cf )
        ROM_LOAD( "dfndr-g.rom", 0x14000, 0x1000, 0xa1b63291 )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
        ROM_LOAD( "dfndr-h.rom", 0xf000, 0x1000, 0x30fced3d )
ROM_END

struct GameDriver defcomnd_driver =
{
	__FILE__,
	0,
	"defcomnd",
	"Defense Command",
	"1980?",
	"???",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	GAME_NOT_WORKING,
	&defcomnd_machine_driver,       /* MachineDriver * */
        0,

	defcomnd_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	defender_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	defender_cmos_load, defender_cmos_save
};

/*
 *   May Day (Alternate)
 */

ROM_START( mayday_rom )
	ROM_REGION(0x15000)
    ROM_LOAD( "mayday.c", 0x0d000, 0x1000, 0x872a2f2d )
    ROM_LOAD( "mayday.b", 0x0e000, 0x1000, 0xc4ab5e22 )
    ROM_LOAD( "mayday.a", 0x0f000, 0x1000, 0x329a1318 )
	/* bank 0 is the place for CMOS ram */
    ROM_LOAD( "mayday.d", 0x10000, 0x1000, 0xc2ae4716 )
    ROM_LOAD( "mayday.e", 0x11000, 0x1000, 0x41225666 )
    ROM_LOAD( "mayday.f", 0x12000, 0x1000, 0xc39be3c0 )
    ROM_LOAD( "mayday.g", 0x13000, 0x1000, 0x2bd0f106 )
    ROM_RELOAD(           0x14000, 0x1000 )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
    ROM_LOAD( "mayday.snd", 0xf800, 0x0800, 0xfefd5b48 )
ROM_END

struct GameDriver mayday_driver =
{
	__FILE__,
	0,
    "mayday",
    "MayDay",
	"1980?",
	"???",
    "Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nDavid Winter",
	GAME_NOT_WORKING,
    &mayday_machine_driver,       /* MachineDriver * */
    0,

    mayday_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	defender_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	defender_cmos_load, defender_cmos_save
};

ROM_START( maydaya_rom )
	ROM_REGION(0x15000)
    ROM_LOAD( "ic03-3.bin", 0x0d000, 0x1000, 0xa1ff6e62 )
    ROM_LOAD( "ic02-2.bin", 0x0e000, 0x1000, 0x62183aea )
    ROM_LOAD( "ic01-1.bin", 0x0f000, 0x1000, 0x5dcb113f )
	/* bank 0 is the place for CMOS ram */
    ROM_LOAD( "ic04-4.bin", 0x10000, 0x1000, 0xea6a4ec8 )
    ROM_LOAD( "ic05-5.bin", 0x11000, 0x1000, 0x0d797a3e )
    ROM_LOAD( "ic06-6.bin", 0x12000, 0x1000, 0xee8bfcd6 )
    ROM_LOAD( "ic07-7d.bin", 0x13000, 0x1000, 0xd9c065e7 )
    ROM_RELOAD(           0x14000, 0x1000 )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
    ROM_LOAD( "ic28-8.bin", 0xf800, 0x0800, 0xfefd5b48 )
    /* Sound ROM is same in both versions. Can be merged !!! */
ROM_END

struct GameDriver maydaya_driver =
{
	__FILE__,
	0,
    "maydaya",
    "MayDay (Alternate)",
	"1980?",
    "Nova",
    "Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nDavid Winter",
	GAME_NOT_WORKING,
    &mayday_machine_driver,       /* MachineDriver (suppose same as 'mayday') * */
    0,

    maydaya_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	defender_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	defender_cmos_load, defender_cmos_save
};


/*
 *  Defender (?) by Jeutel
 */

static void defndjeu_decode(void)
{
/*
    Note: Please do not remove these comments in BETA versions. They are
          helpful to get the games working. When they will work, useless
          comments may be removed as desired.

    The encryption in this game is one of the silliest I have ever seen.
    I just wondered if the ROMs were encrypted, and figured out how they
    were in just about 5 mins...
    Very simple: bits 0 and 7 are swapped in the ROMs (not sound).

    Game does not work due to bad ROMs 16 and 20. However, the others are
    VERY similar (if not nearly SAME) to MAYDAY and DEFENSE ones (and NOT
    DEFENDER), although MAYDAY ROMs are more similar than DEFENSE ones...
    By putting MAYDAY ROMs and encrypting them, I got a first machine test
    and then, reboot... The test was the random graphic drawings, which
    were bad. Each time the full screen was drawn, the game rebooted.
    Unfortunately, I don't remember which roms I took to get that, and I
    could not get the same result anymore (I did not retry ALL the
    possibilities I did at 01:30am). :-(


    ROM equivalences (not including the sound ROM):

    MAYDAY      MAYDAY (Alternate)    DEFENSE       JEUTEL's Defender
    -----------------------------------------------------------------
    ROMC.BIN    IC03-3.BIN            DFNDR-C.ROM           15
    ROMB.BIN    IC02-2.BIN            DFNDR-B.ROM           16
    ROMA.BIN    IC01-1.BIN            DFNDR-A.ROM           17
    ROMG.BIN    IC07-7D.BIN           DFNDR-G.ROM           18
    ROMF.BIN    IC06-6.BIN            DFNDR-F.ROM           19
    ROME.BIN    IC05-5.BIN            DFNDR-E.ROM           20
    ROMD.BIN    IC04-4.BIN            DFNDR-D.ROM           21
*/

    int x;
    unsigned char *DEF_ROM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

    for (x = 0xd000; x < 0x15000; x++)
	{
        unsigned char src, dst;

        src = DEF_ROM[x];
        dst = DEF_ROM[x] & 0x7e;

        if (src & 0x80)
            dst = dst | 0x01;

        if (src & 0x01)
            dst = dst | 0x80;

        DEF_ROM[x] = dst;
	}
}


ROM_START( defndjeu_rom )
	ROM_REGION(0x15000)
    ROM_LOAD( "15", 0x0d000, 0x1000, 0x706a24bd )
    ROM_LOAD( "16", 0x0e000, 0x1000, 0x03201532 )
    ROM_LOAD( "17", 0x0f000, 0x1000, 0x25287eca )
	/* bank 0 is the place for CMOS ram */
    ROM_LOAD( "18", 0x10000, 0x1000, 0xe99d5679 )
    ROM_LOAD( "19", 0x11000, 0x1000, 0x769f5984 )
    ROM_LOAD( "20", 0x12000, 0x1000, 0x12fa0788 )
    ROM_LOAD( "21", 0x13000, 0x1000, 0xbddb71a3 )
    ROM_RELOAD(           0x14000, 0x1000 )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
    ROM_LOAD( "s", 0xf800, 0x0800, 0xcb79ae42 )
ROM_END


struct GameDriver defndjeu_driver =
{
	__FILE__,
	0,
    "defndjeu",
    "Defender ? (Bootleg)",
    "1980",
    "Jeutel",
    "Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nDavid Winter",
	GAME_NOT_WORKING,
    &mayday_machine_driver,       /* MachineDriver (suppose same as 'mayday') * */
    0,

    defndjeu_rom,                   /* RomModule * */
    0, defndjeu_decode,            /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	defender_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	defender_cmos_load, defender_cmos_save
};


ROM_START( defcmnd_rom )
	ROM_REGION(0x15000)
    ROM_LOAD( "defcmnda.1",   0x0d000, 0x1000, 0x68effc1d )
    ROM_LOAD( "defcmnda.2",   0x0e000, 0x1000, 0x1126adc9 )
    ROM_LOAD( "defcmnda.3",   0x0f000, 0x1000, 0x7340209d )
	/* bank 0 is the place for CMOS ram */
    ROM_LOAD( "defcmnda.10",  0x10000, 0x0800, 0x3dddae75 )
    ROM_LOAD( "defcmnda.7",   0x10800, 0x0800, 0x3f1e7cf8 )
    ROM_LOAD( "defcmnda.9",   0x11000, 0x0800, 0x8882e1ff )
    ROM_LOAD( "defcmnda.6",   0x11800, 0x0800, 0xd068f0c5 )
    ROM_LOAD( "defcmnda.8",   0x12000, 0x0800, 0xfef4cb77 )
    ROM_LOAD( "defcmnda.5",   0x12800, 0x0800, 0x49b50b40 )
    ROM_LOAD( "defcmnda.4",   0x13000, 0x0800, 0x43d42a1b )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
    ROM_LOAD( "defcmnda.snd", 0xf800, 0x0800, 0xf122d9c9 )
ROM_END

ROM_START( defence_rom )
	ROM_REGION(0x15000)
    ROM_LOAD( "1",            0x0d000, 0x1000, 0xebc93622 )
    ROM_LOAD( "2",            0x0e000, 0x1000, 0x2a4f4f44 )
    ROM_LOAD( "3",            0x0f000, 0x1000, 0xa4112f91 )
	/* bank 0 is the place for CMOS ram */
    ROM_LOAD( "0",            0x10000, 0x0800, 0x7a1e5998 )
    ROM_LOAD( "7",            0x10800, 0x0800, 0x4c2616a3 )
    ROM_LOAD( "9",            0x11000, 0x0800, 0x7b146003 )
    ROM_LOAD( "6",            0x11800, 0x0800, 0x6d748030 )
    ROM_LOAD( "8",            0x12000, 0x0800, 0x52d5438b )
    ROM_LOAD( "5",            0x12800, 0x0800, 0x4a270340 )
    ROM_LOAD( "4",            0x13000, 0x0800, 0xe13f457c )

	ROM_REGION_DISPOSE(0x1000)      /* temporary space for graphics (disposed after conversion) */
	/* empty memory region - not used by the game, but needed because the main */
	/* core currently always frees region #1 after initialization. */

	ROM_REGION(0x10000)     /* 64k for the sound CPU */
    ROM_LOAD( "12",           0xf800, 0x0800, 0xf122d9c9 )
ROM_END


struct GameDriver defcmnd_driver =
{
	__FILE__,
	&defender_driver,
    "defcmnd",
    "Defense Command",
    "1980",
    "bootleg",
    "Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles\nDavid Winter",
    0,
    &mayday_machine_driver,       /* MachineDriver (suppose same as 'mayday') * */
    0,

    defcmnd_rom,                   /* RomModule * */
    0, 0,            /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	defender_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	defender_cmos_load, defender_cmos_save
};

struct GameDriver defence_driver =
{
	__FILE__,
	&defender_driver,
	"defence",
	"Defence Command",
	"1980",
	"bootleg",
	"Marc Lafontaine\nSteven Hugg\nMirko Buffoni\nAaron Giles",
	0,
	&defender_machine_driver,       /* MachineDriver * */
	0,

	defence_rom,                   /* RomModule * */
	0, 0,                           /* ROM decrypt routines */
	0,                              /* samplenames */
	0,      /* sound_prom */

	defender_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	defender_cmos_load, defender_cmos_save
};
