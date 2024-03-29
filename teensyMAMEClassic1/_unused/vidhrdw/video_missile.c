/***************************************************************************

  vmissile.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *missile_videoram;
int missile_flipscreen;


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int missile_vh_start(void)
{

	/* force video ram to be $0000-$FFFF even though only $1900-$FFFF is used */
	if ((missile_videoram = malloc (256 * 256)) == 0)
		return 1;

	memset (missile_videoram, 0, 256 * 256);
	missile_flipscreen = 0x40;
	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void missile_vh_stop(void)
{
	free (missile_videoram);
}

/********************************************************************************************/
int missile_video_r (int address)
{
	return (missile_videoram[address] & 0xe0);
}

/********************************************************************************************/
/* This routine is called when the flipscreen bit changes. It redraws the entire bitmap. */
void missile_flip_screen (void)
{
	int x, y;
	int temp;

	for (y = 0; y < 115; y ++)
	{
		for (x = 0; x < 256; x ++)
		{
			temp = Machine->scrbitmap->line[y][x];
			Machine->scrbitmap->line[y][x] = Machine->scrbitmap->line[230-y][255-x];
			Machine->scrbitmap->line[230-y][255-x] = temp;
		}
	}
	/* flip the middle line */
	for (x = 0; x < 128; x ++)
	{
		temp = Machine->scrbitmap->line[115][x];
		Machine->scrbitmap->line[115][x] = Machine->scrbitmap->line[115][255-x];
		Machine->scrbitmap->line[115][255-x] = temp;
	}
	osd_mark_dirty (0,0,255,230,0);
}

/********************************************************************************************/
void missile_blit_w (int offset)
{
	int x, y;
	int bottom;
	int color;

	/* The top 25 lines ($0000 -> $18ff) aren't used or drawn */
	y = (offset >> 8) - 25;
	x = offset & 0xff;
	if( y < 231 - 32)
		bottom = 1;
	else
		bottom = 0;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int tmp;

		tmp = x;
		x = y;
		y = tmp;
	}

	if (Machine->orientation & ORIENTATION_FLIP_X)
		x = Machine->scrbitmap->width - 1 - x;
	if (Machine->orientation & ORIENTATION_FLIP_Y)
		y = Machine->scrbitmap->height - 1 - y;

	/* cocktail mode */
	if (missile_flipscreen)
	{
		x = Machine->scrbitmap->width - 1 - x;
		y = Machine->scrbitmap->height - 1 - y;
	}

	color = (missile_videoram[offset] >> 5);

	if (bottom) color &= 0x06;

	Machine->scrbitmap->line[y][x] = Machine->pens[color];
	osd_mark_dirty (x,y,x,y,0);
}

/********************************************************************************************/
void missile_video_w (int address,int data)
{
	/* $0640 - $4fff */
	int wbyte, wbit;
	extern unsigned char *RAM;

	if (address < 0xf800)
	{
		missile_videoram[address] = data;
		missile_blit_w (address);
	}
	else
	{
		missile_videoram[address] = (missile_videoram[address] & 0x20) | data;
		missile_blit_w (address);
		wbyte = ((address - 0xf800) >> 2) & 0xfffe;
		wbit = (address - 0xf800) % 8;
		if(data & 0x20)
			RAM[0x401 + wbyte] |= (1 << wbit);
		else
			RAM[0x401 + wbyte] &= ((1 << wbit) ^ 0xff);
	}
}

void missile_video2_w (int offset, int data)
{
	/* $5000 - $ffff */
	offset += 0x5000;
	missile_video_w (offset, data);
}

/********************************************************************************************/
void missile_video_mult_w (int address, int data)
{
	/*
		$1900 - $3fff

		2-bit color writes in 4-byte blocks.
		The 2 color bits are in x000x000.

		Note that the address range is smaller because 1 byte covers 4 screen pixels.
	*/

	data = (data & 0x80) + ((data & 8) << 3);
	address = address << 2;

	/* If this is the bottom 8 lines of the screen, set the 3rd color bit */
	if (address >= 0xf800) data |= 0x20;

	missile_videoram[address]     = data;
	missile_videoram[address + 1] = data;
	missile_videoram[address + 2] = data;
	missile_videoram[address + 3] = data;

	missile_blit_w (address);
	missile_blit_w (address + 1);
	missile_blit_w (address + 2);
	missile_blit_w (address + 3);
}


/********************************************************************************************/
void missile_video_3rd_bit_w(int address, int data)
{
	int i;
	extern unsigned char *RAM;


	address += 0x400;
	/* This is needed to make the scrolling text work properly */
	RAM[address] = data;

	address = ((address - 0x401) << 2) + 0xf800;
	for (i=0; i<8; i++)
	{
		if (data & (1 << i))
			missile_videoram[address + i] |= 0x20;
		else
			missile_videoram[address + i] &= 0xc0;
		missile_blit_w (address + i);
	}
}


/********************************************************************************************/
void missile_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int address;

	if (full_refresh)
	{
		for (address = 0x1900; address <= 0xffff; address++)
			missile_blit_w (address);
	}
}
