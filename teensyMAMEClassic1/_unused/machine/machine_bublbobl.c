/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#if 1
#define RELEASE
#endif

#define MAKE_ITEMS_EASY

/* configurable */

#ifdef RELEASE
#define WONDERFUL_JOURNEY_TIME 0x01e0 /* originally 0x01E0 */
#else
/* #define LOG_SOUND_EFFECTS */
/* #define LOTSA_CANES */

/* #define LOTSA_POTIONS */
/* #define POTION_TYPE 0x0f */	/* sunflower potion */
/* #define POTION_TYPE 0x10 */	/* flower potion */
/* #define POTION_TYPE 0x11 */	/* clover potion */
/* #define POTION_TYPE 0x12 */	/* rainbow potion */
#define POTION_TYPE 0x13	/* musical potion */

/* #define LOTSA_SHOES */

#define WONDERFUL_JOURNEY_TIME 0x10 /* originally 0x01E0 */
#define FIX_EXTEND_LETTERS
#define P1_FREE_EXTEND_LETTERS 0x3e
#define P2_FREE_EXTEND_LETTERS 0x1f
#endif /* RELEASE */

/* fixed */
#define MODIFY_ROM(addr,val) Machine->memory_region[0][addr] = val;

#define MOD_PAGE(page,addr,data) Machine->memory_region[0][page ? addr-0x8000+0x10000+0x4000*(page-1) : addr] = data;

#define MODIFY_ROM_WORD(addr,val)   \
    MODIFY_ROM(addr, (val)&0xff);   \
    MODIFY_ROM((addr)+1, ((val)&0xff00)>>8);

#define HALT_AT(addr) \
    MODIFY_ROM(addr, 0x76);

#define LOG_PAGE(page,addr)	    \
    if (errorlog) fprintf(errorlog, \
			  "original value in %04x of ROM %x is %02x\n", \
			  addr, page, Machine->memory_region[0][addr+0x10000*page]);

#define SET_ITEM_THRESHOLD(item,threshold) \
	MOD_PAGE(2, 0xB667 - 7 * item, threshold); \
	MOD_PAGE(2, 0xB668 - 7 * item, threshold); \
	MOD_PAGE(2, 0xB669 - 7 * item, threshold); \
	MOD_PAGE(2, 0xB66A - 7 * item, threshold);

#define MAKE_ITEM_EASY(item) \
	SET_ITEM_THRESHOLD(item, 1);

#define MAKE_ITEM_TRIVIAL(item) \
	SET_ITEM_THRESHOLD(item, 0);

#define CURRENT_LEVEL	  bublbobl_sharedram1[0x264b]
#define P1_EXTEND_LETTERS bublbobl_sharedram1[0x2742]
#define P2_EXTEND_LETTERS bublbobl_sharedram1[0x2743]
int start_level = 0;		/* set by '-level <n>' flag' */
int easy_item = -1;
int always_item = -1;

#include "driver.h"

unsigned char *bublbobl_sharedram1,*bublbobl_sharedram2;




