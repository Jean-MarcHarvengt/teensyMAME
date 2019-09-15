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


extern int slapstic_tweak (int offset);


struct atarisys2_mo_data
{
	int *redraw_list, *redraw;
	int xhold;
};



/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *atarisys2_slapstic_base;



/*************************************
 *
 *		Statics
 *
 *************************************/

static unsigned char *playfieldram;
static unsigned char *alpharam;

static int playfieldram_size = 0x4000;
//static int spriteram_size = 0x800;
static int alpharam_size = 0x1800;

static int videobank;

static unsigned char *playfielddirty;

static struct osd_bitmap *playfieldbitmap;

static int *pf_bank, *mo_bank;



/*************************************
 *
 *		Prototypes from other modules
 *
 *************************************/

void atarisys2_vh_stop (void);

#if 0
static void atarisys2_dump_video_ram (void);
#endif


/*************************************
 *
 *		Video system start
 *
 *************************************/

int atarisys2_vh_start(void)
{
	static struct atarigen_modesc atarisys2_modesc =
	{
		256,                 /* maximum number of MO's */
		8,                   /* number of bytes per MO entry */
		2,                   /* number of bytes between MO words */
		3,                   /* ignore an entry if this word == 0xffff */
		3, 3, 0xff,          /* link = (data[linkword] >> linkshift) & linkmask */
		0                    /* render in reverse link order */
	};

	/* allocate banked memory */
	alpharam = calloc (0x8000, 1);
	if (!alpharam)
	{
		atarisys2_vh_stop ();
		return 1;
	}
	spriteram = alpharam + alpharam_size;
	playfieldram = alpharam + 0x4000;

	/* reset the videoram banking */
	videoram = alpharam;
	videobank = 0;

	/* allocate dirty buffers */
	if (!playfielddirty)
		playfielddirty = malloc (playfieldram_size / 2);
	if (!playfielddirty)
	{
		atarisys2_vh_stop ();
		return 1;
	}

	/* allocate bitmaps */
	if (!playfieldbitmap)
		playfieldbitmap = osd_new_bitmap (128*8, 64*8, Machine->scrbitmap->depth);
	if (!playfieldbitmap)
	{
		atarisys2_vh_stop ();
		return 1;
	}

	/*
	 * if we are palette reducing, do the simple thing by marking everything used except for
	 * the transparent sprite and alpha colors; this should give some leeway for machines
	 * that can't give up all 256 colors
	 */
	if (palette_used_colors)
	{
		int i;
		memset (palette_used_colors, PALETTE_COLOR_USED, Machine->drv->total_colors * sizeof(unsigned char));
		for (i = 0; i < 4; i++)
			palette_used_colors[0 + i * 16] = PALETTE_COLOR_TRANSPARENT;
		for (i = 0; i < 8; i++)
			palette_used_colors[64 + i * 4] = PALETTE_COLOR_TRANSPARENT;
	}

	/* initialize the displaylist system */
	return atarigen_init_display_list (&atarisys2_modesc);
}


int paperboy_vh_start (void)
{
	/* playfield bit mapping */
	static int pf_offs[16] =
	{
		0x0000, 0x0400,  /* PFROMSEL0 */
		0x0800, 0x0800,  /* PFROMSEL1 */
		0x0000, 0x0400,  /* PFROMSEL2 */
		0x0800, 0x0800,  /* PFROMSEL3 */
		0x0000, 0x0400,  /* PFROMSEL0 */
		0x0800, 0x0800,  /* PFROMSEL1 */
		0x0000, 0x0400,  /* PFROMSEL2 */
		0x0800, 0x0800   /* PFROMSEL3 */
	};

	/* motion object bit mapping */
	static int mo_offs[32] =
	{
		0x0000, 0x0200, 0x0400, 0x0600,  /* 000 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 001 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 010 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 011 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 100 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 101 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 110 */
		0x0000, 0x0200, 0x0400, 0x0600   /* 111 */
	};

	pf_bank = pf_offs;
	mo_bank = mo_offs;
	return atarisys2_vh_start ();
}


