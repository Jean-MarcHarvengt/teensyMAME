#include "driver.h"

static unsigned char *biosbank;
//static unsigned char *memcard;
unsigned char *neogeo_ram;
unsigned char *neogeo_sram;

static int sram_locked;
static int sram_protection_hack;

int neogeo_game_fix;
extern int neogeo_red_mask,neogeo_green_mask,neogeo_blue_mask; /* From vidhrdw */

extern void install_mem_read_handler(int cpu, int start, int end, int (*handler)(int));
static void neogeo_custom_memory(void);


/* This function is called on every reset */
void neogeo_init_machine (void)
{
	int src,res;

	/* Reset variables & RAM */
	memset (neogeo_ram, 0, 0x10000);

	/* Set up machine country */
    src = readinputport(5);
    res = src&0x3;

    /* Console/arcade mode */
	if (src & 0x04)	res |= 0x8000;

	/* write the ID in the system BIOS ROM */
	WRITE_WORD(&Machine->memory_region[4][0x0400],res);
}


static void fixbadsamples(unsigned char *buf,int length)
{
	while (length >= 0x200000)
	{
		int i;


		for (i = 1;i < 0x200000;i += 2)
			if (buf[i] != 0xff) break;	/* good ROM */

		if (i >= 0x200000)
		{
if (errorlog) fprintf(errorlog,"improving bad sample ROM...\n");
			for (i = 1;i < 0x200000;i += 2)
				buf[i] = 0x80;
		}
		else
		{
			/* special case for miexchng */
			for (i = 2;i < 0x200000;i += 4)
				if (buf[i] != 0xff || buf[i+1] != 0xff) break;	/* good ROM */

			if (i >= 0x200000)
			{
if (errorlog) fprintf(errorlog,"improving bad sample ROM...\n");
				for (i = 2;i < 0x200000;i += 4)
					buf[i] = buf[i+1] = 0x80;
			}
		}

		buf += 0x200000;
		length -= 0x200000;
	}
}


/* This function is only called once per game. */
void neogeo_onetime_init_machine(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
    void *f;
	extern struct YM2610interface neogeo_ym2610_interface;

	fixbadsamples(Machine->memory_region[6],Machine->memory_region_length[6]);
	if (Machine->memory_region[7])
	{
		fixbadsamples(Machine->memory_region[7],Machine->memory_region_length[7]);

		if (errorlog) fprintf(errorlog,"using memory region 7 for Delta T samples\n");
		neogeo_ym2610_interface.pcmroma[0] = 7;
	}
	else
	{
		if (errorlog) fprintf(errorlog,"using memory region 6 for Delta T samples\n");
		neogeo_ym2610_interface.pcmroma[0] = 6;
	}

    /* Allocate ram banks */
	neogeo_ram = malloc (0x10000);
	cpu_setbank(1, neogeo_ram);

	/* Set the biosbank */
	cpu_setbank(3, Machine->memory_region[4]);

	/* Set the 2nd ROM bank */
    RAM = Machine->memory_region[0];
	if (Machine->memory_region_length[0] > 0x100000)
	{
		cpu_setbank(4, &RAM[0x100000]);
	}
	else
	{
		cpu_setbank(4, &RAM[0]);
	}

	/* Set the sound CPU ROM banks */
	RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	cpu_setbank(5,&RAM[0x08000]);
	cpu_setbank(6,&RAM[0x0c000]);
	cpu_setbank(7,&RAM[0x0e000]);
	cpu_setbank(8,&RAM[0x0f000]);

	/* Allocate and point to the memcard - bank 5 */
 //	memcard = calloc (0x1000, 1);
 //	cpu_setbank(2, memcard);


	RAM = Machine->memory_region[4];

	/* Remove memory check for now */
    WRITE_WORD(&RAM[0x11b00],0x4e71);
    WRITE_WORD(&RAM[0x11b02],0x4e71);
    WRITE_WORD(&RAM[0x11b16],0x4ef9);
    WRITE_WORD(&RAM[0x11b18],0x00c1);
    WRITE_WORD(&RAM[0x11b1a],0x1b6a);

	/* Patch bios rom, for Calendar errors */
    WRITE_WORD(&RAM[0x11c14],0x4e71);
    WRITE_WORD(&RAM[0x11c16],0x4e71);
    WRITE_WORD(&RAM[0x11c1c],0x4e71);
    WRITE_WORD(&RAM[0x11c1e],0x4e71);

    /* Rom internal checksum fails for now.. */
    WRITE_WORD(&RAM[0x11c62],0x4e71);
    WRITE_WORD(&RAM[0x11c64],0x4e71);

	/* Install custom memory handlers */
	neogeo_custom_memory();
}

/******************************************************************************/

static int bios_cycle_skip_r(int offset)
{
	cpu_spinuntil_int();
	return 0;
}