int bublbobl_interrupt(void)
{
    bublbobl_sharedram2[0xfc85-0xfc01] = 0x37;

    if (start_level) CURRENT_LEVEL = start_level;

#ifdef FIX_EXTEND_LETTERS
    P1_EXTEND_LETTERS = P1_FREE_EXTEND_LETTERS;
    P2_EXTEND_LETTERS = P2_FREE_EXTEND_LETTERS;
#endif

#ifdef LOTSA_CANES
    bublbobl_sharedram1[0x25f8] = 0x0a;	/* always give a cane which causes
				   giant fruit to appear */
#endif

#ifdef LOTSA_POTIONS
    bublbobl_sharedram1[0x25e7] = POTION_TYPE; /* always give a potion of this type */
#endif

#ifdef LOTSA_SHOES
    bublbobl_sharedram1[0x25e6] = 0x10; /* always give running shoes */
#endif

#ifdef MAKE_ITEMS_EASY
    if (easy_item != -1)
	MAKE_ITEM_EASY(easy_item);

    if (always_item != -1)
	MAKE_ITEM_TRIVIAL(always_item);
#endif

/*  0/00 - red and white sweetie -> long range bubbles
    1/01 - weird looking thing - roots?	 -> fast bubbles
    2/02 - orange and yellow sweetie - no discernable effect?
    3/03 - shoe -> run faster
    4/04 - clock -> ?
    5/05 - bomb -> 10k (free) or 6k (bubbled) diamonds
    6/06 - orange umbrella -> skip 3 levels
    7/07 - red umbrella -> skip 5 levels
    8/08 - purple umbrella -> skip 7 levels
    9/09 - sunflower potion
   10/0a - flower potion
   11/0b - clover potion
   12/0c - rainbow potion
   13/0d - musical potion
   14/0e - flashing heart - freezes monsters & makes you flash & go
	   all invincible
   15/0f - blue ring - points for running
   16/10 - purple ring - points for jumping
   17/11 - red ring - points for bubbles
   18/12 - blue (and red!) cross - drowns monsters in (pink!) water
   19/13 - yellow and red cross - lightning kills monsters
   20/14 - red and orange cross - breath fireballs for 9k diamonds
   21/15 - cyan teapot - no effect?
   22/16 - red teapot - long range (and fast?) bubbles
   23/17 - exploding purple teapot
   24/18 - yellow teapot - long range (and fast?) bubbles
   25/19 - book - shakes screen for 6/8k diamonds
   26/1a - (wrong coloured) diamond necklace - flying junk -> 6k diamonds
   27/1b - red pearl necklace - no effect?
   28/1c - purple? pearl necklace for bouncing thing -> 6k diamonds
   29/1d - fork -> flying vegetables for 6k diamonds
   30/1e - purple mailbox -> 80k diamond
   31/1f - pink/orange mailbox -> 70k diamond
   32/20 - brown mailbox -> 60k diamond
   33/21 - cyan mailbox -> 50k diamond
   34/22 - grey mailbox -> 40k diamond - appears if you get cross $13
   35/23 - red/pink cane 30k pointy cake for D's
   36/24 - red/brown cane 30k iced bun for N's
   37/25 - red cane 20k melon for E's
   38/26 - orange cane 10k apple for T's
   39/27 - light brown cane 10k lolly for  X's
   40/28 - cyan cane 10k double lolly for E's
   41/29 - iron?
   42/2a - spider (turns bubbles into X's at end of level?)
   43/2b - flamingo (turns bubbles into smiling turds at end of level?)
   44/2c - lager (turns bubbles into pizzas at end of level?)
   45/2d - knife for flying cakes/beer/etc and 6k diamonds)
   46/2e - crystal ball (special item come quicker next round (and forever?))
   47/2f - pencil/cigal (makes next 3 special items be potions) - appears once
	   every 777 levels!!!
   48/30 - skull (immediate 'hurry up')
   49/31 - treasure room 20
   50/32 - treasure room 30
   51/33 - treasure room 40
   52/34 - treasure room 50
   53/35 - coke (makes it rain coke cans? -> 6k diamonds) */

    return 0x2e;
}



