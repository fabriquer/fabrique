/** @file driver.cc    Driver for the fabrique compiler. */
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

#include "AST/ASTDump.h"

#include "Backend/Backend.h"
#include "Backend/Dot.h"
#include "Backend/Make.h"
#include "Backend/Ninja.h"
#include "Backend/Null.h"

#include "DAG/DAG.h"

#include "Parsing/Lexer.h"
#include "Parsing/Parser.h"

#include "Support/Arguments.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Support/os.h"

#include "Types/TypeContext.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>

using namespace fabrique;
using std::map;
using std::string;
using std::unique_ptr;
using std::vector;


static Bytestream& err();
static unique_ptr<ast::Scope> Parse(const string& filename, TypeContext&,
                                    const vector<string>& definitions,
                                    string srcroot, string buildroot, bool printAST);

int main(int argc, char *argv[]) {
	//
	// Parse command-line arguments.
	//
	unique_ptr<Arguments> args(Arguments::Parse(argc, argv));
	if (not args or args->help)
	{
		Arguments::PrintUsage(std::cerr);
		return (args ? 0 : 1);
	}

	//
	// Set up debug streams.
	//
	Bytestream::SetDebugPattern(args->debugPattern);
	Bytestream::SetDebugStream(err());

	Bytestream& argDebug = Bytestream::Debug("cli.args");
	Arguments::Print(*args, argDebug);
	argDebug << Bytestream::Reset << "\n";

	//
	// Parse the file, build the DAG and pass it to the backed.
	// These operations can report errors with exceptions, so put them in
	// a `try` block.
	//
	try
	{
		TypeContext ctx;

		const string srcroot = DirectoryOf(args->input, true);
		const string buildroot = AbsoluteDirectory(args->output);

		//
		// Parse the file, optionally pretty-printing it.
		//
		unique_ptr<ast::Scope> ast(
			Parse(args->input, ctx, args->definitions,
			      srcroot, buildroot, args->printAST));

		if (not ast)
			return -1;

		if (args->parseOnly)
			return 0;


		//
		// Convert the parsed AST into a DAG.
		//
		unique_ptr<dag::DAG> dag;
		dag = dag::DAG::Flatten(*ast, ctx, srcroot, buildroot);

		// DAG errors should be reported as exceptions.
		assert(dag);

		if (args->printDAG)
		{
			Bytestream::Stdout()
				<< Bytestream::Comment
				<< "#\n"
				<< "# DAG pretty-printed from '"
				<< args->input << "'\n"
				<< "#\n"
				<< Bytestream::Reset
				<< *dag
				;
		}


		//
		// What should we do with it now?
		//
		unique_ptr<backend::Backend> backend;

		if (args->format == "null")
			backend.reset(new backend::NullBackend());

		else if (args->format == "make")
			backend.reset(backend::MakeBackend::Create());

		else if (args->format == "ninja")
			backend.reset(backend::NinjaBackend::Create());

		else if (args->format == "dot")
			backend.reset(backend::DotBackend::Create());

		else
		{
			err() << "unknown format '" << args->format << "'\n";
			return 1;
		}

		assert(backend);


		//
		// Where should output go?
		//
		std::ofstream outfile;
		unique_ptr<Bytestream> outfileStream;

		const string filename =
			JoinPath(buildroot, backend->DefaultFilename());

		if (not args->printOutput and args->format != "null")
		{
			outfile.open(filename.c_str());
			outfileStream.reset(Bytestream::Plain(outfile));
		}

		Bytestream& out = outfileStream
			? *outfileStream
			: Bytestream::Stdout();

		backend->Process(*dag, out);

		return 0;
	}
	catch (const UserError& e)
	{
		err()
			<< Bytestream::Error << "Error: "
			<< Bytestream::Reset << e
			;
	}
	catch (const OSError& e)
	{
		err()
			<< Bytestream::Error << e.message()
			<< Bytestream::Operator << ": "
			<< Bytestream::ErrorMessage << e.description()
			;
	}
	catch (const SemanticException& e)
	{
		err() << e;
	}
	catch (const SourceCodeException& e)
	{
		err()
			<< Bytestream::Error << "Parse error: "
			<< Bytestream::ErrorMessage << e
			;
	}
#ifdef NDEBUG
	// In debug mode, let uncaught exceptions propagate to ease debugging.
	catch (const std::exception& e)
	{
		err()
			<< Bytestream::Error << "Uncaught exception: "
			<< Bytestream::ErrorMessage << e.what()
			;
	}
#endif

	err() << Bytestream::Reset << "\n";
	return 1;
}


unique_ptr<ast::Scope> Parse(const string& filename, TypeContext& ctx,
                             const vector<string>& definitions,
                             string srcroot, string buildroot, bool printAST)
{

	// Create the parser.
	unique_ptr<ast::Parser> parser(new ast::Parser(ctx, srcroot));

	// Parse command-line arguments.
	auto args = parser->ParseDefinitions(definitions);

	// Open and parse the top-level build description.
	std::ifstream infile(filename.c_str());
	if (!infile)
		throw UserError("no such file: '" + filename + "'");

	map<string,string> builtins {
		std::make_pair("srcroot", srcroot),
		std::make_pair("buildroot", buildroot),
		std::make_pair(ast::Subdirectory, ""),
	};

	unique_ptr<ast::Scope> ast(parser->ParseFile(infile, args, filename, builtins));
	if (not ast)
	{
		for (auto& error : parser->errors())
			err() << *error << "\n";

		if (parser->errors().empty())
			err()
				<< Bytestream::Error << "unknown error"
				<< Bytestream::Reset << "\n"
				;

		return ast;
	}
	assert(parser->errors().empty());

	if (printAST)
	{
		Bytestream::Stdout()
			<< Bytestream::Comment
			<< "#\n"
			<< "# AST pretty-printed from '" << filename << "'\n"
			<< "#\n"
			<< Bytestream::Reset
			;

		for (auto& val : ast->values())
			Bytestream::Stdout() << *val << "\n";
	}

	return ast;
}


static Bytestream& err()
{
	static Bytestream& err = Bytestream::Stderr();
	return err;
}
