/** @file driver.cc    Driver for the fabrique compiler. */
/*
 * Copyright (c) 2013-2014, 2018 Jonathan Anderson
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

#include <fabrique/ast/ASTDump.hh>
#include <fabrique/ast/EvalContext.hh>

#include <fabrique/backend/Backend.hh>

#include <fabrique/dag/DAG.hh>
#include <fabrique/dag/File.hh>
#include <fabrique/dag/Parameter.hh>

#include <fabrique/parsing/Parser.hh>

#include <fabrique/platform/files.hh>

#include <fabrique/plugin/Loader.hh>
#include <fabrique/plugin/Registry.hh>

#include "Support/Bytestream.h"
#include "Support/CLIArguments.h"
#include "Support/exceptions.h"

#include <fabrique/types/TypeContext.hh>

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>

using namespace fabrique;
using namespace fabrique::platform;
using namespace std;
using fabrique::backend::Backend;


static Bytestream& err();
static void reportError(string message, SourceRange, ErrorReport::Severity, string detail);
static UniqPtrVec<ast::Value> Parse(const string& filename, bool printAST);

static dag::ValueMap builtins(TypeContext &types, string srcroot, string buildroot);


int main(int argc, char *argv[]) {
	//
	// Parse command-line arguments.
	//
	unique_ptr<CLIArguments> args(CLIArguments::Parse(argc, argv));
	if (not args or args->help)
	{
		CLIArguments::PrintUsage(cerr);
		return (args ? 0 : 1);
	}

	//
	// Set up debug streams.
	//
	Bytestream::SetDebugPattern(args->debugPattern);
	Bytestream::SetDebugStream(Bytestream::Stdout());

	Bytestream& argDebug = Bytestream::Debug("cli.args");
	CLIArguments::Print(*args, argDebug);
	argDebug << Bytestream::Reset << "\n";

	//
	// Locate input file, source root and build root.
	//
	const string fabfile =
		PathIsDirectory(args->input)
		? JoinPath(args->input, "fabfile")
		: args->input
		;

	if (not PathIsFile(fabfile))
	{
		err()
			<< Bytestream::Error << "Error"
			<< Bytestream::Reset << ": "
			<< Bytestream::ErrorMessage << "no such file: "
			<< Bytestream::Literal << "'" << fabfile << "'"
			<< Bytestream::Reset << "\n"
			;

		return 1;
	}

	const string srcroot = DirectoryOf(AbsolutePath(fabfile));
	const string buildroot = AbsoluteDirectory(args->output, true);

	vector<string> inputFiles = { fabfile };
	vector<string> outputFiles;

	//
	// Prepare backends to receive the build graph.
	//
	UniqPtrVec<Backend> backends;
	for (const string& format : args->outputFormats)
	{
		backends.emplace_back(Backend::Create(format));

		const string filename = backends.back()->DefaultFilename();
		if (not filename.empty())
			outputFiles.push_back(filename);
	}

	//
	// Parse the file, build the DAG and pass it to the backend(s).
	// These operations can report errors with exceptions, so put them in
	// a `try` block.
	//
	try
	{
		//
		// Parse the file, optionally pretty-printing it.
		//
		UniqPtrVec<ast::Value> values = Parse(fabfile, args->printAST);
		if (args->parseOnly)
		{
			return 0;
		}


		//
		// Convert the AST into a build graph.
		//
		TypeContext types;
		plugin::Loader pluginLoader(PluginSearchPaths(args->executable));
		ast::EvalContext ctx(types, builtins(types, srcroot, buildroot));

		SharedPtrVec<dag::Value> dagValues;
		vector<string> targets;
		for (const auto& v : values)
		{
			dagValues.emplace_back(v->evaluate(ctx));
			if (auto &name = v->name())
			{
				targets.push_back(name->name());
			}
		}

		// Add regeneration (if Fabrique files change):
		if (not outputFiles.empty())
			ctx.builder().AddRegeneration(
				*args, inputFiles, outputFiles);

		unique_ptr<dag::DAG> dag = ctx.builder().dag(targets);
		FAB_ASSERT(dag, "null DAG");

		if (args->printDAG)
		{
			dag->PrettyPrint(Bytestream::Stdout());
		}


		//
		// Finally, feed the build graph into the backend(s).
		//
		for (UniqPtr<Backend>& backend : backends)
		{
			std::ofstream outfile;
			unique_ptr<Bytestream> outfileStream;

			const string filename =
				JoinPath(buildroot, backend->DefaultFilename());

			if (not args->printOutput and filename != buildroot)
			{
				outfile.open(filename.c_str());
				outfileStream.reset(Bytestream::Plain(outfile));
			}

			Bytestream& out = outfileStream
				? *outfileStream
				: Bytestream::Stdout();

			backend->Process(*dag, out, reportError);

			outfile.flush();
			outfile.close();
		}

		return 0;
	}
	catch (const UserError& e)
	{
		err()
			<< Bytestream::Error << "Error"
			<< Bytestream::Reset << ": " << e
			;
	}
	catch (const OSError& e)
	{
		err()
			<< Bytestream::Error << e.message()
			<< Bytestream::Reset << ": "
			<< Bytestream::ErrorMessage << e.description()
			;
	}
	catch (const SourceCodeException& e)
	{
		err() << e;
	}
#ifdef NDEBUG
	// In debug mode, let uncaught exceptions propagate to ease debugging.
	catch (const std::exception& e)
	{
		err()
			<< Bytestream::Error << "Uncaught exception"
			<< Bytestream::Reset << ": "
			<< Bytestream::ErrorMessage << e.what()
			;
	}
#endif

	err() << Bytestream::Reset << "\n";
	return 1;
}


UniqPtrVec<ast::Value> Parse(const string& filename, bool printAST)
{
	// Open and parse the top-level build description.
	std::ifstream infile(filename.c_str());
	assert(infile);

	const string absolute =
		PathIsAbsolute(filename) ? filename : AbsolutePath(filename);

	parsing::Parser parser;
	auto result = parser.ParseFile(infile, filename);
	FAB_ASSERT(result.result.empty() xor result.errors.empty(),
	           "cannot have parsing results and parsing errors");

	for (auto &err : result.errors)
	{
		Bytestream::Stderr() << err << "\n";
	}

	if (not result.errors.empty())
	{
		throw UserError("failed to parse '" + filename + "'");
	}

	auto values = std::move(result.result);
	if (printAST)
	{
		Bytestream::Stdout()
			<< Bytestream::Comment
			<< "#\n"
			<< "# AST pretty-printed from '" << filename << "'\n"
			<< "#\n"
			<< Bytestream::Reset
			;

		for (auto& val : values)
			Bytestream::Stdout() << *val << "\n";
	}

	return values;
}


static dag::ValueMap builtins(TypeContext &types, string srcroot, string buildroot)
{
	dag::ValueMap builtins;
	builtins.emplace("srcroot", dag::File::Create(srcroot, types.fileType()));
	builtins.emplace("buildroot", dag::File::Create(buildroot, types.fileType()));

	dag::Function::Evaluator openFile =
		[](dag::ValueMap arguments, dag::DAGBuilder &b, SourceRange src)
	{
		auto filename = arguments["name"];
		SemaCheck(filename, src, "missing filename");

		// TODO: look up subdirectory in the call context?
		string subdir;
		if (auto s = arguments[ast::builtins::Subdirectory])
		{
			subdir = s->str();
		}

		return b.File(subdir, filename->str(), arguments, src);
	};

	SharedPtrVec<dag::Parameter> params;
	params.emplace_back(new dag::Parameter("name", types.stringType()));

	builtins.emplace("file",
		dag::Function::Create(openFile, types.fileType(), params,
		                      SourceRange::None(), true));

	return builtins;
}


static Bytestream& err()
{
	static Bytestream& err = Bytestream::Stderr();
	return err;
}


static void reportError(string message, SourceRange src, ErrorReport::Severity severity,
                        string detail)
{
	err() << ErrorReport(message, src, severity, detail) << Bytestream::Reset << "\n";
}
