/** @file Types/Type.cc    Definition of @ref fabrique::Type. */
/*
 * Copyright (c) 2013-2015 Jonathan Anderson
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
#include "Support/SourceLocation.h"
#include "Types/Type.h"
#include "Types/TypeContext.h"
#include "Types/TypeError.h"

#include <cassert>

using namespace fabrique;
using std::string;


const Type& Type::ListOf(const Type& t, const SourceRange& src)
{
	return t.parent_.listOf(t, src);
}


string Type::UntypedPart(string name)
{
	const size_t i = name.find(':');
	if (i == string::npos)
		return name;

	return name.substr(0, i);
}


Type::Type(const std::string& name, const PtrVec<Type>& params,
           TypeContext& parent)
	: parent_(parent), typeName_(name), parameters_(params)
{
	assert(not typeName_.empty());
}


bool Type::isSupertype(const Type& t) const
{
	return t.isSubtype(*this);
}


void Type::CheckSubtype(const Type& t, SourceRange src) const
{
	if (not this->isSubtype(t))
		throw WrongTypeException(t, *this, src);
}


Type::operator bool() const
{
	assert(this);
	return this->valid();
}


const Type& Type::supertype(const Type& other) const
{
	if (this->isSupertype(other))
		return *this;

	if (other.isSupertype(*this))
		return other;

	return parent_.nilType();
}


const Type& Type::lookupType() const
{
	assert(not isType());
	return *this;
}

const Type& Type::onAddTo(const Type& t) const { return t.parent_.nilType(); }

const Type& Type::onPrefixWith(const Type& t) const { return t.parent_.nilType(); }


Type* Type::Create(const std::string& name, const PtrVec<Type>& params,
                   TypeContext& ctx)
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

Type* Type::Parameterise(const PtrVec<Type>&, const SourceRange&) const
{
	assert(false && "called Type::Parameterise()");
	return nullptr;
}

const Type& Type::Map(TypesMapper convert, const SourceRange& src) const
{
	PtrVec<Type> newTypeParams = convert(parameters_);
	return parent_.find(typeName_, src, newTypeParams);
}


bool Type::isSubtype(const Type& t) const
{
	// The default implementation of this is really easy.
	return (&t == this);
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
