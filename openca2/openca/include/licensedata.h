#ifndef _LICENSEDATA_H_
#define _LICENSEDATA_H_

#include <time.h>

#define MAX_LICENSE_DATA_LENGTH		400

#define LICENSE_CCM_ID			1
#define LICENSE_CCM_HB			2

#define LICENSE_GK_ID			3
#define LICENSE_GK_HB			4

#define LICENSE_SN3_ID			5
#define LICENSE_SN3_HB			6

#define LICENSE_MS_ID			7
#define LICENSE_MS_HB			8

#define LICENSE_H323Agent_ID	9
#define LICENSE_H323Agent_HB	10

#define LICENSE_SipAgent_ID		11
#define LICENSE_SipAgent_HB		12

#define LICENSE_VoiceProxy_ID	13
#define LICENSE_VoiceProxy_HB	14

#define LICENSE_SERVICE_ID		15
#define LICENSE_SERVICE_HB		16


struct ms_license_data_t
{
	int productid;
	
	time_t starttime;
	int lifedays;
	
	//Special data below
	int max_current_used_port;			//最大并发线数
	int max_accumulate_used_port_exit;	//累计使用最大线数后退出
	
	char reserve[24];//4的倍数即可
};

struct sn3_license_data_t	//必须保证该结构的大小是MAX_LICENSE_DATA_LENGTH
{
	int productid;
	
	time_t starttime;
	int lifedays;
	
	//Special data below
	int max_current_call_sip;
	int max_current_call_h323;
	int max_current_call_mgcp;
	int max_accumulate_call_exit;
	int max_accumulate_running_days_exit;
	
	int service00to31;
	int service32to63;
	int service64to95;
	int service96to127;
	
	char service_string[MAX_LICENSE_DATA_LENGTH-11*sizeof(int)-sizeof(time_t)-16];
	
	char reserve[16];//4的倍数即可
};

struct h323agent_license_data_t
{
	int productid;
	
	time_t starttime;
	int lifedays;
	
	//Special data below
	int max_current_call;
	int max_accumulate_call_exit;
	int max_accumulate_running_days_exit;
	
	char reserve[28];//4的倍数即可
};

struct sipagent_license_data_t
{
	int productid;
	
	time_t starttime;
	int lifedays;
	
	//Special data below
	int max_current_call;
	int max_accumulate_call_exit;
	int max_accumulate_running_days_exit;
	
	char reserve[32];//4的倍数即可
};

struct ccm_license_data_t
{
	int productid;
	
	time_t starttime;
	int lifedays;
	
	//Special data below
	int max_current_call;
	int max_accumulate_call_exit;
	int max_accumulate_running_days_exit;
	
	char reserve[36];//4的倍数即可
};

struct vproxy_license_data_t
{
	int productid;
	
	time_t starttime;
	int lifedays;
	
	//Special data below
	int max_current_call;
	char ca_ip1[16];
	char ca_ip2[16];
	
	char reserve[16];//4的倍数即可
};

struct service_license_data_t
{
	int productid;
	
	time_t starttime;
	int lifedays;
	
	//Special data below
	int service_number;
	
	int data1;
	int data2;
	int data3;
	int data4;
	int data5;
	int data6;
	int data7;
	int data8;
	int data9;
	int data10;
	
	//char reserve[16];//4的倍数即可
};


#endif
