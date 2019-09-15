/***************************************************************************

  Bionic Commando Video Hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "osdepend.h"

int bionicc_scroll1_size;
int bionicc_scroll2_size;
unsigned char *bionicc_scroll1;
unsigned char *bionicc_scroll2;
unsigned char *bionicc_palette;



static int scroll1x, scroll1y, scroll2x, scroll2y;


int bionicc_videoreg_r( int offset )
{
	switch( offset )
	{
		case 0x0: return scroll1x;
		case 0x2: return scroll1y;
		case 0x4: return scroll2x;
		case 0x6: return scroll2y;
	}
	return 0;
}

void bionicc_videoreg_w( int offset, int data )
{
	switch( offset )
	{
		case 0x0: scroll1x = data; break;
		case 0x2: scroll1y = data; break;
		case 0x4: scroll2x = data; break;
		case 0x6: scroll2y = data; break;
	}
}

#define COLORTABLE_START(gfxn,color_code) Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + \
				color_code * Machine->gfx[gfxn]->color_granularity
#define GFX_COLOR_CODES(gfxn) Machine->gfx[gfxn]->total_colors
#define GFX_ELEM_COLORS(gfxn) Machine->gfx[gfxn]->color_granularity


int bionicc_vh_start(void)
{
	int i,j;
	scroll1x = scroll1y = scroll2x = scroll2y = 0;

	memset(palette_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors * sizeof(unsigned char));

	for (j=0; j<4; j++)
	{
		for (i = 0;i < GFX_COLOR_CODES(j);i++)
		{
			memset(&palette_used_colors[COLORTABLE_START(j,i)],
				PALETTE_COLOR_USED,
				GFX_ELEM_COLORS(j));
		}
	}
	return 0;
}


void bionicc_vh_stop(void)
{
}


INLINE void bionicc_refresh_palette( void )
{
	/* rebuild the colour lookup table from RAM palette */
	int bank;
	int n=0;

	/* sprite palette RAM area describes 256 colors; all others describe 64 colors */
	int counts[4]={64,64,256,64};
	for( bank=0; bank<4; bank++ ){
		const unsigned char *source = &bionicc_palette[0x200*bank];
		int indx;
		int count=counts[bank];
		for( indx=0; indx < count; indx++ ){
			int palette, red, green, blue, bright;
			palette=READ_WORD(source);
			bright= (palette&0x0f);
                        if (bright != 0x0f)
                        {
                              bright <<= 1;
                        }
                        if (bright) bright += 2;
			red   = ((palette>>12)&0x0f) * bright;
			green = ((palette>>8 )&0x0f) * bright;
			blue  = ((palette>>4 )&0x0f) * bright;
			palette_change_color(n+indx, red, green, blue);
			source+=2;
		}
		n+=count;
	}
	palette_recalc();
}


/* Only call drawgfx for visible tiles */
INLINE void bionicc_draw_scroll2( struct osd_bitmap *bitmap, int priority )
{
	static int bank = 0;
        int transparency = priority?0x8000:0x8000;
	const struct GfxElement *gfx = Machine->gfx[0];
	const struct rectangle *clip = &Machine->drv->visible_area;

	unsigned char *source;

	int scrollx = scroll2x >> 3;
	int scrolly = scroll2y >> 3;

	int x,y, sx, sy;

	sy=-(scroll2y&0x07);
	for( y=0; y<0x21; y++ )
	{
		source = &bionicc_scroll2[((scrolly+y)*0x100)&0x3fff];
		sx=-(scroll2x&0x07);
		for( x=0; x<0x21; x++ )
		{
			int offset=(((scrollx+x)*4) & 0xff);
			int attributes = READ_WORD( &source[offset+2] );
			int flipx = attributes&0x80;
			int flipy = attributes&0x40;
			int tile_number = (READ_WORD(&source[offset])&0xff) +
					  256*(attributes&0x7);
			int color = (attributes>>3)&0x3;
			drawgfx( bitmap,gfx,
				tile_number,
				color,
				flipx, flipy,
				sx,sy,
                                clip,TRANSPARENCY_PENS,transparency);
			sx+=8;
		}
		sy+=8;
	}
}

