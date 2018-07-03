{
  "variables": {
    "gypkg_deps": [
      "../libuv => uv.gyp:libuv",
      "../uv_link_t => uv_link_t.gyp:uv_link_t",
      "../openssl => openssl.gyp:openssl",
      "../ringbuffer => ringbuffer.gyp:ringbuffer",
    ],
  },

  "targets": [{
    "target_name": "uv_ssl_t",
    "type": "<!(gypkg type)",

    "dependencies": [
      "<!@(gypkg deps <(gypkg_deps))",
    ],

    "direct_dependent_settings": {
      "include_dirs": [ "include" ],
    },

    "include_dirs": [
      ".",
    ],

    "sources": [
      "src/bio.c",
      "src/link_methods.c",
      "src/uv_ssl_t.c",
    ],
  }, {
    "target_name": "uv_ssl_t-test",
    "type": "executable",

    "include_dirs": [
      "src"
    ],

    "dependencies": [
      "<!@(gypkg deps <(gypkg_deps))",
      "uv_ssl_t",
    ],

    "sources": [
      "test/src/main.c",
      "test/src/test-handshake.c",
      "test/src/test-error.c",
      "test/src/test-error-on-eof.c",
      "test/src/test-read-incoming.c",
      "test/src/test-shutdown.c",
      "test/src/test-try-write.c",
      "test/src/test-write.c",
      "test/src/test-close-in-read-cb.c",
    ],
  }, {
    "target_name": "uv_ssl_t-example",
    "type": "executable",

    "include_dirs": [
      "example/src"
    ],

    "dependencies": [
      "<!@(gypkg deps <(gypkg_deps))",
      "uv_ssl_t",
    ],

    "sources": [
      "example/src/main.c",
      "example/src/middle.c",
    ],
  }],
}
