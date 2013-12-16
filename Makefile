ROOT=.
include $(ROOT)/Make.on.$(PLATFORM)
 
CC=gcc
PROG= main
OBJS= main.o net.o util.o parse.o response.o requests.o
LFLAG= -lmagic
all:	$(PROG)
 
$(PROG):	$(OBJS)
			$(CC) $(CFLAGS) $(OBJS) $(LFLAGS) -o $(PROG)
 
clean:
		rm -rf $(PROG) *.o a.out
