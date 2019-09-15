#include "driver.h"
#include "vidhrdw/generic.h"


static struct osd_bitmap *tmpbitmap2;
static int scroll0,scroll1;
static int palette_bank;
static const unsigned char *pacland_color_prom;

static struct rectangle spritevisiblearea =
{
	3*8, 39*8-1,
	5*8, 29*8-1
};


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Pacland has one 1024x8 and one 1024x4 palette PROM; and three 1024x8 lookup
  table PROMs (sprites, bg tiles, fg tiles).
  The palette has 1024 colors, but it is bank switched (4 banks) and only 256
  colors are visible at a time. So, instead of creating a static palette, we
  modify it when the bank switching takes place.
  The color PROMs are connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- GREEN
		-- 470 ohm resistor  -- GREEN
		-- 1  kohm resistor  -- GREEN
        -- 2.2kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
		-- 470 ohm resistor  -- RED
		-- 1  kohm resistor  -- RED
  bit 0 -- 2.2kohm resistor  -- RED

  bit 3 -- 220 ohm resistor  -- BLUE
		-- 470 ohm resistor  -- BLUE
		-- 1  kohm resistor  -- BLUE
  bit 0 -- 2.2kohm resistor  -- BLUE

***************************************************************************/
void pacland_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	pacland_color_prom = color_prom;	/* we'll need this later */
	/* skip the palette data, it will be initialized later */
	color_prom += 2 * 1024;
	/* color_prom now points to the beginning of the lookup table */

	/* Sprites */
	for (i = 0;i < TOTAL_COLORS(2)/2;i++)
	{
		COLOR(2,i) = *(color_prom++);
		/* transparent colors are 0x7f and 0xff - map all to 0xff */
		if (COLOR(2,i) == 0x7f) COLOR(2,i) = 0xff;

		/* high priority colors which appear over the foreground even when */
		/* the foreground has priority over sprites */
		if (COLOR(2,i) >= 0xf0) COLOR(2,i + TOTAL_COLORS(2)/2) = COLOR(2,i);
		else COLOR(2,i + TOTAL_COLORS(2)/2) = 0xff;
	}

	/* Foreground */
	for (i = 0;i < TOTAL_COLORS(0);i++)
	{
		COLOR(0,i) = *(color_prom++);
		/* transparent colors are 0x7f and 0xff - map all to 0xff */
		if (COLOR(0,i) == 0x7f) COLOR(0,i) = 0xff;
	}

	/* Background */
	for (i = 0;i < TOTAL_COLORS(1);i++)
	{
		COLOR(1,i) = *(color_prom++);
	}

	/* Intialize transparency */
	if (palette_used_colors)
	{
		memset (palette_used_colors, PALETTE_COLOR_USED, Machine->drv->total_colors * sizeof (unsigned char));
		palette_used_colors[0xff] = PALETTE_COLOR_TRANSPARENT;
	}
}



int pacland_vh_start( void )
{
	if ( ( dirtybuffer = malloc( videoram_size ) ) == 0)
		return 1;
	memset (dirtybuffer, 1, videoram_size);

	if ( ( tmpbitmap = osd_create_bitmap( 64*8, 32*8 ) ) == 0 )
	{
		free( dirtybuffer );
		return 1;
	}

	if ( ( tmpbitmap2 = osd_create_bitmap( 64*8, 32*8 ) ) == 0 )
	{
		osd_free_bitmap(tmpbitmap2);
		free( dirtybuffer );
		return 1;
	}

	palette_bank = -1;

	return 0;
}

void pacland_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	free( dirtybuffer );
	osd_free_bitmap( tmpbitmap );
}




void pacland_scroll0_w(int offset,int data)
{
	scroll0 = data + 256 * offset;
}

void pacland_scroll1_w(int offset,int data)
{
	scroll1 = data + 256 * offset;
}



void pacland_bankswitch_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + ((data & 0x07) << 13);
	cpu_setbank(1,&RAM[bankaddress]);