void bublbobl_patch(void)
{
    /* LOG_PAGE(1, 0xA73E); LOG_PAGE(1, 0xA73F); LOG_PAGE(1, 0xA740); */

    /* set level 0 to have just one baby */
    /* MOD_PAGE(1, 0xA73E, 0x00);
    MOD_PAGE(1, 0xA73F, 0x10);
    MOD_PAGE(1, 0xA740, 0x00); */

    /* speed up the 'wonderful journey' message */
    MODIFY_ROM_WORD(0x25F6, WONDERFUL_JOURNEY_TIME);

    /* remove protection */
    MODIFY_ROM(0x0533,0xc9); MODIFY_ROM(0x0b20,0xc9); MODIFY_ROM(0x0b48,0x18);
    MODIFY_ROM(0x0b8b,0x18); MODIFY_ROM(0x0d4e,0xc9); MODIFY_ROM(0x13c2,0xc9);
    MODIFY_ROM(0x1b07,0x18); MODIFY_ROM(0x1d7e,0x18); MODIFY_ROM(0x1db1,0x18);
    MODIFY_ROM(0x1ecf,0x18); MODIFY_ROM(0x24eb,0xc9); MODIFY_ROM(0x313f,0xc9);
    MODIFY_ROM(0x3349,0xc9); MODIFY_ROM(0x3447,0xc9); MODIFY_ROM(0x37A9,0x18);
    MODIFY_ROM(0x39D5,0x18); MODIFY_ROM(0x3E58,0x18); MODIFY_ROM(0x40E9,0x18);
    MODIFY_ROM(0x4245,0xc9); MODIFY_ROM(0x4866,0xc9); MODIFY_ROM(0x4B93,0x18);
    MODIFY_ROM(0x4F50,0xc9); MODIFY_ROM(0x4F84,0x18); MODIFY_ROM(0x54A1,0x00);
    MODIFY_ROM(0x54A2,0x00); MODIFY_ROM(0x58ED,0x18); MODIFY_ROM(0x5955,0x18);
    MODIFY_ROM(0x5C26,0x18); MODIFY_ROM(0x5D0E,0x18); MODIFY_ROM(0x60A0,0xc9);
    MODIFY_ROM(0x660A,0xc9); MODIFY_ROM(0x6612,0x18); MODIFY_ROM(0x6802,0x18);
    MODIFY_ROM(0x7A31,0x18); MODIFY_ROM(0x7C07,0x18);
    MOD_PAGE(0,0x8291,0x18); MOD_PAGE(0,0x8498,0x18); MOD_PAGE(0,0x85C4,0x18);
    MOD_PAGE(0,0x8F58,0x18); MOD_PAGE(0,0x927B,0xc9); MOD_PAGE(0,0x92FB,0x18);
    MOD_PAGE(0,0x969F,0x18); MOD_PAGE(0,0x98EF,0x18); MOD_PAGE(0,0x9DD0,0xc9);
    MOD_PAGE(0,0x9E3D,0x18); MOD_PAGE(0,0xA15A,0x18); MOD_PAGE(0,0xA609,0x18);
    MOD_PAGE(0,0xB154,0x18); MOD_PAGE(0,0xB937,0xc9); MOD_PAGE(0,0xBC1F,0x18);
    MOD_PAGE(2,0x84F1,0x18); MOD_PAGE(2,0x85E9,0xc9); MOD_PAGE(2,0x87A0,0x18);
    MOD_PAGE(2,0x89D4,0x18); MOD_PAGE(2,0x8BA5,0xc9); MOD_PAGE(2,0x8DA1,0xc9);
    MOD_PAGE(2,0x8F51,0xc9); MOD_PAGE(2,0x90DB,0xc9); MOD_PAGE(2,0x9367,0xc9);
    MOD_PAGE(2,0x96E4,0xc9); MOD_PAGE(2,0xA2FA,0xc9); MOD_PAGE(2,0xA81D,0xc9);
    MOD_PAGE(2,0xB4EF,0xc9); MOD_PAGE(2,0xBB0D,0xc9);

    /* ease up on the RAM check */
    MODIFY_ROM(0x00c5, 1); MODIFY_ROM(0x00c6, 0);
    MODIFY_ROM(0x00f5, 2); MODIFY_ROM(0x00f6, 0);
    MODIFY_ROM(0x00fb, 0); MODIFY_ROM(0x00fc, 0); MODIFY_ROM(0x00fd, 0);
    MODIFY_ROM(0x0102, 0); MODIFY_ROM(0x0103, 0); MODIFY_ROM(0x0104, 0);
    MODIFY_ROM(0x0109, 0); MODIFY_ROM(0x010a, 0); MODIFY_ROM(0x010b, 0);
    MODIFY_ROM(0x010f, 0); MODIFY_ROM(0x0110, 0); MODIFY_ROM(0x0111, 0);

    HALT_AT(0x0534); HALT_AT(0x0B21); HALT_AT(0x0B4A); HALT_AT(0x0B8D);
    HALT_AT(0x0D4F); HALT_AT(0x13C3); HALT_AT(0x1B09); HALT_AT(0x1D80);
    HALT_AT(0x1DB3); HALT_AT(0x1ED1); HALT_AT(0x24EC); HALT_AT(0x3140);
    HALT_AT(0x334A); HALT_AT(0x3448); HALT_AT(0x37AB); HALT_AT(0x39D7);
    HALT_AT(0x3E5A); HALT_AT(0x40EB); HALT_AT(0x4246); HALT_AT(0x4867);
    HALT_AT(0x4B95); HALT_AT(0x4F51); HALT_AT(0x4F89); HALT_AT(0x54A1);
    HALT_AT(0x58EF); HALT_AT(0x5958); HALT_AT(0x5C28); HALT_AT(0x5D10);
    HALT_AT(0x60A1); HALT_AT(0x660A); HALT_AT(0x6614); HALT_AT(0x6804);
    HALT_AT(0x7A33); HALT_AT(0x7C09);
    MOD_PAGE(0, 0x8293, 0x76); MOD_PAGE(0, 0x849A, 0x76);
    MOD_PAGE(0, 0x85C6, 0x76); MOD_PAGE(0, 0x8F5A, 0x76);
    MOD_PAGE(0, 0x927C, 0x76); MOD_PAGE(0, 0x92FD, 0x76);
    MOD_PAGE(0, 0x96A1, 0x76); MOD_PAGE(0, 0x98F1, 0x76);
    MOD_PAGE(0, 0x9DD1, 0x76); MOD_PAGE(0, 0x9E3F, 0x76);
    MOD_PAGE(0, 0xA15C, 0x76); MOD_PAGE(0, 0xA60B, 0x76);
    MOD_PAGE(0, 0xB156, 0x76); MOD_PAGE(0, 0xB938, 0x76);
    MOD_PAGE(0, 0xBC21, 0x76);
    MOD_PAGE(2, 0x84F3, 0x76); MOD_PAGE(2, 0x85EA, 0x76);
    MOD_PAGE(2, 0x87A4, 0x76); MOD_PAGE(2, 0x89D6, 0x76);
    MOD_PAGE(2, 0x8BA6, 0x76); MOD_PAGE(2, 0x8DA2, 0x76);
    MOD_PAGE(2, 0x8F52, 0x76); MOD_PAGE(2, 0x90DC, 0x76);
    MOD_PAGE(2, 0x9368, 0x76); MOD_PAGE(2, 0x96E5, 0x76);
    MOD_PAGE(2, 0xA2FB, 0x76); MOD_PAGE(2, 0xA81E, 0x76);
    MOD_PAGE(2, 0xB4F0, 0x76); MOD_PAGE(2, 0xBB0E, 0x76);
}

