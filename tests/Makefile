CFLAGS = -g -Wall -DNODEBUG
LDFLAGS = 

all: lpel
.PHONY: all

streamtab:
	gcc -c -g check_streamtab.c
	gcc -c -g ../streamtab.c
	gcc -o check_streamtab check_streamtab.o streamtab.o $(LDFLAGS)
	./check_streamtab

linklst:
	gcc -c $(CFLAGS) check_linklst.c
	gcc -c $(CFLAGS) ../linklst.c
	gcc -o check_linklst check_linklst.o linklst.o $(LDFLAGS)
	./check_linklst

flagtree:
	gcc -c -g check_flagtree.c
	gcc -c -g ../flagtree.c
	gcc -o check_flagtree check_flagtree.o flagtree.o $(LDFLAGS)
	./check_flagtree

rwlock:
	gcc -c check_rwlock.c
	gcc -o check_rwlock check_rwlock.o $(LDFLAGS) -lpthread
	./check_rwlock > /dev/null

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
