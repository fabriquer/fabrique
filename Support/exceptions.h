/** @file exceptions.h    Declaration of basic Fabrique exceptions. */
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

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "Support/Printable.h"
#include "Support/SourceLocation.h"

#include <exception>
#include <memory>
#include <string>


namespace fabrique {


//! Some code may choose to throw this exception rather than assert() out.
class AssertionFailure : public std::exception
{
public:
	AssertionFailure(const std::string& condition,
	                 const std::string& message = "");

	const char* what() const noexcept;
	const std::string& condition() const noexcept { return cond; }
	const std::string& message() const noexcept { return msg; }

private:
	const std::string cond;
	const std::string msg;
};

#ifdef NDEBUG
#define FabAssert()
#else
#define FabAssert(condition, ...) \
	if (not condition) \
		throw AssertionFailure(#condition __VA_ARGS__)
#endif


//! An unexpected duplicate was encountered.
class DuplicateException : public std::exception
{
public:
	DuplicateException(const std::string& kind, const std::string& name);
	const char* what() const noexcept;

private:
	const std::string message;
	const std::string kind;
	const std::string name;
};


class ErrorReport;

//! Base class for exceptions related to invalid source code.
class SourceCodeException
	: public std::exception, public HasSource, public Printable
{
public:
	const std::string& message() const;
	virtual const char* what() const noexcept;

	virtual void PrettyPrint(Bytestream&, int indent = 0) const;

protected:
	SourceCodeException(const std::string& message, const SourceRange&);

private:
	std::shared_ptr<ErrorReport> err;
};


//! A syntactic error is present in the Fabrique description.
class SyntaxError : public SourceCodeException
{
public:
	SyntaxError(const std::string& message, const SourceRange&);
};

//! A semantic error is present in the Fabrique description.
class SemanticException : public SourceCodeException
{
public:
	SemanticException(const std::string& message, const SourceRange&);
};

}

#endif
