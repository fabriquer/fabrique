/**
 * @file Support/SourceLocation.cc
 * Definition of @ref fabrique::SourceLocation and @ref SourceRange.
 */
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

#include "Support/Bytestream.h"
#include "Support/SourceLocation.h"

#include <cassert>

using namespace fabrique;


static const SourceLocation& Nowhere()
{
	static SourceLocation& nowhere = *new SourceLocation;
	return nowhere;
}


SourceLocation::SourceLocation(const std::string& file,
                               size_t lineno, size_t colno)
	: filename(file), line(lineno), column(colno)
{
}

SourceLocation::operator bool() const
{
	return not (line == 0);
}

bool SourceLocation::operator < (const SourceLocation& other) const
{
	return filename < other.filename
		or line < other.line
		or column < other.column;
}

bool SourceLocation::operator > (const SourceLocation& other) const
{
	return filename > other.filename
		or line > other.line
		or column > other.column;
}

bool SourceLocation::operator == (const SourceLocation& other) const
{
	return filename == other.filename
		and line == other.line
		and column == other.column;
}


bool SourceLocation::operator != (const SourceLocation& other) const
{
	return not (*this == other);
}


void SourceLocation::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out
		<< Bytestream::Filename
		<< (filename.empty() ? "-" : filename)
		;

	if (line > 0)
		out
			<< Bytestream::Operator << ":"
			<< Bytestream::Line << line
			;

	if (column > 0)
		out
			<< Bytestream::Operator << ":"
			<< Bytestream::Column << column
			;

	out << Bytestream::Reset;
}



const SourceRange& SourceRange::None()
{
	static SourceRange& none = *new SourceRange(Nowhere(), Nowhere());
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


void SourceRange::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out
		<< Bytestream::Filename << begin.filename
		<< Bytestream::Operator << ":"
		;

	// The end column is the first character in the next token; don't
	// report this when printing out the current location.
	size_t endcol = end.column - 1;

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
