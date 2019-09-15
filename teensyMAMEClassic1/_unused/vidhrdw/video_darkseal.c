/***************************************************************************

   Dark Seal Video emulation - Bryan McPhail, mish@tendril.force9.net

*********************************************************************

 Playfield 1 - 8*8 tiles
 Playfield 2 - 16*16 tiles
 Playfield 3 - 16*16 tiles

	Playfield control registers:
	Bank 0:
	0: Bit 0 clear = Flip screen
    2: (scroll?)
    4: (scroll?)
    6: Playfield 3 X scroll
    8: Playfield 3 Y scroll
   10:
   12:
   14:

   	Bank 1:
    0:
    2: Playfield 2 X scroll
    4: Playfield 2 Y scroll
    6: Playfield 1 X scroll
    8: Playfield 1 Y scroll
   10:
   12:
   14:

 All unknown registers do not change at any point in the game, except for
Bank 0, byte 12 which changes to 0x4000 at the character profiles page
in attract mode.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define TEXTRAM_SIZE	0x2000	/* Size of text layer */
#define TILERAM_SIZE	0x2000	/* Size of background and foreground */

unsigned char *darkseal_sprite;
static unsigned char *darkseal_pf1_data,*darkseal_pf2_data,*darkseal_pf3_data;
static unsigned char *darkseal_pf1_dirty,*darkseal_pf3_dirty,*darkseal_pf2_dirty;
static struct osd_bitmap *darkseal_pf1_bitmap;
static struct osd_bitmap *darkseal_pf2_bitmap;
static struct osd_bitmap *darkseal_pf3_bitmap;

static unsigned char darkseal_control_0[16];
static unsigned char darkseal_control_1[16];

static int darkseal_pf1_static,darkseal_pf2_static,darkseal_pf3_static;
static int offsetx[4],offsety[4];


/******************************************************************************/

static void update_24bitcol(int offset)
{
	int r,g,b;

	r = (READ_WORD(&paletteram[offset]) >> 0) & 0xff;
	g = (READ_WORD(&paletteram[offset]) >> 8) & 0xff;
	b = (READ_WORD(&paletteram_2[offset]) >> 0) & 0xff;

	palette_change_color(offset / 2,r,g,b);
}

void darkseal_palette_24bit_rg(int offset,int data)
{
	/* only 1280 out of 2048 colors are actually used */
	if (offset >= 2*Machine->drv->total_colors) return;

	COMBINE_WORD_MEM(&paletteram[offset],data);
	update_24bitcol(offset);
}

void darkseal_palette_24bit_b(int offset,int data)
{
	/* only 1280 out of 2048 colors are actually used */
	if (offset >= 2*Machine->drv->total_colors) return;

	COMBINE_WORD_MEM(&paletteram_2[offset],data);
	update_24bitcol(offset);
}

int darkseal_palette_24bit_rg_r(int offset)
{
	return READ_WORD(&paletteram[offset]);
}

int darkseal_palette_24bit_b_r(int offset)
{
	return READ_WORD(&paletteram_2[offset]);
}

/******************************************************************************/

