all: a2

a2: list.o client.o
	gcc -o s-talk client.o list.o -pthread

list.o: list.h list.c
	gcc -c list.c

client.o: client.c
	gcc -c client.c

clean:
	rm -f *.o s-talk all
