#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "net.h"
#include "parse.h"

/* return 0 if success */
int serve_request(int fd, Req_info *req){
	struct stat sbuf;
	if(stat(req->uri, &sbuf) < 0){
		err_response(fd, 404);
	}
	if(req->cgi == NO_CGI){		/* static */
		if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
			err_response(fd, 403);
			return 1;
		}
		//server_static(fd, req, sbuf.st_size);
	}
	else{						/* dynamic */
		if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
			err_response(fd, 403);
			return 1;
		}
		/* req' uri*/
		//server_dynamic(fd, req)

	}
	return 0;
}

int server_static(int fd, Req_info *req, int fsize){
	char buf[MAXBUF];
	int datafd;
	char *datap;
	/* char *fname = malloc(sizeof(char)*SIZE); */
	/* fname = basename(req->uri); */
	
	/* Send response headers to client */
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	sprintf(buf, "%sServer: Four0Four\r\n", buf);
	sprintf(buf, "%sContent-type: text/html\r\n", buf);
	Send(fd, buf, strlen(buf), 0);

	/* Send response body to client */
	if((datafd = open(req->uri, O_RDONLY, 0)) < 0){
		fprintf(stderr, "open error.\n");
		return 1;
	}
	
	if((datap = mmap(0, fsize, PROT_READ, MAP_PRIVATE, datafd, 0)) == MAP_FAILED){
		fprintf(stderr, "open error.\n");
		return 1;
	}
	Close(datafd);
	Send(fd, datap, fsize, 0);	/* fsize == strlen(datap) */
	munmap(datap, fsize);
	return 0;
}

int server_dynamic(){
	
	return 0;
}

int main(int argc, char *argv[])
{
    
    return 0;
}
