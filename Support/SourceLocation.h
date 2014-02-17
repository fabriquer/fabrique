/**
 * @file SourceLocation.h
 * Declaration of @ref HasSource, @ref SourceLocation and @ref SourceRange.
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

#ifndef LOCATION_H
#define LOCATION_H

#include "ADT/UniqPtr.h"
#include "Support/Printable.h"

#include <string>

namespace fabrique {

class Lexer;


//! A location in the original source code.
class SourceLocation : public Printable
{
public:
	const static SourceLocation Nowhere;

	SourceLocation(const std::string& filename = "",
	               int line = 0, int column = 0);

	operator bool() const;
	bool operator == (const SourceLocation&) const;
	bool operator != (const SourceLocation&) const;

	virtual void PrettyPrint(Bytestream&, int indent = 0) const;

	std::string filename;
	int line;
	int column;
};


class HasSource;

class SourceRange : public Printable
{
public:
	const static SourceRange None;

	//! Construct a short (within a single line) range.
	static SourceRange Span(
		const std::string& filename, int line,
		int begincol, int endcol);

	//! Create a range that spans two @ref HasSource objects.
	template<class T1, class T2>
	static SourceRange Over(const T1& b, const T2& e)
	{
		const SourceLocation& begin =
			b ? b->source().begin : SourceLocation::Nowhere;

		const SourceLocation& end =
			e ? e->source().end : SourceLocation::Nowhere;

		return SourceRange(begin, end);
	}

	SourceRange(const SourceLocation& begin, const SourceLocation& end);
	SourceRange(const SourceRange& begin, const SourceRange& end);
	SourceRange(const HasSource& begin, const HasSource& end);

	operator bool() const;
	bool operator == (const SourceRange&) const;
	bool operator != (const SourceRange&) const;

	virtual void PrettyPrint(Bytestream&, int indent = 0) const;

	SourceLocation begin;
	SourceLocation end;
};


//! A mixin type for something that has a location in source code.
class HasSource
{
public:
	HasSource(const SourceRange& src) : src(src) {}
	const SourceRange& source() const { return src; }

private:
	SourceRange src;
};

} // class fabrique

#endif
