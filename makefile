CC=g++
CFLAGS=-c -Wall -O3 -std=c++11 -g

all: quadtree
quadtree: quadtree.o main.o
	$(CC) -pthread -g main.o quadtree.o -o quadtree
main.o:
	 $(CC) $(CFLAGS) main.cpp -o main.o
quadtree.o:
	$(CC) $(CFLAGS) free_quadtree.cpp -o quadtree.o
clean:
	rm -rf *.o quadtree
