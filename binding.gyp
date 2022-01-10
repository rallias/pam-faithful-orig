{
    "targets": [{
        "target_name": "pam_faithful",
        "sources": [ "src/pam-faithful.cpp" ],
        "libraries": [ "-lpam" ],
        'include_dirs': [
            "<!@(node -p \"require('node-addon-api').include\")"
        ],
        'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
        'cflags!': ['-fno-exceptions'],
        'cflags_cc!': ['-fno-exceptions']
    }]
}
