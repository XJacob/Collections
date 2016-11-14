.PHONY: all
.PHONY: clean

LDLIBS = -lpthread

all: thg_server

thg_server: thg_server.o module.o digital_board.o
	$(CC) -o thg_server thg_server.o module.o digital_board.o $(LDLIBS)

%.o : %.c default_val.h
	$(CC) $(CFLAGS) $(DEBUG_FLAG) -c -o $@ $<

clean:
		@rm -rf *.o
		@rm thg_server
