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
#include "Support/Location.h"

#include <algorithm>

using namespace fabrique;
using namespace fabrique::dag;
using std::shared_ptr;
using std::string;
using std::vector;


SourceRange range(const vector<shared_ptr<Value>>& v)
{
	if (v.empty())
		return SourceRange::None();

	return SourceRange::Over(v.begin()->get(), (--v.end())->get());
}


List::List()
	: Value(SourceRange::None())
{
}

List::List(const vector<shared_ptr<Value>>& v)
	: Value(range(v)), v(v)
{
}

List::iterator List::begin() const { return v.begin(); }
List::iterator List::end() const { return v.end(); }

size_t List::size() const { return v.size(); }

const shared_ptr<Value>& List::operator [] (size_t i) const
{
	return v[i];
}

string List::type() const
{
	string typeParam;
	if (not v.empty())
		typeParam = v.front()->type();

	return "list[" + typeParam + "]";
}

string List::str() const
{
	vector<string> substrings;
#if 0
	std::transform(v.begin(), v.end(), substrings.end(),
	               [](const shared_ptr<Value>& v) { return v->str(); });
#endif

	return "[ " + join(substrings, " ") + "]";
}

void List::PrettyPrint(Bytestream& out, int indent) const
{
	out << Bytestream::Operator << "[ ";

	for (const shared_ptr<Value>& p : v)
		out << *p << " ";

	out << Bytestream::Operator << "]";

}
