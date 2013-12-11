PROG=main
OBJS=main.o net.o util.o parse.o
LIBS=-lbsd
CFLAGS=-g -std=gnu99 -Wall -pedantic-errors

${PROG}: ${OBJS}
	gcc ${CFLAGS} ${OBJS} -o ${PROG} ${LIBS}
clean:
	rm -rf *.o main
