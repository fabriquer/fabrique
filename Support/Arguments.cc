/** @file Arguments.cc    Definition of @ref Arguments. */
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

#include "Support/Arguments.h"

#include <iostream>
#include <sstream>
#include <vector>

using namespace fabrique;
using namespace option;
using std::string;

static std::ostream& err = std::cerr;

enum optionNames
{
	Usage,
	Help,
	Format,
	OutputFile,
	ParseOnly,
	PrettyPrintAST,
	PrettyPrintDAG,
	PrintOutput,
};


enum optionKind
{
	SetOpt,    //!< Set an optional value.
	Enable,    //!< Turn on a boolean value.
	Disable,   //!< Turn off a boolean value.
	OtherOpt,  //!< Do something else (e.g., usage description.)
};

// Option validation functions:
static ArgStatus Required(const option::Option&, bool);
static ArgStatus NonEmpty(const option::Option&, bool);
static ArgStatus IsOutputFormat(const option::Option&, bool);

//! Possible output file formats (name, tool description).
const static string formatStrings[][2] = {
	{ "null", "No output" },
	{ "fab", "Fabrique file (possibly modified/optimised)" },
	{ "dot", "Graphviz .dot graph format" },
	{ "make", "POSIX make (no BSD or GNU extensions)" },
	{ "ninja", "the Ninja build system (http://martine.github.io/ninja)" },
	{ "sh", "Bourne shell" },
};

//! A @a separator -separated string listing all valid output formats.
static string formats(std::string separator = ",");


const size_t formatsLen = sizeof(formatStrings) / sizeof(formatStrings[0]);

const string formatString =
	("  -f,--format      Format of output file (" + formats() + ").");


const option::Descriptor usage[] =
{
	{
		Usage, OtherOpt, "", "", option::Arg::None,
		"Fabrique: a tool for constructing workflows of build tools.\n"
		"\n"
		"Usage:\n"
		"  fab [options] <fabfile>\n"
		"\n"
		"Arguments:\n"
		"  <fabfile>        Build description; defaults to 'fabfile'\n"
		"\n"
		"Options:"
	},
	{
		Help, Enable, "h", "help", option::Arg::None,
		"  -h,--help        Print usage and exit."
	},
	{
		OutputFile, SetOpt, "o", "output", option::Arg::Optional,
		"  -o,--output      Output file: defaults to stdout ('-')."
	},
	{
		Format, SetOpt, "f", "format", IsOutputFormat,
		formatString.c_str()
	},
	{
		ParseOnly, Enable, "", "parse-only", option::Arg::None,
		"  --parse-only     Only parse the AST, don't build the DAG"
	},
	{
		PrettyPrintAST, Enable, "", "print-ast", option::Arg::None,
		"  --print-ast      Pretty-print the AST"
	},
	{
		PrettyPrintDAG, Enable, "", "print-dag", option::Arg::None,
		"  --print-dag      Pretty-print the AST"
	},
	{
		PrintOutput, Enable, "", "stdout", option::Arg::None,
		"  --stdout         Print the result to stdout"
	},
	{ 0, 0, 0, 0, 0, 0 }
};



void Arguments::PrintUsage(std::ostream& out)
{
	option::printUsage(out, usage);
}

Arguments* Arguments::Parse(int argc, char *argv[])
{
	option::Stats stats(usage, argc - 1, argv + 1);
	std::vector<option::Option> options(stats.options_max);
	std::vector<option::Option> buffer(stats.buffer_max);
	option::Parser opts(usage, argc - 1, argv + 1,
	                    options.data(), buffer.data());

	if (opts.nonOptionsCount() > 1)
		return NULL;

	const bool help = options[Help];

	const string input = opts.nonOptionsCount() == 1
	                      ? opts.nonOption(0)
	                      : "fabfile";

	const string output = options[OutputFile]
	                       ? options[OutputFile].arg
	                       : "-";

	const string format = options[Format]
	                       ? options[Format].arg
	                       : "ninja";

	return new Arguments(help, input, output, format, options[ParseOnly],
	                     options[PrettyPrintAST], options[PrettyPrintDAG],
	                     options[PrintOutput]);
}


static ArgStatus Required(const Option& o, bool printErr)
{
	if (o.arg != NULL)
		return ARG_OK;

	if (printErr)
		err << "Option '" << o << "' requires an argument\n";

	return ARG_ILLEGAL;
}


static ArgStatus NonEmpty(const option::Option& o, bool printErr)
{
	ArgStatus req = Required(o, printErr);
	if (req != ARG_OK)
		return req;

	if (o.arg[0] != '\0')
		return ARG_OK;

	if (printErr)
		err << "Missing argument for option '" << o << "'\n";

	return ARG_ILLEGAL;
}


static ArgStatus IsOutputFormat(const Option& opt, bool printErr)
{
	ArgStatus basic = NonEmpty(opt, printErr);
	if (basic != ARG_OK)
		return basic;

	const string arg(opt.arg);

	for (auto& formatString : formatStrings)
		if (arg == formatString[0])
			return ARG_OK;

	return ARG_ILLEGAL;
}


static string formats(string separator)
{
	std::ostringstream oss;

	for (size_t i = 0; i < formatsLen; /* i++ done within loop */)
	{
		oss << formatStrings[i][0];
		if (++i < formatsLen)
			oss << separator;
	}

	return oss.str();
}
