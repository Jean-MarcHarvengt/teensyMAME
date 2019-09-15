/***************************************************************************

	POW - Prisoners Of War, SNK 1988

	Emulation by Bryan McPhail, mish@tendril.force9.net

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/******************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap, int j)
{
	int offs,mx,my,color,tile,fx,fy,i;

	for (offs = 0x0000; offs < 0x1000; offs += 0x80 )
	{
		int multiple;

		mx=(READ_WORD(&spriteram[offs+4+(4*j)])&0xff)<<4;
		my=READ_WORD(&spriteram[offs+6+(4*j)]);
		mx=mx+(my>>12);
		mx=((mx+16)&0x1ff)-16;

		mx=(mx+0x100)&0x1ff;
		my=(my+0x100)&0x1ff;
		mx-=0x100;
		my-=0x100;
		my=0x200 - my;
		my-=0x200;

		for (i=0; i<0x80; i+=4) {
			tile=READ_WORD(&spriteram[offs+2+i+(0x1000*j)+0x1000]);
			color=READ_WORD(&spriteram[offs+i+(0x1000*j)+0x1000]);

			fy=tile&0x8000;
			fx=tile&0x4000; /* Should we adjust X co-ord on flips? */
			tile&=0x3fff;

			if (tile)
				drawgfx(bitmap,Machine->gfx[1],
					tile,
					color&0x7f,
					fx,fy,
					mx,my,
					0,TRANSPARENCY_PEN,0);

			my+=16;
			if (my > 0x100) my-=0x200;
		}
	}
}

/******************************************************************************/

void pow_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int offs,mx,my,color,tile,i;
	int colmask[0x80],code,pal_base;

	/* Build the dynamic palette */
	memset(palette_used_colors,PALETTE_COLOR_UNUSED,2048 * sizeof(unsigned char));

	/* Text layer */
	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;
	for (color = 0;color < 128;color++) colmask[color] = 0;
	for (offs = 0;offs <0x1000;offs += 4)
	{
		color = READ_WORD(&videoram[offs+2]) &0xf;
		code = READ_WORD(&videoram[offs]);
        if (code==0xff20) continue;
		colmask[color] |= Machine->gfx[0]->pen_usage[code&0xff];
	}
	for (color = 0;color < 128;color++)
	{
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	/* Tiles */
	pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;
	for (color = 0;color < 128;color++) colmask[color] = 0;
	for (offs = 0x1000;offs <0x4000;offs += 4 )
	{
		code = READ_WORD(&spriteram[offs+2])&0x3fff;
		color= READ_WORD(&spriteram[offs])&0x7f;
		colmask[color] |= Machine->gfx[1]->pen_usage[code];
	}
	for (color = 0;color < 128;color++)
	{
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	palette_recalc();
	osd_clearbitmap(bitmap);
//	fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);

	/* This appears to be correct priority (with a few glitches) */
	draw_sprites(bitmap,1);
	draw_sprites(bitmap,0);
	draw_sprites(bitmap,2);

	/* Text layer */
	my = -1;
	mx = 0;
	for (offs = 0x000; offs < 0x1000;offs += 4)
	{
		my++;
		if (my == 32)
		{
			my = 0;
			mx++;
		}
		tile = READ_WORD(&videoram[offs]); // + ((READ_WORD(&videoram[offs+2])&0x7)<<8);
		color = READ_WORD(&videoram[offs+2]) &0xf;
		if (tile!=0xff20)
			drawgfx(bitmap,Machine->gfx[0],
					tile & 0xff,
					color,
					0,0,
					8*mx,8*my,
					0,TRANSPARENCY_PEN,0);
    }
}

/******************************************************************************/
