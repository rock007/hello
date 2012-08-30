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
 
 int mainSeq;//��seq													//44 acct_session_id part 1
 int secSeq; //��seq													//44 acct_session_id part 2
 
 int serviceType;														//6 Service-Type
 //0 ͨ��CDR
 //1(0x00000001) ��λ���żƴ�
 //3(0x00000004) ������ǰת�ƴ�
 //4(0x00000008) ��æǰת�ƴ�
 //5(0x00000010) �ýв���ǰת�ƴ�
 //6(0x00000020) �Զ��غ�
 //7(0x00000040) ���еȴ��ƴ�
 //8(0x00000080) �����
 //10(0x00000200) ͬ���ƴ�
 //12(0x00000800) ���߼ƴ�
 
 int cdrType;															//(NT) VSA 1 h323-cdr-type
 // 0 ����
 // 1 ����
 int callerUserType;													//VSA 109 h323-billing-model
 int calleeUserType;													//(NT) VSA 1 h323-billing-model-callee
 // 0 - ��
 // 1 - Ԥ��
 
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
 //�ϳ�/�³�����IP��ַ
 int  upACodec;                                            			//VSA 1 coder-type-rate
 int  downACodec;                                          			//(NT) VSA 1 coder-type-rate-down
 //�ϳ�/�³�������������뷽ʽ
 int cliTermType;														//(NT) VSA 1 call-origin-term-type
 int dnisTermType;														//(NT) VSA 1 call-destin-term-type
 //0 ����
 //1 �û�
 //2 �м�
 char cliRouteLable[MGCP_MAX_RTLABLE_LENGTH];							//VSA 33 h323-gw-id
 char dnisRouteLable[MGCP_MAX_RTLABLE_LENGTH];							//VSA 1  h323-remote-id
 //����/�����м��飬������û�������ֶ�Ϊ�û����룬������м̣�����ֶ�Ϊ�м����ʶ
 
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

