/** @file Arguments.h    Declaration of @ref Arguments. */
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

#ifndef ARGUMENTS_H
#define ARGUMENTS_H

// Include Matthias Benkmann's "Lean Mean C++ Option Parser".
#include "Support/optionparser.h"

#include <ostream>
#include <string>

namespace fabrique {

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
class Arguments
{
public:
	static void PrintUsage(std::ostream&);
	static Arguments* Parse(int argc, char *argv[]);

	const bool help;

	const std::string input;
	const std::string output;
	const bool outputFileSpecified;

	const std::string format;
	const bool parseOnly;
	const bool printAST;
	const bool printDAG;
	const bool printOutput;

private:
	Arguments(const bool help,
	          const std::string& input, const std::string& output,
	          const std::string& format, bool parseOnly,
	          bool printAST, bool printDAG, bool printOut)
		: help(help), input(input), output(output),
		  outputFileSpecified(output.length() > 0 and output != "-"),
		  format(format), parseOnly(parseOnly),
		  printAST(printAST), printDAG(printDAG), printOutput(printOut)
	{
	}

};

} // namespace fabrique

#endif
