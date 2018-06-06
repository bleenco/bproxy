<a name="0.7.0"></a>
# 0.7.0 (2018-06-06)


### Bug Fixes

* **closing connections:** connections are closed after use. ([17d6a78](https://github.com/bleenco/bproxy/commit/17d6a78))
* **closing-connection:** fixed invalid free's on closing connections ([c2860e1](https://github.com/bleenco/bproxy/commit/c2860e1))
* **conns:** closing connections ([de49a2e](https://github.com/bleenco/bproxy/commit/de49a2e))
* **dockerfile:** update ([48c2605](https://github.com/bleenco/bproxy/commit/48c2605))
* **free:** remove free function for dead connections ([8f5651c](https://github.com/bleenco/bproxy/commit/8f5651c))
* **gzipped-chunks:** closes [#12](https://github.com/bleenco/bproxy/issues/12) ([f423828](https://github.com/bleenco/bproxy/commit/f423828))
* **http-headers:** remove redundant CRLF ([ce87a4f](https://github.com/bleenco/bproxy/commit/ce87a4f))
* **http-headers:** replace  with ([cd58020](https://github.com/bleenco/bproxy/commit/cd58020))
* **http-parsing:** parse request in multiple chunks closes [#32](https://github.com/bleenco/bproxy/issues/32) ([b7a6101](https://github.com/bleenco/bproxy/commit/b7a6101))
* **keep-alive:** set tcp keep-alive for websockets (closes [#37](https://github.com/bleenco/bproxy/issues/37)) ([09dd68c](https://github.com/bleenco/bproxy/commit/09dd68c))
* **logging:** loggging of websockets and buffer overflow ([5c8b16b](https://github.com/bleenco/bproxy/commit/5c8b16b))
* **memory leak:** fixed some memory leaks and uninitialized variables. ([d709cb7](https://github.com/bleenco/bproxy/commit/d709cb7))
* **not-found:** do not free 404 response twice ([6c6d3d1](https://github.com/bleenco/bproxy/commit/6c6d3d1))
* **premature-close:** fix premature closing connections ([0c5c9a8](https://github.com/bleenco/bproxy/commit/0c5c9a8))
* **SIGSEGV:** fixed sigsegv in http_read_cb_override ([911035e](https://github.com/bleenco/bproxy/commit/911035e))
* **ssl:** load fullchain in openssl ([8756a51](https://github.com/bleenco/bproxy/commit/8756a51))
* **usage:** update usage text ([756066f](https://github.com/bleenco/bproxy/commit/756066f))
* **uv_ssl_t:** fixed memory issues concerning ssl ([e369673](https://github.com/bleenco/bproxy/commit/e369673))
* **writing error:** fixed writing error by copying data to buffer ([c80f004](https://github.com/bleenco/bproxy/commit/c80f004))


### Features

* **additional headers:** Adding additional http headers to request and response closes [#23](https://github.com/bleenco/bproxy/issues/23) ([f4e2c70](https://github.com/bleenco/bproxy/commit/f4e2c70))
* **gzip:** add gzip configurable mime types inside JSON config ([dc0d2a5](https://github.com/bleenco/bproxy/commit/dc0d2a5))
* **gzip:** added gzip to compress server responses. ([90125a0](https://github.com/bleenco/bproxy/commit/90125a0))
* **headers:** add bproxy version to header ([517126a](https://github.com/bleenco/bproxy/commit/517126a))
* **json:** replace jsmn JSON parser to cJSON parser ([c2daaef](https://github.com/bleenco/bproxy/commit/c2daaef))
* **logger:** add config option to specify log file path ([afbd4a8](https://github.com/bleenco/bproxy/commit/afbd4a8))
* **logger:** implement logger ([3f6beca](https://github.com/bleenco/bproxy/commit/3f6beca))
* **logging:** added logging of requests with response time and status codes ([6dbefc0](https://github.com/bleenco/bproxy/commit/6dbefc0))
* **ssl:** added basic ssl implementation ([84adebe](https://github.com/bleenco/bproxy/commit/84adebe))
* **ssl:** individual certificate for each host ([a7d9c90](https://github.com/bleenco/bproxy/commit/a7d9c90))
* **ssl:** listen on two ports to allow simultaneous http and https. ([7eb8de6](https://github.com/bleenco/bproxy/commit/7eb8de6))
* **uv_link_t:** used uv_link_t with connections closing ([67bbcf8](https://github.com/bleenco/bproxy/commit/67bbcf8))
* **uv_link_t:** using uv_link_t for communication between client and proxy. ([6602e00](https://github.com/bleenco/bproxy/commit/6602e00))
* **wildcard:** implemented hostname wildcard ([82fc997](https://github.com/bleenco/bproxy/commit/82fc997))
* **ws:** handle websockets ([462c9f3](https://github.com/bleenco/bproxy/commit/462c9f3))



<a name="0.6.0"></a>
# 0.6.0 (2018-06-02)


### Bug Fixes

* **closing connections:** connections are closed after use. ([17d6a78](https://github.com/bleenco/bproxy/commit/17d6a78))
* **closing-connection:** fixed invalid free's on closing connections ([c2860e1](https://github.com/bleenco/bproxy/commit/c2860e1))
* **conns:** closing connections ([de49a2e](https://github.com/bleenco/bproxy/commit/de49a2e))
* **dockerfile:** update ([48c2605](https://github.com/bleenco/bproxy/commit/48c2605))
* **free:** remove free function for dead connections ([8f5651c](https://github.com/bleenco/bproxy/commit/8f5651c))
* **gzipped-chunks:** closes [#12](https://github.com/bleenco/bproxy/issues/12) ([f423828](https://github.com/bleenco/bproxy/commit/f423828))
* **http-headers:** remove redundant CRLF ([ce87a4f](https://github.com/bleenco/bproxy/commit/ce87a4f))
* **http-headers:** replace  with ([cd58020](https://github.com/bleenco/bproxy/commit/cd58020))
* **http-parsing:** parse request in multiple chunks closes [#32](https://github.com/bleenco/bproxy/issues/32) ([b7a6101](https://github.com/bleenco/bproxy/commit/b7a6101))
* **logging:** loggging of websockets and buffer overflow ([5c8b16b](https://github.com/bleenco/bproxy/commit/5c8b16b))
* **memory leak:** fixed some memory leaks and uninitialized variables. ([d709cb7](https://github.com/bleenco/bproxy/commit/d709cb7))
* **not-found:** do not free 404 response twice ([6c6d3d1](https://github.com/bleenco/bproxy/commit/6c6d3d1))
* **premature-close:** fix premature closing connections ([0c5c9a8](https://github.com/bleenco/bproxy/commit/0c5c9a8))
* **SIGSEGV:** fixed sigsegv in http_read_cb_override ([911035e](https://github.com/bleenco/bproxy/commit/911035e))
* **ssl:** load fullchain in openssl ([8756a51](https://github.com/bleenco/bproxy/commit/8756a51))
* **usage:** update usage text ([756066f](https://github.com/bleenco/bproxy/commit/756066f))
* **uv_ssl_t:** fixed memory issues concerning ssl ([e369673](https://github.com/bleenco/bproxy/commit/e369673))
* **writing error:** fixed writing error by copying data to buffer ([c80f004](https://github.com/bleenco/bproxy/commit/c80f004))


### Features

* **additional headers:** Adding additional http headers to request and response closes [#23](https://github.com/bleenco/bproxy/issues/23) ([f4e2c70](https://github.com/bleenco/bproxy/commit/f4e2c70))
* **gzip:** add gzip configurable mime types inside JSON config ([dc0d2a5](https://github.com/bleenco/bproxy/commit/dc0d2a5))
* **gzip:** added gzip to compress server responses. ([90125a0](https://github.com/bleenco/bproxy/commit/90125a0))
* **headers:** add bproxy version to header ([517126a](https://github.com/bleenco/bproxy/commit/517126a))
* **json:** replace jsmn JSON parser to cJSON parser ([c2daaef](https://github.com/bleenco/bproxy/commit/c2daaef))
* **logger:** add config option to specify log file path ([afbd4a8](https://github.com/bleenco/bproxy/commit/afbd4a8))
* **logger:** implement logger ([3f6beca](https://github.com/bleenco/bproxy/commit/3f6beca))
* **logging:** added logging of requests with response time and status codes ([6dbefc0](https://github.com/bleenco/bproxy/commit/6dbefc0))
* **ssl:** added basic ssl implementation ([84adebe](https://github.com/bleenco/bproxy/commit/84adebe))
* **ssl:** individual certificate for each host ([a7d9c90](https://github.com/bleenco/bproxy/commit/a7d9c90))
* **ssl:** listen on two ports to allow simultaneous http and https. ([7eb8de6](https://github.com/bleenco/bproxy/commit/7eb8de6))
* **uv_link_t:** used uv_link_t with connections closing ([67bbcf8](https://github.com/bleenco/bproxy/commit/67bbcf8))
* **uv_link_t:** using uv_link_t for communication between client and proxy. ([6602e00](https://github.com/bleenco/bproxy/commit/6602e00))
* **wildcard:** implemented hostname wildcard ([82fc997](https://github.com/bleenco/bproxy/commit/82fc997))
* **ws:** handle websockets ([462c9f3](https://github.com/bleenco/bproxy/commit/462c9f3))



<a name="0.5.0"></a>
# 0.5.0 (2018-05-09)


### Bug Fixes

* **closing connections:** connections are closed after use. ([17d6a78](https://github.com/bleenco/bproxy/commit/17d6a78))
* **conns:** closing connections ([de49a2e](https://github.com/bleenco/bproxy/commit/de49a2e))
* **dockerfile:** update ([48c2605](https://github.com/bleenco/bproxy/commit/48c2605))
* **gzipped-chunks:** closes [#12](https://github.com/bleenco/bproxy/issues/12) ([f423828](https://github.com/bleenco/bproxy/commit/f423828))
* **logging:** loggging of websockets and buffer overflow ([5c8b16b](https://github.com/bleenco/bproxy/commit/5c8b16b))
* **memory leak:** fixed some memory leaks and uninitialized variables. ([d709cb7](https://github.com/bleenco/bproxy/commit/d709cb7))
* **not-found:** do not free 404 response twice ([6c6d3d1](https://github.com/bleenco/bproxy/commit/6c6d3d1))
* **usage:** update usage text ([756066f](https://github.com/bleenco/bproxy/commit/756066f))
* **writing error:** fixed writing error by copying data to buffer ([c80f004](https://github.com/bleenco/bproxy/commit/c80f004))


### Features

* **gzip:** add gzip configurable mime types inside JSON config ([dc0d2a5](https://github.com/bleenco/bproxy/commit/dc0d2a5))
* **gzip:** added gzip to compress server responses. ([90125a0](https://github.com/bleenco/bproxy/commit/90125a0))
* **json:** replace jsmn JSON parser to cJSON parser ([c2daaef](https://github.com/bleenco/bproxy/commit/c2daaef))
* **logger:** add config option to specify log file path ([afbd4a8](https://github.com/bleenco/bproxy/commit/afbd4a8))
* **logger:** implement logger ([3f6beca](https://github.com/bleenco/bproxy/commit/3f6beca))
* **logging:** added logging of requests with response time and status codes ([6dbefc0](https://github.com/bleenco/bproxy/commit/6dbefc0))
* **ssl:** added basic ssl implementation ([84adebe](https://github.com/bleenco/bproxy/commit/84adebe))
* **ssl:** individual certificate for each host ([a7d9c90](https://github.com/bleenco/bproxy/commit/a7d9c90))
* **ssl:** listen on two ports to allow simultaneous http and https. ([7eb8de6](https://github.com/bleenco/bproxy/commit/7eb8de6))
* **uv_link_t:** used uv_link_t with connections closing ([67bbcf8](https://github.com/bleenco/bproxy/commit/67bbcf8))
* **uv_link_t:** using uv_link_t for communication between client and proxy. ([6602e00](https://github.com/bleenco/bproxy/commit/6602e00))
* **wildcard:** implemented hostname wildcard ([82fc997](https://github.com/bleenco/bproxy/commit/82fc997))
* **ws:** handle websockets ([462c9f3](https://github.com/bleenco/bproxy/commit/462c9f3))



<a name="0.4.0"></a>
# 0.4.0 (2018-04-29)


### Bug Fixes

* **writing error:** fixed writing error by copying data to buffer ([c80f004](https://github.com/bleenco/bproxy/commit/c80f004))


### Features

* **uv_link_t:** used uv_link_t with connections closing ([67bbcf8](https://github.com/bleenco/bproxy/commit/67bbcf8))
* **uv_link_t:** using uv_link_t for communication between client and proxy. ([6602e00](https://github.com/bleenco/bproxy/commit/6602e00))



<a name="0.3.0"></a>
# 0.3.0 (2018-04-11)


### Bug Fixes

* **closing connections:** connections are closed after use. ([17d6a78](https://github.com/bleenco/bproxy/commit/17d6a78))
* **conns:** closing connections ([de49a2e](https://github.com/bleenco/bproxy/commit/de49a2e))
* **dockerfile:** update ([48c2605](https://github.com/bleenco/bproxy/commit/48c2605))
* **gzipped-chunks:** closes [#12](https://github.com/bleenco/bproxy/issues/12) ([f423828](https://github.com/bleenco/bproxy/commit/f423828))
* **memory leak:** fixed some memory leaks and uninitialized variables. ([d709cb7](https://github.com/bleenco/bproxy/commit/d709cb7))
* **usage:** update usage text ([756066f](https://github.com/bleenco/bproxy/commit/756066f))


### Features

* **gzip:** add gzip configurable mime types inside JSON config ([dc0d2a5](https://github.com/bleenco/bproxy/commit/dc0d2a5))
* **gzip:** added gzip to compress server responses. ([90125a0](https://github.com/bleenco/bproxy/commit/90125a0))
* **logger:** implement logger ([3f6beca](https://github.com/bleenco/bproxy/commit/3f6beca))
* **wildcard:** implemented hostname wildcard ([82fc997](https://github.com/bleenco/bproxy/commit/82fc997))
* **ws:** handle websockets ([462c9f3](https://github.com/bleenco/bproxy/commit/462c9f3))



<a name="0.2.0"></a>
# 0.2.0 (2018-03-28)


### Bug Fixes

* **dockerfile:** update ([48c2605](https://github.com/bleenco/bproxy/commit/48c2605))
* **usage:** update usage text ([756066f](https://github.com/bleenco/bproxy/commit/756066f))


### Features

* **gzip:** added gzip to compress server responses. ([90125a0](https://github.com/bleenco/bproxy/commit/90125a0))
* **wildcard:** implemented hostname wildcard ([82fc997](https://github.com/bleenco/bproxy/commit/82fc997))
* **ws:** handle websockets ([462c9f3](https://github.com/bleenco/bproxy/commit/462c9f3))



