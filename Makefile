CC=g++
SHELL=/bin/sh
RANLIB=ranlib

INCLUDES=-I. -I./include 
LIB_PATH=-L./lib  -L/usr/local/lib/
LIB=lib/libredisClient.a

OBJ:=$(patsubst %.cpp,%.o, $(wildcard ./src/*.cpp))
LIB_OBJ=./src/ae_head.o ./src/redis_rw_split_client_imp.o

CPPFLAGS=-O0 -g -static -fPIC -finline-functions   -pipe \
        -D_XOPEN_SOURE=500 -D_GNU_SOURCE -fpermissive -D_NDEBUG

LDFLAGS= -lm -lpthread -lgflags -lhiredis -levent

all: asyn  $(LIB) OUTPUT

$(LIB): $(LIB_OBJ)
	ar cr $@ $(LIB_OBJ)
	$(RANLIB) $@

%.o: %.cpp
	$(CC) $(CPPFLAGS) -c $< -o $@ $(INCLUDES) $(LIB_PATH) $( LDFLAGS)

asyn: ${OBJ}  ./test/test_rw_split.o
	$(CC) -o $@ $^ $(INCLUDES) $(LIB_PATH) $(LDFLAGS)

OUTPUT:
	[ -e output ] || mkdir output
	[ -e output/include ] || mkdir -p ./output/include/
	cp -r include/* ./output/include/
	[ -e output/lib ] || mkdir -p ./output/lib/
	cp $(LIB) ./output/lib/
clean:
	rm -rf src/*.o test/*.o asyn $(LIB) output
