/** @file DAG/DAG.cc    Definition of @ref fabrique::dag::DAG. */
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

#include "AST/ast.h"
#include "AST/Visitor.h"

#include "DAG/Build.h"
#include "DAG/DAG.h"
#include "DAG/File.h"
#include "DAG/List.h"
#include "DAG/Primitive.h"
#include "DAG/Rule.h"
#include "DAG/Structure.h"
#include "DAG/Target.h"
#include "DAG/UndefinedValueException.h"
#include "DAG/Value.h"

#include "Support/Bytestream.h"
#include "Support/Join.h"
#include "Support/exceptions.h"

#include "Types/FileType.h"
#include "Types/FunctionType.h"
#include "Types/Type.h"
#include "Types/TypeError.h"

#include "FabContext.h"

#include <cassert>
#include <deque>
#include <stack>

using namespace fabrique;
using namespace fabrique::dag;

using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::string;
using std::vector;

template<class T>
using ConstPtr = const std::unique_ptr<T>;


namespace {

class ImmutableDAG : public DAG
{
public:
	ImmutableDAG(string buildroot, string srcroot,
	             SharedPtrVec<File>& files, SharedPtrVec<Build>& builds,
	             SharedPtrMap<Rule>& rules, SharedPtrMap<Value>& variables,
	             SharedPtrMap<Target>& targets, vector<BuildTarget>& top);

	const string& buildroot() const { return buildroot_; }
	const string& srcroot() const { return srcroot_; }

	const SharedPtrVec<File>& files() const override { return files_; }
	const SharedPtrVec<Build>& builds() const override { return builds_; }
	const SharedPtrMap<Rule>& rules() const override { return rules_; }
	const SharedPtrMap<Value>& variables() const override { return vars_; }
	const SharedPtrMap<Target>& targets() const override
	{
		return targets_;
	}

	const vector<BuildTarget>& topLevelTargets() const override
	{
		return topLevelTargets_;
	}

private:
	const string buildroot_;
	const string srcroot_;

	const SharedPtrVec<File> files_;
	const SharedPtrVec<Build> builds_;
	const SharedPtrMap<Rule> rules_;
	const SharedPtrMap<Value> vars_;
	const SharedPtrMap<Target> targets_;
	const vector<BuildTarget> topLevelTargets_;
};

}


namespace fabrique {
namespace dag {



//! AST Visitor that flattens the AST into a DAG by evaluating expressions.
class DAGBuilder : public ast::Visitor
{
public:
	DAGBuilder(FabContext& ctx)
		: ctx_(ctx)

	{
	}

	~DAGBuilder() {}

	VISIT(ast::Action)
	VISIT(ast::Argument)
	VISIT(ast::BinaryOperation)
	VISIT(ast::BoolLiteral)
	VISIT(ast::Call)
	VISIT(ast::CompoundExpression)
	VISIT(ast::Conditional)
	VISIT(ast::FieldAccess)
	VISIT(ast::Filename)
	VISIT(ast::FileList)
	VISIT(ast::ForeachExpr)
	VISIT(ast::Function)
	VISIT(ast::Identifier)
	VISIT(ast::Import)
	VISIT(ast::IntLiteral)
	VISIT(ast::List)
	VISIT(ast::Parameter)
	VISIT(ast::Scope)
	VISIT(ast::StringLiteral)
	VISIT(ast::SymbolReference)
	VISIT(ast::UnaryOperation)
	VISIT(ast::Value)

	// The values we're creating:
	SharedPtrVec<File> files_;
	SharedPtrVec<Build> builds_;
	SharedPtrMap<Rule> rules_;
	SharedPtrMap<Value> variables_;
	SharedPtrMap<Target> targets_;
	vector<DAG::BuildTarget> topLevelTargets_;

private:
	ValueMap& EnterScope(const string& name);
	ValueMap ExitScope();
	ValueMap& CurrentScope();

	//! Get a named value from the current scope or a parent scope.
	shared_ptr<Value> getNamedValue(const std::string& name);

	shared_ptr<Value> eval(const ast::Expression&);


	FabContext& ctx_;

	//! The components of the current scope's fully-qualified name.
	std::deque<string> scopeName;

	//! Symbols defined in this scope (or the one up from it, or up...).
	std::deque<ValueMap> scopes;

	/** The name of the value we are currently processing. */
	std::stack<string> currentValueName;

	/**
	 * The value currently being processed.
	 * Visitor methods should leave a single item on this stack.
	 */
	std::stack<shared_ptr<Value>> currentValue;
};

} // namespace dag
} // namespace fabrique


