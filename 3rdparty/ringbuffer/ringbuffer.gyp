{
  "targets": [{
    "target_name": "ringbuffer",
    "type": "<!(gypkg type)",
    "direct_dependent_settings": {
      "include_dirs": [ "." ],
    },
    "sources": [
      "ringbuffer.c",
    ],
  }, {
    "target_name": "ringbuffer-test",
    "type": "executable",
    "dependencies": [ "ringbuffer" ],
    "sources": [ "test.c" ],
  }]
}
