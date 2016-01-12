CFLAGS=-Wall -g -I../include -std=c++11
LDFLAGS=-L${DSN_ROOT}/lib -lrrdb.clientlib -lpthread  -lrt

%.o: %.cpp
	g++  -c -o $@ $< $(CFLAGS)

sample: sample.o
	g++ sample.o -o rrdb_sample $(LDFLAGS)
	cp ${DSN_ROOT}/lib/libdsn.core.so .

all: sample

clean:
	-rm *.o *.so rrdb_sample
