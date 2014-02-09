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
#include "Types/FunctionType.h"
#include "Types/Type.h"
#include "Support/exceptions.h"

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

	SourceRange range = SourceRange::Over(id, e);

	if (id->isTyped() and !e->type().isSupertype(*id->type()))
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

	if (&e->type() == NULL)
	{
		ReportError("reference to value with unknown type", *id);
		return NULL;
	}

	return new SymbolReference(id, e, id->source());
}


Action* Parser::DefineAction(PtrVec<Argument>* args, SourceRange *start,
                             PtrVec<Parameter>* params)
{
	unique_ptr<PtrVec<Argument> > a(args);
	unique_ptr<SourceRange> s(start);
	SourceRange current(lex.CurrentTokenRange());

	SourceRange loc(start->begin, current.end);

	StringMap<const Parameter*> parameters;
	if (params)
		for (const Parameter *p : *params)
			parameters[p->getName().name()] = p;


	auto i = parameters.find("in");
	const Type& inType = i != parameters.end()
		? i->second->type()
		: *ctx.fileListType();

	i = parameters.find("out");
	const Type& outType = i != parameters.end()
		? i->second->type()
		: *ctx.fileListType();

	const FunctionType& type = *ctx.functionType(inType, outType);

	return new Action(*args, parameters, type, loc);
}


Function* Parser::DefineFunction(PtrVec<Parameter> *params, const Type *output,
                                 CompoundExpression *body)
{
	unique_ptr<PtrVec<Parameter> > p(params);

	assert(params != NULL);
	assert(output != NULL);

	if (!body->type().isSupertype(*output))
	{
		ReportError(
			"wrong return type ("
			+ body->type().str() + " != " + output->str()
			+ ")", *body);
		return NULL;
	}

	SourceRange loc(savedLoc.begin, lex.CurrentTokenRange().end);
	ExitScope();

	PtrVec<Type> parameterTypes;
	for (const Parameter *p : *params)
		parameterTypes.push_back(&p->type());

	const FunctionType *ty = ctx.functionType(parameterTypes, *output);

	return new Function(*params, *ty, body, loc);
}


Call* Parser::CreateCall(Identifier *name, PtrVec<Argument> *args)
{
	unique_ptr<Identifier> n(name);
	unique_ptr<PtrVec<Argument> > a(args);

	assert(name != NULL);
	assert(args != NULL);

	SourceRange loc(name->source().begin, lex.CurrentTokenRange().end);

	auto *fn(Reference(n.release()));
	if (fn == NULL)
	{
		ReportError("call to undefined function", loc);
		return NULL;
	}

	auto& fnType = dynamic_cast<const FunctionType&>(fn->type());
	const Type& returnType = fnType.returnType();

	const Type *outType = NULL;
	if (auto *action = dynamic_cast<const Action*>(&fn->getValue()))
	{
		std::vector<std::string> argNames;
		for (const Argument *arg : *args)
			argNames.push_back(
				arg->hasName() ? arg->getName().name() : "");

		StringMap<int> allArgNames = action->NameArguments(argNames);

		if (allArgNames.find("out") == allArgNames.end())
		{
			ReportError("missing 'out' argument", loc);
			return NULL;
		}

		outType = &(*args)[allArgNames["out"]]->type();
	}

	const Type& resultType = outType ? *outType : returnType;
	return new Call(fn, *args, resultType, loc);
}


Conditional* Parser::IfElse(SourceRange *ifLoc, Expression *condition,
	                    Expression *thenResult, Expression *elseResult)
{
	unique_ptr<SourceRange> src(ifLoc);

	const Type &tt(thenResult->type()), &et(elseResult->type());
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

	assert(&loopParam->type() != NULL);

	SourceRange loc(begin->begin, lex.CurrentTokenRange().end);
	ExitScope();

	const Type& resultTy = *getType("list", body->type());
	return new ForeachExpr(s.release(), p.release(), b.release(),
	                       resultTy, loc);
}


Parameter* Parser::ForeachParam(Identifier *id)
{
	assert(savedType != NULL);

	const Type& t = *savedType;
	savedType = NULL;

	if (id->isTyped() and !t.isListOf(*id->type()))
	{
		ReportError("type mismatch (" + t.str() + " not list of "
		             + id->type()->str() + ")", *id);
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
			: &elements->front()->type();

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
	SourceRange vbegin = (haveValues ? *val->begin() : result)->source();

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

	if (name->type().name() != "string")
	{
		ReportError("filename should be a string, not "
		             + name->type().str(), *name);
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

	const Type& ty = *ctx.fileListType();
	SourceRange loc(SourceRange::None());

	return new FileList(*files, args ? *args : emptyArgs, ty, loc);
}


UnaryOperation* Parser::UnaryOp(UnaryOperation::Operator op, Expression* e)
{
	return UnaryOperation::Create(op, savedLoc, e);
}


BinaryOperation* Parser::BinaryOp(BinaryOperation::Operator op,
                                  Expression *lhs, Expression *rhs)
{
	if (lhs == NULL or rhs == NULL)
		return NULL;

	try { return BinaryOperation::Create(lhs, op, rhs); }
	catch (fabrique::SourceCodeException& e)
	{
		ReportError(e.message(), e.source());
		return NULL;
	}
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

	const Type *nameType = name->type();

	if (defaultValue != NULL and name->isTyped()
	    and !defaultValue->type().isSupertype(*name->type()))
	{
		ReportError("type mismatch", *defaultValue);
		return NULL;
	}

	const Type& resultType = nameType ? *nameType : defaultValue->type();

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

	SourceRange loc(untyped->source().begin, savedLoc.end);

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


const ErrorReport& Parser::ReportError(const string& msg, const HasSource& s)
{
	return ReportError(msg, s.source());
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
