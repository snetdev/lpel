#
# create libraries
#

CFLAGS = -g -Wall -pthread -fPIC -I.
LDFLAGS = -shared -lpthread -pthread -lcap -lrt -lpcl

OBJS = buffer.o mailbox.o monitoring.o scheduler.o stream.o \
       streamset.o task.o taskqueue.o lpel_main.o worker.o \
       ctx_amd64.o


LIB_ST = liblpel.a
LIB_DYN = liblpel.so

.PHONY: all clean static dynamic

all: static dynamic

static: $(LIB_ST)

dynamic: $(LIB_DYN)

$(LIB_ST): $(OBJS)
	ar ru $@ $(OBJS)

$(LIB_DYN): $(OBJS)
	gcc $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	gcc -c $(CFLAGS) $(FPIC) $<

clean:
	rm -fr $(OBJS) $(LIB_ST) $(LIB_DYN)
