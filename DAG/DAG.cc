/** @file DAG.cc    Definition of @ref DAG. */
/*
 * Copyright (c) 2013-2014 Jonathan Anderson
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

#include "AST/Action.h"
#include "AST/Argument.h"
#include "AST/Builtins.h"
#include "AST/Call.h"
#include "AST/FileList.h"
#include "AST/Filename.h"
#include "AST/List.h"
#include "AST/Scope.h"
#include "AST/SymbolReference.h"
#include "AST/Value.h"
#include "AST/Visitor.h"

#include "DAG/Build.h"
#include "DAG/DAG.h"
#include "DAG/File.h"
#include "DAG/List.h"
#include "DAG/Primitive.h"
#include "DAG/Rule.h"
#include "DAG/UndefinedValueException.h"
#include "DAG/Value.h"

#include "Support/Bytestream.h"
#include "Support/Join.h"
#include "Support/exceptions.h"

#include "Types/Type.h"

#include <deque>
#include <stack>

using namespace fabrique;
using namespace fabrique::dag;

using std::shared_ptr;
using std::string;


//! AST Visitor that flattens the AST into a DAG.
class Flattener : public ast::Visitor
{
public:
	Flattener(FabContext& ctx)
		: ctx(ctx), stringTy(*ctx.type("string"))
	{
	}

	~Flattener() {}

	VISIT(ast::Action)
	VISIT(ast::Argument)
	VISIT(ast::BinaryOperation)
	VISIT(ast::BoolLiteral)
	VISIT(ast::Call)
	VISIT(ast::CompoundExpression)
	VISIT(ast::Conditional)
	VISIT(ast::Filename)
	VISIT(ast::FileList)
	VISIT(ast::ForeachExpr)
	VISIT(ast::Function)
	VISIT(ast::Identifier)
	VISIT(ast::IntLiteral)
	VISIT(ast::List)
	VISIT(ast::Parameter)
	VISIT(ast::Scope)
	VISIT(ast::StringLiteral)
	VISIT(ast::SymbolReference)
	VISIT(ast::Value)

	//! The components of the current scope's fully-qualified name.
	std::deque<string> scopeName;

	//! Symbols defined in this scope (or the one up from it, or up...).
	std::deque<ValueMap> scopes;

	//! All values, named by fully-qualified name.
	ValueMap values;

private:
	//! Object that owns types and other memory.
	FabContext& ctx;

	//! The type of generated strings.
	const Type& stringTy;

	//! Get a named value from the current scope or a parent scope.
	shared_ptr<Value> getNamedValue(const std::string& name);

	shared_ptr<Value> flatten(const ast::Expression&);

	/** The name of the value we are currently processing. */
	std::stack<string> currentValueName;

	/**
	 * The value currently being processed.
	 * Visitor methods should leave a single item on this stack.
	 */
	std::stack<shared_ptr<Value>> currentValue;
};


DAG* DAG::Flatten(const ast::Scope& s, FabContext& ctx)
{
	Flattener f(ctx);
	s.Accept(f);

	return new DAG(f.values);
}


DAG::DAG(const ValueMap& values)
	: values(values)
{
}


void DAG::PrettyPrint(Bytestream& b, int indent) const
{
	for (auto& i : values)
	{
		const string& name = i.first;
		const shared_ptr<Value>& v = i.second;

		assert(v);

		b
			<< Bytestream::Type << v->type()
			<< Bytestream::Definition << " " << name
			<< Bytestream::Operator << " = "
			<< *v
			<< Bytestream::Reset << "\n"
			;
	}
}


ValueMap::const_iterator DAG::begin() const { return values.begin(); }
ValueMap::const_iterator DAG::end() const { return values.end(); }



bool Flattener::Enter(const ast::Action& a)
{
	// TODO: proper function type, e.g. (int,string) => file
	static const Type& ruleType = *ctx.type("rule");
	string command;
	ValueMap arguments;

	if (a.arguments().size() < 1)
		throw SemanticException("Missing action arguments",
				a.getSource());

	for (const ast::Argument *arg : a.arguments())
	{
		// Flatten the argument and convert it to a string.
		shared_ptr<Value> value = flatten(arg->getValue());

		// The only keyword-less argument to action() is its command.
		if (not arg->hasName() or arg->getName().name() == "command")
		{
			if (not command.empty())
				throw SemanticException(
					"Duplicate command", arg->getSource());

			command = value->str();
			continue;
		}

		shared_ptr<Value> v(new String(value->str(), stringTy,
		                               arg->getSource()));
		arguments.emplace(arg->getName().name(), v);
	}

	currentValue.emplace(Rule::Create(currentValueName.top(),
	                                  command, arguments, ruleType));

	return false;
}

