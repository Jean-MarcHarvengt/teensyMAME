#ifndef __AVGDVG__
#define __AVGDVG__

/* vector engine types, passed to vg_init */

#define AVGDVG_MIN          1
#define USE_DVG             1
#define USE_AVG_RBARON      2
#define USE_AVG_BZONE       3
#define USE_AVG             4
#define USE_AVG_TEMPEST     5
#define USE_AVG_MHAVOC      6
#define USE_AVG_SWARS       7
#define USE_AVG_QUANTUM     8
#define AVGDVG_MAX          8

/* vector palette types (non-sega) */
#define VEC_PAL_BW          0
#define VEC_PAL_MONO_AQUA   1
#define VEC_PAL_BZONE       2
#define VEC_PAL_COLOR       3
#define VEC_PAL_SWARS       4

int avgdvg_done (void);
void avgdvg_go (int offset, int data);
void avgdvg_reset (int offset, int data);
int avgdvg_init(int vgType);

void avg_init_colors (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

void tempest_colorram_w (int offset, int data);
void mhavoc_colorram_w (int offset, int data);
void quantum_colorram_w (int offset, int data);

void dvg_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void avg_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
int dvg_start(void);
int avg_start(void);
int avg_start_tempest(void);
int avg_start_mhavoc(void);
int avg_start_starwars(void);
int avg_start_quantum(void);
int avg_start_bzone(void);
int avg_start_redbaron(void);
void dvg_stop(void);
void avg_stop(void);

/* the next one can be r obsolete */

#endif
