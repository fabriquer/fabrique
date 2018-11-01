/** @file Parsing/Parser.cc    Definition of @ref fabrique::ast::Parser. */
/*
 * Copyright (c) 2013-2014, 2018 Jonathan Anderson
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

#include <fabrique/ast/ast.hh>
#include <fabrique/parsing/ASTBuilder.hh>
#include <fabrique/parsing/ErrorListener.hh>
#include <fabrique/parsing/Parser.hh>
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include <fabrique/types/TypeContext.hh>

#include <generated-grammar/FabLexer.h>
#include <generated-grammar/FabParser.h>
#include <generated-grammar/FabParserBaseVisitor.h>

#include <antlr-cxx-runtime/ANTLRFileStream.h>
#include <antlr-cxx-runtime/CommonTokenStream.h>

#include <cassert>
#include <fstream>
#include <sstream>

using namespace fabrique;
using namespace fabrique::parsing;

using std::string;
using std::unique_ptr;


UniqPtr<ast::Value> Parser::Parse(std::string s, SourceRange src)
{
	Bytestream& dbg = Bytestream::Debug("parser");
	dbg
		<< Bytestream::Action << "Parsing"
		<< Bytestream::Type << " string"
		<< Bytestream::Operator << " '"
		<< Bytestream::Literal << s
		<< Bytestream::Operator << "'"
		<< Bytestream::Reset << "\n"
		;

	antlr4::ANTLRInputStream f(s);
	FabLexer lexer(&f);

	antlr4::CommonTokenStream tokens(&lexer);
	FabParser antlrParser(&tokens);

	const string name = src.filename();

	ErrorListener errorListener(name);
	auto &errorProxy = antlrParser.getErrorListenerDispatch();
	errorProxy.removeErrorListeners();
	errorProxy.addErrorListener(&errorListener);

	ASTBuilder visitor(name);
	if (not visitor.visitValue(antlrParser.value()))
	{
		return nullptr;
	}

	auto values = visitor.takeValues();
	SemaCheck(not values.empty(), src, "no value in '" + s + "'");
	SemaCheck(values.size() == 1, src, "multiple values in '" + s + "'");

	return std::move(values.front());
}

bool Parser::ParseFile(std::istream& input, UniqPtrVec<ast::Value>& values, string name)
{
	Bytestream& dbg = Bytestream::Debug("parser.file");
	dbg
		<< Bytestream::Action << "Parsing"
		<< Bytestream::Type << " file"
		<< Bytestream::Operator << " '"
		<< Bytestream::Literal << name
		<< Bytestream::Operator << "'"
		<< Bytestream::Reset << "\n"
		;

	antlr4::ANTLRInputStream f(input);
	FabLexer lexer(&f);

	antlr4::CommonTokenStream tokens(&lexer);
	FabParser antlrParser(&tokens);

	ErrorListener errorListener(name);
	auto &errorProxy = antlrParser.getErrorListenerDispatch();
	errorProxy.removeErrorListeners();
	errorProxy.addErrorListener(&errorListener);

	ASTBuilder visitor(name);
	if (not visitor.visitFile(antlrParser.file()))
	{
		return false;
	}

	values = visitor.takeValues();
	return true;
}


const ErrorReport& Parser::ReportError(const string& msg, const HasSource& s,
                                       ErrorReport::Severity severity)
{
	return ReportError(msg, s.source(), severity);
}

const ErrorReport& Parser::ReportError(const string& message,
                                       const SourceRange& location,
                                       ErrorReport::Severity severity)
{
	errs_.emplace_back(message, location, severity);
	return errs_.back();
}
