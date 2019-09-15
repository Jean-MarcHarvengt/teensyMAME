/***************************************************************************

  liberator.c - 'vidhrdw.c'

  Functions to emulate the video hardware of the machine.

   Liberator's screen is 256 pixels by 256 pixels.  The
     round planet in the middle of the screen is 128 pixels
     tall by 96 equivalent (192 at double pixel rate).  The
     emulator needs to account for the aspect ratio of 4/3
     from the arcade video system in order to make the planet
     appear round.

   Two methods have been tried using the #defines below and in
     the driver.c file.  In the 342x256 case, every 3rd bitmap x
     pixel is doubled, and the planet data is subsampled.  In the
     512x384 case, every bitmap pixel is doubled in x, and
     every other bitmap and planet line is doubled in y.  The larger
     method looks better on the screen (in particular the cursor, missile
     tracks and the spare ships look better) but is somewhat
     slower because of the extra pixels to bash around.

   As it is, the 512x384 driver runs only about 80% frame rate
     on my PowerMac 7100/80 (80MHz 601) using Frameskip3.  The 342x256
     driver runs ~85-95%.  Ideas on speed up welcomed.  I leave it
     to the powers that be to decide which version to ship.  My
     preference is 512x384, but if it is too slow on the 'reference'
     machine, then so be it...

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"

#define LIB_VIDEORAM_SIZE	(0x100*0x100)

/* Important:
//   These next two defines must match the ones in the
//     driver file, or very bad things will happen.		*/
#define LIB_ASPECTRATIO_512x384		0
#define LIB_ASPECTRATIO_342x256		1

/*
// The following structure describes the (up to 32) line segments
//   that make up one horizontal line (latitude) for one display frame of the planet.
// Note: this and the following structure is only used to collect the
//   data before it is packed for actual use.
*/
typedef struct {
	unsigned char	nsegs;			/* the number of segments on this line */
	unsigned char	xmax ;			/* the maximum value of x_a for this line */
	unsigned short	align ;
	unsigned char	cc_a[32] ;		/* the color values  */
	unsigned char	x_a[32] ;		/* and maximum x values for each segment  */
} LibSegs ; /* 32 + 32 + 4 = 68 bytes */

/*
// The following structure describes the lines (latitudes)
//   that make up one complete display frame of the planet.
// Note: this and the previous structure is only used to collect the
//   data before it is packed for actual use.
*/
typedef struct {
	LibSegs line[ 128 ] ;
} LibView ; /* 68 * 128 = 8704 bytes */

/*
// The following structure collects the 256 views of the
//   planet (one per value of startlg (start longitude).
// The data is packed nsegs,xstrt,cc,xx,cc,xx,...  then
//                    nsegs,xstrt,cc,xx,cc,xx...  for the next line, etc
//   for the 128 lines.
*/
typedef struct {
	unsigned char	*view[ 256 ] ;
} LibPlanet ; /* 4 * 256 = 1024 bytes */


/*
//	The following two arrays are Prom dumps off the processor
//	board.  Should we generate a function instead?
*/
static unsigned char ltscale[] = {
	0x25,0x3A,0x4A,0x55,0x5F,0x6A,0x75,0x7A,0x80,0x8A,0x8F,0x95,0x9A,0xA0,0xA5,0xAA,
	0xB0,0xB5,0xB5,0xBA,0xC0,0xC0,0xC5,0xCA,0xCA,0xCF,0xCF,0xD5,0xD5,0xDA,0xDA,0xDF,
	0xDF,0xE5,0xE5,0xE5,0xEA,0xEA,0xEA,0xEF,0xEF,0xEF,0xEF,0xF5,0xF5,0xF5,0xF5,0xFA,
	0xFA,0xFA,0xFA,0xFA,0xFA,0xFA,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFA,0xFA,0xFA,0xFA,0xFA,0xFA,
	0xFA,0xF5,0xF5,0xF5,0xF5,0xEF,0xEF,0xEF,0xEF,0xEA,0xEA,0xEA,0xE5,0xE5,0xE5,0xDF,
	0xDF,0xDA,0xDA,0xD5,0xD5,0xCF,0xCF,0xCA,0xCA,0xC5,0xC0,0xC0,0xBA,0xB5,0xB5,0xB0,
	0xAA,0xA5,0xA0,0x9A,0x95,0x8F,0x8A,0x80,0x7A,0x75,0x6A,0x5F,0x55,0x4A,0x3A,0x25 } ;