/******************************************************************************/
/* Routines to speed up the main processor 				      */
/******************************************************************************/
#define NEO_CYCLE_R(name,pc,hit,other) static int name##_cycle_r(int offset) {	if (cpu_getpc()==pc) {cpu_spinuntil_int(); return hit;} return other;}
#define NEO_CYCLE_RX(name,pc,hit,other,xxx) static int name##_cycle_r(int offset) {	if (cpu_getpc()==pc) {if(other==xxx) cpu_spinuntil_int(); return hit;} return other;}

NEO_CYCLE_R(puzzledp,0x12f2,1,								READ_WORD(&neogeo_ram[0x0000]))
NEO_CYCLE_R(samsho4, 0xaffc,0,								READ_WORD(&neogeo_ram[0x830c]))
NEO_CYCLE_R(karnov_r,0x5b56,0,								READ_WORD(&neogeo_ram[0x3466]))
NEO_CYCLE_R(wjammers,0x1362e,READ_WORD(&neogeo_ram[0x5a])&0x7fff,READ_WORD(&neogeo_ram[0x005a]))
NEO_CYCLE_R(strhoops,0x029a,0,								READ_WORD(&neogeo_ram[0x1200]))
//NEO_CYCLE_R(magdrop3,0xa378,READ_WORD(&neogeo_ram[0x60])&0x7fff,READ_WORD(&neogeo_ram[0x0060]))
NEO_CYCLE_R(neobombe,0x09f2,0xffff,							READ_WORD(&neogeo_ram[0x448c]))
NEO_CYCLE_R(trally, 0x1295c,READ_WORD(&neogeo_ram[0x206])-1,READ_WORD(&neogeo_ram[0x0206]))
//NEO_CYCLE_R(joyjoy,  0x122c,0xffff,							READ_WORD(&neogeo_ram[0x0554]))
NEO_CYCLE_RX(blazstar,0x3b62,0xffff,							READ_WORD(&neogeo_ram[0x1000]),0)
NEO_CYCLE_R(ridhero, 0xedb0,0,								READ_WORD(&neogeo_ram[0x00ca]))
NEO_CYCLE_R(cyberlip,0x2218,0x0f0f,							READ_WORD(&neogeo_ram[0x7bb4]))
NEO_CYCLE_R(lbowling,0x37b0,0,								READ_WORD(&neogeo_ram[0x0098]))
NEO_CYCLE_R(superspy,0x07ca,0xffff,							READ_WORD(&neogeo_ram[0x108c]))
NEO_CYCLE_R(ttbb,    0x0a58,0xffff,							READ_WORD(&neogeo_ram[0x000e]))
NEO_CYCLE_R(alpham2,0x076e,0xffff,							READ_WORD(&neogeo_ram[0xe2fe]))
NEO_CYCLE_R(eightman,0x12fa,0xffff,							READ_WORD(&neogeo_ram[0x046e]))
NEO_CYCLE_R(ararmy,  0x08e8,0,								READ_WORD(&neogeo_ram[0x4010]))
NEO_CYCLE_R(fatfury1,0x133c,0,								READ_WORD(&neogeo_ram[0x4282]))
NEO_CYCLE_R(burningf,0x0736,0xffff,							READ_WORD(&neogeo_ram[0x000e]))
NEO_CYCLE_R(bstars,  0x133c,0,								READ_WORD(&neogeo_ram[0x000a]))
NEO_CYCLE_R(kingofm, 0x1284,0,								READ_WORD(&neogeo_ram[0x0020]))
NEO_CYCLE_R(gpilots, 0x0474,0,								READ_WORD(&neogeo_ram[0xa682]))
NEO_CYCLE_R(lresort, 0x256a,0,								READ_WORD(&neogeo_ram[0x4102]))
NEO_CYCLE_R(fbfrenzy,0x07dc,0,								READ_WORD(&neogeo_ram[0x0020]))
NEO_CYCLE_R(socbrawl,0xa8dc,0xffff,							READ_WORD(&neogeo_ram[0xb20c]))
NEO_CYCLE_R(mutnat,  0x1456,0,								READ_WORD(&neogeo_ram[0x1042]))
NEO_CYCLE_R(artfight,0x6798,0,								READ_WORD(&neogeo_ram[0x8100]))
//NEO_CYCLE_R(countb,  0x16a2,0,								READ_WORD(&neogeo_ram[0x8002]))
NEO_CYCLE_R(ncombat, 0xcb3e,0,								READ_WORD(&neogeo_ram[0x0206]))
NEO_CYCLE_R(sengoku, 0x12f4,0,								READ_WORD(&neogeo_ram[0x0088]))
//NEO_CYCLE_R(ncommand,0x11b44,0,								READ_WORD(&neogeo_ram[0x8206]))
NEO_CYCLE_R(wheroes, 0xf62d4,0xffff,						READ_WORD(&neogeo_ram[0x8206]))
NEO_CYCLE_R(androdun,0x26d6,0xffff,							READ_WORD(&neogeo_ram[0x0080]))
NEO_CYCLE_R(bjourney,0xe8aa,READ_WORD(&neogeo_ram[0x206])+1,READ_WORD(&neogeo_ram[0x0206]))
NEO_CYCLE_R(maglord, 0xb16a,READ_WORD(&neogeo_ram[0x206])+1,READ_WORD(&neogeo_ram[0x0206]))
//NEO_CYCLE_R(janshin, 0x06a0,0,								READ_WORD(&neogeo_ram[0x0026]))
NEO_CYCLE_RX(pulstar, 0x2052,0xffff,							READ_WORD(&neogeo_ram[0x1000]),0)
//NEO_CYCLE_R(mslug   ,0x200a,0xffff,							READ_WORD(&neogeo_ram[0x6ed8]))
NEO_CYCLE_R(neodrift,0x0b76,0xffff,							READ_WORD(&neogeo_ram[0x0424]))
NEO_CYCLE_R(spinmast,0x00f6,READ_WORD(&neogeo_ram[0xf0])+1,	READ_WORD(&neogeo_ram[0x00f0]))
NEO_CYCLE_R(sonicwi2,0x1e6c8,0xffff,						READ_WORD(&neogeo_ram[0xe5b6]))
NEO_CYCLE_R(sonicwi3,0x20bac,0xffff,						READ_WORD(&neogeo_ram[0xea2e]))
NEO_CYCLE_R(goalx3,  0x5298,READ_WORD(&neogeo_ram[0x6])+1,	READ_WORD(&neogeo_ram[0x0006]))
NEO_CYCLE_R(turfmast,0xd5a8,0xffff,							READ_WORD(&neogeo_ram[0x2e54]))
NEO_CYCLE_R(kabukikl,0x10b0,0,								READ_WORD(&neogeo_ram[0x428a]))
NEO_CYCLE_R(panicbom,0x3ee6,0xffff,							READ_WORD(&neogeo_ram[0x009c]))
NEO_CYCLE_R(worlher2,0x2063fc,READ_WORD(&neogeo_ram[0x8206])+1,READ_WORD(&neogeo_ram[0x8206]))
NEO_CYCLE_R(worlhe2j,0x109f4,READ_WORD(&neogeo_ram[0x8206])+1,READ_WORD(&neogeo_ram[0x8206]))
NEO_CYCLE_R(aodk,    0xea62,READ_WORD(&neogeo_ram[0x8206])+1,READ_WORD(&neogeo_ram[0x8206]))
NEO_CYCLE_R(whp,     0xeace,READ_WORD(&neogeo_ram[0x8206])+1,READ_WORD(&neogeo_ram[0x8206]))
NEO_CYCLE_R(overtop, 0x1736,READ_WORD(&neogeo_ram[0x8202])+1,READ_WORD(&neogeo_ram[0x8202]))
NEO_CYCLE_R(twinspri,0x492e,READ_WORD(&neogeo_ram[0x8206])+1,READ_WORD(&neogeo_ram[0x8206]))
NEO_CYCLE_R(stakwin, 0x0596,0xffff,							READ_WORD(&neogeo_ram[0x0b92]))
NEO_CYCLE_R(shocktro,0xdd28,0,								READ_WORD(&neogeo_ram[0x8344]))
NEO_CYCLE_R(tws96,   0x17f4,0xffff,							READ_WORD(&neogeo_ram[0x010e]))
static int zedblade_cycle_r(int offset)
{
	int pc=cpu_getpc();
	if (pc==0xa2fa || pc==0xa2a0 || pc==0xa2ce || pc==0xa396 || pc==0xa3fa) {cpu_spinuntil_int(); return 0;}
	return READ_WORD(&neogeo_ram[0x9004]);
}
//NEO_CYCLE_R(doubledr,0x3574,0,								READ_WORD(&neogeo_ram[0x1c30]))
NEO_CYCLE_R(galaxyfg,0x09ea,READ_WORD(&neogeo_ram[0x1858])+1,READ_WORD(&neogeo_ram[0x1858]))
NEO_CYCLE_R(wakuwak7,0x1a3c,READ_WORD(&neogeo_ram[0x0bd4])+1,READ_WORD(&neogeo_ram[0x0bd4]))
static int mahretsu_cycle_r(int offset)
{
	int pc=cpu_getpc();
	if (pc==0x1580 || pc==0xf3ba ) {cpu_spinuntil_int(); return 0;}
	return READ_WORD(&neogeo_ram[0x13b2]);
}
NEO_CYCLE_R(nam_1975,0x0a1c,0xffff,							READ_WORD(&neogeo_ram[0x12e0]))
NEO_CYCLE_R(tpgolf,  0x105c,0,								READ_WORD(&neogeo_ram[0x00a4]))
NEO_CYCLE_R(legendos,0x1864,0xffff,							READ_WORD(&neogeo_ram[0x0002]))
//NEO_CYCLE_R(viewpoin,0x0c16,0,								READ_WORD(&neogeo_ram[0x1216]))
NEO_CYCLE_R(fatfury2,0x10ea,0,								READ_WORD(&neogeo_ram[0x418c]))
NEO_CYCLE_R(bstars2, 0x7e30,0xffff,							READ_WORD(&neogeo_ram[0x001c]))
NEO_CYCLE_R(sidkicks,0x20b0,0xffff,							READ_WORD(&neogeo_ram[0x8c84]))
NEO_CYCLE_R(kotm2,   0x045a,0,								READ_WORD(&neogeo_ram[0x1000]))
static int samsho_cycle_r(int offset)
{
	int pc=cpu_getpc();
	if (pc==0x3580 || pc==0x0f84 ) {cpu_spinuntil_int(); return 0xffff;}
	return READ_WORD(&neogeo_ram[0x0a76]);
}
NEO_CYCLE_R(fatfursp,0x10da,0,								READ_WORD(&neogeo_ram[0x418c]))
NEO_CYCLE_R(fatfury3,0x9c50,0,								READ_WORD(&neogeo_ram[0x418c]))
NEO_CYCLE_R(tophuntr,0x0ce0,0xffff,							READ_WORD(&neogeo_ram[0x008e]))
NEO_CYCLE_R(savagere,0x056e,0,								READ_WORD(&neogeo_ram[0x8404]))
NEO_CYCLE_R(aof2,    0x8c74,0,								READ_WORD(&neogeo_ram[0x8280]))
NEO_CYCLE_R(ssideki2,0x7850,0xffff,							READ_WORD(&neogeo_ram[0x4292]))
NEO_CYCLE_R(samsho2, 0x1432,0xffff,							READ_WORD(&neogeo_ram[0x0a30]))
NEO_CYCLE_R(samsho3, 0x0858,0,								READ_WORD(&neogeo_ram[0x8408]))
NEO_CYCLE_R(kof95,   0x39474,0xffff,						READ_WORD(&neogeo_ram[0xa784]))
NEO_CYCLE_R(rbff1,   0x80a2,0,								READ_WORD(&neogeo_ram[0x418c]))
//NEO_CYCLE_R(aof3,    0x15d6,0,								READ_WORD(&neogeo_ram[0x4ee8]))
NEO_CYCLE_R(ninjamas,0x2436,READ_WORD(&neogeo_ram[0x8206])+1,READ_WORD(&neogeo_ram[0x8206]))
NEO_CYCLE_R(kof96,   0x8fc4,0xffff,							READ_WORD(&neogeo_ram[0xa782]))
NEO_CYCLE_R(rbffspec,0x8704,0,								READ_WORD(&neogeo_ram[0x418c]))
NEO_CYCLE_R(kizuna,  0x0840,0,								READ_WORD(&neogeo_ram[0x8808]))
NEO_CYCLE_R(kof97,   0x9c54,0xffff,							READ_WORD(&neogeo_ram[0xa784]))
//NEO_CYCLE_R(mslug2,  0x1656,0xffff,						READ_WORD(&neogeo_ram[0x008c]))
NEO_CYCLE_R(realbou2,0xc5d0,0,								READ_WORD(&neogeo_ram[0x418c]))
NEO_CYCLE_R(ragnagrd,0xc6c0,0,								READ_WORD(&neogeo_ram[0x0042]))
NEO_CYCLE_R(lastblad,0x1868,0xffff,							READ_WORD(&neogeo_ram[0x9d4e]))
NEO_CYCLE_R(gururin, 0x0604,0xffff,							READ_WORD(&neogeo_ram[0x1002]))
//NEO_CYCLE_R(magdrop2,0x1cf3a,0,								READ_WORD(&neogeo_ram[0x0064]))
//NEO_CYCLE_R(miexchng,0x,,READ_WORD(&neogeo_ram[0x]))

