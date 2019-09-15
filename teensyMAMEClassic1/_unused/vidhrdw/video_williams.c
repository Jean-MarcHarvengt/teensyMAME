/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *williams_bank_select;
extern unsigned char *blaster_bank_ram;

extern int sinistar_clip;
extern int williams_cocktail;

extern void williams_vram_select_w (int offset, int data);


unsigned char *williams_blitterram;
unsigned char *williams_videoram;
unsigned char *williams_remap_select;

unsigned char *blaster_video_bits;
unsigned char *blaster_color_zero_table;
unsigned char *blaster_color_zero_flags;


static const unsigned char *williams_remap;
static unsigned char *williams_remap_lookup;
static struct osd_bitmap *williams_bitmap;

static int blitter_xor;
static int blitter_solid;

static int blaster_erase_screen;


/* pixel plotters */

typedef void (*put_pix_func) (int offset, int pix);

static put_pix_func put_pix;

static void put_pix_normal (int offset, int pix);
static void put_pix_fx (int offset, int pix);
static void put_pix_fy (int offset, int pix);
static void put_pix_fx_fy (int offset, int pix);

static void put_pix_swap (int offset, int pix);
static void put_pix_swap_fx (int offset, int pix);
static void put_pix_swap_fy (int offset, int pix);
static void put_pix_swap_fx_fy (int offset, int pix);


/* blitter functions */

typedef void (*blitter_func)(int, int, int);

static blitter_func transparent_blitter_w;
static blitter_func transparent_solid_blitter_w;
static blitter_func opaque_blitter_w;
static blitter_func opaque_solid_blitter_w;

static void williams_transparent_blitter_w (int offset, int data, int keepmask);
static void williams_transparent_solid_blitter_w (int offset, int data, int keepmask);
static void williams_opaque_blitter_w (int offset, int data, int keepmask);
static void williams_opaque_solid_blitter_w (int offset, int data, int keepmask);

static void remap_transparent_blitter_w (int offset, int data, int keepmask);
static void remap_transparent_solid_blitter_w (int offset, int data, int keepmask);
static void remap_opaque_blitter_w (int offset, int data, int keepmask);

static void sinistar_transparent_blitter_w (int offset, int data, int keepmask);
static void sinistar_transparent_solid_blitter_w (int offset, int data, int keepmask);
static void sinistar_opaque_blitter_w (int offset, int data, int keepmask);
static void sinistar_opaque_solid_blitter_w (int offset, int data, int keepmask);



/***************************************************************************

	Common Williams routines

***************************************************************************/

void williams_vh_convert_color_prom (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	/* no remap table by default */
	williams_remap_lookup = 0;
}


/*
 *  Start the video hardware emulation
 */

int williams_vh_start (void)
{
	/* Allocate the offscreen bitmap */
	williams_bitmap = osd_create_bitmap (Machine->drv->screen_width, Machine->drv->screen_height);
	if (!williams_bitmap)
		return 1;

	/* Allocate space for video ram  */
	if ((williams_videoram = malloc (videoram_size)) == 0)
	{
		osd_free_bitmap (williams_bitmap);
		return 1;
	}
	memset (williams_videoram, 0, videoram_size);

	/* By default xor with 4 (SC1 chip) */
	blitter_xor = 4;

	/* Set up the standard blitters */
	transparent_blitter_w = williams_remap_lookup ? remap_transparent_blitter_w : williams_transparent_blitter_w;
	opaque_blitter_w = williams_remap_lookup ? remap_opaque_blitter_w : williams_opaque_blitter_w;
	transparent_solid_blitter_w = williams_remap_lookup ? remap_transparent_solid_blitter_w : williams_transparent_solid_blitter_w;
	opaque_solid_blitter_w = williams_opaque_solid_blitter_w;

	/* Reset the erase screen flag */
	blaster_erase_screen = 0;

	/* Set up the pixel plotter */
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		if (Machine->orientation & ORIENTATION_FLIP_Y)
		{
			if (Machine->orientation & ORIENTATION_FLIP_X)
				put_pix = put_pix_swap_fx_fy;
			else
				put_pix = put_pix_swap_fy;
		}
		else
		{
			if (Machine->orientation & ORIENTATION_FLIP_X)
				put_pix = put_pix_swap_fx;
			else
				put_pix = put_pix_swap;
		}
	}
	else
	{
		if (Machine->orientation & ORIENTATION_FLIP_Y)
		{
			if (Machine->orientation & ORIENTATION_FLIP_X)
				put_pix = put_pix_fx_fy;
			else
				put_pix = put_pix_fy;
		}
		else
		{
			if (Machine->orientation & ORIENTATION_FLIP_X)
				put_pix = put_pix_fx;
			else
				put_pix = put_pix_normal;
		}
	}

	return 0;
}


