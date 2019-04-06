#!/usr/bin/env python2.7

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
args.add_argument('--testpath', help = 'PATH containing llvm-lit and FileCheck')
args.add_argument('--withtests', action = 'store_true')
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
	'bin/': (
		'CLIArguments',
		'fab',
	),
	'lib/': (
		'AssertionFailure', 'Bytestream', 'ErrorReport', 'Fabrique', 'FabBuilder',
                'Printable', 'SourceCodeException', 'SourceLocation', 'SourceRange',
                'builtins', 'names', 'strings',
	),
	'lib/ast/': (
		'ASTDump', 'Action', 'Argument', 'Arguments', 'BinaryOperation', 'Call',
		'CompoundExpr', 'Conditional', 'DebugTracePoint', 'EvalContext',
		'Expression', 'FieldAccess', 'FieldQuery', 'FileList', 'FilenameLiteral',
		'Foreach', 'Function', 'HasParameters', 'Identifier', 'List',
		'NameReference', 'Node', 'Parameter', 'Record', 'TypeDeclaration',
		'TypeReference', 'UnaryOperation', 'Value', 'Visitor', 'literals',
	),
	'lib/backend/': (
		'Backend', 'Dot', 'Make', 'Ninja', 'Null',
	),
	'lib/dag/': (
		'Build', 'Callable', 'DAG', 'DAGBuilder',
		'File', 'Formatter', 'Function',
		'List', 'Parameter', 'Primitive',
		'Record', 'Rule', 'TypeReference',
                'UndefinedValueException',
		'Value', 'Visitor',
	),
	'lib/parsing/': (
		'ASTBuilder', 'ErrorListener', 'ErrorReporter', 'Parser', 'Token',
	),
	'lib/platform/': (
		'ABI', 'OSError', 'SharedLibrary',
	),
	'lib/plugin/': (
		'Loader', 'Plugin', 'Registry',
	),
	'lib/types/': (
		'BooleanType', 'FileType', 'FunctionType', 'IntegerType', 'RecordType',
		'SequenceType', 'StringType', 'Type', 'TypeContext', 'TypeError', 'Typed',
	),
	'Support/': (
		'exceptions',
	),
	'vendor/antlr-cxx-runtime/': (
		'ANTLRErrorListener', 'ANTLRErrorStrategy', 'ANTLRFileStream',
		'ANTLRInputStream', 'BailErrorStrategy', 'BaseErrorListener',
		'BufferedTokenStream', 'CharStream', 'CommonToken', 'CommonTokenFactory',
		'CommonTokenStream', 'ConsoleErrorListener', 'DefaultErrorStrategy',
		'DiagnosticErrorListener', 'Exceptions', 'FailedPredicateException',
		'InputMismatchException', 'IntStream', 'InterpreterRuleContext', 'Lexer',
		'LexerInterpreter', 'LexerNoViableAltException', 'ListTokenSource',
		'NoViableAltException', 'Parser', 'ParserInterpreter',
		'ParserRuleContext', 'ProxyErrorListener', 'RecognitionException',
		'Recognizer', 'RuleContext', 'RuleContextWithAltNum', 'RuntimeMetaData',
		'Token', 'TokenSource', 'TokenStream', 'TokenStreamRewriter',
		'UnbufferedCharStream', 'UnbufferedTokenStream', 'Vocabulary',
		'WritableToken',
	),
	'vendor/antlr-cxx-runtime/atn/': (
		'ATN', 'ATNConfig', 'ATNConfigSet', 'ATNDeserializationOptions',
		'ATNDeserializer', 'ATNSerializer', 'ATNSimulator', 'ATNState',
		'AbstractPredicateTransition', 'ActionTransition', 'AmbiguityInfo',
		'ArrayPredictionContext', 'AtomTransition', 'BasicBlockStartState',
		'BasicState', 'BlockEndState', 'BlockStartState',
		'ContextSensitivityInfo', 'DecisionEventInfo', 'DecisionInfo',
		'DecisionState', 'EmptyPredictionContext', 'EpsilonTransition',
		'ErrorInfo', 'LL1Analyzer', 'LexerATNConfig', 'LexerATNSimulator',
		'LexerAction', 'LexerActionExecutor', 'LexerChannelAction',
		'LexerCustomAction', 'LexerIndexedCustomAction', 'LexerModeAction',
		'LexerMoreAction', 'LexerPopModeAction', 'LexerPushModeAction',
		'LexerSkipAction', 'LexerTypeAction', 'LookaheadEventInfo',
		'LoopEndState', 'NotSetTransition', 'OrderedATNConfigSet', 'ParseInfo',
		'ParserATNSimulator', 'PlusBlockStartState', 'PlusLoopbackState',
		'PrecedencePredicateTransition', 'PredicateEvalInfo',
		'PredicateTransition', 'PredictionContext', 'PredictionMode',
		'ProfilingATNSimulator', 'RangeTransition', 'RuleStartState',
		'RuleStopState', 'RuleTransition', 'SemanticContext', 'SetTransition',
		'SingletonPredictionContext', 'StarBlockStartState', 'StarLoopEntryState',
		'StarLoopbackState', 'TokensStartState', 'Transition',
		'WildcardTransition',
	),
	'vendor/antlr-cxx-runtime/dfa/': (
		'DFA', 'DFAState', 'DFASerializer', 'LexerDFASerializer',
	),
	'vendor/antlr-cxx-runtime/misc/': (
		'InterpreterDataReader', 'Interval', 'IntervalSet', 'MurmurHash',
		'Predicate',
	),
	'vendor/antlr-cxx-runtime/support/': (
		'Any', 'Arrays', 'CPPUtils', 'StringUtils', 'guid',
	),
	'vendor/antlr-cxx-runtime/tree/': (
		'ErrorNode', 'ErrorNodeImpl', 'IterativeParseTreeWalker', 'ParseTree',
		'ParseTreeListener', 'ParseTreeVisitor', 'ParseTreeWalker',
		'TerminalNode', 'TerminalNodeImpl', 'Trees',
	),
	'vendor/antlr-cxx-runtime/tree/pattern/': (
		'Chunk', 'ParseTreeMatch', 'ParseTreePattern', 'ParseTreePatternMatcher',
		'RuleTagToken', 'TagChunk', 'TextChunk', 'TokenTagToken',
	),
	'vendor/antlr-cxx-runtime/tree/xpath/': (
		'XPath', 'XPathElement', 'XPathLexer', 'XPathLexerErrorListener',
		'XPathRuleAnywhereElement', 'XPathRuleElement',
		'XPathTokenAnywhereElement', 'XPathTokenElement',
		'XPathWildcardAnywhereElement', 'XPathWildcardElement',
	),
	'vendor/generated-grammar/': (
		'FabLexer', 'FabParser', 'FabParserBaseVisitor', 'FabParserVisitor',
	),
}

