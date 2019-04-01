/** @file Parsing/Parser.cc    Definition of @ref fabrique::parsing::Parser. */
/*
 * Copyright (c) 2013-2014, 2018-2019 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme and at Memorial University
 * of Newfoundland under the NSERC Discovery program (RGPIN-2015-06048).
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

#include <fabrique/Bytestream.hh>
#include <fabrique/ast/ASTDump.hh>
#include <fabrique/parsing/ASTBuilder.hh>
#include <fabrique/parsing/ErrorListener.hh>
#include <fabrique/parsing/Parser.hh>
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


//! Internal ANTLR state
struct ParserState
{
	ParserState(antlr4::ANTLRInputStream i, string name, bool prettyPrint, bool dump)
		: input(i), name_(name), lexer(&input), tokens(&lexer), parser(&tokens),
		  errorListener(name), ast(name), prettyPrint_(prettyPrint), dump_(dump)
	{
		auto &errorProxy = parser.getErrorListenerDispatch();
		errorProxy.removeErrorListeners();
		errorProxy.addErrorListener(&errorListener);
	}

	ParserState(string s, string name, bool prettyPrint, bool dump)
		: ParserState(antlr4::ANTLRInputStream(s), name, prettyPrint, dump)
	{
	}

	ParserState(std::istream& i, string name, bool prettyPrint, bool dump)
		: ParserState(antlr4::ANTLRInputStream(i), name, prettyPrint, dump)
	{
	}

	UniqPtrVec<ast::Value> takeValues()
	{
		auto values = ast.takeValues();

		if (prettyPrint_)
		{
			Bytestream::Stdout()
				<< Bytestream::Comment
				<< "#\n"
				<< "# AST pretty-printed from '" << name_ << "'\n"
				<< "#\n"
				<< Bytestream::Reset
				;

			for (auto& val : values)
				Bytestream::Stdout() << *val << "\n";
		}

		if (dump_)
		{
			ast::ASTDump dumper(Bytestream::Stdout());
			for (auto &val : values)
			{
				val->Accept(dumper);
			}
		}

		return values;
	}

	std::vector<ErrorReport> errors() const
	{
		return errorListener.errors();
	}

	antlr4::ANTLRInputStream input;
	string name_;
	FabLexer lexer;
	antlr4::CommonTokenStream tokens;
	FabParser parser;

	ErrorListener errorListener;
	ASTBuilder ast;
	const bool prettyPrint_;
	const bool dump_;
};


Parser::Parser(bool prettyPrint, bool dump)
	: prettyPrint_(prettyPrint), dump_(dump)
{
}


Parser::ValueResult Parser::Parse(std::string s, SourceRange src)
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

	ParserState state(s, src.filename(), prettyPrint_, dump_);
	bool success = false;
	try
	{
		success = state.ast.visitValue(state.parser.value());
	}
	catch (...)
	{
		FAB_ASSERT(not state.errors().empty(), "parsing failed without error");
	}

	if (not success)
	{
		return ValueResult::Err(state.errors());
	}

	auto values = state.ast.takeValues();
	SemaCheck(not values.empty(), src, "no value in '" + s + "'");
	SemaCheck(values.size() == 1, src, "multiple values in '" + s + "'");

	return ValueResult::Ok(std::move(values.front()));
}

Parser::FileResult Parser::ParseFile(std::istream& input, string name)
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

	ParserState state(input, name, prettyPrint_, dump_);
	bool success = false;
	try
	{
		success = state.ast.visitFile(state.parser.file());
	}
	catch (...)
	{
		FAB_ASSERT(not state.errors().empty(), "parsing failed without error");
	}

	if (not success)
	{
		return FileResult::Err(state.errors());
	}

	auto values = state.ast.takeValues();

	if (prettyPrint_)
	{
		Bytestream::Stdout()
			<< Bytestream::Comment
			<< "#\n"
			<< "# AST pretty-printed from '" << name << "'\n"
			<< "#\n"
			<< Bytestream::Reset
			;

		for (auto& val : values)
		{
			Bytestream::Stdout() << *val << "\n";
		}
	}

	if (dump_)
	{
		ast::ASTDump dumper(Bytestream::Stdout());
		for (auto &val : values)
		{
			val->Accept(dumper);
		}
	}

	return FileResult::Ok(std::move(values));
}
