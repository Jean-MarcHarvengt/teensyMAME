/*************************************************************************

 Driver for Williams/Midway games using the TMS34010 processor

**************************************************************************/
#include "driver.h"
#include "osd_cpu.h"
#include "TMS34010/tms34010.h"
#include "M6809/M6809.h"
#include "6821pia.h"
#include "vidhrdw/generic.h"

#if LSB_FIRST
	#define BYTE_XOR_LE(a) (a)
	#define BIG_DWORD_LE(x) (x)
#else
	#define BYTE_XOR_LE(a) ( (char*) ((unsigned int) (a) ^ 1) )
	#define BIG_DWORD_LE(x) (((UINT32)(x) >> 16) + ((x) << 16))
#endif


#define CODE_ROM     cpu_bankbase[1]
#define GFX_ROM	     cpu_bankbase[8]
#define SCRATCH_RAM	 cpu_bankbase[2]

void narc_sound_w (int offset,int data);
void mk_sound_w (int offset,int data);
void smashtv_sound_w (int offset,int data);
void trog_sound_w (int offset,int data);

void wms_vram_w(int offset, int data);
void wms_objpalram_w(int offset, int data);
int wms_vram_r(int offset);
int wms_objpalram_r(int offset);

extern unsigned short *wms_videoram;

unsigned char *wms_cmos_ram;
int wms_bank2_size;
extern            int wms_videoram_size;
                  int wms_code_rom_size;
                  int wms_gfx_rom_size;
extern unsigned   int wms_rom_loaded;
static unsigned   int wms_cmos_page = 0;
                  int wms_objpalram_select = 0;
       unsigned   int wms_autoerase_enable = 0;
       unsigned   int wms_autoerase_start = 1000;

static unsigned int wms_dma_rows=0;
static          int wms_dma_write_rows=0;
static unsigned int wms_dma_cols=0;
static unsigned int wms_dma_bank=0;
static unsigned int wms_dma_subbank=0;
static unsigned int wms_dma_x=0;
static unsigned int wms_dma_y=0;
       unsigned int wms_dma_pal=0;
       unsigned int wms_dma_pal_word=0;
static unsigned int wms_dma_dst=0;
static unsigned int wms_dma_stat=0;
static unsigned int wms_dma_fgcol=0;
static        short wms_dma_woffset=0;

static unsigned int wms_dma_odd_nibble=0;  /* are we not on a byte boundary? */

static unsigned int wms_dma_14=0;
static unsigned int wms_dma_16=0;
static unsigned int wms_dma_18=0;
static unsigned int wms_dma_1a=0;
static unsigned int wms_dma_1c=0;
static unsigned int wms_dma_1e=0;

static unsigned int smashtv_cmos_w_enable=1;

static unsigned int wms_protect_s=0xffffffff; /* never gets here */
static unsigned int wms_protect_d=0xffffffff;

static int narc_input_r (int offset)
{
	int ans = 0xffff;
	switch (offset)
	{
		case 0:
			ans = (input_port_1_r (offset) << 8) + (input_port_0_r (offset));
			break;
		case 2:
			ans = (input_port_3_r (offset) << 8) + (input_port_2_r (offset));
			break;
		case 4:
			ans = (input_port_5_r (offset) << 8) + (soundlatch3_r (0));
			break;
//		case 6:
//			ans = (input_port_7_r (offset) << 8) + (input_port_6_r (offset));
//			break;
//		case 8:
//			ans = (input_port_9_r (offset) << 8) + (input_port_8_r (offset));
//			break;
		default:
			if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: warning - read from unmapped bit address %x\n", cpu_getpc(), (offset<<3));
	}
	return ans;
}
int wms_input_r (int offset)
{
	int ans = 0xffff;
	switch (offset)
	{
		case 0:
			ans = (input_port_1_r (offset) << 8) + (input_port_0_r (offset));
			break;
		case 2:
			ans = (input_port_3_r (offset) << 8) + (input_port_2_r (offset));
			break;
		case 4:
			ans = (input_port_5_r (offset) << 8) + (input_port_4_r (offset));
			break;
		case 6:
			ans = (input_port_7_r (offset) << 8) + (input_port_6_r (offset));
			break;
		case 8:
			ans = (input_port_9_r (offset) << 8) + (input_port_8_r (offset));
			break;
		default:
			if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: warning - read from unmapped bit address %x\n", cpu_getpc(), (offset<<3));
	}
	return ans;
}

static int term2_input_lo_r (int offset)
{
	int ans = 0xffff;
	switch (offset)
	{
//		case 0:
//			ans = (input_port_5_r (offset) << 8) + (input_port_4_r (offset));
//			break;
//		case 2:
//			ans = (input_port_7_r (offset) << 8) + (input_port_6_r (offset));
//			break;
//		case 4:
//			ans = (input_port_9_r (offset) << 8) + (input_port_8_r (offset));
//			break;
		default:
			if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: warning - read from unmapped bit address %x\n", cpu_getpc(), (offset<<3));
	}
	return ans;
}

static void dma_callback(int param)
{
	wms_dma_stat = 0; /* tell the cpu we're done */
	cpu_cause_interrupt(0,TMS34010_INT1);
}

int wms_dma_r(int offset)
{
	if (wms_dma_stat&0x8000)
	{
		switch (cpu_getpc())
		{
		case 0xfff7aa20: /* narc */
		case 0xffe1c970: /* trog */
		case 0xffe1c9a0: /* trog3 */
		case 0xffe1d4a0: /* trogp */
		case 0xffe07690: /* smashtv */
		case 0xffe00450: /* hiimpact */
		case 0xffe14930: /* strkforc */
		case 0xffe02c20: /* strkforc */
		case 0xffc79890: /* mk */
		case 0xffc7a5a0: /* mk */
		case 0xffc063b0: /* term2 */
		case 0xffc00720: /* term2 */
		case 0xffc07a60: /* totcarn/totcarnp */
		case 0xff805200: /* mk2 */
		case 0xff8044e0: /* mk2 */
		case 0xff82e200: /* nbajam */
			cpu_spinuntil_int();
			wms_dma_stat=0;
			break;

		default:
			if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: read hi dma\n", cpu_getpc());
		}
	}
	return wms_dma_stat;
}


/*****************************************************************************************
 *																						 *
 *			   They may not be optimal for other compilers. If you change anything, make *
 *             sure you test it on a Pentium II. Even a trivial change can make a huge   *
 *             difference!																 *
 *																						 *
 *****************************************************************************************/

#define DMA_DRAW_NONZERO_BYTES(data,advance)        	   \
	if (write_cols >= 0)								   \
	{												       \
		pal = wms_dma_pal;								   \
		line_skip = 511 - write_cols; 			           \
		wrva = &(wms_videoram[wms_dma_dst]);               \
		for (i=wms_dma_write_rows;i;i--)				   \
		{                                                  \
			j=write_cols;						           \
			do											   \
			{                                              \
				if ((write_data = *BYTE_XOR_LE(rda)))	   \
				{                                          \
					*wrva = pal | (data); 		   		   \
				}                                          \
				rda##advance;							   \
				wrva++;                                    \
			}											   \
			while (j--); 								   \
			rda+=dma_skip;                             	   \
			wrva+=line_skip;                               \
		}												   \
	}

#define DMA_DRAW_ALL_BYTES(data)                       	   \
	if (write_cols >= 0)								   \
	{												   	   \
		pal = wms_dma_pal;								   \
		line_skip = 511 - write_cols;                      \
		wrva = &(wms_videoram[wms_dma_dst]);               \
		for (i=wms_dma_write_rows;i;i--)				   \
		{                                                  \
			j=write_cols;						           \
			do											   \
			{                                              \
				*(wrva++) = pal | (data); 		           \
			}											   \
			while (j--);  								   \
			rda+=dma_skip;                                 \
			wrva+=line_skip;                               \
		}												   \
	}

