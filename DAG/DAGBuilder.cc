/** @file DAG/DAGBuilder.cc    Definition of @ref fabrique::dag::DAGBuilder. */
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
#include "DAG/DAGBuilder.h"
#include "DAG/File.h"
#include "DAG/Function.h"
#include "DAG/List.h"
#include "DAG/Parameter.h"
#include "DAG/Primitive.h"
#include "DAG/Rule.h"

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
#include <set>

using namespace fabrique;
using namespace fabrique::dag;

using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::string;
using std::vector;

using FileVec = SharedPtrVec<class File>;

namespace {

class ImmutableDAG : public DAG
{
public:
	ImmutableDAG(string buildroot, string srcroot,
	             const SharedPtrVec<File>& files,
	             const SharedPtrVec<Build>& builds,
	             const SharedPtrMap<Rule>& rules,
	             const SharedPtrMap<Value>& variables,
	             const SharedPtrMap<Value>& targets,
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
	const SharedPtrMap<Value>& targets() const override
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
	const SharedPtrMap<Value> targets_;
	const vector<BuildTarget> topLevelTargets_;
};

}


DAGBuilder::DAGBuilder(Context& ctx)
	: ctx_(ctx)
{
}


DAGBuilder::Context::~Context()
{
}


void DAGBuilder::Define(string name, ValuePtr v)
{
	if (v->type().hasFiles())
		targets_.emplace(name, v);
	else
		variables_.emplace(name, v);
}


UniqPtr<DAG> DAGBuilder::dag(vector<string> topLevelTargets) const
{
	//
	// If we create files in output directories, we should also generate
	// rules to make those directories.
	//
	// Many tools (e.g., compilers) can create the output directories themselves,
	// but sometimes the build tool itself wants to know where the directories
	// come from (e.g., when a build depends on generated include directories).
	//
	std::map<string,shared_ptr<class File>> directories;
	SharedPtrVec<class Build> builds = builds_;
	SharedPtrMap<class Rule> rules = rules_;
	shared_ptr<class Rule> mkdir = MakeDirectory();

	for (auto& file : files_)
	{
		assert(file);

		if (not file->generated())
			continue;

		const string dirname = file->directory();
		if (dirname.empty())
			continue;

		shared_ptr<class File>& dir = directories[dirname];
		if (not dir)
		{
			directories[dirname].reset(
				File::Create(dirname, ValueMap(), ctx_.types().fileType(),
				             SourceRange::None(), true));

			ValueMap buildArgs;
			buildArgs["directory"] = dir;
			builds.emplace_back(
				Build::Create(mkdir, buildArgs, SourceRange::None()));
		}
	}

	//
	// Ensure all files are unique.
	//
	if (not directories.empty())
		rules["mkdir"] = mkdir;

	SharedPtrVec<class File> files = files_;
	for (auto d : directories)
		files.push_back(d.second);

	std::sort(files.begin(), files.end(), File::LessThan);
	files.erase(std::unique(files.begin(), files.end(), File::Equals), files.end());

	//
	// Check for target/filename conflicts.
	//
	for (auto& file : files)
	{
		const string& filename = file->filename();

		const auto& targets = topLevelTargets;
		auto i = std::find(targets.begin(), targets.end(), filename);

		if (i == targets.end())
			continue;

		// It's ok to have a target called 'foo' that generates
		// a file called 'foo'. It's only the ambiguous cases
		// (e.g., file 'foo' and target 'foo' are unrelated)
		// that cause problems.
		auto j = targets_.find(filename);
		if (j != targets_.end())
		{
			if (auto out = dynamic_pointer_cast<class File>(j->second))
			{
				if (out->filename() == filename)
					continue;
			}
		}

		throw SemanticException("target '" + *i + "' conflicts with file",
		                        file->source());
	}


	//
	// Find top-level targets.
	//
	vector<DAG::BuildTarget> top;
	for (const string& name : topLevelTargets)
	{
		auto t = targets_.find(name);
		if (t != targets_.end())
			top.emplace_back(name, t->second);
	}


	return UniqPtr<DAG>(new ImmutableDAG(ctx_.buildroot(), ctx_.srcroot(),
		files, builds, rules, variables_, targets_, top));
}


ValuePtr DAGBuilder::AddRegeneration(const Arguments& commandLineArgs,
                                     const vector<string>& inputFiles,
                                     const vector<string>& outputFiles)
{
	shared_ptr<Value> Nothing;
	const SourceRange& Nowhere = SourceRange::None();

	TypeContext& t = ctx_.types();
	const FileType& inputFileType = t.inputFileType();
	const Type& inputType = t.listOf(inputFileType, Nowhere);
	const FileType& outputType = t.outputFileType();
	const Type& buildType = t.functionType(inputType, outputType);

	//
	// First, construct the rule that regenerates output:file[out]
	// given input:list[file[in]].
	//
	ValueMap ruleArgs;
	ruleArgs["description"] = String("Regenerating ${output}");

	// For backends that support it (Ninja), put regeneration into the
	// 'console' pool (this gives Fabrique direct console access, allowing
	// pretty-printing, etc.).
	ruleArgs["pool"] = String("console");

	SharedPtrVec<Parameter> params;
	params.emplace_back(new Parameter("rootInput", inputFileType, Nothing));
	params.emplace_back(new Parameter("otherInputs", inputType, Nothing));
	params.emplace_back(
		new Parameter("output", t.listOf(outputType, Nowhere), Nothing));

	shared_ptr<dag::Rule> rule;
	{
		const string Name = Rule::RegenerationRuleName();
		const string Command =
			commandLineArgs.executable
			+ Arguments::str(commandLineArgs)
			+ " ${rootInput}"
			;

		ValuePtr r = Rule(Name, Command, ruleArgs, params, buildType);
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
			File(name, ValueMap(), inputFileType, Nowhere));

		assert(file);

		if (rootInput)
			otherInputs.push_back(file);
		else
			rootInput = file;
	}

