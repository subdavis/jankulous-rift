CC=g++
CFLAGS=-Wall -O3
LINKER=-lGL -lglut -lGLU -lGLEW
OUTFILE=PA3

all:
	$(CC) $(CFLAGS) main.cpp -o  $(OUTFILE) $(LINKER)

clean:
	rm $(OUTFILE)