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

#include "Parsing/Lexer.h"
#include "Parsing/Parser.h"
#include "Types/Type.h"
#include "FabContext.h"

using namespace fabrique::ast;

using fabrique::ErrorReport;
using fabrique::SourceRange;
using fabrique::Type;

using std::string;
using std::unique_ptr;


Parser::Parser(FabContext& ctx, const Lexer& lex)
	: ctx(ctx), lex(lex), savedLoc(SourceRange::None())
{
	scopes.push(new Scope());
}

Parser::~Parser()
{
	for (auto *err : errs)
		delete err;
}


void Parser::SetRoot(PtrVec<Value> *values)
{
	unique_ptr<PtrVec<Value> > v(values);
	for (auto *v : *values)
		root.Register(v);
}


const Type* Parser::getType(const string& name, const Type& param)
{
	return getType(name, PtrVec<Type>(1, &param));
}


const Type* Parser::getType(const string& name, const PtrVec<Type>& params)
{
	return ctx.type(name, params);
}

const Type* Parser::getType(Identifier *name, const PtrVec<Type>* params)
{
	unique_ptr<Identifier> i(name);
	unique_ptr<const PtrVec<Type> > p(params);
	static PtrVec<Type> empty;

	assert(name != NULL);

	return getType(name->name(), params ? *params : empty);
}


Scope& Parser::CurrentScope()
{
	// We must always have at least a top-level scope on the stack.
	assert(scopes.size() > 0);
	return *scopes.top();
}

void Parser::EnterScope() { scopes.push(new Scope(&CurrentScope())); }
void Parser::ExitScope() { scopes.pop(); }


SourceRange* Parser::CurrentTokenRange() const
{
	return new SourceRange(lex.CurrentTokenRange());
}

void Parser::SaveLoc() { savedLoc = lex.CurrentTokenRange(); }

void Parser::SaveType(const Type& t)
{
	savedType = &t;
}

Value* Parser::Define(Identifier *id, Expression *e)
{
	if (e == NULL)
		return NULL;

	auto& scope(CurrentScope());

	if (scope.Find(id) != NULL)
	{
		ReportError("redefining value", *id);
		return NULL;
	}

	SourceRange range(id->getSource().begin, e->getSource().end);

	if (id->isTyped() and !e->getType().isSupertype(*id->getType()))
	{
		ReportError("type mismatch", range);
		return NULL;
	}

	Value *v = new Value(id, e);
	scope.Register(v);
	return v;
}


SymbolReference* Parser::Reference(Identifier *id)
{
	const Expression *e = CurrentScope().Find(id);
	if (e == NULL)
	{
		ReportError("reference to undefined value", *id);
		return NULL;
	}

	if (&e->getType() == NULL)
	{
		ReportError("reference to value with unknown type", *id);
		return NULL;
	}

	return new SymbolReference(id, e, id->getSource());
}


Action* Parser::DefineAction(PtrVec<Argument>* args, SourceRange *start,
                             PtrVec<Parameter>* params)
{
	unique_ptr<PtrVec<Argument> > a(args);
	unique_ptr<SourceRange> s(start);
	SourceRange current(lex.CurrentTokenRange());

	SourceRange loc(start->begin, current.end);

	const Type *fileType = getType("file");
	const Type *fileListType = getType("list", PtrVec<Type>(1, fileType));

	StringMap<const Parameter*> parameters;
	if (params)
		for (const Parameter *p : *params)
			parameters[p->getName().name()] = p;

	return new Action(*args, parameters, *fileListType, loc);
}


Function* Parser::DefineFunction(PtrVec<Parameter> *params, const Type *ty,
                                 CompoundExpression *body)
{
	unique_ptr<PtrVec<Parameter> > p(params);

	assert(params != NULL);
	assert(ty != NULL);

	if (!body->getType().isSupertype(*ty))
	{
		ReportError(
			"wrong return type ("
			+ body->getType().str() + " != " + ty->str()
			+ ")", *body);
		return NULL;
	}

	SourceRange loc(savedLoc.begin, lex.CurrentTokenRange().end);
	ExitScope();

	return new Function(*params, *ty, body, loc);
}


