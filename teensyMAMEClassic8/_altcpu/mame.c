#include <stdio.h>
#include <string.h>
#include "machine.h"
#include "osdepend.h"

#define DEFAULT_NAME "pacman"


FILE *errorlog;


int main(int argc,char **argv)
{
	int i,log;


	log = 0;
	for (i = 1;i < argc;i++)
	{
		if (stricmp(argv[i],"-log") == 0)
			log = 1;
	}

	if (log) errorlog = fopen("error.log","wa");

	if (init_machine(argc > 1 && argv[1][0] != '-' ? argv[1] : DEFAULT_NAME) == 0)
	{
		printf("\nPLEASE DO NOT DISTRIBUTE THE SOURCE FILES OR THE EXECUTABLE WITH ROM IMAGES.\n"
			   "DOING SO WILL HARM FURTHER EMULATOR DEVELOPMENT AND WILL CONSIDERABLY ANNOY\n"
			   "THE RIGHTFUL COPYRIGHT HOLDERS OF THOSE ROM IMAGES AND CAN RESULT IN LEGAL\n"
			   "ACTION UNDERTAKEN BY EARLIER MENTIONED COPYRIGHT HOLDERS.\n"
			   "\n\n"
			   "Press <ENTER> to continue.\n");

		getchar();

		if (osd_init(argc,argv) == 0)
		{
			if (run_machine(argc > 1 && argv[1][0] != '-' ? argv[1] : DEFAULT_NAME) != 0)
				printf("Unable to start emulation\n");

			osd_exit();
		}
		else printf("Unable to initialize system\n");
	}
	else printf("Unable to initialize machine emulation\n");

	if (errorlog) fclose(errorlog);

	exit(0);
}



typedef unsigned char		byte;



#define	TRUE			(1)
#define FALSE			(0)

#define OSD_OK			(0)
#define OSD_NOT_OK		(1)

/*
 * Cleanup routines to be executed when the program is terminated.
 */

void osd_exit (void)
{

}


/*
 * Make a snapshot of the screen. Not implemented yet.
 * Current bug : because the path is set to the rom directory, PCX files
 * will be saved there.
 */

int osd_snapshot(void)
{
	return (OSD_OK);
}

struct osd_bitmap *osd_create_bitmap (int width, int height)
{
  //struct osd_bitmap  	*bitmap;

	//return (bitmap);
	return (NULL);
}

void osd_free_bitmap (struct osd_bitmap *bitmap)
{
	if (bitmap)
	{
		//free (bitmap->private);
		free (bitmap);
		bitmap = NULL;
	}
}



struct osd_bitmap *osd_create_display (int width, int height)
{
  //struct osd_bitmap  	*bitmap;

	/* Allocate the bitmap and set the image width and height. */

	//if ((bitmap = malloc (sizeof (struct osd_bitmap) + (height-1) * sizeof(unsigned char *))) == NULL)
	//{
		return (NULL);
	//}

  //return (bitmap);
}

/*
 * Shut down the display.
 */

void osd_close_display (void)
{

}


int osd_obtain_pen (byte red, byte green, byte blue)
{

	return (0);
}

void osd_update_display (void)
{

}


int osd_read_key (void)
{

	  return (0);


}


int osd_key_pressed (int request)
{

		return (FALSE);

}

void osd_poll_joystick (void)
{

}

int osd_joy_pressed (int joycode)
{
 return 0;
}

/*
 * Audio shit.
 */

void osd_update_audio (void)
{
	/* Not used. */
}

void osd_play_sample (int channel, byte *data, int len, int freq,
			int volume, int loop)
{

}

void osd_play_streamed_sample (int channel, byte *data, int len, int freq, int volume)
{
	/* Not used. */
}

void osd_adjust_sample (int channel, int freq, int volume)
{

}

void osd_stop_sample (int channel)
{

}

