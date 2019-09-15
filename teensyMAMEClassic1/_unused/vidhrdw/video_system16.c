/***************************************************************************

Sega System 16 Video Hardware

There are three logical layers of tile-mapped graphics:
- playfield background layer (bg_scroll_x, bg_scroll_y)
- playfield foreground layer (fg_scroll_x, fg_scroll_y)
- fixed text layer (sys16_textram)

Each scrolling layer (foreground, background) is an arrangement
of 4 pages selected from 16 available pages, laid out as follows:

	Page0  Page1
	Page2  Page3

A page is an arrangement of 8x8 tiles, 64 tiles wide, and 32 tiles high.

sys16_tileram contains data describing the 16 selectable pages.

Sprites are drawn between layers, depending on their priority.

Each tile in the foreground layer has a priority bit, that controls
whether it is drawn in front or behind priority 1 sprites.

The rendering order is, from back to front:

	background playfield layer
	sprites (priority 0)
	foreground playfield layer (low priority)
	sprites (priority 1)
	foreground playfield layer (high priority)
	sprites (priority 2)
	fixed text layer
	sprites (priority 3)

***************************************************************************

The following optimizations are used:

- totally obscured tiles are skipped while drawing the background

- totally transparent tiles are skipped when drawing the foreground

- a fatbitmask (one byte per pixel) is used to blit the non-transparent tiles
  (thanks to Neil Bradley and Nicola Salmoria for suggesting this)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/s16sprit.c"

#define TILEMAP_ROWS 32
#define TILEMAP_COLS 64
#define TILEMAP_HEIGHT (TILEMAP_ROWS*8)
#define TILEMAP_WIDTH (TILEMAP_COLS*8)
#define MAX_SPRITES 64
#define SCREENWIDTH 320
#define SCREENHEIGHT 224
#define SCREENCOLS (SCREENWIDTH/8)

static unsigned long screenwise_opacity[SCREENCOLS];
/*
	each unsigned long corresponds to a vertical strip of 8x8 tiles:
	(screenwise_opacity[col]>>row)&1
*/
static unsigned char fg_transparency[4][2][TILEMAP_ROWS][TILEMAP_COLS];
/*
	fg_transparency[quadrant][priority][row][col] = 1 or 0
*/

/* video ram */
extern unsigned char *sys16_textram;
extern unsigned char *sys16_spriteram;
extern unsigned char *sys16_tileram; /* contains tilemaps for 16 pages */

/* video driver constants (potentially different for each game) */
int sys16_spritesystem;
int sys16_sprxoffset;
int *sys16_obj_bank;
extern void (* sys16_update_proc)( void );

/* video registers */
int sys16_tile_bank1;
int sys16_tile_bank0;
int sys16_refreshenable;
int sys16_bg_scrollx, sys16_bg_scrolly;
int sys16_bg_page[4];
int sys16_fg_scrollx, sys16_fg_scrolly;
int sys16_fg_page[4];

/* cached graphics layers */
static int old_bg_page[4];
static int old_fg_page[4];
static unsigned short *fg_page_buffer[4];
static unsigned short *bg_page_buffer[4];
static struct osd_bitmap *bg_bitmap[4];
static struct osd_bitmap *fg_bitmap[4];
static struct osd_bitmap *fg_fatmask[4];

/***************************************************************************/

#define min(a,b) ((a<b)?a:b)
#define max(a,b) ((a>b)?a:b)

void memcpy0( unsigned char *dest, const unsigned char *source,
				const unsigned char *mask, int numbytes ){

	/*
		mask contains one byte per pixel
		0xFF indicates the corresponding source pixel is transparent
		0x00 indicates the corresponding source pixel is opaque
	*/

	const unsigned char *finish = source+numbytes;
	const unsigned char *finishw = finish-(numbytes&0x3);

#ifdef ACORN
	while( source<finishw ){
		if( *mask )
			*dest = ( *dest++ & *mask ) | ( *source++ & ~*mask++ );
		else
		{
			*dest++ = *source++;
			mask++;
		}
	}
#else
	while( source<finishw ){
		int m = *(int *)mask;
		if( m )
			*(int *)dest = ( *(int *)dest & m ) | ( *(int *)source & ~m );
		else
			*(int *)dest = *(int *)source;

		dest += 4; source += 4; mask += 4;
	}
#endif
	while( source<finish ){
		if( *mask==0 ) *dest = *source;
		dest++; source++; mask++;
	}
}

