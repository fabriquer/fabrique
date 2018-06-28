import os.path

srcroot = os.path.abspath(os.path.dirname(__file__))

def FlagsForFile(filename, **kwargs):
    return {
        'flags': [
            # Include files rooted in the current source tree
            '-I%s' % srcroot,
            '-I%s/build' % srcroot,
            '-I%s/build/Debug' % srcroot,

            # Treat vendor headers as system headers (ignore warnings)
            '-isystem ', ' vendor ',

            # Provide lots and lots of warnings!
            '-Weverything',

            # We really, really don't care about C++98 compatibility.
            '-Wno-c++98-compat', '-Wno-c++98-compat-pedantic',

            # We aren't (yet) concerned about ABI stability.
            '-Wno-padded',

            # We use C++11 features (but not C++14) for a balance of non-terribleness
            # and almost-ubiquitous availability.
            '-std=c++11'
        ],
    }
