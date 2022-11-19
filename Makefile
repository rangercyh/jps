.PHONY: all test clean

TOP=.

all: jps.so

LUA_BIN = /usr/local/bin/lua

CFLAGS = $(CFLAG)
CFLAGS += -g3 -O2 -rdynamic -Wall -fPIC -shared -lm

jps.so: jps.c heap.c intlist.c
	gcc $(CFLAGS) -o $@ $^

clean:
	rm jps.so

export LUA_CPATH=$(TOP)/?.so
export LUA_PATH=$(TOP)/test/?.lua

test:
	$(LUA_BIN) test/test.lua

