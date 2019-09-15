/***************************************************************************
    meadows.c
	Video handler
	Dead Eye, Gypsy Juggler

	J. Buchmueller, June '98
****************************************************************************/

#include "vidhrdw/generic.h"

#define USE_OVERLAY

/* some constants to make life easier */
#define SCR_HORZ        32
#define SCR_VERT        30
#define CHR_HORZ        8
#define CHR_VERT        8
#define SPR_COUNT       4
#define SPR_HORZ        16
#define SPR_VERT        16
#define SPR_ADJUST_X    -18
#define SPR_ADJUST_Y    -14

static  int sprite_dirty[SPR_COUNT];    /* dirty flags */
static  int sprite_horz[SPR_COUNT];     /* x position */
static  int sprite_vert[SPR_COUNT];     /* y position */
static  int sprite_index[SPR_COUNT];    /* index 0x00..0x0f, prom 0x10, flip horz 0x20 */

/*************************************************************/
/* video handler start                                       */
/*************************************************************/
int     meadows_vh_start(void)
{
	videoram_size = 1024;
	if (generic_vh_start())
		return 1;
	return 0;
}

/*************************************************************/
/* video handler stop                                        */
/*************************************************************/
void    meadows_vh_stop(void)
{
	generic_vh_stop();
}

/*************************************************************/
/* draw dirty sprites                                        */
/*************************************************************/
static  void    meadows_draw_sprites(void)
{
int     i;
	for (i = 0; i < SPR_COUNT; i++)
	{
	int     x, y, n, p, f;
		if (!sprite_dirty[i])				/* sprite not dirty? */
			continue;
		sprite_dirty[i] = 0;
		x = sprite_horz[i];
		y = sprite_vert[i];
		n = sprite_index[i] & 0x0f; 		/* bit #0 .. #3 select sprite */
//		p = (sprite_index[i] >> 4) & 1; 	/* bit #4 selects prom ??? */
		p = i;								/* that fixes it for now :-/ */
		f = sprite_index[i] >> 5;			/* bit #5 flip vertical flag */
		drawgfx(tmpbitmap,
			Machine->gfx[p + 1],
			n, 0, f, 0, x, y,
			&Machine->drv->visible_area,
			TRANSPARENCY_PEN,0);
		osd_mark_dirty(x,y,x+15,y+15,1);
	}
}


/*************************************************************/
/* mark character cell dirty                                 */
/*************************************************************/
static  void    meadows_char_dirty(int x, int y)
{
int     i;
	osd_mark_dirty(x,y,x+7,y+7,1);
	/* scan sprites */
	for (i = 0; i < 4; i++)
	{
		/* check if sprite rectangle intersects with text rectangle */
		if ((x + 7 >= sprite_horz[i] && x <= sprite_horz[i] + 15) ||
			(y + 7 >= sprite_vert[i] && y <= sprite_vert[i] + 15))
				sprite_dirty[i] = 1;
	}
}

/*************************************************************/
/* deadeye screen refresh                                    */
/*************************************************************/
void deadeye_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
int     i;