static void fast_blit_transparent( struct osd_bitmap *dest_bitmap,
	const struct osd_bitmap *source_bitmap, const struct osd_bitmap *mask_bitmap,
	int sx, int sy,
	const struct rectangle *clip,
	unsigned char layer_transparency[TILEMAP_ROWS][TILEMAP_COLS] ){

	int x1 = max(sx,clip->min_x)-sx;
	int x2 = min(sx+source_bitmap->width,clip->max_x+1)-sx;
	int y1 = max(sy,clip->min_y)-sy;
	int y2 = min(sy+source_bitmap->height,clip->max_y+1)-sy;

	if( x1<x2 && y1<y2 ){
		/* x1,x2,y1,y2 are in source_bitmap coordinates */

		int c1 = x1/8;					/* round down */
		int c2 = (x2+7)/8;				/* round up */

		int align_y1 = (y1+7)&0xfff8;	/* round up */
		int align_y2 = y2&0xfff8;		/* round down */

		int dest_line_offset = dest_bitmap->line[1] - dest_bitmap->line[0];
		int source_line_offset = source_bitmap->line[1] - source_bitmap->line[0];
		int mask_line_offset = mask_bitmap->line[1] - mask_bitmap->line[0];

		int dest_row_offset = dest_line_offset*8;
		int source_row_offset = source_line_offset*8;
		int mask_row_offset = mask_line_offset*8;

		unsigned char *dest_baseaddr = dest_bitmap->line[sy+y1]+sx;
		const unsigned char *source_baseaddr = source_bitmap->line[y1];
		const unsigned char *mask_baseaddr = mask_bitmap->line[y1];

		unsigned char *dest_next;
		const unsigned char *source_next;
		const unsigned char *mask_next;

		int lines;

		int y = y1;
		int y_next = align_y1;
		if( y_next==y1 ) y_next += 8;
		if( y_next>y2 ) y_next = y2;

		lines = y_next-y;
		dest_next = dest_baseaddr+lines*dest_line_offset;
		source_next = source_baseaddr+lines*source_line_offset;
		mask_next = mask_baseaddr+lines*mask_line_offset;


		while( y<y2 ){
			unsigned char *row_transparency = layer_transparency[y/8];
			int col;
			for( col=c1; col<c2; col++ ){
				if( row_transparency[col]==0 ){
					int x_start = col*8;
					int x_end;
					do { col++; } while( col<c2 && row_transparency[col]==0 );
					x_end = col*8;

					/* horizontal clipping */
					if( x_start<x1 ) x_start = x1;
					if( x_end>x2 ) x_end = x2;

					{
						unsigned char *dest0 = dest_baseaddr+x_start;
						const unsigned char *source0 = source_baseaddr+x_start;
						const unsigned char *mask0 = mask_baseaddr+x_start;

						int num_pixels = x_end - x_start;

						int i = y;
						for(;;){
							memcpy0( dest0, source0, mask0, num_pixels );
							if( ++i == y_next ) break;

							dest0 += dest_line_offset;
							source0 += source_line_offset;
							mask0 += mask_line_offset;
						}
					}
				}
			}

			/* next row */
			dest_baseaddr = dest_next;
			source_baseaddr = source_next;
			mask_baseaddr = mask_next;

			y = y_next;
			y_next += 8;

			if( y_next>y2 ){
				y_next = y2;
			}
			else {
				dest_next += dest_row_offset;
				source_next += source_row_offset;
				mask_next += mask_row_offset;
			}
		}
	}
}

