#ifndef _SIPMSG_H__SIP_
#define _SIPMSG_H__SIP_

 
#define TELNUM_LEN		32
#define HOSTIP_LEN		16
#define RTPTYPE_LEN		8
#define RTPCODEC_LEN	8
#define CALLID_LEN		64		//modify by zhu 0305
#define VIA_COUNT		8
#define CONTENTTYPE_LEN	32
#define	MAX_DTMF_NUM	8		//ZHU 0305

#define TELNUM_LENGTH	TELNUM_LEN
#define CALLID_LENGTH   CALLID_LEN

#define FROM_FIELD_LEN	128
#define TO_FIELD_LEN	128

#define PROTO_LEN		8		//TANG 0522

#define EXPIRES_UNREGISTER (-1)


enum LineSep
{
	Sep_UNKNOWN,
	Sep_N,
	Sep_R,
	Sep_RN,
	Sep_NR
};
    
enum MpMsgType
{
    mtreqUNKNOWN  = 0,
    mtreqINVITE   = 1,
    mtreqOPTIONS  = 2,
    mtreqREGISTER = 3,
    mtreqACK      = 4,
    mtreqCANCEL   = 5,
    mtreqBYE      = 6,
    mtreqINFO     = 7,
    mtreqMESSAGE  = 8,									//TANG 0605
	mtreqNOTIFY   = 9,									//ZHU 0305
    
    query_OK		= 60,								//TANG 0124
    query_FAIL		= 61,
    
    respNullCode                         = 99,
    respTrying                           = 100,
    respRinging                          = 180,
    respCallIsBeingForwarded             = 181,
    respQueued                           = 182,
    respOK                               = 200,
    respMultipleChoices                  = 300,
    respMovedPermanently                 = 301,
    respMovedTemporarily                 = 302,
    respSeeOther                         = 303,
    respUseProxy                         = 305,
    respAlternativeService               = 380,
    respBadRequest                       = 400,
    respUnauthorized                     = 401,
    respPaymentRequired                  = 402,
    respForbidden                        = 403,
    respNotFound                         = 404,
    respMethodNotAllowed                 = 405,
    respNotAcceptable                    = 406,
    respProxyAuthenticationRequired      = 407,
    respRequestTimeout                   = 408,
    respConflict                         = 409,
    respGone                             = 410,
    respLengthRequired                   = 411,
    respRequestEntityTooLarge            = 413,
    respRequestURITooLarge               = 414,
    respUnsupportedMediaType             = 415,
    respBadExtension                     = 420,
    respTemporarilyNotAvailable          = 480,
    respCallLegOrTransactionDoesNotExist = 481,
    respLoopDetected                     = 482,
    respTooManyHops                      = 483,
    respAddressIncomplete                = 484,
    respAmbiguous                        = 485,
    respBusyHere                         = 486,
    respCallCanceled                     = 487,
    respInternalServerError              = 500,
    respNotImplemented                   = 501,
    respBadGateway                       = 502,
    respServiceUnavailable               = 503,
    respGatewayTimeout                   = 504,
    respSIPVersionNotSupported           = 505,
    respBusyEverywhere                   = 600,
    respDecline                          = 603,
    respDoesNotExistAnywhere             = 604,
    respNotAcceptableGF                  = 606
};


enum socktype_t
{
	SOCK_UNKNOWN,
	TCP,
	UDP
};

struct hostport_t
{
	char		 	hostIP[HOSTIP_LEN];
	unsigned int  	port;
};
	
struct call_t
{
	char		 	telnum[TELNUM_LEN];
	hostport_t		hostport;
};

struct startLine_t
{
	MpMsgType		Method;
	call_t			call;
};

struct cseq_t
{
	unsigned int 	seq;
	MpMsgType		Method;
};

struct via_t
{
	socktype_t		SockType;		//"TCP" or "UDP", 1 or 2
	hostport_t		hostport;
};

struct sdp_t
{
	char			proto[PROTO_LEN];		//"IN" or "LOCAL", default is "IN"	//TANG 0522
	char			user[TELNUM_LEN];		//don't care it
	char			hostIP[HOSTIP_LEN];		//don't care it
	char			ConnectHostIP[HOSTIP_LEN];
	unsigned int	RTPPort;
	unsigned int	RTPPortNum;				//"49172/2", then 2 is RTPPortNum	//TANG 0522
	char			RTPType[RTPTYPE_LEN];	//"audio"
	int				RTPCodec[RTPCODEC_LEN];	//you must remember to set it to -1 at first
};

struct sipmsg_t
{
	startLine_t		startLine;
	call_t			From;
	char			from[FROM_FIELD_LEN];
	call_t			To;
	char			to[TO_FIELD_LEN];
	call_t 			Contact;
	int				ServiceNumber;
	char			CallID[CALLID_LEN];
	cseq_t			CSeq;
	int				expirestime;
	via_t			Via[VIA_COUNT];
	char			ContentType[CONTENTTYPE_LEN];	//"application/sdp" or "text/plain"
	int				ContentLength;
	int				IsSDP;							//TANG 0524
	sdp_t			sdp;
	char			dtmf[MAX_DTMF_NUM];				//ZHU 0305
};


LineSep detectLineSep(const char * str);
int findContentLength(const char * str);
size_t findMessageEnd(const char * str);
size_t findDoubleSep(const char * str);		//TANG 0524

int sipdecode(char * str, int * len, sipmsg_t * sipmsg);
int sdpdecode(char * str, int len, sdp_t * sdp);
int dtmfdecode(char * str, int len, char *dtmf);	//ZHU	0305

MpMsgType detectMethod(char * method);
int Url_decode(char * str, call_t * call);
int HostPort_decode(char * str, hostport_t * hostport);
int startLine_decode(char * start, startLine_t * startLine);
int From_decode(char * start, call_t * call);
int Via_decode(char * start, via_t * via);


int sipencode(sipmsg_t * sipmsg, char * buf, int buflen);
const char * detectMethodString(MpMsgType msgtype);
const char * detectReasonString(MpMsgType msgtype);
int Url_encode(call_t * call, char * buf);

int sdpencode(sdp_t * sdp, char * buf, int buflen);
int dtmfencode(char *dtmf, char * buf, int buflen);	//ZHU	0305



#endif


/*
      PT         encoding      audio/video    clock rate    channels
                 name          (A/V)          (Hz)          (audio)
      _______________________________________________________________
      0          PCMU          A              8000          1
      1          1016          A              8000          1
      2          G721          A              8000          1
      3          GSM           A              8000          1
      4          unassigned    A              8000          1
      5          DVI4          A              8000          1
      6          DVI4          A              16000         1
      7          LPC           A              8000          1
      8          PCMA          A              8000          1
      9          G722          A              8000          1
      10         L16           A              44100         2
      11         L16           A              44100         1
      12         unassigned    A
      13         unassigned    A
      14         MPA           A              90000        (see text)
      15         G728          A              8000          1
      16--23     unassigned    A
      33         MP2T          AV             90000
      34--71     unassigned    ?
      72--76     reserved      N/A            N/A           N/A
      77--95     unassigned    ?
      96--127    dynamic       ?

	  4			 G723		   A			  8000			1	//added by tang
	  18		 G729		   A			  8000			1	//added by tang

   Table 2: Payload types (PT) for standard audio and video encodings
   *** RFC 1890 ***
*/

