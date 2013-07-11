CC=g++
SHELL=/bin/sh

INCLUDES=-I. -I./include 
LIB_PATH=-L./lib  -L/usr/local/lib/

OBJ:=$(patsubst %.cpp,%.o, $(wildcard ./src/*.cpp))

CPPFLAGS=-O0 -g -static -fPIC -finline-functions   -pipe \
        -D_XOPEN_SOURE=500 -D_GNU_SOURCE -fpermissive

LDFLAGS= -lm -lpthread -lgflags -lhiredis -levent

all: asyn example

%.o: %.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@ $(INCLUDES) $(LIB_PATH) $( LDFLAGS)

asyn: ${OBJ}  ./test/test_rw_split.o
	$(CC) -o $@ $^ $(INCLUDES) $(LIB_PATH) $(LDFLAGS)

clean:
	rm -f src/*.o test/*.o asyn example
