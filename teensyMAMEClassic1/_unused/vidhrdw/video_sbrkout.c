/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *sbrkout_horiz_ram;
unsigned char *sbrkout_vert_ram;

/***************************************************************************
***************************************************************************/

int sbrkout_vh_start(void)
{
        if (generic_vh_start()!=0)
                return 1;

        return 0;
}

/***************************************************************************
***************************************************************************/

void sbrkout_vh_stop(void)
{
        generic_vh_stop();
}

/***************************************************************************
***************************************************************************/

static int sbrkout_overlay_color(int sx, int sy)
{
        int x,y,color;

        x=sx/8;
        y=sy/8;

        if ((x>=27) && (x<=30))               color=2;
        else if ((x>=23) && (x<=26))          color=3;
        else if ((x>=19) && (x<=22))          color=4;
        else if ((x>=4) && (x<=18))           color=5;
        else                                  color=1;

        if ((y<1) || (y>26))                  color=1;

        return color;
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void sbrkout_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
        int offs;
        int ball;

    /* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
                if (dirtybuffer[offs])
                {
                        int charcode;
                        int sx,sy;
                        int color;

                        dirtybuffer[offs]=0;

                        charcode = videoram[offs] & 0x3F;

                        sx = 8*(offs % 32);
                        sy = 8*(offs / 32);

                        /* Check the "draw" bit */
                        if (((videoram[offs] & 0x80)>>7)==0x00)
                                color=0;
                        /* Choose the proper overlay color */
                        else
                                color=sbrkout_overlay_color(sx,sy);

                        drawgfx(tmpbitmap,Machine->gfx[0],
                                        charcode, color,
                                        0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
                }
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

        /* Draw each one of our three balls */
        for (ball=2;ball>=0;ball--)
        {
                int sx,sy;
                int color;
                int picture;
                int i;

                sx=31*8-sbrkout_horiz_ram[ball*2];
                sy=30*8-sbrkout_vert_ram[ball*2];
                /* There's 3 pictures per ball, since we split it into thirds for the color overlay */
                picture=((sbrkout_vert_ram[ball*2+1] & 0x80) >> 7)*3;

                for (i=0;i<3;i++)
                {
                    color=sbrkout_overlay_color(sx+i,sy);

                    drawgfx(bitmap,Machine->gfx[1],
                        picture+i,color,
                        0,0,sx+i,sy,
                        &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
                }
        }
}