Call* Parser::CreateCall(Identifier *name, PtrVec<Argument> *args)
{
	unique_ptr<Identifier> n(name);
	unique_ptr<PtrVec<Argument> > a(args);

	assert(name != NULL);
	assert(args != NULL);

	SourceRange loc(name->getSource().begin, lex.CurrentTokenRange().end);

	auto *fn(Reference(n.release()));
	if (fn == NULL)
		return NULL;

	return new Call(fn, *args, loc);
}


Conditional* Parser::IfElse(SourceRange *ifLoc, Expression *condition,
	                    Expression *thenResult, Expression *elseResult)
{
	unique_ptr<SourceRange> src(ifLoc);

	const Type &tt(thenResult->getType()), &et(elseResult->getType());
	if (!tt.isSupertype(et) and !et.isSupertype(tt))
	{
		ReportError("incompatible types",
		            SourceRange::Over(thenResult, elseResult));
		return NULL;
	}

	return new Conditional(*ifLoc, condition, thenResult, elseResult,
	                       Type::GetSupertype(tt, et));
}


ForeachExpr* Parser::Foreach(Expression *source, const Parameter *loopParam,
                             CompoundExpression *body, SourceRange *begin)
{
	unique_ptr<Expression> s(source);
	unique_ptr<const Parameter> p(loopParam);
	unique_ptr<CompoundExpression> b(body);
	unique_ptr<SourceRange> beg(begin);

	assert(&loopParam->getType() != NULL);

	SourceRange loc(begin->begin, lex.CurrentTokenRange().end);
	ExitScope();

	const Type& resultTy = *getType("list", body->getType());
	return new ForeachExpr(s.release(), p.release(), b.release(),
	                       resultTy, loc);
}


Parameter* Parser::ForeachParam(Identifier *id)
{
	assert(savedType != NULL);

	const Type& t = *savedType;
	savedType = NULL;

	if (id->isTyped() and !t.isListOf(*id->getType()))
	{
		ReportError("type mismatch (" + t.str() + " not list of "
		             + id->getType()->str() + ")", *id);
		return NULL;
	}

	auto *p = new Parameter(id, t[0]);
	CurrentScope().Register(p);
	return p;
}


List* Parser::ListOf(ExprVec* elements)
{
	unique_ptr<ExprVec> e(elements);
	assert(elements != NULL);

	const Type *elementType =
		elements->empty()
			? ctx.nilType()
			: &elements->front()->getType();

	const Type *ty = getType("list", PtrVec<Type>(1, elementType));
	SourceRange loc(savedLoc.begin, lex.CurrentTokenRange().end);

	return new List(*elements, *ty, loc);
}


CompoundExpression* Parser::CompoundExpr(Expression *result, SourceRange *b,
	                                 PtrVec<Value> *val)
{
	unique_ptr<Expression> e(result);
	unique_ptr<PtrVec<Value> > v(val);
	static PtrVec<Value> empty;

	auto& values = val ? *val : empty;
	bool haveValues = (val != NULL) and !val->empty();
	SourceRange vbegin = (haveValues ? *val->begin() : result)->getSource();

	Location begin = (b ? *b : vbegin).begin;
	Location end = lex.CurrentTokenRange().end;

	return new CompoundExpression(values, e.release(),
	                              SourceRange(begin, end));
}


Filename* Parser::Source(Expression *name, SourceRange *source,
                         PtrVec<Argument> *args)
{
	unique_ptr<SourceRange> r(source);
	unique_ptr<PtrVec<Argument> > a(args);
	static PtrVec<Argument> empty;

	assert(name != NULL);
	assert(source != NULL);

	if (name->getType().name() != "string")
	{
		ReportError("filename should be a string, not "
		             + name->getType().str(), *name);
		return NULL;
	}

	return new Filename(name, args ? *args : empty,
	                    *getType("file"), *source);
}


FileList* Parser::Files(PtrVec<Filename> *files, PtrVec<Argument> *args)
{
	unique_ptr<PtrVec<Filename> > f(files);
	unique_ptr<PtrVec<Argument> > a(args);
	static PtrVec<Argument> emptyArgs;

	static const Type *ty = fileListType();
	SourceRange loc(SourceRange::None());

	return new FileList(*files, args ? *args : emptyArgs, *ty, loc);
}


