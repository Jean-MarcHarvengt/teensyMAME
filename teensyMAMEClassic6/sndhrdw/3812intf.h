#ifndef YM3812INTF_H
#define YM3812INTF_H

/* Main emulated vs non-emulated switch */
/* 0 = Use non-emulated YM3812 */
/* 1 = Use emulated YM3812 */
/* default value : 1 ( Use emulated YM3812 ) */
extern int use_emulated_ym3812;

#define MAX_3812 1

struct YM3812interface
{
	int num;
	int baseclock;
	int volume[MAX_3812];
	void (*handler)(void);
};
#define YM3526interface YM3812interface


int YM3812_status_port_0_r(int offset);
void YM3812_control_port_0_w(int offset,int data);
void YM3812_write_port_0_w(int offset,int data);
#define YM3526_status_port_0_r YM3812_status_port_0_r
#define YM3526_control_port_0_w YM3812_control_port_0_w
#define YM3526_write_port_0_w YM3812_write_port_0_w

int YM3812_sh_start(struct YM3812interface *interface);
void YM3812_sh_stop(void);
void YM3812_sh_update(void);

#endif