void wms_dma_w(int offset, int data)
{
	unsigned int i, j, pal, write_data, line_skip, dma_skip;
	int write_cols;
	unsigned char *rda;
	unsigned short *wrva;

	switch (offset)
	{
		case 0:
			write_cols = (wms_dma_cols+wms_dma_x<512?wms_dma_cols:512-wms_dma_x)-1;  /* Note the -1 */
			wms_dma_write_rows = (wms_dma_rows+wms_dma_y<512?wms_dma_rows:512-wms_dma_y);
			wms_dma_dst = ((wms_dma_y<<9) + (wms_dma_x)); /* byte addr */
			rda = &(GFX_ROM[wms_dma_bank+wms_dma_subbank]);
			wms_dma_stat = data;

			/*
			 * DMA registers
			 * ------------------
			 *
			 *  Register | Bit              | Use
			 * ----------+-FEDCBA9876543210-+------------
			 *     0     | x--------------- | trigger write (or clear if zero)
			 *           | ---184-1-------- | unknown
			 *           | ----------x----- | flip y
			 *           | -----------x---- | flip x
			 *           | ------------x--- | blit nonzero pixels as color
			 *           | -------------x-- | blit zero pixels as color
			 *           | --------------x- | blit nonzero pixels
			 *           | ---------------x | blit zero pixels
			 *     1     | xxxxxxxxxxxxxxxx | width offset
			 *     2     | xxxxxxxxxxxxxxxx | source address low word
			 *     3     | xxxxxxxxxxxxxxxx | source address high word
			 *     4     | xxxxxxxxxxxxxxxx | detination x
			 *     5     | xxxxxxxxxxxxxxxx | destination y
			 *     6     | xxxxxxxxxxxxxxxx | image columns
			 *     7     | xxxxxxxxxxxxxxxx | image rows
			 *     8     | xxxxxxxxxxxxxxxx | palette
			 *     9     | xxxxxxxxxxxxxxxx | color
			 */
			switch(data&0x803f)
			{
				case 0x0000: /* clear registers */
					dma_skip=0;
					wms_dma_cols=0;
					wms_dma_rows=0;
					wms_dma_pal=0;
					wms_dma_fgcol=0;
					break;
				case 0x8000: /* draw nothing */
					break;
				case 0x8002: /* draw only nonzero pixels */
					dma_skip = ((wms_dma_cols + wms_dma_woffset+3)&(~3)) - wms_dma_cols;
					DMA_DRAW_NONZERO_BYTES(write_data,++);
					break;
				case 0x8003: /* draw all pixels */
					dma_skip = ((wms_dma_cols + wms_dma_woffset+3)&(~3)) - wms_dma_cols;
					DMA_DRAW_ALL_BYTES(*BYTE_XOR_LE(rda++));
					break;
				case 0x8006: /* draw nonzero pixels, zero as color */
					dma_skip = ((wms_dma_cols + wms_dma_woffset+3)&(~3)) - wms_dma_cols;
					DMA_DRAW_ALL_BYTES((*BYTE_XOR_LE(rda++)?*BYTE_XOR_LE(rda-1):wms_dma_fgcol));
					break;
				case 0x800a: /* ????? */
				case 0x8008: /* draw nonzero pixels as color */
					dma_skip = ((wms_dma_cols + wms_dma_woffset+3)&(~3)) - wms_dma_cols;
					DMA_DRAW_NONZERO_BYTES(wms_dma_fgcol,++);
					break;
				case 0x8009: /* draw nonzero pixels as color, zero as zero */
					dma_skip = ((wms_dma_cols + wms_dma_woffset+3)&(~3)) - wms_dma_cols;
					DMA_DRAW_ALL_BYTES((*BYTE_XOR_LE(rda++)?wms_dma_fgcol:0));
					break;
				case 0x800e: /* ????? */
				case 0x800c: /* draw all pixels as color (fill) */
					DMA_DRAW_ALL_BYTES(wms_dma_fgcol);
					break;
				case 0x8010: /* draw nothing */
					break;
				case 0x8012: /* draw nonzero pixels x-flipped */
					dma_skip = ((wms_dma_woffset + 3 - wms_dma_cols)&(~3)) + wms_dma_cols;
					DMA_DRAW_NONZERO_BYTES(write_data,--);
					break;
				case 0x8013: /* draw all pixels x-flipped */
					dma_skip = ((wms_dma_woffset + 3 - wms_dma_cols)&(~3)) + wms_dma_cols;
					DMA_DRAW_ALL_BYTES(*BYTE_XOR_LE(rda--));
					break;
				case 0x801a: /* ????? */
				case 0x8018: /* draw nonzero pixels as color x-flipped */
					dma_skip = ((wms_dma_woffset + 3 - wms_dma_cols)&(~3)) + wms_dma_cols;
					DMA_DRAW_NONZERO_BYTES(wms_dma_fgcol,--);
					break;
				case 0x8022: /* draw nonzero pixels y-flipped */
					dma_skip = ((wms_dma_cols + wms_dma_woffset+3)&(~3)) - wms_dma_cols;
					DMA_DRAW_NONZERO_BYTES(write_data,++);
					break;
				case 0x8032: /* draw nonzero pixels x-flipped and y-flipped */
					dma_skip = ((wms_dma_woffset + 3 - wms_dma_cols)&(~3)) + wms_dma_cols;
					DMA_DRAW_NONZERO_BYTES(write_data,--);
					break;
				default:
					if (errorlog) fprintf(errorlog, "\nCPU #0 PC %08x: unhandled DMA command %04x:\n",cpu_getpc(), data);
					if (errorlog) fprintf(errorlog, "%04x %04x x%04x y%04x  c%04x r%04x p%04x c%04x\n",wms_dma_subbank, wms_dma_bank, wms_dma_x, wms_dma_y, wms_dma_cols, wms_dma_rows, wms_dma_pal, wms_dma_fgcol );
					break;
			}
			/*
			 * One pixel every 41 ns (1e-9 sec)
			 */
			timer_set (TIME_IN_NSEC(41*wms_dma_cols*wms_dma_rows), data, dma_callback);
			break;
		case 2:
			wms_dma_woffset = data;
			break;
		case 4:
			wms_dma_subbank = data>>3;
			break;
		case 6:
			wms_dma_bank = ((data&0xfe00)?(data-0x200)*0x2000:data*0x2000);
			break;
		case 8:
			wms_dma_x = data&0x1ff;
			break;
		case 0x0a:
			wms_dma_y = data&0x1ff;
			break;
		case 0x0c:
			wms_dma_cols = data;
			dma_skip = data;
			break;
		case 0x0e:
			wms_dma_rows = data;
			break;
		case 0x10:  /* set palette */
    		wms_dma_pal = (data&0xff) << 8;
			wms_dma_pal_word = data;
    		break;
		case 0x12:  /* set color for 1-bit */
			wms_dma_fgcol = data&0xff;
			break;
		default:
			break;
	}
}
void wms_dma2_w(int offset, int data)
{
	/*
	 * This is a MESS! -- lots to do
	 * one pixel every 41 ns (1e-9 sec)
	 * 50,000,000 cycles per second
	 * --> approximately 2 cycles per pixel
	 */

	unsigned int i, pal, write_data, line_skip, dma_skip;
	int j, write_cols;
	unsigned char *rda;
	unsigned short *wrva;

	switch (offset)
	{
		case 0:
			break;
		case 2:
			write_cols = (wms_dma_cols+wms_dma_x<512?wms_dma_cols:512-wms_dma_x)-1;   /* Note the -1 */
			wms_dma_write_rows = (wms_dma_rows+wms_dma_y<512?wms_dma_rows:512-wms_dma_y);
			wms_dma_dst = ((wms_dma_y<<9) + (wms_dma_x)); /* byte addr */
			rda = &(GFX_ROM[wms_dma_bank+wms_dma_subbank]);
			wms_dma_stat = data;

			/*
			 * DMA registers
			 * ------------------
			 *
			 *  Register | Bit              | Use
			 * ----------+-FEDCBA9876543210-+------------
			 *     0     | x--------------- | trigger write (or clear if zero)
			 *           | ---184-1-------- | unknown
			 *           | ----------x----- | flip y
			 *           | -----------x---- | flip x
			 *           | ------------x--- | blit nonzero pixels as color
			 *           | -------------x-- | blit zero pixels as color
			 *           | --------------x- | blit nonzero pixels
			 *           | ---------------x | blit zero pixels
			 *     1     | xxxxxxxxxxxxxxxx | width offset
			 *     2     | xxxxxxxxxxxxxxxx | source address low word
			 *     3     | xxxxxxxxxxxxxxxx | source address high word
			 *     4     | xxxxxxxxxxxxxxxx | detination x
			 *     5     | xxxxxxxxxxxxxxxx | destination y
			 *     6     | xxxxxxxxxxxxxxxx | image columns
			 *     7     | xxxxxxxxxxxxxxxx | image rows
			 *     8     | xxxxxxxxxxxxxxxx | palette
			 *     9     | xxxxxxxxxxxxxxxx | color
			 */
			switch(data&0xe03f)
			{
				case 0x0000: /* clear registers */
					dma_skip=0;
					wms_dma_cols=0;
					wms_dma_rows=0;
//					wms_dma_pal=0;
					wms_dma_fgcol=0;
					break;
				case 0x8000: /* draw nothing */
					break;
				case 0x8002: /* draw only nonzero pixels */
					dma_skip = ((wms_dma_cols + wms_dma_woffset+3)&(~3)) - wms_dma_cols;
					DMA_DRAW_NONZERO_BYTES(write_data,++);
					break;
				case 0xc002: /* draw only nonzero pixels ??? */
//				case 0xe002: /* draw only nonzero pixels ??? */
					dma_skip = ((wms_dma_cols + wms_dma_woffset+1)&(~1)) - wms_dma_cols;
					for (i=0;i<(wms_dma_write_rows<<9);i+=512)
					{
						wrva = &(wms_videoram[wms_dma_dst+i]);
						for (j=write_cols;j>=0;j--)
						{
							if (wms_dma_odd_nibble&0x01)
							{
								write_data = (((*BYTE_XOR_LE(rda++))>>4)&0x0f);
								if (write_data)
								{
									*wrva = wms_dma_pal | write_data;
//									if (errorlog) fprintf(errorlog, "wr: %x\n", write_data);
								}
							}
							else
							{
								write_data = (*BYTE_XOR_LE(rda))&0x0f;
								if (write_data)
								{
									*wrva = wms_dma_pal | write_data;
//									if (errorlog) fprintf(errorlog, "wr: %x\n", write_data);
								}
							}
							wrva++;
							wms_dma_odd_nibble++;
						}
						rda+=dma_skip/2;
					}
					break;
				case 0x8003: /* draw all pixels */
					dma_skip = ((wms_dma_cols + wms_dma_woffset+3)&(~3)) - wms_dma_cols;
					DMA_DRAW_ALL_BYTES(*BYTE_XOR_LE(rda++));
					break;
				case 0x8008: /* draw nonzero pixels as color */
					dma_skip = ((wms_dma_cols + wms_dma_woffset+3)&(~3)) - wms_dma_cols;
					DMA_DRAW_NONZERO_BYTES(wms_dma_fgcol,++);
					break;
				case 0xc008: /* draw nonzero nibbles as color */
					dma_skip = ((wms_dma_cols + wms_dma_woffset+1)&(~1)) - wms_dma_cols;
					for (i=0;i<(wms_dma_write_rows<<9);i+=512)
					{
						wrva = &(wms_videoram[wms_dma_dst+i]);
						for (j=write_cols;j>=0;j--)
						{
							if (wms_dma_odd_nibble&0x01)
							{
								if ((*BYTE_XOR_LE(rda++))&0xf0)
								{
									*wrva = wms_dma_pal | wms_dma_fgcol;
								}
							}
							else
							{
								if ((*BYTE_XOR_LE(rda))&0x0f)
								{
									*wrva = wms_dma_pal | wms_dma_fgcol;
								}
							}
							wrva++;
							wms_dma_odd_nibble++;
						}
						rda+=dma_skip/2;
					}
					break;
				case 0x8009: /* draw nonzero pixels as color, zero as zero */
					dma_skip = ((wms_dma_cols + wms_dma_woffset+3)&(~3)) - wms_dma_cols;
					DMA_DRAW_ALL_BYTES((*BYTE_XOR_LE(rda++)?wms_dma_fgcol:0));
					break;
				case 0xe00c: /* draw all pixels as color (fill) */
				case 0x800c: /* draw all pixels as color (fill) */
					DMA_DRAW_ALL_BYTES(wms_dma_fgcol);
					break;
				case 0x8010: /* draw nothing */
					break;
				case 0xe012: /* draw only nonzero pixels x-flipped???? */
				case 0x8012: /* draw nonzero pixels x-flipped */
					dma_skip = ((wms_dma_woffset + 3 - wms_dma_cols)&(~3)) + wms_dma_cols;
					DMA_DRAW_NONZERO_BYTES(write_data,--);
					break;
				case 0x8013: /* draw all pixels x-flipped */
					dma_skip = ((wms_dma_woffset + 3 - wms_dma_cols)&(~3)) + wms_dma_cols;
					DMA_DRAW_ALL_BYTES(*BYTE_XOR_LE(rda--));
					break;
				case 0x8018: /* draw nonzero pixels as color x-flipped */
					dma_skip = ((wms_dma_woffset + 3 - wms_dma_cols)&(~3)) + wms_dma_cols;
					DMA_DRAW_NONZERO_BYTES(wms_dma_fgcol,--);
					break;
				case 0x8022: /* draw nonzero pixels y-flipped */
					dma_skip = ((wms_dma_cols + wms_dma_woffset+3)&(~3)) - wms_dma_cols;
					DMA_DRAW_NONZERO_BYTES(write_data,++);
					break;
				case 0x8032: /* draw nonzero pixels x-flipped and y-flipped */
					dma_skip = ((wms_dma_woffset + 3 - wms_dma_cols)&(~3)) + wms_dma_cols;
					DMA_DRAW_NONZERO_BYTES(write_data,--);
					break;
				default:
					if (errorlog) fprintf(errorlog, "\nCPU #0 PC %08x: unhandled DMA command %04x:\n",cpu_getpc(), data);
					if (errorlog) fprintf(errorlog, "%04x %04x x%04x y%04x c%04x r%04x p%04x c%04x  %04x %04x %04x %04x %04x %04x\n",wms_dma_subbank, wms_dma_bank, wms_dma_x, wms_dma_y, wms_dma_cols, wms_dma_rows, wms_dma_pal, wms_dma_fgcol, wms_dma_14, wms_dma_16, wms_dma_18, wms_dma_1a, wms_dma_1c, wms_dma_1e );
					break;
			}
			timer_set (TIME_IN_NSEC(41*wms_dma_cols*wms_dma_rows), data, dma_callback);
			break;
		case 4:
			wms_dma_subbank = data>>3;
			wms_dma_odd_nibble=data>>2;
			break;
		case 6:
			wms_dma_bank = ((data&0xfe00)?(data-0x200)*0x2000:data*0x2000);
			break;
		case 8:
			wms_dma_x = data&0x1ff;
			break;
		case 0x0a:
			wms_dma_y = data&0x1ff;
			break;
		case 0x0c:
			wms_dma_cols = data;
			dma_skip = data;
			break;
		case 0x0e:
			wms_dma_rows = data;
			break;
		case 0x10:  /* set palette */
    		wms_dma_pal = (data&0xff00);  /* changed from rev1? */
			wms_dma_pal_word = data;
			break;
		case 0x12:  /* set color for 1-bit */
			wms_dma_fgcol = data&0xff;
			break;
		case 0x14:
			wms_dma_14 = data;
			break;
		case 0x16:
			wms_dma_16 = data;
			break;
		case 0x18:
			wms_dma_18 = data;
			break;
		case 0x1a:
			wms_dma_1a = data;
			break;
		case 0x1c:
			wms_dma_1c = data;
			break;
		case 0x1e:
			wms_dma_1e = data;
			break;
		default:
			if (errorlog) fprintf(errorlog,"bs %x: %x\n",offset, data);
			break;
	}
}

