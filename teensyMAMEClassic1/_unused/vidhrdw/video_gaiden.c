/***************************************************************************


  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *gaiden_videoram;
unsigned char *gaiden_spriteram;
unsigned char *gaiden_videoram2;
unsigned char *gaiden_videoram3;

void gaiden_vh_stop (void);

int gaiden_videoram_size;
int gaiden_spriteram_size;
int gaiden_videoram2_size;
int gaiden_videoram3_size;

static struct osd_bitmap *tmpbitmap2;
static struct osd_bitmap *tmpbitmap3;

static unsigned char *gaiden_dirty2;
static unsigned char *gaiden_dirty3;

unsigned char *gaiden_txscrollx,*gaiden_txscrolly;
unsigned char *gaiden_fgscrollx,*gaiden_fgscrolly;
unsigned char *gaiden_bgscrollx,*gaiden_bgscrolly;




/*
 *   video system start
 */

int gaiden_vh_start (void)
{
	/* Allocate a video RAM */
	gaiden_dirty2 = malloc ( gaiden_videoram2_size/4);
	if (!gaiden_dirty2)
	{
		gaiden_vh_stop();
		return 1;
	}
	memset(gaiden_dirty2,1,gaiden_videoram2_size / 4);

	gaiden_dirty3 = malloc ( gaiden_videoram3_size/4);
	if (!gaiden_dirty3)
	{
		gaiden_vh_stop();
		return 1;
	}
	memset(gaiden_dirty3,1,gaiden_videoram3_size / 4);

	/* Allocate temporary bitmaps */
 	if ((tmpbitmap2 = osd_create_bitmap(1024,512)) == 0)
	{
		gaiden_vh_stop ();
		return 1;
	}
	if ((tmpbitmap3 = osd_create_bitmap(1024,512)) == 0)
	{
		gaiden_vh_stop ();
		return 1;
	}

	/* 0x200 is the background color */
	palette_transparent_color = 0x200;

	return 0;
}


/*
 *   video system shutdown; we also bring down the system memory as well here
 */

void gaiden_vh_stop (void)
{
	/* Free temporary bitmaps */
	if (tmpbitmap3)
		osd_free_bitmap (tmpbitmap3);
	tmpbitmap3 = 0;
	if (tmpbitmap2)
		osd_free_bitmap (tmpbitmap2);
	tmpbitmap2 = 0;

	/* Free video RAM */
	if (gaiden_dirty2)
	        free (gaiden_dirty2);
	if (gaiden_dirty3)
	        free (gaiden_dirty3);
	gaiden_dirty2 = gaiden_dirty3 = 0;
}



/*
 *   scroll write handlers
 */

void gaiden_txscrollx_w(int offset,int data)
{
	COMBINE_WORD_MEM (&gaiden_txscrollx[offset], data);
}
void gaiden_txscrolly_w(int offset,int data)
{
	COMBINE_WORD_MEM (&gaiden_txscrolly[offset], data);
}
void gaiden_fgscrollx_w(int offset,int data)
{
	COMBINE_WORD_MEM (&gaiden_fgscrollx[offset], data);
}
void gaiden_fgscrolly_w(int offset,int data)
{
	COMBINE_WORD_MEM (&gaiden_fgscrolly[offset], data);
}
void gaiden_bgscrollx_w(int offset,int data)
{
	COMBINE_WORD_MEM (&gaiden_bgscrollx[offset], data);
}
void gaiden_bgscrolly_w(int offset,int data)
{
	COMBINE_WORD_MEM (&gaiden_bgscrolly[offset], data);
}



void gaiden_videoram3_w (int offset, int data)
{
	int oldword = READ_WORD(&gaiden_videoram3[offset]);
	int newword = COMBINE_WORD(oldword,data);


	if (oldword != newword)
	{
		WRITE_WORD(&gaiden_videoram3[offset],newword);
		gaiden_dirty3[(offset & 0x0fff) / 2] = 1;
	}
}
int gaiden_videoram3_r (int offset)
{
   return READ_WORD (&gaiden_videoram3[offset]);
}

void gaiden_videoram2_w (int offset, int data)
{
	int oldword = READ_WORD(&gaiden_videoram2[offset]);
	int newword = COMBINE_WORD(oldword,data);


	if (oldword != newword)
	{
		WRITE_WORD(&gaiden_videoram2[offset],newword);
		gaiden_dirty2[(offset & 0x0fff) / 2] = 1;
	}
}
int gaiden_videoram2_r (int offset)
{
   return READ_WORD (&gaiden_videoram2[offset]);
}

void gaiden_videoram_w (int offset, int data)
{
	COMBINE_WORD_MEM(&gaiden_videoram[offset],data);
}

int gaiden_videoram_r (int offset)
{
   return READ_WORD (&gaiden_videoram[offset]);
}

void gaiden_spriteram_w (int offset, int data)
{
	COMBINE_WORD_MEM (&gaiden_spriteram[offset], data);
}

