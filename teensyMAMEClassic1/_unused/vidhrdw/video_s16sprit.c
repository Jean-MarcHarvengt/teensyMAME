#include "driver.h"

/*
 *	Used by System 16 Sprite Drawing
 *
 *	To Do:
 *	make more general
 *	optimize non-zoomed case
 *	rotation
*/

#define IS_OPAQUE(X) ((X+1) & 0xEE)

void drawgfxpicture(
	struct osd_bitmap *bitmap,
	const unsigned char *source,
	int width, int screenheight,
	const unsigned short *pal,
	int xflip, int yflip,
	int sx, int sy,
	int zoom,
	const struct rectangle *clip
);

void drawgfxpicture(
	struct osd_bitmap *bitmap,
	const unsigned char *source,
	int width, int screenheight,
	const unsigned short *pal,
	int xflip, int yflip,
	int sx, int sy,
	int zoom,
	const struct rectangle *clip
){
	int xpos,ypos;
	int ycount = 0;
	int xcount0 = 0;

	int delta = (1<<10)+zoom; /* 10000000000 .. 11111111111 */
	/*
		xcount0, xcount, ycount, and delta are fixed point

		zoom = 0x000 draws a sprite normally (scale = 1.0)
		zoom = 0x3ff draws a sprite shrunk down to fifty percent (scale = 0.50)
	*/

	int screenwidth = width*(1<<10)/delta; /* scale adjust */

	{ /* clipping */
		int pixels;

		pixels = clip->min_x - sx;
		if( pixels>0 ){ /* clip left */
			if( !xflip ) xcount0 = delta*pixels;
			screenwidth -= pixels;
			sx = clip->min_x;
		}

		pixels = clip->min_y - sy;
		if( pixels>0 ){ /* clip top */
			if( !yflip ) ycount = delta*pixels;
			screenheight -= pixels;
			sy = clip->min_y;
		}

		pixels = sx+screenwidth - clip->max_x-1;
		if( pixels>0 ){ /* clip right */
			if( xflip ) xcount0 = delta*pixels;
			screenwidth -= pixels;
		}

		pixels = sy+screenheight - clip->max_y-1;
		if( pixels>0 ){ /* clip bottom */
			if( yflip ) ycount = delta*pixels;
			screenheight -= pixels;
		}
	}

	if( screenwidth>0 ){
		int x1,x2,dx;

		if( yflip ) width = -width;

		if( xflip ){
			x1 = sx+screenwidth-1;
			x2 = sx-1;
			dx = -1;
		}
		else{
			x1 = sx;
			x2 = sx+screenwidth;
			dx = 1;
		}

		if( bitmap->depth!=16 ){
			if( delta==(1<<10) ){
				source += width*(ycount>>10) + (xcount0>>10);

				for( ypos = sy; ypos < sy+screenheight; ypos++ ){
					unsigned char *dest = &bitmap->line[ypos][0];
					int xcount = 0;

					for( xpos = x1; xpos!=x2; xpos+=dx ){
						unsigned char pen = source[xcount];
						if( IS_OPAQUE(pen) ) dest[xpos] = pal[pen];
						xcount++;
					}
					source+=width;
				}
			}
			else {
				for( ypos = sy; ypos < sy+screenheight; ypos++ ){
					unsigned char *dest = &bitmap->line[ypos][0];
					const unsigned char *src = source + width*(ycount>>10);

					int xcount = xcount0;

					for( xpos = x1; xpos!=x2; xpos+=dx ){
						unsigned char pen = src[xcount>>10];
						if( IS_OPAQUE(pen) ) dest[xpos] = pal[pen];
						xcount += delta;
					}
					ycount+=delta;
				}
			}
		}
		else { /* 16 bit color */
			for( ypos = sy; ypos < sy+screenheight; ypos++ ){
				unsigned short *dest = (unsigned short *)&bitmap->line[ypos][0];
				const unsigned char *src = source + width*(ycount>>10);

				int xcount = xcount0;

				for( xpos = x1; xpos!=x2; xpos+=dx ){
					unsigned char pen = src[xcount>>10];
					if( IS_OPAQUE(pen) ) dest[xpos] = pal[pen];
					xcount += delta;
				}
				ycount+=delta;
			}
		}
	}
}