static void wms_to_shiftreg(unsigned int address, unsigned short* shiftreg)
{
	memcpy(shiftreg, &wms_videoram[address>>3], 2*512*sizeof(unsigned short));
}

static void wms_from_shiftreg(unsigned int address, unsigned short* shiftreg)
{
	memcpy(&wms_videoram[address>>3], shiftreg, 2*512*sizeof(unsigned short));
}

void wms_01c00060_w(int offset, int data) /* protection and more */
{
	if ((data&0xfdff) == 0x0000) /* enable CMOS write */
	{
		smashtv_cmos_w_enable = 1;
//		if (errorlog) fprintf(errorlog, "Enable CMOS writes\n");
	}
	else if ((data&0xfdff) == 0x0100) /* disable CMOS write */
	{
//		smashtv_cmos_w_enable = 0;
//		if (errorlog) fprintf(errorlog, "Disable CMOS writes\n");
	}
	else
	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: warning - write %x to protection chip\n", cpu_getpc(), data);
}
int wms_01c00060_r(int offset) /* protection and more */
{
	if (cpu_getpc() == wms_protect_s) /* protection */
	{
		TMS34010_Regs Regs;
		TMS34010_GetRegs(&Regs);
		Regs.pc = wms_protect_d; /* skip it! */
		TMS34010_SetRegs(&Regs);
		return 0xffffffff;
	}
	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: warning - unhandled read from protection chip\n",cpu_getpc());
	return 0xffffffff;
}

void wms_sysreg_w(int offset, int data)
{
	/*
	 * Narc system register
	 * ------------------
	 *
	 *   | Bit              | Use
	 * --+-FEDCBA9876543210-+------------
	 *   | xxxxxxxx-------- |   7 segment led on CPU board
	 *   | --------xx------ |   CMOS page
	 *   | ----------x----- | - OBJ PAL RAM select
	 *   | -----------x---- | - autoerase enable
	 *   | ---------------- | - watchdog
	 *
	 */
	wms_cmos_page = (data&0xc0)<<7; /* 0x2000 offsets */
	if((data&0x20)&&wms_objpalram_select) /* access VRAM */
	{
		install_mem_write_handler(0, TOBYTE(0x00000000), TOBYTE(0x001fffff), wms_vram_w);
		install_mem_read_handler (0, TOBYTE(0x00000000), TOBYTE(0x001fffff), wms_vram_r);
	}
	else if(!(data&0x20)&&!wms_objpalram_select) /* access OBJPALRAM */
	{
		install_mem_write_handler(0, TOBYTE(0x00000000), TOBYTE(0x001fffff), wms_objpalram_w);
		install_mem_read_handler (0, TOBYTE(0x00000000), TOBYTE(0x001fffff), wms_objpalram_r);
	}
	wms_objpalram_select = ((data&0x20)?0:0x40000);
	if (data&0x10) /* turn off auto-erase */
	{
		wms_autoerase_enable = 0;
	}
	else /* enable auto-erase */
	{
		if (!wms_autoerase_enable)
		{
			wms_autoerase_start  = cpu_getscanline();
		}
		wms_autoerase_enable = 1;
	}
}
void wms_sysreg2_w(int offset, int data)
{
	wms_cmos_page = (data&0xc0)<<7; /* 0x2000 offsets */
	if(data&0x20&&wms_objpalram_select) /* access VRAM */
	{
		install_mem_write_handler(0, TOBYTE(0x00000000), TOBYTE(0x003fffff), wms_vram_w);
		install_mem_read_handler (0, TOBYTE(0x00000000), TOBYTE(0x003fffff), wms_vram_r);
	}
	else if(!(data&0x20)&&!wms_objpalram_select) /* access OBJPALRAM */
	{
		install_mem_write_handler(0, TOBYTE(0x00000000), TOBYTE(0x003fffff), wms_objpalram_w);
		install_mem_read_handler (0, TOBYTE(0x00000000), TOBYTE(0x003fffff), wms_objpalram_r);
	}
	wms_objpalram_select = ((data&0x20)?0:0x40000);
//	if (data&0x10) /* turn off auto-erase */
//	{
//		wms_autoerase_enable = 0;
//	}
//	else /* enable auto-erase */
//	{
//		wms_autoerase_enable = 1;
//		wms_autoerase_start  = cpu_getscanline();
//	}
}

static int narc_unknown_r (int offset)
{
	int ans = 0xffff;
	return ans;
}

