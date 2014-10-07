/** @file DAG/EvalContext.cc    Definition of @ref fabrique::dag::EvalContext. */
/*
 * Copyright (c) 2014 Jonathan Anderson
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

#include "AST/Builtins.h"
#include "AST/Scope.h"
#include "AST/Value.h"

#include "DAG/Build.h"
#include "DAG/DAG.h"
#include "DAG/EvalContext.h"
#include "DAG/File.h"
#include "DAG/Function.h"
#include "DAG/List.h"
#include "DAG/Parameter.h"
#include "DAG/Primitive.h"
#include "DAG/Rule.h"
#include "DAG/Target.h"

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

using namespace fabrique;
using namespace fabrique::dag;

using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::string;
using std::vector;

namespace fabrique {
namespace ast {

class Value;

}
}


namespace {

class ImmutableDAG : public DAG
{
public:
	ImmutableDAG(string buildroot, string srcroot,
	             const SharedPtrVec<File>& files,
	             const SharedPtrVec<Build>& builds,
	             const SharedPtrMap<Rule>& rules,
	             const SharedPtrMap<Value>& variables,
	             const SharedPtrMap<Target>& targets,
	             vector<BuildTarget>& topLevelTargets)
		: buildroot_(buildroot), srcroot_(srcroot),
		  files_(files), builds_(builds), rules_(rules), vars_(variables),
		  targets_(targets), topLevelTargets_(topLevelTargets)
	{
	}

	const string& buildroot() const override { return buildroot_; }
	const string& srcroot() const override { return srcroot_; }

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


static ValuePtr AddRegeneration(EvalContext&, TypeContext&,
                                const Arguments& regenArgs,
                                const vector<string>& inputFiles,
                                const vector<string>& outputFiles);


UniqPtr<DAG> EvalContext::Evaluate(const ast::Scope& root, TypeContext& ctx,
                                 string srcroot, string buildroot,
                                 const vector<string>& inputFiles,
                                 const vector<string>& outputFiles,
                                 const Arguments& regenArgs)
{
	EvalContext builder(ctx);
	auto scope(builder.EnterScope("top level scope"));
	vector<DAG::BuildTarget> topLevelTargets;

	for (const UniqPtr<ast::Value>& v : root.values())
		topLevelTargets.emplace_back(
			v->name().name(), v->evaluate(builder));

	//
	// If we're generating a real output file (not stdout), add build logic
	// to re-generate when input Fabrique files change.
	//
	if (not outputFiles.empty())
		AddRegeneration(builder, ctx, regenArgs, inputFiles, outputFiles);

	//
	// Ensure all files are unique.
	//
	SharedPtrVec<class File> f = builder.files_;
	std::sort(f.begin(), f.end(), File::LessThan);
	f.erase(std::unique(f.begin(), f.end(), File::Equals), f.end());

	return UniqPtr<DAG>(new ImmutableDAG(
		buildroot, srcroot, f, builder.builds_, builder.rules_,
		builder.variables_, builder.targets_, topLevelTargets));
}


EvalContext::Scope::Scope(EvalContext& stack, string name, ValueMap& symbols)
	: stack_(stack), name_(name), symbols_(symbols)
{
}

EvalContext::Scope::Scope(Scope&& other)
	: stack_(other.stack_),
	  name_(std::move(other.name_)),
	  symbols_(other.symbols_)
{
}

EvalContext::Scope::~Scope()
{
	if (name_.empty())
		return;

	stack_.PopScope();
}

void EvalContext::Scope::set(string name, ValuePtr v)
{
	stack_.CurrentScope()[name] = v;
}

bool EvalContext::Scope::contains(const string& name)
{
	return (symbols_.find(name) != symbols_.end());
}

ValueMap EvalContext::Scope::leave()
{
	name_ = "";
	return std::move(stack_.PopScope());
}


EvalContext::Scope EvalContext::EnterScope(const string& name)
{
	Bytestream::Debug("eval.scope")
		<< string(scopes_.size(), ' ')
		<< Bytestream::Operator << " >> "
		<< Bytestream::Type << "scope"
		<< Bytestream::Literal << " '" << name << "'"
		<< Bytestream::Reset << "\n"
		;

	scopes_.push_back(ValueMap());
	return std::move(Scope(*this, name, CurrentScope()));
}


EvalContext::AlternateScoping::AlternateScoping(EvalContext& stack,
                                              std::deque<ValueMap>&& scopes)
	: stack_(stack), originalScopes_(scopes)
{
}

EvalContext::AlternateScoping::AlternateScoping(AlternateScoping&& other)
	: stack_(other.stack_), originalScopes_(std::move(other.originalScopes_))
{
}

EvalContext::AlternateScoping::~AlternateScoping()
{
	if (not originalScopes_.empty())
		stack_.scopes_ = std::move(originalScopes_);
}


EvalContext::AlternateScoping EvalContext::ChangeScopeStack(const ValueMap& altScope)
{
	std::deque<ValueMap> originalScopes = std::move(scopes_);
	scopes_.push_back(altScope);

	return AlternateScoping(*this, std::move(originalScopes));
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
	if (not name_.empty())
		done();
}

void EvalContext::ScopedValueName::done()
{
	string poppedName = stack_.PopValueName();
	assert(poppedName == name_);
}


EvalContext::ScopedValueName EvalContext::evaluating(const string& name)
{
	return ScopedValueName(*this, name);
}


ValueMap EvalContext::PopScope()
{
	assert(not scopes_.empty());

	ValueMap values = std::move(CurrentScope());
	scopes_.pop_back();

	Bytestream& dbg = Bytestream::Debug("parser.scope");
	dbg
		<< string(scopes_.size(), ' ')
		<< Bytestream::Operator << " << "
		<< Bytestream::Type << "scope"
		<< Bytestream::Operator << ":"
		;

	for (auto& i : values)
		dbg << " " << i.first;

	dbg << Bytestream::Reset << "\n";

	return std::move(values);
}

ValueMap& EvalContext::CurrentScope()
{
	assert(not scopes_.empty());
	return scopes_.back();
}

void EvalContext::DumpScope()
{
	Bytestream& out = Bytestream::Debug("dag.scope");
	size_t depth = 0;

	out
		<< Bytestream::Operator << "---------------------------\n"
		<< Bytestream::Definition << "Scopes (parent -> current):\n"
		<< Bytestream::Operator << "---------------------------\n"
		;

	for (auto scope = scopes_.begin(); scope != scopes_.end(); scope++)
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

ValueMap EvalContext::CopyCurrentScope()
{
	ValueMap copy;

	for (auto i = scopes_.rbegin(); i != scopes_.rend(); i++)
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


void EvalContext::Define(ScopedValueName& name, ValuePtr v)
{
	assert(&name.stack_ == this);

	ValueMap& currentScope = CurrentScope();
	if (currentScope.find(name.name_) != currentScope.end())
		throw SemanticException("redefining '" + name.name_ + "'",
		                        v->source());

	currentScope.emplace(name.name_, v);
	variables_.emplace(fullyQualifiedName(), v);
}


ValuePtr EvalContext::Lookup(const string& name)
{
	Bytestream& dbg = Bytestream::Debug("dag.lookup");
	dbg
		<< Bytestream::Action << "lookup "
		<< Bytestream::Literal << "'" << name << "'"
		<< Bytestream::Reset << "\n"
		;

	for (auto i = scopes_.rbegin(); i != scopes_.rend(); i++)
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

	// If we are looking for 'subdir' and haven't found it defined anywhere,
	// provide the top-level source subdirectory ('').
	if (name == ast::Subdirectory)
		return File("", ValueMap(), ctx_.fileType(), SourceRange::None());

	return nullptr;
}


string EvalContext::fullyQualifiedName() const
{
	return join(currentValueName_, ".");
}

string EvalContext::qualifyName(string name) const
{
	std::deque<string> copy(currentValueName_);
	copy.push_back(name);

	return join(copy, ".");
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


ValuePtr EvalContext::Bool(bool b, SourceRange src)
{
	return ValuePtr(new class Boolean(b, ctx_.booleanType(), src));
}


shared_ptr<class Build>
EvalContext::Build(shared_ptr<class Rule> rule, ValueMap arguments,
                 ConstPtrMap<Type>& paramTypes, SourceRange src)
{
	shared_ptr<class Build> b(
		Build::Create(rule, arguments, paramTypes, src));

	builds_.push_back(b);

	for (const shared_ptr<class File>& f : b->inputs())
		files_.push_back(f);

	for (const shared_ptr<class File>& f : b->outputs())
		files_.push_back(f);

	return b;
}


ValuePtr EvalContext::File(string fullPath, const ValueMap& attributes,
                         const FileType& t, const SourceRange& src)
{
	files_.emplace_back(File::Create(fullPath, attributes, t, src));
	return files_.back();
}

ValuePtr EvalContext::File(string subdir, string name, const ValueMap& attributes,
                         const FileType& t, const SourceRange& src)
{
	files_.emplace_back(File::Create(subdir, name, attributes, t, src));
	return files_.back();
}


ValuePtr EvalContext::Function(const ast::Function& fn,
                             const SharedPtrVec<Parameter>& params)
{
	return ValuePtr(new class Function(fn, params, CopyCurrentScope()));
}


ValuePtr EvalContext::Integer(int i, SourceRange src)
{
	return ValuePtr(new class Integer(i, ctx_.integerType(), src));
}



ValuePtr EvalContext::Rule(string command, const ValueMap& arguments,
                         const SharedPtrVec<Parameter>& parameters,
                         const Type& type, const SourceRange& source)
{
	const string name = fullyQualifiedName();

	shared_ptr<class Rule> r(
		Rule::Create(name, command, arguments, parameters,
		             type, source)
	);

	rules_[name] = r;

	return r;
}


ValuePtr EvalContext::String(const string& s, SourceRange src)
{
	return ValuePtr(new class String(s, ctx_.stringType(), src));
}


ValuePtr EvalContext::Struct(const vector<Structure::NamedValue>& values,
                           const Type& t, SourceRange source)
{
	return ValuePtr(Structure::Create(values, t, source));
}


ValuePtr EvalContext::Target(const shared_ptr<class Build>& b)
{
	assert(not currentValueName_.empty());

	const string fullName = fullyQualifiedName();

	shared_ptr<class Target> t(Target::Create(fullName, b));
	targets_[fullName] = t;

	return t;
}

ValuePtr EvalContext::Target(const shared_ptr<class File>& f)
{
	assert(not currentValueName_.empty());

	const string fullName = fullyQualifiedName();

	shared_ptr<class Target> t(Target::Create(fullName, f));
	targets_[fullName] = t;

	return t;
}

ValuePtr EvalContext::Target(const shared_ptr<class List>& l)
{
	assert(not currentValueName_.empty());

	const string fullName = fullyQualifiedName();

	shared_ptr<class Target> t(Target::Create(fullName, l));
	targets_[fullName] = t;

	return t;
}

void EvalContext::Alias(const shared_ptr<class Target>& t)
{
	targets_[fullyQualifiedName()] = t;
}


static ValuePtr AddRegeneration(EvalContext& stack, TypeContext& ctx,
                                const Arguments& regenArgs,
                                const vector<string>& inputFiles,
                                const vector<string>& outputFiles)
{
	shared_ptr<Value> Nothing;
	const SourceRange& Nowhere = SourceRange::None();

	const FileType& inputFileType = ctx.inputFileType();
	const Type& inputType = ctx.listOf(inputFileType, Nowhere);
	const FileType& outputType = ctx.outputFileType();
	const Type& buildType = ctx.functionType(inputType, outputType);

	//
	// First, construct the rule that regenerates output:file[out]
	// given input:list[file[in]].
	//
	ValueMap ruleArgs;
	ruleArgs["description"] = stack.String("Regenerating ${output}");

	SharedPtrVec<Parameter> params;
	params.emplace_back(new Parameter("rootInput", inputFileType, Nothing));
	params.emplace_back(new Parameter("otherInputs", inputType, Nothing));
	params.emplace_back(
		new Parameter("output", ctx.listOf(outputType, Nowhere), Nothing));

	shared_ptr<dag::Rule> rule;
	{
		auto ruleName(stack.evaluating(Rule::RegenerationRuleName()));
		const string Command =
			"fab" + Arguments::str(regenArgs) + " ${rootInput}";

		ValuePtr r = stack.Rule(Command, ruleArgs, params, buildType);
		rule = dynamic_pointer_cast<class Rule>(r);
	}
	assert(rule);


	//
	// Now, construct the build step that drives the rule above in order
	// to actually generate the build file.
	//
	shared_ptr<dag::File> rootInput;
	SharedPtrVec<dag::File> otherInputs;
	for (const string& name : inputFiles)
	{
		shared_ptr<dag::File> file = dynamic_pointer_cast<class File>(
			stack.File(name, ValueMap(), inputFileType, Nowhere));

		assert(file);

		if (rootInput)
			otherInputs.push_back(file);
		else
			rootInput = file;
	}

	SharedPtrVec<Value> outputs;
	for (const string& output : outputFiles)
		outputs.push_back(stack.File(output, ValueMap(), outputType));

	ValueMap args;
	args["rootInput"] = rootInput;
	args["otherInputs"].reset(List::of(otherInputs, Nowhere, ctx));

	args["output"].reset(List::of(outputs, Nowhere, ctx));

	ConstPtrMap<Type> paramTypes;
	for (auto& p : params)
		paramTypes[p->name()] = &p->type();

	return stack.Build(rule, args, paramTypes, Nowhere);
}
