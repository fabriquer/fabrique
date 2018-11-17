/** @file DAG/Primitive.cc    Definition of @ref fabrique::dag::Primitive. */
/*
 * Copyright (c) 2013-2014, 2018 Jonathan Anderson
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

#include <fabrique/names.hh>
#include <fabrique/dag/constants.hh>
#include <fabrique/dag/Primitive.hh>
#include <fabrique/dag/Visitor.hh>
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include <fabrique/types/TypeContext.hh>
#include <fabrique/types/TypeError.hh>

#include <cassert>

using namespace fabrique::dag;
using std::dynamic_pointer_cast;
using std::shared_ptr;
using std::string;


Boolean::Boolean(bool b, const Type& t, SourceRange loc)
	: Primitive(t, b, loc)
{
#ifndef NDEBUG
	SemaCheck(t.isSubtype(t.context().booleanType()), loc, "not boolean type");
#endif
}

ValuePtr Boolean::Not(const SourceRange& loc) const
{
	return ValuePtr(new Boolean(not value_, type(), loc));
}

ValuePtr Boolean::And(ValuePtr& v, SourceRange src) const
{
	return Op("and", [](bool x, bool y) { return x and y; }, v, src);
}

ValuePtr Boolean::Or(ValuePtr& v, SourceRange src) const
{
	return Op("or", [](bool x, bool y) { return x or y; }, v, src);
}

ValuePtr Boolean::Xor(ValuePtr& v, SourceRange src) const
{
	return Op("xor", [](bool x, bool y) { return x xor y; }, v, src);
}

ValuePtr Boolean::Equals(ValuePtr& v, SourceRange src) const
{
	return Op("check equality", [](bool x, bool y) { return x == y; }, v, src);
}

ValuePtr Boolean::Op(string name, std::function<bool (bool, bool)> f,
                     ValuePtr v, SourceRange src) const
{
	if (not src)
	{
		src = SourceRange(*this, *v);
	}

	auto other = dynamic_pointer_cast<Boolean>(v);
	SemaCheck(other, src, "cannot " + name + " boolean with " + v->type().str());

	bool value = f(this->value_, other->value_);
	return ValuePtr(new Boolean(value, type().context().booleanType(), src));
}

string Boolean::str() const { return value_ ? names::True : names::False; }

void Boolean::Accept(Visitor& v) const
{
	v.Visit(*this);
}


Integer::Integer(int i, const Type& t, SourceRange loc)
	: Primitive(t, i, loc)
{
}

string Integer::str() const { return std::to_string(value_); }

ValuePtr Integer::Negate(const SourceRange& src) const
{
	return ValuePtr(
		new Integer(-this->value_, type(), SourceRange(src, source())));
}

ValuePtr Integer::Add(ValuePtr& v, SourceRange src) const
{
	return Op("add", [](int x, int y) { return x + y; }, v, src);
}

ValuePtr Integer::DivideBy(ValuePtr& v, SourceRange src) const
{
	if (not src)
	{
		src = SourceRange(*this, *v);
	}

	auto f = [src](int x, int y)
	{
		SemaCheck(y != 0, src, "division by zero");
		return x / y;
	};

	return Op("divide", f, v, src);
}

ValuePtr Integer::MultiplyBy(ValuePtr& v, SourceRange src) const
{
	return Op("multiply", [](int x, int y) { return x * y; }, v, src);
}

ValuePtr Integer::Subtract(ValuePtr& v, SourceRange src) const
{
	return Op("subtract", [](int x, int y) { return x - y; }, v, src);
}

ValuePtr Integer::Equals(ValuePtr& v, SourceRange src) const
{
	if (not src)
	{
		src = SourceRange(*this, *v);
	}

	auto other = dynamic_pointer_cast<Integer>(v);
	SemaCheck(other, v->source(), "not an integer");

	bool value = (this->value_ == other->value_);
	return ValuePtr(new Boolean(value, type().context().booleanType(), src));
}

ValuePtr Integer::Op(string name, std::function<int (int, int)> f, ValuePtr v,
                     SourceRange src) const
{
	if (not src)
	{
		src = SourceRange(*this, *v);
	}

	auto other = dynamic_pointer_cast<Integer>(v);
	SemaCheck(other, src, "cannot " + name + " integer with " + v->type().str());

	bool value = f(this->value_, other->value_);
	return ValuePtr(new Integer(value, type().context().booleanType(), src));
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

ValuePtr String::Add(ValuePtr& v, SourceRange src) const
{
	SourceRange loc = src ? src : SourceRange(*this, *v);

	shared_ptr<String> other = std::dynamic_pointer_cast<String>(v);
	SemaCheck(other, src, "cannot add string with " + v->type().str());

	return ValuePtr(
		new String(this->value_ + other->value_, type(), loc));
}

ValuePtr String::Equals(ValuePtr& v, SourceRange src) const
{
	SourceRange loc = src ? src : SourceRange(*this, *v);

	shared_ptr<String> other = std::dynamic_pointer_cast<String>(v);
	SemaCheck(other, loc, "cannot check string equality with " + v->type().str());

	// Don't trust std::string::compare, it thinks "foo" != "foo\0".
	const char *x = this->value_.data();
	const size_t len = strnlen(x, MaxStringLength);
	SemaCheck(len < MaxStringLength, source(), "string too long");

	const char *y = other->value_.data();
	const bool equal = (strncmp(x, y, len) == 0);

	return ValuePtr(
		new Boolean(equal, type().context().booleanType(), loc));
}

void String::PrettyPrint(Bytestream& b, unsigned int /*indent*/) const
{
	b << Bytestream::Literal << "'" << str() << "'" << Bytestream::Reset;
}

void String::Accept(Visitor& v) const
{
	v.Visit(*this);
}
