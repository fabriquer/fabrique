/** @file Support/exceptions.cc    Definition of basic Fabrique exceptions. */
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
#include "Support/ErrorReport.h"
#include "Support/exceptions.h"

using namespace fabrique;
using std::string;


AssertionFailure::AssertionFailure(const string& condition, const string& msg)
	: condition_(condition),
	  message_(msg.empty() ? ("Assertion failed: " + condition) : msg)
{
}

AssertionFailure::AssertionFailure(const AssertionFailure& orig)
	: condition_(orig.condition_), message_(orig.message_)
{
}

AssertionFailure::~AssertionFailure() {}

const char* AssertionFailure::what() const noexcept
{
	return message_.c_str();
}


OSError::OSError(const string& message, const string& description)
	: message_(message), description_(description),
	  completeMessage_(message + ": " + description)
{
}

OSError::OSError(const OSError& orig)
	: message_(orig.message_), description_(orig.description_),
	  completeMessage_(orig.completeMessage_)
{
}

OSError::~OSError() {}

void OSError::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out
		<< Bytestream::Error << "OS error"
		<< Bytestream::Reset << ": " << message_ << ": "
		<< Bytestream::ErrorMessage << description_
		<< Bytestream::Reset
		;
}


UserError::UserError(const string& message)
	: message_(message)
{
}

UserError::UserError(const UserError& orig)
	: message_(orig.message_)
{
}

UserError::~UserError() {}

void UserError::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out << Bytestream::ErrorMessage << message_ << Bytestream::Reset;
}


SourceCodeException::SourceCodeException(const string& m, const SourceRange& l)
	: HasSource(l), err_(ErrorReport::Create(m, l))
{
}

SourceCodeException::SourceCodeException(const SourceCodeException& orig)
	: HasSource(orig.source()), err_(orig.err_)
{
}

SourceCodeException::~SourceCodeException()
{
}

const string& SourceCodeException::message() const
{
	return err_->getMessage();
}

const char* SourceCodeException::what() const noexcept
{
	return message().c_str();
}

void SourceCodeException::PrettyPrint(Bytestream& out, size_t indent) const
{
	err_->PrettyPrint(out, indent);
}


SyntaxError::SyntaxError(const string& message, const SourceRange& loc)
	: SourceCodeException(message, loc)
{
}

SyntaxError::SyntaxError(const SyntaxError& orig)
	: SourceCodeException(orig.message(), orig.source())
{
}

SyntaxError::~SyntaxError() {}

SemanticException::SemanticException(const string& m, const SourceRange& loc)
	: SourceCodeException(m, loc)
{
}

SemanticException::~SemanticException() {}
