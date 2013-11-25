#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "net.h"
#include "parse.h"

/* TODO currently global, any further advices? */
static int _sock;
static int _simple_response;
static int _head_response;

static void
rd_timeout(int sig) {
    // response to client of 408?
    // log warn: read timeout
    err_response(_sock, 408);
    exit(0);
}

static void
wt_timeout(int sig) {
    // log warn: write timeout
    exit(0);
}

void err_response(int fd, int status);
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
		} else if(ret<2 || buf[ret-2] !='\r'|| ret > MAXBUF) {
			req->status=400;
	         break;
	    } else
			break;
	}
	if (req->status != 200)
		return -1;
		
    return ret;
}

/**
 * read all rest content, including headers and entity-body, it should be parted
 * into read_headers and read_body...but we read headeers only now.
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
	
	/* http://babla. shoudl also be valid
    if (req->uri[0] != '/') {
        req->status = 404;
        return -1;
    }
	*/
	
	/* According to sws man page, request for user home should start with '~'  */
    if (strncmp(req->uri,"~",1) == 0) {
        tmp += 1;
        i = 0;
        while (*tmp != '/' ) {
		/* there must be a slash after the user name, otherwise it's invalid*/
			if ( *tmp == '\0') {
				 req->status=404;
		            return -1;	
			}
            usr[i++] = *tmp++;
        }
        usr[i]='\0';
		
        /* if user not exist, return 404 */
        struct passwd *pwd;
        if ((pwd=getpwnam(usr))==NULL) {
            req->status=404;
            return -1;
        }      
        strncpy(rest,tmp+1,256);
        sprintf(req->uri,"/home/%s/sws/%s",usr,rest);
 
    }

    req->cgi=NO_CGI;
    if (strncmp(req->uri,"/cgi-bin/",9) == 0) {
        req->cgi=DO_CGI;
        tmp = req->uri;
        tmp += 9;
		strncpy(rest,tmp,256);
		if (optInfo->cgiDir != NULL)
        	sprintf(req->uri,"%s%s",optInfo->cgiDir,rest);

    }
	
    /* path convertion, first tokenize, then combine up */
    char buf[256];
    char temp[256];
    char *bufp=buf;
    int ind=0;
    int len=strlen(req->uri);
    buf[0]='/';
    buf[1]=0;
    for (int i=0; i<len; i++) {
        while (i<len && req->uri[i]=='/') i++; 
        while (i<len && req->uri[i]!='/') temp[ind++]=req->uri[i++];
        temp[ind]=0;
        if (temp[0]==0)
            continue;
        if (strcmp(temp, "..")!=0) {
            *bufp='/';
            bufp++;
            for (int j=0; j<ind; j++) {
                *bufp=temp[j];
                bufp++;
            }
            *bufp=0;
        } else {
            while (*bufp!='/') bufp--;
            if (bufp==buf) {
                *bufp='/';
                *(bufp+1)=0;
            } else
                *bufp=0;
        }
        ind=0;
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

    if (strcmp(method, "GET") == 0)
        req->method=GET;
    else if (strcmp(method, "POST") == 0)
        req->method=POST;
    else if (strcmp(method, "HEAD") == 0) {
		req->method=HEAD;
		_head_response = 1;
	}
    else {
        req->status=501;
        return -1;
    }

    if (version[0]==0) {
        // version is not specified, if is GET, validate it as HTTP/0.9 Simple request
		if ((req->method == GET) && req->uri[0] != '\0')
			_simple_response = 1;
		else {
			req->status=400;
	        return -1;
		}
    } else {
        char *http=strstr(version, "HTTP/");
        if (http==NULL || http!=version) {
            req->status=400;
            return -1;
        }
        http+=5;
        int major=-1;
        int minor=-1;
        sscanf(http, "%d.%d", &major, &minor);
        if ((major==1 && minor==0)
            || (major==0 && minor==9)) {
            return 0;
        }
        req->status=505;
        return -1;
    }

    if (req->uri[0]==0) {
        req->status=400;
        return -1;
    }
    
    return 0;
}

void read_sock(int sock, Req_info *req, Arg_t *optInfo)
{
    _sock=sock;
    signal(SIGALRM, rd_timeout);
    alarm(READ_TIMEOUT);

    int ret;
    char buf[MAXBUF];
    
    init_req(req);

    /* read first line*/
    ret=read_req_line(sock,req,buf);
    if (ret==-1) {
        err_response(sock, req->status);
        return;
    }
   
    ret=parse_req_line(buf,req,optInfo);
    if (ret==-1) {
        err_response(sock, req->status);
        return;
    }

    ret=parse_uri(req, optInfo);
    if (ret==-1) {
        err_response(sock, req->status);
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
        err_response(sock, req->status);
        return;
    }
	err_response(sock, req->status);
	
    signal(SIGALRM, wt_timeout);
    alarm(0);
    alarm(WRITE_TIMEOUT);

    //serve_request(req);
    return;
}

void get_timestamp(char *buf)
{
	time_t t;
	struct tm * tmp;
	char *Wday[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
	char *Mth[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
	time(&t);
	tmp = gmtime(&t);
	sprintf(buf,"%s, %d %s %d %d:%d:%d GMT",Wday[tmp->tm_wday],(tmp->tm_mday),Mth[tmp->tm_mon],(1900+tmp->tm_year),(tmp->tm_hour),(tmp->tm_min),(tmp->tm_sec));
	
}

void err_response(int fd, int status) {
	char buf[MAXBUF], body[MAXBUF], msg[LINESIZE];
	get_status_msg(status, msg);
	char date[256];
	sprintf(body, "<!DOCTYPE html><html><title>SWS Error</title>\r\n");
	sprintf(body, "%s<body>%d: %s\r\n", body, status, msg);
	sprintf(body, "%s May the force be with you.</body></html>\r\n", body);
	
	if (_simple_response !=1 ) {
		sprintf(buf, "HTTP/1.0 %d %s\r\n", status, msg);
		Send(fd, buf, strlen(buf),0);
		get_timestamp(date);
		sprintf(buf, "Date: %s\r\n",date);
		Send(fd, buf, strlen(buf),0);
		sprintf(buf, "Server: Four0Four\r\n");
		Send(fd, buf, strlen(buf),0);
		sprintf(buf, "Content-type: text/html\r\n");
		Send(fd, buf, strlen(buf),0);
		sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
		Send(fd, buf, strlen(buf),0);	
	}
	
	if (_head_response != 1)
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
	case 408:
		strcpy(msg, "Request Timeout");
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

