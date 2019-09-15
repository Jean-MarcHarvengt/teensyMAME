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


struct atarisys1_mo_data
{
	int pcolor;
	int *redraw_list, *redraw;
};


/* these macros make accessing the indirection table easier, plus this is how the data
   is stored for the pfmapped array */
#define LDATA(bank,color,offset) ((((color) & 255) << 16) | (((bank) & 15) << 12) | (((offset) & 15) << 8))
#define LCOLOR(data) (((data) >> 16) & 0xff)
#define LBANK(data) (((data) >> 12) & 15)
#define LPICT(data) ((data) & 0x0fff)
#define LFLIP(data) ((data) & 0x01000000)
#define LDIRTYFLAG 0x02000000
#define LDIRTY(data) ((data) & LDIRTYFLAG)

/* these macros are for more conveniently defining groups of motion and playfield objects */
#define LDATA2(b,c,o) LDATA(b,c,(o)+0), LDATA(b,c,(o)+1)
#define LDATA3(b,c,o) LDATA(b,c,(o)+0), LDATA(b,c,(o)+1), LDATA(b,c,(o)+2)
#define LDATA4(b,c,o) LDATA(b,c,(o)+0), LDATA(b,c,(o)+1), LDATA(b,c,(o)+2), LDATA(b,c,(o)+3)
#define LDATA5(b,c,o) LDATA4(b,c,(o)+0), LDATA(b,c,(o)+4)
#define LDATA6(b,c,o) LDATA4(b,c,(o)+0), LDATA2(b,c,(o)+4)
#define LDATA7(b,c,o) LDATA4(b,c,(o)+0), LDATA3(b,c,(o)+4)
#define LDATA8(b,c,o) LDATA4(b,c,(o)+0), LDATA4(b,c,(o)+4)
#define LDATA9(b,c,o) LDATA8(b,c,(o)+0), LDATA(b,c,(o)+8)
#define LDATA10(b,c,o) LDATA8(b,c,(o)+0), LDATA2(b,c,(o)+8)
#define LDATA11(b,c,o) LDATA8(b,c,(o)+0), LDATA3(b,c,(o)+8)
#define LDATA12(b,c,o) LDATA8(b,c,(o)+0), LDATA4(b,c,(o)+8)
#define LDATA13(b,c,o) LDATA8(b,c,(o)+0), LDATA5(b,c,(o)+8)
#define LDATA14(b,c,o) LDATA8(b,c,(o)+0), LDATA6(b,c,(o)+8)
#define LDATA15(b,c,o) LDATA8(b,c,(o)+0), LDATA7(b,c,(o)+8)
#define LDATA16(b,c,o) LDATA8(b,c,(o)+0), LDATA8(b,c,(o)+8)
#define EMPTY2 0,0
#define EMPTY5 0,0,0,0,0
#define EMPTY6 0,0,0,0,0,0
#define EMPTY8 0,0,0,0,0,0,0,0
#define EMPTY15 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
#define EMPTY16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0



/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *atarisys1_bankselect;
unsigned char *atarisys1_prioritycolor;



/*************************************
 *
 *		Statics
 *
 *************************************/

static unsigned int *scrolllist;
static int scrolllist_end;

static int *pflookup, *molookup;

static int *pfmapped;
static int pfbank;

static int mo_map_shift, pf_map_shift;

static struct osd_bitmap *playfieldbitmap;
static struct osd_bitmap *tempbitmap;

static void *int3_timer[YDIM];
static void *int3off_timer;
static int int3_state;

static int xscroll, yscroll;

static unsigned short *roadblst_pen_usage;



/*************************************
 *
 *		Prototypes from other modules
 *
 *************************************/

void atarisys1_vh_stop (void);
void atarisys1_update_display_list (int scanline);

#if 0
static int atarisys1_debug (void);
#endif

static void redraw_playfield_chunk (struct osd_bitmap *bitmap, int xpos, int ypos, int w, int h, int transparency, int transparency_color, int type);


/*************************************
 *
 *		Generic video system start
 *
 *************************************/

int atarisys1_vh_start(void)
{
	static struct atarigen_modesc atarisys1_modesc =
	{
		64,                  /* maximum number of MO's */
		2,                   /* number of bytes per MO entry */
		0x80,                /* number of bytes between MO words */
		1,                   /* ignore an entry if this word == 0xffff */
		3, 0, 0x3f,          /* link = (data[linkword] >> linkshift) & linkmask */
		0                    /* render in reverse link order */
	};

	int i;

	/* allocate dirty buffers */
	if (!pfmapped)
		pfmapped = calloc (atarigen_playfieldram_size / 2 * sizeof (int) +
		                   YDIM * sizeof (int), 1);
	if (!pfmapped)
	{
		atarisys1_vh_stop ();
		return 1;
	}
	scrolllist = (unsigned int *)(pfmapped + atarigen_playfieldram_size / 2);

	/* allocate bitmaps */
	if (!playfieldbitmap)
		playfieldbitmap = osd_new_bitmap (64*8, 64*8, Machine->scrbitmap->depth);
	if (!playfieldbitmap)
	{
		atarisys1_vh_stop ();
		return 1;
	}

	if (!tempbitmap)
		tempbitmap = osd_new_bitmap (Machine->drv->screen_width, Machine->drv->screen_height, Machine->scrbitmap->depth);
	if (!tempbitmap)
	{
		atarisys1_vh_stop ();
		return 1;
	}

	/* reset the scroll list */
	scrolllist_end = 0;

	/* reset the timers */
	memset (int3_timer, 0, sizeof (int3_timer));
	int3_state = 0;

	/* initialize the pre-mapped playfield */
	for (i = atarigen_playfieldram_size / 2; i >= 0; i--)
		pfmapped[i] = pflookup[0] | LDIRTYFLAG;

	/* initialize the displaylist system */
	return atarigen_init_display_list (&atarisys1_modesc);
}



/*************************************
 *
 *		Game-specific video system start
 *
 *************************************/

