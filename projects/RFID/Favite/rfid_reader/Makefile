.PHONY: all
all : reader

LDLIBS = -lpthread

reader: reader.o antenna.o

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	    @rm *.o
		@rm reader
