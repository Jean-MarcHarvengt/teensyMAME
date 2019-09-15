/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char mappy_scroll;

static unsigned char *transparency;
static int motos_special_display;


/***************************************************************************

  Convert the color PROMs into a more useable format.

  mappy has one 32x8 palette PROM and two 256x4 color lookup table PROMs
  (one for characters, one for sprites).

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
void mappy_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;

		bit0 = (color_prom[31-i] >> 0) & 0x01;
		bit1 = (color_prom[31-i] >> 1) & 0x01;
		bit2 = (color_prom[31-i] >> 2) & 0x01;
		palette[3*i] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (color_prom[31-i] >> 3) & 0x01;
		bit1 = (color_prom[31-i] >> 4) & 0x01;
		bit2 = (color_prom[31-i] >> 5) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (color_prom[31-i] >> 6) & 0x01;
		bit2 = (color_prom[31-i] >> 7) & 0x01;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}

	/* characters */
	for (i = 0*4;i < 64*4;i++)
		colortable[i] = 31 - ((color_prom[(i^3) + 32] & 0x0f) + 0x10);

	/* sprites */
	for (i = 64*4;i < Machine->drv->color_table_len;i++)
		colortable[i] = 31 - (color_prom[i + 32] & 0x0f);

   /* now check our characters and mark which ones are completely solid-color */
   {
      struct GfxElement *gfx;
      unsigned char *dp;
      int x, y, color;

      transparency = malloc (64*256);
      if (!transparency)
      	return;
      memset (transparency, 0, 64*256);

      gfx = Machine->gfx[0];
      for (i = 0; i < gfx->total_elements; i++)
      {
			color = gfx->gfxdata->line[i * gfx->height][0];

			for (y = 0; y < gfx->height; y++)
			{
				dp = gfx->gfxdata->line[i * gfx->height + y];
				for (x = 0; x < gfx->width; x++)
					if (dp[x] != color)
						goto done;
			}

			for (y = 0; y < 64; y++)
				if (colortable[y*4 + color] == 0)
					transparency[(i << 6) + y] = 1;

		done:
			;
      }
   }
}


static int common_vh_start(void)
{
	if ((dirtybuffer = malloc(videoram_size)) == 0)
		return 1;
	memset (dirtybuffer, 1, videoram_size);

	if ((tmpbitmap = osd_create_bitmap (60*8,36*8)) == 0)
	{
		free (dirtybuffer);
		return 1;
	}

	return 0;
}

int mappy_vh_start(void)
{
	motos_special_display = 0;
	return common_vh_start();
}

int motos_vh_start(void)
{
	motos_special_display = 1;
	return common_vh_start();
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void mappy_vh_stop(void)
{
	free(transparency);
	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap);
}



void mappy_videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;
		videoram[offset] = data;
	}
}


void mappy_colorram_w(int offset,int data)
{
	if (colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;
		colorram[offset] = data;
	}
}


void mappy_scroll_w(int offset,int data)
{
	mappy_scroll = offset >> 3;
}