INLINE void bionicc_draw_scroll1( struct osd_bitmap *bitmap, int priority )
{
	const struct GfxElement *gfx = Machine->gfx[1];
	const struct rectangle *clip = &Machine->drv->visible_area;

	unsigned char *source;

	int scrollx = scroll1x >> 4;
	int scrolly = scroll1y >> 4;

	int x,y, sx, sy;

	sy=-(scroll1y&0x0f);

	for( y=0; y<0x12; y++ )
	{
		source = &bionicc_scroll1[((scrolly+y)*0x100)&0x3fff];
		sx=-(scroll1x&0x0f);
		for( x=0; x<0x12; x++ )
		{
			int offset=(((scrollx+x)*4) & 0xff);
			int attributes = READ_WORD(&source[offset+2]);
			int flipx = attributes&0x80;
			int flipy = attributes&0x40;
			int tile_priority;

			if( flipx && flipy ){
				tile_priority = 2;
				flipx = flipy = 0;
			}
			else {
				tile_priority = (attributes>>5)&0x1;
			}

			if( tile_priority == priority ){
				int tile_number = (READ_WORD(&source[offset])&0xFF) + 256*(attributes&0x7);
				int color = (attributes>>3)&0x3;

				drawgfx( bitmap, gfx,
					tile_number,
					color,
					flipx,flipy,
					sx,sy,
					clip,TRANSPARENCY_PEN,15);
			}
			sx+=16;
		}
		sy+=16;
	}
}

INLINE void bionicc_draw_sprites( struct osd_bitmap *bitmap )
{
	const struct GfxElement *gfx = Machine->gfx[2];
	const struct rectangle *clip = &Machine->drv->visible_area;
	unsigned char *source = &spriteram[0x4F8];

	while( source>=spriteram ){
		int tile_number = READ_WORD(&source[0])&0x7ff;
		if( tile_number!=0x7FF ){
			int attributes = READ_WORD(&source[2]);
			int color = (attributes&0x1C)>>2;
			int flipx = attributes&0x02;
			int flipy = 0;
			int sx= (signed short)READ_WORD(&source[6]);
			int sy= ((signed short)READ_WORD(&source[4]));
			if(sy>512-16) sy-=512;

			drawgfx( bitmap, gfx,
				tile_number,
				color,
				flipx,flipy,
				sx,sy,
				clip,TRANSPARENCY_PEN,15);
		}
		source -= 8;
	}
}

INLINE void bionicc_draw_text( struct osd_bitmap *bitmap )
{
	const struct GfxElement *gfx = Machine->gfx[3];
	const unsigned char *source = videoram;
	int sx,sy;

	for( sy=0; sy<256; sy+=8 ){
		for( sx=0; sx<256; sx+=8 ){
			int attributes = READ_WORD(&source[0x0800]);
			int tile_number = (READ_WORD(&source[0]) & 0xff)|((attributes&0x01c0)<<2);
			if( tile_number != 0x20)
			{
				int color = attributes&0xf;

				drawgfx(bitmap,gfx,
					tile_number,
					color,
					0,0, /* no flip */
					sx,sy,
					0, /* no need to clip */
					TRANSPARENCY_PEN,3);
			}
			source+=2;
		}
	}
}

void bionicc_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	bionicc_refresh_palette();
	fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);
	bionicc_draw_scroll2( bitmap, 0 );      /* SCROLL2 (back) */
	bionicc_draw_scroll1( bitmap, 2 );      /* SCROLL1 (special - appears behind background) */
	bionicc_draw_scroll2( bitmap, 1 );      /* SCROLL2 (front) */
	bionicc_draw_scroll1( bitmap, 0 );      /* SCROLL1 (normal)*/
	bionicc_draw_sprites( bitmap );         /* sprites */
	bionicc_draw_scroll1( bitmap, 1 );      /* SCROLL1 (appears in front of sprites) */
	bionicc_draw_text( bitmap );            /* VRAM (text layer) */
}


