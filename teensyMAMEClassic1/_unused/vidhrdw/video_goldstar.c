/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *goldstar_video1, *goldstar_video2, *goldstar_video3;
int goldstar_video_size;
unsigned char *goldstar_scroll1, *goldstar_scroll2, *goldstar_scroll3;

static unsigned char *dirtybuffer1, *dirtybuffer2, *dirtybuffer3;
static struct osd_bitmap *tmpbitmap1, *tmpbitmap2, *tmpbitmap3;
static int bgcolor;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int goldstar_vh_start(void)
{
	int i;

	if (generic_vh_start() != 0)
		return 1;

	dirtybuffer1 = malloc(3 * goldstar_video_size * sizeof(unsigned char));
	dirtybuffer2 = dirtybuffer1 + goldstar_video_size;
	dirtybuffer3 = dirtybuffer2 + goldstar_video_size;

	/* the background area is half as high as the screen */
	tmpbitmap1 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height);
	tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height);
	tmpbitmap3 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height);

	if (!(tmpbitmap1 && tmpbitmap2 && tmpbitmap3 && dirtybuffer1))
	{
		if (tmpbitmap3) osd_free_bitmap(tmpbitmap3);
		if (tmpbitmap2) osd_free_bitmap(tmpbitmap2);
		if (tmpbitmap1) osd_free_bitmap(tmpbitmap1);
		free(dirtybuffer1);
		generic_vh_stop();
		return 1;
	}

	/* leave everything at the default, but map all foreground 0 pens as transparent */
	for (i = 0;i < 16;i++) palette_used_colors[8 * i] = PALETTE_COLOR_TRANSPARENT;

	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void goldstar_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap3);
	osd_free_bitmap(tmpbitmap2);
	osd_free_bitmap(tmpbitmap1);
	free(dirtybuffer1);
	generic_vh_stop();
}



void goldstar_video1_w(int offset, int data)
{
	if (goldstar_video1[offset] != data)
	{
		dirtybuffer1[offset] = 1;
		goldstar_video1[offset] = data;
	}
}
void goldstar_video2_w(int offset, int data)
{
	if (goldstar_video2[offset] != data)
	{
		dirtybuffer2[offset] = 1;
		goldstar_video2[offset] = data;
	}
}
void goldstar_video3_w(int offset, int data)
{
	if (goldstar_video3[offset] != data)
	{
		dirtybuffer3[offset] = 1;
		goldstar_video3[offset] = data;
	}
}



void goldstar_fa00_w(int offset,int data)
{
	/* bit 1 toggles continuously - might be irq enable or watchdog reset */

	/* bit 2 selects background gfx color (I think) */
	if (bgcolor != ((data & 0x04) >> 2))
	{
		bgcolor = (data & 0x04) >> 2;
		memset(dirtybuffer1,1,goldstar_video_size);
		memset(dirtybuffer2,1,goldstar_video_size);
		memset(dirtybuffer3,1,goldstar_video_size);
	}
}



struct rectangle visible1 = { 14*8, (14+48)*8-1,  4*8,  (4+7)*8-1 };
struct rectangle visible2 = { 14*8, (14+48)*8-1, 12*8, (12+7)*8-1 };
struct rectangle visible3 = { 14*8, (14+48)*8-1, 20*8, (20+7)*8-1 };

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void goldstar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	if (palette_recalc())
	{
		memset(dirtybuffer,1,videoram_size);
		memset(dirtybuffer1,1,goldstar_video_size);
		memset(dirtybuffer2,1,goldstar_video_size);
		memset(dirtybuffer3,1,goldstar_video_size);
	}

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs % 64;
			sy = offs / 64;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((colorram[offs] & 0xf0) << 4),
					colorram[offs] & 0x0f,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	for (offs = goldstar_video_size - 1;offs >= 0;offs--)
	{
		int sx = offs % 64;
		int sy = offs / 64;


		if (dirtybuffer1[offs])
		{
			dirtybuffer1[offs] = 0;

			drawgfx(tmpbitmap1,Machine->gfx[1],
					goldstar_video1[offs],
					bgcolor,
					0,0,
					sx*8,sy*32,
					0,TRANSPARENCY_NONE,0);
		}

		if (dirtybuffer2[offs])
		{
			dirtybuffer2[offs] = 0;

			drawgfx(tmpbitmap2,Machine->gfx[1],
					goldstar_video2[offs],
					bgcolor,
					0,0,
					sx*8,sy*32,
					0,TRANSPARENCY_NONE,0);
		}

		if (dirtybuffer3[offs])
		{
			dirtybuffer3[offs] = 0;

			drawgfx(tmpbitmap3,Machine->gfx[1],
					goldstar_video3[offs],
					bgcolor,
					0,0,
					sx*8,sy*32,
					0,TRANSPARENCY_NONE,0);
		}
	}

	{
		int i,scrolly[64];

		for (i= 0;i < 64;i++)
			scrolly[i] = -goldstar_scroll1[i];

		copyscrollbitmap(bitmap,tmpbitmap1,0,0,64,scrolly,&visible1,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&visible1,TRANSPARENCY_PEN,palette_transparent_pen);

		for (i= 0;i < 64;i++)
			scrolly[i] = -goldstar_scroll2[i];

		copyscrollbitmap(bitmap,tmpbitmap2,0,0,64,scrolly,&visible2,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&visible2,TRANSPARENCY_PEN,palette_transparent_pen);

		for (i= 0;i < 64;i++)
			scrolly[i] = -goldstar_scroll3[i];

		copyscrollbitmap(bitmap,tmpbitmap3,0,0,64,scrolly,&visible3,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&visible3,TRANSPARENCY_PEN,palette_transparent_pen);
	}
}
