//! @file Parsing/ErrorListener.h   Declaration of @ref fabrique::ast::ErrorListener
/*
 * Copyright (c) 2018 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed at Memorial University of Newfoundland
 * under the NSERC Discovery program (RGPIN-2015-06048).
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

#include <antlr-cxx-runtime/antlr4-runtime.h>

#include "Parsing/ErrorListener.h"
#include "Support/ABI.h"
#include "Support/SourceLocation.h"
#include "Support/exceptions.h"

using namespace fabrique;
using namespace fabrique::ast;
using namespace antlr4;
using std::string;


ErrorListener::ErrorListener(string filename)
	: filename_(filename)
{
}

void ErrorListener::syntaxError(antlr4::Recognizer *r, Token *t, size_t line, size_t col,
                                const string &msg, std::exception_ptr e)
{
	string message = msg;
	string detail;

	try
	{
		std::rethrow_exception(e);
	}
	catch (antlr4::RecognitionException &e)
	{
		message = "syntactically invalid token '" + t->getText() + "'";
		detail = "expected to find one of: "
			+ e.getExpectedTokens().toString(r->getVocabulary())
			;
	}
	catch (std::exception &e)
	{
		FAB_ASSERT(false, "unhandled exception of type " + TypeName(e));
	}

	SourceRange src = SourceRange::Span(filename_, line, col + 1, col + 2);
	throw SyntaxError(message, src, detail);
}