BinaryOperation* Parser::Concat(Expression *lhs, Expression *rhs)
{
	if (lhs == NULL or rhs == NULL)
		return NULL;

	const Type &lt = lhs->getType(), &rt = rhs->getType();
	if (!lt.isSubtype(rt) and !rt.isSubtype(lt))
	{
		ReportError("incompatible types (" + lt.str() + ", "
		             + rt.str() + ")", SourceRange::Over(lhs, rhs));
		return NULL;
	}

	return BinaryOperation::Create(lhs, BinaryOperation::Concatenate, rhs,
	                               Type::GetSupertype(lt, rt));
}


BinaryOperation* Parser::Prefix(Expression *lhs, Expression *rhs)
{
	if (lhs == NULL or rhs == NULL)
		return NULL;

	const Type &lt = lhs->getType(), &rt = rhs->getType();
	if (!rt.isListOf(lt))
	{
		ReportError("incompatible types", SourceRange::Over(lhs, rhs));
		return NULL;
	}

	return BinaryOperation::Create(lhs, BinaryOperation::Prefix, rhs, rt);
}


BinaryOperation* Parser::ScalarAdd(Expression *lhs, Expression *rhs)
{
	if (lhs == NULL or rhs == NULL)
	return NULL;

	const Expression *lst, *element;

	const Type &lt = lhs->getType(), &rt = rhs->getType();
	if (lt.isListOf(rt))
	{
		lst = lhs;
		element = rhs;
	}
	else
	{
		lst = rhs;
		element = lhs;
	}

	// Are we trying to do .+ on unrelated (or non-list) types?
	if (!lst->getType().isListOf(element->getType()))
	{
		ReportError("incompatible types", SourceRange::Over(lhs, rhs));
		return NULL;
	}


	return BinaryOperation::Create(lhs, BinaryOperation::ScalarAdd, rhs,
	                               lst->getType());
}


Argument* Parser::Arg(Expression *e, Identifier *name)
{
	if (e == NULL)
		return NULL;

	return new Argument(name, e);
}


Parameter* Parser::Param(Identifier *name, Expression *defaultValue)
{
	if (name == NULL)
		return NULL;

	const Type *nameType = name->getType();

	if (defaultValue != NULL and name->isTyped()
	    and !defaultValue->getType().isSupertype(*name->getType()))
	{
		ReportError("type mismatch", *defaultValue);
		return NULL;
	}

	const Type& resultType = nameType ? *nameType : defaultValue->getType();

	auto *p = new Parameter(name, resultType, defaultValue);
	CurrentScope().Register(p);

	return p;
}


Identifier* Parser::Id(const std::string& name)
{
	return new Identifier(name, NULL, lex.CurrentTokenRange());
}

Identifier* Parser::Id(Identifier *untyped, const Type *ty)
{
	unique_ptr<Identifier> u(untyped);
	assert(!untyped->isTyped());

	SourceRange loc(untyped->getSource().begin, savedLoc.end);

	return new Identifier(u->name(), ty, loc);
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

StringLiteral* Parser::ParseString(const string& value)
{
	// The lexer's current token range points to the quotation mark that
	// was used to close the string.
	//
	// The real beginning of the source range is value.length() columns
	// before the place the lexer reports. Simple subtraction is safe here
	// because Fabrique strings can't straddle newlines.
	SourceRange current(lex.CurrentTokenRange());
	Location begin(current.begin.filename, current.begin.line,
	               current.begin.column - value.length() - 1);

	assert(begin.column > 0);

	SourceRange loc(begin, current.end);

	return new StringLiteral(value, *getType("string"), loc);
}


const Type* Parser::fileType()
{
	static const Type *t = getType("file");
	return t;
}


const Type* Parser::fileListType()
{
	static const Type *t = getType("list", PtrVec<Type>(1, fileType()));
	return t;
}


const ErrorReport& Parser::ReportError(const string& msg, const HasSource& s)
{
	return ReportError(msg, s.getSource());
}

const ErrorReport& Parser::ReportError(const string& message,
                                       const SourceRange& location)
{
	auto *err(ErrorReport::Create(message, location));
	errs.push_back(err);

	return *err;
}


void Parser::AddToScope(const PtrVec<Argument>& args)
{
	auto& scope(CurrentScope());

	for (auto *arg : args)
		if (arg->hasName())
			scope.Register(arg);
}
