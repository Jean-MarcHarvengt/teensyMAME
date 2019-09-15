/***************************************************************************

 Pang Video Hardware

***************************************************************************/

#include "vidhrdw/generic.h"
#include "palette.h"

/* Constants */
const int pang_paletteram_size=0x1000;
#define pang_max_colors 0x80

/* Globals */
int pang_videoram_size;
int pang_colorram_size;
unsigned char * pang_videoram;
unsigned char * pang_colorram;

/* Private */
static struct osd_bitmap *pang_tmpbitmap;    /* Temp bitmap */

static unsigned char *pang_objram;           /* Sprite RAM */
static unsigned char *pang_color_dirty;      /* Color RAM dirty */

static unsigned char *pang_video_base;       /* Video RAM base ptr (banked) */
static unsigned char *pang_paletteram;       /* Palette RAM (banked) */
static unsigned char *pang_palette_base;
static unsigned char *pang_palette_dirty;    /* Palette RAM dirty (banked) */
static unsigned char *pang_palette_dirty_base;
static unsigned char *pang_video_dirty;      /* Video RAM dirty (banked) */
static unsigned char *pang_video_dirty_base;
static unsigned char pang_dirty_colors[pang_max_colors]; /* Color dirty (banked) */
static unsigned char *pang_dirty_colors_base;

/* Declarations */
void pang_vh_stop(void);


/***************************************************************************
  OBJ / CHAR RAM HANDLERS (BANK 0 = CHAR, BANK 1=OBJ)
***************************************************************************/

void pang_video_bank_w(int offset, int data)
{
	/* Bank handler (sets base pointers for video write) */
	if (data)
	{
		/* OBJ RAM write */
		pang_video_base=pang_objram;
		/* Dummy dirty buffer for obj RAM */
		pang_video_dirty_base=pang_video_dirty+pang_videoram_size;
	}
	else
	{
		/* Character RAM write */
		pang_video_base=pang_videoram;
		pang_video_dirty_base=pang_video_dirty;
	}
}

void pang_videoram_w(int offset, int data)
{
	pang_video_base[offset]=data;
	pang_video_dirty_base[offset]=1;
}

int pang_videoram_r(int offset)
{
	return pang_video_base[offset];
}

/***************************************************************************
  COLOUR RAM
****************************************************************************/

void pang_colorram_w(int offset, int data)
{
	pang_colorram[offset]=data;
	pang_color_dirty[offset]=1;
}

int pang_colorram_r(int offset)
{
	return pang_colorram[offset];
}

/***************************************************************************
  PALETTE HANDLERS (COLOURS: BANK 0 = 0x00-0x3f BANK 1=0x40-0xff)
****************************************************************************/
void pang_palette_bank_w(int offset, int data)
{
	/* Palette bank handlers (sets base pointers form memory handlers) */
	if (data == 0x28)
	{
		pang_palette_base=&pang_paletteram[0x0800];
		pang_palette_dirty_base=&pang_palette_dirty[0x0800];
		pang_dirty_colors_base=&pang_dirty_colors[0x40];
	}
	else
	{
		pang_palette_base=pang_paletteram;
		pang_palette_dirty_base=pang_palette_dirty;
		pang_dirty_colors_base=pang_dirty_colors;
	}
}

void pang_paletteram_w(int offset, int data)
{
	pang_palette_base[offset]=data;
	pang_palette_dirty_base[offset]=1;
	pang_dirty_colors_base[offset/32]=1;
}

int pang_paletteram_r(int offset)
{
	return pang_palette_base[offset];
}

/***************************************************************************
		Video init
***************************************************************************/

int pang_vh_start(void)
{
	pang_tmpbitmap=NULL;
	pang_video_dirty=NULL;
	pang_color_dirty=NULL;
	pang_objram=NULL;
	pang_paletteram=NULL;
	pang_palette_dirty=NULL;

	pang_tmpbitmap = osd_create_bitmap(0x40*8, 0x20*8);
	if (!pang_tmpbitmap)
	{
		pang_vh_stop();
		return 1;
	}

	/*
		Video RAM dirty buffer : NB The video ram memory handler
		also writes sprite data (dirty buffer has to be
		2xpang_videoram_size)
	*/
	pang_video_dirty=malloc(pang_videoram_size*2);
	if (!pang_video_dirty)
	{
		pang_vh_stop();
		return 1;
	}
	memset(pang_video_dirty, 1, pang_videoram_size);
	pang_video_dirty_base=pang_video_dirty;

	/*
		OBJ RAM
	*/
	pang_objram=malloc(pang_videoram_size);
	if (!pang_objram)
	{
		pang_vh_stop();
		return 1;
	}
	memset(pang_objram, 0, pang_videoram_size);

	/*
		Char colour dirty buffer
	*/
	pang_color_dirty=malloc(pang_colorram_size);
	if (!pang_color_dirty)
	{
		pang_vh_stop();
		return 1;
	}
	memset(pang_color_dirty, 1, pang_colorram_size);

	/*
		Palette RAM
	*/
	pang_paletteram=malloc(pang_paletteram_size);
	if (!pang_paletteram)
	{
		pang_vh_stop();
		return 1;
	}
	memset(pang_paletteram, 0, pang_paletteram_size);

	/*
		Palette dirty buffer
	*/
	pang_palette_dirty=malloc(pang_paletteram_size);
	if (!pang_palette_dirty)
	{
		return 1;
	}
	memset(pang_palette_dirty, 1, pang_paletteram_size);

	/* Set up default bank base pointers */
	pang_palette_bank_w(0,0);
	pang_video_bank_w(0,0);


	return 0;
}

