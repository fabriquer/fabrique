/** @file Types/TypeError.cc    Definition of @ref fabrique::TypeError. */
/*
 * Copyright (c) 2014 Jonathan Anderson
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

#include <fabrique/types/Type.hh>
#include <fabrique/types/TypeError.hh>

using namespace fabrique;
using std::string;


TypeError::TypeError(const string& message, const SourceRange& src)
	: SemanticException("type error: " + message, src)
{
}

TypeError::TypeError(const TypeError& orig)
	: SemanticException(orig.message(), orig.source())
{
}

TypeError::~TypeError()
{
}

WrongTypeException::WrongTypeException(const Type& want, const Type& got,
                                       const SourceRange& src)
	: WrongTypeException(want.str(), got.str(), src)
{
}

WrongTypeException::WrongTypeException(const string& want, const Type& got,
                                       const SourceRange& src)
	: WrongTypeException(want, got.str(), src)
{
}

WrongTypeException::WrongTypeException(const string& want, const string& got,
                                       const SourceRange& src)
	: TypeError("expected " + want + ", got " + got, src)
{
}

WrongTypeException::WrongTypeException(const WrongTypeException& orig)
	: TypeError(orig.message(), orig.source())
{
}

WrongTypeException::~WrongTypeException()
{
}
