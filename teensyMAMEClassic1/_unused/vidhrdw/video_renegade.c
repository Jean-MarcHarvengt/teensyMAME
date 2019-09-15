/***************************************************************************

	Renegade Video Hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *renegade_textram;
int renegade_scrollx;
static unsigned char *back_buffer;

int renegade_vh_start( void ){
	int i;

	renegade_scrollx = 0;

	back_buffer = malloc(0x800);
	if( back_buffer ){
		/* the scrolling background is 4 times as wide as the screen */
		tmpbitmap = osd_create_bitmap( 4*Machine->drv->screen_width, Machine->drv->screen_height );
		if( tmpbitmap ) return 0;

		free( back_buffer );
	}


#define COLORTABLE_START(gfxn,color_code) Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + \
				color_code * Machine->gfx[gfxn]->color_granularity
#define GFX_COLOR_CODES(gfxn) Machine->gfx[gfxn]->total_colors
#define GFX_ELEM_COLORS(gfxn) Machine->gfx[gfxn]->color_granularity

	memset(palette_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors * sizeof(unsigned char));
	/* chars */
	for (i = 0;i < GFX_COLOR_CODES(0);i++)
	{
		memset(&palette_used_colors[COLORTABLE_START(0,i)],
				PALETTE_COLOR_USED,
				GFX_ELEM_COLORS(0));
	}
	/* tiles */
	for (i = 0;i < GFX_COLOR_CODES(1);i++)
	{
		memset(&palette_used_colors[COLORTABLE_START(1,i)],
				PALETTE_COLOR_USED,
				GFX_ELEM_COLORS(1));
	}
	/* sprites */
	for (i = 0;i < GFX_COLOR_CODES(9);i++)
	{
		memset(&palette_used_colors[COLORTABLE_START(9,i)],
				PALETTE_COLOR_USED,
				GFX_ELEM_COLORS(9));
	}

	return 1;
}

void renegade_vh_stop(void);
void renegade_vh_stop(void){
	osd_free_bitmap( tmpbitmap );
	free( back_buffer );
}


static void render_sprites( struct osd_bitmap *bitmap );
static void render_sprites( struct osd_bitmap *bitmap ){
	const struct rectangle *clip = &Machine->drv->visible_area;

	unsigned char *source = spriteram;
	unsigned char *finish = source+96*4;

	while( source<finish ){
		int sy = 240-source[0];
		if( sy>=16 ){
			int attributes = source[1]; /* SFCCBBBB */
			int sx = source[3];
			int sprite_number = source[2];
			int sprite_bank = 9 + (attributes&0xF);
			int color = (attributes>>4)&0x3;
			int xflip = attributes&0x40;

			if( sx>248 ) sx -= 256;

			if( attributes&0x80 ){ /* big sprite */
				drawgfx(bitmap,Machine->gfx[sprite_bank],
					sprite_number+1,
					color,
					xflip,0,
					sx,sy+16,
					clip,TRANSPARENCY_PEN,0);
			}
			else {
				sy += 16;
			}
			drawgfx(bitmap,Machine->gfx[sprite_bank],
				sprite_number,
				color,
				xflip,0,
				sx,sy,
				clip,TRANSPARENCY_PEN,0);
		}
		source+=4;
	}
}

static void render_background( struct osd_bitmap *bitmap );
static void render_background( struct osd_bitmap *bitmap ){
	const unsigned char *source = videoram;
	unsigned char *dirty = back_buffer;

	int sx, sy;
	for( sy=0; sy<256; sy+=16 ){
		for( sx=0; sx<256*4; sx+=16 ){
			unsigned char attributes = source[0x400]; /* CCC??BBB */
			unsigned char tile_number = source[0];

			if( tile_number!=dirty[0] || attributes!=dirty[0x400] ){
				drawgfx(tmpbitmap,Machine->gfx[1+(attributes&0x7)],
					tile_number,
					attributes>>5, /* color */
					0,0, /* no flip? */
					sx, sy,
					0, /* no clip */
					TRANSPARENCY_NONE,0);

				dirty[0] = tile_number;
				dirty[0x400] = attributes;
			}

			dirty++;
			source++;
		}
	}

	{
		int scrollx = 256-(renegade_scrollx&0x3ff);
		int scrolly = 0;

		copyscrollbitmap(bitmap,tmpbitmap,
			1,&scrollx,1,&scrolly,
			&Machine->drv->visible_area,
			TRANSPARENCY_NONE,0);
	}
}

static void render_text( struct osd_bitmap *bitmap );
static void render_text( struct osd_bitmap *bitmap ){
	const struct GfxElement *gfx = Machine->gfx[0];
	unsigned char *attr = &renegade_textram[0x400];

	int offset = 1;
	int sx,sy;

	for( sy=0; sy<256-16; sy+=8 ){ /* skip last 2 lines */
		for( sx=8; sx<256-8; sx+=8 ){ /* skip left and right edges */
			unsigned char attributes = attr[offset]; /* CC?? ??BB */
			int tile_number = (attributes&3)*256 + renegade_textram[offset];
			if( tile_number ){ /* skip spaces */
				drawgfx(bitmap,gfx,
					tile_number,
					attributes>>6, /*color*/
					0,0,
					sx,sy,
					0, /* no need to clip: we aren't drawing outside the clip region */
					TRANSPARENCY_PEN,0);
			}
			offset++; /* next character */
		}
		offset += 2; /* skip to next row */
	}
}

void renegade_vh_screenrefresh(struct osd_bitmap *bitmap, int fullrefresh ){
	palette_recalc();

	render_background( bitmap );
	render_sprites( bitmap );
	render_text( bitmap );
}
