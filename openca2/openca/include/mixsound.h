#ifndef _MIX_SOUND_H_
#define _MIX_SOUND_H_


//audiotype
#define PAYLOAD_SPEC_WAV		10000
#define PAYLOAD_G723    		4
#define PAYLOAD_G711A    		8
#define PAYLOAD_G711U    		0
#define PAYLOAD_G726    		2
#define PAYLOAD_G729    		18
#define PAYLOAD_GSM				3
#define PAYLOAD_iLBC			98
#define PAYLOAD_Speex			97
#define PAYLOAD_DTMF_RFC2833	101


#define WAV_FILE_HEADER_LENGTH		(4 + 4 + 4 + (8 + 18) + 8)


int detect_codec(char * name);

//return 1: success
//return 0: fail
//mix sound file 'input1' and 'input2' to file 'output'
//file 'output' has the same length and the same format as file 'input1'
int mixsound(char * output, char * input1, char * input2);

//'input' -> 'output'
//'output' is PCM format
int decode_file(char * input, char * output, int format = -1);

//'input' -> 'output'
//'input' is PCM format
int encode_file(char * input, char * output, int format = -1);


int merge_file(char * outputpcm, char * input1pcm, char * input2pcm);


int rebuild_wav_header(const char* encodedFile);


#endif
