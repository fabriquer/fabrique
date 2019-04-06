//! @file Parsing/ErrorListener.h   Declaration of @ref fabrique::ast::ErrorListener
/*
 * Copyright (c) 2018-2019 Jonathan Anderson
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

#include <fabrique/AssertionFailure.hh>
#include <fabrique/SemanticException.hh>
#include <fabrique/parsing/ErrorListener.hh>
#include <fabrique/platform/ABI.hh>

#include <antlr-cxx-runtime/antlr4-runtime.h>

using namespace fabrique;
using namespace fabrique::parsing;
using namespace antlr4;
using std::string;


ErrorListener::ErrorListener(string filename)
	: filename_(filename)
{
}

std::vector<ErrorReport> ErrorListener::errors() const
{
	return errors_;
}

void ErrorListener::syntaxError(antlr4::Recognizer *r, Token *t, size_t line, size_t col,
                                const string &msg, std::exception_ptr e)
{
	string message = msg;
	string detail;

	try
	{
		if (e)
		{
			std::rethrow_exception(e);
		}
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
		FAB_ASSERT(false, "unhandled exception of type " + platform::TypeName(e));
	}
	catch (...)
	{
		FAB_ASSERT(false, "unhandled exception of unknown type");
	}

	const size_t fullLength = t->getText().length();
	SemaCheck(fullLength < (1L << 32),
		SourceRange::Span(filename_, line, col + 1, col + 2),
		"token has extraordinary length: " + std::to_string(fullLength));

	const auto len = static_cast<unsigned int>(fullLength);
	SourceRange src = SourceRange::Span(filename_, line, col + 1, col + 1 + len);
	errors_.emplace_back(message, src, ErrorReport::Severity::Error, detail);
}
