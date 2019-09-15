/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"

#define XCHARS 42
#define YCHARS 30

#define XDIM (XCHARS*8)
#define YDIM (YCHARS*8)



/*************************************
 *
 *		Statics
 *
 *************************************/

static unsigned char *playfielddirty;

static struct osd_bitmap *playfieldbitmap;



/*************************************
 *
 *		Prototypes from other modules
 *
 *************************************/

void xybots_vh_stop (void);



/*************************************
 *
 *		Video system start
 *
 *************************************/

int xybots_vh_start(void)
{
	static struct atarigen_modesc xybots_modesc =
	{
		64,                  /* maximum number of MO's */
		8,                   /* number of bytes per MO entry */
		2,                   /* number of bytes between MO words */
		0,                   /* ignore an entry if this word == 0xffff */
		-1, 0, 0x3f,         /* link = (data[linkword] >> linkshift) & linkmask */
		0                    /* render in reverse link order */
	};

	/* allocate dirty buffers */
	if (!playfielddirty)
		playfielddirty = malloc (atarigen_playfieldram_size / 2);
	if (!playfielddirty)
	{
		xybots_vh_stop ();
		return 1;
	}
	memset (playfielddirty, 1, atarigen_playfieldram_size / 2);

	/* allocate bitmaps */
	if (!playfieldbitmap)
		playfieldbitmap = osd_new_bitmap (XDIM, YDIM, Machine->scrbitmap->depth);
	if (!playfieldbitmap)
	{
		xybots_vh_stop ();
		return 1;
	}

	/* initialize the displaylist system */
	return atarigen_init_display_list (&xybots_modesc);
}



/*************************************
 *
 *		Video system shutdown
 *
 *************************************/

void xybots_vh_stop(void)
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

int xybots_playfieldram_r (int offset)
{
	return READ_WORD (&atarigen_playfieldram[offset]);
}


void xybots_playfieldram_w (int offset, int data)
{
	int oldword = READ_WORD (&atarigen_playfieldram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&atarigen_playfieldram[offset], newword);
		playfielddirty[offset / 2] = 1;
	}
}



/*************************************
 *
 *		Motion object list handlers
 *
 *************************************/

void xybots_update_display_list (int scanline)
{
	atarigen_update_display_list (atarigen_spriteram, 0, scanline);
}


/*---------------------------------------------------------------------------------
 *
 * 	Motion Object encoding
 *
 *		4 16-bit words are used
 *
 *		Word 1:
 *
 *			Bits 0-13  = index of the image (0-16384)
 *			Bit 15     = horizontal flip
 *
 *		Word 2:
 *
 *			Bits 0-3   = priority
 *
 *		Word 3:
 *
 *			Bits 0-2   = height of the sprite / 8 (ranges from 1-8)
 *			Bits 7-14  = Y position of the sprite
 *
 *		Word 4:
 *
 *			Bits 0-3   = image palette
 *			Bits 7-14  = X position of the sprite
 *
 *---------------------------------------------------------------------------------
 */

void xybots_calc_mo_colors (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	unsigned char *colors = param;
	int color = data[3] & 7;
	colors[color] = 1;
}