/******************************************************************************/
/* Routines to speed up the sound processor AVDB 24-10-1998		      */
/******************************************************************************/

/*
 *	Sound V3.0
 *
 *	Used by puzzle de pon and Super Sidekicks 2
 *
 */
static int cycle_v3_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0x0137) {
		cpu_spinuntil_int();
		return RAM[0xfeb1];
	}
	return RAM[0xfeb1];
}

/*
 *	Also sound revision no 3.0, but different types.
 */
static int sidkicks_cycle_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0x015A) {
		cpu_spinuntil_int();
		return RAM[0xfef3];
	}
	return RAM[0xfef3];
}

static int artfight_cycle_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0x0143) {
		cpu_spinuntil_int();
		return RAM[0xfef3];
	}
	return RAM[0xfef3];
}

/*
 *	Sound V2.0
 *
 *	Used by puzzle Bobble and Goal Goal Goal
 *
 */

static int cycle_v2_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0x0143) {
		cpu_spinuntil_int();
		return RAM[0xfeef];
	}
	return RAM[0xfeef];
}

static int vwpoint_cycle_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0x0143) {
		cpu_spinuntil_int();
		return RAM[0xfe46];
	}
	return RAM[0xfe46];
}

/*
 *	Sound revision no 1.5, and some 2.0 versions,
 *	are not fit for speedups, it results in sound drops !
 *	Games that use this one are : Ghost Pilots, Joy Joy, Nam 1975
 */

