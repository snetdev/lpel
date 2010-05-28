

all: test test2 test3 test4



#
# testing emptying a single coroutine queue by multiple workers
#
test: test.o
	gcc -o test test.o -lpcl -lpthread

test.o: test.c
	gcc -c test.c

#
# testing calling a coroutine subsequently by different workers
#
test2: test2.o
	gcc -o test2 test2.o -lpcl -lpthread

test2.o: test2.c
	gcc -c test2.c

#
# testing creating a coroutine within another coroutine (executed by a worker)
# and calling the newly created coroutine subsequently by different workers
#
test3: test3.o
	gcc -o test3 test3.o -lpcl -lpthread

test3.o: test3.c
	gcc -c test3.c

#
# testing multiple runqueues, one for each worker
#
test4: test4.o
	gcc -o test4 test4.o -lpcl -lpthread

test4.o: test4.c
	gcc -c test4.c

