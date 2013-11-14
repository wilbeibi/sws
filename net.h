/* $$ net.h
 *
 */

#ifndef _NET_H_
#define _NET_H_
typedef struct arg_t{
    char *cgiDir;    /* -c dir */
    char *ipAddr;    /* -i address */
    char *logFile;   /* -l file */
    char *port;	     /* -p port */
}Arg_t;



#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#define WATCH(fmt, ...)							\
    if (DEBUG) printf(fmt, __VA_ARGS__);

#define SIZE 1024
#define LISTENQ 10

/* typedef struct sockaddr SA; */


/****************************************
 *    Wrap basic functions in util.c
 ****************************************/

void sys_err(const char *msg);
void *Calloc(size_t n, size_t size);
int Socket(int family, int type, int protocol);
int Accept(int fd, struct sockaddr *sa, socklen_t *addrlen);
void Bind(int fd, const struct sockaddr *sa, socklen_t addrlen);
void Connect(int fd, const struct sockaddr *sa, socklen_t addrlen);
void Listen(int fd, int backlog);
void Setsockopt(int fd, int level, int optname, const void *optval,int optlen);
void Close(int fd);
void Inet_pton(int family, const char *strptr, void *addrptr);
const char *Inet_ntop(int family, const void *addrptr, char *strptr, size_t len);
int Getaddrinfo(const char *hostname, const char *servname, const struct addrinfo *hints, struct addrinfo **res);
ssize_t Read(int fd, void *buf, size_t count);
ssize_t Write(int fd, const void *buf, size_t count);
sig_t Signal(int sig, sig_t func);



/****************************************
 *    Network related functions in net.c 
 ****************************************/

int server_listen(Arg_t *optInfo);

#endif	/* _NET_H_ */
