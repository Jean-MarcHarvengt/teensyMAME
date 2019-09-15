/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define MAPIDXTOCOL(idx) Machine->gfx[0]->colortable[idx * 2 + 1]

#define SCREEN_COL    4
#define OVERLAY1_COL  3
#define OVERLAY2_COL  2
#define OVERLAY3_COL  1

/* These four values are just guesses */
#define OVERLAY1_Y    20
#define OVERLAY2_Y    OVERLAY1_Y + 16
#define OVERLAY3_Y    OVERLAY2_Y + 16
#define OVERLAY_END_Y OVERLAY3_Y + 16

int Clown_X,Clown_Y,Clown_Z=0;

void circus_clown_x_w(int offset, int data)
{
	Clown_X = 240-data;
}

void circus_clown_y_w(int offset, int data)
{
	Clown_Y = 240-data;
}

/* This register controls the clown image currently displayed */
/* and also is used to enable the amplifier and trigger the   */
/* discrete circuitry that produces sound effects and music   */

void circus_clown_z_w(int offset, int data)
{
	Clown_Z = (data & 0x0f);

	/* Bits 4-6 enable/disable trigger different events */
	/* descriptions are based on Circus schematics      */

	switch ((data & 0x70) >> 4)
	{
		case 0 : /* All Off */
			DAC_data_w (0,0);
			break;

		case 1 : /* Music */
			DAC_data_w (0,0x7f);
			break;

		case 2 : /* Pop */
			break;

		case 3 : /* Normal Video */
			break;

		case 4 : /* Miss */
			break;

		case 5 : /* Invert Video */
			break;

		case 6 : /* Bounce */
			break;

		case 7 : /* Don't Know */
			break;
	};

	/* Bit 7 enables amplifier (1 = on) */

//	if(errorlog) fprintf(errorlog,"clown Z = %02x\n",data);
}

void DrawLine(int x1, int y1, int x2, int y2, int dotted)
{
	/* Draws horizontal and Vertical lines only! */
    int col = MAPIDXTOCOL(SCREEN_COL);

    int ex1,ex2,ey1,ey2;
    int count, skip;

    /* Allow flips & rotates */

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		ex1 = y1;
		ex2 = y2;
		ey1 = x1;
		ey2 = x2;
	}
    else
    {
		ex1 = x1;
		ex2 = x2;
		ey1 = y1;
		ey2 = y2;
	}

	if (Machine->orientation & ORIENTATION_FLIP_X)
    {
		count = 255 - ey1;
		ey1 = 255 - ey2;
		ey2 = count;
	}

	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		count = 255 - ex1;
		ex1 = 255 - ex2;
		ex2 = count;
	}

	/* Draw the Line */

	osd_mark_dirty (ex1,ey1,ex1,ey2,0);

	if (dotted > 0)
		skip = 2;
	else
		skip = 1;

	if (ex1==ex2)
	{
		for (count=ey2;count>=ey1;count -= skip)
		{
			Machine->scrbitmap->line[ex1][count] = tmpbitmap->line[ex1][count] = col;
		}
	}
	else
	{
		for (count=ex2;count>=ex1;count -= skip)
		{
			Machine->scrbitmap->line[count][ey1] = tmpbitmap->line[count][ey1] = col;
		}
	}
}

void RobotBox (int top, int left)
{
	int right,bottom;

	/* Box */

	right  = left + 24;
	bottom = top + 26;

	DrawLine(top,left,top,right,0);				/* Top */
	DrawLine(bottom,left,bottom,right,0);		/* Bottom */
	DrawLine(top,left,bottom,left,0);			/* Left */
	DrawLine(top,right,bottom,right,0);			/* Right */

	/* Score Grid */

	bottom = top + 10;
	DrawLine(bottom,left+8,bottom,right,0);     /* Horizontal Divide Line */
	DrawLine(top,left+8,bottom,left+8,0);
	DrawLine(top,left+16,bottom,left+16,0);
}


