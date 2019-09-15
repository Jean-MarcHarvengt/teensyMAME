/*******************************************************************************

Data East machine functions - Bryan McPhail, mish@tendril.force9.net

* Control reads, protection chip emulations & cycle skipping patches

*******************************************************************************/

#include "driver.h"
#include "M6502/m6502.h"

extern unsigned char *dec0_ram;
static int GAME,i8751_return;

/******************************************************************************/

int dec0_controls_read(int offset)
{
	switch (offset)
	{
		case 0: /* Player 1 & 2 joystick & buttons */
			return (readinputport(0) + (readinputport(1) << 8));

		case 2: /* Credits, start buttons */
			return readinputport(2);

		case 4: /* Byte 4: Dipswitch bank 2, Byte 5: Dipswitch Bank 1 */
			return (readinputport(3) + (readinputport(4) << 8));

		case 8: /* Intel 8751 mc, Bad Dudes & Heavy Barrel only */
			return i8751_return;
	}

	if (errorlog) fprintf(errorlog,"CPU #0 PC %06x: warning - read unmapped memory address %06x\n",cpu_getpc(),0x30c000+offset);
	return 0xffff;
}

/******************************************************************************/

int dec0_rotary_read(int offset)
{
	switch (offset)
	{
		case 0: /* Player 1 rotary */
			return ~(1 << (readinputport(5) * 12 / 256));

		case 8: /* Player 2 rotary */
			return ~(1 << (readinputport(6) * 12 / 256));

		default:
			if (errorlog) fprintf(errorlog,"Unknown rotary read at 300000 %02x\n",offset);
	}

	return 0;
}

/******************************************************************************/

int midres_controls_read(int offset)
{
	switch (offset)
	{
		case 0: /* Player 1 Joystick + start, Player 2 Joystick + start */
			return (readinputport(0) + (readinputport(1) << 8));

		case 2: /* Dipswitches */
			return (readinputport(3) + (readinputport(4) << 8));

		case 4: /* Player 1 rotary */
			return ~(1 << (readinputport(5) * 12 / 256));

		case 6: /* Player 2 rotary */
			return ~(1 << (readinputport(6) * 12 / 256));

		case 8: /* Credits, start buttons */
			return readinputport(2);

		case 12:
			return 0;	/* ?? watchdog ?? */
	}

	if (errorlog) fprintf(errorlog,"PC %06x unknown control read at %02x\n",cpu_getpc(),0x180000+offset);
	return 0xffff;
}

/******************************************************************************/

int slyspy_controls_read(int offset)
{
	switch (offset)
	{
		case 0: /* Dip Switches */
			return (readinputport(3) + (readinputport(4) << 8));

		case 2: /* Player 1 & Player 2 joysticks & fire buttons */
			return (readinputport(0) + (readinputport(1) << 8));

		case 4: /* Credits */
			return readinputport(2);
	}

	if (errorlog) fprintf(errorlog,"Unknown control read at 30c000 %d\n",offset);
	return 0xffff;
}

/******************************************************************************/

#if 0
static int share[10];

int hippo_6510_intf_r(int offset)
{
if (errorlog) fprintf(errorlog,"6510 PC %06x - Read %06x\n",cpu_getpc(),offset);

	return share[offset];

	switch (offset) {
		case 4:
			return share[0];
		case 8:
			return 3;//share[1];
		case 12:
			return share[12];	//pos 8 low mem??


	}

return 0;
}


void hippo_6510_intf_w(int offset,int data)
{
	share[offset]=data;

if (errorlog) fprintf(errorlog,"6510 PC %06x - write %06x %04x\n",cpu_getpc(),offset,data);
}
#endif

static int hippodrm_protection(int offset)
{
	switch (offset) {
    case 8: return 4;
    case 0x14: return 0; /* Unknown.. */
  	case 0x1c: return 4;
  }

	if (errorlog) fprintf(errorlog,"PC %06x - Read %06x\n",cpu_getpc(),0x180000+offset);
  return 0; /* Keep zero */
}

