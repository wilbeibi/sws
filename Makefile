CFLAGS=-g -std=gnu99 -Wall -pedantic-errors
main: main.o net.o util.o
	gcc -o main main.o net.o util.o
main.o: main.c net.h
	gcc -c ${CFLAGS} main.c
net.o: net.c net.h
	gcc -c ${CFLAGS} net.c
util.o: util.c net.h
	gcc -c ${CFLAGS} util.c
clean:
	rm -rf *.o main