UniqPtr<DAG> DAG::Flatten(const ast::Scope& root, FabContext& ctx)
{
	DAGBuilder builder(ctx);
	root.Accept(builder);

	return UniqPtr<DAG>(new ImmutableDAG(
		ctx.buildroot(), ctx.srcroot(),
		builder.files_, builder.builds_, builder.rules_,
		builder.variables_, builder.targets_, builder.topLevelTargets_));
}


void DAG::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	SharedPtrMap<Value> namedValues;
	for (auto& i : rules()) namedValues.emplace(i);
	for (auto& i : targets()) namedValues.emplace(i);
	for (auto& i : variables()) namedValues.emplace(i);

	for (auto& i : namedValues)
	{
		const string& name = i.first;
		const shared_ptr<Value>& v = i.second;

		assert(v);

		out
			<< Bytestream::Type << v->type()
			<< Bytestream::Definition << " " << name
			<< Bytestream::Operator << " = "
			<< *v
			<< Bytestream::Reset << "\n"
			;
	}

	for (const shared_ptr<File>& f : files())
	{
		out
			<< Bytestream::Type << f->type()
			<< Bytestream::Operator << ": "
			<< *f
			<< Bytestream::Reset << "\n"
			;
	}

	for (const shared_ptr<Build>& b : builds())
	{
		out
			<< Bytestream::Type << "build"
			<< Bytestream::Operator << ": "
			<< *b
			<< Bytestream::Reset << "\n"
			;
	}
}


ImmutableDAG::ImmutableDAG(
		string buildroot, string sourceroot,
		SharedPtrVec<File>& files, SharedPtrVec<Build>& builds,
		SharedPtrMap<Rule>& rules, SharedPtrMap<Value>& variables,
		SharedPtrMap<Target>& targets, vector<BuildTarget>& topLevelTargets)
	: buildroot_(buildroot), srcroot_(sourceroot),
	  files_(files), builds_(builds), rules_(rules),
	  vars_(variables), targets_(targets), topLevelTargets_(topLevelTargets)
{
}


bool DAGBuilder::Enter(const ast::Action& a)
{
	string command;
	ValueMap arguments;

	if (a.arguments().size() < 1)
		throw SemanticException("Missing action arguments", a.source());

	for (ConstPtr<ast::Argument>& arg : a.arguments())
	{
		// Evaluate the argument as a string.
		shared_ptr<Value> value = eval(arg->getValue());

		// The only keyword-less argument to action() is its command.
		if (not arg->hasName() or arg->getName().name() == "command")
		{
			if (not command.empty())
				throw SemanticException(
					"Duplicate command", arg->source());

			command = value->str();
			continue;
		}

		shared_ptr<Value> v(new String(value->str(), ctx_.stringType(),
		                               arg->source()));
		arguments.emplace(arg->getName().name(), v);
	}

	// Ensure that files are properly tagged as input or output.
	for (ConstPtr<ast::Parameter>& p : a.parameters())
		FileType::CheckFileTags(p->type(), p->source());

	currentValue.emplace(Rule::Create(currentValueName.top(),
	                                  command, arguments, a.type()));

	return false;
}

void DAGBuilder::Leave(const ast::Action&) {}


bool DAGBuilder::Enter(const ast::Argument& arg)
{
	currentValue.emplace(eval(arg.getValue()));
	return false;
}

void DAGBuilder::Leave(const ast::Argument&) {}


bool DAGBuilder::Enter(const ast::BinaryOperation& o)
{
	shared_ptr<Value> lhs = eval(o.getLHS());
	shared_ptr<Value> rhs = eval(o.getRHS());

	assert(lhs and rhs);

	shared_ptr<Value> result;
	switch (o.getOp())
	{
		case ast::BinaryOperation::Add:
			result = lhs->Add(rhs);
			break;

		case ast::BinaryOperation::Prefix:
			result = rhs->PrefixWith(lhs);
			break;

		case ast::BinaryOperation::ScalarAdd:
			if (lhs->canScalarAdd(*rhs))
				result = lhs->ScalarAdd(rhs);

			else if (rhs->canScalarAdd(*lhs))
				result = rhs->ScalarAdd(lhs);

			else
				throw SemanticException(
					"invalid types for addition",
					SourceRange::Over(lhs.get(), rhs.get())
				);
			break;

		case ast::BinaryOperation::And:
			result = lhs->And(rhs);
			break;

		case ast::BinaryOperation::Or:
			result = lhs->Or(rhs);
			break;

		case ast::BinaryOperation::Xor:
			result = lhs->Xor(rhs);
			break;

		case ast::BinaryOperation::Invalid:
			break;
	}

	assert(result);
	currentValue.emplace(result);

	return false;
}

