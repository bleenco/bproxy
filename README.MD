<div align="center">
  <br/>
  <br/>
  <img src="https://user-images.githubusercontent.com/1796022/39833139-8497ece0-53c9-11e8-8fc4-12517a2d303b.png" width="300px">
  <br/>
  <br/>
</div>

[![AbstruseCI](https://ci.bleenco.io/badge/15)](https://ci.bleenco.io/repo/15)

Bproxy is a lightweight **super-fast** **minimal-configuration** proxy server written in C.

> It supports `gzip` compression and `SSL` out-of-the-box.

### Benchmarks: bproxy vs. nginx

We tested simple whoami app on Golang web server using [Wrk](https://github.com/wg/wrk). The tests have been performed on MacBook Pro Late 2016 with 16GB memory using following commands:

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

The results suggest that `nginx` handles requests with more than 50% less latency, while `bproxy` handles more than twice as much requests in the same period.

When this sounds promising, feel free to use it on your server and help us improve this project further!

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

### Sample configuration

```json
{
  "port": 80,
  "secure_port": 443,
  "gzip_mime_types": ["text/css", "application/javascript", "application/x-javascript"],
  "log_file": "bproxy.log",
  "proxies": [
    {
      "hosts": [
        "bleenco.io",
        "www.bleenco.io"
      ],
      "certificate_path": "certs/bleenco.io.crt",
      "key_path": "certs/bleenco.io.key",
      "ip": "127.0.0.1",
      "port": 7500,
      "force_ssl": true
    },
    {
      "hosts": ["*.bleenco.io"],
      "ip": "127.0.0.1",
      "port": 4550,
      "ssl_passthrough": true
    }
  ]
}
```

`force_ssl` property enables redirect from http to https by responding with 301 http status.

`ssl_passthrough` property enables proxying SSL/TLS servers. That means data is not decrypted or parsed, but is just forwarded to server and vice-versa. This also enables redirection from http to https.
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

### Bproxy Team

<div align="center">
  <table>
    <tbody>
      <tr>
        <td align="center" valign="top">
          <img width="150" height="150" src="https://github.com/martinopresnik.png?s=150">
          <br>
          <a href="https://github.com/martinopresnik">Martin Opresnik</a>
        </td>
        <td align="center" valign="top">
          <img width="150" height="150" src="https://github.com/jkuri.png?s=150">
          <br>
          <a href="https://github.com/jkuri">Jan Kuri</a>
        </td>
        <td align="center" valign="top">
          <img width="150" height="150" src="https://github.com/irmana.png?s=150">
          <br>
          <a href="https://github.com/irmana">Irman Abdic</a>
        </td>
      </tr>
    </tbody>
  </table>
</div>

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