/*
 *  Start the video hardware emulation, but for an SC2 chip game
 */

int williams_vh_start_sc2 (void)
{
	int result = williams_vh_start ();

	/* the SC2 chip fixes the xor bug */
	blitter_xor = 0;

	return result;
}


/*
 *  Stop the video hardware emulation
 */

void williams_vh_stop (void)
{
	/* free any remap lookup tables */
	if (williams_remap_lookup)
		free (williams_remap_lookup);
	williams_remap_lookup = 0;

	/* free other stuff */
	free (williams_videoram);
	osd_free_bitmap (williams_bitmap);
}


/*
 *  Update part of the screen (note that we go directly to the scrbitmap here!)
 */

void williams_vh_update (int counter)
{
	struct rectangle clip;

	/* we only update every 16 scan lines to keep things moving reasonably */
	if (counter & 0x0f) return;

	/* wrap around at the bottom */
	if (counter == 0) counter = 256;

	/* determine the clip rect */
	clip.min_x = 0;
	clip.max_x = Machine->drv->screen_width - 1;
	clip.min_y = counter - 16;
	clip.max_y = clip.min_y + 15;

	/* combine the clip rect with the visible rect */
	if (Machine->drv->visible_area.min_x > clip.min_x)
		clip.min_x = Machine->drv->visible_area.min_x;
	if (Machine->drv->visible_area.max_x < clip.max_x)
		clip.max_x = Machine->drv->visible_area.max_x;
	if (Machine->drv->visible_area.min_y > clip.min_y)
		clip.min_y = Machine->drv->visible_area.min_y;
	if (Machine->drv->visible_area.max_y < clip.max_y)
		clip.max_y = Machine->drv->visible_area.max_y;

	/* copy */
	copybitmap (Machine->scrbitmap, williams_bitmap, 0, 0, 0, 0, &clip, TRANSPARENCY_NONE, 0);

	/* optionally erase from lines 24 downward */
	if (blaster_erase_screen && clip.max_y > 24)
	{
		int offset, count;

		/* erase the actual bitmap */
		if (clip.min_y < 24) clip.min_y = 24;
		fillbitmap (williams_bitmap, Machine->pens[0], &clip);

		/* erase the memory associated with this area */
		count = clip.max_y - clip.min_y + 1;
		for (offset = clip.min_y; offset < videoram_size; offset += 0x100)
			memset (&williams_videoram[offset], 0, count);
	}
}


/*
 *  Video update - not needed, the video is updated in chunks
 */

void williams_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
}


/*
 *  Generic videoram write function; works for every game
 */

void williams_videoram_w (int offset, int data)
{
	/* Put the byte in the videoram */
	williams_videoram[offset] = data;

	/* Put the pixels in our bitmap */
	(*put_pix) (offset, data);
}



/*
 *	Remap table select
 */

void williams_remap_select_w (int offset, int data)
{
	*williams_remap_select = data;
	williams_remap = williams_remap_lookup + data * 256;
}



/***************************************************************************

	Common blitter routines

***************************************************************************/

