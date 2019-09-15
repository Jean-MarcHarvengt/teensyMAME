/***************************************************************************

  vidhrdw/llander.c

  Functions to emulate the blinking control panel in lunar lander.
  Added 11/6/98, by Chris Kirmse (ckirmse@ricochet.net)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "vidhrdw/avgdvg.h"

int png_read_artwork(const char *file_name, struct osd_bitmap **bitmap, unsigned char **palette, int *num_palette, unsigned char **trans, int *num_trans);

#define NUM_LIGHTS 5

static struct osd_bitmap *llander_panel;
static struct osd_bitmap *llander_lit_panel;
static int panel_colors;

static struct rectangle light_areas[NUM_LIGHTS] =
{
   {  0, 205, 0, 127 },
   {206, 343, 0, 127 },
   {344, 481, 0, 127 },
   {482, 616, 0, 127 },
   {617, 799, 0, 127 },
};

/* current status of each light */
static int lights[NUM_LIGHTS];
/* whether or not each light needs to be redrawn*/
static int lights_changed[NUM_LIGHTS];
/***************************************************************************

  Lunar Lander video routines

***************************************************************************/

void llander_init_colors (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
   unsigned char *orig_palette, *orig_lit_palette, *trans;
   int num_pens_used,num_lit_pens_used, num_trans;

   avg_init_colors(palette,colortable,color_prom);

   llander_panel = NULL;

   /* Get background - it better not have too many colors */
   /* Get original picture */
   if (png_read_artwork("llander.png",&llander_panel,&orig_palette,&num_pens_used, &trans, &num_trans) == 0)
      llander_panel = NULL;

   if (llander_panel->width > Machine->scrbitmap->width)
   {
      if (errorlog) fprintf(errorlog,"Bitmap is too wide for the resolution\n");
      osd_free_bitmap(llander_panel);
      llander_panel = NULL;
   }

   if (llander_panel != NULL)
   {
      if (png_read_artwork("llander1.png",&llander_lit_panel,&orig_lit_palette,&num_lit_pens_used, &trans, &num_trans)
          == 0)
         llander_lit_panel = NULL;
   }

   if (llander_panel != NULL && num_pens_used > 50)
   {
      if (errorlog) fprintf(errorlog,"Bitmap had more than 50 colors\n");
      osd_free_bitmap(llander_panel);
      llander_panel = NULL;
   }

   if (llander_lit_panel != NULL && num_lit_pens_used > 50)
   {
      if (errorlog) fprintf(errorlog,"Bitmap had more than 50 colors\n");
      osd_free_bitmap(llander_lit_panel);
      llander_lit_panel = NULL;
   }

   if (llander_panel != NULL && llander_lit_panel != NULL)
   {
      int i;

      panel_colors = num_pens_used;
      /* Load colors into the palette */
      for (i = 0; i < panel_colors; i++)
      {
         palette[3*(150+i)] = orig_palette[i*3];
         palette[3*(150+i)+1] = orig_palette[i*3+1];
         palette[3*(150+i)+2] = orig_palette[i*3+2];
      }
      for (i = 0; i < num_lit_pens_used; i++)
      {
         palette[3*(150+panel_colors+i)] = orig_lit_palette[i*3];
         palette[3*(150+panel_colors+i)+1] = orig_lit_palette[i*3+1];
         palette[3*(150+panel_colors+i)+2] = orig_lit_palette[i*3+2];
      }
   }
}

int llander_start(void)
{
   int i,j;

   if (dvg_start())
      return 1;

   if (llander_panel == NULL)
      return 0;

   for (i=0;i<NUM_LIGHTS;i++)
   {
      lights[i] = 0;
      lights_changed[i] = 1;
   }

   for ( j=0; j<llander_panel->height; j++)
      for (i=0; i<llander_panel->width; i++)
      {
         llander_panel->line[j][i] = Machine->pens[llander_panel->line[j][i]+150];
         llander_lit_panel->line[j][i] =
            Machine->pens[llander_lit_panel->line[j][i]+150+panel_colors];

      }

   return 0;
}

void llander_stop(void)
{
   dvg_stop();

   if (llander_panel != NULL)
      osd_free_bitmap(llander_panel);
   llander_panel = NULL;

   if (llander_lit_panel != NULL)
      osd_free_bitmap(llander_lit_panel);
   llander_lit_panel = NULL;

}

void llander_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
   int i;
   struct osd_bitmap vector_bitmap;
   struct rectangle rect;

   int section_width = bitmap->width/NUM_LIGHTS;

   if (llander_panel == NULL)
   {
      dvg_screenrefresh(bitmap,full_refresh);
      return;
   }

   vector_bitmap.width = bitmap->width;
   vector_bitmap.height = bitmap->height - llander_panel->height;
   vector_bitmap._private = bitmap->_private;
   vector_bitmap.line = bitmap->line;

   dvg_screenrefresh(&vector_bitmap,full_refresh);

   if (full_refresh)
   {
      rect.min_x = 0;
      rect.max_x = llander_panel->width-1;
      rect.min_y = bitmap->height - llander_panel->height;
      rect.max_y = bitmap->height - 1;

      copybitmap(bitmap,llander_panel,0,0,
                 0,bitmap->height - llander_panel->height,&rect,TRANSPARENCY_NONE,0);
      osd_mark_dirty (rect.min_x,rect.min_y,rect.max_x,rect.max_y,0);
   }


   for (i=0;i<NUM_LIGHTS;i++)
   {
      if (lights_changed[i] || full_refresh)
      {
         rect.min_x = light_areas[i].min_x;
         rect.max_x = light_areas[i].max_x;
         rect.min_y = bitmap->height - llander_panel->height + light_areas[i].min_y;
         rect.max_y = bitmap->height - llander_panel->height + light_areas[i].max_y;

         if (lights[i])
            copybitmap(bitmap,llander_lit_panel,0,0,
                       0,bitmap->height - llander_panel->height,&rect,TRANSPARENCY_NONE,0);
         else
            copybitmap(bitmap,llander_panel,0,0,
                       0,bitmap->height - llander_panel->height,&rect,TRANSPARENCY_NONE,0);

         osd_mark_dirty (rect.min_x,rect.min_y,rect.max_x,rect.max_y,0);

         lights_changed[i] = 0;
      }
   }
}

/* Lunar lander LED port seems to be mapped thus:

   NNxxxxxx - Apparently unused
   xxNxxxxx - Unknown gives 4 high pulses of variable duration when coin put in ?
   xxxNxxxx - Start    Lamp ON/OFF == 0/1
   xxxxNxxx - Training Lamp ON/OFF == 1/0
   xxxxxNxx - Cadet    Lamp ON/OFF
   xxxxxxNx - Prime    Lamp ON/OFF
   xxxxxxxN - Command  Lamp ON/OFF

   Selection lamps seem to all be driver 50/50 on/off during attract mode ?

*/

void llander_led_w (int offset,int data)
{
//	if (errorlog) fprintf (errorlog, "LANDER LED: %02x\n",data);

    int i;

    for (i=0;i<5;i++)
    {
       int new_light = (data & (1 << (4-i))) != 0;
       if (lights[i] != new_light)
       {
          lights[i] = new_light;
          lights_changed[i] = 1;
       }
    }



}

