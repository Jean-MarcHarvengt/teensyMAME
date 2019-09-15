/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

Important!  There are two types of NeoGeo romdump - MVS & MGD2.  They are both
converted to a standard format in the vh_start routines.


Graphics information:

0x00000 - 0xdfff	: Blocks of sprite data, each 0x80 bytes:
	Each 0x80 block is made up of 0x20 double words, their format is:
	Word: Sprite number (16 bits)
    Byte: Palette number (8 bits)
    Byte: Bit 0: X flip
          Bit 1: Y flip
          Bit 2: Automatic animation flag (4 tiles?)
          Bit 3: Automatic animation flag (8 tiles?)
          Bit 4: MSB of sprite number (confirmed, Karnov_r, Mslug). See note.
          Bit 5: MSB of sprite number (MSlug2)
          Bit 6: MSB of sprite number (Kof97)
          Bit 7: Unknown for now

    Each double word sprite is drawn directly underneath the previous one,
    based on the starting coordinates.

0x0e000 - 0x0ea00	: Front plane fix tiles (8*8), 2 bytes each

0x10000: Control for sprites banks, arranged in words

	Bit 0 to 3 - Y zoom LSB
    Bit 4 to 7 - Y zoom MSB (ie, 1 byte for Y zoom).
    Bit 8 to 11 - X zoom, 0xf is full size (no scale).
    Bit 12 to 15 - Unknown, probably unused

0x10400: Control for sprite banks, arranged in words

	Bit 0 to 5: Number of sprites in this bank (see note below).
	Bit 6 - If set, this bank is placed to right of previous bank (same Y-coord).
	Bit 7 to 15 - Y position for sprite bank.

0x10800: Control for sprite banks, arranged in words
	Bit 0 to 5: Unknown
	Bit 7 to 15 - X position for sprite bank.

Notes:

* If rom set has less than 0x10000 tiles then msb of tile must be ignored
(see Magician Lord).

***************************************************************************/

#include "driver.h"
#include "common.h"
#include "usrintrf.h"
#include "vidhrdw/generic.h"

//#define NEO_DEBUG

/* The following two will save precomputed palette & graphics data to disk for fast
game startup - good for developers but uses lots of disk space! :) */
//#define CACHE_PENUSAGE
//#define CACHE_GRAPHICS

static unsigned char *vidram;
static unsigned char *neogeo_paletteram;       /* pointer to 1 of the 2 palette banks */
static unsigned char *pal_bank1;		/* 0x100*16 2 byte palette entries */
static unsigned char *pal_bank2;		/* 0x100*16 2 byte palette entries */
static int palno,modulo,where,high_tile,vhigh_tile,vvhigh_tile;
int no_of_tiles;
static int palette_swap_pending,fix_bank;

extern unsigned char *neogeo_ram;
extern unsigned int neogeo_frame_counter;
extern int neogeo_game_fix;
int neogeo_red_mask,neogeo_green_mask,neogeo_blue_mask;

void NeoMVSDrawGfx(unsigned char **line,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
        int zx,int zy);

static char dda_x_skip[16];
static char dda_y_skip[17];
static char full_y_skip[16]={0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};

#ifdef NEO_DEBUG
int neo_unknown[32];
void neo_unknown1(int offset, int data) {WRITE_WORD(&neo_unknown[0],data);}
void neo_unknown2(int offset, int data) {WRITE_WORD(&neo_unknown[2],data);}
void neo_unknown3(int offset, int data) {WRITE_WORD(&neo_unknown[4],data);}
void neo_unknown4(int offset, int data) {if (cpu_getpc()!=0x4a44) WRITE_WORD(&neo_unknown[6],data>>7);}

int dotiles = 0;
int screen_offs = 0x0000;

#endif

/******************************************************************************/

static void swap_palettes(void)
{
	int i,newword,red,green,blue;

    for (i=0; i<0x2000; i+=2) {
       	newword = READ_WORD(&neogeo_paletteram[i]);
    	red=   ((newword>>8)&neogeo_red_mask)*0x11;
		green= ((newword>>4)&neogeo_green_mask)*0x11;
		blue=  ((newword>>0)&neogeo_blue_mask)*0x11;

		palette_change_color(i / 2,red,green,blue);
    }

    palette_swap_pending=0;
}

void neogeo_setpalbank0(int offset,int data)
{
	if (palno != 0) {
		palno = 0;
		neogeo_paletteram = pal_bank1;
		palette_swap_pending=1;
	}
}

void neogeo_setpalbank1(int offset,int data)
{
	if (palno != 1) {
		palno = 1;
		neogeo_paletteram = pal_bank2;
		palette_swap_pending=1;
    }
}

int neogeo_paletteram_r(int offset)
{
	return READ_WORD(&neogeo_paletteram[offset]);
}

void neogeo_paletteram_w(int offset,int data)
{
	int oldword = READ_WORD (&neogeo_paletteram[offset]);
	int newword = COMBINE_WORD (oldword, data);
	int red=((newword>>8)&neogeo_red_mask);
	int green=((newword>>4)&neogeo_green_mask);
	int blue=((newword>>0)&neogeo_blue_mask);

	WRITE_WORD (&neogeo_paletteram[offset], newword);
	red			= red*0x11;
	green		= green*0x11;
	blue		= blue*0x11;

	palette_change_color(offset / 2,red,green,blue);
}

/******************************************************************************/

void neogeo_vh_stop(void)
{
   	if (pal_bank1) free (pal_bank1);
	if (pal_bank2) free (pal_bank2);
	if (vidram) free (vidram);
    if (neogeo_ram) free (neogeo_ram);

	pal_bank1=pal_bank2=vidram=neogeo_ram=0;
}

static int common_vh_start(void)
{
	pal_bank1=pal_bank2=vidram=0;

	pal_bank1 = malloc(0x2000);
	if (!pal_bank1) {
		neogeo_vh_stop();
		return 1;
	}

	pal_bank2 = malloc(0x2000);
	if (!pal_bank2) {
		neogeo_vh_stop();
		return 1;
	}

	vidram = malloc(0x20000); /* 0x20000 bytes even though only 0x10c00 is used */
	if (!vidram) {
		neogeo_vh_stop();
		return 1;
	}
	memset(vidram,0,0x20000);

	neogeo_paletteram = pal_bank1;
	palette_transparent_color = 4095;
    palno=0;
    modulo=1;
	where=0;
    fix_bank=0;
	palette_swap_pending=0;

	return 0;
}


/******************************************************************************/

