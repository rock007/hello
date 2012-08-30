#ifndef _CHILD_PROCESS_H_
#define _CHILD_PROCESS_H_

#include <stdio.h>
#include <stdlib.h>

//进程起来时调用一次，service_id为业务号
int child_process_init(int service_id);

//进程在while循环中不断调用，service_id为业务号
int child_process_hb(int service_id);

//获得主/备标志
//1-本机是active机器，0-本机是standby机器。人工设置值。存在两个机器互为备份时该值有效
int child_process_get_flag_active();

//return 0: success
//return > 0: error
int child_process_get_license_info(int service_number, const char * licensefile, time_t & starttime, int & lifedays, 
	int & data1, int & data2, int & data3, int & data4, int & data5, 
	int & data6, int & data7, int & data8, int & data9, int & data10);

#endif
