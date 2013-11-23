/*
  TODO: Add Send() in util.c
 */
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define LINESIZE 1024
#define BUFSIZE 8192
/* in parse.h, remove later */
typedef struct req_info {
	int method;
	int status;
	int cgi;
	char * uri;
}Req_info;

void err_response(int fd, Req_info * req) {
	char buf[BUFSIZE], body[BUFSIZE], msg[LINESIZE];
	get_status_msg(req->status, msg);
	sprinf(body, "<html><title>SWS Error</title>\r\n");
	sprinf(body, "<body>%s%s: %s\r\n", body, req->status, msg);
	sprinf(body, "%s May the force be with you.</body></html>\r\n", body);

	sprinf(buf, "HTTP/1.0 %s %s\r\n", req->status, msg);
	Send(fd, buf, strlen(buf));
	sprinf(buf, "Content-type: text/html\r\n");
	Send(fd, buf, strlen(buf));
	sprinf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Send(fd, buf, strlen(buf));
	Send(fd, body, strlen(body));
}

void get_status_msg(int code, char msg[]) {
	bzero(msg, LINESIZE);
	switch(code){
	case 200:
		strcpy(msg, "OK");
		break;
	case 400:
		strcpy(msg, "Bad Request");
		break;
	case 404:
		strcpy(msg, "Not Found");
		break;
	case 501:
		strcpy(msg, "Not Implemented");
		break;
	case 505:
		strcpy(msg, "Version Not Support");
		break;
	case 522:
		strcpy(msg, "Connection Timed Out");
		break;
	default:
		strcpy(msg, "Unrecognized Statues");
		break;
	}
}
