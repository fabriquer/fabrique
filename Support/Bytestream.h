/** @file Bytestream.h    Declaration of @ref Bytestream. */
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

#ifndef BYTESTREAM_H
#define BYTESTREAM_H

#include <iostream>


namespace fabrique {

class Printable;

//! A ostream-like class that may support formatting.
class Bytestream
{
public:
	//! Output formats that the bytestream may (optionally) support.
	enum Format
	{
		// Source code formatting:
		Action,
		Comment,
		Definition,
		Filename,
		Literal,
		Operator,
		Reference,
		Type,

		// Error reporting:
		Error,
		ErrorLoc,

		// Source locations:
		Column,
		Line,

		// Other:
		Reset,
	};

	static Bytestream& Stdout();
	static Bytestream& Stderr();

	/**
	 * Construct a formatted @ref Bytestream to wrap an @ref std::ostream.
	 *
	 * The caller is responsible for freeing the returned pointer.
	 */
	static Bytestream* Formatted(std::ostream&);

	/**
	 * Construct a plain @ref Bytestream to wrap an @ref std::ostream.
	 *
	 * The caller is responsible for freeing the returned pointer.
	 */
	static Bytestream* Plain(std::ostream&);


	virtual Bytestream& operator << (enum Format) = 0;
	Bytestream& operator << (const Printable&);
	Bytestream& operator << (const std::string&);
	Bytestream& operator << (unsigned long);

	std::ostream& raw() { return out; }

protected:
	Bytestream(std::ostream& o)
		: out(o)
	{
	}

	std::ostream& out;
};

} // namespace fabrique

#endif // !BYTESTREAM_H
