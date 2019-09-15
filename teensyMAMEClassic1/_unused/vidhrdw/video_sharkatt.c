/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int color_plane = 0;

/***************************************************************************
 sharkatt_vtcsel_w

 TODO:  This writes to a TMS9927 VTAC.  Do we care?
 **************************************************************************/
void sharkatt_vtcsel_w(int offset, int data)
{
}

/***************************************************************************
 sharkatt_color_plane_w
 **************************************************************************/
void sharkatt_color_plane_w(int offset, int data)
{
	/* D0-D3 = WS0-WS3, D4-D5 = RS0-RS1 */
	/* RS = CPU Memory Plane Read Multiplex Select */
	color_plane = (data & 0x3F);
}

/***************************************************************************
 sharkatt_color_map_w
 **************************************************************************/
void sharkatt_color_map_w(int offset, int data)
{
    int vals[4] = {0x00,0x55,0xAA,0xFF};
    int r,g,b;

    r = vals[(data & 0x03) >> 0];
    g = vals[(data & 0x0C) >> 2];
    b = vals[(data & 0x30) >> 4];
	palette_change_color(offset,r,g,b);
}

/***************************************************************************
 sharkatt_videoram_w
 **************************************************************************/
void sharkatt_videoram_w(int offset,int data)
{
	if ((dirtybuffer[offset]) || (videoram[offset] != data))
	{
		int i,x,y;

        dirtybuffer[offset] = 0;

		videoram[offset] = data;

		y = offset / 32;
		x = 8 * (offset % 32);

		for (i = 0;i < 8;i++)
		{
			int col;


			col = Machine->pens[color_plane & 0x0F];

			if (data & 0x80) Machine->scrbitmap->line[y][x] = tmpbitmap->line[y][x] = col;
			else Machine->scrbitmap->line[y][x] = tmpbitmap->line[y][x] = Machine->pens[0];

			osd_mark_dirty(x,y,x,y,0);

			x++;
			data <<= 1;
		}
	}
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void sharkatt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	if (full_refresh)
		/* copy the character mapped graphics */
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