static void fast_blit_opaque( struct osd_bitmap *dest_bitmap,
	const struct osd_bitmap *source_bitmap,
	int sx, int sy,
	const struct rectangle *clip ){

	int x1 = sx;
	int y1 = sy;
	int x2 = sx+source_bitmap->width;
	int y2 = sy+source_bitmap->height;

	if( x1 < clip->min_x ) x1 = clip->min_x;
	if( y1 < clip->min_y ) y1 = clip->min_y;
	if( x2 > clip->max_x+1 ) x2 = clip->max_x+1;
	if( y2 > clip->max_y+1 ) y2 = clip->max_y+1;

	if( x1<x2 && y1<y2 ){
		/* x1,x2,y1,y2 are in screen (dest_bitmap) coordinates */

		int c1 = x1/8;					/* round down */
		int c2 = (x2+7)/8;				/* round up */

		int align_y1 = (y1+7)&0xfff8;	/* round up */
		int align_y2 = y2&0xfff8;		/* round down */

		int dest_line_offset = dest_bitmap->line[1] - dest_bitmap->line[0];
		int source_line_offset = source_bitmap->line[1] - source_bitmap->line[0];

		int dest_row_offset = dest_line_offset*8;
		int source_row_offset = source_line_offset*8;

		unsigned char *dest_baseaddr = dest_bitmap->line[y1];
		const unsigned char *source_baseaddr = source_bitmap->line[y1-sy]-sx;

		unsigned char *dest_next;
		const unsigned char *source_next;

		int lines;

		int y = y1;
		int y_next = align_y1;
		if( y_next==y1 ) y_next += 8;
		if( y_next>y2 ) y_next = y2;

		lines = y_next-y;
		dest_next = dest_baseaddr+lines*dest_line_offset;
		source_next = source_baseaddr+lines*source_line_offset;

		while( y<y2 ){
			int row_mask = 1<<(y/8);
			int col;
			for( col=c1; col<c2; col++ ){
				if( screenwise_opacity[col]&row_mask ){
					int x_start = col*8;
					int x_end;
					do { col++; } while( col<c2 && (screenwise_opacity[col]&row_mask) );
					x_end = col*8;

					/* horizontal clipping */
					if( x_start<x1 ) x_start = x1;
					if( x_end>x2 ) x_end = x2;

					{
						unsigned char *dest0 = dest_baseaddr+x_start;
						const unsigned char *source0 = source_baseaddr+x_start;

						int num_pixels = x_end - x_start;

						int i = y;
						for(;;){
							memcpy( dest0, source0, num_pixels );
							if( ++i == y_next ) break;

							dest0 += dest_line_offset;
							source0 += source_line_offset;
						}
					}
				}
			}

			/* next row */
			dest_baseaddr = dest_next;
			source_baseaddr = source_next;

			y = y_next;
			y_next += 8;

			if( y_next>y2 ){
				y_next = y2;
			}
			else {
				dest_next += dest_row_offset;
				source_next += source_row_offset;
			}
		}
	}
}

/***************************************************************************/

static void draw_background(struct osd_bitmap *bitmap){
	const struct rectangle *clip = &Machine->drv->visible_area;

	int page;
	int scrollx = 320+sys16_bg_scrollx;
	int scrolly = 256-sys16_bg_scrolly;
	static int old_scrollx = -1, old_scrolly = -1;

    if (scrollx < 0) scrollx = 1024 - (-scrollx) % 1024;
	else scrollx = scrollx % 1024;

	if (scrolly < 0) scrolly = 512 - (-scrolly) % 512;
	else scrolly = scrolly % 512;

	if (old_scrollx != scrollx || old_scrolly != scrolly)
	{
		old_scrollx = scrollx;
		old_scrolly = scrolly;
		osd_mark_dirty(clip->min_x, clip->min_y, clip->max_x, clip->max_y, 0);
	}

    for (page=0; page < 4; page++){
		int sx = (page&1)*512+scrollx;
		int sy = (page>>1)*256+scrolly;

		if( sx>320 ) sx-=1024;	/* wraparound */
		if( sy>224 ) sy-=512;	/* wraparound */

		fast_blit_opaque( bitmap, bg_bitmap[page],
			sx,sy,
			clip );
	}
}

