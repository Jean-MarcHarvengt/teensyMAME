/***************************************************************************

  common.c

  Generic functions used in different emulators.
  There's not much for now, but it could grow in the future... ;-)

***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "driver.h"
#include "machine.h"
#include "common.h"
#include "osdepend.h"


/***************************************************************************

  Read ROMs into memory.

  Arguments:
  unsigned char *dest - Pointer to the start of the memory region
                        where the ROMs will be loaded.

  const struct RomModule *romp - pointer to an array of Rommodule structures,
                                 as defined in common.h.

  const char *basename - Name of the directory where the files are
                         stored. The function also supports the
						 control sequence %s in file names: for example,
						 if the RomModule gives the name "%s.bar", and
						 the basename is "foo", the file "foo/foo.bar"
						 will be loaded.

***************************************************************************/
int readroms(unsigned char *dest,const struct RomModule *romp,const char *basename)
{
	FILE *f;
	char buf[100];
	char name[100];


	while (romp->name)
	{
		sprintf(buf,romp->name,basename);
		sprintf(name,"%s/%s",basename,buf);

		if ((f = fopen(name,"rb")) == 0)
		{
			printf("Unable to open file %s\n",name);
			return 1;
		}
		if (fread(dest + romp->offset,1,romp->size,f) != romp->size)
		{
			printf("Unable to read file %s\n",name);
			fclose(f);
			return 1;
		}
		fclose(f);

		romp++;
	}

	return 0;
}



/***************************************************************************

  Function to convert the information stored in the graphic roms into a
  more usable format.

  Back in the early '80s, arcade machines didn't have the memory or the
  speed to handle bitmaps like we do today. They used "character maps",
  instead: they had one or more sets of characters (usually 8x8 pixels),
  which would be placed on the screen in order to form a picture. This was
  very fast: updating a character mapped display is, rougly speaking, 64
  times faster than updating an equivalent bitmap display, since you only
  modify groups of 8x8 pixels and not the single pixels. However, it was
  also much less versatile than a bitmap screen, since with only 256
  characters you had to do all of your graphics. To overcome this
  limitation, some special hardware graphics were used: "sprites". A sprite
  is essentially a bitmap, usually larger than a character, which can be
  placed anywhere on the screen (not limited to character boundaries) by
  just telling the hardware the coordinates. Moreover, sprites can be
  flipped along the major axis just by setting the appropriate bit (some
  machines can flip characters as well). This saves precious memory, since
  you need only one copy of the image instead of four.

  What about colors? Well, the early machines had a limited palette (let's
  say 16-32 colors) and each character or sprite could not use all of them
  at the same time. Characters and sprites data would use a limited amount
  of bits per pixel (typically 2, which allowed them to address only four
  different colors). You then needed some way to tell to the hardware which,
  among the available colors, were the four colors. This was done using a
  "color attribute", which typically was an index for a lookup table.

  OK, after this brief and incomplete introduction, let's come to the
  purpose of this section: how to interpret the data which is stored in
  the graphic roms. Unfortunately, there is no easy answer: it depends on
  the hardware. The easiest way to find how data is encoded, is to start by
  making a bit by bit dump of the rom. You will usually be able to
  immediately recognize some pattern: if you are lucky, you will see
  letters and numbers right away, otherwise you will see something which
  looks like letters and numbers, but with halves switched, dilated, or
  something like that. You'll then have to find a way to put it all
  together to obtain our standard one byte per pixel representation. Two
  things to remember:
  - keep in mind that every pixel has typically two (or more) bits
    associated with it, and they are not necessarily near to each other.
  - characters might be rotated 90 degrees. That's because many games used a
    tube rotated 90 degrees. Think how your monitor would look like if you
	put it on its side ;-)

  After you have successfully decoded the characters, you have to decode
  the sprites. This is usually more difficult, because sprites are larger,
  maybe have more colors, and are more difficult to recognize when they are
  messed up, since they are pure graphics without letters and numbers.
  However, with some work you'll hopefully be able to work them out as
  well. As a rule of thumb, the sprites should be encoded in a way not too
  dissimilar from the characters.

***************************************************************************/
int readbit(const unsigned char *src,int bitnum)
{
	int bits;


	bits = src[bitnum / 8];

	bitnum %= 8;
	while (bitnum-- > 0) bits <<= 1;

	if (bits & 0x80) return 1;
	else return 0;
}

