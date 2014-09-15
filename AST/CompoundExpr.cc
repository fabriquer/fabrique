/** @file AST/CompoundExpr.cc    Definition of @ref fabrique::ast::CompoundExpr. */
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

#include "AST/CompoundExpr.h"
#include "AST/Value.h"
#include "AST/Visitor.h"
#include "DAG/EvalContext.h"
#include "Support/Bytestream.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;


CompoundExpression::CompoundExpression(UniqPtr<Scope>&& scope,
                                       UniqPtr<Expression>& result,
                                       const SourceRange& loc)
	: Expression(result->type(), loc), Scope(std::move(*scope)),
	  result_(std::move(result))
{
	assert(result_);
}


void CompoundExpression::PrettyPrint(Bytestream& out, size_t indent) const
{
	std::string tabs(indent, '\t');
	std::string intabs(indent + 1, '\t');

	out << tabs << Bytestream::Operator << "{\n";
	for (auto& v : values())
	{
		v->PrettyPrint(out, indent + 1);
		out << "\n";
	}

	assert(result_);
	out
		<< intabs << *result_
		<< "\n" << Bytestream::Operator << tabs << "}"
		<< Bytestream::Reset
		;
}


void CompoundExpression::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		for (auto& val : values())
			val->Accept(v);

		result_->Accept(v);
	}

	v.Leave(*this);
}

dag::ValuePtr CompoundExpression::evaluate(dag::EvalContext& ctx) const
{
	auto scope(ctx.EnterScope("CompoundExpression"));

	for (auto& v : values())
		v->evaluate(ctx);

	return result_->evaluate(ctx);
}
