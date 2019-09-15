/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char starfire_vidctrl;
unsigned char starfire_vidctrl1;
unsigned char *starfire_videoram;
unsigned char *starfire_colorram;
unsigned char starfire_color;

void starfire_vidctrl_w(int offset,int data) {
    starfire_vidctrl = data;
}

void starfire_vidctrl1_w(int offset,int data) {
    starfire_vidctrl1 = data;
}

void starfire_colorram_w(int offset,int data){
	int r,g,b;
	int bit0,bit1,bit2;

    starfire_color=data;

    if ((offset & 0xE0) == 0 && starfire_vidctrl & 0x40) {
        if (offset & 0x100)
            offset = (offset & 0x1F) | 0x20;
        else
            offset = offset & 0x1F;

        r = (data & 0x07);
        g = (data & 0x38) >> 3;
        b = (data & 0xC0) >> 6;

        bit0 = (r >> 0) & 0x01;
        bit1 = (r >> 1) & 0x01;
        bit2 = (r >> 2) & 0x01;
        r = 0x20 * bit0 + 0x40 * bit1 + 0x80 * bit2;
        bit0 = (g >> 0) & 0x01;
        bit1 = (g >> 1) & 0x01;
        bit2 = (g >> 2) & 0x01;
        g = 0x20 * bit0 + 0x40 * bit1 + 0x80 * bit2;
        bit0 = (b >> 0) & 0x01;
        bit1 = (b >> 1) & 0x01;
        bit2 = (b >> 2) & 0x01;
        b = 0x20 * bit0 + 0x40 * bit1 + 0x80 * bit2;

        palette_change_color(offset & 0x3f,r,g,b);
    }
}

int starfire_colorram_r(int offset) {
    return starfire_colorram[offset];

}

void starfire_videoram_w(int offset,int data) {

    unsigned char c,d,d2;
    int i;

    offset &= 0x1FFF;
    /* Handle selector 6A */
    if (offset & 0x2000)
        c = starfire_vidctrl;
    else
        c = starfire_vidctrl >> 4;

    /* Handle mirror bits in 5B-5E */
    d2=0;
    d = data & 0xFF;
    if (c & 0x01) {
        for (i=7; i>-1; i--) {
            d2 = d2 | ((d & 0x80) >> i);
            d = d << 1;
        }
    }
    else
        {d2 = d;}


    /* Handle shifters 6E,6D */

    i = (c & 0x0E) >> 1;
    switch (i) {
        case 0x00:
            d = d2;
            break;
        case 0x01:
            d = (d2 >> 1) | ((d2 & 0x01) << 7);
            break;
        case 0x02:
            d = (d2 >> 2) | ((d2 & 0x03) << 6);
            break;
        case 0x03:
            d = (d2 >> 3) | ((d2 & 0x07) << 5);
            break;
        case 0x04:
            d = (d2 >> 4) | ((d2 & 0x0F) << 4);
            break;
        case 0x05:
            d = (d2 >> 5) | ((d2 & 0x1F) << 3);
            break;
        case 0x06:
            d = (d2 >> 6) | ((d2 & 0x3F) << 2);
            break;
        case 0x07:
            d = (d2 >> 7) | ((d2 & 0x7F) << 1);
            break;
    }

    /* Handle ALU 8B,8D */
    switch (starfire_vidctrl1 & 0x0F) {
        case 0:
            break;
        case 1:
            d = (d | starfire_videoram[offset]);
            break;
        case 7:
            d = (d ^ 0xFF) | (starfire_videoram[offset]);
            break;
        case 0x0C:
            d = 0;
            break;
        case 0x0D:
            d = (d ^ 0xFF) &  (starfire_videoram[offset]);
            break;
        default:
            if (errorlog) fprintf(errorlog,"ALU: %x\n",(starfire_vidctrl1 & 0x0F));
            break;
    }
    if (d != 0xFF) {
//        interrupt();
    }
    starfire_videoram[offset] = d;
    starfire_colorram[offset] = starfire_color;
}

int starfire_videoram_r(int offset) {
    unsigned char d;

    offset &= 0x1FFF;
    d = starfire_videoram[offset];
    return (d << 1) | ((d & 0x01) << 7);

}


/***************************************************************************

  Convert the color PROMs into a more useable format.


***************************************************************************/
void starfire_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
    int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
        *(palette++) = ((i & 0x03) >> 0) * 0xff;
        *(palette++) = ((i & 0xc0) >> 2) * 0xff;
        *(palette++) = ((i & 0x30) >> 4) * 0xff;
	}


}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void starfire_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
        int x,y,xx;
        int j,col;
        long pnt;
        int d;

        pnt = 0x0000;
        xx=0;

        for (x=0; x<32; x++) {
            for (y=0; y<256; y++) {
                d= starfire_videoram[pnt];
                col = starfire_colorram[pnt++] & 0x0F;
//                col = 1;
                for (j=0; j<8; j++) {
                    if (d & 0x80)
                        tmpbitmap->line[y][xx+j] = Machine->pens[col+32];
                    else
                        tmpbitmap->line[y][xx+j] = Machine->pens[col];
                    d = d << 1;
                }
            }
            xx=xx+8;
       }
       copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_COLOR,16);
}

int starfire_vh_start(void)
{
    int i;

    if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
        return 1;

    if ((starfire_videoram = malloc(0x2000)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		return 1;
	}
    if ((starfire_colorram = malloc(0x2000)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
        free(starfire_videoram);
		return 1;
	}

    for (i=0; i<0x2000; i++) {
        starfire_videoram[i]=0;
        starfire_colorram[i]=0;
    }
    return 0;
}

void starfire_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
    free(starfire_videoram);
    free(starfire_colorram);
}

