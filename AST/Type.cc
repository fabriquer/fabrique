/** @file Type.cc    Definition of @ref Type. */
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

#include "Type.h"
#include "Support/ostream.h"

#include <sstream>


const Type& Type::GetSupertype(const Type& x, const Type& y)
{
	assert(x.isSupertype(y) or y.isSupertype(x));
	return (x.isSupertype(y) ? x : y);
}


Type* Type::Create(const std::string& name, const PtrVec<Type>& params)
{
	return new Type(name, params);
}


bool Type::operator == (const Type& t) const
{
	return t.isSupertype(*this) and t.isSubtype(*this);
}


const Type& Type::operator [] (size_t i) const
{
	assert(params.size() > i);
	return *params[i];
}

bool Type::isSubtype(const Type& t) const
{
	// for now, this is really easy...
	return (&t == this);
}


bool Type::isSupertype(const Type &t) const
{
	// for now, this is really easy...
	return (&t == this);
}

bool Type::isListOf(const Type& t) const
{
	if (typeName != "list")
		return false;

	assert(params.size() == 1);

	return (t == *params[0]);
}


std::string Type::str() const
{
	std::ostringstream oss;
	PrettyPrint(oss);
	return oss.str();
}


const std::string& Type::name() const { return typeName; }


void Type::PrettyPrint(std::ostream& out, int indent) const
{
	out << Blue << typeName;

	if (params.size() > 0)
	{
		out << Yellow << '[' << ResetAll;

		for (size_t i = 0; i < params.size(); )
		{
			out << *params[i];
			if (++i < params.size())
				out << Yellow << ", " << ResetAll;
		}

		out << Yellow << ']';
	}

	out << ResetAll;
}
