/** @file AST/Parameter.cc    Definition of @ref fabrique::ast::Parameter. */
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

#include <fabrique/Bytestream.hh>
#include <fabrique/ast/Parameter.hh>
#include <fabrique/ast/Visitor.hh>
#include <fabrique/ast/SyntaxError.hh>
#include <fabrique/dag/Parameter.hh>
#include <fabrique/dag/TypeReference.hh>

#include <memory>

using namespace fabrique;
using namespace fabrique::ast;


Parameter::Parameter(UniqPtr<Identifier> name, UniqPtr<TypeReference> type,
                     UniqPtr<Expression> defaultValue)
	: Node(SourceRange::Over(name, defaultValue)),
	  name_(std::move(name)), type_(std::move(type)),
	  defaultValue_(std::move(defaultValue))
{
	if (name_->reservedName())
		throw SyntaxError("reserved name", name_->source());
}


void Parameter::PrettyPrint(Bytestream& out, unsigned int indent) const
{
	out
		<< Bytestream::Definition << *name_
		<< Bytestream::Operator << ":"
		<< Bytestream::Reset
		;

	type_->PrettyPrint(out, indent + 1);

	if (defaultValue_)
	{
		out << Bytestream::Operator << " = ";
		defaultValue_->PrettyPrint(out, indent + 1);
	}
}


void Parameter::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		name_->Accept(v);
		if (defaultValue_)
			defaultValue_->Accept(v);
	}

	v.Leave(*this);
}

std::shared_ptr<dag::Parameter> Parameter::evaluate(EvalContext& ctx) const
{
	dag::ValuePtr defaultValue;
	if (defaultValue_)
		defaultValue = defaultValue_->evaluate(ctx);

	auto &type = type_->evaluateAs<dag::TypeReference>(ctx)->referencedType();

	return std::shared_ptr<dag::Parameter>(
		new dag::Parameter(name_->name(), type, defaultValue, source()));
}
