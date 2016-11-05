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
using std::string;


namespace {

class ScopeBuilder : public Scope
{
public:
	static ScopeBuilder* Create(const Scope *parent, Node::Parser::ChildNodes<Value>&,
	                            Parameters, TypeContext&, parser::ErrorReporter& err);

	virtual const Type& Lookup(const Identifier&) const override;
	virtual PtrVec<Value> values() const override;

	UniqPtr<Scope> Build();

private:
	ScopeBuilder(const Scope *parent, std::vector<string> names,
	             UniqPtrMap<Value::Parser> parsers, Parameters,
	             TypeContext& types, SourceRange src, parser::ErrorReporter& err);

	const Value* LookupOrBuild(const string& name);
	bool BuildAllValues();

	TypeContext& types_;                    //!< TypeContext for use in construction.
	parser::ErrorReporter& err_;            //!< For reporting parse errors.

	std::vector<string> names_;             //!< Names of scoped elements, in order.
	UniqPtrMap<Value> values_;              //!< Already-parsed values.
	UniqPtrMap<Value::Parser> parsers_;     //!< Values we haven't parsed yet.
};

class CompleteScope : public Scope
{
public:
	CompleteScope(const Scope *parent, UniqPtrVec<Value> values, Parameters params,
	              const Type& nil, SourceRange src)
		: Scope(src, params, nil, parent), values_(std::move(values))
	{
		Bytestream& dbg = Bytestream::Debug("ast.scope.new");
		if (dbg)
		{
			dbg
				<< Bytestream::Action << "created "
				<< Bytestream::Type << "ast::CompleteScope"
				<< Bytestream::Operator << ":"
				<< Bytestream::Reset << "\n"
				;

			for (const auto& v : values_)
			{
				assert(v);
				dbg
					<< Bytestream::Operator << " - "
					<< Bytestream::Reset << *v << "\n"
					;
			}
		}
	}

	virtual PtrVec<Value> values() const override;

private:
	UniqPtrVec<Value> values_;
};

} // anonymous namespace


UniqPtr<Scope> Scope::Create(UniqPtrVec<Value> values, Parameters parameters,
                             const Type& nil, const Scope *parent)
{
	SourceRange src =
		values.empty()
		? SourceRange::None()
		: SourceRange::Over(values.front(), values.back())
		;

	return UniqPtr<Scope>(
		new CompleteScope(parent, std::move(values), parameters, nil, src));
}


ScopeBuilder*
ScopeBuilder::Create(const Scope *parent, Node::Parser::ChildNodes<Value>& valueNodes,
                     Parameters params, TypeContext& nil, parser::ErrorReporter& err)
{
	std::vector<string> names;
	UniqPtrMap<Value::Parser> parsers;
	SourceLocation begin, end;

	for (auto& node : valueNodes)
	{
		if (node->source().begin < begin)
			begin = node->source().begin;

		if (node->source().end > end)
			end = node->source().end;

		const string name = node->name(*parent, err);
		if (parsers.find(name) != parsers.end())
		{
			err.ReportError("redefining value", node->source());
			return nullptr;
		}

		if (params.find(name) != params.end())
		{
			err.ReportError("value obscures parameter", node->source());
			return nullptr;
		}

		names.push_back(name);
		parsers.emplace(name, std::move(node));
	}

	SourceRange src(begin, end);
	return new ScopeBuilder(parent, names, std::move(parsers), params, nil, src, err);
}


ScopeBuilder::ScopeBuilder(const Scope *parent, std::vector<string> names,
                           UniqPtrMap<Value::Parser> parsers, Parameters params,
                           TypeContext& types, SourceRange src, parser::ErrorReporter& err)
	: Scope(src, params, types.nilType(), parent), types_(types), err_(err),
	  names_(names), parsers_(std::move(parsers))
{
}