static unsigned char lgscale[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x02,0x02,0x02,0x02,0x03,
	0x03,0x03,0x04,0x04,0x05,0x05,0x05,0x06,0x06,0x07,0x07,0x08,0x08,0x09,0x09,0x0A,
	0x0A,0x0B,0x0B,0x0C,0x0C,0x0D,0x0E,0x0E,0x0F,0x10,0x10,0x11,0x12,0x12,0x13,0x14,
	0x15,0x15,0x16,0x17,0x18,0x19,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x1F,0x20,0x21,
	0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,
	0x32,0x33,0x34,0x35,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x40,0x41,0x42,0x43,
	0x44,0x45,0x46,0x48,0x49,0x4A,0x4B,0x4C,0x4E,0x4F,0x50,0x51,0x52,0x54,0x55,0x56,
	0x57,0x58,0x5A,0x5B,0x5C,0x5D,0x5F,0x60,0x61,0x62,0x63,0x65,0x66,0x67,0x68,0x6A,
	0x6B,0x6C,0x6D,0x6F,0x70,0x71,0x72,0x73,0x75,0x76,0x77,0x78,0x79,0x7B,0x7C,0x7D,
	0x7E,0x7F,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
	0x90,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,0xA0,
	0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAE,
	0xAF,0xB0,0xB1,0xB2,0xB2,0xB3,0xB4,0xB5,0xB5,0xB6,0xB7,0xB7,0xB8,0xB9,0xB9,0xBA,
	0xBB,0xBB,0xBC,0xBC,0xBD,0xBD,0xBE,0xBE,0xBF,0xBF,0xC0,0xC0,0xC1,0xC1,0xC2,0xC2,
	0xC2,0xC3,0xC3,0xC4,0xC4,0xC4,0xC5,0xC5,0xC5,0xC5,0xC6,0xC6,0xC6,0xC6,0xC6,0xC7,
	0xC7,0xC7,0xC7,0xC7,0xC7,0xC7,0xC7,0xC7,0xC7,0xC7,0xC7,0xFF,0xFF,0xFF,0xFF,0xFF } ;

unsigned char				*lib_videoram = NULL,
							*lib_raw_colorram = NULL,
							*lib_basram = NULL  ;
static int					lib_planetbit = 0 ;
static int					lib_startlg = 0 ;

/*
// The following array collects the 2 different planet
//   descriptions, which are selected with lib_planetbit
*/
static LibPlanet *lib_planet_segs[2] = { NULL, NULL } ;


/********************************************************************************************/
static void lib_drawplanet(int data) ;
static void lib_init_planet(void) ;


/********************************************************************************************/
void liberator_startlg_w(int offset,int data)
{
	lib_startlg = data ;

} /* liberator_startlg_w */

/********************************************************************************************/
void liberator_planetbit_w(int offset,int data)
{
	lib_planetbit = (data & 0x10) ? 1 : 0 ;

} /* liberator_planetbit_w */

/********************************************************************************************/
void liberator_bitmap_w(int offset, int data)
{
	int addr;
	int	d ;
	int xt, yt ;
	unsigned char tmb ;
	/* TODO: get rid of this */
	extern unsigned char *RAM;
	int xcoor = RAM[ 0x0000 ] ;
	int ycoor = RAM[ 0x0001 ] ;

	if( offset == 2 )
	{
		addr = ((ycoor<<8) + xcoor)>>0 ;
		d = data & 0xe0 ;

		lib_videoram[addr] = d ;

		tmb = Machine->pens[(d >> 5) + 0x10];

#if LIB_ASPECTRATIO_512x384
		xt = 2 * xcoor ;
		yt = ((ycoor * 3) + 1) / 2 ;
		tmpbitmap->line[yt][xt]   =
		tmpbitmap->line[yt][xt+1] = tmb ;
		if(!( ycoor & 1 ))
		{
			tmpbitmap->line[yt+1][xt]   =
			tmpbitmap->line[yt+1][xt+1] = tmb ;
		}
#elif LIB_ASPECTRATIO_342x256
		xt = ((xcoor * 4) + 2) / 3 ;
		yt = ycoor ;
		tmpbitmap->line[yt][xt] = tmb ;
		if( (xcoor % 3) == 0 )
			tmpbitmap->line[yt][xt+1] = tmb ;
#endif

	}
	else if( offset == 0x341 )		/* part of the write to clear the bitmap RAM	*/
	{
		memset( lib_videoram , 0x00 , LIB_VIDEORAM_SIZE ) ;
		for( ycoor = 0 ; ycoor < Machine->drv->screen_height ; ycoor++)
			for( xcoor = 0 ; xcoor < Machine->drv->screen_width ; xcoor++ )
				tmpbitmap->line[ycoor][xcoor] = Machine->pens[0x10];
	}

} /* liberator_bitmap_w */

