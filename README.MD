<p align="center" style="padding: 0 0 50px 0;">
  <img src="https://user-images.githubusercontent.com/1796022/39833139-8497ece0-53c9-11e8-8fc4-12517a2d303b.png" width="300px">
</p>

bproxy is lightweight super-fast minimal configuration proxy server written in C based on `libuv`.

It supports `gzip` compression and `SSL` out-of-the-box.

### Benchmarks versus nginx

We tested simple whoami app running on Golang web server on MacBook Pro Late 2016 with 16GB memory and here are the results:

1. `nginx`

```
$ wrk -t20 -c1000 -d10s http://whoami.home.local.net
Running 10s test @ http://whoami.home.local.net
  20 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    95.03ms   55.24ms 594.79ms   85.69%
    Req/Sec    78.38     47.01   230.00     65.43%
  7786 requests in 10.06s, 2.85MB read
  Socket errors: connect 0, read 872, write 0, timeout 92
  Non-2xx or 3xx responses: 52
Requests/sec:    774.08
Transfer/sec:    290.10KB
```

2. `bproxy`

```
$ wrk -t20 -c1000 -d10s http://whoami.home.local.net
Running 10s test @ http://whoami.home.local.net
  20 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   274.84ms  117.52ms   1.05s    71.95%
    Req/Sec    97.55     55.21   515.00     71.70%
  19292 requests in 10.10s, 6.66MB read
  Socket errors: connect 0, read 970, write 0, timeout 0
Requests/sec:   1910.05
Transfer/sec:    675.23KB
```

As you can see from the results it is clearly that `nginx` handles requests with less latency but `bproxy` can handle more than twice as much requests in same period than `nginx`.

Not bad for a project that started as a hobby, right?

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
