/*************************************************************************
     Turbo - Sega - 1981

     Video Hardware

*************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern int turbo_f8pa, turbo_f8pb, turbo_f8pc;
extern int turbo_f9pa, turbo_f9pb, turbo_f9pc;
extern int turbo_fbpla, turbo_fbcol;
extern unsigned char turbo_seg_digit[32];
static unsigned char *road_rom;
unsigned char *PR1115_rom;
static unsigned char *crash_rom;
static unsigned char *pal_rom;
static unsigned char *pat_rom1;
static unsigned char *pat_rom2;


/* system8 stuff */
static int scrollx = 0;
static int scrolly = 0;
static int sprite_offset_y = 0;

static void DrawSprites(struct osd_bitmap *bitmap,const struct rectangle *clip);
static unsigned char 	*SpritesData = NULL;
unsigned char 	*turbo_sprites_collisionram;
int 	turbo_sprites_collisionram_size;
unsigned char 	*turbo_SpritesCollisionTable;

unsigned int turbo_collision;



void turbo_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < 512;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = (*color_prom >> 0) & 1;
		bit1 = (*color_prom >> 1) & 1;
		bit2 = (*color_prom >> 2) & 1;
		*(palette++) = 255 - (0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2);
		/* green component */
		bit0 = (*color_prom >> 3) & 1;
		bit1 = (*color_prom >> 4) & 1;
		bit2 = (*color_prom >> 5) & 1;
		*(palette++) = 255 - (0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2);
		/* blue component */
		bit0 = (*color_prom >> 6) & 1;
		bit1 = (*color_prom >> 7) & 1;
		*(palette++) = 255 - (0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2);

		color_prom++;
	}

	/* LED segments colors: black and red */
	*(palette++) = 0x00;
	*(palette++) = 0x00;
	*(palette++) = 0x00;
	*(palette++) = 0xff;
	*(palette++) = 0x00;
	*(palette++) = 0x00;

	/* color_prom now points to the beginning of the lookup table */

	for (i = 0;i < 4*64;i++)
	{
		/* four palette banks */
		COLOR(0,i + 0*4*64) = (*color_prom & 0x07) * 16 + 8 + 0*128;
		COLOR(0,i + 1*4*64) = (*color_prom & 0x07) * 16 + 8 + 1*128;
		COLOR(0,i + 2*4*64) = (*color_prom & 0x07) * 16 + 8 + 2*128;
		COLOR(0,i + 3*4*64) = (*color_prom & 0x07) * 16 + 8 + 3*128;
		/* bit 3 of the PROM is a priority bit; */
		/* bit 7 of the character code is another priority bit */

		color_prom++;
	}

	/* LED segments: */
	COLOR(1,0) = 512;	/* black */
	COLOR(1,1) = 513;	/* red */
}



void turbo_unpack_sprites(int region, int size)
{
	unsigned char *SpritesPackedData = Machine->memory_region[region];

	if (!SpritesData)
		SpritesData = malloc(size*2);

	if (SpritesData)
	{
		int a;
		for (a=0; a < size; a++)
		{
			SpritesData[a*2  ] = SpritesPackedData[a] >> 4;
			SpritesData[a*2+1] = SpritesPackedData[a] & 0xF;
		}
	}
}

/* *** */


static unsigned char number_data[] = {
	0x3e,0x41,0x41,0x41,0x00,0x41,0x41,0x41,0x3e,0,
	0x00,0x01,0x01,0x01,0x00,0x01,0x01,0x01,0x00,0,
	0x3e,0x01,0x01,0x01,0x3e,0x40,0x40,0x40,0x3e,0,
	0x3e,0x01,0x01,0x01,0x3e,0x01,0x01,0x01,0x3e,0,
	0x00,0x41,0x41,0x41,0x3e,0x01,0x01,0x01,0x00,0,
	0x3e,0x40,0x40,0x40,0x3e,0x01,0x01,0x01,0x3e,0,
	0x3e,0x40,0x40,0x40,0x3e,0x41,0x41,0x41,0x3e,0,
	0x3e,0x01,0x01,0x01,0x00,0x01,0x01,0x01,0x00,0,
	0x3e,0x41,0x41,0x41,0x3e,0x41,0x41,0x41,0x3e,0,
	0x3e,0x41,0x41,0x41,0x3e,0x01,0x01,0x01,0x3e,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0};

int turbo_vh_start(void)
{
	int i;
	if (generic_vh_start()!=0) return 1;
	if ((turbo_SpritesCollisionTable = malloc(0x10000)) == 0)
		return 1;
	for (i = 0;i < 16 ;i++)
        decodechar(Machine->gfx[1],i,number_data,
				   Machine->drv->gfxdecodeinfo[1].gfxlayout);
	road_rom = Machine->memory_region[4];
	PR1115_rom = road_rom+0x4800;
	crash_rom = road_rom+0x4820;
	pal_rom = Machine->memory_region[2]+0x300;
	pat_rom1 = Machine->memory_region[2]+0x400;
	pat_rom2 = Machine->memory_region[2]+0x800;
	return 0;
}



