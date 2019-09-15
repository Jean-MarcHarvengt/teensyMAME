/***************************************************************************

  vidhrdw/mcr2.c

  Functions to emulate the video hardware of the machine.

  Journey is an MCR/II game with a MCR/III sprite board so it has it's own
  routines.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static int sprite_transparency[128]; /* no mcr2 game has more than this many sprites */


/***************************************************************************

  Generic MCR/II video routines

***************************************************************************/

void mcr2_paletteram_w(int offset,int data)
{
	int r,g,b;


	paletteram[offset] = data;

	/* bit 2 of the red component is taken from bit 0 of the address */
	r = ((offset & 1) << 2) + (data >> 6);
	g = (data >> 0) & 7;
	b = (data >> 3) & 7;

	/* up to 8 bits */
	r = (r << 5) | (r << 2) | (r >> 1);
	g = (g << 5) | (g << 2) | (g >> 1);
	b = (b << 5) | (b << 2) | (b >> 1);

	palette_change_color(offset/2,r,g,b);
}


void mcr2_videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset & ~1] = 1;
		videoram[offset] = data;
	}
}


void mcr2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int mx,my;
	int attr;
	int color;

	if (full_refresh)
		memset (dirtybuffer, 1, videoram_size);

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 2;offs >= 0;offs -= 2)
	{
		/*
		The attributes are as follows:
		0x01 = character bank
		0x02 = flip x
		0x04 = flip y
		0x18 = color
		*/

		if (dirtybuffer[offs])
		{
			dirtybuffer[offs] = 0;

			mx = (offs/2) % 32;
			my = (offs/2) / 32;
			attr = videoram[offs+1];
			color = (attr & 0x18) >> 3;

			drawgfx(bitmap,Machine->gfx[0],
				videoram[offs] + 256*(attr & 0x01),
				color,
				attr & 0x02, attr & 0x04,
				16*mx, 16*my,
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int x,y,sx,sy,flags;

		if (spriteram[offs] == 0) continue;

		flags = spriteram[offs+3];
		x = (spriteram[offs+2]-3)*2;
		y = (241-spriteram[offs])*2;

		/* TRANSPARENCY_PENS, 0x0101 fixes a black border around the fire */
		/* breath in Satan's Hollow, however it's probably wrong. I don't */
		/* know what's going on here: using the "pen 8 masks underlying */
		/* sprites" as in the MCR3 games doesn't seem to make sense. */
		drawgfx (bitmap,Machine->gfx[1],
			spriteram[offs+1] & 0x3f,
			0,
			spriteram[offs+1] & 0x40, spriteram[offs+1] & 0x80,
			x,y,
			&Machine->drv->visible_area,TRANSPARENCY_PENS,0x0101);

		/* mark tiles underneath as dirty */
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
						dirtybuffer[x2*2 + 32*y2 * 2] = 1;
				}
			}
		}

	}
}


/***************************************************************************

  Journey-specific video routines

***************************************************************************/

int journey_vh_start(void)
{
   /* now check our sprites and mark which ones have color 8 ('draw under') */
   {
      struct GfxElement *gfx;
      int i,x,y;
      unsigned char *dp;

      gfx = Machine->gfx[1];
      for (i=0;i<gfx->total_elements;i++)
      {
			sprite_transparency[i] = 0;

			for (y=0;y<gfx->height;y++)
			{
				dp = gfx->gfxdata->line[i * gfx->height + y];
				for (x=0;x<gfx->width;x++)
					if (dp[x] == 8)
						sprite_transparency[i] = 1;
			}

			if (sprite_transparency[i])
				if (errorlog)
					fprintf(errorlog,"sprite %i has transparency.\n",i);
      }
   }

	return generic_vh_start();
}


void journey_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
   int offs;
   int mx,my;
   int attr;
   int color;

   /* for every character in the Video RAM, check if it has been modified */
   /* since last time and update it accordingly. */
   for (offs = videoram_size - 2;offs >= 0;offs -= 2)
   {
	/*
		The attributes are as follows:
		0x01 = character bank
		0x02 = flip x
		0x04 = flip y
		0x18 = color
	*/

      if (dirtybuffer[offs])
      {
			dirtybuffer[offs] = 0;

			mx = (offs/2) % 32;
			my = (offs/2) / 32;
	      attr = videoram[offs+1];
	      color = (attr & 0x18) >> 3;

			drawgfx(tmpbitmap,Machine->gfx[0],
				videoram[offs]+256*(attr & 0x01),
				color,attr & 0x02,attr & 0x04,16*mx,16*my,
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
      }
   }

   /* copy the character mapped graphics */
   copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

   /* Draw the sprites. */
   for (offs = 0;offs < spriteram_size;offs += 4)
   {
      int code,flipx,flipy,sx,sy,flags;

      if (spriteram[offs] == 0)
			continue;

      code = spriteram[offs+2];
      flags = spriteram[offs+1];
      color = 3 - (flags & 0x03);
      flipx = flags & 0x10;
      flipy = flags & 0x20;
      sx = (spriteram[offs+3]-3)*2;
      sy = (241-spriteram[offs])*2;

      drawgfx(bitmap,Machine->gfx[1],
	      code,color,flipx,flipy,sx,sy,
	      &Machine->drv->visible_area,TRANSPARENCY_PEN,0);

      /* sprites use color 0 for background pen and 8 for the 'under tile' pen.
			The color 8 is used to cover over other sprites.
      	At the beginning we scanned all sprites and marked the ones that contained
			at least one pixel of color 8, so we only need to worry about these few. */
      if (sprite_transparency[code])
      {
			struct rectangle clip;

			clip.min_x = sx;
			clip.max_x = sx+31;
			clip.min_y = sy;
			clip.max_y = sy+31;

			copybitmap(bitmap,tmpbitmap,0,0,0,0,&clip,TRANSPARENCY_THROUGH,Machine->pens[8+(color*16)+1]);
      }
   }
}
