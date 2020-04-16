all : jps.dll

LUA_INCLUDE = ./include
LUA_LIB = ./ -llua53

CFLAGS = $(CFLAG)
CFLAGS += -g -Wall --shared -I$(LUA_INCLUDE)
LDFLAGS = -L$(LUA_LIB)

jps.dll : jps.c fibheap.c
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean :
	rm jps.dll
