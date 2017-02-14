include ../../Rules.make

CFLAGS += -I$(DESTDIR)/usr/include -I$(DESTDIR)/include
LIBS += -lm -lpthread

OBJS = dipi_test.o

%.o: %.c
		$(CC) $(CFLAGS) -c -o $@ $<

all: $(OBJS)
		$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o dipi_test

clean:
		@rm -f *.o dipi_test

