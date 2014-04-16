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

#include "FabContext.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>

using namespace fabrique;
using std::string;
using std::unique_ptr;


static Bytestream& err();
static unique_ptr<ast::Scope> Parse(const string& filename, FabContext&,
                                    bool printAST);

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

	string srcroot = DirectoryOf(args->input, true);
	string buildroot = AbsoluteDirectory(args->output);
	FabContext ctx(srcroot, buildroot);


	//
	// Set up debug streams.
	//
	Bytestream::SetDebugPattern(args->debugPattern);
	Bytestream::SetDebugStream(err());


	//
	// Parse the file, optionally pretty-printing it.
	//
	unique_ptr<ast::Scope> ast(Parse(args->input, ctx, args->printAST));

	if (not ast)
		return -1;

	if (args->parseOnly)
		return 0;


	//
	// Convert the parsed AST into a DAG.
	//
	unique_ptr<dag::DAG> dag;
	try { dag = dag::DAG::Flatten(*ast, ctx); }
	catch (SemanticException& e)
	{
		err()
			<< Bytestream::Error
			<< "Semantic error: "
			<< Bytestream::ErrorMessage
			<< e
			<< Bytestream::Reset
			<< "\n";
		return 1;
	}
	catch (std::exception& e)
	{
		err()
			<< Bytestream::Error
			<< "Unknown error: "
			<< Bytestream::ErrorMessage
			<< e.what()
			<< Bytestream::Reset
			<< "\n";
		return 1;
	}

	assert(dag);

	if (args->printDAG)
	{
		Bytestream::Stdout()
			<< Bytestream::Comment
			<< "#\n"
			<< "# DAG pretty-printed from '" << args->input << "'\n"
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


int yyparse(ast::Parser*);

/**
 * This is a bit ugly, but yyerror() and yylex() need a static pointer to
 * the Lexer and we can't do that without a global constructor and exit-time
 * destructor. Manually disable the warnings for this case, and let their
 * ugliness stand as a warning to future generations of programmers who
 * try to use lex.
 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
static UniqPtr<Lexer> lex;
#pragma clang diagnostic pop

/*
 * I'd like to get rid of the global yyerror() and yylex() functions
 * (since they assume that there is only one lexer),
 * but this is a limitation of byacc.
 */
void yyerror(const char *str)
{
	assert(lex);
	err() << lex->Err(str);
}

int yylex(void *yaccUnion)
{
	assert(lex);
	return lex->yylex((YYSTYPE*) yaccUnion);
}


unique_ptr<ast::Scope> Parse(const string& filename, FabContext& ctx,
                             bool printAST)
{
	unique_ptr<ast::Scope> ast;

	try
	{
		std::ifstream infile(filename.c_str());
		if (!infile)
			throw UserError("no such file: '" + filename + "'");

		// Set up lex.
		lex.reset(new Lexer(filename));
		lex->switch_streams(&infile, &err().raw());


		// Set up the parser.
		unique_ptr<ast::Parser> parser(new ast::Parser(ctx, *lex));

		//
		// Define some magic builtins:
		//
		parser->Builtin("srcroot", ctx.srcroot());
		parser->Builtin("buildroot", ctx.buildroot());

		int result = yyparse(parser.get());
		lex.reset();

		if (result == 0)
		{
			ast = parser->ExitScope();
			assert(parser->errors().empty());
		}
		else
		{
			for (auto& error : parser->errors())
				err() << *error << "\n";
		}
	}
	catch (OSError& e)
	{
		err()
			<< Bytestream::Error << e.message()
			<< Bytestream::Operator << ": "
			<< Bytestream::ErrorMessage << e.description()
			<< Bytestream::Reset
			<< "\n"
			;

		ast.reset();
	}
	catch (SourceCodeException& e)
	{
		err()
			<< Bytestream::Error << "Parse error: "
			<< Bytestream::ErrorMessage << e
			<< Bytestream::Reset
			<< "\n";

		ast.reset();
	}

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
