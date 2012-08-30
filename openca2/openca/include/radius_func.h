#ifndef _RADIUS_FUNC_H_
#define _RADIUS_FUNC_H_


#define	RADIUS_MSG_ACCESSREQUEST		1
#define	RADIUS_MSG_ACCESSACCEPT			2
#define	RADIUS_MSG_ACCESSREJECT			3
#define	RADIUS_MSG_ACCOUNTREQUEST		4
#define	RADIUS_MSG_ACCOUNTRESPONSE		5
#define	RADIUS_MSG_SERVICECHGREQUEST	65
#define	RADIUS_MSG_SERVICECHGACCEPT		66
#define	RADIUS_MSG_SERVICECHGREJECT		67


#define	RADIUS_ATTR_UserName				1
#define	RADIUS_ATTR_UserPassword			2
#define	RADIUS_ATTR_CHAPPassword			3
#define	RADIUS_ATTR_NASIPAddress			4
#define	RADIUS_ATTR_NASPort					5
#define	RADIUS_ATTR_ServiceType				6
#define	RADIUS_ATTR_FramedProtocol			7
#define	RADIUS_ATTR_FramedIPAddress			8
#define	RADIUS_ATTR_ReplyMessage			18
#define	RADIUS_ATTR_CallbackNumber			19
#define	RADIUS_ATTR_CallbackID				20
#define	RADIUS_ATTR_Class					25
#define	RADIUS_ATTR_VendorSpecific			26
#define RADIUS_ATTR_SessionTimeout			27
#define	RADIUS_ATTR_CalledStationId			30
#define	RADIUS_ATTR_CallingStationId		31
#define	RADIUS_ATTR_ProxyState				33
#define	RADIUS_ATTR_AcctStatusType			40
#define	RADIUS_ATTR_AcctDelayTime			41
#define	RADIUS_ATTR_InBytes					42
#define	RADIUS_ATTR_OutByets				43
#define	RADIUS_ATTR_AcctSessionId			44
#define	RADIUS_ATTR_AcctSessionTime			46
#define	RADIUS_ATTR_AcctInputPackets		47
#define	RADIUS_ATTR_AcctOutputPackets		48
#define	RADIUS_ATTR_AcctTerminateCause		49
#define	RADIUS_ATTR_CHAPChallenge			60
#define RADIUS_ATTR_NASPortType				61
#define	RADIUS_ATTR_ServiceChgCmd			105
#define	RADIUS_ATTR_AcctPacketType			106
#define	RADIUS_ATTR_CallReference			107
#define	RADIUS_ATTR_PSTNPort				108
#define	RADIUS_ATTR_UserType				109
#define	RADIUS_ATTR_AcctConnectionTime		110
#define	RADIUS_ATTR_NewPassword				111
#define	RADIUS_ATTR_ErrorReason				112
#define	RADIUS_ATTR_RemainMoney				113
#define	RADIUS_ATTR_CalledRegionId			121
#define	RADIUS_ATTR_AcctCallType			122
#define	RADIUS_ATTR_OrgGKAddress			123
#define	RADIUS_ATTR_OrgGWAddress			124
#define	RADIUS_ATTR_DesGKAddress			125
#define	RADIUS_ATTR_DesGWAddress			126
#define	RADIUS_ATTR_AccessNumber			127
#define	RADIUS_ATTR_RemainTime				128
#define	RADIUS_ATTR_CodecType				131
#define	RADIUS_ATTR_TransferNum				132
#define	RADIUS_ATTR_NewUserNum				133
#define	RADIUS_ATTR_InputFAXPages			134
#define	RADIUS_ATTR_OutputFAXPages			135
#define	RADIUS_ATTR_AcctSessionBegin		136
#define	RADIUS_ATTR_AcctSessionEnd			137
#define	RADIUS_ATTR_LocalFee				138
#define	RADIUS_ATTR_LocalRadiusAddress		139
#define	RADIUS_ATTR_CallingVendorID			140
#define	RADIUS_ATTR_CalledVendorID			141
#define	RADIUS_ATTR_CallingCalledMark		150
#define	RADIUS_ATTR_ServiceQuality			152
#define	RADIUS_ATTR_RoundDelayTime			154
#define	RADIUS_ATTR_LossPackageRate			155
#define	RADIUS_ATTR_CalledGWIP				170


