/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M68000/M68000.h"



unsigned char *punkshot_vidram,*punkshot_scrollram;
int punkshot_vidram_size;

static struct osd_bitmap *tmpbitmap1;
static unsigned char punkshot_yscroll[2];
static unsigned char romsubbank[2];
static unsigned char charrombank[4];
static int read_char_rom;
static int irq_enable;
static int text_colorbase,fg_colorbase,bg_colorbase,sprite_colorbase;
static int priority;



int tmnt_interrupt(void)
{
	if (irq_enable) return MC68000_IRQ_5;
	else return MC68000_INT_NONE;
}

int punkshot_interrupt(void)
{
	if (irq_enable) return MC68000_IRQ_4;
	else return MC68000_INT_NONE;
}

void punkshot_irqenable_w(int offset,int data)
{
	if (offset == 0)
	{
		static unsigned char irqenable[2];

		COMBINE_WORD_MEM(irqenable,data);

		irq_enable = READ_WORD(irqenable) & 0x0400;
	}
}



static int common_vh_start (void)
{
	if ((tmpbitmap = osd_new_bitmap(512,256,Machine->scrbitmap->depth)) == 0)
	{
		return 1;
	}

	if ((tmpbitmap1 = osd_new_bitmap(512,256,Machine->scrbitmap->depth)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		return 1;
	}

	if ((dirtybuffer = malloc(punkshot_vidram_size / 2)) == 0)
	{
		osd_free_bitmap(tmpbitmap1);
		osd_free_bitmap(tmpbitmap);
		return 1;
	}
	memset(dirtybuffer,1,punkshot_vidram_size / 2);

	return 0;
}



static void common_vh_stop (void)
{
	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap1);
	osd_free_bitmap(tmpbitmap);
}


int tmnt_vh_start (void)
{
	text_colorbase = 0;
	sprite_colorbase = 16;
	fg_colorbase = 32;
	bg_colorbase = 40;
	return common_vh_start();
}

void tmnt_vh_stop (void)
{
	common_vh_stop();
}

int punkshot_vh_start (void)
{
	text_colorbase = 0;
	sprite_colorbase = 32;
	fg_colorbase = 64;
	bg_colorbase = 80;
	return common_vh_start();
}

void punkshot_vh_stop (void)
{
	common_vh_stop();
}



void tmnt_paletteram_w(int offset,int data)
{
	int oldword = READ_WORD(&paletteram[offset]);
	int newword = COMBINE_WORD(oldword,data);
	WRITE_WORD(&paletteram[offset],newword);

	offset /= 4;
	{
		int palette = ((READ_WORD(&paletteram[offset * 4]) & 0x00ff) << 8)
				+ (READ_WORD(&paletteram[offset * 4 + 2]) & 0x00ff);
		int r = palette & 31;
		int g = (palette >> 5) & 31;
		int b = (palette >> 10) & 31;

		r = (r << 3) + (r >> 2);
		g = (g << 3) + (g >> 2);
		b = (b << 3) + (b >> 2);

		palette_change_color (offset,r,g,b);
	}
}



void punkshot_spriteram_w(int offset,int data)
{
	COMBINE_WORD_MEM(&spriteram[offset],data);
}

int punkshot_spriteram_r(int offset)
{
	return READ_WORD(&spriteram[offset]);
}


void punkshot_xscroll_w(int offset,int data)
{
	COMBINE_WORD_MEM(&punkshot_scrollram[offset],data);
}

void punkshot_yscroll_w(int offset,int data)
{
	if (offset == 0)
		COMBINE_WORD_MEM(punkshot_yscroll,data);
}


void punkshot_vidram_w(int offset,int data)
{
	int oldword;
	int newword;

	offset &= ~0x1000;	/* handle mirror address */
	oldword = READ_WORD(&punkshot_vidram[offset]);
	newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&punkshot_vidram[offset],newword);
		dirtybuffer[offset / 2] = 1;
	}
}

