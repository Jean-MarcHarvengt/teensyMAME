#include "driver.h"
#include "vidhrdw/generic.h"

/* sprite control : CCCCFFTT */

static void sidepocket_drawsprites( struct osd_bitmap *bitmap ) {

	int offs;

	for (offs = 0;offs < spriteram_size; offs += 4)
	{
		if ( spriteram[offs+1] != 0xf8 )	/* hack - seems like palette 0xf is all black */
		{
			int sx,sy,code,color,flipx,flipy;

			code = spriteram[offs+3] + ( ( spriteram[offs+1] & 3 ) << 8 );

			sx = spriteram[offs+2];
			sy = spriteram[offs];
			color = (spriteram[offs+1] & 0xf0) >> 4;

			flipx = spriteram[offs+1] & 0x8;
			flipy = spriteram[offs+1] & 0x4;

			drawgfx(bitmap,Machine->gfx[1],
					code,
					color,
					flipx,flipy,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}

void sidepocket_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for ( offs = videoram_size - 1; offs >= 0; offs-- ) {
		if ( dirtybuffer[offs] ) {
			int sx,sy,code;

			dirtybuffer[offs] = 0;

			sx = 31 - (offs % 32);
			sy = (offs / 32);

			code = ( videoram[offs] ) + ( (colorram[offs] & 7 ) << 8 );

			drawgfx(tmpbitmap,Machine->gfx[0],
					code,
					( colorram[offs] & 0xf0 ) >> 4,
					0,0,
					sx*8,sy*8,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	sidepocket_drawsprites( bitmap );
}
