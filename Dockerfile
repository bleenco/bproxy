FROM ubuntu:latest as base

RUN apt-get update && apt-get install -y clang build-essential libuv1-dev

FROM base as build

WORKDIR /bproxy

COPY ./include /bproxy/include
COPY ./src /bproxy/src
COPY Makefile /bproxy/Makefile
COPY bproxy.json /bproxy/bproxy.json

RUN make static

FROM ubuntu:latest

ENV TINI_VERSION v0.17.0
ADD https://github.com/krallin/tini/releases/download/${TINI_VERSION}/tini /sbin/tini
RUN chmod +x /sbin/tini
RUN apt-get update && apt-get install libc-bin wget -y && rm -rf /var/lib/apt/lists/*

COPY --from=build /bproxy/bproxy.json /
COPY --from=build /bproxy/build/bproxy /usr/bin/

EXPOSE 80

ENTRYPOINT ["/sbin/tini", "--"]
CMD [ "bproxy" ]
