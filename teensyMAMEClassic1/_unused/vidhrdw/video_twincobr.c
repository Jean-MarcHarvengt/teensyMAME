#include "driver.h"
#include "vidhrdw/generic.h"



static unsigned char *twincobr_bgvideoram;
static unsigned char *twincobr_fgvideoram;
static int twincobr_bgvideoram_size,twincobr_fgvideoram_size;

static unsigned char txscroll[4],bgscroll[4],fgscroll[4];

static int charoffs;
static int bgoffs;
static int fgoffs;


int twincobr_vh_start(void)
{
	/* the video RAM is accessed via ports, it's not memory mapped */
	videoram_size = 0x1000;
	twincobr_fgvideoram_size = 0x2000;
	twincobr_bgvideoram_size = 0x2000;

	if ((videoram = malloc(videoram_size)) == 0)
		return 1;
	memset(videoram,0,videoram_size);

	if ((twincobr_fgvideoram = malloc(twincobr_fgvideoram_size)) == 0)
	{
		free(videoram);
		return 1;
	}
	memset(twincobr_fgvideoram,0,twincobr_fgvideoram_size);

	if ((twincobr_bgvideoram = malloc(twincobr_bgvideoram_size)) == 0)
	{
		free(twincobr_fgvideoram);
		free(videoram);
		return 1;
	}
	memset(twincobr_bgvideoram,0,twincobr_bgvideoram_size);

	if ((dirtybuffer = malloc(twincobr_bgvideoram_size / 2)) == 0)
	{
		free(twincobr_bgvideoram);
		free(twincobr_fgvideoram);
		free(videoram);
		return 1;
	}
	memset(dirtybuffer,1,twincobr_bgvideoram_size / 2);

	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,2*Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer);
		free(twincobr_bgvideoram);
		free(twincobr_fgvideoram);
		free(videoram);
		return 1;
	}

	return 0;
}

void twincobr_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
	free(dirtybuffer);
	free(twincobr_bgvideoram);
	free(twincobr_fgvideoram);
	free(videoram);
}


void twincobr_70004_w(int offset,int data)
{
   charoffs = (2 * data) % videoram_size;
}
int twincobr_7e000_r(int offset)
{
	return READ_WORD(&videoram[charoffs]);
}
void twincobr_7e000_w(int offset,int data)
{
	WRITE_WORD(&videoram[charoffs],data);
}

void twincobr_72004_w(int offset,int data)
{
	bgoffs = (2 * data) % twincobr_bgvideoram_size;
}
int twincobr_7e002_r(int offset)
{
	return READ_WORD(&twincobr_bgvideoram[bgoffs]);
}
void twincobr_7e002_w(int offset,int data)
{
	WRITE_WORD(&twincobr_bgvideoram[bgoffs],data);
	dirtybuffer[bgoffs / 2] = 1;
}

void twincobr_74004_w(int offset,int data)
{
	fgoffs = (2 * data) % twincobr_fgvideoram_size;
}
int twincobr_7e004_r(int offset)
{
	return READ_WORD(&twincobr_fgvideoram[fgoffs]);
}
void twincobr_7e004_w(int offset,int data)
{
	WRITE_WORD(&twincobr_fgvideoram[fgoffs],data);
}



void twincobr_txscroll_w(int offset,int data)
{
	WRITE_WORD(&txscroll[offset],data);
}

void twincobr_bgscroll_w(int offset,int data)
{
	WRITE_WORD(&bgscroll[offset],data);
}

void twincobr_fgscroll_w(int offset,int data)
{
	WRITE_WORD(&fgscroll[offset],data);
}



void twincobr_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


memset(palette_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors * sizeof(unsigned char));

