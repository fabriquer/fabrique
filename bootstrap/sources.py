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

import itertools

#
# Explicitly declare all of the source files that are part of Fabrique.
#
# This is verbose, but that's the price of a lowest-common-denominator
# bootstrapping system that doesn't rely on a high-level build system!
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

def extension(subdir):
    """Does a subdirectory use the .cpp or .cc suffix for its files?"""
    return 'cpp' if subdir.startswith('vendor') else 'cc'


cxx_srcs = list(itertools.chain(*[
    [f'{subdir}{base}.{extension(subdir)}' for base in srcs]
    for (subdir, srcs) in cxx_srcs.items()
]))
