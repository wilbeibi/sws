/* $$ util.c
 * Stolen the idea from UNP
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "net.h" 
#include "parse.h"
void sys_err(const char *msg){
    perror(msg);
    exit( EXIT_FAILURE );
}


void *Calloc(size_t n, size_t size){
    void *p;
    if( (p = calloc(n,size)) == NULL)
		sys_err("calloc error");
    return (p);
}

int Socket(int family, int type, int protocol){
    int n;
    if( (n = socket(family, type, protocol)) < 0)
		sys_err("socket error");
    return (n);
}

int Accept(int fd, struct sockaddr *sa, socklen_t *addrlen){
    int n;
    if( (n = accept(fd, sa, addrlen)) < 0){
		sys_err("accept error");
    }
    return (n);
}

void Bind(int fd, const struct sockaddr *sa, socklen_t addrlen){
    if(bind(fd, sa, addrlen) < 0)
		sys_err("bind error");
}

void Connect(int fd, const struct sockaddr *sa, socklen_t addrlen){
    if(connect(fd, sa, addrlen) < 0)
		sys_err("connect error");
}

void Listen(int fd, int backlog){
    if(listen(fd, backlog) < 0)
		sys_err("listen error");
}

void Setsockopt(int fd, int level, int optname, const void *optval, int optlen){
    if(setsockopt(fd, level, optname, optval, optlen) < 0)
		sys_err("setsockopt error");
}


void Close(int fd){
    if(close(fd) == -1)
		sys_err("close error");
}

void Inet_pton(int family, const char *strptr, void *addrptr){
    int n;
    if( (n = inet_pton(family, strptr, addrptr)) <= 0){
		fprintf(stderr, "inet error ->%s", strptr);
		exit ( EXIT_FAILURE );
    }
}

const char *Inet_ntop(int family, const void *addrptr, char *strptr, size_t len){
    const char *ptr;
    if ( (ptr = inet_ntop(family, addrptr, strptr, len)) == NULL)
		sys_err("inet_ntop error");            
    return(ptr);
}
int Getaddrinfo(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res){
    int n;
    if( (n = getaddrinfo(hostname, servname, hints, res)) != 0){
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(n));
		exit(1);
    }
    return (n);
}

ssize_t Read(int fd, void *buf, size_t count){
    int n;
    if ((n = read(fd, buf, count)) < 0)
		fprintf(stderr, "Read warning: %s.\n", strerror());
    return (n);
}

ssize_t Write(int fd, const void *buf, size_t count){
    int n;
    if ((n = write(fd, buf, count)) < 0)
		fprintf(stderr, "Write warning: %s.\n", strerror(errno));
    return (n);
}

sig_t Signal(int sig, sig_t func){
    sig_t s;
    if((s = signal(sig, func)) == SIG_ERR)
		sys_err("signal child error");
    return (s);
}

int Readline(int fd, char* buf)
{
    int n, rc;
    char c, *bufp = buf;
 
    for (n = 1; n < MAXBUF; n++) { 
		if ((rc = read(fd, &c, 1)) == 1) {
			*bufp++ = c;
			if (c == '\n')
				break;
		} else if (rc == 0) {
			if (n == 1)
				return 0;   /* EOF, no data read */
			else
				break;      /* EOF, some data was read */
		} else
			return -1;	    /* error */
    }
    *bufp = 0;		    	/* append '\0' */
    return n;
}
