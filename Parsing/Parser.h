/** @file Parsing/Parser.h    Declaration of @ref fabrique::parser::Parser. */
/*
 * Copyright (c) 2013-2015 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme, as well as at
 * Memorial University under the NSERC Discovery program (RGPIN-2015-06048).
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

#include "AST/ast.h"
#include "ADT/UniqPtr.h"
#include "Support/ErrorReport.h"

#include <map>
#include <stack>

namespace fabrique {

namespace plugin {
class Loader;
class Plugin;
class Registry;
} // namespace plugin

namespace parser
{

class Parser
{
public:
	Parser(TypeContext&, plugin::Registry&, plugin::Loader&, std::string srcroot);

	//! Parse Fabrique fragments defined at, e.g., the command line.
	const Type& ParseDefinitions(const std::vector<std::string>& defs);

	//! Parse Fabrique input (usually a file) into a @ref Scope.
	UniqPtr<ast::Scope> ParseFile(
		std::istream& input, const Type& arguments, std::string name = "",
		StringMap<std::string> builtins = StringMap<std::string>(),
		SourceRange openedFrom = SourceRange::None());

	//! Errors encountered during parsing.
	const UniqPtrVec<ErrorReport>& errors() const { return errs_; }

	//! Input files encountered during parsing.
	const std::vector<std::string>& files() const { return files_; }

private:
	std::unique_ptr<plugin::Plugin> FindPlugin(std::string name);

	TypeContext& types_;

	plugin::Registry& pluginRegistry_;
	plugin::Loader& pluginLoader_;

	//! Input files, in order they were parsed.
	std::vector<std::string> files_;


	//! The root of all source files (where the top-level Fabrique file lives).
	std::string srcroot_;

	//! The subdirectory that we are currently working from.
	std::stack<std::string> currentSubdirectory_;


	//! Pre-defined values (e.g., from the command line).
	UniqPtr<ast::Scope> definitions_;

	UniqPtrVec<ErrorReport> errs_;
};

} // namespace parser
} // namespace fabrique

#endif