static const unsigned char *neogeo_palette(void)
{
    int color,code,pal_base,y,my=0,x,count,offs,i;
 	int colmask[256];
    unsigned int *pen_usage; /* Save some struct derefs */

	int sx =0,sy =0,oy =0,zx = 1, rzy = 1;
    int tileno,tileatr,t1,t2,t3;
    char fullmode=0;
    int ddax=16,dday=256,rzx=15,yskip=0;

	memset(palette_used_colors,PALETTE_COLOR_UNUSED,4096);

	/* character foreground */
    pen_usage= Machine->gfx[fix_bank]->pen_usage;
	pal_base = Machine->drv->gfxdecodeinfo[fix_bank].color_codes_start;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs=0xe000;offs<0xea00;offs+=2) {
    	code = READ_WORD( &vidram[offs] );
    	color = code >> 12;
        colmask[color] |= pen_usage[code&0xfff];
    }
	for (color = 0;color < 16;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

    /* Tiles */
    pen_usage= Machine->gfx[2]->pen_usage;
    pal_base = Machine->drv->gfxdecodeinfo[2].color_codes_start;
	for (color = 0;color < 256;color++) colmask[color] = 0;
    for (count=0;count<0x300;count+=2) {
		t3 = READ_WORD( &vidram[0x10000 + count] );
		t1 = READ_WORD( &vidram[0x10400 + count] );
		t2 = READ_WORD( &vidram[0x10800 + count] );

        /* If this bit is set this new column is placed next to last one */
		if (t1 & 0x40) {
			sx += rzx;
			if ( sx >= 0x1F0 )
				sx -= 0x200;

            /* Get new zoom for this column */
		    zx = (t3 >> 8) & 0x0f;
if(neogeo_game_fix==7 && (t3==0 || t3==0x147f))         // Gururin Bodge fix
	zx=0xf;
			sy = oy;
		} else {	/* nope it is a new block */
        	/* Sprite scaling */
			zx = (t3 >> 8) & 0x0f;
			rzy = t3 & 0xff;
if(neogeo_game_fix==7 && (t3==0 || t3==0x147f))			// Gururin Bodge fix
{
	zx=0xf;
	rzy=0xff;
}

			sx = (t2 >> 7);
			if ( sx >= 0x1F0 )
				sx -= 0x200;

            /* Number of tiles in this strip */
            my = t1 & 0x3f;
			if (my == 0x20) fullmode = 1;
			else if (my >= 0x21) fullmode = 2;	/* most games use 0x21, but */
												/* Alpha Mission II uses 0x3f */
			else fullmode = 0;

			sy = 0x1F0 - (t1 >> 7);
			if (sy > 0x100) sy -= 0x200;
			if (fullmode == 2 || (fullmode == 1 && rzy == 0xff))
			{
				while (sy < -16) sy += 2 * (rzy + 1);
			}
			oy = sy;

		  	if(my==0x21) my=0x20;
			else if(rzy!=0xff && my!=0)
				my=((my*16*256)/(rzy+1) + 15)/16;

            if(my>0x20) my=0x20;

            ddax=16;		/* setup x zoom */
		}

		/* No point doing anything if tile strip is 0 */
		if (my==0) continue;

		/* Process x zoom */
		if(zx!=15) {
			rzx=0;
			for(i=0;i<16;i++) {
				ddax-=zx+1;
				if(ddax<=0) {
					ddax+=15+1;
					rzx++;
				}
			}
		}
		else rzx=16;

		if(sx>311) continue;

		/* Setup y zoom */
		if(rzy==255)
			yskip=16;
		else
			dday=256;

		offs = count<<6;

        /* my holds the number of tiles in each vertical multisprite block */
        for (y=0; y < my ;y++) {
			tileno  = READ_WORD(&vidram[offs]);
            offs+=2;
			tileatr = READ_WORD(&vidram[offs]);
            offs+=2;

            if (high_tile && tileatr&0x10) tileno+=0x10000;
			if (vhigh_tile && tileatr&0x20) tileno+=0x20000;
			if (vvhigh_tile && tileatr&0x40) tileno+=0x40000;

            if (tileatr&0x8) tileno=(tileno&~7)+((tileno+neogeo_frame_counter)&7);
            else if (tileatr&0x4) tileno=(tileno&~3)+((tileno+neogeo_frame_counter)&3);

			if (fullmode == 2 || (fullmode == 1 && rzy == 0xff))
			{
				if (sy >= 224) sy -= 2 * (rzy + 1);
			}
			else if (fullmode == 1)
			{
				if (y == 0x10) sy -= 2 * (rzy + 1);
			}

            if(rzy!=255) {
            	yskip=0;
                for(i=0;i<16;i++) {
                    dday-=rzy+1;
                    if(dday<=0) {
                    	dday+=256;
                    	yskip++;
                    }
                }
            }

			if ( (tileatr>>8) != 0) // crap below zoomed sprite in nam1975 fix??
			if (sy<224)                // tidy this up later...
			{
				tileatr=tileatr>>8;
		        colmask[tileatr] |= pen_usage[tileno];
			}

			sy +=yskip;

		}  /* for y */
	}  /* for count */

	for (color = 0;color < 256;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	return palette_recalc();
}

/******************************************************************************/

void vidram_offset_w(int offset, int data)
{
	where = data*2;
}

int vidram_data_r(int offset)
{
	return (READ_WORD(&vidram[where & 0x1ffff]));
}

void vidram_data_w(int offset,int data)
{
	WRITE_WORD(&vidram[where & 0x1ffff],data);
	where += modulo;
}

/* Modulo can become negative , Puzzle Bobble Super Sidekicks and a lot */
/* of other games use this */
void vidram_modulo_w(int offset, int data) {
	if (data & 0x8000) {
		/* Sign extend it. */
		/* Where is the SEX instruction when you need it :-) */
		modulo = (data - 0x10000) << 1;
	}
	else
		modulo = data << 1;
}

int vidram_modulo_r(int offset) {
	return modulo >> 1;
}


/* Two routines to enable videoram to be read in debugger */
int mish_vid_r(int offset)
{
	return READ_WORD(&vidram[offset]);
}

void mish_vid_w(int offset, int data)
{
	COMBINE_WORD_MEM(&vidram[offset],data);
}

void neo_board_fix(int offset, int data)
{
	fix_bank=1;
}

void neo_game_fix(int offset, int data)
{
	fix_bank=0;
}

/******************************************************************************/

/* DWORD read - copied from common.c */

#ifdef ACORN /* GSL 980108 read/write nonaligned dword routine for ARM processor etc */

INLINE int read_dword(int *address)
{
	if ((int)address & 3)
	{
#ifdef LSB_FIRST  /* little endian version */
  		return (    *((unsigned char *)address) +
  			   (*((unsigned char *)address+1) << 8)  +
  		   	   (*((unsigned char *)address+2) << 16) +
  		           (*((unsigned char *)address+3) << 24) );
#else             /* big endian version */
  		return (    *((unsigned char *)address+3) +
  			   (*((unsigned char *)address+2) << 8)  +
  		   	   (*((unsigned char *)address+1) << 16) +
  		           (*((unsigned char *)address)   << 24) );
#endif
	}
	else
		return *(int *)address;
}

#else
#define read_dword(address) *(int *)address
#endif

#ifdef LSB_FIRST
#define BL0 0
#define BL1 1
#define BL2 2
#define BL3 3
#define WL0 0
#define WL1 1
#else
#define BL0 3
#define BL1 2
#define BL2 1
#define BL3 0
#define WL0 1
#define WL1 0
#endif

void NeoMVSDrawGfx(unsigned char **line,const struct GfxElement *gfx, /* AJP */
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
        int zx,int zy)
{
	int /*ox,*/oy,ex,ey,y,start,dy;
	unsigned char *bm;
	int col;
    int l; /* Line skipping counter */

    int mydword;

	unsigned char *fspr=0;

	unsigned char *PL1 = Machine->memory_region[2];
    unsigned char *PL2 = Machine->memory_region[3];

	char *l_y_skip;

    /* Safety feature */
    code=code%no_of_tiles;

    /* Check for total transparency, no need to draw */
    if ((gfx->pen_usage[code] & ~1) == 0)
    	return;

   	if(zy==16)
		 l_y_skip=full_y_skip;
	else
		 l_y_skip=dda_y_skip;


	if(code < no_of_tiles/2)
	{
		fspr=PL1;
	}
	else
	{
		fspr=PL2;
		code-=no_of_tiles/2;
	}

	/* Mish/AJP - Most clipping is done in main loop */
    oy=sy;
  	ey = sy + zy -1; 	/* Clip for size of zoomed object */

	if (sx <= -8) return;
	if (sy < 0) sy = 0;
    if (ey >= 224) ey = 224-1;

	if (flipy)	/* Y flip */
	{
		dy = -8;
		fspr+=(code+1)*128 - 8 - (sy-oy)*8;
	}
	else		/* normal */
	{
		dy = 8;
		fspr+=code*128 + (sy-oy)*8;
	}

	{
		const unsigned short *paldata;	/* ASG 980209 */
		paldata = &gfx->colortable[gfx->color_granularity * color];
		if (flipx)	/* X flip */
		{
			l=0;
			if(zx==16)
			{
				for (y = sy;y <= ey;y++)
				{
                    bm  = line[y]+sx;

                    fspr+=l_y_skip[l]*dy;

					mydword = read_dword((int *)(fspr+4));
					col = (mydword>> 28)&0xf; if (col) bm[BL0] = paldata[col];
					col = (mydword>> 20)&0xf; if (col) bm[BL1] = paldata[col];
					col = (mydword>> 12)&0xf; if (col) bm[BL2] = paldata[col];
					col = (mydword>>  4)&0xf; if (col) bm[BL3] = paldata[col];

					col = (mydword>> 24)&0xf; if (col) bm[8 + BL0] = paldata[col];
					col = (mydword>> 16)&0xf; if (col) bm[8 + BL1] = paldata[col];
					col = (mydword>>  8)&0xf; if (col) bm[8 + BL2] = paldata[col];
					col = (mydword>>  0)&0xf; if (col) bm[8 + BL3] = paldata[col];
                    mydword = read_dword((int *)fspr);
                    col = (mydword>> 28)&0xf; if (col) bm[4 + BL0] = paldata[col];
                    col = (mydword>> 20)&0xf; if (col) bm[4 + BL1] = paldata[col];
                    col = (mydword>> 12)&0xf; if (col) bm[4 + BL2] = paldata[col];
                    col = (mydword>>  4)&0xf; if (col) bm[4 + BL3] = paldata[col];

               		col = (mydword>> 24)&0xf; if (col) bm[12 + BL0] = paldata[col];
					col = (mydword>> 16)&0xf; if (col) bm[12 + BL1] = paldata[col];
               		col = (mydword>>  8)&0xf; if (col) bm[12 + BL2] = paldata[col];
               		col = (mydword>>  0)&0xf; if (col) bm[12 + BL3] = paldata[col];
					l++;
				}
			}
			else
			{
				for (y = sy;y <= ey;y++)
				{
           			bm  = line[y]+sx;
                    fspr+=l_y_skip[l]*dy;

                    mydword = read_dword((int *)(fspr+4));
                    #ifdef LSB_FIRST
					if (dda_x_skip[0]) {col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[1]) {col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[2]) {col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[3]) {col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++;}
					#else
					if (dda_x_skip[0]) {col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[1]) {col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[2]) {col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[3]) {col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++;}
					#endif

                    mydword = read_dword((int *)fspr);
                    #ifdef LSB_FIRST
					if (dda_x_skip[4]) {col = (mydword>> 28)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[5]) {col = (mydword>> 20)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[6]) {col = (mydword>> 12)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[7]) {col = (mydword>>  4)&0xf; if (col) *bm = paldata[col]; bm++;}
					#else
					if (dda_x_skip[4]) {col = (mydword>>  4)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[5]) {col = (mydword>> 12)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[6]) {col = (mydword>> 20)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[7]) {col = (mydword>> 28)&0xf; if (col) *bm = paldata[col]; bm++;}
					#endif

                    mydword = read_dword((int *)(fspr+4));
                    #ifdef LSB_FIRST
					if (dda_x_skip[8]) {col = (mydword>> 24)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[9]) {col = (mydword>> 16)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[10]) {col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[11]) {col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++;}
					#else
					if (dda_x_skip[8]) {col = (mydword>>  0)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[9]) {col = (mydword>>  8)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[10]) {col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[11]) {col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++;}
					#endif

                    mydword = read_dword((int *)fspr);
                    #ifdef LSB_FIRST
					if (dda_x_skip[12]) {col = (mydword>> 24)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[13]) {col = (mydword>> 16)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[14]) {col = (mydword>>  8)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[15]) {col = (mydword>>  0)&0xf; if (col) *bm = paldata[col]; bm++;}
					#else
					if (dda_x_skip[12]) {col = (mydword>>  0)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[13]) {col = (mydword>>  8)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[14]) {col = (mydword>> 16)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[15]) {col = (mydword>> 24)&0xf; if (col) *bm = paldata[col]; bm++;}
					#endif

					l++;
				}
			}
		}
		else		/* normal */
		{
      		l=0;
			if(zx==16)
			{
				for (y = sy ;y <= ey;y++)
				{
        			bm  = line[y] + sx;
					fspr+=l_y_skip[l]*dy;

                    mydword = read_dword((int *)fspr);
               		col = (mydword>> 0)&0xf; if (col) bm[BL0] = paldata[col];
               		col = (mydword>> 8)&0xf; if (col) bm[BL1] = paldata[col];
               		col = (mydword>>16)&0xf; if (col) bm[BL2] = paldata[col];
               		col = (mydword>>24)&0xf; if (col) bm[BL3] = paldata[col];

                    col = (mydword>> 4)&0xf; if (col) bm[8 + BL0] = paldata[col];
               		col = (mydword>>12)&0xf; if (col) bm[8 + BL1] = paldata[col];
               		col = (mydword>>20)&0xf; if (col) bm[8 + BL2] = paldata[col];
               		col = (mydword>>28)&0xf; if (col) bm[8 + BL3] = paldata[col];

                    mydword = read_dword((int *)(fspr+4));
               		col = (mydword>> 0)&0xf; if (col) bm[4 + BL0] = paldata[col];
               		col = (mydword>> 8)&0xf; if (col) bm[4 + BL1] = paldata[col];
               		col = (mydword>>16)&0xf; if (col) bm[4 + BL2] = paldata[col];
               		col = (mydword>>24)&0xf; if (col) bm[4 + BL3] = paldata[col];

                    col = (mydword>> 4)&0xf; if (col) bm[12 + BL0] = paldata[col];
               		col = (mydword>>12)&0xf; if (col) bm[12 + BL1] = paldata[col];
               		col = (mydword>>20)&0xf; if (col) bm[12 + BL2] = paldata[col];
               		col = (mydword>>28)&0xf; if (col) bm[12 + BL3] = paldata[col];
					l++;
				}
			}
			else
			{
				for (y = sy ;y <= ey;y++)
				{
        			bm  = line[y] + sx;
                    fspr+=l_y_skip[l]*dy;

                    mydword = read_dword((int *)fspr);
                    #ifdef LSB_FIRST
					if (dda_x_skip[0]) {col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[1]) {col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[2]) {col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[3]) {col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++;}
					#else
					if (dda_x_skip[0]) {col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[1]) {col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[2]) {col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[3]) {col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++;}
					#endif

                    mydword = read_dword((int *)(fspr+4));
                    #ifdef LSB_FIRST
					if (dda_x_skip[4]) {col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[5]) {col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[6]) {col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[7]) {col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++;}
					#else
					if (dda_x_skip[4]) {col = (mydword>>24)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[5]) {col = (mydword>>16)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[6]) {col = (mydword>> 8)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[7]) {col = (mydword>> 0)&0xf; if (col) *bm = paldata[col]; bm++;}
					#endif

                    mydword = read_dword((int *)fspr);
                    #ifdef LSB_FIRST
					if (dda_x_skip[8])  {col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[9])  {col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[10]) {col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[11]) {col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++;}
					#else
					if (dda_x_skip[8])  {col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[9])  {col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[10]) {col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[11]) {col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++;}
					#endif

                    mydword = read_dword((int *)(fspr+4));
                    #ifdef LSB_FIRST
					if (dda_x_skip[12]) {col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[13]) {col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[14]) {col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[15]) {col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++;}
					#else
					if (dda_x_skip[12]) {col = (mydword>>28)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[13]) {col = (mydword>>20)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[14]) {col = (mydword>>12)&0xf; if (col) *bm = paldata[col]; bm++;}
					if (dda_x_skip[15]) {col = (mydword>> 4)&0xf; if (col) *bm = paldata[col]; bm++;}
					#endif

					l++;
				}
			}
		}
	}
}

static void neo_drawgfx_char(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int sx,int sy)
{
	int ex,ey,y,start;
	const unsigned char *sd;
	unsigned char *bm;
	int col;
	int *sd4;
	int col4;
	const unsigned short *paldata;	/* ASG 980209 */
	unsigned char **sline=gfx->gfxdata->line;
	unsigned char **dline=dest->line;

	ex = sx + 7; /* Clip width */
	ey = sy + 7; /* Clip height */

	start = code * 8; /* 8 bytes per character */
	paldata = &gfx->colortable[gfx->color_granularity * color];

	if ((gfx->pen_usage[code] & 1) == 0)
	{
		/* character is totally opaque, can disable transparency */
		for (y = sy;y <= ey;y++)
		{
			bm = dline[y]+sx;
			sd = sline[start];
			bm[0] = paldata[sd[0]];
			bm[1] = paldata[sd[1]];
			bm[2] = paldata[sd[2]];
			bm[3] = paldata[sd[3]];
			bm[4] = paldata[sd[4]];
			bm[5] = paldata[sd[5]];
			bm[6] = paldata[sd[6]];
			bm[7] = paldata[sd[7]];
			start+=1;
		}
	}
	else
	{
		for (y = sy;y <= ey;y++)
		{
			bm = dline[y]+sx;
			sd4 = (int *)(sline[start]);
			if ((col4=read_dword(sd4)) != 0){
				col = col4&0xff;
				if (col) bm[BL0] = paldata[col];
				col = (col4>>8)&0xff;
				if (col) bm[BL1] = paldata[col];
				col = (col4>>16)&0xff;
				if (col) bm[BL2] = paldata[col];
				col = (col4>>24)&0xff;
				if (col) bm[BL3] = paldata[col];
			}
			sd4++;
			bm+=4;
			if ((col4=read_dword(sd4)) != 0){
				col = col4&0xff;
				if (col) bm[BL0] = paldata[col];
				col = (col4>>8)&0xff;
				if (col) bm[BL1] = paldata[col];
				col = (col4>>16)&0xff;
				if (col) bm[BL2] = paldata[col];
				col = (col4>>24)&0xff;
				if (col) bm[BL3] = paldata[col];
			}
			start+=1;
		}
	}
}

/******************************************************************************/

void neogeo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int sx =0,sy =0,oy =0,my =0,zx = 1, rzy = 1;
	int offs,i,count,y,x;
    int tileno,tileatr,t1,t2,t3;
    char fullmode=0;
    int ddax=16,dday=256,rzx=15,yskip=0;
	unsigned char **line=bitmap->line;
    unsigned int *pen_usage;
	struct GfxElement *gfx=Machine->gfx[2]; /* Save constant struct dereference */

    #ifdef NEO_DEBUG
	char buf[80];
	struct DisplayText dt[2];

	/* debug setting, tile view mode connected to '8' */
	if (osd_key_pressed(OSD_KEY_8))
	{
		while (osd_key_pressed(OSD_KEY_8)) ;
		dotiles ^= 1;
	}

	/* tile view - 0x80, connected to '9' */
	if (osd_key_pressed(OSD_KEY_9))
	{
		if (screen_offs > 0)
			screen_offs -= 0x80;
	}

	/* tile view + 0x80, connected to '0' */
	if (osd_key_pressed(OSD_KEY_0))
	{
		if (screen_offs < 0x10000)
			screen_offs += 0x80;
	}
    #endif

    /* Palette swap occured after last frame but before this one */
	if (palette_swap_pending) swap_palettes();

    /* Do compressed palette stuff */
	neogeo_palette();
	fillbitmap(bitmap,palette_transparent_pen,&Machine->drv->visible_area);

#ifdef NEO_DEBUG
if (!dotiles) { 					/* debug */
#endif

	/* Draw sprites */
    for (count=0;count<0x300;count+=2) {
		t3 = READ_WORD( &vidram[0x10000 + count] );
		t1 = READ_WORD( &vidram[0x10400 + count] );
		t2 = READ_WORD( &vidram[0x10800 + count] );

        /* If this bit is set this new column is placed next to last one */
		if (t1 & 0x40) {
			sx += rzx;
			if ( sx >= 0x1F0 )
				sx -= 0x200;

            /* Get new zoom for this column */
            zx = (t3 >> 8) & 0x0f;
if(neogeo_game_fix==7 && (t3==0 || t3==0x147f))         // Gururin Bodge fix
	zx=0xf;
			sy = oy;
		} else {	/* nope it is a new block */
        	/* Sprite scaling */
			zx = (t3 >> 8) & 0x0f;
			rzy = t3 & 0xff;
if(neogeo_game_fix==7 && (t3==0 || t3==0x147f))         // Gururin Bodge fix
{
	zx=0xf;
	rzy=0xff;
}

			sx = (t2 >> 7);
			if ( sx >= 0x1F0 )
				sx -= 0x200;

            /* Number of tiles in this strip */
            my = t1 & 0x3f;
			if (my == 0x20) fullmode = 1;
			else if (my >= 0x21) fullmode = 2;	/* most games use 0x21, but */
												/* Alpha Mission II uses 0x3f */
			else fullmode = 0;

			sy = 0x1F0 - (t1 >> 7);
			if (sy > 0x100) sy -= 0x200;
			if (fullmode == 2 || (fullmode == 1 && rzy == 0xff))
			{
				while (sy < -16) sy += 2 * (rzy + 1);
			}
			oy = sy;

		  	if(my==0x21) my=0x20;
			else if(rzy!=0xff && my!=0)
				my=((my*16*256)/(rzy+1) + 15)/16;

            if(my>0x20) my=0x20;

            ddax=16;		/* setup x zoom */
		}

		/* No point doing anything if tile strip is 0 */
		if (my==0) continue;

		/* Process x zoom */
		if(zx!=15) {
			rzx=0;
			for(i=0;i<16;i++) {
				ddax-=zx+1;
				if(ddax<=0) {
					ddax+=15+1;
					dda_x_skip[i]=1;
					rzx++;
				}
				else dda_x_skip[i]=0;
			}
		}
		else rzx=16;

		if(sx>311) continue;

		/* Setup y zoom */
		if(rzy==255)
			yskip=16;
		else
			dday=256;

		offs = count<<6;

        /* my holds the number of tiles in each vertical multisprite block */
        for (y=0; y < my ;y++) {
			tileno  = READ_WORD(&vidram[offs]);
            offs+=2;
			tileatr = READ_WORD(&vidram[offs]);
            offs+=2;

            if (high_tile && tileatr&0x10) tileno+=0x10000;
			if (vhigh_tile && tileatr&0x20) tileno+=0x20000;
			if (vvhigh_tile && tileatr&0x40) tileno+=0x40000;

            if (tileatr&0x8) tileno=(tileno&~7)+((tileno+neogeo_frame_counter)&7);
            else if (tileatr&0x4) tileno=(tileno&~3)+((tileno+neogeo_frame_counter)&3);

			if (fullmode == 2 || (fullmode == 1 && rzy == 0xff))
			{
				if (sy >= 224) sy -= 2 * (rzy + 1);
			}
			else if (fullmode == 1)
			{
				if (y == 0x10) sy -= 2 * (rzy + 1);
			}

            if(rzy!=255)
            {
            	yskip=0;
                dda_y_skip[0]=0;
                for(i=0;i<16;i++)
                {
                	dda_y_skip[i+1]=0;
                    dday-=rzy+1;
                    if(dday<=0)
                    {
                    	dday+=256;
                    	yskip++;
                        dda_y_skip[yskip]++;
                    }
                    else dda_y_skip[yskip]++;
                }
            }

			if ( (tileatr>>8) != 0) // crap below zoomed sprite in nam1975 fix??
			if (sy<224)                // tidy this up later...
            NeoMVSDrawGfx(line,
				gfx,
                tileno,
				tileatr >> 8,
				tileatr & 0x01,tileatr & 0x02,
				sx,sy,rzx,yskip
            );

			sy +=yskip;
		}  /* for y */
	}  /* for count */



	/* Save some struct de-refs */
	gfx=Machine->gfx[fix_bank];
    pen_usage=gfx->pen_usage;

	/* Character foreground */
 	for (y=2;y<30;y++) {
 		for (x=1;x<39;x++) {

  			int byte1 = (READ_WORD( &vidram[0xE000 + 2*y + x*64] ));
  			int byte2 = byte1 >> 12;
           	byte1 = byte1 & 0xfff;

			if((pen_usage[byte1] & ~ 1) == 0) continue;

/*
  			drawgfx(bitmap,
  				gfx,
  				byte1,
  				byte2,
  				0,0,
  				x*8,(y-2)*8,
  				&Machine->drv->visible_area,
  				TRANSPARENCY_PEN,
  				0);
*/
  			neo_drawgfx_char(bitmap,
  				gfx,
  				byte1,
  				byte2,
  				x*8,(y-2)*8);
  		}
	}



#ifdef NEO_DEBUG
	} else {	/* debug */
		offs = screen_offs;
		for (y=0;y<15;y++) {
			for (x=0;x<20;x++) {

				unsigned char byte1 = vidram[offs + 4*y+x*128];
				unsigned char byte2 = vidram[offs + 4*y+x*128+1];
				unsigned char col = vidram[offs + 4*y+x*128+3];
				unsigned char byte3 = vidram[offs + 4*y+x*128+2];

                tileno = byte1 + (byte2 << 8);
            	if (high_tile && byte3&0x10) tileno+=0x10000;
				if (vhigh_tile && byte3&0x20) tileno+=0x20000;
				if (vvhigh_tile && byte3&0x40) tileno+=0x40000;

                NeoMVSDrawGfx(line,
					Machine->gfx[2],
					tileno,
					col,
					byte3 & 0x01,byte3 & 0x02,
					x*16,y*16,16,16
                 );


			}
		}

		sprintf(buf,"POS : %04X , VDP regs %04X",screen_offs, (screen_offs >> 6) );
		dt[0].text = buf;
		dt[0].color = DT_COLOR_RED;
		dt[0].x = 0;
		dt[0].y = 0;
		dt[1].text = 0;
//		displaytext(dt,0);
	}	/* debug */
#endif

#ifdef NEO_DEBUG
/* More debug stuff :) */
{



	int j;
	char mybuf[20];
	int trueorientation;
	struct osd_bitmap *mybitmap = Machine->scrbitmap;

	trueorientation = Machine->orientation;
	Machine->orientation = ORIENTATION_DEFAULT;


for (i = 0;i < 8;i+=2)
{
	sprintf(buf,"%04X",READ_WORD(&neo_unknown[i]));
	for (j = 0;j < 4;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],DT_COLOR_WHITE,0,0,3*8*i+8*j,8*5,0,TRANSPARENCY_NONE,0);
}

  /*
for (i = 0;i < 8;i+=2)
{
	sprintf(mybuf,"%04X",READ_WORD(&vidram[0x100a0+i]));
	for (j = 0;j < 4;j++)
		drawgfx(mybitmap,Machine->uifont,mybuf[j],DT_COLOR_WHITE,0,0,3*8*i+8*j,8*5,0,TRANSPARENCY_NONE,0);
}


    sprintf(mybuf,"%04X",READ_WORD(&vidram[0x10002]));
	for (j = 0;j < 4;j++)
		drawgfx(mybitmap,Machine->uifont,mybuf[j],DT_COLOR_WHITE,0,0,8*j+4*8,8*7,0,TRANSPARENCY_NONE,0);
    sprintf(mybuf,"%04X",0x1f0-(READ_WORD(&vidram[0x10402])>>7));
	for (j = 0;j < 4;j++)
		drawgfx(mybitmap,Machine->uifont,mybuf[j],DT_COLOR_WHITE,0,0,8*j+10*8,8*7,0,TRANSPARENCY_NONE,0);
    sprintf(mybuf,"%04X",READ_WORD(&vidram[0x10802])>> 7);
	for (j = 0;j < 4;j++)
		drawgfx(mybitmap,Machine->uifont,mybuf[j],DT_COLOR_WHITE,0,0,8*j+16*8,8*7,0,TRANSPARENCY_NONE,0);

*/


//        fprintf(errorlog,"X: %04x Y: %04x Video: %04x\n",READ_WORD(&vidram[0x1089c]),READ_WORD(&vidram[0x1049c]),READ_WORD(&vidram[0x1009c]));

//fprintf(errorlog,"X: %04x Y: %04x Video: %04x\n",READ_WORD(&vidram[0x10930]),READ_WORD(&vidram[0x10530]),READ_WORD(&vidram[0x10130]));


}
#endif

}


