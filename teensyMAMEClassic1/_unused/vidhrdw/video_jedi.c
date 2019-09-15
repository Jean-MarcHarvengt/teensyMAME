/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#define JEDI_PLAYFIELD

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *jedi_backgroundram;
int jedi_backgroundram_size;
unsigned char *jedi_PIXIRAM;
static unsigned int jedi_vscroll;
static unsigned int jedi_hscroll;
static unsigned int jedi_alpha_bank;
static unsigned char *dirtybuffer2;
static struct osd_bitmap *tmpbitmap2;

void jedi_vscroll_w(int offset,int data) {
    jedi_vscroll = data | (offset << 8);
}

void jedi_hscroll_w(int offset,int data) {
    jedi_hscroll = data | (offset << 8);
}

void jedi_alpha_banksel (int offset, int data) {

    if (data)
        jedi_alpha_bank = 256;
    else
        jedi_alpha_bank = 0;
}



/* Color RAM format
   Color RAM is 1024x12
   RAM address: A0..A3 = Playfield color code
                A4..A7 = Motion object color code
                A8..A9 = Alphanumeric color code
   RAM data:
                0..2 = Blue
                3..5 = Green
                6..8 = Blue
                9..11 = Intesnsity
    Output resistor values:
               bit 0 = 22K
               bit 1 = 10K
               bit 2 = 4.7K
*/


void jedi_paletteram_w(int offset,int data)
{
    int r,g,b;
	int bits,intensity;
    unsigned int color;


	paletteram[offset] = data;
	color = paletteram[offset & 0x3FF] | (paletteram[offset | 0x400] << 8);
	intensity = (color >> 9) & 0x07;
	bits = (color >> 6) & 0x07;
	r = 5 * bits * intensity;
	bits = (color >> 3) & 0x07;
	g = 5 * bits * intensity;
	bits = (color >> 0) & 0x07;
	b = 5 * bits * intensity;

	offset &= 0x3ff;
	if ((offset & 0x3f0) == 0)
		palette_change_color (4 + (offset & 0x00f), r, g, b);
	else if ((offset & 0x30f) == 0)
		palette_change_color (4+16 + ((offset >> 4) & 0x0f), r, g, b);
	else if ((offset & 0x0ff) == 0)
		palette_change_color (0 + ((offset >> 8) & 0x03), r, g, b);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int jedi_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(jedi_backgroundram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,1,jedi_backgroundram_size);

	/* the background area is 512x512 */
	if ((tmpbitmap2 = osd_create_bitmap(512,512)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void jedi_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	free(dirtybuffer2);
	generic_vh_stop();
}



void jedi_backgroundram_w(int offset,int data)
{
	if (jedi_backgroundram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		jedi_backgroundram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void jedi_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	for (offs = jedi_backgroundram_size/2 - 1;offs >= 0;offs--)
	{
		if (dirtybuffer2[offs] != 0 || dirtybuffer2[offs + 0x400] != 0)
		{
			int sx,sy,b,c;


			sx = offs % 32;
			sy = offs / 32;
			c = (jedi_backgroundram[offs] & 0xFF);
			b = (jedi_backgroundram[offs + 0x400] & 0x0F);
			c = c | ((b & 0x01) << 8);
			c = c | ((b & 0x08) << 6);
			c = c | ((b & 0x02) << 9);

			dirtybuffer2[offs] = dirtybuffer2[offs + 0x400] = 0;

			drawgfx(tmpbitmap2,Machine->gfx[1],
					c,0,
					b & 0x04,0,
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the background graphics */
	{
		int scrollx,scrolly;


		scrollx = -jedi_hscroll;
		scrolly = -jedi_vscroll;

		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


    /* Character display */
    for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;


		sx = offs % 64;
		sy = offs / 64;

		drawgfx(bitmap,Machine->gfx[0],
				videoram[offs] + jedi_alpha_bank,
				0,
				0,0,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
    }


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
    for (offs = 0;offs < 0x30;offs++)
	{
		int b,c;


		b = ((spriteram[offs+0x40] & 0x02) >> 1);
		b = b | ((spriteram[offs+0x40] & 0x40) >> 5);
		b = b | (spriteram[offs+0x40] & 0x04);

		c = spriteram[offs] + (b * 256);
		if (spriteram[offs+0x40] & 0x08) c |= 1;	/* double height */

		b = spriteram[offs+0x40] & 0x01;

		drawgfx(bitmap,Machine->gfx[2],
				c,
				0,
				(spriteram[offs+0x40] & 0x10),(spriteram[offs+0x40] & 0x20),
				(spriteram[offs+0x100]) | (b << 8),240-spriteram[offs+0x80],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

		if (spriteram[offs+0x40] & 0x08)	/* double height */
			drawgfx(bitmap,Machine->gfx[2],
					c-1,
					0,
					(spriteram[offs+0x40] & 0x10),(spriteram[offs+0x40] & 0x20),
					(spriteram[offs+0x100]) | (b << 8),240-spriteram[offs+0x80]-16,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
    }
}
