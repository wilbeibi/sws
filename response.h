#ifndef _RESPONSE_H_
#define _RESPONSE_H_

void get_status_msg(int code, char msg[]);
void err_response(int fd, int status);
void get_timestamp(char *buf);


/* TODO currently global, any further advices? */
int _sock;
int _simple_response;
int _head_response;

#endif