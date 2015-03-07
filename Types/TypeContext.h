/** @file TypeContext.h    Declaration of fabrique::TypeContext. */
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

#ifndef TYPE_CONTEXT_H
#define TYPE_CONTEXT_H

#include "ADT/PtrVec.h"
#include "Types/Type.h"
#include "Types/StructureType.h"

#include <map>
#include <memory>
#include <string>

namespace fabrique {

class FileType;
class FunctionType;
class SourceRange;
class Type;


/**
 * A context object that holds state for a compilation (e.g., type objects).
 */
class TypeContext
{
public:
	TypeContext();

	//! Find an existing type (nil type if not found).
	const Type& find(const std::string& name, const SourceRange& src,
	                 const PtrVec<Type>& params = PtrVec<Type>());

	//! The type of a typeless thing.
	const Type& nilType();

	//! The type of a boolean expression.
	const Type& booleanType();

	//! The type of an integer number.
	const Type& integerType();

	//! The type of a list.
	const Type& listOf(const Type&, const SourceRange&);

	//! An optional ("maybe") type.
	const Type& maybe(const Type&, const SourceRange&);

	//! A file in a build.
	const FileType& fileType();
	const FileType& inputFileType();
	const FileType& outputFileType();

	//! A list of files (a pretty fundamental type!).
	const Type& fileListType();

	//! A function type for a simple (one in, one out) function.
	const FunctionType& functionType(const Type& in, const Type& out);

	//! A function type, which incorporates the function's signature.
	const FunctionType& functionType(const PtrVec<Type>& argumentTypes,
	                                 const Type& returnType);

	//! A structure type describes its fields' names and types.
	const StructureType& structureType(const Type::NamedTypeVec&);

	//! A string of characters.
	const Type& stringType();


	/**
	 * Find the supertype of a range of elements (or nil).
	 */
	template<class Iterator>
	const Type& supertype(Iterator begin, Iterator end)
	{
		if (begin == end)
			return nilType();

		const Type& current = (*begin)->type();
		return current.supertype(supertype(begin + 1, end));
	}

private:
	typedef std::pair<std::string,PtrVec<Type> > TypeName;

	const Type& Register(Type*);
	static TypeName QualifiedName(const std::string& name,
	                              const PtrVec<Type>& params = PtrVec<Type>());

	Type* rawMaybeType_;
	Type* rawSequenceType_;
	std::map<TypeName,std::unique_ptr<Type>> types;
};

} // namespace fabrique

#endif
