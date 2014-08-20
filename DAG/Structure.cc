/** @file DAG/Structure.h    Declaration of @ref fabrique::dag::Structure. */
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

#include "DAG/Structure.h"
#include "DAG/Visitor.h"
#include "Support/Bytestream.h"
#include "Types/Type.h"

#include <cassert>

using namespace fabrique::dag;
using std::string;
using std::vector;


Structure* Structure::Create(vector<NamedValue>& values, const Type& t, SourceRange src)
{
	if (not src and not values.empty())
	{
		SourceRange begin = values.front().second->source();
		SourceRange end = (--values.end())->second->source();

		src = SourceRange(begin, end);
	}

	return new Structure(values, t, src);
}


Structure::Structure(vector<NamedValue>& values, const Type& t, SourceRange src)
	: Value(t, src), values_(values)
{
}


Structure::~Structure() {}


ValuePtr Structure::field(const std::string& name) const
{
	for (auto& i : values_)
	{
		if (i.first == name)
			return i.second;
	}

	return ValuePtr();
}


void Structure::PrettyPrint(Bytestream& out, size_t indent) const
{
	const string tab(indent, '\t');
	const string innerTab(indent + 1, '\t');

	out << Bytestream::Operator << "{\n"
		;

	for (auto& i : values_)
	{
		out
			<< innerTab
			<< i.second->type() << " "
			<< Bytestream::Definition << i.first
			<< Bytestream::Operator << " = "
			;

		i.second->PrettyPrint(out, indent + 1);

		out
			<< Bytestream::Reset << "\n"
			;
	}

	out << Bytestream::Operator << tab << "}";
}


void Structure::Accept(Visitor& v) const
{
	if (v.Visit(*this))
		for (auto& i : values_)
			i.second->Accept(v);
}