//CISCO VSA
#define	RADIUS_ATTR_Cisco_NAS_port				2
#define	RADIUS_ATTR_h323_billing_model			109
#define	RADIUS_ATTR_h323_call_origin			26
#define	RADIUS_ATTR_h323_call_type				27
#define	RADIUS_ATTR_h323_conf_id				24
#define	RADIUS_ATTR_h323_connect_time			28
#define	RADIUS_ATTR_h323_credit_amount			101
#define	RADIUS_ATTR_h323_credit_time			102
#define	RADIUS_ATTR_h323_currency				110
#define	RADIUS_ATTR_h323_disconnect_cause		30
#define	RADIUS_ATTR_h323_disconnect_time		29
#define	RADIUS_ATTR_h323_gw_id					33
#define	RADIUS_ATTR_h323_incoming_conf_id		1
#define	RADIUS_ATTR_h323_ivr_in					1
#define	RADIUS_ATTR_h323_ivr_out				1
#define	RADIUS_ATTR_h323_preferred_lang			107
#define	RADIUS_ATTR_h323_prompt_id				104
#define	RADIUS_ATTR_h323_redirect_ip_address	108
#define	RADIUS_ATTR_h323_redirect_number		106
#define	RADIUS_ATTR_h323_remote_address			23
#define	RADIUS_ATTR_h323_remote_id				1
#define	RADIUS_ATTR_h323_return_code			103
#define	RADIUS_ATTR_h323_setup_time				25
#define	RADIUS_ATTR_h323_time_and_day			105
#define	RADIUS_ATTR_h323_voice_quality			31
#define	RADIUS_ATTR_subscriber					1
#define	RADIUS_ATTR_in_portgrp_id				1
#define	RADIUS_ATTR_out_portgrp_id				1
#define RADIUS_ATTR_pre_bytes_in				1
#define RADIUS_ATTR_pre_bytes_out				1
#define RADIUS_ATTR_pre_paks_in					1
#define RADIUS_ATTR_pre_paks_out				1
#define RADIUS_ATTR_nas_rx_speed				1
#define RADIUS_ATTR_nas_tx_speed				1
#define RADIUS_ATTR_tariff_type					1
#define RADIUS_ATTR_coder_type_rate				1
//NewTone define
#define RADIUS_ATTR_h323_cdr_type				1
#define RADIUS_ATTR_h323_billing_model_callee	1
#define RADIUS_ATTR_h323_release_time			1
#define RADIUS_ATTR_coder_type_rate_down		1
#define RADIUS_ATTR_call_origin_term_type		1
#define RADIUS_ATTR_call_destin_term_type		1


//Cisco VSA modified
#define	CISCO_RADIUS_ATTR_Cisco_NAS_port				1000
#define	CISCO_RADIUS_ATTR_h323_billing_model			1001
#define	CISCO_RADIUS_ATTR_h323_call_origin				1002
#define	CISCO_RADIUS_ATTR_h323_call_type				1003
#define	CISCO_RADIUS_ATTR_h323_conf_id					1004
#define	CISCO_RADIUS_ATTR_h323_connect_time				1005
#define	CISCO_RADIUS_ATTR_h323_credit_amount			1006
#define	CISCO_RADIUS_ATTR_h323_credit_time				1007
#define	CISCO_RADIUS_ATTR_h323_currency					1008
#define	CISCO_RADIUS_ATTR_h323_disconnect_cause			1009
#define	CISCO_RADIUS_ATTR_h323_disconnect_time			1010
#define	CISCO_RADIUS_ATTR_h323_gw_id					1011
#define	CISCO_RADIUS_ATTR_h323_incoming_conf_id			1012
#define	CISCO_RADIUS_ATTR_h323_ivr_in					1013
#define	CISCO_RADIUS_ATTR_h323_ivr_out					1014
#define	CISCO_RADIUS_ATTR_h323_preferred_lang			1015
#define	CISCO_RADIUS_ATTR_h323_prompt_id				1016
#define	CISCO_RADIUS_ATTR_h323_redirect_ip_address		1017
#define	CISCO_RADIUS_ATTR_h323_redirect_number			1018
#define	CISCO_RADIUS_ATTR_h323_remote_address			1019
#define	CISCO_RADIUS_ATTR_h323_remote_id				1020
#define	CISCO_RADIUS_ATTR_h323_return_code				1021
#define	CISCO_RADIUS_ATTR_h323_setup_time				1022
#define	CISCO_RADIUS_ATTR_h323_time_and_day				1023
#define	CISCO_RADIUS_ATTR_h323_voice_quality			1024
#define	CISCO_RADIUS_ATTR_subscriber					1025
#define	CISCO_RADIUS_ATTR_in_portgrp_id					1026
#define	CISCO_RADIUS_ATTR_out_portgrp_id				1027
#define	CISCO_RADIUS_ATTR_pre_bytes_in					1028
#define	CISCO_RADIUS_ATTR_pre_bytes_out					1029
#define	CISCO_RADIUS_ATTR_pre_paks_in					1030
#define	CISCO_RADIUS_ATTR_pre_paks_out					1031
#define	CISCO_RADIUS_ATTR_nas_rx_speed					1032
#define	CISCO_RADIUS_ATTR_nas_tx_speed					1033
#define CISCO_RADIUS_ATTR_tariff_type					1034
#define CISCO_RADIUS_ATTR_coder_type_rate				1035
//NewTone define
#define CISCO_RADIUS_ATTR_h323_cdr_type					1036
#define CISCO_RADIUS_ATTR_h323_billing_model_callee		1037
#define CISCO_RADIUS_ATTR_h323_release_time				1038
#define CISCO_RADIUS_ATTR_coder_type_rate_down			1039
#define CISCO_RADIUS_ATTR_call_origin_term_type			1040
#define CISCO_RADIUS_ATTR_call_destin_term_type			1041

