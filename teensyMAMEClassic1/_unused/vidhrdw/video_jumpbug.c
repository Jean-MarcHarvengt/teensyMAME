/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define MAX_STARS 250
#define STARS_COLOR_BASE 32

unsigned char *jumpbug_attributesram;
unsigned char *jumpbug_gfxbank;
unsigned char *jumpbug_stars;

struct star
{
	int x,y,code,col;
};
static struct star stars[MAX_STARS];
static int total_stars;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  jumpbug has one 32 bytes palette PROM, connected to the RGB output this
  way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  The output of the background star generator is connected this way:

  bit 5 -- 100 ohm resistor  -- BLUE
        -- 150 ohm resistor  -- BLUE
        -- 100 ohm resistor  -- GREEN
        -- 150 ohm resistor  -- GREEN
        -- 100 ohm resistor  -- RED
  bit 0 -- 150 ohm resistor  -- RED

***************************************************************************/
void jumpbug_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
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


	for (i = 0;i < 4 * 8;i++)
	{
		if (i & 3) colortable[i] = i;
		else colortable[i] = 0;
	}


	/* now the stars */
	for (i = 32;i < 32 + 64;i++)
	{
		int bits;
		int map[4] = { 0x00, 0x88, 0xcc, 0xff };

		bits = ((i-32) >> 0) & 0x03;
		palette[3*i] = map[bits];
		bits = ((i-32) >> 2) & 0x03;
		palette[3*i + 1] = map[bits];
		bits = ((i-32) >> 4) & 0x03;
		palette[3*i + 2] = map[bits];
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int jumpbug_vh_start(void)
{
	int generator;
	int x,y;


	if (generic_vh_start() != 0)
		return 1;


	/* precalculate the star background */
	total_stars = 0;
	generator = 0;

	for (x = 255;x >= 0;x--)
	{
		for (y = 511;y >= 0;y--)
		{
			int bit1,bit2;


			generator <<= 1;
			bit1 = (~generator >> 17) & 1;
			bit2 = (generator >> 5) & 1;

			if (bit1 ^ bit2) generator |= 1;

			if (x >= Machine->drv->visible_area.min_x &&
					x <= Machine->drv->visible_area.max_x &&
					((~generator >> 16) & 1) &&
					(generator & 0xff) == 0xff)
			{
				int color;

				color = (~(generator >> 8)) & 0x3f;
				if (color && total_stars < MAX_STARS)
				{
					stars[total_stars].x = x;
					stars[total_stars].y = y;
					stars[total_stars].code = color;
					stars[total_stars].col = Machine->pens[color + STARS_COLOR_BASE];

					total_stars++;
				}
			}
		}
	}

	return 0;
}



void jumpbug_attributes_w(int offset,int data)
{
	if ((offset & 1) && jumpbug_attributesram[offset] != data)
	{
		int i;


		for (i = offset / 2;i < videoram_size;i += 32)
			dirtybuffer[i] = 1;
	}

	jumpbug_attributesram[offset] = data;
}



void jumpbug_gfxbank_w(int offset,int data)
{
	if (jumpbug_gfxbank[offset] != data)
	{
		jumpbug_gfxbank[offset] = data;
		memset(dirtybuffer,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void jumpbug_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,charcode;


			dirtybuffer[offs] = 0;

			sx = (31 - offs / 32);
			sy = (offs % 32);

			charcode = videoram[offs];
			if ((charcode & 0xc0) == 0x80 && (jumpbug_gfxbank[2] & 1) != 0)
			{
				charcode += 128 + 64 * (jumpbug_gfxbank[0] & 1)
						+ 128 * (jumpbug_gfxbank[1] & 1) + 256 * (~jumpbug_gfxbank[4] & 1);
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode,
					jumpbug_attributesram[2 * sy + 1],
					0,0,8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];


		for (offs = 0;offs < 32;offs++)
			scroll[offs] = jumpbug_attributesram[2 * offs];

		copyscrollbitmap(bitmap,tmpbitmap,32,scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int spritecode;


		spritecode = spriteram[offs + 1] & 0x3f;
		if ((spritecode & 0x30) == 0x20 && (jumpbug_gfxbank[2] & 1) != 0)
		{
			spritecode += 32 + 16 * (jumpbug_gfxbank[0] & 1)
					+ 32 * (jumpbug_gfxbank[1] & 1) + 64 * (~jumpbug_gfxbank[4] & 1);
		}

		drawgfx(bitmap,Machine->gfx[1],
				spritecode,
				spriteram[offs + 2],
				spriteram[offs + 1] & 0x80,spriteram[offs + 1] & 0x40,
				spriteram[offs],spriteram[offs + 3],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* draw the stars */
	if (*jumpbug_stars & 1)
	{
		int bpen;
		static int stars_blink,blink_count;


		blink_count++;
		if (blink_count >= 43)
		{
			blink_count = 0;
			stars_blink = (stars_blink + 1) % 4;
		}

		bpen = Machine->pens[0];
		for (offs = 0;offs < total_stars;offs++)
		{
			int x,y;


			x = stars[offs].x;
			y = stars[offs].y / 2;

			if (((x & 1) ^ ((y >> 4) & 1)) &&
					(bitmap->line[y][x] == bpen))
			{
				switch (stars_blink)
				{
					case 0:
						if (stars[offs].code & 1) bitmap->line[y][x] = stars[offs].col;
						break;
					case 1:
						if (stars[offs].code & 4) bitmap->line[y][x] = stars[offs].col;
						break;
					case 2:
						if (x & 2) bitmap->line[y][x] = stars[offs].col;
						break;
					case 3:
						bitmap->line[y][x] = stars[offs].col;
						break;
				}
			}
		}
	}
}
