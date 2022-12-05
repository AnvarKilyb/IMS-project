CC=g++
DIR=src


all:
	$(CC) -c -o main.o $(DIR)/main.cc
	$(CC) -o main main.o -lsimlib -lm
	rm -f *.o
clean:
	rm -f main.o

run: 
	./main


