/***************************************************************************
 Snow Brothers
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M68000/M68000.h"
#include "Z80/Z80.h"

static unsigned char *ram;

extern unsigned char *snowbros_spriteram;

void snowbros_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void snowbros_spriteram_w(int offset,int data);
int  snowbros_spriteram_r(int offset);



static int snowbros_interrupt(void)
{
	return cpu_getiloops() + 2;	/* IRQs 4, 3, and 2 */
}



static int snowbros_input_r (int offset)
{
	int ans = 0xff;

	switch (offset)
	{
		case 0:
			ans = (input_port_0_r (offset) << 8) + (input_port_3_r (offset));
            break;
		case 2:
			ans = (input_port_1_r (offset) << 8) + (input_port_4_r (offset));
            break;
		case 4:
			ans = input_port_2_r (offset) << 8;
            break;
	}

    return ans;
}

/* Sound Routines */

int snowbros_68000_sound_r(int offset)
{
	int ret;

	/* If the sound CPU is running, read the YM3812 status, otherwise
	   just make it pass the test */
	if (Machine->sample_rate != 0)
	{
		ret = soundlatch_r(offset);
	}
	else
	{
		ret = 3;
	}

	return ret;
}


static void snowbros_68000_sound_w(int offset, int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(1,Z80_NMI_INT);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x000000, 0x03ffff, MRA_ROM },
	{ 0x100000, 0x103fff, MRA_BANK1, &ram },
	{ 0x300000, 0x300001, snowbros_68000_sound_r },
	{ 0x500000, 0x500005, snowbros_input_r },
	{ 0x600000, 0x6001ff, paletteram_word_r },
	{ 0x700000, 0x701dff, snowbros_spriteram_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x000000, 0x03ffff, MWA_ROM },
	{ 0x100000, 0x103fff, MWA_BANK1 },
	{ 0x200000, 0x200001, watchdog_reset_w },
	{ 0x300000, 0x300001, snowbros_68000_sound_w },
//	{ 0x400000, 0x400001, snowbros_interrupt_enable_w },
	{ 0x600000, 0x6001ff, paletteram_xBBBBBGGGGGRRRRR_word_w, &paletteram },
	{ 0x700000, 0x701dff, snowbros_spriteram_w, &snowbros_spriteram, &videoram_size },
	{ 0x800000, 0x800001, MWA_NOP },	/* IRQ 4 acknowledge? */
	{ 0x900000, 0x900001, MWA_NOP },	/* IRQ 3 acknowledge? */
	{ 0xA00000, 0xA00001, MWA_NOP },	/* IRQ 2 acknowledge? */
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ -1 }  /* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x02, 0x02, YM3812_status_port_0_r },
    { 0x04, 0x04, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x02, 0x02, YM3812_control_port_0_w },
	{ 0x03, 0x03, YM3812_write_port_0_w },
    { 0x04, 0x04, soundlatch_w },	/* goes back to the main CPU, checked during boot */
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( snowbros_input_ports )
	PORT_START	/* 500001 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Must be low or game stops! */
													/* probably VBlank */

	PORT_START	/* 500003 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* 500005 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN3 )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START /* DSW 1 */
	PORT_DIPNAME( 0x01, 0x01, "Country (Affects Coinage)", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "America" )
	PORT_DIPSETTING(    0x00, "Europe" )
	PORT_DIPNAME( 0x02, 0x02, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x30, 0x30, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "2 Cn 1 Crd / 3 Cn 1 Crd" )
	PORT_DIPSETTING(    0x30, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Cn 3 Crd / 4 Cn 1 Crd" )
	PORT_DIPSETTING(    0x20, "1 Cn 2 Crd / 2 Cn 1 Crd" )
	PORT_DIPNAME( 0xc0, 0xc0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "2 Cn 1 Crd / 1 Cn 4 Crd" )
	PORT_DIPSETTING(    0xc0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x00, "2 Cn 3 Crd / 1 Cn 6 Crd" )
	PORT_DIPSETTING(    0x80, "1 Cn 2 Crd / 1 Cn 3 Crd" )

    PORT_START /* DSW 2 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "100k / each 200k " )
	PORT_DIPSETTING(    0x0c, "100k Only" )
	PORT_DIPSETTING(    0x08, "200k Only" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x80, "Yes" )
INPUT_PORTS_END



static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 sprites */
	4096,	/* 4096 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{
		0, 4, 8, 12, 16, 20, 24, 28,
		256+0, 256+4, 256+8, 256+12, 256+16, 256+20, 256+24, 256+28,
	},
	{
		0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		16*32, 17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32
	},
	128*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &tilelayout,  0, 16 },
	{ -1 } /* end of array */
};



