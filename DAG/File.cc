/** @file DAG/File.cc    Definition of @ref fabrique::dag::File. */
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

#include "AST/Builtins.h"
#include "DAG/File.h"
#include "DAG/Primitive.h"
#include "DAG/Visitor.h"
#include "Support/Bytestream.h"
#include "Support/exceptions.h"
#include "Support/os.h"
#include "Types/FileType.h"
#include "Types/TypeContext.h"

#include <cassert>

using namespace fabrique::dag;
using std::shared_ptr;
using std::string;


File* File::Create(string fullPath, ValueMap attrs, const FileType& t,
                   SourceRange src, bool generated)
{
	const string filename(FilenameComponent(fullPath));
	const string subdir(DirectoryOf(fullPath));

	return Create(subdir, filename, attrs, t, src, generated);
}

File* File::Create(string dir, string path, ValueMap attrs,
                   const FileType& type, SourceRange src, bool generated)
{
	const string filename(FilenameComponent(path));
	const string subdir(DirectoryOf(path));
	const string directory(
		PathIsAbsolute(path)
			? subdir
			: JoinPath(dir, subdir));

	auto i = attrs.find("generated");
	if (i != attrs.end())
	{
		ValuePtr gen = i->second;
		const Type& t = gen->type();
		t.CheckSubtype(t.context().booleanType(), gen->source());

		auto b = std::dynamic_pointer_cast<Boolean>(gen);
		assert(b);
		generated = b->value();

		attrs.erase(i);
	}

	return new File(filename, directory, PathIsAbsolute(directory), attrs, type,
	                src, generated);
}

bool File::Equals(const shared_ptr<File>& x, const shared_ptr<File>& y)
{
	return (x->fullName() == y->fullName());
}

bool File::LessThan(const shared_ptr<File>& x, const shared_ptr<File>& y)
{
	return (x->fullName() < y->fullName());
}


File::File(string filename, string subdirectory, bool absolute,
           const ValueMap& attributes, const FileType& t, SourceRange source,
	   bool generated)
	: Value(t, source), filename_(filename), subdirectory_(subdirectory),
	  absolute_(absolute), generated_(generated), attributes_(attributes)
{
}


string File::filename() const
{
	return JoinPath(subdirectory_, filename_);
}


string File::directory(bool relativeBuildDirectories) const
{
	if (absolute_)
		return subdirectory_;

	if (relativeBuildDirectories and generated())
		return subdirectory_;

	const string root = generated() ? "${buildroot}" : "${srcroot}";

	if (subdirectory_.empty())
		return root;

	return JoinPath(root, subdirectory_);
}


void File::appendSubdirectory(string subdir)
{
	subdirectory_ = JoinPath(subdirectory_, subdir);
}


string File::relativeName() const
{
	return JoinPath(subdirectory_, filename_);
}


string File::fullName() const
{
	return JoinPath(directory(), filename_);
}


void File::setGenerated(bool gen)
{
	if (absolute_ and gen)
		throw SemanticException(
			"cannot generate a file with absolute path '"
			+ relativeName() + "'", source());

	generated_ = gen;
}



ValuePtr File::field(const string& name) const
{
	TypeContext& ctx = type().context();
	ValuePtr val;

	if (name == ast::builtins::Basename)
		val.reset(new String(BaseName(filename_), ctx.stringType(), source()));

	else if (name == ast::builtins::Extension)
		val.reset(new String(FileExtension(filename_), ctx.stringType(),
		                     source()));

	else if (name == ast::builtins::FileName)
		val.reset(new String(FilenameComponent(filename_), ctx.stringType(),
		                     source()));

	else if (name == ast::builtins::FullName)
		val.reset(new String(fullName(), ctx.stringType(), source()));

	else if (name == ast::builtins::Generated)
		val.reset(new Boolean(generated_, ctx.booleanType(), source()));

	else if (name == ast::builtins::Name)
		val.reset(new String(relativeName(), ctx.stringType(), source()));

	else if (name == ast::builtins::Subdirectory)
		val.reset(File::Create(subdirectory(), ValueMap(), ctx.fileType(),
		                       source()));

	else
	{
		auto i = attributes_.find(name);
		if (i != attributes_.end())
			val = i->second;
	}

	return val;
}


ValuePtr File::Add(ValuePtr& suffix) const
{
	const string file = filename_ + suffix->str();
	const string filename = FilenameComponent(file);
	const string subdir = JoinPath(subdirectory_, DirectoryOf(file));

	shared_ptr<File> f(
		new File(filename, subdir, absolute_,
		         attributes_, type(), SourceRange::Over(this, suffix),
		         generated_));

	return f;
}


ValuePtr File::PrefixWith(ValuePtr& prefix) const
{
	shared_ptr<File> f(
		new File(prefix->str() + filename_, subdirectory_, absolute_,
		         attributes_, type(), SourceRange::Over(prefix, this),
		         generated_));

	return f;
}


const fabrique::FileType& File::type() const
{
	return dynamic_cast<const FileType&>(Value::type());
}


void File::PrettyPrint(Bytestream& out, unsigned int /*indent*/) const
{
	if (not subdirectory_.empty())
		out
			<< Bytestream::Literal << subdirectory_
			<< Bytestream::Operator << "/"
			;

	out << Bytestream::Filename << filename_ << Bytestream::Reset;
}


void File::Accept(Visitor& v) const
{
	v.Visit(*this);
}
