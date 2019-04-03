/** @file Types/SequenceType.cc    Definition of @ref fabrique::SequenceType. */
/*
 * Copyright (c) 2014, 2018 Jonathan Anderson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme and at Memorial University
 * of Newfoundland under the NSERC Discovery program (RGPIN-2015-06048).
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

#include <fabrique/types/SequenceType.hh>
#include <fabrique/types/TypeContext.hh>
#include "Support/exceptions.h"

#include <cassert>

using namespace fabrique;


static const char* Name = "list";


SequenceType::SequenceType(TypeContext &ctx)
	: Type(Name, {}, ctx)
{
}

SequenceType::SequenceType(const Type &elementType)
	: Type(Name, { &elementType }, elementType.context())
{
}

const Type* SequenceType::elementType() const
{
	FAB_ASSERT(typeParameters().size() < 2, "too many type parameters in " + str());

	if (typeParameters().empty())
	{
		return nullptr;
	}
	else
	{
		return &(*this)[0];
	}
}

bool SequenceType::hasFiles() const
{
	for (const Type *t : typeParameters())
	{
		if (t->hasFiles())
			return true;
	}

	return false;
}


bool SequenceType::hasOutput() const
{
	for (const Type *t : typeParameters())
	{
		if (t->hasOutput())
			return true;
	}

	return false;
}


const Type& SequenceType::supertype(const Type& other) const
{
	if (not other.isOrdered())
	{
		return Type::supertype(other);
	}

	FAB_ASSERT(other.typeParameters().size() < 2,
		"too many type parameters in " + other.str());

	return typeParameters().empty() ? other : context().listOf(other[0]);
}

bool SequenceType::isSubtype(const Type& other) const
{
	if (not other.isOrdered())
		return false;

	FAB_ASSERT(typeParameters().size() < 2, "too many type parameters in " + str());

	// Sequences are covariant: list[subtype] is a subtype of list[super].
	// Also, an empty list is a subtype of any list type.
	return typeParameters().empty() or (
		not other.typeParameters().empty() and other[0].isSupertype((*this)[0])
	);
}


const Type& SequenceType::onAddTo(const Type& t) const
{
	if (isSupertype(t))
		return *this;

	if (t.isSupertype(*this))
		return t;

	return context().nilType();
}

const Type& SequenceType::onPrefixWith(const Type& t) const
{
	return typeParameters().empty()
		? context().listOf(t)
		: t.supertype((*this)[0])
		;
}

Type* SequenceType::Parameterise(const PtrVec<Type>& t, const SourceRange &src) const
{
	SemaCheck(typeParameters().empty(), src, str() + " already has a type parameter");
	SemaCheck(t.size() == 1, src, "sequences only take one type parameter");

	return new SequenceType(*t.front());
}