/* handler called by the 3812 emulator when the internal timers cause an IRQ */
static void irqhandler(void)
{
	cpu_cause_interrupt(1,0xff);
}

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip (no more supported) */
	3600000,	/* 3.6 MHz ? (hand tuned) */
	{ 255 },		/* (not supported) */
	irqhandler,
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M68000,
			8000000,	/* 8 Mhz ????? */
			0,
			readmem,writemem,0,0,
			snowbros_interrupt,3
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3600000,	/* 3.6 Mhz ??? */
			2,
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			ignore_interrupt,0	/* IRQs are caused by the YM3812 */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 256,
	0,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_MODIFIES_PALETTE,
	0,
	generic_vh_start,
	generic_vh_stop,
	snowbros_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( snowbros_rom )
	ROM_REGION(0x40000)	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "sn6.bin",  0x00000, 0x20000, 0x4899ddcf )
	ROM_LOAD_ODD ( "sn5.bin",  0x00000, 0x20000, 0xad310d3f )

	ROM_REGION_DISPOSE(0x80000)
	ROM_LOAD( "ch0",          0x00000, 0x20000, 0x36d84dfe )
	ROM_LOAD( "ch1",          0x20000, 0x20000, 0x76347256 )
	ROM_LOAD( "ch2",          0x40000, 0x20000, 0xfdaa634c )
	ROM_LOAD( "ch3",          0x60000, 0x20000, 0x34024aef )

	ROM_REGION(0x10000)	/* 64k for z80 sound code */
	ROM_LOAD( "snowbros.4",   0x0000, 0x8000, 0xe6eab4e4 )
ROM_END

ROM_START( snowbro2_rom )
	ROM_REGION(0x40000)	/* 6*64k for 68000 code */
	ROM_LOAD_EVEN( "snowbros.3a",  0x00000, 0x20000, 0x10cb37e1 )
	ROM_LOAD_ODD ( "snowbros.2a",  0x00000, 0x20000, 0xab91cc1e )

	ROM_REGION_DISPOSE(0x80000)
	ROM_LOAD( "ch0",          0x00000, 0x20000, 0x36d84dfe )
	ROM_LOAD( "ch1",          0x20000, 0x20000, 0x76347256 )
	ROM_LOAD( "ch2",          0x40000, 0x20000, 0xfdaa634c )
	ROM_LOAD( "ch3",          0x60000, 0x20000, 0x34024aef )

	ROM_REGION(0x10000)	/* 64k for z80 sound code */
	ROM_LOAD( "snowbros.4",   0x0000, 0x8000, 0xe6eab4e4 )
ROM_END



static int hiload(void)
{
	void *f;

	/* check if the hi score table has already been initialized */

    if (READ_WORD(&ram[0x208]) == 0x4f)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&ram[0x01C8],64);
            osd_fread(f,&ram[0x14a2],80);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;
}

static void hisave(void)
{
	void *f;

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&ram[0x01C8],64);
        osd_fwrite(f,&ram[0x14a2],80);
		osd_fclose(f);
	}
}



struct GameDriver snowbros_driver =
{
	__FILE__,
	0,
	"snowbros",
	"Snow Bros (set 1)",
	"1990",
	"Toaplan (Romstar license)",
	"Richard Bush (Raine & Info)\nMike Coates (MAME Driver)",
	0,
	&machine_driver,
	0,

	snowbros_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	snowbros_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

struct GameDriver snowbro2_driver =
{
	__FILE__,
	&snowbros_driver,
	"snowbro2",
	"Snow Bros (set 2)",
	"1990",
	"Toaplan (Romstar license)",
	"Richard Bush (Raine & Info)\nMike Coates (MAME Driver)",
	0,
	&machine_driver,
	0,

	snowbro2_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	snowbros_input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_DEFAULT,
	hiload, hisave
};

