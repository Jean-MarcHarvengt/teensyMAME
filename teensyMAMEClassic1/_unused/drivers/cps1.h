#ifndef _CPS1_H_
#define _CPS1_H_

extern unsigned char * cps1_eeprom;
extern int cps1_eeprom_size;
extern unsigned char * cps1_ram;        /* M68000 RAM */
extern unsigned char * cps1_gfxram;     /* Video RAM */
extern unsigned char * cps1_output;     /* Output ports */
extern int cps1_ram_size;
extern int cps1_gfxram_size;
extern int cps1_output_size;

extern int cps1_input_r(int offset);    /* Input ports */
extern int cps1_player_input_r(int offset);    /* Input ports */

extern int cps1_interrupt(void);       /* Ghouls and Ghosts */
extern int cps1_interrupt2(void);      /* Everything else */
extern int cps1_interrupt3(void);      /* (apart from Street Fighter) */

extern int  cps1_vh_start(void);
extern void cps1_vh_stop(void);
extern void cps1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

/* Game specific data */
struct CPS1config
{
        char *name;             /* game driver name */
        int   base_scroll1;     /* Index of first scroll 1 object */
        int   base_obj;         /* Index of first obj object */
        int   base_scroll2;     /* Index of first scroll 2 object */
        int   base_scroll3;     /* Index of first scroll 3 object */
        int   alternative;      /* KLUDGE */
        int   space_scroll1;    /* Space character code for scroll 1 */
        int   space_scroll2;    /* Space character code for scroll 2 */
        int   space_scroll3;    /* Space character code for scroll 3 */
        int   cpsb_addr;        /* CPS board B test register address */
        int   cpsb_value;       /* CPS board B test register expected value */
        int   gng_sprite_kludge;  /* Ghouls n Ghosts sprite kludge */
};

extern struct CPS1config *cps1_game_config;


/* Sound hardware */
//extern int cps1_sh_start(void);
//extern void cps1_sample_w(int offset, int value);

#endif






