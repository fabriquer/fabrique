/** @file AST/TypeReference.cc    Definition of @ref fabrique::ast::TypeReference. */
/*
 * Copyright (c) 2015-2016 Jonathan Anderson
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

#include "AST/Scope.h"
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
using std::string;


TypeReference::TypeReference(const Type& t, SourceRange source)
	: Expression(t.context().find("type", source), source), referencedType_(t)
{
}

TypeReference::~TypeReference()
{
}

const fabrique::Type& TypeReference::referencedType() const
{
	if (referencedType_.isType())
		return dynamic_cast<const UserType&>(referencedType_).userType();

	return referencedType_;
}

void TypeReference::PrettyPrint(Bytestream& out, size_t indent) const
{
	referencedType_.PrettyPrint(out, indent);
}

TypeReference::Parser::~Parser()
{
}


SimpleTypeReference*
SimpleTypeReference::Parser::Build(const Scope& scope, TypeContext& types, Err& err)
{
	UniqPtr<Identifier> name(name_->Build(scope, types, err));
	if (not name)
		return nullptr;

	SourceRange src(name->source());

	// Is this a user-defined type declaration?
	const Type& userType = scope.Lookup(*name);
	const Type& type =
		userType
		? userType.lookupType()
		: types.find(name->name(), src)
		;

	return new SimpleTypeReference(std::move(name), type, src);
}

SimpleTypeReference::SimpleTypeReference(UniqPtr<Identifier> name, const Type& t,
                                         SourceRange src)
	: TypeReference(t, src), name_(std::move(name))
{
}

void SimpleTypeReference::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		name_->Accept(v);
	}

	v.Leave(*this);
}


ParametricTypeReference*
ParametricTypeReference::Parser::Build(const Scope& scope, TypeContext& t, Err& err)
{
	assert(not types_.empty());
	UniqPtr<TypeReference::Parser> baseParser = std::move(types_.front());
	types_.pop_front();

	UniqPtr<TypeReference> base(baseParser->Build(scope, t, err));
	if (not base)
		return nullptr;

	assert(not types_.empty());

	UniqPtrVec<TypeReference> parameters;
	PtrVec<Type> paramTypes;

	for (const UniqPtr<TypeReference::Parser>& p : types_)
	{
		UniqPtr<TypeReference> r(p->Build(scope, t, err));
		paramTypes.push_back(&r->referencedType());

		parameters.emplace_back(std::move(r));
	}

	const string baseName = base->referencedType().name();
	const Type& referencedType = t.find(baseName, source(), paramTypes);

	return new ParametricTypeReference(std::move(base), referencedType, source(),
	                                   std::move(parameters));
}


ParametricTypeReference::ParametricTypeReference(UniqPtr<TypeReference> base,
                                                 const Type& t, SourceRange src,
                                                 UniqPtrVec<TypeReference> params)
	: TypeReference(t, src), base_(std::move(base)), parameters_(std::move(params))
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


FunctionTypeReference*
FunctionTypeReference::Parser::Build(const Scope& s, TypeContext& t, Err& err)
{
	// The result type is the final reference in the types_ vector.
	assert(not types_.empty());
	UniqPtr<TypeReference::Parser> resultParser = std::move(types_.back());
	types_.pop_back();

	UniqPtr<TypeReference> result(resultParser->Build(s, t, err));
	if (not result)
		return nullptr;

	UniqPtrVec<TypeReference> parameters;
	PtrVec<Type> paramTypes;
	for (auto& typeParam : types_)
	{
		UniqPtr<TypeReference> param(typeParam->Build(s, t, err));
		if (not param)
			return nullptr;

		paramTypes.push_back(&param->referencedType());
		parameters.push_back(std::move(param));
	}

	const FunctionType& type = t.functionType(paramTypes, result->referencedType());

	return new FunctionTypeReference(std::move(parameters), std::move(result),
	                                 type, source());
}

FunctionTypeReference::FunctionTypeReference(UniqPtrVec<TypeReference> params,
                                             UniqPtr<TypeReference> result,
                                             const FunctionType& type, SourceRange source)
	: TypeReference(type, source),
	  parameters_(std::move(params)), resultType_(std::move(result))
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


TypeReference*
RecordTypeReference::FieldTypeParser::Build(const Scope&, TypeContext&, Err&)
{
	assert(false && "unreachable");
	return nullptr;
}

RecordTypeReference*
RecordTypeReference::Parser::Build(const Scope& scope, TypeContext& types, Err& err)
{
	Type::NamedTypeVec fieldTypes;
	NamedPtrVec<TypeReference> fieldTypeRefs;

	for (auto& f : fieldTypes_)
	{
		UniqPtr<Identifier> name(f->name_->Build(scope, types, err));
		UniqPtr<TypeReference> type(f->type_->Build(scope, types, err));

		if (not name or not type)
			return nullptr;

		fieldTypes.emplace_back(name->name(), type->referencedType());
		fieldTypeRefs.emplace_back(std::move(name), std::move(type));
	}

	const RecordType& type = types.recordType(fieldTypes);
	return new RecordTypeReference(std::move(fieldTypeRefs), type, source());
}

RecordTypeReference::RecordTypeReference(NamedPtrVec<TypeReference> fieldTypes,
                                         const RecordType& type, SourceRange src)
	: TypeReference(type, src), fieldTypes_(std::move(fieldTypes))
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


fabrique::dag::ValuePtr TypeReference::evaluate(EvalContext&) const
{
	return dag::ValuePtr {
		dag::TypeReference::Create(referencedType(), type(), source())
	};
}