void wms_cmos_w(int offset, int data)
{
	if (smashtv_cmos_w_enable)
	{
		COMBINE_WORD_MEM(&wms_cmos_ram[(offset)+wms_cmos_page], data);
	}
	else
	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: warning - write %x to disabled CMOS address %x\n", cpu_getpc(), data, offset);

}
int wms_cmos_r(int offset)
{
	return READ_WORD(&wms_cmos_ram[(offset)+wms_cmos_page]);
}

static void smashtv_sound_nmi(void)
{
	cpu_cause_interrupt(1,M6809_INT_NMI);
	if (errorlog) fprintf(errorlog, "sound NMI\n");
}
static void smashtv_sound_firq(void)
{
	cpu_cause_interrupt(1,M6809_INT_FIRQ);
	if (errorlog) fprintf(errorlog, "sound FIRQ\n");
}

static pia6821_interface smashtv_pia_intf =
{
	1,                                              /* 1 chip */
	{ PIA_DDRA, PIA_CTLA, PIA_DDRB, PIA_CTLB },     /* offsets */
	{ 0 },                                    /* input port A */
	{ 0 },                                    /* input bit CA1 */
	{ 0 },                                    /* input bit CA2 */
	{ 0 },                                    /* input port B */
	{ 0 },                                    /* input bit CB1 */
	{ 0 },                                    /* input bit CB2 */
	{ DAC_data_w },                           /* output port A */
	{ 0 },                                    /* output port B */
	{ 0 },                                    /* output CA2 */
	{ 0 },                                    /* output CB2 */
	{ smashtv_sound_firq },                   /* IRQ A */
	{ smashtv_sound_nmi }                     /* IRQ B */
};

void smashtv_sound_bank_select_w (int offset,int data)
{
	static int bank[16] = { 0x10000, 0x30000, 0x50000, 0x10000, 0x18000, 0x38000, 0x58000, 0x18000,
							0x20000, 0x40000, 0x60000, 0x10000, 0x28000, 0x48000, 0x68000, 0x18000 };
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	if (errorlog) fprintf(errorlog, "sound bank sel: %x -> %x\n", data, bank[data & 0x0f]);
	/* set bank address */
	cpu_setbank (5, &RAM[bank[data & 0x0f]]);
}
void narc_music_bank_select_w (int offset,int data)
{
	static int bank[8] = { 0x10000, 0x10000, 0x10000, 0x10000, 0x20000, 0x28000, 0x10000, 0x18000 };
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	/* set bank address */
	cpu_setbank (5, &RAM[bank[data&0x07]]);
}
void narc_digitizer_bank_select_w (int offset,int data)
{
	static int bank[8] = { 0x40000, 0x48000, 0x30000, 0x38000, 0x20000, 0x28000, 0x10000, 0x18000 };
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[2].memory_region];
	/* set bank address */
	cpu_setbank (6, &RAM[bank[data&0x07]]);
}
void mk_sound_bank_select_w (int offset,int data)
{
	static int bank[8] = { 0x10000, 0x18000, 0x20000, 0x28000, 0x30000, 0x38000, 0x40000, 0x48000 };
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	if (errorlog) fprintf(errorlog, "bank sel: %x -> %x\n", data, bank[data & 0x07]);
	/* set bank address */
	cpu_setbank (5, &RAM[bank[data&0x07]]);
	cpu_setbank (6, &RAM[0x4c000]);
}

void wms_load_code_roms(void)
{
	memcpy(CODE_ROM,Machine->memory_region[Machine->drv->cpu[0].memory_region],wms_code_rom_size);
}

#define DEREF(REG, SIZE)        *(SIZE*)(&SCRATCH_RAM[TOBYTE((REG) & 0xffffff)])
#define DEREF_INT8(REG)         DEREF(REG, INT8 )
#define DEREF_INT16(REG)        DEREF(REG, INT16)
#define DEREF_INT32(REG)        DEREF(REG, INT32)
#define BURN_TIME(INST_CNT)     TMS34010_ICount -= INST_CNT * TMS34010_AVGCYCLES

#define INT32_MOD(x) BIG_DWORD_LE(x)
#define INT16_MOD(x) (x)

/* Speed up loop body */

#define DO_SPEEDUP_LOOP(OFFS1, OFFS2, A8SIZE, A7SIZE)		\
															\
	a8 = A8SIZE##_MOD(DEREF(a2+OFFS1, A8SIZE));				\
	a7 = A7SIZE##_MOD(DEREF(a2+OFFS2, A7SIZE));				\
															\
	if (a8 > a1) 											\
	{ 														\
		a4 = a0; 											\
		a0 = a2; 											\
		a1 = a8; 											\
		a5 = a7; 											\
        BURN_TIME(10);										\
		continue; 											\
	} 														\
															\
	if ((a8 == a1) && (a7 >= a5)) 							\
	{ 														\
		a4 = a0; 											\
		a0 = a2; 											\
		a1 = a8; 											\
		a5 = a7; 											\
        BURN_TIME(13);										\
		continue; 											\
	} 														\
															\
	DEREF_INT32(a4) = BIG_DWORD_LE(a2);						\
	DEREF_INT32(a0) = DEREF_INT32(a2);						\
	DEREF_INT32(a2) = BIG_DWORD_LE(a0);						\
	a4 = a2; 												\
	BURN_TIME(17);


/* Speedup catch for games using 1 location */

#define DO_SPEEDUP_LOOP_1(LOC, OFFS1, OFFS2, A8SIZE, A7SIZE)	\
															\
	UINT32 a0 = LOC;										\
	UINT32 a2;												\
	UINT32 a4 = 0;											\
	 INT32 a1 = 0x80000000;									\
	 INT32 a5 = 0x80000000;									\
	 INT32 a7,a8;											\
	while (TMS34010_ICount > 0)								\
	{														\
		a2 = BIG_DWORD_LE(DEREF_INT32(a0));					\
		if (!a2)											\
		{													\
			cpu_spinuntil_int();							\
			break;											\
		}													\
		DO_SPEEDUP_LOOP(OFFS1, OFFS2, A8SIZE, A7SIZE);		\
	}


/* Speedup catch for games using 3 locations */

#define DO_SPEEDUP_LOOP_3(LOC1, LOC2, LOC3)					\
															\
	UINT32 a0,a2,temp1,temp2,temp3;							\
	UINT32 a4 = 0;                               			\
	 INT32 a1,a5,a7,a8;										\
															\
	while (TMS34010_ICount > 0)								\
	{														\
		temp1 = BIG_DWORD_LE(DEREF_INT32(LOC1));			\
		temp2 = BIG_DWORD_LE(DEREF_INT32(LOC2));			\
		temp3 = BIG_DWORD_LE(DEREF_INT32(LOC3));			\
		if (!temp1 && !temp2 && !temp3)						\
		{													\
			cpu_spinuntil_int();							\
			break;											\
		}													\
		a0 = LOC1;											\
		a1 = 0x80000000;									\
		a5 = 0x80000000;									\
		do													\
		{													\
			a2 = BIG_DWORD_LE(DEREF_INT32(a0));				\
			if (!a2)										\
			{												\
				TMS34010_ICount -= 20;						\
				break;										\
			}												\
			DO_SPEEDUP_LOOP(0xc0, 0xa0, INT32, INT32);		\
		} while (a2);										\
															\
		a0 = LOC2;											\
		a1 = 0x80000000;									\
		a5 = 0x80000000;									\
		do													\
		{													\
			a2 = BIG_DWORD_LE(DEREF_INT32(a0));				\
			if (!a2)										\
			{												\
				TMS34010_ICount -= 20;						\
				break;										\
			}												\
			DO_SPEEDUP_LOOP(0xc0, 0xa0, INT32, INT32);		\
		} while (a2);										\
															\
		a0 = LOC3;											\
		a1 = 0x80000000;									\
		a5 = 0x80000000;									\
		do													\
		{													\
			a2 = BIG_DWORD_LE(DEREF_INT32(a0));				\
			if (!a2)										\
			{												\
				TMS34010_ICount -= 20;						\
				break;										\
			}												\
			DO_SPEEDUP_LOOP(0xc0, 0xa0, INT32, INT32);		\
		} while (a2);										\
	}


static int narc_speedup_r(int offset)
{
	if (offset)
	{
		UINT32 value1 = READ_WORD(&SCRATCH_RAM[TOBYTE(0x1b310)]);

		/* Suspend cpu if it's waiting for an interrupt */
		if (cpu_getpc() == 0xffde33e0 && !value1)
		{
			DO_SPEEDUP_LOOP_1(0x1000040, 0xc0, 0xa0, INT32, INT32);
		}
		return value1;
	}
	else
	{
		return READ_WORD(&SCRATCH_RAM[TOBYTE(0x1b300)]);
	}
}
static int smashtv_speedup_r(int offset)
{
	if (offset)
	{
		return READ_WORD(&SCRATCH_RAM[TOBYTE(0x86770)]);
	}
	else
	{
		UINT32 value1 = READ_WORD(&SCRATCH_RAM[TOBYTE(0x86760)]);

		/* Suspend cpu if it's waiting for an interrupt */
		if (cpu_getpc() == 0xffe0a340 && !value1)
		{
			DO_SPEEDUP_LOOP_1(0x1000040, 0xa0, 0x80, INT16, INT32);
		}
		return value1;
	}
}

