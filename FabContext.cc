/** @file FabContext.h    Declaration of @ref FabContext. */
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
#include "FabContext.h"

using namespace fabrique;
using std::string;


const Type* FabContext::type(const string& name, const PtrVec<Type>& params)
{
	auto qualifiedName(std::make_pair(name, params));

	auto i = types.find(qualifiedName);
	if (i != types.end())
		return i->second.get();

	Type *t = Type::Create(name, params);
	types[qualifiedName].reset(t);
	return t;
}

const Type* FabContext::nilType()
{
	static const Type *nil = Type::Create("nil", PtrVec<Type>());
	return nil;
}

const Type* FabContext::fileType()
{
	return type("file");
}

const Type* FabContext::fileListType()
{
	return type("list", PtrVec<Type>(1, fileType()));
}


const FunctionType*
FabContext::functionType(const Type& in, const Type& out)
{
	PtrVec<Type> args;
	args.push_back(&in);

	return functionType(args, out);
}

const FunctionType*
FabContext::functionType(const PtrVec<Type>& argTypes, const Type& retType)
{
	return FunctionType::Create(argTypes, retType);
}