int marble_vh_start(void)
{
	static int marble_pflookup[256] =
	{
		LDATA16 (1,0+8,0),
		LDATA16 (1,1+8,0),
		LDATA16 (1,2+8,0),
		LDATA16 (1,3+8,0),
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16
	};

	static int marble_molookup[256] =
	{
		LDATA8 (2,0,8),
		LDATA8 (2,2,8),
		LDATA8 (2,4,8),
		LDATA8 (2,6,8),
		LDATA8 (2,8,8),
		LDATA8 (2,10,8),
		LDATA8 (2,12,8),
		LDATA8 (2,14,8),
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16
	};

	pflookup = marble_pflookup;
	molookup = marble_molookup;
	mo_map_shift = 1;
	pf_map_shift = 1;
	return atarisys1_vh_start ();
}


int peterpak_vh_start(void)
{
	static int peterpak_pflookup[256] =
	{
		LDATA16 (1,0+16,0),
		LDATA16 (2,0+16,0),
		LDATA8 (3,0+16,8), EMPTY8,
		EMPTY16,
		LDATA16 (1,1+16,0),
		LDATA16 (2,1+16,0),
		LDATA8 (3,1+16,8), EMPTY8,
		EMPTY16,
		LDATA16 (1,2+16,0),
		LDATA16 (2,2+16,0),
		LDATA8 (3,2+16,8), EMPTY8,
		EMPTY16,
		LDATA16 (1,3+16,0),
		LDATA16 (2,3+16,0),
		LDATA8 (3,3+16,8), EMPTY8,
		EMPTY16,
	};

	static int peterpak_molookup[256] =
	{
		LDATA16 (1,0,0),
		LDATA16 (2,0,0),
		LDATA8 (3,0,8), EMPTY8,
		EMPTY16,
		LDATA16 (1,1,0),
		LDATA16 (2,1,0),
		LDATA8 (3,1,8), EMPTY8,
		EMPTY16,
		LDATA16 (1,2,0),
		LDATA16 (2,2,0),
		LDATA8 (3,2,8), EMPTY8,
		EMPTY16,
		LDATA16 (1,3,0),
		LDATA16 (2,3,0),
		LDATA8 (3,3,8), EMPTY8,
		EMPTY16,
	};

	pflookup = peterpak_pflookup;
	molookup = peterpak_molookup;
	mo_map_shift = 0;
	pf_map_shift = 0;
	return atarisys1_vh_start ();
}


int indytemp_vh_start(void)
{
	#define SEVEN 1
	#define SIX 2
	#define FIVE 3
	#define THREE 4

	static int indytemp_pflookup[256] =
	{
		LDATA (SEVEN,0+16,0), LDATA7 (SIX,3+16,8), LDATA8 (THREE,2+16,0),
		LDATA8 (THREE,2+16,8), LDATA8 (SIX,1+16,8),
		LDATA16 (FIVE,1+16,0),
		LDATA16 (THREE,1+16,0),
		LDATA8 (SIX,0+16,8), LDATA8 (FIVE,0+16,0),
		LDATA8 (FIVE,0+16,8), LDATA8 (THREE,0+16,0),
		LDATA8 (THREE,0+16,8), LDATA8 (SIX,2+16,8),
		LDATA16 (FIVE,2+16,0),
		LDATA (SEVEN,0+16,0), LDATA7 (SIX,3+16,8), LDATA8 (THREE,2+16,0),
		LDATA8 (THREE,2+16,8), LDATA8 (SIX,1+16,8),
		LDATA16 (FIVE,1+16,0),
		LDATA16 (THREE,1+16,0),
		LDATA8 (SIX,0+16,8), LDATA8 (FIVE,0+16,0),
		LDATA8 (FIVE,0+16,8), LDATA8 (THREE,0+16,0),
		LDATA8 (THREE,0+16,8), LDATA8 (SIX,2+16,8),
		LDATA16 (FIVE,2+16,0),
	};

	static int indytemp_molookup[256] =
	{
		LDATA16 (SEVEN,1,0),
		LDATA16 (SIX,1,0),
		LDATA16 (SEVEN,0,0),
		LDATA16 (SIX,0,0),
		LDATA16 (SEVEN,2,0),
		LDATA16 (SIX,2,0),
		LDATA16 (SEVEN,3,0),
		LDATA16 (SIX,3,0),
		LDATA16 (SEVEN,4,0),
		LDATA16 (SIX,4,0),
		LDATA16 (SEVEN,5,0),
		LDATA16 (SIX,5,0),
		LDATA16 (SEVEN,6,0),
		LDATA16 (SIX,6,0),
		LDATA16 (SEVEN,7,0),
		LDATA16 (SIX,7,0),
	};

	#undef THREE
	#undef FIVE
	#undef SIX
	#undef SEVEN

	pflookup = indytemp_pflookup;
	molookup = indytemp_molookup;
	mo_map_shift = 0;
	pf_map_shift = 0;
	return atarisys1_vh_start ();
}


