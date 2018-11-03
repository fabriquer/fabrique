/** @file DAG/EvalContext.cc    Definition of @ref fabrique::ast::EvalContext. */
/*
 * Copyright (c) 2014, 2018 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme and at Memorial University
 * of Newfoundland under the NSERC Discovery program (RGPIN-2015-06048).
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

#include <fabrique/ast/Builtins.hh>
#include <fabrique/ast/EvalContext.hh>
#include <fabrique/ast/Parameter.hh>
#include <fabrique/ast/Value.hh>

#include <fabrique/dag/DAGBuilder.hh>

#include "Support/Bytestream.h"
#include "Support/Join.h"
#include "Support/exceptions.h"

#include <fabrique/types/FileType.hh>
#include <fabrique/types/FunctionType.hh>
#include <fabrique/types/TypeContext.hh>

#include <cassert>

using namespace fabrique;
using namespace fabrique::ast;
using fabrique::dag::DAG;
using fabrique::dag::ValueMap;
using fabrique::dag::ValuePtr;

using std::shared_ptr;
using std::string;
using std::vector;


EvalContext::EvalContext(TypeContext& ctx, ValueMap builtins)
	: ctx_(ctx), builder_(*this), builtins_(builtins)
{
	// Create top-level scope
	scopes_.emplace_back(std::make_shared<ScopedValues>("top", nullptr));
}

std::vector<DAG::BuildTarget> EvalContext::Evaluate(const UniqPtrVec<ast::Value>& values)
{
	vector<DAG::BuildTarget> topLevelTargets;

	for (const UniqPtr<ast::Value>& v : values)
	{
		if (auto &name = v->name())
		{
			topLevelTargets.emplace_back(name->name(), v->evaluate(*this));
		}
	}

	return topLevelTargets;
}


EvalContext::ScopedValues::ScopedValues(string name, shared_ptr<ScopedValues> parent)
	: name_(name), parent_(parent)
{
}

bool EvalContext::ScopedValues::contains(const string& name) const
{
	return (values_.find(name) != values_.end());
}

EvalContext::ScopedValues&
EvalContext::ScopedValues::Define(const string &name, ValuePtr v, SourceRange src)
{
	if (not src)
	{
		src = v->source();
	}

	SemaCheck(not name.empty(), src, "defining an unnamed value");
	SemaCheck(v, src, "defining a null value");
	SemaCheck(values_.find(name) == values_.end(), src, "redefining " + name);

	values_.emplace(name, v);

	return *this;
}

ValuePtr EvalContext::ScopedValues::Lookup(const string& name) const
{
	auto i = values_.find(name);
	if (i != values_.end())
	{
		Bytestream::Debug("ast.scope.lookup")
			<< Bytestream::Action << "  found "
			<< Bytestream::Literal << "'" << name << "'"
			<< Bytestream::Operator << ": "
			<< *i->second
			<< Bytestream::Reset << "\n"
			;

		return i->second;
	}

	if (parent_)
	{
		return parent_->Lookup(name);
	}

	return nullptr;
}

void EvalContext::ScopedValues::PrettyPrint(Bytestream &out, unsigned int indent) const
{
	const string tabs(indent + 1, '\t');

	out
		<< Bytestream::Type << "scope"
		<< Bytestream::Operator << " '"
		<< Bytestream::Definition << name_
		<< Bytestream::Operator << "':"
		<< Bytestream::Reset << "\n"
		;

	for (const auto& i : values_)
	{
		out
			<< tabs
			<< Bytestream::Definition << i.first
			<< Bytestream::Operator << " = "
			<< Bytestream::Reset
			;

		i.second->PrettyPrint(out, indent + 1);
	}
}


EvalContext::Scope::Scope(shared_ptr<ScopedValues> values, EvalContext &ctx)
	: ctx_(ctx), values_(values), live_(true)
{
}

EvalContext::Scope::Scope(Scope &&orig)
	: ctx_(orig.ctx_), values_(orig.values_), live_(true)
{
	orig.live_ = false;
}

EvalContext::Scope::~Scope()
{
	if (live_)
	{
		ctx_.PopScope();
	}
}

bool EvalContext::Scope::contains(const string &name) const
{
	FAB_ASSERT(values_, "scope has no values_!");
	return values_->contains(name);
}

EvalContext::Scope&
EvalContext::Scope::Define(const string &name, dag::ValuePtr v, SourceRange src)
{
	SemaCheck(live_, src, "defining a value in a dead scope");
	FAB_ASSERT(values_, "Scope has null values_");

	if (not src)
	{
		src = v->source();
	}

	values_->Define(name, v, src);
	return *this;
}


shared_ptr<EvalContext::ScopedValues> EvalContext::CurrentScope()
{
	FAB_ASSERT(not scopes_.empty(), "no scopes in EvalContext");
	return scopes_.back();
}

EvalContext::Scope
EvalContext::EnterScope(const string& name, shared_ptr<ScopedValues> parent)
{
	Bytestream::Debug("eval.scope")
		<< string(scopes_.size(), ' ')
		<< Bytestream::Operator << " >> "
		<< Bytestream::Type << "scope"
		<< Bytestream::Literal << " '" << name << "'"
		<< Bytestream::Reset << "\n"
		;

	if (not parent and not scopes_.empty())
	{
		parent = scopes_.back();
	}

	auto s = std::make_shared<ScopedValues>(name, parent);
	scopes_.push_back(s);
	return Scope(s, *this);
}


EvalContext::ScopedValueName::ScopedValueName(EvalContext& stack, string name)
	: stack_(stack), name_(name)
{
	stack_.PushValueName(name_);
}

EvalContext::ScopedValueName::ScopedValueName(ScopedValueName&& other)
	: stack_(other.stack_), name_(std::move(other.name_))
{
}

EvalContext::ScopedValueName::~ScopedValueName()
{
	string poppedName = stack_.PopValueName();
	assert(poppedName == name_);
}


EvalContext::ScopedValueName EvalContext::evaluating(const string& name)
{
	return ScopedValueName(*this, name);
}


std::shared_ptr<EvalContext::ScopedValues> EvalContext::PopScope()
{
	FAB_ASSERT(not scopes_.empty(), "popping from empty stack");

	auto s = std::move(scopes_.back());
	scopes_.pop_back();

	Bytestream::Debug("ast.eval.scope") << *s << "\n";

	return s;
}

void EvalContext::Define(ScopedValueName& name, ValuePtr v, SourceRange src)
{
	SemaCheck(&name.stack_ == this, src, "mismatched name stack");

	CurrentScope()->Define(name.name_, v, src);
	builder_.Define(fullyQualifiedName(), v);
}


ValuePtr EvalContext::Lookup(const string& name, SourceRange src)
{
	Bytestream& dbg = Bytestream::Debug("dag.lookup");
	dbg
		<< Bytestream::Action << "lookup "
		<< Bytestream::Literal << "'" << name << "'"
		<< Bytestream::Reset << "\n"
		;

	// Try builtins first:
	auto b = builtins_.find(name);
	if (b != builtins_.end())
	{
		return b->second;
	}

	// Next, look for lexically-defined names:
	SemaCheck(not scopes_.empty(), src, "no scopes to lookup in");
	auto s = scopes_.back();
	FAB_ASSERT(s, "top of scopes_ stack is null");

	if (auto v = s->Lookup(name))
	{
		return v;
	}

	// If we are looking for 'builddir' or 'subdir' and haven't found it
	// defined anywhere, provide the top-level build/source subdirectory ('').
	if (name == ast::builtins::BuildDirectory)
		return builder_.File("", ValueMap(), SourceRange::None(), true);

	if (name == ast::builtins::Subdirectory)
		return builder_.File("");

	throw SemanticException("reference to undefined name", src);
}


string EvalContext::currentValueName() const
{
	return fullyQualifiedName();
}

string EvalContext::fullyQualifiedName() const
{
	return join(currentValueName_, ".");
}

void EvalContext::PushValueName(const string& name)
{
	currentValueName_.push_back(name);
}

string EvalContext::PopValueName()
{
	const string name = currentValueName_.back();
	currentValueName_.pop_back();
	return name;
}
