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

#include "AST/Parameter.h"
#include "AST/Visitor.h"
#include "DAG/Parameter.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"

#include <memory>

using namespace fabrique;
using namespace fabrique::ast;


Parameter::Parameter(UniqPtr<Identifier>& name, UniqPtr<TypeReference>& t,
                     UniqPtr<Expression>& defaultArgument)
	: Node(SourceRange::Over(name, defaultArgument), t->referencedType()),
	  name_(std::move(name)), type_(std::move(t)),
	  defaultArgument_(std::move(defaultArgument))
{
	if (name_->reservedName())
		throw SyntaxError("reserved name", name_->source());
}


void Parameter::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out << *name_;

	if (defaultArgument_)
		out
			<< Bytestream::Operator << " = "
			<< *defaultArgument_
			;
}


void Parameter::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		name_->Accept(v);
		if (defaultArgument_)
			defaultArgument_->Accept(v);
	}

	v.Leave(*this);
}


Parameter::Parser::~Parser()
{
}


Parameter* Parameter::Parser::Build(const Scope& s, TypeContext& t, Err& err)
{
	UniqPtr<Identifier> name(name_->Build(s, t, err));
	UniqPtr<TypeReference> type(type_->Build(s, t, err));
	if (not name or not type)
		return nullptr;

	UniqPtr<Expression> defaultArgument;
	if (defaultArgument_) {
		defaultArgument.reset(defaultArgument_->Build(s, t, err));
		if (not defaultArgument)
			return nullptr;
	}

	return new Parameter(name, type, defaultArgument);
}


std::shared_ptr<dag::Parameter> Parameter::evaluate(EvalContext& ctx) const
{
	dag::ValuePtr defaultArgument;
	if (defaultArgument_)
		defaultArgument = defaultArgument_->evaluate(ctx);

	return std::shared_ptr<dag::Parameter>(
		new dag::Parameter(name_->name(), type(), defaultArgument, source())
	);
}