#define	CISCO_RADIUS_ATTR_ERROR							1999


//value
#define RADIUS_VALUE_AcctStatusType_Start				1
#define RADIUS_VALUE_AcctStatusType_Stop				2
#define RADIUS_VALUE_AcctStatusType_Interim_Update		3
#define RADIUS_VALUE_AcctStatusType_Accounting_On		7
#define RADIUS_VALUE_AcctStatusType_Accounting_Off		8
#define RADIUS_VALUE_AcctStatusType_Failed				15

#define RADIUS_VALUE_h323_call_origin_ANSWER			"answer"
#define RADIUS_VALUE_h323_call_origin_ORIGINATE			"originate"
#define RADIUS_VALUE_h323_call_origin_CALLBACK			"callback"

#define RADIUS_VALUE_h323_call_type_Telephony			"Telephony"
#define RADIUS_VALUE_h323_call_type_VoIP				"VoIP"
#define RADIUS_VALUE_h323_call_type_VoFR				"VoFR"

#define RADIUS_VALUE_h323_return_code_success						0
#define RADIUS_VALUE_h323_return_code_invalid_account				1
#define RADIUS_VALUE_h323_return_code_invalid_passwd				2
#define RADIUS_VALUE_h323_return_code_account_in_use				3
#define RADIUS_VALUE_h323_return_code_zero_balance					4
#define RADIUS_VALUE_h323_return_code_card_expire					5
#define RADIUS_VALUE_h323_return_code_credit_limit					6
#define RADIUS_VALUE_h323_return_code_user_denied					7
#define RADIUS_VALUE_h323_return_code_service_not_available			8
#define RADIUS_VALUE_h323_return_code_called_blocked				9
#define RADIUS_VALUE_h323_return_code_retries_number_expired		10
#define RADIUS_VALUE_h323_return_code_invalid_argument				11
#define RADIUS_VALUE_h323_return_code_insufficient_funds			12
#define RADIUS_VALUE_h323_return_code_free_call						13
#define RADIUS_VALUE_h323_return_code_invalid_card_number			14
#define RADIUS_VALUE_h323_return_code_redirect_back_to_PSTN			50
#define RADIUS_VALUE_h323_return_code_redirect_to_called			51
#define RADIUS_VALUE_h323_return_code_redirect_to_custom_service	52
#define RADIUS_VALUE_h323_return_code_redirect_ip_address			53
#define RADIUS_VALUE_h323_return_code_card_freeze					80		//follow defined by Tang
#define RADIUS_VALUE_h323_return_code_failed						81
#define RADIUS_VALUE_h323_return_code_ammount_mismatch				90
#define RADIUS_VALUE_h323_return_code_no_reacr						91
#define RADIUS_VALUE_h323_return_code_no_privilege_using_card		92
#define RADIUS_VALUE_h323_return_code_database_error				93
#define RADIUS_VALUE_h323_return_code_card_already_registered		94
#define RADIUS_VALUE_h323_return_code_not_roaming_card				95
#define RADIUS_VALUE_h323_return_code_unknown_error					96


