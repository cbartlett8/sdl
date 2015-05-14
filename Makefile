#comment #1
CC=g++
#comment #2
CFLAGS=-c -Wall -pedantic

all: main

main: main.o
	$(CC) -o main main.o -lSDL
	
main.o: main.cpp
	$(CC) $(CFLAGS) main.cpp -lSDL
	
clean:
	rm main
	

