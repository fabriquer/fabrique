#!/usr/bin/env python3
#
# Copyright (c) 2019 Jonathan Anderson
#
# This software was developed at Memorial University of Newfoundland
# under the NSERC Discovery program (RGPIN-2015-06048).
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
import collections
import itertools
import sys


class BootstrapBuild:
    """Base class for a build description."""

    def __init__(self, src_root):
        self.cxxflags = []
        self.defines = []
        self.dirs = collections.defaultdict(str)
        self.include_dirs = []
        self.ldflags = []
        self.libraries = []
        self.prefixes = collections.defaultdict(str)
        self.regen = None
        self.sources = []
        self.suffixes = collections.defaultdict(str)
        self.tools = {
            'cxx': 'c++',
            'python': 'python3',
        }

        self.dirs['src'] = src_root

    def add_cxxflags(self, *flags):
        self.cxxflags.extend(flags)

    def add_ldflags(self, *flags):
        self.ldflags.extend(flags)

    def add_library(self, name, *sources):
        full_name = self.dirs['lib'] + \
            self.prefixes['lib'] + \
            name + \
            self.suffixes['lib']

        self.libraries.append((full_name, sources))

    def add_sources(self, *srcs):
        self.sources.extend(srcs)

    def add_regeneration(self, script, args):
        self.regen = (script, args)

    def define(self, flag):
        """Add a preprocessor definition to the build description."""
        self.defines.append(flag)

    def dir(self, name, path):
        self.dirs[name] = path

    def include(self, *dirs):
        """Add an include directory (or directories) to the build description."""
        self.include_dirs.extend(dirs)

    def prefix(self, name, value):
        """Define a named prefix (e.g., 'lib' for libraries)."""
        self.prefixes[name] = value

    def suffix(self, name, value):
        """Define a named suffix (e.g., '.dylib' for shared libraries on macOS)."""
        self.suffixes[name] = value

    def all_cxxflags(self):
        defines = zip(itertools.repeat('-D'), self.defines)
        includes = zip(itertools.repeat('-I'), self.include_dirs)

        return self.cxxflags + list(itertools.chain(*defines, *includes))
