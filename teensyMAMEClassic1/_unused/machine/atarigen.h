/***************************************************************************

  atarigen.h

  General functions for mid-to-late 80's Atari raster games.

***************************************************************************/

#include "driver.h"

#define COLOR_PALETTE_555 1
#define COLOR_PALETTE_4444 2

struct atarigen_modesc
{
	int maxmo;                              /* maximum number of MO's */
	int moskip;                             /* number of bytes per MO entry */
	int mowordskip;                         /* number of bytes between MO words */
	int ignoreword;                         /* ignore an entry if this word == 0xffff */
	int linkword, linkshift, linkmask;		/* link = (data[linkword >> linkshift) & linkmask */
	int reverse;                            /* render in reverse link order */
};

typedef void (*atarigen_morender) (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param);

void atarigen_init_machine (void (*sound_int)(void), int slapstic);

void atarigen_eeprom_enable_w (int offset, int data);
int atarigen_eeprom_r (int offset);
void atarigen_eeprom_w (int offset, int data);

int atarigen_hiload (void);
void atarigen_hisave (void);

int atarigen_slapstic_r (int offset);
void atarigen_slapstic_w (int offset, int data);

void atarigen_sound_reset (void);
void atarigen_sound_w (int offset, int data);
int atarigen_6502_sound_r (int offset);

void atarigen_6502_sound_w (int offset, int data);
int atarigen_sound_r (int offset);

int atarigen_init_display_list (struct atarigen_modesc *_modesc);
void atarigen_update_display_list (unsigned char *base, int start, int scanline);
void atarigen_render_display_list (struct osd_bitmap *bitmap, atarigen_morender morender, void *param);

extern int atarigen_cpu_to_sound, atarigen_cpu_to_sound_ready;
extern int atarigen_sound_to_cpu, atarigen_sound_to_cpu_ready;

extern unsigned char *atarigen_playfieldram;
extern unsigned char *atarigen_spriteram;
extern unsigned char *atarigen_alpharam;
extern unsigned char *atarigen_vscroll;
extern unsigned char *atarigen_hscroll;
extern unsigned char *atarigen_eeprom;
extern unsigned char *atarigen_slapstic;

extern int atarigen_playfieldram_size;
extern int atarigen_spriteram_size;
extern int atarigen_alpharam_size;
extern int atarigen_eeprom_size;
