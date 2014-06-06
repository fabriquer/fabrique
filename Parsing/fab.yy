%{

#include "ADT/PtrVec.h"
#include "AST/ast.h"

#include "Parsing/Parser.h"
#include "Parsing/Token.h"
#include "Parsing/lex.h"
#include "Parsing/fab.yacc.h"
#include "Parsing/yacc.h"

#include "Types/FunctionType.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;
using namespace std;

using std::unique_ptr;


//
// Setters and getters for the YYSTYPE union:
//
#define SetOrDie(...) \
	do { \
		if (not Parser::Set(__VA_ARGS__)) \
			return -1; \
	} while (0)


static void CreateNodeList(YYSTYPE& yyunion)
{
	yyunion.nodes = new UniqPtrVec<Node>;
}

static void Append(YYSTYPE& yyunion, UniqPtr<Node>&& e)
{
	assert(yyunion.nodes);
	assert(e && "NULL AST node!");

	yyunion.nodes->push_back(std::move(e));
}


//
// Getters for said YYSTYPE union:
//
template<class T>
static UniqPtr<T> TakeNode(YYSTYPE& yyunion)
{
	return UniqPtr<T>(dynamic_cast<T*>(yyunion.node));
}

template<class T>
static UniqPtrVec<T>* NodeVec(YYSTYPE& yyunion)
{
	assert(yyunion.nodes);

	unique_ptr<UniqPtrVec<Node>> ownership(yyunion.nodes);
	UniqPtrVec<Node>& generic = *ownership;
	const size_t len = generic.size();

	UniqPtrVec<T> *result = new UniqPtrVec<T>(len);
	UniqPtrVec<T>& vec = *result;

	for (size_t i = 0; i < len; i++)
	{
		assert(generic[i]);

		unique_ptr<Node> n = std::move(generic[i]);
		unique_ptr<T> val(dynamic_cast<T*>(n.release()));

		assert(val);
		assert(not vec[i]);

		vec[i].swap(val);
	}

	return result;
}


#define YYPARSE_PARAM_TYPE fabrique::ast::Parser*
#define YYPARSE_PARAM p

%}

%pure-parser

/** Yacc representation of 'a thing that we've lexed and/or parsed'. */
%union {
	int intVal;
	fabrique::Token *token;

	const fabrique::Type *type;
	fabrique::PtrVec<fabrique::Type> *types;

	fabrique::ast::Node *node;
	fabrique::UniqPtrVec<fabrique::ast::Node> *nodes;
};

%token WHITESPACE
%token IDENTIFIER FILENAME
%token IF ELSE FOREACH AS
%token ACTION FILE_TOKEN FILES FUNCTION IMPORT RETURN
%token OPERATOR
%token INPUT PRODUCES
%token TRUE FALSE
%token NOT
%token STRING_LITERAL INT_LITERAL

%left AND OR XOR
%left ADD SCALAR_ADD
%right PREFIX

%%

fabfile:
	values
	;

expression:
	literal
	| action
	| binaryOperation
	| call
	| compoundExpr
	| conditional
	| '(' expression ')'	{ SetOrDie($$, $2.node); }
	| file
	| fileList
	| foreach
	| function
	| import
	| '[' listElements ']'
	{
		SourceRange src(*Take($1.token), *Take($3.token));
		auto elements = Take(NodeVec<Expression>($2));

		SetOrDie($$, p->ListOf(*elements, src));
	}
	| reference
	| unaryOperation
	;

action:
	actionBegin '(' argumentList ')'
	{
		SourceRange begin = Take(Parser::ParseToken($1))->source();
		auto args = Take(NodeVec<Argument>($3));
		SourceRange end = Take(Parser::ParseToken($4))->source();

		SetOrDie($$, p->DefineAction(args, SourceRange(begin, end)));
	}
	| actionBegin '(' argumentList INPUT parameterList ')'
	{
		SourceRange begin = Take(Parser::ParseToken($1))->source();
		auto args = Take(NodeVec<Argument>($3));
		auto params = Take(NodeVec<Parameter>($5));

		SetOrDie($$, p->DefineAction(args, begin, std::move(params)));
	}
	;

actionBegin:
	ACTION
	{
		p->EnterScope("action");
	}
	;

/**
 * A positional or keyword argument.
 */
argument:
	expression
	{
		auto value = TakeNode<Expression>($1);
		SetOrDie($$, p->Arg(value));
	}
	| identifier '=' expression
	{
		auto name = TakeNode<Identifier>($1);
		auto value = TakeNode<Expression>($3);

		SetOrDie($$, p->Arg(value, std::move(name)));
	}
	;

