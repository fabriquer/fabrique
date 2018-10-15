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

#include "AST/EvalContext.h"
#include "AST/TypeDeclaration.h"
#include "AST/TypeReference.h"
#include "AST/Visitor.h"
#include "DAG/TypeReference.h"
#include "Parsing/ErrorReporter.h"
#include "Support/ABI.h"
#include "Support/Bytestream.h"
#include "Types/FunctionType.h"
#include "Types/RecordType.h"
#include "Types/Type.h"
#include "Types/TypeContext.h"

using namespace fabrique::ast;
using std::dynamic_pointer_cast;
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
	if (auto userDefined = ctx.Lookup(name_->name()))
	{
		return userDefined;
	}

	return dag::TypeReference::Create(ctx.types().find(name_->name()), source());
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
	dag::ValuePtr base = base_->evaluate(ctx);
	auto baseRef = dynamic_pointer_cast<dag::TypeReference>(base);
	const string baseName = baseRef->referencedType().name();

	PtrVec<Type> paramTypes;
	assert(not parameters_.empty());
	for (auto &p : parameters_)
	{
		paramTypes.emplace_back(&p->evaluate(ctx)->type());
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
		auto tr = dynamic_pointer_cast<dag::TypeReference>(p->evaluate(ctx));
		assert(tr);
		paramTypes.emplace_back(&tr->referencedType());
	}

	auto tr = dynamic_pointer_cast<dag::TypeReference>(resultType_->evaluate(ctx));
	assert(tr);
	const Type& resultType = tr->referencedType();

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

	out << Bytestream::Operator << ")=>" << Bytestream::Reset;
	resultType_->PrettyPrint(out, indent);
}

fabrique::dag::ValuePtr RecordTypeReference::evaluate(EvalContext& ctx) const
{
	Type::NamedTypeVec fieldTypes;
	for (auto& f : fieldTypes_)
	{
		const string& name = f.first->name();

		dag::ValuePtr v = f.second->evaluate(ctx);
		auto tr = dynamic_pointer_cast<dag::TypeReference>(v);

		fieldTypes.emplace_back(name, tr->referencedType());
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
			out << Bytestream::Operator << ',' << Bytestream::Reset;
		}
	}

	out << Bytestream::Operator << ']' << Bytestream::Reset;
}
