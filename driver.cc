/** @file driver.cc    Driver for the fabrique compiler. */
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

#include "AST/ASTDump.h"

#include "Parsing/Lexer.h"
#include "Parsing/Parser.h"
#include "Parsing/fab.yacc.h"

#include "Support/Arguments.h"
#include "Support/Bytestream.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>

using namespace fabrique;
using std::auto_ptr;


auto_ptr<Lexer> lex;
Bytestream& err = Bytestream::ANSI(std::cerr);

int yyparse(ast::Parser*);

/*
 * I'd like to get rid of the global yyerror() and yylex() functions
 * (since they assume that there is only one lexer),
 * but this is a limitation of byacc.
 */
void yyerror(const char *str)
{
	assert(lex.get() != NULL);
	err << lex->Err(str);
}

int yylex(void *yylval)
{
	assert(lex.get() != NULL);
	return lex->yylex((YYSTYPE*) yylval);
}

int main(int argc, char *argv[]) {
	//
	// Parse command-line arguments.
	//
	auto_ptr<Arguments> args(Arguments::Parse(argc, argv));
	if (!args.get() or args->help)
	{
		Arguments::PrintUsage(std::cerr);
		return (args.get() ? 0 : 1);
	}


	//
	// Set up input and output files.
	//
	std::ifstream infile(args->input.c_str());

	std::ofstream outfile;
	if (args->outputIsFile)
		outfile.open(args->output.c_str());

	Bytestream& out = args->outputIsFile
		? Bytestream::File(outfile)
		: Bytestream::ANSI(std::cout);

	Bytestream& err = Bytestream::ANSI(std::cerr);

	lex.reset(new Lexer(args->input));
	lex->switch_streams(&infile, &out.raw());


	//
	// Parse the Fabrique input.
	//
	auto_ptr<ast::Parser> parser(new ast::Parser(*lex));
	int result = yyparse(parser.get());

	for (auto *error : parser->errors())
		err << *error << "\n";

	if (result != 0)
	{
		err
			<< "Fabrique:"
			<< Bytestream::Error << " failed to parse "
			<< Bytestream::Filename << args->input
			<< Bytestream::Reset
			<< "\n"
			;

		return 1;
	}

	const ast::Scope& root = parser->getRoot();

	if (args->prettyPrint)
	{
		out
			<< Bytestream::Comment
			<< "#\n"
			<< "# AST pretty-printed from '" << args->input << "'\n"
			<< "#\n"
			<< Bytestream::Reset
			<< root
			;
	}


	//
	// What should we do with it now?
	//
	auto_ptr<ast::Visitor> v;
	if (args->format == "null")
		;

	else if (args->format == "dump")
		v.reset(ast::ASTDump::Create(out));

	else
	{
		std::cerr
			<< "unknown format '" << args->format << "'"
			<< std::endl
			;
		return 1;
	}

	if (v.get() != NULL)
		root.Accept(*v);

	return 0;
}