/*
static int cycle_v15_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0x013D) {
		cpu_spinuntil_int();
		return RAM[0xfe46];
	}
	return RAM[0xfe46];
}
*/

/*
 *	Magician Lord uses a different sound core from all other
 *	Neo Geo Games.
 */

static int maglord_cycle_sr(int offset)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];

	if (cpu_getpc()==0xd487) {
		cpu_spinuntil_int();
		return RAM[0xfb91];
	}
	return RAM[0xfb91];
}

/******************************************************************************/

static void neogeo_custom_memory(void)
{
	/* NeoGeo intro screen cycle skip, used by all games */
//	install_mem_read_handler(0, 0x10fe8c, 0x10fe8d, bios_cycle_skip_r);

    /* Individual games can go here... */

#if 1
//	if (!strcmp(Machine->gamedrv->name,"joyjoy"))   install_mem_read_handler(0, 0x100554, 0x100555, joyjoy_cycle_r);	// Slower
	if (!strcmp(Machine->gamedrv->name,"ridhero"))  install_mem_read_handler(0, 0x1000ca, 0x1000cb, ridhero_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"bstars"))   install_mem_read_handler(0, 0x10000a, 0x10000b, bstars_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"cyberlip")) install_mem_read_handler(0, 0x107bb4, 0x107bb4, cyberlip_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"lbowling")) install_mem_read_handler(0, 0x100098, 0x100099, lbowling_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"superspy")) install_mem_read_handler(0, 0x10108c, 0x10108d, superspy_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"ttbb"))     install_mem_read_handler(0, 0x10000e, 0x10000f, ttbb_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"alpham2"))  install_mem_read_handler(0, 0x10e2fe, 0x10e2ff, alpham2_cycle_r);	// Very little increase.
	if (!strcmp(Machine->gamedrv->name,"eightman")) install_mem_read_handler(0, 0x10046e, 0x10046f, eightman_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"ararmy"))   install_mem_read_handler(0, 0x104010, 0x104011, ararmy_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"fatfury1")) install_mem_read_handler(0, 0x104282, 0x104283, fatfury1_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"burningf")) install_mem_read_handler(0, 0x10000e, 0x10000f, burningf_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"kingofm"))  install_mem_read_handler(0, 0x100020, 0x100021, kingofm_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"gpilots"))  install_mem_read_handler(0, 0x10a682, 0x10a683, gpilots_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"lresort"))  install_mem_read_handler(0, 0x104102, 0x104103, lresort_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"fbfrenzy")) install_mem_read_handler(0, 0x100020, 0x100021, fbfrenzy_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"socbrawl")) install_mem_read_handler(0, 0x10b20c, 0x10b20d, socbrawl_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"mutnat"))   install_mem_read_handler(0, 0x101042, 0x101043, mutnat_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"artfight")) install_mem_read_handler(0, 0x108100, 0x108101, artfight_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"countb"))   install_mem_read_handler(0, 0x108002, 0x108003, countb_cycle_r);   // doesn't seem to speed it up.
	if (!strcmp(Machine->gamedrv->name,"ncombat"))  install_mem_read_handler(0, 0x100206, 0x100207, ncombat_cycle_r);
