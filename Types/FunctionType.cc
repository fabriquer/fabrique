/** @file Types/FunctionType.cc    Definition of @ref fabrique::FunctionType. */
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

#include "Types/FunctionType.h"
#include "Support/Bytestream.h"
#include "Support/Join.h"

#include <memory>

using namespace fabrique;


FunctionType*
FunctionType::Create(const PtrVec<Type>& parameterTypes, const Type& retTy)
{
	PtrVec<Type> signature(parameterTypes);
	signature.push_back(&retTy);

	return new FunctionType(parameterTypes, retTy, signature);
}


const std::string FunctionType::name() const
{
	static const char* name = "function";
	return name;
}


bool FunctionType::isSubtype(const Type& other) const
{
	if (not other.isFunction())
		return false;

	auto& t = dynamic_cast<const FunctionType&>(other);

	//
	// Functions are covariant in their return types
	// and contravariant in their argument types.
	//
	// x:(special_int)=>special_int = ...
	// y:(special_int)=>int = x        # this is ok
	// z:(int)=>special_int = x        # this is not ok
	//

	if (t.typeParameters().size() != typeParameters().size())
		return false;

	for (size_t i = 0; i < typeParameters().size(); i++)
	{
		const Type& mine = *typeParameters()[i];
		const Type& theirs = *t.typeParameters()[i];

		if (not theirs.isSubtype(mine))
			return false;
	}

	return retTy_.isSubtype(t.retTy_);
}


void FunctionType::PrettyPrint(Bytestream& out, unsigned int /*indent*/) const
{
	out
		<< Bytestream::Operator << "("
		<< Join<Type>(",", paramTypes_)    // don't use csv with spaces
		<< Bytestream::Operator << ")=>"
		<< retTy_
		<< Bytestream::Reset
		;
}
