/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *ramtek_videoram;

/* palette colors (see drivers/8080bw.c) */
enum { BLACK, WHITE };





/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int ramtek_vh_start (void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void ramtek_vh_stop (void)
{
	osd_free_bitmap (tmpbitmap);
}

static void plot_pixel_8080 (int x, int y, int col)
{
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = x;
		x = y;
		y = temp;
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
		x = 255 - x;
	if (Machine->orientation & ORIENTATION_FLIP_Y)
		y = 255 - y;

	tmpbitmap->line[y][x] = col;
	/* TODO: we should mark 8 bits dirty at a time */
	osd_mark_dirty (x,y,x,y,0);
}

static unsigned char mask = 0;

void ramtek_mask_w(int offset, int data){
	mask = data;
}

void ramtek_videoram_w (int offset,int data)
{
	data = data & ~mask;

	if (ramtek_videoram[offset] != data)
	{
		int i,x,y;

		ramtek_videoram[offset] = data;

		y = offset / 32;
		x = 8 * (offset % 32);

		for (i = 0; i < 8; i++)
		{
			if (!(data & 0x80))
				plot_pixel_8080 (x, y, Machine->pens[BLACK]);
			else
				plot_pixel_8080 (x, y, Machine->pens[WHITE]);

			x ++;
			data <<= 1;
		}
	}
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void ramtek_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
