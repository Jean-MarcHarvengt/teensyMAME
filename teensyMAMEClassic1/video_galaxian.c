/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  This is the driver for the "Galaxian" style board, used, with small
  variations, by an incredible amount of games in the early 80s.

  This video driver is used by the following drivers:
  - galaxian.c
  - mooncrst.c
  - moonqsr.c
  - scramble.c
  - scobra.c
  - ckongs.c

  TODO: cocktail support hasn't been implemented properly yet

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct rectangle spritevisiblearea =
{
	2*8+1, 32*8-1,
	2*8, 30*8-1
};
static struct rectangle spritevisibleareaflipx =
{
	0*8, 30*8-2,
	2*8, 30*8-1
};

#define MAX_STARS 250
#define STARS_COLOR_BASE 32

unsigned char *galaxian_attributesram;
unsigned char *galaxian_bulletsram;

int galaxian_bulletsram_size;
static int stars_on,stars_blink;
static int stars_type;	/* 0 = Galaxian stars */
						/* 1 = Scramble stars */
						/* 2 = Rescue stars (same as Scramble, but only half screen) */
static unsigned int stars_scroll;

struct star
{
	int x,y,code,col;
};
static struct star stars[MAX_STARS];
static int total_stars;
static int gfx_bank;	/* used by Pisces and "japirem" only */
static int gfx_extend;	/* used by Moon Cresta only */
static int bank_mask;	/* different games have different gfx bank switching */
static int char_bank;	/* used by Moon Quasar only */
static int sprite_bank;	/* used by Calipso only */
static int flipscreen[2];

static int BackGround;					/* MJC 051297 */
static unsigned char backcolour[256];  	/* MJC 220198 */

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Moon Cresta has one 32 bytes palette PROM, connected to the RGB output
  this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  The output of the background star generator is connected this way:

  bit 5 -- 100 ohm resistor  -- BLUE
        -- 150 ohm resistor  -- BLUE
        -- 100 ohm resistor  -- GREEN
        -- 150 ohm resistor  -- GREEN
        -- 100 ohm resistor  -- RED
  bit 0 -- 150 ohm resistor  -- RED

  The blue background in Scramble and other games goes through a 390 ohm
  resistor.

***************************************************************************/
void galaxian_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	unsigned char *opalette;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	opalette = palette;

	/* first, the char acter/sprite palette */
	for (i = 0;i < 32;i++)
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

	/* now the stars */
	for (i = 0;i < 64;i++)
	{
		int bits;
		int map[4] = { 0x00, 0x88, 0xcc, 0xff };


		bits = (i >> 0) & 0x03;
		*(palette++) = map[bits];
		bits = (i >> 2) & 0x03;
		*(palette++) = map[bits];
		bits = (i >> 4) & 0x03;
		*(palette++) = map[bits];
	}

	/* characters and sprites use the same palette */
	for (i = 0;i < TOTAL_COLORS(0);i++)
	{
		if (i & 3) COLOR(0,i) = i;
		else COLOR(0,i) = 0;	/* 00 is always black, regardless of the contents of the PROM */
	}

    /* colours for alternate background */

    if (Machine->drv->total_colors == 224)
    {
    	/* Graduated Blue */

    	for (i=0;i<64;i++)
        {
        	opalette[64*3 + i*3 + 0] = 0;
        	opalette[64*3 + i*3 + 1] = i * 2;
        	opalette[64*3 + i*3 + 2] = i * 4;
        }

        /* Graduated Brown */

    	for (i=0;i<64;i++)
        {
        	opalette[128*3 + i*3 + 0] = i * 3;
        	opalette[128*3 + i*3 + 1] = i * 1.5;
        	opalette[128*3 + i*3 + 2] = i;
        }
    }
    else
    {
		/* use an otherwise unused pen for the standard blue background */

		opalette[3*4] = 0;
		opalette[3*4 + 1] = 0;
		opalette[3*4 + 2] = 0x55;
    }

	/* bullets can be either white or yellow */

	COLOR(2,0) = 0;
	COLOR(2,1) = 0x0f + STARS_COLOR_BASE;	/* yellow */
	COLOR(2,2) = 0;
	COLOR(2,3) = 0x3f + STARS_COLOR_BASE;	/* white */
}

