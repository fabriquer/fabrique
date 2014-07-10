/** @file Types/FileType.h    Declaration of @ref fabrique::FileType. */
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

#ifndef FILE_TYPE_H
#define FILE_TYPE_H

#include "Types/Type.h"

namespace fabrique {

class SourceRange;


/**
 * A type that represents an ordered sequence.
 */
class FileType : public Type
{
public:
	static bool isInput(const Type&);
	static bool isOutput(const Type&);
	static bool isFileOrFiles(const Type&);
	static void CheckFileTags(const Type&, SourceRange);

	virtual bool isSubtype(const Type&) const override;
	virtual bool isFile() const override { return true; }

	virtual bool isInputFile() const;
	virtual bool isOutputFile() const;

	virtual const Type& onAddTo(const Type&) const override;
	virtual const Type& onPrefixWith(const Type&) const override;

protected:
	virtual Type* Parameterise(
		const PtrVec<Type>&, const SourceRange&) const override;

private:
	enum class Tag
	{
		None,
		Input,
		Output,
		Invalid,
	};

	static FileType* Create(TypeContext&);

	FileType(Tag tag, const PtrVec<Type>&, TypeContext&);

	const Tag tag_;
	friend class TypeContext;
};

} // namespace fabrique

#endif

