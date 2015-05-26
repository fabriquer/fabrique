/** @file DAG/Record.h    Declaration of @ref fabrique::dag::Record. */
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

#include "AST/Builtins.h"
#include "DAG/Record.h"
#include "DAG/Visitor.h"
#include "Support/Bytestream.h"
#include "Types/RecordType.h"
#include "Types/Type.h"

#include <cassert>

using namespace fabrique::dag;
using std::string;
using std::vector;


Record* Record::Create(const ValueMap& fields, const Type& t, SourceRange src)
{
	assert(fields.size() >= t.fields().size());
	const RecordType::TypeMap typeFields = t.fields();
	for (auto value : fields)
	{
		const string name = value.first;
		if (name != ast::Arguments and name != ast::BuildDirectory
		    and name != ast::Subdirectory)
			assert(typeFields.find(name) != typeFields.end());

		assert(value.second);
	}

	if (not src and not fields.empty())
	{
		src = SourceRange::Over(fields);
	}

	return new Record(fields, t, src);
}


Record* Record::Create(const ValueMap& fields, SourceRange src)
{
	RecordType::NamedTypeVec types;
	for (auto& v : fields)
		types.emplace_back(v.first, v.second->type());

	assert(not fields.empty());
	TypeContext& ctx = fields.begin()->second->type().context();
	const Type& type = *RecordType::Create(types, ctx);

	return Create(fields, type, src);
}


Record::Record(const ValueMap& fields, const Type& t, SourceRange src)
	: Value(t, src), fields_(fields)
{
}


Record::~Record() {}


ValuePtr Record::field(const std::string& name) const
{
	for (auto& i : fields_)
	{
		if (i.first == name)
			return i.second;
	}

	return ValuePtr();
}


void Record::PrettyPrint(Bytestream& out, size_t indent) const
{
	const string tab(indent, '\t');
	const string innerTab(indent + 1, '\t');

	out << Bytestream::Operator << "{\n"
		;

	for (auto& i : fields_)
	{
		out
			<< innerTab
			<< Bytestream::Definition << i.first
			<< Bytestream::Operator << ":"
			<< Bytestream::Reset << i.second->type()
			<< Bytestream::Operator << " = "
			;

		i.second->PrettyPrint(out, indent + 1);

		out
			<< Bytestream::Reset << "\n"
			;
	}

	out
		<< Bytestream::Operator << tab << "}"
		<< Bytestream::Reset
		;
}


void Record::Accept(Visitor& v) const
{
	if (v.Visit(*this))
		for (auto& i : fields_)
			i.second->Accept(v);
}
