/** @file AST/Function.cc    Definition of @ref fabrique::ast::Function. */
/*
 * Copyright (c) 2013, 2018 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme and at Memorial University
 * of Newfoundland under the NSERC Discovery program (RGPIN-2015-06048).
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

#include <fabrique/ast/CompoundExpr.hh>
#include <fabrique/ast/EvalContext.hh>
#include <fabrique/ast/Function.hh>
#include <fabrique/ast/Parameter.hh>
#include <fabrique/ast/Value.hh>
#include <fabrique/ast/Visitor.hh>
#include "DAG/Function.h"
#include "DAG/Parameter.h"
#include "DAG/TypeReference.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Types/FunctionType.h"
#include "Types/TypeContext.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;


Function::Function(UniqPtrVec<Parameter> params, UniqPtr<TypeReference> resultType,
                   UniqPtr<Expression> body, SourceRange src)
	: Expression(std::move(src)), HasParameters(params),
	  resultType_(std::move(resultType)), body_(std::move(body))
{
}


void Function::PrettyPrint(Bytestream& out, unsigned int indent) const
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
		out << *p;
		if (++printed < params.size())
			out
				<< Bytestream::Operator << ", "
				<< Bytestream::Reset;
	}

	out << Bytestream::Operator << "): " << Bytestream::Reset;

	resultType_->PrettyPrint(out, indent + 1);

	out << "\n";

	body_->PrettyPrint(out, indent);

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
	{
		parameters.emplace_back(p->evaluate(ctx));
	}

	auto ret = resultType_->evaluateAs<dag::TypeReference>(ctx);

	//
	// When executing a function, we don't use symbols in scope
	// at the call site, only those in scope at the function
	// definition site.
	//
	// TODO: make scoping work again!
	//
	//auto fnScope(ctx.ChangeScopeStack(scope));

	dag::Function::Evaluator eval =
		[=,&ctx](const dag::ValueMap args, dag::DAGBuilder&, SourceRange)
	{
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

	return ctx.Function(eval, parameters, ret->referencedType(), source());
}