//	pbc = data & 0x20;

	if (palette_bank != ((data & 0x18) >> 3))
	{
		int i;
		const unsigned char *color_prom;

		palette_bank = (data & 0x18) >> 3;
		color_prom = pacland_color_prom + 256 * palette_bank;

		for (i = 0;i < 256;i++)
		{
			int bit0,bit1,bit2,bit3;
			int r,g,b;

			bit0 = (color_prom[0] >> 0) & 0x01;
			bit1 = (color_prom[0] >> 1) & 0x01;
			bit2 = (color_prom[0] >> 2) & 0x01;
			bit3 = (color_prom[0] >> 3) & 0x01;
			r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
			bit0 = (color_prom[0] >> 4) & 0x01;
			bit1 = (color_prom[0] >> 5) & 0x01;
			bit2 = (color_prom[0] >> 6) & 0x01;
			bit3 = (color_prom[0] >> 7) & 0x01;
			g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
			bit0 = (color_prom[1024] >> 0) & 0x01;
			bit1 = (color_prom[1024] >> 1) & 0x01;
			bit2 = (color_prom[1024] >> 2) & 0x01;
			bit3 = (color_prom[1024] >> 3) & 0x01;
			b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

			color_prom++;

			palette_change_color(i,r,g,b);
		}
	}
}



#define DRAW_SPRITE( code, sx, sy ) \
		{ drawgfx( bitmap, Machine->gfx[ 2+gfx ], code, color, flipx, flipy, sx, sy, \
		&spritevisiblearea, TRANSPARENCY_COLOR,0xff); }

static void pacland_draw_sprites( struct osd_bitmap *bitmap,int priority)
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 2)
	{
		int sprite = spriteram[offs];
		int gfx = ( spriteram_3[offs] >> 7 ) & 1;
		int color = ( spriteram[offs+1] & 0x3f ) + 64 * priority;
		int x = (spriteram_2[offs+1]) + 0x100*(spriteram_3[offs+1] & 1) - 48;
		int y = 256 - spriteram_2[offs] - 24;
		int flipy = spriteram_3[offs] & 2;
		int flipx = spriteram_3[offs] & 1;

		switch ( spriteram_3[offs] & 0x0c )
		{
			case 0:		/* normal size */
				DRAW_SPRITE( sprite, x, y )
			break;

			case 4:		/* 2x horizontal */
				sprite &= ~1;
				if (!flipx)
				{
					DRAW_SPRITE( sprite, x, y )
					DRAW_SPRITE( 1+sprite, x+16, y )
				} else {
					DRAW_SPRITE( 1+sprite, x, y )
					DRAW_SPRITE( sprite, x+16, y )
				}
			break;

			case 8:		/* 2x vertical */
				sprite &= ~2;
				if (!flipy)
				{
					DRAW_SPRITE( sprite, x, y-16 )
					DRAW_SPRITE( 2+sprite, x, y )
				} else {
					DRAW_SPRITE( 2+sprite, x, y-16 )
					DRAW_SPRITE( sprite, x, y )
				}
			break;

			case 12:		/* 2x both ways */
				sprite &= ~3;
				if ( !flipy && !flipx )
				{
					DRAW_SPRITE( sprite, x, y-16 )
					DRAW_SPRITE( 1+sprite, x+16, y-16 )
					DRAW_SPRITE( 2+sprite, x, y )
					DRAW_SPRITE( 3+sprite, x+16, y )
				} else if ( flipy && flipx ) {
					DRAW_SPRITE( 3+sprite, x, y-16 )
					DRAW_SPRITE( 2+sprite, x+16, y-16 )
					DRAW_SPRITE( 1+sprite, x, y )
					DRAW_SPRITE( sprite, x+16, y )
				} else if ( flipx ) {
					DRAW_SPRITE( 1+sprite, x, y-16 )
					DRAW_SPRITE( sprite, x+16, y-16 )
					DRAW_SPRITE( 3+sprite, x, y )
					DRAW_SPRITE( 2+sprite, x+16, y )
				} else /* flipy */ {
					DRAW_SPRITE( 2+sprite, x, y-16 )
					DRAW_SPRITE( 3+sprite, x+16, y-16 )
					DRAW_SPRITE( sprite, x, y )
					DRAW_SPRITE( 1+sprite, x+16, y )
				}
				break;
		}
	}
}



