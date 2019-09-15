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

static unsigned char trans_count_col[XCHARS];
static unsigned char trans_count_row[YCHARS];

static int xscroll, yscroll;



/*************************************
 *
 *		Prototypes from other modules
 *
 *************************************/

void gauntlet_vh_stop (void);



/*************************************
 *
 *		Video system start
 *
 *************************************/

int gauntlet_vh_start(void)
{
	static struct atarigen_modesc gauntlet_modesc =
	{
		1024,                /* maximum number of MO's */
		2,                   /* number of bytes per MO entry */
		0x800,               /* number of bytes between MO words */
		3,                   /* ignore an entry if this word == 0xffff */
		3, 0, 0x3ff,         /* link = (data[linkword] >> linkshift) & linkmask */
		0                    /* render in reverse link order */
	};

	/* allocate dirty buffers */
	if (!playfielddirty)
		playfielddirty = malloc (atarigen_playfieldram_size / 2);
	if (!playfielddirty)
	{
		gauntlet_vh_stop ();
		return 1;
	}
	memset (playfielddirty, 1, atarigen_playfieldram_size / 2);

	/* allocate bitmaps */
	if (!playfieldbitmap)
		playfieldbitmap = osd_new_bitmap (64*8, 64*8, Machine->scrbitmap->depth);
	if (!playfieldbitmap)
	{
		gauntlet_vh_stop ();
		return 1;
	}

	/* initialize the transparency trackers */
	memset (trans_count_col, YCHARS, sizeof (trans_count_col));
	memset (trans_count_row, XCHARS, sizeof (trans_count_row));

	/* initialize the displaylist system */
	return atarigen_init_display_list (&gauntlet_modesc);
}



/*************************************
 *
 *		Video system shutdown
 *
 *************************************/

void gauntlet_vh_stop(void)
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
 *		Vertical scroll/PF bank
 *
 *************************************/

int gauntlet_vscroll_r (int offset)
{
	return READ_WORD (&atarigen_vscroll[offset]);
}


void gauntlet_vscroll_w (int offset, int data)
{
	int oldword = READ_WORD (&atarigen_vscroll[offset]);
	int newword = COMBINE_WORD (oldword, data);
	WRITE_WORD (&atarigen_vscroll[offset], newword);

	/* invalidate the entire playfield if we're switching ROM banks */
	if (offset == 2 && (oldword & 3) != (newword & 3))
		memset (playfielddirty, 1, atarigen_playfieldram_size / 2);
}



/*************************************
 *
 *		Playfield RAM read/write handlers
 *
 *************************************/

int gauntlet_playfieldram_r (int offset)
{
	return READ_WORD (&atarigen_playfieldram[offset]);
}


void gauntlet_playfieldram_w (int offset, int data)
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
 *		Alpha RAM read/write handlers
 *
 *************************************/

int gauntlet_alpharam_r (int offset)
{
	return READ_WORD (&atarigen_alpharam[offset]);
}


void gauntlet_alpharam_w (int offset, int data)
{
	int oldword = READ_WORD (&atarigen_alpharam[offset]);
	int newword = COMBINE_WORD (oldword, data);
	WRITE_WORD (&atarigen_alpharam[offset], newword);

	/* track opacity of rows & columns */
	if ((oldword ^ newword) & 0x8000)
	{
		int sx,sy;

		sx = (offset/2) % 64;
		sy = (offset/2) / 64;

		if (sx < XCHARS && sy < YCHARS)
		{
			if (newword & 0x8000)
				trans_count_col[sx]--, trans_count_row[sy]--;
			else
				trans_count_col[sx]++, trans_count_row[sy]++;
		}
	}
}



/*************************************
 *
 *		Motion object list handlers
 *
 *************************************/

int gauntlet_update_display_list (int scanline)
{
	/* look up the SLIP link */
	int scrolly = (READ_WORD (&atarigen_vscroll[2]) >> 7) & 0x1ff;

	int link = READ_WORD (&atarigen_alpharam[0xf80 + 2 * (((scanline + scrolly) / 8) & 0x3f)]) & 0x3ff;

	atarigen_update_display_list (atarigen_spriteram, link, scanline);

	return scrolly;
}


/*---------------------------------------------------------------------------------
 *
 * 	Motion Object encoding
 *
 *		4 16-bit words are used
 *
 *		Word 1:
 *
 *			Bits 0-14  = index of the image (0-32767)
 *
 *		Word 2:
 *
 *			Bits 0-3   = sprite color
 *			Bits 7-15  = X position of the sprite
 *
 *		Word 3:
 *
 *			Bits 0-2   = height of the sprite / 8 (ranges from 1-8)
 *			Bits 3-5   = width of the sprite / 8 (ranges from 1-8)
 *			Bit  6     = horizontal flip
 *			Bits 7-14  = Y position of the sprite
 *
 *		Word 4:
 *
 *			Bits 0-9   = link to the next image to display
 *
 *---------------------------------------------------------------------------------
 */

void gauntlet_calc_mo_colors (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	unsigned char *colors = param;
	int color = data[1] & 0x0f;
	colors[color] = 1;
}