void Flattener::Leave(const ast::Action&) {}


bool Flattener::Enter(const ast::Argument& arg)
{
	currentValue.emplace(flatten(arg.getValue()));
	return false;
}

void Flattener::Leave(const ast::Argument&) {}


bool Flattener::Enter(const ast::BinaryOperation&) { return false; }
void Flattener::Leave(const ast::BinaryOperation&) {}


bool Flattener::Enter(const ast::BoolLiteral& b)
{
	currentValue.emplace(
		new Boolean(b.value(), b.getType(), b.getSource()));

	return false;
}

void Flattener::Leave(const ast::BoolLiteral&) {}


bool Flattener::Enter(const ast::Call& call)
{
	// TODO: proper function type, e.g. (int,string) => file
	static const Type& buildType = *ctx.type("build");

	const ast::SymbolReference& targetRef = call.target();
	const string name = targetRef.getName().name();
	shared_ptr<Value> target = getNamedValue(name);

	if (shared_ptr<Rule> rule = std::dynamic_pointer_cast<Rule>(target))
	{
		shared_ptr<Value> in, out;
		SharedPtrVec<Value> dependencies, extraOutputs;
		ValueMap arguments;

		//
		// Interpret the first two unnamed arguments as 'in' and 'out'.
		//
		for (const ast::Argument *arg : call)
		{
			shared_ptr<Value> value = flatten(*arg);

			if (arg->hasName())
			{
				const std::string& name = arg->getName().name();

				if (name == "in")
					in = value;

				else if (name == "out")
					out = value;

				else
					arguments[name] = value;
			}
			else
			{
				if (not in)
					in = value;

				else if (not out)
					out = value;
			}
		}

		//
		// Validate against The Rules:
		//  1. there must be input
		//  2. there must be output
		//  3. all other arguments must match explicit parameters
		//
		if (not in)
			throw SemanticException(
				"use of action without input file(s)",
				call.getSource());

		if (not out)
			throw SemanticException(
				"use of action without output file(s)",
				call.getSource());


		const ast::Action& action =
			dynamic_cast<const ast::Action&>(targetRef.getValue());

		auto& params = action.parameters();

		for (auto& i : arguments)
		{
			const string name = i.first;
			shared_ptr<dag::Value> arg = i.second;

			auto j = params.find(name);
			if (j == params.end())
				throw SemanticException(
					"no such parameter '" + name + "'",
					arg->getSource());

			//
			// Additionally, if the parameter is a file, add the
			// argument to the dependency graph.
			//
			const ast::Parameter *p = j->second;
			const Type& type = p->getType();
			if (type.name() != "file")
				continue;

			if (type.typeParamCount() == 0)
				throw SemanticException(
					"file missing [in] or [out] tag",
					p->getSource());

			if (type[0].name() == "in")
				dependencies.push_back(arg);

			else if (type[0].name() == "out")
				extraOutputs.push_back(arg);

			else
				throw SemanticException(
					"expected file[in|out]",
					p->getSource());
		}

		currentValue.emplace(
			Build::Create(rule, in, out,
			              dependencies, extraOutputs,
			              arguments, buildType, call.getSource()));
	}

	return false;
}
void Flattener::Leave(const ast::Call&) {}


bool Flattener::Enter(const ast::CompoundExpression&)
{
	scopes.push_back(ValueMap());
	return true;
}

void Flattener::Leave(const ast::CompoundExpression&)
{
	scopes.pop_back();
}


bool Flattener::Enter(const ast::Conditional&) { return false; }
void Flattener::Leave(const ast::Conditional&) {}


bool Flattener::Enter(const ast::Filename& f)
{
	string name = flatten(f.name())->str();

	if (shared_ptr<Value> subdir = getNamedValue(ast::Subdirectory))
		name = join(subdir->str(), name, "/");

	currentValue.emplace(new File(name, f.getType(), f.getSource()));
	return false;
}
void Flattener::Leave(const ast::Filename&) {}