argumentList:
	/* empty */
	{
		CreateNodeList($$);
	}
	| argument
	{
		CreateNodeList($$);
		Append($$, TakeNode<Argument>($1));
	}
	| argumentList ',' argument
	{
		$$.nodes = $1.nodes;
		Append($$, TakeNode<Argument>($3));
	}
	;

binaryOperation:
	expression binaryOperator expression
	{
		UniqPtr<Expression> lhs = TakeNode<Expression>($1);
		UniqPtr<Expression> rhs = TakeNode<Expression>($3);
		auto op = static_cast<BinaryOperation::Operator>($2.intVal);

		SetOrDie($$, p->BinaryOp(op, lhs, rhs));
	}
	;

binaryOperator:
	ADD			{ $$.intVal = BinaryOperation::Add; }
	| PREFIX		{ $$.intVal = BinaryOperation::Prefix; }
	| SCALAR_ADD		{ $$.intVal = BinaryOperation::ScalarAdd; }
	| AND			{ $$.intVal = BinaryOperation::And; }
	| OR			{ $$.intVal = BinaryOperation::Or; }
	| XOR			{ $$.intVal = BinaryOperation::Xor; }
	;

call:
	reference '(' argumentList ')'
	{
		auto target = TakeNode<SymbolReference>($1);
		auto arguments = Take(NodeVec<Argument>($3));
		auto end = Take($4.token);

		SetOrDie($$, p->CreateCall(target, arguments, end->source()));
	}
	;

compoundExpr:
	compoundBegin values expression '}'
	{
		SourceRange begin = Take(Parser::ParseToken($1))->source();
		// NOTE: values have already been added to the scope by Parser
		auto result = TakeNode<Expression>($3);
		SourceRange end = Take(Parser::ParseToken($4))->source();

		SetOrDie($$, p->CompoundExpr(result, begin, end));
	}
	;

compoundBegin:
	'{'
	{
		p->EnterScope("compound expression");
		$$.token = $1.token;
	}
	;

conditional:
	IF expression expression ELSE expression
	{
		SourceRange begin = Take(Parser::ParseToken($1))->source();
		auto condition = TakeNode<Expression>($2);
		auto then = TakeNode<Expression>($3);
		auto elseClause = TakeNode<Expression>($5);

		SetOrDie($$, p->IfElse(begin, condition, then, elseClause));
	}
	;

fieldAccess:
	reference '.' name
	{
		auto structure = TakeNode<SymbolReference>($1);
		auto field = TakeNode<Identifier>($3);

		SetOrDie($$, p->FieldAccess(structure, field));
	}
	;

file:
	FILE_TOKEN '(' expression argumentList ')'
	{
		SourceRange begin = Take(Parser::ParseToken($1))->source();
		auto name = TakeNode<Expression>($3);
		auto arguments = Take(NodeVec<Argument>($4));

		SetOrDie($$, p->File(name, begin, std::move(arguments)));
	}
	;

fileList:
	FILES '(' files ',' argumentList ')'
	{
		auto begin = Take($1.token);
		auto files = Take(NodeVec<Filename>($3));
		auto arguments = NodeVec<Argument>($5);

		SetOrDie($$, p->Files(begin->source(), files, Take(arguments)));
	}
	| FILES '(' files ')'
	{
		auto begin = Take($1.token);
		auto files = Take(NodeVec<Filename>($3));

		SetOrDie($$, p->Files(begin->source(), files));
	}
	;

files:
	fileInList
	{
		CreateNodeList($$);
		Append($$, TakeNode<Filename>($1));
	}
	| files fileInList
	{
		$$.nodes = $1.nodes;
		Append($$, TakeNode<Filename>($2));
	}
	;

/*
 * The files() production has some special syntax to make fabfiles a bit
 * nicer to write. You can write 'files(file("foo.c") bar.c Makefile)',
 * which contains 1) a file production, 2) a literal filename and
 * 3) a literal filename that would in other circumstances be interpreted as
 * a legal identifier.
 */
fileInList:
	file
	| FILENAME
	{
		UniqPtr<Expression> name(p->ParseString(Take($1.token)));
		SetOrDie($$, p->File(name, name->source()));
	}
	;

foreach:
	foreachbegin mapping expression
	{
		SourceRange begin = Take(Parser::ParseToken($1))->source();
		auto mapping = TakeNode<Mapping>($2);
		auto body = TakeNode<Expression>($3);

		SetOrDie($$, p->Foreach(mapping, body, begin));
	}
	;

foreachbegin:
	FOREACH			{ p->EnterScope("foreach"); }
	;

