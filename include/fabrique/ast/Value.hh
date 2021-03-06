//! @file ast/Value.hh    Declaration of @ref fabrique::ast::Value
/*
 * Copyright (c) 2013, 2016, 2018 Jonathan Anderson
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

#ifndef AST_VALUE_H
#define AST_VALUE_H

#include <fabrique/ast/Identifier.hh>
#include <fabrique/ast/TypeReference.hh>

namespace fabrique {
namespace ast {

/**
 * Base class for expressions that can be evaluated.
 */
class Value : public Expression
{
public:
	Value();
	Value(UniqPtr<Identifier>, UniqPtr<TypeReference>, UniqPtr<Expression>);

	const UniqPtr<Identifier>& name() const { return name_; }
	const UniqPtr<TypeReference>& explicitType() const { return explicitType_; }
	const Expression& value() const { return *value_; }

	virtual void PrettyPrint(Bytestream&, unsigned int indent = 0) const override;
	virtual void Accept(Visitor&) const override;

	virtual dag::ValuePtr evaluate(EvalContext&) const override;

private:
	const UniqPtr<Identifier> name_;
	const UniqPtr<TypeReference> explicitType_;
	const UniqPtr<Expression> value_;
};

} // namespace ast
} // namespace fabrique

#endif