//**	if (!strcmp(Machine->gamedrv->name,"crsword"))  install_mem_read_handler(0, 0x10, 0x10, crsword_cycle_r);			// Can't find this one :-(
	if (!strcmp(Machine->gamedrv->name,"trally"))   install_mem_read_handler(0, 0x100206, 0x100207, trally_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"sengoku"))  install_mem_read_handler(0, 0x100088, 0x100089, sengoku_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"ncommand")) install_mem_read_handler(0, 0x108206, 0x108207, ncommand_cycle_r);	// Slower
	if (!strcmp(Machine->gamedrv->name,"wheroes"))  install_mem_read_handler(0, 0x108206, 0x108207, wheroes_cycle_r);
//**	if (!strcmp(Machine->gamedrv->name,"sengoku2")) install_mem_read_handler(0, 0x10, 0x10, sengoku2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"androdun")) install_mem_read_handler(0, 0x100080, 0x100081, androdun_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"bjourney")) install_mem_read_handler(0, 0x100206, 0x100207, bjourney_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"maglord"))  install_mem_read_handler(0, 0x100206, 0x100207, maglord_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"janshin"))  install_mem_read_handler(0, 0x100026, 0x100027, janshin_cycle_r);	// No speed difference
	if (!strcmp(Machine->gamedrv->name,"pulstar"))  install_mem_read_handler(0, 0x101000, 0x101001, pulstar_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"blazstar")) install_mem_read_handler(0, 0x101000, 0x101001, blazstar_cycle_r);
//**	if (!strcmp(Machine->gamedrv->name,"pbobble"))  install_mem_read_handler(0, 0x10, 0x10, pbobble_cycle_r);		// Can't find this one :-(
	if (!strcmp(Machine->gamedrv->name,"puzzledp")) install_mem_read_handler(0, 0x100000, 0x100001, puzzledp_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"neodrift")) install_mem_read_handler(0, 0x100424, 0x100425, neodrift_cycle_r);