static void draw_foreground(struct osd_bitmap *bitmap, int priority ){
	const struct rectangle *clip = &Machine->drv->visible_area;

	int page;

	int scrollx = 320+sys16_fg_scrollx;
	int scrolly = 256-sys16_fg_scrolly;
	static int old_scrollx = -1, old_scrolly = -1;

	if (scrollx < 0) scrollx = 1024 - (-scrollx) % 1024;
	else scrollx = scrollx % 1024;

	if (scrolly < 0) scrolly = 512 - (-scrolly) % 512;
	else scrolly = scrolly % 512;

	if (old_scrollx != scrollx || old_scrolly != scrolly)
	{
		old_scrollx = scrollx;
        old_scrolly = scrolly;
        osd_mark_dirty(clip->min_x, clip->min_y, clip->max_x, clip->max_y, 0);
	}

    for (page=0; page < 4; page++){
		int sx = (page&1)*512+scrollx;
		int sy = (page>>1)*256+scrolly;
		if( sx>320 ) sx-=1024;	/* wraparound */
		if( sy>224 ) sy-=512;	/* wraparound */

		fast_blit_transparent( bitmap, fg_bitmap[page], fg_fatmask[page],
			sx,sy,
			clip, fg_transparency[page][priority] );
	}
}


static void dirty_all( void ){
	/* mark each cached layer as invalid */
	int page;
	for( page=0; page<4; page++ ) old_bg_page[page] = old_fg_page[page] = -1;
}

/***************************************************************************/

static void update_background( void ){
	const struct GfxElement *gfx = Machine->gfx[0];

	int page;
	for( page=0; page<4; page++ ){
		struct osd_bitmap *bitmap = bg_bitmap[page];
		int all = (old_bg_page[page]!=sys16_bg_page[page]);

		const unsigned short *new_source = ((unsigned short *)sys16_tileram) + sys16_bg_page[page]*0x800;
		unsigned short *old_source = bg_page_buffer[page];

		int sx,sy;
		for( sy=0; sy<TILEMAP_HEIGHT; sy+=8 ){
			for( sx=0; sx<TILEMAP_WIDTH; sx+=8 ){
				unsigned short data = *new_source++;
				if( all || data!=*old_source ){
					int tile_number = (data&0xfff) +
						0x1000*((data&0x1000)?sys16_tile_bank1:sys16_tile_bank0);

					drawgfx( bitmap, gfx,
						tile_number,
						(data>>6)&0x7f, /* color */
						0, 0, /* no need to flip */
						sx, sy,
						0, /* no need to clip */
						TRANSPARENCY_NONE, 0);
					osd_mark_dirty(sx, sy, sx+TILEMAP_WIDTH-1, sy+TILEMAP_HEIGHT-1, 0);

					*old_source = data;
				}
				old_source++;
			} /* x */
		} /* y */
		old_bg_page[page]=sys16_bg_page[page];
	} /* page */
}

