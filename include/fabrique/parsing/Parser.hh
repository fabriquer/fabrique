/** @file Parsing/Parser.h    Declaration of @ref fabrique::ast::Parser. */
/*
 * Copyright (c) 2013-2014, 2018 Jonathan Anderson
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

#ifndef PARSER_H
#define PARSER_H

#include <fabrique/UniqPtr.h>
#include <fabrique/ast/ast.hh>
#include "Support/ErrorReport.h"

#include <map>
#include <stack>

namespace fabrique {

class TypeContext;
class Lexer;
class Token;

namespace plugin {
class Loader;
class Registry;
}

namespace parsing {


/**
 * Parses Fabrique files as driven by flex/byacc.
 */
class Parser
{
public:
	/**
	 * Constructor.
	 *
	 * @param    prettyPrint      pretty-print values or files as they are parsed
	 * @param    dump             dump values as they are parsed
	 */
	Parser(bool prettyPrint, bool dump);

	template<typename T>
	struct Result
	{
		T result;
		std::vector<ErrorReport> errors;

		static Result Ok(T value) { return { std::move(value), {} }; }
		static Result Err(std::vector<ErrorReport> errs) { return { {}, errs }; }
	};

	using ValueResult = Result<UniqPtr<ast::Value>>;
	using FileResult = Result<UniqPtrVec<ast::Value>>;

	//! Parse a single value (e.g., a command-line definition)
	ValueResult Parse(std::string, SourceRange = SourceRange::None());

	//! Parse Fabrique input (usually a file) into @ref Value objects.
	FileResult ParseFile(std::istream&, std::string name = "");

private:
	const bool prettyPrint_;
	const bool dump_;
};

} // namespace parsing
} // namespace fabrique

#endif
