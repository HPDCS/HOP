CC=gcc 
CFLAGS=-I../include

main : main.o profiler.o
	$(CC) -o main main.o
	$(CC) -o profiler profiler.o

clean:
	rm -f main main.o profiler profiler.o
