init: ksocket.h
	gcc -Wall -o init -I. -L. initksocket.c -lksocket