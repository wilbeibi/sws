UNAME:=$(shell uname -s)
PROG= sws
OBJS= main.o net.o util.o parse.o response.o requests.o 
#CFLAGS=-g -std=gnu99 -Wall -pedantic-errors
ifeq ($(UNAME),Darwin)
all: ${PROG}
${PROG}: ${OBJS}
	@echo $@ depends on $?
	cc ${CFLAGS} ${OBJS} -o ${PROG}
else
all: ${PROG}
${PROG}: ${OBJS}
	@echo $@ depends on $?
	cc ${CFLAGS} ${OBJS} -lbsd -o ${PROG}
endif
clean:
	rm -f *.o sws
