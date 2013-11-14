/* $$ main.c
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#include "net.h"


#define DEBUG 1

static void usage();
static char *progname;

int main(int argc, char *argv[])
{
    int opt = 0;
    Arg_t optInfo;
    progname = argv[0];
    
    optInfo.cgiDir = NULL;
    optInfo.ipAddr = NULL;
    optInfo.logFile = NULL;
    optInfo.port = "8080";
    
    
   
    
    const char *optString = "c:dhi:l:p:";
    char *endptr;
    int pt;
    while((opt = getopt(argc, argv, optString)) != -1){
		switch(opt){
		case 'c':		/* CGI */
			optInfo.cgiDir = optarg;
			break;
        case 'd':		/* debugging mode */
			break;
		case 'h':		/* usage summary */
			usage();
			break;
		case 'i':		/* bind to given IPv4/6 address */
			optInfo.ipAddr = optarg;
			break;
		case 'l':		/* Log all request to given file */
			optInfo.logFile = optarg;
			break;
		case 'p':		/* Listen on the given port */
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

    server_listen(&optInfo);

    free(optInfo.cgiDir);
    free(optInfo.ipAddr);
    free(optInfo.logFile);
    free(optInfo.port);
    
    exit( EXIT_SUCCESS );
}

static void usage(){
    fprintf(stderr, "usage: %s <cdhilp>\n \
           -c dir Allow execution of CGIs from the given directory.\n \
           -d Enter debugging mode.\n \
           -h Print a short usage summary and exit.\n \
           -i address Bind to the given IPv4 or IPv6 address\n \
           -l file Log all requests to the given ﬁle.\n \
           -p port Listen on the given port.\n", progname);
    exit( EXIT_FAILURE );
}

