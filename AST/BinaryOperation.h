/** @file AST/BinaryOperation.h    Declaration of @ref fabrique::ast::BinaryOperation. */
/*
 * Copyright (c) 2013-2014 Jonathan Anderson
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

#include "ADT/UniqPtr.h"
#include "AST/Expression.h"

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
	enum class Operator
	{
		Add,
		Prefix,
		ScalarAdd,
		Invalid,

		// logical operators:
		And,
		Or,
		XOr,

		// comparitors:
		LessThan,
		GreaterThan,
		Equal,
		NotEqual,
	};

	static Operator Op(const std::string&);
	static std::string OpStr(Operator);

	Operator getOp() const { return op_; }
	const Expression& getLHS() const { return *lhs_; }
	const Expression& getRHS() const { return *rhs_; }

	virtual void PrettyPrint(Bytestream&, size_t indent = 0) const override;
	virtual void Accept(Visitor&) const override;

	virtual dag::ValuePtr evaluate(EvalContext&) const override;

protected:
	class Parser : public Expression::Parser
	{
	public:
		virtual ~Parser();

		virtual BinaryOperation*
		Build(const Scope&, TypeContext&, Err&) const override = 0;

	protected:
		BinaryOperation* Build(const Scope&, TypeContext&, Err&, Operator) const;

		ChildNodeParser<Expression> lhs_, rhs_;
	};

public:
	class And : public Parser
	{
	public:
		BinaryOperation* Build(const Scope&, TypeContext&, Err&) const override;
	};

	class Or : public Parser
	{
	public:
		BinaryOperation* Build(const Scope&, TypeContext&, Err&) const override;
	};

	class XOr : public Parser
	{
	public:
		BinaryOperation* Build(const Scope&, TypeContext&, Err&) const override;
	};

	class LessThan : public Parser
	{
	public:
		BinaryOperation* Build(const Scope&, TypeContext&, Err&) const override;
	};

	class GreaterThan : public Parser
	{
	public:
		BinaryOperation* Build(const Scope&, TypeContext&, Err&) const override;
	};

	class Equals : public Parser
	{
	public:
		BinaryOperation* Build(const Scope&, TypeContext&, Err&) const override;
	};

	class NotEqual : public Parser
	{
	public:
		BinaryOperation* Build(const Scope&, TypeContext&, Err&) const override;
	};

	class Add : public Parser
	{
	public:
		BinaryOperation* Build(const Scope&, TypeContext&, Err&) const override;
	};

	class Prefix : public Parser
	{
	public:
		BinaryOperation* Build(const Scope&, TypeContext&, Err&) const override;
	};

	class ScalarAdd : public Parser
	{
	public:
		BinaryOperation* Build(const Scope&, TypeContext&, Err&) const override;
	};

private:
	static BinaryOperation* Create(
		UniqPtr<Expression>&&, Operator, UniqPtr<Expression>&&);

	BinaryOperation(UniqPtr<Expression>&& lhs, UniqPtr<Expression>&& rhs,
	                enum Operator, const Type&, const SourceRange&);

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