static int totcarn_speedup_r(int offset)
{
	if (offset)
	{
		return READ_WORD(&SCRATCH_RAM[TOBYTE(0x7ddf0)]);
	}
	else
	{
		UINT32 value1 = READ_WORD(&SCRATCH_RAM[TOBYTE(0x7dde0)]);

		/* Suspend cpu if it's waiting for an interrupt */
		if (cpu_getpc() == 0xffc0c970 && !value1)
		{
			DO_SPEEDUP_LOOP_1(0x1000040, 0xa0, 0x90, INT16, INT16);
		}
		return value1;
	}
}
static int trogp_speedup_r(int offset)
{
	if (offset)
	{
		return READ_WORD(&SCRATCH_RAM[TOBYTE(0xa1ef0)]);
	}
	else
	{
		UINT32 value1 = READ_WORD(&SCRATCH_RAM[TOBYTE(0xa1ee0)]);

		/* Suspend cpu if it's waiting for an interrupt */
		if (cpu_getpc() == 0xffe210d0 && !value1)
		{
			DO_SPEEDUP_LOOP_1(0x1000040, 0xc0, 0xa0, INT32, INT32);
		}
		return value1;
	}
}
static int trog_speedup_r(int offset)
{
	if (offset)
	{
		return READ_WORD(&SCRATCH_RAM[TOBYTE(0xa20b0)]);
	}
	else
	{
		UINT32 value1 = READ_WORD(&SCRATCH_RAM[TOBYTE(0xa20a0)]);

		/* Suspend cpu if it's waiting for an interrupt */
		if (cpu_getpc() == 0xffe20630 && !value1)
		{
			DO_SPEEDUP_LOOP_1(0x1000040, 0xc0, 0xa0, INT32, INT32);
		}
		return value1;
	}
}
static int trog3_speedup_r(int offset)
{
	if (offset)
	{
		UINT32 value1 = READ_WORD(&SCRATCH_RAM[TOBYTE(0xa2090)]);

		/* Suspend cpu if it's waiting for an interrupt */
		if (cpu_getpc() == 0xffe20660 && !value1)
		{
			DO_SPEEDUP_LOOP_1(0x1000040, 0xc0, 0xa0, INT32, INT32);
		}
		return value1;
	}
	else
	{
		return READ_WORD(&SCRATCH_RAM[TOBYTE(0xa2080)]);
	}
}
static int mk_speedup_r(int offset)
{
	if (offset)
	{
		return READ_WORD(&SCRATCH_RAM[TOBYTE(0x4f050)]);
	}
	else
	{
		UINT32 value1 = READ_WORD(&SCRATCH_RAM[TOBYTE(0x4f040)]);

		/* Suspend cpu if it's waiting for an interrupt */
		if (cpu_getpc() == 0xffce1ec0 && !value1)
		{
			DO_SPEEDUP_LOOP_3(0x104b6b0, 0x104b6f0, 0x104b710);
		}
		return value1;
	}
}
static int hiimpact_speedup_r(int offset)
{
	if (!offset)
	{
		return READ_WORD(&SCRATCH_RAM[TOBYTE(0x53140)]);
	}
	else
	{
		UINT32 value1 = READ_WORD(&SCRATCH_RAM[TOBYTE(0x53150)]);

		/* Suspend cpu if it's waiting for an interrupt */
		if (cpu_getpc() == 0xffe28bb0 && !value1)
		{
			DO_SPEEDUP_LOOP_3(0x1000080, 0x10000a0, 0x10000c0);
		}
		return value1;
	}
}
static int shimpact_speedup_r(int offset)
{
	if (!offset)
	{
		return READ_WORD(&SCRATCH_RAM[TOBYTE(0x52060)]);
	}
	else
	{
		UINT32 value1 = READ_WORD(&SCRATCH_RAM[TOBYTE(0x52070)]);

		/* Suspend cpu if it's waiting for an interrupt */
		if (cpu_getpc() == 0xffe27f00 && !value1)
		{
			DO_SPEEDUP_LOOP_3(0x1000000, 0x1000020, 0x1000040);
		}
		return value1;
	}
}

#define T2_FFC08C40																\
	a5x = (INT32)(DEREF_INT8(a1+0x2d0));			/* MOVB   *A1(2D0h),A5  */  \
	DEREF_INT8(a1+0x2d0) = a2 & 0xff;				/* MOVB   A2,*A1(2D0h)  */  \
	a3x = 0xf0;										/* MOVI   F0h,A3		*/	\
	a5x = (UINT32)a5x * (UINT32)a3x;				/* MPYU   A3,A5			*/	\
	a5x += 0x1008000;								/* ADDI   1008000h,A5	*/  \
	a3x = (UINT32)a2  * (UINT32)a3x;				/* MPYU   A2,A3			*/	\
	a3x += 0x1008000;								/* ADDI   1008000h,A3	*/  \
    a7x = (INT32)(DEREF_INT16(a1+0x190));			/* MOVE   *A1(190h),A7,0*/	\
    a6x = (INT32)(DEREF_INT16(a5x+0x50));			/* MOVE   *A5(50h),A6,0 */	\
	a7x -= a6x;										/* SUB    A6,A7			*/  \
    a6x = (INT32)(DEREF_INT16(a3x+0x50));			/* MOVE   *A3(50h),A6,0 */	\
	a7x += a6x;										/* ADD    A6,A7			*/  \
	DEREF_INT16(a1+0x190) = a7x & 0xffff;			/* MOVE   A7,*A1(190h),0*/	\
	a5x = DEREF_INT32(a5x+0xa0);					/* MOVE   *A5(A0h),A5,1 */	\
	a3x = DEREF_INT32(a3x+0xa0);					/* MOVE   *A3(A0h),A3,1 */	\
	a6x = DEREF_INT32(a1+0x140);					/* MOVE   *A1(140h),A6,1*/	\
	a6xa7x = (INT64)a6x * a3x / a5x;				/* MPYS   A3,A6			*/  \
													/* DIVS   A5,A6			*/  \
	DEREF_INT32(a1+0x140) = a6xa7x & 0xffffffff;	/* MOVE   A6,*A1(140h),1*/

