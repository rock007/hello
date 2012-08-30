#ifndef _Q931_H_GK
#define _Q931_H_GK

#define BEARER_CAPABILITY_LEN 		16
#define IE_CAUSE_LEN				8
#define CALLING_PARTY_NUMBER_LEN	32
#define CALLED_PARTY_NUMBER_LEN		32
#define DISPLAY_NAME_LEN			48
#define UUIE_LEN					(3*1024)

struct	Cause
{
	short	len;
	unsigned char	content[IE_CAUSE_LEN];
};

struct bearerCapability
{
	short	len;
	unsigned char	content[BEARER_CAPABILITY_LEN];
};

struct UUIE
{
	short	len;
	char	protocolDiscriminator;	//must be 0x05
	unsigned char	content[UUIE_LEN];
};

struct q931msg		//same to 'q931buf' now		//TANG 2002.11.01
{
	struct {
		unsigned bearerPresent : 1;
		unsigned causePresent : 1;
		unsigned displayPresent : 1;
		unsigned PIPresent : 1;
		unsigned callingPresent : 1;
		unsigned calledPresent : 1;
		unsigned UUIEPresent : 1;
	} m;
	short	crv;
	char	msgtype;	//see below
	char	from;		//0 means from caller to callee, !0 means from callee to caller
	
	struct	bearerCapability	bearer;			//OPTIONAL
	struct	Cause				cause;			//OPTIONAL
	char	PI;									//OPTIONAL
	char	display[DISPLAY_NAME_LEN];			//OPTIONAL
	char	calling[CALLING_PARTY_NUMBER_LEN];	//OPTIONAL
	char	called[CALLED_PARTY_NUMBER_LEN];	//OPTIONAL
	struct 	UUIE				uuie;			//OPTIONAL

	/*
	struct {
		unsigned bearerPresent : 1;
		unsigned displayPresent : 1;
		unsigned calledPresent : 1;
		unsigned UUIEPresent : 1;
	} m;
	short	crv;
	char	msgtype;	//see below
	char	from;		//0 means from caller to callee, !0 means from callee to caller

	char	called[CALLED_PARTY_NUMBER_LEN];	//OPTIONAL
	char	display[DISPLAY_NAME_LEN];			//OPTIONAL
	struct	bearerCapability	bearer;			//OPTIONAL
	struct 	UUIE				uuie;			//OPTIONAL
	*/
};

//return 0 for error
//size is msg len, msg is stored in buf
int Decode_q931(unsigned char * buf, int size, struct q931msg * q931);

//return 0 for error
//size is sizeof(buf) and returned size is msg len
int Encode_q931(struct q931msg * q931, unsigned char * buf, int * size);



//H323 Gatekeeper will use the following structrue and functions
//GK don't care Bearer-Capability or Display or Called-Party_Number or else...
//So put them all in the struct otherIE
//and put UUIE alone
#define Q931_OTHER_IE_LEN	128

struct otherIE
{
	short	len;
	unsigned char	content[Q931_OTHER_IE_LEN];
};

struct q931buf
{
	struct {
		unsigned bearerPresent : 1;
		unsigned causePresent : 1;
		unsigned displayPresent : 1;
		unsigned PIPresent : 1;
		unsigned callingPresent : 1;
		unsigned calledPresent : 1;
		unsigned UUIEPresent : 1;
	} m;
	short	crv;
	char	msgtype;	//see below
	char	from;		//0 means from caller to callee, !0 means from callee to caller
	
	struct	bearerCapability	bearer;			//OPTIONAL
	struct	Cause				cause;			//OPTIONAL
	char	PI;									//OPTIONAL
	char	display[DISPLAY_NAME_LEN];			//OPTIONAL
	char	calling[CALLING_PARTY_NUMBER_LEN];	//OPTIONAL
	char	called[CALLED_PARTY_NUMBER_LEN];	//OPTIONAL
	struct 	UUIE				uuie;			//OPTIONAL
};

//return 0 for error
int Decode_UUIE(unsigned char * buf, int size, struct q931buf * q931);
int Encode_UUIE(struct q931buf * q931, unsigned char * buf, int * size);


#endif

/*
Q931 message type:

01 ALERTING 						//h323
02 CALL PROCEEDING 					//h323
07 CONNECT 							//h323
0F CONNECT ACKNOWLEDGE 
03 PROGRESS 
05 SETUP 							//h323
0B SETUP ACKNOWLEDGE 
24 HOLD * 
28 HOLD ACKNOWLEDGE * 
30 HOLD REJECT * 
26 RESUME 
2E RESUME ACKNOWLEDGE 
22 RESUME REJECT 
31 RETRIEVE * 
33 RETRIEVE ACKNOWLEDGE * 
37 RETRIEVE REJECT * 
25 SUSPEND 
2D SUSPEND ACKNOWLEDGE 
21 SUSPEND REJECT 
20 USER INFORMATION 
45 DISCONNECT 
4D RELEASE 
5A RELEASE COMPLETE 				//h323
46 RESTART 
4E RESTART ACKNOWLEDGE 
79 CONGESTION CONTROL 
62 FACILITY 
7B INFORMATION 
6E NOTIFY 
64 REGISTER * 
60 SEGMENT 
7D STATUS 
75 STATUS ENQUIRY 

*/
