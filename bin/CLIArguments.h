//! @file bin/CLIArguments.h    Declaration of @ref fabrique::CLIArguments
/*
 * Copyright (c) 2013, 2018 Jonathan Anderson
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

#ifndef FAB_CLI_ARGUMENTS_H
#define FAB_CLI_ARGUMENTS_H

#include <string>
#include <vector>

namespace fabrique {

class Bytestream;

enum OutputFormat
{
	Fabrique,
	Make,
	Ninja,
	Sh,
};

/**
 * Command-line options and arguments after parsing, type-checking, etc.
 */
class CLIArguments
{
public:
	static void PrintUsage(std::ostream&);
	static CLIArguments* Parse(int argc, char *argv[]);
	std::vector<std::string> ArgVector();

	void Print(Bytestream&);
	std::string str();

	//! The currently-running binary.
	const std::string executable;

	const bool help;

	const std::string input;
	const std::string output;
	const bool outputFileSpecified;

	const std::vector<std::string> definitions;
	const std::vector<std::string> outputFormats;
	const bool parseOnly;
	const bool printAST;
	const bool dumpAST;
	const bool printDAG;
	const bool printOutput;

	const std::string debugPattern;
};

} // namespace fabrique

#endif
