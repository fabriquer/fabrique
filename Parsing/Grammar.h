/** @file Parsing/Grammar.h    Declaration of the AST grammar. */
/*
 * Copyright (c) 2014-2015 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme, as well as at
 * Memorial University under the NSERC Discovery program (RGPIN-2015-06048).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "ADT/UniqPtr.h"
#include "Pegmatite/pegmatite.hh"

namespace fabrique {
namespace parser {

using pegmatite::Rule;
using pegmatite::operator""_E;
using pegmatite::operator""_R;
using pegmatite::operator""_S;
using pegmatite::ExprPtr;
using pegmatite::BindAST;
using pegmatite::any;
using pegmatite::nl;
using pegmatite::term;
using pegmatite::trace;

#define TRACE_RULE(name, expr) \
	const Rule name = trace(#name, expr)


struct Grammar
{
	//
	// Things that we ignore:
	//
	const Rule Newline = nl('\n');
	const Rule Whitespace = ' '_E | '\t' | Newline;
	const Rule Comment = '#'_E >> *(!Newline >> any()) >> Newline;
	const Rule Ignored = *(Comment | Whitespace);

	//
	// Terminals that need to be declared before parsing rules:
	//
	struct
	{
		const Rule True = term("true");
		const Rule False = term("false");

		const ExprPtr If = term("if");
		const ExprPtr Else = term("else");
		const ExprPtr Foreach = term("foreach");
		const ExprPtr As = term("as");
		const ExprPtr Action = term("action");
		const ExprPtr File = term("file");
		const ExprPtr Files = term("files");
		const ExprPtr Function = term("function");
		const ExprPtr Import = term("import");
		const ExprPtr Nil = term("nil");
		const ExprPtr Record = term("record");
		const ExprPtr Return = term("return");
		const ExprPtr Some = term("some");
	} Keywords;

	const Rule Alpha = ('A'_E - 'Z') | ('a'_E - 'z');
	const Rule Digit = '0'_E - '9';
	const Rule AlphaNum = Alpha | Digit;
	const Rule IdChar = AlphaNum | '_';

	/*
	 * Fabrique supports boolean, integer and string literals.
	 */
	const Rule BoolLiteral = Keywords.True | Keywords.False;
	const Rule IntLiteral = +Digit;

	const Rule SingleQuotedString = "'"_E >> *(!"'"_E >> any()) >> "'";
	const Rule DoubleQuotedString = "\""_E >> *(!"\""_E >> any()) >> "\"";
	const Rule StringLiteral = SingleQuotedString | DoubleQuotedString;

	TRACE_RULE(Literal, term(BoolLiteral | IntLiteral | StringLiteral));


	/*
	 * Types can be simple (e.g., `int`) or parametric (e.g., `list[int]`).
	 */
	TRACE_RULE(SimpleType, Identifier);
	TRACE_RULE(ParametricType, Type >> '[' >> Type >> *(','_E >> Type) >> ']');
	TRACE_RULE(Type, ParametricType | SimpleType);

#if 0
	| STRUCT '[' fieldTypes ']'
	{
		SourceRange begin = Take($1.token)->source();
		auto fields = Take(NodeVec<Identifier>($3));
		SourceRange end = Take($4.token)->source();

		$$.type = p->StructType(fields, SourceRange(begin, end));
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
#endif

	TRACE_RULE(Identifier, term((Alpha | '_') >> *IdChar));

	/*
	 * Almost everything in Fabrique is an Expression.
	 */
	TRACE_RULE(Expression,
		Operation
		/*
		| Conditional
		| File
		| FileList
		*/
	);

	TRACE_RULE(Term, Literal | ParentheticalExpression | List);

	const Rule ParentheticalExpression = term('('_E) >> Expression >> term(')'_E);

	TRACE_RULE(Conditional,
		   Keywords.If >> Expression >> Expression
		   >> Keywords.Else >> Expression);

#if 0
	/**
	 * Files have names and optional arguments.
	 *
	 * Example:
	 * `file('foo.c', cflags = [ '-D' 'FOO' ])`
	 */
	TRACE_RULE(File,
	     Keywords.File >> '('
	     >> Expression >> -(','_E >> NamedArguments)
	     >> ')');

	TRACE_RULE(Filename, term(+(IdChar | '.' | '/')));

	/*
	 * File lists can include raw filenames as well as embedded file declarations,
	 * optionally followed by arguments.
	 *
	 * Example:
	 * ```
	 * files(
	 *   foo.c
	 *   bar.c
	 *   file('baz.c', cflags = [])
	 *   ,
	 *   arg1 = 'hello', arg2 = 42
	 * )
	 * ```
	 */
	TRACE_RULE(FileListWithoutArgs, Keywords.Files >> '(' >> *Filename >> ')');
	TRACE_RULE(FileListWithArgs,
		Keywords.Files >> '(' >> *Filename >> ',' >> NamedArguments >> ')');
	TRACE_RULE(FileList, FileListWithArgs | FileListWithoutArgs);
#endif

	/*
	 * Lists are containers for like types and do not use comma separators.
	 * The element types do not have to be identical, but they do have to have
	 * a common supertype.
	 *
	 * Example:
	 * ```
	 * x:int = 42;
	 * y:special_int = some_special_kind_of_int();
	 *
	 * [ 1 2 3 x y ]   # the type of this is list[int]
	 * ```
	 */
	TRACE_RULE(List, term('['_E) >> *Expression >> term(']'_E));

	TRACE_RULE(Record, Keywords.Record >> term('{'_E) >> Values >> term('}'_E));

	struct
	{
		const char* Input = "<-";
		const char* Produces = "=>";
	} Operators;

	TRACE_RULE(Sum, AddOperation | PrefixOperation | ScalarAddOperation | Term);
	const Rule AddOperation = Sum >> "+" >> Sum;
	const Rule PrefixOperation = Sum >> "::" >> Sum;
	const Rule ScalarAddOperation = Sum >> ".+" >> Sum;

	TRACE_RULE(CompareExpr,
		LessThanOperation | GreaterThanOperation
		| EqualsOperation | NotEqualOperation
		| Sum);

	const Rule LessThanOperation = Sum >> "<" >> Sum;
	const Rule GreaterThanOperation = Sum >> ">" >> Sum;
	const Rule EqualsOperation = Sum >> "==" >> Sum;
	const Rule NotEqualOperation = Sum >> "!=" >> Sum;

	TRACE_RULE(LogicExpr, AndOperation | OrOperation | XOrOperation | CompareExpr);
	const Rule AndOperation = LogicExpr >> "and" >> LogicExpr;
	const Rule OrOperation = LogicExpr >> "or" >> LogicExpr;
	const Rule XOrOperation = LogicExpr >> "xor" >> LogicExpr;

	TRACE_RULE(BinaryOperation, LogicExpr);

#if 0

		AndOperation
		| OrOperation
		| XOrOperation

		| LessThanOperation
		| GreaterThanOperation
		| EqualsOperation
		| NotEqualOperation

		| AddOperation
		| PrefixOperation
		| ScalarAddOperation
	);
