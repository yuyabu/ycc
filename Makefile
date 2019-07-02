CFLAGS=-std=c11 -g -static

9cc: ycc.c

test: ycc
	./test.sh

clean:
	rm -f 9cc *.o *~ tmp*

.PHONY: test clean
