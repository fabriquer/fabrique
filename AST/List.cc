/** @file AST/List.cc    Definition of @ref fabrique::ast::List. */
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

#include "AST/List.h"
#include "AST/Visitor.h"
#include "DAG/List.h"
#include "Support/Bytestream.h"
#include "Types/Type.h"
#include "Types/TypeError.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;


void List::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out << Bytestream::Operator << "[" << Bytestream::Reset;

	for (auto& e : elements_)
		out << " " << *e;

	out << Bytestream::Operator << " ]" << Bytestream::Reset;
}


void List::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		for (auto& e : elements_)
			e->Accept(v);
	}

	v.Leave(*this);
}


dag::ValuePtr List::evaluate(EvalContext& ctx) const
{
	assert(type().isOrdered());
	assert(type().typeParamCount() == 1);

	const Type& subtype = type()[0];

	SharedPtrVec<dag::Value> values;

	for (auto& e : elements_)
	{
		if (not e->type().isSubtype(subtype))
			throw WrongTypeException(subtype,
			                         e->type(), e->source());

		values.push_back(e->evaluate(ctx));
	}

	return dag::ValuePtr(new dag::List(values, type(), source()));
}
