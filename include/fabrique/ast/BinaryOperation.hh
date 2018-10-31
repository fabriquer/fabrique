/** @file AST/BinaryOperation.h    Declaration of @ref fabrique::ast::BinaryOperation. */
/*
 * Copyright (c) 2013-2014, 2018 Jonathan Anderson
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

#include <fabrique/ast/Expression.hh>

#include <memory>

namespace fabrique {

class Bytestream;

namespace ast {

/**
 * An operation with two operands.
 */
class BinaryOperation : public Expression
{
public:
	enum Operator
	{
		// multiplication operators:
		Divide,
		Multiply,

		// addition operatiors:
		Add,
		Prefix,
		Subtract,

		// comparison operators:
		Equal,
		NotEqual,

		// logical operators:
		And,
		Or,
		Xor,

		Invalid,
	};

	BinaryOperation(UniqPtr<Expression> lhs, UniqPtr<Expression> rhs,
	                enum Operator, SourceRange);

	static Operator Op(const std::string&);
	static std::string OpStr(Operator);

	static BinaryOperation* Create(
		UniqPtr<Expression>&&, Operator, UniqPtr<Expression>&&);

	Operator getOp() const { return op_; }
	const Expression& getLHS() const { return *lhs_; }
	const Expression& getRHS() const { return *rhs_; }

	virtual void PrettyPrint(Bytestream&, unsigned int indent = 0) const override;
	virtual void Accept(Visitor&) const override;

	virtual dag::ValuePtr evaluate(EvalContext&) const override;

private:
	static const Type& ResultType(const Type& lhs, const Type& rhs,
                                      Operator, SourceRange&);

	const std::unique_ptr<Expression> lhs_;
	const std::unique_ptr<Expression> rhs_;
	const Operator op_;
};

} // namespace ast

Bytestream& operator << (Bytestream&, enum ast::BinaryOperation::Operator);

} // namespace fabrique

#endif