struct GfxElement *decodegfx(const unsigned char *src,const struct GfxLayout *gl)
{
	int c,plane,x,y,offs;
	struct osd_bitmap *bm;
	struct GfxElement *gfx;


	if ((bm = osd_create_bitmap(gl->width,gl->total * gl->height)) == 0)
		return 0;
	if ((gfx = malloc(sizeof(struct GfxElement))) == 0)
	{
		osd_free_bitmap(bm);
		return 0;
	}
	gfx->width = gl->width;
	gfx->height = gl->height;
	gfx->total_elements = gl->total;
	gfx->color_granularity = 1 << gl->planes;
	gfx->gfxdata = bm;

	for (c = 0;c < gl->total;c++)
	{
		for (plane = 0;plane < gl->planes;plane++)
		{
			offs = c * gl->charincrement + plane * gl->planeincrement;
			for (y = 0;y < gl->height;y++)
			{
				for (x = 0;x < gl->width;x++)
				{
					unsigned char *dp;


					dp = bm->line[c * gl->height + y];
					if (plane == 0) dp[x] = 0;
					else dp[x] <<= 1;

					dp[x] += readbit(src,offs + gl->yoffset[y] + gl->xoffset[x]);
				}
			}
		}
	}

	return gfx;
}



void freegfx(struct GfxElement *gfx)
{
	if (gfx)
	{
		osd_free_bitmap(gfx->gfxdata);
		free(gfx);
	}
}



