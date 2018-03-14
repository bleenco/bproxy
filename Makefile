TARGET = build/bproxy
CC = clang
CFLAGS = -Wall -Iinclude -O3
DEPS = build/obj/jsmn.o build/obj/http_parser.o build/obj/http.o build/obj/config.o
LIBS = -luv -lpthread -ldl

.PHONY: all bproxy checkdir clean

all: checkdir bproxy
recompile: clean checkdir bproxy

bproxy: jsmn.o http_parser.o http.o config.o
	$(CC) $(CFLAGS) $(DEPS) -o $(TARGET) src/bproxy.c $(LIBS)

http_parser.o: checkdir
	$(CC) $(CFLAGS) -c src/http_parser.c -o build/obj/http_parser.o

http.o: checkdir
	$(CC) $(CFLAGS) -c src/http.c -o build/obj/http.o

config.o: checkdir
	$(CC) $(CFLAGS) -c src/config.c -o build/obj/config.o

jsmn.o: checkdir
	$(CC) $(CFLAGS) -c src/jsmn.c -o build/obj/jsmn.o

checkdir:
	@mkdir -p build/obj

clean:
	@rm -rf build/