#endif

	TRACE_RULE(UnaryOperation, "not"_E >> Expression);
	TRACE_RULE(Operation, UnaryOperation | BinaryOperation);

	TRACE_RULE(Arguments, NamedArguments | (Argument >> *(','_E >> Argument)));
	TRACE_RULE(Argument, NamedArgument | UnnamedArgument);
	TRACE_RULE(NamedArgument, Identifier >> '=' >> Expression);
	TRACE_RULE(NamedArguments, NamedArgument >> *(','_E >> NamedArgument));
	TRACE_RULE(UnnamedArgument, Expression);

	TRACE_RULE(Value,
		Identifier >> "=" >> Expression >> ";"
		| Identifier >> ':' >> Type >> "=" >> Expression >> ";"
	);
	TRACE_RULE(Values, *Value);

#if 0
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
#endif


#if 0
expression:
	literal
	| action
	| binaryOperation
	| call
	| compoundExpr
	| conditional
	| '(' expression ')'	{ SetOrDie($$, $2.node); }
	| fieldAccess
	| fieldQuery
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
	| some
	| structInstantiation
	| unaryOperation
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
	| EQUAL			{ $$.intVal = BinaryOperation::Equal; }
	| NEQUAL		{ $$.intVal = BinaryOperation::NotEqual; }
	;

call:
	expression '(' argumentList ')'
	{
		auto target = TakeNode<Expression>($1);
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
	expression '.' name
	{
		auto structure = TakeNode<Expression>($1);
		auto field = TakeNode<Identifier>($3);

		SetOrDie($$, p->FieldAccess(structure, field));
	}
	;

fieldQuery:
	expression '.' name '?' expression
	{
		auto structure = TakeNode<Expression>($1);
		auto field = TakeNode<Identifier>($3);
		auto defaultValue = TakeNode<Expression>($5);

		SourceRange src = SourceRange::Over(structure, defaultValue);

		SetOrDie($$, p->FieldQuery(structure, field, defaultValue, src));
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
	IMPORT '(' literal ',' argumentList ')'
	{
		auto begin = Take($1.token);
		auto name = TakeNode<StringLiteral>($3);
		auto args = Take(NodeVec<Argument>($5));
		auto end = Take($6.token);

		SourceRange src = SourceRange::Over(begin, end);

		SetOrDie($$, p->ImportModule(name, *args, src));
	}
	|
	IMPORT '(' literal ')'
	{
		auto begin = Take($1.token);
		auto name = TakeNode<StringLiteral>($3);
		auto end = Take($4.token);

		UniqPtrVec<Argument> args;
		SourceRange src = SourceRange::Over(begin, end);

		SetOrDie($$, p->ImportModule(name, args, src));
	}
	;

mapping:
	identifier INPUT expression
	{
		auto target = TakeNode<Identifier>($1);
		auto source = TakeNode<Expression>($3);

		SetOrDie($$, p->Map(source, target));
	}
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
	identifier
	{
		SetOrDie($$, p->Reference(TakeNode<Identifier>($1)));
	}
	;

some:
	SOME '(' expression ')'
	{
		SourceRange begin = Take($1.token)->source();
		UniqPtr<Expression> value { TakeNode<Expression>($3) };
		SourceRange end = Take($4.token)->source();
		SourceRange src(begin, end);

		SetOrDie($$, p->Some(value, src));
	}
	;

structInstantiation:
	structBegin '{' values '}'
	{
		SourceRange begin = Take(Parser::ParseToken($1))->source();
		SourceRange end = Take($4.token)->source();

		SetOrDie($$, p->StructInstantiation(SourceRange(begin, end)));
	}
	;

structBegin:
	STRUCT	{ p->EnterScope("struct"); }
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
	;

fieldTypes:
	/* empty */
	{
		CreateNodeList($$);
	}
	| identifier
	{
		CreateNodeList($$);
		Append($$, TakeNode<Identifier>($1));
	}
	| fieldTypes ',' identifier
	{
		$$.nodes = $1.nodes;
		Append($$, TakeNode<Identifier>($3));
	}
	;
#endif

	static const Grammar& get();

	private:
	Grammar() {}

};


} // namespace parser
} // namespace fabrique

#endif
