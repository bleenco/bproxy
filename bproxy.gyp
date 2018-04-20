{
  "variables": {
    "gypkg_deps": [
      "git://github.com/libuv/libuv.git@^v1.x => uv.gyp:libuv",
      "git://github.com/indutny/uv_link_t@v1.0.5 => uv_link_t.gyp:uv_link_t",
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
      "src/jsmn.c",
      "src/http_link.c",
      "src/bproxy.c"
    ]
  }]
}
