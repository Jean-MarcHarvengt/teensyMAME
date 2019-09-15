/***************************************************************************

  bking2.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int xld1=0;
static int xld2=0;
static int xld3=0;
static int yld1=0;
static int yld2=0;
static int yld3=0;
static int msk=0;
static int safe=0;
static int cont1=0;
static int cont2=0;
static int cont3=0;
static int eport1=0;
static int eport2=0;
static int hitclr=0;

void bking2_xld1_w(int offset, int data)
{
    xld1 = data;
    if (errorlog) fprintf(errorlog, "xld1 = %d\n", xld1);
}

void bking2_yld1_w(int offset, int data)
{
    yld1 = data;
    if (errorlog) fprintf(errorlog, "yld1 = %d\n", yld1);
}

void bking2_xld2_w(int offset, int data)
{
    xld2 = data;
}

void bking2_yld2_w(int offset, int data)
{
    yld2 = data;
}

void bking2_xld3_w(int offset, int data)
{
    /* Feeds into Binary Up/Down counters.  Crow Inv changes counter direction */
    xld3 = data;
}

void bking2_yld3_w(int offset, int data)
{
    /* Feeds into Binary Up/Down counters.  Crow Inv changes counter direction */
    yld3 = data;
}

void bking2_msk_w(int offset, int data)
{
    msk = data;
}

void bking2_safe_w(int offset, int data)
{
    safe = data;
}

void bking2_cont1_w(int offset, int data)
{
    /* D0 = COIN LOCK */
    /* D1 =  BALL 5 */
    /* D2 = VINV */
    /* D3 = Not Connected */
    /* D4-D7 = CROW0-CROW3 (Selects crow picture) */
    cont1 = data;
}

void bking2_cont2_w(int offset, int data)
{
    /* D0-D2 = BALL10 - BALL12 (Selects player 1 ball picture) */
    /* D3-D5 = BALL20 - BALL22 (Selects player 2 ball picture) */
    /* D6 = HIT1 */
    /* D7 = HIT2 */
    cont2 = data;
}

void bking2_cont3_w(int offset, int data)
{
    /* D0 = CROW INV (inverts Crow picture and direction?) */
    /* D1-D2 = COLOR 0 - COLOR 1 (switches 4 color palettes, global across all graphics) */
    /* D3 = SOUND STOP */
    cont3 = data;
}

void bking2_eport1_w(int offset, int data)
{
    /* Output to sound DAC? */
    eport1 = data;
}

void bking2_eport2_w(int offset, int data)
{
    /* Some type of sound data? */
    eport2 = data;
}

void bking2_hitclr_w(int offset, int data)
{
    hitclr = data;
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void bking2_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
    int offs;

    /* for every character in the Video RAM, check if it has been modified */
    /* since last time and update it accordingly. */
    for (offs = (videoram_size/2) - 1;offs >= 0;offs--)
    {
        int offs2;

        offs2 = offs * 2;

        if (dirtybuffer[offs2])
        {
            int charcode;
            int charbank;
            int sx,sy;
            int flipx,flipy;

            dirtybuffer[offs2]=0;

            charcode = videoram[offs2];
            charbank = videoram[offs2+1] & 0x03;
            flipx = (videoram[offs2+1] & 0x04) >> 2;
            flipy = (videoram[offs2+1] & 0x08) >> 3;

            sx = 8 * (offs % 32);
            sy = 8 * (offs / 32);
            drawgfx(tmpbitmap,Machine->gfx[charbank],
                    charcode,
                    0, /* color */
                    flipx,flipy,sx,sy,
                    &Machine->drv->visible_area,TRANSPARENCY_NONE,0);
        }
    }

    /* copy the character mapped graphics */
    copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

    /* draw sprites */
    {
        int ball1,ball2;
        int flipx,flipy;

        flipx=flipy=0;

        ball1 = cont2 & 0x07;
        ball2 = (cont2 & 0x38) >> 3;

        drawgfx(bitmap,Machine->gfx[5],
                ball1,
                0, /* color */
                flipx,flipy,xld1,yld1,
                &Machine->drv->visible_area,TRANSPARENCY_PEN,0);

        drawgfx(bitmap,Machine->gfx[5],
                ball2,
                0, /* color */
                flipx,flipy,xld2,yld2,
                &Machine->drv->visible_area,TRANSPARENCY_PEN,0);

    }
}