void DAGBuilder::Leave(const ast::BinaryOperation&) {}


bool DAGBuilder::Enter(const ast::BoolLiteral& b)
{
	currentValue.emplace(
		new Boolean(b.value(), b.type(), b.source()));

	return false;
}

void DAGBuilder::Leave(const ast::BoolLiteral&) {}


bool DAGBuilder::Enter(const ast::Call&) { return false; }
void DAGBuilder::Leave(const ast::Call& call)
{
	const ast::SymbolReference& ref = call.target();
	const string name = ref.name().str();
	auto& target = dynamic_cast<const ast::Callable&>(ref.definition());

	ValueMap args;
	for (auto& i : target.NameArguments(call.arguments()))
		args[i.first] = std::move(eval(*i.second));


	//
	// The target must be an action or a function.
	//
	if (auto rule = dynamic_pointer_cast<Rule>(getNamedValue(name)))
	{
		// Builds need the parameter types, not just the argument types.
		ConstPtrMap<Type> paramTypes;
		auto& params = target.parameters();
		for (auto& a : args)
		{
			const string& argName = a.first;

			auto i = std::find_if(params.begin(), params.end(),
				[&argName](const UniqPtr<ast::Parameter>& p)
				{
					return p->getName().name() == argName;
				});

			assert(i != params.end());
			const Type& t = (*i)->type();
			paramTypes[argName] = &t;
		}

		shared_ptr<Build> build(
			Build::Create(rule, args, paramTypes, call.source()));

		builds_.push_back(build);
		for (const shared_ptr<File>& f : build->outputs())
			files_.push_back(f);

		currentValue.push(build);
	}
	else
	{
		auto& fn = dynamic_cast<const ast::Function&>(ref.definition());

		//
		// We evaluate the function with the given arguments by
		// putting these names into the local scope and then
		// entering the function's CompoundExpr.
		//
		ValueMap& scope = EnterScope("fn eval");

		for (auto& i : args)
			scope[i.first] = i.second;

		currentValue.emplace(eval(fn.body()));
		ExitScope();
	}
}


bool DAGBuilder::Enter(const ast::CompoundExpression& e)
{
	Enter(static_cast<const ast::Scope&>(e));
	return true;
}

void DAGBuilder::Leave(const ast::CompoundExpression& e)
{
	Leave(static_cast<const ast::Scope&>(e));
	assert(not currentValue.empty());
}


bool DAGBuilder::Enter(const ast::Conditional& c)
{
	shared_ptr<Boolean> cond =
		dynamic_pointer_cast<Boolean>(eval(c.condition()));

	if (not cond)
		throw WrongTypeException("bool", c.type(), c.source());

	// Evaluate either the "then" or the "else" clause.
	currentValue.emplace(
		eval(cond->value() ? c.thenClause() : c.elseClause())
	);

	return false;
}

void DAGBuilder::Leave(const ast::Conditional&) {}


bool DAGBuilder::Enter(const ast::FieldAccess&) { return true; }
void DAGBuilder::Leave(const ast::FieldAccess&) {}


bool DAGBuilder::Enter(const ast::Filename& f)
{
	const string filename = eval(f.name())->str();

	string subdirectory;
	for (const UniqPtr<ast::Argument>& a : f.arguments())
	{
		if (a->getName().name() == "subdir")
			subdirectory = eval(a->getValue())->str();

		else
			throw SemanticException("unknown argument", a->source());
	}

	shared_ptr<File> file(
		File::Create(subdirectory, filename, f.type(), f.source()));

	files_.push_back(file);
	currentValue.push(file);

	return false;
}
void DAGBuilder::Leave(const ast::Filename&) {}


bool DAGBuilder::Enter(const ast::FileList& l)
{
	SharedPtrVec<Value> files;
	string subdir;

	for (ConstPtr<ast::Argument>& arg : l.arguments())
	{
		const string name = arg->getName().name();
		if (name == ast::Subdirectory)
			subdir = eval(arg->getValue())->str();

		else
			throw SemanticException("unexpected argument",
			                        arg->source());
	}

	for (ConstPtr<ast::Filename>& file : l)
	{
		auto f = dynamic_pointer_cast<File>(eval(*file));
		assert(f);

		if (not subdir.empty())
			f->setSubdirectory(subdir);

		files.push_back(f);
	}

	currentValue.emplace(List::of(files, l.source()));

	return false;
}
void DAGBuilder::Leave(const ast::FileList&) {}