int roadrunn_vh_start(void)
{
	#define SEVEN 1	/* enable off */
	#define SIX 2
	#define FIVE 3
	#define THREE 4
	#define SEVENY 5	/* enable on */
	#define SEVENX 6  /* looks like 3bpp */

	static int roadrunn_pflookup[256] =
	{
		LDATA (SEVEN,0+16,0), LDATA (SEVEN,15+16,0), LDATA (SEVEN,14+16,0), LDATA (SEVEN,13+16,0),
			LDATA (SEVEN,12+16,0), LDATA (SEVEN,11+16,0), LDATA (SEVEN,10+16,0), LDATA (SEVEN,9+16,0),
			LDATA (SEVEN,8+16,0), LDATA (SEVEN,7+16,0), LDATA (SEVEN,6+16,0), LDATA (SEVEN,5+16,0),
			LDATA (SEVEN,4+16,0), LDATA (SEVEN,3+16,0), LDATA (SEVEN,2+16,0), LDATA (SEVEN,1+16,0),
		LDATA (SEVEN,0+16,0), LDATA9 (SIX,12+16,1), LDATA6 (SIX,1+16,10),
		LDATA16 (FIVE,1+16,0),
		LDATA13 (THREE,5+16,0), LDATA3 (THREE,2+16,13),
		LDATA16 (SEVENY,2+16,0),
		LDATA3 (SEVENX,2+16,0), LDATA7 (SEVENX,4+16,3), LDATA2 (SEVENX,3+16,10), LDATA (SEVENX,6+16,12),
			LDATA (SEVENX,7+16,13), LDATA2 (SEVENX,0+16,14),
		LDATA (FIVE,5+16,15), LDATA (FIVE,2+16,15), LDATA6 (THREE,2+16,7), LDATA (FIVE,4+16,15),
			LDATA2 (THREE,4+16,11), LDATA (SEVENX,4+16,2), LDATA (THREE,3+16,12), LDATA (SEVENX,3+16,9),
			LDATA (FIVE,6+16,15), LDATA (THREE,6+16,11),
		LDATA (THREE,6+16,12), LDATA (SEVENX,6+16,2), LDATA (SEVENX,6+16,9), LDATA (SEVENX,6+16,11),
			LDATA (FIVE,7+16,15), LDATA (THREE,7+16,12), LDATA (SEVENX,7+16,9), LDATA2 (SEVENX,7+16,11),
			LDATA (FIVE,0+16,15), LDATA (SEVENX,0+16,13), EMPTY5,
		LDATA16 (SEVEN,0+16,0),
		LDATA16 (SIX,0+16,0),
		LDATA16 (FIVE,0+16,0),
		LDATA16 (THREE,0+16,0),
		LDATA16 (SEVENY,0+16,0),
		LDATA16 (SEVENX,0+16,0),
		LDATA (SEVEN,1+16,15), LDATA14 (SIX,1+16,0),
		LDATA (SIX,1+16,15), LDATA14 (FIVE,1+16,0)
	};

	static int roadrunn_molookup[256] =
	{
		LDATA16 (SEVEN,0,0),
		LDATA16 (SIX,0,0),
		LDATA16 (SEVEN,1,0),
		LDATA16 (SIX,1,0),
		LDATA16 (SEVEN,2,0),
		LDATA16 (SIX,2,0),
		LDATA16 (SEVEN,3,0),
		LDATA16 (SIX,3,0),
		LDATA16 (SEVEN,4,0),
		LDATA16 (SIX,4,0),
		LDATA16 (SEVEN,5,0),
		LDATA16 (SIX,5,0),
		LDATA16 (SEVEN,6,0),
		LDATA16 (SIX,6,0),
		LDATA16 (SEVEN,7,0),
		LDATA16 (SIX,7,0)
	};

	#undef THREE
	#undef FIVE
	#undef SIX
	#undef SEVEN
	#undef SEVENX
	#undef SEVENY

	pflookup = roadrunn_pflookup;
	molookup = roadrunn_molookup;
	mo_map_shift = 0;
	pf_map_shift = 0;
	return atarisys1_vh_start ();
}


int roadblst_vh_start(void)
{
	#define SEVEN6 1
	#define SEVEN 2
	#define SIX 3
	#define FIVE 4
	#define THREE 5
	#define SEVENMO 6
	#define SEVENMO0 7  /* looks like 3bpp, with a 0 in the high bit of the 2nd byte */
	#define SEVENMO1 8  /* looks like 3bpp, with a 1 in the high bit of the 2nd byte */

	static int roadblst_pflookup[256] =
	{
		LDATA2 (SEVEN6,2+4,0), LDATA14 (SEVEN,1+16,2),
		LDATA2 (SEVEN6,3+4,0), LDATA14 (SEVEN,2+16,2),
		LDATA2 (SEVEN6,1+4,0), LDATA14 (SEVEN,0+16,2),
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16,
		EMPTY16
	};

	static int roadblst_molookup[256] =
	{
		LDATA16 (SIX,0,0),
		LDATA16 (FIVE,0,0),
		LDATA16 (THREE,0,0),
		LDATA16 (SEVENMO,0,0),
		LDATA16 (SEVENMO0,0,0),
		LDATA16 (SEVENMO1,0,0),
		LDATA14 (FIVE,1,2), LDATA2 (THREE,1,0),
		LDATA4 (FIVE,2,12), LDATA12 (THREE,2,0),
		LDATA9 (THREE,3,7), LDATA7 (SEVENMO,3,0),
		LDATA (SEVENMO,3,7), EMPTY15,
		LDATA11 (SEVENMO,4,5), LDATA5 (SEVENMO0,4,0),
		LDATA (SEVENMO,6,15), LDATA15 (SEVENMO0,6,0),
		LDATA5 (SEVENMO0,7,11), LDATA11 (SEVENMO1,7,0),
		LDATA10 (SEVENMO1,5,6), EMPTY6,
		EMPTY16,
		LDATA14 (SEVENMO,0,2), EMPTY2
	};
	#undef THREE
	#undef FIVE
	#undef SIX
	#undef SEVEN
	#undef SEVEN6

	/* allocate our own pen usage table for the 6-bit background tiles */
	{
		struct GfxElement *gfx;
		unsigned short *entry;
		unsigned char *dp;
		int x, y, i;

		gfx = Machine->gfx[1];
		roadblst_pen_usage = malloc (gfx->total_elements * 4 * sizeof (short));
		if (!roadblst_pen_usage)
			return 1;
		memset (roadblst_pen_usage, 0, gfx->total_elements * 4 * sizeof (short));

		for (i = 0, entry = roadblst_pen_usage; i < gfx->total_elements; i++, entry += 4)
		{
			for (y = 0; y < gfx->height; y++)
			{
				dp = gfx->gfxdata->line[i * gfx->height + y];
				for (x = 0; x < gfx->width; x++)
				{
					int color = dp[x];
					entry[(color >> 4) & 3] |= 1 << (color & 15);
				}
			}
		}
	}

	pflookup = roadblst_pflookup;
	molookup = roadblst_molookup;
	pf_map_shift = 0;
	mo_map_shift = 0;
	return atarisys1_vh_start ();
}



/*************************************
 *
 *		Video system shutdown
 *
 *************************************/

void atarisys1_vh_stop (void)
{
	/* free bitmaps */
	if (playfieldbitmap)
		osd_free_bitmap (playfieldbitmap);
	playfieldbitmap = 0;
	if (tempbitmap)
		osd_free_bitmap (tempbitmap);
	tempbitmap = 0;

	/* free dirty buffers */
	if (pfmapped)
		free (pfmapped);
	pfmapped = 0;
	scrolllist = 0;

	/* free the RoadBlasters pen usage */
	if (roadblst_pen_usage)
		free (roadblst_pen_usage);
	roadblst_pen_usage = 0;
}