{
	int color,code,i;
	int colmask[64];
	int pal_base;


	pal_base = Machine->drv->gfxdecodeinfo[2].color_codes_start;

	for (color = 0;color < 16;color++) colmask[color] = 0;

	for (offs = twincobr_bgvideoram_size - 2;offs >= 0;offs -= 2)
	{
		code = READ_WORD(&twincobr_bgvideoram[offs]) & 0x0fff;
		color = (READ_WORD(&twincobr_bgvideoram[offs]) & 0xf000) >> 12;
		colmask[color] |= Machine->gfx[2]->pen_usage[code];
	}

	for (color = 0;color < 16;color++)
	{
		for (i = 0;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}


	pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;

	for (color = 0;color < 16;color++) colmask[color] = 0;

	for (offs = twincobr_fgvideoram_size - 2;offs >= 0;offs -= 2)
	{
		code = READ_WORD(&twincobr_fgvideoram[offs]) & 0x0fff;	/* there must be a bank selector */
		color = (READ_WORD(&twincobr_fgvideoram[offs]) & 0xf000) >> 12;
		colmask[color] |= Machine->gfx[1]->pen_usage[code];
	}

	for (color = 0;color < 16;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}


	pal_base = Machine->drv->gfxdecodeinfo[3].color_codes_start;

	for (color = 0;color < 64;color++) colmask[color] = 0;

	for (offs = 0;offs < spriteram_size;offs += 8)
	{
		code = READ_WORD(&spriteram[offs]) & 0x7ff;
		color = READ_WORD(&spriteram[offs + 2]) & 0x3f;
		colmask[color] |= Machine->gfx[3]->pen_usage[code];
	}

	for (color = 0;color < 64;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}


	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;

	for (color = 0;color < 32;color++) colmask[color] = 0;

	for (offs = videoram_size - 2;offs >= 0;offs -= 2)
	{
		code = READ_WORD(&videoram[offs]) & 0x03ff;
		color = (READ_WORD(&videoram[offs]) & 0xf800) >> 11;
		colmask[color] |= Machine->gfx[0]->pen_usage[code];
	}

	for (color = 0;color < 32;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 8 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 8;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 8 * color + i] = PALETTE_COLOR_USED;
		}
	}


	if (palette_recalc())
	{
		memset(dirtybuffer,1,twincobr_bgvideoram_size / 2);
	}
}



	/* draw the background */
	for (offs = twincobr_bgvideoram_size - 2;offs >= 0;offs -= 2)
	{
		if (dirtybuffer[offs / 2])
		{
			int sx,sy,code,color;


			dirtybuffer[offs / 2] = 0;

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;

			code = READ_WORD(&twincobr_bgvideoram[offs]) & 0x0fff;
			color = (READ_WORD(&twincobr_bgvideoram[offs]) & 0xf000) >> 12;

			drawgfx(tmpbitmap,Machine->gfx[2],
					code,
					color,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the background graphics */
	{
		int scrollx,scrolly;


		scrollx = 0x1c9 - READ_WORD(&bgscroll[0]);
		scrolly = - 0x1e - READ_WORD(&bgscroll[2]);

		copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* draw the foreground */
	for (offs = twincobr_fgvideoram_size - 2;offs >= 0;offs -= 2)
	{
		int sx,sy,code,color;
		int scrollx,scrolly;


		sx = (offs/2) % 64;
		sy = (offs/2) / 64;

		scrollx = 0x1c9 - READ_WORD(&fgscroll[0]);
		scrolly = - 0x1e - READ_WORD(&fgscroll[2]);

		code = READ_WORD(&twincobr_fgvideoram[offs]) & 0x0fff;	/* TODO: there must be a bank selector */
		color = (READ_WORD(&twincobr_fgvideoram[offs]) & 0xf000) >> 12;

		drawgfx(bitmap,Machine->gfx[1],
				code,
				color,
				0,0,
				((8*sx + scrollx + 8) & 0x1ff) - 8,((8*sy + scrolly + 8) & 0x1ff) - 8,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 8)
	{
		int code,color,sx,sy,flipx,flipy;


		code = READ_WORD(&spriteram[offs]) & 0x7ff;
		color = READ_WORD(&spriteram[offs + 2]) & 0x3f;
		sx = READ_WORD(&spriteram[offs + 4]) >> 7;
		sy = READ_WORD(&spriteram[offs + 6]) >> 7;
		flipx = READ_WORD(&spriteram[offs + 2]) & 0x100;
		if (flipx) sx -= 15;
		flipy = READ_WORD(&spriteram[offs + 2]) & 0x200;

		drawgfx(bitmap,Machine->gfx[3],
				code,
				color,
				flipx,flipy,
				sx-32,sy-16,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* draw the front layer */
	for (offs = videoram_size - 2;offs >= 0;offs -= 2)
	{
		int sx,sy,code,color;
		int scrollx,scrolly;


		sx = (offs/2) % 64;
		sy = (offs/2) / 64;

		scrollx = 0x01c9 - READ_WORD(&txscroll[0]);
		scrolly = -0x1e - READ_WORD(&txscroll[2]);

		code = READ_WORD(&videoram[offs]) & 0x07ff;
		color = (READ_WORD(&videoram[offs]) & 0xf800) >> 11;

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				0,0,
				((8*sx + scrollx + 8) & 0x1ff) - 8,((8*sy + scrolly + 8) & 0xff) - 8,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
