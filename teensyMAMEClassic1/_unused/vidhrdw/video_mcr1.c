/***************************************************************************

  vidhrdw/mcr1.c

  Functions to emulate the video hardware of an mcr 1 style machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *mcr1_paletteram_bg,*mcr1_paletteram_r;
static int spriteoffset;


void solarfox_init(void)
{
	spriteoffset = 3;
}

void kick_init(void)
{
	spriteoffset = -3;
}


/***************************************************************************

  Generic MCR/I video routines

***************************************************************************/

void mcr1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int mx,my;


	if (full_refresh)
		memset (dirtybuffer, 1, videoram_size);

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			dirtybuffer[offs] = 0;

			mx = (offs) % 32;
			my = (offs) / 32;

			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs],
					0,
					0,0,
					16*mx,16*my,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int code,x,y,sx,sy;

		if (spriteram[offs] == 0) continue;

		x = (spriteram[offs+2]+spriteoffset)*2;
		y = (241-spriteram[offs])*2;
		code = spriteram[offs+1] & 0x3f;

		drawgfx(bitmap,Machine->gfx[1],
				code,
				0,
				spriteram[offs+1] & 0x40, spriteram[offs+1] & 0x80,
				x,y,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

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
						dirtybuffer[x2 + 32*y2] = 1;
				}
			}
		}

   }
}
