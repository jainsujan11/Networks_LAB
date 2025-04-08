run: ksocket.h
	gcc -Wall -o user1 -I. -L.  user1.c -lksocket
	gcc -Wall -o user2 -I. -L. user2.c -lksocket

clean:
	rm ksocket.o libksocket.a user1 user2 init new_*.txt