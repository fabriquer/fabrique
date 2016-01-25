/** @file AST/Scope.cc    Definition of @ref fabrique::ast::Scope. */
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

	return new Scope(&parentScope, std::move(values), source_);
}


/*
Scope::Scope(const Scope *parent, const std::string& name, const Type& argumentsType,
             SourceRange src)
	: Node(src), parent_(parent), name_(name), arguments_(argumentsType)
{
}
*/


Scope::Scope(const Scope *parent, SourceRange src)
	: Node(src), parent_(parent)
{
}


Scope::Scope(const Scope *parent, UniqPtrVec<Value> values, SourceRange src)
	: Node(src), parent_(parent), values_(std::move(values))
{
}


Scope::Scope(Scope&& other)
	: Node(other.source()), parent_(other.parent_), name_(other.name_),
	  /*arguments_(other.arguments_),*/ symbols_(other.symbols_),
	  values_(std::move(other.values_))
{
	other.symbols_.clear();
	assert(other.values_.empty());
}


#if 0
const Type& Scope::Lookup(const Identifier& id) const
{
	const std::string& name = id.name();

	/*
	// Special case: 'args' is a reserved name derived from external arguments
	//               (command-line or via an import() expression).
	if ((name == ast::Arguments) and arguments_.valid())
		return arguments_;
	*/

	// More special cases: 'builddir' and 'subdir' are files.
	if (name == ast::BuildDirectory or name == ast::Subdirectory)
		return arguments_.context().fileType();

	auto i = symbols_.find(name);
	if (i != symbols_.end())
	{
		const Type& t = i->second;
		if (t.isType())
			return dynamic_cast<const UserType&>(t).userType();

		return t;
	}

	if (parent_)
		return parent_->Lookup(id);

	return arguments_.context().nilType();
}


bool Scope::hasArguments() const
{
	return arguments_.valid();
}
#endif


bool Scope::contains(const Identifier& name) const
{
	return symbols_.find(name.name()) != symbols_.end();
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

	for (auto& symbol : symbols_)
		out
			<< tabs
			<< Bytestream::Definition << symbol.first
			<< Bytestream::Operator << ":"
			<< symbol.second
			<< Bytestream::Reset << "\n"
			;

	out << Bytestream::Operator << "}";
}

void Scope::Accept(Visitor& v) const
{
	v.Enter(*this);

	for (auto& val : values_)
		val->Accept(v);

	v.Leave(*this);
}
