/** @file AST/FieldQuery.cc    Definition of @ref fabrique::ast::FieldQuery. */
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

#include "AST/FieldQuery.h"
#include "AST/Identifier.h"
#include "AST/Visitor.h"
#include "DAG/Record.h"
#include "Parsing/ErrorReporter.h"
#include "Support/Bytestream.h"
#include "Types/Type.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;


FieldQuery::Parser::~Parser()
{
}


FieldQuery* FieldQuery::Parser::Build(const Scope& scope, TypeContext& t, Err& err)
{
	UniqPtr<Expression> base(base_->Build(scope, t, err));
	UniqPtr<Identifier> field(field_->Build(scope, t, err));
	UniqPtr<Expression> defaultValue(default_->Build(scope, t, err));

	if (not base or not field or not defaultValue)
		return nullptr;

	if (not base->type().hasFields())
	{
		err.ReportError("value of type '" + base->type().str()
		                + "' does not have fields", source());
		return nullptr;
	}

	Type::TypeMap fieldTypes = base->type().fields();
	auto i = fieldTypes.find(field->name());

	const Type *type = &defaultValue->type();
	if (i != fieldTypes.end())
	{
		type = &type->supertype(i->second);
	}

	return new FieldQuery(base, field, defaultValue, *type, source());
}


FieldQuery::FieldQuery(UniqPtr<Expression>& base, UniqPtr<Identifier>& field,
                       UniqPtr<Expression>& defaultValue, const Type& ty, SourceRange src)
	: Expression(ty, src), base_(std::move(base)), field_(std::move(field)),
	  defaultValue_(std::move(defaultValue))
{
	assert(base_);
	assert(field_);
	assert(defaultValue_);
}


void FieldQuery::PrettyPrint(Bytestream& out, size_t indent) const
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
	const Type::TypeMap fields = base_->type().fields();
	const std::string fieldName(field_->name());

	auto i = fields.find(fieldName);
	if (i == fields.end())
	{
		// The field doesn't exist, use the default value.
		return defaultValue().evaluate(ctx);
	}

	std::shared_ptr<dag::Record> base =
		std::dynamic_pointer_cast<dag::Record>(base_->evaluate(ctx));
	assert(base);

	return base->field(fieldName);
}
