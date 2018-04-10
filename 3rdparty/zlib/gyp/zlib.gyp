{
  "targets": [{
    "target_name": "zlib",
    "type": "static_library",
    "include_dirs": [
      ".."
    ],
    "direct_dependent_settings": {
      "include_dirs": [
        ".."
      ]
    },
    "sources": [
        "../adler32.c",
        "../compress.c",
        "../crc32.c",
        "../deflate.c",
        "../gzclose.c",
        "../gzlib.c",
        "../gzread.c",
        "../gzwrite.c",
        "../infback.c",
        "../inffast.c",
        "../inflate.c",
        "../inftrees.c",
        "../trees.c",
        "../uncompr.c",
        "../zutil.c",
    ],
  }],
}