/*

	Blitter description from Sean Riddle's page:

	This page contains information about the Williams Special Chips, which
	were 'bit blitters'- block transfer chips that could move data around on
	the screen and in memory faster than the CPU. In fact, I've timed the
	special chips at 16 megs in 18.1 seconds. That's 910K/sec, not bad for
	the early 80s.

	The blitters were not used in Defender and Stargate, but
	were added to the ROM boards of the later games. Splat!, Blaster, Mystic
	Marathon and Joust 2 used Special Chip 2s. The only difference that I've
	seen is that SC1s have a small bug. When you tell the SC1 the size of
	the data to move, you have to exclusive-or the width and height with 2.
	The SC2s eliminate this bug.

	The blitters were accessed at memory location $CA00-CA06.

	CA01 is the mask, usually $FF to move all bits.
	CA02-3 is the source data location.
	CA04-5 is the destination data location.

	Writing to CA00 starts the blit, and the byte written determines how the
	data is blitted.

	Bit 0 indicates that the source data is either laid out linear, one
	pixel after the last, or in screen format, where there are 256 bytes from
	one pair of pixels to the next.

	Bit 1 indicates the same, but for the destination data.

	I'm not sure what bit 2 does. Looking at the image, I can't tell, but
	perhaps it has to do with the mask. My test files only used a mask of $FF.

	Bit 3 tells the blitter only to blit the foreground- that is, everything
	that is not color 0. Also known as transparency mode.

	Bit 4 is 'solid' mode. Only the color indicated by the mask is blitted.
	Note that this just creates a rectangle unless bit 3 is also set, in which
	case it blits the image, but in a solid color.

	Bit 5 shifts the image one pixel to the right. Any data on the far right
	jumps to the far left.

	Bits 6 and 7 only blit every other pixel of the image. Bit 6 says even only,
	while bit 7 says odd only.

*/


/*
 *  Macros for blitter destination read/write access
 */

#define williams_blitter_dest_r(o) (((o) < videoram_size) ? williams_videoram[o] : cpu_readmem16 (o))
#define williams_blitter_dest_w(o,v) if ((o) < videoram_size) { williams_videoram[o] = (v); (*put_pix)((o), (v)); } else cpu_writemem16 ((o), (v))


/*
 *  Handler for the actual writes to the blitter
 */