static void update_foreground( void ){
	const struct GfxElement *gfx = Machine->gfx[0];
	struct rectangle rect;

	int page;
	for( page=0; page<4; page++ ){
		struct osd_bitmap *bitmap = fg_bitmap[page];
		struct osd_bitmap *fatmask = fg_fatmask[page];

		int all = (old_fg_page[page]!=sys16_fg_page[page]);

		const unsigned short *new_source = ((unsigned short *)sys16_tileram) + sys16_fg_page[page]*0x800;
		unsigned short *old_source = fg_page_buffer[page];

		int sx,sy;
		for( sy=0; sy<TILEMAP_HEIGHT; sy+=8 ){
			for( sx=0; sx<TILEMAP_WIDTH; sx+=8 ){
				unsigned short data = *new_source++;
				if( all || data!=*old_source ){
					int col = sx/8;
					int row = sy/8;

					int tile_number = (data&0xfff) +
						0x1000*((data&0x1000)?sys16_tile_bank1:sys16_tile_bank0);

					int pen_usage = gfx->pen_usage[tile_number];

					osd_mark_dirty(sx, sy, sx+TILEMAP_WIDTH-1, sy+TILEMAP_HEIGHT-1, 0);

					if( pen_usage==1 ){
						fg_transparency[page][0][row][col] = 1;
						fg_transparency[page][1][row][col] = 1;
					}
					else {
						if( data&0x8000 ){
							fg_transparency[page][0][row][col] = 1;
							fg_transparency[page][1][row][col] = 0;
						}
						else {
							fg_transparency[page][0][row][col] = 0;
							fg_transparency[page][1][row][col] = 1;
						}

/*
	Drawing a foreground tile into the tempbitmap and fatmask tempbitmap is slow!
	This could easily be sped up with a custom routine.
*/

						/*	foreground (transparent) and background (opaque) tiles share the
							same palette - we can't draw the tile with transparency_none
							because pen#0 isn't transparent. */

						rect.max_y = (rect.min_y = sy)+7;
						rect.max_x = (rect.min_x = sx)+7;
						fillbitmap( bitmap,palette_transparent_pen,&rect );

						drawgfx( bitmap, gfx,
							tile_number,
							(data>>6)&0x7f, /* color */
							0, 0, /* no need to flip */
							sx, sy,
							0, /* no need to clip */
							TRANSPARENCY_PEN, 0 );

						{ /* now, update the fatmask - one byte (0x00 or 0xff) for each pixel */
							int x,y;
							for( y=sy; y<sy+8; y++ ){
								unsigned char *source = bitmap->line[y]+sx;
								unsigned char *dest = fatmask->line[y]+sx;

								*dest++ = (*source++==palette_transparent_pen )?0xff:0x00;
								*dest++ = (*source++==palette_transparent_pen )?0xff:0x00;
								*dest++ = (*source++==palette_transparent_pen )?0xff:0x00;
								*dest++ = (*source++==palette_transparent_pen )?0xff:0x00;

								*dest++ = (*source++==palette_transparent_pen )?0xff:0x00;
								*dest++ = (*source++==palette_transparent_pen )?0xff:0x00;
								*dest++ = (*source++==palette_transparent_pen )?0xff:0x00;
								*dest++ = (*source++==palette_transparent_pen )?0xff:0x00;
							}
						}
					}
					*old_source = data;
				}
				old_source++;
			} /* x */
		} /* y */
		old_fg_page[page]=sys16_fg_page[page];
	} /* page */
}

void sys16_vh_stop( void ){
	int page;
	for( page=0; page<4; page++ ){
		if( fg_page_buffer[page] ) free( fg_page_buffer[page] );
		if( fg_bitmap[page] ) osd_free_bitmap( fg_bitmap[page] );
		if( fg_fatmask[page] ) osd_free_bitmap( fg_fatmask[page] );

		if( bg_page_buffer[page] ) free( bg_page_buffer[page] );
		if( bg_bitmap[page] ) osd_free_bitmap( bg_bitmap[page] );
	}
}

int sys16_vh_start( void ){
	int page;

	int fail = 0;
	for( page=0; page<4; page++ ){
		sys16_fg_page[page] = 0;
		sys16_bg_page[page] = 0;

		fg_page_buffer[page] = (unsigned short *)malloc(2*TILEMAP_ROWS*TILEMAP_COLS);
		fg_bitmap[page] = osd_create_bitmap( TILEMAP_WIDTH, TILEMAP_HEIGHT );
		fg_fatmask[page] = osd_create_bitmap( TILEMAP_WIDTH, TILEMAP_HEIGHT );

		bg_page_buffer[page] = (unsigned short *)malloc(2*TILEMAP_ROWS*TILEMAP_COLS);
		bg_bitmap[page] = osd_create_bitmap( TILEMAP_WIDTH, TILEMAP_HEIGHT );

		if( fg_page_buffer[page]==0 || fg_bitmap[page]==0 || fg_fatmask[page]==0 ||
			bg_page_buffer[page]==0 || bg_bitmap[page]==0 ) fail = 1;
	}
	if( fail ){
		sys16_vh_stop();
		return 1;
	}

	dirty_all();

	{
		int i;
		/* initialize all entries to black - needed for Golden Axe*/
		for( i=0; i<2048; i++ ){
			palette_change_color( i, 0,0,0 );
		}
	}

	sys16_tile_bank0 = 0;
	sys16_tile_bank1 = 1;

	sys16_fg_scrollx = 0;
	sys16_fg_scrolly = 0;

	sys16_bg_scrollx = 0;
	sys16_bg_scrolly = 0;

	sys16_refreshenable = 1;

	/* common defaults */
	sys16_update_proc = 0;
	sys16_spritesystem = 1;
	sys16_sprxoffset = -0xb8;

	return 0;
}

