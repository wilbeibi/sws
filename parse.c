#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "net.h"
#include "parse.h"


void init_req(Req_info * req) 
{
	req->status = 200;
	req->method = NOT_IMPLEMENTED;
	req->cgi = NO_CGI;
	req->uri = NULL;
}

int read_req_line(int sock,Req_info * req,int max,char* buf)
{
	int rval = 0, w=0;
	char c;
	while (w<max) {
		rval = read(sock,&c,1);
		if (rval == 1) {
			if (c != '\n' &&  c!='\r')
				buf[w++] = c;
			else if ( w != 0)
				break;
		}
		else if (rval == 0) 
			break; /* EOF */	
			
		else if (rval == -1){
			req->status = 500;
			break;
		}		
	}
	buf[w] = '\0';		
	return w;
	
}

void read_rest(int sock, Req_info * req, char* buf)
{
	int rval = 0, max =1024, w = 0, i = 1;
	char * tmp,tmpbuf[max];
	
	while(1) {		
		rval = read(sock,tmpbuf,max);
		if (rval == 0) {
			break; /* EOF */
		}
		else if (rval == -1) {
			req->status = 500;
			break;
		}
		else {
			w += rval; 
			if (w > (i*max)) {
				i++;
				if ( (buf = realloc(buf,(i*max))) == NULL) {
					req->status = 500;
					break;
				}	
			}
			sprintf(buf,"%s%s",buf,tmpbuf);
			tmp=strstr(buf,"\r\n"); 
			if (tmp != NULL && tmp == buf) /* direct CRLF after first line*/ 
				break;
			else if ( (tmp = strstr(buf,"\r\n\r\n")) != NULL) /* two CRLFs */
				break;
		}	
	}
}

int parse_uri(char * src, Req_info * req, Arg_t *optInfo)
{
	char * tmp = src;
	char usr[256];
	char rest[256];
	int i;
	
	req->uri = (char*)malloc(1024*sizeof(char));
	if (req->uri == NULL) 
		req->status = 500;
	
	if (src[0] != '/')
		req->status = 400;
		
	if (strncmp(src,"/~",2) == 0) {
		tmp += 2;
		i = 0;
		while (*tmp != '/' && *tmp != '\0') {
			usr[i++] = *tmp++;
		}
		usr[i]='\0';
		strncpy(rest,tmp,256);
		sprintf(req->uri,"/home/%s/sws/%s",usr,rest);
			
	}
	else if (strncmp(src,"/cgi-bin/",9) == 0) {
		tmp = src;
		tmp += 9;
		sprintf(req->uri,"%s%s",optInfo->cgiDir,tmp);
		return DO_CGI;
	}
	return NO_CGI;
}

int parse_req_line(char * buf, Req_info * req, Arg_t *optInfo)
{	
	char method[10], uri[256], version[20];
	
	bzero(method,10);
	bzero(uri,256);
	bzero(version,20);
	sscanf(buf, "%s %s %s", method, uri, version);

	if (strcasecmp(method, "GET")==0) {
		req->method = GET;	
	}else if (strcasecmp(method, "POST")==0)
		req->method = POST;
	else if (strcasecmp(method, "HEAD")==0)
		req->method = HEAD;
	else{
		req->status = 501;	
		return -1;
	}
	
	if ((strcasecmp(version,"HTTP/0.9") != 0) && (strcasecmp(version,"HTTP/1.0") != 0)) {
		req->status = 505;	
		return -1;
	}
	
	req->cgi = parse_uri(uri,req, optInfo);
	
	return 0;
		
}

void read_sock(int sock, Arg_t *optInfo)
{
	int w = 0, max=1024;
	char * buf;
	Req_info  req;
	
	buf = (char*)malloc((max-1)*sizeof(char));
	if (buf == NULL) 
		req.status = 500;
		
	init_req(&req);
	/* read first line*/
	w = read_req_line(sock,&req,max,buf);
	printf("first line: %s\n",buf);
	
	parse_req_line(buf,&req,optInfo);
	if (req.status != 200)
		;//make_response(req.status);
		
	else{
		bzero(buf, max);
		read_rest(sock,&req,buf);
		
		//serve_request(&req);
	}
	free(buf);
}


