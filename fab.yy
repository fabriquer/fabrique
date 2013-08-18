%{

#include "Parser.h"

#include "lex.h"

#include "ast.h"
#include "grammar.h"

using namespace std;

void PrintUnion(void *y);

template<class T>
void CreateList(std::vector<const T*>*& target, const T *firstElement = NULL)
{
	target = new std::vector<const T*>;
	if (firstElement != NULL)
		target->push_back(firstElement);
}

template<class T>
void Append(std::vector<const T*>*& target, std::vector<const T*>* source,
	const T *nextElement)
{
	assert(source != NULL);
	assert(source->size() > 0);
	assert(nextElement != NULL);

	target = source;
	target->push_back(nextElement);
}

#define YYPARSE_PARAM_TYPE Parser*
#define YYPARSE_PARAM p

%}

%union {
	int i;
	CStringRef s;
	Identifier *id;

	Expression *expr;
	ExprList *exprs;

	Type *ty;

	const Argument *arg;
	std::vector<const Argument*> *args;

	const File *file;
	std::vector<const File*> *files;

	BinaryOperator::Operation op;
	Value *val;
};

%token IDENTIFIER FILENAME
%token ACTION FILE FILES FUNCTION
%token OPERATOR
%token TRUE FALSE
%token CONCAT PREFIX SCALAR_ADD
%token STRING_LITERAL INT_LITERAL

%left CONCAT SCALAR_ADD
%right PREFIX

%%

values:
	| values value		{ $2.val->PrettyPrint(std::cout); }
	;

value:
	identifier '=' expr ';'	{ $$.val = Value::take($1.id, $3.expr); }
	;

expr:
	literal
	| action
	| fileList
	| function
	| call
	| identifier		{ $$.expr = SymbolReference::Take($1.id); }
	| '(' expr ')'		{ $$.expr = $2.expr; }
	| '[' exprlist ']'	{ $$.expr = List::Take($2.exprs); }
	| expr CONCAT expr	{ $$.expr = BinaryOperator::Take(
	                                  $1.expr,
	                                  BinaryOperator::Concatenate,
	                                  $3.expr); }
	| expr PREFIX expr	{ $$.expr = BinaryOperator::Take(
	                                  $1.expr,
	                                  BinaryOperator::Prefix,
	                                  $3.expr); }
	| expr SCALAR_ADD expr	{ $$.expr = BinaryOperator::Take(
	                                  $1.expr,
	                                  BinaryOperator::ScalarAdd,
	                                  $3.expr); }
	;

literal:
	TRUE			{ $$.expr = BoolLiteral::True(); }
	| FALSE			{ $$.expr = BoolLiteral::False(); }
	| INT_LITERAL		{ $$.expr = new IntLiteral($1.i); }
	| STRING_LITERAL	{ $$.expr = new StringLiteral($1.s); }
	;

action:
	ACTION '(' params ')'	{ $$.expr = Action::Take($3.exprs); }
	;

fileList:
	FILES '(' files ',' params ')'	{
				$$.expr = FileList::Take($3.files, $5.args); }
	| FILES '(' files ')'	{ $$.expr = FileList::Take($3.files); }
	;

files:
	file			{ CreateList($$.files, $1.file); }
	| files file		{ Append($$.files, $1.files, $2.file); }
	;

file:
	FILE '(' filename ')'	{ $$.expr = File::Source($1.s.str()); }
	| filename		{ $$.expr = File::Source($1.s.str()); }
	;

filename:
	IDENTIFIER
	| FILENAME
	;

function:
	FUNCTION '(' params ')' ':' type '{' values expr '}'	{
				$$.expr = NULL;
	}
	;

call:
	identifier '(' params ')' { $$.expr = Call::Take($1.id, $3.args); }
	;

identifier:
	name
	| name ':' type		{ $$.id = Identifier::AddType($1.id, $3.ty); }
	;

name:
	IDENTIFIER		{ $$.id = Identifier::Create($1.s); }
	;

type:
	name			{ $$.ty = NULL; }
	| FILE			{ $$.ty = NULL; }
	| name '[' type ']'	{ $$.ty = NULL; }
	;

exprlist:
	expr			{ CreateList($$.exprs, $1.expr); }
	| exprlist expr		{ Append($$.exprs, $1.exprs, $2.expr); }

params:
	/* empty */		{ CreateList($$.args); }
	| parameter		{ CreateList($$.args, $1.arg); }
	| params ',' parameter	{ Append($$.args, $1.args, $3.arg); }
	;

parameter:
	identifier '=' expr	{ $$.expr = Argument::Take($1.id, $3.expr); }
	| expr			{ $$.expr = Argument::Take(NULL, $1.expr); }
	;

%%