void williams_blitter_w (int offset, int data)
{
	blitter_func blitter_w;
	int source, sstart, sxadv, syadv;
	int dest, dstart, dxadv, dyadv;
	int i, j, w, h;
	int keepmask;

	/* only writes to location 0 trigger the blit */
	if (offset)
	{
		williams_blitterram[offset] = data;
		return;
	}

	/* compute the starting locations */
	sstart = (williams_blitterram[2] << 8) + williams_blitterram[3];
	dstart = (williams_blitterram[4] << 8) + williams_blitterram[5];

	/* compute the width and height */
	w = williams_blitterram[6] ^ blitter_xor;
	h = williams_blitterram[7] ^ blitter_xor;

	/* adjust the width and height */
	if (w == 0) w = 1;
	if (h == 0) h = 1;
	if (w == 255) w = 256;
	if (h == 255) h = 256;

	/* compute how much to advance in the x and y loops */
	sxadv = (data & 0x01) ? 0x100 : 1;
	syadv = (data & 0x01) ? 1 : w;
	dxadv = (data & 0x02) ? 0x100 : 1;
	dyadv = (data & 0x02) ? 1 : w;

	/* determine the common mask */
	keepmask = 0x00;
	if (data & 0x80) keepmask |= 0xf0;
	if (data & 0x40) keepmask |= 0x0f;
	if (keepmask == 0xff)
		return;

	/* set the solid pixel value to the mask value */
	blitter_solid = williams_blitterram[1];

	/* pick the blitter */
	if (data & 0x10)
		blitter_w = (data & 0x08) ? transparent_solid_blitter_w : opaque_solid_blitter_w;
	else
		blitter_w = (data & 0x08) ? transparent_blitter_w : opaque_blitter_w;

	/* first case: no shifting */
	if (!(data & 0x20))
	{
		/* loop over the height */
		for (i = 0; i < h; i++)
		{
			source = sstart & 0xffff;
			dest = dstart & 0xffff;

			/* loop over the width */
			for (j = w; j > 0; j--)
			{
				(*blitter_w) (dest, cpu_readmem16 (source), keepmask);

				source = (source + sxadv) & 0xffff;
				dest   = (dest + dxadv) & 0xffff;
			}

			sstart += syadv;
			dstart += dyadv;
		}
	}

	/* second case: shifted one pixel */
	else
	{
		/* swap halves of the keep mask and the solid color */
		keepmask = ((keepmask & 0xf0) >> 4) | ((keepmask & 0x0f) << 4);
		blitter_solid = ((blitter_solid & 0xf0) >> 4) | ((blitter_solid & 0x0f) << 4);

		/* loop over the height */
		for (i = 0; i < h; i++)
		{
			int pixdata;

			source = sstart & 0xffff;
			dest = dstart & 0xffff;

			/* left edge case */
			pixdata = cpu_readmem16 (source);
			(*blitter_w) (dest, (pixdata >> 4) & 0x0f, keepmask | 0xf0);

			source = (source + sxadv) & 0xffff;
			dest   = (dest + dxadv) & 0xffff;

			/* loop over the width */
			for (j = w-1; j > 0; j--)
			{
				pixdata = (pixdata << 8) | cpu_readmem16 (source);
				(*blitter_w) (dest, (pixdata >> 4) & 0xff, keepmask);

				source = (source + sxadv) & 0xffff;
				dest   = (dest + dxadv) & 0xffff;
			}

			/* right edge case */
			(*blitter_w) (dest, (pixdata << 4) & 0xf0, keepmask | 0x0f);

			sstart += syadv;
			dstart += dyadv;
		}
	}

	/* Log blits */
	if (errorlog)
	{
		fprintf(errorlog,"---------- Blit %02X--------------PC: %04X\n",data,cpu_getpc());
		fprintf(errorlog,"Source : %02X %02X\n",williams_blitterram[2],williams_blitterram[3]);
		fprintf(errorlog,"Dest   : %02X %02X\n",williams_blitterram[4],williams_blitterram[5]);
		fprintf(errorlog,"W H    : %02X %02X (%d,%d)\n",williams_blitterram[6],williams_blitterram[7],williams_blitterram[6]^4,williams_blitterram[7]^4);
		fprintf(errorlog,"Mask   : %02X\n",williams_blitterram[1]);
	}
}


/*
 *  Default blitting routines
 */

static void williams_transparent_blitter_w (int offset, int data, int keepmask)
{
	if (data)
	{
		int pix = williams_blitter_dest_r (offset);

		if (!(data & 0xf0)) keepmask |= 0xf0;
		if (!(data & 0x0f)) keepmask |= 0x0f;

		pix = (pix & keepmask) | (data & ~keepmask);
		williams_blitter_dest_w (offset, pix);
	}
}

static void williams_transparent_solid_blitter_w (int offset, int data, int keepmask)
{
	if (data)
	{
		int pix = williams_blitter_dest_r (offset);

		if (!(data & 0xf0)) keepmask |= 0xf0;
		if (!(data & 0x0f)) keepmask |= 0x0f;

		pix = (pix & keepmask) | (blitter_solid & ~keepmask);
		williams_blitter_dest_w (offset, pix);
	}
}

static void williams_opaque_blitter_w (int offset, int data, int keepmask)
{
	int pix = williams_blitter_dest_r (offset);
	pix = (pix & keepmask) | (data & ~keepmask);
	williams_blitter_dest_w (offset, pix);
}

