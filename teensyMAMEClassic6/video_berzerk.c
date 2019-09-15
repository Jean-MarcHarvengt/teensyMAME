/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *magicram;
extern int berzerk_irq_end_of_screen;

static int magicram_control = 0xff;
static int magicram_latch = 0xff;
static int collision = 0;


void berzerk_videoram_w(int offset,int data)
{
  int coloroffset, pen1, pen2, black, x, y;

  videoram[offset] = data;

  if (offset < 0x400) return;

  /* Get location of color RAM for this offset */
  coloroffset = ((offset & 0xff80) >> 2) | (offset & 0x1f);

  pen1  = Machine->pens[(colorram[coloroffset] >> 4) & 0x0f];
  pen2  = Machine->pens[ colorram[coloroffset] & 0x0f];
  black = Machine->pens[ 0 ];

  y = ((offset >> 5) & 0xff) - 16;
  x = (offset & 0x1f) << 3;

  if (y < 0)
     return;

	osd_mark_dirty(x,y,x+8,y,0);	/* ASG 971015 */

  /* CMK optimize the most common case--all blank */
  if (data == 0)
  {
     memset(&tmpbitmap->line[y][x],black,8);
     memset(&Machine->scrbitmap->line[y][x],black,8);
     return;
  }

	if (data & 0x80) Machine->scrbitmap->line[y][x    ] = tmpbitmap->line[y][x    ] = pen1; else Machine->scrbitmap->line[y][x    ] = tmpbitmap->line[y][x    ] = black;
	if (data & 0x40) Machine->scrbitmap->line[y][x + 1] = tmpbitmap->line[y][x + 1] = pen1; else Machine->scrbitmap->line[y][x + 1] = tmpbitmap->line[y][x + 1] = black;
	if (data & 0x20) Machine->scrbitmap->line[y][x + 2] = tmpbitmap->line[y][x + 2] = pen1; else Machine->scrbitmap->line[y][x + 2] = tmpbitmap->line[y][x + 2] = black;
	if (data & 0x10) Machine->scrbitmap->line[y][x + 3] = tmpbitmap->line[y][x + 3] = pen1; else Machine->scrbitmap->line[y][x + 3] = tmpbitmap->line[y][x + 3] = black;
	if (data & 0x08) Machine->scrbitmap->line[y][x + 4] = tmpbitmap->line[y][x + 4] = pen2; else Machine->scrbitmap->line[y][x + 4] = tmpbitmap->line[y][x + 4] = black;
	if (data & 0x04) Machine->scrbitmap->line[y][x + 5] = tmpbitmap->line[y][x + 5] = pen2; else Machine->scrbitmap->line[y][x + 5] = tmpbitmap->line[y][x + 5] = black;
	if (data & 0x02) Machine->scrbitmap->line[y][x + 6] = tmpbitmap->line[y][x + 6] = pen2; else Machine->scrbitmap->line[y][x + 6] = tmpbitmap->line[y][x + 6] = black;
	if (data & 0x01) Machine->scrbitmap->line[y][x + 7] = tmpbitmap->line[y][x + 7] = pen2; else Machine->scrbitmap->line[y][x + 7] = tmpbitmap->line[y][x + 7] = black;
}


void berzerk_colorram_w(int offset,int data)
{
  int black, pen1, pen2, x, y, i;

  colorram[offset] = data;

  /* Need to change the affected pixels' colors */

  pen1  = Machine->pens[(data >> 4) & 0x0f];
  pen2  = Machine->pens[ data & 0x0f];
  black = Machine->pens[ 0 ];

  y = ((offset >> 3) & 0xfc) - 16;
  x = (offset & 0x1f) << 3;

  if (y < 0)
     return;

	osd_mark_dirty(x,y,x+7,y+3,0);	/* ASG 971015 */

  for (i = 0; i < 4; i++, y++)
  {
    int dat = videoram[((y + 16) << 5) | (x >> 3)];

	if (dat & 0x80) Machine->scrbitmap->line[y][x    ] = tmpbitmap->line[y][x    ] = pen1; else Machine->scrbitmap->line[y][x    ] = tmpbitmap->line[y][x    ] = black;
	if (dat & 0x40) Machine->scrbitmap->line[y][x + 1] = tmpbitmap->line[y][x + 1] = pen1; else Machine->scrbitmap->line[y][x + 1] = tmpbitmap->line[y][x + 1] = black;
	if (dat & 0x20) Machine->scrbitmap->line[y][x + 2] = tmpbitmap->line[y][x + 2] = pen1; else Machine->scrbitmap->line[y][x + 2] = tmpbitmap->line[y][x + 2] = black;
	if (dat & 0x10) Machine->scrbitmap->line[y][x + 3] = tmpbitmap->line[y][x + 3] = pen1; else Machine->scrbitmap->line[y][x + 3] = tmpbitmap->line[y][x + 3] = black;
	if (dat & 0x08) Machine->scrbitmap->line[y][x + 4] = tmpbitmap->line[y][x + 4] = pen2; else Machine->scrbitmap->line[y][x + 4] = tmpbitmap->line[y][x + 4] = black;
	if (dat & 0x04) Machine->scrbitmap->line[y][x + 5] = tmpbitmap->line[y][x + 5] = pen2; else Machine->scrbitmap->line[y][x + 5] = tmpbitmap->line[y][x + 5] = black;
	if (dat & 0x02) Machine->scrbitmap->line[y][x + 6] = tmpbitmap->line[y][x + 6] = pen2; else Machine->scrbitmap->line[y][x + 6] = tmpbitmap->line[y][x + 6] = black;
	if (dat & 0x01) Machine->scrbitmap->line[y][x + 7] = tmpbitmap->line[y][x + 7] = pen2; else Machine->scrbitmap->line[y][x + 7] = tmpbitmap->line[y][x + 7] = black;
  }
}

