
LDFLAGS = -lcheck -L$(HOME)/lib

all: set cpuassign
.PHONY: all

set:
	gcc -c check_set.c
	gcc -c ../set.c
	gcc -o check_set check_set.o set.o $(LDFLAGS)
	./check_set

cpuassign:
	gcc -c check_cpuassign.c
	gcc -c ../cpuassign.c
	gcc -o check_cpuassign check_cpuassign.o cpuassign.o $(LDFLAGS) -lcap
	./check_cpuassign

lpel:
	rm check_*.o
	gcc -c check_lpel.c
	gcc -c ../*.c -march=i686
	gcc -o check_lpel *.o $(LDFLAGS) -lcap -lpcl -lpthread -lrt
	./check_lpel