int punkshot_100000_r(int offset)
{
	offset &= ~0x1000;	/* handle mirror address */

	if (read_char_rom == 0)
	{
		if (offset < 0x6000) return READ_WORD(&punkshot_vidram[offset]);
	}
	else
	{
	int base,off1,off2,offlow,rom;


	base = READ_WORD(romsubbank) >> 8;	/* range 00-1f */
	off1 = (offset & 0x0ff8) >> 3;
	offlow = (offset & 0x0002) >> 1;
	off2 = charrombank[(offset & 0x6000) >> 13];
	rom = (offset & 0x0004) >> 2;

	return Machine->memory_region[2][0x40000 * rom + 0x8000 * off2 + 0x400 * base + 2 * off1 + offlow] << 8;
/*
offset:
0aa0 bbbb bbbb bcd0

aa:
bbbbbbbbb:
c: 0 = ROM 06, 1 = ROM 05
d:
*/
	}

	return 0;
}
void punkshot_charromsubbank_w(int offset,int data)
{
	if (offset == 0) COMBINE_WORD_MEM(romsubbank,data);
}

void tmnt_0a0000_w(int offset,int data)
{
	if (offset == 0)
	{
		static unsigned char ctrl[2];
		static int last;
		int oldword = READ_WORD(ctrl);
		int newword = COMBINE_WORD(oldword,data);


		WRITE_WORD(ctrl,newword);

		/* bit 0/1 = coin counters */

		/* bit 3 high then low triggers irq on sound CPU */
		if (last == 0x08 && (newword & 0x08) == 0)
			cpu_cause_interrupt(1,0xff);

		last = newword & 0x08;

		/* bit 5 = irq enable */
		irq_enable = newword & 0x20;

		/* bit 7 = enable char ROM reading through the video RAM */
		read_char_rom = newword & 0x80;
	}
}

void punkshot_0a0020_w(int offset,int data)
{
	if (offset == 0)
	{
		static unsigned char ctrl[2];
		static int last;
		int oldword = READ_WORD(ctrl);
		int newword = COMBINE_WORD(oldword,data);


		WRITE_WORD(ctrl,newword);

		/* bit 0 = coin counter */

		/* bit 2 = trigger irq on sound CPU */
		if (last == 0x04 && (newword & 0x04) == 0)
			cpu_cause_interrupt(1,0xff);

		last = newword & 0x04;

		/* bit 3 = enable char ROM reading through the video RAM */
		read_char_rom = newword & 0x08;
	}
}

void punkshot_charrombank0_w(int offset,int data)
{
	if (offset == 0)
	{
		static unsigned char rombank[2];
		int oldword = READ_WORD(rombank);
		int newword = COMBINE_WORD(oldword,data);


		if (oldword != newword)
		{
			WRITE_WORD(rombank,newword);

			charrombank[0] = (newword >> 8) & 0x0f;
			charrombank[1] = (newword >> 12) & 0x0f;
			memset(dirtybuffer,1,punkshot_vidram_size / 2);
		}
	}
}
void punkshot_charrombank1_w(int offset,int data)
{
	if (offset == 0)
	{
		static unsigned char rombank[2];
		int oldword = READ_WORD(rombank);
		int newword = COMBINE_WORD(oldword,data);


		if (oldword != newword)
		{
			WRITE_WORD(rombank,newword);

			charrombank[2] = (newword >> 8) & 0x0f;
			charrombank[3] = (newword >> 12) & 0x0f;
			memset(dirtybuffer,1,punkshot_vidram_size / 2);
		}
	}
}



void tmnt_priority_w(int offset,int data)
{
	if (offset == 0)
	{
		static unsigned char pri[2];

		COMBINE_WORD_MEM(pri,data);

		/* bit 2/3 = priority; other bits unused */
		priority = (READ_WORD(pri) & 0x0c) >> 2;
	}
}