void turbo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	static struct rectangle clip = { 0, 32*8-1, 0, 29*8-1 };
	int offs, h, v, sum0, sum1, sel, coch, cont;
	int offset0, offset1, area1, area2, area3, area4, area5, area;
	int bacol, bar, bag, bab, color, col;
	int babits, slipar, road;
	int pat1, mx;
	int plb = 0; /* don't know it yet plb = sprites present */

	for (offs = 0x3a0 - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;
//			if ((offs==0x01d1)&&errorlog) fprintf(errorlog,"e1d1: %02x\n",videoram[offs]);
			dirtybuffer[offs] = 0;

			sy = offs / 32;
			sx = offs % 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					(videoram[offs] >> 2) + 64 * ((turbo_fbcol & 0x06) >> 1),
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

/*
 *	  draw the road
 */

	cont = 0;
	for (v=0;v<256-3*8;v++)
	{
		sum0 = (v + turbo_f8pa)&0xff;
		for (h=0;h<256;h++)
		{
			sum1 = h + turbo_f8pb;
		if (sum1&0x100)
			{
				sel  = turbo_f9pb;
				coch = turbo_f9pc&0xf0>>4;
			}
			else
			{
				sel  = turbo_f9pa;
				coch = turbo_f9pc&0x0f;
			}
			offset0 = ((sel&0x0f)<<8)+sum0;
			offset1 = ((sel&0xf0)<<4)+sum0;
			area1 = (((road_rom[offset0])+h)&0x100)>>8;
			area2 = (((road_rom[offset0+4096])+h)&0x100)>>8;
			area3 = (((road_rom[offset1+4096*2])+h)&0x100)>>8;
			area4 = (((road_rom[offset1+4096*3])+h)&0x100)>>8;
			/* if ((sum1&0x100)==0) */ cont = coch&0x0f; /* else? */
			area5 = ((road_rom[0x4000+((h&0xf8)>>3)+((turbo_f8pc&0x3f)<<5)])&0x80)>>7;
			area = (area5<<4)+(area4<<3)+(area3<<2)+(area2<<1)+area1;
			babits = ((PR1115_rom[area])&0x07)/*^7*/;
			slipar = (((PR1115_rom[area])&0x10)>>4)/*^1*/;
			road   = (((PR1115_rom[area])&0x20)>>5)/*^1*/;
			bacol = pal_rom[((turbo_fbcol&0x01)<<4)+cont] +
			  ((pal_rom[((turbo_fbcol&0x01)<<4)+cont+0x20])<<8);
			bar = (bacol&0x1f);
			bag = ((bacol&0x3e0)>>5);
			bab = ((bacol&0x7c00)>>10);
//			if (errorlog&&(bacol!=0xffff)) fprintf(errorlog,"bacol=%x\n",bacol);
			turbo_collision |= crash_rom[turbo_SpritesCollisionTable[256*h+v]
								  +(slipar<<3)+(road<<4)];
//			pat1 = pat_rom1[(plb>>1)+((turbo_fbpla&0x07)<<7)];
			pat1 = 0;
			mx = pat_rom2[pat1+((plb&0x01)<<3) /* +PLBE+PLBF */
						 +(babits<<6)+((turbo_fbpla&0x08)<<6)];
			if (mx>=9)
			{
				bar = (bar>>(mx-9))&0x01;
				bag = (bag>>(mx-9))&0x01;
				bab = (bab>>(mx-9))&0x01;
				color = (bar+(bag<<1)+(bab<<2)); /* 7^ is a hack */
				bitmap->line[255-h][v]=Machine->pens[mx+((color)<<4)+((turbo_fbcol&0x06)<<6)];
			}
			else bitmap->line[255-h][v]=Machine->pens[mx*16+8];
		}
	}

	copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,1,&scrolly,&clip,TRANSPARENCY_COLOR,0);

	DrawSprites(bitmap, &clip);

	for (offs = 31; offs >= 2; offs-=5)
	{
		for (v = 0; v < 5; v++)
		{
			drawgfx(bitmap,Machine->gfx[1],
					turbo_seg_digit[offs-v],
					1,
					0,0,
					((offs-1)/5)*16,(30+v)*8,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}
}


/* system8 stuff */

#define SPR_Y_TOP		0
#define SPR_Y_BOTTOM	1
#define SPR_X_LO		2
#define SPR_X_HI		3
#define SPR_BANK		3
#define SPR_WIDTH_LO	4
#define SPR_WIDTH_HI	5
#define SPR_GFXOFS_LO	6
#define SPR_GFXOFS_HI	7
#define SPR_FLIP_X		7

static void ClearSpritesCollisionTable(void)
{
	int col,row,i,sx,sy;

	for (i = 0; i<1024; i++)
	{
		if (1) //bg_dirtybuffer[i])
		{
			sx = (i % 32)<<3;
			sy = (i >> 5)<<3;
			for(row=sy; row<sy+8; row++)
				for(col=sx; col<sx+8; col++)
					turbo_SpritesCollisionTable[256*row+col] = 255;
		}
	}
}

static int GetSpriteBottomY(int spr_number)
{
	return  spriteram[(spr_number<<4) + SPR_Y_BOTTOM];
}


static void Pixel(struct osd_bitmap *bitmap,int x,int y,int spr_number,int color)
{
	int xr,yr,spr_y1,spr_y2;
	int SprOnScreen,coll_dat;
	{
//		SprOnScreen=turbo_SpritesCollisionTable[256*y+x];
//		spr_y1 = GetSpriteBottomY(spr_number);
//		spr_y2 = GetSpriteBottomY(SprOnScreen);
//		if (spr_y1 >= spr_y2)
		{
			bitmap->line[255-y][x]=Machine->pens[color];
			switch(spr_number&0x07)
			{
				case 0:
					coll_dat = 1;
					break;
				case 1:
					coll_dat = 2;
					break;
				case 2:
					coll_dat = 4;
					break;
				default:
					coll_dat = 0;
					break;
			}
			turbo_SpritesCollisionTable[256*y+x]|=coll_dat;
		}
	}
}

static void RenderSprite(struct osd_bitmap *bitmap, const struct rectangle *clip, int spr_number)
{
	int SprX,SprY,Col,Row,Height,DataOffset,FlipX;
	int Color,scrx,scry,Bank;
	unsigned char *SprPalette,*SprReg;
	unsigned short NextLine,Width,Offset16;

	SprReg		= spriteram + (spr_number<<4);
//	Bank		= ((((SprReg[SPR_BANK] & 0x80)>>7)+((SprReg[SPR_BANK] & 0x40)>>5))<<16);
	Bank		= (spr_number&0x07)*0x8000;
	Width 		= SprReg[SPR_WIDTH_LO] + (SprReg[SPR_WIDTH_HI]<<8);
	Height		= SprReg[SPR_Y_BOTTOM] - SprReg[SPR_Y_TOP];
//	FlipX 		= SprReg[SPR_FLIP_X] & 0x80;
	FlipX 		= 0;
	DataOffset 	= (SprReg[SPR_GFXOFS_LO]+((SprReg[SPR_GFXOFS_HI] & 0x7F)<<8)+Width)&0x7fff;
	SprPalette	= 0; //system8_spritepaletteram + (spr_number<<4);
	SprX 		= (SprReg[SPR_X_LO] >> 1) + ((SprReg[SPR_X_HI] & 1) << 7);
	SprY 		= SprReg[SPR_Y_TOP] + 1;
	NextLine	= Width;

	if ((spr_number&0x07)<3)
	{
		SprX = (Width>0?120-2*Width:120+2*Width);  //brutal hack
	}

//	if (DataOffset & 0x8000) FlipX^=0x80;
	if (Width & 0x8000) Width = (~Width)+1;		// width is negative
							// and this means that sprite will has fliped Y

//    SprX = 128 + (SprX/2);
//	Width<<=1;


//	MarkBackgroundDirtyBufferBySprite(SprX,SprY,Width,Height);

	for(Row=0; Row<Height; Row++)
	{
		Offset16 = (DataOffset+(Row*NextLine)); // << 1;
		scry = (unsigned char)(((SprY+Row) + scrolly + sprite_offset_y) % 256);
		if ((clip->min_y <= SprY+Row) && (clip->max_y >= SprY+Row))
		{
			for(Col=0; Col<Width; Col++)
			{
				Color = (FlipX) ? SpritesData[Bank+Offset16+Width-Col-3]
//				Color = (FlipX) ? SpritesData[Bank+Offset16-Col+1]
						: SpritesData[Bank+Offset16+Col];
				if (Color == 4) break;
				if (Color && (clip->min_x <= SprX+Col) && (clip->max_x >= SprX+Col))
				{
					scrx = (unsigned char)(((SprX+Col) - scrollx) % 256);
					Pixel(bitmap,scry,scrx,spr_number,((Color&0x07)*16)+(spr_number&0x07)); //SprPalette[Color]);
				}
			}
		}
	}
}

static void DrawSprites(struct osd_bitmap *bitmap,const struct rectangle *clip)
{
	int spr_number,SprBottom,SprTop;
	unsigned char *SprReg;

	if (!clip) clip = &Machine->drv->visible_area;

//	if (Check_SpriteRAM_for_Clear)
//		Check_SpriteRAM_for_Clear();

	memset(turbo_sprites_collisionram,0,turbo_sprites_collisionram_size);

	for(spr_number=0; spr_number<16; spr_number++)
	{
		SprReg 		= spriteram + (spr_number<<4);
		SprTop		= SprReg[SPR_Y_TOP];
		SprBottom	= SprReg[SPR_Y_BOTTOM];
		if (SprBottom && (SprBottom-SprTop > 0))
			RenderSprite(bitmap,clip,spr_number);
	}

//	ClearSpritesCollisionTable();
}
