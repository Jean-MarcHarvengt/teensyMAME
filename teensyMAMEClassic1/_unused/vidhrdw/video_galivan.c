/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

d800-dbff	foreground:			char low bits (1 screen * 1024 chars/screen)
dc00-dfff	foreground attribute:	7		?
						 6543		color
						     21	?
						       0	char hi bit


e000-e0ff	spriteram:  64 sprites (4 bytes/sprite)
		offset :	0		1		2		3
		meaning:	ypos(lo)	sprite(lo)	attribute	xpos(lo)
								7   flipy
								6   flipx
								5-2 color
								1   sprite(hi)
								0   xpos(hi)


background:	0x4000 bytes of ROM:	76543210	tile code low bits
		0x4000 bytes of ROM:	7		?
						 6543		color
						     2	?
						      10	tile code high bits

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static unsigned char galivan_scrollx[2], galivan_scrolly[2];
static int flipscreen;

static struct osd_bitmap *background;


/***************************************************************************

  Rom banking routines.

***************************************************************************/


/* Sets the ROM bank READ at c000-dfff. bank must be 0 or 1 */
void galivan_setrombank(int bank)
{
unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	cpu_setbank(1,&RAM[0x10000 + 0x2000 * bank]);
}




void galivan_init_machine(void)
{
	galivan_setrombank(0);
}


/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/
static unsigned char wrong_color_prom[] =
{
  0, 0, 0, 255, 0, 0, 0, 255, 0, 255, 255, 0,
  0, 0, 255, 255, 0, 255, 0, 255, 255, 255, 255, 255,
  64, 64, 64, 128, 0, 0, 0, 128, 0, 128, 128, 0,
  0, 0, 128, 128, 0, 128, 0, 128, 128, 128, 128, 128
};
void galivan_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < 16;i++)
	{
		*(palette++) =  wrong_color_prom[i*3+0];	/* R */
		*(palette++) =  wrong_color_prom[i*3+1];	/* G */
		*(palette++) =  wrong_color_prom[i*3+2];	/* B */
	}

	for (i = 0;i < (Machine->drv->total_colors-16) ;i++)
	{
		*(palette++) =  color_prom[i+  0]*0x11;	/* R */
		*(palette++) =  color_prom[i+256]*0x11;	/* G */
		*(palette++) =  color_prom[i+512]*0x11;	/* B */
	}

}





/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

void galivan_draw_background(int flip)
{
	unsigned char *RAM = Machine->memory_region[2];
	int x,y;

	/* draw the whole background */
	for(y = 0; y < 128 ; y++)
	{
		for(x = 0; x < 128 ; x++)
		{
			int sx,sy;
			int addr = y*128+x;
			int attr = RAM[addr+0x4000];
			int code = RAM[addr]+((attr & 0x03) << 8);

			sx =16*x;
			sy =16*y;
			if (flip)
			{
				sx = 2048-16-sx;
				sy = 2048-16-sy;
			}


			drawgfx(background, Machine->gfx[1],
				code,
				(attr>>3)&0x0F,
				flip, flip,
				sx, sy,
				0, TRANSPARENCY_NONE, 0);
		}
	}
}



int galivan_vh_start(void)
{

	if  ( (background = osd_new_bitmap(2048, 2048, 4)) )
	{
		galivan_draw_background(flipscreen);
		return generic_vh_start();
	}
	else
		return 1;
}





/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/

void galivan_vh_stop(void)
{
  osd_free_bitmap(background);
  generic_vh_stop();
}



/***************************************************************************

  Video related registers.

***************************************************************************/


/* Written through port 40 */
void galivan_gfxbank_w(int offset,int data)
{
unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* bits 0 and 1 coin counters */
	coin_counter_w(0,data & 1);
	coin_counter_w(1,data & 2);

	/* bit 2 flip screen */
	if (flipscreen != (data & 0x04))
	{
		flipscreen = data & 0x04;
		galivan_draw_background(flipscreen);
	}

	/* bit 7 selects one of two ROM banks for c000-dfff */
	galivan_setrombank((data>>7)&1);

//	if (errorlog) fprintf(errorlog,"Address: %04X - port 40 = %02x\n",cpu_getpc(),data);
}


/* Written through port 41-42 */
void galivan_scrollx_w(int offset,int data)
{
	galivan_scrollx[offset] = data;

/* bits 765 probably enable/control priority of the gfx layers */

}

/* Written through port 43-44 */
void galivan_scrolly_w(int offset,int data)
{
	galivan_scrolly[offset] = data;
}




/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void galivan_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];



	/* copy the static background graphics */
	if ( (galivan_scrollx[1]&0x40)==0 ) /* this is wrong, but works for now */
	{
		int scrollx,scrolly;

		scrollx = -(galivan_scrollx[0] + 256 * (galivan_scrollx[1]&0x7));
		scrolly = -(galivan_scrolly[0] + 256 * (galivan_scrolly[1]&0x7));
		if (flipscreen)
		{
			scrollx = -(scrollx-256);
			scrolly = -(scrolly-256);
		}



		copyscrollbitmap(bitmap, background,
					1, &scrollx, 1, &scrolly,
					&Machine->drv->visible_area,
					TRANSPARENCY_NONE,0);
	}
	else
		osd_clearbitmap(Machine->scrbitmap);



	/* draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int attr = spriteram[offs+2];
		int flipx = attr & 0x40;
		int flipy = attr & 0x80;
		int sx,sy;

		sy = 240 - spriteram[offs];
		sx = (spriteram[offs+3] - 0x80) + 256 * (attr & 1);
		if (flipscreen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[2],
				spriteram[offs+1]+((attr&2)<<7),
				(attr >> 2) & 0xF,
				flipx,flipy,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,15);
	}



	/* draw the characters */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;
		int attr = colorram[offs];

/* swapped, strange isn't it? */
		sy = offs % 32;
		sx = offs / 32;

		if (flipscreen)
		{
			sx = 31 - sx;
			sy = 31 - sy;
		}

		drawgfx(bitmap,Machine->gfx[0],
				videoram[offs]+256*(attr&1),
				(attr>>3)&0x0F,
				flipscreen,flipscreen,
				8*sx,8*sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,15);
	}


}
