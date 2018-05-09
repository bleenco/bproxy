<p align="center" style="margin-top: 50px; margin-bottom: 50px">
  <img src="https://user-images.githubusercontent.com/1796022/39833139-8497ece0-53c9-11e8-8fc4-12517a2d303b.png">
</p>

bproxy is lightweight super-fast minimal configuration proxy server written in C based on `libuv`.

It supports `gzip` compression and `SSL` out-of-the-box.

### Building project

This step assumes you have Node.JS installed on your system.

Install dependencies.

```sh
$ npm install --prod
```

Run the build.

```sh
$ npm run build
```

Build artifacts will be stored in `out/Release/` directory.


### Install

Script will install `bproxy` in `/usr/local/bin`.

```sh
$ sudo make install
```

### Running server

```sh
$ ./bproxy
```

or

```
$ ./bproxy -c /path/to/bproxy-config.json
```

### Building Docker Image

```sh
$ make docker_image
```

### Using Docker

You can pull image from Docker Hub as `docker pull bleenco/bproxy` or build from source as stated in previous section.

Then on your server:

```
$ docker run -it --name bproxy --net host -v ~/path/to/bproxy.json:/bproxy/bproxy.json -v /var/log/bproxy.log:/var/log/bproxy.log --restart always bleenco/bproxy
```

### Running Tests

```sh
$ npm install
$ npm test
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