void gauntlet_render_mo (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	int sx, sy, x, y, xadv;

	/* extract data from the various words */
	int pict = data[0] & 0x7fff;
	int color = data[1] & 0x0f;
	int xpos = xscroll + (data[1] >> 7);
	int vsize = (data[2] & 7) + 1;
	int hsize = ((data[2] >> 3) & 7) + 1;
	int hflip = data[2] & 0x40;
	int ypos = yscroll - (data[2] >> 7) - vsize * 8;

	/* adjust for h flip */
	if (hflip)
		xpos += (hsize - 1) * 8, xadv = -8;
	else
		xadv = 8;

	/* adjust the final coordinates */
	xpos &= 0x1ff;
	ypos &= 0x1ff;
	if (xpos >= XDIM) xpos -= 0x200;
	if (ypos >= YDIM) ypos -= 0x200;

	/* loop over the height */
	for (y = 0, sy = ypos; y < vsize; y++, sy += 8)
	{
		/* clip the Y coordinate */
		if (sy <= clip->min_y - 8)
		{
			pict += hsize;
			continue;
		}
		else if (sy > clip->max_y)
			break;

		/* loop over the width */
		for (x = 0, sx = xpos; x < hsize; x++, sx += xadv, pict++)
		{
			/* clip the X coordinate */
			if (sx <= -8 || sx >= XDIM)
				continue;

			/* draw the sprite */
			drawgfx (bitmap, Machine->gfx[1],
					pict ^ 0x800, color, hflip, 0, sx, sy, clip, TRANSPARENCY_PEN, 0);
		}
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void gauntlet_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	unsigned char mo_map[16], al_map[32], pf_map[16];
	int x, y, sx, sy, xoffs, yoffs, i, offs, bank;
	struct rectangle clip;



	/* reset color tracking */
	memset (mo_map, 0, sizeof (mo_map));
	memset (pf_map, 0, sizeof (pf_map));
	memset (al_map, 0, sizeof (al_map));
	memset (palette_used_colors, PALETTE_COLOR_UNUSED, Machine->drv->total_colors * sizeof(unsigned char));

	/* update color usage for the playfield */
	for (offs = 0; offs < 64*64; offs++)
	{
		int data = READ_WORD (&atarigen_playfieldram[offs * 2]);
		int color = ((data >> 12) & 7) + 8;
		pf_map[color] = 1;
	}

	/* update color usage for the mo's */
	atarigen_render_display_list (bitmap, gauntlet_calc_mo_colors, mo_map);

	/* update color usage for the alphanumerics */
	for (sy = 0; sy < YCHARS; sy++)
	{
		for (sx = 0, offs = sy * 64; sx < XCHARS; sx++, offs++)
		{
			int data = READ_WORD (&atarigen_alpharam[offs * 2]);
			int color = (data >> 10) & 0x1f;
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
		if (al_map[16+i])
			memset (&palette_used_colors[0 + (i+32) * 4], PALETTE_COLOR_USED, 4);
	}

	if (palette_recalc ())
		memset (playfielddirty, 1, atarigen_playfieldram_size / 2);



	/* clip out any rows and columns that are completely covered by characters */
	for (x = 0; x < XCHARS; x++)
		if (trans_count_col[x]) break;
	clip.min_x = x*8;

	for (x = XCHARS-1; x > 0; x--)
		if (trans_count_col[x]) break;
	clip.max_x = x*8+7;

	for (y = 0; y < YCHARS; y++)
		if (trans_count_row[y]) break;
	clip.min_y = y*8;

	for (y = YCHARS-1; y > 0; y--)
		if (trans_count_row[y]) break;
	clip.max_y = y*8+7;

	/* compute scrolling so we know what to update */
	xscroll = READ_WORD (&atarigen_hscroll[0]);
	yscroll = READ_WORD (&atarigen_vscroll[2]);
	xscroll = -(xscroll & 0x1ff);
	yscroll = -((yscroll >> 7) & 0x1ff);

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

	/* update only the portion of the playfield that's visible. */
	bank = (READ_WORD (&atarigen_vscroll[2]) & 3) << 12;
	xoffs = (-xscroll / 8) + (clip.min_x / 8);
	yoffs = (-yscroll / 8) + (clip.min_y / 8);

	/* loop over the visible Y region */
	for (y = yoffs + (clip.max_y - clip.min_y) / 8 + 1; y >= yoffs; y--)
	{
		sy = y & 63;

		/* loop over the visible X region */
		for (x = xoffs + clip.max_x/8 + 1; x >= xoffs; x--)
		{
			/* compute the offset */
			sx = x & 63;
			offs = sx * 64 + sy;

			/* rerender if dirty */
			if (playfielddirty[offs])
			{
				int data = READ_WORD (&atarigen_playfieldram[offs * 2]);
				int color = ((data >> 12) & 7) + 8;
				int hflip = (data >> 15) & 1;
				int pict = bank + (data & 0xfff);

				drawgfx (playfieldbitmap, Machine->gfx[1], pict ^ 0x800, color + 0x10, hflip, 0,
						8 * sx, 8 * sy, 0, TRANSPARENCY_NONE, 0);
				playfielddirty[offs] = 0;
			}
		}
	}

	/* copy the playfield to the destination */
	copyscrollbitmap (bitmap, playfieldbitmap, 1, &xscroll, 1, &yscroll, &clip, TRANSPARENCY_NONE, 0);

	/* render the motion objects */
	atarigen_render_display_list (bitmap, gauntlet_render_mo, NULL);

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
				int color = ((data >> 10) & 0xf) | ((data >> 9) & 0x20);

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
