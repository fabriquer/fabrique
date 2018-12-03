/** @file AST/Call.cc    Definition of @ref fabrique::ast::Call. */
/*
 * Copyright (c) 2013, 2018 Jonathan Anderson
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

#include <fabrique/Bytestream.hh>
#include <fabrique/names.hh>
#include <fabrique/ast/Call.hh>
#include <fabrique/ast/EvalContext.hh>
#include <fabrique/ast/Function.hh>
#include <fabrique/ast/NameReference.hh>
#include <fabrique/ast/Visitor.hh>
#include <fabrique/dag/Callable.hh>
#include "Support/SourceLocation.h"
#include "Support/exceptions.h"
#include <fabrique/types/FunctionType.hh>

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;
using std::shared_ptr;


Call::Call(UniqPtr<Expression> target, UniqPtr<Arguments> arguments, SourceRange src)
	: Expression(src), target_(std::move(target)), arguments_(std::move(arguments))
{
}

void Call::PrettyPrint(Bytestream& out, unsigned int indent) const
{
	out
		<< *target_
		<< Bytestream::Operator << "("
		<< Bytestream::Reset
		;

	arguments_->PrettyPrint(out, indent + 1);

	out
		<< Bytestream::Operator << ")"
		<< Bytestream::Reset;
}


void Call::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		target_->Accept(v);
		arguments_->Accept(v);
	}

	v.Leave(*this);
}

dag::ValuePtr Call::evaluate(EvalContext& ctx) const
{
	Bytestream& dbg = Bytestream::Debug("eval.call");
	dbg << Bytestream::Action << "calling " << *target_ << "\n";

	auto target = target_->evaluateAs<dag::Callable>(ctx);

	dag::ValueMap args;
	StringMap<SourceRange> argLocations;

	for (auto& i : target->NameArguments(arguments_->positional()))
	{
		const std::string name = i.first;
		const ast::Expression& value = *i.second;

		argLocations.emplace(name, value.source());
		args[name] = value.evaluate(ctx);
	}

	for (auto& a : arguments_->keyword())
	{
		const std::string name = a->getName().name();
		const ast::Expression& value = a->getValue();

		SemaCheck(not args[name], a->source(), "redefining '" + name + "'");

		argLocations.emplace(name, SourceRange::Over(a, &value));
		args[name] = value.evaluate(ctx);
	}

	//
	// Calls to file() and import() are special: they implicitly get access to the
	// current `subdir` value (if none has been specified explicitly)
	//
	if (auto *n = dynamic_cast<NameReference*>(target_.get()))
	{
		const std::string &name = n->name().name();
		using names::Subdirectory;

		if ((name == names::File or name == names::Import)
		    and not args[Subdirectory])
		{
			args[Subdirectory] = ctx.Lookup(Subdirectory, source());
			argLocations.emplace(Subdirectory, source());
		}
	}

	target->CheckArguments(args, argLocations, source());
	return target->Call(args, ctx.builder(), source());
}
