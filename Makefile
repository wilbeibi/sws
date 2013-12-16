ROOT=.
include $(ROOT)/Make.on.$(PLATFORM)
CC=gcc
PROG= sws
OBJS= main.o net.o util.o parse.o response.o requests.o
# LFLAG= -lmagic 
all:	$(PROG)
 
$(PROG):	$(OBJS)
			$(CC) $(CFLAGS) $(OBJS) -o $(PROG) $(LIB)
 
clean:
		rm -rf $(PROG) *.o a.out