void boblbobl_patch(void)
{
    /* these shouldn't be necessary, surely - this is a bootleg ROM
     * with the protection removed - so what are all these JP's to
     * 0xa288 doing?  and why does the emulator fail the ROM checks?
     */

	MOD_PAGE(3,0x9a71,0x00); MOD_PAGE(3,0x9a72,0x00); MOD_PAGE(3,0x9a73,0x00);
	MOD_PAGE(3,0xa4af,0x00); MOD_PAGE(3,0xa4b0,0x00); MOD_PAGE(3,0xa4b1,0x00);
	MOD_PAGE(3,0xa55d,0x00); MOD_PAGE(3,0xa55e,0x00); MOD_PAGE(3,0xa55f,0x00);
	MOD_PAGE(3,0xb561,0x00); MOD_PAGE(3,0xb562,0x00); MOD_PAGE(3,0xb563,0x00);
}



int bublbobl_sharedram1_r(int offset)
{
	return bublbobl_sharedram1[offset];
}
int bublbobl_sharedram2_r(int offset)
{
	return bublbobl_sharedram2[offset];
}

void bublbobl_sharedram1_w(int offset, int data)
{
	bublbobl_sharedram1[offset] = data;
}
void bublbobl_sharedram2_w(int offset, int data)
{
	bublbobl_sharedram2[offset] = data;
}



void bublbobl_bankswitch_w(int offset,int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((data & 3) == 0) { cpu_setbank(1,&RAM[0x8000]); }
	else { cpu_setbank(1,&RAM[0x10000 + 0x4000 * ((data & 3) - 1)]); }
}
