/** @file AST/Value.h    Declaration of @ref fabrique::ast::Value. */
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

#include "AST/EvalContext.h"
#include "AST/Value.h"
#include "AST/Visitor.h"
#include "DAG/Build.h"
#include "DAG/File.h"
#include "DAG/List.h"
#include "DAG/Target.h"
#include "Support/Bytestream.h"
#include "Types/Type.h"
#include "Types/TypeError.h"

#include <cassert>
#include <iomanip>

using namespace fabrique;
using namespace fabrique::ast;
using std::string;


Value::Value(UniqPtr<Identifier>& id, UniqPtr<Expression>& value, const Type& t)
	: Expression(t, SourceRange::Over(id, value)),
	  name_(std::move(id)), value_(std::move(value))
{
	assert(not name_->isTyped() or t.isSubtype(name_->type()));
	assert(value_->type().isSubtype(t));

	assert(name_);
	assert(value_);
}


void Value::PrettyPrint(Bytestream& out, size_t indent) const
{
	string tabs(indent, '\t');

	out
		<< tabs
		<< Bytestream::Definition << name_->name()
		<< Bytestream::Operator << ":"
		<< Bytestream::Type << type()
		<< Bytestream::Operator << " = "
		<< Bytestream::Reset << *value_
		<< Bytestream::Operator << ";"
		<< Bytestream::Reset
		;
}

void Value::Accept(Visitor& v) const
{
	if (v.Enter(*this))
	{
		name_->Accept(v);
		value_->Accept(v);
	}

	v.Leave(*this);
}


dag::ValuePtr Value::evaluate(EvalContext& ctx) const
{
	const string name(name_->name());
	Bytestream& dbg = Bytestream::Debug("eval.value");
	dbg
		<< Bytestream::Action << "Evaluating "
		<< *name_
		<< Bytestream::Operator << "..."
		<< "\n"
		;

	auto valueName(ctx.evaluating(name));
	dag::ValuePtr val(value_->evaluate(ctx));
	assert(val);

	dbg
		<< *name_
		<< Bytestream::Operator << " == "
		<< *val
		<< "\n"
		;

	val->type().CheckSubtype(type(), val->source());

	//
	// If the right-hand side is a build, file or list of files,
	// convert to a named target (files and builds are already in the DAG).
	//
	if (auto build = std::dynamic_pointer_cast<dag::Build>(val))
		val = ctx.builder().Target(build);

	else if (auto file = std::dynamic_pointer_cast<dag::File>(val))
		val = ctx.builder().Target(file);

	else if (auto list = std::dynamic_pointer_cast<dag::List>(val))
	{
		if (list->type().elementType().isFile())
			val = ctx.builder().Target(list);
	}
	else if (auto target = std::dynamic_pointer_cast<dag::Target>(val))
	{
		const Type& t = target->type();
		if (t.isFile() or (t.isOrdered() and t[0].isFile()))
			ctx.Alias(target);
	}

	assert(val);
	ctx.Define(valueName, val);

	return val;
}
