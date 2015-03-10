/** @file Types/RecordType.cc    Definition of @ref fabrique::RecordType. */
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

#include "Support/Bytestream.h"
#include "Support/Join.h"
#include "Types/RecordType.h"
#include "Types/TypeContext.h"

#include <cassert>

using namespace fabrique;
using std::string;
using std::vector;


RecordType*
RecordType::Create(const NamedTypeVec& fields, TypeContext& ctx)
{
	StringMap<const Type&> types;
	vector<string> names;

	for (auto& i : fields)
	{
		const string& name = i.first;
		const Type& type = i.second;

		names.push_back(name);
		types.emplace(name, type);
	}

	return new RecordType(types, names, ctx);
}


RecordType::RecordType(const StringMap<const Type&>& fieldTypes,
                             const vector<string>& fieldNames, TypeContext& ctx)
	: Type("struct", PtrVec<Type>(), ctx),
	  fieldTypes_(fieldTypes), fieldNames_(fieldNames)
{
#ifndef NDEBUG
	for (const string& name : fieldNames_)
		assert(fieldTypes_.find(name) != fieldTypes_.end());
#endif
}

RecordType::~RecordType()
{
}


bool RecordType::isSubtype(const Type& t) const
{
	if (t.name() != name())
		return false;

	const RecordType *rt = dynamic_cast<const RecordType*>(&t);
	if (not rt)
		return false;

	size_t fieldsChecked = 0;
	for (auto& theirField : rt->fields())
	{
		const string& name = theirField.first;
		const Type& theirType = theirField.second;

		auto i = fieldTypes_.find(name);
		if (i == fieldTypes_.end())
			return false;

		//
		// Records are covariant: you can assign
		// struct[foo:special_int] to struct[foo:int]
		// but not the other way around.
		//
		const Type& ourType = i->second;
		if (not theirType.isSubtype(ourType))
			return false;

		++fieldsChecked;
	}

	return true;
}


const Type& RecordType::supertype(const Type& t) const
{
	if (this->isSupertype(t))
		return *this;

	if (t.isSupertype(*this))
		return t;

	TypeContext& ctx = context();

	if (t.name() != name())
		return ctx.nilType();

	const RecordType *rt = dynamic_cast<const RecordType*>(&t);
	if (not rt)
		return ctx.nilType();

	NamedTypeVec commonFields;

	for (auto& field : fields())
	{
		const string& name = field.first;
		const Type& fieldType = field.second;

		auto i = rt->fieldTypes_.find(name);
		if (i == rt->fieldTypes_.end())
			continue;

		assert(i->first == name);
		assert(i->second);

		const Type& supertype = fieldType.supertype(i->second);
		if (supertype)
			commonFields.emplace_back(name, supertype);
	}

	return ctx.recordType(commonFields);
}

void RecordType::PrettyPrint(Bytestream& out, size_t /*indent*/) const
{
	out << Bytestream::Type << "struct" << Bytestream::Reset;

	if (fieldNames_.empty())
		return;

	out << Bytestream::Operator << '[';

	bool first = true;
	for (const string& name : fieldNames_)
	{
		if (not first)
			out << Bytestream::Operator << ", "
			;

		first = false;

		auto i = fieldTypes_.find(name);
		assert(i != fieldTypes_.end());

		const Type& type = i->second;
		out
			<< Bytestream::Definition << name
			<< Bytestream::Operator << ':'
			<< type
			;
	}

	out << Bytestream::Operator << ']' << Bytestream::Reset;
}
