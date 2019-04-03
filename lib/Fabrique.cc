//! @file Fabrique.cc    Definition of the top-level Fabrique type
/*
 * Copyright (c) 2018 Jonathan Anderson
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

#include <fabrique/Bytestream.hh>
#include <fabrique/Fabrique.hh>
#include <fabrique/ast/EvalContext.hh>
#include <fabrique/dag/DAGBuilder.hh>
#include <fabrique/parsing/Parser.hh>
#include <fabrique/platform/files.hh>
#include <fabrique/plugin/Loader.hh>
#include "Support/exceptions.h"
#include <fabrique/types/TypeContext.hh>

#include <fstream>

using namespace fabrique;
using namespace fabrique::platform;
using namespace std;
using namespace std::placeholders;


Fabrique::Fabrique(bool parseOnly, bool printASTs, bool dumpASTs, bool printDAG,
                   bool printToStdout, UniqPtrVec<backend::Backend> backends,
                   string outputDir, vector<string> pluginPaths, string regenCommand,
                   ErrorReporter err)
	: parseOnly_(parseOnly), printDAG_(printDAG), printToStdout_(printToStdout),
	  backends_(std::move(backends)), err_(err), parser_(printASTs, dumpASTs),
	  outputDirectory_(outputDir), pluginPaths_(pluginPaths),
	  regenerationCommand_(regenCommand)
{
	for (auto &b : backends_)
	{
		const string filename = b->DefaultFilename();
		if (not filename.empty())
		{
			outputFiles_.push_back(filename);
		}
	}
}

void Fabrique::AddArgument(const string &s)
{
	auto parseResult = parser_.Parse(s);
	if (not parseResult)
	{
		for (auto &err : parseResult.errors())
		{
			err_(err);
		}
		throw UserError("invalid definition: '" + s + "'");
	}

	FAB_ASSERT(parseResult, "!errors and !result");

	if (auto &name = parseResult.ok().name())
	{
		ast::EvalContext ctx(types_);
		auto value = parseResult.ok().evaluate(ctx);
		arguments_.emplace(name->name(), value);
	}
	else
	{
		err_(ErrorReport("ignoring definition '" + s + "' with no name",
		                 SourceRange::None(), ErrorReport::Severity::Warning));
	}
}

void Fabrique::AddArguments(const vector<string> &args)
{
	for (const string &a : args)
	{
		AddArgument(a);
	}
}


const UniqPtrVec<ast::Value>& Fabrique::Parse(std::istream &f, const std::string &name)
{
	FAB_ASSERT(f.good(), "invalid input stream");

	auto parseResult = parser_.ParseFile(f, name);

	if (not parseResult)
	{
		for (auto &err : parseResult.errors())
		{
			err_(err);
		}
		throw UserError("failed to parse " + name);
	}

	return parseResult.ok();
}


void Fabrique::Process(const std::string &filename)
{
	//
	// Locate input file, source root and build root.
	//
	const string fabfile =
		PathIsDirectory(filename)
		? JoinPath(filename, "fabfile")
		: filename
		;

	const string abspath = PathIsAbsolute(fabfile) ? fabfile : AbsolutePath(fabfile);
	const string srcroot = DirectoryOf(abspath);

	if (not PathIsFile(abspath))
	{
		throw UserError("no such file: '" + fabfile + "'");
	}

	//
	// Open and parse the file.
	//
	std::ifstream infile(abspath);
	if (not infile.good())
	{
		throw UserError("failed to open '" + filename + "'");
	}

	auto &values = Parse(infile, abspath);

	if (parseOnly_)
	{
		return;
	}


	//
	// Convert the AST into a build graph.
	//
	plugin::Loader pluginLoader(pluginPaths_);
	ast::EvalContext ctx(types_);
	dag::DAGBuilder &builder = ctx.builder();

	auto scope = ctx.EnterScope(fabfile);
	scope.DefineReserved("args", builder.Record(arguments_));
	scope.DefineReserved("srcroot", builder.File(srcroot));
	scope.DefineReserved("buildroot", builder.File(outputDirectory_));
	scope.DefineReserved("fields", builtins::Fields(builder));
	scope.DefineReserved("file", builtins::OpenFile(builder));
	scope.DefineReserved("import",
		builtins::Import(parser_, pluginLoader, srcroot, ctx));
	scope.DefineReserved("print", builtins::Print(builder));

	// Also define srcroot as an explicit variable in the DAG:
	builder.Define("srcroot", builder.String(srcroot));

	vector<string> targets;
	for (const auto& v : values)
	{
		ctx.Define(*v);
		if (auto &name = v->name())
		{
			targets.push_back(name->name());
		}
	}

	// Add regeneration (if Fabrique files change):
	if (not regenerationCommand_.empty() and not outputFiles_.empty())
	{
		builder.AddRegeneration(
			regenerationCommand_, parser_.inputs(), outputFiles_);
	}

	unique_ptr<dag::DAG> dag = builder.dag(targets);
	FAB_ASSERT(dag, "null DAG");

	if (printDAG_)
	{
		dag->PrettyPrint(Bytestream::Stdout());
	}


	//
	// Finally, feed the build graph into the backend(s).
	//
	auto err = std::bind(&Fabrique::ReportError, this, _1, _2, _3, _4);

	for (const auto &b : backends_)
	{
		std::ofstream outfile;
		unique_ptr<Bytestream> outfileStream;
		Bytestream *out;


		if (printToStdout_)
		{
			out = &Bytestream::Stdout();
		}
		else
		{
			const string outputFilename =
				JoinPath(outputDirectory_, b->DefaultFilename());

			outfile.open(outputFilename);
			outputFiles_.push_back(outputFilename);

			outfileStream.reset(Bytestream::Plain(outfile));
			out = outfileStream.get();
		}

		b->Process(*dag, *out, err);

		outfile.flush();
		outfile.close();
	}
}


void Fabrique::ReportError(string message, SourceRange src, ErrorReport::Severity severity,
                           string detail)
{
	err_(ErrorReport(message, src, severity, detail));
}
