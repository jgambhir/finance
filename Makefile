CC = gcc
PORT=56180
CFLAGS = -DPORT=\$(PORT) -Wall -Werror -g

buxserver: buxserver.o lists.o lists.h wrapsock.o wrapsock.h
	$(CC) $(CFLAGS) -o buxserver buxserver.o lists.o wrapsock.o

buxfer: buxfer.o lists.o lists.h
	$(CC) $(CFLAGS) -o buxfer buxfer.o lists.o
	
buxserver.o: buxserver.c lists.h wrapsock.h
	$(CC) $(CFLAGS) -c buxserver.c

buxfer.o: buxfer.c lists.h
	$(CC) $(CFLAGS) -c buxfer.c

lists.o: lists.c lists.h
	$(CC) $(CFLAGS) -c lists.c
	
wrapsock.o: wrapsock.c wrapsock.h
	$(CC) $(CFLAGS) -c wrapsock.c

clean: 
	rm buxserver *.o
