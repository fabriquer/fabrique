/** @file AST/FieldQuery.h    Declaration of @ref fabrique::ast::FieldQuery. */
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

#ifndef FIELD_QUERY_H
#define FIELD_QUERY_H

#include "ADT/UniqPtr.h"
#include "AST/Expression.h"

namespace fabrique {
namespace ast {

class Identifier;
class SymbolReference;


/**
 * An expression that imports a Fabrique module.
 */
class FieldQuery : public Expression
{
public:
	FieldQuery(UniqPtr<SymbolReference>& base, UniqPtr<Identifier>& field,
	           UniqPtr<Expression>& defaultValue, const Type&, SourceRange);

	const SymbolReference& base() const { return *base_; }
	const Identifier& field() const { return *field_; }
	const Expression& defaultValue() const { return *defaultValue_; }

	virtual void PrettyPrint(Bytestream&, size_t indent = 0) const override;
	virtual void Accept(Visitor&) const;

private:
	const UniqPtr<SymbolReference> base_;
	const UniqPtr<Identifier> field_;
	const UniqPtr<Expression> defaultValue_;
};

} // namespace ast
} // namespace fabrique

#endif
