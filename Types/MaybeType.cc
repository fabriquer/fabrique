/** @file Types/MaybeType.cc    Definition of @ref fabrique::MaybeType. */
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
#include "Types/BooleanType.h"
#include "Types/MaybeType.h"
#include "Types/TypeContext.h"
#include <cassert>
using namespace fabrique;


static const char* Name = "maybe";


MaybeType::MaybeType(const Type& elementTy)
	: Type(Name, PtrVec<Type>(1, &elementTy), elementTy.context()),
	  elementType_(elementTy)
{
}


MaybeType::~MaybeType()
{
}


Type::TypeMap MaybeType::fields() const
{
	assert(typeParameters().size() == 1);

	return TypeMap {
		{ ast::MaybeExists,  context().booleanType() },
		{ ast::MaybeValue,   *typeParameters()[0] },
	};
}


bool MaybeType::isSubtype(const Type& other) const
{
	if (not other.isOptional())
		return false;

	auto& t = dynamic_cast<const MaybeType&>(other);
	assert(t.typeParamCount() == t.typeParamCount());

	// Maybes are covariant: maybe[subtype] is a subtype of maybe[super].
	if (elementType().isSubtype(t.elementType()))
		return true;

	return false;
}


RawMaybeType::RawMaybeType(TypeContext& ctx)
	: Type(Name, PtrVec<Type>(), ctx)
{
}


Type* RawMaybeType::Parameterise(const PtrVec<Type>& t, const SourceRange&) const
{
	assert(t.size() == 1);
	return new MaybeType(*t.front());
}
