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
#include "Support/ostream.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>

using std::auto_ptr;


auto_ptr<Lexer> lex;

int yyparse(Parser*);

void yyerror(const char *str)
{
	assert(lex.get() != NULL);
	std::cerr << lex->Err(str);
}

int yylex(void *yylval)
{
	assert(lex.get() != NULL);
	return lex->yylex((YYSTYPE*) yylval);
}

int main(int argc, char *argv[]) {
	auto_ptr<Arguments> args(Arguments::Parse(argc, argv));
	if (!args.get())
	{
		Arguments::Usage(std::cerr, argv[0]);
		return 1;
	}

	std::ifstream infile(args->input.c_str());

	bool outputIsFile = (args->output.length() > 0);
	std::ofstream outfile;
	if (outputIsFile)
		outfile.open(args->output.c_str());

	std::ostream& out(outputIsFile ? outfile : std::cout);

	lex.reset(new Lexer(args->input));
	lex->switch_streams(&infile, &out);

	auto_ptr<Parser> parser(new Parser(*lex));
	int err = yyparse(parser.get());

	for (auto *err : parser->errors())
		std::cerr << *err << std::endl;

	if (err != 0)
	{
		std::cerr
			<< Bold << "Fabrique:"
			<< Red << " failed to parse "
			<< Magenta << args->input
			<< ResetAll
			<< std::endl
			;

		return 1;
	}

	auto& root = parser->getRoot();

	if (args->prettyPrint)
		std::cout << root;

	auto_ptr<Visitor> v;
	if (args->format == "dump")
		v.reset(ASTDump::Create(out));

	else
	{
		std::cerr
			<< "unknown format '" << args->format << "'"
			<< std::endl
			;
		return 1;
	}

	root.Accept(*v);

	return 0;
}
