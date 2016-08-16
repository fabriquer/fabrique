/** @file AST/ForeachExpr.h    Declaration of @ref fabrique::ast::ForeachExpr. */
/*
 * Copyright (c) 2013-2014, 2016 Jonathan Anderson
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

#ifndef FOREACH_H
#define FOREACH_H

#include "AST/CompoundExpr.h"
#include "AST/Mapping.h"
#include "AST/Parameter.h"

namespace fabrique {

class Type;

namespace ast {

class Value;


/**
 * An expression that maps list elements into another list.
 */
class ForeachExpr : public Expression
{
public:
	virtual void PrettyPrint(Bytestream&, size_t indent = 0) const override;
	virtual void Accept(Visitor&) const override;

	virtual dag::ValuePtr evaluate(EvalContext&) const override;

	class Parser : public Expression::Parser
	{
	public:
		virtual ~Parser();
		ForeachExpr* Build(const Scope&, TypeContext&, Err&) override;

	private:
		ChildNodeParser<Identifier> loopVariable_;
		ChildNodeParser<TypeReference, true> explicitType_;
		ChildNodeParser<Expression> sourceValue_;
		ChildNodeParser<Expression> body_;
	};

private:
	ForeachExpr(UniqPtr<Identifier>& loopVar, UniqPtr<TypeReference>& explicitType,
	            UniqPtr<Expression>& source, UniqPtr<Scope>& scope,
	            UniqPtr<Expression>& body, const Type&, const SourceRange&);

	const UniqPtr<Identifier> loopVariable_;
	const UniqPtr<TypeReference> explicitType_;
	const UniqPtr<Expression> sourceValue_;
	const UniqPtr<Scope> scope_;
	const UniqPtr<Expression> body_;
};

} // namespace ast
} // namespace fabrique

#endif
