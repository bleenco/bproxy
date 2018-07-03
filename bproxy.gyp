{
  "variables": {
    "gypkg_deps": [
      "3rdparty/libuv => uv.gyp:libuv",
      "3rdparty/uv_link_t => uv_link_t.gyp:uv_link_t",
      "3rdparty/uv_ssl_t => uv_ssl_t.gyp:uv_ssl_t",
      "3rdparty/openssl  => openssl.gyp:openssl",
      "3rdparty/zlib => gyp/zlib.gyp:zlib"
    ]
  },
  "targets": [{
    "target_name": "bproxy",
    "type": "executable",
    "dependencies": [
      "<!@(gypkg deps <(gypkg_deps))",
    ],
    "direct_dependent_settings": {
      "include_dirs": ["include"],
    },
    "include_dirs": [
      "include"
    ],
    "sources": [
      "src/log.c",
      "src/config.c",
      "src/gzip.c",
      "src/http_parser.c",
      "src/http.c",
      "src/cJSON.c",
      "src/http_link.c",
      "src/bproxy.c"
    ]
  }]
}
