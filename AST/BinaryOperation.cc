/** @file AST/BinaryOperation.cc    Definition of @ref fabrique::ast::BinaryOperation. */
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

#include "AST/BinaryOperation.h"
#include "AST/Visitor.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Types/SequenceType.h"
#include "Types/TypeContext.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;
using std::function;
using std::shared_ptr;


namespace fabrique
{
	Type::TypesMapper AddElementTypeTo(const Type&);
}


BinaryOperation::Parser::~Parser()
{
}


BinaryOperation*
BinaryOperation::Parser::Build(const Scope& scope, TypeContext& types, Err& err,
                               Operator op) const
{
	UniqPtr<Expression> lhs(lhs_->Build(scope, types, err));
	UniqPtr<Expression> rhs(rhs_->Build(scope, types, err));

	if (not lhs or not rhs)
		return nullptr;

	return BinaryOperation::Create(std::move(lhs), op, std::move(rhs));
}


BinaryOperation*
BinaryOperation::And::Build(const Scope& scope, TypeContext& types, Err& err) const
{
	return Parser::Build(scope, types, err, Operator::And);
}

BinaryOperation*
BinaryOperation::Or::Build(const Scope& scope, TypeContext& types, Err& err) const
{
	return Parser::Build(scope, types, err, Operator::Or);
}

BinaryOperation*
BinaryOperation::XOr::Build(const Scope& scope, TypeContext& types, Err& err) const
{
	return Parser::Build(scope, types, err, Operator::XOr);
}


BinaryOperation*
BinaryOperation::LessThan::Build(const Scope& scope, TypeContext& types, Err& err) const
{
	return Parser::Build(scope, types, err, Operator::LessThan);
}

BinaryOperation*
BinaryOperation::GreaterThan::Build(const Scope& scope, TypeContext& types, Err& err) const
{
	return Parser::Build(scope, types, err, Operator::GreaterThan);
}

BinaryOperation*
BinaryOperation::Equals::Build(const Scope& scope, TypeContext& types, Err& err) const
{
	return Parser::Build(scope, types, err, Operator::Equal);
}

BinaryOperation*
BinaryOperation::NotEqual::Build(const Scope& scope, TypeContext& types, Err& err) const
{
	return Parser::Build(scope, types, err, Operator::NotEqual);
}


BinaryOperation*
BinaryOperation::Add::Build(const Scope& scope, TypeContext& types, Err& err) const
{
	return Parser::Build(scope, types, err, Operator::Add);
}


BinaryOperation*
BinaryOperation::Prefix::Build(const Scope& scope, TypeContext& types, Err& err) const
{
	return Parser::Build(scope, types, err, Operator::Prefix);
}


BinaryOperation*
BinaryOperation::ScalarAdd::Build(const Scope& scope, TypeContext& types, Err& err) const
{
	return Parser::Build(scope, types, err, Operator::ScalarAdd);
}


BinaryOperation* BinaryOperation::Create(UniqPtr<Expression>&& lhs,
                                         Operator op,
                                         UniqPtr<Expression>&& rhs)
{
	assert(lhs);
	assert(rhs);

	SourceRange loc = SourceRange::Over(lhs, rhs);
	const Type& resultType = ResultType(lhs->type(), rhs->type(), op, loc);

	return new BinaryOperation(
		std::move(lhs), std::move(rhs), op, resultType, loc);
}


BinaryOperation::BinaryOperation(
		UniqPtr<Expression>&& lhs, UniqPtr<Expression>&& rhs,
		enum Operator op, const Type& ty, const SourceRange& src)
	: Expression(ty, src), lhs_(std::move(lhs)), rhs_(std::move(rhs)),
	  op_(op)
{
}


BinaryOperation::Operator BinaryOperation::Op(const std::string& o)
{
	Operator op;

	if (o == "+") op = Operator::Add;
	else if (o == "::") op = Operator::Prefix;
	else if (o == ".+") op = Operator::ScalarAdd;
	else op = Operator::Invalid;

	assert(o == OpStr(op));

	return op;
}

std::string BinaryOperation::OpStr(Operator op)
{
	switch (op)
	{
		case Operator::Add:           return "+";
		case Operator::Prefix:        return "::";
		case Operator::ScalarAdd:     return ".+";
		case Operator::And:           return "and";
		case Operator::Or:            return "or";
		case Operator::XOr:           return "xor";
		case Operator::LessThan:      return "<";
		case Operator::GreaterThan:   return ">";
		case Operator::Equal:         return "==";
		case Operator::NotEqual:      return "!=";
		case Operator::Invalid:       assert(false && "op == Invalid");
	}

	assert(false && "unhandled Operator type");
	return "";
}


void BinaryOperation::PrettyPrint(Bytestream& out, size_t indent) const
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

	switch (op_)
	{
		case Operator::Add:          return lhs->Add(rhs);
		case Operator::Prefix:       return rhs->PrefixWith(lhs);
		case Operator::And:          return lhs->And(rhs);
		case Operator::Or:           return lhs->Or(rhs);
		case Operator::XOr:          return lhs->Xor(rhs);

		case Operator::LessThan:     return lhs->Equals(rhs);
		case Operator::GreaterThan:  return lhs->Equals(rhs);
		case Operator::Equal:        return lhs->Equals(rhs);
		case Operator::NotEqual:     return lhs->Equals(rhs)->Negate(source());

		case Operator::ScalarAdd:
			if (lhs->canScalarAdd(*rhs))
				return lhs->ScalarAdd(rhs);

			else if (rhs->canScalarAdd(*lhs))
				return rhs->ScalarAdd(lhs);

			else
				throw SemanticException(
					"invalid types for addition", source());

		case Operator::Invalid:
			throw SemanticException("invalid operation", source());
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
		case Operator::Add:
			if (auto& t = lhs.onAddTo(rhs))
				return t;

			if (auto& t = rhs.onAddTo(lhs))
				return t;

			break;

		case Operator::Prefix:
			if (auto& t = rhs.onPrefixWith(lhs))
				return t;

			break;

		case Operator::ScalarAdd:
		{
			if (lhs.isOrdered() and lhs.typeParamCount() == 1
			    and lhs[0].onAddTo(rhs))
			{
				return lhs.Map(AddElementTypeTo(rhs), loc);
			}

			if (rhs.isOrdered() and rhs.typeParamCount() == 1
			    and rhs[0].onAddTo(lhs))
			{
				return rhs.Map(AddElementTypeTo(lhs), loc);
			}

			break;
		}

		case Operator::And:
		case Operator::Or:
		case Operator::XOr:

		case Operator::LessThan:
		case Operator::GreaterThan:
		case Operator::Equal:
		case Operator::NotEqual:
			if (lhs == rhs)
				return lhs.context().booleanType();
			break;

		case Operator::Invalid:
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