plugins = {
	'platform': [ 'PlatformTests' ],
	'sysctl': [ 'SysctlPlugin' ],
	'which': [ 'Which' ],
}

def extension(subdir):
	return 'cpp' if subdir.startswith('vendor') else 'cc'

cxx_srcs = list(itertools.chain(*[
	[ '%s%s.%s' % (subdir,base,extension(subdir)) for base in srcs ]
		for (subdir,srcs) in cxx_srcs.items()
]))

src_root = os.path.dirname(os.path.dirname(bootstrap))

defines = []
ldflags = []
system = platform.system()

if system in [ 'Darwin', 'FreeBSD', 'Linux' ]:
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

	# Require C++14.
	'-std=c++14',

	# Use position-independent code.
	'-fPIC',
] + args.cxxflags.split()

warnings = [
	# Treat vendor headers as system headers: disable warnings.
	'-isystem %s/include' % src_root,
	'-isystem %s/vendor' % src_root,
	'-isystem %s/vendor/antlr-cxx-runtime' % src_root,
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
cxxflags = cxxflags + warnings

plugin_files = dict([
	(
		('%s%s%s%s' % (libdir, libprefix, name, libsuffix)),
		[ 'base-plugins/%s.cc' % s for s in sources ]
	)
	for (name, sources) in plugins.items()
])


def which(name):
	paths = []
	if args.testpath: paths.append(args.testpath)
	paths += os.environ.get('PATH', '').split(os.pathsep)

	for p in paths:
		fullname = os.path.join(p, name)
		if os.path.isfile(fullname) and os.access(fullname, os.X_OK):
			return fullname

	raise OSError('no %s in --testpath (%s) or $PATH (%s)' % (
		name, args.testpath, os.environ.get('PATH')))


variables = {
	# paths
	'path': os.pathsep.join([ builddir, os.environ['PATH'] ]),

	# tools
	'cc': which('clang'),
	'cxx': which('clang++'),

	# flags
	'cxxflags': ' '.join(cxxflags),
	'ldflags': ' '.join(ldflags),
}

if args.withtests:
	variables['lit'] = which('llvm-lit')



# Then describe the mechanics of how to generate a build file.
out = open(os.path.join(builddir, 'build.ninja'), 'w')


for var in variables.items():
	out.write('%s = %s\n' % var)

out.write('\n')

if args.withtests:
	test_output = os.path.join(builddir, 'tests')
	if not os.path.isdir(builddir):
		os.mkdir(test_output)

	lit_config = '-sv --param=build-dir=%s --param=output-dir=%s --output=%s' % (
		builddir, test_output, os.path.join(test_output, 'report.txt'),
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

	'lib': {
		'command': '$cxx -shared -o $out $ldflags $in',
		'description': 'Linking library $out',
	},

	'rebuild': {
		'command': 'python2.7 $in $args',
		'description': 'Regenerating $out',
		'generator': '',
	},
}

if args.withtests:
	rules['lit'] = {
		'command': 'PATH=$path $lit ' + lit_config + ' $in',
		'description': 'Running unit tests',
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
if args.testpath: bootstrap_args += [ '--testpath', args.testpath ]
if args.withtests: bootstrap_args.append('--withtests')

out.write('''build build.ninja: rebuild %s
  args = %s

''' % (pipes.quote(bootstrap), ' '.join(bootstrap_args)))

# Unit tests:
if args.withtests:
	out.write('build test: phony run-tests\n')
	out.write('build run-tests: lit %s/tests | %sfab\n' % (
		src_root, bindir))


# Main executable
objs = [ '%s.o' % o for o in cxx_srcs ]

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
	src = os.path.join(src_root, '%s' % src)
	out.write('build %s: cxx %s\n' % (obj, src))

out.write('\n')
