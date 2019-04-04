//! @file Fabrique.hh    Declaration of the top-level Fabrique type
/*
 * Copyright (c) 2018-2019 Jonathan Anderson
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

#ifndef FABRIQUE_H_
#define FABRIQUE_H_

#include <fabrique/builtins.hh>
#include <fabrique/ErrorReport.hh>
#include <fabrique/backend/Backend.hh>
#include <fabrique/dag/Value.hh>
#include <fabrique/parsing/Parser.hh>
#include <fabrique/types/TypeContext.hh>

#include <functional>


namespace fabrique {

/**
 * A complete Fabrique instance that can parse and interpret Fabrique files, manage
 * types associated with Fabrique values and manage backends.
 */
class Fabrique
{
public:
	using ErrorReporter = std::function<void (ErrorReport)>;

	/**
	 * Constructor.
	 *
	 * Rather than providing arguments for all of these parameters positionally,
	 * it is probably more convenient to use a FabBuilder.
	 */
	Fabrique(bool parseOnly, bool printASTs, bool dumpASTs, bool printDAG,
	         bool printToStdout, UniqPtrVec<backend::Backend> backends,
		 std::string outputDir, std::vector<std::string> pluginSearchPaths,
		 std::string regenCommand, ErrorReporter);

	Fabrique(Fabrique&&);


	//! Parse several arguments for the top-level Fabrique file
	void AddArguments(const std::vector<std::string>&);

	/**
	 * Process a top-level Fabrique file.
	 *
	 * Depending on the options and backends we have been configured with,
	 * this may also cause DAG and Backend processing to occur.
	 */
	void Process(const std::string &filename);

	const dag::ValueMap& arguments() const { return arguments_; }
	TypeContext& types() { return types_; }

private:
	/**
	 * Parse an argument to the top-level Fabrique file (e.g., an argument
	 * passed at the command line, i.e., `fab -D foo=42`).
	 */
	void AddArgument(const std::string&);

	//! Parse a file, optionally pretty-printing it.
	const UniqPtrVec<ast::Value>& Parse(std::istream&, const std::string &filename);

	void ReportError(std::string message, SourceRange, ErrorReport::Severity,
	                 std::string detail);

	const bool parseOnly_;
	const bool printDAG_;
	const bool printToStdout_;

	const UniqPtrVec<backend::Backend> backends_;

	ErrorReporter err_;
	TypeContext types_;
	parsing::Parser parser_;

	dag::ValueMap arguments_;

	std::vector<std::string> outputFiles_;

	const std::string outputDirectory_;
	const std::vector<std::string> pluginPaths_;

	//! The command used to regenerate our build description (if set)
	const std::string regenerationCommand_;
};

} // namespace fabrique

#endif  // FABRIQUE_H_
