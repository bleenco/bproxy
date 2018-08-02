dist:
	npm run build

all: dist

install:
	cp out/Release/bproxy /usr/local/bin/bproxy

clean:
	@rm -rf out/ gypkg_deps/ build/

distclean:
	@rm -rf out/ gypkg_deps/

format:
	@echo "==> Formatting code..."
	@find src/ include/ -type f ! -name 'cJSON.*' -iname *.h -o -iname *.c ! -iname *cJSON.* | xargs clang-format -i

docker_image:
	docker build -t bleenco/bproxy .

.PHONY: all clean dist distclean format