/***************************************************************************

  Draw graphic elements in the specified bitmap.

  transparency == TRANSPARENCY_NONE - no transparency.
  transparency == TRANSPARENCY_PEN - bits whose _original_ value is == transparent_color
                                     are transparent. This is the most common kind of
									 transparency.
  transparency == TRANSPARENCY_COLOR - bits whose _remapped_ value is == transparent_color
                                     are transparent. This is used by e.g. Pac Man.

***************************************************************************/
void drawgfx(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		struct rectangle *clip,int transparency,int transparent_color)
{
	int ox,oy,ex,ey,x,y,start;
	const unsigned char *sd;
	unsigned char *bm;
	int col;


	/* check bounds */
	ox = sx;
	oy = sy;
	ex = sx + gfx->width-1;
	if (sx < 0) sx = 0;
	if (clip && sx < clip->min_x) sx = clip->min_x;
	if (ex >= dest->width) ex = dest->width-1;
	if (clip && ex > clip->max_x) ex = clip->max_x;
	if (sx > ex) return;
	ey = sy + gfx->height-1;
	if (sy < 0) sy = 0;
	if (clip && sy < clip->min_y) sy = clip->min_y;
	if (ey >= dest->height) ey = dest->height-1;
	if (clip && ey > clip->max_y) ey = clip->max_y;
	if (sy > ey) return;

	start = (code % gfx->total_elements) * gfx->height;
	if (gfx->colortable)	/* remap colors */
	{
		const unsigned char *paldata;


		paldata = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)];

		switch (transparency)
		{
			case TRANSPARENCY_NONE:
				if (flipx)
				{
					if (flipy)	/* XY flip */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + gfx->height-1 - (y-oy)] + gfx->width-1 - (sx-ox);
							for (x = sx;x <= ex;x++)
								*(bm++) = paldata[*(sd--)];
						}
					}
					else 	/* X flip */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + (y-oy)] + gfx->width-1 - (sx-ox);
							for (x = sx;x <= ex;x++)
								*(bm++) = paldata[*(sd--)];
						}
					}
				}
				else
				{
					if (flipy)	/* Y flip */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + gfx->height-1 - (y-oy)] + (sx-ox);
							for (x = sx;x <= ex;x++)
								*(bm++) = paldata[*(sd++)];
						}
					}
					else		/* normal */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + (y-oy)] + (sx-ox);
							for (x = sx;x <= ex;x++)
								*(bm++) = paldata[*(sd++)];
						}
					}
				}
				break;

			case TRANSPARENCY_PEN:
				if (flipx)
				{
					if (flipy)	/* XY flip */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + gfx->height-1 - (y-oy)] + gfx->width-1 - (sx-ox);
							for (x = sx;x <= ex;x++)
							{
								col = *(sd--);
								if (col != transparent_color) *bm = paldata[col];
								bm++;
							}
						}
					}
					else 	/* X flip */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + (y-oy)] + gfx->width-1 - (sx-ox);
							for (x = sx;x <= ex;x++)
							{
								col = *(sd--);
								if (col != transparent_color) *bm = paldata[col];
								bm++;
							}
						}
					}
				}
				else
				{
					if (flipy)	/* Y flip */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + gfx->height-1 - (y-oy)] + (sx-ox);
							for (x = sx;x <= ex;x++)
							{
								col = *(sd++);
								if (col != transparent_color) *bm = paldata[col];
								bm++;
							}
						}
					}
					else		/* normal */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + (y-oy)] + (sx-ox);
							for (x = sx;x <= ex;x++)
							{
								col = *(sd++);
								if (col != transparent_color) *bm = paldata[col];
								bm++;
							}
						}
					}
				}
				break;

			case TRANSPARENCY_COLOR:
				if (flipx)
				{
					if (flipy)	/* XY flip */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + gfx->height-1 - (y-oy)] + gfx->width-1 - (sx-ox);
							for (x = sx;x <= ex;x++)
							{
								col = paldata[*(sd--)];
								if (col != transparent_color) *bm = col;
								bm++;
							}
						}
					}
					else 	/* X flip */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + (y-oy)] + gfx->width-1 - (sx-ox);
							for (x = sx;x <= ex;x++)
							{
								col = paldata[*(sd--)];
								if (col != transparent_color) *bm = col;
								bm++;
							}
						}
					}
				}
				else
				{
					if (flipy)	/* Y flip */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + gfx->height-1 - (y-oy)] + (sx-ox);
							for (x = sx;x <= ex;x++)
							{
								col = paldata[*(sd++)];
								if (col != transparent_color) *bm = col;
								bm++;
							}
						}
					}
					else		/* normal */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + (y-oy)] + (sx-ox);
							for (x = sx;x <= ex;x++)
							{
								col = paldata[*(sd++)];
								if (col != transparent_color) *bm = col;
								bm++;
							}
						}
					}
				}
				break;
		}
	}
	else
	{
		switch (transparency)
		{
			case TRANSPARENCY_NONE:		/* do a verbatim copy (faster) */
				if (flipx)
				{
					if (flipy)	/* XY flip */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + gfx->height-1 - (y-oy)] + gfx->width-1 - (sx-ox);
							for (x = sx;x <= ex;x++)
								*(bm++) = *(sd--);
						}
					}
					else 	/* X flip */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + (y-oy)] + gfx->width-1 - (sx-ox);
							for (x = sx;x <= ex;x++)
								*(bm++) = *(sd--);
						}
					}
				}
				else
				{
					if (flipy)	/* Y flip */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + gfx->height-1 - (y-oy)] + (sx-ox);
							for (x = sx;x <= ex;x++)
							memcpy(bm,sd,ex-sx+1);
						}
					}
					else		/* normal */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + (y-oy)] + (sx-ox);
							memcpy(bm,sd,ex-sx+1);
						}
					}
				}
				break;

			case TRANSPARENCY_PEN:
			case TRANSPARENCY_COLOR:
				if (flipx)
				{
					if (flipy)	/* XY flip */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + gfx->height-1 - (y-oy)] + gfx->width-1 - (sx-ox);
							for (x = sx;x <= ex;x++)
							{
								col = *(sd--);
								if (col != transparent_color) *bm = col;
								bm++;
							}
						}
					}
					else 	/* X flip */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + (y-oy)] + gfx->width-1 - (sx-ox);
							for (x = sx;x <= ex;x++)
							{
								col = *(sd--);
								if (col != transparent_color) *bm = col;
								bm++;
							}
						}
					}
				}
				else
				{
					if (flipy)	/* Y flip */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + gfx->height-1 - (y-oy)] + (sx-ox);
							for (x = sx;x <= ex;x++)
							{
								col = *(sd++);
								if (col != transparent_color) *bm = col;
								bm++;
							}
						}
					}
					else		/* normal */
					{
						for (y = sy;y <= ey;y++)
						{
							bm = dest->line[y] + sx;
							sd = gfx->gfxdata->line[start + (y-oy)] + (sx-ox);
							for (x = sx;x <= ex;x++)
							{
								col = *(sd++);
								if (col != transparent_color) *bm = col;
								bm++;
							}
						}
					}
				}
				break;
		}
	}
}



