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


struct vindictr_mo_data
{
	int *redraw_list, *redraw;
};



/*************************************
 *
 *		Statics
 *
 *************************************/

static unsigned char *playfielddirty;

static struct osd_bitmap *playfieldbitmap;

static int xscroll, yscroll;
static int last_pfbank, pfbank;



/*************************************
 *
 *		Prototypes from other modules
 *
 *************************************/

void vindictr_vh_stop (void);

#if 0
static void vindictr_debug (void);
#endif



/*************************************
 *
 *		Video system start
 *
 *************************************/

int vindictr_vh_start(void)
{
	static struct atarigen_modesc vindictr_modesc =
	{
		1024,                /* maximum number of MO's */
		8,                   /* number of bytes per MO entry */
		2,                   /* number of bytes between MO words */
		0,                   /* ignore an entry if this word == 0xffff */
		3, 0, 0x3ff,         /* link = (data[linkword] >> linkshift) & linkmask */
		0                    /* reverse order */
	};

	/* allocate dirty buffers */
	if (!playfielddirty)
		playfielddirty = malloc (atarigen_playfieldram_size / 2);
	if (!playfielddirty)
	{
		vindictr_vh_stop ();
		return 1;
	}
	memset (playfielddirty, 1, atarigen_playfieldram_size / 2);
	last_pfbank = 0;

	/* allocate bitmaps */
	if (!playfieldbitmap)
		playfieldbitmap = osd_new_bitmap (64*8, 64*8, Machine->scrbitmap->depth);
	if (!playfieldbitmap)
	{
		vindictr_vh_stop ();
		return 1;
	}

	/* initialize the displaylist system */
	return atarigen_init_display_list (&vindictr_modesc);
}



/*************************************
 *
 *		Video system shutdown
 *
 *************************************/

void vindictr_vh_stop (void)
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
 *		Latch write handler
 *
 *************************************/

void vindictr_latch_w (int offset, int data)
{
}



/*************************************
 *
 *		Playfield RAM read/write handlers
 *
 *************************************/

int vindictr_playfieldram_r (int offset)
{
	return READ_WORD (&atarigen_playfieldram[offset]);
}


void vindictr_playfieldram_w (int offset, int data)
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
 *		Sprite RAM read/write handlers
 *
 *************************************/

int vindictr_spriteram_r (int offset)
{
	return READ_WORD (&atarigen_spriteram[offset]);
}


void vindictr_spriteram_w (int offset, int data)
{
	COMBINE_WORD_MEM (&atarigen_spriteram[offset], data);
}



/*************************************
 *
 *		Alpha RAM read/write handlers
 *
 *************************************/

int vindictr_alpharam_r (int offset)
{
	return READ_WORD (&atarigen_alpharam[offset]);
}


void vindictr_alpharam_w (int offset, int data)
{
	COMBINE_WORD_MEM (&atarigen_alpharam[offset], data);
}



/*************************************
 *
 *		Motion object list handlers
 *
 *************************************/

int vindictr_update_display_list (int scanline)
{
	int link, i;

	/* on the first update of a frame, grab the scroll values and pf bank */
	if (scanline == 0)
	{
		xscroll = yscroll = pfbank = -1;
		for (i = 0xed8; i < 0xfff && (xscroll < 0 || yscroll < 0 || pfbank < 0); i += 2)
		{
			int val = READ_WORD (&atarigen_alpharam[i]);
			int flag = val & 0x7e00;
			if (flag == 0x7e00)
				yscroll = val & 0x1ff;
			else if (flag == 0x7600)
				xscroll = val & 0x1ff;
			else if (flag == 0x7400)
				pfbank = val & 0x00f;
		}
	}

	/* look up the SLIP link */
	link = READ_WORD (&atarigen_alpharam[0xf80 + 2 * (((scanline + yscroll) / 8) & 0x3f)]) & 0x3ff;
	atarigen_update_display_list (atarigen_spriteram, link, scanline);

	return yscroll;
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

void vindictr_calc_mo_colors (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	unsigned char *colors = param;
	int color = data[1] & 0x0f;
	colors[color] = 1;
}

void vindictr_render_mo (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
//	struct vindictr_mo_data *modata = param;
//	int *redraw_list = modata->redraw_list;
//	int *redraw = modata->redraw;
	int xadv, x, y, sx, sy;

	/* extract data from the various words */
	int pict = data[0] & 0x7fff;
	int hsize = ((data[2] >> 3) & 7) + 1;
	int vsize = (data[2] & 7) + 1;
	int xpos = xscroll + (data[1] >> 7);
	int ypos = yscroll - (data[2] >> 7) - vsize * 8;
	int color = data[1] & 15;
	int hflip = data[2] & 0x0040;

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
			drawgfx (bitmap, Machine->gfx[1], pict, color, hflip, 0,
						sx, sy, clip, TRANSPARENCY_PEN, 0);
		}
	}
}


