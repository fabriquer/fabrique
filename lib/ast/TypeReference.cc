/** @file AST/TypeReference.cc    Definition of @ref fabrique::ast::TypeReference. */
/*
 * Copyright (c) 2015-2017 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed at Memorial University of Newfoundland under
 * the NSERC Discovery program (RGPIN-2015-06048).
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
#include <fabrique/ast/TypeDeclaration.hh>
#include <fabrique/ast/TypeReference.hh>
#include <fabrique/ast/Visitor.hh>
#include <fabrique/dag/TypeReference.hh>
#include "Support/ABI.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include <fabrique/types/FunctionType.hh>
#include <fabrique/types/RecordType.hh>
#include <fabrique/types/TypeContext.hh>

using namespace fabrique::ast;
using std::string;


TypeReference::TypeReference(SourceRange source)
	: Expression(source)
{
}

TypeReference::~TypeReference()
{
}


SimpleTypeReference::SimpleTypeReference(UniqPtr<Identifier> name, SourceRange src)
	: TypeReference(src), name_(std::move(name))
{
}

fabrique::dag::ValuePtr SimpleTypeReference::evaluate(EvalContext& ctx) const
{
	// Some type names (e.g., `file` and `list` are special)
	if (name_->reservedName())
	{
		auto &t = ctx.types().find(name_->name());
		SemaCheck(t, source(), "invalid type");

		return dag::TypeReference::Create(t, source());
	}

	// Other type names must be user-defined syntactically
	return ctx.Lookup(name_->name(), source());
}

void SimpleTypeReference::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		name_->Accept(v);
	}

	v.Leave(*this);
}

void SimpleTypeReference::PrettyPrint(Bytestream& out, unsigned int indent) const
{
	name_->PrettyPrint(out, indent);
}

fabrique::dag::ValuePtr ParametricTypeReference::evaluate(EvalContext& ctx) const
{
	auto baseTypeRef = base_->evaluateAs<dag::TypeReference>(ctx);
	const string baseName = baseTypeRef->referencedType().name();

	PtrVec<Type> paramTypes;
	SemaCheck(not parameters_.empty(), source(), "no type parameters");

	for (auto &p : parameters_)
	{
		auto paramTypeRef = p->evaluateAs<dag::TypeReference>(ctx);
		paramTypes.emplace_back(&paramTypeRef->referencedType());
	}

	return dag::TypeReference::Create(
		ctx.types().find(baseName, paramTypes), source());
}

ParametricTypeReference::ParametricTypeReference(UniqPtr<TypeReference> base,
                                                 SourceRange src,
                                                 UniqPtrVec<TypeReference> params)
	: TypeReference(src), base_(std::move(base)), parameters_(std::move(params))
{
}

void ParametricTypeReference::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		base_->Accept(v);

		for (auto& p : parameters_)
		{
			p->Accept(v);
		}
	}

	v.Leave(*this);
}

void ParametricTypeReference::PrettyPrint(Bytestream& out, unsigned int indent) const
{
	base_->PrettyPrint(out, indent);
	out << Bytestream::Operator << '[' << Bytestream::Reset;
	for (size_t i = 0, length = parameters_.size(); i < length; i++)
	{
		parameters_[i]->PrettyPrint(out, indent);
		if (i < (length - 1))
		{
			out << Bytestream::Operator << ',' << Bytestream::Reset;
		}
	}
	out << Bytestream::Operator << ']' << Bytestream::Reset;
}


fabrique::dag::ValuePtr FunctionTypeReference::evaluate(EvalContext& ctx) const
{
	PtrVec<Type> paramTypes;
	for (auto &p : parameters_)
	{
		auto paramTypeRef = p->evaluateAs<dag::TypeReference>(ctx);
		paramTypes.emplace_back(&paramTypeRef->referencedType());
	}

	auto resultTypeRef = resultType_->evaluateAs<dag::TypeReference>(ctx);
	const Type& resultType = resultTypeRef->referencedType();

	return dag::TypeReference::Create(
		ctx.types().functionType(paramTypes, resultType),
		source()
	);
}


FunctionTypeReference::FunctionTypeReference(UniqPtrVec<TypeReference> params,
                                             UniqPtr<TypeReference> result,
                                             SourceRange source)
	: TypeReference(source), parameters_(std::move(params)),
	  resultType_(std::move(result))
{
}

void FunctionTypeReference::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		for (auto& p : parameters_)
			p->Accept(v);

		resultType_->Accept(v);
	}

	v.Leave(*this);
}

void FunctionTypeReference::PrettyPrint(Bytestream& out, unsigned int indent) const
{
	out << Bytestream::Operator << '(' << Bytestream::Reset;

	for (size_t i = 0, length = parameters_.size(); i < length; i++)
	{
		parameters_[i]->PrettyPrint(out, indent);
		if (i < (length - 1))
		{
			out << Bytestream::Operator << ',' << Bytestream::Reset;
		}
	}

	out << Bytestream::Operator << ")->" << Bytestream::Reset;
	resultType_->PrettyPrint(out, indent);
}

fabrique::dag::ValuePtr RecordTypeReference::evaluate(EvalContext& ctx) const
{
	Type::NamedTypeVec fieldTypes;
	for (auto& f : fieldTypes_)
	{
		const string& name = f.first->name();
		auto typeRef = f.second->evaluateAs<dag::TypeReference>(ctx);

		fieldTypes.emplace_back(name, typeRef->referencedType());
	}

	return dag::TypeReference::Create(ctx.types().recordType(fieldTypes), source());
}

RecordTypeReference::RecordTypeReference(NamedPtrVec<TypeReference> fieldTypes,
                                         SourceRange src)
	: TypeReference(src), fieldTypes_(std::move(fieldTypes))
{
}

void RecordTypeReference::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		for (auto& f : fieldTypes_)
			f.second->Accept(v);
	}

	v.Leave(*this);
}

void RecordTypeReference::PrettyPrint(Bytestream& out, unsigned int indent) const
{
	out
		<< Bytestream::Type << "record"
		<< Bytestream::Operator << '['
		<< Bytestream::Reset
		;

	for (size_t i = 0, length = fieldTypes_.size(); i < length; i++)
	{
		auto &field = fieldTypes_[i];

		field.first->PrettyPrint(out, indent);
		out << Bytestream::Operator << ':' << Bytestream::Reset;
		field.second->PrettyPrint(out, indent);

		if (i < (length - 1))
		{
			out << Bytestream::Operator << ", " << Bytestream::Reset;
		}
	}

	out << Bytestream::Operator << ']' << Bytestream::Reset;
}
