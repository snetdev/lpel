
CCFLAGS=-Wall -D_GNU_SOURCE -DNOASSIGN_CORE
LDFLAGS=-lpthread

test: test.o stream.o
	gcc -o test test.o stream.o $(LDFLAGS)

stream.o: stream.c stream.h
	gcc -c stream.c $(CCFLAGS)

test.o: test.c stream.h
	gcc -c test.c $(CCFLAGS)

