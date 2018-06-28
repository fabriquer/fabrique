/** @file AST/TypeReference.h    Declaration of @ref fabrique::ast::TypeReference. */
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
	virtual ~TypeReference() override;

protected:
	TypeReference(SourceRange);
};


//! A simple type is just a name, e.g., `int`.
class SimpleTypeReference : public TypeReference
{
public:
	virtual dag::ValuePtr evaluate(EvalContext&) const override;

	virtual void Accept(Visitor&) const override;
	void PrettyPrint(Bytestream&, unsigned int indent) const override;

private:
	SimpleTypeReference(UniqPtr<Identifier> name, SourceRange);

	UniqPtr<Identifier> name_;
};


/**
 * A parameterized type has a base type and parameters.
 *
 * ```fab
 * list[int]
 * ```
 */
class ParametricTypeReference : public TypeReference
{
public:
	virtual dag::ValuePtr evaluate(EvalContext&) const override;

	virtual void Accept(Visitor&) const override;
	void PrettyPrint(Bytestream&, unsigned int indent) const override;

private:
	ParametricTypeReference(UniqPtr<TypeReference> base, SourceRange,
	                        UniqPtrVec<TypeReference> parameters);

	UniqPtr<TypeReference> base_;
	UniqPtrVec<TypeReference> parameters_;
};


/**
 * The type of something that can be called (an action or a function).
 *
 * ```fab
 * (x:int, y:string, z:list[file])=>list[file]
 * ```
 */
class FunctionTypeReference : public TypeReference
{
public:
	virtual dag::ValuePtr evaluate(EvalContext&) const override;

	virtual void Accept(Visitor&) const override;
	void PrettyPrint(Bytestream&, unsigned int indent) const override;

private:
	FunctionTypeReference(UniqPtrVec<TypeReference> params,
	                      UniqPtr<TypeReference> result, SourceRange);

	UniqPtrVec<TypeReference> parameters_;
	UniqPtr<TypeReference> resultType_;
};


/**
 * The type of a record (unordered structure).
 *
 * ```fab
 * record[x:int, y:string, z:list[file]]
 * ```
 */
class RecordTypeReference : public TypeReference
{
public:
	virtual dag::ValuePtr evaluate(EvalContext&) const override;

	virtual void Accept(Visitor&) const override;
	void PrettyPrint(Bytestream&, unsigned int indent) const override;

private:
	RecordTypeReference(NamedPtrVec<TypeReference>, SourceRange);

	NamedPtrVec<TypeReference> fieldTypes_;
};


} // namespace ast
} // namespace fabrique

#endif