extern void dec0_pf3_control_1_w(int offset,int data);
extern void dec0_pf3_control_0_w(int offset,int data);

extern void dec0_pf3_data_w(int offset,int data);
extern void dec0_pf2_data_w(int offset,int data);
extern void dec0_priority_w(int offset,int data);
/*
void hippodrm_prot_w(int offset,int data)
{
	share[offset/2]=data&0xff;

	if (offset==0) cpu_cause_interrupt(2,M6502_INT_IRQ);
	if (errorlog) fprintf(errorlog,"PC %06x - write %06x %04x\n",cpu_getpc(),0x180000+offset,data);

}
*/

void hippodrm_prot_w(int offset,int data)
{
	static int scroll,position,posdata;

	switch (offset) {

		case 0:
//			cpu_cause_interrupt(2,M6502_INT_IRQ);
			//break;
		case 0x14:
		//	share[4]=data&0xff;
		//	cpu_cause_interrupt(2,M6502_INT_IRQ);
			return;

		case 0x2: /* Position of floating data window */
			position=data&0xf;
			return;

		case 0x4: /* Byte 5 is MSB of window data */
			posdata=((data&0xff)<<8)+(posdata&0xff);

			if (position<4)
				dec0_pf3_control_0_w(position<<1,posdata);
			else
				dec0_pf3_control_1_w((position-4)<<1,posdata);

			break;

		case 0x6: /* Byte 7 is LSB of window data */
			posdata=(posdata&0xff00)+(data&0xff);

			if (position<4)
				dec0_pf3_control_0_w(position<<1,posdata);
			else
				dec0_pf3_control_1_w((position-4)<<1,posdata);

			return;

		case 0x8:
		case 0x16:
		//cpu_cause_interrupt(2,M6502_INT_IRQ);
		return;

		case 0x18: /* Scroll MSB, byte 0x19 */
			scroll=((data&0xff)<<8)+(scroll&0xff);
			dec0_pf3_control_1_w(0,scroll);
			return;

		case 0x1a: /* Scroll LSB, byte 0x1b */
			scroll=(scroll&0xff00)+(data&0xff);
			dec0_pf3_control_1_w(0,scroll);
			return;

	}

	if (errorlog) fprintf(errorlog,"PC %06x - write %06x %04x\n",cpu_getpc(),0x180000+offset,data);
}

/******************************************************************************/

static void hbarrel_i8751_write(int data)
{
  static int level,state;

  int title[]={  1, 2, 5, 6, 9,10,13,14,17,18,21,22,25,26,29,30,33,34,37,38,41,42,0,
                 3, 4, 7, 8,11,12,15,16,19,20,23,24,27,28,31,32,35,36,39,40,43,44,0,
                45,46,49,50,53,54,57,58,61,62,65,66,69,70,73,74,77,78,81,82,0,
                47,48,51,52,55,56,59,60,63,64,67,68,71,72,75,76,79,80,83,84,0,
                85,86,89,90,93,94,97,98,101,102,105,106,109,110,113,114,117,118,121,122,125,126,0,
                87,88,91,92,95,96,99,100,103,104,107,108,111,112,115,116,119,120,123,124,127,128,0,
                129,130,133,134,137,138,141,142,145,146,149,150,153,154,157,158,161,162,165,166,169,170,173,174,0,
                131,132,135,136,139,140,143,144,147,148,151,152,155,156,159,160,163,164,167,168,171,172,175,176,0,
                177,178,0,179,180,-1
                };

	/* Unknown, initialise the variables? */
	if (data==0xb3b || data==0x500) i8751_return=level=0;

	/* Command 0x301 is written at end of each level */
	if (data==0x301) {level++;i8751_return=level;}
//  if (data==0x301) {level=5;i8751_return=level;} /* Cheat: Go to level 5 at end of 1 */

	/* 0x200 selects level */
	if (data==0x200) i8751_return=level;

	/* Protection */
	if (data==7) i8751_return=0xc000; /* Stack pointer */
	if (data==0x175) i8751_return=0x68b; /* Protection - USA version */
	if (data==0x174) i8751_return=0x68c; /* Protection - Japan version */

	/* Related to appearance & placement of special weapons */
	if (data>0x5ff && data<0x700) i8751_return=rand()%0xfff;

	/* All commands in range 4xx are related to title screen.. */
	if (data==0x4ff) state=0;
	if (data>0x3ff && data<0x4ff) {
		state++;

		if (title[state-1]==0) i8751_return=0xfffe;
		else if (title[state-1]==-1) i8751_return=0xffff;
		else i8751_return=title[state-1]+128+15+0x2000;

		/* We have to use a state as the microcontroller remembers previous commands */
	}

//if (errorlog && data!=0x600) fprintf(errorlog,"CPU #0 PC %06x: warning - write %02x to i8751\n",cpu_getpc(),data);
}

