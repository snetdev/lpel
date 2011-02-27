CFLAGS = -g -Wall -I../../
LDFLAGS = -lcap -lpcl -lpthread -lrt

.PHONY: all clean

TARGETS = pthr_ringtest ringtest pthr_pipetest pipetest

all: $(TARGETS)

pthr_ringtest: pthr_ringtest.o pthr_streams.o
	gcc -o $@ $^ $(LDFLAGS) 

pthr_pipetest: pthr_pipetest.o pthr_streams.o
	gcc -o $@ $^ $(LDFLAGS) 

ringtest: ringtest.o liblpel.a
	gcc -o $@ $^ $(LDFLAGS) 

pipetest: pipetest.o liblpel.a
	gcc -o $@ $^ $(LDFLAGS) 

%.o: %.c
	gcc -c $< $(CFLAGS)


clean:
	rm -fr $(TARGETS) *.o *mon_*.log
