%{

#include "ADT/PtrVec.h"
#include "AST/ast.h"

#include "Parsing/Parser.h"
#include "Parsing/Token.h"
#include "Parsing/lex.h"
#include "Parsing/fab.yacc.h"
#include "Parsing/yacc.h"

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


static void Append(YYSTYPE& yyunion, UniqPtr<Expression>&& e)
{
	assert(yyunion.exprs);
	assert(e && "NULL expression!");

	yyunion.exprs->push_back(std::move(e));
}


//
// Getters for said YYSTYPE union:
//
template<class T>
static UniqPtr<T> Expr(YYSTYPE& yyunion)
{
	return UniqPtr<T>(dynamic_cast<T*>(yyunion.expr));
}

template<class T>
static UniqPtrVec<T>* ExprVec(YYSTYPE& yyunion)
{
	assert(yyunion.exprs);

	unique_ptr<UniqPtrVec<Expression>> ownership(yyunion.exprs);
	UniqPtrVec<Expression>& generic = *ownership;
	const size_t len = generic.size();

	UniqPtrVec<T> *result = new UniqPtrVec<T>(len);
	UniqPtrVec<T>& vec = *result;

	for (size_t i = 0; i < len; i++)
	{
		assert(generic[i]);

		unique_ptr<Expression> e = std::move(generic[i]);
		unique_ptr<T> val(dynamic_cast<T*>(e.release()));

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

/**
 * Since we're using C++, we don't have to use hopefully-typed unions:
 * we can use run-time type information!
 */
%union {
	int intVal;

	fabrique::ast::Identifier *id;
	fabrique::ast::Expression *expr;
	fabrique::UniqPtrVec<fabrique::ast::Expression> *exprs;

	const fabrique::Type *type;
	fabrique::PtrVec<fabrique::Type> *types;

	//fabrique::SourceRange *src;
	fabrique::Token *token;
};

%token WHITESPACE
%token IDENTIFIER FILENAME
%token IF ELSE FOREACH AS
%token ACTION FILE_TOKEN FILES FUNCTION RETURN
%token OPERATOR
%token INPUT
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
	| conditional
	| '(' expression ')'	{ SetOrDie($$, $2.expr); }
	| file
	| fileList
	| foreach
	| function
	| identifier		{ SetOrDie($$, p->Reference(Take($1.id))); }
	| '[' listElements ']'
	{
		SourceRange src(*Take($1.token), *Take($3.token));
		SetOrDie($$, p->ListOf(Take($2.exprs), src));
	}
	| unaryOperation
	;

action:
	ACTION '(' argumentList ')'
	{
		SourceRange begin = Take(Parser::Token($1))->source();
		auto args = Take(ExprVec<Argument>($3));
		SourceRange end = Take(Parser::Token($4))->source();

		SetOrDie($$, p->DefineAction(args, SourceRange(begin, end)));
	}
	| ACTION '(' argumentList INPUT parameterList ')'
	{
		SourceRange begin = Take(Parser::Token($1))->source();
		auto args = Take(ExprVec<Argument>($3));
		auto params = Take(ExprVec<Parameter>($5));

		SetOrDie($$, p->DefineAction(args, begin, std::move(params)));
	}
	;

/**
 * A positional or keyword argument.
 */
argument:
	expression
	{
		auto value = Expr<Expression>($1);
		SetOrDie($$, p->Arg(value));
	}
	| identifier '=' expression
	{
		auto name = $1.id;
		auto value = Expr<Expression>($3);

		SetOrDie($$, p->Arg(value, Take(name)));
	}
	;

argumentList:
	/* empty */
	{
		$$.exprs = new UniqPtrVec<Expression>;
	}
	| argument
	{
		$$.exprs = new UniqPtrVec<Expression>;
		Append($$, Expr<Argument>($1));
	}
	| argumentList ',' argument
	{
		$$.exprs = $1.exprs;
		Append($$, Expr<Argument>($3));
	}
	;

binaryOperation:
	expression binaryOperator expression
	{
		UniqPtr<Expression> lhs = Expr<Expression>($1);
		UniqPtr<Expression> rhs = Expr<Expression>($3);
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
	identifier '(' argumentList ')'
	{
		auto fnName = Take($1.id);
		auto arguments = Take(ExprVec<Argument>($3));

		SetOrDie($$, p->CreateCall(fnName, arguments));
	}
	;

compoundExpr:
	compoundBegin compoundBody
	{
		SetOrDie($$, Expr<CompoundExpression>($2).release());
	}
	;

compoundBegin:
	{
		p->EnterScope("compound expression");
	}
	;

compoundBody:
	expression
	{
		auto singleton = Expr<Expression>($1);
		SetOrDie($$, p->CompoundExpr(singleton));
	}
	| '{' expression '}'
	{
		SourceRange begin = Take(Parser::Token($1))->source();
		auto result = Expr<Expression>($2);
		SourceRange end = Take(Parser::Token($3))->source();

		SetOrDie($$, p->CompoundExpr(result, begin, end));
	}
	| '{' values expression '}'
	{
		SourceRange begin = Take(Parser::Token($1))->source();
		// NOTE: values have already been added to the scope by Parser
		auto result = Expr<Expression>($3);
		SourceRange end = Take(Parser::Token($4))->source();

		SetOrDie($$, p->CompoundExpr(result, begin, end));
	}
	;

conditional:
	IF expression compoundExpr ELSE compoundExpr
	{
		SourceRange begin = Take(Parser::Token($1))->source();
		auto condition = Expr<Expression>($2);
		auto then = Expr<CompoundExpression>($3);
		auto elseClause = Expr<CompoundExpression>($5);

		SetOrDie($$, p->IfElse(begin, condition, then, elseClause));
	}
	;

file:
	FILE_TOKEN '(' expression argumentList ')'
	{
		SourceRange begin = Take(Parser::Token($1))->source();
		auto name = Expr<Expression>($3);
		auto arguments = Take(ExprVec<Argument>($4));

		SetOrDie($$, p->File(name, begin, std::move(arguments)));
	}
	;

fileList:
	FILES '(' files ',' argumentList ')'
	{
		auto begin = Take($1.token);
		auto files = Take(ExprVec<Filename>($3));
		auto arguments = ExprVec<Argument>($5);

		SetOrDie($$, p->Files(begin->source(), files, Take(arguments)));
	}
	| FILES '(' files ')'
	{
		auto begin = Take($1.token);
		auto files = Take(ExprVec<Filename>($3));

		SetOrDie($$, p->Files(begin->source(), files));
	}
	;

files:
	fileInList
	{
		$$.exprs = new UniqPtrVec<Expression>;
		Append($$, Expr<Filename>($1));
	}
	| files fileInList
	{
		$$.exprs = $1.exprs;
		Append($$, Expr<Filename>($2));
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
	| IDENTIFIER
	{
		UniqPtr<Expression> name(p->ParseString(Take($1.token)));
		SetOrDie($$,p->File(name, name->source()));
	}
	;

foreach:
	foreachbegin expression AS parameter compoundExpr
	{
		SourceRange begin = Take(Parser::Token($1))->source();
		auto seq = Expr<Expression>($2);
		auto loopParameter = Expr<Parameter>($4);
		auto body = Expr<CompoundExpression>($5);

		SetOrDie($$, p->Foreach(seq, loopParameter, body, begin));
	}
	;

foreachbegin:
	FOREACH			{ p->EnterScope("foreach"); }
	;

function:
	functiondecl '(' parameterList ')' ':' type compoundExpr
	{
		SourceRange begin = Take(Parser::Token($1))->source();
		auto params = Take(ExprVec<Parameter>($3));
		auto *retTy = $6.type;
		auto body = Expr<CompoundExpression>($7);

		SetOrDie($$, p->DefineFunction(begin, params, body, retTy));
	}
	;

functiondecl:
	FUNCTION		{ p->EnterScope("function"); }
	;

identifier:
	name			/* keep result of 'name' production */
	| name ':' type		{ SetOrDie($$, p->Id(Take($1.id), $3.type)); }
	;

listElements:
	/* empty */
	{
		$$.exprs = new UniqPtrVec<Expression>;
	}
	| listElements expression
	{
		$$.exprs = $1.exprs;
		Append($$, Expr<Expression>($2));
	}

literal:
	TRUE			{ SetOrDie($$, p->True()); }
	| FALSE			{ SetOrDie($$, p->False()); }
	| INT_LITERAL		{ SetOrDie($$, p->ParseInt($1.intVal)); }
	| STRING_LITERAL	{ SetOrDie($$, p->ParseString(Take($1.token))); }
	;

name:
	IDENTIFIER		{ SetOrDie($$, p->Id(Take($1.token))); }
	;

parameter:
	identifier
	{
		SetOrDie($$, p->Param(Take($1.id)));
	}
	| identifier '=' expression
	{
		SetOrDie($$, p->Param(Take($1.id), Expr<Expression>($3)));
	}
	;

parameterList:
	/* empty */
	{
		$$.exprs = new UniqPtrVec<Expression>;
	}
	| parameter
	{
		$$.exprs = new UniqPtrVec<Expression>;
		Append($$, Expr<Parameter>($1));
	}
	| parameterList ',' parameter
	{
		$$.exprs = $1.exprs;
		Append($$, Expr<Parameter>($3));
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
		$$.type = p->getType(Take($1.id));
	}
	| name '[' types ']'
	{
		auto *subtypes = $3.types;
		$$.type = p->getType(Take($1.id), Take(subtypes));
	}
	| FILE_TOKEN
	{
		$$.type = p->getType("file");
	}
	| FILE_TOKEN '[' types ']'
	{
		$$.type = p->getType("file", *$3.types);
	}
	;

types:
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
		auto e = Expr<Expression>($2);
		UnaryOperation::Operator op = ast::UnaryOperation::Negate;

		SetOrDie($$, p->UnaryOp(op, opsrc, e));
	}
	;

values:
	value
	| values value
	;

value:
	identifier '=' expression ';'	{
		auto name = Take($1.id);
		auto initialiser = Expr<Expression>($3);

		if (not p->DefineValue(name, initialiser))
			return -1;
	}
	;
%%
