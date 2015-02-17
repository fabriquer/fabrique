#!/usr/bin/env python

import argparse
import itertools
import pipes
import platform
import os
import sys

args = argparse.ArgumentParser()
args.add_argument('builddir', nargs = '?', default = '.')
args.add_argument('--cxxflags', default = '')
args.add_argument('--debug', action = 'store_true')
args = args.parse_args()

bootstrap = os.path.realpath(sys.argv[0])

builddir = os.path.realpath(args.builddir)

if not os.path.exists(builddir):
	os.makedirs(builddir)

if not os.path.isdir(builddir):
	sys.write("Build directory '%s' is not a directory" % args.builddir)
	sys.exit(1)


# First, let's declare what we actually want to build.
cxx_srcs = {
	'AST/': (
		'Action', 'Argument', 'ASTDump', 'BinaryOperation', 'Call',
		'CompoundExpr', 'Conditional', 'DebugTracePoint',
		'EvalContext', 'Expression',
		'FieldAccess', 'FieldQuery', 'Filename', 'FileList',
		'Foreach', 'Function',
		'HasParameters', 'HasScope', 'Identifier', 'Import', 'List',
		'Mapping', 'Node', 'Parameter',
		'Scope', 'SomeValue', 'StructInstantiation', 'SymbolReference',
		'UnaryOperation', 'Value', 'Visitor',
		'literals',
	),
	'Backend/': (
		'Backend', 'Dot', 'Make', 'Ninja', 'Null',
	),
	'DAG/': (
		'Build', 'Callable', 'DAG', 'DAGBuilder',
		'File', 'Formatter', 'Function',
		'List', 'Parameter', 'Primitive',
		'Rule', 'Structure', 'Target', 'UndefinedValueException',
		'Value', 'Visitor',
	),
	'Parsing/': (
		'Lexer', 'Parser', 'Token',
	),
	'Plugin/': (
		'Loader', 'Plugin', 'Registry',
	),
	'Support/': (
		'Arguments', 'Bytestream', 'ErrorReport', 'Join', 'Printable',
		'SharedLibrary', 'SourceLocation', 'String',
		'exceptions', 'os-posix',
	),
	'Types/': (
		'BooleanType', 'FileType', 'FunctionType', 'IntegerType', 'MaybeType',
		'OptionallyTyped', 'SequenceType', 'StringType', 'StructureType',
		'Type', 'TypeContext', 'TypeError', 'Typed',
	),
	'': (
		'driver',
	),
}

plugins = {
	'platform-tests': [ 'PlatformTests' ],
	'sysctl': [ 'SysctlPlugin' ],
	'which': [ 'Which' ],
}

cxx_srcs = list(itertools.chain(*[
	[ '%s%s' % (subdir,base) for base in srcs ]
		for (subdir,srcs) in cxx_srcs.items()
]))

lex = { 'Parsing/fab.lxx': 'Parsing/fab.lex' }
yacc = { 'Parsing/fab.yy': 'Parsing/fab.yacc' }

src_root = os.path.dirname(os.path.dirname(bootstrap))

defines = []
ldflags = []
system = platform.system()

if system in [ 'Darwin', 'FreeBSD', 'Linux' ]:
	defines.append('OS_POSIX')

	bindir = 'bin/'
	libdir = 'lib/fabrique/'
	libprefix = 'lib'
	cxx_srcs += [ 'Support/PosixError', 'Support/PosixSharedLibrary' ]

	if system == 'Darwin':
		ldflags += [ '-undefined', 'dynamic_lookup' ]
		libsuffix = '.dylib'
	else:
		ldflags += [ '-rdynamic' ]
		libsuffix = '.so'

elif system == 'Windows':
	defines.append('OS_WINDOWS')

	bindir = ''
	libdir = ''
	libprefix = ''
	libsuffix = '.dll'

else:
	raise ValueError('Unknown platform: %s' % system)

if system == 'Darwin':
	defines.append('OS_DARWIN')

defines = list(
	itertools.chain.from_iterable(
		itertools.izip(itertools.repeat('-D'), defines)
	)
)
cxxflags = defines + [
	'-I', src_root, '-I', builddir,

	# Require C++11.
	'-std=c++11',

	# Use position-independent code.
	'-fPIC',
] + args.cxxflags.split()

warnings = [
	# Use lots and lots of warnings.
	'-Weverything',

	# We intentionally hide yyFlexLexer::yylex().
	'-Wno-overloaded-virtual',

	# We really, really don't care about C++98 compatibility.
	'-Wno-c++98-compat', '-Wno-c++98-compat-pedantic',

	# We aren't attempting to preserve an ABI. At least not yet.
	'-Wno-padded',

	# Treat vendor headers as system headers: disable warnings.
	'-isystem %s/vendor' % src_root
]

plugin_warnings = [
	'-Wno-global-constructors',
	'-Wno-exit-time-destructors',
]

if args.debug:
    cxxflags += [ '-g', '-ggdb', '-O0' ]
    ldflags += [ '-g', '-ggdb' ]

else:
    cxxflags += [ '-D NDEBUG', '-O2' ]

