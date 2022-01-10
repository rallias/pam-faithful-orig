{
    "targets": [{
        "target_name": "pam_faithful",
        "sources": [ "src/pam-faithful.cpp" ],
        "libraries": [ "-lpam" ],
        'include_dirs': [
            "<!@(node -p \"require('node-addon-api').include\")",
            "<!@(node -p \"require('napi_thread_safe_promise').include\")"
        ],
        'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"]
    }]
}