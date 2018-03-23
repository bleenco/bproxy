TARGET = build/bproxy
CC = clang
CFLAGS = -Wall -Iinclude -O3
DEPS = build/obj/jsmn.o build/obj/http_parser.o build/obj/http.o build/obj/config.o build/obj/gzip.o
LIBS = -luv -lpthread -ldl -lz

.PHONY: all bproxy checkdir clean

all: checkdir bproxy
static: checkdir bproxy_static
recompile: clean checkdir bproxy

install:
	cp build/bproxy /usr/local/bin/bproxy

bproxy: jsmn.o http_parser.o http.o config.o gzip.o
	$(CC) $(CFLAGS) $(DEPS) -o $(TARGET) src/bproxy.c $(LIBS)

bproxy_static: jsmn.o http_parser.o http.o config.o gzip.o
	$(CC) -Wall -Iinclude -O3 -static -pthread -ldl $(DEPS) -o $(TARGET) src/bproxy.c /usr/lib/x86_64-linux-gnu/libuv.a

http_parser.o: checkdir
	$(CC) $(CFLAGS) -c src/http_parser.c -o build/obj/http_parser.o

http.o: checkdir
	$(CC) $(CFLAGS) -c src/http.c -o build/obj/http.o

config.o: checkdir
	$(CC) $(CFLAGS) -c src/config.c -o build/obj/config.o

jsmn.o: checkdir
	$(CC) $(CFLAGS) -c src/jsmn.c -o build/obj/jsmn.o

gzip.o: checkdir
	$(CC) $(CFLAGS) -c src/gzip.c -o build/obj/gzip.o

checkdir:
	@mkdir -p build/obj

clean:
	@rm -rf build/
