/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *segar_characterram;
unsigned char *segar_characterram2;
unsigned char *segar_mem_colortable;
unsigned char *segar_mem_bcolortable;
unsigned char *segar_mem_blookup;
unsigned char *segar_mem_slookup;

static unsigned char segar_dirtycharacter[256];

static unsigned char segar_colorRAM[0x40];
static unsigned char segar_colorwrite=0;
static unsigned char segar_refresh=0;
static unsigned char segar_cocktail=0;
static unsigned char segar_char_refresh=0;

static unsigned char segar_bcolorRAM[0x40];
static unsigned char segar_bcolorwrite=0;
static unsigned char segar_has_background=0;

static unsigned char segar_bcocktail=0;
static unsigned char segar_brefresh=0;
static unsigned char segar_backfill=0;
static unsigned char segar_fill_background=0;
static unsigned int segar_backshift=0;
static unsigned int segar_backscene=0;
static unsigned int segar_backoffset=0;

static struct osd_bitmap *horizbackbitmap;
static struct osd_bitmap *vertbackbitmap;

/***************************************************************************

  The Sega raster games don't have a color PROM.  Instead, it has a color RAM
  that can be filled with bytes of the form BBGGGRRR.  We'll still build up
  an initial palette, and set our colortable to point to a different color
  for each entry in the colortable, which we'll adjust later using
  osd_modify_pen.

***************************************************************************/