static void williams_opaque_solid_blitter_w (int offset, int data, int keepmask)
{
	int pix = williams_blitter_dest_r (offset);
	pix = (pix & keepmask) | (blitter_solid & ~keepmask);
	williams_blitter_dest_w (offset, pix);
}


/*
 *  Remapping blitting routines; used by Blaster, maybe others
 */

static void remap_transparent_blitter_w (int offset, int data, int keepmask)
{
	data = williams_remap[data & 0xff];
	if (data)
	{
		int pix = williams_blitter_dest_r (offset);

		if (!(data & 0xf0)) keepmask |= 0xf0;
		if (!(data & 0x0f)) keepmask |= 0x0f;

		pix = (pix & keepmask) | (data & ~keepmask);
		williams_blitter_dest_w (offset, pix);
	}
}

static void remap_transparent_solid_blitter_w (int offset, int data, int keepmask)
{
	data = williams_remap[data & 0xff];
	if (data)
	{
		int pix = williams_blitter_dest_r (offset);

		if (!(data & 0xf0)) keepmask |= 0xf0;
		if (!(data & 0x0f)) keepmask |= 0x0f;

		pix = (pix & keepmask) | (blitter_solid & ~keepmask);
		williams_blitter_dest_w (offset, pix);
	}
}

static void remap_opaque_blitter_w (int offset, int data, int keepmask)
{
	int pix = williams_blitter_dest_r (offset);
	data = williams_remap[data & 0xff];
	pix = (pix & keepmask) | (data & ~keepmask);
	williams_blitter_dest_w (offset, pix);
}



/***************************************************************************

	Utility functions to put 2 Pixels in the real bitmap

***************************************************************************/

static void put_pix_normal (int offset, int pix)
{
	int p1 = Machine->pens[pix >> 4], p2 = Machine->pens[pix & 0x0f];
	int x = (offset / 256) * 2, y = offset % 256;
	unsigned char *dest = &williams_bitmap->line[y][x];
	*dest++ = p1;
	*dest = p2;
	osd_mark_dirty (x,y,x+1,y,0);
}

static void put_pix_fx (int offset, int pix)
{
	int p1 = Machine->pens[pix >> 4], p2 = Machine->pens[pix & 0x0f];
	int x = (williams_bitmap->width-2) - (offset / 256) * 2, y = offset % 256;
	unsigned char *dest = &williams_bitmap->line[y][x];
	*dest++ = p2;
	*dest = p1;
	osd_mark_dirty (x,y,x+1,y,0);
}

static void put_pix_fy (int offset, int pix)
{
	int p1 = Machine->pens[pix >> 4], p2 = Machine->pens[pix & 0x0f];
	int x = (offset / 256) * 2, y = (williams_bitmap->height-1) - (offset % 256);
	unsigned char *dest = &williams_bitmap->line[y][x];
	*dest++ = p1;
	*dest = p2;
	osd_mark_dirty (x,y,x+1,y,0);
}

static void put_pix_fx_fy (int offset, int pix)
{
	int p1 = Machine->pens[pix >> 4], p2 = Machine->pens[pix & 0x0f];
	int x = (williams_bitmap->width-2) - (offset / 256) * 2, y = (williams_bitmap->height-1) - (offset % 256);
	unsigned char *dest = &williams_bitmap->line[y][x];
	*dest++ = p2;
	*dest = p1;
	osd_mark_dirty (x,y,x+1,y,0);
}

static void put_pix_swap (int offset, int pix)
{
	int p1 = Machine->pens[pix >> 4], p2 = Machine->pens[pix & 0x0f];
	int y = (offset / 256) * 2, x = offset % 256;
	williams_bitmap->line[y][x] = p1;
	williams_bitmap->line[y+1][x] = p2;
	osd_mark_dirty (x,y,x,y+1,0);
}

