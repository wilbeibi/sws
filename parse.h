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
	int contLen;
	char uri[256];
	char fstLine[256];				/* for logging */
	char clientIp[INET_ADDRSTRLEN]; /* for logging */
	char recvTime[256];				/* for logging */
}Req_info;

void read_sock(int sock, Req_info *req, Arg_t *optInfo);

#endif
