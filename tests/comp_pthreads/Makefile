CFLAGS = -g -Wall -pthread -I../../
LDFLAGS = -lcap -lpthread -lrt

.PHONY: all clean ring pipe

TARGETS = pthr_ringtest ringtest pthr_pipetest pipetest

all: $(TARGETS)

ring: ringtest pthr_ringtest

pipe: pipetest pthr_pipetest

pthr_ringtest: pthr_ringtest.o pthr_streams.o
	gcc -o $@ $^ $(LDFLAGS) 

pthr_pipetest: pthr_pipetest.o pthr_streams.o
	gcc -o $@ $^ $(LDFLAGS) 

ringtest: ringtest.o liblpel.a
	gcc -o $@ $^ $(LDFLAGS) 

pipetest: pipetest.o liblpel.a
	gcc -o $@ $^ $(LDFLAGS) 

%.o: %.c
	gcc -c $< $(CFLAGS) $(DEFINES)


clean:
	rm -fr $(TARGETS) *.o *mon_*.log
