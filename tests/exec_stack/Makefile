

all: stack


stack: stack.o ctx.o
	gcc -o stack stack.o ctx.o

stack.o: stack.c
	gcc -c stack.c -I../../ -g -Wall

ctx.o:  ../../ctx_amd64-stk.S
	gcc -o $@ -c $< -g

clean:
	rm -fr *.o stack
