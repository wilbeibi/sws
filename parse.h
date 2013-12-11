#ifndef _PARSE_H_
#define _PARSE_H_

#define GET 1
#define POST 2
#define HEAD 3
#define NOT_IMPLEMENTED 0
#define NO_CGI 0
#define DO_CGI 1



typedef struct req_info {
    int method;
    int status;
    int cgi;
    char uri[256];
}Req_info;

void err_response(int fd, int status);
void read_sock(int sock, Req_info *req, Arg_t *optInfo);

#endif
