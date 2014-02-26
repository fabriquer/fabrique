/** @file List.h    Definition of @ref List. */
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

#include "DAG/List.h"
#include "Support/Bytestream.h"
#include "Support/Join.h"
#include "Support/SourceLocation.h"
#include "Support/exceptions.h"
#include "Types/Type.h"
#include "Types/TypeError.h"

#include <algorithm>
#include <cassert>

using namespace fabrique;
using namespace fabrique::dag;
using std::shared_ptr;
using std::string;
using std::vector;


List* List::of(const SharedPtrVec<Value>& values, const SourceRange& src)
{
	assert(not values.empty());
	const Type& t = Type::ListOf(values.front()->type());

	return new List(values, t, src);
}


List::List(const SharedPtrVec<Value>& v, const Type& t, const SourceRange& src)
	: Value(t, src), v(v), elementType(t[0])
{
	if (v.size() > 0)
		assert(t.isListOf(v.front()->type()));

	for (auto& value : v)
		assert(value);
}

List::iterator List::begin() const { return v.begin(); }
List::iterator List::end() const { return v.end(); }

size_t List::size() const { return v.size(); }

const Value& List::operator [] (size_t i) const
{
	return *v[i];
}


shared_ptr<Value> List::Add(shared_ptr<Value>& n)
{
	SourceRange loc = SourceRange::Over(this, n.get());

	shared_ptr<List> next = std::dynamic_pointer_cast<List>(n);
	if (not next)
		throw SemanticException(
			"lists can only be concatenated with lists", loc);

	if (not elementType.isSupertype(next->elementType)
	    and not next->elementType.isSupertype(elementType))
		throw SemanticException(
			"incompatible operands to concatenate (types "
			+ type().str() + ", " + next->type().str() + ")", loc);


	// The result type is the most general case (supertype).
	const Type& resultType =
		next->elementType.isSupertype(type())
			? next->type()
			: this->type();

	SharedPtrVec<Value> values(v);
	values.insert(values.end(), next->v.begin(), next->v.end());

	return shared_ptr<Value>(new List(values, resultType, loc));
}

shared_ptr<Value> List::PrefixWith(shared_ptr<Value>& prefix)
{
	if (prefix->type() != elementType)
		throw WrongTypeException(elementType,
		                         prefix->type(), prefix->source());

	SharedPtrVec<Value> values;
	values.push_back(prefix);
	values.insert(values.end(), v.begin(), v.end());

	return shared_ptr<Value>(
		new List(values, type(), SourceRange::Over(prefix.get(), this))
	);
}

shared_ptr<Value> List::ScalarAdd(shared_ptr<Value>& scalar)
{
	assert(type().isListOf(scalar->type()));

	SharedPtrVec<Value> values;
	for (const shared_ptr<Value>& v : this->v)
		values.push_back(v->Add(scalar));

	return shared_ptr<Value>(
		new List(values, type(), SourceRange::Over(this, scalar.get()))
	);
}

bool List::canScalarAdd(const Value& other)
{
	return type().isListOf(other.type());
}

void List::PrettyPrint(Bytestream& out, int indent) const
{
	out
		<< Bytestream::Operator << "[ "
		<< Bytestream::Reset
		;

	for (const shared_ptr<Value>& p : v)
		out << *p << " ";

	out
		<< Bytestream::Operator << "]"
		<< Bytestream::Reset
		;
}