/********************************************************************************************/
int liberator_bitmap_r (int address)
{
	int addr,data;
	/* TODO: get rid of this */
	extern unsigned char *RAM;
	int xcoor = RAM[ 0x0000 ] ;
	int ycoor = RAM[ 0x0001 ] ;

	addr = (ycoor<<8) + xcoor ;
	data = lib_videoram[addr] ;

	return( data ) ;
} /* liberator_bitmap_r */


/********************************************************************************************/
void liberator_basram_w(int address, int data)
{
	int	addr ;

	addr = address & 0x1f ;

	lib_basram[ addr ] = data ;

} /* liberator_basram_w */

/********************************************************************************************/
void liberator_colorram_w(int offset,int data)
{
	unsigned char	red, green, blue ;
	static unsigned char	map[]     = {0xff,0xdf,0xb8,0x97,0x68,0x47,0x20,0x00} ;
	static unsigned char	bluemap[] = {0xff,0x00,0xb8,0x00,0x68,0x00,0x00,0x00} ;
	/* handle the hardware flip of the bit order from 765 to 576 that
	//   hardware does between vram and color ram */
	static unsigned char	penmap[]={0x10,0x12,0x14,0x16,0x11,0x13,0x15,0x17} ;


	red   = map[((data >> 3) & 0x07)] ;
	green = map[((data     ) & 0x07)] ;
	blue  = bluemap[((data >> 5) & 0x06)] ;

	lib_raw_colorram[offset] = data ;

	if (offset & 0x10)
	{
		/* bitmap colorram values */
		offset = penmap[offset & 0x07] ;
 	}
	else
	{
		offset ^= 0x0f;
	}

	palette_change_color(offset,red,green,blue);

} /* liberator_colorram_w */


/********************************************************************************************/
void liberator_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* put the planet into tmpbitmap */
	lib_drawplanet( lib_startlg ) ;

	/* copy tmpbitmap to the screen */
	copybitmap(
		bitmap,							/* struct osd_bitmap *dest,			*/
		tmpbitmap,						/* struct osd_bitmap *src,			*/
		0,0,							/* int flipx,int flipy,				*/
		0,0,							/* int sx,int sy,					*/
		&Machine->drv->visible_area,	/* const struct rectangle *clip,	*/
		TRANSPARENCY_NONE,				/* int transparency,				*/
		0								/* int transparent_color			*/
		) ;
} /* liberator_vh_update */



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int liberator_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	lib_videoram     	= calloc( 1 , LIB_VIDEORAM_SIZE ) ;
	lib_raw_colorram 	= calloc( 1 , 0x20 ) ;
	lib_basram      	= calloc( 1 , 0x20 ) ;


	/*
	// allocate the planet descriptor structure
	*/
	if( lib_planet_segs[0] == NULL )
	{
		lib_planet_segs[0] = calloc( 1 , sizeof( LibPlanet ) ) ;
	}
	if( lib_planet_segs[1] == NULL )
	{
		lib_planet_segs[1] = calloc( 1 , sizeof( LibPlanet ) ) ;
	}

	/*
	// for each planet in the planet ROMs
	*/
	lib_planetbit = 1 ;
	lib_init_planet() ;

	lib_planetbit = 0 ;
	lib_init_planet() ;

	return 0;
} /* liberator_vh_start */

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void liberator_vh_stop(void)
{
	int i ;

	if( lib_videoram )
	{
		free( lib_videoram ) ;
		lib_videoram = NULL ;
	}
	if( lib_raw_colorram )
	{
		free( lib_raw_colorram ) ;
		lib_raw_colorram = NULL ;
	}
	if( lib_basram )
	{
		free( lib_basram ) ;
		lib_basram = NULL ;
	}
	if( lib_planet_segs[0] )
	{
		for( i = 0 ; i < 256 ; i++ )
			if( (*lib_planet_segs[0]).view[i] )
				free( (*lib_planet_segs[0]).view[i] ) ;
		free( lib_planet_segs[0] ) ;
		lib_planet_segs[0] = NULL ;
	}
	if( lib_planet_segs[1] )
	{
		for( i = 0 ; i < 256 ; i++ )
			if( (*lib_planet_segs[1]).view[i] )
				free( (*lib_planet_segs[1]).view[i] ) ;
		free( lib_planet_segs[1] ) ;
		lib_planet_segs[1] = NULL ;
	}

	osd_free_bitmap(tmpbitmap);
	tmpbitmap = NULL ;

} /* liberator_vh_stop */

