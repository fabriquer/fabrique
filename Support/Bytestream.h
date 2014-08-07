/** @file Support/Bytestream.h    Declaration of @ref fabrique::Bytestream. */
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
		Warning,
		Info,
		ErrorLoc,
		ErrorMessage,

		// Source locations:
		Column,
		Line,

		// Other:
		Reset,
	};

	static Bytestream& Stdout();
	static Bytestream& Stderr();
	static Bytestream& None();

	/**
	 * Retrieve the debug output stream or a do-nothing stream, based
	 * on the (hierarchical) debug naming scheme.
	 *
	 * Fabrique runs with a debug pattern (that defaults to "") that is
	 * used to select how much debug output is actually output.
	 * For instance, if running with --debug=*, all names will match,
	 * whereas if running with --debug=parser (equivalent to parser.*),
	 * Debug("parser.foo") will return the debug stream but
	 * Debug("lexer.bar") will return the do-nothing stream.
	 */
	static Bytestream& Debug(const std::string& name);
	static void SetDebugPattern(const std::string&);
	static void SetDebugStream(Bytestream&);

	/**
	 * Construct a formatted @ref fabrique::Bytestream to wrap an @ref std::ostream.
	 *
	 * The caller is responsible for freeing the returned pointer.
	 */
	static Bytestream* Formatted(std::ostream&);

	/**
	 * Construct a plain @ref fabrique::Bytestream to wrap an @ref std::ostream.
	 *
	 * The caller is responsible for freeing the returned pointer.
	 */
	static Bytestream* Plain(std::ostream&);


	virtual ~Bytestream() {}

	virtual Bytestream& operator << (enum Format) = 0;
	virtual Bytestream& operator << (const Printable&);
	virtual Bytestream& operator << (const std::string&);
	virtual Bytestream& operator << (char);
	virtual Bytestream& operator << (int);
	virtual Bytestream& operator << (unsigned long);

	std::ostream& raw() { return out_; }

protected:
	Bytestream(std::ostream& o)
		: out_(o)
	{
	}

	std::ostream& out_;
};

} // namespace fabrique

#endif // !BYTESTREAM_H
