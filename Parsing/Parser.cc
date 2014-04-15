/** @file Parsing/Parser.cc    Definition of @ref Parser. */
/*
 * Copyright (c) 2013-2014 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
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

#include "AST/ast.h"
#include "Parsing/Lexer.h"
#include "Parsing/Parser.h"
#include "Parsing/Token.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Types/FunctionType.h"
#include "Types/Type.h"

#include "FabContext.h"

#include <cassert>

using namespace fabrique::ast;

using fabrique::ErrorReport;
using fabrique::SourceRange;
using fabrique::Type;

using std::string;
using std::unique_ptr;


Parser::Parser(FabContext& ctx, const Lexer& lex)
	: ctx_(ctx), lexer_(lex)
{
	scopes_.emplace(new Scope(nullptr, "file scope"));
}



//
// AST scopes:
//
Scope& Parser::EnterScope(const string& name)
{
	Bytestream::Debug("parser.scope")
		<< string(scopes_.size(), ' ')
		<< Bytestream::Operator << " >> "
		<< Bytestream::Type << "scope"
		<< Bytestream::Literal << " '" << name << "'"
		<< Bytestream::Reset << "\n"
		;

	scopes_.emplace(new Scope(&CurrentScope(), name));
	return *scopes_.top();
}

unique_ptr<Scope> Parser::ExitScope()
{
	unique_ptr<Scope> scope = std::move(scopes_.top());
	assert(scope and not scopes_.top());
	scopes_.pop();

	Bytestream& dbg = Bytestream::Debug("parser.scope");
	dbg
		<< string(scopes_.size(), ' ')
		<< Bytestream::Operator << " << "
		<< Bytestream::Type << "scope"
		<< Bytestream::Literal << " '" << scope->name() << "'"
		<< Bytestream::Operator << ":"
		;

	for (auto& v : *scope)
		dbg << " " << v.first;

	dbg
		<< Bytestream::Reset << "\n"
		;

	return std::move(scope);
}



//
// Type creation (memoised):
//
const Type* Parser::getType(const string& name, const PtrVec<Type>& params)
{
	return ctx_.type(name, params);
}

const Type* Parser::getType(const string& name, const Type& param)
{
	return getType(name, PtrVec<Type>(1, &param));
}

const Type* Parser::getType(UniqPtr<Identifier>&& name,
                            UniqPtr<const PtrVec<Type>>&& params)
{
	static const PtrVec<Type>& empty = *new PtrVec<Type>;
	if (not name)
		return nullptr;

	return getType(name->name(), params ? *params : empty);
}



Action* Parser::DefineAction(UniqPtr<UniqPtrVec<Argument>>& args,
                             const SourceRange& src,
                             UniqPtr<UniqPtrVec<Parameter>>&& params)
{
	if (not args)
		return nullptr;

	ExitScope();
	return Action::Create(*args, params, src, ctx_);
}


Argument* Parser::Arg(UniqPtr<Expression>& value, UniqPtr<Identifier>&& name)
{
	if (not value)
		return nullptr;

	return new Argument(name, value);
}


BinaryOperation* Parser::BinaryOp(BinaryOperation::Operator op,
                                  UniqPtr<Expression>& lhs,
                                  UniqPtr<Expression>& rhs)
{
	if (not lhs or not rhs)
		return nullptr;

	return BinaryOperation::Create(std::move(lhs), op, std::move(rhs));
}


bool Parser::Builtin(const string& name, int value)
{
	SourceRange src = SourceRange::Span("(builtin)", 0, 0, 0);
	UniqPtr<Token> token(new Token(name, src));

	UniqPtr<Identifier> id(Id(std::move(token)));
	UniqPtr<Expression> val(ParseInt(value));

	return DefineValue(id, val);
}


bool Parser::Builtin(const string& name, const std::string& value)
{
	SourceRange src = SourceRange::Span("(builtin)", 0, 0, 0);
	UniqPtr<Token> nameToken(new Token(name, src));
	UniqPtr<Token> valueToken(new Token(value, src));

	UniqPtr<Identifier> id(Id(std::move(nameToken)));
	UniqPtr<Expression> val(ParseString(std::move(valueToken)));

	return DefineValue(id, val);
}


Call* Parser::CreateCall(UniqPtr<Identifier>& name,
                         UniqPtr<UniqPtrVec<Argument>>& args,
                         const SourceRange& end)
{
	if (not name or not args)
		return nullptr;

	SourceRange src(name->source(), end);

	UniqPtr<SymbolReference> ref(Reference(std::move(name)));
	if (not ref)
	{
		ReportError("call to undefined action/function", src);
		return nullptr;
	}

	// Run the arguments through Callable::CheckArguments to validate
	// various rules (e.g. no positional arguments after keyword arguments).
	auto& callable = dynamic_cast<const Callable&>(ref->definition());
	try { callable.CheckArguments(*args, src); }
	catch (SourceCodeException& e)
	{
		ReportError(e.message(), e.source());
		return nullptr;
	}

	auto& fnType = dynamic_cast<const FunctionType&>(ref->type());
	return new Call(ref, *args, fnType.returnType(), src);
}


CompoundExpression* Parser::CompoundExpr(UniqPtr<Expression>& result,
                                         SourceRange begin, SourceRange end)
{
	if (not result)
		return nullptr;

	SourceRange src = result->source();
	if (begin != SourceRange::None())
	{
		assert(end != SourceRange::None());
		src = SourceRange(begin, end);
	}

	return new CompoundExpression(ExitScope(), result, src);
}



Filename* Parser::File(UniqPtr<Expression>& name, const SourceRange& src,
                       UniqPtr<UniqPtrVec<Argument>>&& args)
{
	static UniqPtrVec<Argument>& empty = *new UniqPtrVec<Argument>;

	if (not name->type().isSubtype(*getType("string")))
	{
		ReportError("filename should be of type 'string', not '"
		             + name->type().str() + "'", *name);
		return nullptr;
	}

	return new Filename(name, args ? *args : empty, *ctx_.fileType(), src);
}


FileList* Parser::Files(const SourceRange& begin,
                        UniqPtr<UniqPtrVec<Filename>>& files,
                        UniqPtr<UniqPtrVec<Argument>>&& args)
{
	static UniqPtrVec<Argument>& emptyArgs = *new UniqPtrVec<Argument>;

	const Type& ty = *ctx_.fileListType();
	SourceRange src(begin);

	return new FileList(*files, args ? *args : emptyArgs, ty, src);
}


ForeachExpr* Parser::Foreach(UniqPtr<Mapping>& mapping,
                             UniqPtr<CompoundExpression>& body,
                             const SourceRange& begin)
{
	SourceRange src(begin, body->source());
	ExitScope();

	const Type& resultTy = *getType("list", body->type());
	return new ForeachExpr(mapping, body, resultTy, src);
}


Function* Parser::DefineFunction(const SourceRange& begin,
                                 UniqPtr<UniqPtrVec<Parameter>>& params,
                                 UniqPtr<CompoundExpression>& body,
                                 const Type *resultType)
{
	if (not params or not body)
		return nullptr;

	if (!body->type().isSupertype(*resultType))
	{
		ReportError(
			"wrong return type ("
			+ body->type().str() + " != " + resultType->str()
			+ ")", *body);
		return nullptr;
	}

	SourceRange loc(begin, *body);

	PtrVec<Type> parameterTypes;
	for (auto& p : *params)
		parameterTypes.push_back(&p->type());

	ExitScope();

	const FunctionType *ty = ctx_.functionType(parameterTypes, *resultType);
	return new Function(*params, *ty, body, loc);
}



Identifier* Parser::Id(UniqPtr<fabrique::Token>&& name)
{
	if (not name)
		return nullptr;

	return new Identifier(*name, nullptr, name->source());
}

Identifier* Parser::Id(UniqPtr<Identifier>&& untyped, const Type *ty)
{
	if (not untyped)
		return nullptr;

	assert(not untyped->isTyped());

	SourceRange loc(untyped->source().begin, lexer_.CurrentTokenRange().end);
	return new Identifier(untyped->name(), ty, loc);
}


Conditional* Parser::IfElse(const SourceRange& ifLocation,
                            UniqPtr<Expression>& condition,
                            UniqPtr<CompoundExpression>& thenResult,
                            UniqPtr<CompoundExpression>& elseResult)
{
	const Type &tt(thenResult->type()), &et(elseResult->type());
	if (!tt.isSupertype(et) and !et.isSupertype(tt))
	{
		ReportError("incompatible types for conditional clauses: "
		            + tt.str() + " vs " + et.str(),
		            SourceRange::Over(thenResult, elseResult));
		return nullptr;
	}

	return new Conditional(ifLocation, condition, thenResult, elseResult,
	                       Type::GetSupertype(tt, et));
}


List* Parser::ListOf(UniqPtrVec<Expression>& elements,
                     const SourceRange& src)
{
	const Type *elementType =
		elements.empty()
			? ctx_.nilType()
			: &elements.front()->type();

	const Type *ty = getType("list", PtrVec<Type>(1, elementType));
	return new List(elements, *ty, src);
}


Mapping* Parser::Map(UniqPtr<Expression>& source, UniqPtr<Identifier>& target)
{
	if (not source or not target)
		return nullptr;

	UniqPtr<Identifier> id(std::move(target));

	assert(source->type().typeParamCount() == 1);
	const Type& elementType = source->type()[0];

	if (id->isTyped())
	{
		if (not id->type().isSupertype(elementType))
		{
			ReportError("incompatible types for map: "
			            + id->type().str() + " vs "
			            + elementType.str(),
			            SourceRange::Over(source, target));
			return nullptr;
		}
	}
	else
		id.reset(Id(std::move(id), &elementType));


	UniqPtr<Parameter> parameter(Param(std::move(id)));

	SourceRange src(*parameter, *source);
	return new Mapping(parameter, source, src);
}

BoolLiteral* Parser::True()
{
	return new BoolLiteral(true, *getType("bool"),
	                       lexer_.CurrentTokenRange());
}

BoolLiteral* Parser::False()
{
	return new BoolLiteral(false, *getType("bool"),
	                       lexer_.CurrentTokenRange());
}

IntLiteral* Parser::ParseInt(int value)
{
	return new IntLiteral(value, *getType("int"),
	                      lexer_.CurrentTokenRange());
}

StringLiteral* Parser::ParseString(UniqPtr<fabrique::Token>&& t)
{
	return new StringLiteral(*t, *getType("string"), t->source());
}


Parameter* Parser::Param(UniqPtr<Identifier>&& name,
                         UniqPtr<Expression>&& defaultValue)
{
	if (not name)
		return nullptr;

	if (not name->isTyped() and not defaultValue)
	{
		ReportError("expected type or default value", *name);
		return nullptr;
	}

	if (defaultValue != nullptr and name->isTyped()
	    and !defaultValue->type().isSubtype(name->type()))
	{
		ReportError("expected type " + name->type().str()
		            + ", got " + defaultValue->type().str(),
		            *defaultValue);
		return nullptr;
	}

	const Type& resultType =
		name->isTyped() ? name->type() : defaultValue->type();

	auto *p = new Parameter(name, resultType, std::move(defaultValue));
	CurrentScope().Register(p);

	return p;
}


SymbolReference* Parser::Reference(UniqPtr<Identifier>&& id)
{
	const Expression *e = CurrentScope().Lookup(*id);
	if (e == nullptr)
	{
		ReportError("reference to undefined value", *id);
		return nullptr;
	}

	if (&e->type() == nullptr)
	{
		ReportError("reference to value with unknown type", *id);
		return nullptr;
	}

	return new SymbolReference(std::move(id), *e, id->source());
}


UnaryOperation* Parser::UnaryOp(UnaryOperation::Operator op,
                                const SourceRange& opSrc,
                                UniqPtr<Expression>& e)
{
	return UnaryOperation::Create(op, opSrc, e);
}



bool Parser::DefineValue(UniqPtr<Identifier>& id, UniqPtr<Expression>& e)
{
	if (not id or not e)
		return false;

	auto& scope(CurrentScope());

	if (scope.contains(*id))
	{
		ReportError("redefining value", *id);
		return false;
	}

	SourceRange range = SourceRange::Over(id, e);

	if (id->isTyped() and !e->type().isSupertype(id->type()))
	{
		ReportError("assigning " + e->type().str()
		             + " to identifier of type " + id->type().str(),
		            range);
		return false;
	}

	scope.Take(new Value(id, e));

	return true;
}


Scope& Parser::CurrentScope()
{
	// We must always have at least a top-level scope on the stack.
	assert(scopes_.size() > 0);
	return *scopes_.top();
}


void Parser::AddToScope(const PtrVec<Argument>& args)
{
	auto& scope(CurrentScope());

	for (auto *arg : args)
		if (arg->hasName())
			scope.Register(arg);
}


fabrique::Token* Parser::ParseToken(YYSTYPE& yyunion)
{
	assert(yyunion.token);
	assert(dynamic_cast<fabrique::Token*>(yyunion.token));

	return yyunion.token;
}


bool Parser::Set(YYSTYPE& yyunion, Node *e)
{
	if (not e)
		return false;

	Bytestream& dbg = Bytestream::Debug("parser.node");
	dbg
		<< Bytestream::Action << "parsed "
		<< Bytestream::Type << "AST node"
		<< Bytestream::Reset
		;

	if (auto *typed = dynamic_cast<const Typed*>(e))
		dbg << " of type " << typed->type();

	else if (auto *ot = dynamic_cast<const OptionallyTyped*>(e))
	{
		if (ot->isTyped())
			dbg << " with type " << ot->type();
	}

	dbg
		<< Bytestream::Operator << ": "
		<< Bytestream::Reset << *e
		<< Bytestream::Operator << " @ " << e->source()
		<< "\n"
		;

	yyunion.node = e;
	return true;
}


const ErrorReport& Parser::ReportError(const string& msg, const HasSource& s)
{
	return ReportError(msg, s.source());
}

const ErrorReport& Parser::ReportError(const string& message,
                                       const SourceRange& location)
{
	errs_.push_back(
		unique_ptr<ErrorReport>(ErrorReport::Create(message, location))
	);

	return *errs_.back();
}
