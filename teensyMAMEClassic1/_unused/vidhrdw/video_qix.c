/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *qix_palettebank;
unsigned char *qix_videoaddress;
static unsigned char *screen;

//#define DEBUG_LEDS

#ifdef DEBUG_LEDS
#include <stdio.h>
FILE	*led_log;
#endif


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Qix doesn't have colors PROMs, it uses RAM. The meaning of the bits are
  bit 7 -- Red
        -- Red
        -- Green
        -- Green
        -- Blue
        -- Blue
        -- Intensity
  bit 0 -- Intensity

***************************************************************************/
static void update_pen (int pen, int val)
{
	/* this conversion table should be about right. It gives a reasonable */
	/* gray scale in the test screen, and the red, green and blue squares */
	/* in the same screen are barely visible, as the manual requires. */
	static unsigned char table[16] =
	{
		0x00,	/* value = 0, intensity = 0 */
		0x12,	/* value = 0, intensity = 1 */
		0x24,	/* value = 0, intensity = 2 */
		0x49,	/* value = 0, intensity = 3 */
		0x12,	/* value = 1, intensity = 0 */
		0x24,	/* value = 1, intensity = 1 */
		0x49,	/* value = 1, intensity = 2 */
		0x92,	/* value = 1, intensity = 3 */
		0x5b,	/* value = 2, intensity = 0 */
		0x6d,	/* value = 2, intensity = 1 */
		0x92,	/* value = 2, intensity = 2 */
		0xdb,	/* value = 2, intensity = 3 */
		0x7f,	/* value = 3, intensity = 0 */
		0x91,	/* value = 3, intensity = 1 */
		0xb6,	/* value = 3, intensity = 2 */
		0xff	/* value = 3, intensity = 3 */
	};

	int bits,intensity,red,green,blue;

	intensity = (val >> 0) & 0x03;
	bits = (val >> 6) & 0x03;
	red = table[(bits << 2) | intensity];
	bits = (val >> 4) & 0x03;
	green = table[(bits << 2) | intensity];
	bits = (val >> 2) & 0x03;
	blue = table[(bits << 2) | intensity];

	palette_change_color(pen,red,green,blue);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int qix_vh_start(void)
{
	if ((screen = malloc(256*256)) == 0)
		return 1;

	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(screen);
		return 1;
	}

	#ifdef DEBUG_LEDS
	led_log = fopen ("led.log","w");
	#endif

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void qix_vh_stop(void)
{
	osd_free_bitmap (tmpbitmap);
	free (screen);

#ifdef DEBUG_LEDS
	if (led_log) fclose (led_log);
#endif
}



/* The screen is 256x256 with eight bit pixels (64K).  The screen is divided
into two halves each half mapped by the video CPU at $0000-$7FFF.  The
high order bit of the address latch at $9402 specifies which half of the
screen is being accessed.

The address latch works as follows.  When the video CPU accesses $9400,
the screen address is computed by using the values at $9402 (high byte)
and $9403 (low byte) to get a value between $0000-$FFFF.  The value at
that location is either returned or written. */

int qix_videoram_r(int offset)
{
	offset += (qix_videoaddress[0] & 0x80) * 0x100;
	return screen[offset];
}

void qix_videoram_w(int offset,int data)
{
	int x, y, temp;

	offset += (qix_videoaddress[0] & 0x80) * 0x100;

	x = offset & 0xff;
	y = offset >> 8;

	/* rotate if necessary */
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		temp = x; x = y; y = temp;
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
		x = ~x & 0xff;
	if (Machine->orientation & ORIENTATION_FLIP_Y)
		y = ~y & 0xff;


	Machine->scrbitmap->line[y][x] = tmpbitmap->line[y][x] = Machine->pens[data];

	osd_mark_dirty(x,y,x,y,0);

	screen[offset] = data;
}



int qix_addresslatch_r(int offset)
{
	offset = qix_videoaddress[0] * 0x100 + qix_videoaddress[1];
	return screen[offset];
}



void qix_addresslatch_w(int offset,int data)
{
	int x, y, temp;

	offset = qix_videoaddress[0] * 0x100 + qix_videoaddress[1];

	x = offset & 0xff;
	y = offset >> 8;

	/* rotate if necessary */
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		temp = x; x = y; y = temp;
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
		x = ~x & 0xff;
	if (Machine->orientation & ORIENTATION_FLIP_Y)
		y = ~y & 0xff;


	Machine->scrbitmap->line[y][x] = tmpbitmap->line[y][x] = Machine->pens[data];

	osd_mark_dirty(x,y,x,y,0);

	screen[offset] = data;
}



/* The color RAM works as follows.  The color RAM contains palette values for
four pages (0-3).  When a write to $8800 on the video CPU occurs, the color
RAM page is taken from the lowest 2 bits of the value.  This selects one of
the color RAM pages as follows:

     colorRAMAddr = 0x9000 + ((data & 0x03) * 0x100);

Qix uses a palette of 64 colors (2 each RGB) and four intensities (RRGGBBII).
*/
void qix_paletteram_w(int offset,int data)
{
	paletteram[offset] = data;

	if ((*qix_palettebank & 0x03) == (offset / 256))
		update_pen (offset % 256, data);
}



void qix_palettebank_w(int offset,int data)
{
	if ((*qix_palettebank & 0x03) != (data & 0x03))
	{
		unsigned char *pram = &paletteram[256 * (data & 0x03)];
		int i;

		for (i = 0;i < 256;i++)
			update_pen (i, *pram++);
	}

	*qix_palettebank = data;

#ifdef DEBUG_LEDS
	data = ~(data) & 0xfc;
	#if 0
		if (led_log)
		printf ("LEDS: %d %d %d %d %d %d", (data & 0x80)>>7, (data & 0x40)>>6, (data & 0x20)>>5,
			(data & 0x10)>>4, (data & 0x08)>>3, (data & 0x04)>>2 );
	#else
		if (led_log)
			fprintf (led_log, "LEDS: %d %d %d %d %d %d\n", (data & 0x80)>>7, (data & 0x40)>>6,
				(data & 0x20)>>5, (data & 0x10)>>4, (data & 0x08)>>3, (data & 0x04)>>2 );
//			fprintf (led_log, "PC: %04x LEDS: %d %d %d %d %d %d\n", cpu_getpc(),
//				(data & 0x04)>>2,(data & 0x08)>>3,
//				(data & 0x10)>>4, (data & 0x20)>>5, (data & 0x40)>>6, (data & 0x80)>>7);
	#endif
#endif
}


void qix_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* recalc the palette if necessary */
	if (palette_recalc ())
	{
		int offs;

		for (offs = 0; offs < 256*256; offs++)
		{
			int x = offs & 0xff;
			int y = offs >> 8;
			int temp;

			/* rotate if necessary */
			if (Machine->orientation & ORIENTATION_SWAP_XY)
			{
				temp = x; x = y; y = temp;
			}
			if (Machine->orientation & ORIENTATION_FLIP_X)
				x = ~x & 0xff;
			if (Machine->orientation & ORIENTATION_FLIP_Y)
				y = ~y & 0xff;


			Machine->scrbitmap->line[y][x] = tmpbitmap->line[y][x] = Machine->pens[screen[offs]];
		}

		osd_mark_dirty (0,0,255,255,0);
	}

	if (full_refresh)
		/* copy the screen */
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
