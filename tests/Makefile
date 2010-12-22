CFLAGS = -g -Wall -DNODEBUG
LDFLAGS = 

all: lpel
.PHONY: all


mpsctq:
	gcc -c -g check_mpsctq.c
	gcc -c -g ../mpsctq.c
	gcc -o check_mpsctq check_mpsctq.o mpsctq.o $(LDFLAGS) -lpthread
	./check_mpsctq

timing:
	gcc -c check_timing.c
	gcc -o check_timing check_timing.o $(LDFLAGS) -lrt
	./check_timing

lpel:
	rm -f check_*.o
	gcc -c check_lpel.c $(CFLAGS)
	gcc -c ../*.c $(CFLAGS)
	gcc -o check_lpel *.o $(LDFLAGS) -lcap -lpcl -lpthread -lrt
	./check_lpel

lpel2:
	rm -f check_*.o
	gcc -c check_lpel2.c $(CFLAGS)
	gcc -c ../*.c $(CFLAGS)
	gcc -o check_lpel2 *.o $(LDFLAGS) -lcap -lpcl -lpthread -lrt
	./check_lpel2
