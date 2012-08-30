#ifndef LICENSE_OPENCA_
#define LICENSE_OPENCA_


#define LICENSE_ERROR_KEY_LENGTH_TOO_LONG		10
#define LICENSE_ERROR_TOTAL_LENGTH_ERROR		20
#define LICENSE_ERROR_CRC1_ERROR				30
#define LICENSE_ERROR_DECRYPT_ERROR				40
#define LICENSE_ERROR_CRC2_ERROR				50
#define LICENSE_ERROR_PARAMETER_DATALEN_ERROR	60
#define LICENSE_ERROR_KEY_ERROR					70


int encode_license(long hostid, char * data, int len);
int check_license(long hostid, char * buffer, int buflen, char * data, int datalen);


//newlicense TANG 2002.12.31
int encode_license_2(void * userkey, int userkeylen, void * data, int datalen);
int encode_license_3(void * userkey, int userkeylen, void * data, int datalen, void * buffer, int buflen);

int check_license_2(void * userkey, int userkeylen, void * buffer, int buflen, void * data, int datalen);


#endif

