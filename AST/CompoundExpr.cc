/** @file CompoundExpr.cc    Definition of @ref CompoundExpr. */
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

#include "AST/CompoundExpr.h"
#include "AST/Value.h"
#include "Backend/Visitor.h"
#include "Support/ostream.h"


CompoundExpression::~CompoundExpression()
{
	for (auto *v : values)
		delete v;
}


bool CompoundExpression::isStatic() const
{
	for (auto *v : values)
		if (!v->isStatic())
			return false;

	return result->isStatic();
}


void CompoundExpression::PrettyPrint(std::ostream& out, int indent) const
{
	std::string tabs(indent, '\t');
	std::string intabs(indent + 1, '\t');
	bool isComplex = !values.empty();

	if (isComplex)
	{
		out << tabs << Yellow << "{\n";
		for (auto *v : values)
			v->PrettyPrint(out, indent + 1);
		out << intabs;
	}

	out << *result;

	if (isComplex)
		out << "\n" << Yellow << tabs << "}";

	out << ResetAll;
}


void CompoundExpression::Accept(Visitor& v) const
{
	v.Enter(*this);

	for (auto *val : values)
		val->Accept(v);

	result->Accept(v);

	v.Leave(*this);
}
