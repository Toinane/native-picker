{
  'targets': [
    {
      'target_name': 'picker',
      'sources': ["<!@(node config/sources.js)"],
      'include_dirs': ["<!@(node -p \"require('node-addon-api').include\")"],
      'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'conditions': [
        ["OS=='win'", {
          "defines": [
            "_HAS_EXCEPTIONS=1",
            "IS_WINDOWS",
          ],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1
            },
          }
        }],
        ["OS=='mac'", {
          'xcode_settings': {
            'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES', # -fvisibility=hidden
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'CLANG_CXX_LIBRARY': 'libc++',
            'MACOSX_DEPLOYMENT_TARGET': '11',
            'CFLAGS': [
              '-arch x86_64',
              '-arch arm64',
            ],
            'LDFLAGS': [
              '-arch x86_64',
              '-arch arm64',
            ],
            'include_dirs': [
              'System/Library/Frameworks/CoreFoundation.Framework/Headers',
              'System/Library/Frameworks/Carbon.Framework/Headers',
              'System/Library/Frameworks/ApplicationServices.framework/Headers',
              'System/Library/Frameworks/OpenGL.framework/Headers',
            ],
            'link_settings': {
              'libraries': [
                '-framework Carbon',
                '-framework CoreFoundation',
                '-framework ApplicationServices',
                '-framework OpenGL'
              ]
            }
          },
        }],
        ['OS == "linux"', {
          'link_settings': {
            'libraries': [
              '-lpng',
              '-lz',
              '-lX11',
              '-lXtst'
            ]
          }
        }],
      ],
    }
  ]
}