/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct osd_bitmap *tmpbitmap2,*charbitmap;
unsigned char bx,by,bmap;
static unsigned char inverse_palette[256];


/***************************************************************************

  Convert the color PROMs into a more useable format.

  CLOAK & DAGGER doesn't have a color PROM. It uses RAM to dynamically
  create the palette. The resolution is 9 bit (3 bits per gun). The palette
  contains 64 entries, but it is accessed through a memory windows 128 bytes
  long: writing to the first 64 bytes sets the msb of the red component to 0,
  while writing to the last 64 bytes sets it to 1.
  The first 32 entries are used for sprites; the last 32 for the background
  bitmap.

  I don't know the exact values of the resistors between the RAM and the
  RGB output, I assumed the usual ones.
  bit 8 -- inverter -- 220 ohm resistor  -- RED
  bit 7 -- inverter -- 470 ohm resistor  -- RED
        -- inverter -- 1  kohm resistor  -- RED
        -- inverter -- 220 ohm resistor  -- GREEN
        -- inverter -- 470 ohm resistor  -- GREEN
        -- inverter -- 1  kohm resistor  -- GREEN
        -- inverter -- 220 ohm resistor  -- BLUE
        -- inverter -- 470 ohm resistor  -- BLUE
  bit 0 -- inverter -- 1  kohm resistor  -- BLUE

***************************************************************************/
void cloak_paletteram_w(int offset,int data)
{
	int r,g,b;
	int bit0,bit1,bit2;


	r = (data & 0xC0) >> 6;
	g = (data & 0x38) >> 3;
	b = (data & 0x07);
	/* a write to offset 64-127 means to set the msb of the red component */
	r += (offset & 0x40) >> 4;

	/* bits are inverted */
	r = 7-r;
	g = 7-g;
	b = 7-b;

	bit0 = (r >> 0) & 0x01;
	bit1 = (r >> 1) & 0x01;
	bit2 = (r >> 2) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	bit0 = (g >> 0) & 0x01;
	bit1 = (g >> 1) & 0x01;
	bit2 = (g >> 2) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	bit0 = (b >> 0) & 0x01;
	bit1 = (b >> 1) & 0x01;
	bit2 = (b >> 2) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	palette_change_color(offset & 0x3f,r,g,b);
}

void plotmap(int data)
{
	if (bmap)
		tmpbitmap->line[by][bx] = Machine->pens[(data & 0x07) + 16];
	else
		tmpbitmap2->line[by][bx] = Machine->pens[(data & 0x07) + 16];
}

void cloak_clearbmp_w(int offset, int data)
{
	bmap = data & 1;
	if (data & 2)	/* clear */
	{
		if (bmap)
			fillbitmap(tmpbitmap,Machine->pens[16],&Machine->drv->visible_area);
		else
			fillbitmap(tmpbitmap2,Machine->pens[16],&Machine->drv->visible_area);
	}
}

int graph_processor_r(int offset)
{
        int n;

        if (!bmap)
                n = inverse_palette[tmpbitmap->line[by][bx]] & 0x07;
	else
                n = inverse_palette[tmpbitmap2->line[by][bx]] & 0x07;

	switch(offset)
        {
		case 0x0:
			bx--;
			by++;
			break;
		case 0x1:
			by--;
			break;
		case 0x2:
			bx--;
			break;
		case 0x4:
			bx++;
			by++;
			break;
		case 0x5:
			by++;
			break;
		case 0x6:
			bx++;
			break;
	}
        return n;
}

void graph_processor_w(int offset, int data)
{
        switch (offset)
        {
		case 0x3:
			bx=data;
			break;
		case 0x7:
			by=data;
			break;
		case 0x0:
			plotmap(data);
			bx--;
			by++;
			break;
		case 0x1:
			plotmap(data);
			by--;
			break;
		case 0x2:
			plotmap(data);
			bx--;
			break;
		case 0x4:
			plotmap(data);
			bx++;
			by++;
			break;
		case 0x5:
			plotmap(data);
			by++;
			break;
		case 0x6:
			plotmap(data);
			bx++;
			break;
        }
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int cloak_vh_start(void)
{
	int i;


	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	if ((charbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		return 1;
	}

	if ((dirtybuffer = malloc(videoram_size)) == 0)
	{
		osd_free_bitmap(charbitmap);
		osd_free_bitmap(tmpbitmap);
		return 1;
	}
	memset(dirtybuffer,1,videoram_size);

	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(charbitmap);
		osd_free_bitmap(tmpbitmap);
		free(dirtybuffer);
		return 1;
	}

	bx = by = bmap = 0;

	for (i = 0;i < Machine->drv->total_colors;i++)
		inverse_palette[Machine->pens[i]] = i;

	return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void cloak_vh_stop(void)
{
        osd_free_bitmap(charbitmap);
	osd_free_bitmap(tmpbitmap2);
	osd_free_bitmap(tmpbitmap);
        free(dirtybuffer);
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void cloak_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
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
			sy = offs / 32 - 3;

			drawgfx(charbitmap,Machine->gfx[0],
					videoram[offs],0,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the temporary bitmap to the screen */
        copybitmap(bitmap,charbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	if (bmap)
                copybitmap(bitmap,tmpbitmap2,0,0,-6,-24,&Machine->drv->visible_area,TRANSPARENCY_COLOR,16);
	else
                copybitmap(bitmap,tmpbitmap,0,0,-6,-24,&Machine->drv->visible_area,TRANSPARENCY_COLOR,16);


	/* Draw the sprites */
	for (offs = spriteram_size/4-1; offs >= 0; offs--)
	{
		drawgfx(bitmap,Machine->gfx[1],
				spriteram[offs+64] & 0x7f,
				0,
				spriteram[offs+64] & 0x80,0,
				spriteram[offs+192],216-spriteram[offs],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
