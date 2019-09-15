/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "ctype.h"



unsigned char *bublbobl_objectram;
int bublbobl_objectram_size;



void bublbobl_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	/* no color PROMs here, only RAM, but the gfx data is inverted so we */
	/* cannot use the default lookup table */
	for (i = 0;i < Machine->drv->color_table_len;i++)
		colortable[i] = i ^ 0x0f;
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int bublbobl_vh_start(void)
{
	/* In Bubble Bobble the video RAM and the color RAM and interleaved, */
	/* forming a contiguous memory region 0x800 bytes long. We only need half */
	/* that size for the dirtybuffer */
	if ((dirtybuffer = malloc(videoram_size / 2)) == 0)
		return 1;
	memset(dirtybuffer,1,videoram_size / 2);

	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void bublbobl_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
	free(dirtybuffer);
}



void bublbobl_videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset / 2] = 1;
		videoram[offset] = data;
	}
}



void bublbobl_objectram_w(int offset,int data)
{
	if (bublbobl_objectram[offset] != data)
	{
		/* gfx bank selector for the background playfield */
		if (offset < 0x40 && offset % 4 == 3) memset(dirtybuffer,1,videoram_size / 2);
		bublbobl_objectram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void bublbobl_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int sx,sy,xc,yc;
	int gfx_num,gfx_code,gfx_offs;


	/* recalc the palette if necessary */
	if (palette_recalc ())
		memset (dirtybuffer,1,videoram_size / 2);


	/* Bubble Bobble doesn't have a real video RAM. All graphics (characters */
	/* and sprites) are stored in the same memory region, and information on */
	/* the background character columns is stored inthe area dd00-dd3f */
	for (offs = 0;offs < 16;offs++)
	{
		gfx_num = bublbobl_objectram[4*offs + 1];

		gfx_code = bublbobl_objectram[4*offs + 3];

		sx = offs * 16;
		gfx_offs = ((gfx_num & 0x3f) * 0x80);

		for (xc = 0;xc < 2;xc++)
		{
			for (yc = 0;yc < 32;yc++)
			{
				int goffs,color;


				goffs = gfx_offs + xc * 0x40 + yc * 0x02;
				color = (videoram[goffs + 1] & 0x3c) >> 2;
				if (dirtybuffer[goffs / 2])
				{
					dirtybuffer[goffs / 2] = 0;

					drawgfx(tmpbitmap,Machine->gfx[0],
							videoram[goffs] + 256 * (videoram[goffs + 1] & 0x03) + 1024 * (gfx_code & 0x0f),
							color,
							videoram[goffs + 1] & 0x40,videoram[goffs + 1] & 0x80,
							sx + xc * 8,yc * 8,
							0,TRANSPARENCY_NONE,0);
				}
			}
		}
	}


	/* copy the background graphics to the screen */
	{
		int scroll[16];


		for (offs = 0;offs < 16;offs++)
			scroll[offs] = -bublbobl_objectram[4*offs];

		copyscrollbitmap(bitmap,tmpbitmap,0,0,16,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* draw the sprites */
	for (offs = 0x40;offs < bublbobl_objectram_size;offs += 4)
    {
		int height;


		gfx_num = bublbobl_objectram[offs + 1];

		gfx_code = bublbobl_objectram[offs + 3];

		sx = bublbobl_objectram[offs + 2];
		if ((gfx_num & 0x80) == 0)	/* sprites */
		{
			sy = 256 - 8*8 - (bublbobl_objectram[offs + 0]);
			gfx_offs = ((gfx_num & 0x1f) * 0x80) + ((gfx_num & 0x60) >> 1);
			height = 8;
		}
		else	/* foreground chars (each "sprite" is one 16x256 column) */
		{
			sy = -(bublbobl_objectram[offs + 0]);
			gfx_offs = ((gfx_num & 0x3f) * 0x80);
			height = 32;
		}

		for (xc = 0;xc < 2;xc++)
		{
			for (yc = 0;yc < height;yc++)
			{
				int goffs;


				goffs = gfx_offs + xc * 0x40 + yc * 0x02;
				if (videoram[goffs])
				{
					drawgfx(bitmap,Machine->gfx[0],
							videoram[goffs] + 256 * (videoram[goffs + 1] & 0x03) + 1024 * (gfx_code & 0x0f),
							(videoram[goffs + 1] & 0x3c) >> 2,
							videoram[goffs + 1] & 0x40,videoram[goffs + 1] & 0x80,
							-4*(gfx_code & 0x40) + sx + xc * 8,sy + yc * 8,
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

					if (height == 32)
					{
						/* redraw shifted down 256 pixels for wraparound */
						drawgfx(bitmap,Machine->gfx[0],
								videoram[goffs] + 256 * (videoram[goffs + 1] & 0x03) + 1024 * (gfx_code & 0x0f),
								(videoram[goffs + 1] & 0x3c) >> 2,
								videoram[goffs + 1] & 0x40,videoram[goffs + 1] & 0x80,
								-4*(gfx_code & 0x40) + sx + xc * 8,sy + yc * 8 + 256,
								&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
					}
				}
			}
		}
	}
}
