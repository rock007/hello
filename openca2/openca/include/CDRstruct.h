#ifndef _CDRSTRUCT_H_
#define _CDRSTRUCT_H_


#define MGCP_MAX_CARDNUMBER_LENGTH		32
#define MGCP_MAX_TELNUMBER_LENGTH		40
#define MGCP_MAX_RTLABLE_LENGTH			32

#define MD5_RESULT_LENGTH				16
#define RADIUS_H323_CONF_ID_LENGTH		36
#define RADIUS_PASSWD_LENGTH			16
#define RADIUS_SHAREKEY_LENGTH			32


struct Mgcp_CDR_radius_t
{
 int flag;//not for radius
 //it is not the cdr item but for logfile name.value 0 is the default 
 //and 1 if for the prepaid that has not been sent ok.  
 
 int mainSeq;//主seq													//44 acct_session_id part 1
 int secSeq; //从seq													//44 acct_session_id part 2
 
 int serviceType;														//6 Service-Type
 //0 通话CDR
 //1(0x00000001) 缩位拨号计次
 //3(0x00000004) 无条件前转计次
 //4(0x00000008) 遇忙前转计次
 //5(0x00000010) 久叫不答前转计次
 //6(0x00000020) 自动回呼
 //7(0x00000040) 呼叫等待计次
 //8(0x00000080) 免打扰
 //10(0x00000200) 同呼计次
 //12(0x00000800) 热线计次
 
 int cdrType;															//(NT) VSA 1 h323-cdr-type
 // 0 呼叫
 // 1 月租
 int callerUserType;													//VSA 109 h323-billing-model
 int calleeUserType;													//(NT) VSA 1 h323-billing-model-callee
 // 0 - 后付
 // 1 - 预付
 
 char cardNum[MGCP_MAX_CARDNUMBER_LENGTH];								//1 User-Name
                                                                
 char cli[MGCP_MAX_TELNUMBER_LENGTH];  //caller telephone number.		//31 Calling-Station-Id
 char dnis[MGCP_MAX_TELNUMBER_LENGTH]; //callee telephone number.		//30 Called-Station-Id
                                                                 
 time_t callBeginTime;													//VSA 25 h323_setup_time
 time_t connectBeginTime;                                               //VSA 28 h323_connect_time
 time_t connectEndTime;                                                	//VSA 29 h323_disconnect_time
 time_t callEndTime;                                                   	//(NT) VSA 1 h323_release_time
 
 int  releasedReason;                                  					//VSA 30 h323_disconnect_cause
 //add by yinhaifeng 2002.12.17 for unionnet.                          
 unsigned int upGwAddr;                                                 //4
 unsigned int downGwAddr;                                          		//VSA 23 h323_remote_address
 //上车/下车网关IP地址
 int  upACodec;                                            			//VSA 1 coder-type-rate
 int  downACodec;                                          			//(NT) VSA 1 coder-type-rate-down
 //上车/下车网关语音编解码方式
 int cliTermType;														//(NT) VSA 1 call-origin-term-type
 int dnisTermType;														//(NT) VSA 1 call-destin-term-type
 //0 保留
 //1 用户
 //2 中继
 char cliRouteLable[MGCP_MAX_RTLABLE_LENGTH];							//VSA 33 h323-gw-id
 char dnisRouteLable[MGCP_MAX_RTLABLE_LENGTH];							//VSA 1  h323-remote-id
 //主叫/被叫中继组，如果是用户，则该字段为用户号码，如果是中继，则该字段为中继组标识
 
 time_t intime;//not for radius
 int    reservedUnit;//not for radius
};


struct authRequest_t
{
	char card[MGCP_MAX_CARDNUMBER_LENGTH];
	char passwd[RADIUS_PASSWD_LENGTH];
	
	char card2[MGCP_MAX_CARDNUMBER_LENGTH];
	char passwd2[RADIUS_PASSWD_LENGTH];
	
	char cli[MGCP_MAX_TELNUMBER_LENGTH];
	char dnis[MGCP_MAX_TELNUMBER_LENGTH];
	
	char h323_conf_id[RADIUS_H323_CONF_ID_LENGTH];
	
	int h323_credit_amount;
};

struct radius_info_t
{
	unsigned char 	msg_identifier;
	unsigned char 	authenticator[MD5_RESULT_LENGTH];
	unsigned int	vendorID;
	
	int   uflag;
	union {
		struct Mgcp_CDR_t	acctRequest;
		authRequest_t		authRequest;
	}u;
	
	int h323_return_code;
};


//return 1 for success
//return 0 for fail
int decode_radius_cdr(unsigned char * msgbuf, int msglen, struct Mgcp_CDR_t * cdr1);
int decode_radius_msg(unsigned char * msgbuf, int msglen, struct radius_info_t * qrsinfo);

//return real message length
//return 0 for fail
int encode_radius_cdr(struct Mgcp_CDR_t * cdr1, unsigned char * buffer, int buflen);
int encode_radius_cdr_response(struct radius_info_t * prsinfo, unsigned char * buffer, int buflen);
int encode_radius_msg(struct radius_info_t * prsinfo, unsigned char * buffer, int buflen, int flag_accept = 1);

//buffer1 is radius request message, buffer2 is radius response message
//return 0 for fail
int check_radius_validation(unsigned char * buffer1, unsigned char * buffer2, char * sharekey);
int check_radius_validation(void * buffer1, void * buffer2);


#endif

