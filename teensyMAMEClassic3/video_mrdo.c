/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *mrdo_videoram2;
unsigned char *mrdo_colorram2;
unsigned char *mrdo_scroll_y;
static struct osd_bitmap *tmpbitmap1,*tmpbitmap2;
static int flipscreen;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Mr. Do! has two 32 bytes palette PROM and a 32 bytes sprite color lookup
  table PROM.
  The palette PROMs are connected to the RGB output this way:

  U2:
  bit 7 -- unused
        -- unused
        -- 100 ohm resistor  -- BLUE
        --  75 ohm resistor  -- BLUE
        -- 100 ohm resistor  -- GREEN
        --  75 ohm resistor  -- GREEN
        -- 100 ohm resistor  -- RED
  bit 0 --  75 ohm resistor  -- RED

  T2:
  bit 7 -- unused
        -- unused
        -- 150 ohm resistor  -- BLUE
        -- 120 ohm resistor  -- BLUE
        -- 150 ohm resistor  -- GREEN
        -- 120 ohm resistor  -- GREEN
        -- 150 ohm resistor  -- RED
  bit 0 -- 120 ohm resistor  -- RED

***************************************************************************/
void mrdo_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i,j;


	for (i = 0;i < 64;i++)
	{
		for (j = 0;j < 4;j++)
		{
			int bit0,bit1,bit2,bit3;


			bit0 = (color_prom[4 * (i / 8) + j + 32] >> 1) & 0x01;
			bit1 = (color_prom[4 * (i / 8) + j + 32] >> 0) & 0x01;
			bit2 = (color_prom[4 * (i % 8) + j] >> 1) & 0x01;
			bit3 = (color_prom[4 * (i % 8) + j] >> 0) & 0x01;
			*(palette++) = 0x2c * bit0 + 0x37 * bit1 + 0x43 * bit2 + 0x59 * bit3;
			bit0 = (color_prom[4 * (i / 8) + j + 32] >> 3) & 0x01;
			bit1 = (color_prom[4 * (i / 8) + j + 32] >> 2) & 0x01;
			bit2 = (color_prom[4 * (i % 8) + j] >> 3) & 0x01;
			bit3 = (color_prom[4 * (i % 8) + j] >> 2) & 0x01;
			*(palette++) = 0x2c * bit0 + 0x37 * bit1 + 0x43 * bit2 + 0x59 * bit3;
			bit0 = (color_prom[4 * (i / 8) + j + 32] >> 5) & 0x01;
			bit1 = (color_prom[4 * (i / 8) + j + 32] >> 4) & 0x01;
			bit2 = (color_prom[4 * (i % 8) + j] >> 5) & 0x01;
			bit3 = (color_prom[4 * (i % 8) + j] >> 4) & 0x01;
			*(palette++) = 0x2c * bit0 + 0x37 * bit1 + 0x43 * bit2 + 0x59 * bit3;
		}
	}

	/* reserve the last color for the transparent pen (none of the game colors can have */
	/* these RGB components) */
	*(palette++) = 1;
	*(palette++) = 1;
	*(palette++) = 1;


	/* characters with pen 0 = black */
	for (i = 0;i < 4 * 64;i++)
	{
		if (i % 4 == 0) colortable[i] = 256;	/* transparent */
		else colortable[i] = i;
	}
	/* characters with colored pen 0 */
	for (i = 0;i < 4 * 64;i++)
		colortable[i + 4 * 64] = i;

	/* sprites */
	for (i = 0;i < 4 * 8;i++)
	{
		int bits1,bits2;


		/* low 4 bits are for sprite n */
		bits1 = (color_prom[i + 64] >> 0) & 3;
		bits2 = (color_prom[i + 64] >> 2) & 3;
		colortable[i + 4 * 128] = bits1 + (bits2 << 2) + (bits2 << 5);

		/* high 4 bits are for sprite n + 8 */
		bits1 = (color_prom[i + 64] >> 4) & 3;
		bits2 = (color_prom[i + 64] >> 6) & 3;
		colortable[i + 4 * 136] = bits1 + (bits2 << 2) + (bits2 << 5);
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int mrdo_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((tmpbitmap1 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(tmpbitmap1);
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void mrdo_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	osd_free_bitmap(tmpbitmap1);
	generic_vh_stop();
}



void mrdo_videoram2_w(int offset,int data)
{
	if (mrdo_videoram2[offset] != data)
	{
		dirtybuffer[offset] = 1;

		mrdo_videoram2[offset] = data;
	}
}



void mrdo_colorram2_w(int offset,int data)
{
	if (mrdo_colorram2[offset] != data)
	{
		dirtybuffer[offset] = 1;

		mrdo_colorram2[offset] = data;
	}
}



void mrdo_flipscreen_w(int offset,int data)
{
	/* bits 1-3 control the playfield priority, but they are not used by */
	/* Mr. Do! so we don't emulate them */

	if (flipscreen != (data & 0x01))
	{
		flipscreen = data & 0x01;
		memset(dirtybuffer,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void mrdo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;
			if (flipscreen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
			}


			/* Mr. Do! has two playfields, one of which can be scrolled. However, */
			/* during gameplay this feature is not used, so I keep the composition */
			/* of the two playfields in a third temporary bitmap, to speed up rendering. */
			drawgfx(tmpbitmap1,Machine->gfx[1],
					videoram[offs] + 2 * (colorram[offs] & 0x80),
					colorram[offs] & 0x7f,
					flipscreen,flipscreen,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
			drawgfx(tmpbitmap2,Machine->gfx[0],
					mrdo_videoram2[offs] + 2 * (mrdo_colorram2[offs] & 0x80),
					mrdo_colorram2[offs] & 0x7f,
					flipscreen,flipscreen,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);

			drawgfx(tmpbitmap,Machine->gfx[1],
					videoram[offs] + 2 * (colorram[offs] & 0x80),
					colorram[offs] & 0x7f,
					flipscreen,flipscreen,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
			drawgfx(tmpbitmap,Machine->gfx[0],
					mrdo_videoram2[offs] + 2 * (mrdo_colorram2[offs] & 0x80),
					mrdo_colorram2[offs] & 0x7f,
					flipscreen,flipscreen,
					8*sx,8*sy,
					&Machine->drv->visible_area,(mrdo_colorram2[offs] & 0x40) ? TRANSPARENCY_NONE : TRANSPARENCY_PEN,0);
		}
	}


	/* copy the character mapped graphics */
	if (*mrdo_scroll_y)
	{
		int scroll;


		scroll = -*mrdo_scroll_y;

		copyscrollbitmap(bitmap,tmpbitmap1,0,0,1,&scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

		copybitmap(bitmap,tmpbitmap2,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_COLOR,256);
	}
	else
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		if (spriteram[offs + 1] != 0)
		{
			drawgfx(bitmap,Machine->gfx[2],
					spriteram[offs],spriteram[offs + 2] & 0x0f,
					spriteram[offs + 2] & 0x10,spriteram[offs + 2] & 0x20,
					spriteram[offs + 3],256 - spriteram[offs + 1],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
