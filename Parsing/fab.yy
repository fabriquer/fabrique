%{

#include "ADT/PtrVec.h"
#include "AST/ast.h"

#include "Parsing/Parser.h"
#include "Parsing/lex.h"
#include "Parsing/yacc.h"

using namespace std;

template<class T>
void CreateList(PtrVec<T>*& target, const T *firstElement = NULL)
{
	target = new PtrVec<T>;
	if (firstElement != NULL)
		target->push_back(firstElement);
}

template<class T>
void Append(PtrVec<T>*& target, PtrVec<T>* source, const T *nextElement)
{
	if (source == NULL)
	{
		target = NULL;
		return;
	}

	if (nextElement == NULL)
	{
		target = NULL;
		delete source;
		return;
	}

	target = source;
	target->push_back(nextElement);
}

#define YYPARSE_PARAM_TYPE Parser*
#define YYPARSE_PARAM p

%}

%pure-parser

%union {
	int i;
	CStringRef s;
	std::string *str;
	SourceRange *src;

	Identifier *id;

	Expression *expr;
	PtrVec<Expression> *exprs;

	const Type *ty;
	PtrVec<Type> *types;

	const Argument *arg;
	PtrVec<Argument> *args;

	const Parameter *param;
	PtrVec<Parameter> *params;

	const File *file;
	PtrVec<File> *files;

	BinaryOperation::Operator op;

	Value *val;
	PtrVec<Value> *values;
};

%token WHITESPACE
%token IDENTIFIER FILENAME
%token IF ELSE
%token ACTION FILE FILES FUNCTION RETURN
%token OPERATOR
%token TRUE FALSE
%token CONCAT PREFIX SCALAR_ADD
%token STRING_LITERAL INT_LITERAL

%left CONCAT SCALAR_ADD
%right PREFIX

%%

fabfile:
	values			{ p->SetRoot($1.values); }
	;

values:
	value			{ CreateList($$.values, $1.val); }
	| values value		{ Append($$.values, $1.values, $2.val); }
	;

value:
	identifier '=' expr ';'	{
		$$.val = p->Define($1.id, $3.expr);
		if ($$.val == NULL) return -1;
	}
	;

expr:
	expression		{ if ($$.expr == NULL) return -1; }
	;

expression:
	literal
	| action
	| call
	| conditional
	| fileList
	| function
	| identifier		{ $$.expr = p->Reference($1.id); }
	| '(' expr ')'		{ $$.expr = $2.expr; }
	| '[' exprlist ']'	{ $$.expr = p->ListOf($2.exprs); }
	| expr CONCAT expr	{ $$.expr = p->Concat($1.expr, $3.expr); }
	| expr PREFIX expr	{ $$.expr = p->Prefix($1.expr, $3.expr); }
	| expr SCALAR_ADD expr	{ $$.expr = p->ScalarAdd($1.expr, $3.expr); }
	;

literal:
	TRUE			{ $$.expr = p->True(); }
	| FALSE			{ $$.expr = p->False(); }
	| INT_LITERAL		{ $$.expr = p->ParseInt($1.i); }
	| STRING_LITERAL	{ $$.expr = p->ParseString($1.s); }
	;

action:
	ACTION '(' args ')'	{ $$.expr = p->DefineAction($3.args, $1.src); }
	;

call:
	identifier '(' args ')' { $$.expr = p->CreateCall($1.id, $3.args); }
	;

conditional:
	IF expr expr ELSE expr	{
		$$.expr = p->IfElse($1.src, $2.expr, $3.expr, $5.expr);
	}
	;

file:
	FILE '(' expr args ')'	{
		$$.expr = p->Source($3.expr, $1.src, $4.args);
	}
	;

fileList:
	FILES '(' files ',' args ')' { $$.expr = p->Files($3.files, $5.args); }
	| FILES '(' files ')'	{ $$.expr = p->Files($3.files); }
	;

files:
	fileInList		{ CreateList($$.files, $1.file); }
	| files fileInList	{ Append($$.files, $1.files, $2.file); }
	;

fileInList:
	file
	| FILENAME		{
		$$.expr = p->Source(p->ParseString($1.s),
		                    p->CurrentTokenRange());
	}
	| IDENTIFIER		{
		$$.expr = p->Source(p->ParseString($1.s),
		                    p->CurrentTokenRange());
	}
	;

function:
	fndecl '(' params ')' ':' type '{' values RETURN expr ';' '}'	{
		$$.expr = p->DefineFunction($3.params, $6.ty,
		                            $8.values, $10.expr);
	}
	;

fndecl:
	FUNCTION		{ p->EnterScope(); p->SaveLoc(); }
	;

identifier:
	name			/* keep id parsed in the 'name' production */
	| name ':' type		{ p->SaveLoc(); $$.id = p->Id($1.id, $3.ty); }
	;

name:
	IDENTIFIER		{ $$.id = p->Id($1.s); }
	;

type:
	name			{ $$.ty = p->TakeType($1.id); }
	| FILE			{ $$.ty = p->getType("file"); }
	| name '[' types ']'	{ $$.ty = p->TakeType($1.id, $3.types); }
	;

types:
	type			{ CreateList($$.types, $1.ty); p->SaveLoc(); }
	| types ',' type	{ Append($$.types, $1.types, $3.ty); }

exprlist:
	expr			{ p->SaveLoc(); CreateList($$.exprs, $1.expr); }
	| exprlist expr		{ Append($$.exprs, $1.exprs, $2.expr); }

args:
	/* empty */		{ CreateList($$.args); }
	| arg			{ CreateList($$.args, $1.arg); }
	| args ',' arg		{ Append($$.args, $1.args, $3.arg); }
	;

arg:
	identifier '=' expr	{ $$.arg = p->Arg($3.expr, $1.id); }
	| expr			{ $$.arg = p->Arg($1.expr); }
	;

params:
	/* empty */		{ CreateList($$.params); }
	| parameter		{ CreateList($$.params, $1.param); }
	| params ',' parameter	{ Append($$.params, $1.params, $3.param); }
	;

parameter:
	identifier		{ $$.param = p->Param($1.id); }
	;

%%
