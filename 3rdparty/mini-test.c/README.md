# mini-test.c

Minimalistic portable test runner for C projects.

## Usage

### Tutorial

[![asciicast](https://asciinema.org/a/0uwkwwcdjxqsbc5it8jk888cj.png)](https://asciinema.org/a/0uwkwwcdjxqsbc5it8jk888cj)

### Minimal configuration

```c
#define TEST_LIST(V) \
    V(first_test)    \
    V(second_test)

#include "mini/main.h"

TEST_IMPL(first_test) {
  CHECK_EQ(0, 1, "What else did I expect to happen?");
}

TEST_IMPL(second_test) {
  CHECK(0 == 0, "YES, THIS IS TRUE");
  CHECK_NE(0, 1, "What else did I expect to happen?");
}
```

### Optimal configuration

This type of configuration requires at least three files:

* `test-list.h` with `TEST_LIST` define
* `runner.c` with just `test-list.h` and `mini/main.h` includes
* `test-first.c` (and others) for each individual test. These files have
  `test-list.h` and `mini/test.h` includes and `TEST_IMPL(...) {}` just as in
  minimal configuration above.

## Installation

This project is intended to be used with [gypkg][0]. The `.gyp` file may look
like this:

```json
{
  "targets": [{
    "target_name": "test",
    "type": "executable",

    "variables": {
      "gypkg_deps": [
        "git://github.com/indutny/mini-test.c.git@^v1.1.0 => mini-test.gyp:mini-test",
      ],
    },

    "dependencies": [
      "<!@(gypkg deps <(gypkg_deps))",
    ],

    "sources": [
      "test/runner.c",
      "test/test-first-test.c",
      "test/test-second-test.c",
    ],
  }],
}
```

## LICENSE

This software is licensed under the MIT License.

Copyright Fedor Indutny, 2017.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

[0]: http://gypkg.io/
[1]: https://github.com/indutny/mini-test.c/issues/1
