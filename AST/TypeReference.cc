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
#include "Types/RecordType.h"
#include "Types/Type.h"
#include "Types/TypeContext.h"

using namespace fabrique::ast;
using std::string;


TypeReference::FieldTypeParser::~FieldTypeParser()
{
}


TypeReference* TypeReference::FieldTypeParser::Build(const Scope&, TypeContext&, Err&)
{
	assert(false && "unreachable");
	return nullptr;
}



TypeReference::Parser::~Parser()
{
}


TypeReference*
TypeReference::Parser::Build(const Scope& scope, TypeContext& types, Err& err)
{
	if (not parameters_.empty())
	{
		return BuildParameterized(scope, types, err);
	}
	else if (name_)
	{
		return BuildSimpleType(scope, types, err);
	}
	else
	{
		return BuildRecordType(scope, types, err);
	}
}


TypeReference*
TypeReference::Parser::BuildSimpleType(const Scope& scope, TypeContext& types, Err& err)
{
	assert(name_);
	assert(fieldTypes_.empty());
	assert(parameters_.empty());

	UniqPtr<Identifier> name(name_->Build(scope, types, err));
	SourceRange src(name->source());


	// Is this a user-defined type declaration?
	const Type& userType = scope.Lookup(*name);
	const Type& type =
		userType
		? userType.lookupType()
		: types.find(name->name(), src)
		;

	return new TypeReference(std::move(name), type, src);
}


TypeReference*
TypeReference::Parser::BuildParameterized(const Scope& scope, TypeContext& types, Err& err)
{
	assert(name_);
	assert(fieldTypes_.empty());
	assert(not parameters_.empty());

	UniqPtr<Identifier> name(name_->Build(scope, types, err));
	SourceRange src(name->source());

	UniqPtrVec<TypeReference> parameters;
	PtrVec<Type> paramTypes;

	for (const UniqPtr<TypeReference::Parser>& p : parameters_)
	{
		if (p->source().end > src.end)
			src.end = p->source().end;

		UniqPtr<TypeReference> r(p->Build(scope, types, err));
		paramTypes.push_back(&r->referencedType());

		parameters.emplace_back(std::move(r));
	}

	const Type& referencedType = types.find(name->name(), src, paramTypes);

	return new TypeReference(std::move(name), referencedType, src,
	                         std::move(parameters));
}


TypeReference*
TypeReference::Parser::BuildRecordType(const Scope& scope, TypeContext& types, Err& err)
{
	assert(not name_);
	assert(parameters_.empty());

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
	return new TypeReference(std::move(fieldTypeRefs), type, source());
}


TypeReference::TypeReference(UniqPtr<Identifier> name, const Type& referencedType,
                             SourceRange src, UniqPtrVec<TypeReference> params)
	: Expression(referencedType.context().find("type", src), src),
	  name_(std::move(name)), parameters_(std::move(params)),
	  referencedType_(referencedType)
{
}

TypeReference::TypeReference(NamedPtrVec<TypeReference> fieldTypes,
                             const RecordType& referencedType, SourceRange src)
	: Expression(referencedType.context().find("type", src), src),
	  fieldTypes_(std::move(fieldTypes)), referencedType_(referencedType)
{
}

void TypeReference::PrettyPrint(Bytestream& out, size_t indent) const
{
	if (name_)
	{
		out << Bytestream::Type << *name_;

		if (not parameters_.empty())
		{
			out << Bytestream::Operator << "[";

			for (size_t i = 0; i < parameters_.size(); i++)
			{
				out << *parameters_[i];

				if ((i + 1) < parameters_.size())
				{
					out << Bytestream::Operator << ", ";
				}
			}

			out
				<< Bytestream::Operator << "]"
				<< Bytestream::Reset
				;
		}
	}
	else
	{
		referencedType_.PrettyPrint(out, indent);
	}
}

void TypeReference::Accept(Visitor& v) const
{
	v.Enter(type());
	v.Leave(type());
}

const fabrique::Type& TypeReference::referencedType() const
{
	if (referencedType_.isType())
		return dynamic_cast<const UserType&>(referencedType_).userType();

	return referencedType_;
}

fabrique::dag::ValuePtr TypeReference::evaluate(EvalContext&) const
{
	return dag::ValuePtr {
		dag::TypeReference::Create(type().context().nilType(), type(), source())
	};
}
