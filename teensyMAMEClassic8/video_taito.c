/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *taito_videoram2,*taito_videoram3;
unsigned char *taito_characterram;
unsigned char *taito_scrollx1,*taito_scrollx2,*taito_scrollx3;
unsigned char *taito_scrolly1,*taito_scrolly2,*taito_scrolly3;
unsigned char *taito_colscrolly1,*taito_colscrolly2,*taito_colscrolly3;
unsigned char *taito_gfxpointer;
unsigned char *taito_colorbank,*taito_video_priority;
static unsigned char *dirtybuffer2,*dirtybuffer3;
static struct osd_bitmap *tmpbitmap2,*tmpbitmap3;
static unsigned char dirtycharacter1[256],dirtycharacter2[256];
static unsigned char dirtysprite1[64],dirtysprite2[64];
static int taito_video_enable;
static int flipscreen[2];



/***************************************************************************

  Convert the color PROMs into a more useable format.

  The Taito games don't have a color PROM. They use RAM to dynamically
  create the palette. The resolution is 9 bit (3 bits per gun).

  The RAM is connected to the RGB output this way:

  bit 0 -- inverter -- 220 ohm resistor  -- RED
  bit 7 -- inverter -- 470 ohm resistor  -- RED
        -- inverter -- 1  kohm resistor  -- RED
        -- inverter -- 220 ohm resistor  -- GREEN
        -- inverter -- 470 ohm resistor  -- GREEN
        -- inverter -- 1  kohm resistor  -- GREEN
        -- inverter -- 220 ohm resistor  -- BLUE
        -- inverter -- 470 ohm resistor  -- BLUE
  bit 0 -- inverter -- 1  kohm resistor  -- BLUE

***************************************************************************/
void taito_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* all gfx elements use the same palette */
	for (i = 0;i < 64;i++)
	{
		COLOR(0,i) = i;
		/* we create both a "normal" lookup table and one where pen 0 is */
		/* always mapped to color 0. This is needed for transparency. */
		if (i % 8 == 0) COLOR(0,i + 64) = 0;
		else COLOR(0,i + 64) = i;
	}
}



