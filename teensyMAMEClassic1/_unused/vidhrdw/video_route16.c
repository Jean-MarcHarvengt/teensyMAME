/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *route16_sharedram;
unsigned char *route16_videoram1;
unsigned char *route16_videoram2;

static struct osd_bitmap *tmpbitmap1;
static struct osd_bitmap *tmpbitmap2;

static int video_flip;
static int video_color_select_1;
static int video_color_select_2;
static int video_disable_1 = 0;
static int video_disable_2 = 0;
static int video_remap_1;
static int video_remap_2;
static const unsigned char *route16_color_prom;
static int route16;

/* Local functions */
static void modify_pen(int pen, int colorindex);
static void common_videoram_w(int offset,int data,
                              int coloroffset, struct osd_bitmap *bitmap);



void route16_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	route16_color_prom = color_prom;	/* we'll need this later */
}


/***************************************************************************

  Set hardware dependent flag.

***************************************************************************/
void route16_set_machine_type(void)
{
    route16 = 1;
}

void stratvox_set_machine_type(void)
{
    route16 = 0;
}


/***************************************************************************

  Handle Stratovox's extra sound effects.

***************************************************************************/
void stratvox_samples_w (int offset,int data)
{
    /* Stratovox sends the following sequence of commands to start the samples:

       Explosion:   0x24  0x25  0x24
       Bonus Ship:  0x64  0x65  0x64

       I'm only looking for the middle value. */

    switch (data)
    {
    case 0xff:
        // Stop samples

        // Route 16 also writes this, but it would be a waste to
        // include the Sampleinterface just for that, so we'll just ignore it
        if (!route16) sample_stop(0);
        break;

    case 0x24:
    case 0x64:
        // Ignore these
        break;

    case 0x25:
        // Explosion
        sample_start(0,0,0);
        break;

    case 0x65:
        // Bonus Ship
        sample_start(0,1,0);
        break;

    default:
        // Shouldn't happen
        if (errorlog) fprintf(errorlog, "SN76477 Write %X02 to %X04\n", data, cpu_getpc());
    }
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int route16_vh_start(void)
{
        if ((tmpbitmap1 = osd_new_bitmap(Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
        {
                return 1;
        }

        if ((tmpbitmap2 = osd_new_bitmap(Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
        {
                osd_free_bitmap(tmpbitmap1);
                tmpbitmap1 = 0;
                return 1;
        }

        video_flip=0;
        video_color_select_1=0;
        video_color_select_2=0;
        video_disable_1=0;
        video_disable_2=0;
        video_remap_1=1;
        video_remap_2=1;

        /* Since we only modify the bitmap if the video ram contents changes,
        we need to initialize it to colors corresponding to the zeroed
        out video ram. */
        fillbitmap(tmpbitmap1,Machine->pens[0],0);
        fillbitmap(tmpbitmap2,Machine->pens[4],0);

        return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void route16_vh_stop(void)
{
        osd_free_bitmap(tmpbitmap1);
        osd_free_bitmap(tmpbitmap2);
}


/***************************************************************************
  route16_out0_w
***************************************************************************/
void route16_out0_w(int offset,int data)
{
        static int last_write = 0;

        if (data == last_write) return;

        video_disable_1 = ((data & 0x02) << 6) && route16;
        video_color_select_1 = ((data & 0x1F) << 2);

		/* Bit 5 is the coin counter. */
		coin_counter_w(0, data & 0x20);

        video_remap_1 = 1;
        last_write = data;
}

/***************************************************************************
  route16_out1_w
***************************************************************************/
void route16_out1_w(int offset,int data)
{
        static int last_write = 0;

        if (data == last_write) return;

        video_disable_2 = ((data & 0x02) << 6 ) && route16;
        video_color_select_2 = ((data & 0x1F) << 2);

        video_flip = (data & 0x20) >> 5;

        video_remap_2 = 1;
        last_write = data;
}

/***************************************************************************
  route16_sharedram_r
***************************************************************************/
int route16_sharedram_r(int offset)
{
        return route16_sharedram[offset];
}

/***************************************************************************
  route16_sharedram_w
***************************************************************************/
void route16_sharedram_w(int offset,int data)
{
        route16_sharedram[offset] = data;

        // 4313-4319 are used in Route 16 as triggers to wake the other CPU
        if (offset >= 0x0313 && offset <= 0x0319 && data == 0xff && route16)
        {
                // Let the other CPU run
                cpu_yield();
        }
}

/***************************************************************************
  route16_videoram1_r
***************************************************************************/
int  route16_videoram1_r(int offset)
{
        return route16_videoram1[offset];
}

/***************************************************************************
  route16_videoram2_r
***************************************************************************/
int  route16_videoram2_r(int offset)
{
        return route16_videoram1[offset];
}

/***************************************************************************
  route16_videoram1_w
***************************************************************************/
void route16_videoram1_w(int offset,int data)
{
        if (route16_videoram1[offset] == data) return;

        route16_videoram1[offset] = data;

        common_videoram_w(offset, data, 0, tmpbitmap1);
}

/***************************************************************************
  route16_videoram2_w
***************************************************************************/
void route16_videoram2_w(int offset,int data)
{
        if (route16_videoram2[offset] == data) return;

        route16_videoram2[offset] = data;

        common_videoram_w(offset, data, 4, tmpbitmap2);
}

/***************************************************************************
  common_videoram_w
***************************************************************************/
static void common_videoram_w(int offset,int data,
                              int coloroffset, struct osd_bitmap *bitmap)
{
        int x, y, color1, color2, color3, color4;

        y = (offset & 0xffc0) >> 6;
        x = 255 - ((offset & 0x3f) << 2);

        // flip?
        if (video_flip)
                osd_mark_dirty(255-y,255-x,255-y,258-x,0);
        else
                osd_mark_dirty(y,x-3,y,x,0);

        color1 = ((data & 0x80) >> 6) | ((data & 0x08) >> 3);
        color2 = ((data & 0x40) >> 5) | ((data & 0x04) >> 2);
        color3 = ((data & 0x20) >> 4) | ((data & 0x02) >> 1);
        color4 = ((data & 0x10) >> 3) | ((data & 0x01)     );

        bitmap->line[x    ][y] = Machine->pens[color4 | coloroffset];
        bitmap->line[x - 1][y] = Machine->pens[color3 | coloroffset];
        bitmap->line[x - 2][y] = Machine->pens[color2 | coloroffset];
        bitmap->line[x - 3][y] = Machine->pens[color1 | coloroffset];
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void route16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
        if (video_remap_1)
        {
                osd_mark_dirty(0,0,256,256,0);

                modify_pen(0, video_color_select_1 + 0);
                modify_pen(1, video_color_select_1 + 1);
                modify_pen(2, video_color_select_1 + 2);
                modify_pen(3, video_color_select_1 + 3);

                video_remap_1 = 0;
        }

        if (video_remap_2)
        {
                osd_mark_dirty(0,0,256,256,0);

                modify_pen(4, video_color_select_2 + 0);
                modify_pen(5, video_color_select_2 + 1);
                modify_pen(6, video_color_select_2 + 2);
                modify_pen(7, video_color_select_2 + 3);

                video_remap_2 = 0;
        }


        if (!video_disable_2)
        {
                copybitmap(bitmap,tmpbitmap2,video_flip,video_flip,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
        }

        if (!video_disable_1)
        {
                if (video_disable_2)
                        copybitmap(bitmap,tmpbitmap1,video_flip,video_flip,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
                else
                        copybitmap(bitmap,tmpbitmap1,video_flip,video_flip,0,0,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
        }
}

/***************************************************************************
  mofify_pen
 ***************************************************************************/
static void modify_pen(int pen, int colorindex)
{
	int r,g,b,color;

	color = route16_color_prom[colorindex];
	r = ((color & 1) ? 0xff : 0x00);
	g = ((color & 2) ? 0xff : 0x00);
	b = ((color & 4) ? 0xff : 0x00);
	palette_change_color(pen,r,g,b);
}
