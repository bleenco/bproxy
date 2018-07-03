{
  "targets": [ {
    "target_name": "mini-test",
    "type": "<!(gypkg type)",

    "direct_dependent_settings": {
      "include_dirs": [
        "include",
      ],
    },

    "include_dirs": [
      ".",
    ],

    "conditions": [
      ["OS=='win'", {
        "sources": [
          "src/spawn-win.c",
        ],
      }, {
        "sources": [
          "src/spawn-unix.c",
        ],
      }],
    ],
  }, {
    "target_name": "mini-test-test",
    "type": "executable",

    "dependencies": [
      "mini-test",
    ],

    "include_dirs": [
      ".",
    ],

    "sources": [
      "test/test.c",
    ],
  }],
}