void segar_init_colors(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
{
	static unsigned char color_scale[] = {0x00, 0x40, 0x80, 0xC0 };
	int i;

	/* Our first color needs to be 0 */
	*(palette++) = 0;
	*(palette++) = 0;
	*(palette++) = 0;

	/* For 005, Astro Blaster, and Monster Bash, it doesn't matter */
	/* what we set these colors to, since they'll get overwritten. */
	/* Space Odyssey uses a static palette for the background, so */
	/* our choice of colors isn't exactly arbitrary.  S.O. uses a */
	/* 6-bit color setup, so we make sure that every 0x40 colors */
	/* gets a nice 6-bit palette. */

	for (i = 0;i < (Machine->drv->total_colors - 1);i++)
	{
		*(palette++) = color_scale[((i & 0x30) >> 4)];
		*(palette++) = color_scale[((i & 0x0C) >> 2)];
		*(palette++) = color_scale[((i & 0x03) << 0)];
	}

	for (i = 0;i < Machine->drv->total_colors;i++)
		colortable[i] = i;

	for (i = 0; i < 0x40; i++)
	{
		segar_colorRAM[i]=0;
		segar_bcolorRAM[i]=0;
	}

}


/***************************************************************************
***************************************************************************/

void segar_videoram_w(int offset,int data)
{
	dirtybuffer[offset] = 1;

	if (videoram[offset] != data)
	{
		videoram[offset] = data;
	}
}

/***************************************************************************
The two bit planes are separated in memory.  If either bit plane changes,
mark the character as modified.
***************************************************************************/

void segar_characterram_w(int offset,int data)
{
	segar_dirtycharacter[offset / 8] = 1;

	segar_characterram[offset] = data;
}

void segar_characterram2_w(int offset,int data)
{
	segar_dirtycharacter[offset / 8] = 1;

	segar_characterram2[offset] = data;
}

/***************************************************************************
When our colorwrite bit flips off, we reset our whole palette.
Also, note that we need to refresh the entire screen, since the colors changed
and it's possible that a color change affected the transparency.
***************************************************************************/
void segar_video_port_w(int offset,int data)
{
	static unsigned char red[] = {0x00, 0x24, 0x49, 0x6D, 0x92, 0xB6, 0xDB, 0xFF };
	static unsigned char grn[] = {0x00, 0x24, 0x49, 0x6D, 0x92, 0xB6, 0xDB, 0xFF };
	static unsigned char blu[] = {0x00, 0x55, 0xAA, 0xFF };

	if (errorlog) fprintf(errorlog, "VPort = %02X\n",data);

	if ((data & 0x01) != segar_cocktail)
	{
		segar_cocktail=data & 0x01;
		segar_refresh=1;
	}

	if (data & 0x04)
		segar_char_refresh=1;

	if (data & 0x02)
		segar_colorwrite=1;
	else if (data & 0x40)
		segar_bcolorwrite=1;
	else
	{
		if (segar_colorwrite==1)
		{
			int i,r,g,b;

			for (i = 0; i < 0x40; i++)
			{
				b = blu[(segar_colorRAM[i] & 0xC0) >> 6];
				g = grn[(segar_colorRAM[i] & 0x38) >> 3];
				r = red[(segar_colorRAM[i] & 0x07)];

				osd_modify_pen(Machine->pens[i+1],r,g,b);

				if ((r==0) && (g==0) && (b==0))
					Machine->gfx[0]->colortable[i] = Machine->pens[0];
				else
					Machine->gfx[0]->colortable[i] = Machine->pens[i+1];
			}

			segar_refresh=1;
		}

		if (segar_bcolorwrite==1)
		{
			int i,r,g,b;
			if (errorlog) fprintf(errorlog,"Write to background color!\n");

			for (i = 0; i < 0x40; i++)
			{
				b = blu[(segar_bcolorRAM[i] & 0xC0) >> 6];
				g = grn[(segar_bcolorRAM[i] & 0x38) >> 3];
				r = red[(segar_bcolorRAM[i] & 0x07)];

				osd_modify_pen(Machine->pens[i + 0x40 + 1],r,g,b);
			}
		}

		segar_colorwrite=0;
		segar_bcolorwrite=0;
	}
}

/* For some reason, our colortable is being written slightly nonstandard... */
void segar_colortable_w(int offset,int data)
{
	if (segar_colorwrite)
	{
		switch (offset % 4)
		{
			case 0:
			case 3:
				segar_colorRAM[offset] = data;
				break;
			case 1:
				segar_colorRAM[offset+1] = data;
				break;
			case 2:
				segar_colorRAM[offset-1] = data;
				break;
		}
	}
	else
		segar_mem_colortable[offset] = data;
}

void segar_bcolortable_w(int offset,int data)
{
	if (segar_bcolorwrite)
	{
		if (errorlog) fprintf(errorlog,"Writing to background colortable!\n");

		switch (offset % 4)
		{
			case 0:
			case 3:
				segar_bcolorRAM[offset] = data;
				break;
			case 1:
				segar_bcolorRAM[offset+1] = data;
				break;
			case 2:
				segar_bcolorRAM[offset-1] = data;
				break;
		}
	}
	else
		segar_mem_bcolortable[offset] = data;
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/


/***************************************************************************
Since Monster Bash, Space Odyssey, and Pig Newton all have special
background boards, the video refresh code common across all these
games has been divided out to here.
***************************************************************************/
static void segar_common_screenrefresh(struct osd_bitmap *bitmap, int sprite_transparency, int copy_transparency)
{
	int offs;
	int charcode;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if ((segar_char_refresh) && (segar_dirtycharacter[videoram[offs]]))
			dirtybuffer[offs]=1;

		/* Redraw every character if our palette or scene changed */
		if ((dirtybuffer[offs]) || segar_refresh)
		{
			int sx,sy;

			sx = 8 * (offs / 32);
			sy = 8 * (31 - offs % 32);

			if (segar_cocktail)
			{
				sx = 27*8 - sx;
				sy = 31*8 - sy;
			}

			charcode = videoram[offs];

			/* decode modified characters */
			if (segar_dirtycharacter[charcode] == 1)
			{
				decodechar(Machine->gfx[0],charcode,segar_characterram,
						Machine->drv->gfxdecodeinfo[0].gfxlayout);
				segar_dirtycharacter[charcode] = 2;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode,charcode>>4,
					segar_cocktail,segar_cocktail,sx,sy,
					&Machine->drv->visible_area,sprite_transparency,0);

			dirtybuffer[offs] = 0;

		}
	}

	for (offs=0;offs<256;offs++)
		if (segar_dirtycharacter[offs]==2)
			segar_dirtycharacter[offs]=0;

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,copy_transparency,0);

	segar_char_refresh=0;
	segar_refresh=0;
}


/***************************************************************************
"Standard" refresh for games without background boards.
***************************************************************************/
void segar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	if (full_refresh)
		segar_refresh = 1;

	segar_common_screenrefresh(bitmap, TRANSPARENCY_NONE, TRANSPARENCY_NONE);
}


/***************************************************************************
 ---------------------------------------------------------------------------
 Space Odyssey Functions
 ---------------------------------------------------------------------------
***************************************************************************/

