#ifndef YM2413INTF_H
#define YM2413INTF_H

#define MAX_2413 MAX_3812

#define YM2413interface YM3812interface

int  YM2413_status_port_0_r(int offset);
void YM2413_register_port_0_w(int offset,int data);
void YM2413_data_port_0_w(int offset,int data);

int  YM2413_sh_start(struct YM2413interface *interface);
void YM2413_sh_stop(void);
void YM2413_sh_update(void);

#endif