static int term2_speedup_r(int offset)
{
	if (offset)
	{
		return READ_WORD(&SCRATCH_RAM[TOBYTE(0xaa050)]);
	}
	else
	{
		UINT32 value1 = READ_WORD(&SCRATCH_RAM[TOBYTE(0xaa040)]);

		/* Suspend cpu if it's waiting for an interrupt */
		if (cpu_getpc() == 0xffcdc270 && !value1)
		{
			INT32 a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a14,b0,b1,b2;
			INT32 a3x,a5x,a6x,a7x;
			INT64 a6xa7x;

			b1 = 0;			 									// CLR    B1
			b2 = (INT32)(DEREF_INT16(0x100F640));				// MOVE   @100F640h,B2,0
			if (!b2)											// JREQ   FFC029F0h
			{
				cpu_spinuntil_int();
				return value1;
			}
			b2--;												// DEC    B2
			b0  = 0x01008000;									// MOVI   1008000h,B0
			a4  = 0x7fffffff;									// MOVI   7FFFFFFFh,A4

			while (1)
			{
				// FFC07770
				a10 = b0;										// MOVE   B0,A10
				a0  = a10;										// MOVE   A10,A0
				a3  = a4;										// MOVE   A4,A3
																// CMP    B2,B1
				if (b1 < b2)									// JRLT   FFC07800h
				{
					// FFC07800
					a4 = (INT32)(DEREF_INT16(a10+0xc0));		// MOVE   *A10(C0h),A4,0
					a4 <<= 16;									// SLL    10h,A4
				}
				else
				{
                    // FFC077C0
					a4 = 0x80000000;							// MOVI   80000000h,A4
				}												// JR     FFC07830h

				// FFC07830
				b0 += 0xf0;									    // ADDI   F0h,B0
				a6 = 0x80000000;                                // MOVI   80000000h,A6
				a5 = 0x80000000;                                // MOVI   80000000h,A5
				goto t2_FFC07DD0;								// JR     FFC07DD0h

			t2_FFC078C0:
				a8  = DEREF_INT32(a1+0x1c0);					// MOVE   *A1(1C0h),A8,1
				a7  = DEREF_INT32(a1+0x1a0);					// MOVE   *A1(1A0h),A7,1
				a14 = (INT32)(DEREF_INT16(a1+0x220));			// MOVE   *A1(220h),A14,0
				if (a14 & 0x6000)								// BTST   Eh,A14
				{												// JRNE   FFC07C50h
					goto t2_FFC07C50;							// BTST   Dh,A14
				}												// JRNE   FFC07C50h

				if (a8 <= a3)									// CMP    A3,A8
				{
					goto t2_FFC07AE0;							// JRLE   FFC07AE0h
				}

				a2 = b1 - 1;									// MOVE   B1,A2;  DEC    A2
				T2_FFC08C40										// CALLR  FFC08C40h
				a14 = DEREF_INT32(a1);							// MOVE   *A1,A14,1
				DEREF_INT32(a0) = a14;							// MOVE   A14,*A0,1
				DEREF_INT32(a14+0x20) = a0;						// MOVE   A0,*A14(20h),1
				a14 = b0 - 0x1e0;								// MOVE   B0,A14; SUBI   1E0h,A14
				DEREF_INT32(a1+0x20) = a14;						// MOVE   A14,*A1(20h),1
				a9 = DEREF_INT32(a14);							// MOVE   *A14,A9,1
				DEREF_INT32(a14) = a1;							// MOVE   A1,*A14,1
				DEREF_INT32(a9+0x20) = a1;						// MOVE   A1,*A9(20h),1
				DEREF_INT32(a1) = a9;							// MOVE   A9,*A1,1
				goto t2_FFC07DD0;								// JR     FFC07DD0h

			t2_FFC07AE0:
				if (a8 >= a4)									// CMP    A4,A8
				{
					goto t2_FFC07C50;							// JRGE   FFC07C50h
				}

				a2 = b1 + 1;									// MOVE   B1,A2; INC    A2
				T2_FFC08C40										// CALLR  FFC08C40h
				a14 = DEREF_INT32(a1);							// MOVE   *A1,A14,1
				DEREF_INT32(a0) = a14;							// MOVE   A14,*A0,1
				DEREF_INT32(a14+0x20) = a0;						// MOVE   A0,*A14(20h),1
				a14 = b0;										// MOVE   B0,A14
				a9 = DEREF_INT32(a14+0x20);						// MOVE   *A14(20h),A9,1
				DEREF_INT32(a1) = a14;							// MOVE   A14,*A1,1
				DEREF_INT32(a14+0x20) = a1;						// MOVE   A1,*A14(20h),1
				DEREF_INT32(a9) = a1;							// MOVE   A1,*A9,1
				DEREF_INT32(a1+0x20) = a9;						// MOVE   A9,*A1(20h),1
				goto t2_FFC07DD0;

			t2_FFC07C50:
				if (a8 > a6) 									// CMP    A6,A8
				{
					a1 = a0; 									// MOVE   A1,A0
					a6 = a8; 									// MOVE   A8,A6
					a5 = a7; 									// MOVE   A7,A5
					goto t2_FFC07DD0;
				}

				if ((a8 == a6) && (a7 >= a5)) 					// CMP    A5,A7
				{
					a1 = a0; 									// MOVE   A1,A0
					a6 = a8; 									// MOVE   A8,A6
					a5 = a7; 									// MOVE   A7,A5
					goto t2_FFC07DD0;
				}

				// FFC07CC0
				a14 = DEREF_INT32(a0+0x20);						// MOVE   *A0(20h),A14,1
				DEREF_INT32(a14) = a1;							// MOVE   A1,*A14,1
				DEREF_INT32(a1+0x20) = a14;						// MOVE   A14,*A1(20h),1
				a14 = DEREF_INT32(a1);							// MOVE   *A1,A14,1
				DEREF_INT32(a0) = a14;							// MOVE   A14,*A0,1
				DEREF_INT32(a1) = a0;							// MOVE   A0,*A1,1
				DEREF_INT32(a0 +0x20) = a1;						// MOVE   A1,*A0(20h),1
				DEREF_INT32(a14+0x20) = a0;						// MOVE   A0,*A14(20h),1

			t2_FFC07DD0:
				BURN_TIME(50);
				if (TMS34010_ICount <= 0)
				{
					break;
				}

				a1 = DEREF_INT32(a0);							// MOVE   *A0,A1,1
				if (a10 != a1)									// CMP    A1,A10
				{
					goto t2_FFC078C0;							// JRNE   FFC078C0h
				}

				b1++;											// INC    B1
				if (b1 > b2)									// CMP    B2,B1
				{
					cpu_spinuntil_int();
					return value1;								// JRLE   FFC07770h; RTS
				}
			}
		}

		return value1;
	}
}
int strkforc_speedup_r(int offset)
{
	if (!offset)
	{
		return READ_WORD(&SCRATCH_RAM[TOBYTE(0x71dc0)]);
	}
	else
	{
		UINT32 value1 = READ_WORD(&SCRATCH_RAM[TOBYTE(0x71dd0)]);

		/* Suspend cpu if it's waiting for an interrupt */
		if (cpu_getpc() == 0xffe0a290 && !value1)
		{
			DO_SPEEDUP_LOOP_1(0x1000060, 0xc0, 0xa0, INT32, INT32);
		}
		return value1;
	}
}
static int mk2_speedup_r(int offset)
{
	if (!offset)
	{
		return READ_WORD(&SCRATCH_RAM[TOBYTE(0x68e60)]);
	}
	else
	{
		UINT32 value1 = READ_WORD(&SCRATCH_RAM[TOBYTE(0x68e70)]);

		/* Suspend cpu if it's waiting for an interrupt */
		if (cpu_getpc() == 0xff80db70 && !value1)
		{
			DO_SPEEDUP_LOOP_3(0x105d480, 0x105d4a0, 0x105d4c0);
		}
		return value1;
	}
}
static int nbajam_speedup_r(int offset)
{
	if (offset)
	{
		return READ_WORD(&SCRATCH_RAM[TOBYTE(0x754d0)]);
	}
	else
	{
		UINT32 value1 = READ_WORD(&SCRATCH_RAM[TOBYTE(0x754c0)]);

		/* Suspend cpu if it's waiting for an interrupt */
		if (cpu_getpc() == 0xff833480 && !value1)
		{
			DO_SPEEDUP_LOOP_1(0x1008040, 0xd0, 0xb0, INT16, INT16);
		}
		return value1;
	}
}

static int narc_music_speedup_r (int offset)
{
	unsigned char a, b;
	a = Machine->memory_region[Machine->drv->cpu[1].memory_region][0x0228];
	b = Machine->memory_region[Machine->drv->cpu[1].memory_region][0x0226];
	if ((a==b)&&(cpu_getpc()==0xc786)) cpu_spinuntil_int();
	return a;
}
static int narc_digitizer_speedup_r (int offset)
{
	unsigned char a, b;
	a = Machine->memory_region[Machine->drv->cpu[2].memory_region][0x0228];
	b = Machine->memory_region[Machine->drv->cpu[2].memory_region][0x0226];
	if ((a==b)&&(cpu_getpc()==0xc786)) cpu_spinuntil_int();
	return a;
}
static int mk_sound_speedup_r (int offset)
{
	unsigned char a, b;
	a = Machine->memory_region[Machine->drv->cpu[1].memory_region][0x0218];
	b = Machine->memory_region[Machine->drv->cpu[1].memory_region][0x0216];
	if ((a==b)&&(cpu_getpc()==0xf579)) cpu_spinuntil_int(); /* MK */
	if ((a==b)&&(cpu_getpc()==0xf5db)) cpu_spinuntil_int(); /* totcarn */
	if ((a==b)&&(cpu_getpc()==0xf5d2)) cpu_spinuntil_int(); /* term2 */
	return a;
}
static int smashtv_sound_speedup_r (int offset)
{
	unsigned char a, b;
	a = Machine->memory_region[Machine->drv->cpu[1].memory_region][0x0218];
	b = Machine->memory_region[Machine->drv->cpu[1].memory_region][0x0216];
	if ((a==b)&&(cpu_getpc()==0x963d)) cpu_spinuntil_int(); /* smashtv */
	if ((a==b)&&(cpu_getpc()==0x97d1)) cpu_spinuntil_int(); /* trog, trogp */
	if ((a==b)&&(cpu_getpc()==0x97be)) cpu_spinuntil_int(); /* hiimpact */
	if ((a==b)&&(cpu_getpc()==0x984c)) cpu_spinuntil_int(); /* shimpact */
	if ((a==b)&&(cpu_getpc()==0x9883)) cpu_spinuntil_int(); /* strkforc */
//	if (errorlog) fprintf(errorlog, "PC: 0x%04x -- smashtv_sound_speedup\n", cpu_getpc());
	return a;
}

static void remove_access_errors(void)
{
	/* get rid of unmapped access errors during tests */
	install_mem_write_handler(0, TOBYTE(0x00200000), TOBYTE(0x0020003f), MWA_NOP);
	install_mem_write_handler(0, TOBYTE(0x01100000), TOBYTE(0x0110003f), MWA_NOP);
	install_mem_read_handler(0, TOBYTE(0x00200000), TOBYTE(0x0020003f), MRA_NOP);
	install_mem_read_handler(0, TOBYTE(0x01100000), TOBYTE(0x0110003f), MRA_NOP);
}


static void load_gfx_roms_4bit(void)
{
	int i;
	unsigned char d1,d2,d3,d4;
	unsigned char *mem_rom;
	memset(GFX_ROM,0,wms_gfx_rom_size);
	mem_rom = Machine->memory_region[2];
	/* load the graphics ROMs -- quadruples 2 bits each */
	for (i=0;i<wms_gfx_rom_size;i+=2)
	{
		d1 = ((mem_rom[                   (i  )/4])>>(2*((i  )%4)))&0x03;
		d2 = ((mem_rom[wms_gfx_rom_size/4+(i  )/4])>>(2*((i  )%4)))&0x03;
		d3 = ((mem_rom[                   (i+1)/4])>>(2*((i+1)%4)))&0x03;
		d4 = ((mem_rom[wms_gfx_rom_size/4+(i+1)/4])>>(2*((i+1)%4)))&0x03;
		WRITE_WORD(&GFX_ROM[i],d1|(d2<<2)|(d1<<4)|(d2<<6)|(d3<<8)|(d4<<10)|(d3<<12)|(d4<<14));
	}
	free(Machine->memory_region[2]);
	Machine->memory_region[2] = 0;
}
static void load_gfx_roms_6bit(void)
{
	int i;
	unsigned char d1,d2,d3,d4,d5,d6;
	unsigned char *mem_rom;
	memset(GFX_ROM,0,wms_gfx_rom_size);
	mem_rom = Machine->memory_region[2];
	/* load the graphics ROMs -- quadruples 2 bits each */
	for (i=0;i<wms_gfx_rom_size;i+=2)
	{
		d1 = ((mem_rom[                     (i  )/4])>>(2*((i  )%4)))&0x03;
		d2 = ((mem_rom[  wms_gfx_rom_size/4+(i  )/4])>>(2*((i  )%4)))&0x03;
		d3 = ((mem_rom[2*wms_gfx_rom_size/4+(i  )/4])>>(2*((i  )%4)))&0x03;
		d4 = ((mem_rom[                     (i+1)/4])>>(2*((i+1)%4)))&0x03;
		d5 = ((mem_rom[  wms_gfx_rom_size/4+(i+1)/4])>>(2*((i+1)%4)))&0x03;
		d6 = ((mem_rom[2*wms_gfx_rom_size/4+(i+1)/4])>>(2*((i+1)%4)))&0x03;
		WRITE_WORD(&GFX_ROM[i],d1|(d2<<2)|(d3<<4)|(d4<<8)|(d5<<10)|(d6<<12));
	}
	free(Machine->memory_region[2]);
	Machine->memory_region[2] = 0;
}
static void load_gfx_roms_8bit(void)
{
	int i;
	unsigned char d1,d2;
	unsigned char *mem_rom;
	memset(GFX_ROM,0,wms_gfx_rom_size);
	mem_rom = Machine->memory_region[2];
	/* load the graphics ROMs -- quadruples */
	for (i=0;i<wms_gfx_rom_size;i+=4)
	{
		d1 = mem_rom[                     i/4];
		d2 = mem_rom[  wms_gfx_rom_size/4+i/4];
		WRITE_WORD(&GFX_ROM[i  ],(unsigned int)((unsigned int)(d1) | ((unsigned int)(d2)<<8)));
		d1 = mem_rom[2*wms_gfx_rom_size/4+i/4];
		d2 = mem_rom[3*wms_gfx_rom_size/4+i/4];
		WRITE_WORD(&GFX_ROM[i+2],(unsigned int)((unsigned int)(d1) | ((unsigned int)(d2)<<8)));
	}
	free(Machine->memory_region[2]);
	Machine->memory_region[2] = 0;
}