void taito_paletteram_w(int offset,int data)
{
	int bit0,bit1,bit2;
	int r,g,b,val;


	paletteram[offset] = data;

	/* red component */
	val = paletteram[offset | 1];
	bit0 = (~val >> 6) & 0x01;
	bit1 = (~val >> 7) & 0x01;
	val = paletteram[offset & ~1];
	bit2 = (~val >> 0) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	/* green component */
	val = paletteram[offset | 1];
	bit0 = (~val >> 3) & 0x01;
	bit1 = (~val >> 4) & 0x01;
	bit2 = (~val >> 5) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	/* blue component */
	val = paletteram[offset | 1];
	bit0 = (~val >> 0) & 0x01;
	bit1 = (~val >> 1) & 0x01;
	bit2 = (~val >> 2) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	palette_change_color(offset / 2,r,g,b);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int taito_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(videoram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,1,videoram_size);

	if ((dirtybuffer3 = malloc(videoram_size)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer3,1,videoram_size);

	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer3);
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	if ((tmpbitmap3 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(tmpbitmap2);
		free(dirtybuffer3);
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	flipscreen[0] = flipscreen[1] = 0;

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void taito_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap3);
	osd_free_bitmap(tmpbitmap2);
	free(dirtybuffer3);
	free(dirtybuffer2);
	generic_vh_stop();
}



int taito_gfxrom_r(int offset)
{
	int offs;


	offs = taito_gfxpointer[0]+taito_gfxpointer[1]*256;

	taito_gfxpointer[0]++;
	if (taito_gfxpointer[0] == 0) taito_gfxpointer[1]++;

	if (offs < 0x8000)
		return Machine->memory_region[2][offs];
	else return 0;
}



void taito_videoram2_w(int offset,int data)
{
	if (taito_videoram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		taito_videoram2[offset] = data;
	}
}



void taito_videoram3_w(int offset,int data)
{
	if (taito_videoram3[offset] != data)
	{
		dirtybuffer3[offset] = 1;

		taito_videoram3[offset] = data;
	}
}



void taito_colorbank_w(int offset,int data)
{
	if (taito_colorbank[offset] != data)
	{
if (errorlog) fprintf(errorlog,"colorbank %d = %02x\n",offset,data);
		memset(dirtybuffer,1,videoram_size);
		memset(dirtybuffer2,1,videoram_size);
		memset(dirtybuffer3,1,videoram_size);

		taito_colorbank[offset] = data;
	}
}



void taito_videoenable_w(int offset,int data)
{
	if (taito_video_enable != data)
	{
if (errorlog) fprintf(errorlog,"videoenable = %02x\n",data);


		if ((taito_video_enable & 3) != (data & 3))
		{
			flipscreen[0] = data & 1;
			flipscreen[1] = data & 2;

			memset(dirtybuffer,1,videoram_size);
			memset(dirtybuffer2,1,videoram_size);
			memset(dirtybuffer3,1,videoram_size);
		}

		taito_video_enable = data;
	}
}



void taito_characterram_w(int offset,int data)
{
	if (taito_characterram[offset] != data)
	{
		if (offset < 0x1800)
		{
			dirtycharacter1[(offset / 8) & 0xff] = 1;
			dirtysprite1[(offset / 32) & 0x3f] = 1;
		}
		else
		{
			dirtycharacter2[(offset / 8) & 0xff] = 1;
			dirtysprite2[(offset / 32) & 0x3f] = 1;
		}

		taito_characterram[offset] = data;
	}
}


/***************************************************************************

  As if the hardware weren't complicated enough, it also has built-in
  collision detection.

  For Alpine Ski, hack offset 1 to return collision half of the time. Otherwise
  offset 3 will be ignored, because all collision detection registers are read
  and copied by the game in order, then the values checked, starting with offset 1.

***************************************************************************/
int taito_collision_detection_r(int offset)
{
	static int alternate_hack = 0;
	extern struct GameDriver frontlin_driver;
	extern struct GameDriver alpine_driver;
	extern struct GameDriver alpinea_driver;

	if (Machine->gamedrv == &alpine_driver || Machine->gamedrv == &alpinea_driver)
	{
		if (offset == 1)	/* simulate collision with other skiers, snowmobile */
			if ((alternate_hack ^= 1) == 1)
				return 0x0f;
		if (offset == 3)	/* simulate collision with rocks, trees, ice, flags, points */
				return 0x0f;
	}
	else if (Machine->gamedrv == &frontlin_driver && (offset == 1 || offset == 2))
		return 0xff;	/* simulate collision for Front Line */

	return 0;
}


void taito_collision_detection_w(int offset,int data)
{
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.


  I call the three planes with the conventional names "front", "middle" and
  "back", because that's their default order, but they can be arranged,
  together with the sprites, in any order. The priority is selected by
  register 0xd300, which works as follow:

  bits 0-3 go to A4-A7 of a 256x4 PROM
  bit 4 selects D0/D1 or D2/D3 of the PROM
  bit 5-7 n.c.
  A0-A3 of the PROM is fed with a mask of the inactive planes
  (i.e. all-zero) in the order sprites-front-middle-back
  the 2-bit code which comes out from the PROM selects the plane
  to display.

  Here is a dump of the PROM; on the right is the resulting order
  (s = sprites f = front m = middle b = back)

                                                        d300 pri    d300 pri
  00: 08 09 08 0A 00 05 00 0F 08 09 08 0A 00 05 00 0F |  00  sfmb    10  msfb
  10: 08 09 08 0B 00 0D 00 0F 08 09 08 0A 00 05 00 0F |  01  sfbm    11  msbf
  20: 08 0A 08 0A 04 05 00 0F 08 0A 08 0A 04 05 00 0F |  02  smfb    12  mfsb
  30: 08 0A 08 0A 04 07 0C 0F 08 0A 08 0A 04 05 00 0F |  03  smbf    13  mfbs
  40: 08 0B 08 0B 0C 0F 0C 0F 08 09 08 0A 00 05 00 0F |  04  sbfm    14  mbsf
  50: 08 0B 08 0B 0C 0F 0C 0F 08 0A 08 0A 04 05 00 0F |  05  sbmf    15  mbfs
  60: 0D 0D 0C 0E 0D 0D 0C 0F 01 05 00 0A 01 05 00 0F |  06  fsmb    16  bsfm
  70: 0D 0D 0C 0F 0D 0D 0C 0F 01 09 00 0A 01 05 00 0F |  07  fsbm    17  bsmf
  80: 0D 0D 0E 0E 0D 0D 0C 0F 05 05 02 0A 05 05 00 0F |  08  fmsb    18  bfsm
  90: 0D 0D 0E 0E 0D 0D 0F 0F 05 05 0A 0A 05 05 00 0F |  09  fmbs    19  bfms
  A0: 0D 0D 0F 0F 0D 0D 0F 0F 09 09 08 0A 01 05 00 0F |  0A  fbsm    1A  bmsf
  B0: 0D 0D 0F 0F 0D 0D 0F 0F 09 09 0A 0A 05 05 00 0F |  0B  fbms    1B  bmfs
  C0: 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F |  0C   -      1C   -
  D0: 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F |  0D   -      1D   -
  E0: 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F |  0E   -      1E   -
  F0: 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F 0F |  0F   -      1F   -

***************************************************************************/
static int draworder[32][4] =
{
	{ 3,2,1,0 },{ 2,3,1,0 },{ 3,1,2,0 },{ 1,3,2,0 },{ 2,1,3,0 },{ 1,2,3,0 },
	{ 3,2,0,1 },{ 2,3,0,1 },{ 3,0,2,1 },{ 0,3,2,1 },{ 2,0,3,1 },{ 0,2,3,1 },
	{ 3,3,3,3 },{ 3,3,3,3 },{ 3,3,3,3 },{ 3,3,3,3 },
	{ 3,1,0,2 },{ 1,3,0,2 },{ 3,0,1,2 },{ 0,3,1,2 },{ 1,0,3,2 },{ 0,1,3,2 },
	{ 2,1,0,3 },{ 1,2,0,3 },{ 2,0,1,3 },{ 0,2,1,3 },{ 1,0,2,3 },{ 0,1,2,3 },
	{ 3,3,3,3 },{ 3,3,3,3 },{ 3,3,3,3 },{ 3,3,3,3 },
};


static void drawsprites(struct osd_bitmap *bitmap)
{
	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	if (taito_video_enable & 0x80)
	{
		int offs;


		for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
		{
			int sx,sy,flipx,flipy;


			sx = ((spriteram[offs] + 13) & 0xff) - 14;	/* ?? */
			sy = 240 - spriteram[offs + 1];
			flipx = spriteram[offs + 2] & 1;
			flipy = spriteram[offs + 2] & 2;
			if (flipscreen[0])
			{
				sx = 238 - sx;
				flipx = !flipx;
			}
			if (flipscreen[1])
			{
				sy = 242 - sy;
				flipy = !flipy;
			}

			drawgfx(bitmap,Machine->gfx[(spriteram[offs + 3] & 0x40) ? 3 : 1],
					spriteram[offs + 3] & 0x3f,
					2 * ((taito_colorbank[1] >> 4) & 0x03) + ((spriteram[offs + 2] >> 2) & 1),
					flipx,flipy,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}


static void drawplayfield1(struct osd_bitmap *bitmap)
{
	if (taito_video_enable & 0x10)
	{
		int i,scrollx,scrolly[32];


		scrollx = *taito_scrollx1;
		if (flipscreen[0])
			scrollx = (scrollx & 0xf8) + ((scrollx) & 7) + 3;
		else
			scrollx = -(scrollx & 0xf8) + ((scrollx) & 7) + 3;

		if (flipscreen[1])
		{
			for (i = 0;i < 32;i++)
				scrolly[31-i] = taito_colscrolly1[i] + *taito_scrolly1;
		}
		else
		{
			for (i = 0;i < 32;i++)
				scrolly[i] = -taito_colscrolly1[i] - *taito_scrolly1;
		}

		copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,32,scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}
}


static void drawplayfield2(struct osd_bitmap *bitmap)
{
	if (taito_video_enable & 0x20)
	{
		int i,scrollx,scrolly[32];


		scrollx = *taito_scrollx2;
		if (flipscreen[0])
			scrollx = (scrollx & 0xf8) + ((scrollx+1) & 7) + 10;
		else
			scrollx = -(scrollx & 0xf8) + ((scrollx+1) & 7) + 10;

		if (flipscreen[1])
		{
			for (i = 0;i < 32;i++)
				scrolly[31-i] = taito_colscrolly2[i] + *taito_scrolly2;
		}
		else
		{
			for (i = 0;i < 32;i++)
				scrolly[i] = -taito_colscrolly2[i] - *taito_scrolly2;
		}

		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,32,scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}
}


static void drawplayfield3(struct osd_bitmap *bitmap)
{
	if (taito_video_enable & 0x40)
	{
		int i,scrollx,scrolly[32];


		scrollx = *taito_scrollx3;
		if (flipscreen[0])
			scrollx = (scrollx & 0xf8) + ((scrollx-1) & 7) + 12;
		else
			scrollx = -(scrollx & 0xf8) + ((scrollx-1) & 7) + 12;

		if (flipscreen[1])
		{
			for (i = 0;i < 32;i++)
				scrolly[31-i] = taito_colscrolly3[i] + *taito_scrolly3;
		}
		else
		{
			for (i = 0;i < 32;i++)
				scrolly[i] = -taito_colscrolly3[i] - *taito_scrolly3;
		}

		copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,32,scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}
}



static void drawplane(int n,struct osd_bitmap *bitmap)
{
	switch (n)
	{
		case 0:
			drawsprites(bitmap);
			break;
		case 1:
			drawplayfield1(bitmap);
			break;
		case 2:
			drawplayfield2(bitmap);
			break;
		case 3:
			drawplayfield3(bitmap);
			break;
	}
}



void taito_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs,i;


	/* decode modified characters */
	for (offs = 0;offs < 256;offs++)
	{
		if (dirtycharacter1[offs] == 1)
		{
			decodechar(Machine->gfx[0],offs,taito_characterram,Machine->drv->gfxdecodeinfo[0].gfxlayout);
			dirtycharacter1[offs] = 0;
		}
		if (dirtycharacter2[offs] == 1)
		{
			decodechar(Machine->gfx[2],offs,taito_characterram + 0x1800,Machine->drv->gfxdecodeinfo[2].gfxlayout);
			dirtycharacter2[offs] = 0;
		}
	}
	/* decode modified sprites */
	for (offs = 0;offs < 64;offs++)
	{
		if (dirtysprite1[offs] == 1)
		{
			decodechar(Machine->gfx[1],offs,taito_characterram,Machine->drv->gfxdecodeinfo[1].gfxlayout);
			dirtysprite1[offs] = 0;
		}
		if (dirtysprite2[offs] == 1)
		{
			decodechar(Machine->gfx[3],offs,taito_characterram + 0x1800,Machine->drv->gfxdecodeinfo[3].gfxlayout);
			dirtysprite2[offs] = 0;
		}
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;
			if (flipscreen[0]) sx = 31 - sx;
			if (flipscreen[1]) sy = 31 - sy;

			drawgfx(tmpbitmap,Machine->gfx[taito_colorbank[0] & 0x08 ? 2 : 0],
					videoram[offs],
					(taito_colorbank[0] & 0x07) + 8,	/* use transparent pen 0 */
					flipscreen[0],flipscreen[1],
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}

		if (dirtybuffer2[offs])
		{
			int sx,sy;


			dirtybuffer2[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;
			if (flipscreen[0]) sx = 31 - sx;
			if (flipscreen[1]) sy = 31 - sy;

			drawgfx(tmpbitmap2,Machine->gfx[taito_colorbank[0] & 0x80 ? 2 : 0],
					taito_videoram2[offs],
					((taito_colorbank[0] >> 4) & 0x07) + 8,	/* use transparent pen 0 */
					flipscreen[0],flipscreen[1],
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}

		if (dirtybuffer3[offs])
		{
			int sx,sy;


			dirtybuffer3[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;
			if (flipscreen[0]) sx = 31 - sx;
			if (flipscreen[1]) sy = 31 - sy;

			drawgfx(tmpbitmap3,Machine->gfx[taito_colorbank[1] & 0x08 ? 2 : 0],
					taito_videoram3[offs],
					(taito_colorbank[1] & 0x07) + 8,	/* use transparent pen 0 */
					flipscreen[0],flipscreen[1],
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* first of all, fill the screen with the background color */
	fillbitmap(bitmap,Machine->gfx[0]->colortable[8 * (taito_colorbank[1] & 0x07)],
			&Machine->drv->visible_area);

	for (i = 0;i < 4;i++)
		drawplane(draworder[*taito_video_priority & 0x1f][i],bitmap);
}
