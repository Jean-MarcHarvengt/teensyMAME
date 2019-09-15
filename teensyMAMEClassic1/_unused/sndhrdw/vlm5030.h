#ifndef VLM5030_h
#define VLM5030_h

struct VLM5030interface
{
	int baseclock;          /* master clock (normaly 3.58MHz) */
	int volume;         /* volume                         */
	int memory_region;  /* memory region of speech rom    */
	int vcu;            /* vcu pin level                  */
};

/* use sampling data when speech_rom == 0 */
int VLM5030_sh_start( struct VLM5030interface *interface );
void VLM5030_sh_stop (void);
void VLM5030_sh_update (void);

void VLM5030_update(void);

/* get BSY pin level */
int VLM5030_BSY(void);
/* latch contoll data */
void VLM5030_data_w(int offset,int data);
/* set RST pin level : reset / set table address A8-A15 */
void VLM5030_RST (int pin );
/* set VCU pin level : ?? unknown */
void VLM5030_VCU(int pin );
/* set ST pin level  : set table address A0-A7 / start speech */
void VLM5030_ST(int pin );

#endif

