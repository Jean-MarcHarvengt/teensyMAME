#ifndef DRIVER_H
#define DRIVER_H


#include "Z80.h"
#include "common.h"
#include "machine.h"
#define MAX_DIP_SWITCHES 2

struct MemoryReadAddress
{
	int start,end;
	int (*handler)(int address,int offset);
};



struct MemoryWriteAddress
{
	int start,end;
	void (*handler)(int address,int offset,int data);
};



struct GfxDecodeInfo
{
	int start;	/* beginning of data data to decode (offset in RAM[]) */
	struct GfxLayout *gfxlayout;
	int first_color_code;	/* first and last color codes used by this */
	int last_color_code;	/* gfx elements */
};



struct MachineDriver
{
	const char *name;
	const struct RomModule *rom;

	/* basic machine hardware */
	int cpu_clock;
	int frames_per_second;
	const struct MemoryReadAddress *memory_read;
	const struct MemoryWriteAddress *memory_write;
	const struct DSW *dswsettings;
	int defaultdsw[MAX_DIP_SWITCHES];	/* default dipswitch settings */

	int (*init_machine)(const char *gamename);
	int (*interrupt)(void);
	void (*out)(byte Port,byte Value);

	/* video hardware */
	int screen_width,screen_height;
	struct GfxDecodeInfo *gfxdecodeinfo;
	const unsigned char *palette;
	int total_colors;	/* palette is 3*total_colors bytes long */
	const unsigned char *colortable;
	int color_codes;	/* colortable has color_codes tuples - the length */
						/* of each tuple depends on the graphic data, for example */
						/* 2-bitplane characters use 4 bytes in each tuple. */

	int	numbers_start;	/* start of numbers and letters in the character roms */
	int letters_start;	/* (used by displaytext() ) */
	int white_text,yellow_text;	/* used by the dip switch menu */
	int paused_x,paused_y,paused_color;	/* used to print PAUSED on the screen */

	int (*vh_init)(const char *gamename);
	int (*vh_start)(void);
	void (*vh_stop)(void);
	void (*vh_screenrefresh)(void);

	/* sound hardware */
	unsigned char *samples;
	int (*sh_init)(const char *gamename);
	int (*sh_start)(void);
	void (*sh_stop)(void);
	int (*sh_out)(byte A,byte V);
	int (*sh_in)(byte A);
	void (*sh_update)(void);
};


extern struct MachineDriver *drivers[];


#endif
