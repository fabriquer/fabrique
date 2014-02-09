/** @file UnaryOperation.cc    Definition of @ref UnaryOperation. */
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

#include "AST/UnaryOperation.h"
#include "AST/Visitor.h"
#include "Support/Bytestream.h"
#include "Support/Location.h"
#include "Support/exceptions.h"
#include "Types/Type.h"

using namespace fabrique::ast;


UnaryOperation* UnaryOperation::Create(Operator op, const SourceRange& opLoc,
                                       Expression *e)
{
	assert(e);

	SourceRange loc = SourceRange::Over(opLoc, e->getSource());
	return new UnaryOperation(e, op, e->type(), loc);
}

UnaryOperation::UnaryOperation(Expression *e, enum Operator op,
                               const Type& ty, const SourceRange& loc)
	: Expression(ty, loc), subexpr(e), op(op)
{
}


UnaryOperation::Operator UnaryOperation::Op(const std::string& o)
{
	Operator op;

	if (o == "not") op = Negate;
	else op = Invalid;

	assert(o == OpStr(op));

	return op;
}

std::string UnaryOperation::OpStr(Operator op)
{
	switch (op)
	{
		case Negate:            return "not";
		case Invalid:           assert(false && "op == Invalid");
	}

	assert(false && "unhandled Operator type");
	return "";
}


void UnaryOperation::PrettyPrint(Bytestream& out, int indent) const
{
	out
		<< Bytestream::Operator << OpStr(op)
		<< Bytestream::Reset << " "
		;

	subexpr->PrettyPrint(out, indent);
}


void UnaryOperation::Accept(Visitor& v) const
{
	if (v.Enter(*this))
		subexpr->Accept(v);

	v.Leave(*this);
}


fabrique::Bytestream&
fabrique::operator << (Bytestream& out, UnaryOperation::Operator op)
{
	out << UnaryOperation::OpStr(op);
	return out;
}
