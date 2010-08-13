CFLAGS = -g -Wall -DNODEBUG
LDFLAGS = 

all: set cpuassign
.PHONY: all

set:
	gcc -c check_set.c
	gcc -c ../set.c
	gcc -o check_set check_set.o set.o $(LDFLAGS)
	./check_set

streamset:
	gcc -c -g check_streamset.c
	gcc -c -g ../streamset.c
	gcc -o check_streamset check_streamset.o streamset.o $(LDFLAGS)
	./check_streamset

streamtable:
	gcc -c -g check_streamtable.c
	gcc -c -g ../streamtable.c
	gcc -o check_streamtable check_streamtable.o streamtable.o $(LDFLAGS)
	./check_streamtable

flagtree:
	gcc -c -g check_flagtree.c
	gcc -c -g ../flagtree.c
	gcc -o check_flagtree check_flagtree.o flagtree.o $(LDFLAGS)
	./check_flagtree

timing:
	gcc -c check_timing.c
	gcc -o check_timing check_timing.o $(LDFLAGS) -lrt
	./check_timing

cpuassign:
	gcc -c check_cpuassign.c
	gcc -c ../cpuassign.c
	gcc -o check_cpuassign check_cpuassign.o cpuassign.o $(LDFLAGS) -lcap
	./check_cpuassign

lpel:
	rm -f check_*.o
	gcc -c check_lpel2.c $(CFLAGS)
	gcc -c ../*.c $(CFLAGS)
	gcc -o check_lpel2 *.o $(LDFLAGS) -lcap -lpcl -lpthread -lrt
	./check_lpel2
