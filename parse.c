#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "net.h"
#include "parse.h"

void err_response(int fd, Req_info *req);
void get_status_msg(int code, char msg[]);

void init_req(Req_info * req) 
{
    req->status = 200;
    req->method = NOT_IMPLEMENTED;
    req->cgi = NO_CGI;
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
read_req_line(int sock, Req_info *req, char *buf)
{
    int ret=0;	
	while(1) {
		if ((ret=Readline(sock, buf))==-1) {
	        req->status=500;
	        break;
	    } else if (strcmp(buf,"\r\n") == 0){
			continue;
		}
		else if(ret<2 || buf[ret-2] !='\r'|| ret > MAXBUF) {
			req->status=400;
	         break;
	    }
		else
			break;
	}
	if (req->status != 200)
		return -1;
		
    return ret;
}

// int read_req_line(int sock,Req_info * req,int max,char* buf)
// {
//     int rval = 0, w=0;
//     char c;
//     while (w<max) {
//         rval = read(sock,&c,1);
//         if (rval == 1) {
//             if (c != '\n' &&  c!='\r')
//                 buf[w++] = c;
//             else if ( w != 0)
//                 break;
//         }
//         else if (rval == 0) 
//             break; /* EOF */    
//             
//         else if (rval == -1){
//             req->status = 500;
//             break;
//         }        
//     }
//     buf[w] = '\0';        
//     return w;
//     
// }

/**
 * read all rest content, including headers and entity-body, it should be parted
 * into read_headers and read_body...
 */
static int
read_rest(int sock, Req_info *req, char *buf)
{
    int ret;
    int tot=0;
	char * tmp = buf;
    while ((ret=Readline(sock, tmp))) {
        if (ret==-1) {
            req->status=500;
            return ret;
        }
        if (ret>=MAXBUF) {
            req->status=400;
            return ret;
        }
        tmp+=ret;
        tot+=ret;
        if (tot>=MAXBUF) {
            req->status=400;
            return ret;
        }
		if (tmp == (buf+2) && strcmp(buf,"\r\n") == 0)
			break;
		if (strcmp(tmp-4,"\r\n\r\n") == 0)
			break;
    }
    return tot;
}

// void read_rest(int sock, Req_info * req, char* buf)
// {
//     int rval = 0, max =1024, w = 0, i = 1;
//     char * tmp,tmpbuf[max];
//     
//     while(1) {        
//         rval = read(sock,tmpbuf,max);
//         if (rval == 0) {
//             break; /* EOF */
//         }
//         else if (rval == -1) {
//             req->status = 500;
//             break;
//         }
//         else {
//             w += rval; 
//             if (w > (i*max)) {
//                 i++;
//                 if ( (buf = realloc(buf,(i*max))) == NULL) {
//                     req->status = 500;
//                     break;
//                 }    
//             }
//             sprintf(buf,"%s%s",buf,tmpbuf);
//             tmp=strstr(buf,"\r\n"); 
//             if (tmp != NULL && tmp == buf) /* direct CRLF after first line*/ 
//                 break;
//             else if ( (tmp = strstr(buf,"\r\n\r\n")) != NULL) /* two CRLFs */
//                 break;
//         }    
//     }
// }

/**
 * We only need to implement [ path ], at this point, uri doesn't contain SP
 * Request-URI      = abs_path
 * abs_path         = "/" rel_path
 * rel_path         = [ path ] [ ";" params ] [ "?" query ]
 * path             = fsegment *( "/" segment )
 * fsegment         = 1*pchar
 * segment          = *pchar
 */
int parse_uri(Req_info * req, Arg_t *optInfo)
{
    char * tmp = req->uri;
    char usr[256];
    char rest[256];
    int i;

    if (req->uri[0] != '/') {
        req->status = 400;
        return -1;
    }

    if (strncmp(req->uri,"/~",2) == 0) {
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
    if (strncmp(req->uri,"/cgi-bin/",9) == 0) {
        req->cgi=DO_CGI;
        tmp = req->uri;
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
    char method[10], version[20];
    
    bzero(method,10);
    bzero(req->uri,256);
    bzero(version,20);
    sscanf(buf, "%s %s %s", method, req->uri, version);

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
    
    if ((strcasecmp(version,"HTTP/0.9") != 0)
        && (strcasecmp(version,"HTTP/1.0") != 0)) {
        req->status = 505;    
        return -1;
    }
    
    return 0;
}

void read_sock(int sock, Req_info *req, Arg_t *optInfo)
{
    int ret;
    char buf[MAXBUF];
    
    init_req(req);

    /* read first line*/
    ret=read_req_line(sock,req,buf);

    if (ret==-1) {
        err_response(sock, req);
        return;
    }

    printf("first line: %s\n",buf);
    
    ret=parse_req_line(buf,req,optInfo);
    if (ret==-1) {
        err_response(sock, req);
        return;
    }
        
    ret=parse_uri(req, optInfo);
    if (ret==-1) {
        err_response(sock, req);
        return;
    }
    /**
     * at this point, buf only contains request-line and the position is at
     * buf[0] because we didn't do buf+=w+1. so read_rest will override
     * request-line, which is acceptable here.
     */
    bzero(buf, MAXBUF);
    ret=read_rest(sock,req,buf);
    if (ret==-1) {
        err_response(sock, req);
        return;
    }

	
    //serve_request(req);
	return;
}


void err_response(int fd, Req_info *req) {
	char buf[MAXBUF], body[MAXBUF], msg[LINESIZE];
	get_status_msg(req->status, msg);

	sprintf(buf, "HTTP/1.0 %d %s\r\n", req->status, msg);
	Send(fd, buf, strlen(buf),0);
	sprintf(buf, "Content-type: text/html\r\n");
	Send(fd, buf, strlen(buf),0);
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Send(fd, buf, strlen(buf),0);
	
	sprintf(body, "<html><title>SWS Error</title>\r\n");
	sprintf(body, "<body>%s%d: %s\r\n", body, req->status, msg);
	sprintf(body, "%s May the force be with you.</body></html>\r\n", body);
	Send(fd, body, strlen(body),0);
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