bool Flattener::Enter(const ast::FileList& l)
{
	SharedPtrVec<Value> files;
	ValueMap listScope;

	for (const ast::Argument *arg : l.arguments())
	{
		const string name = arg->getName().name();
		shared_ptr<Value> value = flatten(arg->getValue());

		if (name == ast::Subdirectory)
			if (shared_ptr<Value> existing = getNamedValue(name))
			{
				string base = existing->str();
				string subdir = join(base, value->str(), "/");
				SourceRange loc = arg->getSource();

				value.reset(new String(subdir, stringTy, loc));
			}

		listScope[name] = value;
	}

	scopes.push_back(listScope);

	for (const ast::Filename *file : l)
	{
		shared_ptr<Value> f = flatten(*file);
		files.push_back(f);

		assert(f == std::dynamic_pointer_cast<File>(f));
	}

	scopes.pop_back();

	currentValue.emplace(new List(files, l.getType(), l.getSource()));

	return false;
}
void Flattener::Leave(const ast::FileList&) {}


bool Flattener::Enter(const ast::ForeachExpr&) { return false; }
void Flattener::Leave(const ast::ForeachExpr&) {}


bool Flattener::Enter(const ast::Function&) { return false; }
void Flattener::Leave(const ast::Function&) {}


bool Flattener::Enter(const ast::Identifier&) { return false; }
void Flattener::Leave(const ast::Identifier&) {}


bool Flattener::Enter(const ast::IntLiteral& i)
{
	currentValue.emplace(
		new Integer(i.value(), i.getType(), i.getSource()));

	return false;
}

void Flattener::Leave(const ast::IntLiteral&) {}


bool Flattener::Enter(const ast::List& l)
{
	assert(l.getType().name() == "list");
	assert(l.getType().typeParamCount() == 1);
	const Type& subtype = l.getType()[0];

	SharedPtrVec<Value> values;

	for (const ast::Expression *e : l)
	{
		if (e->getType() != subtype)
			throw SemanticException(
				"list element's type (" + e->getType().str()
				+ ") != list subtype (" + subtype.str() + ")",
				e->getSource());

		values.push_back(flatten(*e));
	}

	currentValue.emplace(new List(values, l.getType(), l.getSource()));

	return false;
}

void Flattener::Leave(const ast::List&) {}


bool Flattener::Enter(const ast::Parameter&) { return false; }
void Flattener::Leave(const ast::Parameter&) {}


bool Flattener::Enter(const ast::Scope&)
{
	scopes.push_back(ValueMap());
	return false;
}

void Flattener::Leave(const ast::Scope&)
{
	ValueMap scopedSymbols = std::move(scopes.back());
	scopes.pop_back();

	const string scopeName = join(this->scopeName, ".");

	for (auto symbol : scopedSymbols)
	{
		const string name = join(scopeName, symbol.first, ".");
		values.emplace(name, std::move(symbol.second));
	}
}


bool Flattener::Enter(const ast::StringLiteral& s)
{
	currentValue.emplace(new String(s.str(), s.getType(), s.getSource()));
	return false;
}

void Flattener::Leave(const ast::StringLiteral&) {}


bool Flattener::Enter(const ast::SymbolReference& r)
{
	const string& name = r.getName().name();
	const ValueMap& symbols = scopes.back();

	auto i = symbols.find(name);
	if (i == symbols.end())
		throw UndefinedValueException(name, r.getSource());

	assert(i->first == name);
	currentValue.emplace(std::move(i->second));

	return false;
}

void Flattener::Leave(const ast::SymbolReference&) {}


bool Flattener::Enter(const ast::Value& v)
{
	currentValueName.push(v.getName().name());
	return true;
}

void Flattener::Leave(const ast::Value& v)
{
	// TODO: later, fire this assertion. for now, ignore.
	if (currentValue.empty())
		return;

	assert(not currentValue.empty());

	ValueMap& currentScope = scopes.back();

	currentScope.emplace(currentValueName.top(),
	                     std::move(currentValue.top()));
	currentValueName.pop();
	currentValue.pop();
}


shared_ptr<Value> Flattener::getNamedValue(const string& name)
{
	for (auto i = scopes.rbegin(); i != scopes.rend(); i++)
	{
		const ValueMap& scope = *i;

		auto value = scope.find(name);
		if (value != scope.end())
			return value->second;
	}

	return NULL;
}

shared_ptr<Value> Flattener::flatten(const ast::Expression& e)
{
	e.Accept(*this);

	// TODO: don't do this
	if (currentValue.empty())
		return NULL;

	assert(not currentValue.empty());

	shared_ptr<Value> v(currentValue.top());
	currentValue.pop();

	return v;
}
