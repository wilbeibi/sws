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

/**
 * Full-Request = Request-Line 
 *                *( General-Header
 *                 | Request-Header
 *                 | Entity-Header )
 *                CRLF
 *                [ Entity-Body ]
 */
static int 
read_req_line(int sock, Req_info *req, int max, char *buf)
{
    /**
     * stt=0   meet other 
     * stt=1   meet \r
     * stt=2   meet \n after \r
     */
    int ret=0;
    int stt=0;
    int ind=0;

    char c;
    while ((ret=read(sock, &c, 1))!=0) {
        buf[ind++]=c;
        if (ret==-1) {
            req->status=500;
            return -1;
        }
        if ((stt==0 && c=='\n') || (stt==1 && c!='\n') || stt==2) {
            req->status=400;
            return -1;
        }
        if (stt==0 && c=='\r') {
            stt=1;
            continue;
        }
        if (stt==1 && c=='\n') {
            stt=2;
            break;
        }
    }
    if (stt!=2) {
        req->status=400;
        return -1;
    }

    return ind;
}

// int read_req_line(int sock,Req_info * req,int max,char* buf)
// {
// 	int rval = 0, w=0;
// 	char c;
// 	while (w<max) {
// 		rval = read(sock,&c,1);
// 		if (rval == 1) {
// 			if (c != '\n' &&  c!='\r')
// 				buf[w++] = c;
// 			else if ( w != 0)
// 				break;
// 		}
// 		else if (rval == 0) 
// 			break; /* EOF */	
// 			
// 		else if (rval == -1){
// 			req->status = 500;
// 			break;
// 		}		
// 	}
// 	buf[w] = '\0';		
// 	return w;
// 	
// }

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

/**
 * We only need to implement [ path ], at this point, uri doesn't contain SP
 * Request-URI      = abs_path
 * abs_path         = "/" rel_path
 * rel_path         = [ path ] [ ";" params ] [ "?" query ]
 * path             = fsegment *( "/" segment )
 * fsegment         = 1*pchar
 * segment          = *pchar
 */
int parse_uri(char * src, Req_info * req, Arg_t *optInfo)
{
	char * tmp = src;
	char usr[256];
	char rest[256];
	int i;

	if (src[0] != '/') {
		req->status = 400;
        return -1;
    }
		
    strncpy(req->uri, src, strlen(src));

	if (strncmp(src,"/~",2) == 0) {
		tmp += 2;
		i = 0;
		while (*tmp != '/' && *tmp != '\0') {
			usr[i++] = *tmp++;
		}
		usr[i]='\0';
        /* if user not exist, return 400 */
        struct passwd *pwd;
        if ((pwd=getpwnam(usr))==NULL) {
            req->status=400;
            return -1;
        }
        free(pwd);
		strncpy(rest,tmp,256);
		sprintf(req->uri,"/home/%s/sws/%s",usr,rest);
			
	}

    req->cgi=NO_CGI;
	if (strncmp(src,"/cgi-bin/",9) == 0) {
        req->cgi=DO_CGI;
		tmp = src;
		tmp += 9;
		strncpy(rest,tmp,256);
		sprintf(req->uri,"%s%s",optInfo->cgiDir,tmp);
	}

    /* path convertion, first tokenize, then combine up */
    char tos[256][256];
    int add[256];
    int ind0=0;
    int ind1=0;
    int len=strlen(req->uri);
    memset(add, 0, sizeof(add));
    for (int i=0; i<len; i++) {
        while (i<len && req->uri[i]=='/')
            i++; 
        while (i<len && req->uri[i]!='/')
            tos[ind0][ind1++]=req->uri[i++];
        if (strcmp(tos[ind0], "..")!=0)
            add[ind0]=1;
        else {
            add[ind0]=0;
            if (ind0>0)
                add[ind0-1]=0;
        }
        ind0++;
        ind1=0;
    }

    int ind=0;
    char buf[256];
    for (int i=0; i<ind0; i++) if (add[i]) {
        buf[ind++]='/';
        for (int j=0; j<strlen(tos[i]); j++)
            buf[ind++]=tos[i][j];
    }

    strncpy(req->uri, buf, strlen(buf));

    return 0;
}

/**
 * Note method is case-sensitive
 * SP               = <space>
 * CRLF             = <'\r\n'>
 * token            = 1*<any CHAR except CTLs or tspecials>
 *
 * Request-line     = Method SP Request-URI SP HTTP-Version CRLF
 * HTTP-Version     = "HTTP/" 1*DIGIT "." 1*DIGIT
 * Method           = "GET" | "POST" | "HEAD" | extension-method
 * extension-method = token
 */
int parse_req_line(char * buf, Req_info * req, Arg_t *optInfo)
{	
    int ret;
	char method[10], uri[256], version[20];
	
	bzero(method,10);
	bzero(uri,256);
	bzero(version,20);
	sscanf(buf, "%s %s %s", method, uri, version);

    if (strcmp(method, "GET")==0)
        req->method=GET;
    else if (strcmp(method, "POST")==0)
        req->method=POST;
    else if (strcmp(method, "HEAD")==0)
        req->method=HEAD;
    else {
        req->status=501;
        return -1;
    }

	// if (strcasecmp(method, "GET")==0) {
	// 	req->method = GET;	
	// }else if (strcasecmp(method, "POST")==0)
	// 	req->method = POST;
	// else if (strcasecmp(method, "HEAD")==0)
	// 	req->method = HEAD;
	// else{
	// 	req->status = 501;	
	// 	return -1;
	// }
	
	if ((strcasecmp(version,"HTTP/0.9") != 0) && (strcasecmp(version,"HTTP/1.0") != 0)) {
		req->status = 505;	
		return -1;
	}
	
	return 0;
}

void read_sock(int sock, Arg_t *optInfo)
{
	int w = 0, max=1024;
    int ret;
	char buf[max];
	Req_info  req;
	
	init_req(&req);

	/* read first line*/
	w = read_req_line(sock,&req,max,buf);
    if (w==-1)
        // TODO make_response;

	printf("first line: %s\n",buf);
	
	ret=parse_req_line(buf,&req,optInfo);
    if (ret==-1)
        // TODO make_response;
		
    ret=parse_uri(req.uri, optInfo);
    if (ret==-1)
        // TODO make_response;

	else{
		bzero(buf, max);
		read_rest(sock,&req,buf);
		
		//serve_request(&req);
	}
	free(buf);
}