void setdipswitches(int *dsw,const struct DSW *dswsettings)
{
	struct DisplayText dt[40];
	int settings[20];
	int i,s,key;
	int total;


	total = 0;
	while (dswsettings[total].num != -1)
	{
		int msk,val;


		msk = dswsettings[total].mask;
		if (msk == 0) return;	/* error in DSW definition, quit */
		val = dsw[dswsettings[total].num];
		while ((msk & 1) == 0)
		{
			val >>= 1;
			msk >>= 1;
		}
		settings[total] = val & msk;

		total++;
	}

	s = 0;
	do
	{
		for (i = 0;i < total;i++)
		{
			dt[2 * i].color = (i == s) ? Machine->drv->yellow_text : Machine->drv->white_text;
			dt[2 * i].text = dswsettings[i].name;
			dt[2 * i].x = 2*8;
			dt[2 * i].y = 2*8 * i + (Machine->drv->screen_height - 2*8 * (total - 1)) / 2;
			dt[2 * i + 1].color = (i == s) ? Machine->drv->yellow_text : Machine->drv->white_text;
			dt[2 * i + 1].text = dswsettings[i].values[settings[i]];
			dt[2 * i + 1].x = Machine->drv->screen_width - 2*8 - 8*strlen(dt[2 * i + 1].text);
			dt[2 * i + 1].y = dt[2 * i].y;
		}
		dt[2 * i].text = 0;	/* terminate array */

		displaytext(dt,1);

		key = osd_read_key();

		switch (key)
		{
			case OSD_KEY_DOWN:
				if (s < total - 1) s++;
				break;

			case OSD_KEY_UP:
				if (s > 0) s--;
				break;

			case OSD_KEY_RIGHT:
				if (dswsettings[s].reverse == 0)
				{
					if (dswsettings[s].values[settings[s] + 1] != 0) settings[s]++;
				}
				else
				{
					if (settings[s] > 0) settings[s]--;
				}
				break;

			case OSD_KEY_LEFT:
				if (dswsettings[s].reverse == 0)
				{
					if (settings[s] > 0) settings[s]--;
				}
				else
				{
					if (dswsettings[s].values[settings[s] + 1] != 0) settings[s]++;
				}
				break;
		}
	} while (key != OSD_KEY_TAB && key != OSD_KEY_ESC);

	while (osd_key_pressed(key));	/* wait for key release */

	if (key == OSD_KEY_ESC) Z80_Running = 0;

	while (--total >= 0)
	{
		int msk;


		msk = dswsettings[total].mask;
		while ((msk & 1) == 0)
		{
			settings[total] <<= 1;
			msk >>= 1;
		}

		dsw[dswsettings[total].num] = (dsw[dswsettings[total].num] & ~dswsettings[total].mask) | settings[total];
	}

	/* clear the screen before returning */
	for (i = 0;i < Machine->scrbitmap->height;i++)
		memset(Machine->scrbitmap->line[i],Machine->background_pen,Machine->scrbitmap->width);
}






/***************************************************************************

  Display text on the screen. If erase is 0, it superimposes the text on
  the last frame displayed.

***************************************************************************/
void displaytext(const struct DisplayText *dt,int erase)
{
	if (erase)
	{
		int i;


		for (i = 0;i < Machine->scrbitmap->height;i++)
			memset(Machine->scrbitmap->line[i],Machine->background_pen,Machine->scrbitmap->width);
	}

	while (dt->text)
	{
		int x;
		const unsigned char *c;


		x = dt->x;
		c = dt->text;

		while (*c)
		{
			if (*c != ' ')
			{
				if (*c >= '0' && *c <= '9')
					drawgfx(Machine->scrbitmap,Machine->gfx[0],*c - '0' + Machine->drv->numbers_start,dt->color,0,0,x,dt->y,0,TRANSPARENCY_NONE,0);
				else drawgfx(Machine->scrbitmap,Machine->gfx[0],*c - 'A' + Machine->drv->letters_start,dt->color,0,0,x,dt->y,0,TRANSPARENCY_NONE,0);
			}
			x += 8;
			c++;
		}

		dt++;
	}

	osd_update_display();
}
