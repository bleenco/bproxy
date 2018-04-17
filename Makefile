include target.mk
TARGET = build/bproxy_$(PLATFORM)_$(ARCH)
CC = clang
CFLAGS = -Wall -Iinclude -O3
DEPS = build/obj/jsmn.o build/obj/http_parser.o build/obj/http.o build/obj/config.o build/obj/gzip.o build/obj/log.o
LIBS = -luv -lpthread -ldl -lz

ifeq ($(PLATFORM),linux)
	ifeq ($(ARCH),amd64)
		STATIC_LIBS = /usr/lib/x86_64-linux-gnu/libuv.a /usr/lib/x86_64-linux-gnu/libz.a
	endif
	ifeq ($(ARCH),arm)
		STATIC_LIBS = /usr/lib/arm-linux-gnueabihf/libuv.a /usr/lib/arm-linux-gnueabihf/libz.a
	endif
endif

dist:
	npm run build

all: checkdir bproxy
static: checkdir bproxy_static
recompile: clean checkdir bproxy

install:
	cp build/$(TARGET) /usr/local/bin/bproxy

bproxy: jsmn.o http_parser.o http.o config.o gzip.o log.o
	$(CC) $(CFLAGS) $(DEPS) -o $(TARGET) src/bproxy.c $(LIBS)

bproxy_static: jsmn.o http_parser.o http.o config.o gzip.o log.o
	$(CC) -Wall -Iinclude -O3 -static -pthread -ldl $(DEPS) -o $(TARGET) src/bproxy.c $(STATIC_LIBS)

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

log.o: checkdir
	$(CC) $(CFLAGS) -c src/log.c -o build/obj/log.o

checkdir:
	@mkdir -p build/obj

clean:
	@rm -rf build/

docker_image:
	docker build -t bproxy .

.PHONY: all bproxy checkdir clean dist
