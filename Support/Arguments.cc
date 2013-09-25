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

#include "Arguments.h"

using namespace fabrique;
using std::string;


void Arguments::Usage(std::ostream& out, const string& name)
{
	out
		<< "Usage:  " << name << " [options] <input-filename>\n"
		<< "\n"
		<< "Options:\n"
		<< "  --print              pretty-print the parsed file\n"
		<< "\n"
		<< "  -f format            output format (default: ninja)\n"
		<< "  -o output            output filename (default: stdout)\n"
		<< "\n"
		;
}

Arguments* Arguments::Parse(int argc, char *argv[])
{
	string input;
	string output;
	string format = "ninja";
	bool prettyPrint = false;
	string *next = NULL;

	for (int i = 1; i < argc; i++)
	{
		string arg(argv[i]);

		if (arg == "-o")
		{
			next = &output;
		}
		else if (arg == "-f")
		{
			next = &format;
		}
		else if (next)
		{
			*next = arg;
			next = NULL;
		}
		else if (arg == "--print")
		{
			prettyPrint = true;
		}
		else if (input.length() == 0)
		{
			input = arg;
		}
		else
		{
			return NULL;
		}
	}

	return new Arguments(input, output, format, prettyPrint);
}