/*************************/

#define GET_PIX0() ((p1)&1) + (((p2)&1)<<1) + (((p3)&1)<<2) + (((p4)&1)<<3)
#define GET_PIX1() ((p1>>1)&1) + (((p2)&2)) + (((p3)&2)<<1) + (((p4)&2)<<2)
#define GET_PIX2() ((p1>>2)&1) + (((p2>>1)&2)) + (((p3)&4)) + (((p4)&4)<<1)
#define GET_PIX3() ((p1>>3)&1) + (((p2>>2)&2)) + (((p3>>1)&4)) + (((p4)&8))
#define GET_PIX4() ((p1>>4)&1) + (((p2>>3)&2)) + (((p3>>2)&4)) + (((p4>>1)&8))
#define GET_PIX5() ((p1>>5)&1) + (((p2>>4)&2)) + (((p3>>3)&4)) + (((p4>>2)&8))
#define GET_PIX6() ((p1>>6)&1) + (((p2>>5)&2)) + (((p3>>4)&4)) + (((p4>>3)&8))
#define GET_PIX7() ((p1>>7)&1) + (((p2>>6)&2)) + (((p3>>5)&4)) + (((p4>>4)&8))


/* For MGD-2 dumps */
int neogeo_mgd2_vh_start(void)
{
	int i,x,y,tiles,p1,p2,p3,p4;
    unsigned char mybyte;
	unsigned char *PL1 = Machine->memory_region[2];
    unsigned char *PL2 = Machine->memory_region[3];

   	unsigned char *dest,*s;
	unsigned char swap[128];
	unsigned char swap2[128];
	unsigned int *pen_usage;
	unsigned int pen;


    /* For MGD2 games we need to calculate the pen_usage array ourself */
	if (Machine->gfx[2]->pen_usage)
    	free(Machine->gfx[2]->pen_usage);
    tiles=Machine->memory_region_length[2]/64;
    no_of_tiles=tiles;
    if (no_of_tiles>0x10000) high_tile=1; else high_tile=0;
	if (no_of_tiles>0x20000) vhigh_tile=1; else vhigh_tile=0;
	if (no_of_tiles>0x40000) vvhigh_tile=1; else vvhigh_tile=0;
	if (errorlog && high_tile) fprintf(errorlog,"Using high tile code\n");
	if (errorlog && vhigh_tile) fprintf(errorlog,"Using very high tile code\n");
	if (errorlog && vvhigh_tile) fprintf(errorlog,"Using very very high tile code!\n");
    Machine->gfx[2]->pen_usage=malloc(tiles * sizeof(int));

	pen_usage=Machine->gfx[2]->pen_usage;

/* swap all MGD2 planes into a single area.
 This would be much quicker if the interleaving was done on loading.
 */
	{
		int block_size,block_count,block_start,block_offset,block_steps,blocks;
		int block_tiles=no_of_tiles*4;
		int j,k;
		unsigned char *d;

		for(k=0;k<2;k++)
		{
			block_size=block_tiles*128/4/4;
			block_steps=block_tiles/4;
			block_offset=block_tiles*128/4;

			block_start=block_size;

			s=PL1+block_start;
			d=PL2;
			blocks=1;

			for(i=0;i<block_steps;i++)
			{
				memcpy(swap,s,32);
				memcpy(s,d,32);
				memcpy(d,swap,32);
				s+=32;
				d+=32;
			}

			do
			{
				block_size/=2;
				block_steps/=2;
				block_start/=2;
				block_offset/=2;
				blocks*=2;
				for(i=0;i<blocks/2;i++)
				{
					s=PL1+block_start+block_offset*i;
					d=PL1+block_start+block_offset*i+block_size;
					for(j=0;j<block_steps;j++)
					{
						memcpy(swap,s,32);
						memcpy(s,d,32);
						memcpy(d,swap,32);
						s+=32;
						d+=32;
					}

					s=PL2+block_start+block_offset*i;
					d=PL2+block_start+block_offset*i+block_size;
					for(j=0;j<block_steps;j++)
					{
						memcpy(swap,s,32);
						memcpy(s,d,32);
						memcpy(d,swap,32);
						s+=32;
						d+=32;
					}
				}
				if(block_steps==3)
				{
					block_offset/=2;
					for(i=0;i<blocks;i++)
					{
						s=PL1+block_offset*i;
						memcpy(swap,s+1*32,32);
						memcpy(s+1*32,s+3*32,32);
						memcpy(swap2,s+2*32,32);
						memcpy(s+2*32,swap,32);
						memcpy(s+3*32,s+4*32,32);
						memcpy(s+4*32,swap2,32);

						s=PL2+block_offset*i;
						memcpy(swap,s+1*32,32);
						memcpy(s+1*32,s+3*32,32);
						memcpy(swap2,s+2*32,32);
						memcpy(s+2*32,swap,32);
						memcpy(s+3*32,s+4*32,32);
						memcpy(s+4*32,swap2,32);
					}
					block_steps=1;
				}
				else if(block_steps==5)
				{
					block_offset/=2;
					for(i=0;i<blocks;i++)
					{
						s=PL1+block_offset*i;
						memcpy(swap,s+1*32,32);
						memcpy(s+1*32,s+5*32,32);
						memcpy(s+5*32,s+7*32,32);
						memcpy(s+7*32,s+8*32,32);
						memcpy(s+8*32,s+4*32,32);
						memcpy(s+4*32,s+2*32,32);
						memcpy(s+2*32,swap,32);
						memcpy(swap,s+3*32,32);
						memcpy(s+3*32,s+6*32,32);
						memcpy(s+6*32,swap,32);

						s=PL2+block_offset*i;
						memcpy(swap,s+1*32,32);
						memcpy(s+1*32,s+5*32,32);
						memcpy(s+5*32,s+7*32,32);
						memcpy(s+7*32,s+8*32,32);
						memcpy(s+8*32,s+4*32,32);
						memcpy(s+4*32,s+2*32,32);
						memcpy(s+2*32,swap,32);
						memcpy(swap,s+3*32,32);
						memcpy(s+3*32,s+6*32,32);
						memcpy(s+6*32,swap,32);
					}
				}
				else if(block_steps==7)			// not used yet
				{
					block_offset/=2;
					for(i=0;i<blocks;i++)
					{
						s=PL1+block_offset*i;
						memcpy(swap,s+1*32,32);
						memcpy(s+1*32,s+7*32,32);
						memcpy(s+7*32,s+10*32,32);
						memcpy(s+10*32,s+5*32,32);
						memcpy(s+5*32,s+9*32,32);
						memcpy(s+9*32,s+11*32,32);
						memcpy(s+11*32,s+12*32,32);
						memcpy(s+12*32,s+6*32,32);
						memcpy(s+6*32,s+3*32,32);
						memcpy(s+3*32,s+8*32,32);
						memcpy(s+8*32,s+4*32,32);
						memcpy(s+4*32,s+2*32,32);
						memcpy(s+2*32,swap,32);

						s=PL2+block_offset*i;
						memcpy(swap,s+1*32,32);
						memcpy(s+1*32,s+7*32,32);
						memcpy(s+7*32,s+10*32,32);
						memcpy(s+10*32,s+5*32,32);
						memcpy(s+5*32,s+9*32,32);
						memcpy(s+9*32,s+11*32,32);
						memcpy(s+11*32,s+12*32,32);
						memcpy(s+12*32,s+6*32,32);
						memcpy(s+6*32,s+3*32,32);
						memcpy(s+3*32,s+8*32,32);
						memcpy(s+8*32,s+4*32,32);
						memcpy(s+4*32,s+2*32,32);
						memcpy(s+2*32,swap,32);
					}
					block_steps=1;
				}
				else if(block_steps==9)			// not used yet
				{
					block_offset/=2;
					for(i=0;i<blocks;i++)
					{
						s=PL1+block_offset*i;
						memcpy(swap,s+1*32,32);
						memcpy(s+1*32,s+9*32,32);
						memcpy(s+9*32,s+13*32,32);
						memcpy(s+13*32,s+15*32,32);
						memcpy(s+15*32,s+16*32,32);
						memcpy(s+16*32,s+8*32,32);
						memcpy(s+8*32,s+4*32,32);
						memcpy(s+4*32,s+2*32,32);
						memcpy(s+2*32,swap,32);
						memcpy(swap,s+3*32,32);
						memcpy(s+3*32,s+10*32,32);
						memcpy(s+10*32,s+5*32,32);
						memcpy(s+5*32,s+11*32,32);
						memcpy(s+11*32,s+14*32,32);
						memcpy(s+14*32,s+7*32,32);
						memcpy(s+7*32,s+12*32,32);
						memcpy(s+12*32,s+6*32,32);
						memcpy(s+6*32,swap,32);

						s=PL2+block_offset*i;
						memcpy(swap,s+1*32,32);
						memcpy(s+1*32,s+9*32,32);
						memcpy(s+9*32,s+13*32,32);
						memcpy(s+13*32,s+15*32,32);
						memcpy(s+15*32,s+16*32,32);
						memcpy(s+16*32,s+8*32,32);
						memcpy(s+8*32,s+4*32,32);
						memcpy(s+4*32,s+2*32,32);
						memcpy(s+2*32,swap,32);
						memcpy(swap,s+3*32,32);
						memcpy(s+3*32,s+10*32,32);
						memcpy(s+10*32,s+5*32,32);
						memcpy(s+5*32,s+11*32,32);
						memcpy(s+11*32,s+14*32,32);
						memcpy(s+14*32,s+7*32,32);
						memcpy(s+7*32,s+12*32,32);
						memcpy(s+12*32,s+6*32,32);
						memcpy(s+6*32,swap,32);
					}
					block_steps=1;
				}
				else if(block_steps==11)			// not used yet
				{
					block_offset/=2;
					for(i=0;i<blocks;i++)
					{
						s=PL1+block_offset*i;
						memcpy(swap,s+1*32,32);
						memcpy(s+1*32,s+11*32,32);
						memcpy(s+11*32,s+16*32,32);
						memcpy(s+16*32,s+8*32,32);
						memcpy(s+8*32,s+4*32,32);
						memcpy(s+4*32,s+2*32,32);
						memcpy(s+2*32,swap,32);
						memcpy(swap,s+3*32,32);
						memcpy(s+3*32,s+12*32,32);
						memcpy(s+12*32,s+6*32,32);
						memcpy(s+6*32,swap,32);
						memcpy(swap,s+5*32,32);
						memcpy(s+5*32,s+13*32,32);
						memcpy(s+13*32,s+17*32,32);
						memcpy(s+17*32,s+19*32,32);
						memcpy(s+19*32,s+20*32,32);
						memcpy(s+20*32,s+10*32,32);
						memcpy(s+10*32,swap,32);
						memcpy(swap,s+7*32,32);
						memcpy(s+7*32,s+14*32,32);
						memcpy(s+14*32,swap,32);
						memcpy(swap,s+9*32,32);
						memcpy(s+9*32,s+15*32,32);
						memcpy(s+15*32,s+18*32,32);
						memcpy(s+18*32,swap,32);

						s=PL2+block_offset*i;
						memcpy(swap,s+1*32,32);
						memcpy(s+1*32,s+11*32,32);
						memcpy(s+11*32,s+16*32,32);
						memcpy(s+16*32,s+8*32,32);
						memcpy(s+8*32,s+4*32,32);
						memcpy(s+4*32,s+2*32,32);
						memcpy(s+2*32,swap,32);
						memcpy(swap,s+3*32,32);
						memcpy(s+3*32,s+12*32,32);
						memcpy(s+12*32,s+6*32,32);
						memcpy(s+6*32,swap,32);
						memcpy(swap,s+5*32,32);
						memcpy(s+5*32,s+13*32,32);
						memcpy(s+13*32,s+17*32,32);
						memcpy(s+17*32,s+19*32,32);
						memcpy(s+19*32,s+20*32,32);
						memcpy(s+20*32,s+10*32,32);
						memcpy(s+10*32,swap,32);
						memcpy(swap,s+7*32,32);
						memcpy(s+7*32,s+14*32,32);
						memcpy(s+14*32,swap,32);
						memcpy(swap,s+9*32,32);
						memcpy(s+9*32,s+15*32,32);
						memcpy(s+15*32,s+18*32,32);
						memcpy(s+18*32,swap,32);
					}
					block_steps=1;
				}
				else if(block_steps==13)			// not used yet
				{
					block_offset/=2;
					for(i=0;i<blocks;i++)
					{
						s=PL1+block_offset*i;
						memcpy(swap,s+1*32,32);
						memcpy(s+1*32,s+13*32,32);
						memcpy(s+13*32,s+19*32,32);
						memcpy(s+19*32,s+22*32,32);
						memcpy(s+22*32,s+11*32,32);
						memcpy(s+11*32,s+18*32,32);
						memcpy(s+18*32,s+9*32,32);
						memcpy(s+9*32,s+17*32,32);
						memcpy(s+17*32,s+21*32,32);
						memcpy(s+21*32,s+23*32,32);
						memcpy(s+23*32,s+24*32,32);
						memcpy(s+24*32,s+12*32,32);
						memcpy(s+12*32,s+6*32,32);
						memcpy(s+6*32,s+3*32,32);
						memcpy(s+3*32,s+14*32,32);
						memcpy(s+14*32,s+7*32,32);
						memcpy(s+7*32,s+16*32,32);
						memcpy(s+16*32,s+8*32,32);
						memcpy(s+8*32,s+4*32,32);
						memcpy(s+4*32,s+2*32,32);
						memcpy(s+2*32,swap,32);
						memcpy(swap,s+5*32,32);
						memcpy(s+5*32,s+15*32,32);
						memcpy(s+15*32,s+20*32,32);
						memcpy(s+20*32,s+10*32,32);
						memcpy(s+10*32,swap,32);

						s=PL2+block_offset*i;
						memcpy(swap,s+1*32,32);
						memcpy(s+1*32,s+13*32,32);
						memcpy(s+13*32,s+19*32,32);
						memcpy(s+19*32,s+22*32,32);
						memcpy(s+22*32,s+11*32,32);
						memcpy(s+11*32,s+18*32,32);
						memcpy(s+18*32,s+9*32,32);
						memcpy(s+9*32,s+17*32,32);
						memcpy(s+17*32,s+21*32,32);
						memcpy(s+21*32,s+23*32,32);
						memcpy(s+23*32,s+24*32,32);
						memcpy(s+24*32,s+12*32,32);
						memcpy(s+12*32,s+6*32,32);
						memcpy(s+6*32,s+3*32,32);
						memcpy(s+3*32,s+14*32,32);
						memcpy(s+14*32,s+7*32,32);
						memcpy(s+7*32,s+16*32,32);
						memcpy(s+16*32,s+8*32,32);
						memcpy(s+8*32,s+4*32,32);
						memcpy(s+4*32,s+2*32,32);
						memcpy(s+2*32,swap,32);
						memcpy(swap,s+5*32,32);
						memcpy(s+5*32,s+15*32,32);
						memcpy(s+15*32,s+20*32,32);
						memcpy(s+20*32,s+10*32,32);
						memcpy(s+10*32,swap,32);
					}
					block_steps=1;
				}
			} while(block_steps>1);
		}
	}

#ifdef CACHE_PENUSAGE
{
	FILE *fp;
    char text[40];

	sprintf(text,"%s.p1",Machine->gamedrv->name);
    fp=fopen(text,"rb");
    if (fp) {
    	fread(Machine->gfx[2]->pen_usage,tiles * sizeof(int), 1 ,fp);
    	fclose(fp);
        return common_vh_start();
    }
}
#endif

    for (i=0; i<tiles/2; i++) {
//    	Machine->gfx[2]->pen_usage[i] = 0;
		pen=0;

		s=swap;
		dest=PL1;

		for (y = 0;y < 16;y++)
		{
        	p1=*(PL1+16);
        	p2=*(PL1+32+16);
       	 	p3=*(PL1+64+16);
        	p4=*(PL1+96+16);

			pen |= 1 << (s[0]=GET_PIX0());
			pen |= 1 << (s[1]=GET_PIX1());
			pen |= 1 << (s[2]=GET_PIX2());
			pen |= 1 << (s[3]=GET_PIX3());
			pen |= 1 << (s[4]=GET_PIX4());
			pen |= 1 << (s[5]=GET_PIX5());
			pen |= 1 << (s[6]=GET_PIX6());
			pen |= 1 << (s[7]=GET_PIX7());

			p1=*(PL1);
        	p2=*(PL1+32);
       	 	p3=*(PL1+64);
        	p4=*(PL1+96);
	        PL1+=1;

			pen |= 1 <<(mybyte=GET_PIX0());s[0]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX1());s[1]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX2());s[2]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX3());s[3]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX4());s[4]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX5());s[5]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX6());s[6]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX7());s[7]+=mybyte<<4;

			s+=8;

    	}
		pen_usage[i]=pen;
		PL1+=32+64+16;

		memcpy(dest,swap,128);
    }
    for (i=tiles/2; i<tiles; i++) {
//    	Machine->gfx[2]->pen_usage[i] = 0;
		pen=0;

		s=swap;
		dest=PL2;

		for (y = 0;y < 16;y++)
		{
        	p1=*(PL2+16);
        	p2=*(PL2+32+16);
       	 	p3=*(PL2+64+16);
        	p4=*(PL2+96+16);

			pen |= 1 << (s[0]=GET_PIX0());
			pen |= 1 << (s[1]=GET_PIX1());
			pen |= 1 << (s[2]=GET_PIX2());
			pen |= 1 << (s[3]=GET_PIX3());
			pen |= 1 << (s[4]=GET_PIX4());
			pen |= 1 << (s[5]=GET_PIX5());
			pen |= 1 << (s[6]=GET_PIX6());
			pen |= 1 << (s[7]=GET_PIX7());

			p1=*(PL2);
        	p2=*(PL2+32);
       	 	p3=*(PL2+64);
        	p4=*(PL2+96);
	        PL2+=1;

			pen |= 1 <<(mybyte=GET_PIX0());s[0]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX1());s[1]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX2());s[2]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX3());s[3]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX4());s[4]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX5());s[5]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX6());s[6]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX7());s[7]+=mybyte<<4;

			s+=8;

    	}
		pen_usage[i]=pen;
		PL2+=32+64+16;

		memcpy(dest,swap,128);
    }

    if (errorlog) fprintf(errorlog,"Calculated pen usage for %d tiles\n",tiles);

