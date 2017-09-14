all: fsmem.o

CFLAGS = -O3 -Wall
#CFLAGS = -g -Wall


# Build


fsmem.o: fsmem.c fsmem.h Makefile
	gcc ${CFLAGS} -c -o fsmem.o fsmem.c


# Cleanup and maintenance


clean:
	rm -f *.o

strip: clean
	rm -rf *~