void punkshot_priority_w(int offset,int data)
{
	if (offset == 0)
	{
		static unsigned char pri[2];

		COMBINE_WORD_MEM(pri,data);

/* values for priority:
   0x26  bg spr fg txt
   0x2c  bg fg spr txt
   0x3c  fg bg spr txt
*/
		priority = READ_WORD(pri);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

    /*
     * Sprite Format
     * ------------------
     *
     * Byte(s) | Bit(s)   | Use
     * --------+-76543210-+----------------
     *    0    | x------- | active (show this sprite)
     *    0    | -xxxxxxx | priority order
     *    1    | xxx----- | sprite size (see below)
     *    1    | ---xxxxx | sprite number (high 5 bits)
     *    2    | xxxxxxxx | Sprite Number (low 8 bits)
     *    3    | x------- | in Punk Shot, SHADOW
     *    3    | --x----- | in Punk Shot, priority over text layer (0 = have priority)
     *    3    | ---x---- | if set, use second gfx ROM bank
     *    3    | ----xxxx | palette
     *    4    | ------x- | flip y
     *    4    | -------x | y position (high bit)
     *    5    | xxxxxxxx | y position (low 8 bits)
     *    6    | ------x- | flip x
     *    6    | -------x | x position (high 1 bit)
     *    7    | xxxxxxxx | x position (low 8 bits)
     */
static void tmnt_drawsprites(struct osd_bitmap *bitmap)
{
	int offs;
	int pri_code;


//	for (pri_code = 0x8000;pri_code < 0x10000;pri_code += 0x100)
	for (pri_code = 0x8000;pri_code < 0x0ff00;pri_code += 0x100)
	{
		for (offs = spriteram_size - 8;offs >= 0;offs -= 8)
		{
			int sx,sy,col,code,size,w,h,x,y,subcount,flipx,flipy;
			/* sprites can be grouped up to 8x8. The draw order is
				 0  1  4  5 16 17 20 21
				 2  3  6  7 18 19 22 23
				 8  9 12 13 24 25 28 29
				10 11 14 15 26 27 30 31
				32 33 36 37 48 49 52 53
				34 35 38 39 50 51 54 55
				40 41 44 45 56 57 60 61
				42 43 46 47 58 59 62 63
			*/
			static int xoffset[8] = { 0, 1, 4, 5, 16, 17, 20, 21 };
			static int yoffset[8] = { 0, 2, 8, 10, 32, 34, 40, 42 };
			static int width[8] =  { 1, 2, 1, 2, 4, 2, 4, 8 };
			static int height[8] = { 1, 1, 2, 2, 2, 4, 4, 8 };


			if ((READ_WORD(&spriteram[offs]) & 0xff00) != pri_code) continue;

			size = (READ_WORD(&spriteram[offs]) & 0xe0) >> 5;
			w = width[size];
			h = height[size];

			code = (READ_WORD(&spriteram[offs+2]) >> 8) +
					((READ_WORD(&spriteram[offs]) & 0x1f) << 8) +
					(((READ_WORD(&spriteram[offs+2]) & 0x10) >> 4) << 13);

			col = sprite_colorbase + (READ_WORD(&spriteram[offs+2]) & 0x0f);

			sx = READ_WORD(&spriteram[offs+6]) & 0x01ff;
			sy = 256 - (READ_WORD(&spriteram[offs+4]) & 0x01ff);
			flipx = READ_WORD(&spriteram[offs+6]) & 0x0200;
			flipy = READ_WORD(&spriteram[offs+4]) & 0x0200;

			if (flipx) sx += 16 * (w - 1);
			if (flipy) sy += 16 * (h - 1);

			for (y = 0;y < h;y++)
			{
				for (x = 0;x < w;x++)
				{
static unsigned char code_conv_table[] =
{
	/* PROM G7 */
	0x04,0x04,0x04,0x06,0x04,0x05,0x05,0x05,0x06,0x06,0x06,0x03,0x03,0x02,0x01,0x00,
	0x06,0x06,0x04,0x07,0x05,0x06,0x06,0x06,0x03,0x06,0x06,0x04,0x05,0x03,0x06,0x06,
	0x04,0x04,0x04,0x04,0x06,0x06,0x06,0x06,0x03,0x03,0x05,0x00,0x02,0x01,0x05,0x05,
	0x04,0x04,0x04,0x04,0x06,0x06,0x06,0x06,0x04,0x04,0x04,0x04,0x06,0x06,0x06,0x06,
	0x04,0x01,0x03,0x03,0x03,0x03,0x06,0x03,0x06,0x06,0x06,0x06,0x06,0x06,0x03,0x06,
	0x06,0x06,0x06,0x03,0x06,0x06,0x06,0x03,0x06,0x06,0x06,0x06,0x06,0x06,0x03,0x06,
	0x06,0x06,0x06,0x03,0x06,0x06,0x05,0x04,0x06,0x06,0x04,0x03,0x01,0x02,0x00,0x06,
	0x06,0x06,0x06,0x06,0x06,0x06,0x03,0x06,0x06,0x03,0x05,0x04,0x01,0x01,0x02,0x00,
	0x06,0x06,0x06,0x06,0x03,0x00,0x00,0x02,0x03,0x03,0x04,0x04,0x01,0x04,0x02,0x02,
	0x03,0x04,0x01,0x03,0x03,0x00,0x06,0x04,0x04,0x03,0x05,0x06,0x03,0x04,0x06,0x02,
	0x01,0x03,0x02,0x05,0x04,0x03,0x01,0x00,0x02,0x05,0x00,0x01,0x02,0x03,0x00,0x01,
	0x03,0x02,0x01,0x04,0x03,0x04,0x03,0x06,0x00,0x03,0x06,0x03,0x04,0x01,0x04,0x03,
	0x03,0x06,0x02,0x05,0x06,0x00,0x03,0x04,0x02,0x05,0x06,0x03,0x05,0x06,0x06,0x06,
	0x04,0x04,0x04,0x04,0x00,0x01,0x04,0x03,0x03,0x01,0x00,0x02,0x06,0x03,0x06,0x06,
	0x03,0x05,0x04,0x03,0x03,0x06,0x03,0x03,0x06,0x06,0x06,0x06,0x06,0x06,0x00,0x06,
	0x03,0x06,0x06,0x03,0x03,0x06,0x06,0x07,0x07,0x07,0x07,0x06,0x07,0x07,0x07,0x07
};
#define CA0 0
#define CA1 1
#define CA2 2
#define CA4 3
#define CA5 4
#define CA6 5
#define CA7 6
#define CA8 7
#define CA9 8

/* following table derived from the schematics. It indicates, for each of the */
/* 9 low bits of the sprite line address, which bit to pick it from. */
/* For example, in the 4x2 case, bit OA1 comes from CA5, OA2 from CA0, and so on. */
static unsigned char bit_pick_table[9][8] =
{
	/*1x1  2x1  1x2  2x2  4x2  2x4  4x4  8x8 */
	{ CA0, CA0, CA5, CA5, CA5, CA5, CA5, CA5 },	/* OA1 */
	{ CA1, CA1, CA0, CA0, CA0, CA7, CA7, CA7 },	/* OA2 */
	{ CA2, CA2, CA1, CA1, CA1, CA0, CA0, CA9 },	/* OA3 */
	{ CA4, CA4, CA2, CA2, CA2, CA1, CA1, CA0 },	/* OA4 */
	{ CA5, CA6, CA4, CA4, CA4, CA2, CA2, CA1 },	/* OA5 */
	{ CA6, CA5, CA6, CA6, CA6, CA4, CA4, CA2 },	/* OA6 */
	{ CA7, CA7, CA7, CA7, CA8, CA6, CA6, CA4 },	/* OA7 */
	{ CA8, CA8, CA8, CA8, CA7, CA8, CA8, CA6 },	/* OA8 */
	{ CA9, CA9, CA9, CA9, CA9, CA9, CA9, CA8 }	/* OA9 */
};
					for (subcount = 0;subcount < 16;subcount++)
					{
						int c;
						int entry;
						int bits[9];
						int newbits[9];
						int i;


/* TMNT draws sprites in a peculiar way. The basic block is 16x16, and they can
   be grouped in various arrangements from 1x1 up to 8x8. The sprite data, however,
   is not stored in the gfx ROMs as 16x16 blocks like in almost every other game.
   It is instead spread around differently, depending on the size of the combined
   sprite. To handle that, we draw the sprite one line at a time.
   The weird thing is that the way the gfx data is decoded by the hardware does
   not depend on the 'size' attribute handled above, but on the sprite code! So,
   theoretically, you could have data arranged for one size, but draw it using a
   different size. I don't know if this is actually done by the game, anyway just
   to be sure we emulate that faithfully here, taking the sprite code and
   rearraging its bits basing on the information contained in a PROM, exactly like
   the hardware does.
*/

						/* take the sprite line address */
						c = (16 * (code + xoffset[x] + yoffset[y]) + subcount);

						/* pick the correct entry in the PROM (top 8 bits of the code) */
						entry = (c & 0x3fc00) >> 10;

						/* the bits to scramble are the low 9 ones */
						for (i = 0;i < 9;i++)
							bits[i] = (c >> i) & 0x01;

						/* rearrange the bits basing on the table */
						for (i = 0;i < 9;i++)
							newbits[i] = bits[bit_pick_table[i][code_conv_table[entry]]];

						/* build the converted sprite line address */
						c &= 0xffe00;
						for (i = 0;i < 9;i++)
							c |= newbits[i] << i;

						/* and finally draw this line */
						drawgfx(bitmap,Machine->gfx[1 + ((c & 0x3c000) >> 14)],
								c & 0x03fff,
								col,
								flipx,flipy,
								sx + 16 * x * (flipx ? -1 : 1),sy + (16 * y + subcount) * (flipy ? -1 : 1),
								&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
					}
				}
			}
		}
	}
}


static void punkshot_drawsprites(struct osd_bitmap *bitmap,int pri)
{
	int offs;
	int pri_code;


	for (pri_code = 0x8000;pri_code < 0x10000;pri_code += 0x100)
	{
		for (offs = spriteram_size - 8;offs >= 0;offs -= 8)
		{
			int sx,sy,col,code,size,w,h,x,y,flipx,flipy;
			/* sprites can be grouped up to 8x8. The draw order is
				 0  1  4  5 16 17 20 21
				 2  3  6  7 18 19 22 23
				 8  9 12 13 24 25 28 29
				10 11 14 15 26 27 30 31
				32 33 36 37 48 49 52 53
				34 35 38 39 50 51 54 55
				40 41 44 45 56 57 60 61
				42 43 46 47 58 59 62 63
			*/
			static int xoffset[8] = { 0, 1, 4, 5, 16, 17, 20, 21 };
			static int yoffset[8] = { 0, 2, 8, 10, 32, 34, 40, 42 };
			static int width[8] =  { 1, 2, 1, 2, 4, 2, 4, 8 };
			static int height[8] = { 1, 1, 2, 2, 2, 4, 4, 8 };


			if ((READ_WORD(&spriteram[offs]) & 0xff00) != pri_code) continue;

			if (pri == 0)
			{
				if ((READ_WORD(&spriteram[offs+2]) & 0x20) == 0)
					continue;
			}
			else if (pri == 1)
			{
				if ((READ_WORD(&spriteram[offs+2]) & 0x20) != 0)
					continue;
			}

			size = (READ_WORD(&spriteram[offs]) & 0xe0) >> 5;
			w = width[size];
			h = height[size];

			code = (READ_WORD(&spriteram[offs+2]) >> 8) +
					((READ_WORD(&spriteram[offs]) & 0x1f) << 8) +
					(((READ_WORD(&spriteram[offs+2]) & 0x10) >> 4) << 13);

			/* I'm not sure the following is correct, but some sort of alignment */
			/* is certainly needed to fix the score table. */
			if (w == 2) code &= ~1;
			if (w == 4) code &= ~7;
			if (w == 8) code &= ~31;

			col = sprite_colorbase + (READ_WORD(&spriteram[offs+2]) & 0x0f);

			sx = READ_WORD(&spriteram[offs+6]) & 0x01ff;
			sy = 256 - (READ_WORD(&spriteram[offs+4]) & 0x01ff);
			flipx = READ_WORD(&spriteram[offs+6]) & 0x0200;
			flipy = READ_WORD(&spriteram[offs+4]) & 0x0200;

			if (flipx) sx += 16 * (w - 1);
			if (flipy) sy += 16 * (h - 1);

			for (y = 0;y < h;y++)
			{
				for (x = 0;x < w;x++)
				{
					drawgfx(bitmap,Machine->gfx[1],
							code + xoffset[x] + yoffset[y],
							col,
							flipx,flipy,
							sx + 16 * x * (flipx ? -1 : 1),sy + 16 * y * (flipy ? -1 : 1),
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				}
			}
		}
	}
}


void tmnt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs,i;


	/* palette remapping first */
	{
		unsigned short palette_map[64];
		int tile,code,color;

		memset (palette_map, 0, sizeof (palette_map));

		/* foreground */
		for (offs = 0x3000 - 2;offs >= 0x2000;offs -= 2)
		{
			tile = READ_WORD(&punkshot_vidram[offs]);
			color = fg_colorbase + ((tile & 0xe000) >> 13);
			code = (tile & 0x03ff) + ((tile & 0x1000) >> 2) +
					0x800 * charrombank[(tile & 0x0c00) >> 10];
			palette_map[color] |= Machine->gfx[0]->pen_usage[code];
		}

		/* background */
		for (offs = 0x5000 - 2;offs >= 0x4000;offs -= 2)
		{
			tile = READ_WORD(&punkshot_vidram[offs]);
			color = bg_colorbase + ((tile & 0xe000) >> 13);
			code = (tile & 0x03ff) + ((tile & 0x1000) >> 2) +
					0x800 * charrombank[(tile & 0x0c00) >> 10];
			palette_map[color] |= Machine->gfx[0]->pen_usage[code];
		}

		/* characters */
		for (offs = 0x1000 - 2;offs >= 0;offs -= 2)
		{
			tile = READ_WORD(&punkshot_vidram[offs]);
			color = text_colorbase + ((tile & 0xe000) >> 13);
			code = (tile & 0x03ff) + ((tile & 0x1000) >> 2) +
					0x800 * charrombank[(tile & 0x0c00) >> 10];
			palette_map[color] |= Machine->gfx[0]->pen_usage[code];
		}

		/* sprites */
		for (offs = spriteram_size - 8;offs >= 0;offs -= 8)
		{
			color = sprite_colorbase + (READ_WORD(&spriteram[offs+2]) & 0x0f);
			palette_map[color] |= 0xffff;
		}

		/* now build the final table */
		for (i = 0; i < 64; i++)
		{
			int usage = palette_map[i], j;
			if (usage)
			{
				palette_used_colors[i * 16 + 0] = PALETTE_COLOR_TRANSPARENT;
				for (j = 1; j < 16; j++)
					if (usage & (1 << j))
						palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
					else
						palette_used_colors[i * 16 + j] = PALETTE_COLOR_UNUSED;
			}
			else
				memset (&palette_used_colors[i * 16 + 0], PALETTE_COLOR_UNUSED, 16);
		}

		/* recalc */
		if (palette_recalc ())
			memset(dirtybuffer,1,punkshot_vidram_size / 2);
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0x3000 - 2;offs >= 0x2000;offs -= 2)
	{
		int sx,sy,tile,code,color;

		if (dirtybuffer[offs / 2])
		{
			tile = READ_WORD(&punkshot_vidram[offs]);
			color = fg_colorbase + ((tile & 0xe000) >> 13);

			dirtybuffer[offs / 2] = 0;

			sx = (offs/2) % 64;
			sy = (offs/2) / 64 - 64;

			code = (tile & 0x03ff) + ((tile & 0x1000) >> 2) +
					0x800 * charrombank[(tile & 0x0c00) >> 10];

			drawgfx(tmpbitmap,Machine->gfx[0],
					code,
					color,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	for (offs = 0x5000 - 2;offs >= 0x4000;offs -= 2)
	{
		int sx,sy,tile,code,color;

		if (dirtybuffer[offs / 2])
		{
			tile = READ_WORD(&punkshot_vidram[offs]);
			color = bg_colorbase + ((tile & 0xe000) >> 13);

			dirtybuffer[offs / 2] = 0;

			sx = (offs/2) % 64;
			sy = (offs/2) / 64 - 128;

			code = (tile & 0x03ff) + ((tile & 0x1000) >> 2) +
					0x800 * charrombank[(tile & 0x0c00) >> 10];

			drawgfx(tmpbitmap1,Machine->gfx[0],
					code,
					color,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int xscroll,yscroll;


		yscroll = -(READ_WORD(punkshot_yscroll) & 0xff);
		xscroll = -((READ_WORD(&punkshot_scrollram[0]) & 0xff) + 256 * (READ_WORD(&punkshot_scrollram[2]) & 0xff));
		xscroll += 6;
		copyscrollbitmap(bitmap,tmpbitmap1,1,&xscroll,1,&yscroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* draw the sprites if they don't have priosity over the foreground */
	if ((priority & 1) == 1) tmnt_drawsprites(bitmap);


	/* draw the foreground */
	{
		int xscroll,yscroll;


		xscroll = -((READ_WORD(&punkshot_scrollram[0]) >> 8) + 256 * (READ_WORD(&punkshot_scrollram[2]) >> 8));
		xscroll += 6;
		yscroll = -(READ_WORD(punkshot_yscroll) >> 8);
		copyscrollbitmap(bitmap,tmpbitmap,1,&xscroll,1,&yscroll,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	}


	/* draw the sprites if they have priosity over the foreground */
	if ((priority & 1) == 0) tmnt_drawsprites(bitmap);


	/* draw the foreground characters */
	for (offs = 0x1000 - 2;offs >= 0;offs -= 2)
	{
		int sx,sy,tile,code,color;


		sx = (offs/2) % 64;
		sy = (offs/2) / 64;
		tile = READ_WORD(&punkshot_vidram[offs]);
		color = text_colorbase + ((tile & 0xe000) >> 13);

		code = (tile & 0x03ff) + ((tile & 0x1000) >> 2) +
				0x800 * charrombank[(tile & 0x0c00) >> 10];

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				0,0,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}

void punkshot_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs,i;


	/* palette remapping first */
	{
		unsigned short palette_map[128];
		int tile,code,color;

		memset (palette_map, 0, sizeof (palette_map));

		/* foreground */
		for (offs = 0x3000 - 2;offs >= 0x2000;offs -= 2)
		{
			tile = READ_WORD(&punkshot_vidram[offs]);
			color = fg_colorbase + ((tile & 0xe000) >> 13);
			code = (tile & 0x03ff) + ((tile & 0x1000) >> 2) +
					0x800 * charrombank[(tile & 0x0c00) >> 10];
			palette_map[color] |= Machine->gfx[0]->pen_usage[code];
		}

		/* background */
		for (offs = 0x5000 - 2;offs >= 0x4000;offs -= 2)
		{
			tile = READ_WORD(&punkshot_vidram[offs]);
			color = bg_colorbase + ((tile & 0xe000) >> 13);
			code = (tile & 0x03ff) + ((tile & 0x1000) >> 2) +
					0x800 * charrombank[(tile & 0x0c00) >> 10];
			palette_map[color] |= Machine->gfx[0]->pen_usage[code];
		}

		/* characters */
		for (offs = 0x1000 - 2;offs >= 0;offs -= 2)
		{
			tile = READ_WORD(&punkshot_vidram[offs]);
			color = text_colorbase + ((tile & 0xe000) >> 13);
			code = (tile & 0x03ff) + ((tile & 0x1000) >> 2) +
					0x800 * charrombank[(tile & 0x0c00) >> 10];
			palette_map[color] |= Machine->gfx[0]->pen_usage[code];
		}

		/* sprites */
		for (offs = spriteram_size - 8;offs >= 0;offs -= 8)
		{
			color = sprite_colorbase + (READ_WORD(&spriteram[offs+2]) & 0x0f);
			palette_map[color] |= 0xffff;
		}

		/* now build the final table */
		for (i = 0; i < 128; i++)
		{
			int usage = palette_map[i], j;
			if (usage)
			{
				palette_used_colors[i * 16 + 0] = PALETTE_COLOR_TRANSPARENT;
				for (j = 1; j < 16; j++)
					if (usage & (1 << j))
						palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
					else
						palette_used_colors[i * 16 + j] = PALETTE_COLOR_UNUSED;
			}
			else
				memset (&palette_used_colors[i * 16 + 0], PALETTE_COLOR_UNUSED, 16);
		}

		/* recalc */
		if (palette_recalc ())
			memset(dirtybuffer,1,punkshot_vidram_size / 2);
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0x3000 - 2;offs >= 0x2000;offs -= 2)
	{
		int sx,sy,tile,code,color;


		tile = READ_WORD(&punkshot_vidram[offs]);
		color = fg_colorbase + ((tile & 0xe000) >> 13);

		if (dirtybuffer[offs / 2])
		{
			dirtybuffer[offs / 2] = 0;

			sx = (offs/2) % 64;
			sy = (offs/2) / 64 - 64;

			code = (tile & 0x03ff) + ((tile & 0x1000) >> 2) +
					0x800 * charrombank[(tile & 0x0c00) >> 10];

			drawgfx(tmpbitmap,Machine->gfx[0],
					code,
					color,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	for (offs = 0x5000 - 2;offs >= 0x4000;offs -= 2)
	{
		int sx,sy,tile,code,color;


		tile = READ_WORD(&punkshot_vidram[offs]);
		color = bg_colorbase + ((tile & 0xe000) >> 13);

		if (dirtybuffer[offs / 2])
		{
			dirtybuffer[offs / 2] = 0;

			sx = (offs/2) % 64;
			sy = (offs/2) / 64 - 128;

			code = (tile & 0x03ff) + ((tile & 0x1000) >> 2) +
					0x800 * charrombank[(tile & 0x0c00) >> 10];

			drawgfx(tmpbitmap1,Machine->gfx[0],
					code,
					color,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */

	if (priority == 0x3c)
	{
		int xscroll,yscroll;


		xscroll = -((READ_WORD(&punkshot_scrollram[0]) >> 8) + 256 * (READ_WORD(&punkshot_scrollram[2]) >> 8));
		xscroll += 6;
		yscroll = -(READ_WORD(punkshot_yscroll) >> 8);
		copyscrollbitmap(bitmap,tmpbitmap,1,&xscroll,1,&yscroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	{
		int xscroll[256],yscroll;


		yscroll = -(READ_WORD(punkshot_yscroll) & 0xff);
		for (offs = 0;offs < 256;offs++)
		{
			xscroll[(offs - yscroll) & 0xff] = -((READ_WORD(&punkshot_scrollram[4*offs]) & 0xff) + 256 * (READ_WORD(&punkshot_scrollram[4*offs+2]) & 0xff));
			xscroll[(offs - yscroll) & 0xff] += 6;
		}
		if (priority == 0x3c)
			copyscrollbitmap(bitmap,tmpbitmap1,256,xscroll,1,&yscroll,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
		else
			copyscrollbitmap(bitmap,tmpbitmap1,256,xscroll,1,&yscroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

	if (priority == 0x26) punkshot_drawsprites(bitmap,0);

	if (priority != 0x3c)
	{
		int xscroll,yscroll;


		xscroll = -((READ_WORD(&punkshot_scrollram[0]) >> 8) + 256 * (READ_WORD(&punkshot_scrollram[2]) >> 8));
		xscroll += 6;
		yscroll = -(READ_WORD(punkshot_yscroll) >> 8);
		copyscrollbitmap(bitmap,tmpbitmap,1,&xscroll,1,&yscroll,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	}


	if (priority != 0x26) punkshot_drawsprites(bitmap,0);


	/* draw the foreground characters */
	for (offs = 0x1000 - 2;offs >= 0;offs -= 2)
	{
		int sx,sy,tile,code,color;


		sx = (offs/2) % 64;
		sy = (offs/2) / 64;
		tile = READ_WORD(&punkshot_vidram[offs]);
		color = text_colorbase + ((tile & 0xe000) >> 13);

		code = (tile & 0x03ff) + ((tile & 0x1000) >> 2) +
				0x800 * charrombank[(tile & 0x0c00) >> 10];

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				0,0,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}

	punkshot_drawsprites(bitmap,1);
}
