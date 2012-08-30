#ifndef _NEWTONEMSG_H_
#define _NEWTONEMSG_H_


int new_msg(unsigned char *msg, unsigned char id, unsigned char type);
int get_p(unsigned char *msg, unsigned char param, short *len, unsigned char *btype, void *datap);
int put_p(unsigned char *msg, unsigned char param, short len, unsigned char btype, void *datap);
int reverseint(unsigned char * myint);
int one_msg_len(unsigned char * msg);
int getlen(unsigned char * msg);


#endif

