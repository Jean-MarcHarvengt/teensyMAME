#include "driver.h"

#define ym2413_channels 9

static int pending_register[MAX_2413];   /* Pending command register */

/*
Built-in instrument table
00=Original (user)      08=Organ
01=Violin               09=Horn
02=Guitar               10=Synthesizer
03=Piano                11=Harpsicord
04=Flute                12=Vibraphone
05=Clarinet             13=Synthesizer Bass
06=Oboe                 14=Acoustic bass
07=Trumpet              15=Electric Guitar
*/
int ym2413_instruments[0x13][12]=
{
     /* AM1  AM2  KS1  KS2  AD1  AD2  SR1  SR2 WS1  WS2   FB */
    {  0x21,0x01,0x72,0x04,0xf1,0x84,0x7e,0x6d,0x00,0x00,0x00 },        /* 00 User          */
    {  0x01,0x22,0x23,0x07,0xf0,0xf0,0x07,0x18,0x00,0x00,0x00 },        /* 01 Violin        */
    {  0x23,0x01,0x68,0x05,0xf2,0x74,0x6c,0x89,0x00,0x00,0x00 },        /* 02 Guitar        */
    {  0x13,0x11,0x25,0x00,0xd2,0xb2,0xf4,0xf4,0x00,0x00,0x00 },        /* 03 Piano         */
    {  0x22,0x21,0x1b,0x05,0xc0,0xa1,0x18,0x08,0x00,0x00,0x00 },        /* 04 Flute         */
    {  0x22,0x21,0x2c,0x03,0xd2,0xa1,0x18,0x57,0x00,0x00,0x00 },        /* 05 Clarinet      */
    {  0x01,0x22,0xba,0x01,0xf1,0xf1,0x1e,0x04,0x00,0x00,0x00 },        /* 06 Oboe          */
    {  0x21,0x21,0x28,0x06,0xf1,0xf1,0x6b,0x3e,0x00,0x00,0x00 },        /* 07 Trumpet       */
    {  0x27,0x21,0x60,0x00,0xf0,0xf0,0x0d,0x0f,0x00,0x00,0x00 },        /* 08 Organ         */
    {  0x20,0x21,0x2b,0x06,0x85,0xf1,0x6d,0x89,0x00,0x00,0x00 },        /* 09 Horn          */
    {  0x01,0x21,0xbf,0x02,0x53,0x62,0x5f,0xae,0x01,0x00,0x00 },        /* 10 Synthesizer   */
    {  0x23,0x21,0x70,0x07,0xd4,0xa3,0x4e,0x64,0x01,0x00,0x00 },        /* 11 Harpsicode    */
    {  0x2b,0x21,0xa4,0x07,0xf6,0x93,0x5c,0x4d,0x00,0x00,0x00 },        /* 12 Vibraphone    */
    {  0x21,0x23,0xad,0x07,0x77,0xf1,0x18,0x37,0x00,0x00,0x00 },        /* 13 SyntheBass    */
    {  0x21,0x21,0x2a,0x03,0xf3,0xe2,0x29,0x46,0x00,0x00,0x00 },	/* 14 AcousticBass  */
    {  0x21,0x23,0x37,0x03,0xf3,0xe2,0x29,0x46,0x00,0x00,0x00 },	/* 15 ElectricGuitar*/

    {  0x13,0x11,0x25,0x00,0xd7,0xb7,0xf4,0xf4,0x00,0x00,0x00 },        /* 16 Rhythm 1      */
    {  0x13,0x11,0x25,0x00,0xd7,0xb7,0xf4,0xf4,0x00,0x00,0x00 },        /* 17 Rhythm 2      */
    {  0x13,0x11,0x25,0x00,0xd7,0xb7,0xf4,0xf4,0x00,0x00,0x00 },        /* 18 Rhythm 3      */
};

INLINE void OPL_WRITE(int reg, int data)
{
        YM3812_control_port_0_w(0, reg);
        YM3812_write_port_0_w(0, data);
}

INLINE void OPL_WRITE_DATA1(int offset, int channel, int data)
{
        int order[ym2413_channels]={0x00,0x01,0x02,0x08,0x09,0x0a,0x10,0x11,0x12};
        OPL_WRITE(offset+order[channel], data);
}

