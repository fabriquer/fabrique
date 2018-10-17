/** @file AST/Call.cc    Definition of @ref fabrique::ast::Call. */
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

#include "AST/Call.h"
#include "AST/EvalContext.h"
#include "AST/Function.h"
#include "AST/Visitor.h"
#include "DAG/Build.h"
#include "DAG/Callable.h"
#include "DAG/Function.h"
#include "DAG/Parameter.h"
#include "DAG/Rule.h"
#include "Support/Bytestream.h"
#include "Support/SourceLocation.h"
#include "Support/exceptions.h"
#include "Types/FunctionType.h"

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;
using std::dynamic_pointer_cast;
using std::shared_ptr;


Call::Call(UniqPtr<Expression> target, UniqPtrVec<Expression> positionalArgs,
           UniqPtrVec<Argument> keywordArgs, SourceRange src)
	: Expression(src), target_(std::move(target)),
	  positionalArgs_(std::move(positionalArgs)),
	  keywordArgs_(std::move(keywordArgs))
{
}

void Call::PrettyPrint(Bytestream& out, unsigned int /*indent*/) const
{
	out
		<< *target_
		<< Bytestream::Operator << "("
		<< Bytestream::Reset
		;

	for (size_t i = 0; i < positionalArgs_.size(); )
	{
		out << *positionalArgs_[i];
		if (++i < positionalArgs_.size() or not keywordArgs_.empty())
			out
				<< Bytestream::Operator << ", "
				<< Bytestream::Reset;
	}

	for (size_t i = 0; i < keywordArgs_.size(); )
	{
		out << *keywordArgs_[i];
		if (++i < keywordArgs_.size())
			out
				<< Bytestream::Operator << ", "
				<< Bytestream::Reset;
	}

	out
		<< Bytestream::Operator << ")"
		<< Bytestream::Reset;
}


void Call::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		target_->Accept(v);

		for (auto& a : positionalArgs_)
			a->Accept(v);

		for (auto& a : keywordArgs_)
			a->Accept(v);
	}

	v.Leave(*this);
}

dag::ValuePtr Call::evaluate(EvalContext& ctx) const
{
	Bytestream& dbg = Bytestream::Debug("eval.call");
	dbg << Bytestream::Action << "calling " << *target_ << "\n";

	dag::ValuePtr targetValue = target_->evaluate(ctx);

	auto target = dynamic_pointer_cast<dag::Callable>(targetValue);
	assert(target);

	//
	// Check argument legality.
	//
	for (auto& a : keywordArgs_)
		if (not target->hasParameterNamed(a->getName().name()))
			throw SemanticException("invalid argument", a->source());

	dag::ValueMap args;
	StringMap<SourceRange> argLocations;
	for (auto& i : target->NameArguments(positionalArgs_))
	{
		const std::string name = i.first;
		const ast::Expression& value = *i.second;

		argLocations.emplace(name, value.source());
		args[name] = value.evaluate(ctx);
	}

	for (auto& a : keywordArgs_)
	{
		const std::string name = a->getName().name();
		const ast::Expression& value = a->getValue();

		argLocations.emplace(name, value.source());
		args[name] = value.evaluate(ctx);
	}

	target->CheckArguments(args, argLocations, source());
	return target->Call(args, ctx.builder(), source());
}
