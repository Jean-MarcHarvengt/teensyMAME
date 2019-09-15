/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



static int flipscreen;



static struct rectangle spritevisiblearea =
{
	4*8, 31*8-1,
	2*8, 30*8-1
};
static struct rectangle spritevisibleareaflip =
{
	1*8, 28*8-1,
	2*8, 30*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Time Pilot has two 32x8 palette PROMs and two 256x4 lookup table PROMs
  (one for characters, one for sprites).
  The palette PROMs are connected to the RGB output this way:

  bit 7 -- 390 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 560 ohm resistor  -- BLUE
        -- 820 ohm resistor  -- BLUE
        -- 1.2kohm resistor  -- BLUE
        -- 390 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
  bit 0 -- 560 ohm resistor  -- GREEN

  bit 7 -- 820 ohm resistor  -- GREEN
        -- 1.2kohm resistor  -- GREEN
        -- 390 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 560 ohm resistor  -- RED
        -- 820 ohm resistor  -- RED
        -- 1.2kohm resistor  -- RED
  bit 0 -- not connected

***************************************************************************/
void timeplt_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3,bit4;


		bit0 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 4) & 0x01;
		bit4 = (color_prom[Machine->drv->total_colors] >> 5) & 0x01;
		*(palette++) = 0x19 * bit0 + 0x24 * bit1 + 0x35 * bit2 + 0x40 * bit3 + 0x4d * bit4;
		bit0 = (color_prom[Machine->drv->total_colors] >> 6) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 7) & 0x01;
		bit2 = (color_prom[0] >> 0) & 0x01;
		bit3 = (color_prom[0] >> 1) & 0x01;
		bit4 = (color_prom[0] >> 2) & 0x01;
		*(palette++) = 0x19 * bit0 + 0x24 * bit1 + 0x35 * bit2 + 0x40 * bit3 + 0x4d * bit4;
		bit0 = (color_prom[0] >> 3) & 0x01;
		bit1 = (color_prom[0] >> 4) & 0x01;
		bit2 = (color_prom[0] >> 5) & 0x01;
		bit3 = (color_prom[0] >> 6) & 0x01;
		bit4 = (color_prom[0] >> 7) & 0x01;
		*(palette++) = 0x19 * bit0 + 0x24 * bit1 + 0x35 * bit2 + 0x40 * bit3 + 0x4d * bit4;

		color_prom++;
	}

	color_prom += Machine->drv->total_colors;
	/* color_prom now points to the beginning of the lookup table */


	/* sprites */
	for (i = 0;i < TOTAL_COLORS(1);i++)
		COLOR(1,i) = *(color_prom++) & 0x0f;

	/* characters */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = (*(color_prom++) & 0x0f) + 0x10;
}



void timeplt_flipscreen_w(int offset,int data)
{
	if (flipscreen != (data & 1))
	{
		flipscreen = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}



/* Return the current video scan line */
int timeplt_scanline_r(int offset)
{
	return cpu_scalebyfcount(256);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void timeplt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,flipx,flipy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;
			flipx = colorram[offs] & 0x40;
			flipy = colorram[offs] & 0x80;
			if (flipscreen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + 8 * (colorram[offs] & 0x20),
					colorram[offs] & 0x1f,
					flipx,flipy,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* In Time Pilot, the characters can appear either behind or in front of the */
	/* sprites. The priority is selected by bit 4 of the color attribute of the */
	/* character. This feature is used to limit the sprite visibility area, and */
	/* as a sort of copyright notice protection ("KONAMI" on the title screen */
	/* alternates between characters and sprites, but they are both white so you */
	/* can't see it). To speed up video refresh, we do the sprite clipping ourselves. */

	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = spriteram_size - 2;offs >= 0;offs -= 2)
	{
		drawgfx(bitmap,Machine->gfx[1],
				spriteram[offs + 1],
				spriteram_2[offs] & 0x3f,
				spriteram_2[offs] & 0x40,!(spriteram_2[offs] & 0x80),
				240-spriteram[offs],spriteram_2[offs + 1]-1,
				flipscreen ? &spritevisibleareaflip : &spritevisiblearea,TRANSPARENCY_PEN,0);

		if (spriteram_2[offs + 1] < 240)
		{
			/* clouds are drawn twice, offset by 128 pixels horizontally and vertically */
			/* this is done by the program, multiplexing the sprites; we don't emulate */
			/* that, we just reproduce the behaviour. */
			if (offs <= 2*2 || offs >= 19*2)
			{
				drawgfx(bitmap,Machine->gfx[1],
						spriteram[offs + 1],
						spriteram_2[offs] & 0x3f,
						spriteram_2[offs] & 0x40,~spriteram_2[offs] & 0x80,
						(240-spriteram[offs]+128) & 0xff,(spriteram_2[offs + 1]-1+128) & 0xff,
						flipscreen ? &spritevisibleareaflip : &spritevisiblearea,TRANSPARENCY_PEN,0);
			}
		}
	}
}
