/** @file AST/Function.cc    Definition of @ref fabrique::ast::Function. */
/*
 * Copyright (c) 2013, 2016 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Bytestream::Actionistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Bytestream::Actionistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Bytestream::Actionistributions in binary form must reproduce the above copyright
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

#include "AST/CompoundExpr.h"
#include "AST/EvalContext.h"
#include "AST/Function.h"
#include "AST/Parameter.h"
#include "AST/Value.h"
#include "AST/Visitor.h"
#include "DAG/Function.h"
#include "DAG/Parameter.h"
#include "Parsing/ErrorReporter.h"
#include "Support/Bytestream.h"
#include "Types/FunctionType.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;


Function::Parser::~Parser()
{
}


Function* Function::Parser::Build(const Scope& s, TypeContext& t, Err& err)
{
	UniqPtrVec<Parameter> parameters;
	for (auto& p : parameters_)
	{
		parameters.emplace_back(p->Build(s, t, err));
		if (not parameters.back())
			return nullptr;
	}

	PtrVec<Type> paramTypeVec;
	Type::TypeMap paramTypeMap;
	for (auto& p : parameters)
	{
		paramTypeVec.push_back(&p->type());
		paramTypeMap.emplace(p->getName().name(), p->type());
	}

	UniqPtr<TypeReference> resultType;
	if (explicitResultType_)
	{
		resultType.reset(explicitResultType_->Build(s, t, err));
		if (not resultType)
			return nullptr;
	}

	UniqPtr<Scope> fnScope(Scope::Create(paramTypeMap, {}, t, &s));
	assert(fnScope);

	UniqPtr<Expression> body(body_->Build(*fnScope, t, err));
	if (not body)
		return nullptr;

	const Type *result;
	if (resultType)
	{
		result = &resultType->referencedType();
	}
	else
	{
		result = &body->type();
	}

	const FunctionType& type = t.functionType(paramTypeVec, *result);

	return new Function(parameters, resultType, fnScope, body, type, source());
}


Function::Function(UniqPtrVec<Parameter>& params, UniqPtr<TypeReference>& resultType,
                   UniqPtr<Scope>& scope, UniqPtr<Expression>& body,
                   const FunctionType& type, const SourceRange& loc)
	: Expression(type, loc), HasParameters(std::move(params)),
	  explicitResultType_(std::move(resultType)), scope_(std::move(scope)),
	  body_(std::move(body))
{
}


const FunctionType& Function::type() const
{
	// We know that our type is a FunctionType because that's what we
	// passed to Expression() in the constructor.
	return dynamic_cast<const FunctionType&>(Expression::type());
}


void Function::PrettyPrint(Bytestream& out, size_t indent) const
{
	std::string tabs(indent, '\t');
	std::string intabs(indent + 1, '\t');

	out
		<< Bytestream::Action << "function"
		<< Bytestream::Operator << "("
		;

	const UniqPtrVec<Parameter>& params = parameters();
	size_t printed = 0;

	for (auto& p : params)
	{
		p->PrettyPrint(out, indent);
		if (++printed < params.size())
			out
				<< Bytestream::Operator << ", "
				<< Bytestream::Reset;
	}

	out
		<< Bytestream::Operator << "): "
		<< Bytestream::Reset
		<< type().returnType()
		<< "\n" << intabs
		;

	body_->PrettyPrint(out, indent + 1);

	out << Bytestream::Reset;
}


void Function::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		for (auto& p : parameters())
			p->Accept(v);

		body_->Accept(v);
	}

	v.Leave(*this);
}


dag::ValuePtr Function::evaluate(EvalContext& ctx) const
{
	SharedPtrVec<dag::Parameter> parameters;
	for (auto& p : this->parameters())
		parameters.emplace_back(p->evaluate(ctx));

	dag::Function::Evaluator eval =
		[=,&ctx](const dag::ValueMap& scope, const dag::ValueMap args,
	                 dag::DAGBuilder& /*builder*/, SourceRange)
	{
		//
		// When executing a function, we don't use symbols in scope
		// at the call site, only those in scope at the function
		// definition site.
		//
		// We will return to the original stack when the
		// `fnScope` object goes out of scope.
		//
		auto fnScope(ctx.ChangeScopeStack(scope));

		//
		// We evaluate the function with the given arguments by
		// putting default paramters and arguments into the local scope
		// and then evalating the function's CompoundExpr.
		//
		auto evalScope(ctx.EnterScope("function call evaluation"));

		for (auto& p : parameters)
			if (dag::ValuePtr v = p->defaultValue())
				evalScope.set(p->name(), v);

		for (auto& i : args)
			evalScope.set(i.first, i.second);

		return body().evaluate(ctx);
	};

	return ctx.Function(eval, parameters, type(), source());
}
