/** @file TypeContext.cc    Definition of fabrique::TypeContext. */
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

#include "Types/BooleanType.h"
#include "Types/FileType.h"
#include "Types/FunctionType.h"
#include "Types/IntegerType.h"
#include "Types/MaybeType.h"
#include "Types/RecordType.h"
#include "Types/SequenceType.h"
#include "Types/StringType.h"
#include "Types/UserType.h"
#include "Support/Bytestream.h"
#include "Support/SourceLocation.h"
#include "TypeContext.h"

#include <cassert>

using namespace fabrique;
using std::string;


namespace
{
	class NilType : public Type
	{
	public:
		NilType(TypeContext& ctx)
			: Type("nil", PtrVec<Type>(), ctx)
		{
		}

		virtual bool valid() const override { return false; }
		virtual bool isSubtype(const Type&) const override { return true; }
	};
}


TypeContext::TypeContext()
{
	booleanType();
	fileType();
	inputFileType();
	integerType();
	outputFileType();
	stringType();

	// Bare types required to build list[foo], maybe[foo], etc.
	Register(rawMaybeType_ = new RawMaybeType(*this));
	Register(rawSequenceType_ = new RawSequenceType(*this));
}


const Type& TypeContext::find(const string& name, const PtrVec<Type>& params)
{
	auto i = types.find(QualifiedName(name, params));
	if (i != types.end())
		return *i->second.get();

	if (not params.empty())
	{
		i = types.find(QualifiedName(name));
		if (i != types.end())
		{
			const Type& unparam = *i->second;
			Type *parameterised = unparam.Parameterise(params, 
			                                           SourceRange::None());

			if (parameterised and *parameterised)
				Register(parameterised);

			return *parameterised;
		}
	}

	return nilType();
}

const Type& TypeContext::nilType()
{
	static const Type& nil = *new NilType(*this);
	return nil;
}

const Type& TypeContext::booleanType()
{
	static const Type& t = Register(
		new Type("bool", PtrVec<Type>(), *this));

	return t;
}

const Type& TypeContext::integerType()
{
	static const Type& t = Register(new IntegerType(*this));
	return t;
}

const Type& TypeContext::listOf(const Type& elementTy, const SourceRange&)
{
	PtrVec<Type> params(1, &elementTy);
	return find(rawSequenceType_->name(), params);
}

const Type& TypeContext::maybe(const Type& elementTy, const SourceRange&)
{
	PtrVec<Type> params(1, &elementTy);
	return find(rawMaybeType_->name(), params);
}

const FileType& TypeContext::fileType()
{
	static const FileType& f =
		dynamic_cast<const FileType&>(Register(FileType::Create(*this)));
	return f;
}

const FileType& TypeContext::inputFileType()
{
	static const Type& in = Register(Type::Create("in", PtrVec<Type>(), *this));
	static const FileType& ty = dynamic_cast<const FileType&>(
		find("file", PtrVec<Type>(1, &in)));

	return ty;
}

const FileType& TypeContext::outputFileType()
{
	static const Type& out = Register(Type::Create("out", PtrVec<Type>(), *this));
	static const FileType& ty = dynamic_cast<const FileType&>(
		find("file", PtrVec<Type>(1, &out)));

	return ty;
}

const Type& TypeContext::fileListType()
{
	static const Type& f = listOf(fileType(), SourceRange::None());
	return f;
}

const Type& TypeContext::stringType()
{
	static const Type& t = Register(new StringType(*this));
	return t;
}


const FunctionType&
TypeContext::functionType(const Type& in, const Type& out)
{
	return functionType(PtrVec<Type>(1, &in), out);
}

const FunctionType&
TypeContext::functionType(const PtrVec<Type>& argTypes, const Type& retType)
{
	FunctionType *t = FunctionType::Create(argTypes, retType);
	//Register(t);

	return *t;
}

const RecordType&
TypeContext::recordType(const Type::NamedTypeVec& fields)
{
	RecordType *t = RecordType::Create(fields, *this);
	//Register(t);

	return *t;
}

const Type&
TypeContext::typeType()
{
	static const Type& t = Register(new Type("type", PtrVec<Type>(), *this));
	return t;
}

const Type&
TypeContext::Register(Type *t)
{
	auto fullName(QualifiedName(t->name(), t->parameters_));
	assert(types.find(fullName) == types.end());

	types[fullName].reset(t);
	return *t;
}


TypeContext::TypeName
TypeContext::QualifiedName(const string& name, const PtrVec<Type>& params)
{
	return std::make_pair(name, params);
}
