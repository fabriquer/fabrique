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
#include "DAG/UndefinedValueException.h"

#include "Support/Bytestream.h"
#include "Support/Join.h"
#include "Support/exceptions.h"

#include <deque>
#include <stack>

using namespace fabrique;
using namespace fabrique::dag;
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

	StringMap<string> variables;
	StringMap<File*> files;
	StringMap<Rule*> rules;

	std::deque<string> scopeName;
	std::deque<StringMap<string>> scopeSymbols;
	std::stack<string> currentValue;
	std::unique_ptr<File> currentFile;
	std::unique_ptr<Rule> currentRule;
};


//! Append a string to a deque.
static std::vector<string>&&
	operator+ (const std::deque<string>& d, const string& s)
{
	std::vector<string> result(d.begin(), d.end());
	result.push_back(s);
	return std::move(result);
}


DAG* DAG::Flatten(const ast::Scope& s)
{
	Flattener f;
	s.Accept(f);

	return new DAG(f.variables, f.files, f.rules);
}


DAG::DAG(const StringMap<string>& vars,
         const StringMap<File*>& files, const StringMap<Rule*>& rules)
	: vars(vars), files(files), rules(rules)
{
}


DAG::~DAG()
{
	for (auto& r : rules)
		delete r.second;

	for (auto& f : files)
		delete f.second;
}


void DAG::PrettyPrint(Bytestream& b, int indent) const
{
	for (auto& v : vars)
		b
			<< Bytestream::Type << "var "
			<< Bytestream::Definition << v.first
			<< Bytestream::Operator << " = "
			<< Bytestream::Literal << "'" << v.second << "'"
			<< Bytestream::Reset << "\n"
			;

	for (auto& r : rules)
		b
			<< Bytestream::Type << "rule "
			<< Bytestream::Definition << r.first
			<< Bytestream::Operator << " = "
			<< Bytestream::Reset << *r.second
			<< "\n"
			;

	for (auto& f : files)
		b
			<< Bytestream::Type << "file "
			<< Bytestream::Definition << f.first
			<< Bytestream::Operator << " = "
			<< Bytestream::Reset << *f.second
			<< "\n"
			;
}



bool Flattener::Enter(const ast::Action& a)
{
	string command;
	StringMap<string> parameters;

	if (a.arguments().size() < 1)
		throw SemanticException("Missing action arguments",
				a.getSource());

	for (const ast::Argument *arg : a)
	{
		if (not arg->hasName() and not command.empty())
		{
			// The only keyword-less argument to action() is
			// its command.
			throw SemanticException(
				"Duplicate command", arg->getSource());
		}

		arg->getValue().Accept(*this);
		assert(not currentValue.empty());

		const string value = currentValue.top();
		currentValue.pop();

		if (not arg->hasName()
		    or strcmp(arg->getName().name().c_str(), "command") == 0)
			command = value;

		else
			parameters[arg->getName().name()] = value;
	}

	currentRule.reset(Rule::Create(command, parameters));

	return false;
}

void Flattener::Leave(const ast::Action&) {}


bool Flattener::Enter(const ast::Argument&) { return false; }
void Flattener::Leave(const ast::Argument&) {}


bool Flattener::Enter(const ast::BinaryOperation&) { return false; }
void Flattener::Leave(const ast::BinaryOperation&) {}


bool Flattener::Enter(const ast::BoolLiteral&) { return false; }
void Flattener::Leave(const ast::BoolLiteral&) {}


bool Flattener::Enter(const ast::Call&) { return false; }
void Flattener::Leave(const ast::Call&) {}


bool Flattener::Enter(const ast::CompoundExpression&)
{
	scopeSymbols.push_back(StringMap<string>());
	return true;
}

void Flattener::Leave(const ast::CompoundExpression&)
{
	scopeSymbols.pop_back();
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
	currentValue.push(i.str());
	return false;
}

void Flattener::Leave(const ast::IntLiteral&) {}


bool Flattener::Enter(const ast::List& l)
{
	assert(l.getType().name() == "list");
	assert(l.getType().typeParamCount() == 1);

	const ast::Type subtype = l.getType()[0];
	if (subtype.name() == "string")
	{
		std::vector<string> values;

		for (const ast::Expression *e : l)
		{
			e->Accept(*this);

			// TODO: don't do this
			if (currentValue.empty())
				continue;

			assert(not currentValue.empty());

			values.push_back(currentValue.top());
			currentValue.pop();
		}

		currentValue.push(join(values, " "));
	}

	else throw SemanticException(
		"Unable to flatten list[" + subtype.name() + "]",
		l.getSource());

	return false;
}

void Flattener::Leave(const ast::List&) {}


bool Flattener::Enter(const ast::Parameter&) { return false; }
void Flattener::Leave(const ast::Parameter&) {}


bool Flattener::Enter(const ast::Scope&)
{
	scopeSymbols.push_back(StringMap<string>());
	return false;
}

void Flattener::Leave(const ast::Scope&)
{
	scopeSymbols.pop_back();
}


bool Flattener::Enter(const ast::StringLiteral& s)
{
	currentValue.push(s.str());
	return false;
}

void Flattener::Leave(const ast::StringLiteral&) {}


bool Flattener::Enter(const ast::SymbolReference& r)
{
	const string& name = r.getName().name();
	const StringMap<string>& symbols = scopeSymbols.back();

	auto i = symbols.find(name);
	if (i == symbols.end())
		throw UndefinedValueException(name, r.getSource());

	assert(i->first == name);
	currentValue.push(i->second);

	return false;
}

void Flattener::Leave(const ast::SymbolReference&) {}


bool Flattener::Enter(const ast::Type&) { return false; }
void Flattener::Leave(const ast::Type&) {}


bool Flattener::Enter(const ast::Value& v) { return true; }
void Flattener::Leave(const ast::Value& v)
{
	const bool isStringVal = not currentValue.empty();

	const string name = v.getName().name();
	const string scopedName = join(scopeName + name, ".");

	if (isStringVal)
	{
		assert(not (currentFile or currentRule));

		const string value = currentValue.top();
		currentValue.pop();

		variables[name] = value;
		scopeSymbols.back()[name] = value;
	}
	else if (currentFile)
	{
		assert(not (isStringVal or currentRule));
		files[name] = currentFile.release();
	}
	else if (currentRule)
	{
		assert(not (isStringVal or currentFile));
		rules[name] = currentRule.release();
	}
	else
	{
		// TODO: later, throw this exception. for now, ignore.
		/*
		throw SemanticException(
			"Value '" + name + "' is not a variable, file or rule",
			v.getSource());
		*/
	}
}
