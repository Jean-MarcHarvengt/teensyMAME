/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *gundealr_bsvideoram;
unsigned char *gundealr_bigspriteram;



void gundealr_paletteram_w(int offset,int data)
{
	int r,g,b,val;


	paletteram[offset] = data;

	val = paletteram[offset & ~1];
	r = (val >> 4) & 0x0f;
	g = (val >> 0) & 0x0f;

	val = paletteram[offset | 1];
	b = (val >> 4) & 0x0f;
	/* TODO: the bottom 4 bits are used as well, but I'm not sure about the meaning */

	r = 0x11 * r;
	g = 0x11 * g;
	b = 0x11 * b;

	palette_change_color(offset / 2,r,g,b);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void gundealr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	if (palette_recalc())
		memset(dirtybuffer,1,videoram_size);


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 2;offs >= 0;offs -= 2)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs + 1])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;
			dirtybuffer[offs + 1] = 0;

			sx = (offs/2) / 32;
			sy = (offs/2) % 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((videoram[offs + 1] & 0x07) << 8),
					(videoram[offs + 1] & 0xf0) >> 4,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* draw the front layer */
	{
		int sx,sy,x,y;


		sx = 256 * (gundealr_bigspriteram[0] & 0x01) - gundealr_bigspriteram[1];
		sy = 256 * (gundealr_bigspriteram[2] & 0x01) - gundealr_bigspriteram[3];

		for (y = 0;y < 16;y++)
		{
			for (x = 0;x < 16;x++)
			{
				drawgfx(bitmap,Machine->gfx[1],
						gundealr_bsvideoram[2*y + 0x20*x] + ((gundealr_bsvideoram[1 + 2*y + 0x20*x] & 0x03) << 8),
						(gundealr_bsvideoram[1 + 2*y + 0x20*x] & 0xf0) >> 4,
						0,0,
						sx + 16*x,sy + 16*y,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,15);
			}
		}
	}
}
