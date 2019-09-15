/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *frogger_attributesram;

/* To whomever will implement cocktail mode: sprites are offset by two pixels */
/* by the program when the screen is flipped */


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Frogger has one 32 bytes palette PROM, connected to the RGB output
  this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  Additionally, there is a bit which is 1 in the upper half of the display
  (136 lines? I'm not sure of the exact value); it is connected to blue
  through a 470 ohm resistor. It is used to make the river blue instead of
  black.

***************************************************************************/
void frogger_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;


		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		palette[3*i] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}

	/* use an otherwise unused pen for the river background */
	palette[3*4] = 0;
	palette[3*4 + 1] = 0;
	palette[3*4 + 2] = 0x47;

	/* normal */
	for (i = 0;i < 4 * 8;i++)
	{
		if (i & 3) colortable[i] = i;
		else colortable[i] = 0;
	}
	/* blue background (river) */
	for (i = 4 * 8;i < 4 * 16;i++)
	{
		if (i & 3) colortable[i] = i - 4*8;
		else colortable[i] = 4;
	}
}



void frogger_attributes_w(int offset,int data)
{
	if ((offset & 1) && frogger_attributesram[offset] != data)
	{
		int i;


		for (i = offset / 2;i < videoram_size;i += 32)
			dirtybuffer[i] = 1;
	}

	frogger_attributesram[offset] = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void frogger_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,col;


			dirtybuffer[offs] = 0;

			sx = (31 - offs / 32);
			sy = (offs % 32);
			col = frogger_attributesram[2 * sy + 1] & 7;
			col = ((col >> 1) & 0x03) | ((col << 2) & 0x04);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					col + (sy <= 15 ? 8 : 0),	/* blue background in the upper 128 lines */
					0,0,8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32],s;


		for (i = 0;i < 32;i++)
		{
			s = frogger_attributesram[2 * i];
			scroll[i] = ((s << 4) & 0xf0) | ((s >> 4) & 0x0f);
		}

		copyscrollbitmap(bitmap,tmpbitmap,32,scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		if (spriteram[offs + 3] != 0)
		{
			int x,col;


			x = spriteram[offs];
			x = ((x << 4) & 0xf0) | ((x >> 4) & 0x0f);
			col = spriteram[offs + 2] & 7;
			col = ((col >> 1) & 0x03) | ((col << 2) & 0x04);

			drawgfx(bitmap,Machine->gfx[1],
					spriteram[offs + 1] & 0x3f,
					col,
					spriteram[offs + 1] & 0x80,spriteram[offs + 1] & 0x40,
					x,spriteram[offs + 3],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}


/* the alternate version doesn't have the sprite & scroll registers mangling, */
/* but it still has the color code mangling. */
void frogger2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,col;


			dirtybuffer[offs] = 0;

			sx = (31 - offs / 32);
			sy = (offs % 32);
			col = frogger_attributesram[2 * sy + 1] & 7;
			col = ((col >> 1) & 0x03) | ((col << 2) & 0x04);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					col + (sy <= 15 ? 8 : 0),	/* blue background in the upper 128 lines */
					0,0,8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];


		for (i = 0;i < 32;i++)
			scroll[i] = frogger_attributesram[2 * i];

		copyscrollbitmap(bitmap,tmpbitmap,32,scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		if (spriteram[offs + 3] != 0)
		{
			int col;


			col = spriteram[offs + 2] & 7;
			col = ((col >> 1) & 0x03) | ((col << 2) & 0x04);

			drawgfx(bitmap,Machine->gfx[1],
					spriteram[offs + 1] & 0x3f,
					col,
					spriteram[offs + 1] & 0x80,spriteram[offs + 1] & 0x40,
					spriteram[offs],spriteram[offs + 3],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