static void baddudes_i8751_write(int data)
{
	i8751_return=0;

	switch (data&0xffff) {
		case 0x714: i8751_return=0x700; break;
		case 0x73b: i8751_return=0x701; break;
		case 0x72c: i8751_return=0x702; break;
		case 0x73f: i8751_return=0x703; break;
		case 0x755: i8751_return=0x704; break;
		case 0x722: i8751_return=0x705; break;
		case 0x72b: i8751_return=0x706; break;
		case 0x724: i8751_return=0x707; break;
		case 0x728: i8751_return=0x708; break;
		case 0x735: i8751_return=0x709; break;
		case 0x71d: i8751_return=0x70a; break;
		case 0x721: i8751_return=0x70b; break;
		case 0x73e: i8751_return=0x70c; break;
		case 0x761: i8751_return=0x70d; break;
		case 0x753: i8751_return=0x70e; break;
		case 0x75b: i8751_return=0x70f; break;
	}

	if (errorlog && !i8751_return) fprintf(errorlog,"%04x: warning - write unknown command %02x to 8571\n",cpu_getpc(),data);
}

static void *i8751_timer;

static void i8751_callback(int param)
{
	/* Signal main cpu microcontroller task is complete */
	cpu_cause_interrupt(0,5);

	timer_remove(i8751_timer);
	i8751_timer=NULL;


if (errorlog) fprintf(errorlog,"i8751:  Timer called!!!\n");

}

void dec0_i8751_write(int data)
{
	/* Writes to this address cause an IRQ to the i8751 microcontroller */
	if (GAME==1) hbarrel_i8751_write(data);
	if (GAME==2) baddudes_i8751_write(data);

	cpu_cause_interrupt(0,5);

	/* Simulate the processing time of the i8751, time value is guessed
	if (i8751_timer) {
		if (errorlog) fprintf(errorlog,"i8751:  Missed a timer!!!\n");
	}
	else
		i8751_timer = timer_set(TIME_NOW, 0, i8751_callback);*/

/* There is a timing problem in Heavy Barrel if the processing time is not
simulated - if the interrupt is triggered straight away then HB will reset
at the end of level one, the processing time needs to be at least the cycles
of a TST.W + BMI.S (ie, not very much at all).

See the code about 0xb60 (USA version)

*/

}

void dec0_i8751_reset(void)
{
	i8751_return=0;
}

/******************************************************************************/

static int hbarrel_cycle_r(int offset)
{
	if (cpu_getpc()==0x131a0) {cpu_spinuntil_int(); return 0;} return READ_WORD(&dec0_ram[0x10]);
}

static int hbarrelj_cycle_r(int offset)
{
	if (cpu_getpc()==0x130ca) {cpu_spinuntil_int(); return 0;} return READ_WORD(&dec0_ram[0x10]);
}

static int dude_skip(int offset)
{
	cpu_spinuntil_int();
	return READ_WORD(&dec0_ram[0x212]);
}