int gaiden_spriteram_r (int offset)
{
   return READ_WORD (&gaiden_spriteram[offset]);
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
/*
 *  some ideas from tecmo.c
 *  (sprites in separate function and handling of 16x16 sprites)
 */

void gaiden_drawsprites(struct osd_bitmap *bitmap, int priority)
{
    /*
     * Sprite Format
     * ------------------
     *
     * Byte(s) | Bit(s)   | Use
     * --------+-76543210-+----------------
     *    0    | -------- | unknown
     *    1    | -------x | flip x
     *    1    | ------x- | flip y
     *    1    | -----x-- | active (show this sprite)
     *    1    | x------- | occluded (sprite is behind front background layer
     *    2    | xxxxxxxx | Sprite Number (High 8 bits)
     *    3    | xxxx---- | Sprite Number (Low 4 bits)
     *    3    | ----xx-- | Additional Index (for small sprites)
     *    4    | -------- | unknown
     *    5    | ------xx | size 1=16x16, 2=32x32;
     *    5    | xxxx---- | palette
     *    6    | xxxxxxxx | y position (high 8 bits)
     *    7    | xxxxxxxx | y position (low 8 bits)
     *    8    | xxxxxxxx | x position (negative of high 8 bits)
     *    9    | -------- | x position (low 8 bits)
     */
        int offs;
	for (offs = 0; offs<0x800 ; offs += 16)
	{
		int sx,sy,col,spr_pri;
		int num,bank,flip,size;
//		sx = -(READ_WORD(&gaiden_spriteram[offs+8])&0xff00)
//		     + (READ_WORD(&gaiden_spriteram[offs+8])&0x00ff);
		sx = READ_WORD(&gaiden_spriteram[offs+8]) & 0x1ff;
		if (sx >= 256) sx -= 512;
		sy = READ_WORD(&gaiden_spriteram[offs+6]) & 0x1ff;
		if (sy >= 256) sy -= 512;
		num = READ_WORD(&gaiden_spriteram[offs+2])>>4;
		spr_pri=(READ_WORD(&gaiden_spriteram[offs])&0x0080)>>7;
		if ((READ_WORD(&gaiden_spriteram[offs])&0x0004) && (spr_pri==priority)
				 && sx > -64 && sx < 256 && sy > -64 && sy < 256)
		{
			num &= 0x7ff;
			bank=0;
			size = READ_WORD(&gaiden_spriteram[offs+4])&0x0003;
			flip = READ_WORD(&gaiden_spriteram[offs])&0x0003;
			col = ((READ_WORD(&gaiden_spriteram[offs+4])&0x00f0)>>4);
			if (size==1)
			{
				num=(num<<2)+((READ_WORD(&gaiden_spriteram[offs+2])&0x000c)>>2);
				bank=1;
			}
			else if (size==0)
			{
				/* 8x8 not supported! */
				num=rand();
			}

			if (size != 3)
				drawgfx(bitmap, Machine->gfx[3+bank],
					num,
					col,
					flip&0x01,flip&0x02,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			else
			{
				int n1,n2,n3,n4;

				switch (flip&0x03)
				{
					case 0:
					default:
						n1 = num+0;n2 = num+1;n3 = num+2;n4 = num+3;
						break;
					case 1:
						n1 = num+1;n2 = num+0;n3 = num+3;n4 = num+2;
						break;
					case 2:
						n1 = num+2;n2 = num+3;n3 = num+0;n4 = num+1;
						break;
					case 3:
						n1 = num+3;n2 = num+2;n3 = num+1;n4 = num+0;
						break;
				}

				drawgfx(bitmap, Machine->gfx[3+bank],
					n1,
					col,
					flip&0x01,flip&0x02,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				drawgfx(bitmap, Machine->gfx[3+bank],
					n2,
					col,
					flip&0x01,flip&0x02,
					sx+32,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				drawgfx(bitmap, Machine->gfx[3+bank],
					n3,
					col,
					flip&0x01,flip&0x02,
					sx,sy+32,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				drawgfx(bitmap, Machine->gfx[3+bank],
					n4,
					col,
					flip&0x01,flip&0x02,
					sx+32,sy+32,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}
}

void gaiden_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int scrollx,scrolly;


memset(palette_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors * sizeof(unsigned char));

{
	int color,code,i;
	int colmask[16];
	int pal_base;


	pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;

	for (color = 0;color < 16;color++) colmask[color] = 0;

	for (offs = (gaiden_videoram3_size / 2) - 2;offs >= 0;offs -= 2)
	{
		code = READ_WORD(&gaiden_videoram3[offs + 0x1000]) & 0x0fff;
		color = (READ_WORD(&gaiden_videoram3[offs]) & 0x00f0) >> 4;
		colmask[color] |= Machine->gfx[1]->pen_usage[code];
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


	pal_base = Machine->drv->gfxdecodeinfo[2].color_codes_start;

	for (color = 0;color < 16;color++) colmask[color] = 0;

	for (offs = (gaiden_videoram2_size / 2) - 2;offs >= 0;offs -= 2)
	{
		code = READ_WORD(&gaiden_videoram2[offs + 0x1000]) & 0x0fff;
		color = (READ_WORD(&gaiden_videoram2[offs]) & 0x00f0) >> 4;
		colmask[color] |= Machine->gfx[2]->pen_usage[code];
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


	pal_base = Machine->drv->gfxdecodeinfo[3].color_codes_start;

	for (color = 0;color < 16;color++) colmask[color] = 0;

	for (offs = 0; offs<0x800 ; offs += 16)
	{
		int sx,sy;
		int bank,size;

		sx = READ_WORD(&gaiden_spriteram[offs+8]) & 0x1ff;
		if (sx >= 256) sx -= 512;
		sy = READ_WORD(&gaiden_spriteram[offs+6]) & 0x1ff;
		if (sy >= 256) sy -= 512;
		code = READ_WORD(&gaiden_spriteram[offs+2])>>4;
		if ((READ_WORD(&gaiden_spriteram[offs])&0x0004) && sx > -64 && sx < 256 && sy > -64 && sy < 256)
		{
			code &= 0x7ff;
			bank=0;
			size = READ_WORD(&gaiden_spriteram[offs+4])&0x0003;
			color = ((READ_WORD(&gaiden_spriteram[offs+4])&0x00f0)>>4);
#if 0
to use this code, we must handle the size == 3 case (64x64)
			if (size==1)
			{
				code=(code<<2)+((READ_WORD(&gaiden_spriteram[offs+2])&0x000c)>>2);
				bank=1;
			}
			colmask[color] |= Machine->gfx[3+bank]->pen_usage[code];
#endif
			colmask[color] |= 0xffff;
		}
	}

	for (color = 0;color < 16;color++)
	{
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}


	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;

	for (color = 0;color < 16;color++) colmask[color] = 0;

	for (offs = (gaiden_videoram_size / 2) - 2;offs >= 0;offs -= 2)
	{
		code = READ_WORD(&gaiden_videoram[offs + 0x800]) & 0x07ff;
		color = (READ_WORD(&gaiden_videoram[offs]) & 0x00f0) >> 4;
		colmask[color] |= Machine->gfx[0]->pen_usage[code];
	}

	for (color = 0;color < 16;color++)
	{
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	if (palette_recalc())
	{
		memset(gaiden_dirty2,1,gaiden_videoram2_size / 4);
		memset(gaiden_dirty3,1,gaiden_videoram3_size / 4);
	}
}


	/* update graphics tiles */
	for (offs = (gaiden_videoram3_size / 2) - 2;offs >= 0;offs -= 2)
	{
		if (gaiden_dirty3[offs / 2])
		{
			int sx,sy;


			gaiden_dirty3[offs / 2] = 0;
			sx = (offs/2) % 64;
			sy = (offs/2) / 64;
			drawgfx(tmpbitmap3,Machine->gfx[1],
					READ_WORD(&gaiden_videoram3[offs + 0x1000]) & 0x0fff,
					(READ_WORD(&gaiden_videoram3[offs]) & 0x00f0) >> 4,
					0,0,
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	for (offs = (gaiden_videoram2_size / 2) - 2;offs >= 0;offs -= 2)
	{
		if (gaiden_dirty2[offs / 2])
		{
			int sx,sy;


			gaiden_dirty2[offs / 2] = 0;
			sx = (offs/2) % 64;
			sy = (offs/2) / 64;
			drawgfx(tmpbitmap2,Machine->gfx[2],
					READ_WORD(&gaiden_videoram2[offs + 0x1000]) & 0x0fff,
					(READ_WORD(&gaiden_videoram2[offs]) & 0x00f0) >> 4,
					0,0,
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	scrollx = -READ_WORD(gaiden_bgscrollx);
	scrolly = -READ_WORD(gaiden_bgscrolly);
	copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,1,&scrolly,&Machine->drv->visible_area, TRANSPARENCY_NONE,0);

	/* draw occluded sprites */
	gaiden_drawsprites(bitmap,1);

	scrollx = -READ_WORD(gaiden_fgscrollx);
	scrolly = -READ_WORD(gaiden_fgscrolly);
	copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

	/* draw non-occluded sprites */
	gaiden_drawsprites(bitmap,0);

	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	scrollx = -READ_WORD(gaiden_txscrollx);
	scrolly = -READ_WORD(gaiden_txscrolly);
	for (offs = (gaiden_videoram_size / 2) - 2;offs >= 0;offs -= 2)
	{
		int sx,sy;

		sx = (offs/2) % 32;
		sy = (offs/2) / 32;

		drawgfx(bitmap,Machine->gfx[0],
				READ_WORD(&gaiden_videoram[offs + 0x0800]) & 0x07ff,
				(READ_WORD(&gaiden_videoram[offs]) & 0x00f0) >> 4,
				0,0,
				(8*sx + scrollx) & 0xff,(8*sy + scrolly) & 0xff,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
