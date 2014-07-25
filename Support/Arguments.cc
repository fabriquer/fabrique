/** @file Support/Arguments.cc    Definition of @ref fabrique::Arguments. */
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
#include "Support/Bytestream.h"
#include "Support/os.h"

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
	Define,
	Format,
	OutputDirectory,
	ParseOnly,
	PrettyPrintAST,
	PrettyPrintDAG,
	PrintOutput,
	DebugPattern,
};


enum optionKind
{
	SetOpt,    //!< Set an optional value.
	AppendOpt, //!< Append an optional value to an ordered list.
	Enable,    //!< Turn on a boolean value.
	Disable,   //!< Turn off a boolean value.
	OtherOpt,  //!< Do something else (e.g., usage description.)
};

// Option validation functions:
static ArgStatus Required(const option::Option&, bool);
static ArgStatus NonEmpty(const option::Option&, bool);
static ArgStatus IsOutputFormat(const option::Option&, bool);

//! Possible output file formats (name, tool description).
const static char* formatStrings[][2] = {
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


// We need to use global constructors/destructors for this interface with
// optionparser.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"

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
		OutputDirectory, SetOpt, "o", "output", option::Arg::Optional,
		"  -o,--output      Output directory (default: .)."
	},
	{
		Define, AppendOpt, "D", "define", Required,
		"  -D,--define      A value to expose to Fabrique description."
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
	{
		DebugPattern, SetOpt, "", "debug", option::Arg::Optional,
		"  --debug          Show debug output (e.g. 'parser', equivalent to 'parser.*')"
	},
	{ 0, 0, 0, 0, 0, 0 }
};

#pragma clang diagnostic pop


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

	const string output = options[OutputDirectory]
	                       ? options[OutputDirectory].arg
	                       : ".";

	const bool outputSpecified = options[OutputDirectory];

	std::vector<string> definitions;
	for (Option *o = options[Define]; o; o = o->next())
		definitions.emplace_back(o->arg);

	const string format = options[Format]
	                       ? options[Format].arg
	                       : "ninja";

	const string debugPattern = options[DebugPattern]
	                             ? options[DebugPattern].arg
	                             : "none";

	return new Arguments {
		help,
		input,
		output,
		outputSpecified,
		definitions,
		format,
		options[ParseOnly],
		options[PrettyPrintAST],
		options[PrettyPrintDAG],
		options[PrintOutput],
		debugPattern
	};
}


std::vector<string> Arguments::ArgVector(const Arguments& args)
{
	std::vector<string> argv;

	argv.push_back("--debug='" + args.debugPattern + "'");

	if (args.help)
		argv.push_back("--help");

	if (args.parseOnly)
		argv.push_back("--parse-only");
	else
		argv.push_back("--format=" + args.format);

	if (args.printAST)
		argv.push_back("--print-ast");

	if (args.printDAG)
		argv.push_back("--print-dag");

	if (args.printOutput)
		argv.push_back("--stdout");
	else
		argv.push_back("--output=" + AbsoluteDirectory(args.output));

	for (const string& d : args.definitions)
		argv.push_back("-D '" + d + "'");

	return std::move(argv);
}


#define ARG(name) \
	Bytestream::Definition << tab << #name \
	<< Bytestream::Operator << " = " \
	<< Bytestream::Literal << args.name << "\n"

static Bytestream& operator<< (Bytestream& out, const std::vector<string>&);

void Arguments::Print(const Arguments& args, Bytestream& out)
{
	const string tab(1, '\t');

	out
		<< Bytestream::Action << "Arguments\n"
		<< Bytestream::Operator << "{\n"
		<< ARG(help)
		<< ARG(input)
		<< ARG(output)
		<< ARG(outputFileSpecified)
		<< ARG(format)
		<< ARG(definitions)
		<< ARG(parseOnly)
		<< ARG(printAST)
		<< ARG(printDAG)
		<< ARG(printOutput)
		<< ARG(debugPattern)
		<< Bytestream::Operator << "}"
		<< Bytestream::Reset
		;
}

string Arguments::str(const Arguments& args)
{
	std::ostringstream oss;

	for (const string& a : Arguments::ArgVector(args))
		oss << " " << a;

	return oss.str();
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

	for (const char** const& format : formatStrings)
		if (arg == format[0])
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

static Bytestream& operator<< (Bytestream& out, const std::vector<string>& v)
{
	out << Bytestream::Operator << "[ ";

	for (const string& s : v)
		out
			<< Bytestream::Operator << "'"
			<< Bytestream::Literal << s
			<< Bytestream::Operator << "' "
			;

	out << Bytestream::Operator << "]";

	return out;
}