/*************************************
 *
 *		Graphics bank selection
 *
 *************************************/

void atarisys1_bankselect_w (int offset, int data)
{
	int oldword = READ_WORD (&atarisys1_bankselect[offset]);
	int newword = COMBINE_WORD (oldword, data);
	int diff = oldword ^ newword;

	/* update memory */
	WRITE_WORD (&atarisys1_bankselect[offset], newword);

	/* sound CPU reset */
	if (diff & 0x80)
	{
		if (data & 0x80)
			atarigen_sound_reset ();
		else
			cpu_halt (1, 0);
	}

	/* motion object bank select */
	atarisys1_update_display_list (cpu_getscanline ());

	/* playfield bank select */
	if (diff & 0x04)
	{
		int i, *pf;

		/* set the new bank globally */
		pfbank = (newword & 0x04) ? 0x80 : 0x00;

		/* and remap the entire playfield */
		for (i = atarigen_playfieldram_size / 2, pf = pfmapped; i >= 0; i--)
		{
			int val = READ_WORD (&atarigen_playfieldram[i * 2]);
			int map = pflookup[pfbank | ((val >> 8) & 0x7f)] | (val & 0xff) | ((val & 0x8000) << 9) | LDIRTYFLAG;
			*pf++ = map;
		}
	}
}



/*************************************
 *
 *		Playfield horizontal scroll
 *
 *************************************/

void atarisys1_hscroll_w (int offset, int data)
{
	int oldword = READ_WORD (&atarigen_hscroll[offset]);
	int newword = COMBINE_WORD (oldword, data);

	WRITE_WORD (&atarigen_hscroll[offset], newword);

	if (!offset && (oldword & 0x1ff) != (newword & 0x1ff))
	{
		int scrollval = ((oldword & 0x1ff) << 12) + (READ_WORD (&atarigen_vscroll[0]) & 0x1ff);
		int i, end = cpu_getscanline ();

		if (end > YDIM)
			end = YDIM;

		for (i = scrolllist_end; i < end; i++)
			scrolllist[i] = scrollval;
		scrolllist_end = end;
	}
}



/*************************************
 *
 *		Playfield vertical scroll
 *
 *************************************/

void atarisys1_vscroll_w (int offset, int data)
{
	int oldword = READ_WORD (&atarigen_vscroll[offset]);
	int newword = COMBINE_WORD (oldword, data);

	WRITE_WORD (&atarigen_vscroll[offset], newword);

	if (!offset && (oldword & 0x1ff) != (newword & 0x1ff))
	{
		int scrollval = ((READ_WORD (&atarigen_hscroll[0]) & 0x1ff) << 12) + (oldword & 0x1ff);
		int i, end = cpu_getscanline ();

		if (end > YDIM)
			end = YDIM;

		for (i = scrolllist_end; i < end; i++)
			scrolllist[i] = scrollval;
		scrolllist_end = end;
	}
}



/*************************************
 *
 *		Playfield RAM read/write handlers
 *
 *************************************/

int atarisys1_playfieldram_r (int offset)
{
	return READ_WORD (&atarigen_playfieldram[offset]);
}


void atarisys1_playfieldram_w (int offset, int data)
{
	int oldword = READ_WORD (&atarigen_playfieldram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		int map, oldmap;

		WRITE_WORD (&atarigen_playfieldram[offset], newword);

		/* remap it now and mark it dirty in the process */
		map = pflookup[pfbank | ((newword >> 8) & 0x7f)] | (newword & 0xff) | ((newword & 0x8000) << 9) | LDIRTYFLAG;
		oldmap = pfmapped[offset / 2];
		pfmapped[offset / 2] = map;
	}
}



/*************************************
 *
 *		Sprite RAM read/write handlers
 *
 *************************************/

int atarisys1_spriteram_r (int offset)
{
	return READ_WORD (&atarigen_spriteram[offset]);
}


void atarisys1_spriteram_w (int offset, int data)
{
	int oldword = READ_WORD (&atarigen_spriteram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&atarigen_spriteram[offset], newword);

		/* if modifying a timer, beware */
		if (((offset & 0x180) == 0x000 && READ_WORD (&atarigen_spriteram[offset | 0x080]) == 0xffff) ||
		    ((offset & 0x180) == 0x080 && newword == 0xffff))
		{
			/* if the timer is in the active bank, update the display list */
			if ((offset >> 9) == ((READ_WORD (&atarisys1_bankselect[0]) >> 3) & 7))
			{
				if (errorlog) fprintf (errorlog, "Caught timer mod!\n");
				atarisys1_update_display_list (cpu_getscanline ());
			}
		}
	}
}



/*************************************
 *
 *		Motion object interrupt handlers
 *
 *************************************/

void atarisys1_int3off_callback (int param)
{
	/* clear the state */
	int3_state = 0;

	/* clear the interrupt generated as well */
	cpu_clear_pending_interrupts (0);

	/* make this timer go away */
	int3off_timer = 0;
}


void atarisys1_int3_callback (int param)
{
	/* generate the interrupt */
	cpu_cause_interrupt (0, 3);

	/* update the state */
	int3_state = 1;

	/* set a timer to turn it off */
	if (int3off_timer)
		timer_remove (int3off_timer);
	int3off_timer = timer_set (cpu_getscanlineperiod (), 0, atarisys1_int3off_callback);

	/* set ourselves up to go off next frame */
	int3_timer[param] = timer_set (TIME_IN_HZ (Machine->drv->frames_per_second), param, atarisys1_int3_callback);
}


int atarisys1_int3state_r (int offset)
{
	return int3_state ? 0x0080 : 0x0000;
}



/*************************************
 *
 *		Motion object list handlers
 *
 *************************************/

