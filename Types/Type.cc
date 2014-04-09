/** @file Types/Type.cc    Definition of @ref fabrique::Type. */
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

#include "Support/Bytestream.h"
#include "Types/Type.h"
#include "FabContext.h"

#include <cassert>

using namespace fabrique;


const Type& Type::ListOf(const Type& t)
{
	PtrVec<Type> types(1, &t);
	return *t.parent_.type("list", types);
}


Type::Type(const std::string& name, const PtrVec<Type>& params,
           FabContext& parent)
	: parent_(parent), typeName_(name), parameters_(params)
{
	assert(not typeName_.empty());
}

Type::Type(const std::string& name, const PtrVec<Type>& params, const Type& t)
	: parent_(t.parent_), typeName_(name), parameters_(params)
{
	assert(not typeName_.empty());
}


const Type& Type::GetSupertype(const Type& x, const Type& y)
{
	assert(x.isSupertype(y) or y.isSupertype(x));
	return (x.isSupertype(y) ? x : y);
}


Type* Type::Create(const std::string& name, const PtrVec<Type>& params,
                   FabContext& ctx)
{
	return new Type(name, params, ctx);
}


bool Type::operator == (const Type& t) const
{
	return t.isSupertype(*this) and t.isSubtype(*this);
}


const Type& Type::operator [] (size_t i) const
{
	assert(parameters_.size() > i);
	return *parameters_[i];
}

bool Type::isSubtype(const Type& t) const
{
	// TODO: drop this dirty hack: can pass file to a file[in] parameter
	if (typeName_ == "file" and t.typeName_ == "file")
		return true;

	// TODO: drop this dirty hack: list[nil] is a subtype of list[*]
	if (typeName_ == "list" and t.typeName_ == typeName_)
	{
		assert(t.parameters_.size() == 1);
		if (parameters_.front()->typeName_ == "nil")
			return true;
	}

	// for now, this is really easy...
	return (&t == this);
}


bool Type::isSupertype(const Type &t) const
{
	return t.isSubtype(*this);
}

bool Type::isFile() const
{
	return (typeName_ == "file");
}

bool Type::isListOf(const Type& t) const
{
	if (typeName_ != "list")
		return false;

	assert(parameters_.size() == 1);

	return (t == *parameters_[0]);
}


const std::string Type::name() const { return typeName_; }


void Type::PrettyPrint(Bytestream& out, size_t indent) const
{
	out << Bytestream::Type << typeName_;

	if (parameters_.size() > 0)
	{
		out
			<< Bytestream::Operator << "["
			<< Bytestream::Reset;

		for (size_t i = 0; i < parameters_.size(); )
		{
			parameters_[i]->PrettyPrint(out, indent);
			if (++i < parameters_.size())
				out
					<< Bytestream::Operator << ", "
					<< Bytestream::Reset;
		}

		out << Bytestream::Operator << "]";
	}

	out << Bytestream::Reset;
}