/********************************************************************************************
  lib_init_planet()

  The data for the planet is stored in ROM using a run-length type of encoding.  This
  function does the conversion to the above structures and then a smaller
  structure which is quicker to use in real time.

  Its a multi-step process, reflecting the history of the code.  Not quite as efficient
  as it might be, but this is not realtime stuff, so who cares...
 ********************************************************************************************/
static void
lib_init_planet()
{
	unsigned long	i, addr, cc, lg, misc, fsg, lgs, lts, x, nsegs, maxnsegs=0, totalnsegs=0, strt_scg ;
	unsigned long	x_a[32],cc_a[32],fsg_a[32] ;
	unsigned long	startlg, vdl, scg;
	unsigned char	*buf ;
	unsigned short	pprom ;
	LibSegs			*line = NULL ;
	LibView			*view = NULL ;

	/*
	// for each starting longitude
	*/
	for(startlg=0 ; startlg < 0x100 ; startlg++)
	{
		totalnsegs = 0 ;

		if( view == NULL )
			if( (view = (LibView *)calloc( 1, sizeof( LibView ) )) == NULL )
				return ;

		/*
		// for each latitude (vdl)
		*/
		for( vdl = 0 ; vdl <= 0x7f ; vdl++ )
		{
			/*
			// point to the structure which will hold the data for this line
			*/
			line = &(*view).line[ vdl ] ;

			/*
			// latitude scaling factor
			*/
			lts = ltscale[ vdl ] ;

			/*
			// for this latitude (vdl), load the 32 segments into the _a arrays
			*/
			memset( fsg_a , 0 , 32*sizeof(unsigned long) ) ;
			for( scg = 0 ; scg <= 0x1f ; scg++ )
			{
				/*
				// read the planet picture ROM and get the
				//   latitude and longitude scaled from the scaling PROMS
				*/
				addr = (vdl << 5) + scg ;
				if( lib_planetbit )
					pprom = (ROM[0x0000+addr] << 8) + ROM[0x1000+addr] ;
				else
					pprom = (ROM[0x2000+addr] << 8) + ROM[0x3000+addr] ;

				misc =  (pprom >> 12) & 0x07 ;
				cc   =  (pprom >>  8) & 0x0f ;
				lg   = ((pprom <<  1) & 0x1fe) + ((pprom >> 15) & 0x01) ;

				/*
				// scale the longitude limit (adding the starting longitude)
				*/
				addr = startlg + ( lg >> 1 ) + ( lg & 1 ) ;		/* shift with rounding */
				fsg        =
				fsg_a[scg] = (( addr & 0x100 ) ? 1 : 0) ;
				if( addr & 0x80 )
				{
					lgs = 0xff ;
				}
				else
				{
					addr = ((addr & 0x7f) << 1) + (((lg & 1) || fsg) ? 0 : 1) ;
					lgs = lgscale[ addr ] ;
				}

				/*
				// x_a  is the x coordinate limit for this segment
				// cc_a is the color of this segment
				*/
				x_a[ scg ]  = ((lts * lgs) + 0x80) >> 8 ;	/* round it */
				cc_a[ scg ] = cc ;

			} /* scg */

			/*
			// determine which segment is the western horizon and
			//   leave scg indexing it.
			*/
			for( scg = 0 ; scg < 0x20 ; scg++ )
				if( fsg_a[scg] ) break;
			if( scg >= 0x20 )
				scg = 0x1f ;

			/*
			// transfer from the temporary arrays to the structure
			*/
			(*line).xmax =  (lts * 0xc0) >> 8 ;
			if( (*line).xmax & 1 )
				(*line).xmax += 1 ; 				/* make it even */

			/*
			// as part of the quest to reduce memory usage (and to a lesser degree
			//   execution time), stitch together segments that have the same color
			*/
			nsegs = 0 ;
			i = 0 ;
			strt_scg = scg ;
			do {
				cc = cc_a[scg] ;
				while( cc == cc_a[scg] )
				{
					x = x_a[scg] ;
					scg = (scg+1) & 0x1f ;
					if( scg == strt_scg )
						break;
				}
				(*line).cc_a[ i ] = cc ;
				(*line).x_a[ i ]  = (x > (*line).xmax) ? (*line).xmax : x ;
				i++ ;
				nsegs++ ;
			} while( (i < 32) && (x <= (*line).xmax) ) ;
			if( nsegs > maxnsegs ) maxnsegs = nsegs ;
			totalnsegs += nsegs ;
			(*line).nsegs = nsegs ;

		} /* vdl */

		/* now that the all the lines have been processed, and we know how
		//   many segments it will take to store the description, allocate the
		//   space for it and copy the data to it.
		*/
		if( (buf = (unsigned char *)calloc( sizeof(unsigned char), 2*(128 + totalnsegs) ) ) == NULL)
			return ;
		(*lib_planet_segs[ lib_planetbit ]).view[ startlg ] = buf ;
		for( vdl = 0 ; vdl < 128 ; vdl++ )
		{
			line  = &(*view).line[ vdl ] ;
			nsegs  = (*line).nsegs ;
			*buf++ = nsegs ;
#if LIB_ASPECTRATIO_512x384
			/* calculate the tmpbitmap's x coordinate for the western horizon
			//   center of tmpbitmap - (the number of planet pixels) / 2 */
			*buf++ = Machine->drv->screen_width/2 - (((*line).xmax) + 1) / 2 ;
#elif LIB_ASPECTRATIO_342x256
			/* calculate the tmpbitmap's x coordinate for the western horizon
			//   center of tmpbitmap - (two thirds of number of planet pixels) / 2 */
			*buf++ = Machine->drv->screen_width/2 - (((*line).xmax + 1) * 2) / 6 ;
#endif
			for( i = 0 ; i < nsegs ; i++ )
			{
				*buf++ = (*line).cc_a[ i ] ;
				*buf++ = (*line).x_a[ i ] ;
			}

		}

	} /* startlg */

	if( view != NULL )
		free( view ) ;

	return ;

} /* lib_init_planet */