//**	if (!strcmp(Machine->gamedrv->name,"neomrdo"))  install_mem_read_handler(0, 0x10, 0x10, neomrdo_cycle_r);		// Can't find this one :-(
	if (!strcmp(Machine->gamedrv->name,"spinmast")) install_mem_read_handler(0, 0x100050, 0x100051, spinmast_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"karnov_r")) install_mem_read_handler(0, 0x103466, 0x103467, karnov_r_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"wjammers")) install_mem_read_handler(0, 0x10005a, 0x10005b, wjammers_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"strhoops")) install_mem_read_handler(0, 0x101200, 0x101201, strhoops_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"magdrop3")) install_mem_read_handler(0, 0x100060, 0x100061, magdrop3_cycle_r);	// The game starts glitching.
//**	if (!strcmp(Machine->gamedrv->name,"pspikes2")) install_mem_read_handler(0, 0x10, 0x10, pspikes2_cycle_r);		// Can't find this one :-(
	if (!strcmp(Machine->gamedrv->name,"sonicwi2")) install_mem_read_handler(0, 0x10e5b6, 0x10e5b7, sonicwi2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"sonicwi3")) install_mem_read_handler(0, 0x10ea2e, 0x10ea2f, sonicwi3_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"goalx3"))   install_mem_read_handler(0, 0x100006, 0x100007, goalx3_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"mslug"))    install_mem_read_handler(0, 0x106ed8, 0x106ed9, mslug_cycle_r);		// Doesn't work properly.
	if (!strcmp(Machine->gamedrv->name,"turfmast")) install_mem_read_handler(0, 0x102e54, 0x102e55, turfmast_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"kabukikl")) install_mem_read_handler(0, 0x10428a, 0x10428b, kabukikl_cycle_r);

	if (!strcmp(Machine->gamedrv->name,"panicbom")) install_mem_read_handler(0, 0x10009c, 0x10009d, panicbom_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"neobombe")) install_mem_read_handler(0, 0x10448c, 0x10448d, neobombe_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"worlher2")) install_mem_read_handler(0, 0x108206, 0x108207, worlher2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"worlhe2j")) install_mem_read_handler(0, 0x108206, 0x108207, worlhe2j_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"aodk"))     install_mem_read_handler(0, 0x108206, 0x108207, aodk_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"whp"))      install_mem_read_handler(0, 0x108206, 0x108207, whp_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"overtop"))  install_mem_read_handler(0, 0x108202, 0x108203, overtop_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"twinspri")) install_mem_read_handler(0, 0x108206, 0x108207, twinspri_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"stakwin"))  install_mem_read_handler(0, 0x100b92, 0x100b93, stakwin_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"shocktro")) install_mem_read_handler(0, 0x108344, 0x108345, shocktro_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"tws96"))    install_mem_read_handler(0, 0x10010e, 0x10010f, tws96_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"zedblade")) install_mem_read_handler(0, 0x109004, 0x109005, zedblade_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"doubledr")) install_mem_read_handler(0, 0x101c30, 0x101c31, doubledr_cycle_r);
