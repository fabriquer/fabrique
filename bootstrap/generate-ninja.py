#!/usr/bin/env python3
#
# Copyright (c) 2014-2016, 2018-2019 Jonathan Anderson
#
# This software was developed by SRI International and the University of
# Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
# ("CTSRD"), as part of the DARPA CRASH research programme and at Memorial University
# of Newfoundland under the NSERC Discovery program (RGPIN-2015-06048).
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
import argparse
import itertools
import pipes
import platform
import os
import sys

args = argparse.ArgumentParser()
args.add_argument('builddir', nargs='?', default='.')
args.add_argument('--debug', action='store_true')
args = args.parse_args()

bootstrap = os.path.realpath(sys.argv[0])

builddir = os.path.realpath(args.builddir)

if not os.path.exists(builddir):
    os.makedirs(builddir)

if not os.path.isdir(builddir):
    sys.write(f'--builddir "{builddir}" is not a directory\n')
    sys.exit(1)


# What source files do we need to build?
import sources
cxx_srcs = sources.cxx_srcs

plugins = {
    'platform': ['PlatformTests'],
    'which': ['Which'],
}

src_root = os.path.dirname(os.path.dirname(bootstrap))

defines = []
ldflags = []
system = platform.system()

if system in ['Darwin', 'FreeBSD', 'Linux']:
    defines.append('OS_POSIX')

    bindir = 'bin/'
    libdir = 'lib/fabrique/'
    libprefix = 'lib'
    cxx_srcs += [
        'lib/platform/posix/PosixError.cc',
        'lib/platform/posix/PosixSharedLibrary.cc',
        'lib/platform/posix/files.cc',
    ]

    if system == 'Darwin':
        ldflags += ['-undefined', 'dynamic_lookup']
        libsuffix = '.dylib'
    else:
        ldflags += ['-rdynamic']
        libsuffix = '.so'

elif system == 'Windows':
    defines.append('OS_WINDOWS')

    bindir = ''
    libdir = ''
    libprefix = ''
    libsuffix = '.dll'

else:
    raise ValueError(f'Unknown platform: {system}')

if system == 'Darwin':
    defines.append('OS_DARWIN')

defines = zip(itertools.repeat('-D'), defines)

include_dirs = zip(itertools.repeat('-I'), [
    src_root,
    builddir,
    f'{src_root}/include',
    f'{src_root}/vendor',
    f'{src_root}/vendor/antlr-cxx-runtime',
])

cxxflags = [
    # Require C++14.
    '-std=c++14',

    # Use position-independent code.
    '-fPIC',

    # Disable all warnings in bootstrap (rather than trying to be selective about
    # warnings in our code vs vendor code, etc.)
    '-w',
]

if args.debug:
    cxxflags += ['-g', '-ggdb', '-O0']
    ldflags += ['-g', '-ggdb']

else:
    cxxflags += ['-D NDEBUG', '-O2']

cxxflags = cxxflags + list(itertools.chain(*defines, *include_dirs))

plugin_files = dict([
    (
        f'{libdir}{libprefix}{name}{libsuffix}',
        [f'base-plugins/{src}.cc' for src in sources]
    )
    for (name, sources) in plugins.items()
])


def which(name):
    paths = os.environ.get('PATH', '').split(os.pathsep)

    for p in paths:
        fullname = os.path.join(p, name)
        if os.path.isfile(fullname) and os.access(fullname, os.X_OK):
            return fullname

    raise OSError(f'no {name} in paths: {" ".join(paths)}')


variables = {
    # tools
    'cc': which('cc'),
    'cxx': which('c++'),

    # flags
    'cxxflags': ' '.join(cxxflags),
    'ldflags': ' '.join(ldflags),
}


# Then describe the mechanics of how to generate a build file.
out = open(os.path.join(builddir, 'build.ninja'), 'w')


for (key, val) in variables.items():
    out.write(f'{key} = {val}\n')

out.write('\n')


# Build rules: how we actually build things.
rules = {
    'bin': {
        'command': 'c++ $ldflags -o $out $in',
        'description': 'Linking $out',
    },

    'cc': {
        'command': '$cc -c $cflags -MMD -MT $out -MF $out.d -o $out $in',
        'description': 'Compiling $in',
        'depfile': '$out.d',
    },

    'cxx': {
        'command': '$cxx -c $cxxflags -MMD -MT $out -MF $out.d -o $out $in',
        'description': 'Compiling $in',
        'depfile': '$out.d',
    },

    'lib': {
        'command': '$cxx -shared -o $out $ldflags $in',
        'description': 'Linking library $out',
    },

    'rebuild': {
        'command': 'python3 $in $args',
        'description': 'Regenerating $out',
        'generator': '',
    },
}

for (name, variables) in rules.items():
    out.write(f'rule {name}\n')
    for (key, val) in variables.items():
        out.write(f'  {key} = {val}\n')
    out.write('\n')


#
# Finally, build statements.
#

# Rebuild the Ninja file:
bootstrap_args = [pipes.quote(builddir)]
if args.debug:
    bootstrap_args.append('--debug')

out.write(f'''build build.ninja: rebuild {pipes.quote(bootstrap)}
  args = {' '.join(bootstrap_args)}

''')


# Main executable
objs = [f'{src}.o' for src in cxx_srcs]

out.write(f'build {bindir}fab: bin {" ".join(objs)}\n\n')
out.write(f'build fab: phony {bindir}fab\n\n')
out.write('default fab\n\n')


# Plugins:
for (plugin, srcs) in plugin_files.items():
    flags = ' '.join(cxxflags)
    objs = [f'{src}.o' for src in srcs]

    out.write(f'build {plugin}: lib {" ".join(objs)}\n\n')
    out.write(f'default {plugin}\n\n')

    for (src, obj) in zip(srcs, objs):
        src = os.path.join(src_root, src)
        out.write(f'build {obj}: cxx {src}\n')
        out.write(f'    cxxflags = {flags}\n')


# C++ -> object files:
for src in cxx_srcs:
    out.write(f'build {src}.o: cxx {os.path.join(src_root, src)}\n')

out.write('\n')
