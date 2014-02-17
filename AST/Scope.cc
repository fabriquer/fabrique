/** @file Scope.cc    Definition of @ref Scope. */
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

#include "AST/Argument.h"
#include "AST/Parameter.h"
#include "AST/Scope.h"
#include "AST/Value.h"
#include "AST/Visitor.h"
#include "Support/Bytestream.h"

using namespace fabrique::ast;


Scope::Scope(const Scope *parent, const std::string& name)
	: parent(parent), debugName(name)
{
}


Scope::Scope(Scope&& other)
	: parent(other.parent), symbols(other.symbols),
	  ownedValues(std::move(other.ownedValues))
{
}


const Expression* Scope::Lookup(const Identifier& name) const
{
	auto i = symbols.find(name.name());
	if (i != symbols.end())
		return i->second;

	if (parent)
		return parent->Lookup(name);

	return NULL;
}


void Scope::Register(const Argument *a)
{
	assert(a->hasName());
	Register(a->getName(), &a->getValue());
}


void Scope::Register(const Parameter *p)
{
	Register(p->getName(), p);
}


void Scope::Register(const Value& v)
{
	Register(v.getName(), &v.getValue());
}


void Scope::Register(const Identifier& id, const Expression *e)
{
	assert(e);

	Bytestream::Debug("ast.scope")
		<< Bytestream::Action << "scope"
		<< Bytestream::Operator << " <- "
		<< Bytestream::Type << "symbol"
		<< Bytestream::Operator << ": " << id
		<< Bytestream::Operator << " = " << *e
		<< "\n"
		;

	assert(symbols.find(id.name()) == symbols.end());
	symbols[id.name()] = e;
}


void Scope::Take(Value *v)
{
	std::unique_ptr<Value> val(v);
	assert(val);

	Register(*val);

	Bytestream::Debug("ast.scope")
		<< Bytestream::Action << "scope"
		<< Bytestream::Operator << " <- "
		<< Bytestream::Type << "value"
		<< Bytestream::Operator << ": " << *val
		<< "\n"
		;

	ownedValues.push_back(std::move(val));
}


void Scope::Accept(Visitor& v) const
{
	v.Enter(*this);

	for (auto& val : ownedValues)
		val->Accept(v);

	v.Leave(*this);
}
