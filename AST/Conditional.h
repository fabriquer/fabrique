/** @file Conditional.h    Declaration of @ref Conditional. */
/*
 * Copyright (c) 2013 Jonathan Anderson
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

#ifndef CONDITIONAL_H
#define CONDITIONAL_H

#include "Expression.h"

namespace fabrique {
namespace ast {

/**
 * A function allows users to create build abstractions.
 */
class Conditional : public Expression
{
public:
	Conditional(const SourceRange& ifLoc, Expression *condition,
	            Expression *thenResult, Expression *elseResult,
	            const Type& resultTy)
		: Expression(resultTy,
		             SourceRange(ifLoc.begin,
		                         elseResult->getSource().end)),
		  condition(condition),
		  thenResult(thenResult),
		  elseResult(elseResult)
	{
	}

	virtual bool isStatic() const;
	virtual void PrettyPrint(Bytestream&, int indent = 0) const;

	virtual void Accept(Visitor&) const;

private:
	std::auto_ptr<Expression> condition;
	std::auto_ptr<Expression> thenResult;
	std::auto_ptr<Expression> elseResult;
};

} // namespace ast
} // namespace fabrique

#endif