const Type& ScopeBuilder::Lookup(const Identifier& id) const
{
	const string name = id.name();

	//
	// Play a slightly naughty game: de-`const`ify `this`.
	//
	// Since a ScopeBuilder is a very short-lived thing, but we need to
	// satisfy the same contract as a long-lived Scope object, we just
	// strip the `const` for the duration of this lookup.
	//
	ScopeBuilder *self = const_cast<ScopeBuilder*>(this);
	if (const Value *v = self->LookupOrBuild(id.name()))
	{
		assert(v->isTyped());
		return v->type();
	}

	// Is there a parameter by this name?
	auto i = parameters_.find(id.name());
	if (i != parameters_.end())
	{
		return i->second;
	}

	// Do we have a parent scope to answer the question?
	if (parent_)
		return parent_->Lookup(id);

	return types_.nilType();
}


const Value* ScopeBuilder::LookupOrBuild(const string& name)
{
	// Have we already built this value?
	auto i = values_.find(name);
	if (i != values_.end())
		return i->second.get();

	// Do we have a parser to build it with?
	auto j = parsers_.find(name);
	if (j != parsers_.end())
	{
		Bytestream& dbg = Bytestream::Debug("ast.scope.builder");
		if (dbg)
		{
			dbg
				<< Bytestream::Action << "building "
				<< Bytestream::Definition << name
				<< Bytestream::Reset << "\n"
				;
		}

		Value::Parser& p = *j->second;

		UniqPtr<Value> value(p.Build(*this, types_, err_));
		if (not value)
			return nullptr;

		const Value *ptr = value.get();
		values_.emplace(name, std::move(value));

		return ptr;
	}

	return nullptr;
}


PtrVec<Value> ScopeBuilder::values() const
{
	PtrVec<Value> values;

	for (auto& i : values_)
		values.push_back(i.second.get());

	return values;
}


UniqPtr<Scope> ScopeBuilder::Build()
{
	if (not BuildAllValues())
		return UniqPtr<Scope>();

	UniqPtrVec<Value> values;
	for (const string& name : names_)
	{
		assert(values_.find(name) != values_.end());
		values.push_back(std::move(values_[name]));
	}

	const Type& nil = types_.nilType();
	return UniqPtr<Scope>(
		new CompleteScope(parent_, std::move(values), parameters_, nil, source()));
}


bool ScopeBuilder::BuildAllValues()
{
	for (const string& name : names_)
	{
		if (not LookupOrBuild(name))
		{
			return false;
		}
	}

	return true;
}


PtrVec<Value> CompleteScope::values() const
{
	PtrVec<Value> values;

	for (auto& i : values_)
		values.push_back(i.get());

	return values;
}


Scope::Parser::~Parser()
{
}


Scope* Scope::Parser::Build(const Scope& parentScope, TypeContext& types, Err& err)
{
	UniqPtr<ScopeBuilder> builder(
		ScopeBuilder::Create(&parentScope, values_, Parameters(), types, err));

	if (not builder)
		return nullptr;

	UniqPtr<Scope> scope = builder->Build();
	if (not scope or err.hasErrors())
		return nullptr;

	return scope.release();
}


const Scope& Scope::None(TypeContext& t)
{
	const Type& nil = t.nilType();
	const Scope& none =
		*new CompleteScope(nullptr, UniqPtrVec<Value>(), Parameters(),
		                   nil, SourceRange::None());

	return none;
}


Scope::Scope(SourceRange src, Parameters parameters, const Type& nil, const Scope* parent)
	: Node(src), parent_(parent), parameters_(parameters), nil_(nil)
{
}


Scope::~Scope()
{
}


const Type& Scope::Lookup(const Identifier& id) const
{
	for (auto& v : values())
	{
		if (v->name() == id)
		{
			return v->type();
		}
	}

	auto i = parameters_.find(id.name());
	if (i != parameters_.end())
	{
		return i->second;
	}

	if (parent_)
		return parent_->Lookup(id);

	return nil_;
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
	string tabs(indent + 1, '\t');

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
