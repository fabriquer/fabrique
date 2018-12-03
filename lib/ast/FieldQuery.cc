/** @file AST/FieldQuery.cc    Definition of @ref fabrique::ast::FieldQuery. */
/*
 * Copyright (c) 2014, 2018 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme and at Memorial University
 * of Newfoundland under the NSERC Discovery program (RGPIN-2015-06048).
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

#include <fabrique/Bytestream.hh>
#include <fabrique/ast/FieldQuery.hh>
#include <fabrique/ast/Identifier.hh>
#include <fabrique/ast/Visitor.hh>
#include <fabrique/dag/Record.hh>
#include <fabrique/types/Type.hh>

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;


FieldQuery::FieldQuery(UniqPtr<Expression> base, UniqPtr<Identifier> field,
                       UniqPtr<Expression> defaultValue, SourceRange src)
	: Expression(src), base_(std::move(base)), field_(std::move(field)),
	  defaultValue_(std::move(defaultValue))
{
	SemaCheck(base_, src, "invalid base");
	SemaCheck(field_, src, "invalid field");
	SemaCheck(defaultValue_, src, "invalid default value");
}


void FieldQuery::PrettyPrint(Bytestream& out, unsigned int indent) const
{
	out
		<< *base_
		<< Bytestream::Operator << "."
		<< Bytestream::Reference << field_->name()
		<< Bytestream::Operator << " ? "
		;

	defaultValue_->PrettyPrint(out, indent + 1);
}


void FieldQuery::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		base_->Accept(v);
		field_->Accept(v);
	}

	v.Leave(*this);
}


dag::ValuePtr FieldQuery::evaluate(EvalContext& ctx) const
{
	auto base = base_->evaluate(ctx);
	SemaCheck(base->hasFields(), source(), "no fields in " + base->type().str());

	if (auto result = base->field(field_->name()))
	{
		return result;
	}

	return defaultValue().evaluate(ctx);
}
