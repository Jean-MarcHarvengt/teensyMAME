/***************************************************************************

   SPEED RUMBLER

   vidhrdw.c

   Functions to emulate the video hardware of the machine.

   Todo:
   Priority tiles (should be half transparent).

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *srumbler_backgroundram;
unsigned char *srumbler_scrolly;
unsigned char *srumbler_scrollx;

int srumbler_backgroundram_size;

static unsigned char *bkgnd_dirty;
static struct osd_bitmap *tmpbitmap2;



/* Unknown video control register */
static int srumbler_video_control;

/*
Currently not used. The end of level score-board should either be
transparent or the scroll tiles need to be turned off.
*/
static int chon=1,objon=1,scrollon=1;



/***************************************************************************

   Start the video hardware emulation.

***************************************************************************/

int srumbler_vh_start(void)
{
	int i;


	if (generic_vh_start() != 0)
	return 1;

	if ((bkgnd_dirty = malloc(srumbler_backgroundram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(bkgnd_dirty,1,srumbler_backgroundram_size);


	if ((tmpbitmap2 = osd_new_bitmap(0x40*16, 0x40*16, Machine->scrbitmap->depth )) == 0)
	{
		free(bkgnd_dirty);
		generic_vh_stop();
		return 1;
	}

#define COLORTABLE_START(gfxn,color_code) Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + \
				color_code * Machine->gfx[gfxn]->color_granularity
#define GFX_COLOR_CODES(gfxn) Machine->gfx[gfxn]->total_colors
#define GFX_ELEM_COLORS(gfxn) Machine->gfx[gfxn]->color_granularity

	memset(palette_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors * sizeof(unsigned char));
	/* chars */
	for (i = 0;i < GFX_COLOR_CODES(0);i++)
	{
		memset(&palette_used_colors[COLORTABLE_START(0,i)],
				PALETTE_COLOR_USED,
				GFX_ELEM_COLORS(0));
	}
	/* bg tiles */
	for (i = 0;i < GFX_COLOR_CODES(1);i++)
	{
		memset(&palette_used_colors[COLORTABLE_START(1,i)],
				PALETTE_COLOR_USED,
				GFX_ELEM_COLORS(1));
	}
	/* sprites */
	for (i = 0;i < GFX_COLOR_CODES(2);i++)
	{
		memset(&palette_used_colors[COLORTABLE_START(2,i)],
				PALETTE_COLOR_USED,
				GFX_ELEM_COLORS(2));
	}

	return 0;
}



/***************************************************************************

   Stop the video hardware emulation.

***************************************************************************/
void srumbler_vh_stop(void)
{
osd_free_bitmap(tmpbitmap2);
	 free(bkgnd_dirty);
generic_vh_stop();
}

void srumbler_4009_w(int offset, int data)
{
	 /* Unknown video control register */
	 srumbler_video_control=data;
}

void srumbler_background_w(int offset,int data)
{
	 if (srumbler_backgroundram[offset] != data)
{
		 bkgnd_dirty[offset] = 1;
		 srumbler_backgroundram[offset] = data;
}
}


/***************************************************************************

   Draw the game screen in the given osd_bitmap.
   Do NOT call osd_update_display() from this function, it will be called by
   the main emulation engine.

***************************************************************************/

void srumbler_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int scrollx, scrolly;
	int offs;
	int sx, sy, x, y;


	if (palette_recalc())
		memset(bkgnd_dirty,1,srumbler_backgroundram_size);


	 scrollx = srumbler_scrollx[0]+256*srumbler_scrollx[1];
	 scrollx &= 0x03ff;
	 scrolly = (srumbler_scrolly[0]+256*srumbler_scrolly[1]);

	 if (scrollon)
	 {
	    offs=0;
	    for (sx=0; sx<0x40; sx++)
	    {
		 for (sy=0; sy<0x40; sy++)
		 {
			 /* TILES
			    =====
			    Attribute
			    0x80 Colour
			    0x40 Colour
			    0x20 Colour
			    0x10 Tile priority
			    0x08 Y flip
			    0x04 Code
			    0x02 Code
			    0x01 Code
			 */

			 if (bkgnd_dirty[offs]||bkgnd_dirty[offs+1])
			 {
				 int attr=srumbler_backgroundram[offs];
				 int code=attr&0x07;
				 code <<= 8;
				 code+=srumbler_backgroundram[offs+1];

				 bkgnd_dirty[offs]=bkgnd_dirty[offs+1]=0;

				 drawgfx(tmpbitmap2,Machine->gfx[1],
						 code,
						 (attr>>5),
						 0,
						 attr&0x08,
						 sx*16, sy*16,
						 0, TRANSPARENCY_NONE,0);
			}
			offs+=2;
		  }
	     }


	     /* copy the background graphics */
	     {
		int scrlx, scrly;
		scrlx =-scrollx;
		scrly=-scrolly;
		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrlx,1,&scrly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	     }
	 }
	 else  fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);


	 if (objon)
	 {
		 /* Draw the sprites. */
		 for (offs = spriteram_size-4; offs>=0;offs -= 4)
		 {
			 /* SPRITES
			    =====
			    Attribute
			    0x80 Code MSB
			    0x40 Code MSB
			    0x20 Code MSB
			    0x10 Colour
			    0x08 Colour
			    0x04 Colour
			    0x02 y Flip
			    0x01 X MSB
			 */


			 int code,colour;
			 int attr=spriteram[offs+1];
			 code = spriteram[offs];
			 code += ( (attr&0xe0) << 3 );
			 colour = (attr & 0x1c)>>2;
			 sy = spriteram[offs + 2];
			 sx = spriteram[offs + 3] + 0x100 * ( attr & 0x01);

			 drawgfx(bitmap,Machine->gfx[2],
					 code,
					 colour,
					 0,
					 attr&0x02,
					 sx, sy,
					 &Machine->drv->visible_area,TRANSPARENCY_PEN,15);
		  }

		 /* now draw high priority sprite parts */
		 if( 0 )for (offs = spriteram_size-4; offs>=0;offs -= 4)
		 {
			 /* SPRITES
			    =====
			    Attribute
			    0x80 Code MSB
			    0x40 Code MSB
			    0x20 Code MSB
			    0x10 Colour
			    0x08 Colour
			    0x04 Colour
			    0x02 y Flip
			    0x01 X MSB
			 */


			 int code,colour;
			 int attr=spriteram[offs+1];
			 code = spriteram[offs];
			 code += ( (attr&0xe0) << 3 );
			 colour = (attr & 0x1c)>>2;
			 sy = spriteram[offs + 2];
			 sx = spriteram[offs + 3] + 0x100 * ( attr & 0x01);

			 drawgfx(bitmap,Machine->gfx[2],
					 code,
					 colour,
					 0,
					 attr&0x02,
					 sx, sy,
					 &Machine->drv->visible_area,TRANSPARENCY_PENS,(1<<15)|0xFF);
		  }
	 }

/* redraw the background tiles which have priority over sprites */
	 if (scrollon)
	 {
		 x=-(scrollx & 0x0f);

		 for (sx=0; sx<0x18; sx++)
		 {
		       offs=(scrollx >>4)*0x80;
		       offs+=(scrolly>>4)*2;
		       offs+=sx*0x80;
		       offs&=0x1fff;
		       y=-(scrolly & 0x0f);
		       for (sy=0; sy<0x11; sy++)
		       {
			      int attr=srumbler_backgroundram[offs];
			      if (attr & 0x10)
			      {
				     int code=attr&0x07;
				     code <<= 8;
				     code+=srumbler_backgroundram[offs+1];
				     drawgfx(bitmap,Machine->gfx[1],
						  code,
						  (attr>>5),
						  0,
						  attr&0x08,
						  x, y,
						  &Machine->drv->visible_area,

						  TRANSPARENCY_PENS,0xFF );
			     }
			     y+=16;
			     offs+=2;
		       }
		       x+=16;
		 }
	 }

	if (chon)
	{
		/* draw the frontmost playfield. They are characters, but draw them as sprites */
		for (offs = videoram_size - 2;offs >= 0;offs -= 2)
		{
			sx = (offs/2) / 32;
			sy = (offs/2) % 32;

			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs + 1] + ((videoram[offs] & 0x03) << 8),
					(videoram[offs] & 0x3c) >> 2,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,
					(videoram[offs] & 0x40) ? TRANSPARENCY_NONE : TRANSPARENCY_PEN,3);
		}
	}
}
