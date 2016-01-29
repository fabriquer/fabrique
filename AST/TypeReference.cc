/** @file AST/TypeReference.cc    Definition of @ref fabrique::ast::TypeReference. */
/*
 * Copyright (c) 2015 Jonathan Anderson
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

#include "AST/TypeReference.h"
#include "AST/Visitor.h"
#include "DAG/TypeReference.h"
#include "Types/Type.h"
#include "Types/TypeContext.h"

using namespace fabrique::ast;
using std::string;


TypeReference::Parser::~Parser()
{
}


TypeReference*
TypeReference::Parser::Build(const Scope& scope, TypeContext& types, Err& err)
{
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

	return new TypeReference(std::move(name), std::move(parameters),
	                         referencedType, src);
}


TypeReference::TypeReference(UniqPtr<Identifier> name, UniqPtrVec<TypeReference> params,
                             const Type& referencedType, SourceRange src)
	: Expression(referencedType.context().find("type", src), src),
	  name_(std::move(name)), parameters_(std::move(params)),
	  referencedType_(referencedType)
{
}

void TypeReference::PrettyPrint(Bytestream& out, size_t indent) const
{
	type().PrettyPrint(out, indent);
}

void TypeReference::Accept(Visitor& v) const
{
	v.Enter(type());
	v.Leave(type());
}

const fabrique::Type& TypeReference::referencedType() const
{
	return referencedType_;
}

fabrique::dag::ValuePtr TypeReference::evaluate(EvalContext&) const
{
	return dag::ValuePtr {
		dag::TypeReference::Create(type().context().nilType(), type(), source())
	};
}