static int robocop_skip(int offset)
{
	int p=cpu_getpc();
	int result=READ_WORD(&dec0_ram[0x4]);

	if (p==0x17c8 || p==0x17f6) { cpu_spinuntil_int(); return result+1; }

	return result;
}

static int slyspy_skip(int offset)
{
	int result=READ_WORD(&dec0_ram[0x2018]);

	if (cpu_getpc()==0xe04 && !(result&0x100)) { cpu_spinuntil_int(); return (READ_WORD(&dec0_ram[0x2018]) | 0x0100); }
	return result;
}

static int midres_skip(int offset)
{
	if (cpu_getpc()==0x3a9c) cpu_spinuntil_int();
	return READ_WORD(&dec0_ram[0x207c]);
}

static void sprite_mirror(int offset, int data)
{
	extern unsigned char *spriteram;
	WRITE_WORD (&spriteram[offset], data);
}

/******************************************************************************/

static void hbarrel_custom_memory(void)
{
	install_mem_read_handler(0, 0xff8010, 0xff8011, hbarrel_cycle_r);
	GAME=1;

{ /* Remove this patch once processing time of i8751 is simulated */
unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
WRITE_WORD (&RAM[0xb68],0x8008);
}
}

static void hbarrelj_custom_memory(void)
{
	install_mem_read_handler(0, 0xff8010, 0xff8011, hbarrelj_cycle_r);
	GAME=1;
{ /* Remove this patch once processing time of i8751 is simulated */
unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
WRITE_WORD (&RAM[0xb3e],0x8008);
}
}

static void baddudes_custom_memory(void)
{
	install_mem_read_handler(0, 0xff8212, 0xff8213, dude_skip);
	GAME=2;
}

static void robocop_custom_memory(void)
{
	install_mem_read_handler(0, 0xff8004, 0xff8005, robocop_skip);
	install_mem_read_handler (0, 0x242800, 0x243fff, MRA_BANK3); /* Extra ram bank */
	install_mem_write_handler(0, 0x242800, 0x243fff, MWA_BANK3);
}

static void hippodrm_custom_memory(void)
{
//	install_mem_read_handler(0, 0xff8004, 0xff8005, robocop_skip);
	install_mem_read_handler(0, 0x180000, 0x180fff, hippodrm_protection);
	install_mem_write_handler(0, 0x180000, 0x180fff, hippodrm_prot_w);
	install_mem_write_handler(0, 0xffc800, 0xffcfff, sprite_mirror);
}

static void midres_custom_memory(void)
{
	install_mem_read_handler(0, 0x10207c, 0x10207d, midres_skip);
}

static void slyspy_custom_memory(void)
{
	install_mem_read_handler(0, 0x306018, 0x306019, slyspy_skip);
}

void dec0_custom_memory(void)
{
	GAME=0;
	i8751_timer=NULL;

	if (!strcmp(Machine->gamedrv->name,"hbarrel")) hbarrel_custom_memory();
	if (!strcmp(Machine->gamedrv->name,"hbarrelj")) hbarrelj_custom_memory();
	if (!strcmp(Machine->gamedrv->name,"baddudes")) baddudes_custom_memory();
	if (!strcmp(Machine->gamedrv->name,"drgninja")) baddudes_custom_memory();
	if (!strcmp(Machine->gamedrv->name,"robocopp")) robocop_custom_memory();
	if (!strcmp(Machine->gamedrv->name,"hippodrm")) hippodrm_custom_memory();
	if (!strcmp(Machine->gamedrv->name,"ffantasy")) hippodrm_custom_memory();
//	if (!strcmp(Machine->gamedrv->name,"slyspy")) slyspy_custom_memory();
//	if (!strcmp(Machine->gamedrv->name,"midres")) midres_custom_memory();
//	if (!strcmp(Machine->gamedrv->name,"midresj")) midres_custom_memory();
}