//**	if (!strcmp(Machine->gamedrv->name,"gowcaizr")) install_mem_read_handler(0, 0x10, 0x10, gowcaizr_cycle_r);		// Can't find this one :-(
	if (!strcmp(Machine->gamedrv->name,"galaxyfg")) install_mem_read_handler(0, 0x101858, 0x101859, galaxyfg_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"wakuwak7")) install_mem_read_handler(0, 0x100bd4, 0x100bd5, wakuwak7_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"mahretsu")) install_mem_read_handler(0, 0x1013b2, 0x1013b3, mahretsu_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"nam_1975")) install_mem_read_handler(0, 0x1012e0, 0x1012e1, nam_1975_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"tpgolf"))   install_mem_read_handler(0, 0x1000a4, 0x1000a5, tpgolf_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"legendos")) install_mem_read_handler(0, 0x100002, 0x100003, legendos_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"viewpoin")) install_mem_read_handler(0, 0x101216, 0x101217, viewpoin_cycle_r);	// Doesn't work
	if (!strcmp(Machine->gamedrv->name,"fatfury2")) install_mem_read_handler(0, 0x10418c, 0x10418d, fatfury2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"bstars2"))  install_mem_read_handler(0, 0x10001c, 0x10001c, bstars2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"sidkicks")) install_mem_read_handler(0, 0x108c84, 0x108c85, sidkicks_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"kotm2"))    install_mem_read_handler(0, 0x101000, 0x101001, kotm2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"samsho"))   install_mem_read_handler(0, 0x100a76, 0x100a77, samsho_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"fatfursp")) install_mem_read_handler(0, 0x10418c, 0x10418d, fatfursp_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"fatfury3")) install_mem_read_handler(0, 0x10418c, 0x10418d, fatfury3_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"tophuntr")) install_mem_read_handler(0, 0x10008e, 0x10008f, tophuntr_cycle_r);	// Can't test this at the moment, it crashes.
	if (!strcmp(Machine->gamedrv->name,"savagere")) install_mem_read_handler(0, 0x108404, 0x108405, savagere_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"kof94"))    install_mem_read_handler(0, 0x10, 0x10, kof94_cycle_r);				// Can't do this I think. There seems to be too much code in the idle loop.
	if (!strcmp(Machine->gamedrv->name,"aof2"))     install_mem_read_handler(0, 0x108280, 0x108281, aof2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"ssideki2")) install_mem_read_handler(0, 0x104292, 0x104293, ssideki2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"samsho2"))  install_mem_read_handler(0, 0x100a30, 0x100a31, samsho2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"samsho3"))  install_mem_read_handler(0, 0x108408, 0x108409, samsho3_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"kof95"))    install_mem_read_handler(0, 0x10a784, 0x10a785, kof95_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"rbff1"))    install_mem_read_handler(0, 0x10418c, 0x10418d, rbff1_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"aof3"))     install_mem_read_handler(0, 0x104ee8, 0x104ee9, aof3_cycle_r);		// Doesn't work properly.
	if (!strcmp(Machine->gamedrv->name,"ninjamas")) install_mem_read_handler(0, 0x108206, 0x108207, ninjamas_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"kof96"))    install_mem_read_handler(0, 0x10a782, 0x10a783, kof96_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"samsho4"))  install_mem_read_handler(0, 0x10830c, 0x10830d, samsho4_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"rbffspec")) install_mem_read_handler(0, 0x10418c, 0x10418d, rbffspec_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"kizuna"))   install_mem_read_handler(0, 0x108808, 0x108809, kizuna_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"kof97"))    install_mem_read_handler(0, 0x10a784, 0x10a785, kof97_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"mslug2"))   install_mem_read_handler(0, 0x10008c, 0x10008d, mslug2_cycle_r);	// Breaks the game
	if (!strcmp(Machine->gamedrv->name,"realbou2")) install_mem_read_handler(0, 0x10418c, 0x10418d, realbou2_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"ragnagrd")) install_mem_read_handler(0, 0x100042, 0x100043, ragnagrd_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"lastblad")) install_mem_read_handler(0, 0x109d4e, 0x109d4f, lastblad_cycle_r);
	if (!strcmp(Machine->gamedrv->name,"gururin"))  install_mem_read_handler(0, 0x101002, 0x101003, gururin_cycle_r);
//	if (!strcmp(Machine->gamedrv->name,"magdrop2")) install_mem_read_handler(0, 0x100064, 0x100065, magdrop2_cycle_r);	// Graphic Glitches
//	if (!strcmp(Machine->gamedrv->name,"miexchng")) install_mem_read_handler(0, 0x10, 0x10, miexchng_cycle_r);			// Can't do this.

//	if (!strcmp(Machine->gamedrv->name,"")) install_mem_read_handler(0, 0x10, 0x10, _cycle_r);
#endif

	/* AVDB cpu spins based on sound processor status */
	if (!strcmp(Machine->gamedrv->name,"puzzledp")) install_mem_read_handler(1, 0xfeb1, 0xfeb1, cycle_v3_sr);
	if (!strcmp(Machine->gamedrv->name,"ssideki2")) install_mem_read_handler(1, 0xfeb1, 0xfeb1, cycle_v3_sr);

	if (!strcmp(Machine->gamedrv->name,"sidkicks")) install_mem_read_handler(1, 0xfef3, 0xfef3, sidkicks_cycle_sr);
	if (!strcmp(Machine->gamedrv->name,"artfight")) install_mem_read_handler(1, 0xfef3, 0xfef3, artfight_cycle_sr);

	if (!strcmp(Machine->gamedrv->name,"pbobble")) install_mem_read_handler(1, 0xfeef, 0xfeef, cycle_v2_sr);
	if (!strcmp(Machine->gamedrv->name,"goalx3")) install_mem_read_handler(1, 0xfeef, 0xfeef, cycle_v2_sr);
	if (!strcmp(Machine->gamedrv->name,"fatfury1")) install_mem_read_handler(1, 0xfeef, 0xfeef, cycle_v2_sr);
	if (!strcmp(Machine->gamedrv->name,"mutnat")) install_mem_read_handler(1, 0xfeef, 0xfeef, cycle_v2_sr);

	if (!strcmp(Machine->gamedrv->name,"maglord")) install_mem_read_handler(1, 0xfb91, 0xfb91, maglord_cycle_sr);
	if (!strcmp(Machine->gamedrv->name,"vwpoint")) install_mem_read_handler(1, 0xfe46, 0xfe46, vwpoint_cycle_sr);

