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
 *		Globals we own
 *
 *************************************/

unsigned char *eprom_playfieldpalram;

int eprom_playfieldpalram_size;


/*************************************
 *
 *		Statics
 *
 *************************************/

static unsigned char *playfielddirty;

static struct osd_bitmap *playfieldbitmap;

static int xscroll, yscroll;



/*************************************
 *
 *		Prototypes from other modules
 *
 *************************************/

void eprom_vh_stop (void);

static void redraw_playfield_chunk (struct osd_bitmap *bitmap, int xpos, int ypos, int w, int h, int pri);
static void eprom_debug (void);



/*************************************
 *
 *		Video system start
 *
 *************************************/

int eprom_vh_start(void)
{
	static struct atarigen_modesc eprom_modesc =
	{
		1024,                /* maximum number of MO's */
		8,                   /* number of bytes per MO entry */
		2,                   /* number of bytes between MO words */
		0,                   /* ignore an entry if this word == 0xffff */
		0, 0, 0x3ff,         /* link = (data[linkword] >> linkshift) & linkmask */
		1                    /* reverse order */
	};

	/* allocate dirty buffers */
	if (!playfielddirty)
		playfielddirty = malloc (atarigen_playfieldram_size / 2);
	if (!playfielddirty)
	{
		eprom_vh_stop ();
		return 1;
	}
	memset (playfielddirty, 1, atarigen_playfieldram_size / 2);

	/* allocate bitmaps */
	if (!playfieldbitmap)
		playfieldbitmap = osd_new_bitmap (64*8, 64*8, Machine->scrbitmap->depth);
	if (!playfieldbitmap)
	{
		eprom_vh_stop ();
		return 1;
	}

	/* initialize the displaylist system */
	return atarigen_init_display_list (&eprom_modesc);
}



/*************************************
 *
 *		Video system shutdown
 *
 *************************************/

void eprom_vh_stop (void)
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

void eprom_latch_w (int offset, int data)
{
	/* reset extra CPU */
	if (!(data & 0x00ff0000))
	{
		if (!(data & 1))
		{
			cpu_halt (2, 0);
			cpu_reset (2);
		}
		else
			cpu_halt (2, 1);
	}
}



/*************************************
 *
 *		Playfield RAM read/write handlers
 *
 *************************************/

int eprom_playfieldram_r (int offset)
{
	return READ_WORD (&atarigen_playfieldram[offset]);
}


void eprom_playfieldram_w (int offset, int data)
{
	int oldword = READ_WORD (&atarigen_playfieldram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&atarigen_playfieldram[offset], newword);
		playfielddirty[offset / 2] = 1;
	}
}


int eprom_playfieldpalram_r (int offset)
{
	return READ_WORD (&eprom_playfieldpalram[offset]);
}


void eprom_playfieldpalram_w (int offset, int data)
{
	int oldword = READ_WORD (&eprom_playfieldpalram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&eprom_playfieldpalram[offset], newword);
		playfielddirty[offset / 2] = 1;
	}
}



/*************************************
 *
 *		Motion object list handlers
 *
 *************************************/

int eprom_update_display_list (int scanline)
{
	/* look up the SLIP link */
	int scrolly = (READ_WORD (&atarigen_alpharam[0xf02]) >> 7) & 0x1ff;

	int link = READ_WORD (&atarigen_alpharam[0xf80 + 2 * (((scanline + scrolly) / 8) & 0x3f)]) & 0x3ff;

	atarigen_update_display_list (atarigen_spriteram, link, scanline);

	return scrolly;
}


/*---------------------------------------------------------------------------------
 *
 * 	Motion Object encoding
 *
 *		4 16-bit words are used total
 *
 *		Word 1: Link
 *
 *			Bits 0-7   = link to the next motion object
 *
 *		Word 2: Image
 *
 *			Bits 0-11  = image index
 *
 *		Word 3: Horizontal position
 *
 *			Bits 0-3   = motion object palette
 *			Bits 7-15  = horizontal position
 *
 *		Word 4: Vertical position
 *
 *			Bits 0-2   = vertical size of the object, in tiles
 *			Bit  3     = horizontal flip
 *			Bits 4-6   = horizontal size of the object, in tiles
 *			Bits 7-15  = vertical position
 *
 *---------------------------------------------------------------------------------
 */

void eprom_calc_mo_colors (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	unsigned char *colors = param;
	int color = data[2] & 15;
	colors[color] = 1;
}

void eprom_render_mo (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	int xadv, x, y, sx, sy;

	/* extract data from the various words */
	int pict = data[1] & 0x7fff;
	int hsize = ((data[3] >> 4) & 7) + 1;
	int vsize = (data[3] & 7) + 1;
	int xpos = xscroll + (data[2] >> 7);
	int ypos = yscroll - (data[3] >> 7) - vsize * 8;
	int color = data[2] & 15;
	int hflip = data[3] & 0x0008;
//	int pri = (data[2] >> 4) & 3;

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

	/* now redraw the playfield if we need to */
/*	redraw_playfield_chunk (bitmap, xpos, ypos, hsize, vsize, pri);*/
}


