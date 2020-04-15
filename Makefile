all : jps.dll

LUA_INCLUDE = ./include
LUA_LIB = -L./ -llua53

jps.dll : jps.c
	gcc -g -Wall --shared -o $@ $^ -I$(LUA_INCLUDE) $(LUA_LIB)

clean :
	rm jps.dll
