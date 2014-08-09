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
#include "AST/Builtins.h"
#include "AST/Visitor.h"

#include "DAG/Build.h"
#include "DAG/Callable.h"
#include "DAG/DAG.h"
#include "DAG/File.h"
#include "DAG/Function.h"
#include "DAG/List.h"
#include "DAG/Parameter.h"
#include "DAG/Primitive.h"
#include "DAG/Rule.h"
#include "DAG/Structure.h"
#include "DAG/Target.h"
#include "DAG/UndefinedValueException.h"
#include "DAG/Value.h"

#include "Support/Arguments.h"
#include "Support/Bytestream.h"
#include "Support/Join.h"
#include "Support/exceptions.h"
#include "Support/os.h"

#include "Types/FileType.h"
#include "Types/FunctionType.h"
#include "Types/Type.h"
#include "Types/TypeContext.h"
#include "Types/TypeError.h"

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


void DebugNewDefinition(const ValueMap& scope, string name)
{
	assert(not scope.empty());
	assert(scope.find(name) != scope.end());

	Bytestream& dbg = Bytestream::Debug("dag.scope");
	dbg
		<< Bytestream::Action << "defined "
		<< Bytestream::Literal << "'" << name << "'"
		<< Bytestream::Operator << " = "
		<< Bytestream::Reset << *scope.find(name)->second
		<< Bytestream::Reset << "\n"
		;
}

}


namespace fabrique {
namespace dag {



//! AST Visitor that flattens the AST into a DAG by evaluating expressions.
class DAGBuilder : public ast::Visitor
{
public:
	DAGBuilder(TypeContext& ctx)
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
	VISIT(ast::FieldQuery)
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
	VISIT(ast::SomeValue)
	VISIT(ast::StringLiteral)
	VISIT(ast::StructInstantiation)
	VISIT(ast::SymbolReference)
	VISIT(ast::UnaryOperation)
	VISIT(ast::Value)

	//! Add build steps to regenerate the output when Fabrique files change.
	void AddRegeneration(const Arguments& regenArgs,
                             const vector<string>& inputFiles, string outputFile);

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
	void DumpScope();

	//! Make a deep copy of the current scope and all of its parents.
	ValueMap CopyCurrentScope();

	//! Get a named value from the current scope or a parent scope.
	ValuePtr getNamedValue(const std::string& name);

	//! Evaluate an expression as, well, a @ref Value.
	ValuePtr eval(const ast::Expression&);

	//! Parameters aren't really values: we can't store them, etc.
	Parameter* ConvertParameter(const ast::Parameter&);

	TypeContext& ctx_;

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
	std::stack<ValuePtr> currentValue;
};

} // namespace dag
} // namespace fabrique


