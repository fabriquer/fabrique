from pygments.lexer import RegexLexer, bygroups
from pygments.token import *

__all__ = [ 'FabLexer' ]

class FabLexer(RegexLexer):
	name = 'Fabriquer'
	aliases = [ 'fab', 'fabriquer' ]
	filenames = [ '*.fab', 'fabfile' ]

	tokens = {
		'root': [
			(r'#.*\n', Comment.Single),

			(r"'", String, 'singlestring'),
			(r'"', String, 'doublestring'),

			(r'(.*?)(\s*)(=)(\s*)(function)',
				bygroups(Name.Function, Whitespace, Operator,
				         Whitespace, Name.Builtin)),

			(r'action', Name.Builtin),
			(r'buildroot', Name.Builtin),
			(r'files', Name.Builtin),
			(r'function', Name.Builtin),
			(r'srcroot', Name.Builtin),

			(r'bool', Keyword.Type),
			(r'file', Keyword.Type),
			(r'int', Keyword.Type),
			(r'list', Keyword.Type),
			(r'string', Keyword.Type),

			(r'in', Keyword.Pseudo),
			(r'out', Keyword.Pseudo),

			(r'if', Keyword),
			(r'else', Keyword),
			(r'foreach', Keyword),
			(r'as', Keyword),

			(r'([_a-zA-Z]\w*)(\s*)(=)',
				bygroups(Name.Constant, Whitespace, Operator)),

			(r'([_a-zA-Z]\w*)(\s*)(:)(\s*)(\w*)(\s*)(=)',
				bygroups(Name.Constant,
					Whitespace,
					Punctuation,
					Whitespace,
					Keyword.Type,
					Whitespace,
					Operator)),

			(r'([_a-zA-Z]\w*)',
				bygroups(Name.Variable)),

			(r'([_a-zA-Z]\w*)(\s*)(:)(\s*)(\w*)',
				bygroups(Name.Constant,
					Whitespace,
					Punctuation,
					Whitespace,
					Keyword.Type)),

			(r'\+', Operator),
			(r'\.\+', Operator),
			(r'<-', Operator),
			(r'=>', Operator),
			(r'[=:]', Operator),
			(r'and', Operator.Word),
			(r'or', Operator.Word),
			(r'xor', Operator.Word),

			(r'[\.,(){}\[\];]', Punctuation),

			(r'[\s+]', Whitespace),
		],

		'singlestring': [
			(r"'", String, '#pop'),
			(r"[^']", String),
		],

		'doublestring': [
			(r'"', String, '#pop'),
			(r'[^"]', String),
		],
	}




#syn match	fabValue	display '^\<[_a-zA-Z][_a-zA-Z0-9]\+\>' nextgroup=fabAssign skipwhite
#hi def link	fabValue	Identifier
#
#
#syn keyword	fabBuiltin	action files function
#hi def link	fabBuiltin	Function
#
#syn keyword	fabInOut	in out
#hi def link	fabInOut	Label
#
#syn keyword	fabKeyword	return
#hi def link	fabKeyword	Keyword
#
#syn keyword	fabConditional	if else
#hi def link	fabConditional	Conditional
#syn keyword	fabRepeat	foreach as
#hi def link	fabRepeat	Repeat
#
#syn match	fabAdd		'+'
#syn match	fabAnd		'\<and\>'
#syn match	fabAssign	'='
#syn match	fabColon	':'
#syn match	fabInput	'<-'
#syn match	fabNot		'\<not\>'
#syn match	fabOr		'\<or\>'
#syn match	fabParen	'[\[\]{}()]'
#syn match	fabPrefix	'::'
#syn match	fabScalarAdd	'.+'
#syn match	fabSemicolon	';'
#syn match	fabXor		'\<xor\>'
#hi def link	fabAdd		Operator
#hi def link	fabAnd		Operator
#hi def link	fabAssign	Operator
#hi def link	fabColon	Operator
#hi def link	fabInput	Operator
#hi def link	fabNot		Operator
#hi def link	fabOr		Operator
#hi def link	fabParen	Operator
#hi def link	fabPrefix	Operator
#hi def link	fabScalarAdd	Operator
#hi def link	fabSemicolon	Operator
#hi def link	fabXor		Operator
#
#
#"
#" Comments:
#"
#syn match	fabComment	"#.*$" display contains=fabTodo
#hi def link	fabComment	Comment
#
#syn keyword	fabTodo		TODO FIXME XXX contained
#hi def link	fabTodo		Todo
#
#
#"
#" Strings:
#"
#syn region	fabString	start=+[bB]\='+ skip=+\\\\\|\\'\|\\$+ excludenl end=+'+ end=+$+ keepend
#syn region	fabString	start=+[bB]\="+ skip=+\\\\\|\\"\|\\$+ excludenl end=+"+ end=+$+ keepend
#syn region	fabString	start=+[bB]\="""+ end=+"""+ keepend
#syn region	fabString	start=+[bB]\='''+ end=+'''+ keepend
#hi def link	fabString	String
#
#syn match	fabStrVar	"${.*}" contained containedin=fabString
#hi def link	fabStrVar	Special
#
#
#"
#" Other literals:
#"
#syn match	fabBool		'\<true\>'
#syn match	fabBool		'\<false\>'
#hi def link	fabBool		Boolean
#
#syn match	fabNumber	'\<[0-9]\+\>'
#hi def link	fabNumber	Number
#
#let b:current_syntax = "fab"
##
