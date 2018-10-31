/** @file AST/ForeachExpr.cc    Declaration of @ref fabrique::ast::ForeachExpr. */
/*
 * Copyright (c) 2013-2014, 2018 Jonathan Anderson
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

#include <fabrique/ast/EvalContext.hh>
#include <fabrique/ast/Foreach.hh>
#include <fabrique/ast/Parameter.hh>
#include <fabrique/ast/Value.hh>
#include <fabrique/ast/Visitor.hh>
#include "DAG/List.h"
#include "Support/Bytestream.h"
#include "Types/Type.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;


ForeachExpr::ForeachExpr(UniqPtr<Identifier> loopVarName,
                         UniqPtr<TypeReference> explicitType,
                         UniqPtr<Expression> inputValue,
                         UniqPtr<Expression> body,
                         SourceRange source)
	: Expression(source), loopVarName_(std::move(loopVarName)),
	  explicitType_(std::move(explicitType)), inputValue_(std::move(inputValue)),
	  body_(std::move(body))
{
}


void ForeachExpr::PrettyPrint(Bytestream& out, unsigned int indent) const
{
	const std::string tabs(indent, '\t');

	out << Bytestream::Operator << "foreach " << Bytestream::Reset;
	loopVarName_->PrettyPrint(out, indent + 1);

	if (explicitType_)
	{
		out << Bytestream::Operator << ":" << Bytestream::Reset;
		explicitType_->PrettyPrint(out, indent + 1);
	}

	out << Bytestream::Operator << " <- " << Bytestream::Reset;

	inputValue_->PrettyPrint(out, indent + 1);

	out << " ";

	body_->PrettyPrint(out, indent);

	out << Bytestream::Reset;
}


void ForeachExpr::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		loopVarName_->Accept(v);
		if (explicitType_)
		{
			explicitType_->Accept(v);
		}
		body_->Accept(v);
	}

	v.Leave(*this);
}


dag::ValuePtr ForeachExpr::evaluate(EvalContext& ctx) const
{
	SharedPtrVec<dag::Value> values;

	auto target = sourceSequence().evaluate(ctx);
	SemaCheck(target->asList(), target->source(),
	          "cannot iterate over " + target->type().str());

	//
	// For each input element, put its value in scope as the loop parameter
	// and then evaluate the CompoundExpression.
	//
	auto loopVarName = loopVarName_->name();
	for (const dag::ValuePtr& element : *target->asList())
	{
		auto scope(ctx.EnterScope("foreach body"));
		scope.set(loopVarName, element);

		dag::ValuePtr result = body_->evaluate(ctx);
		assert(result);

		values.push_back(std::move(result));
	}

	return dag::ValuePtr(dag::List::of(values, source(), ctx.types()));
}
