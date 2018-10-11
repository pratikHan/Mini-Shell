CC = cc
CFLAGS = -std=c99 -pedantic -Wall
OBJECTS = shell.o

all: shell

hw2.o: shell.c
	$(CC) $(CFLAGS) -c shell.c

shell: $(OBJECTS)
	$(CC) $(OBJECTS) -o shell

clean:
	rm -f *.o shell
