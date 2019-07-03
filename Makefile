CFLAGS=-std=c11 -g -static

9cc: ycc.c

test: ycc
	./test.sh

clean:
	rm -f ycc *.o *~ tmp*

.PHONY: test clean
