FROM mhart/alpine-node:10 as base

RUN apk add --no-cache alpine-sdk python

FROM base as build

WORKDIR /bproxy

COPY ./include /bproxy/include
COPY ./src /bproxy/src
COPY Makefile /bproxy/Makefile
COPY package.json /bproxy/package.json
COPY bproxy.gyp /bproxy/bproxy.gyp
COPY options.gypi /bproxy/options.gypi
COPY common.gypi /bproxy/common.gypi
COPY binding.gyp /bproxy/binding.gyp
COPY bproxy.json /bproxy/bproxy.json
COPY 3rdparty /bproxy/3rdparty

RUN npm install --prod && npm run build

FROM alpine:3.7

WORKDIR /bproxy

COPY --from=build /bproxy/bproxy.json /bproxy/bproxy.json
COPY --from=build /bproxy/out/Release/bproxy /usr/bin/bproxy

EXPOSE 80 443

CMD [ "bproxy" ]
