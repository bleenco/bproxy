FROM arm32v7/ubuntu:bionic as base

RUN apt update && apt install -y build-essential python curl
RUN curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.33.11/install.sh | bash && export NVM_DIR="$HOME/.nvm" \
    && [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh" && nvm install node

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

RUN apt update && apt install git -y && export NVM_DIR="$HOME/.nvm" && [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh" && npm install --prod && npm run build

FROM arm32v7/ubuntu:bionic

WORKDIR /bproxy

COPY --from=build /bproxy/bproxy.json /bproxy/bproxy.json
COPY --from=build /bproxy/out/Release/bproxy /usr/bin/bproxy

EXPOSE 80 443

CMD [ "bproxy" ]
