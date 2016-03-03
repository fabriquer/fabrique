/** @file AST/UnaryOperation.cc    Definition of @ref fabrique::ast::UnaryOperation. */
/*
 * Copyright (c) 2014, 2016 Jonathan Anderson
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

#include "AST/UnaryOperation.h"
#include "AST/Visitor.h"
#include "Parsing/ErrorReporter.h"
#include "Support/Bytestream.h"
#include "Support/SourceLocation.h"
#include "Support/exceptions.h"
#include "Types/Type.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;


UnaryOperation::Parser::~Parser()
{
}


UnaryOperation*
UnaryOperation::Negative::Build(const Scope& scope, TypeContext& types, Err& err)
{
	UniqPtr<Expression> operand(operand_->Build(scope, types, err));
	if (not operand)
		return nullptr;

	const Type& t = operand->type();
	if (not t.isNumeric())
	{
		err.ReportError("cannot apply unary negative operator"
		                " to non-numeric type " + t.str(), source());
		return nullptr;
	}

	return new UnaryOperation(operand, Operator::Negative, t, source());
}


UnaryOperation*
UnaryOperation::Not::Build(const Scope& scope, TypeContext& types, Err& err)
{
	UniqPtr<Expression> operand(operand_->Build(scope, types, err));
	if (not operand)
		return nullptr;

	const Type& t = operand->type();
	if (not t.canBeNegated())
	{
		err.ReportError("cannot apply negation operator to " + t.str(), source());
		return nullptr;
	}

	return new UnaryOperation(operand, Operator::Not, t, source());
}


UnaryOperation*
UnaryOperation::Positive::Build(const Scope& scope, TypeContext& types, Err& err)
{
	UniqPtr<Expression> operand(operand_->Build(scope, types, err));
	if (not operand)
		return nullptr;

	const Type& t = operand->type();
	if (not t.isNumeric())
	{
		err.ReportError("cannot apply unary positive operator"
		                " to non-numeric type " + t.str(), source());
		return nullptr;
	}

	return new UnaryOperation(operand, Operator::Positive, t, source());
}


UnaryOperation::UnaryOperation(UniqPtr<Expression>& e, enum Operator op,
                               const Type& ty, const SourceRange& loc)
	: Expression(ty, loc), subexpr_(std::move(e)), op_(op)
{
}


std::string UnaryOperation::OpStr(Operator op)
{
	switch (op)
	{
		case Operator::Negative:          return "-";
		case Operator::Not:               return "not";
		case Operator::Positive:          return "+";
	}

	assert(false && "unhandled Operator type");
	return "";
}


void UnaryOperation::PrettyPrint(Bytestream& out, size_t indent) const
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
	assert(subexpr);

	switch (op_)
	{
		case Operator::Negative:
			assert(false && "unimplemented");
			break;

		case Operator::Not:
			return subexpr->Negate(source());

		case Operator::Positive:
			return subexpr;
	}

	assert(false && "unreachable");
}


fabrique::Bytestream&
fabrique::operator << (Bytestream& out, UnaryOperation::Operator op)
{
	out << UnaryOperation::OpStr(op);
	return out;
}
