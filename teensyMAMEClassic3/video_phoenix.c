/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *phoenix_videoram2;
static unsigned char bg_scroll;
static int palette_bank;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Phoenix has two 256x4 palette PROMs, one containing the high bits and the
  other the low bits (2x2x2 color space).
  The palette PROMs are connected to the RGB output this way:

  bit 3 --
        -- 270 ohm resistor  -- GREEN
        -- 270 ohm resistor  -- BLUE
  bit 0 -- 270 ohm resistor  -- RED

  bit 3 --
        -- GREEN
        -- BLUE
  bit 0 -- RED

  plus 270 ohm pullup and pulldown resistors on all lines

***************************************************************************/
void phoenix_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		*(palette++) = 0x55 * bit0 + 0xaa * bit1;
		bit0 = (color_prom[0] >> 2) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		*(palette++) = 0x55 * bit0 + 0xaa * bit1;
		bit0 = (color_prom[0] >> 1) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		*(palette++) = 0x55 * bit0 + 0xaa * bit1;

		color_prom++;
	}

	/* first bank of characters use colors 0-31 and 64-95 */
	for (i = 0;i < 8;i++)
	{
		int j;


		for (j = 0;j < 2;j++)
		{
			COLOR(0,4*i + j*4*8) = i + j*64;
			COLOR(0,4*i + j*4*8 + 1) = 8 + i + j*64;
			COLOR(0,4*i + j*4*8 + 2) = 2*8 + i + j*64;
			COLOR(0,4*i + j*4*8 + 3) = 3*8 + i + j*64;
		}
	}

	/* second bank of characters use colors 32-63 and 96-127 */
	for (i = 0;i < 8;i++)
	{
		int j;


		for (j = 0;j < 2;j++)
		{
			COLOR(1,4*i + j*4*8) = i + 32 + j*64;
			COLOR(1,4*i + j*4*8 + 1) = 8 + i + 32 + j*64;
			COLOR(1,4*i + j*4*8 + 2) = 2*8 + i + 32 + j*64;
			COLOR(1,4*i + j*4*8 + 3) = 3*8 + i + 32 + j*64;
		}
	}
}



void phoenix_videoreg_w (int offset,int data)
{
	if (palette_bank != ((data >> 1) & 1))
	{
		palette_bank = (data >> 1) & 1;

		memset(dirtybuffer,1,videoram_size);
	}
}

void phoenix_scroll_w (int offset,int data)
{
	bg_scroll = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void phoenix_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
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

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					(videoram[offs] >> 5) + 8 * palette_bank,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	{
		int scroll;


		scroll = -bg_scroll;

		copyscrollbitmap(bitmap,tmpbitmap,1,&scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;


		sx = offs % 32;
		sy = offs / 32;

		if (sx >= 1)
		{
			drawgfx(bitmap,Machine->gfx[1],
					phoenix_videoram2[offs],
					(phoenix_videoram2[offs] >> 5) + 8 * palette_bank,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
		else
			drawgfx(bitmap,Machine->gfx[1],
					phoenix_videoram2[offs],
					(phoenix_videoram2[offs] >> 5) + 8 * palette_bank,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}
}