void pang_vh_stop(void)
{
	if (pang_tmpbitmap)
		osd_free_bitmap(pang_tmpbitmap);
	if (pang_video_dirty)
		free(pang_video_dirty);
	if (pang_color_dirty)
		free(pang_color_dirty);
	if (pang_objram)
		free(pang_objram);
	if (pang_paletteram)
		free(pang_paletteram);
	if (pang_palette_dirty)
		free(pang_palette_dirty);
}

/***************************************************************************
 Palette refresh
***************************************************************************/
INLINE void pang_refresh_palette( void )
{
	/* rebuild the color lookup table from RAM palette */
	int offs, i;
	unsigned char pang_used_colors[pang_max_colors];

	/* Rebuild any dirty palette entries */
	for( offs=0; offs<pang_max_colors*16*2; offs+=2 )
	{
		if (pang_palette_dirty[offs] || pang_palette_dirty[offs+1])
		{
			int bg, r, red, green, blue, bright;
			pang_palette_dirty[offs]=pang_palette_dirty[offs+1]=0;
			bg=pang_paletteram[offs];
			r =pang_paletteram[offs+1];
			red   = (r&0x0f);
			green = ((bg>>4)&0x0f);
			blue  = (bg&0x0f);
			red   = red<<4 | red;
			green = green<<4 | green;
			blue  = blue<<4 | blue;
			palette_change_color(offs/2, red, green, blue);
		}
	}

	/* Work out what colors we are actually using */
	memset(pang_used_colors,0, sizeof(pang_used_colors));
	memset(palette_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors * sizeof(unsigned char));

	/* Characters */
	for (i=0; i<pang_colorram_size;i++)
	{
		pang_used_colors[pang_colorram[i]&0x7f]=1;
	}
	/* Sprites */
	for (i=1; i<pang_videoram_size; i+=4)
	{
		pang_used_colors[pang_objram[i]&0x0f]=1;
	}
	for (i=0; i<pang_max_colors; i++)
	{
		if (pang_used_colors[i])
		{
			memset(palette_used_colors+i*16, PALETTE_COLOR_USED, 16);
			palette_used_colors[i*16+15]=PALETTE_COLOR_TRANSPARENT;
		}
	}
	if (palette_recalc())
	{
		/* Whole screen needs a refresh */
		memset(pang_color_dirty, 1, pang_colorram_size);
	}
}

/***************************************************************************
  screen update
***************************************************************************/
void pang_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs, offsclr, sx, sy;

	/* Refresh the palette */
	pang_refresh_palette();

	/* Draw chars to temporary bitmap */
	offs=0;
	offsclr=0;
	for (sy=0; sy<0x20; sy++)
	{
		for (sx=0; sx<0x40; sx++)
		{
			int attrib=pang_colorram[offsclr];
			int color=attrib & 0x7f;
			if (    pang_video_dirty[offs] ||
					pang_video_dirty[offs+1] ||
					pang_color_dirty[offsclr] ||
					pang_dirty_colors[color] )
			{
					int code=pang_videoram[offs] +
							 pang_videoram[offs+1]*256;

					pang_video_dirty[offs]=0;
					pang_video_dirty[offs+1]=0;
					pang_color_dirty[offsclr]=0;

					drawgfx(pang_tmpbitmap,Machine->gfx[0],
							code,
							color,
							attrib & 0x80,0,
							8*sx,8*sy,
							NULL,
							TRANSPARENCY_NONE,0);
				}
				offs+=2;
				offsclr++;
		}
	}
	memset(pang_dirty_colors, 0, sizeof(pang_dirty_colors));

	copybitmap(bitmap,pang_tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Draw sprites */
	for (offs=0x1000-0x20; offs >=0 ; offs-=0x20)
	{
		sy=pang_objram[offs+2];
		if ( sy )
		{
			int code = pang_objram[offs];
			int attr = pang_objram[offs+1];
			int color=attr & 0x0f;
			sx=pang_objram[offs+3]+((attr&0x10)<<4);
			code+=(attr&0xe0)<<3;
			drawgfx(bitmap,Machine->gfx[1],
					 code,
					 color,
					 0,0,
					 sx,sy,
					 &Machine->drv->visible_area,
					 TRANSPARENCY_PEN,15);
		}
	}
}

