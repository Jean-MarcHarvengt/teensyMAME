/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"

#define XCHARS 64
#define YCHARS 48

#define XDIM (XCHARS*8)
#define YDIM (YCHARS*8)


struct toobin_mo_data
{
	int *redraw_list, *redraw;
};



/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *toobin_intensity;
unsigned char *toobin_moslip;



/*************************************
 *
 *		Statics
 *
 *************************************/

static unsigned char *playfielddirty;

static struct osd_bitmap *playfieldbitmap;

static int xscroll, yscroll;

static int last_intensity;



/*************************************
 *
 *		Prototypes from other modules
 *
 *************************************/

void toobin_vh_stop (void);

#if 0
static void toobin_dump_video_ram (void);
#endif


/*************************************
 *
 *		Video system start
 *
 *************************************/

int toobin_vh_start(void)
{
	static struct atarigen_modesc toobin_modesc =
	{
		256,                 /* maximum number of MO's */
		8,                   /* number of bytes per MO entry */
		2,                   /* number of bytes between MO words */
		2,                   /* ignore an entry if this word == 0xffff */
		2, 0, 0xff,          /* link = (data[linkword] >> linkshift) & linkmask */
		0                    /* render in reverse link order */
	};

	/* allocate dirty buffers */
	if (!playfielddirty)
		playfielddirty = malloc (atarigen_playfieldram_size / 4);
	if (!playfielddirty)
	{
		toobin_vh_stop ();
		return 1;
	}
	memset (playfielddirty, 1, atarigen_playfieldram_size / 4);

	/* allocate bitmaps */
	if (!playfieldbitmap)
		playfieldbitmap = osd_new_bitmap (128*8, 64*8, Machine->scrbitmap->depth);
	if (!playfieldbitmap)
	{
		toobin_vh_stop ();
		return 1;
	}

	last_intensity = 0;

	/* initialize the displaylist system */
	return atarigen_init_display_list (&toobin_modesc);
}



/*************************************
 *
 *		Video system shutdown
 *
 *************************************/

void toobin_vh_stop(void)
{
	/* free bitmaps */
	if (playfieldbitmap)
		osd_free_bitmap (playfieldbitmap);
	playfieldbitmap = 0;

	/* free dirty buffers */
	if (playfielddirty)
		free (playfielddirty);
	playfielddirty = 0;
}



/*************************************
 *
 *		Playfield RAM read/write handlers
 *
 *************************************/

int toobin_playfieldram_r (int offset)
{
	return READ_WORD (&atarigen_playfieldram[offset]);
}


void toobin_playfieldram_w (int offset, int data)
{
	int oldword = READ_WORD (&atarigen_playfieldram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&atarigen_playfieldram[offset], newword);
		playfielddirty[offset / 4] = 1;
	}
}



/*************************************
 *
 *		Palette RAM read/write handlers
 *
 *************************************/

void toobin_paletteram_w (int offset, int data)
{
	int oldword = READ_WORD (&paletteram[offset]);
	int newword = COMBINE_WORD (oldword, data);
	WRITE_WORD (&paletteram[offset], newword);

	{
		int red =   (((newword >> 10) & 31) * 224) >> 5;
		int green = (((newword >>  5) & 31) * 224) >> 5;
		int blue =  (((newword      ) & 31) * 224) >> 5;

		if (red) red += 38;
		if (green) green += 38;
		if (blue) blue += 38;

		if (!(newword & 0x8000))
		{
			red = (red * last_intensity) >> 5;
			green = (green * last_intensity) >> 5;
			blue = (blue * last_intensity) >> 5;
		}

		palette_change_color ((offset / 2) & 0x3ff, red, green, blue);
	}
}



/*************************************
 *
 *		Motion object list handlers
 *
 *************************************/

void toobin_update_display_list (int scanline)
{
	int link = READ_WORD (&toobin_moslip[0]) & 0xff;
	atarigen_update_display_list (atarigen_spriteram, link, scanline);
}


void toobin_moslip_w (int offset, int data)
{
	COMBINE_WORD_MEM (&toobin_moslip[offset], data);
	toobin_update_display_list (cpu_getscanline ());
}


