/** @file Parsing/Grammar.h    Declaration of the AST grammar. */
/*
 * Copyright (c) 2014-2016 Jonathan Anderson
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
	// Terminals that need to be declared before rules:
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
		const ExprPtr Type = term("type");
	} Keywords;

	struct
	{
		const ExprPtr Assign = term('=');
		const ExprPtr Colon = term(':');
		const ExprPtr Comma = term(',');
		const ExprPtr Semicolon = term(';');

		const ExprPtr OpenBrace = term('{');
		const ExprPtr CloseBrace = term('}');

		const ExprPtr OpenBracket = term('[');
		const ExprPtr CloseBracket = term(']');

		const ExprPtr OpenParen = term('(');
		const ExprPtr CloseParen = term(')');
	} Symbols;

	struct
	{
		const ExprPtr Dot = term('.');
		const ExprPtr Query = term('?');

		const ExprPtr Input = trace("Operators.Input", term("<-"));
		const ExprPtr Produces = term("=>");

		const ExprPtr Minus = term("-");
		const ExprPtr Plus = term("+");
		const ExprPtr Prefix = term("::");
		const ExprPtr ScalarAdd = term(".+");

		const ExprPtr GreaterThan = term(">");
		const ExprPtr LessThan = term("<");
		const ExprPtr Equals = term("==");
		const ExprPtr NotEqual = term("!=");

		const ExprPtr And = term("and");
		const ExprPtr Not = term("not");
		const ExprPtr Or = term("or");
		const ExprPtr XOr = term("xor");

		const ExprPtr Assign = term("=");
	} Operators;

	const Rule Alpha = ('A'_E - 'Z') | ('a'_E - 'z');
	const Rule Digit = '0'_E - '9';
	const Rule AlphaNum = Alpha | Digit;
	const Rule IdChar = AlphaNum | '_';

	//! An identifier starts with [A-Za-z_] and contains [A-Za-z0-9_].
	const Rule Identifier = term((Alpha | '_') >> *IdChar);

	/*
	 * Fabrique supports boolean, integer and string literals.
	 */
	const Rule BoolLiteral = Keywords.True | Keywords.False;
	const Rule IntLiteral = +Digit;

	const Rule SingleQuotedString = "'"_E >> *(!"'"_E >> any()) >> "'";
	const Rule DoubleQuotedString = "\""_E >> *(!"\""_E >> any()) >> "\"";
	const Rule StringLiteral = SingleQuotedString | DoubleQuotedString;

	const Rule Literal = term(BoolLiteral | IntLiteral | StringLiteral);


	/**
	 * There are four syntaxes for naming types:
	 *
	 *  - function types: (type1, type2) => resultType
	 *  - record types: record[field1:type2, field2:type2]
	 *  - parametric types: simpleName[typeArg1, typeArg2]
	 *  - simple types: int, string, foo, etc.
	 */
	const Rule Type = RecordType | ParametricType | SimpleType;

	const Rule RecordType =
		Keywords.Record >> Symbols.OpenBracket
		>> -(FieldType >> *(Symbols.Comma >> FieldType))
		>> Symbols.CloseBracket
		;

	const Rule FieldType = Identifier >> Symbols.Colon >> Type;

	const Rule ParametricType =
		SimpleType >> Symbols.OpenBracket
		>> Type >> *(Symbols.Comma >> Type)
		>> Symbols.CloseBracket
		;

	const Rule SimpleType = Identifier;

#if 0
	| '(' types ')' PRODUCES type
	{
		SourceRange begin = Take($1.token)->source();
		auto argTypes(Take($2.types));
		SourceRange end = Take($3.token)->source();
		const Type *returnType = $5.type;

		SourceRange src(begin, end);

		$$.type = &p->FnType(*argTypes, *returnType, src);
	}