bool DAGBuilder::Enter(const ast::ForeachExpr& f)
{
	SharedPtrVec<Value> values;

	auto target = eval(f.sourceSequence());
	assert(target->type().isOrdered());
	assert(target->asList());

	//
	// For each input element, put its value in scope as the loop parameter
	// and then evaluate the CompoundExpression.
	//
	const ast::Parameter& loopParam = f.loopParameter();
	for (const shared_ptr<Value>& element : *target->asList())
	{
		assert(element->type().isSubtype(f.loopParameter().type()));

		ValueMap& scope = EnterScope("foreach body");
		scope[loopParam.getName().name()] = element;

		shared_ptr<Value> result = eval(f.loopBody());
		assert(result);
		assert(result->type().isSubtype(f.loopBody().type()));

		values.push_back(std::move(result));

		ExitScope();
	}

	currentValue.emplace(List::of(values, f.source()));
	return false;
}

void DAGBuilder::Leave(const ast::ForeachExpr&) {}


bool DAGBuilder::Enter(const ast::Function& fn)
{
	// Put parameters with default names into the function's scope.
	ValueMap& currentScope = CurrentScope();

	for (auto& p : fn.parameters())
	{
		if (const UniqPtr<ast::Expression>& v = p->defaultValue())
		{
			const string name(p->getName().name());
			assert(currentScope.find(name) == currentScope.end());

			currentScope.emplace(p->getName().name(), eval(*v));
		}
	}

	return false;
}

void DAGBuilder::Leave(const ast::Function&) { currentValueName.pop(); }


bool DAGBuilder::Enter(const ast::Identifier&) { return false; }
void DAGBuilder::Leave(const ast::Identifier&) {}


bool DAGBuilder::Enter(const ast::Import& import)
{
	std::vector<Structure::NamedValue> values;

	for (auto& i : import.scope())
		values.emplace_back(i.first, eval(*i.second));

	currentValue.emplace(Structure::Create(values, import.type()));

	return false;
}

void DAGBuilder::Leave(const ast::Import&) {}


bool DAGBuilder::Enter(const ast::IntLiteral& i)
{
	currentValue.emplace(
		new Integer(i.value(), i.type(), i.source()));

	return false;
}

void DAGBuilder::Leave(const ast::IntLiteral&) {}


bool DAGBuilder::Enter(const ast::List& l)
{
	assert(l.type().name() == "list");
	assert(l.type().typeParamCount() == 1);
	const Type& subtype = l.type()[0];

	SharedPtrVec<Value> values;

	for (ConstPtr<ast::Expression>& e : l)
	{
		if (!e->type().isSubtype(subtype))
			throw WrongTypeException(subtype,
			                         e->type(), e->source());

		values.push_back(eval(*e));
	}

	currentValue.emplace(new List(values, l.type(), l.source()));

	return false;
}

void DAGBuilder::Leave(const ast::List&) {}


bool DAGBuilder::Enter(const ast::Parameter&) { return false; }
void DAGBuilder::Leave(const ast::Parameter&) {}


bool DAGBuilder::Enter(const ast::Scope&)
{
	EnterScope("AST scope");
	return false;
}

void DAGBuilder::Leave(const ast::Scope&)
{
	ValueMap scopedSymbols = ExitScope();

	// We only save top-level values if we are, in fact, at the top level.
	if (not scopes.empty())
		return;

	const string currentScopeName = join(this->scopeName, ".");

	for (auto symbol : scopedSymbols)
	{
		const string name = join(currentScopeName, symbol.first, ".");
		shared_ptr<Value>& v = symbol.second;

		if (auto rule = dynamic_pointer_cast<Rule>(v))
		{
			rules_[name] = rule;
		}
		else if (auto target = dynamic_pointer_cast<Target>(v))
		{
			targets_[name] = target;
			variables_[name] = target->files();
		}
		else if (dynamic_pointer_cast<Build>(v))
		{
			// Builds should not be included in variables.
		}
		else
		{
			variables_[name] = v;
		}
	}
}


bool DAGBuilder::Enter(const ast::StringLiteral& s)
{
	currentValue.emplace(new String(s.str(), s.type(), s.source()));
	return false;
}

void DAGBuilder::Leave(const ast::StringLiteral&) {}


bool DAGBuilder::Enter(const ast::SymbolReference& r)
{
	const string& name = r.name().str();

	shared_ptr<Value> value = getNamedValue(name);
	if (not value)
		throw UndefinedValueException(name, r.source());

	currentValue.emplace(value);

	return false;
}

