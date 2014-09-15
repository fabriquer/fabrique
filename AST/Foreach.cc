/** @file AST/Foreach.cc    Declaration of @ref fabrique::ast::ForeachExpr. */
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

#include "AST/Foreach.h"
#include "AST/Parameter.h"
#include "AST/Value.h"
#include "AST/Visitor.h"
#include "DAG/EvalContext.h"
#include "DAG/List.h"
#include "Support/Bytestream.h"
#include "Types/Type.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;


ForeachExpr::ForeachExpr(UniqPtr<Mapping>& mapping, UniqPtr<Expression>& body,
                         const Type& type, const SourceRange& source)
	: Expression(type, source),
	  mapping_(std::move(mapping)), body_(std::move(body))
{
}


void ForeachExpr::PrettyPrint(Bytestream& out, size_t indent) const
{
	out
		<< Bytestream::Operator << "foreach "
		<< *mapping_
		<< "\n"
		;

	body_->PrettyPrint(out, indent + 1);

	out << Bytestream::Reset;
}


void ForeachExpr::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		mapping_->Accept(v);
		body_->Accept(v);
	}

	v.Leave(*this);
}


dag::ValuePtr ForeachExpr::evaluate(dag::EvalContext& ctx) const
{
	SharedPtrVec<dag::Value> values;

	auto target = sourceSequence().evaluate(ctx);
	assert(target->type().isOrdered());
	assert(target->asList());

	//
	// For each input element, put its value in scope as the loop parameter
	// and then evaluate the CompoundExpression.
	//
	const ast::Parameter& loopParam = loopParameter();
	for (const dag::ValuePtr& element : *target->asList())
	{
		assert(element->type().isSubtype(loopParameter().type()));

		auto scope(ctx.EnterScope("foreach body"));
		scope.set(loopParam.getName().name(), element);

		dag::ValuePtr result = body_->evaluate(ctx);
		assert(result);
		assert(result->type().isSubtype(body_->type()));

		values.push_back(std::move(result));
	}

	return dag::ValuePtr(dag::List::of(values, source(), type().context()));
}
