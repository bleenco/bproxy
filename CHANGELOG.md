<a name="0.4.0"></a>
# 0.4.0 (2018-04-29)


### Bug Fixes

* **writing error:** fixed writing error by copying data to buffer ([c80f004](https://github.com/jkuri/bproxy/commit/c80f004))


### Features

* **uv_link_t:** used uv_link_t with connections closing ([67bbcf8](https://github.com/jkuri/bproxy/commit/67bbcf8))
* **uv_link_t:** using uv_link_t for communication between client and proxy. ([6602e00](https://github.com/jkuri/bproxy/commit/6602e00))



<a name="0.3.0"></a>
# 0.3.0 (2018-04-11)


### Bug Fixes

* **closing connections:** connections are closed after use. ([17d6a78](https://github.com/jkuri/bproxy/commit/17d6a78))
* **conns:** closing connections ([de49a2e](https://github.com/jkuri/bproxy/commit/de49a2e))
* **dockerfile:** update ([48c2605](https://github.com/jkuri/bproxy/commit/48c2605))
* **gzipped-chunks:** closes [#12](https://github.com/jkuri/bproxy/issues/12) ([f423828](https://github.com/jkuri/bproxy/commit/f423828))
* **memory leak:** fixed some memory leaks and uninitialized variables. ([d709cb7](https://github.com/jkuri/bproxy/commit/d709cb7))
* **usage:** update usage text ([756066f](https://github.com/jkuri/bproxy/commit/756066f))


### Features

* **gzip:** add gzip configurable mime types inside JSON config ([dc0d2a5](https://github.com/jkuri/bproxy/commit/dc0d2a5))
* **gzip:** added gzip to compress server responses. ([90125a0](https://github.com/jkuri/bproxy/commit/90125a0))
* **logger:** implement logger ([3f6beca](https://github.com/jkuri/bproxy/commit/3f6beca))
* **wildcard:** implemented hostname wildcard ([82fc997](https://github.com/jkuri/bproxy/commit/82fc997))
* **ws:** handle websockets ([462c9f3](https://github.com/jkuri/bproxy/commit/462c9f3))



<a name="0.2.0"></a>
# 0.2.0 (2018-03-28)


### Bug Fixes

* **dockerfile:** update ([48c2605](https://github.com/jkuri/bproxy/commit/48c2605))
* **usage:** update usage text ([756066f](https://github.com/jkuri/bproxy/commit/756066f))


### Features

* **gzip:** added gzip to compress server responses. ([90125a0](https://github.com/jkuri/bproxy/commit/90125a0))
* **wildcard:** implemented hostname wildcard ([82fc997](https://github.com/jkuri/bproxy/commit/82fc997))
* **ws:** handle websockets ([462c9f3](https://github.com/jkuri/bproxy/commit/462c9f3))



