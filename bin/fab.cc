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

#include "CLIArguments.h"

#include <fabrique/builtins.hh>

#include <fabrique/ast/ASTDump.hh>
#include <fabrique/ast/EvalContext.hh>

#include <fabrique/backend/Backend.hh>

#include <fabrique/dag/DAG.hh>

#include <fabrique/parsing/Parser.hh>

#include <fabrique/platform/files.hh>

#include <fabrique/plugin/Loader.hh>

#include "Support/Bytestream.h"
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


int main(int argc, char *argv[]) {
	//
	// Parse command-line arguments.
	//
	CLIArguments args = CLIArguments::Parse(argc, argv);
	if (not args or args.help)
	{
		CLIArguments::PrintUsage(cerr);
		return (args ? 0 : 1);
	}

	//
	// Set up debug streams.
	//
	Bytestream::SetDebugPattern(args.debugPattern);
	Bytestream::SetDebugStream(Bytestream::Stdout());

	Bytestream& argDebug = Bytestream::Debug("cli.args");
	args.Print(argDebug);
	argDebug << Bytestream::Reset << "\n";

	//
	// Locate input file, source root and build root.
	//
	const string fabfile =
		PathIsDirectory(args.input)
		? JoinPath(args.input, "fabfile")
		: args.input
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

	const string abspath = PathIsAbsolute(fabfile) ? fabfile : AbsolutePath(fabfile);
	const string srcroot = DirectoryOf(abspath);
	const string buildroot = AbsoluteDirectory(args.output, true);

	vector<string> inputFiles = { fabfile };
	vector<string> outputFiles;

	//
	// Prepare backends to receive the build graph.
	//
	UniqPtrVec<Backend> backends;
	for (const string& format : args.outputFormats)
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
		parsing::Parser parser(args.printAST, args.dumpAST);
		TypeContext types;
		ast::EvalContext ctx(types);

		//
		// Parse command-line definitions.
		//
		Bytestream &defDbg = Bytestream::Debug("cli.definitions");
		dag::ValueMap definitions;
		for (const string &d : args.definitions)
		{
			defDbg
				<< Bytestream::Action << "parsing "
				<< Bytestream::Type << "command-line definition "
				<< Bytestream::Operator << "'"
				<< Bytestream::Definition << d
				<< Bytestream::Operator << "'"
				<< "\n"
				;

			auto parseResult = parser.Parse(d);
			if (not parseResult.errors.empty())
			{
				for (auto &err : parseResult.errors)
				{
					Bytestream::Stderr() << err << "\n";
				}
				throw UserError("invalid definition: '" + d + "'");
			}

			FAB_ASSERT(parseResult.result, "!errors and !result");
			if (auto &name = parseResult.result->name())
			{
				auto value = parseResult.result->evaluate(ctx);
				definitions[name->name()] = value;
			}
			else
			{
				Bytestream::Stdout()
					<< Bytestream::Warning << "warning: "
					<< Bytestream::Action << "ignoring"
					<< Bytestream::Reset << " definition '"
					<< Bytestream::Literal << d
					<< Bytestream::Reset << "' with no name\n"
					;
			}
		}

		//
		// Parse the file, optionally pretty-printing it.
		//
		std::ifstream infile(fabfile.c_str());
		if (not infile)
		{
			throw UserError("failed to open '" + fabfile + "'");
		}

		auto parseResult = parser.ParseFile(infile, fabfile);

		if (not parseResult.errors.empty())
		{
			for (auto &err : parseResult.errors)
			{
				Bytestream::Stderr() << err << "\n";
			}
			return 1;
		}

		if (args.parseOnly)
		{
			return 0;
		}

		auto values = std::move(parseResult.result);


		//
		// Convert the AST into a build graph.
		//
		plugin::Loader pluginLoader(PluginSearchPaths(args.executable));
		dag::DAGBuilder &builder = ctx.builder();

		auto scope = ctx.EnterScope(fabfile);
		scope.DefineReserved("args", builder.Record(definitions));
		scope.DefineReserved("srcroot", builder.File(srcroot));
		scope.DefineReserved("buildroot", builder.File(buildroot));
		scope.DefineReserved("file", builtins::OpenFile(builder));
		scope.DefineReserved("import", builtins::Import(parser, srcroot, ctx));

		SharedPtrVec<dag::Value> dagValues;
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
		string regenerationCommand = args.executable + args.str();
		if (not outputFiles.empty())
			ctx.builder().AddRegeneration(
				regenerationCommand, inputFiles, outputFiles);

		unique_ptr<dag::DAG> dag = ctx.builder().dag(targets);
		FAB_ASSERT(dag, "null DAG");

		if (args.printDAG)
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

			if (not args.printOutput and filename != buildroot)
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
	catch (const std::exception& e)
	{
		err()
			<< Bytestream::Error << "Uncaught exception"
			<< Bytestream::Reset << ": "
			<< Bytestream::ErrorMessage << e.what()
			;
	}

	err() << Bytestream::Reset << "\n";
	return 1;
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
