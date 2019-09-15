/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *xain_videoram;
unsigned char *xain_videoram2;
int xain_videoram_size;
int xain_videoram2_size;

static struct osd_bitmap *tmpbitmap2;
static struct osd_bitmap *tmpbitmap3;
unsigned char *dirtybuffer2;

static unsigned char xain_scrollxP2[2];
static unsigned char xain_scrollyP2[2];
static unsigned char xain_scrollxP3[2];
static unsigned char xain_scrollyP3[2];

void xain_vh_stop(void);



void xain_scrollxP2_w(int offset,int data)
{
        xain_scrollxP2[offset] = data;
}

void xain_scrollyP2_w(int offset,int data)
{
        xain_scrollyP2[offset] = data;
}

void xain_scrollxP3_w(int offset,int data)
{
        xain_scrollxP3[offset] = data;
}

void xain_scrollyP3_w(int offset,int data)
{
        xain_scrollyP3[offset] = data;
}

void xain_videoram2_w(int offset,int data)
{
        if (xain_videoram2[offset] != data)
        {  dirtybuffer2[offset] = 1;
           xain_videoram2[offset] = data;}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int xain_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

        if ((dirtybuffer2 = malloc(xain_videoram2_size)) == 0)
	{
		xain_vh_stop();
		return 1;
	}
        memset(dirtybuffer2,1,xain_videoram2_size);

        /* the background area is 2 x 2 */
        if ((tmpbitmap2 = osd_new_bitmap(2*Machine->drv->screen_width,
                2*Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		xain_vh_stop();
		return 1;
	}

        /* the background area is 2 x 2 */
        if ((tmpbitmap3 = osd_new_bitmap(2*Machine->drv->screen_width,
                2*Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		xain_vh_stop();
		return 1;
	}

	return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void xain_vh_stop(void)
{
        free(dirtybuffer2);
	osd_free_bitmap(tmpbitmap2);
        osd_free_bitmap(tmpbitmap3);
	generic_vh_stop();
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void xain_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
        struct rectangle *r = malloc(sizeof(int)*4);

	if (palette_recalc())
	{
		memset(dirtybuffer,1,videoram_size);
		memset(dirtybuffer2,1,xain_videoram2_size);
	}

        /* background Plane 3 & Plane 2*/
	for (offs = (videoram_size/2)-1;offs >= 0;offs--)
	{
		if ((dirtybuffer[offs]) || (dirtybuffer[offs+0x400]))
		{
			int sx,sy;
                        int banktile = ((videoram[offs+0x400]&6)>>1);
                        int numtile = videoram[offs] +
                        ((videoram[offs+0x400]&1)<<8);
                        int flipx = ((videoram[offs+0x400]&0x80)>>7);
                        int color = ((videoram[offs+0x400]&0x70)>>4);

			dirtybuffer[offs] = dirtybuffer[offs+0x400] = 0;

                        sx = ((offs>>8)&1)*16 + (offs&0xff)%16;
                        sy = ((offs>>9)&1)*16 + (offs&0xff)/16;

			drawgfx(tmpbitmap2,Machine->gfx[5+banktile],
                                        numtile,
					color, // color
					flipx,0,
					16 * sx,16 * sy,
					0,TRANSPARENCY_NONE,0);
		}

		if ((dirtybuffer2[offs]) || (dirtybuffer2[offs+0x400]))
		{
                        int numtile = xain_videoram2[offs] +
                        ((xain_videoram2[offs+0x400]&1)<<8);
			int sx,sy;

                        sx = ((offs>>8)&1)*16 + (offs&0xff)%16;
                        sy = ((offs>>9)&1)*16 + (offs&0xff)/16;

                        dirtybuffer2[offs] = dirtybuffer2[offs+0x400] = 0;

                        if (!numtile)
                        {
                          r->min_x = 16*sx;
                          r->max_x = 16*sx+15;
                          r->min_y = 16*sy;
                          r->max_y = 16*sy+15;
                          fillbitmap(tmpbitmap3,0,r);
                        } else {
                          int banktile = ((xain_videoram2[offs+0x400]&6)>>1);
                          int flipx = ((xain_videoram2[offs+0x400]&0x80)>>7);
                          int color = ((xain_videoram2[offs+0x400]&0x70)>>4);

                          /* Necessary to use TRANSPARENCY_PEN here */
                          r->min_x = 16*sx;
                          r->max_x = 16*sx+15;
                          r->min_y = 16*sy;
                          r->max_y = 16*sy+15;
                          fillbitmap(tmpbitmap3,0,r);
                          /******************************************/

			  drawgfx(tmpbitmap3,Machine->gfx[1+banktile],
                                        numtile,
					color,
					flipx,0,
					16 * sx,16 * sy,
					0,TRANSPARENCY_PEN,0);
                        }
		}
	}

        /* copy the background graphics */
	{
                int scrollx,scrolly;

                scrollx = -(xain_scrollxP3[0] + 256 * xain_scrollxP3[1]);
                scrolly = -(xain_scrollyP3[0] + 256 * xain_scrollyP3[1]);

		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,
                &scrolly,&Machine->drv->visible_area,
                TRANSPARENCY_NONE,0);

                scrollx = -(xain_scrollxP2[0] + 256 * xain_scrollxP2[1]);
                scrolly = -(xain_scrollyP2[0] + 256 * xain_scrollyP2[1]);

		copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,1,
                &scrolly,&Machine->drv->visible_area,
                TRANSPARENCY_PEN,0);
	}

        /* draw the sprites */
	for (offs = 0; offs < spriteram_size ;offs+=4)
	{
                int sx,sy;
                int numtile = spriteram[offs+2] +
                              ((spriteram[offs+1]&1)<<8);
                int banktile = ((spriteram[offs+1]&6)>>1);
                int color = ((spriteram[offs+1]&0x38)>>3);

                sx = spriteram[offs+3];
                sy = spriteram[offs];

		if ((sx < 0xf8) && (sy <0xf8)) /* else don't draw */
		{
                  switch(spriteram[offs+1]&0xC0)
                  {
                    case 0x80: // 16x32 - NOT Flip
                      drawgfx(bitmap,Machine->gfx[9+banktile],
		      numtile,
		      color,
		      0,0,// FlipX, FlipY
		      0xf0-sx,0xe0-sy,
		      &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
                      drawgfx(bitmap,Machine->gfx[9+banktile],
		      numtile+1,
		      color,
		      0,0,// FlipX, FlipY
		      0xf0-sx,0xf0-sy,
		      &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
                    break;
                    case 0xC0: // 16x32 - Flip
                      drawgfx(bitmap,Machine->gfx[9+banktile],
		      numtile,
		      color,
		      1,0,// FlipX, FlipY
		      0xf0-sx,0xe0-sy,
		      &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
                      drawgfx(bitmap,Machine->gfx[9+banktile],
		      numtile+1,
		      color,
		      1,0,// FlipX, FlipY
		      0xf0-sx,0xf0-sy,
		      &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
                    break;
                    case 0x00: // 16x16 - NOT Flip
                      drawgfx(bitmap,Machine->gfx[9+banktile],
		      numtile,
		      color,
		      0,0,// FlipX, FlipY
		      0xf0-sx,0xf0-sy,
		      &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
                    break;
                    case 0x40: // 16x16 - Flip
                      drawgfx(bitmap,Machine->gfx[9+banktile],
		      numtile,
		      color,
		      1,0,// FlipX, FlipY
		      0xf0-sx,0xf0-sy,
		      &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
                    break;
                 }
		}
	}

        /* draw the frontmost playfield. They are characters. */
	for (offs = (xain_videoram_size/2) - 1;offs >= 0;offs--)
	{
                int numtile = xain_videoram[offs] +
                              ((xain_videoram[offs+0x400]&3)<<8);
                int color = ((xain_videoram[offs+0x400]&0xe0)>>5);

		if (numtile != 0x0) 	/* don't draw 0 */
		{
			int sx,sy;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(bitmap,Machine->gfx[0],
					numtile,
					color, // Color?
					0,0,// FlipX, FlipY
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}

        free(r);
}
