/** @file Parsing/Lexer.h    Declaration of @ref fabrique::Lexer. */
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

#ifndef LEXER_H
#define LEXER_H

#include "ADT/PtrVec.h"
#include "Support/ErrorReport.h"

#include "lex.h"
#include "yacc.h"

#include <list>
#include <stack>

struct yy_buffer_state;

namespace fabrique {

class Token;

/**
 * Tokenizes Fabrique source code.
 */
class Lexer : public yyFlexLexer
{
public:
	//! Access the singleton @ref Lexer instance.
	static Lexer& instance();

	virtual ~Lexer();

	void PushFile(std::istream& input, std::string name);
	void PopFile();

	Token NextToken() const;
	SourceRange CurrentTokenRange() const;

	const ErrorReport& Err(const char *message);

	int yylex(YYSTYPE *yylval);

private:
	void SetComment(YYSTYPE*, bool includesNewline = true);
	void SetToken(YYSTYPE*);

	void BeginString();
	void AppendChar(char);
	void EndString(YYSTYPE*);

	std::string currentFilename() const;

	std::stack<std::string> filenames_;
	std::stack<SourceLocation> locations_;
	UniqPtrVec<ErrorReport> errs_;

	fabrique::Token currentToken_;
	fabrique::SourceLocation stringStart_;
	std::vector<char> buffer_;
};

} // namespace fabrique

#endif
