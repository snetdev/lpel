CFLAGS = -g -Wall -DNODEBUG
LDFLAGS = 

all: lpel
.PHONY: all

hashtab:
	gcc -c -DDEBUG -g check_hashtab.c
	gcc -c -DDEBUG -g ../hashtab.c
	gcc -o check_hashtab check_hashtab.o hashtab.o $(LDFLAGS)
	./check_hashtab


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