unsigned char berzerk_reverse_byte(unsigned char data)
{
   return ((data & 0x01) << 7) |
      ((data & 0x02) << 5) |
      ((data & 0x04) << 3) |
      ((data & 0x08) << 1) |
      ((data & 0x10) >> 1) |
      ((data & 0x20) >> 3) |
      ((data & 0x40) >> 5) |
      ((data & 0x80) >> 7);

}

int berzerk_shifter_flopper(unsigned char data)
{
   int shift_command,flop_command,outval=0;

   shift_command = (magicram_control >> 1) & 0x03;

   switch (shift_command)
   {
   case 0 :
      outval = (data | (magicram_latch << 8)) & 0x1ff;
      break;
   case 1 :
      outval = ((data >> 2) | (magicram_latch << 6)) & 0x1ff;
      break;
   case 2 :
      outval = ((data >> 4) | (magicram_latch << 4)) & 0x1ff;
      break;
   case 3 :
      outval = ((data >> 6) | (magicram_latch << 2)) & 0x1ff;
      break;
   }

   flop_command = ((magicram_control >> 2) & 0x02) | (magicram_control & 0x01);

   switch (flop_command)
   {
   case 0 :
      break;
   case 1 :
      outval >>= 1;
      break;
   case 2 :
      outval = berzerk_reverse_byte(outval & 0xff);
      break;
   case 3 :
      outval = berzerk_reverse_byte((outval >> 1) & 0xff);
      break;
   }

   return outval;
}

void berzerk_magicram_w(int offset,int data)
{
   int data2;

   data2 = berzerk_shifter_flopper(data);

   magicram_latch = data;

   /* Check for collision */
   if (!collision)
      collision = (data2 & videoram[offset]);

  switch (magicram_control & 0xf0)
  {
  case 0x00:
    magicram[offset] = data2;
    break;
  case 0x10:
    magicram[offset] = data2 | videoram[offset];
    break;
  case 0x20:
    magicram[offset] = data2 | ~videoram[offset];
    break;
  case 0x30:
    magicram[offset] = 0xff;
    break;
  case 0x40:
    magicram[offset] = data2 & videoram[offset];
    break;
  case 0x50:
    magicram[offset] = videoram[offset];
    break;
  case 0x60:
    magicram[offset] = ~(data2 ^ videoram[offset]);
    break;
  case 0x70:
    magicram[offset] = ~data2 | videoram[offset];
    break;
  case 0x80:
    magicram[offset] = data2 & ~videoram[offset];
    break;
  case 0x90:
    magicram[offset] = data2 ^ videoram[offset];
    break;
  case 0xa0:
    magicram[offset] = ~videoram[offset];
    break;
  case 0xb0:
    magicram[offset] = ~(data2 & videoram[offset]);
    break;
  case 0xc0:
    magicram[offset] = 0x00;
    break;
  case 0xd0:
    magicram[offset] = ~data2 & videoram[offset];
    break;
  case 0xe0:
    magicram[offset] = ~(data2 | videoram[offset]);
    break;
  case 0xf0:
    magicram[offset] = ~data2;
    break;
  }

  berzerk_videoram_w(offset,magicram[offset]);
}


int berzerk_magicram_r(int offset)
{
   return magicram[offset];
}

void berzerk_magicram_control_w(int offset,int data)
{
  magicram_control = data;
  magicram_latch = 0;
  collision = 0;
}


int berzerk_collision_r(int offset)
{
  int ret = (collision ? 0x80 : 0x00);

  return ret | berzerk_irq_end_of_screen;
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void berzerk_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	if (full_refresh)
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