function:
	functiondecl '(' parameterList ')' ':' type expression
	{
		SourceRange begin = Take(Parser::ParseToken($1))->source();
		auto params = Take(NodeVec<Parameter>($3));
		auto *retTy = $6.type;
		auto body = TakeNode<Expression>($7);

		SetOrDie($$, p->DefineFunction(begin, params, body, retTy));
	}
	|
	functiondecl '(' parameterList ')' expression
	{
		SourceRange begin = Take(Parser::ParseToken($1))->source();
		auto params = Take(NodeVec<Parameter>($3));
		auto body = TakeNode<Expression>($5);

		SetOrDie($$, p->DefineFunction(begin, params, body));
	}
	;

functiondecl:
	FUNCTION		{ p->EnterScope("function"); }
	;

identifier:
	name			/* keep result of 'name' production */
	| name ':' type		{ SetOrDie($$, p->Id(TakeNode<Identifier>($1), $3.type)); }
	;

import:
	IMPORT '(' literal ')'
	{
		auto begin = Take($1.token);
		auto name = TakeNode<StringLiteral>($3);
		auto end = Take($4.token);

		SetOrDie($$, p->ImportModule(name, SourceRange::Over(begin, end)));
	}
	;

listElements:
	/* empty */
	{
		CreateNodeList($$);
	}
	| listElements expression
	{
		$$.nodes = $1.nodes;
		Append($$, TakeNode<Expression>($2));
	}

literal:
	TRUE			{ SetOrDie($$, p->True()); }
	| FALSE			{ SetOrDie($$, p->False()); }
	| INT_LITERAL		{ SetOrDie($$, p->ParseInt($1.intVal)); }
	| STRING_LITERAL	{ SetOrDie($$, p->ParseString(Take($1.token))); }
	;

mapping:
	identifier INPUT expression
	{
		auto target = TakeNode<Identifier>($1);
		auto source = TakeNode<Expression>($3);

		SetOrDie($$, p->Map(source, target));
	}
	;

name:
	IDENTIFIER		{ SetOrDie($$, p->Id(Take($1.token))); }
	;

parameter:
	identifier
	{
		SetOrDie($$, p->Param(TakeNode<Identifier>($1)));
	}
	| identifier '=' expression
	{
		SetOrDie($$, p->Param(TakeNode<Identifier>($1), TakeNode<Expression>($3)));
	}
	;

parameterList:
	/* empty */
	{
		CreateNodeList($$);
	}
	| parameter
	{
		CreateNodeList($$);
		Append($$, TakeNode<Parameter>($1));
	}
	| parameterList ',' parameter
	{
		$$.nodes = $1.nodes;
		Append($$, TakeNode<Parameter>($3));
	}
	;

reference:
	fieldAccess
	{
		SetOrDie($$, p->Reference(TakeNode<FieldAccess>($1)));
	}
	|
	identifier
	{
		SetOrDie($$, p->Reference(TakeNode<Identifier>($1)));
	}
	;

/*
 * 'file' and 'file[in]' are valid types, so we need to do provide
 * alternate handling paths here.
 *
 * TODO: add typeName production
 */
type:
	name
	{
		$$.type = &p->getType(TakeNode<Identifier>($1));
	}
	| name '[' types ']'
	{
		auto *subtypes = $3.types;
		$$.type = &p->getType(TakeNode<Identifier>($1), Take(subtypes));
	}
	| FILE_TOKEN
	{
		SourceRange src = Take($1.token)->source();
		$$.type = &p->getType("file", src, src);
	}
	| FILE_TOKEN '[' types ']'
	{
		SourceRange begin = Take($1.token)->source();
		SourceRange end = Take($4.token)->source();

		$$.type = &p->getType("file", begin, end, *$3.types);
	}
	| '(' types ')' PRODUCES type
	{
		SourceRange begin = Take($1.token)->source();
		auto argTypes(Take($2.types));
		SourceRange end = Take($3.token)->source();
		const Type *returnType = $5.type;

		SourceRange src(begin, end);

		$$.type = &p->FnType(*argTypes, *returnType, src);
	}
	;

types:
	/* empty */
	{
		$$.types = new PtrVec<Type>();
	}
	|
	type
	{
		$$.types = new PtrVec<Type>(1, $1.type);
	}
	| types ',' type
	{
		$$.types = $1.types;
		$$.types->push_back($3.type);
	}

unaryOperation:
	NOT expression
	{
		auto opsrc = Take($1.token)->source();
		auto e = TakeNode<Expression>($2);
		UnaryOperation::Operator op = ast::UnaryOperation::Negate;

		SetOrDie($$, p->UnaryOp(op, opsrc, e));
	}
	;

values:
	/* empty */
	| values value
	;

value:
	identifier '=' expression ';'	{
		auto name = TakeNode<Identifier>($1);
		auto initialiser = TakeNode<Expression>($3);

		if (not p->DefineValue(name, initialiser))
			return -1;
	}
	;
%%