	SharedPtrVec<Value> outputs;
	for (const string& output : outputFiles)
		outputs.push_back(File(output, ValueMap(), outputType));

	ValueMap args;
	args["rootInput"] = rootInput;
	args["otherInputs"].reset(List::of(otherInputs, Nowhere, t));

	args["output"].reset(List::of(outputs, Nowhere, t));

	return Build(rule, args, Nowhere);
}

ValuePtr DAGBuilder::Bool(bool b, SourceRange src)
{
	return ValuePtr(new class Boolean(b, ctx_.types().booleanType(), src));
}


shared_ptr<class Build>
DAGBuilder::Build(shared_ptr<class Rule> rule, ValueMap arguments,
                  SourceRange src)
{
	shared_ptr<class Build> b(Build::Create(rule, arguments, src));

	builds_.push_back(b);

	for (const shared_ptr<class File>& f : b->inputs())
		files_.push_back(f);

	for (const shared_ptr<class File>& f : b->outputs())
		files_.push_back(f);

	return b;
}


ValuePtr DAGBuilder::File(string fullPath, const ValueMap& attributes,
                         const FileType& t, const SourceRange& src, bool generated)
{
	files_.emplace_back(File::Create(fullPath, attributes, t, src, generated));
	return files_.back();
}

ValuePtr DAGBuilder::File(string subdir, string name, const ValueMap& attributes,
                          const FileType& t, const SourceRange& src, bool generated)
{
	files_.emplace_back(File::Create(subdir, name, attributes, t, src, generated));
	return files_.back();
}


ValuePtr DAGBuilder::Function(const Function::Evaluator fn, ValueMap scope,
                              const SharedPtrVec<Parameter>& params,
                              const FunctionType& type, SourceRange source)
{
	return ValuePtr(
		Function::Create(fn, std::move(scope), params, type, source));
}


ValuePtr DAGBuilder::Integer(int i, SourceRange src)
{
	return ValuePtr(new class Integer(i, ctx_.types().integerType(), src));
}



ValuePtr DAGBuilder::Rule(string name, string command, const ValueMap& arguments,
                         const SharedPtrVec<Parameter>& parameters,
                         const Type& type, const SourceRange& source)
{
	shared_ptr<class Rule> r(
		Rule::Create(name, command, arguments, parameters,
		             type, source)
	);
	r->setSelf(r);

	rules_[name] = r;

	return r;
}



ValuePtr DAGBuilder::Rule(string command, const ValueMap& arguments,
                         const SharedPtrVec<Parameter>& parameters,
                         const Type& type, const SourceRange& source)
{
	return Rule(ctx_.currentValueName(), command, arguments, parameters,
	            type, source);
}


ValuePtr DAGBuilder::String(const string& s, SourceRange src)
{
	return ValuePtr(new class String(s, ctx_.types().stringType(), src));
}


ValuePtr DAGBuilder::Record(const vector<Record::Field>& fields,
                            const Type& t, SourceRange source)
{
	return ValuePtr(Record::Create(fields, t, source));
}


shared_ptr<class Rule> DAGBuilder::MakeDirectory() const
{
	TypeContext& ctx = ctx_.types();
	const Type& str = ctx.stringType();
	const Type& file = ctx.outputFileType();
	const FunctionType& type = ctx.functionType(str, file);

	ValueMap arguments;
	arguments["description"].reset(new class String("Creating ${directory}", str));

	SharedPtrVec<class Parameter> parameters;
	parameters.emplace_back(new class Parameter("directory", file, ValuePtr()));

	return shared_ptr<class Rule>(
		Rule::Create("mkdir", CreateDirCommand("${directory}"),
			arguments, parameters, type));
}