void atarisys1_update_display_list (int scanline)
{
	int bank = ((READ_WORD (&atarisys1_bankselect[0]) >> 3) & 7) * 0x200;
	unsigned char *base = &atarigen_spriteram[bank];
	unsigned char spritevisit[64], timer[YDIM];
	int link = 0, i;

	/* generic update first */
	if (!scanline)
	{
		scrolllist_end = 0;
		atarigen_update_display_list (base, 0, 0);
	}
	else
		atarigen_update_display_list (base, 0, scanline + 1);

	/* visit all the sprites and look for timers */
	memset (spritevisit, 0, sizeof (spritevisit));
	memset (timer, 0, sizeof (timer));
	while (!spritevisit[link])
	{
		int data2 = READ_WORD (&base[link * 2 + 0x080]);

		/* a picture of 0xffff is really an interrupt - gross! */
		if (data2 == 0xffff)
		{
			int data1 = READ_WORD (&base[link * 2 + 0x000]);
			int vsize = (data1 & 15) + 1;
			int ypos = (256 - (data1 >> 5) - vsize * 8) & 0x1ff;

			/* only generate timers on visible scanlines */
			if (ypos < YDIM)
				timer[ypos] = 1;
		}

		/* link to the next object */
		spritevisit[link] = 1;
		link = READ_WORD (&atarigen_spriteram[bank + link * 2 + 0x180]) & 0x3f;
	}

	/* update our interrupt timers */
	for (i = 0; i < YDIM; i++)
	{
		if (timer[i] && !int3_timer[i])
			int3_timer[i] = timer_set (cpu_getscanlinetime (i), i, atarisys1_int3_callback);
		else if (!timer[i] && int3_timer[i])
		{
			timer_remove (int3_timer[i]);
			int3_timer[i] = 0;
		}
	}
}


/*
 *---------------------------------------------------------------------------------
 *
 * 	Motion Object encoding
 *
 *		4 16-bit words are used total
 *
 *		Word 1: Vertical position
 *
 *			Bits 0-3   = vertical size of the object, in tiles
 *			Bits 5-13  = vertical position
 *			Bit  15    = horizontal flip
 *
 *		Word 2: Image
 *
 *			Bits 0-15  = index of the image; the upper 8 bits are passed through a
 *			             pair of lookup PROMs to select which graphics bank and color
 *			              to use (high bit of color is ignored and comes from Bit 15, below)
 *
 *		Word 3: Horizontal position
 *
 *			Bits 0-3   = horizontal size of the object, in tiles
 *			Bits 5-13  = horizontal position
 *			Bit  15    = special playfield priority
 *
 *		Word 4: Link
 *
 *			Bits 0-5   = link to the next motion object
 *
 *---------------------------------------------------------------------------------
 */

void atarisys1_calc_mo_colors (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	unsigned char *colors = param;
	int lookup = molookup[data[1] >> 8];
	int color = LCOLOR (lookup);
	colors[color] = 1;
}

void atarisys1_render_mo (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	struct atarisys1_mo_data *modata = param;
	int y, sy, redraw_val;

	/* extract data from the various words */
	int pict = data[1];
	int lookup = molookup[pict >> 8];
	int vsize = (data[0] & 15) + 1;
	int xpos = data[2] >> 5;
	int ypos = 256 - (data[0] >> 5) - vsize * 8;
	int color = LCOLOR (lookup);
	int bank = LBANK (lookup);
	int hflip = data[0] & 0x8000;
	int hipri = data[2] & 0x8000;

	/* adjust the final coordinates */
	xpos &= 0x1ff;
	ypos &= 0x1ff;
	redraw_val = (xpos << 23) + (ypos << 14) + vsize;
	if (xpos >= XDIM) xpos -= 0x200;
	if (ypos >= YDIM) ypos -= 0x200;

	/* bail if X coordinate is out of range */
	if (xpos <= -8 || xpos >= XDIM)
		return;

	/* do we have a priority color active? */
	if (modata->pcolor)
	{
		int *redraw_list = modata->redraw_list, *redraw = modata->redraw;
		int *r;

		/* if so, add an entry to the redraw list for later */
		for (r = redraw_list; r < redraw; )
			if (*r++ == redraw_val)
				break;

		/* but only add it if we don't have a matching entry already */
		if (r == redraw)
		{
			*redraw++ = redraw_val;
			modata->redraw = redraw;
		}
	}

	/*
	 *
	 *      case 1: normal
	 *
	 */

	if (!hipri)
	{
		/* loop over the height */
		for (y = 0, sy = ypos; y < vsize; y++, sy += 8, pict++)
		{
			/* clip the Y coordinate */
			if (sy <= clip->min_y - 8)
				continue;
			else if (sy > clip->max_y)
				break;

			/* draw the sprite */
			drawgfx (bitmap, Machine->gfx[bank],
					LPICT (lookup) | (pict & 0xff), color,
					hflip, 0, xpos, sy, clip, TRANSPARENCY_PEN, 0);
		}
	}

	/*
	 *
	 *      case 2: translucency
	 *
	 */

	else
	{
		struct rectangle tclip;

		/* loop over the height */
		for (y = 0, sy = ypos; y < vsize; y++, sy += 8, pict++)
		{
			/* clip the Y coordinate */
			if (sy <= clip->min_y - 8)
				continue;
			else if (sy > clip->max_y)
				break;

			/* draw the sprite in bright pink on the real bitmap */
			drawgfx (bitmap, Machine->gfx[bank],
					LPICT (lookup) | (pict & 0xff), 0x30 << mo_map_shift,
					hflip, 0, xpos, sy, clip, TRANSPARENCY_PEN, 0);

			/* also draw the sprite normally on the temp bitmap */
			drawgfx (tempbitmap, Machine->gfx[bank],
					LPICT (lookup) | (pict & 0xff), 0x20 << mo_map_shift,
					hflip, 0, xpos, sy, clip, TRANSPARENCY_NONE, 0);
		}

		/* now redraw the playfield tiles over top of the sprite */
		redraw_playfield_chunk (tempbitmap, xpos, ypos, 1, vsize, TRANSPARENCY_PEN, 0, -1);

		/* finally, copy this chunk to the real bitmap */
		tclip.min_x = xpos;
		tclip.max_x = xpos + 7;
		tclip.min_y = ypos;
		tclip.max_y = ypos + vsize * 8 - 1;
		if (tclip.min_y < clip->min_y) tclip.min_y = clip->min_y;
		if (tclip.max_y > clip->max_y) tclip.max_y = clip->max_y;
		copybitmap (bitmap, tempbitmap, 0, 0, 0, 0, &tclip, TRANSPARENCY_THROUGH, palette_transparent_pen);
	}
}




