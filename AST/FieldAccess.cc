/** @file AST/FieldAccess.cc    Definition of @ref fabrique::ast::FieldAccess. */
/*
 * Copyright (c) 2014 Jonathan Anderson
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

#include "AST/FieldAccess.h"
#include "AST/HasScope.h"
#include "AST/Identifier.h"
#include "AST/Scope.h"
#include "AST/Visitor.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;


FieldAccess::FieldAccess(UniqPtr<Expression>& base, UniqPtr<Identifier>& field)
	: Expression(SourceRange::Over(base, field)),
	  base_(std::move(base)), field_(std::move(field))
{
}


void FieldAccess::PrettyPrint(Bytestream& out, unsigned int /*indent*/) const
{
	out
		<< *base_
		<< Bytestream::Operator << "."
		<< Bytestream::Reference << field_->name()
		;
}


void FieldAccess::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		base_->Accept(v);
		field_->Accept(v);
	}

	v.Leave(*this);
}


dag::ValuePtr FieldAccess::evaluate(EvalContext& ctx) const
{
	dag::ValuePtr base = base_->evaluate(ctx);
	if (not base->hasFields())
		throw AssertionFailure("base->hasFields()",
			base->type().str() + " (" + typeid(base).name()
			+ ") should have fields");

	const std::string fieldName(field_->name());
	dag::ValuePtr field = base->field(fieldName);

	if (not field)
		throw AssertionFailure("field",
			base->type().str() + " (" + typeid(base).name()
			+ ") should have field '" + fieldName + "'");

	return field;
}