//ISO 4217
#define CURRENCY_China			"CNY"
#define CURRENCY_USA			"USD"
#define CURRENCY_HongKong		"HKD"
#define CURRENCY_TaiWan			"TWD"



//for Cisco VSA attribute name
struct string_vsa {
	int i;
	char * str;
};

static struct string_vsa	vsas[] = 
{
	{ CISCO_RADIUS_ATTR_Cisco_NAS_port,				"Cisco-NAS-port" }, 
	{ CISCO_RADIUS_ATTR_h323_billing_model,			"h323-billing-model" }, 
	{ CISCO_RADIUS_ATTR_h323_call_origin,			"h323-call-origin" }, 
	{ CISCO_RADIUS_ATTR_h323_call_type,				"h323-call-type" }, 
	{ CISCO_RADIUS_ATTR_h323_conf_id,				"h323-conf-id" }, 
	{ CISCO_RADIUS_ATTR_h323_connect_time,			"h323-connect-time" }, 
	{ CISCO_RADIUS_ATTR_h323_credit_amount,			"h323-credit-amount" }, 
	{ CISCO_RADIUS_ATTR_h323_credit_time,			"h323-credit-time" }, 
	{ CISCO_RADIUS_ATTR_h323_currency,				"h323-currency" }, 
	{ CISCO_RADIUS_ATTR_h323_disconnect_cause,		"h323-disconnect-cause" }, 
	{ CISCO_RADIUS_ATTR_h323_disconnect_time,		"h323-disconnect-time" }, 
	{ CISCO_RADIUS_ATTR_h323_gw_id,					"h323-gw-id" }, 
	{ CISCO_RADIUS_ATTR_h323_incoming_conf_id,		"h323-incoming-conf-id" }, 
	{ CISCO_RADIUS_ATTR_h323_ivr_in,				"h323-ivr-in" }, 
	{ CISCO_RADIUS_ATTR_h323_ivr_out,				"h323-ivr-out" }, 
	{ CISCO_RADIUS_ATTR_h323_preferred_lang,		"h323-preferred-lang" }, 
	{ CISCO_RADIUS_ATTR_h323_prompt_id,				"h323-prompt-id" }, 
	{ CISCO_RADIUS_ATTR_h323_redirect_ip_address,	"h323-redirect-ip-address" }, 
	{ CISCO_RADIUS_ATTR_h323_redirect_number,		"h323-redirect-number" }, 
	{ CISCO_RADIUS_ATTR_h323_remote_address,		"h323-remote-address" }, 
	{ CISCO_RADIUS_ATTR_h323_remote_id,				"h323-remote-id" }, 
	{ CISCO_RADIUS_ATTR_h323_return_code,			"h323-return-code" }, 
	{ CISCO_RADIUS_ATTR_h323_setup_time,			"h323-setup-time" }, 
	{ CISCO_RADIUS_ATTR_h323_time_and_day,			"h323-time-and-day" }, 
	{ CISCO_RADIUS_ATTR_h323_voice_quality,			"h323-voice-quality" }, 
	{ CISCO_RADIUS_ATTR_subscriber,					"subscriber" }, 
	{ CISCO_RADIUS_ATTR_in_portgrp_id,				"in-portgrp-id" }, 
	{ CISCO_RADIUS_ATTR_out_portgrp_id,				"out-portgrp-id" }, 
	{ CISCO_RADIUS_ATTR_pre_bytes_in,				"pre-bytes-in" }, 
	{ CISCO_RADIUS_ATTR_pre_bytes_out,				"pre-bytes-out" }, 
	{ CISCO_RADIUS_ATTR_pre_paks_in,				"pre-paks-in" }, 
	{ CISCO_RADIUS_ATTR_pre_paks_out,				"pre-paks-out" }, 
	{ CISCO_RADIUS_ATTR_nas_rx_speed,				"nas-rx-speed" }, 
	{ CISCO_RADIUS_ATTR_nas_tx_speed,				"nas-tx-speed" }, 
	{ CISCO_RADIUS_ATTR_tariff_type,				"tariff-type" }, 
	{ CISCO_RADIUS_ATTR_coder_type_rate,			"coder-type-rate" }, 
	//NewTone define
	{ CISCO_RADIUS_ATTR_h323_cdr_type,				"h323-cdr-type" }, 
	{ CISCO_RADIUS_ATTR_h323_billing_model_callee,	"h323-billing-model-callee" }, 
	{ CISCO_RADIUS_ATTR_h323_release_time,			"h323-release-time" }, 
	{ CISCO_RADIUS_ATTR_coder_type_rate_down,		"coder-type-rate-down" }, 
	{ CISCO_RADIUS_ATTR_call_origin_term_type,		"call-origin-term-type" }, 
	{ CISCO_RADIUS_ATTR_call_destin_term_type,		"call-destin-term-type" }, 
	