#ifdef CACHE_PENUSAGE
{
	FILE *fp;
    char text[40];

	sprintf(text,"%s.p1",Machine->gamedrv->name);
    fp=fopen(text,"wb");
    fwrite(Machine->gfx[2]->pen_usage,tiles * sizeof(int), 1 ,fp);
    fclose(fp);
}
#endif

	#undef GET_PIX
	return common_vh_start();
}

/* MVS cartidges */
int neogeo_mvs_vh_start(void)
{
	int i,x,y,tiles,p1,p2,p3,p4;
    unsigned char mybyte;
	unsigned char *PL1 = Machine->memory_region[2];
    unsigned char *PL2 = Machine->memory_region[3];

   	unsigned char *dest,*s;
	unsigned char swap[128];
	unsigned char swap2[128];
	unsigned int *pen_usage;
	unsigned int pen;

    #ifdef CACHE_GRAPHICS
    int cache_ok=0;
    #endif

    /* For MVS games we need to calculate the pen_usage array ourself */
	if (Machine->gfx[2]->pen_usage)
    	free(Machine->gfx[2]->pen_usage);
    tiles=Machine->memory_region_length[2]/64;
    no_of_tiles=tiles;
    if (no_of_tiles>0x10000) high_tile=1; else high_tile=0;
	if (no_of_tiles>0x20000) vhigh_tile=1; else vhigh_tile=0;
	if (no_of_tiles>0x40000) vvhigh_tile=1; else vvhigh_tile=0;
	if (errorlog && high_tile) fprintf(errorlog,"Using high tile code\n");
	if (errorlog && vhigh_tile) fprintf(errorlog,"Using very high tile code\n");
	if (errorlog && vvhigh_tile) fprintf(errorlog,"Using very very high tile code!\n");
    Machine->gfx[2]->pen_usage=malloc(tiles * sizeof(int));

	pen_usage=Machine->gfx[2]->pen_usage;

#ifdef CACHE_GRAPHICS
{
	FILE *fp;
    char text[40];

	sprintf(text,"%s.g1",Machine->gamedrv->name);
    fp=fopen(text,"rb");
    if (fp) {
    	fread(Machine->memory_region[2],1, Machine->memory_region_length[2] ,fp);
    	fclose(fp);
		if (errorlog) fprintf(errorlog,"Loaded graphics 1 for %d tiles\n",tiles);
    }

	sprintf(text,"%s.g2",Machine->gamedrv->name);
    fp=fopen(text,"rb");
    if (fp && cache_ok==0) {
    	fread(Machine->memory_region[3],1, Machine->memory_region_length[2] ,fp);
    	fclose(fp);
		if (errorlog) fprintf(errorlog,"Loaded graphics 2 for %d tiles\n",tiles);
        cache_ok=1;
    }
}
#endif

/* swap all MVS planes into a single area.
 This would be much quicker if the interleaving was done on loading.
 */
#ifdef CACHE_GRAPHICS
if (cache_ok==0)
#endif
	{
		int block_size,block_count,block_start,block_offset,block_steps,blocks;
		int old_steps;
		int block_tiles=no_of_tiles*2;
		int j,k;
		unsigned char *d;

		block_size=block_tiles*128/4/2;
		block_steps=block_tiles/4;
		block_offset=block_tiles*128/2;

		block_start=block_size;

		s=PL1+block_start;
		d=PL2;
		blocks=1;

		for(i=0;i<block_steps;i++)
		{
			memcpy(swap,s,64);
			memcpy(s,d,64);
			memcpy(d,swap,64);
			s+=64;
			d+=64;
		}

		do
		{
			block_size/=2;
			block_steps/=2;
			block_start/=2;
			block_offset/=2;
			blocks*=2;
			for(i=0;i<blocks/2;i++)
			{
				s=PL1+block_start+block_offset*i;
				d=PL1+block_start+block_offset*i+block_size;
				for(j=0;j<block_steps;j++)
				{
					memcpy(swap,s,64);
					memcpy(s,d,64);
					memcpy(d,swap,64);
					s+=64;
					d+=64;
				}

				s=PL2+block_start+block_offset*i;
				d=PL2+block_start+block_offset*i+block_size;
				for(j=0;j<block_steps;j++)
				{
					memcpy(swap,s,64);
					memcpy(s,d,64);
					memcpy(d,swap,64);
					s+=64;
					d+=64;
				}
			}
			if(block_steps==3)
			{
				block_offset/=2;
				for(i=0;i<blocks;i++)
				{
					s=PL1+block_offset*i;
					memcpy(swap,s+1*64,64);
					memcpy(s+1*64,s+3*64,64);
					memcpy(swap2,s+2*64,64);
					memcpy(s+2*64,swap,64);
					memcpy(s+3*64,s+4*64,64);
					memcpy(s+4*64,swap2,64);

					s=PL2+block_offset*i;
					memcpy(swap,s+1*64,64);
					memcpy(s+1*64,s+3*64,64);
					memcpy(swap2,s+2*64,64);
					memcpy(s+2*64,swap,64);
					memcpy(s+3*64,s+4*64,64);
					memcpy(s+4*64,swap2,64);
				}
				block_steps=1;
			}
			else if(block_steps==5)
			{
				block_offset/=2;
				for(i=0;i<blocks;i++)
				{
					s=PL1+block_offset*i;
					memcpy(swap,s+1*64,64);
					memcpy(s+1*64,s+5*64,64);
					memcpy(s+5*64,s+7*64,64);
					memcpy(s+7*64,s+8*64,64);
					memcpy(s+8*64,s+4*64,64);
					memcpy(s+4*64,s+2*64,64);
					memcpy(s+2*64,swap,64);
					memcpy(swap,s+3*64,64);
					memcpy(s+3*64,s+6*64,64);
					memcpy(s+6*64,swap,64);

					s=PL2+block_offset*i;
					memcpy(swap,s+1*64,64);
					memcpy(s+1*64,s+5*64,64);
					memcpy(s+5*64,s+7*64,64);
					memcpy(s+7*64,s+8*64,64);
					memcpy(s+8*64,s+4*64,64);
					memcpy(s+4*64,s+2*64,64);
					memcpy(s+2*64,swap,64);
					memcpy(swap,s+3*64,64);
					memcpy(s+3*64,s+6*64,64);
					memcpy(s+6*64,swap,64);
				}
				block_steps=1;
			}
			else if(block_steps==7)
			{
				block_offset/=2;
				for(i=0;i<blocks;i++)
				{
					s=PL1+block_offset*i;
					memcpy(swap,s+1*64,64);
					memcpy(s+1*64,s+7*64,64);
					memcpy(s+7*64,s+10*64,64);
					memcpy(s+10*64,s+5*64,64);
					memcpy(s+5*64,s+9*64,64);
					memcpy(s+9*64,s+11*64,64);
					memcpy(s+11*64,s+12*64,64);
					memcpy(s+12*64,s+6*64,64);
					memcpy(s+6*64,s+3*64,64);
					memcpy(s+3*64,s+8*64,64);
					memcpy(s+8*64,s+4*64,64);
					memcpy(s+4*64,s+2*64,64);
					memcpy(s+2*64,swap,64);

					s=PL2+block_offset*i;
					memcpy(swap,s+1*64,64);
					memcpy(s+1*64,s+7*64,64);
					memcpy(s+7*64,s+10*64,64);
					memcpy(s+10*64,s+5*64,64);
					memcpy(s+5*64,s+9*64,64);
					memcpy(s+9*64,s+11*64,64);
					memcpy(s+11*64,s+12*64,64);
					memcpy(s+12*64,s+6*64,64);
					memcpy(s+6*64,s+3*64,64);
					memcpy(s+3*64,s+8*64,64);
					memcpy(s+8*64,s+4*64,64);
					memcpy(s+4*64,s+2*64,64);
					memcpy(s+2*64,swap,64);
				}
				block_steps=1;
			}
			else if(block_steps==9)
			{
				block_offset/=2;
				for(i=0;i<blocks;i++)
				{
					s=PL1+block_offset*i;
					memcpy(swap,s+1*64,64);
					memcpy(s+1*64,s+9*64,64);
					memcpy(s+9*64,s+13*64,64);
					memcpy(s+13*64,s+15*64,64);
					memcpy(s+15*64,s+16*64,64);
					memcpy(s+16*64,s+8*64,64);
					memcpy(s+8*64,s+4*64,64);
					memcpy(s+4*64,s+2*64,64);
					memcpy(s+2*64,swap,64);
					memcpy(swap,s+3*64,64);
					memcpy(s+3*64,s+10*64,64);
					memcpy(s+10*64,s+5*64,64);
					memcpy(s+5*64,s+11*64,64);
					memcpy(s+11*64,s+14*64,64);
					memcpy(s+14*64,s+7*64,64);
					memcpy(s+7*64,s+12*64,64);
					memcpy(s+12*64,s+6*64,64);
					memcpy(s+6*64,swap,64);

					s=PL2+block_offset*i;
					memcpy(swap,s+1*64,64);
					memcpy(s+1*64,s+9*64,64);
					memcpy(s+9*64,s+13*64,64);
					memcpy(s+13*64,s+15*64,64);
					memcpy(s+15*64,s+16*64,64);
					memcpy(s+16*64,s+8*64,64);
					memcpy(s+8*64,s+4*64,64);
					memcpy(s+4*64,s+2*64,64);
					memcpy(s+2*64,swap,64);
					memcpy(swap,s+3*64,64);
					memcpy(s+3*64,s+10*64,64);
					memcpy(s+10*64,s+5*64,64);
					memcpy(s+5*64,s+11*64,64);
					memcpy(s+11*64,s+14*64,64);
					memcpy(s+14*64,s+7*64,64);
					memcpy(s+7*64,s+12*64,64);
					memcpy(s+12*64,s+6*64,64);
					memcpy(s+6*64,swap,64);
				}
				block_steps=1;
			}
			else if(block_steps==11)			// not used yet
			{
				block_offset/=2;
				for(i=0;i<blocks;i++)
				{
					s=PL1+block_offset*i;
					memcpy(swap,s+1*64,64);
					memcpy(s+1*64,s+11*64,64);
					memcpy(s+11*64,s+16*64,64);
					memcpy(s+16*64,s+8*64,64);
					memcpy(s+8*64,s+4*64,64);
					memcpy(s+4*64,s+2*64,64);
					memcpy(s+2*64,swap,64);
					memcpy(swap,s+3*64,64);
					memcpy(s+3*64,s+12*64,64);
					memcpy(s+12*64,s+6*64,64);
					memcpy(s+6*64,swap,64);
					memcpy(swap,s+5*64,64);
					memcpy(s+5*64,s+13*64,64);
					memcpy(s+13*64,s+17*64,64);
					memcpy(s+17*64,s+19*64,64);
					memcpy(s+19*64,s+20*64,64);
					memcpy(s+20*64,s+10*64,64);
					memcpy(s+10*64,swap,64);
					memcpy(swap,s+7*64,64);
					memcpy(s+7*64,s+14*64,64);
					memcpy(s+14*64,swap,64);
					memcpy(swap,s+9*64,64);
					memcpy(s+9*64,s+15*64,64);
					memcpy(s+15*64,s+18*64,64);
					memcpy(s+18*64,swap,64);

					s=PL2+block_offset*i;
					memcpy(swap,s+1*64,64);
					memcpy(s+1*64,s+11*64,64);
					memcpy(s+11*64,s+16*64,64);
					memcpy(s+16*64,s+8*64,64);
					memcpy(s+8*64,s+4*64,64);
					memcpy(s+4*64,s+2*64,64);
					memcpy(s+2*64,swap,64);
					memcpy(swap,s+3*64,64);
					memcpy(s+3*64,s+12*64,64);
					memcpy(s+12*64,s+6*64,64);
					memcpy(s+6*64,swap,64);
					memcpy(swap,s+5*64,64);
					memcpy(s+5*64,s+13*64,64);
					memcpy(s+13*64,s+17*64,64);
					memcpy(s+17*64,s+19*64,64);
					memcpy(s+19*64,s+20*64,64);
					memcpy(s+20*64,s+10*64,64);
					memcpy(s+10*64,swap,64);
					memcpy(swap,s+7*64,64);
					memcpy(s+7*64,s+14*64,64);
					memcpy(s+14*64,swap,64);
					memcpy(swap,s+9*64,64);
					memcpy(s+9*64,s+15*64,64);
					memcpy(s+15*64,s+18*64,64);
					memcpy(s+18*64,swap,64);
				}
				block_steps=1;
			}
			else if(block_steps==13)
			{
				block_offset/=2;
				for(i=0;i<blocks;i++)
				{
					s=PL1+block_offset*i;
					memcpy(swap,s+1*64,64);
					memcpy(s+1*64,s+13*64,64);
					memcpy(s+13*64,s+19*64,64);
					memcpy(s+19*64,s+22*64,64);
					memcpy(s+22*64,s+11*64,64);
					memcpy(s+11*64,s+18*64,64);
					memcpy(s+18*64,s+9*64,64);
					memcpy(s+9*64,s+17*64,64);
					memcpy(s+17*64,s+21*64,64);
					memcpy(s+21*64,s+23*64,64);
					memcpy(s+23*64,s+24*64,64);
					memcpy(s+24*64,s+12*64,64);
					memcpy(s+12*64,s+6*64,64);
					memcpy(s+6*64,s+3*64,64);
					memcpy(s+3*64,s+14*64,64);
					memcpy(s+14*64,s+7*64,64);
					memcpy(s+7*64,s+16*64,64);
					memcpy(s+16*64,s+8*64,64);
					memcpy(s+8*64,s+4*64,64);
					memcpy(s+4*64,s+2*64,64);
					memcpy(s+2*64,swap,64);
					memcpy(swap,s+5*64,64);
					memcpy(s+5*64,s+15*64,64);
					memcpy(s+15*64,s+20*64,64);
					memcpy(s+20*64,s+10*64,64);
					memcpy(s+10*64,swap,64);

					s=PL2+block_offset*i;
					memcpy(swap,s+1*64,64);
					memcpy(s+1*64,s+13*64,64);
					memcpy(s+13*64,s+19*64,64);
					memcpy(s+19*64,s+22*64,64);
					memcpy(s+22*64,s+11*64,64);
					memcpy(s+11*64,s+18*64,64);
					memcpy(s+18*64,s+9*64,64);
					memcpy(s+9*64,s+17*64,64);
					memcpy(s+17*64,s+21*64,64);
					memcpy(s+21*64,s+23*64,64);
					memcpy(s+23*64,s+24*64,64);
					memcpy(s+24*64,s+12*64,64);
					memcpy(s+12*64,s+6*64,64);
					memcpy(s+6*64,s+3*64,64);
					memcpy(s+3*64,s+14*64,64);
					memcpy(s+14*64,s+7*64,64);
					memcpy(s+7*64,s+16*64,64);
					memcpy(s+16*64,s+8*64,64);
					memcpy(s+8*64,s+4*64,64);
					memcpy(s+4*64,s+2*64,64);
					memcpy(s+2*64,swap,64);
					memcpy(swap,s+5*64,64);
					memcpy(s+5*64,s+15*64,64);
					memcpy(s+15*64,s+20*64,64);
					memcpy(s+20*64,s+10*64,64);
					memcpy(s+10*64,swap,64);
				}
				block_steps=1;
			}

		} while(block_steps>1);
	}

 	/* Calculate pen usage */

#ifdef CACHE_PENUSAGE
{
	FILE *fp;
    char text[40];

	sprintf(text,"%s.p1",Machine->gamedrv->name);
    fp=fopen(text,"rb");
    if (fp) {
    	fread(Machine->gfx[2]->pen_usage,sizeof(int), tiles ,fp);
    	fclose(fp);
		if (errorlog) fprintf(errorlog,"Loaded pen usage for %d tiles\n",tiles);
        return common_vh_start();
    }
}
#endif

    for (i=0; i<tiles/2; i++) {
		pen=0;

		s=swap;
		dest=PL1;

		for (y = 0;y < 16;y++)
		{
        	p1=*(PL1+32);
        	p2=*(PL1+33);
       	 	p3=*(PL1+32+64);
        	p4=*(PL1+33+64);

			pen |= 1 << (s[0]=GET_PIX0());
			pen |= 1 << (s[1]=GET_PIX1());
			pen |= 1 << (s[2]=GET_PIX2());
			pen |= 1 << (s[3]=GET_PIX3());
			pen |= 1 << (s[4]=GET_PIX4());
			pen |= 1 << (s[5]=GET_PIX5());
			pen |= 1 << (s[6]=GET_PIX6());
			pen |= 1 << (s[7]=GET_PIX7());

			p1=*(PL1);
			p2=*(PL1+1);
			p3=*(PL1+64);
			p4=*(PL1+1+64);
	        PL1+=2;

			pen |= 1 <<(mybyte=GET_PIX0());s[0]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX1());s[1]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX2());s[2]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX3());s[3]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX4());s[4]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX5());s[5]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX6());s[6]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX7());s[7]+=mybyte<<4;

			s+=8;
    	}
		PL1+=32+64;
		pen_usage[i]=pen;

		memcpy(dest,swap,128);

    }
    for (i=tiles/2; i<tiles; i++) {
		pen=0;

		s=swap;
		dest=PL2;

		for (y = 0;y < 16;y++)
		{
        	p1=*(PL2+32);
        	p2=*(PL2+33);
       	 	p3=*(PL2+32+64);
        	p4=*(PL2+33+64);

			pen |= 1 << (s[0]=GET_PIX0());
			pen |= 1 << (s[1]=GET_PIX1());
			pen |= 1 << (s[2]=GET_PIX2());
			pen |= 1 << (s[3]=GET_PIX3());
			pen |= 1 << (s[4]=GET_PIX4());
			pen |= 1 << (s[5]=GET_PIX5());
			pen |= 1 << (s[6]=GET_PIX6());
			pen |= 1 << (s[7]=GET_PIX7());

			p1=*(PL2);
			p2=*(PL2+1);
			p3=*(PL2+64);
			p4=*(PL2+1+64);
	        PL2+=2;

			pen |= 1 <<(mybyte=GET_PIX0());s[0]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX1());s[1]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX2());s[2]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX3());s[3]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX4());s[4]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX5());s[5]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX6());s[6]+=mybyte<<4;
			pen |= 1 <<(mybyte=GET_PIX7());s[7]+=mybyte<<4;

			s+=8;
    	}
		PL2+=32+64;
		pen_usage[i]=pen;

		memcpy(dest,swap,128);
    }

    if (errorlog) fprintf(errorlog,"Calculated pen usage for %d tiles\n",tiles);

#ifdef CACHE_PENUSAGE
{
	FILE *fp;
    char text[40];

	sprintf(text,"%s.p1",Machine->gamedrv->name);
    fp=fopen(text,"wb");
    fwrite(Machine->gfx[2]->pen_usage,sizeof(int), tiles ,fp);
    fclose(fp);
}
#endif

#ifdef CACHE_GRAPHICS
{
	FILE *fp;
    char text[40];

	sprintf(text,"%s.g1",Machine->gamedrv->name);
    fp=fopen(text,"wb");
    fwrite(Machine->memory_region[2], 1, Machine->memory_region_length[2] ,fp);
    fclose(fp);

	sprintf(text,"%s.g2",Machine->gamedrv->name);
    fp=fopen(text,"wb");
    fwrite(Machine->memory_region[3], 1, Machine->memory_region_length[2] ,fp);
    fclose(fp);
}
#endif

	return common_vh_start();
}
