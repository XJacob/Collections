.PHONY: all


CFLAGS = -I $(PWD)/header -Wall

LIB += -lpthread

demo: demo.o
	$(CC) $(CFLAGS) -o demo demo.o $(LIB)
demo.o: demo.c
	$(CC) $(CFLAGS) -c demo.c

clean:
	    @rm *.o
		@rm demo
