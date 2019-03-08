/** @file AST/UnaryOperation.cc    Definition of @ref fabrique::ast::UnaryOperation. */
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

#include <fabrique/names.hh>
#include <fabrique/Bytestream.hh>
#include <fabrique/ast/UnaryOperation.hh>
#include <fabrique/ast/Visitor.hh>
#include "Support/exceptions.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;


UnaryOperation::UnaryOperation(UniqPtr<Expression> e, enum Operator op, SourceRange loc)
	: Expression(loc), subexpr_(std::move(e)), op_(op)
{
}


UnaryOperation::Operator UnaryOperation::Op(const std::string& o)
{
	Operator op;

	if (o == "+") op = Plus;
	else if (o == "-") op = Minus;
	else if (o == names::Not) op = LogicalNot;
	else op = Invalid;

	FAB_ASSERT(o == OpStr(op), "symmetric unary operation check failed");

	return op;
}

std::string UnaryOperation::OpStr(Operator op)
{
	switch (op)
	{
		case Plus:              return "+";
		case Minus:             return "-";
		case LogicalNot:        return names::Not;
		case Invalid:           FAB_ASSERT(false, "unreachable Invalid unary op");
	}

	FAB_ASSERT(false, "unreachable: invalid operator " + std::to_string(op));
}


void UnaryOperation::PrettyPrint(Bytestream& out, unsigned int indent) const
{
	out
		<< Bytestream::Operator << OpStr(op_)
		<< Bytestream::Reset << " "
		;

	subexpr_->PrettyPrint(out, indent);
}


void UnaryOperation::Accept(Visitor& v) const
{
	if (v.Enter(*this))
		subexpr_->Accept(v);

	v.Leave(*this);
}


dag::ValuePtr UnaryOperation::evaluate(EvalContext& ctx) const
{
	dag::ValuePtr subexpr = subexpr_->evaluate(ctx);
	SemaCheck(subexpr, subexpr_->source(), "invalid subexpression");

	switch (op_)
	{
		case ast::UnaryOperation::Plus:
			return subexpr;

		case ast::UnaryOperation::Minus:
			return subexpr->Negate(source());

		case ast::UnaryOperation::LogicalNot:
			return subexpr->Not(source());

		case ast::UnaryOperation::Invalid:
			throw SemanticException("Invalid operation", source());
	}

	FAB_ASSERT(false, "unreachable: invalid operator " + std::to_string(op_));
}


fabrique::Bytestream&
fabrique::operator << (Bytestream& out, UnaryOperation::Operator op)
{
	out << UnaryOperation::OpStr(op);
	return out;
}
