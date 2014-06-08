"
" Vim syntax file for Fabrique.
"

if exists("b:current_syntax")
  finish
endif


"
" Identifiers:
"
syn match	fabValue	display '^\<[_a-zA-Z][_a-zA-Z0-9]*\>' nextgroup=fabAssign skipwhite
hi def link	fabValue	Identifier


"
" Keywords:
"

syn keyword	fabType		bool int string list file
hi def link	fabType		Type

syn keyword	fabBuiltin	action buildroot files function import srcroot subdir
hi def link	fabBuiltin	Function

syn keyword	fabInOut	in out
hi def link	fabInOut	Label

syn keyword	fabKeyword	return
hi def link	fabKeyword	Keyword

syn keyword	fabConditional	if else
hi def link	fabConditional	Conditional
syn keyword	fabRepeat	foreach as
hi def link	fabRepeat	Repeat

syn match	fabAdd		'+'
syn match	fabAnd		'\<and\>'
syn match	fabAssign	'='
syn match	fabColon	':'
syn match	fabInput	'<-'
syn match	fabNot		'\<not\>'
syn match	fabOr		'\<or\>'
syn match	fabParen	'[\[\]{}()]'
syn match	fabPrefix	'::'
syn match	fabProduces	'=>'
syn match	fabScalarAdd	'.+'
syn match	fabSemicolon	';'
syn match	fabXor		'\<xor\>'
hi def link	fabAdd		Operator
hi def link	fabAnd		Operator
hi def link	fabAssign	Operator
hi def link	fabColon	Operator
hi def link	fabInput	Operator
hi def link	fabNot		Operator
hi def link	fabOr		Operator
hi def link	fabParen	Operator
hi def link	fabPrefix	Operator
hi def link	fabProduces	Operator
hi def link	fabScalarAdd	Operator
hi def link	fabSemicolon	Operator
hi def link	fabXor		Operator


"
" Comments:
"
syn match	fabComment	"#.*$" display contains=fabTodo
hi def link	fabComment	Comment

syn keyword	fabTodo		TODO FIXME XXX contained
hi def link	fabTodo		Todo


"
" Strings:
"
syn region	fabString	start=+[bB]\='+ skip=+\\\\\|\\'\|\\$+ excludenl end=+'+ end=+$+ keepend
syn region	fabString	start=+[bB]\="+ skip=+\\\\\|\\"\|\\$+ excludenl end=+"+ end=+$+ keepend
syn region	fabString	start=+[bB]\="""+ end=+"""+ keepend
syn region	fabString	start=+[bB]\='''+ end=+'''+ keepend
hi def link	fabString	String

syn match	fabStrVar	"${.*}" contained containedin=fabString
hi def link	fabStrVar	Special


"
" Other literals:
"
syn match	fabBool		'\<true\>'
syn match	fabBool		'\<false\>'
hi def link	fabBool		Boolean

syn match	fabNumber	'\<[0-9]\+\>'
hi def link	fabNumber	Number

let b:current_syntax = "fab"
