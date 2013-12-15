UNAME:=$(shell uname -s)
PROG= sws
OBJS= main.o net.o util.o parse.o response.o requests.o 
CFLAGS=-g -std=gnu99 -Wall -pedantic-errors
ifeq ($(UNAME),Darwin)
else
LFLAG=-lbsd 
endif
all: ${PROG}
${PROG}: ${OBJS}
	@echo $@ depends on $?
	cc ${CFLAGS} ${OBJS} ${LFLAG} -o ${PROG}

clean:
	rm -f *.o sws
