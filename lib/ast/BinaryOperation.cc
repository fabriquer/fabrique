//! @file ast/BinaryOperation.cc    Definition of @ref fabrique::ast::BinaryOperation
/*
 * Copyright (c) 2013-2014, 2018, 2019 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme and at Memorial University
 * of Newfoundland under the NSERC Discovery program (RGPIN-2015-06048).
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

#include <fabrique/AssertionFailure.hh>
#include <fabrique/Bytestream.hh>
#include <fabrique/names.hh>
#include <fabrique/ast/BinaryOperation.hh>
#include <fabrique/ast/Visitor.hh>
#include <fabrique/types/TypeContext.hh>

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;
using std::function;
using std::shared_ptr;


namespace fabrique
{
	Type::TypesMapper AddElementTypeTo(const Type&);
}


BinaryOperation::BinaryOperation(UniqPtr<Expression> lhs, UniqPtr<Expression> rhs,
                                 enum Operator op, SourceRange src)
	: Expression(src), lhs_(std::move(lhs)), rhs_(std::move(rhs)), op_(op)
{
}


BinaryOperation::Operator BinaryOperation::Op(const std::string& o)
{
	Operator op;

	if (o == "/") op = Divide;
	else if (o == "*") op = Multiply;

	else if (o == "+") op = Add;
	else if (o == "::") op = Prefix;
	else if (o == "-") op = Subtract;

	else if (o == "==") op = Equal;
	else if (o == "!=") op = NotEqual;

	else if (o == names::And) op = And;
	else if (o == names::Or) op = Or;
	else if (o == names::XOr) op = Xor;

	else op = Invalid;

	FAB_ASSERT(o == OpStr(op), "symmetric binary operation check failed: '"
		+ o + "' vs '" + OpStr(op) + "'");

	return op;
}

std::string BinaryOperation::OpStr(Operator op)
{
	switch (op)
	{
		case Divide:            return "/";
		case Multiply:          return "*";

		case Add:               return "+";
		case Prefix:            return "::";
		case Subtract:          return "-";

		case Equal:             return "==";
		case NotEqual:          return "!=";

		case And:               return names::And;
		case Or:                return names::Or;
		case Xor:               return names::XOr;

		case Invalid:           FAB_ASSERT(false, "unreachable Invalid binary op");
	}

	FAB_ASSERT(false, "unreachable: invalid operator " + std::to_string(op));
}


void BinaryOperation::PrettyPrint(Bytestream& out, unsigned int indent) const
{
	lhs_->PrettyPrint(out, indent);
	out
		<< " "
		<< Bytestream::Operator << OpStr(op_)
		<< Bytestream::Reset
		<< " ";
	rhs_->PrettyPrint(out, indent);
}


void BinaryOperation::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		lhs_->Accept(v);
		rhs_->Accept(v);
	}

	v.Leave(*this);
}


dag::ValuePtr BinaryOperation::evaluate(EvalContext& ctx) const
{
	dag::ValuePtr lhs = lhs_->evaluate(ctx);
	dag::ValuePtr rhs = rhs_->evaluate(ctx);
	assert(lhs and rhs);

	SourceRange loc = source();

	switch (op_)
	{
		case Divide:    return lhs->DivideBy(rhs, loc);
		case Multiply:  return lhs->MultiplyBy(rhs, loc);

		case Add:       return lhs->Add(rhs, loc);
		case Prefix:    return rhs->PrefixWith(lhs, loc);
		case Subtract:  return rhs->Subtract(lhs, loc);

		case Equal:     return lhs->Equals(rhs, loc);
		case NotEqual:  return lhs->Equals(rhs, loc)->Not(loc);

		case And:       return lhs->And(rhs, loc);
		case Or:        return lhs->Or(rhs, loc);
		case Xor:       return lhs->Xor(rhs, loc);

		case Invalid:   throw SemanticException("invalid operation", loc);
	}

	assert(false && "unreachable");
}


fabrique::Bytestream&
fabrique::operator << (Bytestream& out, BinaryOperation::Operator op)
{
	out << BinaryOperation::OpStr(op);
	return out;
}



const fabrique::Type& BinaryOperation::ResultType(const Type& lhs, const Type& rhs,
                                                  Operator op, SourceRange& loc)
{
	switch (op)
	{
		case Divide:
		case Multiply:
			if (auto &t = lhs.onMultiply(rhs))
				return t;

			if (auto &t = rhs.onMultiply(lhs))
				return t;

			break;

		case Add:
		case Subtract:
			if (auto& t = lhs.onAddTo(rhs))
				return t;

			if (auto& t = rhs.onAddTo(lhs))
				return t;

			break;

		case Prefix:
			if (auto& t = rhs.onPrefixWith(lhs))
				return t;

			break;

		case And:
		case Or:
		case Xor:
		case Equal:
		case NotEqual:
			if (lhs == rhs)
				return lhs.context().booleanType();
			break;

		case Invalid:
			break;
	}

	throw SemanticException("incompatible types: "
                         + lhs.str() + " vs " + rhs.str(), loc);
}


namespace fabrique
{
	Type::TypesMapper AddElementTypeTo(const Type& t)
	{
		return [&](const PtrVec<Type>& params)
		{
			assert(params.size() == 1);
			const Type& elemTy = *params.front();
			return PtrVec<Type>(1, &elemTy.onAddTo(t));
		};
	}
}