gencxxflags = cxxflags + [ '-Wno-deprecated-register' ]
if args.debug: gencxxflags += [ '-D YYDEBUG' ]

cxxflags = cxxflags + warnings

plugin_files = dict([
	(
		('%s%s%s%s' % (libdir, libprefix, name, libsuffix)),
		[ 'plugins/%s.cc' % s for s in sources ]
	)
	for (name, sources) in plugins.items()
])


def which(name):
	for p in os.environ.get('PATH', '').split(os.pathsep):
		fullname = os.path.join(p, name)
		if os.path.isfile(fullname) and os.access(fullname, os.X_OK):
			return fullname

	raise OSError('no %s in $PATH (%s)' % (
		name, os.environ.get('PATH')))


variables = {
	# paths
	'path': os.pathsep.join([ builddir, os.environ['PATH'] ]),

	# tools
	'cc': which('clang'),
	'cxx': which('clang++'),
	'lex': which('flex'),
	'lit': which('llvm-lit'),
	'yacc': which('byacc'),

	# flags
	'cxxflags': ' '.join(cxxflags),
	'ldflags': ' '.join(ldflags),
	'yaccflags': '-d -g -t -v',
}



# Then describe the mechanics of how to generate a build file.
out = open(os.path.join(builddir, 'build.ninja'), 'w')


for var in variables.items():
	out.write('%s = %s\n' % var)

out.write('\n')

test_output = os.path.join(builddir, 'test')
if not os.path.isdir(builddir):
	os.mkdir(test_output)

lit_config = '-sv --param=output-dir=%s --output=%s' % (
	test_output, os.path.join(test_output, 'report.txt'),
)


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

	'lex': {
		'command': '$lex -c++ --header-file=$header --outfile=$main_out $in',
		'description': 'Processing $in',
	},

	'lib': {
		'command': '$cxx -shared -o $out $ldflags $in',
		'description': 'Linking library $out',
	},

	'lit': {
		'command': 'PATH=$path $lit ' + lit_config + ' $in',
		'description': 'Running unit tests',
	},

	'rebuild': {
		'command': 'PATH=$path python $in $args',
		'description': 'Regenerating $out',
		'generator': '',
	},

	'yacc': {
		'command': '$yacc $yaccflags -o $main_out $in',
		'description': 'Processing $in',
	},
}

for (name, variables) in rules.items():
	out.write('rule %s\n' % name)
	for var in variables.items():
		out.write('  %s = %s\n' % var)
	out.write('\n')


#
# Finally, build statements.
#

# Rebuild the Ninja file:
bootstrap_args = [ pipes.quote(builddir) ]
if args.cxxflags: bootstrap_args += [ '--cxxflags="%s"' % args.cxxflags ]
if args.debug: bootstrap_args.append('--debug')

out.write('''build build.ninja: rebuild %s
  args = %s

''' % (pipes.quote(bootstrap), ' '.join(bootstrap_args)))

# Unit tests:
out.write('build test: phony run-tests\n')
out.write('build run-tests: lit %s/test | %sfab\n' % (src_root, bindir))


# Main executable
objs = [ '%s.o' % o for o in cxx_srcs + lex.values() + yacc.values() ]

out.write('build %sfab: bin %s\n\n' % (bindir, ' '.join(objs)))
out.write('build fab: phony %sfab\n\n' % bindir)
out.write('default fab\n\n')


# Plugins:
for (plugin, srcs) in plugin_files.items():
	flags = ' '.join(cxxflags + plugin_warnings)
	objs = [ '%s.o' % o for o in srcs ]

	out.write('build %s: lib %s\n\n' % (plugin, ' '.join(objs)))
	out.write('default %s\n\n' % plugin)

	for (src, obj) in itertools.izip(srcs, objs):
		src = os.path.join(src_root, src)
		out.write('build %s: cxx %s\n' % (obj, src))
		out.write('    cxxflags = %s\n' % flags)


# C++ -> object files:
for src in cxx_srcs:
	obj = '%s.o' % src
	src = os.path.join(src_root, '%s.cc' % src)

	if 'Lex' in src or 'Parser' in src or 'driver' in src:
		src += ' | Parsing/fab.yacc.h'

	out.write('build %s: cxx %s\n' % (obj, src))

out.write('\n')

# Lex and yacc:
for (src,target) in lex.items():
	src = os.path.join(src_root, src)
	out.write('build %s.o: cxx %s.cc\n' % (target, target))
	out.write('  cxxflags = %s\n\n' % ' '.join(gencxxflags))

	out.write('build %s.h %s.cc: lex %s\n' % (target, target, src))
	out.write('  header = %s.h\n' % target)
	out.write('  main_out = %s.cc\n\n' % target)

for (src,target) in yacc.items():
	src = os.path.join(src_root, src)
	out.write('build %s.o: cxx %s.cc\n' % (target, target))
	out.write('  cxxflags = %s\n\n' % ' '.join(gencxxflags))

	out.write('build %s.h %s.cc: yacc %s\n' % (target, target, src))
	out.write('  main_out = %s.cc\n\n' % target)
