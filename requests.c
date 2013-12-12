#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <libgen.h>
#include <dirent.h>

#include "net.h"
#include "parse.h"
#include "response.h"

int serve_static(int fd, Req_info *req, int fsize);
int serve_dir(int fd, Req_info * req);

/* return 0 if success */
int serve_request(int fd, Req_info *req){
	struct stat sbuf;
	if(stat(req->uri, &sbuf) < 0){
		err_response(fd, 404);
		return 1;
	}
	
	if(req->cgi == NO_CGI){		/* static */
		if( (S_ISREG(sbuf.st_mode)) && (S_IRUSR & sbuf.st_mode)) {
			return serve_static(fd, req, sbuf.st_size);
			
		}
		else if ((S_ISDIR(sbuf.st_mode)) && (S_IRUSR & sbuf.st_mode)) {			
			return serve_dir(fd, req);
		}
		else {
			err_response(fd, 403);
			return 1;
		}		
	}
	else {						/* dynamic */
		if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
			err_response(fd, 403);
			return 1;
		}
		/* req' uri*/
		//server_dynamic(fd, req)
	}
	return 0;
}

int serve_dir(int fd, Req_info * req) {
	
	DIR *dp;
	char index[256];
	char buf[MAXBUF];
	char content[MAXBUF];
	char date[256];
	struct stat sbuf;
	struct dirent *dirp;
	
	sprintf(index, "%sindex.html",req->uri);
	if( !lstat(index, &sbuf)){
		strncpy(req->uri,index,strlen(index)+1);
		return serve_static(fd, req,sbuf.st_size);
	}
	
	printf("dir detail\n");	
	if ((dp = opendir(req->uri)) == NULL ) {
		err_response(fd, 403);
		return 1;
	}
	sprintf(content,"<!DOCTYPE><html><head><title>Four0Four sws</title></head><body><h1>Index of %s:</h1><br/>",basename(req->uri));	
	while ((dirp = readdir(dp)) != NULL ) {
		if (dirp->d_name[0] != '.') {
			//printf("%s\n", dirp->d_name);
			sprintf(content, "%s<p>%s</p>",content,dirp->d_name);	
		}
	}		
	sprintf(content, "%s</body></html>",content);	
	closedir(dp);
	
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	get_timestamp(date);
	sprintf(buf, "%sDate: %s\r\n",buf,date);
	sprintf(buf, "%sServer: Four0Four\r\n", buf);
	sprintf(buf, "%sContent-type: text/html\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n\r\n", buf, (int)strlen(content));
	
	Send(fd, buf, strlen(buf), 0);
	Send(fd, content, strlen(content), 0);
	return 0;
}

int serve_static(int fd, Req_info *req, int fsize){
	char buf[MAXBUF];
	int datafd;
	char *datap;
	char date[256];
	
	if((datafd = open(req->uri, O_RDONLY, 0)) < 0){
		fprintf(stderr, "open error.\n");
		return 1;
	}
	
	if((datap = mmap(0, fsize, PROT_READ, MAP_PRIVATE, datafd, 0)) == MAP_FAILED){
		fprintf(stderr, "open error.\n");
		return 1;
	}
	Close(datafd);
	
	/* Send response headers to client */
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	get_timestamp(date);
	sprintf(buf, "%sDate: %s\r\n",buf,date);
	sprintf(buf, "%sServer: Four0Four\r\n", buf);
	sprintf(buf, "%sContent-type: text/html\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n\r\n", buf,fsize);
	Send(fd, buf, strlen(buf), 0);
	
	/* Send response body to client */
	Send(fd, datap, fsize, 0);	/* fsize == strlen(datap) */
	munmap(datap, fsize);
	return 0;
}

int serve_dynamic(){
	
	return 0;
}