UniqPtr<DAG> DAG::Flatten(const ast::Scope& root, TypeContext& ctx,
                          string srcroot, string buildroot,
                          const vector<string>& inputFiles,
                          string outputFile, const Arguments& regenArgs)
{
	DAGBuilder builder(ctx);
	root.Accept(builder);

	//
	// If we're generating a real output file (not stdout), add build logic
	// to re-generate when input Fabrique files change.
	//
	if (not outputFile.empty())
		builder.AddRegeneration(regenArgs, inputFiles, outputFile);

	//
	// Ensure all files are unique.
	//
	SharedPtrVec<File>& f = builder.files_;
	std::sort(f.begin(), f.end(), File::LessThan);
	f.erase(std::unique(f.begin(), f.end(), File::Equals), f.end());

	return UniqPtr<DAG>(new ImmutableDAG(
		buildroot, srcroot, builder.files_, builder.builds_, builder.rules_,
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
		const ValuePtr& v = i.second;

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
		ValuePtr value = eval(arg->getValue());

		// The only keyword-less argument to action() is its command.
		if (not arg->hasName() or arg->getName().name() == "command")
		{
			if (not command.empty())
				throw SemanticException(
					"Duplicate command", arg->source());

			command = value->str();
			continue;
		}

		ValuePtr v(new String(value->str(), ctx_.stringType(),
		                               arg->source()));
		arguments.emplace(arg->getName().name(), v);
	}

	SharedPtrVec<Parameter> parameters;
	for (ConstPtr<ast::Parameter>& p : a.parameters())
	{
		// Ensure that files are properly tagged as input or output.
		FileType::CheckFileTags(p->type(), p->source());

		parameters.emplace_back(ConvertParameter(*p));
	}

	shared_ptr<Rule> rule(Rule::Create(currentValueName.top(),
	                                   command, arguments, parameters,
	                                   a.type()));
	currentValue.emplace(rule);
	rules_[rule->name()] = rule;

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
	ValuePtr lhs = eval(o.getLHS());
	ValuePtr rhs = eval(o.getRHS());

	assert(lhs and rhs);

	ValuePtr result;
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

		case ast::BinaryOperation::Equal:
			result = lhs->Equals(rhs);
			break;

		case ast::BinaryOperation::NotEqual:
			result = lhs->Equals(rhs)->Negate(o.source());
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
	static Bytestream& debug = Bytestream::Debug("dag.call");
	ValuePtr value = eval(call.target());

	auto target = dynamic_pointer_cast<Callable>(value);
	assert(target);

	//
	// Check argument legality.
	//
	for (auto& a : call.arguments())
		if (a->hasName()
		    and not target->hasParameterNamed(a->getName().name()))
			// TODO: argument, not parameter!
			throw SemanticException(
				"invalid parameter", a->source());

	debug
		<< Bytestream::Action << "calling "
		<< Bytestream::Reset << call.target()
		<< Bytestream::Reset << " with arguments:\n"
		;

	ValueMap args;
	StringMap<SourceRange> argLocations;
	for (auto& i : target->NameArguments(call.arguments()))
	{
		const string name = i.first;
		const ast::Argument& arg = *i.second;
		ValuePtr argValue { eval(arg) };

		debug
			<< Bytestream::Operator << " - "
			<< Bytestream::Reset << arg
			<< Bytestream::Operator << " = "
			<< *argValue
			<< Bytestream::Reset << "\n"
			;

		argLocations.emplace(name, arg.source());
		args[name] = std::move(argValue);
	}

	target->CheckArguments(args, argLocations, call.source());

	//
	// The target must be an action or a function.
	//
	if (auto rule = dynamic_pointer_cast<Rule>(target))
	{
		// Builds need the parameter types, not just the argument types.
		ConstPtrMap<Type> paramTypes;
		for (auto& p : target->parameters())
			paramTypes[p->name()] = &p->type();

		shared_ptr<Build> build(
			Build::Create(rule, args, paramTypes, call.source()));

		builds_.push_back(build);

		for (const shared_ptr<File>& f : build->inputs())
			files_.push_back(f);

		for (const shared_ptr<File>& f : build->outputs())
			files_.push_back(f);

		currentValue.push(build);
	}
	else if (auto fn = dynamic_pointer_cast<Function>(target))
	{
		//
		// When executing a function, we don't use symbols in scope
		// at the call site, only those in scope at the function
		// definition site.
		//
		std::deque<ValueMap> callSiteScopes(std::move(scopes));
		scopes.push_back(fn->scope());

		//
		// We evaluate the function with the given arguments by
		// putting these names into the local scope and then
		// entering the function's CompoundExpr.
		//
		ValueMap& scope = EnterScope("fn eval");

		for (auto& i : args)
			scope[i.first] = i.second;

		// Get default parameter values.
		for (auto& p : fn->function().parameters())
		{
			if (const UniqPtr<ast::Expression>& v = p->defaultValue())
			{
				const string name(p->getName().name());
				if (scope.find(name) != scope.end())
					continue;

				scope.emplace(p->getName().name(), eval(*v));
			}
		}

		currentValue.emplace(eval(fn->function().body()));
		ExitScope();

		// Pop the function's containing scope.
		ExitScope();

		// Go back to the normal scope stack.
		scopes = std::move(callSiteScopes);
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


bool DAGBuilder::Enter(const ast::FieldAccess& f)
{
	shared_ptr<Structure> base(dynamic_pointer_cast<Structure>(eval(f.base())));
	if (not base)
		throw SemanticException("base of field access is not a structure",
		                        f.base().source());

	const string fieldName(f.field().name());
	currentValue.emplace(base->field(fieldName));

	return false;
}

void DAGBuilder::Leave(const ast::FieldAccess&) {}


bool DAGBuilder::Enter(const ast::FieldQuery& q)
{
	const ast::Scope& scope =
		dynamic_cast<const ast::HasScope&>(q.base().definition()).scope();

	if (scope.contains(q.field()))
		currentValue.emplace(eval(*scope.Lookup(q.field())));
	else
		currentValue.emplace(eval(q.defaultValue()));

	return false;
}

void DAGBuilder::Leave(const ast::FieldQuery&) {}


bool DAGBuilder::Enter(const ast::Filename& f)
{
	const string filename = eval(f.name())->str();

	assert(getNamedValue(ast::Subdirectory));
	string subdirectory = getNamedValue(ast::Subdirectory)->str();

	for (const UniqPtr<ast::Argument>& a : f.arguments())
	{
		if (a->getName().name() == ast::Subdirectory)
			subdirectory = eval(a->getValue())->str();

		else
			throw SemanticException("unknown argument", a->source());
	}

	shared_ptr<File> file(
		File::Create(subdirectory, filename, f.type(), f.source()));

	currentValue.push(file);

	return false;
}
void DAGBuilder::Leave(const ast::Filename&) {}


bool DAGBuilder::Enter(const ast::FileList& l)
{
	assert(getNamedValue(ast::Subdirectory));
	const string subdir = getNamedValue(ast::Subdirectory)->str();

	ValueMap& scope = EnterScope("files");
	SharedPtrVec<Value> files;

	for (ConstPtr<ast::Argument>& arg : l.arguments())
	{
		const string name = arg->getName().name();
		if (name == ast::Subdirectory)
		{
			const string subsubdir = eval(arg->getValue())->str();
			const string completeSubdir = JoinPath(subdir, subsubdir);

			scope[name].reset(
				new String(completeSubdir, ctx_.stringType(),
				           arg->getValue().source())
			);
		}

		else
			throw SemanticException("unexpected argument",
			                        arg->source());
	}

	for (ConstPtr<ast::Filename>& file : l)
		files.push_back(dynamic_pointer_cast<File>(eval(*file)));

	ExitScope();

	currentValue.emplace(List::of(files, l.source(), ctx_));

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
	for (const ValuePtr& element : *target->asList())
	{
		assert(element->type().isSubtype(f.loopParameter().type()));

		ValueMap& scope = EnterScope("foreach body");
		scope[loopParam.getName().name()] = element;

		ValuePtr result = eval(f.loopBody());
		assert(result);
		assert(result->type().isSubtype(f.loopBody().type()));

		values.push_back(std::move(result));

		ExitScope();
	}

	currentValue.emplace(List::of(values, f.source(), ctx_));
	return false;
}

void DAGBuilder::Leave(const ast::ForeachExpr&) {}


bool DAGBuilder::Enter(const ast::Function& fn)
{
	ValueMap scope(CopyCurrentScope());

	Bytestream& dbg = Bytestream::Debug("dag.fnscope");
	dbg << Bytestream::Action << "Copied scope:\n";
	for (auto i : scope)
		dbg << "  "
			<< Bytestream::Definition << i.first
			<< Bytestream::Operator << ":"
			<< *i.second
			<< "\n"
			;

	SharedPtrVec<Parameter> parameters;
	for (auto& p : fn.parameters())
		parameters.emplace_back(ConvertParameter(*p));

	currentValue.emplace(
		new dag::Function(fn, parameters, std::move(scope)));

	return false;
}

void DAGBuilder::Leave(const ast::Function&) {}


bool DAGBuilder::Enter(const ast::Identifier&) { return false; }
void DAGBuilder::Leave(const ast::Identifier&) {}


bool DAGBuilder::Enter(const ast::Import& import)
{
	const string name = currentValueName.top();
	std::vector<Structure::NamedValue> values;

	ValueMap& importScope = EnterScope("import(" + name + ")");
	ValuePtr subdir(
		new String(import.subdirectory(), ctx_.stringType()));
	CurrentScope().emplace(ast::Subdirectory, subdir);

	//
	// Import expressions can take arguments, which are exposed as an 'args'
	// struct in the imported module.  We can't initialize a structure with
	// ast::Arguments, though (it requires Values), so we have to manually
	// build the 'args' struct here.
	//
	vector<Structure::NamedValue> args;
	Type::NamedTypeVec argTypes;

	for (const UniqPtr<ast::Argument>& a : import.arguments())
	{
		if (not a->hasName())
			throw SemanticException("import arguments must be named",
			                        a->source());

		const string argName { a->getName().name() };

		argTypes.emplace_back(argName, a->type());
		args.emplace_back(argName, eval(a->getValue()));
	}

	ValuePtr argStruct { Structure::Create(args, ctx_.structureType(argTypes)) };
	importScope[ast::Arguments] = argStruct;
	DebugNewDefinition(importScope, ast::Arguments);

	EnterScope(name);

	for (const UniqPtr<ast::Value>& v : import.scope().values())
		// Ignore the AST-provided 'args'.
		if (v->name().name() != ast::Arguments)
			eval(*v);

	const ValueMap& scope = ExitScope();
	ExitScope();

	for (auto& i : scope)
		values.emplace_back(i.first, i.second);

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


bool DAGBuilder::Enter(const ast::Scope& scope)
{
	EnterScope(scope.name());
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
		ValuePtr& v = symbol.second;

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


bool DAGBuilder::Enter(const ast::SomeValue& s)
{
	vector<Structure::NamedValue> values;
	for (auto& v : s.scope().values())
		values.emplace_back(v->name().str(), eval(v->value()));

	currentValue.emplace(Structure::Create(values, s.type()));
	return false;
}

void DAGBuilder::Leave(const ast::SomeValue&) {}

bool DAGBuilder::Enter(const ast::StringLiteral& s)
{
	currentValue.emplace(new String(s.str(), s.type(), s.source()));
	return false;
}

void DAGBuilder::Leave(const ast::StringLiteral&) {}


bool DAGBuilder::Enter(const ast::StructInstantiation& s) {
	EnterScope("struct");

	for (auto& field : s.scope().values())
		eval(*field);

	ValueMap structScope = ExitScope();

	vector<Structure::NamedValue> values;
	for (auto& i : structScope)
		values.emplace_back(i.first, i.second);

	currentValue.emplace(Structure::Create(values, s.type()));
	return false;
}

void DAGBuilder::Leave(const ast::StructInstantiation&) {}


bool DAGBuilder::Enter(const ast::SymbolReference& r)
{
	static Bytestream& debug = Bytestream::Debug("dag.lookup");
	const string& name = Type::UntypedPart(r.name().str());

	shared_ptr<Structure> base;
	ValuePtr value;

	//
	// A symbol reference can have multiple dot-separated components:
	//
	// foo = bar.baz.wibble;
	//
	// In this case, 'bar' and 'bar.baz' must both be structures
	// (things that can contain named things), but 'wibble' can
	// be any kind of Value.
	//
	// Iterate over each name component.
	//
	for (size_t begin = 0, end; begin < name.length(); begin = end + 1)
	{
		end = name.find('.', begin + 1);
		const string component = name.substr(begin, end - begin);

		debug
			<< Bytestream::Action << "lookup component "
			<< Bytestream::Operator << "'"
			<< Bytestream::Literal << component
			<< Bytestream::Operator << "'"
			<< Bytestream::Reset << "\n"
			;

		value = base
			? base->field(component)
			: getNamedValue(component);

		if (not value)
			throw UndefinedValueException(
				name.substr(0, end), r.source());

		// Is this the last component?
		if (end == string::npos)
			break;

		// Not the last component: must be a structure!
		base = dynamic_pointer_cast<Structure>(value);
		if (not base)
			throw SemanticException(
				name.substr(0, end)
				+ " (" + typeid(*value).name() + ") is not a structure",
				r.source());
	}

	currentValue.emplace(value);
	return false;
}

void DAGBuilder::Leave(const ast::SymbolReference&) {}


bool DAGBuilder::Enter(const ast::UnaryOperation& o)
{
	ValuePtr subexpr = eval(o.getSubExpr());
	assert(subexpr);

	ValuePtr result;
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
	assert(not currentValue.empty());

	ValueMap& currentScope = CurrentScope();

	ValuePtr val = std::move(currentValue.top());
	currentValue.pop();
	assert(val);

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

	assert(val);
	currentScope.emplace(name, std::move(val));
	DebugNewDefinition(currentScope, name);

	assert(not val);
	currentValue.emplace(nullptr);
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
	assert(not scopes.empty());
	return scopes.back();
}

void DAGBuilder::DumpScope()
{
	Bytestream& out = Bytestream::Debug("dag.scope");
	size_t depth = 0;

	out
		<< Bytestream::Operator << "---------------------------\n"
		<< Bytestream::Definition << "Scopes (parent -> current):\n"
		<< Bytestream::Operator << "---------------------------\n"
		;

	for (auto scope = scopes.begin(); scope != scopes.end(); scope++)
	{
		const string indent("  ", depth);
		for (auto i : *scope)
		{
			const string name(i.first);
			const ValuePtr value(i.second);

			out << indent
				<< Bytestream::Operator << "- "
				<< Bytestream::Definition << name
				<< Bytestream::Operator << ": "
				<< *value
				<< Bytestream::Reset << "\n"
				;
		}

		depth++;
	}

	out
		<< Bytestream::Operator << "---------------------------\n"
		<< Bytestream::Reset
		;
}

ValueMap DAGBuilder::CopyCurrentScope()
{
	ValueMap copy;

	for (auto i = scopes.rbegin(); i != scopes.rend(); i++)
	{
		// Unfortunately, std::copy() doesn't work here because
		// it wants to copy *into* the string part of a const
		// pair<string,SharedPtr<Value>>.
		for (auto j : *i)
		{
			string name = j.first;
			ValuePtr value = j.second;

			copy.emplace(name, value);
		}
	}

	return copy;
}


ValuePtr DAGBuilder::getNamedValue(const string& name)
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

ValuePtr DAGBuilder::eval(const ast::Expression& e)
{
	e.Accept(*this);

	assert(not currentValue.empty());
	ValuePtr v(currentValue.top());
	currentValue.pop();

	return v;
}


void DAGBuilder::AddRegeneration(const Arguments& regenArgs,
                                 const vector<string>& inputFiles, string outputFile)
{
	shared_ptr<Value> Nothing;
	const SourceRange& Nowhere = SourceRange::None();

	const Type& inputFileType = ctx_.inputFileType();
	const Type& inputType = ctx_.listOf(inputFileType, Nowhere);
	const Type& outputType = ctx_.outputFileType();
	const Type& buildType = ctx_.functionType(inputType, outputType);

	//
	// First, construct the rule that regenerates output:file[out]
	// given input:list[file[in]].
	//
	ValueMap ruleArgs;
	ruleArgs["description"].reset(
		new String("Regenerating ${output}", ctx_.stringType()));

	SharedPtrVec<Parameter> params;
	params.emplace_back(new Parameter("rootInput", inputFileType, Nothing));
	params.emplace_back(new Parameter("otherInputs", inputType, Nothing));
	params.emplace_back(new Parameter("output", outputType, Nothing));

	const string& Name { Rule::RegenerationRuleName() };
	const string Command { "fab" + Arguments::str(regenArgs) + " ${rootInput}" };

	shared_ptr<Rule> rule(Rule::Create(Name, Command, ruleArgs, params, buildType));
	rules_[Name] = rule;


	//
	// Now, construct the build step that drives the rule above in order
	// to actually generate the build file.
	//
	shared_ptr<File> rootInput;
	SharedPtrVec<File> otherInputs;
	for (const string& name : inputFiles)
	{
		shared_ptr<File> f(File::Create(name, inputFileType, Nowhere));

		if (rootInput)
			otherInputs.push_back(f);
		else
			rootInput = f;
	}

	ValueMap args;
	args["rootInput"] = rootInput;
	args["otherInputs"].reset(List::of(otherInputs, Nowhere, ctx_));
	args["output"].reset(File::Create(outputFile, outputType, Nowhere));

	ConstPtrMap<Type> paramTypes;
	for (auto& p : params)
		paramTypes[p->name()] = &p->type();

	builds_.emplace_back(Build::Create(rule, args, paramTypes, Nowhere));
}


Parameter* DAGBuilder::ConvertParameter(const ast::Parameter& p)
{
	const string name = p.getName().name();
	const Type& type = p.type();
	SourceRange src = p.source();

	ValuePtr defaultValue;
	if (auto& v = p.defaultValue())
		defaultValue = eval(*v);

	return new Parameter(name, type, defaultValue, src);
}