void circus_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs, x, y, col;
	int sx,sy;

	if (full_refresh)
	{
		memset (dirtybuffer, 1, videoram_size);
		/* copy the character mapped graphics */
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

	/* for every character in the Video RAM,        */
	/* check if it has been modified since          */
	/* last time and update it accordingly.         */

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			col=SCREEN_COL;

			dirtybuffer[offs] = 0;

			sy = offs / 32;
			sx = offs % 32;

			/* Sort out colour overlay */

			switch (sy)
			{
				case 2 :
				case 3 :
					col = OVERLAY1_COL;
					break;

				case 4 :
				case 5 :
					col = OVERLAY2_COL;
					break;

				case 6 :
				case 7 :
					col = OVERLAY3_COL;
					break;
			}

			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs],
					col,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

    /* The sync generator hardware is used to   */
    /* draw the border and diving boards        */

    DrawLine (18,0,18,255,0);
    DrawLine (249,0,249,255,1);
    DrawLine (18,0,248,0,0);
    DrawLine (18,247,248,247,0);

    DrawLine (137,0,137,17,0);
    DrawLine (137,231,137,248,0);
    DrawLine (193,0,193,17,0);
    DrawLine (193,231,193,248,0);

	/* Redraw portions that fall under the overlay */
	col = MAPIDXTOCOL(OVERLAY1_COL);

	for (y = OVERLAY1_Y; y < OVERLAY_END_Y; y++)
	{
		int y1 = y;
		int y2 = y;
		int x1 = 0;
		int x2 = 247;

		if (y == OVERLAY2_Y)
			col = MAPIDXTOCOL(OVERLAY2_COL);

		if (y == OVERLAY3_Y)
			col = MAPIDXTOCOL(OVERLAY3_COL);

    	/* Allow flips & rotates */
		if (Machine->orientation & ORIENTATION_SWAP_XY)
		{
			int temp;

			temp = x1;
			x1 = y1;
			y1 = temp;
			temp = x2;
			x2 = y2;
			y2 = temp;
		}

		if (Machine->orientation & ORIENTATION_FLIP_X)
		{
			x1 = 255 - x1;
			x2 = 255 - x2;
		}

		if (Machine->orientation & ORIENTATION_FLIP_Y)
		{
			y1 = 255 - y1;
			y2 = 255 - y2;
		}

		osd_mark_dirty (x1,y1,x2,y2,0);
		bitmap->line[y1][x1] = tmpbitmap->line[y1][x1] = col;
		bitmap->line[y2][x2] = tmpbitmap->line[y2][x2] = col;
	}

    /* Draw the clown in white and afterwards compensate for the overlay */
	drawgfx(bitmap,Machine->gfx[1],
			Clown_Z,
			SCREEN_COL,
			0,0,
			Clown_Y,Clown_X,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

	/* mark tiles underneath as dirty */
	sx = Clown_Y >> 3;
	sy = Clown_X >> 3;

	{
		int max_x = 2;
		int max_y = 2;
		int x2, y2;

		if (Clown_Y & 0x0f) max_x ++;
		if (Clown_X & 0x0f) max_y ++;

		for (y2 = sy; y2 < sy + max_y; y2 ++)
		{
			for (x2 = sx; x2 < sx + max_x; x2 ++)
			{
				if ((x2 < 32) && (y2 < 32) && (x2 >= 0) && (y2 >= 0))
					dirtybuffer[x2 + 32*y2] = 1;
			}
		}
	}

    /* ZV980110 */
	/* The variable names Clown_X and Clown_Y are backwards! */

	for (y = Clown_X; y < Clown_X + 16; y++)
	{
		int currentcolidx, currentcol;
		int basecol = MAPIDXTOCOL(SCREEN_COL);

		if (y >= OVERLAY_END_Y) break;
		else if (y >= OVERLAY3_Y) currentcolidx = OVERLAY3_COL;
		else if (y >= OVERLAY2_Y) currentcolidx = OVERLAY2_COL;
		else if (y >= OVERLAY1_Y) currentcolidx = OVERLAY1_COL;
		else continue;

		currentcol = MAPIDXTOCOL(currentcolidx);

		for (x = Clown_Y; x < Clown_Y + 16; x++ )
		{
			int x2, y2;

			y2 = y; x2 = x;

    		/* Allow flips & rotates */
			if (Machine->orientation & ORIENTATION_SWAP_XY)
			{
				y2 = x; x2 = y;
			}
			if (Machine->orientation & ORIENTATION_FLIP_X)
				x2 = 255 - x2;
			if (Machine->orientation & ORIENTATION_FLIP_Y)
				y2 = 255 - y2;

			/* Remap to overlay color */
			if (bitmap->line[y2][x2] == basecol)
			{
				bitmap->line[y2][x2] = tmpbitmap->line[y2][x2] = currentcol;
			}
		}
	}
}