static void put_pix_swap_fx (int offset, int pix)
{
	int p1 = Machine->pens[pix >> 4], p2 = Machine->pens[pix & 0x0f];
	int y = (offset / 256) * 2, x = (williams_bitmap->width-1) - offset % 256;
	williams_bitmap->line[y][x] = p1;
	williams_bitmap->line[y+1][x] = p2;
	osd_mark_dirty (x,y,x,y+1,0);
}

static void put_pix_swap_fy (int offset, int pix)
{
	int p1 = Machine->pens[pix >> 4], p2 = Machine->pens[pix & 0x0f];
	int y = (williams_bitmap->height-2) - (offset / 256) * 2, x = offset % 256;
	williams_bitmap->line[y][x] = p2;
	williams_bitmap->line[y+1][x] = p1;
	osd_mark_dirty (x,y,x,y+1,0);
}

static void put_pix_swap_fx_fy (int offset, int pix)
{
	int p1 = Machine->pens[pix >> 4], p2 = Machine->pens[pix & 0x0f];
	int y = (williams_bitmap->height-2) - (offset / 256) * 2, x = (williams_bitmap->width-1) - offset % 256;
	williams_bitmap->line[y][x] = p2;
	williams_bitmap->line[y+1][x] = p1;
	osd_mark_dirty (x,y,x,y+1,0);
}



/***************************************************************************

	Defender-specific routines

***************************************************************************/

/*
 * Defender video ram Write
 * Same as the others but Write in RAM[]
 */

void defender_videoram_w (int offset, int data)
{
	/* Write to the real video RAM */
	videoram[offset] = data;

	/* Put the pixels in our bitmap */
	(*put_pix) (offset, data);
}



/***************************************************************************

	Sinistar-specific routines

***************************************************************************/

int sinistar_vh_start (void)
{
	int result = williams_vh_start ();

	/* Sinistar uses special blitters with a clipping circuit */
	transparent_blitter_w = sinistar_transparent_blitter_w;
	transparent_solid_blitter_w = sinistar_transparent_solid_blitter_w;
	opaque_blitter_w = sinistar_opaque_blitter_w;
	opaque_solid_blitter_w = sinistar_opaque_solid_blitter_w;

	return result;
}


/*
 *  Sinistar blitting routines -- same as above, but also perform clipping
 */

static void sinistar_transparent_blitter_w (int offset, int data, int keepmask)
{
	if (data && (!sinistar_clip || offset < 0x7400))
	{
		int pix = williams_blitter_dest_r (offset);

		if (!(data & 0xf0)) keepmask |= 0xf0;
		if (!(data & 0x0f)) keepmask |= 0x0f;

		pix = (pix & keepmask) | (data & ~keepmask);
		williams_blitter_dest_w (offset, pix);
	}
}

static void sinistar_transparent_solid_blitter_w (int offset, int data, int keepmask)
{
	if (data && (!sinistar_clip || offset < 0x7400))
	{
		int pix = williams_blitter_dest_r (offset);

		if (!(data & 0xf0)) keepmask |= 0xf0;
		if (!(data & 0x0f)) keepmask |= 0x0f;

		pix = (pix & keepmask) | (blitter_solid & ~keepmask);
		williams_blitter_dest_w (offset, pix);
	}
}

static void sinistar_opaque_blitter_w (int offset, int data, int keepmask)
{
	if (!sinistar_clip || offset < 0x7400)
	{
		int pix = williams_blitter_dest_r (offset);
		pix = (pix & keepmask) | (data & ~keepmask);
		williams_blitter_dest_w (offset, pix);
	}
}

static void sinistar_opaque_solid_blitter_w (int offset, int data, int keepmask)
{
	if (!sinistar_clip || offset < 0x7400)
	{
		int pix = williams_blitter_dest_r (offset);
		pix = (pix & keepmask) | (blitter_solid & ~keepmask);
		williams_blitter_dest_w (offset, pix);
	}
}



/***************************************************************************

	Blaster-specific routines

***************************************************************************/

/*
 *  Create the palette
 */

