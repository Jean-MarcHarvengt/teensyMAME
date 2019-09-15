/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *zaxxon_char_color_bank;
unsigned char *zaxxon_background_position;
unsigned char *zaxxon_background_color_bank;
unsigned char *zaxxon_background_enable;
static struct osd_bitmap *backgroundbitmap1,*backgroundbitmap2;
static const unsigned char *color_codes;

int zaxxon_vid_type;	/* set by init_machine; 0 = zaxxon; 1 = congobongo */

#define ZAXXON_VID	0
#define CONGO_VID	1
#define FUTSPY_VID	2


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Zaxxon has one 256x8 palette PROM and one 256x4 PROM which contains the
  color codes to use for characters on a per row/column basis (groups of
  of 4 characters in the same row).
  Congo Bongo has similar hardware, but it has color RAM instead of the
  lookup PROM.

  The palette PROM is connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
void zaxxon_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}


	/* color_prom now points to the beginning of the character color codes */
	color_codes = color_prom;	/* we'll need it later */


	/* all gfx elements use the same palette */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i;
}

/* handle rotation of the background bitmaps */
static void copy_rotated_pixel (struct osd_bitmap *dst_bm, int dx, int dy,
	struct osd_bitmap *src_bm, int sx, int sy)
{
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = dx;
		dx = dy;
		dy = temp;
		temp = sx;
		sx = sy;
		sy = temp;
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		dx = dst_bm->width - 1 - dx;
		sx = src_bm->width - 1 - sx;
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		dy = dst_bm->height - 1 - dy;
		sy = src_bm->height - 1 - sy;
	}

	dst_bm->line[dy][dx] = src_bm->line[sy][sx];
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int zaxxon_vh_start(void)
{
	int offs;
	int sx,sy;
	struct osd_bitmap *prebitmap;
	int width, height;


	if (generic_vh_start() != 0)
		return 1;

	/* for speed, backgrounds are arranged differently if axis is swapped */
	if (Machine->orientation & ORIENTATION_SWAP_XY)
		height = 512, width = 2303+32;
	else
		/* leave a screenful of black pixels at each end */
		height = 256+4096+256, width = 256;

	/* large bitmap for the precalculated background */
	if ((backgroundbitmap1 = osd_create_bitmap(width,height)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	if (zaxxon_vid_type == ZAXXON_VID || zaxxon_vid_type == FUTSPY_VID)
	{
		if ((backgroundbitmap2 = osd_create_bitmap(width,height)) == 0)
		{
			osd_free_bitmap(backgroundbitmap1);
			generic_vh_stop();
			return 1;
		}
	}

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		/* create a temporary bitmap to prepare the background before converting it */
		if ((prebitmap = osd_create_bitmap(256,4096)) == 0)
		{
			if (zaxxon_vid_type == ZAXXON_VID || zaxxon_vid_type == FUTSPY_VID)
				osd_free_bitmap(backgroundbitmap2);
			osd_free_bitmap(backgroundbitmap1);
			generic_vh_stop();
			return 1;
		}
	}
	else
		prebitmap = backgroundbitmap1;

	/* prepare the background */
	for (offs = 0;offs < 0x4000;offs++)
	{
		sy = 8 * (offs / 32);
		sx = 8 * (offs % 32);

		if (!(Machine->orientation & ORIENTATION_SWAP_XY))
			/* leave screenful of black pixels at end */
			sy += 256;

		drawgfx(prebitmap,Machine->gfx[1],
				Machine->memory_region[2][offs] + 256 * (Machine->memory_region[2][0x4000 + offs] & 3),
				Machine->memory_region[2][0x4000 + offs] >> 4,
				0,0,
				sx,sy,
				0,TRANSPARENCY_NONE,0);
	}

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		/* the background is stored as a rectangle, but is drawn by the hardware skewed: */
		/* go right two pixels, then up one pixel. Doing the conversion at run time would */
		/* be extremely expensive, so we do it now. To save memory, we squash the image */
		/* horizontally (doing line shifts at run time is much less expensive than doing */
		/* column shifts) */
		for (offs = -510;offs < 4096;offs += 2)
		{
			sx = (2302-510/2) - offs/2;

			for (sy = 0;sy < 512;sy += 2)
			{
				if (offs + sy >= 0 && offs + sy < 4096)
				{
					copy_rotated_pixel (backgroundbitmap1, sx, 511 - sy, prebitmap, sy/2, 4095 - (offs+sy));
					copy_rotated_pixel (backgroundbitmap1, sx, 511 - (sy+1), prebitmap, sy/2, 4095 - (offs+sy+1));
				}
			}
		}
	}

	if (zaxxon_vid_type == ZAXXON_VID || zaxxon_vid_type == FUTSPY_VID)
	{
		if (!(Machine->orientation & ORIENTATION_SWAP_XY))
			prebitmap = backgroundbitmap2;

		/* prepare a second background with different colors, used in the death sequence */
		for (offs = 0;offs < 0x4000;offs++)
		{
			sy = 8 * (offs / 32);
			sx = 8 * (offs % 32);

			if (!(Machine->orientation & ORIENTATION_SWAP_XY))
				/* leave screenful of black pixels at end */
				sy += 256;

			drawgfx(prebitmap,Machine->gfx[1],
					Machine->memory_region[2][offs] + 256 * (Machine->memory_region[2][0x4000 + offs] & 3),
					16 + (Machine->memory_region[2][0x4000 + offs] >> 4),
					0,0,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}

		if (Machine->orientation & ORIENTATION_SWAP_XY)
		{
			/* the background is stored as a rectangle, but is drawn by the hardware skewed: */
			/* go right two pixels, then up one pixel. Doing the conversion at run time would */
			/* be extremely expensive, so we do it now. To save memory, we squash the image */
			/* horizontally (doing line shifts at run time is much less expensive than doing */
			/* column shifts) */
			for (offs = -510;offs < 4096;offs += 2)
			{
				sx = (2302-510/2) - offs/2;

				for (sy = 0;sy < 512;sy += 2)
				{
					if (offs + sy >= 0 && offs + sy < 4096)
					{
						copy_rotated_pixel (backgroundbitmap2, sx, 511 - sy, prebitmap, sy/2, 4095 - (offs+sy));
						copy_rotated_pixel (backgroundbitmap2, sx, 511 - (sy+1), prebitmap, sy/2, 4095 - (offs+sy+1));
					}
				}
			}
		}
	}

	if (Machine->orientation & ORIENTATION_SWAP_XY)
		osd_free_bitmap(prebitmap);

	/* free the graphics ROMs, they are no longer needed */
	free(Machine->memory_region[2]);
	Machine->memory_region[2] = 0;


	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void zaxxon_vh_stop(void)
{
	if (zaxxon_vid_type == ZAXXON_VID || zaxxon_vid_type == FUTSPY_VID)
		osd_free_bitmap(backgroundbitmap2);
	osd_free_bitmap(backgroundbitmap1);
	generic_vh_stop();
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void zaxxon_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	/* copy the background */
	/* TODO: there's a bug here which shows only in test mode. The background doesn't */
	/* cover the whole screen, so the image is not fully overwritten and part of the */
	/* character color test screen remains on screen when it is replaced by the background */
	/* color test. */
	if (*zaxxon_background_enable)
	{
		int i,skew,scroll;
		struct rectangle clip;


		if (Machine->orientation & ORIENTATION_SWAP_XY)
		{
			/* standard rotation - skew background horizontally */
			if (zaxxon_vid_type == CONGO_VID)
				scroll = 1023+63 - (zaxxon_background_position[0] + 256*zaxxon_background_position[1]);
			else
				scroll = 2048+63 - (zaxxon_background_position[0] + 256*(zaxxon_background_position[1]&7));

			skew = 128 - 512 + 2 * Machine->drv->visible_area.min_x;

			clip.min_y = Machine->drv->visible_area.min_y;
			clip.max_y = Machine->drv->visible_area.max_y;

			for (i = Machine->drv->visible_area.min_x;i <= Machine->drv->visible_area.max_x;i++)
			{
				clip.min_x = i;
				clip.max_x = i;

				if ((zaxxon_vid_type == ZAXXON_VID || zaxxon_vid_type == FUTSPY_VID)
						&& (*zaxxon_background_color_bank & 1))
					copybitmap(bitmap,backgroundbitmap2,0,0,-scroll,skew,&clip,TRANSPARENCY_NONE,0);
				else
					copybitmap(bitmap,backgroundbitmap1,0,0,-scroll,skew,&clip,TRANSPARENCY_NONE,0);

				skew += 2;
			}
		}
		else
		{
			/* skew background up one pixel every 2 horizontal pixels */
			if (zaxxon_vid_type == CONGO_VID)
				scroll = 2050 + 2*(zaxxon_background_position[0] + 256*zaxxon_background_position[1])
					- backgroundbitmap1->height + 256;
			else
				scroll = 2*(zaxxon_background_position[0] + 256*(zaxxon_background_position[1]&7))
					- backgroundbitmap1->height + 256;

			skew = 64 - (255 - Machine->drv->visible_area.max_y);

			clip.min_x = Machine->drv->visible_area.min_x;
			clip.max_x = Machine->drv->visible_area.max_x;

			for (i = Machine->drv->visible_area.max_y;i >= Machine->drv->visible_area.min_y;i-=2)
			{
				clip.min_y = i-1;
				clip.max_y = i;

				if ((zaxxon_vid_type == ZAXXON_VID || zaxxon_vid_type == FUTSPY_VID)
						&& (*zaxxon_background_color_bank & 1))
					copybitmap(bitmap,backgroundbitmap2,0,0,skew,scroll,&clip,TRANSPARENCY_NONE,0);
				else
					copybitmap(bitmap,backgroundbitmap1,0,0,skew,scroll,&clip,TRANSPARENCY_NONE,0);

				skew--;
			}
		}
	}
	else fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);


	if (zaxxon_vid_type == CONGO_VID)
	{
		int i;
		static unsigned int sprpri[0x100]; /* this really should not be more
		                             * than 0x1e, but I did not want to check
		                             * for 0xff which is set when sprite is off
		                             * -V-
		                             */

		/* Draw the sprites. Note that it is important to draw them exactly in this */
		/* order, to have the correct priorities. */
		/* Sprites actually start at 0xff * [0xc031], it seems to be static tho'*/
		/* The number of active sprites is stored at 0xc032 */

		for (offs = 0x1e * 0x20 ;offs >= 0x00 ;offs -= 0x20)
			sprpri[ spriteram[offs+1] ] = offs;

		for (i=0x1e ; i>=0; i--)
		{
			offs = sprpri[i];

			if (spriteram[offs+2] != 0xff)
			{
				drawgfx(bitmap,Machine->gfx[2],
						spriteram[offs+2+1]& 0x7f,
						spriteram[offs+2+2],
						spriteram[offs+2+2] & 0x80,spriteram[offs+2+1] & 0x80,
						((spriteram[offs+2+3] + 16) & 0xff) - 31,255 - spriteram[offs+2] - 15,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}
	else if (zaxxon_vid_type == FUTSPY_VID)
	{
		/* Draw the sprites. Note that it is important to draw them exactly in this */
		/* order, to have the correct priorities. */
		for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
		{
			if (spriteram[offs] != 0xff)
			{
					drawgfx(bitmap,Machine->gfx[2],
						spriteram[offs+1] & 0x7f,
						spriteram[offs+2] & 0x3f,
						spriteram[offs+1] & 0x80,spriteram[offs+1] & 0x80,	/* ?? */
						((spriteram[offs+3] + 16) & 0xff) - 32,255 - spriteram[offs] - 16,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}
	else
	{
		/* Draw the sprites. Note that it is important to draw them exactly in this */
		/* order, to have the correct priorities. */
		for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
		{
			if (spriteram[offs] != 0xff)
			{
					drawgfx(bitmap,Machine->gfx[2],
						spriteram[offs+1] & 0x3f,
						spriteram[offs+2] & 0x3f,
						spriteram[offs+1] & 0x40,spriteram[offs+1] & 0x80,
						((spriteram[offs+3] + 16) & 0xff) - 32,255 - spriteram[offs] - 16,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;
		int color;


		sy = offs / 32;
		sx = offs % 32;

		if (zaxxon_vid_type == CONGO_VID)
			color = colorram[offs];
		else
			/* not sure about the color code calculation - char_color_bank is used only in test mode */
			color =	(color_codes[sx + 32 * (sy/4)] & 0x0f) + 16 * (*zaxxon_char_color_bank & 1);

		drawgfx(bitmap,Machine->gfx[0],
				videoram[offs],
				color,
				0,0,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