static void draw_playfield_chunk (struct osd_bitmap *bitmap, struct rectangle *clip, int bank)
{
	int w = (clip->max_x - clip->min_x + 1) / 8;
	int h = (clip->max_y - clip->min_y + 1) / 8;
	int x, y, sx, xpos, ypos;

	/* wrap */
	if (clip->min_x > XDIM) clip->min_x -= 0x200, clip->max_x -= 0x200;
	if (clip->min_y > YDIM) clip->min_y -= 0x200, clip->max_y -= 0x200;

	/* round the positions */
	xpos = (clip->min_x - xscroll) / 8;
	ypos = (clip->min_y - yscroll) / 8;

	/* loop over the columns */
	for (x = xpos + w; x >= xpos; x--)
	{
		/* compute the scroll-adjusted x position */
		sx = (x * 8 + xscroll) & 0x1ff;
		if (sx > 0x1f8) sx -= 0x200;

		/* loop over the rows */
		for (y = ypos + h; y >= ypos; y--)
		{
			int sy, offs, data, color, hflip;

			/* compute the scroll-adjusted y position */
			sy = (y * 8 + yscroll) & 0x1ff;
			if (sy > 0x1f8) sy -= 0x200;

			/* process the data */
			offs = (x & 0x3f) * 64 + (y & 0x3f);
			data = READ_WORD (&atarigen_playfieldram[offs * 2]);
			color = (data >> 11) & 14;

			hflip = data & 0x8000;
			drawgfx (bitmap, Machine->gfx[1], bank + (data & 0xfff), 0x10 + color, hflip, 0,
					sx, sy, clip, TRANSPARENCY_NONE, 0);
		}
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void vindictr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	unsigned char mo_map[16], al_map[32], pf_map[16];
	int x, y, sx, sy, offs, xoffs, yoffs, i;
	int redraw_list[1024];
	struct vindictr_mo_data modata;

#if 0
	vindictr_debug ();
#endif


	xscroll = -xscroll;
	yscroll = -yscroll;

	/* reset color tracking */
	memset (mo_map, 0, sizeof (mo_map));
	memset (pf_map, 0, sizeof (pf_map));
	memset (al_map, 0, sizeof (al_map));
	memset (palette_used_colors, PALETTE_COLOR_UNUSED, Machine->drv->total_colors * sizeof(unsigned char));

	/* update color usage for the playfield */
	for (offs = 0; offs < 64*64; offs++)
	{
		int data = READ_WORD (&atarigen_playfieldram[offs * 2]);
		int color = (data >> 11) & 14;
		pf_map[color] = 1;
	}

	/* update color usage for the mo's */
	atarigen_render_display_list (bitmap, vindictr_calc_mo_colors, mo_map);

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


	/*
	 *---------------------------------------------------------------------------------
	 *
	 * 	Playfield encoding
	 *
	 *		1 16-bit word is used
	 *
	 *			Bits 0-12  = image number
	 *			Bits 13-15 = palette
	 *
	 *---------------------------------------------------------------------------------
	 */

	/* make sure we have a valid bank */
	if (pfbank < 0) pfbank = last_pfbank;
	pfbank = (pfbank & 7) * 0x1000;

	/* if it's different than last frame, we need to refresh the whole playfield */
	if (pfbank != last_pfbank)
		memset (playfielddirty, 1, atarigen_playfieldram_size / 2);
	last_pfbank = pfbank;

	/* update only the portion of the playfield that's visible. */
	xoffs = (-xscroll / 8);
	yoffs = (-yscroll / 8);

	/* loop over the visible Y region */
	for (y = yoffs + YCHARS; y >= yoffs; y--)
	{
		sy = y & 63;

		/* loop over the visible X region */
		for (x = xoffs + XCHARS; x >= xoffs; x--)
		{
			/* read the data word */
			sx = x & 63;
			offs = sx * 64 + sy;

			/* rerender if dirty */
			if (playfielddirty[offs])
			{
				int data = READ_WORD (&atarigen_playfieldram[offs * 2]);
				int hflip = data & 0x8000;
				int color = (data >> 11) & 14;

				drawgfx (playfieldbitmap, Machine->gfx[1], pfbank + (data & 0x0fff), 0x10 + color, hflip, 0,
						8 * sx, 8 * sy, 0, TRANSPARENCY_NONE, 0);
				playfielddirty[offs] = 0;
			}
		}
	}

	/* copy the playfield to the destination */
	{
		struct rectangle clip;
		int curbank = pfbank;

		clip.min_x = 0;
		clip.max_x = XCHARS * 8 - 1;
		clip.min_y = 0;
		for (y = 0; y < YCHARS; y++)
		{
			offs = y * 64 + XCHARS;
			for (x = XCHARS; x < 64; x++, offs++)
			{
				int data = READ_WORD (&atarigen_alpharam[offs * 2]);
				if ((data & 0x7e00) == 0x7400 && ((data & 7) * 0x1000) != curbank)
				{
					clip.max_y = (y + 1) * 8 - 1;
					if (curbank == pfbank)
						copyscrollbitmap (bitmap, playfieldbitmap, 1, &xscroll, 1, &yscroll,
								&clip, TRANSPARENCY_NONE, 0);
					else
						draw_playfield_chunk (bitmap, &clip, curbank);
					clip.min_y = clip.max_y + 1;
					curbank = (data & 7) * 0x1000;
					break;
				}
			}
		}
		if (clip.min_y != YCHARS * 8)
		{
			clip.max_y = YCHARS * 8 - 1;
			if (curbank == pfbank)
				copyscrollbitmap (bitmap, playfieldbitmap, 1, &xscroll, 1, &yscroll,
						&clip, TRANSPARENCY_NONE, 0);
			else
				draw_playfield_chunk (bitmap, &clip, curbank);
		}
	}

	/* prepare the motion object data structure */
	modata.redraw_list = modata.redraw = redraw_list;

	/* render the motion objects */
	atarigen_render_display_list (bitmap, vindictr_render_mo, &modata);

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

				drawgfx (bitmap, Machine->gfx[0], pict, color, 0, 0,
						8 * sx, 8 * sy, 0, (data & 0x8000) ? TRANSPARENCY_NONE : TRANSPARENCY_PEN, 0);
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
static void vindictr_debug (void)
{
	if (osd_key_pressed (OSD_KEY_9))
	{
		static int count;
		char name[50];
		FILE *f;
		int i;

		while (osd_key_pressed (OSD_KEY_9)) { }

		sprintf (name, "Dump %d", ++count);
		f = fopen (name, "wt");

		fprintf (f, "\n\nAlpha Palette:\n");
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

		fprintf (f, "\n\nPlayfield Palette:\n");
		for (i = 0x200; i < 0x400; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&paletteram[i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		fprintf (f, "\n\nMotion Object Config:\n");
		for (i = 0x00; i < 0x40; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_alpharam[0xf00 + i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		fprintf (f, "\n\nMotion Object SLIPs:\n");
		for (i = 0x00; i < 0x40; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_alpharam[0xf80 + i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		fprintf (f, "\n\nMotion Objects\n");
		for (i = 0; i < 0x400; i++)
		{
			unsigned short *data = (unsigned short *)&atarigen_spriteram[i*8];
			int pict = data[1] & 0x7fff;
			int hsize = ((data[3] >> 4) & 7) + 1;
			int vsize = (data[3] & 7) + 1;
			int xpos = xscroll + (data[2] >> 7);
			int ypos = yscroll - (data[3] >> 7) - vsize * 8;
			int color = data[2] & 15;
			int hflip = data[3] & 0x0008;
			fprintf (f, "   Object %03X: L=%03X P=%04X C=%X X=%03X Y=%03X W=%d H=%d F=%d LEFT=(%04X %04X %04X %04X)\n",
					i, data[0] & 0x3ff, pict, color, xpos & 0x1ff, ypos & 0x1ff, hsize, vsize, hflip,
					data[0], data[1], data[2], data[3]);
		}

		fprintf (f, "\n\nPlayfield dump\n");
		for (i = 0; i < atarigen_playfieldram_size / 2; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_playfieldram[i*2]));
			if ((i & 63) == 63) fprintf (f, "\n");
		}

		fprintf (f, "\n\nAlpha dump\n");
		for (i = 0; i < atarigen_alpharam_size / 2; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_alpharam[i*2]));
			if ((i & 63) == 63) fprintf (f, "\n");
		}

		fclose (f);
	}
}
#endif
