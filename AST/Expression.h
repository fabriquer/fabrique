/** @file AST/Expression.h    Declaration of @ref fabrique::ast::Expression. */
/*
 * Copyright (c) 2013, 2018 Jonathan Anderson
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

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "ADT/PtrVec.h"
#include "AST/Node.h"
#include "DAG/Value.h"
#include "Support/ABI.h"
#include "Support/exceptions.h"

namespace fabrique {

namespace ast {

class EvalContext;

/**
 * Base class for expressions that can be evaluated.
 */
class Expression : public Node
{
public:
	virtual ~Expression() override;

	virtual dag::ValuePtr evaluate(EvalContext&) const  = 0;

	template<class T>
	std::shared_ptr<T> evaluateAs(EvalContext &ctx)
	{
		auto plainValue = evaluate(ctx);
		SemaCheck(plainValue, source(), "failed to evaluate");

		auto asSubtype = std::dynamic_pointer_cast<T>(plainValue);
		SemaCheck(asSubtype, plainValue->source(), "not a " + Demangle(typeid(T)));

		return asSubtype;
	}

protected:
	Expression(const SourceRange& src)
		: Node(src)
	{
	}
};

typedef PtrVec<Expression> ExprVec;

} // namespace ast
} // namespace fabrique

#endif
