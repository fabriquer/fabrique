/** @file Parser.cc    Definition of @ref Parser. */
/*
 * Copyright (c) 2013 Jonathan Anderson
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

using namespace fabrique::ast;

using fabrique::ErrorReport;
using fabrique::SourceRange;
using fabrique::Type;

using std::string;
using std::unique_ptr;


Parser::Parser(FabContext& ctx, const Lexer& lex)
	: ctx(ctx), lex(lex)
{
	scopes.emplace(new Scope(nullptr, "file scope"));
}

Parser::~Parser()
{
	FabAssert(scopes.empty());
}



//
// AST scopes:
//
Scope& Parser::EnterScope(const string& name)
{
	Bytestream::Debug("parser.scope")
		<< string(scopes.size(), ' ')
		<< Bytestream::Operator << " >> "
		<< Bytestream::Type << "scope"
		<< Bytestream::Literal << " '" << name << "'"
		<< Bytestream::Reset << "\n"
		;

	scopes.emplace(new Scope(&CurrentScope(), name));
	return *scopes.top();
}

unique_ptr<Scope> Parser::ExitScope()
{
	unique_ptr<Scope> scope = std::move(scopes.top());
	FabAssert(scope and not scopes.top());
	scopes.pop();

	Bytestream& dbg = Bytestream::Debug("parser.scope");
	dbg
		<< string(scopes.size(), ' ')
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
	return ctx.type(name, params);
}

const Type* Parser::getType(const string& name, const Type& param)
{
	return getType(name, PtrVec<Type>(1, &param));
}

const Type* Parser::getType(UniqPtr<Identifier>&& name,
                            UniqPtr<const PtrVec<Type>>&& params)
{
	static const PtrVec<Type> empty;
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
	return Action::Create(*args, params, src, ctx);
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


Call* Parser::CreateCall(UniqPtr<Identifier>& name,
                         UniqPtr<UniqPtrVec<Argument>>& args)
{
	if (not name or not args)
		return nullptr;

	SourceRange loc(name->source().begin, lex.CurrentTokenRange().end);

	UniqPtr<SymbolReference> fn(Reference(std::move(name)));
	if (not fn)
	{
		ReportError("call to undefined function", loc);
		return nullptr;
	}

	auto& fnType = dynamic_cast<const FunctionType&>(fn->type());
	const Type& returnType = fnType.returnType();

	const Type *outType = nullptr;
	if (auto *action = dynamic_cast<const Action*>(&fn->definition()))
	{
		StringMap<const Argument*> namedArguments
			= action->NameArguments(*args);

		auto out = namedArguments.find("out");
		if (out == namedArguments.end())
		{
			ReportError("missing 'out' argument", loc);
			return nullptr;
		}

		outType = &out->second->type();

#if 0
		// TODO(JA): ensure 'out' is a subtype
		if (not outType->isSubType(...))
			;
#endif
	}

	const Type& resultType = outType ? *outType : returnType;
	return new Call(fn, *args, resultType, loc);
}


CompoundExpression* Parser::CompoundExpr(UniqPtr<Expression>& result,
                                         SourceRange begin, SourceRange end)
{
	if (not result)
		return nullptr;

	SourceRange src = result->source();
	if (begin != SourceRange::None)
	{
		FabAssert(end != SourceRange::None);
		src = SourceRange(begin, end);
	}

	return new CompoundExpression(ExitScope(), result, src);
}



Filename* Parser::File(UniqPtr<Expression>& name, const SourceRange& src,
                       UniqPtr<UniqPtrVec<Argument>>&& args)
{
	static UniqPtrVec<Argument> empty;

	if (not name->type().isSubtype(*getType("string")))
	{
		ReportError("filename should be of type 'string', not '"
		             + name->type().str() + "'", *name);
		return nullptr;
	}

	return new Filename(name, args ? *args : empty, *ctx.fileType(), src);
}


FileList* Parser::Files(const SourceRange& begin,
                        UniqPtr<UniqPtrVec<Filename>>& files,
                        UniqPtr<UniqPtrVec<Argument>>&& args)
{
	static UniqPtrVec<Argument> emptyArgs;

	const Type& ty = *ctx.fileListType();
	SourceRange src(begin);

	return new FileList(*files, args ? *args : emptyArgs, ty, src);
}


ForeachExpr* Parser::Foreach(UniqPtr<Expression>& source,
                             UniqPtr<Parameter>& loopParam,
                             UniqPtr<CompoundExpression>& body,
                             const SourceRange& begin)
{
	SourceRange loc(begin, lex.CurrentTokenRange());
	ExitScope();

	const Type& resultTy = *getType("list", body->type());
	return new ForeachExpr(source, loopParam, body, resultTy, loc);
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

	const FunctionType *ty = ctx.functionType(parameterTypes, *resultType);
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

	FabAssert(not untyped->isTyped());

	SourceRange loc(untyped->source().begin, lex.CurrentTokenRange().end);
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
		ReportError("incompatible types",
		            SourceRange::Over(thenResult, elseResult));
		return nullptr;
	}

	return new Conditional(ifLocation, condition, thenResult, elseResult,
	                       Type::GetSupertype(tt, et));
}


List* Parser::ListOf(UniqPtr<UniqPtrVec<Expression>>&& elements,
                     const SourceRange& src)
{
	if (not elements)
		return nullptr;

	const Type *elementType =
		elements->empty()
			? ctx.nilType()
			: &elements->front()->type();

	const Type *ty = getType("list", PtrVec<Type>(1, elementType));
	return new List(*elements, *ty, src);
}


BoolLiteral* Parser::True()
{
	return new BoolLiteral(true, *getType("bool"),
	                       lex.CurrentTokenRange());
}

BoolLiteral* Parser::False()
{
	return new BoolLiteral(false, *getType("bool"),
	                       lex.CurrentTokenRange());
}

IntLiteral* Parser::ParseInt(int value)
{
	return new IntLiteral(value, *getType("int"),
	                      lex.CurrentTokenRange());
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

	const Type *nameType = name->type();

	if (not name->isTyped() and not defaultValue)
	{
		ReportError("expected type or default value", *name);
		return nullptr;
	}

	if (defaultValue != nullptr and name->isTyped()
	    and !defaultValue->type().isSupertype(*name->type()))
	{
		ReportError("type mismatch", *defaultValue);
		return nullptr;
	}

	const Type& resultType = nameType ? *nameType : defaultValue->type();

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

	if (scope.Lookup(*id) != nullptr)
	{
		ReportError("redefining value", *id);
		return false;
	}

	SourceRange range = SourceRange::Over(id, e);

	if (id->isTyped() and !e->type().isSupertype(*id->type()))
	{
		ReportError("type mismatch", range);
		return false;
	}

	scope.Take(new Value(id, e));

	return true;
}


Scope& Parser::CurrentScope()
{
	// We must always have at least a top-level scope on the stack.
	FabAssert(scopes.size() > 0);
	return *scopes.top();
}


void Parser::AddToScope(const PtrVec<Argument>& args)
{
	auto& scope(CurrentScope());

	for (auto *arg : args)
		if (arg->hasName())
			scope.Register(arg);
}


fabrique::Token* Parser::Token(YYSTYPE& yyunion)
{
	FabAssert(yyunion.token);
	FabAssert(dynamic_cast<fabrique::Token*>(yyunion.token));

	return yyunion.token;
}


bool Parser::Set(YYSTYPE& yyunion, Expression *e)
{
	if (not e)
		return false;

	Bytestream::Debug("parser.expr")
		<< Bytestream::Action << "parsed "
		<< Bytestream::Type << "expression"
		<< Bytestream::Operator << ": "
		<< Bytestream::Reset << *e
		<< Bytestream::Operator << " @ " << e->source()
		<< "\n"
		;

	yyunion.expr = e;
	return true;
}

bool Parser::Set(YYSTYPE& yyunion, Identifier *id)
{
	if (not id)
		return false;

	Bytestream::Debug("parser.id")
		<< Bytestream::Action << "parsed "
		<< Bytestream::Type << "identifier"
		<< Bytestream::Operator << ": "
		<< Bytestream::Reset << *id
		<< Bytestream::Operator << " @ " << id->source()
		<< "\n"
		;

	yyunion.id = id;
	return true;
}


const ErrorReport& Parser::ReportError(const string& msg, const HasSource& s)
{
	return ReportError(msg, s.source());
}

const ErrorReport& Parser::ReportError(const string& message,
                                       const SourceRange& location)
{
	errs.push_back(
		unique_ptr<ErrorReport>(ErrorReport::Create(message, location))
	);

	return *errs.back();
}
