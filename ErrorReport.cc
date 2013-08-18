/** @file ErrorReport.h    Declaration of @ref ErrorReport. */
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

#include "ErrorReport.h"

#include <fstream>
#include <iostream>
#include <list>
#include <map>

#include "ostream.h"

using std::ostream;
using std::string;


ErrorReport* ErrorReport::Create(const std::string& message, std::string source,
                                 int line, int column, int len,
                                 int context)
{
	return new ErrorReport(message, source, line, column, len, context);
}


void ErrorReport::print(std::ostream& out) const
{
	out
		<< "\n"
		<< Bold
		<< Magenta << (sourceFile.empty() ? "-" : sourceFile)
		<< White << ":" << lineno << ":" << column << ": "
		<< Red << "error" << White << ": "
		<< Cyan << message
		<< ResetAll << "\n"
		;

	/*
	 * If we are reading a file (rather than stdin), re-read the
	 * source file to display the offending line.
	 *
	 * Currently, we are very careful not to make any assumptions about
	 * how much of the original source buffer lex has kept around, so
	 * there's no such output for source from stdin.
	 */
	if (!sourceFile.empty())
	{
		std::ifstream source(sourceFile.c_str());
		for (size_t i = 0; i < lineno; i++) {
			string line;
			getline(source, line);

			if (i >= (lineno - context))
				out
					<< Blue << i << "\t"
					<< White << line << "\n"
					;
		}

		out
			<< "\t"
			<< string(column - 1, ' ')
			<< Green
			<< "^"
			<< string(len - 1, '~')
			<< "\n"
			;
	}

	out << ResetAll;
}
