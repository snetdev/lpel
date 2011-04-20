
all:
	make -f Makefile-lib clean
	make -f Makefile-lib
	make -f Makefile-so clean
	make -f Makefile-so