/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/


/*
 *   playfield redraw function
 */

static void redraw_playfield_chunk (struct osd_bitmap *bitmap, int xpos, int ypos, int w, int h, int transparency, int transparency_color, int type)
{
	struct rectangle clip;
	int y, x;

	/* make a clip */
	clip.min_x = xpos;
	clip.max_x = xpos + w * 8 - 1;
	clip.min_y = ypos;
	clip.max_y = ypos + h * 8 - 1;

	/* round the positions */
	xpos = (xpos - xscroll) / 8;
	ypos = (ypos - yscroll) / 8;

	/* loop over the rows */
	for (y = ypos + h; y >= ypos; y--)
	{
		/* compute the scroll-adjusted y position */
		int sy = (y * 8 + yscroll) & 0x1ff;
		if (sy > 0x1f8) sy -= 0x200;

		/* loop over the columns */
		for (x = xpos + w; x >= xpos; x--)
		{
			/* compute the scroll-adjusted x position */
			int sx = (x * 8 + xscroll) & 0x1ff;
			if (sx > 0x1f8) sx -= 0x200;

			{
				/* process the data */
				int data = pfmapped[(y & 0x3f) * 64 + (x & 0x3f)];
				int color = LCOLOR (data);

				/* draw */
				if (type == -1)
					drawgfx (bitmap, Machine->gfx[LBANK (data)], LPICT (data), color, LFLIP (data), 0,
							sx, sy, &clip, transparency, transparency_color);
				else if (type == -2)
				{
					if (color == (16 >> pf_map_shift))
						drawgfx (bitmap, Machine->gfx[LBANK (data)], LPICT (data), color, LFLIP (data), 0,
								sx, sy, &clip, transparency, transparency_color);
				}
				else
					drawgfx (bitmap, Machine->gfx[LBANK (data)], LPICT (data), type, LFLIP (data), 0,
							sx, sy, &clip, transparency, transparency_color);
			}
		}
	}
}



/*************************************
 *
 *		Generic System 1 refresh
 *
 *************************************/

void atarisys1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	unsigned char mo_map[32], al_map[8], pf_map[32];
	int x, y, sx, sy, xoffs, yoffs, offs, i, *r;
	struct atarisys1_mo_data modata;
	int redraw_list[1024];


/*	int hidebank = atarisys1_debug ();*/


	/* reset color tracking */
	memset (mo_map, 0, sizeof (mo_map));
	memset (pf_map, 0, sizeof (pf_map));
	memset (al_map, 0, sizeof (al_map));
	memset (palette_used_colors, PALETTE_COLOR_UNUSED, Machine->drv->total_colors * sizeof(unsigned char));

	/* assign our special pen here */
	memset (&palette_used_colors[1024], PALETTE_COLOR_TRANSPARENT, 16);

	/* update color usage for the playfield */
	for (offs = 0; offs < 64*64; offs++)
	{
		int data = pfmapped[offs];
		int color = LCOLOR (data);
		pf_map[color] = 1;
	}

	/* update color usage for the mo's */
	atarigen_render_display_list (bitmap, atarisys1_calc_mo_colors, mo_map);

	/* update color usage for the alphanumerics */
	for (sy = 0; sy < YCHARS; sy++)
	{
		for (sx = 0, offs = sy * 64; sx < XCHARS; sx++, offs++)
		{
			int data = READ_WORD (&atarigen_alpharam[offs * 2]);
			int color = (data >> 10) & 7;
			al_map[color] = 1;
		}
	}

	/* rebuild the palette */
	for (i = 0; i < 32; i++)
	{
		if (pf_map[i])
			memset (&palette_used_colors[256 + i * (16 << pf_map_shift)], PALETTE_COLOR_USED, 16 << pf_map_shift);
		if (mo_map[i])
		{
			palette_used_colors[256 + i * (16 >> mo_map_shift)] = PALETTE_COLOR_TRANSPARENT;
			memset (&palette_used_colors[256 + i * (16 >> mo_map_shift) + 1], PALETTE_COLOR_USED, (16 >> mo_map_shift) - 1);
		}
	}
	for (i = 0; i < 8; i++)
		if (al_map[i])
			memset (&palette_used_colors[0 + i * 4], PALETTE_COLOR_USED, 4);

	/* always remap the transluscent colors */
	memset (&palette_used_colors[768], PALETTE_COLOR_USED, 16 >> mo_map_shift);

	if (palette_recalc ())
	{
		for (i = atarigen_playfieldram_size / 2 - 1; i >= 0; i--)
			pfmapped[i] |= LDIRTYFLAG;
	}



	/* load the scroll values from the start of the previous frame */
	if (scrolllist_end)
	{
		xscroll = scrolllist[0] >> 12;
		yscroll = scrolllist[0];
	}
	else
	{
		xscroll = READ_WORD (&atarigen_hscroll[0]);
		yscroll = READ_WORD (&atarigen_vscroll[0]);
	}
	xscroll = -(xscroll & 0x1ff);
	yscroll = -(yscroll & 0x1ff);


	/*
	 *---------------------------------------------------------------------------------
	 *
	 * 	Playfield encoding
	 *
	 *		1 16-bit word is used
	 *
	 *			Bits 0-14  = index of the image; a 16th bit is pulled from the playfield
	 *                    bank selection bit.  The upper 8 bits of this 16-bit index
	 *                    are passed through a pair of lookup PROMs to select which
	 *                    graphics bank and color to use
	 *			Bit  15    = horizontal flip
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
			int data;

			/* read the data word */
			sx = x & 63;
			offs = sy * 64 + sx;
			data = pfmapped[offs];

			/* rerender if dirty */
			if (LDIRTY (data))
			{
				int bank = LBANK (data);
				if (bank)
					drawgfx (playfieldbitmap, Machine->gfx[bank], LPICT (data), LCOLOR (data), LFLIP (data), 0,
							8*sx, 8*sy, 0, TRANSPARENCY_NONE, 0);
				pfmapped[offs] = data & ~LDIRTYFLAG;
			}
		}
	}

	/* copy the playfield to the destination */
	copyscrollbitmap (bitmap, playfieldbitmap, 1, &xscroll, 1, &yscroll, &Machine->drv->visible_area,
			TRANSPARENCY_NONE, 0);

	/* prepare the motion object data structure */
	modata.pcolor = READ_WORD (&atarisys1_prioritycolor[0]) & 0xff;
	modata.redraw_list = modata.redraw = redraw_list;

	/* render the motion objects */
	atarigen_render_display_list (bitmap, atarisys1_render_mo, &modata);

	/* redraw playfield tiles with higher priority */
	for (r = modata.redraw_list; r < modata.redraw; r++)
	{
		int val = *r;
		int xpos = (val >> 23) & 0x1ff;
		int ypos = (val >> 14) & 0x1ff;
		int h = val & 0x1f;

		redraw_playfield_chunk (bitmap, xpos, ypos, 1, h, TRANSPARENCY_PENS, ~modata.pcolor, -2);
	}

	/*
	 *---------------------------------------------------------------------------------
	 *
	 * 	Alpha layer encoding
	 *
	 *		1 16-bit word is used
	 *
	 *			Bits 0-9   = index of the character
	 *			Bits 10-12 = color
	 *			Bit  13    = transparency
	 *
	 *---------------------------------------------------------------------------------
	 */

	/* redraw the alpha layer completely */
	for (sy = 0; sy < YCHARS; sy++)
	{
		for (sx = 0, offs = sy*64; sx < XCHARS; sx++, offs++)
		{
			int data = READ_WORD (&atarigen_alpharam[offs*2]);
			int pict = (data & 0x3ff);

			if (pict || (data & 0x2000))
			{
				int color = ((data >> 10) & 7);

				drawgfx (bitmap, Machine->gfx[0],
						pict, color,
						0, 0,
						8*sx, 8*sy,
						0,
						(data & 0x2000) ? TRANSPARENCY_NONE : TRANSPARENCY_PEN, 0);
			}
		}
	}
}



