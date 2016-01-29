/** @file AST/TypeReference.h    Declaration of @ref fabrique::ast::TypeReference. */
/*
 * Copyright (c) 2015, 2016 Jonathan Anderson
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

#ifndef TYPE_REFERENCE_H
#define TYPE_REFERENCE_H

#include "Expression.h"
#include "Identifier.h"

namespace fabrique {
namespace ast {

/**
 * A reference to a named type.
 */
class TypeReference : public Expression
{
public:
	virtual void PrettyPrint(Bytestream&, size_t indent = 0) const override;
	virtual void Accept(Visitor&) const override;

	const Type& referencedType() const;

	virtual dag::ValuePtr evaluate(EvalContext&) const override;


	class Parser : public Node::Parser
	{
	public:
		virtual ~Parser();
		TypeReference* Build(const Scope&, TypeContext&, Err&) override;

	private:
		ChildNodeParser<Identifier> name_;
		ChildNodes<TypeReference> parameters_;
	};

private:
	TypeReference(UniqPtr<Identifier> name, UniqPtrVec<TypeReference> parameters,
	              const Type& referencedType, SourceRange);

	UniqPtr<Identifier> name_;
	UniqPtrVec<TypeReference> parameters_;
	const Type& referencedType_;
};

} // namespace ast
} // namespace fabrique

#endif
