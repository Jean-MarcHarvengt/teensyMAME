/***************************************************************************

Various Data East 8 bit games:

	Cobra Command               (c) 1988 Data East Corporation (6809)
	The Real Ghostbusters       (c) 1987 Data East USA (6809 + I8751)
	Mazehunter                  (c) 1987 Data East Corporation (6809 + I8751)
	Super Real Darwin           (c) 1987 Data East Corporation (6809 + I8751)
	Psycho-Nics Oscar           (c) 1988 Data East USA (2*6809 + I8751)
	Psycho-Nics Oscar (Japan)   (c) 1987 Data East Corporation (2*6809 + I8751)

(The following aren't finished but will be soon):
	Gondomania					(6809 + I8751)
	Last Mission                (2*6809 + I8751)
	Shackled                    (2*6809 + I8751)
    Breywood                    (2*6809 + I8751)

Emulation by Bryan McPhail, mish@tendril.force9.net

Note!!!

I am currently working on the 4 non-working games above and also have some
improvements to make to the others, if you have any alterations to make to
this file let me know in case I have done it already!  This file and the
video hardware file are subject to massive alteration any time :)

Emulation Notes:

* Dip switches only confirmed for Oscar, the others seem reasonable.
* Ghostbusters, Darwin, Oscar use a "Deco 222" custom 6502 for sound.  The code
is encrypted.  See attempts to decrypt it at the bottom of this file.
* Darwin crashes at end of attract mode/end of level 1
* Maze Hunter is using Ghostbusters proms for now...

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809/m6809.h"
#include "M6502/M6502.h"

int dec8_video_r(int offset);
void dec8_video_w(int offset, int data);
void dec8_pf1_w(int offset, int data);
void dec8_pf2_w(int offset, int data);
void dec8_scroll1_w(int offset, int data);
void dec8_scroll2_w(int offset, int data);
void dec8_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
void ghostb_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
void srdarwin_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
void lastmiss_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
void oscar_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
int dec8_vh_start(void);
void dec8_vh_stop(void);

int oscar_share_r(int offset);
void oscar_share_w(int offset,int data);
int oscar_share2_r(int offset);
void oscar_share2_w(int offset,int data);
int shackled_share_r(int offset);
void shackled_share_w(int offset,int data);

void srdarwin_palette_rg(int offset, int data);
void srdarwin_palette_b(int offset, int data);
int srdarwin_palette_rg_r(int offset);
int srdarwin_palette_b_r(int offset);
void srdarwin_control_w(int offset, int data);

void lastmiss_control_w(int offset, int data);
void lastmiss_scrollx_w(int offset, int data);
void lastmiss_scrolly_w(int offset, int data);

void ghostb_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

/******************************************************************************/

extern unsigned char *dec8_row;
static int prota, protb;
static int coin_flag;
static int ghost_prot;

static int gondo_prot1_r(int offset)
{
	if (errorlog) fprintf(errorlog,"PC %06x - Read from 8751 low\n",cpu_getpc());
	return 0x74;
}

static int gondo_prot2_r(int offset)
{
	if (errorlog) fprintf(errorlog,"PC %06x - Read from 8751 low\n",cpu_getpc());
	return rand()%0x7;
}

static int prot3_r(int offset)
{
//  	if (errorlog) fprintf(errorlog,"PC %06x - Read from vbl\n",cpu_getpc());
	return ((readinputport(2)+rand()%0xf)&0xfc) + readinputport(2);
}

static int lastmiss_prot1(int offset)
{
 // 	if (errorlog) fprintf(errorlog,"PC %06x - Read from prot1\n",cpu_getpc());
	return 0x84;
}
static int lastmiss_prot2(int offset)
{
 // 	if (errorlog) fprintf(errorlog,"PC %06x - Read from prot2\n",cpu_getpc());
	return 1;
}
static int i8751_l_r(int offset)
{
	static int latch[3];
	int i8751_out=readinputport(3);

 	if (errorlog && cpu_getpc()!=0x8a20) fprintf(errorlog,"PC %06x - Read from 8751 low\n",cpu_getpc());

	/* Ghostbusters protection */
	if ((i8751_out & 0x4) == 0x4) latch[0]=1;
	if ((i8751_out & 0x2) == 0x2) latch[1]=1;
	if ((i8751_out & 0x1) == 0x1) latch[2]=1;

	if (((i8751_out & 0x4) != 0x4) && latch[0]) {latch[0]=0; return 0x80; } /* Player 1 coin */
	if (((i8751_out & 0x2) != 0x2) && latch[1]) {latch[1]=0; return 0x40; } /* Player 2 coin */
	if (((i8751_out & 0x1) != 0x1) && latch[2]) {latch[2]=0; return 0x10; } /* Service */

	if (protb==0xaa && prota==0) return 6;
	if (protb==0x1a && prota==2) return 6;

	/* Darwin */
	if (protb==0x00 && prota==0x00) return 0x00;
	if (protb==0x00 && prota==0x40) return 0x40;
	if (protb==0x0f && prota==0x40) return 0x40;

	/* Maze Hunter */
	if (protb==0x1b && prota==0x02) return 0x6;

	return 0;
}

static int i8751_h_r(int offset)
{
	if (errorlog) fprintf(errorlog,"PC %06x - Read from 8751 high\n",cpu_getpc());

	/* Ghostbusters protection */
	if (protb==0xaa && prota==0) return 0x55;
	if (protb==0x1a && prota==2) return 0xe5;

	/* Darwin */
	if (protb==0x00 && prota==0x00) return 0x00;
	if (protb==0x63 && prota==0x30) return 0x9c;
	if (protb==0x00 && prota==0x40) return 0x00;
	if (protb==0x0f && prota==0x40) return 0x0f;
	if (protb==0x00 && prota==0x50) return 3; /* number of credits */

	/* Maze Hunter */
	if (coin_flag) {
		coin_flag=0;
		return 1;
	}
	if (protb==0x1b && prota==0x02) return 0xe4;

	return 0;
}

/******************************************************************************/

static void i8751_l_w(int offset, int data)
{
	prota=data;
	if (errorlog) fprintf(errorlog,"PC %06x - Write %02x to 8751 low\n",cpu_getpc(),data);
}

static void i8751_h_w(int offset, int data)
{
	protb=data;
	if (errorlog) fprintf(errorlog,"PC %06x - Write %02x to 8751 high\n",cpu_getpc(),data);
}

/******************************************************************************/

static void dec8_bank_w(int offset, int data)
{
 	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
//if (errorlog) fprintf(errorlog,"PC %06x - Bank switch %02x (%02x)\n",cpu_getpc(),data&0xf,data);
	bankaddress = 0x10000 + (data & 0x0f) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);
}

static void gondo_bank_w(int offset, int data)
{
 	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	bankaddress = 0x10000 + (data & 0x0f) * 0x4000;

	cpu_setbank(1,&RAM[bankaddress]);
	if (errorlog) fprintf(errorlog,"PC %06x - Bank switch %02x (%02x)\n",cpu_getpc(),data&0xf,data);
}

static void ghostb_bank_w(int offset, int data)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
 	int bankaddress;

	bankaddress = 0x10000 + (data >> 4) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);
	ghost_prot=data;
}

static void dec8_sound_w(int offset, int data)
{
 	soundlatch_w(0,data);
	cpu_cause_interrupt(1, M6502_INT_NMI);
}

static void oscar_sound_w(int offset, int data)
{
 	soundlatch_w(0,data);
	cpu_cause_interrupt(2, M6502_INT_NMI);
}

/******************************************************************************/

static void oscar_int_w(int offset, int data)
{
	/* Deal with interrupts, coins also generate NMI to CPU 0 */
	switch (offset) {
		case 0: /* IRQ2 */
			cpu_cause_interrupt (1, M6809_INT_IRQ);
			return;
		case 1: /* IRC 1 */
			return;
		case 2: /* IRQ 1 */
			cpu_cause_interrupt (0, M6809_INT_IRQ);
			return;
		case 3: /* IRC 2 */
			return;
	}
}

static void lastmiss_int_w(int offset, int data)
{
  //	if (errorlog) fprintf(errorlog,"PC %06x - Int %02x to %d\n",cpu_getpc(),data,offset);
	switch (offset) {
		case 0: /* CPU 2 - IRQ acknowledge */
    //     cpu_cause_interrupt (0, M6809_INT_FIRQ);
            return;
        case 1: /* CPU 1 - IRQ acknowledge */
        	return;
        case 2: /* CPU 1 - FIRQ acknowledge */
	 //		cpu_cause_interrupt (1, M6809_INT_IRQ);
            return;
        case 4:
        /* CPU 1>???? */
		   //	cpu_cause_interrupt (0, M6809_INT_FIRQ);
            cpu_cause_interrupt (1, M6809_INT_IRQ);
            return;
	}
}



static void shackled_int_w(int offset, int data)
{

 //  	if (errorlog) fprintf(errorlog,"PC %06x - Int %02x to %d\n",cpu_getpc(),data,offset);
	/* Deal with other interrupts */
	switch (offset) {
		case 0: /* IRQ1??? */
       //   cpu_cause_interrupt (0, M6809_INT_IRQ);
            return;
        case 1: /* IRQ 2 Request */
			cpu_cause_interrupt (1, M6809_INT_FIRQ);
        	return;

        case 2: /* IRQ 2 Acknowledge */
            return;

        case 3: /* IRC 1? */
          //    cpu_cause_interrupt (0, M6809_INT_IRQ);

     //        cpu_cause_interrupt (1, M6809_INT_FIRQ);
			return;
	}
}

