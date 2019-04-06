//! @file SourceRange.cc     Definition of @ref fabrique::SourceRange
/*
 * Copyright (c) 2013, 2016, 2018 Jonathan Anderson
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

#include <fabrique/Bytestream.hh>
#include <fabrique/HasSource.hh>
#include <fabrique/SourceRange.hh>

#include <cassert>
#include <fstream>

using namespace fabrique;
using std::string;


const SourceRange& SourceRange::None()
{
	static SourceRange& none = *new SourceRange({}, {});
	return none;
}


SourceRange::SourceRange(const SourceLocation& b, const SourceLocation& e)
	: begin(b), end(e)
{
}

SourceRange::SourceRange(const SourceRange& b, const SourceRange& e)
	: SourceRange(b.begin, e.end)
{
}

SourceRange::SourceRange(const HasSource& b, const HasSource& e)
	: SourceRange(b.source(), e.source())
{
}


SourceRange SourceRange::Span(const std::string& filename, size_t line,
                              size_t begin, size_t end)
{
	return SourceRange(
		SourceLocation(filename, line, begin),
		SourceLocation(filename, line, end)
	);
}


SourceRange::operator bool() const
{
	return begin and end;
}

bool SourceRange::operator < (const SourceRange& other) const
{
	return begin < other.begin or end < other.end;
}

bool SourceRange::operator > (const SourceRange& other) const
{
	return begin > other.begin or end > other.end;
}

bool SourceRange::operator == (const SourceRange& other) const
{
	return begin == other.begin and end == other.end;
}


bool SourceRange::operator != (const SourceRange& other) const
{
	return not (*this == other);
}


bool SourceRange::isInside(const SourceRange& other) const
{
	if (begin.filename != other.begin.filename
	    or end.filename != other.end.filename)
	{
		return false;
	}

	if (begin.line < other.begin.line)
	{
		return false;
	}

	if (begin.line == other.begin.line and begin.column < other.begin.column)
	{
		return false;
	}

	if (end.line > other.end.line)
	{
		return false;
	}

	if (end.line == other.end.line and end.column > other.end.column)
	{
		return false;
	}

	return true;
}

string SourceRange::filename() const
{
	return begin.filename;
}

void SourceRange::PrettyPrint(Bytestream& out, unsigned int /*indent*/) const
{
	out
		<< Bytestream::Filename << begin.filename
		<< Bytestream::Operator << ":"
		;

	// The end column is the first character in the next token; don't
	// report this when printing out the current location.
	size_t endcol = end.column ? end.column - 1 : 0;

	if (begin.line == end.line)
	{
		out
			<< Bytestream::Line << begin.line
			<< Bytestream::Operator << ":"
			<< Bytestream::Column << begin.column
			;

		if (endcol != begin.column)
			out
				<< Bytestream::Operator << "-"
				<< Bytestream::Column << endcol
				;
	}
	else
		out
			<< Bytestream::Line << begin.line
			<< Bytestream::Operator << ":"
			<< Bytestream::Column << begin.column
			<< Bytestream::Operator << "-"
			<< Bytestream::Line << end.line
			<< Bytestream::Operator << ":"
			<< Bytestream::Column << endcol
			;

	out << Bytestream::Reset;
}


Bytestream& SourceRange::PrintSource(Bytestream& out, SourceLocation caret,
                                     unsigned int context) const
{
	/*
	 * If we are reading a file (rather than stdin), re-read the
	 * source file to display the line in question.
	 *
	 * We are very careful not to make any assumptions about how the parsing
	 * code works, so we need to re-open the file. Also, this means that we can't
	 * do anything similar for stdin.
	 */
	const string filename = begin.filename;

	if (!filename.empty())
	{
		std::ifstream sourceFile(filename.c_str());
		if (not sourceFile.good())
		{
			return out;
		}

		string line;   // the last-read line

		const size_t firstLine = begin.line > context ? (begin.line - context) : 1;
		size_t endColumn = end.column;

		for (size_t i = 1; i <= end.line; i++)
		{
			getline(sourceFile, line);

			if (i >= firstLine)
			{
				if (i >= begin.line and begin.line != end.line)
				{
					endColumn = std::max(endColumn, line.length());
				}

				out
					<< Bytestream::Line << i << "\t"
					<< Bytestream::Reset << line << "\n"
					;
			}
		}

		/*
		 * If the expression starts on a line before the caret point,
		 * start highlighting with '~' characters from the beginning
		 * of the line.
		 *
		 * Otherwise, start where the source range says to.
		 */
		const size_t beginColumn = (begin.line == end.line) ? begin.column : 1;
		const size_t preCaretHighlight = caret ? (caret.column - beginColumn) : 0;
		const size_t soFar = caret ? caret.column + 1 : beginColumn;
		const size_t postCaretHighlight =
			(soFar > endColumn) ? 0 : endColumn - soFar;

		assert(preCaretHighlight >= 0);
		assert(postCaretHighlight >= 0);

		out << "\t";

		for (size_t i = 0; i < beginColumn - 1; i++)
		{
			out << ((line.length() > i and line[i] == '\t') ? '\t' : ' ');
		}

		out
			<< Bytestream::ErrorLoc
			<< string(preCaretHighlight, '~')
			<< (caret ? "^" : "")
			<< string(postCaretHighlight, '~')
			<< Bytestream::Reset << "\n"
			;
	}

	return out;
}
