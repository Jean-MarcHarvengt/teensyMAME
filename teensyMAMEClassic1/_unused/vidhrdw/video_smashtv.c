/*************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

**************************************************************************/
#include "driver.h"
#include "TMS34010/tms34010.h"
#include "vidhrdw/generic.h"

void wms_stateload(void);
void wms_statesave(void);

unsigned short *wms_videoram;
int wms_videoram_size = 0x80000;

/* Variables in machine/smashtv.c */
extern unsigned char *wms_cmos_ram;
extern unsigned int wms_autoerase_enable;
extern unsigned int wms_autoerase_start;
extern unsigned int wms_dma_pal_word;

void wms_vram_w(int offset, int data)
{
	unsigned int tempw,tempwb;
	unsigned short tempwordhi;
	unsigned short tempwordlo;
	unsigned short datalo;
	unsigned short datahi;
	unsigned int mask;

	/* first vram data */
	tempwordhi = wms_videoram[offset+1];
	tempwordlo = wms_videoram[offset];
	tempw = (wms_videoram[offset]&0x00ff) + ((wms_videoram[offset+1]&0x00ff)<<8);
	tempwb = COMBINE_WORD(tempw,data);
	datalo = tempwb&0x00ff;
	datahi = (tempwb&0xff00)>>8;
	wms_videoram[offset] = (tempwordlo&0xff00)|datalo;
	wms_videoram[offset+1] = (tempwordhi&0xff00)|datahi;

	/* now palette data */
	tempwordhi = wms_videoram[offset+1];
	tempwordlo = wms_videoram[offset];
	mask = (~(((unsigned int) data)>>16))|0xffff0000;
	data = ((data&0xffff0000) | wms_dma_pal_word) & mask;
	tempw = (((unsigned short) (wms_videoram[offset]&0xff00))>>8) + (wms_videoram[offset+1]&0xff00);
	tempwb = COMBINE_WORD(tempw,data);
	datalo = tempwb&0x00ff;
	datahi = (tempwb&0xff00)>>8;
	wms_videoram[offset] = (tempwordlo&0x00ff)|(datalo<<8);
	wms_videoram[offset+1] = (tempwordhi&0x00ff)|(datahi<<8);
}
void wms_objpalram_w(int offset, int data)
{
	unsigned int tempw,tempwb;
	unsigned short tempwordhi;
	unsigned short tempwordlo;
	unsigned short datalo;
	unsigned short datahi;
	unsigned int mask;

	tempwordhi = wms_videoram[offset+1];
	tempwordlo = wms_videoram[offset];
	tempw = ((wms_videoram[offset]&0xff00)>>8) + (wms_videoram[offset+1]&0xff00);
	tempwb = COMBINE_WORD(tempw,data);
	datalo = tempwb&0x00ff;
	datahi = (tempwb&0xff00)>>8;
	wms_videoram[offset] = (tempwordlo&0x00ff)|(datalo<<8);
	wms_videoram[offset+1] = (tempwordhi&0x00ff)|(datahi<<8);
}
int wms_vram_r(int offset)
{
	return (wms_videoram[offset]&0x00ff) | (wms_videoram[offset+1]<<8);
}
int wms_objpalram_r(int offset)
{
	return (wms_videoram[offset]>>8) | (wms_videoram[offset+1]&0xff00);
}

int wms_vh_start(void)
{
	if ((wms_cmos_ram = malloc(0x8000)) == 0)
	{
		if (errorlog) fprintf(errorlog, "smashtv.c: Couldn't Alloc CMOS RAM\n");
		return 1;
	}
	if ((paletteram = malloc(0x4000)) == 0)
	{
		if (errorlog) fprintf(errorlog, "smashtv.c: Couldn't Alloc color RAM\n");
		free(wms_cmos_ram);
		return 1;
	}
	if ((wms_videoram = malloc(wms_videoram_size)) == 0)
	{
		if (errorlog) fprintf(errorlog, "smashtv.c: Couldn't Alloc video RAM\n");
		free(wms_cmos_ram);
		free(paletteram);
		return 1;
	}
	memset(wms_cmos_ram,0,0x8000);
	return 0;
}
int wms_t_vh_start(void)
{
	if ((wms_cmos_ram = malloc(0x8000)) == 0)
	{
		if (errorlog) fprintf(errorlog, "smashtv.c: Couldn't Alloc CMOS RAM\n");
		return 1;
	}
	if ((paletteram = malloc(0x4000)) == 0)
	{
		if (errorlog) fprintf(errorlog, "smashtv.c: Couldn't Alloc color RAM\n");
		free(wms_cmos_ram);
		return 1;
	}
	if ((wms_videoram = malloc(0x100000)) == 0)
	{
		if (errorlog) fprintf(errorlog, "smashtv.c: Couldn't Alloc video RAM\n");
		free(wms_cmos_ram);
		free(paletteram);
		return 1;
	}
	memset(wms_cmos_ram,0,0x8000);
	return 0;
}
void wms_vh_stop (void)
{
	free(wms_cmos_ram);
	free(wms_videoram);
	free(paletteram);
}

void wms_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int v,h;
	unsigned short *rv;
	int skip,col;

	//if (osd_key_pressed(OSD_KEY_Q)) wms_statesave();
	//if (osd_key_pressed(OSD_KEY_W)) wms_stateload();

	//if (osd_key_pressed(OSD_KEY_E)&&errorlog) fprintf(errorlog, "adpcm: error\n");
	//if (osd_key_pressed(OSD_KEY_R)&&errorlog) fprintf(errorlog, "adpcm: okay\n");

	rv = &wms_videoram[(~TMS34010_get_DPYSTRT(0) & 0x1ff0)<<5];
	col = Machine->drv->visible_area.max_x;
	skip = 511 - col;

	if (bitmap->depth==16)
	{
		unsigned int min = Machine->drv->visible_area.min_y;
		unsigned int max = Machine->drv->visible_area.max_y;
		unsigned short *pens = Machine->pens;										
																							
		for (v = min; v <= max; v++)									
		{																			
			unsigned short *rows = &((unsigned short *)bitmap->line[v])[0];			
			unsigned int diff = rows - rv;						
			h = col;
			do	
			{	
				*(rv+diff) = pens[*rv];											
				rv++;				
			}
			while (h--);
																					
			if (wms_autoerase_enable||(v>=wms_autoerase_start))						
			{																		
				memcpy(rv-col-1,&wms_videoram[510*512],(col+1)*sizeof(unsigned short));	
			}																		
																					
			rv = (((rv + skip) - wms_videoram) & 0x3ffff) + wms_videoram;			
		}																			
	}
	else
	{
		unsigned int min = Machine->drv->visible_area.min_y;
		unsigned int max = Machine->drv->visible_area.max_y;
		unsigned short *pens = Machine->pens;										
																							
		for (v = min; v <= max; v++)									
		{																			
			unsigned char *rows = &(bitmap->line[v])[0];			
			h = col;
			do	
			{	
				*(rows++) = pens[*(rv++)];											
			}
			while (h--);
																					
			if (wms_autoerase_enable||(v>=wms_autoerase_start))						
			{																		
				memcpy(rv-col-1,&wms_videoram[510*512],(col+1)*sizeof(unsigned short));	
			}																		
																					
			rv = (((rv + skip) - wms_videoram) & 0x3ffff) + wms_videoram;			
		}																			
	}
	wms_autoerase_start=1000;
}

