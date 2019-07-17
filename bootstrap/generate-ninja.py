#!/usr/bin/env python3

import argparse
import itertools
import pipes
import platform
import os
import sys

args = argparse.ArgumentParser()
args.add_argument('builddir', nargs='?', default='.')
args.add_argument('--cxxflags', default='')
args.add_argument('--debug', action='store_true')
args = args.parse_args()

bootstrap = os.path.realpath(sys.argv[0])

builddir = os.path.realpath(args.builddir)

if not os.path.exists(builddir):
    os.makedirs(builddir)

if not os.path.isdir(builddir):
    sys.write(f'--builddir "{builddir}" is not a directory\n')
    sys.exit(1)


# First, let's declare what we actually want to build.
cxx_srcs = {
    'bin/': (
        'CLIArguments',
        'fab',
    ),
    'lib/': (
        'AssertionFailure', 'Bytestream', 'ErrorReport', 'Fabrique', 'FabBuilder',
        'Printable', 'SemanticException',
        'SourceCodeException', 'SourceLocation', 'SourceRange', 'UserError',
        'builtins', 'names', 'strings',
    ),
    'lib/ast/': (
        'ASTDump', 'Action', 'Argument', 'Arguments', 'BinaryOperation', 'Call',
        'CompoundExpr', 'Conditional', 'EvalContext', 'Expression',
        'FieldAccess', 'FieldQuery', 'FileList', 'FilenameLiteral',
        'Foreach', 'Function', 'HasParameters', 'Identifier', 'List',
        'NameReference', 'Node', 'Parameter', 'Record', 'SyntaxError',
        'TypeDeclaration', 'TypeReference',
        'UnaryOperation', 'Value', 'Visitor', 'literals',
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
        'ASTBuilder', 'ErrorListener', 'ErrorReporter',
        'Parser', 'ParserError', 'Token',
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
    'platform': ['PlatformTests'],
    'sysctl': ['SysctlPlugin'],
    'which': ['Which'],
}


def extension(subdir):
    return 'cpp' if subdir.startswith('vendor') else 'cc'


cxx_srcs = list(itertools.chain(*[
    [f'{subdir}{base}.{extension(subdir)}' for base in srcs]
    for (subdir, srcs) in cxx_srcs.items()
]))

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

defines = itertools.chain(*zip(itertools.repeat('-D'), defines))
cxxflags = list(defines) + [
    '-I', src_root, '-I', builddir,

    # Require C++14.
    '-std=c++14',

    # Use position-independent code.
    '-fPIC',
] + args.cxxflags.split()

warnings = [
    # Treat vendor headers as system headers: disable warnings.
    f'-isystem {src_root}/include',
    f'-isystem {src_root}/vendor',
    f'-isystem {src_root}/vendor/antlr-cxx-runtime',
]

plugin_warnings = [
    '-Wno-global-constructors',
    '-Wno-exit-time-destructors',
]

if args.debug:
    cxxflags += ['-g', '-ggdb', '-O0']
    ldflags += ['-g', '-ggdb']

else:
    cxxflags += ['-D NDEBUG', '-O2']

gencxxflags = cxxflags + ['-Wno-deprecated-register']
cxxflags = cxxflags + warnings

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
    # paths
    'path': os.pathsep.join([builddir, os.environ['PATH']]),

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
if args.cxxflags:
    bootstrap_args += [f'--cxxflags="{args.cxxflags}"']
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
    flags = ' '.join(cxxflags + plugin_warnings)
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