/*************************************
 *
 *		Road Blasters refresh
 *
 *************************************/

void roadblst_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	unsigned char mo_map[32], al_map[8];
	unsigned short pf_map[32];
	int i, y, sx, sy, offs, shift, lasty, *scroll;
	struct atarisys1_mo_data modata;


/*	int hidebank = atarisys1_debug ();*/


	/* reset color tracking */
	memset (mo_map, 0, sizeof (mo_map));
	memset (pf_map, 0, sizeof (pf_map));
	memset (al_map, 0, sizeof (al_map));
	memset (palette_used_colors, PALETTE_COLOR_UNUSED, Machine->drv->total_colors * sizeof(unsigned char));

	/* assign our special pen here */
	memset (&palette_used_colors[1024], PALETTE_COLOR_TRANSPARENT, 16);

	/* update color usage for the playfield */
	for (offs = 0; offs < 64*64; offs++)
	{
		int data = pfmapped[offs];
		int color = LCOLOR (data);
		int bank = LBANK (data);
		int pict = LPICT (data);
		if (bank == 1)
		{
			pf_map[color * 4 + 0] |= roadblst_pen_usage[pict * 4 + 0];
			pf_map[color * 4 + 1] |= roadblst_pen_usage[pict * 4 + 1];
			pf_map[color * 4 + 2] |= roadblst_pen_usage[pict * 4 + 2];
			pf_map[color * 4 + 3] |= roadblst_pen_usage[pict * 4 + 3];
		}
		else
			pf_map[color] |= Machine->gfx[bank]->pen_usage[pict];
	}

	/* update color usage for the mo's */
	atarigen_render_display_list (bitmap, atarisys1_calc_mo_colors, mo_map);

	/* update color usage for the alphanumerics */
	for (sy = 0; sy < YCHARS; sy++)
	{
		for (sx = 0, offs = sy * 64; sx < XCHARS; sx++, offs++)
		{
			int data = READ_WORD (&atarigen_alpharam[offs * 2]);
			int color = (data >> 10) & 7;
			al_map[color] = 1;
		}
	}

	/* rebuild the palette */
	for (i = 0; i < 32; i++)
	{
		if (pf_map[i])
		{
			int j;
			for (j = 0; j < 16; j++)
				if (pf_map[i] & (1 << j))
					palette_used_colors[256 + i * 16 + j] = PALETTE_COLOR_USED;
		}
		if (mo_map[i])
		{
			palette_used_colors[256 + i * (16 >> mo_map_shift)] = PALETTE_COLOR_TRANSPARENT;
			memset (&palette_used_colors[256 + i * 16 + 1], PALETTE_COLOR_USED, 15);
		}
	}
	for (i = 0; i < 8; i++)
		if (al_map[i])
			memset (&palette_used_colors[0 + i * 4], PALETTE_COLOR_USED, 4);

	if (palette_recalc ())
	{
		for (i = atarigen_playfieldram_size / 2 - 1; i >= 0; i--)
			pfmapped[i] |= LDIRTYFLAG;
	}




	/*
	 *---------------------------------------------------------------------------------
	 *
	 * 	Playfield encoding
	 *
	 *		1 16-bit word is used
	 *
	 *			Bits 0-14  = index of the image; a 16th bit is pulled from the playfield
	 *                    bank selection bit.  The upper 8 bits of this 16-bit index
	 *                    are passed through a pair of lookup PROMs to select which
	 *                    graphics bank and color to use
	 *			Bit  15    = horizontal flip
	 *
	 *---------------------------------------------------------------------------------
	 */

	/* loop over the entire Y region */
	for (sy = offs = 0; sy < 64; sy++)
	{
		/* loop over the entire X region */
		for (sx = 0; sx < 64; sx++, offs++)
		{
			int data = pfmapped[offs];

			/* things only get tricky if we're not already dirty */
			if (LDIRTY (data))
			{
				int color = LCOLOR (data);
				int gfxbank = LBANK (data);

				/* draw this character */
				drawgfx (playfieldbitmap, Machine->gfx[gfxbank], LPICT (data), color, LFLIP (data), 0,
						8 * sx, 8 * sy, 0, TRANSPARENCY_NONE, 0);
				pfmapped[offs] = data & ~LDIRTYFLAG;
			}
		}
	}


	/* WARNING: this code won't work rotated! */
	shift = bitmap->depth / 8 - 1;
	lasty = -1;
	scroll = (int *)scrolllist;

	/* finish the scrolling list from the previous frame */
	xscroll = READ_WORD (&atarigen_hscroll[0]) & 0x1ff;
	yscroll = READ_WORD (&atarigen_vscroll[0]) & 0x1ff;
	offs = (xscroll << 12) + yscroll;
	for (y = scrolllist_end; y < YDIM; y++)
		scrolllist[y] = offs;

	/* loop over and copy the data row by row */
	for (y = 0; y < YDIM; y++)
	{
		int scrollx = (*scroll >> 12) & 0x1ff;
		int scrolly = *scroll++ & 0x1ff;
		int dy;

		/* when a write to the scroll register occurs, the counter is reset */
		if (scrolly != lasty)
		{
			offs = y;
			lasty = scrolly;
		}
		dy = (scrolly + y - offs) & 0x1ff;

		/* handle the wrap around case */
		if (scrollx + XDIM > 0x200)
		{
			int chunk = 0x200 - scrollx;
			memcpy (&bitmap->line[y][0], &playfieldbitmap->line[dy][scrollx << shift], chunk << shift);
			memcpy (&bitmap->line[y][chunk << shift], &playfieldbitmap->line[dy][0], (XDIM - chunk) << shift);
		}
		else
			memcpy (&bitmap->line[y][0], &playfieldbitmap->line[dy][scrollx << shift], XDIM << shift);
	}

	/* prepare the motion object data structure */
	modata.pcolor = 0;
	modata.redraw_list = modata.redraw = 0;

	/* render the motion objects */
	atarigen_render_display_list (bitmap, atarisys1_render_mo, &modata);

	/*
	 *---------------------------------------------------------------------------------
	 *
	 * 	Alpha layer encoding
	 *
	 *		1 16-bit word is used
	 *
	 *			Bits 0-9   = index of the character
	 *			Bits 10-12 = color
	 *			Bit  13    = transparency
	 *
	 *---------------------------------------------------------------------------------
	 */

	for (sy = 0; sy < YCHARS; sy++)
	{
		for (sx = 0, offs = sy*64; sx < XCHARS; sx++, offs++)
		{
			int data = READ_WORD (&atarigen_alpharam[offs*2]);
			int pict = (data & 0x3ff);

			if (pict || (data & 0x2000))
			{
				int color = ((data >> 10) & 7);

				drawgfx (bitmap, Machine->gfx[0],
						pict, color,
						0, 0,
						8*sx, 8*sy,
						0,
						(data & 0x2000) ? TRANSPARENCY_NONE : TRANSPARENCY_PEN, 0);
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
static int atarisys1_debug (void)
{
	static int lasttrans[0x200];
	int hidebank = -1;

	if (memcmp (lasttrans, &paletteram[0x600], 0x200))
	{
		static FILE *trans;
		int i;

		if (!trans) trans = fopen ("trans.log", "w");
		if (trans)
		{
			fprintf (trans, "\n\nTrans Palette:\n");
			for (i = 0x300; i < 0x400; i++)
			{
				fprintf (trans, "%04X ", READ_WORD (&paletteram[i*2]));
				if ((i & 15) == 15) fprintf (trans, "\n");
			}
		}
		memcpy (lasttrans, &paletteram[0x600], 0x200);
	}

	if (osd_key_pressed (OSD_KEY_Q)) hidebank = 0;
	if (osd_key_pressed (OSD_KEY_W)) hidebank = 1;
	if (osd_key_pressed (OSD_KEY_E)) hidebank = 2;
	if (osd_key_pressed (OSD_KEY_R)) hidebank = 3;
	if (osd_key_pressed (OSD_KEY_T)) hidebank = 4;
	if (osd_key_pressed (OSD_KEY_Y)) hidebank = 5;
	if (osd_key_pressed (OSD_KEY_U)) hidebank = 6;
	if (osd_key_pressed (OSD_KEY_I)) hidebank = 7;

	if (osd_key_pressed (OSD_KEY_A)) hidebank = 8;
	if (osd_key_pressed (OSD_KEY_S)) hidebank = 9;
	if (osd_key_pressed (OSD_KEY_D)) hidebank = 10;
	if (osd_key_pressed (OSD_KEY_F)) hidebank = 11;
	if (osd_key_pressed (OSD_KEY_G)) hidebank = 12;
	if (osd_key_pressed (OSD_KEY_H)) hidebank = 13;
	if (osd_key_pressed (OSD_KEY_J)) hidebank = 14;
	if (osd_key_pressed (OSD_KEY_K)) hidebank = 15;

	if (osd_key_pressed (OSD_KEY_9))
	{
		static int count;
		char name[50];
		FILE *f;
		int i, bank;

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
		for (i = 0x200; i < 0x300; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&paletteram[i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		fprintf (f, "\n\nTrans Palette:\n");
		for (i = 0x300; i < 0x400; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&paletteram[i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		bank = (READ_WORD (&atarisys1_bankselect[0]) >> 3) & 7;
		fprintf (f, "\n\nMotion Object Bank = %d\n", bank);
		bank *= 0x200;
		for (i = 0; i < 0x40; i++)
		{
			fprintf (f, "   Object %02X:  P=%04X  X=%04X  Y=%04X  L=%04X\n",
					i,
					READ_WORD (&atarigen_spriteram[bank+0x080+i*2]),
					READ_WORD (&atarigen_spriteram[bank+0x100+i*2]),
					READ_WORD (&atarigen_spriteram[bank+0x000+i*2]),
					READ_WORD (&atarigen_spriteram[bank+0x180+i*2])
			);
		}

		fprintf (f, "\n\nPlayfield dump\n");
		for (i = 0; i < atarigen_playfieldram_size / 2; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_playfieldram[i*2]));
			if ((i & 63) == 63) fprintf (f, "\n");
		}

		fclose (f);
	}

	return hidebank;
}
#endif
