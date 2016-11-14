.PHONY: all
.PHONY: clean

LDLIBS = -lpthread

all: reader_cln reader_srv

reader_cln: reader_cln.o reader_cln_llc.o
	gcc -o reader_cln reader_cln.o reader_cln_llc.o $(LDLIBS)
reader_cln.o: reader_cln.c
	gcc -c reader_cln.c
reader_cln_llc.o: reader_cln_llc.c
	gcc -c reader_cln_llc.c

reader_srv: reader_srv.o favite_llc.o module.o digital_board.o
	$(CC) -o reader_srv reader_srv.o favite_llc.o module.o digital_board.o $(LDLIBS)

%.o : %.c
	$(CC) $(CFLAGS) $(DEBUG_FLAG) -c -o $@ $<

clean:
		@rm -rf *.o
		@rm reader_cln reader_srv