INLINE void OPL_WRITE_DATA2(int offset, int channel, int data)
{
        int order[ym2413_channels]={0x03,0x04,0x05,0x0b,0x0c,0x0d,0x13,0x14,0x15};
	OPL_WRITE(offset+order[channel], data);
}

void ym2413_setinstrument(int channel, int inst)
{
    static int reg[10]=
    {
        0x20,0x20,0x40,0x40,0x60,0x60,0x80,0x80,0xe0,0xe0
    };
    int *pn=&(ym2413_instruments[inst][0]);
    int i;
    for (i=0; i<10; i++)
    {
        if (i & 0x01)
        {
            OPL_WRITE_DATA2(reg[i], channel, *pn);
        }
        else
        {
            OPL_WRITE_DATA1(reg[i], channel, *pn);
        }
        pn++;
    }

	/* I dont know wich connection type the YM2413 has */
	/* So we leave feedback to the default for now */
        /*    OPL_WRITE(0xc0+channel, ( (*pn) << 1 )); */
}

int YM2413_sh_start(struct YM2413interface *interface)
{
        int i;
        for (i=0; i<ym2413_channels; i++)
        {
                pending_register[i]=0;
        }
	return YM3812_sh_start(interface);
}

void YM2413_sh_stop(void)
{
        int i;
        /* Clear down the OPL chip (emulated or real) */
        for (i=0; i<0xf5; i++)
        {
                OPL_WRITE(i, 0);
        }

        YM3812_sh_stop();
}

void YM2413_sh_update(void)
{
	YM3812_sh_update();
}

int  YM2413_status_port_0_r(int offset)
{
	return YM3812_status_port_0_r(offset);
}

void YM2413_register_port_0_w(int chip,int data)
{
        pending_register[chip]=data;
}

void YM2413_data_port_0_w(int chip,int data)
{
        int value, channel, instrument, i;
        int pending=pending_register[chip];

        switch(pending)
        {
                case 0x00:
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                case 0x05:
                case 0x06:
                case 0x07:
                        ym2413_instruments[0][pending] = data;
                        break;

                case 0x0e: /* Rhythm control */
                        if (data & 0x20 )
                        {
                                /* We are entering rhythm control */
                                for (i=6; i<9; i++)
                                {
                                        /* Turn off key select */
                                        OPL_WRITE(0xb0+i, 0);
                                }
                                ym2413_setinstrument(6, 16);
                                ym2413_setinstrument(7, 17);
                                ym2413_setinstrument(8, 18);
                        }

                        OPL_WRITE(0xbd, data & 0x3f);
                        break;
                        /* FRQ 8 bits */
                case 0x10:
                case 0x11:
                case 0x12:
                case 0x13:
                case 0x14:
                case 0x15:
                case 0x16:
                case 0x17:
                case 0x18:
                        channel=pending-0x10;
                        OPL_WRITE(0xa0+channel, data);
                        break;
			/* MSB / note on / off */
                case 0x20:
                case 0x21:
                case 0x22:
                case 0x23:
                case 0x24:
                case 0x25:
                case 0x26:
                case 0x27:
                case 0x28:
                        channel=pending-0x20;
                        /* THIS IS WRONG !!! */
                        value = ( data & 0xfe ) << 1; /* translate key on-off & octave */
                        value |= ( data & 1 );            /* add freq MSB */
                        OPL_WRITE(0xb0+channel, value);
			break;

			/* INSTRUMENT / VOLUME */
                case 0x30:
                case 0x31:
                case 0x32:
                case 0x33:
                case 0x34:
                case 0x35:
                case 0x36:
                case 0x37:
                case 0x38:
			channel=pending-0x30;

                        /* Write instrument data */
                        instrument=(data >> 4) & 0x0f;
                        ym2413_setinstrument(channel, instrument);

                        /* Write volume (output level of operator 2) */
                        /*
                        value=( ( data & 0x0f ) << 2 );
                        value |= ym2413_instruments[channel][3] & 0xc0;
                        OPL_WRITE_DATA2(0x40, channel, value);
                        */
                        break;

                default:
                        if ( errorlog )
                                fprintf(errorlog,"YM2413: Write to register %02x\n", pending);
			break;
    }
}
