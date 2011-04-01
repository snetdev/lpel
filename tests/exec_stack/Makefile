
CFLAGS = -g -Wall # -fomit-frame-pointer
all: stack


stack: stack.o ctx.o
	gcc -o stack stack.o ctx.o

stack.o: stack.c
	gcc -c stack.c -I../../ $(CFLAGS)

ctx.o:  ../../ctx_amd64.S
	gcc -o $@ -c $< $(CFLAGS)

clean:
	rm -fr *.o stack
