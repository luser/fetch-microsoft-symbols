# -*- Mode: python; indent-tabs-mode: nil; -*-
{
    'targets': [
    {
        'target_name': 'fetch_microsoft_symbols',
        'type': 'executable',
        'sources': [
            'fetch_microsoft_symbols.cpp',
            'cabextract.cpp',
        ],
        'cflags': [
            '-g',
            '-Wall',
            '-Werror',
            '--std=c++0x',
            '<!@(pkg-config --cflags libmspack libcurl)',
        ],
        'libraries': [
            '<!@(pkg-config --libs libmspack libcurl)',
        ],
    }
    ]
}
