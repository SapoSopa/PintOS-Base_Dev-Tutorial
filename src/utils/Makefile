all: setitimer-helper squish-unix #squish-pty

CC = gcc
CFLAGS = -Wall -W
LOADLIBES = -lm
setitimer-helper: setitimer-helper.o
squish-unix: squish-unix.o
# TODO: Add the squish-pty

clean: 
	rm -f *.o setitimer-helper squish-unix
