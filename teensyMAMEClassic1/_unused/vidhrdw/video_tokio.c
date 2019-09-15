#include "driver.h"
#include "vidhrdw/generic.h"
#include "ctype.h"

extern unsigned char *tokio_sharedram;

unsigned char *tokio_objectram;
int tokio_objectram_size;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int tokio_vh_start(void)
{
	/* In Tokio the video RAM and the color RAM are interleaved, */
	/* forming a contiguous memory region 0x800 bytes long. */
	/* We only need half that size for the dirtybuffer. */
	if ((dirtybuffer = malloc(videoram_size / 2)) == 0)
		return 1;
	memset(dirtybuffer,1,videoram_size / 2);

	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width+4*8,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

	return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void tokio_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
	free(dirtybuffer);
}

int tokio_videoram_r(int offset)
{
	return videoram[offset];
}

void tokio_videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset / 2] = 1;
		videoram[offset] = data;
	}
}

int tokio_objectram_r(int offset)
{
	return tokio_objectram[offset];
}

void tokio_objectram_w(int offset,int data)
{
	if (tokio_objectram[offset] != data)
	{
		/* gfx bank selector for the background playfield */
		if (offset < 0x40 && offset % 4 == 3)
			memset(dirtybuffer,1,videoram_size / 2);
		tokio_objectram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void tokio_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,offset,width,height;
	int row_scroll,col_scroll[18];
	int sx,sy,xc,yc;
	int gfx_map,gfx_bank,gfx_offs;
	int color,goffs;


	/* recalc the palette if necessary */
	if (palette_recalc ())
		memset (dirtybuffer,1,videoram_size / 2);

	/* Tokio doesn't have a real video RAM! */
	/* All graphics (characters and sprites) are stored in the same */
	/* memory region, and information on the background character */
	/* columns is stored in the area dd00-dd3f. */

	/* setup background tiles in temporary buffer */
	for (offset = 0;offset < 0x48;offset += 4)
	{
		gfx_map = tokio_objectram[offset + 1] & 0x3f;
		gfx_bank = tokio_objectram[offset + 3] & 0x3f;

		sx = offset * 4;
		gfx_offs = gfx_map * 0x80;

		for (xc = 0;xc < 2;xc++)
		{
			for (yc = 0;yc < 32;yc++)
			{
				goffs = gfx_offs + xc * 0x40 + yc * 0x02;
				color = (videoram[goffs + 1] & 0x3c) >> 2;
				if (dirtybuffer[goffs / 2])
				{
					dirtybuffer[goffs / 2] = 0;
					drawgfx(tmpbitmap,Machine->gfx[0],
						videoram[goffs] + 256 * (videoram[goffs + 1] & 0x03) + 1024 * (gfx_bank & 0x0f),
						color,
						videoram[goffs + 1] & 0x40,videoram[goffs + 1] & 0x80,
						sx + xc * 8,yc * 8,
						0,TRANSPARENCY_NONE,0);
				}
			}
		}
	}

	/* copy background to the screen */
	if (tokio_objectram[0 + 2])
		row_scroll = tokio_sharedram[0x473] * 0x20 + tokio_sharedram[0x472] - 1;
	else
		row_scroll = 0;
	for (i = 0;i < 18;i++)
		col_scroll[i]=-tokio_objectram[i * 4 + 0];
	copyscrollbitmap(bitmap,tmpbitmap,
		1,&row_scroll,18,col_scroll,
		&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* draw the sprites */
	for (offset = 0x48;offset < tokio_objectram_size;offset += 4)
	{
		/* skip empty sprites */
		if (!(tokio_objectram[offset+0]+tokio_objectram[offset+1]+tokio_objectram[offset+2]+tokio_objectram[offset+3]))
			continue;

		gfx_map = tokio_objectram[offset + 1];
		gfx_bank = tokio_objectram[offset + 3];

		sx = tokio_objectram[offset + 2];
		if ((gfx_map & 0x80) == 0) /* sprites */
		{
			sy = 256 - 2*8 - (tokio_objectram[offset + 0]);
			gfx_offs = ((gfx_map & 0x1f) * 0x80) + ((gfx_map & 0x60) >> 1) + 12;
			height = 2;
			width = 2;
		}
		else /* foreground chars (each "sprite" is one 16x256 column) */
		{
			sy = -(tokio_objectram[offset + 0]);
			gfx_offs = ((gfx_map & 0x3f) * 0x80);
			width = 32;
			height = 2;
			if (gfx_map == 0x8d)
				height = 4;  /* hack for big tank on ground */
		}

		for (xc = 0;xc < height;xc++)
		{
			for (yc = 0;yc < width;yc++)
			{
				goffs = gfx_offs + xc * 0x40 + yc * 0x02;
				drawgfx(bitmap,Machine->gfx[0],
					videoram[goffs] + 256 * (videoram[goffs + 1] & 0x03) + 1024 * (gfx_bank & 0x0f),
					(videoram[goffs + 1] & 0x3c) >> 2,
					videoram[goffs + 1] & 0x40,videoram[goffs + 1] & 0x80,
					-4*(gfx_bank & 0x40) + sx + xc * 8, sy + yc * 8,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

				if (width == 32)
				{
					/* redraw shifted down 256 pixels for wraparound */
					drawgfx(bitmap,Machine->gfx[0],
						videoram[goffs] + 256 * (videoram[goffs + 1] & 0x03) + 1024 * (gfx_bank & 0x0f),
						(videoram[goffs + 1] & 0x3c) >> 2,
						videoram[goffs +  1] & 0x40,videoram[goffs + 1] & 0x80,
						-4*(gfx_bank & 0x40) + sx + xc * 8, sy + yc * 8 + 256,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				}
			}
		}
	}
}
