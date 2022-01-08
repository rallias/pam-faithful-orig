{
    "targets": [{
        "target_name": "pam_faithful",
        "sources": [ "src/pam-faithful.cpp" ],
        "libraries": [ "-lpam" ],
        "include_dirs": [
            "<!(node -e \"require('nan')\")"
        ]
    }]
}