void DAGBuilder::Leave(const ast::SymbolReference&) {}


bool DAGBuilder::Enter(const ast::UnaryOperation& o)
{
	shared_ptr<Value> subexpr = eval(o.getSubExpr());
	assert(subexpr);

	shared_ptr<Value> result;
	switch (o.getOp())
	{
		case ast::UnaryOperation::Negate:
			result = subexpr->Negate(o.source());
			break;

		case ast::UnaryOperation::Invalid:
			break;
	}

	assert(result);
	currentValue.emplace(result);

	return false;
}

void DAGBuilder::Leave(const ast::UnaryOperation&) {}


bool DAGBuilder::Enter(const ast::Value& v)
{
	currentValueName.push(v.name().name());
	return true;
}

void DAGBuilder::Leave(const ast::Value&)
{
	// Things like function definitions will never be evaluated.
	if (currentValue.empty())
		return;

	ValueMap& currentScope = CurrentScope();

	Bytestream& dbg = Bytestream::Debug("dag.scope");
	dbg
		<< Bytestream::Action << "defining "
		<< Bytestream::Literal << "'" << currentValueName.top() << "'"
		<< Bytestream::Operator << ":"
		;

	shared_ptr<Value> val = std::move(currentValue.top());
	currentValue.pop();

	const string name = currentValueName.top();
	currentValueName.pop();


	//
	// If the right-hand side is a build, file or list of files,
	// convert to a named target (files and builds are already in the DAG).
	//
	if (auto build = dynamic_pointer_cast<Build>(val))
		val.reset(Target::Create(name, build));

	else if (auto file = dynamic_pointer_cast<File>(val))
		val.reset(Target::Create(name, file));

	else if (auto list = dynamic_pointer_cast<List>(val))
	{
		if (list->type().elementType().isFile())
			val.reset(Target::Create(name, list));
	}

	if (currentValueName.empty())
		topLevelTargets_.emplace_back(name, val);

	currentScope.emplace(name, std::move(val));

	for (auto& i : currentScope)
		dbg << Bytestream::Definition << " " << i.first;

	dbg << Bytestream::Reset << "\n";
}


ValueMap& DAGBuilder::EnterScope(const string& name)
{
	Bytestream::Debug("dag.scope")
		<< string(scopes.size(), ' ')
		<< Bytestream::Operator << " >> "
		<< Bytestream::Type << "scope"
		<< Bytestream::Literal << " '" << name << "'"
		<< Bytestream::Reset << "\n"
		;

	scopes.push_back(ValueMap());
	return CurrentScope();
}

ValueMap DAGBuilder::ExitScope()
{
	ValueMap values = std::move(CurrentScope());
	scopes.pop_back();

	Bytestream& dbg = Bytestream::Debug("parser.scope");
	dbg
		<< string(scopes.size(), ' ')
		<< Bytestream::Operator << " << "
		<< Bytestream::Type << "scope"
		<< Bytestream::Operator << ":"
		;

	for (auto& i : values)
		dbg << " " << i.first;

	dbg << Bytestream::Reset << "\n";

	return std::move(values);
}

ValueMap& DAGBuilder::CurrentScope()
{
	return scopes.back();
}


shared_ptr<Value> DAGBuilder::getNamedValue(const string& name)
{
	Bytestream& dbg = Bytestream::Debug("dag.lookup");
	dbg
		<< Bytestream::Action << "lookup "
		<< Bytestream::Literal << "'" << name << "'"
		<< Bytestream::Reset << "\n"
		;

	for (auto i = scopes.rbegin(); i != scopes.rend(); i++)
	{
		const ValueMap& scope = *i;

		auto value = scope.find(name);
		if (value != scope.end())
		{
			assert(value->second);

			dbg
				<< Bytestream::Action << "  found "
				<< Bytestream::Literal << "'" << name << "'"
				<< Bytestream::Operator << ": "
				<< *value->second
				<< Bytestream::Reset << "\n"
				;
			return value->second;
		}

		dbg
			<< "  no "
			<< Bytestream::Literal << "'" << name << "'"
			<< Bytestream::Operator << ":"
			;

		for (auto& j : scope)
			dbg << " " << Bytestream::Definition << j.first;

		dbg << Bytestream::Reset << "\n";
	}

	return NULL;
}

shared_ptr<Value> DAGBuilder::eval(const ast::Expression& e)
{
	e.Accept(*this);

	assert(not currentValue.empty());
	shared_ptr<Value> v(currentValue.top());
	currentValue.pop();

	return v;
}