void robotbowl_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int sx,sy;

	if (full_refresh)
	{
		memset (dirtybuffer, 1, videoram_size);
		/* copy the character mapped graphics */
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

	/* for every character in the Video RAM,  */
	/* check if it has been modified since    */
	/* last time and update it accordingly.   */

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int col=SCREEN_COL;

			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs],
					col,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

    /* The sync generator hardware is used to   */
    /* draw the bowling alley & scorecards      */

    /* Scoreboards */

    for(offs=15;offs<=63;offs+=24)
    {
        RobotBox(31, offs);
        RobotBox(63, offs);
        RobotBox(95, offs);

        RobotBox(31, offs+152);
        RobotBox(63, offs+152);
        RobotBox(95, offs+152);
    }

    RobotBox(127, 39);                  /* 10th Frame */
    DrawLine(137,39,137,47,0);          /* Extra digit box */

    RobotBox(127, 39+152);
    DrawLine(137,39+152,137,47+152,0);

    /* Bowling Alley */

    DrawLine(17,103,205,103,0);
    DrawLine(17,111,203,111,1);
    DrawLine(17,152,205,152,0);
    DrawLine(17,144,203,144,1);

	/* Draw the Ball */

	drawgfx(bitmap,Machine->gfx[1],
			Clown_Z,
			SCREEN_COL,
			0,0,
			Clown_Y+8,Clown_X+8, /* Y is horizontal position */
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

	/* mark tiles underneath as dirty */
	sx = Clown_Y >> 3;
	sy = Clown_X >> 3;

	{
		int max_x = 2;
		int max_y = 2;
		int x2, y2;

		if (Clown_Y & 0x0f) max_x ++;
		if (Clown_X & 0x0f) max_y ++;

		for (y2 = sy; y2 < sy + max_y; y2 ++)
		{
			for (x2 = sx; x2 < sx + max_x; x2 ++)
			{
				if ((x2 < 32) && (y2 < 32) && (x2 >= 0) && (y2 >= 0))
					dirtybuffer[x2 + 32*y2] = 1;
			}
		}
	}

}

void crash_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int sx,sy;

	if (full_refresh)
	{
		memset (dirtybuffer, 1, videoram_size);
		/* copy the character mapped graphics */
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

	/* for every character in the Video RAM,	*/
	/* check if it has been modified since 		*/
	/* last time and update it accordingly. 	*/

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
            int col=4;

			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs],
					col,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* Draw the Car */
    drawgfx(bitmap,Machine->gfx[1],
			Clown_Z,
			4,
			0,0,
			Clown_Y,Clown_X, /* Y is horizontal position */
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

	/* mark tiles underneath as dirty */
	sx = Clown_Y >> 3;
	sy = Clown_X >> 3;

	{
		int max_x = 2;
		int max_y = 2;
		int x2, y2;

		if (Clown_Y & 0x0f) max_x ++;
		if (Clown_X & 0x0f) max_y ++;

		for (y2 = sy; y2 < sy + max_y; y2 ++)
		{
			for (x2 = sx; x2 < sx + max_x; x2 ++)
			{
				if ((x2 < 32) && (y2 < 32) && (x2 >= 0) && (y2 >= 0))
					dirtybuffer[x2 + 32*y2] = 1;
			}
		}
	}
}