static void load_adpcm_roms_512k(void)
{
	unsigned char *mem_reg4;
	unsigned char *mem_reg5;
	unsigned char *mem_reg6;
	mem_reg4 = Machine->memory_region[4];
	mem_reg5 = Machine->memory_region[5];
	mem_reg6 = Machine->memory_region[6];
	memcpy(mem_reg4+0x40000, mem_reg4+0x00000, 0x40000); /* copy u12 */
	memcpy(mem_reg5+0x60000, mem_reg4+0x00000, 0x20000); /* copy u12 */
	memcpy(mem_reg5+0x20000, mem_reg4+0x20000, 0x20000); /* copy u12 */
	memcpy(mem_reg6+0x60000, mem_reg4+0x00000, 0x20000); /* copy u12 */
	memcpy(mem_reg6+0x20000, mem_reg4+0x20000, 0x20000); /* copy u12 */
	memcpy(mem_reg6+0x40000, mem_reg5+0x00000, 0x20000); /* copy u13 */
	memcpy(mem_reg6+0x00000, mem_reg5+0x40000, 0x20000); /* copy u13 */
}

static void wms_modify_pen(int i, int rgb)
{
	extern unsigned short *shrinked_pens;

#define rgbpenindex(r,g,b) ((Machine->scrbitmap->depth==16) ? ((((r)>>3)<<10)+(((g)>>3)<<5)+((b)>>3)) : ((((r)>>5)<<5)+(((g)>>5)<<2)+((b)>>6)))

	int r,g,b;

	r = (rgb >> 10) & 0x1f;
	g = (rgb >>  5) & 0x1f;
	b = (rgb >>  0) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	Machine->pens[i] = shrinked_pens[rgbpenindex(r,g,b)];
}

static void wms_8bit_paletteram_xRRRRRGGGGGBBBBB_word_w(int offset,int data)
{
	int i;
	int oldword = READ_WORD(&paletteram[offset]);
	int newword = COMBINE_WORD(oldword,data);

	int base = offset / 2;

	WRITE_WORD(&paletteram[offset],newword);

	for (i = 0; i < 8; i++)
	{
		wms_modify_pen(base | (i << 13), newword);
	}
}

static void wms_6bit_paletteram_xRRRRRGGGGGBBBBB_word_w(int offset,int data)
{
	/*
	 * the palette entry to find is mapped like this:
	 * Bit 15 - Not Used
	 * Bit 14 - Not Used
	 * Bit 13 - Not Used
	 * Bit 12 - Not Used
	 * Bit 11 - PAL Bit 03
	 * Bit 10 - PAL Bit 02
	 * Bit 09 - PAL Bit 01
	 * Bit 08 - PAL Bit 00
	 * Bit 07 - PAL Bit 07
	 * Bit 06 - PAL Bit 06
	 * Bit 05 - DATA Bit 05
	 * Bit 04 - DATA Bit 04
	 * Bit 03 - DATA Bit 03
	 * Bit 02 - DATA Bit 02
	 * Bit 01 - DATA Bit 01
	 * Bit 00 - DATA Bit 00
	 */

	int i;
	int oldword = READ_WORD(&paletteram[offset]);
	int newword = COMBINE_WORD(oldword,data);

	int base = offset / 2;
	base = (base & 0xf3f) | ((base & 0xc0) << 8);

	WRITE_WORD(&paletteram[offset],newword);

	for (i = 0; i < 16; i++)
	{
		wms_modify_pen(base | ((i & 3) << 6) | ((i & 0x0c) << 10), newword);
	}
}

static void wms_4bit_paletteram_xRRRRRGGGGGBBBBB_word_w(int offset,int data)
{
	/*
	 * the palette entry to find is mapped like this:
	 * Bit 07 - PAL Bit 07
	 * Bit 06 - PAL Bit 06
	 * Bit 05 - PAL Bit 05
	 * Bit 04 - PAL Bit 04
	 * Bit 03 - DATA Bit 03
	 * Bit 02 - DATA Bit 02
	 * Bit 01 - DATA Bit 01
	 * Bit 00 - DATA Bit 00
	 */

	int i;
	int oldword = READ_WORD(&paletteram[offset]);
	int newword = COMBINE_WORD(oldword,data);

	int base = offset / 2;
	base = (base & 0x0f) | ((base & 0xf0) << 8);

	WRITE_WORD(&paletteram[offset],newword);

	for (i = 0; i < 256; i++)
	{
		wms_modify_pen(base | (i << 4), newword);
	}
}

static void init_8bit(void)
{
	install_mem_write_handler(0, TOBYTE(0x01800000), TOBYTE(0x0181ffff), wms_8bit_paletteram_xRRRRRGGGGGBBBBB_word_w);
	install_mem_read_handler (0, TOBYTE(0x01800000), TOBYTE(0x0181ffff), paletteram_word_r);
}
static void init_6bit(void)
{
	install_mem_write_handler(0, TOBYTE(0x01810000), TOBYTE(0x0181ffff), wms_6bit_paletteram_xRRRRRGGGGGBBBBB_word_w);
	install_mem_read_handler (0, TOBYTE(0x01810000), TOBYTE(0x0181ffff), paletteram_word_r);
}
static void init_4bit(void)
{
	install_mem_write_handler(0, TOBYTE(0x0181f000), TOBYTE(0x0181ffff), wms_4bit_paletteram_xRRRRRGGGGGBBBBB_word_w);
	install_mem_read_handler (0, TOBYTE(0x0181f000), TOBYTE(0x0181ffff), paletteram_word_r);
}

/* driver_init functions */

