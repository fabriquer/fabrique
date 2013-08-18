/** @file driver.h    Driver for the fabrique compiler. */
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

#include "Arguments.h"
#include "Lexer.h"
#include "Parser.h"

#include "fab.yacc.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>

using std::auto_ptr;


auto_ptr<Lexer> lex;

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

void yyparse(Parser*);

int main(int argc, char *argv[]) {
	auto_ptr<Arguments> Args(Arguments::Parse(argc, argv));
	if (!Args.get())
	{
		Arguments::Usage(std::cerr, argv[0]);
		return 1;
	}

	std::ifstream infile(Args->input.c_str());

	bool outputIsFile = (Args->output.length() > 0);
	std::ofstream outfile;
	if (outputIsFile)
		outfile.open(Args->output.c_str());

	lex.reset(new Lexer(Args->input));
	lex->switch_streams(&infile, &(outputIsFile ? outfile : std::cout));

	auto_ptr<Parser> parser(new Parser(*lex));
	yyparse(parser.get());

	return 0;
}
