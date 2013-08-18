/** @file BinaryOperator.h    Declaration of @ref BinaryOperator. */
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

#ifndef BINARY_OPERATOR_H
#define BINARY_OPERATOR_H

#include "Expression.h"
#include "CStringRef.h"

#include <memory>


/**
 * Base class for expressions that can be evaluated.
 */
class BinaryOperator : public Expression
{
public:
	enum Operation
	{
		Concatenate,
		Prefix,
		ScalarAdd,
		Invalid,
	};

	static Operation Op(CStringRef);
	static std::string OpStr(Operation);

	static BinaryOperator* Take(Expression*, Operation, Expression*);

	virtual bool isStatic() const;
	virtual void PrettyPrint(std::ostream&, int indent = 0) const;

private:
	BinaryOperator(Expression *LHS, enum Operation op, Expression *RHS)
		: LHS(LHS), RHS(RHS), op(op)
	{
	}

	const std::auto_ptr<Expression> LHS;
	const std::auto_ptr<Expression> RHS;
	const Operation op;
};

#endif
