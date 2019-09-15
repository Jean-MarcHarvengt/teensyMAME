/*************************************************************************
     Turbo - Sega - 1981

     Machine Hardware

*************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int turbo_f8pa, turbo_f8pb, turbo_f8pc;
int turbo_f9pa, turbo_f9pb, turbo_f9pc;
int turbo_fbpla, turbo_fbcol;
unsigned char turbo_seg_digit[32];
static unsigned int turbo_seg_add, turbo_seg_auto_inc;

void turbo_unpack_sprites(int region, int size);
extern unsigned int turbo_collision;
extern unsigned char 	*turbo_SpritesCollisionTable;

void turbo_init_machine(void)
{
//	system8_define_checkspriteram(NULL);
//	system8_define_banksupport(SYSTEM8_SUPPORTS_SPRITEBANKS);
//	system8_define_cliparea(16,239,0,223);
	turbo_unpack_sprites(3, 0x20000);
//	system8_define_sprite_offset_y(16);
}

int turbo_interrupt(void)   // this is just temporary
{
	return 1;
}

int turbo_fa00_r(int offset)
{
	if (errorlog) fprintf(errorlog, "read 0xfa%02x\n", offset);
	switch (offset%4)
	{
		case 0x00:
			return 0xff;
		case 0x01:
			return 0xc0;
		case 0x02:
			return 0xaf;
		case 0x03:
			return 0xef;
		default:
			return 0;
	}
}

int turbo_fb00_r(int offset)
{
	switch (offset%4)
	{
		case 0x00:
			return readinputport(4);  // WHEEL
		case 0x01:
			return readinputport(2);  // DSW 2
		case 0x02:
	if (errorlog) fprintf(errorlog, "read 0xfb%02x\n", offset);
			return 0x80;
		case 0x03:
	if (errorlog) fprintf(errorlog, "read 0xfb%02x\n", offset);
			return 0x80;
		default:
			return 0;
	}
}

int turbo_fc00_r(int offset)
{
	switch (offset%2)
	{
		case 0x00:
			return readinputport(1);  // DSW 1;
		case 0x01:
	if (errorlog) fprintf(errorlog, "read 0xfc%02x\n", offset);
			return 0x10;
		default:
			return 0;
	}
}

int turbo_fd00_r(int offset)
{
	return readinputport(0); // coins, etc.
}

int turbo_fe00_r(int offset)
{
	if (errorlog) fprintf(errorlog, "read 0xfe%02x\n", offset);
	return readinputport(3)|turbo_collision; // DSW 3 + 4 bits
}

void turbo_b800_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog, "write b800: %x\n",data);
//	memset(spriteram,0,spriteram_size);
//	memset(turbo_SpritesCollisionTable,0,0x10000);
	turbo_collision=0;
}

void turbo_f800_w(int offset, int data)
{
	switch (offset%4)
	{
		case 0x00:
			turbo_f8pa = data;
			break;
		case 0x01:
			turbo_f8pb = data;
			break;
		case 0x02:
			turbo_f8pc = data;
			break;
		case 0x03:
			if (data!=0x80)
			{
				if (errorlog) fprintf(errorlog, "8255: Unsupported mode\n");
			}
			break;
	}
}

void turbo_f900_w(int offset, int data)
{
	switch (offset%4)
	{
		case 0x00:
			turbo_f9pa = data;
			break;
		case 0x01:
			turbo_f9pb = data;
			break;
		case 0x02:
			turbo_f9pc = data;
			break;
		case 0x03:
			if (data!=0x80)
			{
				if (errorlog) fprintf(errorlog, "8255: Unsupported mode\n");
			}
			break;
	}
}

void turbo_fa00_w(int offset, int data)
{
//	if (errorlog) fprintf(errorlog, "write 0xfa%02x with %02x\n", offset, data);
	switch (offset%4)
	{
		case 0x00:
			if ((data&0x02)==0) /* Trig1 */
			{
				sample_start (0,0,0);
			}
			if ((data&0x04)==0) /* Trig2 */
			{
				sample_start (0,1,0);
			}
			if ((data&0x08)==0) /* Trig3 */
			{
				sample_start (0,2,0);
			}
			if ((data&0x10)==0) /* Trig4 */
			{
				sample_start (0,3,0);
			}
			break;
		case 0x01:
			if ((data&0x40)==0) /* Ambu */
			{
				if (!sample_playing(1)) sample_start (1,4,1);
				else if (errorlog) fprintf(errorlog, "ambu didnt start\n");
			}
			else
			{
				sample_stop (1);
			}
			break;
		case 0x02:
//			turbo_f9pc = data;
			break;
		case 0x03:
			if (data!=0x80)
			{
				if (errorlog) fprintf(errorlog, "8255: Unsupported mode\n");
			}
			break;
	}
}

void turbo_fb00_w(int offset, int data)
{
	switch (offset%4)
	{
		case 0x02:
			turbo_fbpla = data&0x0f;
			if (turbo_fbcol != ((data&0x70)>>4))
			{
				turbo_fbcol = (data&0x70)>>4;
				memset(dirtybuffer,1,videoram_size);
			}
			break;
		case 0x03:
			{
//				if (errorlog) fprintf(errorlog, "fb03: %x\n",data);
			}
			break;
	}
}

void turbo_fc00_w(int offset, int data) /* 8279 */
{
	switch (offset%2)
	{
		case 0x00:
			turbo_seg_digit[turbo_seg_add*2] = (data&0x0f);
			turbo_seg_digit[turbo_seg_add*2+1] = ((data&0xf0)>>4);
			turbo_seg_add+=turbo_seg_auto_inc;
			if (turbo_seg_add==16) turbo_seg_add=0;
			break;
		case 0x01:
			switch (data&0xe0)
			{
				case 0x80:
					turbo_seg_add = data&0x0f;
					turbo_seg_auto_inc = 0;
					break;
				case 0x90:
					turbo_seg_add = data&0x0f;
					turbo_seg_auto_inc = 1;
					break;
				case 0xc0:
					memset(turbo_seg_digit,0,32);
					break;
			}
			break;
	}
}
