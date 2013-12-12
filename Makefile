PROG= sws
OBJS= main.o net.o util.o parse.o response.o requests.o 
#CFLAGS=-g -std=gnu99 -Wall -pedantic-errors
all: ${PROG}
${PROG}: ${OBJS}
	@echo $@ depends on $?
	cc ${CFLAGS} ${OBJS} -o ${PROG}
clean:
	rm -f *.o sws
