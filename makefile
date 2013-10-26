CC=g++
CFLAGS=-c -Wall -O -std=c++11

all: quadtree
quadtree: quadtree.o main.o
	$(CC) main.o quadtree.o -o quadtree
main.o:
	 $(CC) $(CFLAGS) main.cpp -o main.o
quadtree.o:
	$(CC) $(CFLAGS) free_quadtree.cpp -o quadtree.o
clean:
	rm -rf *.o quadtree
