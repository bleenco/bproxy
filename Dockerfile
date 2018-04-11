FROM ubuntu:bionic as base

RUN apt-get update && apt-get install -y clang build-essential curl git \
    && curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.33.8/install.sh | bash \
    && export NVM_DIR="$HOME/.nvm" && [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh" \
    && nvm install node

FROM base as build

WORKDIR /bproxy

COPY ./include /bproxy/include
COPY ./src /bproxy/src
COPY Makefile /bproxy/Makefile
COPY target.mk /bproxy/target.mk
COPY package.json /bproxy/package.json
COPY bproxy.gyp /bproxy/bproxy.gyp
COPY options.gypi /bproxy/options.gypi
COPY common.gypi /bproxy/common.gypi
COPY binding.gyp /bproxy/binding.gyp
COPY bproxy.json /bproxy/bproxy.json
COPY 3rdparty /bproxy/3rdparty

RUN export NVM_DIR="$HOME/.nvm" && [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh" && npm install && npm run build

FROM ubuntu:bionic

RUN apt-get update && apt-get install libc-bin -y && rm -rf /var/lib/apt/lists/*

COPY --from=build /bproxy/bproxy.json /
COPY --from=build /bproxy/out/Release/bproxy /usr/bin/bproxy

EXPOSE 80

CMD [ "bproxy" ]
