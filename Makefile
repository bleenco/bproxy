dist:
	npm run build

all: dist

install:
	cp out/Release/bproxy /usr/local/bin/bproxy

clean:
	@rm -rf out/

distclean:
	@rm -rf out/ gypkg_deps/

docker_image:
	docker build -t bproxy .

.PHONY: all clean dist distclean
