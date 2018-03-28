## bproxy

bproxy is lightweight super-fast minimal configuration proxy server written in C based on `libuv` that supports gzip natively.

### Building project

```sh
$ make
```

Build artifacts will be stored in `build/` directory.

#### Static Build

This is only possible on Linux for now. Makefile is configured to search for `libuv.a` in `/usr/local/lib/libuv.a`.

```sh
$ make static
```

### Install

Script will install `bproxy` in `/usr/local/bin`.

```sh
sudo make install
```

### Running server

```sh
./bproxy
```

or

```
./bproxy -c /path/to/bproxy-config.json
```

### LICENSE

```
The MIT License

Copyright (c) 2018 Bleenco GmbH http://bleenco.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```
