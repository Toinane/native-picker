{
  "variables": {
    "glfw": "<@(module_root_dir)/libs"
  },
  'targets': [
    {
      'target_name': 'picker',
      'sources': [
        'src/Picker.cc',
        'src/windows/WinPicker.cc'
      ],
      'include_dirs': [
        "<!(node -p \"require('node-addon-api').include_dir\")",
        "<@(module_root_dir)"
        ],
      'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'conditions': [
        ["OS=='win'", {
          "defines": [
            "_HAS_EXCEPTIONS=1"
          ],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1
            },
          },
          "libraries": [
            "<@(module_root_dir)/libs/win32/glfw3dll.lib",
          ]
        }],
        ["OS=='mac'", {
          'cflags+': ['-fvisibility=hidden'],
          'xcode_settings': {
            'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES', # -fvisibility=hidden
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'CLANG_CXX_LIBRARY': 'libc++',
            'MACOSX_DEPLOYMENT_TARGET': '10.7',
          },
        }],
      ],
    }
  ]
}