void mappy_draw_sprite(struct osd_bitmap *dest,unsigned int code,unsigned int color,
	int flipx,int flipy,int sx,int sy)
{
	if (motos_special_display) sx--;

	drawgfx(dest,Machine->gfx[1],code,color,flipx,flipy,sx,sy,&Machine->drv->visible_area,
		TRANSPARENCY_COLOR,16);
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void mappy_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	static unsigned short overoffset[2048];
	unsigned short *save = overoffset;
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int color = colorram[offs];
		int video = videoram[offs];

		/* characters with bit 0x40 set are higher priority than sprites; remember and redraw later */
		if (color & 0x40)
			if (!transparency[(video << 6) + (color & 0x3f)])
				*save++ = offs;

		if (dirtybuffer[offs])
		{
			int sx,sy,mx,my;


			dirtybuffer[offs] = 0;

			if (offs >= videoram_size - 64)
			{
				int off;


				off = offs;
				if (motos_special_display)
				{
					if (off == 0x07d1 || off == 0x07d0 || off == 0x07f1 || off == 0x07f0)
						off -= 0x10;
					if (off == 0x07c1 || off == 0x07c0 || off == 0x07e1 || off == 0x07e0)
						off += 0x10;
				}

				/* Draw the top 2 lines. */
				mx = off % 32;
				my = (off - (videoram_size - 64)) / 32;

				sx = 29 - mx;
				sy = my;
			}
			else if (offs >= videoram_size - 128)
			{
				/* Draw the bottom 2 lines. */
				mx = offs % 32;
				my = (offs - (videoram_size - 128)) / 32;

				sx = 29 - mx;
				sy = my + 34;
			}
			else
			{
				/* draw the rest of the screen */
				mx = offs / 32;
				my = offs % 32;

				sx = 59 - mx;
				sy = my + 2;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					video,
					color,
					0,0,8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[36];


		for (offs = 0;offs < 2;offs++)
			scroll[offs] = 0;
		for (offs = 2;offs < 34;offs++)
			scroll[offs] = mappy_scroll - 256;
		for (offs = 34;offs < 36;offs++)
			scroll[offs] = 0;

		copyscrollbitmap(bitmap,tmpbitmap,36,scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 2)
	{
		/* is it on? */
		if ((spriteram_3[offs+1] & 2) == 0)
		{
			int sprite = spriteram[offs];
			int color = spriteram[offs+1];
			int x = spriteram_2[offs]-16;
			int y = (spriteram_2[offs+1]-40) + 0x100*(spriteram_3[offs+1] & 1);
			int flipx = spriteram_3[offs] & 2;
			int flipy = spriteram_3[offs] & 1;

			switch (spriteram_3[offs] & 0x0c)
			{
				case 0:		/* normal size */
					mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,x,y);
					break;

				case 4:		/* 2x vertical */
					sprite &= ~1;
					if (!flipy)
					{
						mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,x,y);
						mappy_draw_sprite(bitmap,1+sprite,color,flipx,flipy,x,16+y);
					}
					else
					{
						mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,x,16+y);
						mappy_draw_sprite(bitmap,1+sprite,color,flipx,flipy,x,y);
					}
					break;

				case 8:		/* 2x horizontal */
					sprite &= ~2;
					if (!flipx)
					{
						mappy_draw_sprite(bitmap,2+sprite,color,flipx,flipy,x,y);
						mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,16+x,y);
					}
					else
					{
						mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,x,y);
						mappy_draw_sprite(bitmap,2+sprite,color,flipx,flipy,16+x,y);
					}
					break;

				case 12:		/* 2x both ways */
					sprite &= ~3;
					if (!flipy && !flipx)
					{
						mappy_draw_sprite(bitmap,2+sprite,color,flipx,flipy,x,y);
						mappy_draw_sprite(bitmap,3+sprite,color,flipx,flipy,x,16+y);
						mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,16+x,y);
						mappy_draw_sprite(bitmap,1+sprite,color,flipx,flipy,16+x,16+y);
					}
					else if (flipy && flipx)
					{
						mappy_draw_sprite(bitmap,1+sprite,color,flipx,flipy,x,y);
						mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,x,16+y);
						mappy_draw_sprite(bitmap,3+sprite,color,flipx,flipy,16+x,y);
						mappy_draw_sprite(bitmap,2+sprite,color,flipx,flipy,16+x,16+y);
					}
					else if (flipx)
					{
						mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,x,y);
						mappy_draw_sprite(bitmap,1+sprite,color,flipx,flipy,x,16+y);
						mappy_draw_sprite(bitmap,2+sprite,color,flipx,flipy,16+x,y);
						mappy_draw_sprite(bitmap,3+sprite,color,flipx,flipy,16+x,16+y);
					}
					else /* flipy */
					{
						mappy_draw_sprite(bitmap,3+sprite,color,flipx,flipy,x,y);
						mappy_draw_sprite(bitmap,2+sprite,color,flipx,flipy,x,16+y);
						mappy_draw_sprite(bitmap,1+sprite,color,flipx,flipy,16+x,y);
						mappy_draw_sprite(bitmap,sprite,color,flipx,flipy,16+x,16+y);
					}
					break;
			}
		}
	}

	/* Draw the high priority characters */
	while (save > overoffset)
	{
		int sx,sy,mx,my;

		offs = *--save;

		if (offs >= videoram_size - 64)
		{
			/* Draw the top 2 lines. */
			mx = offs % 32;
			my = (offs - (videoram_size - 64)) / 32;

			sx = 29 - mx;
			sy = my;

			sx *= 8;
		}
		else if (offs >= videoram_size - 128)
		{
			/* Draw the bottom 2 lines. */
			mx = offs % 32;
			my = (offs - (videoram_size - 128)) / 32;

			sx = 29 - mx;
			sy = my + 34;

			sx *= 8;
		}
		else
		{
			/* draw the rest of the screen */
			mx = offs / 32;
			my = offs % 32;

			sx = 59 - mx;
			sy = my + 2;

			sx = (8*sx+mappy_scroll-256);
		}

		drawgfx(bitmap,Machine->gfx[0],
				videoram[offs],
				colorram[offs],
				0,0,sx,8*sy,
				0,TRANSPARENCY_COLOR,0);
	}
}
