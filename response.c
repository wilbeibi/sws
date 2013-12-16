#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "net.h"
#include "response.h"

extern int debug;
extern FILE *log_fd;

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

void sws_response(int fd, Req_info *req) 
{
    req->contLen = err_response(fd, req->status);
    logging(req);
}

int err_response(int fd, int status) {
    char buf[MAXBUF], body[MAXBUF], msg[LINESIZE];
    get_status_msg(status, msg);
    char date[256];
    sprintf(body, "<!DOCTYPE html><html><title>SWS Response</title>\r\n");
    sprintf(body, "%s<body>%d: %s\r\n", body, status, msg);
    sprintf(body, "%s May the force be with you.</body></html>\r\n", body);
    
    if (_simple_response !=1 ) {
        sprintf(buf, "HTTP/1.0 %d %s\r\n", status, msg);
        Send(fd, buf, strlen(buf),0);
        get_timestamp(date);
        sprintf(buf, "Date: %s\r\n",date);
        Send(fd, buf, strlen(buf), 0);
        sprintf(buf, "Server: Four0Four\r\n");
        Send(fd, buf, strlen(buf), 0);
        sprintf(buf, "Content-type: text/html\r\n");
        Send(fd, buf, strlen(buf), 0);
        sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
        Send(fd, buf, strlen(buf), 0);    
    }
    
    if (_head_response != 1)
        Send(fd, body, strlen(body), 0);
        
    return (int)strlen(body);
}


void get_status_msg(int code, char msg[]) {
    bzero(msg, LINESIZE);
    switch(code){
    case 200:
        strcpy(msg, "OK");
        break;
    case 304:
        strcpy(msg,"Not Modified");
        break;
    case 400:
        strcpy(msg, "Bad Request");
        break;
    case 403:
        strcpy(msg, "Forbidden");
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

void logging(Req_info *req) {
    if (logDir != NULL) {
         char msg[MAXBUF];
        req->fstLine[strlen(req->fstLine)-2] = '\0';
        sprintf(msg, "%s %s \"%s\" %d %d\n",req->clientIp,req->recvTime,req->fstLine,req->status,req->contLen);
        (void)fwrite(msg,sizeof(char),strlen(msg),log_fd);
        // fclose(fp);
    }    
}