//	if (!strcmp(Machine->gamedrv->name,"joyjoy")) install_mem_read_handler(1, 0xfe46, 0xfe46, cycle_v15_sr);
//	if (!strcmp(Machine->gamedrv->name,"nam_1975")) install_mem_read_handler(1, 0xfe46, 0xfe46, cycle_v15_sr);
//	if (!strcmp(Machine->gamedrv->name,"gpilots")) install_mem_read_handler(1, 0xfe46, 0xfe46, cycle_v15_sr);

	/* kludges */
	neogeo_game_fix=-1;
	if (!strcmp(Machine->gamedrv->name,"blazstar")) neogeo_game_fix=0;
	if (!strcmp(Machine->gamedrv->name,"gowcaizr")) neogeo_game_fix=1;
	if (!strcmp(Machine->gamedrv->name,"realbou2")) neogeo_game_fix=2;
	if (!strcmp(Machine->gamedrv->name,"samsho3")) neogeo_game_fix=3;
	if (!strcmp(Machine->gamedrv->name,"overtop")) neogeo_game_fix=4;
	if (!strcmp(Machine->gamedrv->name,"kof97")) neogeo_game_fix=5;
	if (!strcmp(Machine->gamedrv->name,"miexchng")) neogeo_game_fix=6;
	if (!strcmp(Machine->gamedrv->name,"gururin")) neogeo_game_fix=7;
	if (!strcmp(Machine->gamedrv->name,"ncommand")) neogeo_game_fix=8;

	/* hacks to make the games which do protection checks run in arcade mode */
	/* we write protect a SRAM location so it cannot be set to 1 */
	sram_protection_hack = -1;
	if (!strcmp(Machine->gamedrv->name,"fatfury3") ||
			 !strcmp(Machine->gamedrv->name,"samsho3") ||
			 !strcmp(Machine->gamedrv->name,"samsho4") ||
			 !strcmp(Machine->gamedrv->name,"aof3") ||
			 !strcmp(Machine->gamedrv->name,"rbff1") ||
			 !strcmp(Machine->gamedrv->name,"rbffspec") ||
			 !strcmp(Machine->gamedrv->name,"kof95") ||
			 !strcmp(Machine->gamedrv->name,"kof96") ||
			 !strcmp(Machine->gamedrv->name,"kof97") ||
			 !strcmp(Machine->gamedrv->name,"kizuna") ||
			 !strcmp(Machine->gamedrv->name,"lastblad") ||
			 !strcmp(Machine->gamedrv->name,"realbou2") ||
			 !strcmp(Machine->gamedrv->name,"mslug2"))
		sram_protection_hack = 0x100;

	if (!strcmp(Machine->gamedrv->name,"pulstar"))
		sram_protection_hack = 0x35a;

	/* Improve games which don't work in 8 bit colour very well */
    neogeo_red_mask=0xf;
    neogeo_green_mask=0xf;
	neogeo_blue_mask=0xf;
	if (!strcmp(Machine->gamedrv->name,"blazstar")) neogeo_blue_mask=0xe;
	if (!strcmp(Machine->gamedrv->name,"karnov_r")) neogeo_blue_mask=0xe;
	if (!strcmp(Machine->gamedrv->name,"kof95")) neogeo_blue_mask=0xe;
	if (!strcmp(Machine->gamedrv->name,"kof96")) neogeo_blue_mask=neogeo_red_mask=0xe;
	if (!strcmp(Machine->gamedrv->name,"kof97")) neogeo_blue_mask=neogeo_red_mask=neogeo_green_mask=0xe;
	if (!strcmp(Machine->gamedrv->name,"ragnagrd")) neogeo_blue_mask=neogeo_red_mask=neogeo_green_mask=0xe;
	if (!strcmp(Machine->gamedrv->name,"whp")) neogeo_blue_mask=neogeo_red_mask=0xe;
}



void neogeo_sram_lock_w(int offset,int data)
{
	sram_locked = 1;
}

void neogeo_sram_unlock_w(int offset,int data)
{
	sram_locked = 0;
}

int neogeo_sram_r(int offset)
{
	return READ_WORD(&neogeo_sram[offset]);
}

void neogeo_sram_w(int offset,int data)
{
	if (sram_locked)
	{
if (errorlog) fprintf(errorlog,"PC %06x: warning: write %02x to SRAM %04x while it was protected\n",cpu_getpc(),data,offset);
	}
	else
	{
		if (offset == sram_protection_hack)
		{
			if (data == 0x0001 || data == 0xff000001)
				return;	/* fake protection pass */
		}

		COMBINE_WORD_MEM(&neogeo_sram[offset],data);
	}
}

int neogeo_sram_load(void)
{
	void *f;


    /* Load the SRAM settings for this game */
	memset(neogeo_sram,0,0x10000);
	f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0);
	if (f)
	{
		osd_fread_msbfirst(f,neogeo_sram,0x2000);
		osd_fclose(f);
	}

	return 1;
}

void neogeo_sram_save(void)
{
    void *f;


    /* Save the SRAM settings */
	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite_msbfirst(f,neogeo_sram,0x2000);
		osd_fclose(f);
	}
}
