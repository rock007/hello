#ifndef _CHILD_PROCESS_H_
#define _CHILD_PROCESS_H_

#include <stdio.h>
#include <stdlib.h>

//��������ʱ����һ�Σ�service_idΪҵ���
int child_process_init(int service_id);

//������whileѭ���в��ϵ��ã�service_idΪҵ���
int child_process_hb(int service_id);

//�����/����־
//1-������active������0-������standby�������˹�����ֵ����������������Ϊ����ʱ��ֵ��Ч
int child_process_get_flag_active();

//return 0: success
//return > 0: error
int child_process_get_license_info(int service_number, const char * licensefile, time_t & starttime, int & lifedays, 
	int & data1, int & data2, int & data3, int & data4, int & data5, 
	int & data6, int & data7, int & data8, int & data9, int & data10);

#endif