	{ CISCO_RADIUS_ATTR_ERROR, 						"error" } 
};




#define MD5_RESULT_LENGTH				16
#define RADIUS_H323_CONF_ID_LENGTH		36
#define RADIUS_PASSWD_LENGTH			16
#define RADIUS_SHAREKEY_LENGTH			32
#define RADIUS_MSG_MAX_LEN				4096


class rc
{
private:
	char sharekey[RADIUS_SHAREKEY_LENGTH];
	unsigned int vendorID;
	
public:
	unsigned char * msg;
	int msglen;
	
public:
	rc();
	~rc();
	
	//build radius msg
	int begin_msg(unsigned char msgtype, unsigned char msgid, unsigned char * pauthen = NULL);
	
	int put_p(unsigned char paraid, void * para, int paralen);
	int get_p(unsigned char * msgbuf, unsigned char paraid, void * para, int & paralen);
	
	int put_p_vsa(unsigned char paraid, void * para, int paralen);
	int get_p_vsa(unsigned char * msgbuf, unsigned char paraid, void * para, int & paralen, char * header = NULL);
	
	int end_msg();
	
	int set_sharekey(char * key);
	int set_vendorid(unsigned int vendorid);
};



#ifndef MGCP_MAX_CARDNUMBER_LENGTH
#define MGCP_MAX_CARDNUMBER_LENGTH		32
#endif
#ifndef MGCP_MAX_TELNUMBER_LENGTH
#define MGCP_MAX_TELNUMBER_LENGTH		32
#endif
#ifndef MGCP_MAX_RTLABLE_LENGTH
#define MGCP_MAX_RTLABLE_LENGTH			32
#endif

struct ccm3_Mgcp_CDR_t 
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


struct auth_request_authorize_call_t
{
	unsigned int nasIP;
	int nasPort;
	
	char card[MGCP_MAX_CARDNUMBER_LENGTH];
	char passwd[RADIUS_PASSWD_LENGTH];
	
	char cli[MGCP_MAX_TELNUMBER_LENGTH];
	char dnis[MGCP_MAX_TELNUMBER_LENGTH];
	
	char h323_conf_id[RADIUS_H323_CONF_ID_LENGTH];
};

struct auth_response_authorize_call_t
{
	
};

struct acctRequest1_t
{
	
};

struct acctResponse1_t
{
	
};


struct radius_info_t
{
	unsigned char	msg_type;
	unsigned char 	msg_identifier;
	unsigned char 	authenticator[MD5_RESULT_LENGTH];
	unsigned int	vendorID;
	
	int t;
	union 
	{
		struct ccm3_Mgcp_CDR_t	acctRequest;		//t=1
		struct authRequest_t	authRequest;		//t=2
		
		struct auth_request_authorize_call_t		authcall_request;		//t=3
		struct auth_response_authorize_call_t		authcall_response;		//t=4
		
		struct acctRequest1_t						acctRequest1;			//t=5
		struct acctResponse1_t						acctResponse1;			//t=6
	}u;
	
	int h323_return_code;
};



char * string_addr(unsigned int ip);

//return 1 for success
//return 0 for fail
int decode_radius_cdr(unsigned char * msgbuf, int msglen, struct ccm3_Mgcp_CDR_t * cdr1);
int decode_radius_msg(unsigned char * msgbuf, int msglen, struct radius_info_t * qrsinfo);

//return real message length
//return 0 for fail
int encode_radius_cdr(struct ccm3_Mgcp_CDR_t * cdr1, unsigned char * buffer, int buflen);
int encode_radius_cdr_response(struct radius_info_t * prsinfo, unsigned char * buffer, int buflen);
int encode_radius_msg(struct radius_info_t * prsinfo, unsigned char * buffer, int buflen, int flag_accept = 1);

//buffer1 is radius request message, buffer2 is radius response message
//return 0 for fail
int check_radius_validation(unsigned char * buffer1, unsigned char * buffer2, char * sharekey);
int check_radius_validation(void * buffer1, void * buffer2);


int encode_radius_access_request(struct auth_request_authorize_call_t * preq1, unsigned char * buffer, int buflen);


#endif

