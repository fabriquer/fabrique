/** @file AST/Some.cc    Definition of @ref fabrique::ast::Some. */
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

#include "AST/Builtins.h"
#include "AST/EvalContext.h"
#include "AST/Scope.h"
#include "AST/SomeValue.h"
#include "AST/Visitor.h"
#include "DAG/Primitive.h"
#include "Support/Bytestream.h"
#include "Types/TypeContext.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;


SomeValue::SomeValue(const Type& type, UniqPtr<Expression>& init, SourceRange src)
	: Expression(type, src), initializer_(std::move(init))
{
}


void SomeValue::PrettyPrint(Bytestream& out, unsigned int indent) const
{
	out << Bytestream::Operator << "some(";
	initializer_->PrettyPrint(out, indent + 1);
	out << Bytestream::Operator << ")";
}

void SomeValue::Accept(Visitor& v) const
{
	if (v.Enter(*this))
		initializer_->Accept(v);

	v.Leave(*this);
}

dag::ValuePtr SomeValue::evaluate(EvalContext& ctx) const
{
	const Type& t = type();

	dag::ValuePtr exists(
		new dag::Boolean(true, t.context().booleanType(), source())
	);

	dag::ValueMap fields {
		{
			ast::MaybeExists,
			exists
		},
		{
			ast::MaybeValue,
			initializer_->evaluate(ctx)
		},
	};

	return ctx.builder().Record(fields, t, source());
}
