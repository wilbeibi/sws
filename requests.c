#include <stdio.h>

#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <libgen.h>
#include <dirent.h>

#ifdef __APPLE__
#include <stdlib.h>
#include <string.h>
#else

#include <bsd/string.h>
#include <bsd/stdlib.h>

#endif

#include "net.h"
#include "parse.h"
#include "response.h"

int serve_static(int fd, Req_info *req, int fsize);
int serve_dir(int fd, Req_info * req);

int get_wday(char *wday)
{
	if(!strcasecmp(wday,"Mon") || !strcasecmp(wday,"Monday")) return 1;
	else if (!strcasecmp(wday,"Tue") || !strcasecmp(wday,"Tuesday")) return 2;
	else if (!strcasecmp(wday,"Wed") || !strcasecmp(wday,"Wednesday")) return 3;
	else if (!strcasecmp(wday,"Thu") || !strcasecmp(wday,"Thursday") ) return 4;
	else if (!strcasecmp(wday,"Fri") || !strcasecmp(wday,"Friday")) return 5;
	else if (!strcasecmp(wday,"Sat") || !strcasecmp(wday,"Saturday")) return 6;
	else if (!strcasecmp(wday,"Sun") || !strcasecmp(wday,"Sunday")) return 0;
	else return -1;
}

int get_mth(char* mon)
{
	if (!strcasecmp(mon,"Jan")) return 0;
	else if (!strcasecmp(mon,"Feb")) return 1;
	else if (!strcasecmp(mon,"Mar")) return 2;
	else if (!strcasecmp(mon,"Apr")) return 3;
	else if (!strcasecmp(mon,"May")) return 4;
	else if (!strcasecmp(mon,"Jun")) return 5;
	else if (!strcasecmp(mon,"Jul")) return 6;
	else if (!strcasecmp(mon,"Aug")) return 7;
	else if (!strcasecmp(mon,"Sep")) return 8;
	else if (!strcasecmp(mon,"Oct")) return 9;
	else if (!strcasecmp(mon,"Nov")) return 10;
	else if (!strcasecmp(mon,"Dec")) return 11;
	else return -1;
}

/* idea derived from mini_httpd */
time_t get_timet(char * str)
{
	time_t t;
	struct tm tm;
	char wday[10], mon[10];
	int year,mday,hour,min,sec,w_day,mth;
	char * tmp = str;	
	memset(&tm,0,sizeof(struct tm));	
	while ( *tmp == ' ' )
		tmp++;		
	/*  An example of the field is:
	      If-Modified-Since: Sat, 29 Oct 1994 19:43:31 GMT	
	
		HTTP-date = rfc1123-date | rfc850-date | asctime-date
		
		add more format later
	*/
	
	/* wkday, DD mth year HH:MM:SS GMT (rfc1123-date) */ 
	if (sscanf(tmp,"%400[a-zA-Z], %d %400[a-zA-Z] %d %d:%d:%d GMT",wday,&mday,mon,&year,&hour,&min,&sec) == 7 &&
		(w_day = get_wday(wday)) != -1 && (mth = get_mth(mon)) != -1 ) 
	{
		tm.tm_wday = w_day;
		tm.tm_mday = mday;
		tm.tm_mon = mth;
		tm.tm_year = year;
		tm.tm_hour = hour;
		tm.tm_min = min;
		tm.tm_sec = sec;
		printf("%s\n",str);
		printf("%d %d %d %d %d:%d:%d\n",year,mth,mday,w_day,hour,min,sec);
	}
	
	/* weekday, DD-mth-YY HH:MM:SS GMT (rfc850-date)*/ 
	else if (sscanf( tmp, "%[a-zA-Z], %d-%400[a-zA-Z]-%d %d:%d:%d GMT",wday,&mday,mon,&year,&hour,&min,&sec) == 7 &&
		(w_day = get_wday(wday)) != -1 && (mth = get_mth(mon)) != -1 )
	{
		w_day = get_wday(wday);
		mth = get_mth(mon);
		
		tm.tm_wday = w_day;
		tm.tm_mday = mday;
		tm.tm_mon = mth;
		tm.tm_year = year;
		tm.tm_hour = hour;
		tm.tm_min = min;
		tm.tm_sec = sec;
		printf("%s\n",str);
		printf("%d %d %d %d %d:%d:%d\n",year,mth,mday,w_day,hour,min,sec);
	}
	else return (time_t)-1;
	
	if ( tm.tm_year > 1900 )
		tm.tm_year -= 1900;
    else if ( tm.tm_year < 70 )
		tm.tm_year += 100;	
		
	t = timegm(&tm);	
	return t;
}

int check_modified_since( Req_info * req, struct stat *st)
{
	if (!strlen(req->ifModified)) {
		printf("no if modified since header\n");
		return 0;
	}
		
	time_t t = get_timet(req->ifModified);
	
	printf("ims mtime:%ld\n",t);
	printf("stat mtime:%ld\n",st->st_mtime);
	if ( t != (time_t) -1 && t >= st->st_mtime ) {
		return 1;
	}		
    return 0;
}

