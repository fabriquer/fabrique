/** @file DAG/Primitive.cc    Definition of @ref fabrique::dag::Primitive. */
/*
 * Copyright (c) 2013-2014, 2016 Jonathan Anderson
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

#include "DAG/constants.h"
#include "DAG/Primitive.h"
#include "DAG/Visitor.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Types/Type.h"
#include "Types/TypeContext.h"
#include "Types/TypeError.h"

#include <cassert>

using namespace fabrique::dag;
using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::string;


Boolean::Boolean(bool b, const BooleanType& t, SourceRange loc)
	: Primitive(t, b, loc)
{
	// TODO: assert(t is a subtype of bool?)
}

const fabrique::BooleanType& Boolean::type() const
{
	return dynamic_cast<const BooleanType&>(Primitive::type());
}

ValuePtr Boolean::Negate(const SourceRange& loc) const
{
	return ValuePtr(new Boolean(not value_, type(), loc));
}

ValuePtr Boolean::And(ValuePtr& v) const
{
	auto b = dynamic_pointer_cast<Boolean>(v);
	assert(b);

	return ValuePtr(
		new Boolean(value_ and b->value_, type(), SourceRange(*this, *b))
	);
}

ValuePtr Boolean::Or(ValuePtr& v) const
{
	auto other = dynamic_pointer_cast<Boolean>(v);
	assert(other);

	return ValuePtr(
		new Boolean(value_ or other->value_, type(), SourceRange(*this, *other)));
}

ValuePtr Boolean::Xor(ValuePtr& v) const
{
	auto other = dynamic_pointer_cast<Boolean>(v);
	assert(other);

	return ValuePtr(
		new Boolean(value_ xor other->value_, type(), SourceRange(*this, *other)));
}

ValuePtr Boolean::Equals(ValuePtr& v) const
{
	auto other = dynamic_pointer_cast<Boolean>(v);
	assert(other);

	return ValuePtr(
		new Boolean(value_ == other->value_, type(), SourceRange(*this, *other)));
}

string Boolean::str() const { return value_ ? "true" : "false"; }

void Boolean::Accept(Visitor& v) const
{
	v.Visit(*this);
}


Integer::Integer(int i, const IntegerType& t, SourceRange loc)
	: Primitive(t, i, loc)
{
}

const fabrique::IntegerType& Integer::type() const
{
	return dynamic_cast<const IntegerType&>(Primitive::type());
}

string Integer::str() const { return std::to_string(value_); }

ValuePtr Integer::Add(ValuePtr& v) const
{
	SourceRange loc = SourceRange(*this, *v);

	shared_ptr<Integer> other = std::dynamic_pointer_cast<Integer>(v);
	if (not other)
		throw WrongTypeException("int", v->type(), v->source());

	return ValuePtr(
		new Integer(this->value_ + other->value_, type(), loc));
}

ValuePtr Integer::Equals(ValuePtr& v) const
{
	SourceRange loc = SourceRange(*this, *v);

	shared_ptr<Integer> other = std::dynamic_pointer_cast<Integer>(v);
	if (not other)
		throw WrongTypeException("int", v->type(), v->source());

	const bool eq = this->value_ == other->value_;
	const BooleanType& boolTy = type().context().booleanType();
	return ValuePtr(new Boolean(eq, boolTy, loc));
}

void Integer::Accept(Visitor& v) const
{
	v.Visit(*this);
}


String::String(string s, const Type& t, SourceRange loc)
	: Primitive(t, s, loc)
{
}

string String::str() const { return value_; }

ValuePtr String::Add(ValuePtr& v) const
{
	SourceRange loc = SourceRange(*this, *v);

	shared_ptr<String> other = std::dynamic_pointer_cast<String>(v);
	if (not other)
		throw WrongTypeException("string", v->type(), loc);

	return ValuePtr(
		new String(this->value_ + other->value_, type(), loc));
}

ValuePtr String::Equals(ValuePtr& v) const
{
	SourceRange loc = SourceRange(*this, *v);

	shared_ptr<String> other = std::dynamic_pointer_cast<String>(v);
	if (not other)
		throw WrongTypeException("string", v->type(), loc);

	// Don't trust std::string::compare, it thinks "foo" != "foo\0".
	const char *x = this->value_.data();
	const size_t len = strnlen(x, MaxStringLength);
	if (len == MaxStringLength)
		throw SemanticException("string too long", source());

	const char *y = other->value_.data();
	const bool equal = (strncmp(x, y, len) == 0);

	return ValuePtr(
		new Boolean(equal, type().context().booleanType(), loc));
}

void String::PrettyPrint(Bytestream& b, size_t /*indent*/) const
{
	b << Bytestream::Literal << "'" << str() << "'" << Bytestream::Reset;
}

void String::Accept(Visitor& v) const
{
	v.Visit(*this);
}
