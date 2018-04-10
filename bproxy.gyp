{
  "variables": {
    "gypkg_deps": [
      "git://github.com/libuv/libuv.git@^v1.x => uv.gyp:libuv",
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
      "src/bproxy.c"
    ]
  }]
}