static void redraw_playfield_chunk (struct osd_bitmap *bitmap, int xpos, int ypos, int w, int h, int pri)
{
	struct rectangle clip;
	int x, y, sx;

	/* wrap */
	if (xpos > XDIM) xpos -= 0x200;
	if (ypos > YDIM) ypos -= 0x200;

	/* make a clip */
	clip.min_x = xpos;
	clip.max_x = xpos + w * 8 - 1;
	clip.min_y = ypos;
	clip.max_y = ypos + h * 8 - 1;

	/* round the positions */
	xpos = (xpos - xscroll) / 8;
	ypos = (ypos - yscroll) / 8;

	/* loop over the columns */
	for (x = xpos + w; x >= xpos; x--)
	{
		/* compute the scroll-adjusted x position */
		sx = (x * 8 + xscroll) & 0x1ff;
		if (sx > 0x1f8) sx -= 0x200;

		/* loop over the rows */
		for (y = ypos + h; y >= ypos; y--)
		{
			int sy, offs, data2, color;

			/* compute the scroll-adjusted y position */
			sy = (y * 8 + yscroll) & 0x1ff;
			if (sy > 0x1f8) sy -= 0x200;

			/* process the data */
			offs = (x & 0x3f) * 64 + (y & 0x3f);
			data2 = READ_WORD (&eprom_playfieldpalram[offs * 2]);
			color = (data2 >> 8) & 15;

			/* the logic is more complicated than this, but this is close */
			if (pri != 3)
			{
				int data1 = READ_WORD (&atarigen_playfieldram[offs * 2]);
				int hflip = data1 & 0x8000;
				int mask = 0x0000;

				if (pri & 1) mask |= 0x00ff;
				if (pri & 2) mask |= 0xff00;

				drawgfx (bitmap, Machine->gfx[1], data1 & 0x7fff, 0x10 + color, hflip, 0,
						sx, sy, &clip, TRANSPARENCY_PENS, mask);
			}
		}
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void eprom_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	unsigned char mo_map[16], al_map[32], pf_map[16];
	int x, y, sx, sy, offs, xoffs, yoffs, i;


	eprom_debug ();


	/* reset color tracking */
	memset (mo_map, 0, sizeof (mo_map));
	memset (pf_map, 0, sizeof (pf_map));
	memset (al_map, 0, sizeof (al_map));
	memset (palette_used_colors, PALETTE_COLOR_UNUSED, Machine->drv->total_colors * sizeof(unsigned char));

	/* update color usage for the playfield */
	for (offs = 0; offs < 64*64; offs++)
	{
		int data2 = READ_WORD (&eprom_playfieldpalram[offs * 2]);
		int color = (data2 >> 8) & 15;
		pf_map[color] = 1;
	}

	/* update color usage for the mo's */
	atarigen_render_display_list (bitmap, eprom_calc_mo_colors, mo_map);

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



	/* compute scrolling so we know what to update */
	xscroll = READ_WORD (&atarigen_alpharam[0xf00]);
	yscroll = READ_WORD (&atarigen_alpharam[0xf02]);
	xscroll = -((xscroll >> 7) & 0x1ff);
	yscroll = -((yscroll >> 7) & 0x1ff);

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

	/* update only the portion of the playfield that's visible. */
	xoffs = (-xscroll / 8);
	yoffs = (-yscroll / 8);

	/* loop over the visible Y region */
	for (y = yoffs + YCHARS + 1; y >= yoffs; y--)
	{
		sy = y & 63;

		/* loop over the visible X region */
		for (x = xoffs + XCHARS + 1; x >= xoffs; x--)
		{
			/* read the data word */
			sx = x & 63;
			offs = sx * 64 + sy;

			/* rerender if dirty */
			if (playfielddirty[offs])
			{
				int data1 = READ_WORD (&atarigen_playfieldram[offs * 2]);
				int data2 = READ_WORD (&eprom_playfieldpalram[offs * 2]);
				int color = (data2 >> 8) & 15;
				int hflip = data1 & 0x8000;

				drawgfx (playfieldbitmap, Machine->gfx[1], data1 & 0x7fff, 0x10 + color, hflip, 0,
						8 * sx, 8 * sy, 0, TRANSPARENCY_NONE, 0);
				playfielddirty[offs] = 0;
			}
		}
	}

	/* copy the playfield to the destination */
	copyscrollbitmap (bitmap, playfieldbitmap, 1, &xscroll, 1, &yscroll, &Machine->drv->visible_area,
			TRANSPARENCY_NONE, 0);

	/* render the motion objects */
	atarigen_render_display_list (bitmap, eprom_render_mo, NULL);

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


/*************************************
 *
 *		Debugging
 *
 *************************************/

static void eprom_debug (void)
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
			fprintf (f, "%04X ", READ_WORD (&atarigen_playfieldram[0xf00 + i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		fprintf (f, "\n\nMotion Object SLIPs:\n");
		for (i = 0x00; i < 0x40; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_playfieldram[0xf80 + i*2]));
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
					data[0] & 0xfc00, data[1] & 0x0000, data[2] & 0x0070, data[3] & 0x0000);
		}

		fprintf (f, "\n\nPlayfield dump\n");
		for (i = 0; i < atarigen_playfieldram_size / 2; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_playfieldram[i*2]));
			if ((i & 63) == 63) fprintf (f, "\n");
		}

		fclose (f);
	}
}
