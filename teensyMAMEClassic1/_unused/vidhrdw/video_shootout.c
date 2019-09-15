#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *shootout_textram;

#if 0
static unsigned char unknown_prom[] = { /* priority? */
	0x00,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x00,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
	0x01,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
	0x01,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01
};
#endif

void shootout_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}
}

static void draw_sprites( int priority, struct osd_bitmap *bitmap ) {
	const struct rectangle *clip = &Machine->drv->visible_area;

	unsigned char *source = spriteram;
	unsigned char *finish = source+spriteram_size;

	while( source < finish ){
		int attributes = source[1]; // TTTS PFXE

		if ( attributes & 0x01 ){ /* enabled */
			if( ( ( attributes >> 3 ) & 1 ) == priority ){ /* not perfect, but better */
				int sx = 240 - source[2];
				int sy = 240 - source[0];
				int code = source[3] + 256*( (attributes>>5) & 3 );
				int flipx = attributes & 0x04;
				int bank = ( attributes & 0x80 ) ? 2 : 1;
				// attributes & 0x2 ?

				if (attributes & 0x10 ){ /* double height */
					drawgfx(bitmap,Machine->gfx[bank],
						code & ~1,
						0,
						flipx,0,
						sx,sy - 16,
						clip,TRANSPARENCY_PEN,0);

					drawgfx(bitmap,Machine->gfx[bank],
						code | 1,
						0,
						flipx,0,
						sx,sy,
						clip,TRANSPARENCY_PEN,0);
				}
				else
				{
					drawgfx(bitmap,Machine->gfx[bank],
						code,
						0,
						flipx,0,
						sx,sy,
						clip,TRANSPARENCY_PEN,0);
				}
			} /* sprite priority */
		} /* sprite enabled */
		source+=4; /* process next sprite */
	} /* next sprite */
}

static void draw_background( struct osd_bitmap *bitmap ){
	const struct rectangle *clip = &Machine->drv->visible_area;
	int offs;
	for( offs=0; offs<videoram_size; offs++ ){
		if( dirtybuffer[offs] ){
			int sx = (offs%32)*8;
			int sy = (offs/32)*8;
			int attributes = colorram[offs]; /* CCCC XTTT */
			int tile_number = videoram[offs] + 256*(attributes & 3);
			int color = attributes>>4;

			drawgfx(tmpbitmap,Machine->gfx[(attributes&0x04)?4:3],
					tile_number,
					color,
					0,0,
					sx,sy,
					clip,TRANSPARENCY_NONE,0);

			dirtybuffer[offs] = 0;
		}
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}

static void draw_foreground( struct osd_bitmap *bitmap ){
	const struct rectangle *clip = &Machine->drv->visible_area;
	const struct GfxElement *gfx = Machine->gfx[0];
	int sx,sy;

	unsigned char *source = shootout_textram;

	for( sy=0; sy<256; sy+=8 ){
		for( sx=0; sx<256; sx+=8 ){
			int attributes = *(source+videoram_size); /* CCCC XXTT */
			int tile_number = 256*(attributes&0x3) + *source++;
			int color = attributes>>4;

			drawgfx(bitmap,gfx,
				tile_number, /* 0..1024 */
				color,
				0,0,
				sx,sy,
				clip,TRANSPARENCY_PEN,0);
		}
	}
}

void shootout_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh){
	draw_background( bitmap );
	draw_sprites( 1, bitmap );
	draw_foreground( bitmap );
	draw_sprites( 0, bitmap );
}
