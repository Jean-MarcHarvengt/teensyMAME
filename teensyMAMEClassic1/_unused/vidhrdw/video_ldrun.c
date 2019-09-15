/***************************************************************************

  Video Hardware for Irem Games:
  Lode Runner, Kid Niki, Spelunker

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



static unsigned char scroll[2];
static int flipscreen;
static const unsigned char *sprite_height_prom;

static struct rectangle spritevisiblearea =
{
	8*8, (64-8)*8-1,
	1*8, 32*8-1
};
static struct rectangle flipspritevisiblearea =
{
	8*8, (64-8)*8-1,
	0*8, 31*8-1
};

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Kung Fu Master has a six 256x4 palette PROMs (one per gun; three for
  characters, three for sprites).
  I don't know the exact values of the resistors between the RAM and the
  RGB output. I assumed these values (the same as Commando)

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void ldrun_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;

		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}

	color_prom += 2*Machine->drv->total_colors;
	/* color_prom now points to the beginning of the sprite height table */

	sprite_height_prom = color_prom;	/* we'll need this at run time */
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int ldrun_vh_start(void)
{
	if ((dirtybuffer = malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer,1,videoram_size);

	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void ldrun_vh_stop(void)
{
	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap);
}

void ldrun2p_scroll_w(int offset,int data)
{
	scroll[offset] = data;
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void ldrun_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size-2;offs >= 0;offs -= 2)
	{
		if ( (dirtybuffer[offs]) || (dirtybuffer[offs+1]) )
		{
			int sx,sy,flipx;


			dirtybuffer[offs] = 0;
			dirtybuffer[offs+1] = 0;

			sx = ( offs % 128 ) / 2;
			sy = offs / 128;
			flipx = videoram[offs+1] & 0x20;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((videoram[offs+1] & 0xc0) << 2),
					videoram[offs+1] & 0x1f,
					flipx,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 8)
	{
		int i,incr,code,col,flipx,flipy,sx,sy;


		code = spriteram[offs+4];

		if (code != 0)
		{
			col = spriteram[offs+0] & 0x1f;
			sx = 256 * (spriteram[offs+7] & 1) + spriteram[offs+6],
			sy = 256+128-15 - (256 * (spriteram[offs+3] & 1) + spriteram[offs+2]),
			flipx = spriteram[offs+5] & 0x40;
			flipy = spriteram[offs+5] & 0x80;

			i = sprite_height_prom[code / 32];
			if (i == 1)	/* double height */
			{
				code &= ~1;
				sy -= 16;
			}
			else if (i == 2)	/* quadruple height */
			{
				i = 3;
				code &= ~3;
				sy -= 3*16;
			}

			if (flipscreen)
			{
				sx = 240 - sx;
				sy = 242 - i*16 - sy;	/* sprites are slightly misplaced by the hardware */
				flipx = !flipx;
				flipy = !flipy;
			}

			if (flipy)
			{
				incr = -1;
				code += i;
			}
			else incr = 1;

			do
			{
				drawgfx(bitmap,Machine->gfx[1],
						code + i * incr,col,
						flipx,flipy,
						sx,sy + 16 * i,
						flipscreen ? &flipspritevisiblearea : &spritevisiblearea,TRANSPARENCY_PEN,0);

				i--;
			} while (i >= 0);
		}
	}
}

/* almost identical but scrolling background, more characters, */
/* no char x flip, and more sprites */
void ldrun2p_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size-2;offs >= 0;offs -= 2)
	{
		if ( (dirtybuffer[offs]) || (dirtybuffer[offs+1]) )
		{
			int sx,sy;


			dirtybuffer[offs] = 0;
			dirtybuffer[offs+1] = 0;

			sx = ( offs % 128 ) / 2;
			sy = offs / 128;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((videoram[offs+1] & 0xc0) << 2) + ((videoram[offs+1] & 0x20) << 5),
					videoram[offs+1] & 0x1f,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	{
		int scrollx;

		scrollx = -(scroll[1] + 256 * scroll[0] - 2);

		copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 8)
	{
		int i,incr,code,col,flipx,flipy,sx,sy;


		code = spriteram[offs+4] + 256 * (spriteram[offs+5] & 0x03);

		if (code != 0)
		{
			col = spriteram[offs+0] & 0x1f;
			sx = 256 * (spriteram[offs+7] & 1) + spriteram[offs+6],
			sy = 256+128-15 - (256 * (spriteram[offs+3] & 1) + spriteram[offs+2]),
			flipx = spriteram[offs+5] & 0x40;
			flipy = spriteram[offs+5] & 0x80;

			i = sprite_height_prom[code / 32];
			if (i == 1)	/* double height */
			{
				code &= ~1;
				sy -= 16;
			}
			else if (i == 2)	/* quadruple height */
			{
				i = 3;
				code &= ~3;
				sy -= 3*16;
			}

			if (flipscreen)
			{
				sx = 240 - sx;
				sy = 242 - i*16 - sy;	/* sprites are slightly misplaced by the hardware */
				flipx = !flipx;
				flipy = !flipy;
			}

			if (flipy)
			{
				incr = -1;
				code += i;
			}
			else incr = 1;

			do
			{
				drawgfx(bitmap,Machine->gfx[1],
						code + i * incr,col,
						flipx,flipy,
						sx,sy + 16 * i,
						flipscreen ? &flipspritevisiblearea : &spritevisiblearea,TRANSPARENCY_PEN,0);

				i--;
			} while (i >= 0);
		}
	}
}

/*************************************************************/

static int irem_background_bank;
static int irem_background_hscroll;
static int irem_background_vscroll;
static int irem_text_vscroll;

unsigned char *irem_textram;
static int dirty_buffer_size;

#define MEM_SPR_SIZE	6

/* graphics banks */
#define GFX_TILES		0
#define GFX_CHARS		1
#define GFX_SPRITES		2

static int irem_vh_start( int width, int height ){
	irem_background_bank = 0;
	irem_background_hscroll = 0;
	irem_background_vscroll = 128;
	irem_text_vscroll = 0x180;

	dirty_buffer_size = (width/8)*(height/8);
	dirtybuffer = malloc(dirty_buffer_size);
	if( dirtybuffer ){
		tmpbitmap = osd_create_bitmap( width, height );
		if( tmpbitmap ){
			memset(dirtybuffer,1,dirty_buffer_size);
			return 0;
		}
		free( dirtybuffer );
	}
	return 1; /* error */
}

void irem_vh_stop(void){
	osd_free_bitmap(tmpbitmap);
	free( dirtybuffer );
}

static void irem_draw_sprites( struct osd_bitmap *bitmap ){
	const struct rectangle *clip = &Machine->drv->visible_area;
	const struct GfxElement *gfx = Machine->gfx[GFX_SPRITES];
	const unsigned char *source = spriteram;
	const unsigned char *finish = source+0x100;
	const unsigned char *sprite_prom = Machine->memory_region[MEM_SPR_SIZE];

	while( source<finish ){
		int attributes = source[5];
		int tile_number = source[4]+(attributes&0x7)*256;
		int color = source[0];
		int sy = 256-(source[2]+256*source[3]);
		int sx = (source[7]&1)*256+source[6]-64;
		int flipx = attributes&0x40;
		int flipy = attributes&0x80;

		int size = sprite_prom[(tile_number>>5)&0x1f];

		switch( size ){
			case 0:
			size = 1;
			sy += 112;
			break;

			case 1:
			size = 2;
			sy += 96;
			tile_number &= 0xffe;
			break;

			case 2:
			size = 4;
			sy += 64;
			tile_number &= 0xffc;
			break;
		}

		while( size-- ){
			drawgfx(bitmap,gfx,
				tile_number, color,
				flipx,flipy,sx,sy,
				clip,TRANSPARENCY_PEN,0);

			tile_number++;
			sy += flipy?-16:16;
		}

		source+=8;
	}
}


void irem_background_hscroll_w( int offset, int data ){
	switch( offset ){
		case 0:
		irem_background_hscroll = (irem_background_hscroll&0xff00)|data;
		break;

		case 1:
		irem_background_hscroll = (irem_background_hscroll&0xff)|(data<<8);
		break;
	}
}

void irem_background_vscroll_w( int offset, int data ){
	switch( offset ){
		case 0:
		irem_background_vscroll = (irem_background_vscroll&0xff00)|data;
		break;

		case 1:
		irem_background_vscroll = (irem_background_vscroll&0xff)|(data<<8);
		break;
	}
}

void irem_text_vscroll_w( int offset, int data ){
	switch( offset ){
		case 0:
		irem_text_vscroll = (irem_text_vscroll&0xff00)|data;
		break;

		case 1:
		irem_text_vscroll = (irem_text_vscroll&0xff)|(data<<8);
		break;
	}
}

void irem_background_bank_w( int offset, int data ){
	if( data != irem_background_bank ){
		irem_background_bank = data;
		memset(dirtybuffer,1,dirty_buffer_size);
	}
}


/*************************************************************/

int kidniki_vh_start(void){
	return irem_vh_start(512,256);
}

static void kidniki_draw_background( struct osd_bitmap *bitmap ){
	const struct GfxElement *gfx = Machine->gfx[GFX_TILES];
	int i;
	for( i=0; i<dirty_buffer_size; i++ ){
		if( dirtybuffer[i] ){
			int sx = (i%64)*8;
			int sy = (i/64)*8;
			int attributes = videoram[i*2+1];
			int color = attributes&0x1f;
			int tile_number = videoram[i*2] + 256*(attributes>>5);
			if( irem_background_bank ) tile_number += 0x800;
			drawgfx( tmpbitmap, gfx,
				tile_number, color,
				0, 0, /* no flip */
				sx,sy,
				0, /* no need to clip */
				TRANSPARENCY_NONE, 0
			);
			dirtybuffer[i] = 0;
		}
	}

	{
		int scrollx = -irem_background_hscroll - 64;
		int scrolly = -irem_background_vscroll - 128;

		copyscrollbitmap(bitmap,tmpbitmap,
			1,&scrollx,
			1,&scrolly,
			&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}
}

static void kidniki_draw_text( struct osd_bitmap *bitmap ){
	int scrolly = irem_text_vscroll-0x180;
	const struct rectangle *clip = &Machine->drv->visible_area;
	const struct GfxElement *gfx = Machine->gfx[GFX_CHARS];
	const unsigned char *source = irem_textram;

	int x,y;
	for( y=-scrolly; y<64*8-scrolly; y+=8 ){
		for( x=0; x<32*12; x+=12 ){
			int attributes = source[1];
			int tile_number = source[0] + 256*(attributes>>6);
			int color = (attributes&0x1f);
			drawgfx( bitmap, gfx,
				tile_number,color,
				0, 0,
				x,y,
				clip,
				TRANSPARENCY_PEN, 0
			);

			source+=2;
		}
	}
}

void kidniki_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh){
	kidniki_draw_background( bitmap );
	irem_draw_sprites( bitmap );
	kidniki_draw_text( bitmap );
}

/*************************************************************/

int spelunk2_vh_start(void){
	return irem_vh_start(512,512);
}

static void spelunk2_draw_text( struct osd_bitmap *bitmap ){
	const struct GfxElement *gfx1 = Machine->gfx[GFX_CHARS];
	const struct GfxElement *gfx0 = Machine->gfx[GFX_CHARS+2];
	const unsigned char *source = irem_textram;

	int x,y;
	for( y=0; y<32*8; y+=8 ){
		for( x=0; x<32*12; x+=12 ){
			int attributes = source[1];
			drawgfx( bitmap, (attributes&0x10)?gfx0:gfx1,
				source[0], /* tile number */
				(attributes&0xf), /* color */
				0, 0,
				x,y,
				0, /* no need to clip */
				TRANSPARENCY_PEN, 0
			);
			source+=2;
		}
	}
}

static void spelunk2_draw_background( struct osd_bitmap *bitmap ){
	const struct GfxElement *gfx = Machine->gfx[GFX_TILES];
	int i;

	for( i=0; i<dirty_buffer_size; i++ ){
		if( dirtybuffer[i] ){
			int sx = (i%64)*8;
			int sy = (i/64)*8;
			int attributes = videoram[i*2+1];
			int color = attributes&0xf;
			int tile_number = videoram[i*2]+256*(attributes>>4);

			drawgfx( tmpbitmap, gfx,
				tile_number, color,
				0, 0, /* no flip */
				sx,sy,
				0, /* no need to clip */
				TRANSPARENCY_NONE, 0
			);

			dirtybuffer[i] = 0;
		}
	}

	{
		int scrollx = -irem_background_hscroll - 64;
		int scrolly = -irem_background_vscroll - 128;

		copyscrollbitmap(bitmap,tmpbitmap,
			1,&scrollx,
			1,&scrolly,
			&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}
}

void spelunk2_vh_screenrefresh( struct osd_bitmap *bitmap, int full_refresh ){
	spelunk2_draw_background( bitmap );
	irem_draw_sprites( bitmap );
	spelunk2_draw_text( bitmap );
}
