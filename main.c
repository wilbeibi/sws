/* $$ main.c
 * 
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <bsd/libutil.h>

#include <libgen.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <netinet/in.h>

#include "net.h"

#define DEBUG 1

static void usage();
static char *progname;
static int debug=0;

struct pidfh *pfh;
static void clean_up()
{
    /* note we have closed pfh in child process */
    (void)pidfile_remove(pfh);
    return;
}

static void sig_term(int sig)
{
    clean_up();
    Signal(SIGTERM, SIG_DFL);
    raise(SIGTERM);
    return;
}

static void check_path(char * path)
{
	struct stat st;
	char * tmp;
	if (path != NULL){
        if (lstat(path, &st) == -1) 
            sys_err(path);			
        if ( !(st.st_mode & S_IFDIR ))
            sys_err("Invalid path. Not a dir");
        /* add '/' if there isn't one in the end of the dir */
        tmp = path;
        while (*tmp != '\0')
            tmp++;
        if (*(tmp-1) != '/') {
            *(tmp) = '/';
            *(tmp+1) = '\0';
        }
    }
}

int main(int argc, char *argv[])
{
    int opt = 0;
    Arg_t optInfo;
    progname = argv[0];
	FILE * fp = NULL;
	logDir = NULL;

    optInfo.cgiDir = NULL;
    optInfo.ipAddr = NULL;
    optInfo.logFile = NULL;
    optInfo.port = "8080"; 
    
    const char *optString = "c:dhi:l:p:";
    char *endptr;
    int pt;

    while((opt = getopt(argc, argv, optString)) != -1){
        switch(opt){
        case 'c':        /* CGI */
            optInfo.cgiDir = optarg;
            break;
        case 'd':        /* debugging mode */
            debug=1;
            break;
        case 'h':        /* usage summary */
            usage();
            break;
        case 'i':        /* bind to given IPv4/6 address */
            optInfo.ipAddr = optarg;
            break;
        case 'l':        /* Log all request to given file */
            optInfo.logFile = optarg;
            break;
        case 'p':        /* Listen on the given port */
            pt =  strtol(optarg, &endptr, 10);
            if(!*endptr && pt >= 0 && pt <= 65535)
                optInfo.port = optarg;
            else
                fprintf(stderr,"Please input Port Number between 0-65535\n"), exit(1);
            break;
        default:    
            usage();
            break;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc>1) {
        usage();
    }

    if (*argv == NULL)
        usage();

	/* check all paths from getopt is valid*/
    optInfo.dir= *argv;
    optInfo.dir = realpath(optInfo.dir,NULL);
    check_path(optInfo.dir);
    check_path(optInfo.cgiDir);

	if (optInfo.logFile) {
		char temp[256];
		strcpy(temp,optInfo.logFile);
		check_path(dirname(temp));
		logDir = strdup(optInfo.logFile);
		/*create log file if not exist*/
		fp = fopen( logDir, "a" );
		if (fp == NULL)
			sys_err("open log failed");
		fclose(fp);
	}

    /**
     * it should have different implementation on other platforms.
     * however, to daemonize:
     * fork -> setsid -> close fd0, 1, 2 and chdir
     * to handle pidfle:
     * flopen pidfile, write, remove on error and exit
     */
    if (!debug) {
        Signal(SIGTERM, sig_term);
        atexit(clean_up);
        int opid;
        pfh=pidfile_open("./pidfile", 0700, &opid);
        if (pfh==NULL) {
            if (errno==EEXIST)
                errx(1, "already running, pid: %d", opid);
            warn("can't open or create pidfile");
        }
        if (daemon(1, 0)==-1) {
            pidfile_remove(pfh);
            err(1, "can't daemonize");
        }
        pidfile_write(pfh);
    }
    
    printf("running with %d\n", getpid());
    server_listen(&optInfo);

    free(optInfo.port);
    free(optInfo.ipAddr);
    free(optInfo.dir);
	if (logDir) free(logDir);
    if (optInfo.cgiDir) free(optInfo.cgiDir);
    if (optInfo.logFile) free(optInfo.logFile);

    exit( EXIT_SUCCESS );
}

static void usage(){
    fprintf(stderr, "usage: %s <cdhilp> dir\n \
           -c dir Allow execution of CGIs from the given directory.\n \
           -d Enter debugging mode.\n \
           -h Print a short usage summary and exit.\n \
           -i address Bind to the given IPv4 or IPv6 address\n \
           -l file Log all requests to the given file.\n \
           -p port Listen on the given port.\n", progname);
    exit( EXIT_FAILURE );
}

