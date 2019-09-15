/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char spacefb_vref=0;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Space FB has one 32 bytes palette PROM, connected to the RGB output this
  way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
void spacefb_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;

		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		palette[3*i + 0] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		if (bit1 | bit2)
			bit0 = 1;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}

	for (i = 0;i < 4 * 8;i++)
	{
		if (i & 3) colortable[i] = i;
		else colortable[i] = 0;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void spacefb_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int spriteno;



	/* Clear the bitmap */
	fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);

	/* Draw the sprite/chars */
	for (offs = 0,spriteno = spacefb_vref;offs < 128;offs++,spriteno++)
	{
		unsigned char h,v,chr,cnt;
		h = videoram[spriteno];
		v = videoram[spriteno+0x100];

		h = ((255-h)*256)/260;
		v = 255-v;

		chr = videoram[spriteno+0x200];
		cnt = videoram[spriteno+0x300];

		if (cnt) {
			if (cnt & 0x20) {
			/* Draw bullets */
				unsigned char charnum = chr & 63;
				unsigned char pal = 7-(cnt & 0x7);

				drawgfx(bitmap,Machine->gfx[1],
						charnum,
						pal,
						0,0,v,h,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

			} else if (cnt & 0x40) {
				unsigned char charnum = 255-chr;
				unsigned char pal = 7-(cnt & 0x7);

				drawgfx(bitmap,Machine->gfx[0],
						charnum,
						pal,
						0,0,v,h,
						&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
			}
		}
	}

}
