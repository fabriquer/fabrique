/** @file BinaryOperation.cc    Definition of @ref BinaryOperation. */
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

#include "AST/BinaryOperation.h"
#include "AST/Visitor.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Types/Type.h"

using namespace fabrique::ast;


BinaryOperation* BinaryOperation::Create(Expression *lhs,
                                         Operator op,
                                         Expression *rhs)
{
	assert(lhs);
	assert(rhs);

	SourceRange loc = SourceRange::Over(lhs, rhs);
	const Type &lt = lhs->getType(), &rt = rhs->getType();
	const Type *resultType = NULL;

	switch (op)
	{
		case Add:
			if (lt.isSubtype(rt) or rt.isSubtype(lt))
				resultType = &Type::GetSupertype(lt, rt);
			break;

		case Prefix:
			if (rt.isListOf(lt))
				resultType = &rt;
			break;

		case ScalarAdd:
			if (lt.isListOf(rt))
				resultType = &lt;

			else if (rt.isListOf(lt))
				resultType = &rt;
			break;

		case Invalid:
			break;
	}

	if (not resultType)
		throw SemanticException("incompatible types", loc);

	return new BinaryOperation(lhs, op, rhs, *resultType, loc);
}

BinaryOperation::Operator BinaryOperation::Op(CStringRef o)
{
	Operator op;

	if (o == "+") op = Add;
	else if (o == "::") op = Prefix;
	else if (o == ".+") op = ScalarAdd;
	else op = Invalid;

	assert(o == OpStr(op));

	return op;
}

std::string BinaryOperation::OpStr(Operator op)
{
	switch (op)
	{
		case Add:               return "+";
		case Prefix:            return "::";
		case ScalarAdd:         return ".+";
		case Invalid:           assert(false && "op == Invalid");
	}

	assert(false && "unhandled Operator type");
	return "";
}

bool BinaryOperation::isStatic() const
{
	return (LHS->isStatic() and RHS->isStatic());
}


void BinaryOperation::PrettyPrint(Bytestream& out, int indent) const
{
	LHS->PrettyPrint(out, indent);
	out
		<< " "
		<< Bytestream::Operator << OpStr(op)
		<< Bytestream::Reset
		<< " ";
	RHS->PrettyPrint(out, indent);
}


void BinaryOperation::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		LHS->Accept(v);
		RHS->Accept(v);
	}

	v.Leave(*this);
}


fabrique::Bytestream&
fabrique::operator << (Bytestream& out, BinaryOperation::Operator op)
{
	out << BinaryOperation::OpStr(op);
	return out;
}
