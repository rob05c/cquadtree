CC=g++
CFLAGS=-c -Wall -O3 -std=c++11 -g

all: quadtree
gui: quadtree.o lquadtree.o gui.o
	$(CC) -pthread -g gui.o quadtree.o lquadtree.o -o quadtree -lncursesw
quadtree: quadtree.o lquadtree.o main.o
	$(CC) -pthread -g main.o quadtree.o lquadtree.o -o quadtree
gui.o:
	 $(CC) $(CFLAGS) gui.cpp -o gui.o
main.o:
	 $(CC) $(CFLAGS) main.cpp -o main.o
lquadtree.o:
	$(CC) $(CFLAGS) lock_quadtree.cpp -o lquadtree.o
quadtree.o:
	$(CC) $(CFLAGS) free_quadtree.cpp -o quadtree.o
clean:
	rm -rf *.o quadtree