int apb_vh_start (void)
{
	/* playfield bit mapping */
	static int pf_offs[16] =
	{
		0x0800, 0x0c00,  /* PFROMSEL0 */
		0x1800, 0x1c00,  /* PFROMSEL1 */
		0x2800, 0x2c00,  /* PFROMSEL2 */
		0x3800, 0x3c00,  /* PFROMSEL3 */
		0x0000, 0x0400,  /* PFROMSEL0 */
		0x1000, 0x1400,  /* PFROMSEL1 */
		0x2000, 0x2400,  /* PFROMSEL2 */
		0x3000, 0x3400   /* PFROMSEL3 */
	};

	/* motion object bit mapping */
	static int mo_offs[32] =
	{
		0x0200, 0x0600, 0x0a00, 0x0e00,  /* 000 */
		0x0000, 0x0400, 0x0800, 0x0c00,  /* 001 */
		0x1200, 0x1600, 0x1a00, 0x1e00,  /* 010 */
		0x1000, 0x1400, 0x1800, 0x1c00,  /* 011 */
		0x0200, 0x0600, 0x0a00, 0x0e00,  /* 100 */
		0x0000, 0x0400, 0x0800, 0x0c00,  /* 101 */
		0x1200, 0x1600, 0x1a00, 0x1e00,  /* 110 */
		0x1000, 0x1400, 0x1800, 0x1c00   /* 111 */
	};

	pf_bank = pf_offs;
	mo_bank = mo_offs;
	return atarisys2_vh_start ();
}


int a720_vh_start (void)
{
	/* playfield bit mapping */
	static int pf_offs[16] =
	{
		0x0000, 0x0400,  /* PFROMSEL0 */
		0x0800, 0x0c00,  /* PFROMSEL1 */
		0x1000, 0x1400,  /* PFROMSEL2 */
		0x1800, 0x1c00,  /* PFROMSEL3 */
		0x0000, 0x0400,  /* PFROMSEL0 */
		0x0800, 0x0c00,  /* PFROMSEL1 */
		0x1000, 0x1400,  /* PFROMSEL2 */
		0x1800, 0x1c00   /* PFROMSEL3 */
	};

	/* motion object bit mapping */
	static int mo_offs[32] =
	{
		0x0200, 0x0600, 0x0a00, 0x0e00,  /* 000 */
		0x0000, 0x0400, 0x0800, 0x0c00,  /* 001 */
		0x1200, 0x1600, 0x1a00, 0x1e00,  /* 010 */
		0x1000, 0x1400, 0x1800, 0x1c00,  /* 011 */
		0x0200, 0x0600, 0x0a00, 0x0e00,  /* 100 */
		0x0000, 0x0400, 0x0800, 0x0c00,  /* 101 */
		0x1200, 0x1600, 0x1a00, 0x1e00,  /* 110 */
		0x1000, 0x1400, 0x1800, 0x1c00   /* 111 */
	};

	pf_bank = pf_offs;
	mo_bank = mo_offs;
	return atarisys2_vh_start ();
}


int ssprint_vh_start (void)
{
	/* playfield bit mapping */
	static int pf_offs[16] =
	{
		0x0800, 0x0c00,  /* PFROMSEL0 */
		0x1800, 0x1c00,  /* PFROMSEL1 */
		0x2800, 0x2c00,  /* PFROMSEL2 */
		0x3800, 0x3c00,  /* PFROMSEL3 */
		0x0000, 0x0400,  /* PFROMSEL0 */
		0x1000, 0x1400,  /* PFROMSEL1 */
		0x2000, 0x2400,  /* PFROMSEL2 */
		0x3000, 0x3400   /* PFROMSEL3 */
	};

	/* motion object bit mapping */
	static int mo_offs[32] =
	{
		0x0000, 0x0200, 0x0400, 0x0600,  /* 000 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 001 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 010 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 011 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 100 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 101 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 110 */
		0x0000, 0x0200, 0x0400, 0x0600   /* 111 */
	};

	pf_bank = pf_offs;
	mo_bank = mo_offs;
	return atarisys2_vh_start ();
}