void narc_driver_init(void)
{
	/* set up speedup loops */
	install_mem_read_handler(0, TOBYTE(0x0101b300), TOBYTE(0x0101b31f), narc_speedup_r);
	install_mem_read_handler(1, 0x0228, 0x0228, narc_music_speedup_r);
	install_mem_read_handler(2, 0x0228, 0x0228, narc_digitizer_speedup_r);

	TMS34010_set_stack_base(0, SCRATCH_RAM, TOBYTE(0x1000000));
}
void trog_driver_init(void)
{
	/* set up speedup loops */
	install_mem_read_handler(0, TOBYTE(0x010a20a0), TOBYTE(0x010a20bf), trog_speedup_r);
	install_mem_read_handler(1, 0x0218, 0x0218, smashtv_sound_speedup_r);
	wms_protect_s = 0xffe47c40;
	wms_protect_d = 0xffe47af0;

	TMS34010_set_stack_base(0, SCRATCH_RAM, TOBYTE(0x1000000));
}
void trog3_driver_init(void)
{
	/* set up speedup loops */
	install_mem_read_handler(0, TOBYTE(0x010a2080), TOBYTE(0x010a209f), trog3_speedup_r);
	install_mem_read_handler(1, 0x0218, 0x0218, smashtv_sound_speedup_r);
	wms_protect_s = 0xffe47c70;
	wms_protect_d = 0xffe47b20;

	TMS34010_set_stack_base(0, SCRATCH_RAM, TOBYTE(0x1000000));
}
void trogp_driver_init(void)
{
	/* set up speedup loops */
	install_mem_read_handler(0, TOBYTE(0x010a1ee0), TOBYTE(0x010a1eff), trogp_speedup_r);
	install_mem_read_handler(1, 0x0218, 0x0218, smashtv_sound_speedup_r);

	TMS34010_set_stack_base(0, SCRATCH_RAM, TOBYTE(0x1000000));
}
void smashtv_driver_init(void)
{
	/* set up speedup loops */
	install_mem_read_handler(0, TOBYTE(0x01086760), TOBYTE(0x0108677f), smashtv_speedup_r);
	install_mem_read_handler(1, 0x0218, 0x0218, smashtv_sound_speedup_r);

	TMS34010_set_stack_base(0, SCRATCH_RAM, TOBYTE(0x01000000));
}
void hiimpact_driver_init(void)
{
	/* set up speedup loops */
	install_mem_read_handler(0, TOBYTE(0x01053140), TOBYTE(0x0105315f), hiimpact_speedup_r);
	install_mem_read_handler(1, 0x0218, 0x0218, smashtv_sound_speedup_r);
	wms_protect_s = 0xffe77c20;
	wms_protect_d = 0xffe77ad0;

	TMS34010_set_stack_base(0, SCRATCH_RAM, TOBYTE(0x1000000));
}
void shimpact_driver_init(void)
{
	/* set up speedup loops */
	install_mem_read_handler(0, TOBYTE(0x01052060), TOBYTE(0x0105207f), shimpact_speedup_r);
	install_mem_read_handler(1, 0x0218, 0x0218, smashtv_sound_speedup_r);
	wms_protect_s = 0xffe07a40;
	wms_protect_d = 0xffe078f0;

	TMS34010_set_stack_base(0, SCRATCH_RAM, TOBYTE(0x1000000));
}
void strkforc_driver_init(void)
{
	/* set up speedup loops */
	install_mem_read_handler(0, TOBYTE(0x01071dc0), TOBYTE(0x01071ddf), strkforc_speedup_r);
	install_mem_read_handler(1, 0x0218, 0x0218, smashtv_sound_speedup_r);
	wms_protect_s = 0xffe4c100;
	wms_protect_d = 0xffe4c1d0;

	TMS34010_set_stack_base(0, SCRATCH_RAM, TOBYTE(0x1000000));
}
void mk_driver_init(void)
{
	/* set up speedup loops */
	install_mem_read_handler(0, TOBYTE(0x0104f040), TOBYTE(0x0104f05f), mk_speedup_r);
	install_mem_read_handler(1, 0x0218, 0x0218, mk_sound_speedup_r);
	wms_protect_s = 0xffc98930;
	wms_protect_d = 0xffc987f0;

	TMS34010_set_stack_base(0, SCRATCH_RAM, TOBYTE(0x1000000));
}
void term2_driver_init(void)
{
	/* extra input handler */
	install_mem_read_handler(0, TOBYTE(0x01600020), TOBYTE(0x0160005f), term2_input_lo_r ); /* ??? */

	/* set up speedup loops */
	install_mem_read_handler(0, TOBYTE(0x010aa040), TOBYTE(0x010aa05f), term2_speedup_r);
	install_mem_read_handler(1, 0x0218, 0x0218, mk_sound_speedup_r);
	wms_protect_s = 0xffd64f30;
	wms_protect_d = 0xffd64de0;

	TMS34010_set_stack_base(0, SCRATCH_RAM, TOBYTE(0x1000000));
}
void totcarn_driver_init(void)
{
	/* set up speedup loops */
	install_mem_read_handler(0, TOBYTE(0x0107dde0), TOBYTE(0x0107ddff), totcarn_speedup_r);
	install_mem_read_handler(1, 0x0218, 0x0218, mk_sound_speedup_r);
	wms_protect_s = 0xffd1fd30;
	wms_protect_d = 0xffd1fbf0;

	TMS34010_set_stack_base(0, SCRATCH_RAM, TOBYTE(0x1000000));
}
void totcarnp_driver_init(void)
{
	/* set up speedup loops */
	install_mem_read_handler(0, TOBYTE(0x0107dde0), TOBYTE(0x0107ddff), totcarn_speedup_r);
	install_mem_read_handler(1, 0x0218, 0x0218, mk_sound_speedup_r);
	wms_protect_s = 0xffd1edd0;
	wms_protect_d = 0xffd1ec90;

	TMS34010_set_stack_base(0, SCRATCH_RAM, TOBYTE(0x1000000));
}
void mk2_driver_init(void)
{
	/* set up speedup loops */
	install_mem_read_handler(0, TOBYTE(0x01068e60), TOBYTE(0x01068e7f), mk2_speedup_r);
}
void nbajam_driver_init(void)
{
	/* set up speedup loops */
	install_mem_read_handler(0, TOBYTE(0x010754c0), TOBYTE(0x010754df), nbajam_speedup_r);
}

/* init_machine functions */

void narc_init_machine(void)
{
	/*
	 * Z-Unit
	 *
	 * music board is 6809 driving YM2151, DAC
	 * effect board is 6809 driving CVSD, DAC
	 *
	 */

	/* set up ROMs */
	if (!wms_rom_loaded)
	{
		wms_load_code_roms();
		load_gfx_roms_8bit();
		wms_rom_loaded = 1;
	}

	/* set up palette */
	init_8bit();

	/* get rid of unmapped access errors during tests */
	remove_access_errors();

	/* set up sound board */
	narc_music_bank_select_w(0,0);
	narc_digitizer_bank_select_w(0,0);
	m6809_Flags = M6809_FAST_S;
	install_mem_write_handler(0, TOBYTE(0x01e00000), TOBYTE(0x01e0001f), narc_sound_w);

	/* special input handler */
	install_mem_read_handler(0, TOBYTE(0x01c00000), TOBYTE(0x01c0005f), narc_input_r );

	install_mem_read_handler(0, TOBYTE(0x09afffd0), TOBYTE(0x09afffef), narc_unknown_r); /* bug? */
	install_mem_read_handler(0, TOBYTE(0x38383900), TOBYTE(0x383839ff), narc_unknown_r); /* bug? */

	TMS34010_set_shiftreg_functions(0, wms_to_shiftreg, wms_from_shiftreg);
}
void smashtv_init_machine(void)
{
	/*
	 * Y-Unit
	 *
	 * sound board is 6809 driving YM2151, DAC, and CVSD
	 *
	 */

	/* set up ROMs */
	if (!wms_rom_loaded)
	{
		wms_load_code_roms();
		load_gfx_roms_6bit();
		wms_rom_loaded = 1;
	}

	/* set up palette */
	init_6bit();

	/* get rid of unmapped access errors during tests */
	remove_access_errors();

	/* set up sound board */
	smashtv_sound_bank_select_w(0,0);
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&smashtv_pia_intf);
	pia_1_ca1_w (0, (1));
	install_mem_write_handler(0, TOBYTE(0x01e00000), TOBYTE(0x01e0001f), smashtv_sound_w);

	TMS34010_set_shiftreg_functions(0, wms_to_shiftreg, wms_from_shiftreg);
}
void mk_init_machine(void)
{
	/*
	 * Y-Unit
	 *
	 * sound board is 6809 driving YM2151, DAC, and OKIM6295
	 *
	 */

	/* set up ROMs */
	if (!wms_rom_loaded)
	{
		wms_load_code_roms();
		load_gfx_roms_6bit();
		load_adpcm_roms_512k();
		wms_rom_loaded = 1;
	}

	/* set up palette */
	init_6bit();

	/* get rid of unmapped access errors during tests */
	remove_access_errors();

	/* set up sound board */
	mk_sound_bank_select_w(0,0);
	m6809_Flags = M6809_FAST_NONE;
	install_mem_write_handler(0, TOBYTE(0x01e00000), TOBYTE(0x01e0001f), mk_sound_w);

	TMS34010_set_shiftreg_functions(0, wms_to_shiftreg, wms_from_shiftreg);
}
void trog_init_machine(void)
{
	/*
	 * Y-Unit
	 *
	 * sound board is 6809 driving YM2151, DAC, and OKIM6295
	 *
	 */

	/* set up ROMs */
	if (!wms_rom_loaded)
	{
		wms_load_code_roms();
		load_gfx_roms_4bit();
		wms_rom_loaded = 1;
	}

	/* set up palette */
	init_4bit();

	/* get rid of unmapped access errors during tests */
	remove_access_errors();

	/* set up sound board */
	smashtv_sound_bank_select_w(0,0);
	m6809_Flags = M6809_FAST_NONE;
	pia_startup (&smashtv_pia_intf);
	pia_1_ca1_w (0, (1));
	/* fix sound (hack) */
	install_mem_write_handler(0, TOBYTE(0x01e00000), TOBYTE(0x01e0001f), trog_sound_w);

	TMS34010_set_shiftreg_functions(0, wms_to_shiftreg, wms_from_shiftreg);
}
void mk2_init_machine(void)
{
	unsigned int i,j;
	if (!wms_rom_loaded)
	{
		wms_load_code_roms();
		load_gfx_roms_8bit();
		wms_rom_loaded = 1;
	}

	/* set up palette */
	init_8bit();

	/*for (i=0;i<256;i++)
	{
		for (j=0;j<256;j++)
		{
			wms_conv_table[(i<<8)+j] = ((i&0x1f)<<8) | j;
		}
	}*/
	wms_videoram_size = 0x80000*2;

	TMS34010_set_shiftreg_functions(0, wms_to_shiftreg, wms_from_shiftreg);
}
void nbajam_init_machine(void)
{
	unsigned int i,j;
	if (!wms_rom_loaded)
	{
		wms_load_code_roms();
		load_gfx_roms_8bit();
		wms_rom_loaded = 1;
	}

	/* set up palette */
	init_8bit();

	/*for (i=0;i<256;i++)
	{
		for (j=0;j<256;j++)
		{
			wms_conv_table[(i<<8)+j] = ((i&0x1f)<<8) | j;
		}
	}*/
	wms_videoram_size = 0x80000*2;

	/* set up sound board */
	mk_sound_bank_select_w(0,0);
	m6809_Flags = M6809_FAST_NONE;
//	install_mem_write_handler(0, TOBYTE(0x01d01020), TOBYTE(0x01d0103f), nbajam_sound_w);

	TMS34010_set_shiftreg_functions(0, wms_to_shiftreg, wms_from_shiftreg);
}