static void darkseal_update_palette(void)
{
	int offs,color,code,i,pal_base;
	int colmask[32];

	memset(palette_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors * sizeof(unsigned char));

	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;

	for (color = 0;color < 16;color++) colmask[color] = 0;

	for (offs = 0; offs < TEXTRAM_SIZE;offs += 2)
	{
		code = READ_WORD(&darkseal_pf1_data[offs]);
		color = (code & 0xf000) >> 12;
		code &= 0x0fff;
		colmask[color] |= Machine->gfx[0]->pen_usage[code];
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


	pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;

	for (color = 0;color < 16;color++) colmask[color] = 0;

	for (offs = 0; offs < TILERAM_SIZE;offs += 2)
	{
		code = READ_WORD(&darkseal_pf2_data[offs]);
		color = (code & 0xf000) >> 12;
		code &= 0x0fff;
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

	pal_base = Machine->drv->gfxdecodeinfo[2].color_codes_start;

	for (color = 0;color < 16;color++) colmask[color] = 0;

	for (offs = 0; offs < TILERAM_SIZE;offs += 2)
	{
		code = READ_WORD(&darkseal_pf3_data[offs]);
		color = (code & 0xf000) >> 12;
		code &= 0x0fff;
		colmask[color] |= Machine->gfx[2]->pen_usage[code];
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

  /* Sprites */
	pal_base = Machine->drv->gfxdecodeinfo[3].color_codes_start;
	for (color = 0;color < 32;color++) colmask[color] = 0;

	for (offs = 0;offs < 0x800;offs += 8)
	{
		int x,y,sprite,multi;

    sprite = READ_WORD (&darkseal_sprite[offs+2]) & 0x1fff;
    if (!sprite) continue;

		y = READ_WORD(&darkseal_sprite[offs]);
		x = READ_WORD(&darkseal_sprite[offs+4]);
		color = (x & 0x1e00) >> 9;

		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		sprite &= ~multi;

		while (multi >= 0)
		{
			colmask[color] |= Machine->gfx[3]->pen_usage[sprite + multi];
			multi--;
		}
	}

	for (color = 0;color < 32;color++)
	{
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	if (palette_recalc())
	{
		memset(darkseal_pf1_dirty,1,TEXTRAM_SIZE);
		memset(darkseal_pf2_dirty,1,TILERAM_SIZE);
		memset(darkseal_pf3_dirty,1,TILERAM_SIZE);
    darkseal_pf1_static=1;
    darkseal_pf2_static=1;
    darkseal_pf3_static=1;
	}
}

static void darkseal_drawsprites(struct osd_bitmap *bitmap)
{
	int offs;

	for (offs = 0;offs < 0x800;offs += 8)
	{
		int x,y,sprite,colour,multi,fx,fy,inc;

    sprite = READ_WORD (&darkseal_sprite[offs+2]) & 0x1fff;
    if (!sprite) continue;

		y = READ_WORD(&darkseal_sprite[offs]);
		x = READ_WORD(&darkseal_sprite[offs+4]);

		colour = (x & 0x3e00) >> 9;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 256) x -= 512;
		if (y >= 256) y -= 512;
		x = 240 - x;
		y = 240 - y;

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[3],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y - 16 * multi,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

			multi--;
		}
	}
}

static void darkseal_pf2_update(void)
{
	int offs,mx,my,color,tile,quarter;

  darkseal_pf2_static=0;
	for (quarter = 0;quarter < 4;quarter++)
	{
		mx = -1;
		my = 0;

		for (offs = 0x800 * quarter; offs < 0x800 * quarter + 0x800;offs += 2)
		{
			mx++;
			if (mx == 32)
			{
				mx = 0;
				my++;
			}

			if (darkseal_pf2_dirty[offs])
			{
				darkseal_pf2_dirty[offs] = 0;
				tile = READ_WORD(&darkseal_pf2_data[offs]);
				color = (tile & 0xf000) >> 12;

				drawgfx(darkseal_pf2_bitmap,Machine->gfx[1],
						tile & 0x0fff,
						color,
						0,0,
						16*mx + offsetx[quarter],16*my + offsety[quarter],
						0,TRANSPARENCY_NONE,0);
			}
		}
	}
}

static void darkseal_pf3_update(void)
{
	int offs,mx,my,color,tile,quarter;

  darkseal_pf3_static=0;
	for (quarter = 0;quarter < 4;quarter++)
	{
		mx = -1;
		my = 0;

		for (offs = 0x800 * quarter; offs < 0x800 * quarter + 0x800;offs += 2)
		{
			mx++;
			if (mx == 32)
			{
				mx = 0;
				my++;
			}

			if (darkseal_pf3_dirty[offs])
			{
				darkseal_pf3_dirty[offs] = 0;
				tile = READ_WORD(&darkseal_pf3_data[offs]);
				color = (tile & 0xf000) >> 12;

				drawgfx(darkseal_pf3_bitmap,Machine->gfx[2],
						tile & 0x0fff,
						color,
						0,0,
						16*mx + offsetx[quarter],16*my + offsety[quarter],
						0,TRANSPARENCY_NONE,0);
			}
		}
	}
}

/******************************************************************************/

void darkseal_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
  int xscroll_f,yscroll_f,xscroll_b,yscroll_b;
	int scrollx,scrolly;
	int mx,my,offs,tile,color;

	darkseal_update_palette();

  /* Scroll positions */
  xscroll_f=READ_WORD (&darkseal_control_1[2]);
  yscroll_f=READ_WORD (&darkseal_control_1[4]);

  xscroll_b=READ_WORD (&darkseal_control_0[6]);
  yscroll_b=READ_WORD (&darkseal_control_0[8]);

  /* Hmm, kludge? See attract sequence */
  if ((READ_WORD(&darkseal_control_0[12])>>8)==0x40) xscroll_b+=256;

	/* Draw playfields if needed */
	if (darkseal_pf2_static)
		darkseal_pf2_update();
	if (darkseal_pf3_static)
		darkseal_pf3_update();

	/* Background */
	scrollx=-xscroll_b;
	scrolly=-yscroll_b;
	copyscrollbitmap(bitmap,darkseal_pf3_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Foreground */
	scrollx=-xscroll_f;
	scrolly=-yscroll_f;
	copyscrollbitmap(bitmap,darkseal_pf2_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

  /* Sprites */
	darkseal_drawsprites(bitmap);

	/* Playfield 1 - 8 * 8 Text */
	if (!darkseal_pf1_static) goto PF1_STATIC;
	darkseal_pf1_static=0;

	mx = -1;
	my = 0;

	for (offs = 0; offs < 0x2000 ;offs += 2)
	{
		mx++;
		if (mx == 64)
    {
			mx = 0;
			my++;
		}

		if (darkseal_pf1_dirty[offs])
		{
			darkseal_pf1_dirty[offs] = 0;
			tile = READ_WORD(&darkseal_pf1_data[offs]);
			color = (tile & 0xf000) >> 12;

			drawgfx(darkseal_pf1_bitmap,Machine->gfx[0],
					tile & 0x0fff,
					color,
					0,0,
					8*mx,8*my,
					0,TRANSPARENCY_NONE,0);
		}
  }

  PF1_STATIC:

	scrollx = -READ_WORD(&darkseal_control_1[6]);
	scrolly = -READ_WORD(&darkseal_control_1[8]);
	copyscrollbitmap(bitmap,darkseal_pf1_bitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
}

/******************************************************************************/

void darkseal_pf1_data_w(int offset,int data)
{
	int oldword = READ_WORD(&darkseal_pf1_data[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&darkseal_pf1_data[offset],newword);
		darkseal_pf1_dirty[offset] = 1;
    darkseal_pf1_static=1;
	}
}

void darkseal_pf2_data_w(int offset,int data)
{
	int oldword = READ_WORD(&darkseal_pf2_data[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&darkseal_pf2_data[offset],newword);
		darkseal_pf2_dirty[offset] = 1;
    darkseal_pf2_static=1;
	}
}

void darkseal_pf3_data_w(int offset,int data)
{
	int oldword = READ_WORD(&darkseal_pf3_data[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&darkseal_pf3_data[offset],newword);
		darkseal_pf3_dirty[offset] = 1;
    darkseal_pf3_static=1;
	}
}

void darkseal_pf3b_data_w(int offset,int data)
{
	int oldword = READ_WORD(&darkseal_pf3_data[offset+0x1000]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&darkseal_pf3_data[offset+0x1000],newword);
		darkseal_pf3_dirty[offset+0x1000] = 1;
    darkseal_pf3_static=1;
	}
}

void darkseal_control_0_w(int offset,int data)
{
	COMBINE_WORD_MEM(&darkseal_control_0[offset],data);
}

void darkseal_control_1_w(int offset,int data)
{
	COMBINE_WORD_MEM(&darkseal_control_1[offset],data);
}

/******************************************************************************/

void darkseal_vh_stop (void)
{
	osd_free_bitmap(darkseal_pf2_bitmap);
	osd_free_bitmap(darkseal_pf3_bitmap);
	osd_free_bitmap(darkseal_pf1_bitmap);
	free(darkseal_pf3_data);
	free(darkseal_pf2_data);
	free(darkseal_pf1_data);
	free(darkseal_pf3_dirty);
	free(darkseal_pf2_dirty);
	free(darkseal_pf1_dirty);
}

int darkseal_vh_start(void)
{
	/* Allocate bitmaps */
	if ((darkseal_pf1_bitmap = osd_create_bitmap(512,512)) == 0) {
		darkseal_vh_stop ();
		return 1;
	}

	if ((darkseal_pf2_bitmap = osd_create_bitmap(1024,1024)) == 0) {
		darkseal_vh_stop ();
		return 1;
	}

	if ((darkseal_pf3_bitmap = osd_create_bitmap(1024,1024)) == 0) {
		darkseal_vh_stop ();
		return 1;
	}

	darkseal_pf1_data = malloc(TEXTRAM_SIZE);
	darkseal_pf1_dirty = malloc(TEXTRAM_SIZE);
	darkseal_pf3_data = malloc(TILERAM_SIZE);
	darkseal_pf3_dirty = malloc(TILERAM_SIZE);
	darkseal_pf2_data = malloc(TILERAM_SIZE);
	darkseal_pf2_dirty = malloc(TILERAM_SIZE);

  darkseal_pf1_static=1;
  darkseal_pf2_static=1;
  darkseal_pf3_static=1;

	memset(darkseal_pf1_dirty,1,TEXTRAM_SIZE);
	memset(darkseal_pf2_dirty,1,TILERAM_SIZE);
	memset(darkseal_pf3_dirty,1,TILERAM_SIZE);

  offsetx[0] = 0;
	offsetx[1] = 512;
	offsetx[2] = 0;
	offsetx[3] = 512;
	offsety[0] = 0;
	offsety[1] = 0;
	offsety[2] = 512;
	offsety[3] = 512;

	return 0;
}

/******************************************************************************/

