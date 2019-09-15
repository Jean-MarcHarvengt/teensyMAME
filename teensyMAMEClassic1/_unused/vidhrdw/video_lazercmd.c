/*************************************************************/
/*                                                           */
/* Lazer Command video handler                               */
/*                                                           */
/*************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/lazercmd.h"

extern  int lazercmd_latches[32];

static  int overlay = 0;

/* scale a markers vertical position */
static  int vert_scale(int data)
{
    return (data & 3) + ((data & 4) ? 6 : 0) + ((data & ~7) >> 3) * VERT_CHR;
}

/* return the (overlay) color for coord x, y */
static  int x_y_color(int x, int y)
{
int color = 2;
    if (overlay)
	{
			/* left mustard yellow, right jade green */
			color  = (x < 16*HORZ_CHR) ? 0 : 1;
			/* but swapped in first and last lines */
			if ((y < 1*VERT_CHR) || (y > 21*VERT_CHR))
					color ^= 1;
	}
	return color;
}

/* mark the character occupied by the left (0) or right (1) marker dirty */
void    lazercmd_marker_dirty(int marker)
{
int x, y;
	if (marker)
	{
		x = MARKER_HORZ_ADJ + lazercmd_latches[MARKER_HORZ_R];
		y = MARKER_VERT_ADJ + vert_scale(lazercmd_latches[MARKER_VERT_R]);
	}
	else
	{
		x = MARKER_HORZ_ADJ + lazercmd_latches[MARKER_HORZ_L];
		y = MARKER_VERT_ADJ + vert_scale(lazercmd_latches[MARKER_VERT_L]);
    }
	if (x < 0 || x >= HORZ_RES * HORZ_CHR)
		return;
	if (y < 0 || y >= VERT_RES * VERT_CHR)
        return;
	/* mark all occupied character positions dirty */
    dirtybuffer[(y+0)/VERT_CHR * HORZ_RES + (x+0)/HORZ_CHR] = 1;
	dirtybuffer[(y+3)/VERT_CHR * HORZ_RES + (x+0)/HORZ_CHR] = 1;
	dirtybuffer[(y+0)/VERT_CHR * HORZ_RES + (x+3)/HORZ_CHR] = 1;
	dirtybuffer[(y+3)/VERT_CHR * HORZ_RES + (x+3)/HORZ_CHR] = 1;
}

/* plot a bitmap pattern (for displaying markers) */
static  void plot_pattern(int x, int y, int pattern)
{
int bit;
	if (y < 0 || y >= VERT_RES * VERT_CHR)
		return;
	for (bit = 0; bit < 3; bit++)
	{
		if (x+bit < 0 || x+bit >= HORZ_RES * HORZ_CHR)
			continue;
		if (!(pattern & (4 >> bit)))
			tmpbitmap->line[y][x+bit] = x_y_color(x+bit,y);
	}
}

void lazercmd_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
int     i;

        if (overlay != (input_port_2_r(0) & 0x80))
        {
                overlay = input_port_2_r(0) & 0x80;
                memset(dirtybuffer, 1, videoram_size);
        }

        /* The first row of characters is invsisible */
		for (i = 0; i < (VERT_RES - 1) * HORZ_RES; i++)
        {
                if (dirtybuffer[i])
                {
				int 	x, y;

                        dirtybuffer[i] = 0;

                        x = i % HORZ_RES;
                        y = i / HORZ_RES;

                        x *= HORZ_CHR;
                        y *= VERT_CHR;

                        drawgfx(tmpbitmap,
                                Machine->gfx[0],
								videoram[i],
								Machine->colortable[x_y_color(x,y)],
                                0,0, x,y,
                                &Machine->drv->visible_area,
                                TRANSPARENCY_NONE,0);
                        osd_mark_dirty(x,y,x+HORZ_CHR-1,y+VERT_CHR-1,1);
                }
        }
		if (lazercmd_latches[MARKER_ACTIVE_L] & 0x10)
		{
		int x, y;
			x = MARKER_HORZ_ADJ + lazercmd_latches[MARKER_HORZ_L];
			y = MARKER_VERT_ADJ + vert_scale(lazercmd_latches[MARKER_VERT_L]);
			plot_pattern(x,y+0,0);
			plot_pattern(x,y+1,2);
			plot_pattern(x,y+2,0);
        }
		if (lazercmd_latches[MARKER_ACTIVE_R] & 0x10)
		{
		int x, y;
			x = MARKER_HORZ_ADJ + lazercmd_latches[MARKER_HORZ_R];
			y = MARKER_VERT_ADJ + vert_scale(lazercmd_latches[MARKER_VERT_R]);
            plot_pattern(x,y+0,0);
			plot_pattern(x,y+1,0);
			plot_pattern(x,y+2,0);
        }
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}

