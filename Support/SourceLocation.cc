/**
 * @file SourceLocation.cc
 * Definition of @ref SourceLocation and @ref SourceRange.
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

using namespace fabrique;


void SourceLocation::PrettyPrint(Bytestream& out, int indent) const
{
	if (!filename.empty())
		out
			<< Bytestream::Filename << filename
			<< Bytestream::Reset << ":";

	if (line > 0)
		out
			<< Bytestream::Line << line
			<< Bytestream::Reset << ":";

	if (column > 0)
		out
			<< Bytestream::Column << column
			<< Bytestream::Reset;
}

SourceRange SourceRange::Span(const std::string& filename, int line,
                              int begin, int end)
{
	return SourceRange(
		SourceLocation(filename, line, begin),
		SourceLocation(filename, line, end)
	);
}

SourceRange SourceRange::Over(const HasSource *begin, const HasSource *end)
{
	static const SourceLocation nowhere;

	const SourceLocation& b(begin ? begin->source().begin : nowhere);
	const SourceLocation& e(end ? end->source().end : nowhere);

	return SourceRange(b, e);
}

SourceRange SourceRange::Over(const HasSource& begin, const HasSource& end)
{
	return SourceRange::Over(begin.source(), end.source());
}

SourceRange SourceRange::Over(const SourceRange& begin, const SourceRange& end)
{
	return SourceRange(begin.begin, end.end);
}


SourceRange SourceRange::None()
{
	SourceLocation nowhere;
	return SourceRange(nowhere, nowhere);
}

void SourceRange::PrettyPrint(Bytestream& out, int indent) const
{
	out
		<< Bytestream::Filename << begin.filename
		<< Bytestream::Reset << ":"
		;

	if (begin.line == end.line)
		out
			<< Bytestream::Line << begin.line
			<< Bytestream::Reset << ":"
			<< Bytestream::Column << begin.column
			<< Bytestream::Reset << "-"
			<< Bytestream::Column << end.column
			;
	else
		out
			<< Bytestream::Line << begin.line
			<< Bytestream::Reset << ":"
			<< Bytestream::Column << begin.column
			<< Bytestream::Reset << "-"
			<< Bytestream::Line << end.line
			<< Bytestream::Reset << ":"
			<< Bytestream::Column << end.column
			;

	out << Bytestream::Reset;
}