static int ghostb_cycle_r(int offset)
{
	unsigned char *RAM = Machine->memory_region[0];
	int p=cpu_getpc();

	if (p==0x8db8 || p==0x846c) cpu_spinuntil_int();
	return RAM[0x69];
}

/******************************************************************************/

static struct MemoryReadAddress cobra_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1fff, dec8_video_r },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x2800, 0x2fff, MRA_RAM },
	{ 0x3200, 0x37ff, MRA_RAM }, /* Unknown, probably unused in this game */
	{ 0x3800, 0x3800, input_port_0_r }, /* Player 1 */
	{ 0x3801, 0x3801, input_port_1_r }, /* Player 2 */
	{ 0x3802, 0x3802, input_port_3_r }, /* Dip 1 */
	{ 0x3803, 0x3803, input_port_4_r }, /* Dip 2 */
	{ 0x3a00, 0x3a00, input_port_2_r }, /* VBL & coins */
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress cobra_writemem[] =
{
 	{ 0x0000, 0x0fff, MWA_RAM },
 	{ 0x1000, 0x1fff, dec8_video_w },
	{ 0x2000, 0x27ff, MWA_RAM, &videoram, &videoram_size },
	{ 0x2800, 0x2fff, MWA_RAM, &spriteram },
	{ 0x3000, 0x31ff, paletteram_xxxxBBBBGGGGRRRR_swap_w, &paletteram },
	{ 0x3200, 0x37ff, MWA_RAM }, /* Unknown, probably unused in this game */
 	{ 0x3800, 0x3806, dec8_pf1_w },
	{ 0x3810, 0x3813, dec8_scroll1_w },
	{ 0x3a00, 0x3a06, dec8_pf2_w },
 	{ 0x3a10, 0x3a13, dec8_scroll2_w },
	{ 0x3c00, 0x3c00, dec8_bank_w },
	{ 0x3c02, 0x3c02, MWA_NOP }, /* Lots of 1s written here, don't think it's a watchdog */
 	{ 0x3e00, 0x3e00, dec8_sound_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress ghostb_readmem[] =
{
//	{ 0x0069, 0x0069, ghostb_cycle_r },
	{ 0x0000, 0x1fff, MRA_RAM },
	{ 0x2000, 0x27ff, dec8_video_r },
	{ 0x2800, 0x2dff, MRA_RAM },
	{ 0x3000, 0x37ff, MRA_RAM },
	{ 0x3800, 0x3800, input_port_0_r }, /* Player 1 */
	{ 0x3801, 0x3801, input_port_1_r }, /* Player 2 */
	{ 0x3802, 0x3802, input_port_0_r }, /* ????? Mazeh only */
/*{ 0x3803, 0x3803, input_port_2_r },*/ /* Start buttons + VBL */
	{ 0x3803, 0x3803, prot3_r },
 	{ 0x3820, 0x3820, input_port_4_r }, /* Dip */
	{ 0x3840, 0x3840, i8751_l_r },
	{ 0x3860, 0x3860, i8751_h_r },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress ghostb_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x17ff, MWA_RAM },
	{ 0x1800, 0x1fff, MWA_RAM, &videoram },
	{ 0x2000, 0x27ff, dec8_video_w },
	{ 0x2800, 0x2bff, MWA_RAM }, /* Scratch ram for rowscroll? */
	{ 0x2c00, 0x2dff, MWA_RAM, &dec8_row },
	{ 0x2e00, 0x2fff, MWA_RAM }, /* Unused */
	{ 0x3000, 0x37ff, MWA_RAM, &spriteram },
	{ 0x3800, 0x3800, dec8_sound_w },
	{ 0x3820, 0x3827, dec8_pf2_w },
	{ 0x3830, 0x3833, dec8_scroll2_w },
	{ 0x3840, 0x3840, ghostb_bank_w },
	{ 0x3860, 0x3860, i8751_l_w },
	{ 0x3861, 0x3861, i8751_h_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

/* Darwin: BPX ba38 for latest crash */
static struct MemoryReadAddress srdarwin_readmem[] =
{
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x0800, 0x13ff, MRA_RAM },
	{ 0x1400, 0x17ff, dec8_video_r },
	{ 0x2000, 0x2000, i8751_l_r },
	{ 0x2001, 0x2001, i8751_h_r },
	{ 0x3800, 0x3800, input_port_1_r }, /* dip? */
	{ 0x3801, 0x3801, input_port_0_r }, /* Player 1 */
	{ 0x3802, 0x3802, input_port_2_r }, /* */
	{ 0x3803, 0x3803, input_port_3_r }, /* Dip */
 	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress srdarwin_writemem[] =
{
	{ 0x0000, 0x05ff, MWA_RAM },
	{ 0x0600, 0x07ff, MWA_RAM, &spriteram },
	{ 0x0800, 0x0fff, MWA_RAM, &videoram },
	{ 0x1000, 0x13ff, MWA_RAM },
	{ 0x1400, 0x17ff, dec8_video_w },
	{ 0x1800, 0x1800, i8751_l_w },
	{ 0x1801, 0x1801, i8751_h_w },
	{ 0x1802, 0x180f, srdarwin_control_w },
	{ 0x2000, 0x2000, dec8_sound_w },
	{ 0x2800, 0x288f, srdarwin_palette_rg },
	{ 0x3000, 0x308f, srdarwin_palette_b },
	{ 0x3800, 0x3800, MWA_NOP },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress gondo_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_RAM },

	{ 0x2000, 0x27ff, MRA_RAM },

	{ 0x2800, 0x2bff, MRA_RAM },  /* palette */
	{ 0x2c00, 0x2fff, MRA_RAM },  /* palette */

	{ 0x3000, 0x37ff, MRA_RAM }, /* Unknown */

	{ 0x3838, 0x3838, gondo_prot2_r },
	{ 0x3839, 0x3839, gondo_prot1_r }, // gb
//	{ 0x3800, 0x3800, input_port_0_r }, /* Player 1 */
//	{ 0x3801, 0x3801, input_port_1_r }, /* Player 2 */
	{ 0x380e, 0x380e, input_port_2_r }, /* Gondomania VBL */
//	{ 0x3803, 0x3803, input_port_4_r }, /* Dip 2 */

//  { 0x3a00, 0x3a00, input_port_2_r }, /* VBL & coins */

 	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};


/*
	Gondomania Protection notes:

  Each interrupt value read from 0x3838, used as key to a lookup table
  of functions:




  3830, nmi mask??


*/

static struct MemoryWriteAddress gondo_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_RAM },
	{ 0x1000, 0x17ff, MWA_RAM }, /* Main RAM */
	{ 0x1800, 0x1fff, MWA_RAM, &videoram, &videoram_size },    /* Correct */

  	{ 0x2000, 0x27ff, MWA_RAM, &spriteram },  /* palette */
//  { 0x3000, 0x31ff, dec8_palette_w },
 //	{ 0x3000, 0x37ff, MWA_RAM }, /* Unknown */

	{ 0x2800, 0x2bff, MWA_RAM },  /* palette */
	{ 0x2c00, 0x2fff, MWA_RAM },  /* palette */

	{ 0x3830, 0x3830, gondo_bank_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress oscar_readmem[] =
{
	{ 0x0000, 0x0eff, oscar_share_r },
 	{ 0x0f00, 0x0fff, MRA_RAM },
	{ 0x1000, 0x1fff, oscar_share2_r },
	{ 0x2000, 0x27ff, MRA_RAM },

	{ 0x2800, 0x2fff, dec8_video_r },
	{ 0x3000, 0x37ff, MRA_RAM }, /* Sprites */
	{ 0x3800, 0x3bff, paletteram_r },

  	{ 0x3c00, 0x3c00, input_port_0_r },
  	{ 0x3c01, 0x3c01, input_port_1_r },
  	{ 0x3c02, 0x3c02, input_port_2_r }, /* VBL & coins */
  	{ 0x3c03, 0x3c03, input_port_3_r }, /* Dip 1 */
  	{ 0x3c04, 0x3c04, input_port_4_r },

	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress oscar_writemem[] =
{
	{ 0x0000, 0x0eff, oscar_share_w },
  	{ 0x0f00, 0x0fff, MWA_RAM },
 	{ 0x1000, 0x1fff, oscar_share2_w },
	{ 0x2000, 0x27ff, MWA_RAM, &videoram, &videoram_size },
 	{ 0x2800, 0x2fff, dec8_video_w },
	{ 0x3000, 0x37ff, MWA_RAM, &spriteram },
	{ 0x3800, 0x3bff, paletteram_xxxxBBBBGGGGRRRR_swap_w, &paletteram },
	{ 0x3c10, 0x3c13, dec8_scroll2_w },
	{ 0x3c80, 0x3c80, MWA_NOP },       /* DMA */
	{ 0x3d00, 0x3d00, dec8_bank_w },   /* BNKS */
	{ 0x3d80, 0x3d80, oscar_sound_w }, /* SOUN */
	{ 0x3e00, 0x3e00, MWA_NOP },       /* COINCL */
	{ 0x3e80, 0x3e83, oscar_int_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress oscar_sub_readmem[] =
{
	{ 0x0000, 0x0eff, oscar_share_r },
  	{ 0x0f00, 0x0fff, MRA_RAM },
  	{ 0x1000, 0x1fff, oscar_share2_r },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress oscar_sub_writemem[] =
{
	{ 0x0000, 0x0eff, oscar_share_w },
 	{ 0x0f00, 0x0fff, MWA_RAM },
  	{ 0x1000, 0x1fff, oscar_share2_w },
 	{ 0x3e80, 0x3e83, oscar_int_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress lastmiss_readmem[] =
{
	{ 0x0000, 0x0dff, oscar_share_r },
 	{ 0x0e00, 0x0fff, MRA_RAM },
	{ 0x1000, 0x13ff, srdarwin_palette_rg_r },
	{ 0x1400, 0x17ff, srdarwin_palette_b_r },
    { 0x1800, 0x1800, input_port_0_r },
	{ 0x1801, 0x1801, input_port_1_r },
	{ 0x1802, 0x1802, input_port_2_r },
    { 0x1803, 0x1803, input_port_3_r }, //????
    { 0x1804, 0x1804, input_port_4_r }, //????

{ 0x1806, 0x1806, lastmiss_prot2 },
	{ 0x1807, 0x1807, lastmiss_prot1 },

// 1807 prot byte


	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x2800, 0x2fff, MRA_RAM },
    { 0x3000, 0x37ff, MRA_RAM },
	{ 0x3800, 0x3fff, dec8_video_r },

	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress lastmiss_writemem[] =
{
	{ 0x0000, 0x0dff, oscar_share_w },
	{ 0x0e00, 0x0fff, MWA_RAM },
	{ 0x1000, 0x13ff, srdarwin_palette_rg },
	{ 0x1400, 0x17ff, srdarwin_palette_b },

   	{ 0x1800, 0x1804, lastmiss_int_w },
	{ 0x1805, 0x1805, MWA_NOP }, /* DMA */

	{ 0x1809, 0x1809, lastmiss_scrollx_w }, /* Scroll LSB */
	{ 0x180b, 0x180b, lastmiss_scrolly_w }, /* Scroll LSB */
	{ 0x180c, 0x180c, oscar_sound_w },
	{ 0x180d, 0x180d, lastmiss_control_w }, /* Bank switch + Scroll MSB */

	{ 0x2000, 0x27ff, MWA_RAM, &videoram, &videoram_size },
	{ 0x2800, 0x2fff, MWA_RAM, &spriteram },
	{ 0x3000, 0x37ff, MWA_RAM },
	{ 0x3800, 0x3fff, dec8_video_w },

	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress lastmiss_sub_readmem[] =
{
	{ 0x0000, 0x0dff, oscar_share_r },
	{ 0x0e00, 0x0fff, MRA_RAM },

//  180d bank switch  1802 vbl
	{ 0x1802, 0x1802, input_port_2_r },

    { 0x3000, 0x37ff, MRA_RAM },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress lastmiss_sub_writemem[] =
{
	{ 0x0000, 0x0dff, oscar_share_w },
	{ 0x0e00, 0x0fff, MWA_RAM },

   	{ 0x1800, 0x1804, lastmiss_int_w },


    { 0x3000, 0x37ff, MWA_RAM},
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};


int shackled_sprite_r(int offset)
{
	return spriteram[offset];
}

void shackled_sprite_w(int offset,int data)
{
	spriteram[offset]=data;
}




static struct MemoryReadAddress shackled_readmem[] =
{
	{ 0x0000, 0x0fff, oscar_share_r },
   	{ 0x1000, 0x13ff, srdarwin_palette_rg_r },
	{ 0x1400, 0x17ff, srdarwin_palette_b_r },
 	{ 0x1800, 0x1800, input_port_0_r },
	{ 0x1801, 0x1801, input_port_1_r },
	{ 0x1802, 0x1802, input_port_2_r },
  	{ 0x1803, 0x1803, input_port_3_r },   /* protection?  0x10 ??*/
  	{ 0x1804, 0x1804, input_port_4_r },   /* protection?  0x10 ??*/

//    	{ 0x2000, 0x27ff, MRA_RAM },

	{ 0x2800, 0x2fff, MRA_RAM },
	{ 0x3000, 0x37ff, shackled_share_r },
	{ 0x3800, 0x3fff, dec8_video_r },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress shackled_writemem[] =
{
	{ 0x0000, 0x0fff, oscar_share_w },
	{ 0x1000, 0x13ff, srdarwin_palette_rg },
	{ 0x1400, 0x17ff, srdarwin_palette_b },

	{ 0x1800, 0x1803, shackled_int_w },
   	{ 0x1809, 0x1809, lastmiss_scrollx_w }, /* Scroll LSB */
	{ 0x180b, 0x180b, lastmiss_scrolly_w }, /* Scroll LSB */
	{ 0x180c, 0x180c, oscar_sound_w },
	{ 0x180d, 0x180d, lastmiss_control_w }, /* Bank switch + Scroll MSB */

//	{ 0x2000, 0x27ff, MWA_RAM, &videoram, &videoram_size },
	{ 0x2800, 0x2fff, MWA_RAM, &spriteram },
	{ 0x3000, 0x37ff, shackled_share_w },
	{ 0x3800, 0x3fff, dec8_video_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

// f6c7 - bpx for end of rom tst then 821d

static int random_ret(int offset)
{

if (errorlog && cpu_getpc()!=0x4110) fprintf(errorlog,"PC %06x - prot read\n",cpu_getpc());

// unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

  	if (readinputport(0)!=0xff)
 	return 1;
    else return 0; //RAM[0x180b];
}

static int random_ret2(int offset)
{

if (errorlog && cpu_getpc()!=0x4110) fprintf(errorlog,"PC %06x - prot read\n",cpu_getpc());


   	if (readinputport(1)!=0xff)
 	return rand()%0xff;
    else return 0;
}

static struct MemoryReadAddress shackled_sub_readmem[] =
{
	{ 0x0000, 0x0fff, oscar_share_r },
//{ 0x0f00, 0x0fff, MRA_RAM },

   	{ 0x1000, 0x13ff, srdarwin_palette_rg_r },
	{ 0x1400, 0x17ff, srdarwin_palette_b_r },

   	{ 0x1800, 0x1800, input_port_0_r },    /* Unchecked */
	{ 0x1801, 0x1801, input_port_1_r },    /* Unchecked */
	{ 0x1802, 0x1802, input_port_2_r },
   	{ 0x1803, 0x1803, input_port_3_r }, //???
	{ 0x1804, 0x1804, input_port_4_r },

{ 0x1806, 0x1806, random_ret },
{ 0x1807, 0x1807, random_ret2 },

	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x2800, 0x37ff, shackled_sprite_r },
	{ 0x3000, 0x37ff, shackled_share_r },
	{ 0x3800, 0x3fff, dec8_video_r },
	{ 0x4000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress shackled_sub_writemem[] =
{
	{ 0x0000, 0x0fff, oscar_share_w },
	{ 0x1000, 0x13ff, srdarwin_palette_rg },
	{ 0x1400, 0x17ff, srdarwin_palette_b },

	{ 0x1800, 0x1803, shackled_int_w },
	{ 0x1805, 0x1805, MWA_NOP }, /* DMA */

	{ 0x1809, 0x1809, lastmiss_scrollx_w }, /* Scroll LSB */
	{ 0x180b, 0x180b, lastmiss_scrolly_w }, /* Scroll LSB */
	{ 0x180c, 0x180c, oscar_sound_w },
	{ 0x180d, 0x180d, lastmiss_control_w }, /* Bank switch + Scroll MSB */
	{ 0x180e, 0x180f, MWA_NOP },

	{ 0x2000, 0x27ff, MWA_RAM, &videoram, &videoram_size },
	{ 0x2800, 0x2fff, shackled_sprite_w },
	{ 0x3000, 0x37ff, shackled_share_w },
	{ 0x3800, 0x3fff, dec8_video_w },
	{ 0x4000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

/******************************************************************************/

/* Also used for Maze Hunter, probably others */
static struct MemoryReadAddress cobra_s_readmem[] =
{
	{ 0x0000, 0x05ff, MRA_RAM},
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress cobra_s_writemem[] =
{
 	{ 0x0000, 0x05ff, MWA_RAM},
	{ 0x2000, 0x2000, YM2203_control_port_0_w }, /* OPN */
	{ 0x2001, 0x2001, YM2203_write_port_0_w },
	{ 0x4000, 0x4000, YM3812_control_port_0_w }, /* OPL */
	{ 0x4001, 0x4001, YM3812_write_port_0_w },
 	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

/* Used by Last Mission, Shackled & Breywood */
static struct MemoryReadAddress lastmiss_s_readmem[] =
{
	{ 0x0000, 0x05ff, MRA_RAM},
	{ 0x3000, 0x3000, soundlatch_r },
	{ 0x8000, 0xffff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress lastmiss_s_writemem[] =
{
 	{ 0x0000, 0x05ff, MWA_RAM},
	{ 0x0800, 0x0800, YM2203_control_port_0_w }, /* OPN */
	{ 0x0801, 0x0801, YM2203_write_port_0_w },
	{ 0x1000, 0x1000, YM3526_control_port_0_w }, /* ? */
	{ 0x1001, 0x1001, YM3526_write_port_0_w },
 	{ 0x8000, 0xffff, MWA_ROM },
	{ -1 }  /* end of table */
};

/******************************************************************************/

INPUT_PORTS_START( input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

 	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Test mode on other games */
	PORT_DIPNAME( 0x20, 0x20, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Screen Rotation", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Reverse" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Cocktail" )
	PORT_DIPSETTING(    0x00, "Upright" )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x00, "Infinite" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x10, "Yes" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

INPUT_PORTS_START( ghostb_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

 	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
 	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
 	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
 	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_VBLANK )  /* Dummy */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* Dummy input for i8751 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START	/* Dip switch */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, "Timer", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "5.00" )
	PORT_DIPSETTING(    0x20, "6.00" )
	PORT_DIPSETTING(    0x10, "4.30" )
	PORT_DIPSETTING(    0x00, "4.00" )
	PORT_DIPNAME( 0x40, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPSETTING(    0x80, "Off" )
INPUT_PORTS_END

INPUT_PORTS_START( darwin_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_VBLANK ) /* real one? */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 )

	PORT_START	/* IN2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_BITX(0,        0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "99", IP_KEY_NONE, IP_JOY_NONE, 0)
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_COIN1 | IPF_IMPULSE, "Coin A", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_COIN2 | IPF_IMPULSE, "Coin B", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, "Coin C", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 2)

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( oscar_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

 	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x00, "2 Coins/1 Credit" )
	PORT_DIPNAME( 0x20, 0x20, "Demo Freeze Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "No" )
	PORT_DIPSETTING(    0x00, "Yes" )
	PORT_DIPNAME( 0x20, 0x20, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Screen Rotation", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Reverse" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Cocktail" )
	PORT_DIPSETTING(    0x00, "Upright" )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPSETTING(    0x00, "Infinite" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "Every 40000" )
	PORT_DIPSETTING(    0x20, "Every 60000" )
	PORT_DIPSETTING(    0x10, "Every 90000" )
	PORT_DIPSETTING(    0x00, "50000 only" )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x80, "Yes" )
INPUT_PORTS_END



INPUT_PORTS_START( lastmiss_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* USED??? dont think so - mask is 0x3f*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

 	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 ) /* test */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Test mode on other games */
	PORT_DIPNAME( 0x20, 0x20, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Screen Rotation", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Reverse" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Cocktail" )
	PORT_DIPSETTING(    0x00, "Upright" )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x00, "Infinite" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x10, "Yes" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



INPUT_PORTS_START( shackled_input_ports )
	PORT_START	/* Player 1 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )  /* USED??? dont think so - mask is 0x3f*/
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

 	PORT_START	/* Player 2 controls */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 ) /* test */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* Dip switch bank 1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x01, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin/2 Credits" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x04, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x08, "1 Coin/2 Credits" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Test mode on other games */
	PORT_DIPNAME( 0x20, 0x20, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Screen Rotation", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x00, "Reverse" )
	PORT_DIPNAME( 0x80, 0x80, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Cocktail" )
	PORT_DIPSETTING(    0x00, "Upright" )

	PORT_START	/* Dip switch bank 2 */
	PORT_DIPNAME( 0x03, 0x03, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x00, "Infinite" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x0c, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x10, 0x10, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x10, "Yes" )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout_32k =
{
	8,8,
	1024,
	2,
	{ 0x4000*8,0x0000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

static struct GfxLayout charlayout_3bpp =
{
	8,8,
	1024,
	3,
	{ 0x6000*8,0x4000*8,0x2000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

static struct GfxLayout charlayout_last =
{
	8,8,
	1024,
	3,
	{ 0x6000*8,0x2000*8,0x4000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

/* SRDarwin characters - very unusual layout for Data East */
static struct GfxLayout charlayout_16k =
{
	8,8,	/* 8*8 characters */
	1024,
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 0x2000*8+0, 0x2000*8+1, 0x2000*8+2, 0x2000*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout gondo_charlayout =
{
	8,8,
	1024,
	3,
	{ 0x6000*8,0x4000*8,0x2000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

static struct GfxLayout oscar_charlayout =
{
	8,8,
	1024,
	3,
	{ 0x3000*8,0x2000*8,0x1000*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every sprite takes 8 consecutive bytes */
};

/* Darwin sprites - only 3bpp */
static struct GfxLayout sr_tiles =
{
	16,16,
	2048,
	3,
 	{ 0x10000*8,0x20000*8,0x00000*8 },
	{ 16*8, 1+(16*8), 2+(16*8), 3+(16*8), 4+(16*8), 5+(16*8), 6+(16*8), 7+(16*8),
		0,1,2,3,4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 ,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8},
	16*16
};

static struct GfxLayout tiles =
{
	16,16,
	2048,
	4,
 	{ 0x30000*8,0x20000*8,0x10000*8,0x00000*8 },
	{ 16*8, 1+(16*8), 2+(16*8), 3+(16*8), 4+(16*8), 5+(16*8), 6+(16*8), 7+(16*8),
		0,1,2,3,4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 ,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8},
	16*16
};

static struct GfxLayout tilesl =
{
	16,16,
	4096,
	4,
 	{ 0x60000*8,0x40000*8,0x20000*8,0x00000*8 },
	{ 16*8, 1+(16*8), 2+(16*8), 3+(16*8), 4+(16*8), 5+(16*8), 6+(16*8), 7+(16*8),
		0,1,2,3,4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 ,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8},
	16*16
};

/* X flipped on Ghostbusters tiles */
static struct GfxLayout tiles_t =
{
	16,16,
	2048,
	4,
 	{ 0x20000*8,0x00000*8,0x30000*8,0x10000*8 },
	{ 7,6,5,4,3,2,1,0,
		7+(16*8), 6+(16*8), 5+(16*8), 4+(16*8), 3+(16*8), 2+(16*8), 1+(16*8), 0+(16*8) },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 ,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8},
	16*16
};

static struct GfxLayout tiles_4096 =
{
	16,16,
	4096,
	4,
 	{ 0x60000*8,0x00000*8,0x20000*8,0x40000*8 },
	{ 16*8, 1+(16*8), 2+(16*8), 3+(16*8), 4+(16*8), 5+(16*8), 6+(16*8), 7+(16*8),
		0,1,2,3,4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 ,8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8},
	16*16
};

static struct GfxLayout tiles2 =
{
	16,16,
	256,
	4,
	{ 0x8000*8, 0x8000*8+4, 0, 4 },
	{ 0, 1, 2, 3, 1024*8*8+0, 1024*8*8+1, 1024*8*8+2, 1024*8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+1024*8*8+0, 16*8+1024*8*8+1, 16*8+1024*8*8+2, 16*8+1024*8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every tile takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout_32k,     0, 2*64  },
	{ 1, 0x08000, &tiles,  0, 2*64 },
	{ 1, 0x48000, &tiles,  0, 2*64 },
	{ 1, 0x88000, &tiles,  0, 2*64 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo ghostb_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout_3bpp,	0,   4  },
	{ 1, 0x08000, &tiles_4096,		256, 16 },
	{ 1, 0x88000, &tiles_t,			512, 16 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo srdarwin_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout_16k,     128, 4  },
	{ 1, 0x08000, &sr_tiles,  64, 8 },
	{ 1, 0x38000, &tiles2,  0, 4 },
  	{ 1, 0x48000, &tiles2,  0, 4 },
    { 1, 0x58000, &tiles2,  0, 4 },
    { 1, 0x68000, &tiles2,  0, 4 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gondo_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &gondo_charlayout,     0, 2*64  },
	{ 1, 0x08000, &tiles,  0, 2*64 },
	{ 1, 0x48000, &tiles,  0, 2*64 },
	{ 1, 0x88000, &tiles,  0, 2*64 },
	{ 1, 0xc8000, &tiles,  0, 2*64 },
 	{ -1 } /* end of array */
};

static struct GfxDecodeInfo oscar_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &oscar_charlayout, 256, 8  }, /* Chars */
	{ 1, 0x08000, &tiles,  0, 16 }, /* Sprites */
	{ 1, 0x48000, &tiles,  384, 8 }, /* Tiles */
 	{ -1 } /* end of array */
};

static struct GfxDecodeInfo lastmiss_gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout_last,     0, 8 },  /* COULD BE 4?? oscar too */
	{ 1, 0x08000, &tilesl, 256, 16 },
	{ 1, 0x88000, &tiles,  768, 16 },
 	{ -1 } /* end of array */
};
/******************************************************************************/

static struct YM2203interface ym2203_interface =
{
	1,
	1500000,	/* Unknown */
	{ YM2203_VOL(140,0x20ff) },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct YM3526interface ym3526_interface =
{
	1,			/* 1 chip (no more supported) */
	3000000,	/* 3 MHz ? (not supported) */
	{ 255 }		/* (not supported) */
};

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip (no more supported) */
	3000000,	/* 3 MHz ? (not supported) */
	{ 255 }		/* (not supported) */
};

/******************************************************************************/

static int ghost_interrupt(void)
{
	static int a=0;

 	if (a) {a=0; return M6809_INT_NMI;}
 	a=1;
	return M6809_INT_IRQ;
}

/* IRQ on coin insert only */
static int mazeh_interrupt(void)
{
	static int a=0,latch;

 	if (a) {a=0; return M6809_INT_NMI;}
 	a=1;

	if ((readinputport(3) & 0x7) == 0x7) latch=1;
	if (!coin_flag && latch && (readinputport(3) & 0x7) != 0x7) {
		coin_flag=1;
		latch=0;
		return M6809_INT_IRQ;
   }
	return 0;
}

static int gondo_interrupt(void)
{
	static int a=0;

 	if (a) {a=0; return M6809_INT_IRQ;}
 	a=1;

	if ((readinputport(0) & 0x1) != 0x1) return M6809_INT_NMI;
	return 0;
}

static int lastmiss_interrupt(void)
{
	static int latch=1;

	if ((readinputport(2) & 0x3) == 0x3) latch=1;
	if (latch && (readinputport(2) & 0x3) != 0x3) {
//    	cpu_cause_interrupt (0, M6809_INT_FIRQ);
    }

	return 0;
}

/* Coins generate NMI's */
static int oscar_interrupt(void)
{
	static int latch=1;

	if ((readinputport(2) & 0x7) == 0x7) latch=1;
	if (latch && (readinputport(2) & 0x7) != 0x7) {
		latch=0;
    	cpu_cause_interrupt (0, M6809_INT_NMI);
    }

	return 0;
}

/******************************************************************************/

static struct MachineDriver cobra_machine_driver =
{
	/* basic machine hardware */
	{
 		{
			CPU_M6809,
			1250000,
			0,
			cobra_readmem,cobra_writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ? */
			2,	/* memory region #2 */
			cobra_s_readmem,cobra_s_writemem,0,0,
			interrupt,8     /* Set by hand. */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,	/* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
  //64*8, 64*8, { 0*8, 64*8-1, 1*8, 64*8-1 },

	gfxdecodeinfo,
	256,2*64*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec8_vh_start,
	dec8_vh_stop,
	dec8_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		}
	}
};

static struct MachineDriver ghostb_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,  /* Really HD6309 */
			3000000,  /* Could be 4? */
			0,
			ghostb_readmem,ghostb_writemem,0,0,
			ghost_interrupt,2
		}
#if 0
,

		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ? */
			2,	/* memory region #2 */
			cobra_s_readmem,cobra_s_writemem,0,0,
			interrupt,8     /* Set by hand. */
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,	/* init machine */

	/* video hardware */
  	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	ghostb_gfxdecodeinfo,
	1024,1024,
	ghostb_vh_convert_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec8_vh_start,
	dec8_vh_stop,
	ghostb_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		}
	}
};

static struct MachineDriver mazeh_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,  /* Really HD6309 */
			3000000,
			0,
			ghostb_readmem,ghostb_writemem,0,0,
			mazeh_interrupt,2
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ? */
			2,	/* memory region #2 */
  			cobra_s_readmem,cobra_s_writemem,0,0,
			interrupt,8     /* Set by hand. */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,	/* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	ghostb_gfxdecodeinfo,
	1024,1024,
	ghostb_vh_convert_color_prom,

	VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec8_vh_start,
	dec8_vh_stop,
	ghostb_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		}
	}
};

static struct MachineDriver srdarwin_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,  /* MC68A09EP */
			2000000,
			0,
			srdarwin_readmem,srdarwin_writemem,0,0,
			nmi_interrupt,1
		}
#if 0
,
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ? */
			2,	/* memory region #2 */
			cobra_s_readmem,cobra_s_writemem,0,0,
			interrupt,8     /* Set by hand. */
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,	/* init machine */

	/* video hardware */
  	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	srdarwin_gfxdecodeinfo,
	144,144,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec8_vh_start,
	dec8_vh_stop,
	srdarwin_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3812,
			&ym3812_interface
		}
	}
};

static struct MachineDriver gondo_machine_driver =
{
	/* basic machine hardware */
	{
 		{
			CPU_M6809,
			1250000,
			0,
			gondo_readmem,gondo_writemem,0,0,
		   //	ghost_interrupt,2
           gondo_interrupt,0
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ? */
			2,	/* memory region #2 */
			cobra_s_readmem,cobra_s_writemem,0,0,
			interrupt,8     /* Set by hand. */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,	/* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
  //64*8, 64*8, { 0*8, 64*8-1, 1*8, 64*8-1 },

	gondo_gfxdecodeinfo,
	256,2*64*16,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	dec8_vh_start,
	dec8_vh_stop,
	dec8_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3526,
			&ym3526_interface
		}
	}
};

static struct MachineDriver oscar_machine_driver =
{
	/* basic machine hardware */
	{
	  	{
			CPU_M6809,
			2000000,
			0,
			oscar_readmem,oscar_writemem,0,0,
			oscar_interrupt,1
		},
	 	{
			CPU_M6809,
			2000000,
			3,
			oscar_sub_readmem,oscar_sub_writemem,0,0,
			ignore_interrupt,0
		}
#if 0
,
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ? */
			2,	/* memory region #2 */
			cobra_s_readmem,cobra_s_writemem,0,0,
			interrupt,8     /* Set by hand. */
		}
#endif
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,	/* init machine */

	/* video hardware */
  	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },

	oscar_gfxdecodeinfo,
	512,512,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec8_vh_start,
	dec8_vh_stop,
	oscar_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3526,
			&ym3526_interface
		}
	}
};

static struct MachineDriver lastmiss_machine_driver =
{
	/* basic machine hardware */
	{
  		{
			CPU_M6809,
			2000000,
			0,
			lastmiss_readmem,lastmiss_writemem,0,0,
			lastmiss_interrupt,1
		},
     	{
			CPU_M6809,
			2000000,
			3,
			lastmiss_sub_readmem,lastmiss_sub_writemem,0,0,
			ignore_interrupt,0
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ? */
			2,	/* memory region #2 */
			lastmiss_s_readmem,lastmiss_s_writemem,0,0,
			interrupt,8     /* Set by hand. */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,	/* init machine */

	/* video hardware */
  	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
  //64*8, 64*8, { 0*8, 64*8-1, 1*8, 64*8-1 },

	lastmiss_gfxdecodeinfo,
	1024,1024,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec8_vh_start,
	dec8_vh_stop,
	lastmiss_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3526,
			&ym3526_interface
		}
	}
};

static struct MachineDriver shackled_machine_driver =
{
	/* basic machine hardware */
	{
  		{
			CPU_M6809,
			2000000,
			0,
			shackled_readmem,shackled_writemem,0,0,
		   //	ignore_interrupt,0
			interrupt,1
		},
     	{
			CPU_M6809,
			2000000,
			3,
			shackled_sub_readmem,shackled_sub_writemem,0,0,
     //	oscar_readmem,oscar_writemem,0,0,


			ignore_interrupt,0
            //linterrupt,1
		},
		{
			CPU_M6502 | CPU_AUDIO_CPU,
			1250000,        /* 1.25 Mhz ? */
			2,	/* memory region #2 */
			lastmiss_s_readmem,lastmiss_s_writemem,0,0,
			interrupt,8     /* Set by hand. */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,
	0,	/* init machine */

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 1*8, 31*8-1 },
  //64*8, 64*8, { 0*8, 64*8-1, 1*8, 64*8-1 },

	lastmiss_gfxdecodeinfo,
	1024,1024,
	0,

 /* PC 0082c2 - Bank switch 07 (87)
CPU #0 PC 8449: warning - write 10 to ROM address 40dc
CPU #0 PC 8108: warning - read 00 from unmapped memory address 394c
CPU #0 PC 8108: warning - read 00 from unmapped memory address 394d
C

*/


	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE | VIDEO_UPDATE_BEFORE_VBLANK,
	0,
	dec8_vh_start,
	dec8_vh_stop,
	lastmiss_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM2203,
			&ym2203_interface
		},
		{
			SOUND_YM3526,
			&ym3526_interface
		}
	}
};

/******************************************************************************/

ROM_START( cobracom_rom )
	ROM_REGION(0x30000)
 	ROM_LOAD( "eh-11.rom",    0x08000, 0x08000, 0x868637e1 )
 	ROM_LOAD( "eh-12.rom",    0x10000, 0x10000, 0x7c878a83 )
 	ROM_LOAD( "eh-13.rom",    0x20000, 0x10000, 0x04505acb )

	ROM_REGION_DISPOSE(0xc0000)	/* temporary space for graphics */
	ROM_LOAD( "eh-14.rom",    0x00000, 0x8000, 0x47246177 )	/* Characters */
	ROM_LOAD( "eh-00.rom",    0x08000, 0x10000, 0xd96b6797 ) /* Sprites */
	ROM_LOAD( "eh-01.rom",    0x18000, 0x10000, 0x3fef9c02 )
	ROM_LOAD( "eh-02.rom",    0x28000, 0x10000, 0xbfae6c34 )
	ROM_LOAD( "eh-03.rom",    0x38000, 0x10000, 0xd56790f8 )
	ROM_LOAD( "eh-05.rom",    0x48000, 0x10000, 0x1c4f6033 ) /* Tiles */
	ROM_LOAD( "eh-06.rom",    0x58000, 0x10000, 0xd24ba794 )
	ROM_LOAD( "eh-04.rom",    0x68000, 0x10000, 0xd80a49ce )
	ROM_LOAD( "eh-07.rom",    0x78000, 0x10000, 0x6d771fc3 )
	ROM_LOAD( "eh-08.rom",    0x88000, 0x08000, 0xcb0dcf4c ) /* Tiles 2 */
	ROM_CONTINUE(0x98000,0x8000)
	ROM_LOAD( "eh-09.rom",    0xa8000, 0x08000, 0x1fae5be7 )
	ROM_CONTINUE(0xb8000,0x8000)

	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "eh-10.rom",    0x8000,  0x8000,  0x62ca5e89 )
ROM_END

ROM_START( ghostb_rom )
	ROM_REGION(0x50000)
 	ROM_LOAD( "dz-01.rom", 0x08000, 0x08000, 0x7c5bb4b1 )
 	ROM_LOAD( "dz-02.rom", 0x10000, 0x10000, 0x8e117541 )
	ROM_LOAD( "dz-03.rom", 0x20000, 0x10000, 0x5606a8f4 )
	ROM_LOAD( "dz-04.rom", 0x30000, 0x10000, 0xd09bad99 )
	ROM_LOAD( "dz-05.rom", 0x40000, 0x10000, 0x0315f691 )

	ROM_REGION_DISPOSE(0xc8000)	/* temporary space for graphics */
	ROM_LOAD( "dz-00.rom", 0x00000, 0x08000, 0x992b4f31 )  /* Characters */

	ROM_LOAD( "dz-11.rom", 0x08000, 0x10000, 0xa5e19c24 ) /* Sprites - two banks interleaved */
	ROM_LOAD( "dz-13.rom", 0x18000, 0x10000, 0x3e7c0405 )
	ROM_LOAD( "dz-12.rom", 0x28000, 0x10000, 0x817fae99 )
	ROM_LOAD( "dz-14.rom", 0x38000, 0x10000, 0x0abbf76d )
	ROM_LOAD( "dz-15.rom", 0x48000, 0x10000, 0xa01a5fd9 )
	ROM_LOAD( "dz-16.rom", 0x58000, 0x10000, 0x5a9a344a )
	ROM_LOAD( "dz-17.rom", 0x68000, 0x10000, 0x40361b8b )
	ROM_LOAD( "dz-18.rom", 0x78000, 0x10000, 0x8d219489 )

	ROM_LOAD( "dz-07.rom", 0x88000, 0x10000, 0xe7455167 ) /* Tiles */
 	ROM_LOAD( "dz-08.rom", 0x98000, 0x10000, 0x32f9ddfe )
 	ROM_LOAD( "dz-09.rom", 0xa8000, 0x10000, 0xbb6efc02 )
	ROM_LOAD( "dz-10.rom", 0xb8000, 0x10000, 0x6ef9963b )

	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "dz-06.rom", 0x8000, 0x8000, 0x798f56df )

	ROM_REGION_DISPOSE(0x800)	/* Colour proms */
	ROM_LOAD( "dz19a.10d", 0x0000, 0x0400, 0x47e1f83b )
	ROM_LOAD( "dz20a.11d", 0x0400, 0x0400, 0xd8fe2d99 )
ROM_END

ROM_START( mazeh_rom )
	ROM_REGION(0x40000)
 	ROM_LOAD( "dw-01.rom", 0x08000, 0x08000, 0x87610c39 )
 	ROM_LOAD( "dw-02.rom", 0x10000, 0x10000, 0x40c9b0b8 )
 	ROM_LOAD( "dw-03.rom", 0x20000, 0x10000, 0x5606a8f4 )
 	ROM_LOAD( "dw-04.rom", 0x30000, 0x10000, 0x235c0c36 )

	ROM_REGION_DISPOSE(0xc8000)	/* temporary space for graphics */
	ROM_LOAD( "dw-00.rom", 0x00000, 0x8000, 0x3d25f15c ) /* Characters */

	ROM_LOAD( "dw-10.rom", 0x08000, 0x10000, 0x57667546 ) /* 2 sets of sprites, interleaved */
	ROM_LOAD( "dw-12.rom", 0x18000, 0x10000, 0x4c548db8 )
	ROM_LOAD( "dw-11.rom", 0x28000, 0x10000, 0x1b1fcca7 )
	ROM_LOAD( "dw-13.rom", 0x38000, 0x10000, 0xe7413056 )
	ROM_LOAD( "dw-14.rom", 0x48000, 0x10000, 0x9b0dbfa9 )
	ROM_LOAD( "dw-15.rom", 0x58000, 0x10000, 0x95683fda )
	ROM_LOAD( "dw-16.rom", 0x68000, 0x10000, 0xe5bcf927 )
	ROM_LOAD( "dw-17.rom", 0x78000, 0x10000, 0x9e10f723 )

	ROM_LOAD( "dw-06.rom", 0x88000, 0x10000, 0xb65e029d ) /* Tiles */
	ROM_LOAD( "dw-07.rom", 0x98000, 0x10000, 0x668d995d )
	ROM_LOAD( "dw-08.rom", 0xa8000, 0x10000, 0xbb2cf4a0 )
	ROM_LOAD( "dw-09.rom", 0xb8000, 0x10000, 0x6a528d13 )

	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "dw-05.rom", 0x8000, 0x8000, 0xc28c4d82 )

	ROM_REGION_DISPOSE(0x800)	/* Colour proms */
	ROM_LOAD( "dz19a.10d", 0x0000, 0x0400, 0x00000000 ) /* Not the real ones! */
	ROM_LOAD( "dz20a.11d", 0x0400, 0x0400, 0x00000000 ) /* These are from ghostbusters */
ROM_END

ROM_START( srdarwin_rom )
	ROM_REGION(0x28000)
 	ROM_LOAD( "dy_01.rom", 0x20000, 0x08000, 0x1eeee4ff )
	ROM_CONTINUE(	0x8000, 0x8000 )
 	ROM_LOAD( "dy_00.rom", 0x10000, 0x10000, 0x2bf6b461 )

	ROM_REGION_DISPOSE(0xd0000)	/* temporary space for graphics */
	ROM_LOAD( "dy_05.rom", 0x00000, 0x4000, 0x8780e8a3 ) /* Characters */

	ROM_LOAD( "dy_06.rom", 0x10000, 0x8000, 0xc279541b ) /* Sprites */
	ROM_LOAD( "dy_07.rom", 0x08000, 0x8000, 0x97eaba60 )
	ROM_LOAD( "dy_08.rom", 0x20000, 0x8000, 0x71d645fd )
	ROM_LOAD( "dy_09.rom", 0x18000, 0x8000, 0xd30d1745 )
	ROM_LOAD( "dy_10.rom", 0x30000, 0x8000, 0x88770ab8 )
	ROM_LOAD( "dy_11.rom", 0x28000, 0x8000, 0xfd9ccc5b )

	ROM_LOAD( "dy_03.rom", 0x38000, 0x4000, 0x44f2a4f9 )
	ROM_CONTINUE(0x48000,0x4000)
	ROM_CONTINUE(0x58000,0x4000)
	ROM_CONTINUE(0x68000,0x4000)

	ROM_LOAD( "dy_02.rom", 0x40000, 0x4000, 0x522d9a9e )
	ROM_CONTINUE(0x50000,0x4000)
	ROM_CONTINUE(0x60000,0x4000)
	ROM_CONTINUE(0x70000,0x4000)

	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "dy_04.rom", 0x8000, 0x8000, 0x2ae3591c )
ROM_END

ROM_START( gondo_rom )
	ROM_REGION(0x40000)
 	ROM_LOAD( "dt-00.256", 0x08000, 0x08000, 0x0 )
 	ROM_LOAD( "dt-03.512", 0x10000, 0x10000, 0x0 )
	ROM_LOAD( "dt-02.512", 0x20000, 0x10000, 0x0 )
	ROM_LOAD( "dt-01.512", 0x30000, 0x10000, 0x0 )

	ROM_REGION_DISPOSE(0x100000)	/* temporary space for graphics */
	ROM_LOAD( "dt-14.256", 0x00000, 0x08000, 0x0 )
	ROM_LOAD( "dt-15.512", 0x08000, 0x10000, 0x0 )
	ROM_LOAD( "dt-16.512", 0x18000, 0x10000, 0x0 )
	ROM_LOAD( "dt-19.512", 0x28000, 0x10000, 0x0 )
	ROM_LOAD( "dt-21.512", 0x38000, 0x10000,  0x0 )
	ROM_LOAD( "dt-17.256", 0x48000, 0x8000,  0x0 )
	ROM_LOAD( "dt-18.256", 0x58000, 0x8000,  0x0 )
	ROM_LOAD( "dt-20.256", 0x68000, 0x8000,  0x0 )
	ROM_LOAD( "dt-22.256", 0x78000, 0x8000, 0x0 )
	ROM_LOAD( "dt-06.512", 0x88000, 0x10000, 0x0 )
	ROM_LOAD( "dt-08.512", 0x98000, 0x10000, 0x0 )
	ROM_LOAD( "dt-10.512", 0xa8000, 0x10000, 0x0 )
	ROM_LOAD( "dt-12.512", 0xb8000, 0x10000, 0x0 )
	ROM_LOAD( "dt-09.256", 0xc8000, 0x8000, 0x0 )
	ROM_LOAD( "dt-11.256", 0xd8000, 0x8000, 0x0 )
	ROM_LOAD( "dt-13.256", 0xe8000, 0x8000, 0x0 )
	ROM_LOAD( "dt-07.256", 0xf8000, 0x8000,  0x0 )

	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "dt-05.256", 0x8000, 0x8000, 0x0 )
ROM_END

ROM_START( oscar_rom )
	ROM_REGION(0x20000)
 	ROM_LOAD( "ed10", 0x08000, 0x08000, 0xf9b0d4d4 )
 	ROM_LOAD( "ed09", 0x10000, 0x10000, 0xe2d4bba9 )

	ROM_REGION_DISPOSE(0xc8000)	/* temporary space for graphics */
	ROM_LOAD( "ed08", 0x00000, 0x04000, 0x308ac264 )	/* characters */
	ROM_LOAD( "ed04", 0x08000, 0x10000, 0x416a791b )
	ROM_LOAD( "ed05", 0x18000, 0x10000, 0xfcdba431 )
	ROM_LOAD( "ed06", 0x28000, 0x10000, 0x7d50bebc )
	ROM_LOAD( "ed07", 0x38000, 0x10000, 0x8fdf0fa5 )

	ROM_LOAD( "ed00", 0x68000, 0x10000, 0xac201f2d )
	ROM_LOAD( "ed01", 0x48000, 0x10000, 0xd3a58e9e )
	ROM_LOAD( "ed02", 0x78000, 0x10000, 0x7ddc5651 )
	ROM_LOAD( "ed03", 0x58000, 0x10000, 0x4fc4fb0f )

	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "ed12", 0x8000, 0x8000,  0x432031c5 )

	ROM_REGION(0x10000)	/* CPU 2, 1st 16k is empty */
	ROM_LOAD( "ed11", 0x0000, 0x10000,  0x10e5d919 )
ROM_END

ROM_START( oscarj_rom )
	ROM_REGION(0x20000)
 	ROM_LOAD( "du10", 0x08000, 0x08000, 0x120040d8 )
 	ROM_LOAD( "ed09", 0x10000, 0x10000, 0xe2d4bba9 )

	ROM_REGION_DISPOSE(0x88000)	/* temporary space for graphics */
	ROM_LOAD( "ed08", 0x00000, 0x04000, 0x308ac264 )	/* characters */
	ROM_LOAD( "ed04", 0x08000, 0x10000, 0x416a791b )
	ROM_LOAD( "ed05", 0x18000, 0x10000, 0xfcdba431 )
	ROM_LOAD( "ed06", 0x28000, 0x10000, 0x7d50bebc )
	ROM_LOAD( "ed07", 0x38000, 0x10000, 0x8fdf0fa5 )

	ROM_LOAD( "ed00", 0x68000, 0x10000, 0xac201f2d )
	ROM_LOAD( "ed01", 0x48000, 0x10000, 0xd3a58e9e )
	ROM_LOAD( "ed02", 0x78000, 0x10000, 0x7ddc5651 )
	ROM_LOAD( "ed03", 0x58000, 0x10000, 0x4fc4fb0f )

	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "ed12", 0x8000, 0x8000, 0x432031c5 )

	ROM_REGION(0x10000)	/* CPU 2, 1st 16k is empty */
	ROM_LOAD( "du11", 0x0000, 0x10000, 0xff45c440 )
ROM_END

ROM_START( lastmiss_rom )
	ROM_REGION(0x20000)
 	ROM_LOAD( "lm_dl03.rom", 0x08000, 0x08000, 0x0 )
 	ROM_LOAD( "lm_dl04.rom", 0x10000, 0x10000, 0x0 )

	ROM_REGION_DISPOSE(0xc8000)	/* temporary space for graphics */
	ROM_LOAD( "lm_dl01.rom", 0x00000, 0x8000, 0x0 )	/* characters */
	ROM_LOAD( "lm_dl13.rom", 0x08000, 0x8000, 0x0 )
	ROM_LOAD( "lm_dl12.rom", 0x28000, 0x8000, 0x0 )
	ROM_LOAD( "lm_dl11.rom", 0x48000, 0x8000, 0x0 )
	ROM_LOAD( "lm_dl10.rom", 0x68000, 0x8000, 0x0 )

	ROM_LOAD( "lm_dl09.rom", 0x88000, 0x10000, 0x0 )
	ROM_LOAD( "lm_dl08.rom", 0x98000, 0x10000, 0x0 )
	ROM_LOAD( "lm_dl07.rom", 0xa8000, 0x10000, 0x0 )
	ROM_LOAD( "lm_dl06.rom", 0xb8000, 0x10000, 0x0 )

	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "lm_dl05.rom", 0x8000, 0x8000, 0x0 )

	ROM_REGION(0x10000)	/* CPU 2, 1st 16k is empty */
	ROM_LOAD( "lm_dl02.rom", 0x0000, 0x10000, 0x0 )
ROM_END

ROM_START( shackled_rom )
	ROM_REGION(0x48000)
 	ROM_LOAD( "dk-02.rom", 0x08000, 0x08000, 0x0 )
	ROM_LOAD( "dk-06.rom", 0x10000, 0x10000, 0x0 )
	ROM_LOAD( "dk-05.rom", 0x20000, 0x10000, 0x0 )
	ROM_LOAD( "dk-04.rom", 0x30000, 0x10000, 0x0 )
    ROM_LOAD( "dk-03.rom", 0x40000, 0x08000, 0x0 )

	ROM_REGION_DISPOSE(0xc8000)	/* temporary space for graphics */
	ROM_LOAD( "dk-00.rom", 0x00000, 0x8000, 0x0 )	/* characters */

	ROM_LOAD( "dk-12.rom", 0x08000, 0x10000, 0x0 )
	ROM_LOAD( "dk-13.rom", 0x18000, 0x10000, 0x0 )
	ROM_LOAD( "dk-14.rom", 0x28000, 0x10000,  0x0 )
	ROM_LOAD( "dk-15.rom", 0x38000, 0x10000, 0x0 )
	ROM_LOAD( "dk-16.rom", 0x48000, 0x10000, 0x0 )
	ROM_LOAD( "dk-17.rom", 0x58000, 0x10000, 0x0 )
	ROM_LOAD( "dk-18.rom", 0x68000, 0x10000, 0x0 )
	ROM_LOAD( "dk-19.rom", 0x78000, 0x10000, 0x0 )

	ROM_LOAD( "dk-08.rom", 0x88000, 0x10000, 0x0 )
	ROM_LOAD( "dk-09.rom", 0x98000, 0x10000, 0x0 )
	ROM_LOAD( "dk-10.rom", 0xa8000, 0x10000, 0x0 )
	ROM_LOAD( "dk-11.rom", 0xb8000, 0x10000, 0x0 )

	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "dk-07.rom", 0x8000, 0x8000,  0x0 )

	ROM_REGION(0x10000)	/* CPU 2, 1st 16k is empty */
	ROM_LOAD( "dk-01.rom", 0x0000, 0x10000,  0x0 )
ROM_END

ROM_START( breywood_rom )
	ROM_REGION(0x48000)
 	ROM_LOAD( "7.bin", 0x08000, 0x08000, 0x0 )
   	ROM_LOAD( "3.bin", 0x10000, 0x10000, 0x0 )
	ROM_LOAD( "4.bin", 0x20000, 0x10000, 0x0 )
	ROM_LOAD( "5.bin", 0x30000, 0x10000, 0x0 )
    ROM_LOAD( "6.bin", 0x40000, 0x08000, 0x0 )

	ROM_REGION_DISPOSE(0xc8000)	/* temporary space for graphics */
	ROM_LOAD( "1.bin",  0x00000, 0x8000, 0x0 )	/* characters */

	ROM_LOAD( "dk-12.rom", 0x08000, 0x10000, 0x0 )
	ROM_LOAD( "dk-13.rom", 0x18000, 0x10000, 0x0 )
	ROM_LOAD( "dk-14.rom", 0x28000, 0x10000,  0x0 )
	ROM_LOAD( "dk-15.rom", 0x38000, 0x10000, 0x0 )
	ROM_LOAD( "dk-16.rom", 0x48000, 0x10000, 0x0 )
	ROM_LOAD( "dk-17.rom", 0x58000, 0x10000, 0x0 )
	ROM_LOAD( "dk-18.rom", 0x68000, 0x10000, 0x0 )
	ROM_LOAD( "dk-19.rom", 0x78000, 0x10000, 0x0 )



	ROM_LOAD( "9.bin",  0x88000, 0x10000, 0x0 )
	ROM_LOAD( "10.bin", 0x98000, 0x10000, 0x0 )
	ROM_LOAD( "11.bin", 0xa8000, 0x10000, 0x0 )
	ROM_LOAD( "12.bin", 0xb8000, 0x10000, 0x0 )
 /*
	ROM_LOAD( "dk-13.rom", 0x48000, 0x10000, 0x434ccb9e, 0x0 )
	ROM_LOAD( "dk-15.rom", 0x58000, 0x10000, 0x434ccb9e, 0x0 )
	ROM_LOAD( "dk-17.rom", 0x68000, 0x10000, 0x434ccb9e, 0x0 )
	ROM_LOAD( "dk-19.rom", 0x78000, 0x10000, 0x434ccb9e, 0x0 )

	ROM_LOAD( "dk-08.rom", 0x88000, 0x10000, 0x434ccb9e, 0x0 )
	ROM_LOAD( "dk-09.rom", 0x98000, 0x10000, 0x434ccb9e, 0x0 )
	ROM_LOAD( "dk-10.rom", 0xa8000, 0x10000, 0x434ccb9e, 0x0 )
	ROM_LOAD( "dk-11.rom", 0xb8000, 0x10000, 0x434ccb9e, 0x0 )
*/
	ROM_REGION(0x10000)	/* 64K for sound CPU */
	ROM_LOAD( "2.bin", 0x8000, 0x8000,  0x0 )

	ROM_REGION(0x10000)	/* CPU 2, 1st 16k is empty */
	ROM_LOAD( "8.bin", 0x0000, 0x10000,  0x0 )
ROM_END

/******************************************************************************/

static void mazeh_patch(void)
{
	/* Blank out garbage in colour prom to avoid colour overflow */
	unsigned char *RAM = Machine->memory_region[3];
	memset(RAM+0x20,0,0xe0);
}

static void ghostb_patch(void)
{
	/* Decrypt sound CPU - NOT YET CORRECT! */
	unsigned char *RAM = Machine->memory_region[2];
	int i;

	for (i=0x8000; i<0x9000; i++) {
 		int swap1=RAM[i]&0x20;
		int swap2=RAM[i]&0x40;

		/* Mask to 0x9f, misses bits 5 & 6 */
			RAM[i]=(RAM[i]&0x9f);
			if (swap1) RAM[i]+=0x40;
			if (swap2) RAM[i]+=0x20;
	}

   RAM[0x828f]=0xcf;

#if 0
{
    FILE *fp;

    fp=fopen("dump.rom","wb");
    fwrite(RAM+0x8000,1,0x8000,fp);
    fclose(fp);
}

{
    FILE *fp;

    fp=fopen("dw-05.rom","rb");
    fread(RAM+0x8000,1,0x1000,fp);
    fclose(fp);
}
#endif

	/* A few patches */
	RAM = Machine->memory_region[0];
	RAM[0x83d8]=0x12;
	RAM[0x83d9]=0x12;
	memset(RAM+0x83c8,0x12,0x11);

	/* Blank out garbage in colour prom to avoid colour overflow */
	RAM = Machine->memory_region[3];
	memset(RAM+0x20,0,0xe0);
}

static void srdarwin_patch(void)
{
	/* Decrypt sound CPU - NOT YET WORKING! */
	unsigned char *RAM = Machine->memory_region[2];
	int i;

    for (i=0x8000; i<0xf000; i++) {
		int swap1=RAM[i]&0x20;
		int swap2=RAM[i]&0x40;
		int hi=RAM[i]>>4,lo=RAM[i]&0xf;

		/* Only swap bits if != 3 or 5 */
		if (hi!=3 && hi!=5 && lo!=3) {
			/* Mask to 0x9f, misses bits 5 & 6 */
			RAM[i]=(RAM[i]&0x9f);
			if (swap1) RAM[i]+=0x40;
			if (swap2) RAM[i]+=0x20;
    	}
	}

#if 0
{
	FILE *fp;
	fp=fopen("dump.rom","wb");
	fwrite(RAM+0x8000,1,0x8000,fp);
	fclose(fp);
}
#endif
}

static void gondo_patch(void)
{
	unsigned char *RAM = Machine->memory_region[0];

	RAM = Machine->memory_region[0];
 /*	RAM[0x80d3]=0x12;
	RAM[0x80d4]=0x12;  */
}

static void lastmiss_patch(void)
{
	unsigned char *RAM = Machine->memory_region[0];

	RAM = Machine->memory_region[0];
	RAM[0xf9d4]=0x12;
	RAM[0xf9d5]=0x12;
    RAM[0xf9d8]=0x12;
	RAM[0xf9d9]=0x12;
}

static void shackled_patch(void)
{
	unsigned char *RAM = Machine->memory_region[3];
                                             //hmmm & above
	RAM = Machine->memory_region[3];
	RAM[0x4112]=0x12;
	RAM[0x4113]=0x12;

}

static void oscar_patch(void)
{
	/* Decrypt sound CPU - NOT YET CORRECT! */
	unsigned char *RAM = Machine->memory_region[2];
	int i;

	for (i=0x8000; i<0x9000; i++) {
 		int swap1=RAM[i]&0x20;
		int swap2=RAM[i]&0x40;

		/* Mask to 0x9f, misses bits 5 & 6 */
        if ((RAM[i]&0xf0)!=0x20 && (RAM[i]&0xf0)!=0x30 && (RAM[i]&0xf0)!=0x50 && (RAM[i]&0x0f)!=0x04) {
			RAM[i]=(RAM[i]&0x9f);
			if (swap1) RAM[i]+=0x40;
			if (swap2) RAM[i]+=0x20;
        }
	}

//   RAM[0x828f]=0xcf;

#if 0
{
    FILE *fp;

    fp=fopen("dump.rom","wb");
    fwrite(RAM+0x8000,1,0x8000,fp);
    fclose(fp);
}

{
    FILE *fp;

    fp=fopen("dw-05.rom","rb");
    fread(RAM+0x8000,1,0x1000,fp);
    fclose(fp);
}
#endif

}

/******************************************************************************/

static int cobracom_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x06c6],"\x00\x84\x76",3) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x06c6],30);
			osd_fclose(f);

			/* copy the high score to the work RAM as well */
            RAM[0x0135] = RAM[0x06c6];
            RAM[0x0136] = RAM[0x06c7];
            RAM[0x0137] = RAM[0x06c8];
		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void cobracom_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x06c6],30);
		osd_fclose(f);
	}
}

/******************************************************************************/

struct GameDriver cobracom_driver =
{
	__FILE__,
	0,
	"cobracom",
	"Cobra Command",
	"1988",
	"Data East Corporation",
	"Bryan McPhail",
	0,
	&cobra_machine_driver,
	0,

	cobracom_rom,
	0, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	cobracom_hiload, cobracom_hisave
};

struct GameDriver ghostb_driver =
{
	__FILE__,
	0,
	"ghostb",
	"The Real Ghostbusters",
	"1987",
	"Data East USA",
	"Bryan McPhail",
	0,
	&ghostb_machine_driver,
	0,

	ghostb_rom,
	ghostb_patch, 0,
	0,
	0,

	ghostb_input_ports,

	PROM_MEMORY_REGION(3), 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver mazeh_driver =
{
	__FILE__,
	&ghostb_driver,
	"mazeh",
	"Maze Hunter (Japan)",
	"1987",
	"Data East Corporation",
	"Bryan McPhail",
	0,
	&mazeh_machine_driver,
	0,

	mazeh_rom,
	mazeh_patch, 0,
	0,
	0,

	ghostb_input_ports,

	PROM_MEMORY_REGION(3), 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver srdarwin_driver =
{
	__FILE__,
	0,
	"srdarwin",
	"Super Real Darwin",
	"1987",
	"Data East Corporation",
	"Bryan McPhail",
	GAME_NOT_WORKING,
	&srdarwin_machine_driver,
	0,

	srdarwin_rom,
	srdarwin_patch, 0,
	0,
	0,

	darwin_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

struct GameDriver gondo_driver =
{
	__FILE__,
	0,
	"gondo",
	"Gondomania",
	"????",
	"?????",
	"Bryan McPhail",
	0,
	&gondo_machine_driver,
	0,

	gondo_rom,
	gondo_patch, 0,
	0,
	0,

	input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

struct GameDriver oscar_driver =
{
	__FILE__,
	0,
	"oscar",
	"Psycho-Nics Oscar",
	"1988",
	"Data East USA",
	"Bryan McPhail",
	0,
	&oscar_machine_driver,
	0,

	oscar_rom,
	oscar_patch, 0,
	0,
	0,

	oscar_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver oscarj_driver =
{
	__FILE__,
	&oscar_driver,
	"oscarj",
	"Psycho-Nics Oscar (Japan)",
	"1987",
	"Data East Corporation",
	"Bryan McPhail",
	0,
	&oscar_machine_driver,
	0,

	oscarj_rom,
	oscar_patch, 0,
	0,
	0,

	oscar_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver lastmiss_driver =
{
	__FILE__,
	0,
	"lastmiss",
	"Last Mission",
	"1986",
	"Data East USA",
	"Bm",
	0,
	&lastmiss_machine_driver,
	0,

	lastmiss_rom,
	lastmiss_patch, 0,
	0,
	0,

	shackled_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,

	0,0
};

struct GameDriver shackled_driver =
{
	__FILE__,
	0,
	"shackled",
	"Shackled",
	"1986",
	"Data East USA",
	"Bm",
	0,
	&shackled_machine_driver,
	0,

	shackled_rom,
	shackled_patch, 0,
	0,
	0,

	shackled_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};

struct GameDriver breywood_driver =
{
	__FILE__,
	&shackled_driver,
	"breywood",
	"breywood",
	"????",
	"?????",
	"Bm",
	0,
	&shackled_machine_driver,
	0,

	breywood_rom,
	shackled_patch, 0,
	0,
	0,

	lastmiss_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,

	0,0
};

