/** @file AST/Scope.cc    Definition of @ref fabrique::ast::Scope. */
/*
 * Copyright (c) 2013, 2016 Jonathan Anderson
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
#include "AST/Builtins.h"
#include "AST/Parameter.h"
#include "AST/Scope.h"
#include "AST/Value.h"
#include "AST/Visitor.h"
#include "Parsing/ErrorReporter.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Types/FileType.h"
#include "Types/TypeContext.h"
#include "Types/UserType.h"

#include <cassert>
#include <set>

using namespace fabrique;
using namespace fabrique::ast;


namespace {

class CompleteScope : public Scope
{
public:
	CompleteScope(const Scope *parent, UniqPtrVec<Value> values, SourceRange src)
		: Scope(src, parent), values_(std::move(values))
	{
	}

	virtual const UniqPtrVec<Value>& values() const override { return values_; }

private:
	UniqPtrVec<Value> values_;
};

class ScopeBuilder : public Scope
{
public:
	virtual const UniqPtrVec<Value>& values() const override;

private:
	//! Already-parsed values.
	UniqPtrVec<Value> values_;

	//! Values we haven't parsed yet.
	UniqPtrVec<Value::Parser> parsers_;
};

} // anonymous namespace


UniqPtr<Scope> Scope::Create(UniqPtrVec<Value> values, const Scope *parent)
{
	SourceRange src =
		values.empty()
		? SourceRange::None()
		: SourceRange::Over(values.front(), values.back())
		;

	return UniqPtr<Scope>(new CompleteScope(parent, std::move(values), src));
}


Scope::Parser::~Parser()
{
}


Scope* Scope::Parser::Build(const Scope& parentScope, TypeContext& types, Err& err) const
{
	UniqPtrVec<Value> values;
	std::set<std::string> names;
	SourceLocation begin, end;

	for (const std::unique_ptr<Value::Parser>& v : values_)
	{
		if (v->source().begin < begin)
			begin = v->source().begin;

		if (v->source().end > end)
			end = v->source().end;

		// TODO: really handle scope lookup

		UniqPtr<Value> value(v->Build(parentScope, types, err));
		const std::string name = value->name().name();

		if (names.find(name) != names.end())
		{
			err.ReportError("redefining value", *value);
			return nullptr;
		}

		names.insert(name);
		values.emplace_back(std::move(value));
	}

	if (err.hasErrors())
		return nullptr;

	return new CompleteScope(&parentScope, std::move(values), source_);
}


const Scope& Scope::None()
{
	const Scope& none =
		*new CompleteScope(nullptr, UniqPtrVec<Value>(), SourceRange::None());

	return none;
}


Scope::Scope(SourceRange src, const Scope* parent)
	: Node(src), parent_(parent)
{
}


Scope::~Scope()
{
}


const UniqPtr<Value>& Scope::Lookup(const Identifier& id) const
{
	static UniqPtr<Value>& NoValue = *new UniqPtr<Value>();

	for (auto& v : values())
	{
		if (v->name() == id)
		{
			return v;
		}
	}

	if (parent_)
		return parent_->Lookup(id);

	return NoValue;
}


bool Scope::contains(const Identifier& name) const
{
	return static_cast<bool>(Lookup(name));
}


/*
void Scope::Register(const Argument *a)
{
	assert(a);
	assert(a->hasName());
	Register(a->getName(), a->getValue().type());
}


void Scope::Register(const Parameter *p)
{
	assert(p);
	Register(p->getName(), p->type());
}


void Scope::Register(const Value& v)
{
	Register(v.name(), v.value().type());
}


void Scope::Register(const Identifier& id, const Type& t)
{
	Bytestream::Debug("ast.scope")
		<< Bytestream::Action << "scope"
		<< Bytestream::Operator << " <- "
		<< Bytestream::Type << "symbol"
		<< Bytestream::Operator << " = " << id
		<< Bytestream::Operator << ": " << t
		<< "\n"
		;

	if (symbols_.find(id.name()) != symbols_.end())
		throw SyntaxError("name '" + id.name() + "' already defined",
		                  id.source());

	symbols_.emplace(id.name(), t);
}


void Scope::Take(Value *v)
{
	std::unique_ptr<Value> val(v);
	Take(val);
}


void Scope::Take(UniqPtr<Value>& val)
{
	assert(val);

	Register(*val);

	Bytestream::Debug("ast.scope")
		<< Bytestream::Action << "scope"
		<< Bytestream::Operator << " <- "
		<< Bytestream::Type << "value"
		<< Bytestream::Operator << ": " << *val
		<< "\n"
		;

	ownedValues_.push_back(std::move(val));
}


UniqPtrVec<Value> Scope::TakeValues()
{
	return std::move(ownedValues_);
}
*/


void Scope::PrettyPrint(Bytestream& out, size_t indent) const
{
	std::string tabs(indent + 1, '\t');

	out << Bytestream::Operator << "{\n";

	for (auto& v : values())
		out
			<< tabs
			<< Bytestream::Definition << v->name()
			<< Bytestream::Operator << ":"
			<< *v
			<< Bytestream::Reset << "\n"
			;

	out << Bytestream::Operator << "}";
}

void Scope::Accept(Visitor& v) const
{
	v.Enter(*this);

	for (auto& val : values())
		val->Accept(v);

	v.Leave(*this);
}
