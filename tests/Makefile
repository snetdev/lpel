
all:
	gcc -c check_set.c
	gcc -c ../set.c
	gcc -o check_set check_set.o set.o -lcheck -L$(HOME)/lib
	./check_set
