library: ksocket.o
	ar rcs libksocket.a ksocket.o

ksocket.o: ksocket.h
	gcc -Wall -c -I. ksocket.c