void blaster_vh_convert_color_prom (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	/* Expand the lookup table so that we do one lookup per byte */
	williams_remap_lookup = malloc (256 * 256);
	if (williams_remap_lookup)
	{
		int i;

		for (i = 0; i < 256; i++)
		{
			const unsigned char *table = color_prom + (i & 0x7f) * 16;
			int j;
			for (j = 0; j < 256; j++)
				williams_remap_lookup[i * 256 + j] = (table[j >> 4] << 4) | table[j & 0x0f];
		}
	}
}


/*
 *  Blaster-specific screen refresh; handles the zero color palette change
 */

void blaster_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i, j;
	int back_color;
	int pen0 = palette_transparent_pen;
	int back_pen;
	int first = -1;

	/* Recalculate palette */
	for (j = 0; j < 0x100; j++)
	{
		paletteram_BBGGGRRR_w(j + 16,blaster_color_zero_table[j] ^ 0xff);
	}


	if (palette_recalc ())
	{
		for (i = 0; i < 0x9800; i++)
			williams_videoram_w (i, williams_videoram[i]);
	}

	/* Copy williams_bitmap in bitmap */
	williams_vh_screenrefresh(bitmap,full_refresh);

	/* The color 0 of the palette can change at each video line */
	/* Since we cannot do that on a PC, we do that in a copy of the bitmap */
	/* This cannot be done in williams_bitmap because we have to keep the original bitmap intact */
	if ((*blaster_video_bits & 0x01) != 0)
	{
		back_color = 0;

		for (j = 0; j < 0x100; j++)
		{
			if ((blaster_color_zero_flags[j] & 0x01) != 0)
			{
				if ((blaster_color_zero_table[j] ^ 0xff) == 0)
					back_color = 0;
				else back_color = 16 + j;
			}

			/* Update the dirty marking */
			if (back_color == 0)
			{
				if (first != -1) osd_mark_dirty (0, first, Machine->drv->screen_width - 1, j-1, 0);
				continue;
			}
			if (first == -1) first = j;

			/* change all 0-colored pixels on this line */
			back_pen = Machine->pens[back_color];
			for (i = 0; i < Machine->drv->screen_width - 2; i++)
				if (bitmap->line[j][i] == pen0)
					bitmap->line[j][i] = back_pen;
		}

		if (first != -1) osd_mark_dirty (0, first, Machine->drv->screen_width - 1, 0xff, 0);
	}
}


/*
 *  Blaster-specific video flags handler
 */

void blaster_video_bits_w (int offset, int data)
{
	*blaster_video_bits = data;
	blaster_erase_screen = data & 0x02;
}

int blaster_vh_start(void)
{
	int i;


	/* mark color 0 as transparent. We will draw the rainbow background behind it. */
	palette_used_colors[0] = PALETTE_COLOR_TRANSPARENT;
	for (i = 0;i < 256;i++)
	{
		/* mark as used only the colors used for the visible background lines */
		if (i < Machine->drv->visible_area.min_y ||
				i > Machine->drv->visible_area.max_y)
			palette_used_colors[16 + i] = PALETTE_COLOR_UNUSED;

		/* TODO: this leaves us with a total of 255+1 colors used, which is just */
		/* a bit too much for the palette system to handle them efficiently. */
		/* As a quick workaround, I set the top three lines to be always black. */
		/* To do it correctly, vh_screenrefresh() should group the background */
		/* lines of the same color and mark the others as COLOR_UNUSED. */
		/* The background is very redundant so this can be done easily. */
		palette_used_colors[16 + Machine->drv->visible_area.min_y] = PALETTE_COLOR_TRANSPARENT;
		palette_used_colors[16 + 1+Machine->drv->visible_area.min_y] = PALETTE_COLOR_TRANSPARENT;
		palette_used_colors[16 + 2+Machine->drv->visible_area.min_y] = PALETTE_COLOR_TRANSPARENT;
	}

	return williams_vh_start_sc2();
}