/***************************************************************************/

extern void drawgfxpicture( /* helper function for drawing sprites */
	struct osd_bitmap *bitmap,
	const unsigned char *source,
	int width, int screenheight,
	const unsigned short *pal,
	int xflip, int yflip,
	int sx, int sy,
	int zoom,
	const struct rectangle *clip
);

void sys16_draw_sprites( struct osd_bitmap *bitmap, int priority ){
	const struct rectangle *clip = &Machine->drv->visible_area;
	const unsigned short *base_pal = Machine->gfx[0]->colortable + 1024;
	const unsigned char *base_gfx = Machine->memory_region[2];

	unsigned short *source = (unsigned short *)sys16_spriteram;
	unsigned short *finish = source+MAX_SPRITES*8;

	if( sys16_spritesystem==1  ){ /* standard sprite hardware */
		do{
			unsigned short attributes = source[4];

			if( ((attributes>>6)&0x3) == priority ){
				const unsigned char *gfx = base_gfx + source[3]*4 + (sys16_obj_bank[(attributes>>8)&0xf] << 17);
				const unsigned short *pal = base_pal + ((attributes&0x3f)<<4);

				int sy = source[0];
				int end_line = sy>>8;
				sy &= 0xff;

				if( end_line == 0xff ) break;

				{
					int sx = source[1] + sys16_sprxoffset;
					int zoom = source[5]&0x3ff;

					int width = source[2];
					int horizontal_flip = width&0x100;
					int vertical_flip = width&0x80;
					width = (width&0x7f)*4;

					if( vertical_flip ){
						width = 512-width;
					}

					if( horizontal_flip ){
						gfx += 4;
						if( vertical_flip ) gfx -= width*2;
					}
					else{
						if( vertical_flip ) gfx -= width; else gfx += width;
					}

					drawgfxpicture(
						bitmap,
						gfx,
						width, end_line - sy,
						pal,
						horizontal_flip, vertical_flip,
						sx, sy,
						zoom,
						clip
					);
					osd_mark_dirty(sx, sy, sx+width-1, end_line-1, 0);
                }
			}

			source += 8;
		} while( source<finish );
	}
	else if( sys16_spritesystem==0 ){ /* passing shot */
		do{
			unsigned short attributes = source[5];
			if( ((attributes>>14)&0x3) == priority ){
				int sy = source[1];
				if( sy != 0xffff ){
					int bank = (attributes>>4)&0xf;
					const unsigned short *pal = base_pal + ((attributes>>4)&0x3f0);

					int number = source[2];
					int horizontal_flip = number & 0x8000;

					int zoom = source[4]&0x3ff;

					int sx = source[0] + sys16_sprxoffset;

					int width = source[3];
					int vertical_flip = width&0x80;

					int end_line = sy>>8;
					sy = sy&0xff;

					sy+=2;
					end_line+=2;


					if( vertical_flip ){
						width &= 0x7f;
						width = 0x80-width;
					}
					else{
						width &= 0x7f;
					}

					if( horizontal_flip ){
						bank = (bank-1) & 0xf;
						if( vertical_flip ){
							sx += 5;
						}
						else {
							number += 1-width;
						}
					}

					drawgfxpicture(
						bitmap,
						base_gfx + number*4 + (sys16_obj_bank[bank] << 17),
						width*4, end_line - sy,
						pal,
						horizontal_flip, vertical_flip,
						sx, sy,
						zoom,
						clip
					);
					osd_mark_dirty(sx, sy, sx+width*4-1, end_line-1, 0);
                }
			}

			source += 8;
		}while( source<finish );
	}
}