/*---------------------------------------------------------------------------------
 *
 * 	Motion Object encoding
 *
 *		4 16-bit words are used
 *
 *		Word 1:
 *
 *			Bits 0-2   = width of the sprite / 16 (ranges from 1-8)
 *			Bits 3-5   = height of the sprite / 16 (ranges from 1-8)
 *			Bits 6-14  = Y position of the sprite
 *			Bit  15    = absolute/relative positioning (1 = absolute)
 *
 *		Word 2:
 *
 *			Bits 0-13  = index of the image (0-16383)
 *			Bit  14    = horizontal flip
 *			Bit  15    = vertical flip
 *
 *		Word 3:
 *
 *			Bits 0-7   = link to the next image to display
 *			Bits 12-15 = priority (only upper 2 bits used)
 *
 *		Word 4:
 *
 *			Bits 0-3   = sprite color
 *			Bits 6-15  = X position of the sprite
 *
 *---------------------------------------------------------------------------------
 */

void toobin_calc_mo_colors (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	unsigned short *colors = param;
	int color = data[3] & 0x0f;
	int hsize = (data[0] & 7) + 1;
	int vsize = ((data[0] >> 3) & 7) + 1;
	int pict = data[1] & 0x3fff;
	int i;

	colors += color;
	for (i = hsize * vsize - 1; i >= 0; i--, pict++)
		*colors |= Machine->gfx[2]->pen_usage[pict];
}

