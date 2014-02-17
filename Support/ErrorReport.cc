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

#include <cassert>
#include <fstream>
#include <list>
#include <map>

#include "Support/Bytestream.h"

using namespace fabrique;
using std::string;


ErrorReport* ErrorReport::Create(const string& message, const SourceRange& loc,
                                 int contextLines)
{
	return new ErrorReport(message, loc, loc.begin, contextLines);
}


void ErrorReport::PrettyPrint(Bytestream& out, int indent) const
{
	string tabs(indent, '\t');

	out << "\n" << tabs << caret;

	out
		<< ": "
		<< Bytestream::Error << "error"
		<< Bytestream::Reset << ": " << message
		<< Bytestream::Reset << "\n"
		;

	/*
	 * If we are reading a file (rather than stdin), re-read the
	 * source file to display the offending line.
	 *
	 * Currently, we are very careful not to make any assumptions about
	 * how much of the original source buffer lex has kept around, so
	 * there's no such output for source from stdin.
	 */
	string filename(caret.filename);
	const SourceRange& source = this->source();
	if (filename.empty()) filename = source.begin.filename;

	if (!filename.empty())
	{
		std::ifstream sourceFile(filename.c_str());
		for (int i = 1; i <= caret.line; i++) {
			string line;
			getline(sourceFile, line);

			if (i >= (caret.line - contextLines))
				out
					<< tabs
					<< Bytestream::Line << i << "\t"
					<< Bytestream::Reset << line << "\n"
					;
		}

		/*
		 * If the expression starts on a line before the caret point,
		 * start highlighting with '~' characters from the beginning
		 * of the line.
		 *
		 * Otherwise, start where the source range says to.
		 */
		const int firstHighlightColumn =
			source.begin.column < caret.column
				? caret.column - source.begin.column
				: caret.column;

		const int preCaretHighlight =
			caret.column - firstHighlightColumn;

		const int postCaretHighlight =
			source.end.column >= caret.column
			  ? source.end.column - caret.column
			  : 0;

		assert(firstHighlightColumn >= 1);
		assert(preCaretHighlight >= 0);
		assert(postCaretHighlight >= 0);

		out
			<< tabs << "\t"
			<< string(firstHighlightColumn - 1, ' ')
			<< Bytestream::ErrorLoc
			<< string(preCaretHighlight, '~')
			<< "^"
			<< string(postCaretHighlight, '~')
			<< "\n"
			;
	}

	out << Bytestream::Reset;
}