static void palette_sprites( unsigned short *base ){
	unsigned short *source = (unsigned short *)sys16_spriteram;
	unsigned short *finish = source+MAX_SPRITES*8;

	if( sys16_spritesystem==1 ){ /* standard sprite hardware */
		do{
			if( (source[0]>>8) == 0xff ) break;
			base[source[4]&0x3f] = 1;
			source+=8;
		}while( source<finish );
	}
	else if( sys16_spritesystem==0 ){ /* passing shot */
		do{
			if( source[1]!=0xffff ) base[(source[5]>>8)&0x3f] = 1;
			source+=8;
		}while( source<finish );
	}
}

/***************************************************************************/

static void draw_text(struct osd_bitmap *bitmap){
	const struct GfxElement *gfx = Machine->gfx[0];
	const unsigned short *source = (unsigned short *)sys16_textram;

	int sx,sy;
	for( sy = 0; sy < 28*8; sy+=8 ){
		source += TILEMAP_COLS-40;
		for( sx = 0; sx < 40*8; sx+=8 ){
			unsigned short data = *source++;
			int tile_number = data&0x1ff;
			if( tile_number ){ /* skip spaces */
				drawgfx(bitmap,gfx,
					tile_number,
					(data >> 9)%8, /* color */
					0,0, /* no flip*/
					sx,sy,
					0, /* no need to clip */
					TRANSPARENCY_PEN,0);
				osd_mark_dirty(sx, sy, sx+8-1, sy+8-1, 0);
            }
		}
	}
}

/***************************************************************************/

static void palette_background( unsigned char *base ){
	const struct GfxElement *gfx = Machine->gfx[0];
	const int mask = Machine->gfx[0]->total_elements - 1;

	int page;
	for (page=0; page < 4; page++){
		const unsigned short *source = ((unsigned short *)sys16_tileram) +
			sys16_bg_page[page]*0x800;
		const unsigned short *finish = source+TILEMAP_COLS*TILEMAP_ROWS;
		do {
			unsigned short data = *source++;
			int tile_number = (data&0xfff) +
				0x1000*((data&0x1000)?sys16_tile_bank1:sys16_tile_bank0);
			base[(data >> 6)%128] |= gfx->pen_usage[tile_number & mask];
		}while( source<finish );
	} /* next page */
}

static void palette_foreground( unsigned char *base ){
	const struct GfxElement *gfx = Machine->gfx[0];
	const int mask = Machine->gfx[0]->total_elements - 1;

	int page;
	for( page=0; page < 4; page++ ){
		const unsigned short *source = ((unsigned short *)sys16_tileram) +
			sys16_fg_page[page]*0x800;

		int row,col;

		unsigned long layer_opacity[TILEMAP_COLS];
		for( col=0; col<TILEMAP_COLS; col++ ){
			layer_opacity[col] = 0;
		}

		for( row=0; row<TILEMAP_ROWS; row++ ){
			for( col=0; col<TILEMAP_COLS; col++ ){
				unsigned short data = *source++;
				int tile_number = (data&0xfff) +
					0x1000*((data&0x1000)?sys16_tile_bank1:sys16_tile_bank0);
				int pen_usage = gfx->pen_usage[tile_number & mask];
				if( pen_usage&1 ) layer_opacity[col] |= (1<<row);
				base[(data >> 6)%128] |= 0xFE & pen_usage;
			}
		}

		/* compute screenwise opacity */
		{
			unsigned long temp[SCREENCOLS+2];

			int sx = ((page&1)*512+320+sys16_fg_scrollx)&0x3ff;
			int sy = ((page>>1)*256+256-sys16_fg_scrolly)&0x1ff;
			int unaligned_x = sx&0x7;
			int unaligned_y = sy&0x7;
			if( sx>320 ) sx-=1024;
			if( sy>224 ) sy-=512;

			if( sy > -TILEMAP_HEIGHT && sy<224 ){
				sx = sx/8+1;
				sy = sy/8+1;

				for( col=0; col<SCREENCOLS+2; col++ ){
					int c = col-sx;
					if( c>=0 && c<TILEMAP_COLS ){
						if( sy>0 ){
							temp[col] = layer_opacity[c]<<sy;
						}
						else {
							temp[col] = layer_opacity[c]>>(-sy);
						}
					}
					else {
						temp[col] = 0;
					}
				}

				if( unaligned_y ){
					for( col=0; col<SCREENCOLS+2; col++ ){
						temp[col] |= (temp[col]>>1)|(temp[col]<<1);
					}
				}

				if( unaligned_x ){
					for( col=1; col<SCREENCOLS+1; col++ ) temp[col] |= temp[col+1];
					for( col=SCREENCOLS; col>=1; col-- ) temp[col] |= temp[col-1];
				}

				for( col=0; col<SCREENCOLS; col++ ){
					screenwise_opacity[col] |= temp[col+1]>>1;
				}
			}
		}
	} /* next page */
}

