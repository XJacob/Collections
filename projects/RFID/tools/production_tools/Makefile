.PHONY: all

ver=TYPE_A2_3

CFLAGS = -I $(PWD)/header -Wall
ifeq ($(ver), TYPE_A2_3)
CFLAGS += -DTYPE_A2_3
endif

LIB += -lpthread
LIB += $(PWD)/../libs/ThingMagic/libmercuryapi.a
LIB += $(PWD)/../libs/ThingMagic/libltkc.a
LIB += $(PWD)/../libs/ThingMagic/libltkctm.a

pro_tools: pro_tools.o target.o
	$(CC) $(CFLAGS) -o pro_tools pro_tools.o target.o $(LIB)
pro_tools.o: pro_tools.c
	$(CC) $(CFLAGS) -c pro_tools.c
ifeq ($(ver), TYPE_A2_3)
target.o : target_A23.c target_A23.h
	$(CC) $(CFLAGS) -o target.o -c target_A23.c
else
target.o : target_empty.c
	$(CC) $(CFLAGS) -o target.o -c target_empty.c
endif

clean:
	    @rm *.o
		@rm pro_tools