/***************************************************************************

Create two background bitmaps for Space Odyssey - one for the horizontal
scrolls that's 4 times wider than the screen, and one for the vertical
scrolls that's 4 times taller than the screen.

***************************************************************************/

int spaceod_vh_start(void)
{
	if (generic_vh_start()!=0)
		return 1;

	if ((horizbackbitmap = osd_create_bitmap(4*Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	if ((vertbackbitmap = osd_create_bitmap(Machine->drv->screen_width,4*Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(horizbackbitmap);
		generic_vh_stop();
		return 1;
	}

	return 0;
}

/***************************************************************************

Get rid of the Space Odyssey background bitmaps.

***************************************************************************/

void spaceod_vh_stop(void)
{
	osd_free_bitmap(horizbackbitmap);
	osd_free_bitmap(vertbackbitmap);
	generic_vh_stop();
}

/***************************************************************************
This port controls which background to draw for Space Odyssey.	The tempscene
and tempoffset are analogous to control lines used to select the background.
If the background changed, refresh the screen.
***************************************************************************/

void spaceod_back_port_w(int offset,int data)
{
	unsigned int tempscene, tempoffset;

	tempscene  = 0x1000 * ((data & 0xC0) >> 6);
	tempoffset = 0x100	* ((data & 0x04) >> 2);

	if (tempscene != segar_backscene)
	{
		segar_backscene=tempscene;
		segar_brefresh=1;
	}
	if (tempoffset != segar_backoffset)
	{
		segar_backoffset=tempoffset;
		segar_brefresh=1;
	}

	/* Our cocktail flip-the-screen bit. */
	if ((data & 0x01) != segar_bcocktail)
	{
		segar_bcocktail=data & 0x01;
		segar_brefresh=1;
	}

	segar_has_background=1;
	segar_fill_background=0;
}

/***************************************************************************
This port controls the Space Odyssey background scrolling.	Each write to
this port scrolls the background by one bit.  Faster speeds are achieved
by the program writing more often to this port.  Oddly enough, the value
sent to this port also seems to indicate the speed, but the value itself
is never checked.
***************************************************************************/
void spaceod_backshift_w(int offset,int data)
{
	segar_backshift= (segar_backshift + 1) % 0x400;
	segar_has_background=1;
	segar_fill_background=0;
}

/***************************************************************************
This port resets the Space Odyssey background to the "top".  This is only
really important for the Black Hole level, since the only way the program
can line up the background's Black Hole with knowing when to spin the ship
is to force the background to restart every time you die.
***************************************************************************/
void spaceod_backshift_clear_w(int offset,int data)
{
	segar_backshift=0;
	segar_has_background=1;
	segar_fill_background=0;
}

/***************************************************************************
Space Odyssey also lets you fill the background with a specific color.
***************************************************************************/
void spaceod_backfill_w(int offset,int data)
{
	segar_backfill=data + 0x40 + 1;
	segar_fill_background=1;
}

/***************************************************************************
***************************************************************************/
void spaceod_nobackfill_w(int offset,int data)
{
	segar_backfill=0;
	segar_fill_background=0;
}


/***************************************************************************
Special refresh for Space Odyssey, this code refreshes the static background.
***************************************************************************/
void spaceod_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int charcode;
	int sprite_transparency;
	int vert_scene;

	if (full_refresh)
		segar_refresh = 1;

	vert_scene = ((segar_backscene & 0x2000) == 0x2000);

	sprite_transparency=TRANSPARENCY_COLOR;

	/* If the background picture changed, draw the new one in temp storage */
	if (segar_brefresh)
	{
		segar_brefresh=0;

		for (offs = 0x1000 - 1; offs >= 0; offs--)
		{
			int sx,sy;

			/* Use Vertical Back Scene */
			if (vert_scene)
			{
				sx = 8 * ((((offs % 0x400) / 32)-3)%32);
				sy = 8 * (32*(3-(((offs+0xC00) / 0x400)%4)) + (31-(offs % 32)));

				if (segar_bcocktail)
				{
				   sx = 27*8 - sx; /* is this right? */
				   sy = 31*8 - sy;
				}
			}
			/* Use Horizontal Back Scene */
			else
			{
				sx = 8 * (offs / 32);
				sy = 8 * (31 - offs % 32);

				if (segar_bcocktail)
				{
				   sx = 127*8 - sx; /* is this right? */
				   sy = 31*8 - sy;
				}
			}

			charcode = segar_mem_blookup[offs + segar_backscene] + segar_backoffset;

			if (vert_scene)
			{
				drawgfx(vertbackbitmap,Machine->gfx[1],
					  charcode,0,
					  segar_bcocktail,segar_bcocktail,sx,sy,
					  0,TRANSPARENCY_NONE,0);
			}
			else
			{
				drawgfx(horizbackbitmap,Machine->gfx[1],
					  charcode,0,
					  segar_bcocktail,segar_bcocktail,sx,sy,
					  0,TRANSPARENCY_NONE,0);
			}
		}
	}

	/* Copy the scrolling background */
	{
		int scroll;

		if (vert_scene)
		{
			if (segar_bcocktail)
				scroll = -segar_backshift;
			else
				scroll = segar_backshift;

			copyscrollbitmap(bitmap,vertbackbitmap,0,0,1,&scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
		else
		{
			if (segar_bcocktail)
				scroll = segar_backshift;
			else
				scroll = -segar_backshift;

			copyscrollbitmap(bitmap,horizbackbitmap,1,&scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	if (segar_fill_background==1)
	{
		fillbitmap(bitmap,Machine->pens[segar_backfill],&Machine->drv->visible_area);
	}

	/* Refresh the "standard" graphics */
	segar_common_screenrefresh(bitmap, TRANSPARENCY_NONE, TRANSPARENCY_COLOR);
}


/***************************************************************************
 ---------------------------------------------------------------------------
 Monster Bash Functions
 ---------------------------------------------------------------------------
***************************************************************************/

/***************************************************************************
This port controls which background to draw for Monster Bash.  The tempscene
and tempoffset are analogous to control lines used to bank switch the
background ROMs.  If the background changed, refresh the screen.
***************************************************************************/

void monsterb_back_port_w(int offset,int data)
{
	unsigned int tempscene,tempoffset;

	tempscene  = 0x400 * ((data & 0x70) >> 4);
	tempoffset = 0x100 *  (data & 0x03);

	if (segar_backscene != tempscene)
	{
		segar_backscene = tempscene;
		segar_refresh=1;
	}
	if (segar_backoffset != tempoffset)
	{
		segar_backoffset = tempoffset;
		segar_refresh=1;
	}

	/* This bit turns the background off and on. */
	if ((data & 0x80) && (segar_has_background==0))
	{
		segar_has_background=1;
		segar_refresh=1;
	}
	else if (((data & 0x80)==0) && (segar_has_background==1))
	{
		segar_has_background=0;
		segar_refresh=1;
	}
}

/***************************************************************************
Special refresh for Monster Bash, this code refreshes the static background.
***************************************************************************/
void monsterb_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int charcode;
	int sprite_transparency;

	if (full_refresh)
		segar_refresh = 1;

	sprite_transparency=TRANSPARENCY_NONE;

	/* If the background is turned on, refresh it first. */
	if (segar_has_background)
	{
		/* for every character in the Video RAM, check if it has been modified */
		/* since last time and update it accordingly. */
		for (offs = videoram_size - 1;offs >= 0;offs--)
		{
			if ((segar_char_refresh) && (segar_dirtycharacter[videoram[offs]]))
				dirtybuffer[offs]=1;

			/* Redraw every background character if our palette or scene changed */
			if ((dirtybuffer[offs]) || segar_refresh)
			{
				int sx,sy;

				sx = 8 * (offs / 32);
				sy = 8 * (31 - offs % 32);

				if (segar_cocktail)
				{
					sx = 27*8 - sx;
					sy = 31*8 - sy;
				}

				charcode = segar_mem_blookup[offs + segar_backscene] + segar_backoffset;

				drawgfx(tmpbitmap,Machine->gfx[1],
					charcode,((charcode & 0xF0)>>4),
					segar_cocktail,segar_cocktail,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
			}
		}
		sprite_transparency=TRANSPARENCY_COLOR;
	}

	/* Refresh the "standard" graphics */
	segar_common_screenrefresh(bitmap, sprite_transparency, TRANSPARENCY_NONE);
}

/***************************************************************************
 ---------------------------------------------------------------------------
 Pig Newton Functions
 ---------------------------------------------------------------------------
***************************************************************************/

/***************************************************************************
These ports might control the background color for Pig Newton.
As you can see, my guessing obviously aren't quite right...
***************************************************************************/

void pignewt_back_color_w(int offset,int data)
{
	static unsigned char red[] = {0x00, 0x24, 0x49, 0x6D, 0x92, 0xB6, 0xDB, 0xFF };
	static unsigned char grn[] = {0x00, 0x24, 0x49, 0x6D, 0x92, 0xB6, 0xDB, 0xFF };
	static unsigned char blu[] = {0x00, 0x55, 0xAA, 0xFF };

	static unsigned int pignewt_bcolor_offset=0;

	int i,r,g,b;

	if (offset == 0)
	{
		pignewt_bcolor_offset = data;
	}
	else
	{
		b = blu[(data & 0xC0) >> 6];
		g = grn[(data & 0x38) >> 3];
		r = red[(data & 0x07)];

		osd_modify_pen(Machine->pens[pignewt_bcolor_offset + 0x40 + 1],r,g,b);
	}
}

/***************************************************************************
These ports control which background to draw for Pig Newton.  They might
also control other video aspects, since without schematics the usage of
many of the data lines is indeterminate.  Segar_backscene and segar_backoffset
are analogous to registers used to control bank-switching of the background
"videorom" ROMs and the background graphics ROMs, respectively.
If the background changed, refresh the screen.
***************************************************************************/

void pignewt_back_ports_w(int offset,int data)
{
	unsigned int tempscene;

	if (errorlog) fprintf(errorlog,"Port %02X:%02X\n",offset + 0xb8,data);

	/* These are all guesses.  There are some bits still being ignored! */
	switch (offset)
	{
		case 1:
			/* Bit D7 turns the background off and on? */
			if ((data & 0x80) && (segar_has_background==0))
			{
				segar_has_background=1;
				segar_refresh=1;
			}
			else if (((data & 0x80)==0) && (segar_has_background==1))
			{
				segar_has_background=0;
				segar_refresh=1;
			}
			/* Bits D0-D1 help select the background? */
			tempscene = (segar_backscene & 0x0C) | (data & 0x03);
			if (segar_backscene != tempscene)
			{
				segar_backscene = tempscene;
				segar_refresh=1;
			}
			break;
		case 3:
			/* Bits D0-D1 help select the background? */
			tempscene = ((data << 2) & 0x0C) | (segar_backscene & 0x03);
			if (segar_backscene != tempscene)
			{
				segar_backscene = tempscene;
				segar_refresh=1;
			}
			break;
		case 4:
			if (segar_backoffset != (data & 0x03))
			{
				segar_backoffset = data & 0x03;
				segar_refresh=1;
			}
			break;
	}
}

/***************************************************************************
Special refresh for Pig Newton, this code refreshes the static background.
***************************************************************************/
void pignewt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int charcode;
	int sprite_transparency;
	unsigned long backoffs;
	unsigned long backscene;


	if (full_refresh)
		segar_refresh = 1;

	sprite_transparency=TRANSPARENCY_NONE;

	/* If the background is turned on, refresh it first. */
	if (segar_has_background)
	{
		/* for every character in the Video RAM, check if it has been modified */
		/* since last time and update it accordingly. */
		for (offs = videoram_size - 1;offs >= 0;offs--)
		{
			if ((segar_char_refresh) && (segar_dirtycharacter[videoram[offs]]))
				dirtybuffer[offs]=1;

			/* Redraw every background character if our palette or scene changed */
			if ((dirtybuffer[offs]) || segar_refresh)
			{
				int sx,sy;

				sx = 8 * (offs / 32);
				sy = 8 * (31 - offs % 32);

				if (segar_cocktail)
				{
					sx = 27*8 - sx;
					sy = 31*8 - sy;
				}

				backscene = (segar_backscene & 0x0C) << 10;

				backoffs = (offs & 0x01F) + ((offs & 0x3E0) << 2) + ((segar_backscene & 0x03) << 5);

				charcode = segar_mem_blookup[backoffs + backscene] + (segar_backoffset * 0x100);

				drawgfx(tmpbitmap,Machine->gfx[1],
//					charcode,((charcode & 0xF0)>>4),
					charcode, 0,
					segar_cocktail,segar_cocktail,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
			}
		}
		sprite_transparency=TRANSPARENCY_COLOR;
	}

	/* Refresh the "standard" graphics */
	segar_common_screenrefresh(bitmap, sprite_transparency, TRANSPARENCY_NONE);

}