void toobin_render_mo (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	struct toobin_mo_data *modata = param;
	int *redraw_list = modata->redraw_list;
	int *redraw = modata->redraw;
	int *r, redraw_val;

	/* extract data from the various words */
	int xpos = data[3] >> 6;
	int ypos = -(data[0] >> 6);
	int hsize = (data[0] & 7) + 1;
	int vsize = ((data[0] >> 3) & 7) + 1;
	int hflip = data[1] & 0x4000;
	int vflip = data[1] & 0x8000;
	int pict = data[1] & 0x3fff;
	int color = data[3] & 0x0f;
	int absolute = data[0] & 0x8000;
	int priority = 3;
	int x, y, sx, sy, xadv, yadv;

	/* adjust position if relative */
	if (!absolute)
	{
		xpos += xscroll;
		ypos += yscroll;
	}

	/* adjust for h flip */
	if (hflip)
		xpos += (hsize - 1) * 16, xadv = -16;
	else
		xadv = 16;

	/* adjust for vflip */
	if (vflip)
		ypos -= 16, yadv = -16;
	else
		ypos -= vsize * 16, yadv = 16;

	/* adjust the final coordinates */
	xpos &= 0x3ff;
	ypos &= 0x1ff;
	redraw_val = (xpos << 22) + (ypos << 13) + (priority << 10) + (hsize << 4) + vsize;
	if (xpos >= XDIM) xpos -= 0x400;
	if (ypos >= YDIM) ypos -= 0x200;

	/* see if we already have a redraw entry in the list for this MO */
	for (r = redraw_list; r < redraw; )
		if (*r++ == redraw_val)
			break;

	/* if not, add it */
	if (r == redraw)
	{
		*redraw++ = redraw_val;
		modata->redraw = redraw;
	}

	/* loop over the width */
	for (x = 0, sx = xpos; x < hsize; x++, sx += xadv)
	{
		/* clip the X coordinate */
		if (sx <= -16)
		{
			pict += vsize;
			continue;
		}
		else if (sx >= XDIM)
			break;

		/* loop over the height */
		for (y = 0, sy = ypos; y < vsize; y++, sy += yadv, pict++)
		{
			/* clip the Y coordinate */
			if (sy <= clip->min_y - 16 || sy > clip->max_y)
				continue;

			/* draw the sprite */
			drawgfx (bitmap, Machine->gfx[2],
					pict, color, hflip, vflip, sx, sy, clip, TRANSPARENCY_PEN, 0);
		}
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

/*
 *		the real deal
 */

void toobin_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	unsigned char al_map[16], pf_map[16];
	unsigned short mo_map[16];
	int x, y, sx, sy, xoffs, yoffs, offs, i, intensity;
	struct toobin_mo_data modata;
	int redraw_list[1024], *r;


	/* compute the intensity and modify the palette if it's different */
	intensity = 0x1f - (READ_WORD (&toobin_intensity[0]) & 0x1f);
	if (intensity != last_intensity)
	{
		last_intensity = intensity;
		for (i = 0; i < 256+256+64; i++)
		{
			int newword = READ_WORD (&paletteram[i*2]);
			int red =   (((newword >> 10) & 31) * 224) >> 5;
			int green = (((newword >>  5) & 31) * 224) >> 5;
			int blue =  (((newword      ) & 31) * 224) >> 5;

			if (red) red += 38;
			if (green) green += 38;
			if (blue) blue += 38;

			if (!(newword & 0x8000))
			{
				red = (red * last_intensity) >> 5;
				green = (green * last_intensity) >> 5;
				blue = (blue * last_intensity) >> 5;
			}

			palette_change_color (i, red, green, blue);
		}
	}


/*	if (osd_key_pressed (OSD_KEY_9)) toobin_dump_video_ram ();*/


	/* reset color tracking */
	memset (mo_map, 0, sizeof (mo_map));
	memset (pf_map, 0, sizeof (pf_map));
	memset (al_map, 0, sizeof (al_map));
	memset (palette_used_colors, PALETTE_COLOR_UNUSED, Machine->drv->total_colors * sizeof(unsigned char));

	/* update color usage for the playfield */
	for (offs = 0; offs < 128*64; offs++)
	{
		int data1 = READ_WORD (&atarigen_playfieldram[offs * 4]);
		int color = data1 & 15;
		pf_map[color] = 1;
	}

	/* update color usage for the mo's */
	atarigen_render_display_list (bitmap, toobin_calc_mo_colors, mo_map);

	/* update color usage for the alphanumerics */
	for (sy = 0; sy < YCHARS; sy++)
	{
		for (sx = 0, offs = sy * 64; sx < XCHARS; sx++, offs++)
		{
			int data = READ_WORD (&atarigen_alpharam[offs * 2]);
			int color = (data >> 12) & 15;
			al_map[color] = 1;
		}
	}

	/* rebuild the palette */
	for (i = 0; i < 16; i++)
	{
		if (pf_map[i])
			memset (&palette_used_colors[0 + i * 16], PALETTE_COLOR_USED, 16);
		if (mo_map[i])
		{
			int j;
			palette_used_colors[256 + i * 16] = PALETTE_COLOR_TRANSPARENT;
			for (j = 1; j < 16; j++)
				if (mo_map[i] & (1 << j))
					palette_used_colors[256 + i * 16 + j] = PALETTE_COLOR_USED;
		}
		if (al_map[i])
			memset (&palette_used_colors[512 + i * 4], PALETTE_COLOR_USED, 4);
	}

	if (palette_recalc ())
		memset (playfielddirty, 1, atarigen_playfieldram_size / 2);



	/* compute scrolling so we know what to update */
	xscroll = (READ_WORD (&atarigen_hscroll[0]) >> 6);
	yscroll = (READ_WORD (&atarigen_vscroll[0]) >> 6);
	xscroll = -(xscroll & 0x3ff);
	yscroll = -(yscroll & 0x1ff);


	/*
	 *---------------------------------------------------------------------------------
	 *
	 * 	Playfield encoding
	 *
	 *		2 16-bit words are used
	 *
	 *		Word 1:
	 *
	 *			Bits 0-3   = color of the image
	 *			Bits 4-7   = priority of the image
	 *
	 *		Word 2:
	 *
	 *			Bits 0-13  = index of the image
	 *			Bit  14    = horizontal flip
	 *			Bit  15    = vertical flip
	 *
	 *---------------------------------------------------------------------------------
	 */

	/* update only the portion of the playfield that's visible. */
	xoffs = -xscroll / 8;
	yoffs = -yscroll / 8;

	/* loop over the visible Y portion */
	for (y = yoffs + YCHARS + 1; y >= yoffs; y--)
	{
		sy = y & 63;

		/* loop over the visible X portion */
		for (x = xoffs + XCHARS + 1; x >= xoffs; x--)
		{
			/* compute the offset */
			sx = x & 127;
			offs = sy * 128 + sx;

			/* rerender if dirty */
			if (playfielddirty[offs])
			{
				int data1 = READ_WORD (&atarigen_playfieldram[offs * 4]);
				int data2 = READ_WORD (&atarigen_playfieldram[offs * 4 + 2]);
				int color = data1 & 15;
				int vflip = data2 & 0x8000;
				int hflip = data2 & 0x4000;
				int pict = data2 & 0x3fff;

				drawgfx (playfieldbitmap, Machine->gfx[1], pict, color, hflip, vflip,
						8 * sx, 8 * sy, 0, TRANSPARENCY_NONE, 0);
				playfielddirty[offs] = 0;
			}
		}
	}

	/* copy the playfield to the destination */
	copyscrollbitmap (bitmap, playfieldbitmap, 1, &xscroll, 1, &yscroll, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);

	/* prepare the motion object data structure */
	modata.redraw_list = modata.redraw = redraw_list;

	/* render the motion objects */
	atarigen_render_display_list (bitmap, toobin_render_mo, &modata);

	/* redraw playfield tiles with higher priority */
	for (r = redraw_list; r < modata.redraw; r++)
	{
		int val = *r;
		int xpos = (val >> 22) & 0x3ff;
		int ypos = (val >> 13) & 0x1ff;
//		int pri = (val >> 9) & 6;
		int w = (val >> 4) & 15;
		int h = val & 15;
		struct rectangle clip;

		/* wrap */
		if (xpos > XDIM) xpos -= 0x400;
		if (ypos > YDIM) ypos -= 0x200;

		/* make a clip */
		clip.min_x = xpos;
		clip.max_x = xpos + w * 16 - 1;
		clip.min_y = ypos;
		clip.max_y = ypos + h * 16 - 1;

		/* round the positions */
		xpos = (xpos - xscroll) / 8;
		ypos = (ypos - yscroll) / 8;

		/* loop over the columns */
		for (x = xpos + w * 2; x >= xpos; x--)
		{
			/* compute the scroll-adjusted x position */
			sx = (x * 8 + xscroll) & 0x3ff;
			if (sx > 0x3f8) sx -= 0x400;

			/* loop over the rows */
			for (y = ypos + h * 2; y >= ypos; y--)
			{
				int data1, pfpri;

				/* compute the scroll-adjusted y position */
				sy = (y * 8 + yscroll) & 0x1ff;
				if (sy > 0x1f8) sy -= 0x200;

				/* process the data */
				offs = (y & 0x3f) * 128 + (x & 0x7f);
				data1 = READ_WORD (&atarigen_playfieldram[offs * 4]);
				pfpri = (data1 >> 4) & 3;

				/* there is a PAL on the board which does priority; this is wrong, but workable */
				if (pfpri)
				{
					int data2 = READ_WORD (&atarigen_playfieldram[offs * 4 + 2]);
					int vflip = data2 & 0x8000;
					int hflip = data2 & 0x4000;
					int pict = data2 & 0x3fff;
					int color = data1 & 15;

					drawgfx (bitmap, Machine->gfx[1], pict, color, hflip, vflip,
							sx, sy, &clip, TRANSPARENCY_PENS, 0x000000ff);
				}
			}
		}
	}


	/*
	 *---------------------------------------------------------------------------------
	 *
	 * 	Alpha layer encoding
	 *
	 *		1 16-bit word is used
	 *
	 *			Bits 0-10  = index of the character
	 *			Bit  11    = horizontal flip
	 *			Bit  12-15 = color
	 *
	 *---------------------------------------------------------------------------------
	 */

	/* loop over the Y coordinate */
	for (sy = 0; sy < YCHARS; sy++)
	{
		/* loop over the X coordinate */
		for (sx = 0, offs = sy * 64; sx < XCHARS; sx++, offs++)
		{
			int data = READ_WORD (&atarigen_alpharam[offs * 2]);
			int pict = data & 0x3ff;

			/* if there's a non-zero picture or if we're fully opaque, draw the tile */
			if (pict)
			{
				int color = (data >> 12) & 15;
				int hflip = data & 0x400;

				/* draw the character */
				drawgfx (bitmap, Machine->gfx[0], pict, color, hflip, 0,
						8 * sx, 8 * sy, 0, TRANSPARENCY_PEN, 0);
			}
		}
	}
}



/*************************************
 *
 *		Debugging
 *
 *************************************/

#if 0
static void toobin_dump_video_ram (void)
{
	static int count;
	char name[50];
	FILE *f;
	int i;

	while (osd_key_pressed (OSD_KEY_9)) { }

	sprintf (name, "Dump %d", ++count);
	f = fopen (name, "wt");

	fprintf (f, "\n\nPlayfield Palette:\n");
	for (i = 0x000; i < 0x100; i++)
	{
		fprintf (f, "%04X ", READ_WORD (&paletteram[i*2]));
		if ((i & 15) == 15) fprintf (f, "\n");
	}

	fprintf (f, "\n\nMotion Object Palette:\n");
	for (i = 0x100; i < 0x200; i++)
	{
		fprintf (f, "%04X ", READ_WORD (&paletteram[i*2]));
		if ((i & 15) == 15) fprintf (f, "\n");
	}

	fprintf (f, "\n\nAlpha Palette\n");
	for (i = 0x200; i < 0x300; i++)
	{
		fprintf (f, "%04X ", READ_WORD (&paletteram[i*2]));
		if ((i & 15) == 15) fprintf (f, "\n");
	}

	fprintf (f, "\n\nX Scroll = %03X\nY Scroll = %03X\n",
		READ_WORD (&atarigen_hscroll[0]) >> 6, READ_WORD (&atarigen_vscroll[0]) >> 6);

	fprintf (f, "\n\nMotion Objects\n");
	for (i = 0; i < atarigen_spriteram_size; i += 8)
	{
		int data1 = READ_WORD (&atarigen_spriteram[i+0]);
		int data2 = READ_WORD (&atarigen_spriteram[i+2]);
		int data3 = READ_WORD (&atarigen_spriteram[i+4]);
		int data4 = READ_WORD (&atarigen_spriteram[i+6]);

		int ypos = (data1 >> 6) & 0x1ff;
		int vsize = ((data1 >> 3) & 7) + 1;
		int hsize = (data1 & 7) + 1;
		int pict = (data2 & 0x3fff);
		int vflip = ((data2 >> 15) & 1);
		int hflip = ((data2 >> 14) & 1);
		int link = (data3 & 0xff);
		int color = (data4 & 0x0f);
		int xpos = (data4 >> 6) & 0x3ff;

		fprintf (f, "  Object %02X: P=%04X C=%01X X=%03X Y=%03X HSIZ=%d VSIZ=%d HFLP=%d VFLP=%d L=%02X Leftovers: %04X %04X\n",
				i/8, pict, color, xpos, ypos, hsize, vsize, hflip, vflip, link, data3 & 0xff00, data4 & 0x30);
	}

	fprintf (f, "\n\nPlayfield dump\n");
	for (i = 0; i < atarigen_playfieldram_size / 4; i++)
	{
		fprintf (f, "%01X%04X ", READ_WORD (&atarigen_playfieldram[i*4]), READ_WORD (&atarigen_playfieldram[i*4+2]));
		if ((i & 127) == 127) fprintf (f, "\n");
		else if ((i & 127) == 63) fprintf (f, "\n      ");
	}

	fprintf (f, "\n\nAlpha dump\n");
	for (i = 0; i < atarigen_alpharam_size / 2; i++)
	{
		fprintf (f, "%04X ", READ_WORD (&atarigen_alpharam[i*2]));
		if ((i & 63) == 63) fprintf (f, "\n");
	}

	fclose (f);
}
#endif