/* return 0 if success */
int serve_request(int fd, Req_info *req){
	struct stat sbuf;
	if(stat(req->uri, &sbuf) < 0){
		req->status = 404;
		sws_response(fd, req);
		return 1;
	}
	
	if (check_modified_since(req, &sbuf)) {
		req->status = 304;
		sws_response(fd, req);
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
			req->status = 403;
			sws_response(fd, req);
			return 1;
		}		
	}
	else {						/* dynamic */
		if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
			req->status = 403;
			sws_response(fd, req);
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
	struct stat sb;
	
	struct dirent *dirp;
	char last_modified[256];
	char path[256];
	char permission[20];
	if (req->uri[strlen(req->uri)-1]!='/')
		strcat(req->uri,"/");
	sprintf(index, "%sindex.html",req->uri);
	if( !lstat(index, &sbuf)){
		strncpy(req->uri,index,strlen(index)+1);
		return serve_static(fd, req,sbuf.st_size);
	}
	
	if ((dp = opendir(req->uri)) == NULL ) {
		req->status = 403;
		sws_response(fd, req);
		return 1;
	}
	sprintf(content,"<!DOCTYPE><html><head><title>Four0Four sws</title></head><body><h1>Index of %s:</h1><br/><table><tr><th align='left'>Permission:</th><th align='left'>Name:</th><th align='right'>Last_Modified:</th></tr><tr><th colspan='5'><hr></th></tr>",basename(req->uri));	
	
	while ((dirp = readdir(dp)) != NULL ) {
		if (dirp->d_name[0] != '.') {
			sprintf(path,"%s%s",req->uri,dirp->d_name);
			if (stat(path, &sb) == -1) {
				if (lstat(path, &sb) == -1) {
					strcpy(last_modified,"Cannot stat\0");
				}
			} 
			else {
				strmode(sb.st_mode, permission);
				struct tm * tmp;
				tmp = gmtime(&sb.st_mtime);
				char *Wday[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
				char *Mth[]={"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
				sprintf(last_modified,"%s, %d %s %d %d:%d:%d GMT",Wday[tmp->tm_wday],(tmp->tm_mday),Mth[tmp->tm_mon],(1900+tmp->tm_year),(tmp->tm_hour),(tmp->tm_min),(tmp->tm_sec));	
			}			
			sprintf(content, "%s<tr><td align='left'>%s</td><td align='left'>%s</td><td align='right'>%s</td></tr>",content,permission,dirp->d_name,last_modified);	
		}
	}		
	sprintf(content, "%s<tr><th colspan='5'><hr></th></tr></table><br/><span>Four0Four Server page</span></body></html>",content);	
	closedir(dp);
	
	if (_simple_response !=1 ) {
		sprintf(buf, "HTTP/1.0 200 OK\r\n");
		get_timestamp(date);
		sprintf(buf, "%sDate: %s\r\n",buf,date);
		sprintf(buf, "%sServer: Four0Four\r\n", buf);
		sprintf(buf, "%sContent-type: text/html\r\n", buf);
		sprintf(buf, "%sContent-length: %d\r\n\r\n", buf, (int)strlen(content));
		Send(fd, buf, strlen(buf), 0);
	}
	
	if (_head_response != 1)
		Send(fd, content, strlen(content), 0);
		
	req->contLen = (int)strlen(content);
	logging(req);
	return 0;
}

int serve_static(int fd, Req_info *req, int fsize){
	char buf[MAXBUF];
	int datafd;
	char *datap;
	char date[256];
	
	if((datafd = open(req->uri, O_RDONLY, 0)) < 0){
		req->status = 403;
		sws_response(fd, req);
		fprintf(stderr, "open error.\n");
		return 1;
	}
	
	if((datap = mmap(0, fsize, PROT_READ, MAP_PRIVATE, datafd, 0)) == MAP_FAILED){
		req->status = 403;
		sws_response(fd, req);
		fprintf(stderr, "open error.\n");
		return 1;
	}
	Close(datafd);
	
	if (_simple_response !=1 ) {
		/* Send response headers to client */
		sprintf(buf, "HTTP/1.0 200 OK\r\n");
		get_timestamp(date);
		sprintf(buf, "%sDate: %s\r\n",buf,date);
		sprintf(buf, "%sServer: Four0Four\r\n", buf);
		sprintf(buf, "%sContent-type: text/html\r\n", buf);
		sprintf(buf, "%sContent-length: %d\r\n\r\n", buf,fsize);
		Send(fd, buf, strlen(buf), 0);
	}

	/* Send response body to client */
	if (_head_response != 1)
		Send(fd, datap, fsize, 0);	/* fsize == strlen(datap) */
		
	req->contLen = fsize;
	logging(req);
	munmap(datap, fsize);
	return 0;
}

int serve_dynamic(){
	
	return 0;
}
