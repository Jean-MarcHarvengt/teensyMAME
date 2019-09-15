#include "vidhrdw/generic.h"

int tigeroad_base_bank;
unsigned char *tigeroad_scrollram;



static void render_background( struct osd_bitmap *bitmap, int priority )
{
	int scrollx = 	READ_WORD(&tigeroad_scrollram[0]) & 0xfff; /* 0..4096 */
	int scrolly =	READ_WORD(&tigeroad_scrollram[2]) & 0xfff; /* 0..4096 */

	unsigned char *p = Machine->memory_region[2];

	int alignx = scrollx%32;
	int aligny = scrolly%32;

	int row = scrolly/32;	/* 0..127 */
	int sy = 224+aligny;

	int transp0,transp1;

	if( priority ){ /* foreground */
		transp0 = 0xFFFF;	/* draw nothing (all pens transparent) */
		transp1 = 0x01FF;	/* high priority half of tile */
	}
	else { /* background */
		transp0 = 0;		/* NO_TRANSPARENCY */
		transp1 = 0xFE00;	/* low priority half of tile */
	}

	while( sy>-32 ){
		int col = scrollx/32;	/* 0..127 */
		int sx = -alignx;

		while( sx<256 ){
			int offset = 2*(col%8) + 16*(row%8) + 128*(col/8) + 2048*(row/8);

			int tile_number = p[offset];
			int attributes = p[offset+1];

			int flipx = attributes&0x20;
			int flipy = 0;
			int color = attributes&0xF;
			int bank = tigeroad_base_bank+(attributes>>6);

			drawgfx(bitmap,Machine->gfx[0],
				tile_number + bank*256,
				color,
				flipx,flipy,
				sx,sy,
				&Machine->drv->visible_area,
				TRANSPARENCY_PENS,(attributes & 0x10)?transp1:transp0);

			sx+=32;
			col++;
			if( col>=128 ) col-=128;
		}
		sy-=32;
		row++;
		if( row>=128 ) row-=128;
	}
}

static void render_sprites( struct osd_bitmap *bitmap )
{
	unsigned char *source = &spriteram[spriteram_size] - 8;
	unsigned char *finish = spriteram;

	while( source>=finish ){
		int tile_number = READ_WORD( source );
		if( tile_number!=0xFFF ){
			int attributes = READ_WORD( source+2 );
			int sy = READ_WORD( source+4 ) & 0x1ff;
			int sx = READ_WORD( source+6 ) & 0x1ff;

			int flipx = attributes&2;
			int flipy = attributes&1;
			int color = (attributes>>2)&0xf;

			if( sx>0x100 ) sx -= 0x200;
			if( sy>0x100 ) sy -= 0x200;

			drawgfx(bitmap,Machine->gfx[1],
				tile_number,
				color,
				flipx,flipy,
				sx,240-sy,
				&Machine->drv->visible_area,
				TRANSPARENCY_PEN,15);
		}
		source-=8;
	}
}

static void render_text( struct osd_bitmap *bitmap )
{
	unsigned char *source = videoram;
	int sx,sy;
	for( sy=0; sy<256; sy+=8 ) for( sx=0; sx<256; sx+=8 ){
		int data = READ_WORD( source );
		int attributes = data>>8;
		int flipy = attributes&0x10;
		int color = attributes&0x0F;

		int tile_number = data&0xFF;

		if( attributes&0x80 ) tile_number += 512;
		if( attributes&0x40 ) tile_number += 256;
		if( attributes&0x20 ) tile_number += 1024;

		drawgfx(bitmap,Machine->gfx[2],
			tile_number,
			color,
			0,flipy,
			sx,sy,
			&Machine->drv->visible_area,
			TRANSPARENCY_PEN,3);

		source+=2;
	}
}



void tigeroad_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	palette_recalc();

	render_background( bitmap,0 );
	render_sprites( bitmap );
	render_background( bitmap,1 );
	render_text( bitmap );
}
