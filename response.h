#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#include "parse.h"

void logging(Req_info *req);
void get_status_msg(int code, char msg[]);
int err_response(int fd, int status);
void get_timestamp(char *buf);
void sws_response(int fd, Req_info *req);

/* TODO currently global, any further advices? */
int _sock;
int _simple_response;
int _head_response;

#endif
