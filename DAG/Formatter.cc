/** @file DAG/Formatter.cc    Definition of @ref fabrique::dag::Formatter. */
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

#include "DAG/Formatter.h"
#include "DAG/Value.h"

#include <cassert>

using namespace fabrique::dag;
using std::string;


string Formatter::Format(const Value& v)
{
	v.Accept(*this);
	assert(not values_.empty());

	string value = values_.top();
	values_.pop();

	return value;
}

#define FORMAT_VISIT(T) \
	bool Formatter::Visit(const T& x) \
	{ \
		values_.push(Format(x)); \
		return false; \
	}

FORMAT_VISIT(Boolean)
FORMAT_VISIT(Build)
FORMAT_VISIT(File)
FORMAT_VISIT(Function)
FORMAT_VISIT(Integer)
FORMAT_VISIT(List)
FORMAT_VISIT(Rule)
FORMAT_VISIT(String)
FORMAT_VISIT(Structure)
FORMAT_VISIT(Target)
