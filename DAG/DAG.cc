/** @file DAG.cc    Definition of @ref DAG. */
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

#include "AST/Action.h"
#include "AST/Argument.h"
#include "AST/List.h"
#include "AST/Scope.h"
#include "AST/SymbolReference.h"
#include "AST/Type.h"
#include "AST/Value.h"
#include "AST/Visitor.h"

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

#include <deque>
#include <stack>

using namespace fabrique;
using namespace fabrique::dag;
using ValueMap = DAG::ValueMap;

using std::shared_ptr;
using std::string;


//! AST Visitor that flattens the AST into a DAG.
class Flattener : public ast::Visitor
{
public:
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
	VISIT(ast::Type)
	VISIT(ast::Value)

	//! The components of the current scope's fully-qualified name.
	std::deque<string> scopeName;

	//! Symbols defined in this scope (or the one up from it, or up...).
	std::deque<ValueMap> scopes;

	//! All values, named by fully-qualified name.
	ValueMap values;

private:
	shared_ptr<Value> flatten(const ast::Expression&);

	/**
	 * The value currently being processed.
	 * Visitor methods should leave a single item on this stack.
	 */
	std::stack<shared_ptr<Value>> currentValue;
};


DAG* DAG::Flatten(const ast::Scope& s)
{
	Flattener f;
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
	string command;
	ValueMap parameters;

	if (a.arguments().size() < 1)
		throw SemanticException("Missing action arguments",
				a.getSource());

	for (const ast::Argument *arg : a)
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

		shared_ptr<Value> v(new String(value->str(), arg->getSource()));
		parameters.emplace(arg->getName().name(), v);
	}

	currentValue.emplace(Rule::Create(command, parameters));

	return false;
}

void Flattener::Leave(const ast::Action&) {}


bool Flattener::Enter(const ast::Argument&) { return false; }
void Flattener::Leave(const ast::Argument&) {}


bool Flattener::Enter(const ast::BinaryOperation&) { return false; }
void Flattener::Leave(const ast::BinaryOperation&) {}


bool Flattener::Enter(const ast::BoolLiteral& b)
{
	currentValue.emplace(new Boolean(b.value(), b.getSource()));
	return false;
}

void Flattener::Leave(const ast::BoolLiteral&) {}


bool Flattener::Enter(const ast::Call&) { return false; }
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


bool Flattener::Enter(const ast::Filename&) { return false; }
void Flattener::Leave(const ast::Filename&) {}


bool Flattener::Enter(const ast::FileList&) { return false; }
void Flattener::Leave(const ast::FileList&) {}


bool Flattener::Enter(const ast::ForeachExpr&) { return false; }
void Flattener::Leave(const ast::ForeachExpr&) {}


bool Flattener::Enter(const ast::Function&) { return false; }
void Flattener::Leave(const ast::Function&) {}


bool Flattener::Enter(const ast::Identifier&) { return false; }
void Flattener::Leave(const ast::Identifier&) {}


bool Flattener::Enter(const ast::IntLiteral& i)
{
	currentValue.emplace(new Integer(i.value(), i.getSource()));
	return false;
}

void Flattener::Leave(const ast::IntLiteral&) {}


bool Flattener::Enter(const ast::List& l)
{
	assert(l.getType().name() == "list");
	assert(l.getType().typeParamCount() == 1);
	const ast::Type& subtype = l.getType()[0];

	std::vector<shared_ptr<Value>> values;

	for (const ast::Expression *e : l)
	{
		if (e->getType() != subtype)
			throw SemanticException(
				"list element's type (" + e->getType().str()
				+ ") != list subtype (" + subtype.str() + ")",
				e->getSource());

		values.push_back(flatten(*e));
	}

	currentValue.emplace(new List(values));

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
	currentValue.emplace(new String(s.str(), s.getSource()));
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


bool Flattener::Enter(const ast::Type&) { return false; }
void Flattener::Leave(const ast::Type&) {}


bool Flattener::Enter(const ast::Value& v) { return true; }
void Flattener::Leave(const ast::Value& v)
{
	// TODO: later, fire this assertion. for now, ignore.
	if (currentValue.empty())
		return;

	assert(not currentValue.empty());

	const string& name = v.getName().name();
	ValueMap& currentScope = scopes.back();

	currentScope.emplace(name, std::move(currentValue.top()));
	currentValue.pop();
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
