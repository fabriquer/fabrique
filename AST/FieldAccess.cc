/** @file AST/FieldAccess.cc    Definition of @ref fabrique::ast::FieldAccess. */
/*
 * Copyright (c) 2014, 2016 Jonathan Anderson
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
#include "Parsing/ErrorReporter.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;


FieldAccess::Parser::~Parser()
{
}


FieldAccess* FieldAccess::Parser::Build(const Scope& scope, TypeContext& t, Err& err)
{
	UniqPtr<Expression> base(base_->Build(scope, t, err));
	if (not base)
		return nullptr;

	UniqPtrVec<Identifier> fields;
	assert(not fields_.empty());

	const Type *type = &base->type();
	for (auto& f : fields_)
	{
		UniqPtr<Identifier> field(f->Build(scope, t, err));
		if (not f)
			return nullptr;

		if (not type->hasFields())
		{
			err.ReportError("value of type '" + type->str()
			                + "' does not have fields",
			                SourceRange(*base, *field));
			return nullptr;
		}

		Type::TypeMap fieldTypes = type->fields();
		auto i = fieldTypes.find(field->name());
		if (i == fieldTypes.end())
		{
			err.ReportError("no such field", SourceRange(*base, *field));
			return nullptr;
		}

		type = &i->second;
		fields.emplace_back(std::move(field));
	}

	return new FieldAccess(base, fields, *type, source());
}


FieldAccess::FieldAccess(UniqPtr<Expression>& base, UniqPtrVec<Identifier>& fields,
                         const Type& type, SourceRange src)
	: Expression(type, src), base_(std::move(base)), fields_(std::move(fields))
{
}


void FieldAccess::PrettyPrint(Bytestream& out, size_t indent) const
{
	base_->PrettyPrint(out, indent);

	for (auto& f : fields_)
	{
		out
			<< Bytestream::Operator << "."
			<< Bytestream::Reference << f->name()
			;
	}
}


void FieldAccess::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		base_->Accept(v);

		for (auto& f : fields_)
		{
			f->Accept(v);
		}
	}

	v.Leave(*this);
}


dag::ValuePtr FieldAccess::evaluate(EvalContext& ctx) const
{
	dag::ValuePtr val = base_->evaluate(ctx);

	for (auto& f : fields_)
	{
		if (not val->hasFields())
			throw AssertionFailure("val->hasFields()",
				val->type().str() + " (" + typeid(val).name()
				+ ") should have fields");

		const std::string fieldName(f->name());
		dag::ValuePtr field = val->field(fieldName);

		if (not field)
			throw AssertionFailure("field",
				val->type().str() + " (" + typeid(val).name()
				+ ") should have field '" + fieldName + "'");

		val = field;
	}

	return val;
}
