CC = gcc
CFLAGS = -Wall -std=c11 -pthread

compile: p1client p1server p2client p2server

p1client: build/p1client.o build/ttt.o
	$(CC) $(CFLAGS) build/p1client.o build/ttt.o -o p1client

p2client: build/p2client.o build/ttt.o
	$(CC) $(CFLAGS) build/p2client.o build/ttt.o -o p2client

p1server: build/p1server.o build/ttt.o
	$(CC) $(CFLAGS) build/p1server.o build/ttt.o -o p1server

p2server: build/p2server.o build/ttt.o
	$(CC) $(CFLAGS) build/p2server.o build/ttt.o -o p2server

build:
	mkdir -p build

build/ttt.o: include/ttt.h src/ttt.c build
	$(CC) $(CFLAGS) -c src/ttt.c -o build/ttt.o

build/p1client.o: include/ttt.h src/client-p1.c build
	$(CC) $(CFLAGS) -c src/client-p1.c -o build/p1client.o

build/p2client.o: include/ttt.h src/client-p2.c build
	$(CC) $(CFLAGS) -c src/client-p2.c -o build/p2client.o

build/p1server.o: include/ttt.h src/server-p1.c build
	$(CC) $(CFLAGS) -c src/server-p1.c -o build/p1server.o

build/p2server.o: include/ttt.h src/server-p2.c build
	$(CC) $(CFLAGS) -c src/server-p2.c -o build/p2server.o

clean:
	rm -rf build p1client p1server p2client p2server
