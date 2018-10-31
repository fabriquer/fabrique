/** @file Types/FileType.cc    Definition of @ref fabrique::FileType. */
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

#include <fabrique/ast/Builtins.hh>
#include <fabrique/types/FileType.hh>
#include <fabrique/types/SequenceType.hh>
#include <fabrique/types/TypeContext.hh>
#include <fabrique/types/TypeError.hh>
#include "Support/exceptions.h"

#include <cassert>

using namespace fabrique;


static const char *InTagName = "in";
static const char *OutTagName = "out";


bool FileType::isInput(const Type& t)
{
	if (t.isFile())
		return dynamic_cast<const FileType&>(t).isInputFile();

	if (t.isOrdered())
		return isInput(dynamic_cast<const SequenceType&>(t).elementType());

	return false;
}

bool FileType::isOutput(const Type& t)
{
	if (t.isFile())
		return dynamic_cast<const FileType&>(t).isOutputFile();

	if (t.isOrdered())
		return isOutput(dynamic_cast<const SequenceType&>(t).elementType());

	return false;
}

bool FileType::isFileOrFiles(const Type& t)
{
	if (t.isOrdered())
		return isFileOrFiles(
			dynamic_cast<const SequenceType&>(t).elementType());

	return t.isFile();
}

void FileType::CheckFileTags(const Type& t, SourceRange src)
{
	if (t.isFile())
	{
		auto& file = dynamic_cast<const FileType&>(t);
		if (not file.isInputFile() and not file.isOutputFile())
			throw WrongTypeException("file[in|out]", file, src);
	}
	else if (t.isOrdered())
	{
		auto& seq = dynamic_cast<const SequenceType&>(t);
		CheckFileTags(seq.elementType(), src);
	}
}


FileType::TypeMap FileType::fields() const
{
	TypeContext& ctx = context();

	TypeMap map = {
		{ ast::builtins::Basename,      ctx.stringType() },
		{ ast::builtins::Extension,     ctx.stringType() },
		{ ast::builtins::FileName,      ctx.stringType() },
		{ ast::builtins::FullName,      ctx.stringType() },
		{ ast::builtins::Generated,     ctx.booleanType() },
		{ ast::builtins::Name,          ctx.stringType() },
		{ ast::builtins::Subdirectory,  ctx.fileType() },
	};

	for (auto& i : arguments_)
		map.emplace(i.first, i.second);

	return map;
}


FileType& FileType::WithArguments(const TypeMap& args) const
{
	FileType *withArgs = new FileType(tag_, typeParameters(), args, context());
	//context().Register(withArgs);

	return *withArgs;
}


FileType::FileType(Tag tag, const PtrVec<Type>& params, TypeMap args, TypeContext& ctx)
	: Type("file", params, ctx), tag_(tag), arguments_(std::move(args))
{
}


bool FileType::isInputFile() const
{
	return (tag_ == Tag::Input);
}


bool FileType::isOutputFile() const
{
	return (tag_ == Tag::Output);
}


bool FileType::isSubtype(const Type& candidateSupertype) const
{
	if (not candidateSupertype.isFile())
		return false;

	auto& t = dynamic_cast<const FileType&>(candidateSupertype);
	if (not t)
		return false;

	if (tag_ == t.tag_)
		return true;

	// Can always pass a file to a file[in] or file[out] parameter.
	if (tag_ == Tag::None)
		return true;

	return false;
}


const Type& FileType::onAddTo(const Type& t) const
{
	/// We can add strings to files, creating files with longer names.
	if (t.isString())
		return *this;

	/// We can also concatenate filenames together.
	if (t.isSubtype(*this))
		return *this;

	return context().nilType();
}


const Type& FileType::onPrefixWith(const Type& t) const
{
	/**!
	 * We can also prefix files with strings. This modifies the filename,
	 * but not the `subdir` or `{src|build}root` directories.
	 */
	if (t.isString())
		return *this;

	return context().nilType();
}


FileType* FileType::Create(TypeContext& ctx)
{
	return new FileType(Tag::None, PtrVec<Type>(), TypeMap(), ctx);
}


Type* FileType::Parameterise(const PtrVec<Type>& params, const SourceRange& src) const
{
	assert(tag_ == Tag::None);
	assert(params.size() == 1);

	const std::string name = params[0]->name();

	if (name == InTagName)
		return new FileType(Tag::Input, params, arguments_, context());

	else if (name == OutTagName)
		return new FileType(Tag::Output, params, arguments_, context());

	throw SemanticException("invalid file tag '" + name + "'", src);
}
