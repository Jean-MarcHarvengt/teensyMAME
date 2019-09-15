/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  The video display is just a 256x256 bitmap in memory ($0000-$1FFF).
  We decode it as 8x1 characters for convenience.  Note that the top
  two rows are not displayed since $0000-$01FF is used by the 6502 for
  temporary storage and stack space.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void avalnche_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	/* Don't bother checking 0x0000-0x01FF since we won't draw it anyways */
	for (offs = videoram_size - 1;offs >= 0x200;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,color;

			/* decode modified background */
			decodechar(Machine->gfx[0],offs,videoram,
						Machine->drv->gfxdecodeinfo[0].gfxlayout);

			dirtybuffer[offs] = 0;

			sx = 8 * (offs % 32);
			sy = (offs / 32);

			color = 0;
			drawgfx(tmpbitmap,Machine->gfx[0],
					offs,color,
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

			if (!full_refresh)
				drawgfx(bitmap,Machine->gfx[0],
					offs,color,
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	if (full_refresh)
		/* copy the character mapped graphics */
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}

