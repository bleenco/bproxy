FROM ubuntu:bionic as base

RUN apt-get update && apt-get install -y clang build-essential libuv1-dev zlib1g-dev

FROM base as build

WORKDIR /bproxy

COPY ./include /bproxy/include
COPY ./src /bproxy/src
COPY Makefile /bproxy/Makefile
COPY target.mk /bproxy/target.mk
COPY bproxy.json /bproxy/bproxy.json

RUN make static

FROM ubuntu:bionic

RUN apt-get update && apt-get install libc-bin -y && rm -rf /var/lib/apt/lists/*

COPY --from=build /bproxy/bproxy.json /
COPY --from=build /bproxy/build/bproxy* /usr/bin/bproxy

EXPOSE 80

CMD [ "bproxy" ]
