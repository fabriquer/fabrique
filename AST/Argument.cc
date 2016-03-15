/** @file AST/Argument.cc    Definition of @ref fabrique::ast::Argument. */
/*
 * Copyright (c) 2013, 2016 Jonathan Anderson
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

#include "AST/Argument.h"
#include "AST/Visitor.h"
#include "Support/Bytestream.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;



Argument::Parser::~Parser()
{
}

Argument* Argument::Parser::Build(const Scope& scope, TypeContext& types, Err& err)
{
	UniqPtr<Identifier> name;
	if (name_)
	{
		name.reset(name_->Build(scope, types, err));
	}

	UniqPtr<Expression> value(value_->Build(scope, types, err));
	if ((name_ and not name) or not value)
	{
		return nullptr;
	}

	return new Argument(name, value);
}

Argument::Argument(UniqPtr<Identifier>& id, UniqPtr<Expression>& value)
	: Expression(value->type(), SourceRange::Over(id, value)),
	  name_(std::move(id)), value_(std::move(value))
{
	assert(value_);
}


void Argument::PrettyPrint(Bytestream& out, size_t indent) const
{
	if (name_)
		out
			<< Bytestream::Definition << name_->name()
			<< Bytestream::Operator << " = "
			;

	assert(this->value_);
	value_->PrettyPrint(out, indent);
}


void Argument::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		if (name_)
			name_->Accept(v);

		value_->Accept(v);
	}

	v.Leave(*this);
}

dag::ValuePtr Argument::evaluate(EvalContext& ctx) const
{
	return value_->evaluate(ctx);
}