#endif

	/**
	 * Almost everything in Fabrique is an Expression.
	 */
	TRACE_RULE(Expression, FieldReference | BinaryOperation);

	/**
	 * The most fundamental component of an Expression (evaluated first).
	 */
	TRACE_RULE(Term,
		Literal
		| ParentheticalExpression
		| Action
		| CompoundExpression
		| Conditional
		| File
		| FileList
		| List
		| Record
		| TypeDeclaration
		| UnaryOperation

		// Put identifier references after keywords so that
		// we don't match keywords as identifiers:
		| NameReference
	);

	/**
	 * An expression in parenthesis is evaluated before other operations.
	 */
	const Rule ParentheticalExpression =
		Symbols.OpenParen >> Expression >> Symbols.CloseParen;

	/**
	 * A build action: transforms input files to some number of output files.
	 */
	const Rule Action =
		Keywords.Action
		>> Symbols.OpenParen
		>> Arguments
		>> -(Operators.Input >> Parameters)
		>> Symbols.CloseParen
		;

	/**
	 * A compound expression includes zero or more value definitions and ends with
	 * an expression (which the compound expression evaluates to).
	 *
	 * Example:
	 * ```
	 * {
	 *   foo = 42;
	 *   bar = 3.1415926;
	 *
	 *   foo / bar
	 * }
	 * ```
	 */
	const Rule CompoundExpression =
		Symbols.OpenBrace >> Values >> Expression >> Symbols.CloseBrace;

	/**
	 * A conditional expression: evaluates to either the `then` or the `else` clause.
	 */
	const Rule Conditional =
		Keywords.If >> Expression >> Expression
		>> Keywords.Else >> Expression;

	const Rule FieldReference = trace("FieldReference",
		Term >> +(Operators.Dot >> Identifier)
		);

	/**
	 * A file in the described build, with a name and, optionally, arguments.
	 *
	 * Example:
	 * `file('foo.c', cflags = [ '-D' 'FOO' ])`
	 */
	const Rule File =
	     Keywords.File >> '('
	     >> Expression >> -(','_E >> NamedArguments)
	     >> ')';

	const Rule Filename = term(+(IdChar | '.' | '/'));

	/**
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
	const Rule FileList =
		Keywords.Files
		>> Symbols.OpenParen
		>> *(File | Filename)
		>> -(Symbols.Comma >> NamedArguments)
		>> Symbols.CloseParen
		;

	/**
	 * Lists are containers for like values and do not use comma separators.
	 * The type of the list is taken to be "list of the supertype of all of the
	 * list's elements".
	 *
	 * Example:
	 * ```
	 * x:int = 42;
	 * y:special_int = some_special_kind_of_int();
	 *
	 * [ 1 2 3 x y ]   # the type of this is list[int]
	 * ```
	 */
	const Rule List = Symbols.OpenBracket >> *Expression >> Symbols.CloseBracket;

	const Rule NameReference = Identifier;

	const Rule RecordTypeDeclaration =
		Keywords.Record >> Symbols.OpenBracket
		>> -(FieldType >> *(Symbols.Comma >> FieldType))
		>> Symbols.CloseBracket
		;

	const Rule TypeDeclaration = RecordTypeDeclaration;

	const Rule UnaryOperation = NotOperation | NegativeOperation | PositiveOperation;
	const Rule NotOperation = Operators.Not >> Expression;
	const Rule NegativeOperation = Operators.Minus >> Expression;
	const Rule PositiveOperation = Operators.Plus >> Expression;

	const Rule Sum = AddOperation | PrefixOperation | ScalarAddOperation | Term;
	const Rule AddOperation = Sum >> Operators.Plus >> Sum;
	const Rule PrefixOperation = Sum >> Operators.Prefix >> Sum;
	const Rule ScalarAddOperation = Sum >> Operators.ScalarAdd >> Sum;

	const Rule CompareExpr =
		LessThanOperation | GreaterThanOperation
		| EqualsOperation | NotEqualOperation
		| Sum
		;

	const Rule GreaterThanOperation = Sum >> Operators.GreaterThan >> Sum;
	const Rule LessThanOperation = Sum >> Operators.LessThan >> Sum;
	const Rule EqualsOperation = Sum >> Operators.Equals >> Sum;
	const Rule NotEqualOperation = Sum >> Operators.NotEqual >> Sum;

	const Rule LogicExpr = AndOperation | OrOperation | XOrOperation | CompareExpr;
	const Rule AndOperation = LogicExpr >> Operators.And >> LogicExpr;
	const Rule OrOperation = LogicExpr >> Operators.Or >> LogicExpr;
	const Rule XOrOperation = LogicExpr >> Operators.XOr >> LogicExpr;

	const Rule BinaryOperation = LogicExpr;

	TRACE_RULE(Arguments, NamedArguments | (Argument >> *(Symbols.Comma >> Argument)));
	const Rule Argument = NamedArgument | UnnamedArgument;
	const Rule NamedArgument = Identifier >> Symbols.Assign >> Expression;
	const Rule NamedArguments = NamedArgument >> *(Symbols.Comma >> NamedArgument);
	const Rule UnnamedArgument = Expression;

	TRACE_RULE(Parameters,
		Parameters >> Symbols.Comma >> Parameter
		| Parameter
	);

	TRACE_RULE(Parameter,
		Identifier >> Symbols.Colon >> Type
		>> -(Symbols.Assign >> Expression)
	);

	TRACE_RULE(Value,
		Identifier >> Operators.Assign >> Expression
		| Identifier >> Symbols.Colon >> Type >> Operators.Assign >> Expression
	);
	TRACE_RULE(Values, *(Value >> Symbols.Semicolon));

	const Rule Record =
		Keywords.Record >> Symbols.OpenBrace >> Values >> Symbols.CloseBrace;

#if 0
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
	| some
	| structInstantiation
	| unaryOperation
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

foreach:
	foreachbegin mapping expression
	{
		SourceRange begin = Take(Parser::ParseToken($1))->source();
		auto mapping = TakeNode<Mapping>($2);
		auto body = TakeNode<Expression>($3);

		SetOrDie($$, p->Foreach(mapping, body, begin));
	}
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
#endif

	static const Grammar& get();

	private:
	Grammar() {}

};


} // namespace parser
} // namespace fabrique

#endif