/********************************************************************************************/
static void lib_drawplanet(int data)
{
	unsigned int	vdl, scg, startlg ;
	unsigned int	xa , cc, base, x, y, nsegs  ;
	unsigned int	tbmX  ;
	unsigned char	*tbm ;
	unsigned char	*buf ;
	unsigned char reverse_map[256];


	for (x = 0;x < 0x20;x++)
		reverse_map[Machine->pens[x]] = x;

	startlg = data & 0xff ;

	if( lib_planet_segs[ lib_planetbit ] )
		buf = (*lib_planet_segs[ lib_planetbit ]).view[ startlg ] ;
	else
		return ;

	/*
	// for each latitude (vdl)
	*/
	for( vdl = 0 ; vdl <= 0x7f ; vdl++ )
	{
		/*
		// grab the color value for the base (if any) at this latitude
		*/
		base = lib_basram[ (vdl>>3) & 0x0f ] ;
		base = base ^ 0x0f ;

		x = 0 ;					/* from the western horizon */
		nsegs = *buf++ ;
		tbmX  = *buf++ ;

#if LIB_ASPECTRATIO_512x384

		y = ((64 + vdl) * 3 + 1) / 2 ;

#elif LIB_ASPECTRATIO_342x256

		y = 64 + vdl ;

#endif

		/*
		// run through the segments, drawing its color
		//   until its x_a value comes up.
		*/
		for( scg = 0 ; scg < nsegs ; scg++ )
		{
			cc = *buf++ ;
			xa = *buf++ ;
			if( (cc & 0x0c) == 0x0c )
				cc = base ;

			while( x < xa )
			{

#if LIB_ASPECTRATIO_512x384
				/* planet video doesn't overwrite bitmap video, so
				//   check the tmpbitmap where we want to draw into.
				// bitmap writes into the tmpbitmap all have bit 4 (0x10) set
				//   (they use pens 0x10-0x17)
				*/
				tbm = &(tmpbitmap->line[y][tbmX]) ;
				if (reverse_map[*tbm] <= 0x10)
					*tbm = Machine->pens[cc];
				if( (vdl & 1) == 0 )
				{
					tbm = &(tmpbitmap->line[y+1][tbmX]) ;
					if (reverse_map[*tbm] <= 0x10)
						*tbm = Machine->pens[cc];
				}

#elif LIB_ASPECTRATIO_342x256
				tbm = &(tmpbitmap->line[y][tbmX]) ;
				/* Draw the planet only over itself, or over black. */
				/* The planet uses pens 0x00-0x0f, black is 0x10 */
				if (reverse_map[*tbm] <= 0x10)
					*tbm = Machine->pens[cc];

				/* taking two out of three of the planet pixels.  skip over
				//   an x pixel every other tmpbitmap pixel */
				if( tbmX & 1 )
					x++ ;

#endif
				x++ ;
				tbmX++ ;

			} /* while */
		} /* scg */
	} /* vdl */

} /* lib_drawplanet */


/* -- eof -- */