#define W 0
#define G 1
#define B 2
#define Y 3
static  int deadeye_color_overlay[30][32] =
{
	{G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G}, /* invisible */
	{G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G}, /* invisible */
	{G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
	{G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
	{B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B},
	{B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B},
	{B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B},
	{B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B},
	{Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y},
	{Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y},
	{Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B},
	{B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B},
	{B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B},
	{B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B},
};


    /* the first two rows are invisible */
	for (i = 0; i < SCR_VERT * SCR_HORZ; i++)
	{
		if (dirtybuffer[i])
		{
		int     x, y, color;
			x = (i % SCR_HORZ);
			y = (i / SCR_HORZ);
			dirtybuffer[i] = 0;
#ifdef USE_OVERLAY
			color = deadeye_color_overlay[y][x];
#else
			color = W;
#endif

			x *= CHR_HORZ;
			y *= CHR_VERT;

			drawgfx(tmpbitmap,
				Machine->gfx[0],
				videoram[i] & 0x7f,
				color,
				0,0,
				x,y,
				&Machine->drv->visible_area,
				TRANSPARENCY_NONE,0);
			meadows_char_dirty(x,y);
		}
	}
	/* now draw the sprites */
	meadows_draw_sprites();

	copybitmap(bitmap,tmpbitmap,0,0,0,0,
		   &Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}

void gypsyjug_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
int     i;

#define W 0
#define G 1
#define B 2
#define Y 3

static  int gypsyjug_color_overlay[SCR_VERT][SCR_HORZ] =
{
	{Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y}, /* invisible */
	{Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y}, /* invisible */
	{Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y},
	{Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y},
	{G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G,G},
	{B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B},
	{B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B},
	{B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B,B},
	{Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y},
	{Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y},
	{Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,W,Y},
	{Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y},
	{Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y},
	{Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y},
};

    /* the first two are invisible */
	for (i = 0; i < SCR_VERT * SCR_HORZ; i++)
	{
		if (dirtybuffer[i])
		{
		int     x, y, color;
			x = (i % SCR_HORZ);
			y = (i / SCR_HORZ);
			dirtybuffer[i] = 0;
#ifdef USE_OVERLAY
			color = gypsyjug_color_overlay[y][x];
#else
			color = W;
#endif

			x *= CHR_HORZ;
			y *= CHR_VERT;

			drawgfx(tmpbitmap,
				Machine->gfx[0],
				videoram[i] & 0x7f,
				color, 0, 0, x, y,
				&Machine->drv->visible_area,
				TRANSPARENCY_NONE,0);
			meadows_char_dirty(x,y);
		}
	}

	/* now draw the sprites */
	meadows_draw_sprites();

        copybitmap(bitmap,tmpbitmap, 0, 0, 0, 0,
		   &Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}

/*************************************************************/
/*                                                           */
/* Video RAM write                                           */
/*                                                           */
/*************************************************************/
void    meadows_videoram_w(int offset, int data)
{
	if (offset >= videoram_size)
		return;
	if (videoram[offset] == data)
		return;
	videoram[offset] = data;
	dirtybuffer[offset] = 1;
}


/*************************************************************/
/* Mark sprite n covered area dirty                          */
/*************************************************************/
static  void    dirty_sprite(int n)
{
int     x, y;

	sprite_dirty[n] = 1;

	for (y = sprite_vert[n] / CHR_VERT;
		 y < (sprite_vert[n] + CHR_VERT - 1) / CHR_VERT + SPR_VERT / CHR_VERT ;
		 y++)
	{
		for (x = sprite_horz[n] / CHR_HORZ;
			 x < (sprite_horz[n] + CHR_HORZ - 1) / CHR_HORZ + SPR_HORZ / CHR_HORZ;
			 x++)
		{
			if (y >= 0 && y < SCR_VERT && x >= 0 && x < SCR_HORZ)
				dirtybuffer[y * SCR_HORZ + x] = 1;
		}
	}

//	if (errorlog) fprintf(errorlog, "sprite_dirty(%d) %d %d\n", n, sprite_horz[n], sprite_vert[n]);
}

/*************************************************************/
/* write to the sprite registers                             */
/*************************************************************/
void    meadows_sprite_w(int offset, int data)
{
int     n = offset % SPR_COUNT;
	switch (offset)
	{
		case 0: /* sprite 0 horz */
		case 1: /* sprite 1 horz */
		case 2: /* sprite 2 horz */
		case 3: /* sprite 3 horz */
			if (sprite_horz[n] != data)
			{
				dirty_sprite(n);
				sprite_horz[n] = data + SPR_ADJUST_X;
			}
			break;
		case 4: /* sprite 0 vert */
		case 5: /* sprite 1 vert */
		case 6: /* sprite 2 vert */
		case 7: /* sprite 3 vert */
			if (sprite_horz[n] != data)
			{
				dirty_sprite(n);
				sprite_vert[n] = data + SPR_ADJUST_Y;
			}
			break;
		case  8: /* prom 1 select + reverse shifter */
		case  9: /* prom 2 select + reverse shifter */
		case 10: /* ??? prom 3 select + reverse shifter */
		case 11: /* ??? prom 4 select + reverse shifter */
			if (sprite_index[n] != data)
			{
				dirty_sprite(n);
				sprite_index[n] = data;
			}
			break;
	}
}



