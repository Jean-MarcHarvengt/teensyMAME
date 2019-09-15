/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80/Z80.h"


#define VIDEORAM_START 0x8000

void kangaroo_bankselect_w(int offset,int data);

unsigned char *kangaroo_bank_select;
unsigned char *kangaroo_blitter;

static struct osd_bitmap *tmpbitmap_b;
static unsigned char inverse_palette[256];
static unsigned char inverse_palette_b[256];



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Kangaroo doesn't have color PROMs, the playfield data is directly converted
  into colors: 1 bit per gun, therefore only 8 possible colors, but there is
  also a global mask register which controls intensities of the three guns,
  separately for foreground and background. The fourth bit in the video RAM
  data disables this mask, making the color display at full intensity
  regardless of the mask value.
  Actually the mask doesn't directly control intensity. The guns are rapidly
  turned on and off at a subpixel rate, relying on the monitor to blend the
  colors into a more or less uniform half intensity color.

  We use three groups of 8 pens: the first is fixed and contains the 8
  possible colors; the other two are dynamically modified when the mask
  register is written to, one is for the background, the other for sprites.

***************************************************************************/
void kangaroo_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = ((i & 4) >> 2) * 0xff;
		*(palette++) = ((i & 2) >> 1) * 0xff;
		*(palette++) = ((i & 1) >> 0) * 0xff;
	}

	/* initialize the color table */
	colortable[0] = 0;	/* transparent */
	colortable[8] = 8;	/* not transparent */
	colortable[16] = 0;	/* transparent */
	colortable[24] = 16;	/* not transparent */
	for (i = 1;i < 8;i++)
	{
		colortable[i] = 8+i;    /* A - no Z bit */
		colortable[8+i] = i;    /* A - Z bit */
		colortable[16+i] = 16+i;/* B - no Z bit */
		colortable[24+i] = i;   /* B - Z bit */
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int kangaroo_vh_start(void)
{
	int i;


	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	if ((tmpbitmap_b = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		return 1;
	}

	for (i = 0;i < 16;i++)
	{
		inverse_palette[ Machine->gfx[0]->colortable[i] ] = i;
		inverse_palette_b[ Machine->gfx[0]->colortable[16+i] ] = i;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void kangaroo_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap_b);
	osd_free_bitmap(tmpbitmap);
}



void kangaroo_color_mask_w(int offset,int data)
{
	int i;


	/* color mask for A plane */
	for (i = 0;i < 8;i++)
	{
		int r,g,b;


		r = ((i & 4) >> 2) * ((data & 0x20) ? 0xff : 0x7f);
		g = ((i & 2) >> 1) * ((data & 0x10) ? 0xff : 0x7f);
		b = ((i & 1) >> 0) * ((data & 0x08) ? 0xff : 0x7f);

		palette_change_color(8 + i,r,g,b);
	}

	/* color mask for B plane */
	for (i = 0;i < 8;i++)
	{
		int r,g,b;


		r = ((i & 4) >> 2) * ((data & 0x04) ? 0xff : 0x7f);
		g = ((i & 2) >> 1) * ((data & 0x02) ? 0xff : 0x7f);
		b = ((i & 1) >> 0) * ((data & 0x01) ? 0xff : 0x7f);

		palette_change_color(16 + i,r,g,b);
	}
}



void kangaroo_blitter_w(int offset,int data)
{
	kangaroo_blitter[offset] = data;

	if (offset == 5)	/* trigger DMA */
	{
		int src,dest;
		int ofsx, ofsy,x, y, xb,yb;


		src = kangaroo_blitter[0] + 256 * kangaroo_blitter[1];
		dest = kangaroo_blitter[2] + 256 * kangaroo_blitter[3];

		ofsx = (dest - VIDEORAM_START) % 256;
		ofsy = (0x3f - (dest - VIDEORAM_START) / 256) * 4;
		xb = kangaroo_blitter[4];
		yb = kangaroo_blitter[5];
		/* kangaroo_blitter[6] (vertical start address in bitmap) and */
		/* kangaroo_blitter[7] (horizontal start address in bitmap) seem */
		/* to be always 0 */

		for (y = 0;y <= yb;y++)
		{
			for (x = 0;x <= xb;x++)
			{
#ifdef ACCURATE_BLITTER	/* doesn't work!! */
				if ((*kangaroo_bank_select & 0x05) && (*kangaroo_bank_select & 0x0a))
				{
					int old;


					/* if both planes active, write to one at a time */
					old = *kangaroo_bank_select;
					kangaroo_bankselect_w(0,old & 0x05);
					cpu_writemem16(dest + x + 256*y,cpu_readmem16(src));
					kangaroo_bankselect_w(0,old & 0x0a);
					cpu_writemem16(dest + x + 256*y,cpu_readmem16(src));
					kangaroo_bankselect_w(0,old);
				}
				else
					cpu_writemem16(dest + x + 256*y,cpu_readmem16(src));
#else
				if (*kangaroo_bank_select & 0x0c)
					drawgfx(tmpbitmap_b,Machine->gfx[0],src,1,0,0,
							ofsx + x,ofsy - 4*y,
							&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
				if (*kangaroo_bank_select & 0x03)
					drawgfx(tmpbitmap,Machine->gfx[0],src,0,0,0,
							ofsx + x,ofsy - 4*y,
							&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
#endif
				src++;
			}
		}
	}
}



/* this is almost the same as in arabian.c, the planes are arranged
   a bit differently. -V-
*/
void kangaroo_videoram_w(int offset,int data)
{
	int a_Z_R,a_G_B,b_Z_R,b_G_B;
	unsigned char *bm;
	int sx, sy;


	a_Z_R = *kangaroo_bank_select & 0x01;
	a_G_B = *kangaroo_bank_select & 0x02;
	b_Z_R = *kangaroo_bank_select & 0x04;
	b_G_B = *kangaroo_bank_select & 0x08;


	sx = offset % 256;
	sy = (0x3f - (offset / 256)) * 4;

	tmpbitmap->line[sy][sx] = inverse_palette[ tmpbitmap->line[sy][sx] ];
	tmpbitmap->line[sy+1][sx] = inverse_palette[ tmpbitmap->line[sy+1][sx] ];
	tmpbitmap->line[sy+2][sx] = inverse_palette[ tmpbitmap->line[sy+2][sx] ];
	tmpbitmap->line[sy+3][sx] = inverse_palette[ tmpbitmap->line[sy+3][sx] ];
	tmpbitmap_b->line[sy][sx] = inverse_palette_b[ tmpbitmap_b->line[sy][sx] ];
	tmpbitmap_b->line[sy+1][sx] = inverse_palette_b[ tmpbitmap_b->line[sy+1][sx] ];
	tmpbitmap_b->line[sy+2][sx] = inverse_palette_b[ tmpbitmap_b->line[sy+2][sx] ];
	tmpbitmap_b->line[sy+3][sx] = inverse_palette_b[ tmpbitmap_b->line[sy+3][sx] ];


	if (a_G_B)
	{
		bm = tmpbitmap->line[sy] + sx;
		*bm &= 0xfc;
		/* Green */
		if (data & 0x80) *bm |= 2;
		/* Blue */
		if (data & 0x08) *bm |= 1;

		bm = tmpbitmap->line[sy+1] + sx;
		*bm &= 0xfc;
		/* Green */
		if (data & 0x40) *bm |= 2;
		/* Blue */
		if (data & 0x04) *bm |= 1;

		bm = tmpbitmap->line[sy+2] + sx;
		*bm &= 0xfc;
		/* Green */
		if (data & 0x20) *bm |= 2;
		/* Blue */
		if (data & 0x02) *bm |= 1;

		bm = tmpbitmap->line[sy+3] + sx;
		*bm &= 0xfc;
		/* Green */
		if (data & 0x10) *bm |= 2;
		/* Blue */
		if (data & 0x01) *bm |= 1;
	}

	if (a_Z_R)
	{
		bm = tmpbitmap->line[sy] + sx;
		*bm &= 0xf3;
		/* Z - mask */
		if (data & 0x80) *bm |= 8;
		/* Red */
		if (data & 0x08) *bm |= 4;

		bm = tmpbitmap->line[sy+1] + sx;
		*bm &= 0xf3;
		/* Z - mask */
		if (data & 0x40) *bm |= 8;
		/* Red */
		if (data & 0x04) *bm |= 4;

		bm = tmpbitmap->line[sy+2] + sx;
		*bm &= 0xf3;
		/* Z - mask */
		if (data & 0x20) *bm |= 8;
		/* Red */
		if (data & 0x02) *bm |= 4;

		bm = tmpbitmap->line[sy+3] + sx;
		*bm &= 0xf3;
		/* Z - mask */
		if (data & 0x10) *bm |= 8;
		/* Red */
		if (data & 0x01) *bm |= 4;
	}

	if (b_G_B)
	{
		bm = tmpbitmap_b->line[sy] + sx;
		*bm &= 0xfc;
		/* Green */
		if (data & 0x80) *bm |= 2;
		/* Blue */
		if (data & 0x08) *bm |= 1;

		bm = tmpbitmap_b->line[sy+1] + sx;
		*bm &= 0xfc;
		/* Green */
		if (data & 0x40) *bm |= 2;
		/* Blue */
		if (data & 0x04) *bm |= 1;

		bm = tmpbitmap_b->line[sy+2] + sx;
		*bm &= 0xfc;
		/* Green */
		if (data & 0x20) *bm |= 2;
		/* Blue */
		if (data & 0x02) *bm |= 1;

		bm = tmpbitmap_b->line[sy+3] + sx;
		*bm &= 0xfc;
		/* Green */
		if (data & 0x10) *bm |= 2;
		/* Blue */
		if (data & 0x01) *bm |= 1;
	}

	if (b_Z_R)
	{
		bm = tmpbitmap_b->line[sy] + sx;
		*bm &= 0xf3;
		/* Z - mask */
		if (data & 0x80) *bm |= 8;
		/* Red */
		if (data & 0x08) *bm |= 4;

		bm = tmpbitmap_b->line[sy+1] + sx;
		*bm &= 0xf3;
		/* Z - mask */
		if (data & 0x40) *bm |= 8;
		/* Red */
		if (data & 0x04) *bm |= 4;

		bm = tmpbitmap_b->line[sy+2] + sx;
		*bm &= 0xf3;
		/* Z - mask */
		if (data & 0x20) *bm |= 8;
		/* Red */
		if (data & 0x02) *bm |= 4;

		bm = tmpbitmap_b->line[sy+3] + sx;
		*bm &= 0xf3;
		/* Z - mask */
		if (data & 0x10) *bm |= 8;
		/* Red */
		if (data & 0x01) *bm |= 4;
	}


	tmpbitmap->line[sy][sx]    = Machine->gfx[0]->colortable[tmpbitmap->line[sy][sx]];
	tmpbitmap->line[sy+1][sx]  = Machine->gfx[0]->colortable[tmpbitmap->line[sy+1][sx]];
	tmpbitmap->line[sy+2][sx]  = Machine->gfx[0]->colortable[tmpbitmap->line[sy+2][sx]];
	tmpbitmap->line[sy+3][sx]  = Machine->gfx[0]->colortable[tmpbitmap->line[sy+3][sx]];
	tmpbitmap_b->line[sy][sx]   = Machine->gfx[0]->colortable[16 + tmpbitmap_b->line[sy][sx]];
	tmpbitmap_b->line[sy+1][sx] = Machine->gfx[0]->colortable[16 + tmpbitmap_b->line[sy+1][sx]];
	tmpbitmap_b->line[sy+2][sx] = Machine->gfx[0]->colortable[16 + tmpbitmap_b->line[sy+2][sx]];
	tmpbitmap_b->line[sy+3][sx] = Machine->gfx[0]->colortable[16 + tmpbitmap_b->line[sy+3][sx]];

	osd_mark_dirty (sx,sy,sx,sy+3,0);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void kangaroo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* Plane B is primary */
	if (*kangaroo_bank_select & 0x01)
	{
		copybitmap(bitmap,tmpbitmap_b,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}
	/* Plane A is primary */
	else
	{
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,tmpbitmap_b,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}
}
