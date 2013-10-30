CC=g++
CFLAGS=-c -Wall -O3 -std=c++11 -g

all: quadtree
gui: quadtree.o gui.o
	$(CC) -pthread -g gui.o quadtree.o -o quadtree -lncurses
quadtree: quadtree.o main.o
	$(CC) -pthread -g main.o quadtree.o -o quadtree
gui.o:
	 $(CC) $(CFLAGS) gui.cpp -o gui.o
main.o:
	 $(CC) $(CFLAGS) main.cpp -o main.o
quadtree.o:
	$(CC) $(CFLAGS) free_quadtree.cpp -o quadtree.o
clean:
	rm -rf *.o quadtree
