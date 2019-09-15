/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of Astro Fighter hardware games.

  Lee Taylor 28/11/1997

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *astrof_color;
unsigned char *tomahawk_protection;

static int flipscreen = 0;
static int force_refresh = 0;
static int do_modify_palette = 0;
static int palette_bank = -1, red_on = -1;
static int protection_value;
static const unsigned char *prom;


/* Just save the colorprom pointer */
void astrof_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	prom = color_prom;
}

/***************************************************************************

  Convert the color PROMs into a more useable format.

  The palette PROMs are connected to the RGB output this way:

  bit 0 -- RED
        -- RED
        -- GREEN
	  	-- GREEN
        -- BLUE
  bit 5 -- BLUE

  I couldn't really determine the resistances, (too many resistors and capacitors)
  so the value below might be off a tad. But since there is also a variable
  resistor for each color gun, this is one of the concievable settings

***************************************************************************/
static void modify_palette(void)
{
	int i, col_index;

	col_index = (palette_bank ? 16 : 0);

	for (i = 0;i < Machine->drv->total_colors; i++)
	{
		int bit0,bit1,r,g,b;

		bit0 = ((prom[col_index] >> 0) & 0x01) | (red_on >> 3);
		bit1 = ((prom[col_index] >> 1) & 0x01) | (red_on >> 3);
		r = 0xc0 * bit0 + 0x3f * bit1;

		bit0 = ( prom[col_index] >> 2) & 0x01;
		bit1 = ( prom[col_index] >> 3) & 0x01;
		g = 0xc0 * bit0 + 0x3f * bit1;

		bit0 = ( prom[col_index] >> 4) & 0x01;
		bit1 = ( prom[col_index] >> 5) & 0x01;
		b = 0xc0 * bit0 + 0x3f * bit1;

		col_index++;

		palette_change_color(i,r,g,b);
	}
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int astrof_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;
	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void astrof_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



static void common_videoram_w(int offset,int data,int palette)
{
	/* DO NOT try to optimize this by comparing if the value actually changed.
	   The game writes the same bytes with different colors. For example, the
	   fuel meter doesn't work with that 'optimization' */

	int i,x,y,dy,fore,back,color;

	videoram[offset] = data;

	color = *astrof_color & 0x0e;

	fore = Machine->pens[color | palette | 1];
	back = Machine->pens[color | palette    ];

	x = offset & 0xff;
	y = (offset >> 8) << 3;
	dy = 1;

	if (flipscreen)
	{
		x = 255 - x;
		y = 255 - y;
		dy = -1;
	}

	osd_mark_dirty(x,y,x,y+(7*dy),0);

	for (i = 0; i < 8; i++)
	{
		if (data & 0x01)
			Machine->scrbitmap->line[y][x] = tmpbitmap->line[y][x] = fore;
		else
			Machine->scrbitmap->line[y][x] = tmpbitmap->line[y][x] = back;

		y += dy;
		data >>= 1;
	}
}

void astrof_videoram_w(int offset,int data)
{
	// Astro Fighter's palette is set in astrof_video_control2_w, D0 is unused
	common_videoram_w(offset, data, 0);
}

void tomahawk_videoram_w(int offset,int data)
{
	// Tomahawk's palette is set per byte
	int palette = (*astrof_color & 0x01) << 4;

	common_videoram_w(offset, data, palette);
}


void astrof_video_control1_w(int offset,int data)
{
	int x,y;

	// Video control register 1
	//
	// Bit 0     = Flip screen
	// Bit 1     = Shown in schematics as what appears to be a screen clear
	//             bit, but it's always zero in Astro Fighter
	// Bit 2     = Not hooked up in the schematics, but at one point the game
	//			   sets it to 1.
	// Bit 3-7   = Not hooked up

	if (input_port_2_r(0) & 0x02) /* Cocktail mode */
	{
		if (flipscreen != (data & 0x01))
		{
			flipscreen = data & 0x01;
			force_refresh = 1;

			// Flip bitmap
			for (y = 0; y < 128; y++)
			{
				for (x = 0; x < 128; x++)
				{
					unsigned char t = tmpbitmap->line[y][x];
					tmpbitmap->line[y][x] = tmpbitmap->line[255 - y][255 - x];
					tmpbitmap->line[255 - y][255 - x] = t;

					t = tmpbitmap->line[255 - y][x];
					tmpbitmap->line[255 - y][x] = tmpbitmap->line[y][255 - x];
					tmpbitmap->line[y][255 - x] = t;
				}
			}
		}
	}
}


// Video control register 2
//
// Bit 0     = Hooked up to a connector called OUT0, don't know what it does
// Bit 1     = Hooked up to a connector called OUT1, don't know what it does
// Bit 2     = Palette select in Astro Fighter, unused in Tomahawk
// Bit 3     = Turns on RED color gun regardless of what the value is
// 			   in the color PROM
// Bit 4-7   = Not hooked up

void astrof_video_control2_w(int offset,int data)
{
	if (palette_bank != (data & 0x04))
	{
		palette_bank = (data & 0x04);
		do_modify_palette = 1;
	}

	if (red_on != (data & 0x08))
	{
		red_on = data & 0x08;
		do_modify_palette = 1;
	}

	/* Defer changing the colors to avoid flicker */
}

void tomahawk_video_control2_w(int offset,int data)
{
	if (palette_bank == -1)
	{
		palette_bank = 0;
		do_modify_palette = 1;
	}

	if (red_on != (data & 0x08))
	{
		red_on = data & 0x08;
		do_modify_palette = 1;
	}

	/* Defer changing the colors to avoid flicker */
}


int tomahawk_protection_r(int offset)
{
	switch (*tomahawk_protection)
	{
	case 0xeb:
		return 0xd7;

	case 0x58:
		return 0x1a;

	case 0x43:
		return 0xc2;

	default:
		if (errorlog) fprintf(errorlog, "Unknown protection read %02X\n", protection_value);
		return 0x00;
	}
}
/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void astrof_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	if (do_modify_palette)
	{
		modify_palette();

		do_modify_palette = 0;
	}

	if (!full_refresh && !force_refresh)
		return;

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	force_refresh = 0;
}