void scramble_background_w(int offset, int data)	/* MJC 051297 */
{
	if (BackGround != data)
    {
		BackGround = data;
		memset(dirtybuffer,1,videoram_size);
    }

    if(errorlog) fprintf(errorlog,"background changed %d\n",data);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

static int common_vh_start(void)
{
	int generator;
	int x,y;

	gfx_bank = 0;
	gfx_extend = 0;
	stars_on = 0;
	flipscreen[0] = 0;
	flipscreen[1] = 0;

	if (generic_vh_start() != 0)
		return 1;


    /* Default alternate background - Solid Blue - MJC 220198 */

    for(x=0;x<256;x++) backcolour[x] = Machine->pens[4];
	BackGround=0;


	/* precalculate the star background */

	total_stars = 0;
	generator = 0;

	for (y = 255;y >= 0;y--)
	{
		for (x = 511;x >= 0;x--)
		{
			int bit1,bit2;


			generator <<= 1;
			bit1 = (~generator >> 17) & 1;
			bit2 = (generator >> 5) & 1;

			if (bit1 ^ bit2) generator |= 1;

			if (y >= Machine->drv->visible_area.min_y &&
					y <= Machine->drv->visible_area.max_y &&
					((~generator >> 16) & 1) &&
					(generator & 0xff) == 0xff)
			{
				int color;

				color = (~(generator >> 8)) & 0x3f;
				if (color && total_stars < MAX_STARS)
				{
					stars[total_stars].x = x;
					stars[total_stars].y = y;
					stars[total_stars].code = color;
					stars[total_stars].col = Machine->pens[color + STARS_COLOR_BASE];

					total_stars++;
				}
			}
		}
	}

	return 0;
}

int galaxian_vh_start(void)
{
	bank_mask = 0;
	stars_type = 0;
	char_bank = 0;
	sprite_bank = 0;
	return common_vh_start();
}

int moonqsr_vh_start(void)
{
	bank_mask = 0x20;
	stars_type = 0;
	char_bank = 1;
	sprite_bank = 0;
	return common_vh_start();
}

int scramble_vh_start(void)
{
	bank_mask = 0;
	stars_type = 1;
	char_bank = 0;
	sprite_bank = 0;
	return common_vh_start();
}

int rescue_vh_start(void)
{
	int ans,x;

	bank_mask = 0;
	stars_type = 2;
	char_bank = 0;
	sprite_bank = 0;
	ans = common_vh_start();

    /* Setup background colour array (blue sky, blue sea, black bottom line) */

    for (x=0;x<64;x++)
	{
		backcolour[x*2]   = Machine->pens[64+x];
		backcolour[x*2+1] = Machine->pens[64+x];
    }

    for (x=0;x<60;x++)
	{
		backcolour[128+x*2]   = Machine->pens[68+x];
		backcolour[128+x*2+1] = Machine->pens[68+x];
    }

    for (x=248;x<256;x++) backcolour[x] = Machine->pens[0];

    return ans;
}

int minefield_vh_start(void)
{
	int ans,x;

	bank_mask = 0;
	stars_type = 2;
	char_bank = 0;
	sprite_bank = 0;
	ans = common_vh_start();

    /* Setup background colour array (blue sky, brown earth, black bottom line) */

    for (x=0;x<64;x++)
	{
		backcolour[x*2]   = Machine->pens[64+x];
		backcolour[x*2+1] = Machine->pens[64+x];
    }

    for (x=0;x<60;x++)
	{
		backcolour[128+x*2]   = Machine->pens[128+x];
		backcolour[128+x*2+1] = Machine->pens[128+x];
    }

    for (x=248;x<256;x++) backcolour[x] = Machine->pens[0];

    return ans;
}

int ckongs_vh_start(void)
{
	bank_mask = 0x10;
	stars_type = 1;
	char_bank = 0;
	sprite_bank = 0;
	return common_vh_start();
}

int calipso_vh_start(void)
{
	bank_mask = 0;
	stars_type = 1;
	char_bank = 0;
	sprite_bank = 1;
	return common_vh_start();
}



void galaxian_flipx_w(int offset,int data)
{
	if (flipscreen[0] != (data & 1))
	{
		flipscreen[0] = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}

void galaxian_flipy_w(int offset,int data)
{
	if (flipscreen[1] != (data & 1))
	{
		flipscreen[1] = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}



void galaxian_attributes_w(int offset,int data)
{
	if ((offset & 1) && galaxian_attributesram[offset] != data)
	{
		int i;


		for (i = offset / 2;i < videoram_size;i += 32)
			dirtybuffer[i] = 1;
	}

	galaxian_attributesram[offset] = data;
}



void galaxian_stars_w(int offset,int data)
{
	stars_on = (data & 1);
	stars_scroll = 0;
}



void pisces_gfxbank_w(int offset,int data)
{
	gfx_bank = data & 1;
}

void mooncrgx_gfxextend_w(int offset,int data)
{
  /* for the Moon Cresta bootleg on Galaxian H/W the gfx_extend is
     located at 0x6000-0x6002.  Also, 0x6000 and 0x6001 are reversed. */
     if(offset == 1)
       offset = 0;
     else if(offset == 0)
       offset = 1;    /* switch 0x6000 and 0x6001 */
	if (data) gfx_extend |= (1 << offset);
	else gfx_extend &= ~(1 << offset);
}


void mooncrst_gfxextend_w(int offset,int data)
{
	if (data) gfx_extend |= (1 << offset);
	else gfx_extend &= ~(1 << offset);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void galaxian_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,charcode;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

  			if (flipscreen[0]) sx = 31 - sx;
  			if (flipscreen[1]) sy = 31 - sy;

			charcode = videoram[offs];

			/* bit 5 of [2*(offs%32)+1] is used only by Moon Quasar */
			if (char_bank)
			{
				if (galaxian_attributesram[2 * (offs % 32) + 1] & 0x20)
					charcode += 256;
			}

			/* gfx_bank is used by Pisces and japirem/Uniwars only */
			if (gfx_bank)
				charcode += 256;

			/* gfx_extend is used by Moon Cresta only */
			if ((gfx_extend & 4) && (charcode & 0xc0) == 0x80)
				charcode = (charcode & 0x3f) | (gfx_extend << 6);

            if (BackGround)
            {
            	/* Fill with Background colours */

            	int startx,starty,backline,j;

                if (Machine->orientation & ORIENTATION_SWAP_XY)
                {
			        starty = backline = sx * 8;
			        startx = sy * 8;

				    if (Machine->orientation & ORIENTATION_FLIP_X)
                	    startx = (255 - startx)-7;

				    if (Machine->orientation & ORIENTATION_FLIP_Y)
                    {
                    	if (errorlog) fprintf(errorlog,"flip_y\n");
					    starty = (255 - starty);

				        for (i=0;i<8;i++)
                        {
                	        for(j=0;j<8;j++)
                            {
						        tmpbitmap->line[starty-i][startx+j] = backcolour[backline+i];
                            }
                        }
                    }
                    else
                    {
				        for (i=0;i<8;i++)
                        {
                	        for(j=0;j<8;j++)
                            {
						        tmpbitmap->line[starty+i][startx+j] = backcolour[backline+i];
                            }
                        }
                    }
                }
                else
                {
			        starty = sy * 8;
			        startx = backline = sx * 8;

				    if (Machine->orientation & ORIENTATION_FLIP_Y)
                	    starty = (255 - starty) - 7;

				    if (Machine->orientation & ORIENTATION_FLIP_X)
                    {
					    startx = (255 - startx);

				        for (i=0;i<8;i++)
                        {
                	        for(j=0;j<8;j++)
                            {
						        tmpbitmap->line[starty+i][startx-j] = backcolour[backline+j];
                            }
                        }
                    }
                    else
                    {
				        for (i=0;i<8;i++)
                        {
                	        for(j=0;j<8;j++)
                            {
						        tmpbitmap->line[starty][startx+j] = backcolour[backline+j];
                            }
                            starty++;
                        }
                    }
                }

                /* Overlay foreground */

 			    drawgfx(tmpbitmap,Machine->gfx[0],
					    charcode,
					    galaxian_attributesram[2 * (offs % 32) + 1] & 0x07,
					    flipscreen[0],flipscreen[1],
					    8*sx,8*sy,
					    0,TRANSPARENCY_COLOR,0);
            }
            else
            {
 			    drawgfx(tmpbitmap,Machine->gfx[0],
					    charcode,
					    galaxian_attributesram[2 * (offs % 32) + 1] & 0x07,
					    flipscreen[0],flipscreen[1],
					    8*sx,8*sy,
					    0,TRANSPARENCY_NONE,0);
            };
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];


		if (flipscreen[0])
		{
			for (i = 0;i < 32;i++)
			{
				scroll[31-i] = -galaxian_attributesram[2 * i];
				if (flipscreen[1]) scroll[31-i] = -scroll[31-i];
			}
		}
		else
		{
			for (i = 0;i < 32;i++)
			{
				scroll[i] = -galaxian_attributesram[2 * i];
				if (flipscreen[1]) scroll[i] = -scroll[i];
			}
		}

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the bullets */
	for (offs = 0;offs < galaxian_bulletsram_size;offs += 4)
	{
		int x,y;
		int color;


		if (offs == 7*4) color = 0;	/* yellow */
		else color = 1;	/* white */

		x = 255 - galaxian_bulletsram[offs + 3] - Machine->drv->gfxdecodeinfo[2].gfxlayout->width;
		y = 255 - galaxian_bulletsram[offs + 1];
		if (flipscreen[1]) y = 255 - y;

		drawgfx(bitmap,Machine->gfx[2],
				0,	/* this is just a line, generated by the hardware */
				color,
				0,0,
				x,y,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* Draw the sprites */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int flipx,flipy,sx,sy,spritecode;


		sx = (spriteram[offs + 3] + 1) & 0xff;	/* ??? */
		sy = 240 - spriteram[offs];
		if (!sprite_bank)
		{
			flipx = spriteram[offs + 1] & 0x40;
			flipy = spriteram[offs + 1] & 0x80;
		}
		else
		{
			flipx = flipy = 0;
		}

		if (flipscreen[0])
		{
			sx = 241 - sx;	/* note: 241, not 240 (this is correct in Amidar, at least) */
			flipx = !flipx;
		}
		if (flipscreen[1])
		{
			sy = 240 - sy;
			flipy = !flipy;
		}

		/* In Amidar, */
		/* Sprites #0, #1 and #2 need to be offset one pixel to be correctly */
		/* centered on the ladders in Turtles (we move them down, but since this */
		/* is a rotated game, we actually move them left). */
		/* Note that the adjustment must be done AFTER handling flipscreen, thus */
		/* proving that this is a hardware related "feature" */
		/* This is not Amidar, it is Galaxian/Scramble/hundreds of clones, and I'm */
		/* not sure it should be the same. A good game to test alignment is Armored Car */
/*		if (offs <= 2*4) sy++;*/

		spritecode = spriteram[offs + 1];
		if (!sprite_bank) spritecode &= 0x3f;

		/* Moon Quasar and Crazy Kong have different bank selection bits*/
		if (spriteram[offs + 2] & bank_mask)
			spritecode += 64;

		/* gfx_bank is used by Pisces and japirem/Uniwars only */
		if (gfx_bank)
			spritecode += 64;

		/* gfx_extend is used by Moon Cresta only */
		if ((gfx_extend & 4) && (spritecode & 0x30) == 0x20)
			spritecode = (spritecode & 0x0f) | (gfx_extend << 4);

		drawgfx(bitmap,Machine->gfx[1],
				spritecode,
				spriteram[offs + 2] & 0x07,
				flipx,flipy,
				sx,sy,
				flipscreen[0] ? &spritevisibleareaflipx : &spritevisiblearea,TRANSPARENCY_PEN,0);
	}


	/* draw the stars */
	if (stars_on)
	{
		switch (stars_type)
		{
			case 0:	/* Galaxian stars */
				for (offs = 0;offs < total_stars;offs++)
				{
					int x,y;


					x = (stars[offs].x + stars_scroll/2) % 256;
					y = stars[offs].y;

					if ((y & 1) ^ ((x >> 4) & 1))
					{
						if (Machine->orientation & ORIENTATION_SWAP_XY)
						{
							int temp;


							temp = x;
							x = y;
							y = temp;
						}
						if (Machine->orientation & ORIENTATION_FLIP_X)
							x = 255 - x;
						if (Machine->orientation & ORIENTATION_FLIP_Y)
							y = 255 - y;

						if (bitmap->line[y][x] == Machine->pens[0] ||
								bitmap->line[y][x] == backcolour[x])
							bitmap->line[y][x] = stars[offs].col;
					}
				}
				break;

			case 1:	/* Scramble stars */
			case 2:	/* Rescue stars */
				for (offs = 0;offs < total_stars;offs++)
				{
					int x,y;


					x = stars[offs].x / 2;
					y = stars[offs].y;

					if ((stars_type != 2 || x < 128) &&	/* draw only half screen in Rescue */
							((y & 1) ^ ((x >> 4) & 1)))
					{
						if (Machine->orientation & ORIENTATION_SWAP_XY)
						{
							int temp;


							temp = x;
							x = y;
							y = temp;
						}
						if (Machine->orientation & ORIENTATION_FLIP_X)
							x = 255 - x;
						if (Machine->orientation & ORIENTATION_FLIP_Y)
							y = 255 - y;

						if (bitmap->line[y][x] == Machine->pens[0] ||
								bitmap->line[y][x] == backcolour[x])
						{
							switch (stars_blink)
							{
								case 0:
									if (stars[offs].code & 1) bitmap->line[y][x] = stars[offs].col;
									break;
								case 1:
									if (stars[offs].code & 4) bitmap->line[y][x] = stars[offs].col;
									break;
								case 2:
									if (stars[offs].x & 4) bitmap->line[y][x] = stars[offs].col;
									break;
								case 3:
									bitmap->line[y][x] = stars[offs].col;
									break;
							}
						}
					}
				}
				break;
		}
	}
}



int galaxian_vh_interrupt(void)
{
	stars_scroll++;

	return nmi_interrupt();
}



int scramble_vh_interrupt(void)
{
	static int blink_count;


	blink_count++;
	if (blink_count >= 45)
	{
		blink_count = 0;
		stars_blink = (stars_blink + 1) % 4;
	}

	return nmi_interrupt();
}