int csprint_vh_start (void)
{
	/* playfield bit mapping */
	static int pf_offs[16] =
	{
		0x0800, 0x0c00,  /* PFROMSEL0 */
		0x1800, 0x1c00,  /* PFROMSEL1 */
		0x2800, 0x2c00,  /* PFROMSEL2 */
		0x3800, 0x3c00,  /* PFROMSEL3 */
		0x0000, 0x0400,  /* PFROMSEL0 */
		0x1000, 0x1400,  /* PFROMSEL1 */
		0x2000, 0x2400,  /* PFROMSEL2 */
		0x3000, 0x3400   /* PFROMSEL3 */
	};

	/* motion object bit mapping */
	static int mo_offs[32] =
	{
		0x0000, 0x0200, 0x0400, 0x0600,  /* 000 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 001 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 010 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 011 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 100 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 101 */
		0x0000, 0x0200, 0x0400, 0x0600,  /* 110 */
		0x0000, 0x0200, 0x0400, 0x0600   /* 111 */
	};

	pf_bank = pf_offs;
	mo_bank = mo_offs;
	return atarisys2_vh_start ();
}



/*************************************
 *
 *		Video system shutdown
 *
 *************************************/

void atarisys2_vh_stop(void)
{
	/* free memory */
	if (alpharam)
		free (alpharam);
	alpharam = playfieldram = spriteram = 0;

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
 *		Scroll/playfield bank w
 *
 *************************************/

void atarisys2_vscroll_w (int offset, int data)
{
	int oldword = READ_WORD (&atarigen_vscroll[offset]);
	int newword = COMBINE_WORD (oldword, data);
	WRITE_WORD (&atarigen_vscroll[offset], newword);

	/* if we changed the bank, we need to rerender the playfield */
	if (offset == 0 && (oldword & 15) != (newword & 15))
		memset (playfielddirty, 1, playfieldram_size / 2);
}


void atarisys2_hscroll_w (int offset, int data)
{
	int oldword = READ_WORD (&atarigen_hscroll[offset]);
	int newword = COMBINE_WORD (oldword, data);
	WRITE_WORD (&atarigen_hscroll[offset], newword);

	/* if we changed the bank, we need to rerender the playfield */
	if (offset == 0 && (oldword & 15) != (newword & 15))
		memset (playfielddirty, 1, playfieldram_size / 2);
}



/*************************************
 *
 *		Palette RAM read/write handlers
 *
 *************************************/

void atarisys2_paletteram_w (int offset, int data)
{
	static const int intensity_table[16] =
	{
		#define ZB 115
		#define Z3 78
		#define Z2 37
		#define Z1 17
		#define Z0 9
		0, ZB+Z0, ZB+Z1, ZB+Z1+Z0, ZB+Z2, ZB+Z2+Z0, ZB+Z2+Z1, ZB+Z2+Z1+Z0,
		ZB+Z3, ZB+Z3+Z0, ZB+Z3+Z1, ZB+Z3+Z1+Z0,ZB+ Z3+Z2, ZB+Z3+Z2+Z0, ZB+Z3+Z2+Z1, ZB+Z3+Z2+Z1+Z0
	};
	static const int color_table[16] =
		{ 0x0, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xe, 0xf, 0xf };

	int inten, red, green, blue;

	int oldword = READ_WORD (&paletteram[offset]);
	int newword = COMBINE_WORD (oldword, data);
	int indx = offset / 2;

	WRITE_WORD (&paletteram[offset], newword);

	inten = intensity_table[newword & 15];
	red = (color_table[(newword >> 12) & 15] * inten) >> 4;
	green = (color_table[(newword >> 8) & 15] * inten) >> 4;
	blue = (color_table[(newword >> 4) & 15] * inten) >> 4;
	palette_change_color (indx, red, green, blue);
}



/*************************************
 *
 *		Video RAM bank read/write handlers
 *
 *************************************/

int atarisys2_slapstic_r (int offset)
{
	slapstic_tweak (offset / 2);

	/* an extra tweak for the next opcode fetch */
	videobank = slapstic_tweak (0x1234);
	videoram = alpharam + videobank * 0x2000;

	return READ_WORD (&atarisys2_slapstic_base[offset]);
}


void atarisys2_slapstic_w (int offset, int data)
{
	slapstic_tweak (offset / 2);

	/* an extra tweak for the next opcode fetch */
	videobank = slapstic_tweak (0x1234);
	videoram = alpharam + videobank * 0x2000;
}


void atarisys2_vmmu_w (int offset, int data)
{
	if (offset == 0)
	{
		videobank = (data >> 12) & 3;
		videoram = alpharam + videobank * 0x2000;
	}
}



/*************************************
 *
 *		Video RAM read/write handlers
 *
 *************************************/

int atarisys2_videoram_r (int offset)
{
	return READ_WORD (&videoram[offset]);
}


void atarisys2_videoram_w (int offset, int data)
{
	int oldword = READ_WORD (&videoram[offset]);
	int newword = COMBINE_WORD (oldword, data);
	WRITE_WORD (&videoram[offset], newword);

	if (videobank >= 2)
		if ((oldword & 0x3fff) != (newword & 0x3fff))
		{
			int offs = (&videoram[offset] - playfieldram) / 2;
			playfielddirty[offs] = 1;
		}
}



/*************************************
 *
 *		Motion object list handlers
 *
 *************************************/

void atarisys2_update_display_list (int scanline)
{
	atarigen_update_display_list (spriteram, 0, scanline);
}


/*---------------------------------------------------------------------------------
 *
 * 	Motion Object encoding
 *
 *		4 16-bit words are used total
 *
 *		Word 1: Vertical position
 *
 *			Bits 0-2   = upper 3 bits of the image index
 *			Bits 6-14  = vertical position
 *
 *		Word 2: Image
 *
 *			Bits 0-10  = index of the image
 *			Bits 11-13 = size of the image (1..8)
 *			Bit  14    = horizontal flip
 *			Bit  15    = hold X position from last MO
 *
 *		Word 3: Horizontal position
 *
 *			Bits 6-15  = horizontal position
 *
 *		Word 4: Link
 *
 *			Bits 3-10  = link to the next motion object
 *			Bits 12-13 = palette of the image
 *			Bits 14-15 = image priority
 *
 *---------------------------------------------------------------------------------
 */

void atarisys2_render_mo (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	struct atarisys2_mo_data *modata = param;
	int *redraw_list = modata->redraw_list;
	int *redraw = modata->redraw;
	int *r, redraw_val;

	/* extract data from the various words */
	int xpos = (data[2] >> 6);
	int vsize = ((data[1] >> 11) & 7) + 1;
	int ypos = -(data[0] >> 6) - vsize * 16;
	int hflip = data[1] & 0x4000;
	int hold = data[1] & 0x8000;
	int pict = (data[1] & 0x7ff) + ((data[0] & 7) << 11);
	int color = (data[3] >> 12) & 3;
	int priority = (data[3] >> 14) & 3;
	int y, sy;

	/* shuffle the bits */
	pict = (pict & 0x1ff) | mo_bank[pict >> 9];

	/* adjust x position for holding */
	if (hold)
		xpos = modata->xhold;
	modata->xhold = xpos + 16;

	/* adjust the final coordinates */
	xpos &= 0x3ff;
	ypos &= 0x1ff;
	redraw_val = (xpos << 22) + (ypos << 13) + (priority << 10) + vsize;
	if (xpos >= XDIM) xpos -= 0x400;
	if (ypos >= YDIM) ypos -= 0x200;

	/* clip the X coordinate */
	if (xpos <= -16 || xpos >= XDIM)
		return;

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

	/* loop over the height */
	for (y = 0, sy = ypos; y < vsize; y++, sy += 16, pict++)
	{
		/* clip the Y coordinate */
		if (sy <= clip->min_y - 16)
			continue;
		else if (sy > clip->max_y)
			break;

		/* draw the sprite */
		drawgfx (bitmap, Machine->gfx[2], pict, color, hflip, 0,
			xpos, sy, clip, TRANSPARENCY_PEN, 15);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void atarisys2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int x, y, sx, sy, xoffs, yoffs, xscroll, yscroll, offs;
	struct atarisys2_mo_data modata;
	int redraw_list[1024], *r;
	int bank[2];


/*	if (osd_key_pressed (OSD_KEY_9)) atarisys2_dump_video_ram ();*/


	/* recalc the palette if necessary */
	if (palette_recalc ())
		memset (playfielddirty, 1, playfieldram_size / 2);


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
	 *		1 16-bit word is used
	 *
	 *			Bits 0-9   = index of the image
	 *			Bit  10    = index of which bank to use
	 *			Bits 11-13 = palette of the image
	 *			Bit  14-15 = image priority
	 *
	 *---------------------------------------------------------------------------------
	 */

	/* update only the portion of the playfield that's visible. */
	xoffs = -xscroll / 8;
	yoffs = -yscroll / 8;

	/* look up the banks */
	bank[0] = pf_bank[READ_WORD (&atarigen_hscroll[0]) & 15];
	bank[1] = pf_bank[READ_WORD (&atarigen_vscroll[0]) & 15];

	/* loop over the visible Y portion */
	for (y = yoffs + YCHARS + 1; y >= yoffs; y--)
	{
		sy = y & 63;

		/* loop over the visible X portion */
		for (x = xoffs + XCHARS + 1; x >= xoffs; x--)
		{
			int data;

			/* compute the offset */
			sx = x & 127;
			offs = sy * 128 + sx;
			data = READ_WORD (&playfieldram[offs * 2]);

			/* redraw if dirty */
			if (playfielddirty[offs])
			{
				int pfbank = bank[(data >> 10) & 1];
				int pict = data & 0x3ff;
				int color = (data >> 11) & 7;

				drawgfx (playfieldbitmap, Machine->gfx[1], pfbank + pict, color, 0, 0,
						8 * sx, 8 * sy, 0, TRANSPARENCY_NONE, 0);
				playfielddirty[offs] = 0;
			}
		}
	}

	/* copy the playfield to the destination */
	copyscrollbitmap (bitmap, playfieldbitmap, 1, &xscroll, 1, &yscroll, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);

	/* prepare the motion object data structure */
	modata.xhold = 0;
	modata.redraw_list = modata.redraw = redraw_list;

	/* render the motion objects */
	atarigen_render_display_list (bitmap, atarisys2_render_mo, &modata);

	/* redraw playfield tiles with higher priority */
	for (r = redraw_list; r < modata.redraw; r++)
	{
		int val = *r;
		int xpos = (val >> 22) & 0x3ff;
		int ypos = (val >> 13) & 0x1ff;
		int pri = (val >> 9) & 6;
		int h = val & 15;
		struct rectangle clip;

		/* wrap */
		if (xpos > XDIM) xpos -= 0x400;
		if (ypos > YDIM) ypos -= 0x200;

		/* make a clip */
		clip.min_x = xpos;
		clip.max_x = xpos + 15;
		clip.min_y = ypos;
		clip.max_y = ypos + h * 16 - 1;

		/* round the positions */
		xpos = (xpos - xscroll) / 8;
		ypos = (ypos - yscroll) / 8;

		/* loop over the columns */
		for (x = xpos + 2; x >= xpos; x--)
		{
			/* compute the scroll-adjusted x position */
			sx = (x * 8 + xscroll) & 0x3ff;
			if (sx > 0x3f8) sx -= 0x400;

			/* loop over the rows */
			for (y = ypos + h * 2; y >= ypos; y--)
			{
				int  data, pfpri;

				/* compute the scroll-adjusted y position */
				sy = (y * 8 + yscroll) & 0x1ff;
				if (sy > 0x1f8) sy -= 0x200;

				/* process the data */
				offs = (y & 0x3f) * 128 + (x & 0x7f);
				data = READ_WORD (&playfieldram[offs * 2]);
				pfpri = ((~data >> 13) & 6) | 1;

				/* this is the priority equation from the schematics */
				if ((pri + pfpri) & 4)
				{
					int pfbank = bank[(data >> 10) & 1];
					int pict = data & 0x3ff;
					int color = (data >> 11) & 7;

					drawgfx (bitmap, Machine->gfx[1], pfbank + pict, color, 0, 0,
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
			int data = READ_WORD (&alpharam[offs * 2]);
			int pict = data & 0x3ff;

			/* if there's a non-zero picture or if we're fully opaque, draw the tile */
			if (pict)
			{
				int color = (data >> 13) & 7;

				/* draw the character */
				drawgfx (bitmap, Machine->gfx[0], pict, color, 0, 0,
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
static void atarisys2_dump_video_ram (void)
{
	static int count;
	char name[50];
	FILE *f;
	int i;

	while (osd_key_pressed (OSD_KEY_9)) { }

	sprintf (name, "Dump %d", ++count);
	f = fopen (name, "wt");

	fprintf (f, "MO Palette\n");
	for (i = 0x00; i < 0x40; i++)
	{
		fprintf (f, "%04X ", READ_WORD (&paletteram[i*2]));
		if ((i & 15) == 15) fprintf (f, "\n");
	}

	fprintf (f, "\n\nAlpha Palette\n");
	for (i = 0x40; i < 0x80; i++)
	{
		fprintf (f, "%04X ", READ_WORD (&paletteram[i*2]));
		if ((i & 15) == 15) fprintf (f, "\n");
	}

	fprintf (f, "\n\nPlayfield Palette\n");
	for (i = 0x80; i < 0x100; i++)
	{
		fprintf (f, "%04X ", READ_WORD (&paletteram[i*2]));
		if ((i & 15) == 15) fprintf (f, "\n");
	}

	fprintf (f, "\n\nX Scroll = %03X\nY Scroll = %03X\n",
		READ_WORD (&atarigen_hscroll[0]) >> 6, READ_WORD (&atarigen_vscroll[0]) >> 6);

	fprintf (f, "\n\nX PFBank0 = %X\nY PFBank1 = %X\n",
		READ_WORD (&atarigen_hscroll[0]) & 15, READ_WORD (&atarigen_vscroll[0]) & 15);

	fprintf (f, "\n\nMotion Objects\n");
	for (i = 0; i < spriteram_size; i += 8)
	{
		int data1 = READ_WORD (&spriteram[i+0]);
		int data2 = READ_WORD (&spriteram[i+2]);
		int data3 = READ_WORD (&spriteram[i+4]);
		int data4 = READ_WORD (&spriteram[i+6]);

		int ypos = (data1 >> 6) & 0x1ff;
		int vsize = ((data1 >> 3) & 7) + 1;
		int hsize = (data1 & 7) + 1;
		int pict = (data2 & 0x3fff);
		int vflip = ((data2 >> 15) & 1);
		int hflip = ((data2 >> 14) & 1);
		int link = (data3 & 0xff);
		int color = (data4 & 0x0f);
		int xpos = (data4 >> 6) & 0x3ff;

		fprintf (f, "  Object %02X: 0=%04X 1=%04X 2=%04X 3=%04X\n",
			i, data1, data2, data3, data4);
	}

	fprintf (f, "\n\nPlayfield dump\n");
	for (i = 0; i < playfieldram_size / 2; i++)
	{
		fprintf (f, "%04X ", READ_WORD (&playfieldram[i*2]));
		if ((i & 127) == 127) fprintf (f, "\n");
		else if ((i & 127) == 63) fprintf (f, "\n      ");
	}

	fprintf (f, "\n\nAlpha dump\n");
	for (i = 0; i < alpharam_size / 2; i++)
	{
		fprintf (f, "%04X ", READ_WORD (&alpharam[i*2]));
		if ((i & 63) == 63) fprintf (f, "\n");
	}

	fclose (f);
}
#endif
