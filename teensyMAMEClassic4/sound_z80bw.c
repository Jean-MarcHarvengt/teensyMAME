/* z80bw.c *********************************
 updated: 1997-04-09 08:46 TT
 updated  20-3-1998 LT Added colour changes on base explosion
 updated  02-6-1998 HJB copied from 8080bw and removed unneeded code
 *
 * Author      : Tormod Tjaberg
 * Created     : 1997-04-09
 * Description : Sound routines for the 'astinvad' games
 *
 * Note:
 * The samples were taken from Michael Strutt's (mstrutt@pixie.co.za)
 * excellent space invader emulator and converted to signed samples so
 * they would work under SEAL. The port info was also gleaned from
 * his emulator. These sounds should also work on all the invader games.
 *
 * The sounds are generated using output port 3 and 5
 *
 * Port 4:
 * bit 0=UFO  (repeats)       0.raw
 * bit 1=Shot                 1.raw
 * bit 2=Base hit             2.raw
 * bit 3=Invader hit          3.raw
 *
 * Port 5:
 * bit 0=Fleet movement 1     4.raw
 * bit 1=Fleet movement 2     5.raw
 * bit 2=Fleet movement 3     6.raw
 * bit 3=Fleet movement 4     7.raw
 * bit 4=UFO 2                8.raw
 */

#include "driver.h"

/* LT 20-3-1998 */
void astinvad_sh_port4_w(int offset, int data)
{
	static unsigned char Sound = 0;

	if (data & 0x01 && ~Sound & 0x01)
		sample_start (0, 0, 1);

	if (~data & 0x01 && Sound & 0x01)
		sample_stop (0);

	if (data & 0x02 && ~Sound & 0x02)
		sample_start (1, 1, 0);

	if (~data & 0x02 && Sound & 0x02)
		sample_stop (1);

	if (data & 0x04 && ~Sound & 0x04){
            sample_start (2, 2, 0);
    /* turn all colours red here */
            palette_change_color(1,0xff,0x00,0x00);
            palette_change_color(2,0xff,0x00,0x00);
            palette_change_color(3,0xff,0x00,0x00);
            palette_change_color(4,0xff,0x00,0x00);
            palette_change_color(5,0xff,0x00,0x00);
            palette_change_color(6,0xff,0x00,0x00);
            palette_change_color(7,0xff,0x00,0x00);
        }

	if (~data & 0x04 && Sound & 0x04){
            sample_stop (2);
    /* restore colours here */
            palette_change_color(1,0x20,0x20,0xff);
            palette_change_color(2,0x20,0xff,0x20);
            palette_change_color(3,0x20,0xff,0xff);
            palette_change_color(4,0xff,0x20,0x20);
            palette_change_color(5,0xff,0x20,0xff);
            palette_change_color(6,0xff,0xff,0x20);
            palette_change_color(7,0xff,0xff,0xff);
        }
	if (data & 0x08 && ~Sound & 0x08)
		sample_start (3, 3, 0);

	if (~data & 0x08 && Sound & 0x08)
		sample_stop (3);

	Sound = data;
}



void astinvad_sh_port5_w(int offset, int data)
{
	static unsigned char Sound = 0;

	if (data & 0x01 && ~Sound & 0x01)
		sample_start (4, 4, 0);

	if (data & 0x02 && ~Sound & 0x02)
		sample_start (5, 5, 0);

	if (data & 0x04 && ~Sound & 0x04)
		sample_start (6, 6, 0);

	if (data & 0x08 && ~Sound & 0x08)
		sample_start (7, 7, 0);

	if (data & 0x10 && ~Sound & 0x10)
		sample_start (8, 8, 0);

	if (~data & 0x10 && Sound & 0x10)
		sample_stop (5);

	Sound = data;
}

