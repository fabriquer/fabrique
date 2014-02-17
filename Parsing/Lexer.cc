/** @file Lexer.h    Definition of @ref Lexer. */
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

#include "Parsing/Lexer.h"
#include "Parsing/fab.yacc.h"
#include "Support/Bytestream.h"
#include "Support/ErrorReport.h"

#include <cassert>
#include <list>
#include <map>

using namespace fabrique;

extern int yylineno;
extern int yycolumn;


static SourceRange range(const char *text, size_t len, SourceLocation begin)
{
	size_t line = begin.line;
	size_t column = begin.column;

	for (size_t i = 0; i < len; i++)
	{
		if (text[i] == '\n')
		{
			line++;
			column = 1;
		}
		else
			column++;
	}

	SourceLocation end(begin.filename, line, column);

	return SourceRange(begin, end);
}


const ErrorReport& Lexer::Err(const char *message)
{
	errs.push_back(
		std::unique_ptr<ErrorReport>(
			ErrorReport::Create(message, CurrentTokenRange())));

	return *errs.back();
}

Token Lexer::NextToken() const
{
	return Token(yytext, yyleng, CurrentTokenRange());
}

SourceRange Lexer::CurrentTokenRange() const
{
	SourceLocation begin(inputFilename, yylineno, yycolumn);
	return range(yytext, yyleng, begin);
}


void Lexer::SetComment(YYSTYPE *yyunion, bool includesNewline)
{
	std::string s = currentToken.str();
	SourceRange src = currentToken.source();

	if (includesNewline)
	{
		s = s.substr(0, s.length() - 1);

		src.begin.line--;
		src = range(s.data(), s.length(), src.begin);
	}

	yyunion->token = new Token(s, src);
	Token &t = *yyunion->token;

	Bytestream::Debug("lex.comment")
		<< Bytestream::Action << "lexed "
		<< Bytestream::Type << "comment"
		<< Bytestream::Operator << ": '"
		<< Bytestream::Comment << t.str()
		<< Bytestream::Operator << "' @ " << t.source()
		<< Bytestream::Reset << "\n"
		;
}


void Lexer::SetToken(YYSTYPE *yyunion)
{
	Bytestream::Debug("lex.token")
		<< Bytestream::Action << "lexed "
		<< Bytestream::Type << "token"
		<< Bytestream::Operator << ": " << currentToken
		<< Bytestream::Reset << "\n"
		;

	yyunion->token = new Token(currentToken);
}


void Lexer::BeginString()
{
	stringStart = currentToken.source().begin;
	assert(buf.empty());
}


void Lexer::AppendChar(char c)
{
	buf.push_back(c);
}


void Lexer::EndString(YYSTYPE* yyunion)
{
	SourceRange src = range(buf.data(), buf.size(), stringStart);

	std::string s(buf.data(), 0, buf.size());
	buf.clear();
	yyunion->token = new Token(s, src);
}


int yyFlexLexer::yylex() { assert(false && "unreachable"); return 0; }
int yyFlexLexer::yywrap() { return 1; }
