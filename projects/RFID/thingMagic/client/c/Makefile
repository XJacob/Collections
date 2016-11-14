.PHONY: all

CFLAGS = -I $(PWD)/header -Wall

LIB += -lpthread
LIB += $(PWD)/lib/libmercuryapi.a
LIB += $(PWD)/lib/libltkc.a
LIB += $(PWD)/lib/libltkctm.a

client_reader: client_reader.o client_menu.o client_config.o thingMagic_fn.o demo.o
	gcc $(CFLAGS) -o client_reader client_reader.o client_menu.o client_config.o thingMagic_fn.o demo.o $(LIB)
client_reader.o: client_reader.c
	gcc $(CFLAGS) -c client_reader.c
client_menu.o: client_menu.c
	gcc $(CFLAGS) -c client_menu.c
client_config.o: client_config.c
	gcc $(CFLAGS) -c client_config.c
thingMagic_fn.o: thingMagic_fn.c
	gcc $(CFLAGS) -c thingMagic_fn.c
demo.o: demo.c
	gcc $(CFLAGS) -c demo.c

clean:
	    @rm *.o
		@rm client_reader
