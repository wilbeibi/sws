/* $$ net.c
 *  
 */

#ifdef __linux 
#include <bsd/libutil.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "net.h"
#include "parse.h"

#define DEBUG 1

static void child_handler(int sig);

/* pfh is defined in main.c */
extern struct pidfh *pfh;

int server_listen( Arg_t *optInfo){
    struct addrinfo hints, *res;
    char ipstr[INET_ADDRSTRLEN];
    int listenfd, connfd;
    struct sockaddr_storage client_addr;
    socklen_t clientlen;
    int ignore = 1;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; /* Should support IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; 
    if(optInfo->ipAddr == NULL){
        hints.ai_flags = AI_PASSIVE;
        Getaddrinfo(NULL, optInfo->port, &hints, &res);
    }
    else{
        Getaddrinfo(optInfo->ipAddr, optInfo->port, &hints, &res);
    }
    
    listenfd = Socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    /* Avoid Address already in used error */
    Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &ignore, sizeof(int));
    Bind(listenfd, res->ai_addr, res->ai_addrlen);
    Listen(listenfd, LISTENQ);

    signal(SIGCHLD, child_handler);

    while(1){
        pid_t childpid;
        struct sockaddr_in* cliAdr = (struct sockaddr_in*)&client_addr;
        int ipAddr = cliAdr->sin_addr.s_addr;
        clientlen = sizeof(client_addr);

        Req_info req;
        struct timeval to;
        fd_set rdy;

        to.tv_sec=5;
        to.tv_usec=0;

        FD_ZERO(&rdy);
        FD_SET(listenfd, &rdy);

        int ret=select(listenfd+1, &rdy, 0, 0, &to);
        if (ret==-1)
            continue;
        else if (ret==0)
            continue;
        else if (FD_ISSET(listenfd, &rdy)==0)
            // do some fun stuff...
            continue;

        connfd = Accept(listenfd, (struct sockaddr *) &client_addr, &clientlen);
        // there is noneed to pop this for each connection
        // printf ("Server is up on port:%s.\n", optInfo->port);
    
        /* Communicate with client */
        if( (childpid = fork()) == 0){
#ifdef __linux
            pidfile_close(pfh);
#else
            fclose( pidfp );
#endif
            signal(SIGTERM, SIG_DFL);
            Close(listenfd);
            Inet_ntop(AF_INET, &ipAddr, ipstr, INET_ADDRSTRLEN); /* Get client address in ipstr */
            memset(req.clientIp,0,sizeof(req.clientIp));
            strncpy(req.clientIp,ipstr,INET_ADDRSTRLEN);
            read_sock(connfd, &req, optInfo);    
        
            exit(0);
        }
        
        Close(connfd);
    }
    freeaddrinfo(res);
    return connfd;
}



static void child_handler(int sig){
    pid_t p;
    int status;
    while( (p = waitpid(-1, &status, WNOHANG)) != -1)
        ;
}