static void palette_text( unsigned char *base ){
	const struct GfxElement *gfx = Machine->gfx[0];
	const int mask = Machine->gfx[0]->total_elements - 1;
	const unsigned short *source = (unsigned short *)sys16_textram;

	int sx,sy;
	for( sy = 0; sy < 28*8; sy+=8 ){
		source += TILEMAP_COLS-40;
		for( sx = 0; sx < 40*8; sx+=8 ){
			unsigned short data = *source++;
			int tile_number = data&0x1ff;
			if( tile_number ){ /* skip spaces */
				base[(data >> 9)%8] |= 0xFE & gfx->pen_usage[tile_number & mask] & 0xfe;
			}
		}
	}
}

static void refresh_palette( void ){
	unsigned char *pal = &palette_used_colors[0];

	/* compute palette usage */
	unsigned char palette_map[128];
	unsigned short sprite_map[MAX_SPRITES];
	int i,j;

	for( i=0; i<SCREENCOLS; i++ ) screenwise_opacity[i] = 0;
	memset (palette_map, 0, sizeof (palette_map));
	memset (sprite_map, 0, sizeof (sprite_map));

	palette_background( palette_map );
	palette_foreground( palette_map );
	palette_text( palette_map );
	palette_sprites( sprite_map );

	/* expand the results */
	for( i = 0; i < 128; i++ ){
		int usage = palette_map[i];
		if (usage){
			for (j = 0; j < 8; j++)
				if (usage & (1 << j))
					pal[j] = PALETTE_COLOR_USED;
				else
					pal[j] = PALETTE_COLOR_UNUSED;
		}
		else {
			memset (pal, PALETTE_COLOR_UNUSED, 8);
		}
		pal += 8;
	}
	for (i = 0; i < MAX_SPRITES; i++){
		if ( sprite_map[i] ){
			pal[0] = PALETTE_COLOR_UNUSED;
			for (j = 1; j < 15; j++) pal[j] = PALETTE_COLOR_USED;
			pal[15] = PALETTE_COLOR_UNUSED;
		}
		else {
			memset( pal, PALETTE_COLOR_UNUSED, 16 );
		}
		pal += 16;
	}

	if( palette_recalc () ) dirty_all();
}

/***************************************************************************/

void sys16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh){
	if( sys16_update_proc ) sys16_update_proc();

	if( sys16_refreshenable ){
		int opacity_check = osd_key_pressed( OSD_KEY_O );

		refresh_palette();

		update_background();
		update_foreground();

		if( opacity_check ) fillbitmap( bitmap, 0, 0 );

		draw_background(bitmap);

		sys16_draw_sprites(bitmap,1);

		if( !opacity_check ) draw_foreground(bitmap,0);

		sys16_draw_sprites(bitmap,2);

		if( !opacity_check ) draw_foreground(bitmap,1);

		draw_text(bitmap);

		sys16_draw_sprites(bitmap,3);
	}
}
