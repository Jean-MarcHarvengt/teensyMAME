/***************************************************************************

  vidhrdw/mcr68.c

  Xenophobe video hardware very similar to Rampage.

  Colour 8 in sprites indicates transparency in closed area.
  Each tile has an attribute to indicate tile drawn on top of sprite.

There is one remaining graphic glitch - on first base, the room to the right
of the start room, there are some tiles marked as having priority that should
not have...

July 1998: Changed from tmpbitmap style drawing method to Brad's 'sprite/dirty
	tile' method (see mcr1/2/3) - much, much faster :)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *mcr68_spriteram;

void mcr68_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	struct osd_bitmap *bitmap = Machine->gfx[0]->gfxdata;
	int y, x, i;

	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	/* the palette will be initialized by the game. We just set it to some */
	/* pre-cooked values so the startup copyright notice can be displayed. */
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = ((i & 1) >> 0) * 0xff;
		*(palette++) = ((i & 2) >> 1) * 0xff;
		*(palette++) = ((i & 4) >> 2) * 0xff;
	}

	/* characters and sprites use the same palette */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i;

   /* Convert transparency styles */
   {
      struct GfxElement *gfx;
      unsigned char *dp;

      gfx = Machine->gfx[1];
      for (i=0;i<gfx->total_elements;i++)
			for (y=0;y<gfx->height;y++)
			{
				dp = gfx->gfxdata->line[i * gfx->height + y];
				for (x=0;x<gfx->width;x++)
					if (dp[x] == 8)
                    	dp[x] = 0;
			}
   }

	/* the tile graphics are reverse colors; we preswap them here */
	for (y = bitmap->height - 1; y >= 0; y--)
		for (x = bitmap->width - 1; x >= 0; x--)
			bitmap->line[y][x] ^= 15;
}

/******************************************************************************/

void mcr68_paletteram_w(int offset,int data)
{
	int r,g,b;
	int oldword = READ_WORD(&paletteram[offset]);
	int newword = COMBINE_WORD(oldword,data);

	WRITE_WORD(&paletteram[offset],newword);

	r = ((offset & 1) << 2) + (newword >> 6);
	g = (newword >> 0) & 7;
	b = (newword >> 3) & 7;

	/* up to 8 bits */
	r = (r << 5) | (r << 2) | (r >> 1);
	g = (g << 5) | (g << 2) | (g >> 1);
	b = (b << 5) | (b << 2) | (b >> 1);

	palette_change_color(offset/2,r,g,b);
}

void mcr68_videoram_w(int offset,int data)
{
	int oldword = READ_WORD(&videoram[offset]);
	int newword = COMBINE_WORD(oldword,data);

    if (oldword!=newword) {
		dirtybuffer[offset & ~2] = 1;
		WRITE_WORD(&videoram[offset],newword);
	}
}

int mcr68_videoram_r(int offset)
{
	return READ_WORD(&videoram[offset]);
}

/******************************************************************************/

void xenophobe_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int mx,my;
	int tile,attr;
	int color;

	if (full_refresh)
		memset(dirtybuffer, 1, videoram_size);

   /* for every character in the Video RAM, check if it has been modified */
   /* since last time and update it accordingly. */
   for (offs = videoram_size - 4;offs >= 0;offs -= 4)
   {
		if (dirtybuffer[offs])
		{
				dirtybuffer[offs] = 0;
				mx = (offs/4) % 32;
				my = (offs/4) / 32;
				tile=READ_WORD(&videoram[offs])&0xff;
				attr=READ_WORD(&videoram[offs+2])&0xff;

				tile=tile+(256*(attr&0x3));
				if (attr&0x40) tile+=1024;

				color = (attr & 0x30) >> 4;
				drawgfx(bitmap,Machine->gfx[0],
					tile,
					3-color,attr & 0x04,attr&0x08,16*mx,16*my,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

   	/* Draw the sprites. */
	for (offs = 0; offs < 0x800; offs += 8)
   	{
   		int code,flipx,flipy,sx,sy,flags,x,y;

     	code=(READ_WORD(&mcr68_spriteram[offs+4]))&0xff;
        flags=(READ_WORD(&mcr68_spriteram[offs+2]))&0xff;

        code += 256 * ((flags >> 3) & 1);
        if (flags&0x80) code+=1024;

		if (!code)
			continue;

        color = 3 - (flags & 0x03);
		flipx = flags & 0x10;
		flipy = flags & 0x20;

        /* Slight differences from Rampage here */
		x=((READ_WORD(&mcr68_spriteram[offs+6])&0xff)-2)*2;
		y=(240-(READ_WORD(&mcr68_spriteram[offs])&0xff))*2;

      	drawgfx(bitmap,Machine->gfx[1],
	      code,color,flipx,flipy,x,y,
	      &Machine->drv->visible_area,TRANSPARENCY_PEN,0);

		/* Brad's code - mark tiles underneath as dirty */
		sx = x >> 4;
		sy = y >> 4;

		{
			int max_x = 2;
			int max_y = 2;
			int x2, y2;

			if (x & 0x1f) max_x = 3;
			if (y & 0x1f) max_y = 3;

			for (y2 = sy; y2 < sy + max_y; y2 ++)
			{
				for (x2 = sx; x2 < sx + max_x; x2 ++)
				{
					if ((x2 < 32) && (y2 < 30) && (x2 >= 0) && (y2 >= 0))
						dirtybuffer[x2*4 + 32*y2 * 4] = 1;
				}
			}
		}
	}

    /* Redraw tiles with priority over sprites */
	for (offs = videoram_size - 4;offs >= 0;offs -= 4)
	{
        /* Sprites _underneath_ this tile will have marked it dirty above */
        if (!dirtybuffer[offs]) continue;

       	/* MSB of tile is priority, if _tile_ is underneath then leave dirty */
		attr=READ_WORD(&videoram[offs+2])&0xff;
		if (!(attr&0x80)) continue;

        /* We can reset this tile as there is no need to draw it underneath the
        	sprite in first loop above */
        dirtybuffer[offs]=0;

		mx = (offs/4) % 32;
		my = (offs/4) / 32;

		tile=READ_WORD(&videoram[offs])&0xff;
		tile=tile+(256*(attr&0x3));
		if (attr&0x40) tile+=1024;

		color = (attr & 0x30) >> 4;
		drawgfx(bitmap,Machine->gfx[0],
			tile,
			3-color,attr&0x04,attr&0x08,16*mx,16*my,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}

