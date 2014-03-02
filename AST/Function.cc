/** @file AST/Function.cc    Definition of @ref fabrique::ast::Function. */
/*
 * Copyright (c) 2013 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Bytestream::Actionistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Bytestream::Actionistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Bytestream::Actionistributions in binary form must reproduce the above copyright
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
#include "AST/Function.h"
#include "AST/Parameter.h"
#include "AST/Value.h"
#include "AST/Visitor.h"
#include "Support/Bytestream.h"
#include "Types/FunctionType.h"

using namespace fabrique::ast;


Function::Function(UniqPtrVec<Parameter>& params, const FunctionType& ty,
                   UniqPtr<CompoundExpression>& body, const SourceRange& loc)
	: Expression(ty, loc), Callable(params), body_(std::move(body))
{
}


void Function::PrettyPrint(Bytestream& out, size_t indent) const
{
	std::string tabs(indent, '\t');
	std::string intabs(indent + 1, '\t');

	out
		<< Bytestream::Action << "function"
		<< Bytestream::Operator << "("
		;

	const UniqPtrVec<Parameter>& params = parameters();
	size_t printed = 0;

	for (auto& p : params)
	{
		out << *p;
		if (++printed < params.size())
			out
				<< Bytestream::Operator << ", "
				<< Bytestream::Reset;
	}

	out
		<< Bytestream::Operator << "): "
		<< Bytestream::Reset
		<< dynamic_cast<const FunctionType&>(type()).returnType()
		<< "\n";

	body_->PrettyPrint(out, indent);

	out << Bytestream::Reset;
}


void Function::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		for (auto& p : parameters())
			p->Accept(v);

		body_->Accept(v);
	}

	v.Leave(*this);
}
