/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *dday_videoram2;
unsigned char *dday_videoram3;
static int control = 0;

void dday_sound_enable(int enabled);

/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/
void dday_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = color_prom[0*Machine->drv->total_colors];
		*(palette++) = color_prom[1*Machine->drv->total_colors];
		*(palette++) = color_prom[2*Machine->drv->total_colors];

		color_prom++;
	}
}


void dday_colorram_w(int offset, int data)
{
    colorram[offset & 0x3e0] = data;
}

int dday_colorram_r(int offset)
{
    return colorram[offset & 0x3e0];
}

void dday_control_w(int offset, int data)
{
	//fprintf(errorlog,"Control = %02X\n", data);

	/* Bit 0 is coin counter 1 */
	coin_counter_w(0, data & 0x01);

	/* Bit 1 is coin counter 2 */
	coin_counter_w(1, data & 0x02);

	/* Bit 4 is sound enable */
	dday_sound_enable(data & 0x10);

	control = data;
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void dday_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

	/* for every character in the backround RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;

			dirtybuffer[offs] = 0;

			sy = 8 * (offs / 32);
			sx = 8 * (31 - offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[1],
					videoram[offs],
					0,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Copy foreground planes. They are characters, but draw them as sprites
	   Don't know about priorities. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx, sy, attrib, flipx;

		sy = offs / 32;
		sx = 31 - offs % 32;

		drawgfx(bitmap,Machine->gfx[0],
			dday_videoram2[offs],
			0,
			0,0,
			8*sx,8*sy,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

		/* Only know what Bit 0 is. (And also Bit 2, but I don't think */
		/* the hardware uses that) */
		attrib = colorram[offs & 0x3e0];
		flipx  = attrib & 0x01;

		if (flipx) sx = 31 - sx;

		drawgfx(bitmap,Machine->gfx[2],
			dday_videoram3[offs],
			0,
			flipx,0,
			8*sx,8*sy,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
