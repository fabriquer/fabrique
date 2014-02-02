/** @file Primitive.cc    Definition of @ref Primitive. */
/*
 * Copyright (c) 2013-2014 Jonathan Anderson
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

#include "DAG/Primitive.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Types/Type.h"

using namespace fabrique::dag;
using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::string;


Boolean::Boolean(bool b, const Type& t, SourceRange loc)
	: Primitive(t, b, loc)
{
	// TODO: assert(t is a subtype of bool?)
}

shared_ptr<Value> Boolean::Negate(const SourceRange& loc) const
{
	return shared_ptr<Value>(new Boolean(not val, type(), loc));
}

shared_ptr<Value> Boolean::And(shared_ptr<Value>& v)
{
	auto other = dynamic_pointer_cast<Boolean>(v);
	assert(other);

	return shared_ptr<Value>(
		new Boolean(val and other->val,
			Type::GetSupertype(type(), other->type()),
			SourceRange(*this, *other))
	);
}

shared_ptr<Value> Boolean::Or(shared_ptr<Value>& v)
{
	auto other = dynamic_pointer_cast<Boolean>(v);
	assert(other);

	return shared_ptr<Value>(
		new Boolean(val or other->val,
			Type::GetSupertype(type(), other->type()),
			SourceRange(*this, *other))
	);
}

shared_ptr<Value> Boolean::Xor(shared_ptr<Value>& v)
{
	auto other = dynamic_pointer_cast<Boolean>(v);
	assert(other);

	return shared_ptr<Value>(
		new Boolean(val xor other->val,
			Type::GetSupertype(type(), other->type()),
			SourceRange(*this, *other))
	);
}

string Boolean::str() const { return val ? "true" : "false"; }


Integer::Integer(int i, const Type& t, SourceRange loc)
	: Primitive(t, i, loc)
{
}

string Integer::str() const { return std::to_string(val); }

shared_ptr<Value> Integer::Add(shared_ptr<Value>& v)
{
	SourceRange loc = SourceRange(*this, *v);

	shared_ptr<Integer> other = std::dynamic_pointer_cast<Integer>(v);
	if (not other)
		throw SemanticException(
			"integers can only add with integers", loc);

	return shared_ptr<Value>(
		new Integer(this->val + other->val, type(), loc));
}


String::String(string s, const Type& t, SourceRange loc)
	: Primitive(t, s, loc)
{
}

string String::str() const { return val; }

shared_ptr<Value> String::Add(shared_ptr<Value>& v)
{
	SourceRange loc = SourceRange(*this, *v);

	shared_ptr<String> other = std::dynamic_pointer_cast<String>(v);
	if (not other)
		throw SemanticException(
			"strings can only concatenate with strings", loc);

	return shared_ptr<Value>(
		new String(this->val + other->val, type(), loc));
}

void String::PrettyPrint(Bytestream& b, int indent) const
{
	b << Bytestream::Literal << "'" << str() << "'" << Bytestream::Reset;
}