void pacland_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int sx,sy, code, flipx, flipy, color;


	/* recalc the palette if necessary */
	if (palette_recalc ())
		memset (dirtybuffer, 1, videoram_size);


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for ( offs = videoram_size / 2; offs < videoram_size; offs += 2 )
	{
		if ( dirtybuffer[offs] || dirtybuffer[offs+1] )
		{
			dirtybuffer[offs] = dirtybuffer[offs+1] = 0;

			sx = ( ( ( offs - ( videoram_size / 2 ) ) % 128 ) / 2 );
			sy = ( ( ( offs - ( videoram_size / 2 ) ) / 128 ) );

			flipx = videoram[offs+1] & 0x40;
			flipy = videoram[offs+1] & 0x80;

			code = videoram[offs] + ((videoram[offs+1] & 0x01) << 8);
			color = ((videoram[offs+1] & 0x3e) >> 1) + ((code & 0x1c0) >> 1);

			drawgfx(tmpbitmap,Machine->gfx[1],
					code,
					color,
					flipx,flipy,
					sx*8,sy*8,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy scrolled contents */
	{
		int i,scroll[32];

		/* x position is adjusted to make the end of level door border aligned */
		for ( i = 0; i < 32; i++ )
		{
			if ( i < 5 || i > 28 )
				scroll[i] = 2;
			else
				scroll[i] = -scroll1+2;
		}

		copyscrollbitmap( bitmap, tmpbitmap, 32, scroll, 0, 0, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0 );
	}

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for ( offs = 0; offs < videoram_size / 2; offs += 2 )
	{
		if ( dirtybuffer[offs] || dirtybuffer[offs+1] )
		{
			dirtybuffer[offs] = dirtybuffer[offs+1] = 0;

			sx = ( ( offs % 128 ) / 2 );
			sy = ( ( offs / 128 ) );

			flipx = videoram[offs+1] & 0x40;
			flipy = videoram[offs+1] & 0x80;

			code = videoram[offs] + ((videoram[offs+1] & 0x01) << 8);
			color = ((videoram[offs+1] & 0x1e) >> 1) + ((code & 0x1e0) >> 1);

			drawgfx(tmpbitmap2,Machine->gfx[0],
					code,
					color,
					flipx,flipy,
					sx*8,sy*8,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy scrolled contents */
	{
		int i,scroll[32];

		for ( i = 0; i < 32; i++ )
		{
			if ( i < 5 || i > 28 )
				scroll[i] = 0;
			else
				scroll[i] = -scroll0;
		}

		copyscrollbitmap( bitmap, tmpbitmap2, 32, scroll, 0, 0, &Machine->drv->visible_area, TRANSPARENCY_COLOR, 0xff );
	}

	pacland_draw_sprites(bitmap,0);

	/* redraw the tiles which have priority over the background */
	for ( offs = 0; offs < videoram_size / 2; offs += 2 )
	{
		if (videoram[offs+1] & 0x20)
		{
			int scroll;


			sx = ( ( offs % 128 ) / 2 );
			sy = ( ( offs / 128 ) );

			if ( sy < 5 || sy > 28 )
				scroll = 0;
			else
				scroll = -scroll0;

			if (sx*8 + scroll < -8) scroll += 512;

			flipx = videoram[offs+1] & 0x40;
			flipy = videoram[offs+1] & 0x80;

			code = videoram[offs] + ((videoram[offs+1] & 0x01) << 8);
			color = ((videoram[offs+1] & 0x1e) >> 1) + ((code & 0x1e0) >> 1);

			drawgfx(bitmap,Machine->gfx[0],
					code,
					color,
					flipx,flipy,
					sx*8 + scroll,sy*8,
					&Machine->drv->visible_area,TRANSPARENCY_COLOR,0xff);
		}
	}

	pacland_draw_sprites(bitmap,1);
}