void xybots_render_mo (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	struct rectangle pfclip;
	int sx, sy, x, y;

	/* extract data from the various words */
	int pict = data[0] & 0x3fff;
	int hflip = data[0] & 0x8000;
	int color = data[3] & 7;
	int xpos = data[3] >> 7;
	int vsize = (data[2] & 7) + 1;
	int ypos = -(data[2] >> 7) - vsize * 8;
	int pri = ~data[1] & 15;

	/* adjust the final coordinates */
	xpos &= 0x1ff;
	ypos &= 0x1ff;
	if (xpos >= XDIM) xpos -= 0x200;
	if (ypos >= YDIM) ypos -= 0x200;

	/* clip the X coordinate */
	if (xpos <= -8 || xpos >= XDIM)
		return;

	/* loop over the height */
	for (y = 0, sy = ypos; y < vsize; y++, sy += 8, pict++)
	{
		/* clip the Y coordinate */
		if (sy <= clip->min_y - 8)
			continue;
		else if (sy > clip->max_y)
			break;

		/* draw the sprite */
		drawgfx (bitmap, Machine->gfx[2], pict, color, hflip, 0, xpos, sy, clip, TRANSPARENCY_PEN, 0);
	}

	/* bring the clip in */
	pfclip.min_x = xpos;
	pfclip.max_x = xpos + 7;
	pfclip.min_y = (ypos > clip->min_y) ? ypos : clip->min_y;
	pfclip.max_y = (ypos + vsize * 8 < clip->max_y) ? ypos + vsize * 8 - 1 : clip->min_y;

	/* round the positions */
	xpos /= 8;
	ypos /= 8;

	/* loop over the columns */
	for (x = xpos + 1; x >= xpos; x--)
	{
		/* compute the scroll-adjusted x position */
		sx = (x * 8) & 0x1ff;
		if (sx > 0x1f8) sx -= 0x200;

		/* loop over the rows */
		for (y = ypos + vsize; y >= ypos; y--)
		{
			int offs;
			int dat;

			/* compute the scroll-adjusted y position */
			sy = (y * 8) & 0x1ff;
			if (sy > 0x1f8) sy -= 0x200;

			/* process the data */
			offs = (y & 0x3f) * 64 + (x & 0x3f);
			dat = READ_WORD (&atarigen_playfieldram[offs * 2]);
			color = (dat >> 11) & 15;

			/* this is the priority equation from the schematics */
			if (pri > color)
			{
				hflip = dat & 0x8000;
				drawgfx (bitmap, Machine->gfx[1], dat & 0x1fff, color, hflip, 0,
						sx, sy, &pfclip, TRANSPARENCY_NONE, 0);
			}
		}
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void xybots_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	unsigned char mo_map[16], al_map[8], pf_map[16];
	int x, y, sx, sy, i, offs;

if (osd_key_pressed (OSD_KEY_9))
{
	FILE *f = fopen ("xybots.log", "w");
	while (osd_key_pressed (OSD_KEY_9)) { }
	for (i = 0; i < 0x200; i += 2)
	{
		fprintf (f, "%04X ", READ_WORD (&atarigen_spriteram[i]));
		if ((i & 0x3e) == 0x3e) fprintf (f, "\n");
	}
	fclose (f);
}


	/* reset color tracking */
	memset (mo_map, 0, sizeof (mo_map));
	memset (pf_map, 0, sizeof (pf_map));
	memset (al_map, 0, sizeof (al_map));
	memset (palette_used_colors, PALETTE_COLOR_UNUSED, Machine->drv->total_colors * sizeof(unsigned char));

	/* update color usage for the playfield */
	for (y = 0; y < YCHARS; y++)
	{
		for (x = 0; x < XCHARS; x++)
		{
			int data, color;

			/* read the data word */
			offs = y * 64 + x;
			data = READ_WORD (&atarigen_playfieldram[offs * 2]);

			/* update color statistics */
			color = (data >> 11) & 15;
			pf_map[color] = 1;
		}
	}

	/* update color usage for the mo's */
	atarigen_render_display_list (bitmap, xybots_calc_mo_colors, mo_map);

	/* update color usage for the alphanumerics */
	for (sy = 0; sy < YCHARS; sy++)
	{
		for (sx = 0, offs = sy * 64; sx < XCHARS; sx++, offs++)
		{
			int data = READ_WORD (&atarigen_alpharam[offs * 2]);
			int color = (data >> 12) & 7;
			al_map[color] = 1;
		}
	}

	/* rebuild the palette */
	for (i = 0; i < 16; i++)
	{
		if (pf_map[i])
			memset (&palette_used_colors[512 + i * 16], PALETTE_COLOR_USED, 16);
		if (mo_map[i])
		{
			palette_used_colors[256 + i * 16] = PALETTE_COLOR_TRANSPARENT;
			memset (&palette_used_colors[256 + i * 16 + 1], PALETTE_COLOR_USED, 15);
		}
		if (al_map[i])
			memset (&palette_used_colors[0 + i * 4], PALETTE_COLOR_USED, 4);
	}

	if (palette_recalc ())
		memset (playfielddirty, 1, atarigen_playfieldram_size / 2);



	/*
	 *---------------------------------------------------------------------------------
	 *
	 * 	Playfield encoding
	 *
	 *		1 16-bit word is used
	 *
	 *			Bits 0-11  = image
	 *			Bits 12-14 = color
	 *			Bit  15    = horizontal flip
	 *
	 *---------------------------------------------------------------------------------
	 */

	/* loop over the visible Y region */
	for (y = 0; y < YCHARS; y++)
	{
		/* loop over the visible X region */
		for (x = 0; x < XCHARS; x++)
		{
			offs = y * 64 + x;

			/* rerender if dirty */
			if (playfielddirty[offs])
			{
				int data = READ_WORD (&atarigen_playfieldram[offs * 2]);
				int color = (data >> 11) & 15;
				int hflip = data & 0x8000;
				int pict = data & 0x1fff;

				drawgfx (playfieldbitmap, Machine->gfx[1], pict, color, hflip, 0,
						8 * x, 8 * y, 0, TRANSPARENCY_NONE, 0);
				playfielddirty[offs] = 0;
			}
		}
	}

	/* copy the playfield to the destination */
	copybitmap (bitmap, playfieldbitmap, 0, 0, 0, 0, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);

	/* render the motion objects */
	atarigen_render_display_list (bitmap, xybots_render_mo, NULL);

	/*
	 *---------------------------------------------------------------------------------
	 *
	 * 	Alpha layer encoding
	 *
	 *		1 16-bit word is used
	 *
	 *			Bits 0-9   = index of the character
	 *			Bit  10-13 = color
	 *			Bit  15    = transparent/opaque
	 *
	 *---------------------------------------------------------------------------------
	 */

	/* redraw the alpha layer completely */
	for (sy = 0; sy < YCHARS; sy++)
	{
		for (sx = 0, offs = sy * 64; sx < XCHARS; sx++, offs++)
		{
			int data = READ_WORD (&atarigen_alpharam[offs * 2]);
			int pict = (data & 0x3ff);

			if (pict || (data & 0x8000))
			{
				int color = (data >> 12) & 7;

				drawgfx (bitmap, Machine->gfx[0],
						pict, color,
						0, 0,
						8 * sx, 8 * sy,
						0,
						(data & 0x8000) ? TRANSPARENCY_NONE : TRANSPARENCY_PEN, 0);
			}
		}
	}
}
